#include <stdint.h>
#include "py/builtin.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/objtuple.h"
#include "py/mphal.h"
#include "py/mpthread.h"
#include "vex/vex.h"

#define MP_VEX_CALL_BLOCKING(wait, ...) \
    do { \
        if (wait) { \
            nlr_buf_t nlr; \
            if (nlr_push(&nlr) == 0) { \
                MP_THREAD_GIL_EXIT(); \
                __VA_ARGS__; \
                MP_THREAD_GIL_ENTER(); \
                nlr_pop(); \
            } else { \
                MP_THREAD_GIL_ENTER(); \
                nlr_raise(nlr.ret_val); \
            } \
        } else { \
            __VA_ARGS__; \
        } \
    } while (0)

static mp_obj_t mp_vex_wait(size_t n_args, const mp_obj_t *args) {
    mp_arg_check_num(n_args, 0, 1, 2, false);
    double duration = mp_obj_get_float(args[0]);
    if (duration < 0.0) {
        duration = 1.0; // Clamp negative loop remainder to 1 ms
    }
    int units = n_args == 2 ? mp_obj_get_int(args[1]) : VEX_TIME_UNITS_SECONDS;
    MP_THREAD_GIL_EXIT();
    bool valid = vex_logic_wait(duration, units);
    MP_THREAD_GIL_ENTER();
    if (!valid) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid wait duration or time units"));
    }
    return mp_const_none;
}
    
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_vex_wait_obj, 1, 2, mp_vex_wait);

MP_REGISTER_ROOT_POINTER(mp_obj_t vex_callback_roots);
MP_REGISTER_ROOT_POINTER(mp_obj_t vex_competition_callbacks);
MP_REGISTER_ROOT_POINTER(mp_obj_t vex_pending_competition_event);

typedef struct _mp_vex_thread_obj_t{
    mp_obj_base_t base;
    mp_uint_t thread_id;
    bool stopped;
    bool done;
} mp_vex_thread_obj_t;

mp_obj_t mp_vex_callback_executor(mp_obj_t callback_data) {
    size_t count;
    mp_obj_t *items;
    mp_obj_get_array(callback_data, &count, &items);
    mp_vex_thread_obj_t *thread = NULL;
    if (mp_obj_is_true(items[3])) {
        mp_obj_list_remove(MP_STATE_VM(vex_callback_roots), callback_data);
    }
    if (count > 4) {
        thread = MP_OBJ_TO_PTR(items[4]);
        if (thread->stopped) {
            thread->done = true;
            return mp_const_none;
        }
    }

    nlr_buf_t nlr;
    mp_obj_t result;
    if (nlr_push(&nlr) == 0) {
        if (mp_obj_is_true(items[2])) {
            result = mp_call_function_0(items[0]);
        } else if (mp_obj_is_type(items[1], &mp_type_tuple) || mp_obj_is_type(items[1], &mp_type_list)) {
            size_t arg_count;
            mp_obj_t *callback_args;
            mp_obj_get_array(items[1], &arg_count, &callback_args);
            result = mp_call_function_n_kw(items[0], arg_count, 0, callback_args);
        } else {
            result = mp_call_function_1(items[0], items[1]);
        }
        nlr_pop();
    } else {
        if (thread != NULL) {
            thread->done = true;
        }
        nlr_raise(nlr.ret_val);
    }

    if (thread != NULL) {
        thread->done = true;
    }
    return result;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_callback_executor_obj, mp_vex_callback_executor);

static void mp_vex_callback_trampoline(void *callback_data) {
    while (!mp_sched_schedule(MP_OBJ_FROM_PTR(&mp_vex_callback_executor_obj), MP_OBJ_FROM_PTR(callback_data))) {
        mp_hal_delay_ms(1);
    }
}

static void *mp_vex_thread_entry(void *thread_data) {
    mp_obj_t callback_data = MP_OBJ_FROM_PTR(thread_data);
    size_t count;
    mp_obj_t *items;
    mp_obj_get_array(callback_data, &count, &items);
    mp_vex_thread_obj_t *thread = MP_OBJ_TO_PTR(items[4]);

    mp_state_thread_t thread_state;
    // Pass real stack size (48 KB) instead of 0!
    mp_thread_init_state(&thread_state, 0x3000 * sizeof(uint32_t), MP_OBJ_TO_PTR(items[5]), MP_OBJ_TO_PTR(items[6]));

    #if MICROPY_ENABLE_PYSTACK
    mp_obj_t mini_pystack[128];
    mp_pystack_init(mini_pystack, &mini_pystack[128]);
    #endif

    MP_THREAD_GIL_ENTER();
    mp_thread_start();
    if (!thread->stopped) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            if (mp_obj_is_true(items[2])) {
                mp_call_function_0(items[0]);
            } else if (mp_obj_is_type(items[1], &mp_type_tuple) || mp_obj_is_type(items[1], &mp_type_list)) {
                size_t arg_count;
                mp_obj_t *callback_args;
                mp_obj_get_array(items[1], &arg_count, &callback_args);
                mp_call_function_n_kw(items[0], arg_count, 0, callback_args);
            } else {
                mp_call_function_1(items[0], items[1]);
            }
            nlr_pop();
        } else {
            mp_printf(&mp_plat_print, "Unhandled exception in vex.Thread callback:\n");
            mp_obj_print_exception(&mp_plat_print, nlr.ret_val);
        }
    }

    thread->done = true;
    mp_obj_list_remove(MP_STATE_VM(vex_callback_roots), callback_data);
    mp_thread_finish();
    MP_THREAD_GIL_EXIT();
    return NULL;
}

static mp_obj_t mp_vex_callback_data(mp_obj_t callback, mp_obj_t args, bool no_args, bool one_shot) {
    if (!mp_obj_is_callable(callback)) {
        mp_raise_TypeError("callback must be callable");
    }

    if (MP_STATE_VM(vex_callback_roots) == MP_OBJ_NULL) {
        MP_STATE_VM(vex_callback_roots) = mp_obj_new_list(0, NULL);
    }

    mp_obj_t items[] = {callback, args, mp_obj_new_bool(no_args), mp_obj_new_bool(one_shot)};
    mp_obj_t callback_data = mp_obj_new_tuple(4, items);
    mp_obj_list_append(MP_STATE_VM(vex_callback_roots), callback_data);
    return callback_data;
}

static mp_obj_t mp_vex_competition_dispatch(mp_obj_t event_obj) {
    int event = mp_obj_get_int(event_obj);
    if (MP_STATE_VM(vex_competition_callbacks) == MP_OBJ_NULL) {
        MP_STATE_VM(vex_pending_competition_event) = event_obj;
        return mp_const_none;
    }

    size_t callback_index;
    switch (event) {
        case VEX_COMPETITION_DRIVER_CONTROL:
            callback_index = 0;
            break;
        case VEX_COMPETITION_AUTONOMOUS:
            callback_index = 1;
            break;
        case VEX_COMPETITION_PRE_AUTON:
            callback_index = 2;
            break;
        case VEX_COMPETITION_DISABLED:
            callback_index = 3;
            break;
        default:
            return mp_const_none;
    }

    mp_obj_tuple_t *callbacks = MP_OBJ_TO_PTR(MP_STATE_VM(vex_competition_callbacks));
    mp_obj_t callback = callbacks->items[callback_index];
    if (callback == mp_const_none) {
        MP_STATE_VM(vex_pending_competition_event) = event_obj;
        return mp_const_none;
    }

    mp_obj_t callback_data = mp_vex_callback_data(callback, mp_const_none, true, true);
    while (!mp_sched_schedule(MP_OBJ_FROM_PTR(&mp_vex_callback_executor_obj), callback_data)) {
        mp_hal_delay_ms(1);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_competition_dispatch_obj, mp_vex_competition_dispatch);

void vex_competition_callback(int event) {
    mp_obj_t event_obj = MP_OBJ_NEW_SMALL_INT(event);
    while (!mp_sched_schedule(MP_OBJ_FROM_PTR(&mp_vex_competition_dispatch_obj), event_obj)) {
        mp_hal_delay_ms(1);
    }
}

typedef struct _mp_vex_competition_obj_t {
    mp_obj_base_t base;
} mp_vex_competition_obj_t;

static void mp_vex_competition_set_callback(size_t index, mp_obj_t callback) {
    if (callback != mp_const_none && !mp_obj_is_callable(callback)) {
        mp_raise_TypeError(MP_ERROR_TEXT("competition callback must be callable or None"));
    }

    mp_obj_t callbacks[4] = {mp_const_none, mp_const_none, mp_const_none, mp_const_none};
    if (MP_STATE_VM(vex_competition_callbacks) != MP_OBJ_NULL) {
        mp_obj_tuple_t *current = MP_OBJ_TO_PTR(MP_STATE_VM(vex_competition_callbacks));
        for (size_t i = 0; i < MP_ARRAY_SIZE(callbacks); ++i) {
            callbacks[i] = current->items[i];
        }
    }
    callbacks[index] = callback;
    MP_STATE_VM(vex_competition_callbacks) = mp_obj_new_tuple(MP_ARRAY_SIZE(callbacks), callbacks);

    if (MP_STATE_VM(vex_pending_competition_event) != MP_OBJ_NULL) {
        mp_obj_t pending_event = MP_STATE_VM(vex_pending_competition_event);
        MP_STATE_VM(vex_pending_competition_event) = MP_OBJ_NULL;
        mp_vex_competition_dispatch(pending_event);
    }
}

static mp_obj_t mp_vex_competition_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 2, false);
    mp_vex_competition_obj_t *self = mp_obj_malloc(mp_vex_competition_obj_t, type);

    mp_obj_t callbacks[4] = {mp_const_none, mp_const_none, mp_const_none, mp_const_none};
    if (n_args > 0) {
        callbacks[0] = args[0];
    }
    if (n_args > 1) {
        callbacks[1] = args[1];
    }
    for (size_t i = 0; i < 2; ++i) {
        if (callbacks[i] != mp_const_none && !mp_obj_is_callable(callbacks[i])) {
            mp_raise_TypeError(MP_ERROR_TEXT("competition callback must be callable or None"));
        }
    }
    MP_STATE_VM(vex_competition_callbacks) = mp_obj_new_tuple(MP_ARRAY_SIZE(callbacks), callbacks);

    if (MP_STATE_VM(vex_pending_competition_event) != MP_OBJ_NULL) {
        mp_obj_t pending_event = MP_STATE_VM(vex_pending_competition_event);
        MP_STATE_VM(vex_pending_competition_event) = MP_OBJ_NULL;
        mp_vex_competition_dispatch(pending_event);
    }
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_vex_competition_autonomous(mp_obj_t self_in, mp_obj_t callback) {
    (void)self_in;
    mp_vex_competition_set_callback(1, callback);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_vex_competition_autonomous_obj, mp_vex_competition_autonomous);

static mp_obj_t mp_vex_competition_drivercontrol(mp_obj_t self_in, mp_obj_t callback) {
    (void)self_in;
    mp_vex_competition_set_callback(0, callback);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_vex_competition_drivercontrol_obj, mp_vex_competition_drivercontrol);

static mp_obj_t mp_vex_competition_pre_auton(mp_obj_t self_in, mp_obj_t callback) {
    (void)self_in;
    mp_vex_competition_set_callback(2, callback);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_vex_competition_pre_auton_obj, mp_vex_competition_pre_auton);

static mp_obj_t mp_vex_competition_disabled(mp_obj_t self_in, mp_obj_t callback) {
    (void)self_in;
    mp_vex_competition_set_callback(3, callback);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_vex_competition_disabled_obj, mp_vex_competition_disabled);

static mp_obj_t mp_vex_competition_is_autonomous(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_bool(vex_competition_is_autonomous());
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_competition_is_autonomous_obj, mp_vex_competition_is_autonomous);

static mp_obj_t mp_vex_competition_is_driver_control(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_bool(vex_competition_is_driver_control());
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_competition_is_driver_control_obj, mp_vex_competition_is_driver_control);

static mp_obj_t mp_vex_competition_is_enabled(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_bool(vex_competition_is_enabled());
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_competition_is_enabled_obj, mp_vex_competition_is_enabled);

static mp_obj_t mp_vex_competition_is_disabled(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_bool(vex_competition_is_disabled());
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_competition_is_disabled_obj, mp_vex_competition_is_disabled);

static mp_obj_t mp_vex_competition_is_competition_switch(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_bool(vex_competition_is_competition_switch());
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_competition_is_competition_switch_obj, mp_vex_competition_is_competition_switch);

static mp_obj_t mp_vex_competition_is_field_control(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_bool(vex_competition_is_field_control());
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_competition_is_field_control_obj, mp_vex_competition_is_field_control);

static const mp_rom_map_elem_t mp_vex_competition_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_autonomous), MP_ROM_PTR(&mp_vex_competition_autonomous_obj) },
    { MP_ROM_QSTR(MP_QSTR_drivercontrol), MP_ROM_PTR(&mp_vex_competition_drivercontrol_obj) },
    { MP_ROM_QSTR(MP_QSTR_driver_control), MP_ROM_PTR(&mp_vex_competition_drivercontrol_obj) },
    { MP_ROM_QSTR(MP_QSTR_pre_auton), MP_ROM_PTR(&mp_vex_competition_pre_auton_obj) },
    { MP_ROM_QSTR(MP_QSTR_disabled), MP_ROM_PTR(&mp_vex_competition_disabled_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_autonomous), MP_ROM_PTR(&mp_vex_competition_is_autonomous_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_driver_control), MP_ROM_PTR(&mp_vex_competition_is_driver_control_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_enabled), MP_ROM_PTR(&mp_vex_competition_is_enabled_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_disabled), MP_ROM_PTR(&mp_vex_competition_is_disabled_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_competition_switch), MP_ROM_PTR(&mp_vex_competition_is_competition_switch_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_field_control), MP_ROM_PTR(&mp_vex_competition_is_field_control_obj) },
};
static MP_DEFINE_CONST_DICT(mp_vex_competition_locals_dict, mp_vex_competition_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(
    mp_vex_competition_type,
    MP_QSTR_competition,
    MP_TYPE_FLAG_NONE,
    make_new, mp_vex_competition_make_new,
    locals_dict, &mp_vex_competition_locals_dict
);

typedef struct _mp_motor_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_motor_obj_t;

typedef struct _mp_motorgroup_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_motorgroup_obj_t;

typedef struct _mp_drivetrain_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_drivetrain_obj_t;

typedef struct _mp_message_link_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_message_link_obj_t;

typedef struct _mp_serial_link_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_serial_link_obj_t;

typedef struct _mp_rotation_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_rotation_obj_t;

typedef struct _mp_inertial_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_inertial_obj_t;

typedef struct _mp_optical_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_optical_obj_t;

typedef struct _mp_distance_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_distance_obj_t;

typedef struct _mp_gps_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_gps_obj_t;

typedef struct _mp_digital_out_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_digital_out_obj_t;

typedef struct _mp_three_wire_device_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_three_wire_device_obj_t;

typedef struct _mp_three_wire_port_obj_t{
    mp_obj_base_t base;
    int port;
} mp_three_wire_port_obj_t;

typedef struct _mp_three_wire_ports_obj_t{
    mp_obj_base_t base;
    mp_obj_t ports[8];
} mp_three_wire_ports_obj_t;

typedef struct _mp_timer_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_timer_obj_t;

typedef struct _mp_brain_battery_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_brain_battery_obj_t;

typedef struct _mp_brain_screen_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_brain_screen_obj_t;

typedef struct _mp_brain_sdcard_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_brain_sdcard_obj_t;

typedef struct _mp_brain_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
    mp_obj_t screen;  // Stores instance of mp_brain_screen_obj_t
    mp_obj_t sdcard;  // Stores instance of mp_brain_sdcard_obj_t
    mp_obj_t battery; // Stores instance of mp_brain_battery_obj_t
    mp_obj_t three_wire_port;
} mp_brain_obj_t;

typedef struct _mp_controller_screen_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_controller_screen_obj_t;

typedef struct _mp_controller_button_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_controller_button_obj_t;

typedef struct _mp_controller_axis_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_controller_axis_obj_t;

typedef struct _mp_controller_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
    mp_obj_t screen;
    mp_obj_t buttonA;
    mp_obj_t buttonB;
    mp_obj_t buttonX;
    mp_obj_t buttonY;
    mp_obj_t buttonUp;
    mp_obj_t buttonDown;
    mp_obj_t buttonLeft;
    mp_obj_t buttonRight;
    mp_obj_t buttonR1;
    mp_obj_t buttonR2;
    mp_obj_t buttonL1;
    mp_obj_t buttonL2;
    mp_obj_t axis1;
    mp_obj_t axis2;
    mp_obj_t axis3;
    mp_obj_t axis4;
} mp_controller_obj_t;

typedef struct _mp_color_obj_t{
    mp_obj_base_t base;
    uint32_t value;
} mp_color_obj_t;

typedef struct _mp_vex_event_obj_t {
    mp_obj_base_t base;
    mp_obj_t handlers; // Python list of (callback, args, no_args) tuples
} mp_vex_event_obj_t;

extern const mp_obj_type_t mp_motor_type;
extern const mp_obj_type_t mp_motorgroup_type;
extern const mp_obj_type_t mp_rotation_type;
extern const mp_obj_type_t mp_inertial_type;
extern const mp_obj_type_t mp_optical_type;
extern const mp_obj_type_t mp_distance_type;
extern const mp_obj_type_t mp_digital_out_type;
extern const mp_obj_type_t mp_analog_in_type;
extern const mp_obj_type_t mp_bumper_type;
extern const mp_obj_type_t mp_digital_in_type;
extern const mp_obj_type_t mp_encoder_type;
extern const mp_obj_type_t mp_gyro_type;
extern const mp_obj_type_t mp_led_type;
extern const mp_obj_type_t mp_light_type;
extern const mp_obj_type_t mp_limit_type;
extern const mp_obj_type_t mp_line_type;
extern const mp_obj_type_t mp_object_sensor_type;
extern const mp_obj_type_t mp_potentiometer_type;
extern const mp_obj_type_t mp_potentiometer_v2_type;
extern const mp_obj_type_t mp_pwm_type;
extern const mp_obj_type_t mp_sonar_type;
extern const mp_obj_type_t mp_accelerometer_type;
extern const mp_obj_type_t mp_addressable_led_type;
extern const mp_obj_type_t mp_three_wire_port_type;
extern const mp_obj_type_t mp_three_wire_ports_type;
extern const mp_obj_type_t mp_timer_type;
extern const mp_obj_type_t mp_brain_type;
extern const mp_obj_type_t mp_brain_screen_type;
extern const mp_obj_type_t mp_brain_sdcard_type;
extern const mp_obj_type_t mp_brain_battery_type;
extern const mp_obj_type_t mp_controller_screen_type;
extern const mp_obj_type_t mp_controller_button_type;
extern const mp_obj_type_t mp_controller_axis_type;
extern const mp_obj_type_t mp_controller_type;
extern const mp_obj_type_t mp_color_type;

static mp_obj_t mp_color_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 3, 3, false);

    int red = mp_obj_get_int(args[0]);
    int green = mp_obj_get_int(args[1]);
    int blue = mp_obj_get_int(args[2]);
    if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255) {
        mp_raise_ValueError(MP_ERROR_TEXT("RGB values must be between 0 and 255"));
    }

    mp_color_obj_t *self = mp_obj_malloc(mp_color_obj_t, type);
    self->value = ((uint32_t)red << 16) | ((uint32_t)green << 8) | (uint32_t)blue;
    return MP_OBJ_FROM_PTR(self);
}

static int mp_color_get_value(mp_obj_t color_obj) {
    if (mp_obj_is_type(color_obj, &mp_color_type)) {
        return (int)((mp_color_obj_t *)MP_OBJ_TO_PTR(color_obj))->value;
    }
    return mp_obj_get_int(color_obj);
}

static mp_obj_t mp_motor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 3, false); // strict 3 arguments
    int port = mp_obj_get_int(args[0]);
    int gears = (n_args > 1) ? mp_obj_get_int(args[1])   : 18;
    bool reversed = (n_args > 2) ? mp_obj_is_true(args[2])   : false;

    mp_motor_obj_t *self = mp_obj_malloc(mp_motor_obj_t, type);
    self->cpp_handle = motor_create(port, gears, reversed);
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_motor_spin(size_t n_args, const mp_obj_t *args) {
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int direction = mp_obj_get_int(args[1]);
    float velocity = (n_args > 2) ? mp_obj_get_float(args[2]) : 601.0f;
    int units = (n_args > 3) ? mp_obj_get_int(args[3]) : 24;  // Default is RPM.

    motor_spin(self->cpp_handle, direction, velocity, units);
    return mp_const_none;
}
// obj, requred args, total args, function.
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_spin_obj, 1, 4, mp_motor_spin);

static mp_obj_t mp_motor_spin_for(size_t n_args, const mp_obj_t *args) {
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    int direction = mp_obj_get_int(args[1]);
    int angle = mp_obj_get_int(args[2]);

    // Defining default values.
    int units = (n_args > 3) ? mp_obj_get_int(args[3])   : 29;      // DEGREES macro value
    float velocity = (n_args > 4) ? mp_obj_get_float(args[4]) : 601.0f;
    int units_v = (n_args > 5) ? mp_obj_get_int(args[5])   : 24;      // RPM macro value
    bool wait = (n_args > 6) ? mp_obj_is_true(args[6])   : true;

    MP_VEX_CALL_BLOCKING(wait, motor_spin_for(self -> cpp_handle, direction, angle, units, velocity, units_v, wait));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_spin_for_obj, 3, 7, mp_motor_spin_for);

static mp_obj_t mp_motor_spin_to_position(size_t n_args, const mp_obj_t *args){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int rotation = mp_obj_get_int(args[1]);

    int units = (n_args > 2) ? mp_obj_get_int(args[2])   : 29;
    float velocity = (n_args > 3) ? mp_obj_get_float(args[3]) : 601.0f;
    int units_v = (n_args > 4) ? mp_obj_get_int(args[4])   : 24;
    bool wait = (n_args > 5) ? mp_obj_is_true(args[5])   : true;

    MP_VEX_CALL_BLOCKING(wait, motor_spin_to_position(self -> cpp_handle, rotation, units, velocity, units_v, wait));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_spin_to_position_obj, 2, 6, mp_motor_spin_to_position);

static mp_obj_t mp_motor_stop(size_t n_args, const mp_obj_t *args){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    int mode = (n_args > 1) ? mp_obj_get_int(args[1])   : -1; // NALL macro value

    motor_stop(self -> cpp_handle, mode);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_stop_obj, 1, 2, mp_motor_stop);

static mp_obj_t mp_motor_set_position(mp_obj_t self_ptr, mp_obj_t position_t, mp_obj_t units_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    float position = mp_obj_get_float(position_t);
    int units = mp_obj_get_int(units_t);

    motor_set_position(self -> cpp_handle, position, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_motor_set_position_obj, mp_motor_set_position);

static mp_obj_t mp_motor_set_velosity(mp_obj_t self_ptr, mp_obj_t velosity_t, mp_obj_t units_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    float velosity = mp_obj_get_float(velosity_t);
    int units = mp_obj_get_int(units_t);

    motor_set_velocity(self -> cpp_handle, velosity, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_motor_set_velosity_obj, mp_motor_set_velosity);

static mp_obj_t mp_motor_set_stopping(mp_obj_t self_ptr, mp_obj_t mode_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int mode = mp_obj_get_int(mode_t);

    motor_set_stopping(self -> cpp_handle, mode);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_set_stopping_obj, mp_motor_set_stopping);

static mp_obj_t mp_motor_set_max_torque(mp_obj_t self_ptr, mp_obj_t torque_t, mp_obj_t units_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    float torque = mp_obj_get_float(torque_t);
    int units = mp_obj_get_int(units_t);

    motor_set_max_torque(self -> cpp_handle, torque, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_motor_set_max_torque_obj, mp_motor_set_max_torque);

static mp_obj_t mp_motor_set_timeout(mp_obj_t self_ptr, mp_obj_t value_t, mp_obj_t units_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    float value = mp_obj_get_float(value_t);
    int units = mp_obj_get_int(units_t);

    motor_set_timeout(self -> cpp_handle, value, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_motor_set_timeout_obj, mp_motor_set_timeout);

static mp_obj_t mp_motor_set_reversed(mp_obj_t self_ptr, mp_obj_t value_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    bool value = mp_obj_is_true(value_t);

    vex_motor_set_reversed(self -> cpp_handle, value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_set_reversed_obj, mp_motor_set_reversed);

static mp_obj_t mp_motor_reset_position(mp_obj_t self_ptr){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    motor_reset_position(self -> cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_motor_reset_position_obj, mp_motor_reset_position);

static mp_obj_t mp_motor_is_done(mp_obj_t self_ptr){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    bool output;
    output = motor_is_done(self -> cpp_handle);
    return mp_obj_new_bool(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_motor_is_done_obj, mp_motor_is_done);

static mp_obj_t mp_motor_is_spinning(mp_obj_t self_ptr){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    bool output;
    output = motor_is_spinning(self -> cpp_handle);
    return mp_obj_new_bool(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_motor_is_spinning_obj, mp_motor_is_spinning);

static mp_obj_t mp_motor_position(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_position(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_position_obj, mp_motor_position);

static mp_obj_t mp_motor_velocity(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_velocity(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_velocity_obj, mp_motor_velocity);

static mp_obj_t mp_motor_current(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_current(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_current_obj, mp_motor_current);

static mp_obj_t mp_motor_power(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_power(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_power_obj, mp_motor_power);

static mp_obj_t mp_motor_torque(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_torque(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_torque_obj, mp_motor_torque);

static mp_obj_t mp_motor_efficiency(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_efficiency(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_efficiency_obj, mp_motor_efficiency);

static mp_obj_t mp_motor_temperature(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_temperature(self -> cpp_handle, units);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_temperature_obj, mp_motor_temperature);

static mp_obj_t mp_motor_get_timeout(mp_obj_t self_ptr){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int output;
    output = motor_get_timeout(self -> cpp_handle);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_motor_get_timeout_obj, mp_motor_get_timeout);

static mp_obj_t mp_motor_installed(mp_obj_t self_ptr){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return motor_installed(self->cpp_handle) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_motor_installed_obj, mp_motor_installed);

static const mp_rom_map_elem_t mp_motor_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_PythonMethodName), MP_ROM_PTR(&C_Function_Object) }
    { MP_ROM_QSTR(MP_QSTR_spin), MP_ROM_PTR(&mp_motor_spin_obj) },
    { MP_ROM_QSTR(MP_QSTR_spin_for), MP_ROM_PTR(&mp_motor_spin_for_obj) },
    { MP_ROM_QSTR(MP_QSTR_spin_to_position), MP_ROM_PTR(&mp_motor_spin_to_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&mp_motor_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_position), MP_ROM_PTR(&mp_motor_set_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_velocity), MP_ROM_PTR(&mp_motor_set_velosity_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_max_stopping), MP_ROM_PTR(&mp_motor_set_stopping_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_stopping), MP_ROM_PTR(&mp_motor_set_stopping_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_max_torque), MP_ROM_PTR(&mp_motor_set_max_torque_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_timeout), MP_ROM_PTR(&mp_motor_set_timeout_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_reversed), MP_ROM_PTR(&mp_motor_set_reversed_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_position), MP_ROM_PTR(&mp_motor_reset_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_done), MP_ROM_PTR(&mp_motor_is_done_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_spinning), MP_ROM_PTR(&mp_motor_is_spinning_obj) },
    { MP_ROM_QSTR(MP_QSTR_position), MP_ROM_PTR(&mp_motor_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_velocity), MP_ROM_PTR(&mp_motor_velocity_obj) },
    { MP_ROM_QSTR(MP_QSTR_current), MP_ROM_PTR(&mp_motor_current_obj) },
    { MP_ROM_QSTR(MP_QSTR_power), MP_ROM_PTR(&mp_motor_power_obj) },
    { MP_ROM_QSTR(MP_QSTR_torque), MP_ROM_PTR(&mp_motor_torque_obj) },
    { MP_ROM_QSTR(MP_QSTR_efficiency), MP_ROM_PTR(&mp_motor_efficiency_obj) },
    { MP_ROM_QSTR(MP_QSTR_temperature), MP_ROM_PTR(&mp_motor_temperature_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_timeout), MP_ROM_PTR(&mp_motor_get_timeout_obj) },
    { MP_ROM_QSTR(MP_QSTR_installed), MP_ROM_PTR(&mp_motor_installed_obj) },
};
static MP_DEFINE_CONST_DICT(mp_motor_locals_dict, mp_motor_locals_dict_table);

// Register the finalized Motor class metadata type
MP_DEFINE_CONST_OBJ_TYPE(
    mp_motor_type,
    MP_QSTR_motor,
    MP_TYPE_FLAG_NONE,
    make_new, mp_motor_make_new,    // Constructor attachment
    locals_dict, &mp_motor_locals_dict
);


static mp_obj_t mp_motor_group_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, false);

    size_t count = n_args;
    const mp_obj_t *items = args;

    // Also allow a single list/tuple for backwards compatibility if passed
    if (n_args == 1 && (mp_obj_is_type(args[0], &mp_type_tuple) || mp_obj_is_type(args[0], &mp_type_list))) {
        mp_obj_t *seq_items;
        mp_obj_get_array(args[0], &count, &seq_items);
        items = seq_items;
    }

    if (count == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("MotorGroup requires at least one Motor"));
    }

    void *motor_pointers[count];
    for (size_t i = 0; i < count; i++) {
        if (!mp_obj_is_type(items[i], &mp_motor_type)) {
            mp_raise_TypeError(MP_ERROR_TEXT("MotorGroup arguments must be Motor objects"));
        }
        mp_motor_obj_t *motor_obj = MP_OBJ_TO_PTR(items[i]);
        motor_pointers[i] = motor_obj->cpp_handle;
    }

    mp_motorgroup_obj_t *self = mp_obj_malloc(mp_motorgroup_obj_t, type);
    self->cpp_handle = motor_group_create(motor_pointers, (int)count);
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_motor_group_spin(size_t n_args, const mp_obj_t *args) {
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int direction = mp_obj_get_int(args[1]);
    float velocity = mp_obj_get_float(args[2]);
    int units = mp_obj_get_int(args[3]);

    motor_group_spin(self->cpp_handle, direction, velocity, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_group_spin_obj, 4, 4, mp_motor_group_spin);

static mp_obj_t mp_motor_group_spin_for(size_t n_args, const mp_obj_t *args) {
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    int direction = mp_obj_get_int(args[1]);
    int angle = mp_obj_get_int(args[2]);

    // Defining default values.
    int units = (n_args > 3) ? mp_obj_get_int(args[3])   : 29;      // DEGREES macro value
    float velocity = (n_args > 4) ? mp_obj_get_float(args[4]) : 601.0f;
    int units_v = (n_args > 5) ? mp_obj_get_int(args[5])   : 24;      // RPM macro value
    bool wait = (n_args > 6) ? mp_obj_is_true(args[6])   : true;

    MP_VEX_CALL_BLOCKING(wait, motor_group_spin_for(self -> cpp_handle, direction, angle, units, velocity, units_v, wait));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_group_spin_for_obj, 3, 7, mp_motor_group_spin_for);

static mp_obj_t mp_motor_group_spin_to_position(size_t n_args, const mp_obj_t *args){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int rotation = mp_obj_get_int(args[1]);

    int units = (n_args > 2) ? mp_obj_get_int(args[2])   : 29;
    float velocity = (n_args > 3) ? mp_obj_get_float(args[3]) : 601.0f;
    int units_v = (n_args > 4) ? mp_obj_get_int(args[4])   : 24;
    bool wait = (n_args > 5) ? mp_obj_is_true(args[5])   : true;

    MP_VEX_CALL_BLOCKING(wait, motor_group_spin_to_position(self -> cpp_handle, rotation, units, velocity, units_v, wait));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_group_spin_to_position_obj, 2, 6, mp_motor_group_spin_to_position);

static mp_obj_t mp_motor_group_stop(size_t n_args, const mp_obj_t *args){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    int mode = (n_args > 1) ? mp_obj_get_int(args[1])   : -1; // NALL macro value

    motor_group_stop(self -> cpp_handle, mode);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_group_stop_obj, 1, 2, mp_motor_group_stop);

static mp_obj_t mp_motor_group_set_position(mp_obj_t self_ptr, mp_obj_t position_t, mp_obj_t units_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    float position = mp_obj_get_float(position_t);
    int units = mp_obj_get_int(units_t);

    motor_group_set_position(self -> cpp_handle, position, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_motor_group_set_position_obj, mp_motor_group_set_position);

static mp_obj_t mp_motor_group_set_velosity(mp_obj_t self_ptr, mp_obj_t velosity_t, mp_obj_t units_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    float velosity = mp_obj_get_float(velosity_t);
    int units = mp_obj_get_int(units_t);

    motor_group_set_velocity(self -> cpp_handle, velosity, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_motor_group_set_velosity_obj, mp_motor_group_set_velosity);

static mp_obj_t mp_motor_group_set_stopping(mp_obj_t self_ptr, mp_obj_t mode_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int mode = mp_obj_get_int(mode_t);

    motor_group_set_stopping(self -> cpp_handle, mode);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_group_set_stopping_obj, mp_motor_group_set_stopping);

static mp_obj_t mp_motor_group_set_max_torque(mp_obj_t self_ptr, mp_obj_t torque_t, mp_obj_t units_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    float torque = mp_obj_get_float(torque_t);
    int units = mp_obj_get_int(units_t);

    motor_group_set_max_torque(self -> cpp_handle, torque, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_motor_group_set_max_torque_obj, mp_motor_group_set_max_torque);

static mp_obj_t mp_motor_group_set_timeout(mp_obj_t self_ptr, mp_obj_t value_t, mp_obj_t units_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    float value = mp_obj_get_float(value_t);
    int units = mp_obj_get_int(units_t);

    motor_group_set_timeout(self -> cpp_handle, value, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_motor_group_set_timeout_obj, mp_motor_group_set_timeout);

static mp_obj_t mp_motor_group_set_reversed(mp_obj_t self_ptr, mp_obj_t value_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    bool value = mp_obj_is_true(value_t);

    motor_group_set_reversed(self -> cpp_handle, value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_group_set_reversed_obj, mp_motor_group_set_reversed);

static mp_obj_t mp_motor_group_reset_position(mp_obj_t self_ptr){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    motor_group_reset_position(self -> cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_motor_group_reset_position_obj, mp_motor_group_reset_position);

static mp_obj_t mp_motor_group_is_done(mp_obj_t self_ptr){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    bool output;
    output = motor_group_is_done(self -> cpp_handle);
    return mp_obj_new_bool(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_motor_group_is_done_obj, mp_motor_group_is_done);

static mp_obj_t mp_motor_group_is_spinning(mp_obj_t self_ptr){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    bool output;
    output = motor_group_is_spinning(self -> cpp_handle);
    return mp_obj_new_bool(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_motor_group_is_spinning_obj, mp_motor_group_is_spinning);

static mp_obj_t mp_motor_group_position(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_group_position(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_group_position_obj, mp_motor_group_position);

static mp_obj_t mp_motor_group_velocity(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_group_velocity(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_group_velocity_obj, mp_motor_group_velocity);

static mp_obj_t mp_motor_group_current(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_group_current(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_group_current_obj, mp_motor_group_current);

static mp_obj_t mp_motor_group_power(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_group_power(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_group_power_obj, mp_motor_group_power);

static mp_obj_t mp_motor_group_torque(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_group_torque(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_group_torque_obj, mp_motor_group_torque);

static mp_obj_t mp_motor_group_efficiency(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_group_efficiency(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_group_efficiency_obj, mp_motor_group_efficiency);

static mp_obj_t mp_motor_group_temperature(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_t);

    float output;
    output = motor_group_temperature(self -> cpp_handle, units);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_motor_group_temperature_obj, mp_motor_group_temperature);

static mp_obj_t mp_motor_group_get_timeout(mp_obj_t self_ptr){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int output;
    output = motor_group_get_timeout(self -> cpp_handle);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_motor_group_get_timeout_obj, mp_motor_group_get_timeout);

static mp_obj_t mp_motor_group_count(mp_obj_t self_ptr){
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int output;
    output = motor_group_count(self -> cpp_handle);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_motor_group_count_obj, mp_motor_group_count);

static const mp_rom_map_elem_t mp_motorgroup_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_spin), MP_ROM_PTR(&mp_motor_group_spin_obj) },
    { MP_ROM_QSTR(MP_QSTR_spin_for), MP_ROM_PTR(&mp_motor_group_spin_for_obj) },
    { MP_ROM_QSTR(MP_QSTR_spin_to_position), MP_ROM_PTR(&mp_motor_group_spin_to_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&mp_motor_group_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_position), MP_ROM_PTR(&mp_motor_group_set_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_velocity), MP_ROM_PTR(&mp_motor_group_set_velosity_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_max_stopping), MP_ROM_PTR(&mp_motor_group_set_stopping_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_stopping), MP_ROM_PTR(&mp_motor_group_set_stopping_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_max_torque), MP_ROM_PTR(&mp_motor_group_set_max_torque_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_timeout), MP_ROM_PTR(&mp_motor_group_set_timeout_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_reversed), MP_ROM_PTR(&mp_motor_group_set_reversed_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_position), MP_ROM_PTR(&mp_motor_group_reset_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_done), MP_ROM_PTR(&mp_motor_group_is_done_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_spinning), MP_ROM_PTR(&mp_motor_group_is_spinning_obj) },
    { MP_ROM_QSTR(MP_QSTR_position), MP_ROM_PTR(&mp_motor_group_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_velocity), MP_ROM_PTR(&mp_motor_group_velocity_obj) },
    { MP_ROM_QSTR(MP_QSTR_current), MP_ROM_PTR(&mp_motor_group_current_obj) },
    { MP_ROM_QSTR(MP_QSTR_power), MP_ROM_PTR(&mp_motor_group_power_obj) },
    { MP_ROM_QSTR(MP_QSTR_torque), MP_ROM_PTR(&mp_motor_group_torque_obj) },
    { MP_ROM_QSTR(MP_QSTR_efficiency), MP_ROM_PTR(&mp_motor_group_efficiency_obj) },
    { MP_ROM_QSTR(MP_QSTR_temperature), MP_ROM_PTR(&mp_motor_group_temperature_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_timeout), MP_ROM_PTR(&mp_motor_group_get_timeout_obj) },
    { MP_ROM_QSTR(MP_QSTR_count), MP_ROM_PTR(&mp_motor_group_count_obj) },
};
static MP_DEFINE_CONST_DICT(mp_motorgroup_locals_dict, mp_motorgroup_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_motorgroup_type,
    MP_QSTR_motorgroup,
    MP_TYPE_FLAG_NONE,
    make_new, mp_motor_group_make_new,
    locals_dict, &mp_motorgroup_locals_dict
);

static size_t mp_drivetrain_motor_ports(mp_obj_t motors_obj, int8_t *ports) {
    size_t count = 0;
    if (mp_obj_is_type(motors_obj, &mp_motor_type)) {
        mp_motor_obj_t *motor = MP_OBJ_TO_PTR(motors_obj);
        ports[0] = (int8_t)motor_get_port(motor->cpp_handle);
        return 1;
    }

    mp_obj_t *items;
    mp_obj_get_array(motors_obj, &count, &items);
    if (count == 0 || count > 22) {
        mp_raise_ValueError("each drivetrain side requires 1 to 22 motors");
    }
    for (size_t i = 0; i < count; ++i) {
        if (!mp_obj_is_type(items[i], &mp_motor_type)) {
            mp_raise_TypeError("drivetrain motor lists must contain Motor objects");
        }
        mp_motor_obj_t *motor = MP_OBJ_TO_PTR(items[i]);
        ports[i] = (int8_t)motor_get_port(motor->cpp_handle);
    }
    return count;
}

static void* mp_drivetrain_extract_side(mp_obj_t side_obj, bool *is_group) {
    if (mp_obj_is_type(side_obj, &mp_motor_type)) {
        *is_group = false;
        return ((mp_motor_obj_t *)MP_OBJ_TO_PTR(side_obj))->cpp_handle;
    }
    if (mp_obj_is_type(side_obj, &mp_motorgroup_type)) {
        *is_group = true;
        return ((mp_motorgroup_obj_t *)MP_OBJ_TO_PTR(side_obj))->cpp_handle;
    }
    mp_raise_TypeError(MP_ERROR_TEXT("DriveTrain left and right arguments must be Motor or MotorGroup"));
}

static mp_obj_t mp_drivetrain_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 2, 7, false);

    bool left_is_group = false;
    bool right_is_group = false;
    void *left_ptr = mp_drivetrain_extract_side(args[0], &left_is_group);
    void *right_ptr = mp_drivetrain_extract_side(args[1], &right_is_group);

    float wheel_travel = n_args > 2 ? mp_obj_get_float(args[2]) : 300.0f;
    float track_width = n_args > 3 ? mp_obj_get_float(args[3]) : 320.0f;
    float wheel_base = n_args > 4 ? mp_obj_get_float(args[4]) : 320.0f;
    int units = n_args > 5 ? mp_obj_get_int(args[5]) : 40;
    float gear_ratio = n_args > 6 ? mp_obj_get_float(args[6]) : 1.0f;
    if (wheel_travel <= 0.0f || track_width <= 0.0f || wheel_base <= 0.0f || gear_ratio <= 0.0f) {
        mp_raise_ValueError("drivetrain dimensions and gear ratio must be positive");
    }

    mp_drivetrain_obj_t *self = mp_obj_malloc(mp_drivetrain_obj_t, type);
    self->cpp_handle = drivetrain_create(left_ptr, left_is_group, right_ptr, right_is_group,
                                         wheel_travel, track_width, wheel_base, units, gear_ratio);
    return MP_OBJ_FROM_PTR(self);
}


static mp_obj_t mp_drivetrain_drive(size_t n_args, const mp_obj_t *args) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int direction = mp_obj_get_int(args[1]);
    float velocity = n_args > 2 ? mp_obj_get_float(args[2]) : -1.0f;
    int units = n_args > 3 ? mp_obj_get_int(args[3]) : 23;
    drivetrain_drive(self->cpp_handle, direction, velocity, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_drivetrain_drive_obj, 2, 4, mp_drivetrain_drive);

static mp_obj_t mp_drivetrain_drive_for(size_t n_args, const mp_obj_t *args) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int direction = mp_obj_get_int(args[1]);
    float distance = mp_obj_get_float(args[2]);
    int units = n_args > 3 ? mp_obj_get_int(args[3]) : 40;
    float velocity = n_args > 4 ? mp_obj_get_float(args[4]) : -1.0f;
    int velocity_units = n_args > 5 ? mp_obj_get_int(args[5]) : 23;
    bool wait = n_args > 6 ? mp_obj_is_true(args[6]) : true;
    drivetrain_drive_for(self->cpp_handle, direction, distance, units, velocity, velocity_units, wait);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_drivetrain_drive_for_obj, 3, 7, mp_drivetrain_drive_for);

static mp_obj_t mp_drivetrain_turn(size_t n_args, const mp_obj_t *args) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int direction = mp_obj_get_int(args[1]);
    float velocity = n_args > 2 ? mp_obj_get_float(args[2]) : -1.0f;
    int units = n_args > 3 ? mp_obj_get_int(args[3]) : 23;
    drivetrain_turn(self->cpp_handle, direction, velocity, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_drivetrain_turn_obj, 2, 4, mp_drivetrain_turn);

static mp_obj_t mp_drivetrain_turn_for(size_t n_args, const mp_obj_t *args) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int direction = mp_obj_get_int(args[1]);
    float angle = mp_obj_get_float(args[2]);
    int units = n_args > 3 ? mp_obj_get_int(args[3]) : 29;
    float velocity = n_args > 4 ? mp_obj_get_float(args[4]) : -1.0f;
    int velocity_units = n_args > 5 ? mp_obj_get_int(args[5]) : 23;
    bool wait = n_args > 6 ? mp_obj_is_true(args[6]) : true;
    drivetrain_turn_for(self->cpp_handle, direction, angle, units, velocity, velocity_units, wait);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_drivetrain_turn_for_obj, 3, 7, mp_drivetrain_turn_for);

static mp_obj_t mp_drivetrain_turn_to_heading(size_t n_args, const mp_obj_t *args) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    float heading = mp_obj_get_float(args[1]);
    int units = n_args > 2 ? mp_obj_get_int(args[2]) : 29;
    float velocity = n_args > 3 ? mp_obj_get_float(args[3]) : -1.0f;
    int velocity_units = n_args > 4 ? mp_obj_get_int(args[4]) : 23;
    bool wait = n_args > 5 ? mp_obj_is_true(args[5]) : true;
    drivetrain_turn_to_heading(self->cpp_handle, heading, units, velocity, velocity_units, wait);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_drivetrain_turn_to_heading_obj, 2, 6, mp_drivetrain_turn_to_heading);

static mp_obj_t mp_drivetrain_turn_to_rotation(size_t n_args, const mp_obj_t *args) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    float rotation = mp_obj_get_float(args[1]);
    int units = n_args > 2 ? mp_obj_get_int(args[2]) : 29;
    float velocity = n_args > 3 ? mp_obj_get_float(args[3]) : -1.0f;
    int velocity_units = n_args > 4 ? mp_obj_get_int(args[4]) : 23;
    bool wait = n_args > 5 ? mp_obj_is_true(args[5]) : true;
    drivetrain_turn_to_rotation(self->cpp_handle, rotation, units, velocity, velocity_units, wait);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_drivetrain_turn_to_rotation_obj, 2, 6, mp_drivetrain_turn_to_rotation);

static mp_obj_t mp_drivetrain_stop(size_t n_args, const mp_obj_t *args) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int mode = n_args > 1 ? mp_obj_get_int(args[1]) : -1;
    drivetrain_stop(self->cpp_handle, mode);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_drivetrain_stop_obj, 1, 2, mp_drivetrain_stop);

static mp_obj_t mp_drivetrain_set_drive_velocity(mp_obj_t self_ptr, mp_obj_t velocity_obj, mp_obj_t units_obj) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    drivetrain_set_drive_velocity(self->cpp_handle, mp_obj_get_float(velocity_obj), mp_obj_get_int(units_obj));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_drivetrain_set_drive_velocity_obj, mp_drivetrain_set_drive_velocity);

static mp_obj_t mp_drivetrain_set_turn_velocity(mp_obj_t self_ptr, mp_obj_t velocity_obj, mp_obj_t units_obj) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    drivetrain_set_turn_velocity(self->cpp_handle, mp_obj_get_float(velocity_obj), mp_obj_get_int(units_obj));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_drivetrain_set_turn_velocity_obj, mp_drivetrain_set_turn_velocity);

static mp_obj_t mp_drivetrain_set_stopping(mp_obj_t self_ptr, mp_obj_t mode_obj) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    drivetrain_set_stopping(self->cpp_handle, mp_obj_get_int(mode_obj));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_drivetrain_set_stopping_obj, mp_drivetrain_set_stopping);

static mp_obj_t mp_drivetrain_set_timeout(mp_obj_t self_ptr, mp_obj_t value_obj, mp_obj_t units_obj) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    drivetrain_set_timeout(self->cpp_handle, mp_obj_get_float(value_obj), mp_obj_get_int(units_obj));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_drivetrain_set_timeout_obj, mp_drivetrain_set_timeout);

static mp_obj_t mp_drivetrain_set_heading(mp_obj_t self_ptr, mp_obj_t value_obj, mp_obj_t units_obj) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    drivetrain_set_heading(self->cpp_handle, mp_obj_get_float(value_obj), mp_obj_get_int(units_obj));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_drivetrain_set_heading_obj, mp_drivetrain_set_heading);

static mp_obj_t mp_drivetrain_set_rotation(mp_obj_t self_ptr, mp_obj_t value_obj, mp_obj_t units_obj) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    drivetrain_set_rotation(self->cpp_handle, mp_obj_get_float(value_obj), mp_obj_get_int(units_obj));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_drivetrain_set_rotation_obj, mp_drivetrain_set_rotation);

static mp_obj_t mp_drivetrain_is_done(mp_obj_t self_ptr) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_bool(drivetrain_is_done(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_drivetrain_is_done_obj, mp_drivetrain_is_done);

static mp_obj_t mp_drivetrain_is_moving(mp_obj_t self_ptr) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_bool(drivetrain_is_moving(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_drivetrain_is_moving_obj, mp_drivetrain_is_moving);

static mp_obj_t mp_drivetrain_heading(size_t n_args, const mp_obj_t *args) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : 29;
    return mp_obj_new_float(drivetrain_heading(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_drivetrain_heading_obj, 1, 2, mp_drivetrain_heading);

static mp_obj_t mp_drivetrain_rotation(size_t n_args, const mp_obj_t *args) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : 29;
    return mp_obj_new_float(drivetrain_rotation(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_drivetrain_rotation_obj, 1, 2, mp_drivetrain_rotation);

static mp_obj_t mp_drivetrain_velocity(mp_obj_t self_ptr, mp_obj_t units_obj) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_float(drivetrain_velocity(self->cpp_handle, mp_obj_get_int(units_obj)));
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_drivetrain_velocity_obj, mp_drivetrain_velocity);

static mp_obj_t mp_drivetrain_get_timeout(mp_obj_t self_ptr) {
    mp_drivetrain_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_int(drivetrain_get_timeout(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_drivetrain_get_timeout_obj, mp_drivetrain_get_timeout);

static const mp_rom_map_elem_t mp_drivetrain_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_drive), MP_ROM_PTR(&mp_drivetrain_drive_obj) },
    { MP_ROM_QSTR(MP_QSTR_drive_for), MP_ROM_PTR(&mp_drivetrain_drive_for_obj) },
    { MP_ROM_QSTR(MP_QSTR_turn), MP_ROM_PTR(&mp_drivetrain_turn_obj) },
    { MP_ROM_QSTR(MP_QSTR_turn_for), MP_ROM_PTR(&mp_drivetrain_turn_for_obj) },
    { MP_ROM_QSTR(MP_QSTR_turn_to_heading), MP_ROM_PTR(&mp_drivetrain_turn_to_heading_obj) },
    { MP_ROM_QSTR(MP_QSTR_turn_to_rotation), MP_ROM_PTR(&mp_drivetrain_turn_to_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&mp_drivetrain_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_drive_velocity), MP_ROM_PTR(&mp_drivetrain_set_drive_velocity_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_turn_velocity), MP_ROM_PTR(&mp_drivetrain_set_turn_velocity_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_stopping), MP_ROM_PTR(&mp_drivetrain_set_stopping_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_timeout), MP_ROM_PTR(&mp_drivetrain_set_timeout_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_heading), MP_ROM_PTR(&mp_drivetrain_set_heading_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_rotation), MP_ROM_PTR(&mp_drivetrain_set_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_done), MP_ROM_PTR(&mp_drivetrain_is_done_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_moving), MP_ROM_PTR(&mp_drivetrain_is_moving_obj) },
    { MP_ROM_QSTR(MP_QSTR_heading), MP_ROM_PTR(&mp_drivetrain_heading_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotation), MP_ROM_PTR(&mp_drivetrain_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_velocity), MP_ROM_PTR(&mp_drivetrain_velocity_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_timeout), MP_ROM_PTR(&mp_drivetrain_get_timeout_obj) },
};
static MP_DEFINE_CONST_DICT(mp_drivetrain_locals_dict, mp_drivetrain_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(
    mp_drivetrain_type,
    MP_QSTR_drivetrain,
    MP_TYPE_FLAG_NONE,
    make_new, mp_drivetrain_make_new,
    locals_dict, &mp_drivetrain_locals_dict
);

static mp_obj_t mp_message_link_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 3, 4, false);
    mp_message_link_obj_t *self = mp_obj_malloc(mp_message_link_obj_t, type);
    self->cpp_handle = message_link_create(
        mp_obj_get_int(args[0]), mp_obj_str_get_str(args[1]), mp_obj_get_int(args[2]),
        n_args > 3 ? mp_obj_is_true(args[3]) : true);
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_message_link_is_linked(mp_obj_t self_ptr) {
    mp_message_link_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_bool(message_link_is_linked(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_message_link_is_linked_obj, mp_message_link_is_linked);

static mp_obj_t mp_message_link_send(size_t n_args, const mp_obj_t *args) {
    mp_message_link_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *message = mp_obj_str_get_str(args[1]);
    int integer_input = n_args > 2 ? mp_obj_get_int(args[2]) : 0;
    float float_input = n_args > 3 ? mp_obj_get_float(args[3]) : 0.0f;
    message_link_send(self->cpp_handle, message, integer_input, float_input);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_message_link_send_obj, 2, 4, mp_message_link_send);

static mp_obj_t mp_message_link_receive(size_t n_args, const mp_obj_t *args) {
    mp_message_link_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int timeout = n_args > 1 ? mp_obj_get_int(args[1]) : 0;
    char buffer[241];
    int integer_output = 0;
    float float_output = 0.0f;
    int length = message_link_receive(self->cpp_handle, timeout, buffer, sizeof(buffer),
                                      &integer_output, &float_output);
    if (length == -1) {
        return mp_const_none;
    }
    if (length < 0) {
        mp_raise_ValueError("received VexLink message exceeds buffer capacity");
    }
    mp_obj_t values[] = {
        mp_obj_new_str(buffer, (size_t)length),
        mp_obj_new_int(integer_output),
        mp_obj_new_float(float_output),
    };
    return mp_obj_new_tuple(3, values);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_message_link_receive_obj, 1, 2, mp_message_link_receive);

static const mp_rom_map_elem_t mp_message_link_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_is_linked), MP_ROM_PTR(&mp_message_link_is_linked_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&mp_message_link_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_receive), MP_ROM_PTR(&mp_message_link_receive_obj) },
};
static MP_DEFINE_CONST_DICT(mp_message_link_locals_dict, mp_message_link_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(
    mp_message_link_type,
    MP_QSTR_messagelink,
    MP_TYPE_FLAG_NONE,
    make_new, mp_message_link_make_new,
    locals_dict, &mp_message_link_locals_dict
);

static mp_obj_t mp_serial_link_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 3, 4, false);
    mp_serial_link_obj_t *self = mp_obj_malloc(mp_serial_link_obj_t, type);
    self->cpp_handle = serial_link_create(
        mp_obj_get_int(args[0]), mp_obj_str_get_str(args[1]), mp_obj_get_int(args[2]),
        n_args > 3 ? mp_obj_is_true(args[3]) : true);
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_serial_link_is_linked(mp_obj_t self_ptr) {
    mp_serial_link_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_bool(serial_link_is_linked(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_serial_link_is_linked_obj, mp_serial_link_is_linked);

static mp_obj_t mp_serial_link_send(size_t n_args, const mp_obj_t *args) {
    mp_serial_link_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *message = mp_obj_str_get_str(args[1]);
    int integer_input = n_args > 2 ? mp_obj_get_int(args[2]) : 0;
    float float_input = n_args > 3 ? mp_obj_get_float(args[3]) : 0.0f;
    serial_link_send(self->cpp_handle, message, integer_input, float_input);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_serial_link_send_obj, 2, 4, mp_serial_link_send);

static mp_obj_t mp_serial_link_receive(size_t n_args, const mp_obj_t *args) {
    mp_serial_link_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int timeout = n_args > 1 ? mp_obj_get_int(args[1]) : 0;
    char buffer[241];
    int integer_output = 0;
    float float_output = 0.0f;
    int length = serial_link_receive(self->cpp_handle, timeout, buffer, sizeof(buffer),
                                     &integer_output, &float_output);
    if (length == -1) {
        return mp_const_none;
    }
    if (length < 0) {
        mp_raise_ValueError("received VexLink message exceeds buffer capacity");
    }
    mp_obj_t values[] = {
        mp_obj_new_str(buffer, (size_t)length),
        mp_obj_new_int(integer_output),
        mp_obj_new_float(float_output),
    };
    return mp_obj_new_tuple(3, values);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_serial_link_receive_obj, 1, 2, mp_serial_link_receive);

static const mp_rom_map_elem_t mp_serial_link_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_is_linked), MP_ROM_PTR(&mp_serial_link_is_linked_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&mp_serial_link_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_receive), MP_ROM_PTR(&mp_serial_link_receive_obj) },
};
static MP_DEFINE_CONST_DICT(mp_serial_link_locals_dict, mp_serial_link_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(
    mp_serial_link_type,
    MP_QSTR_seriallink,
    MP_TYPE_FLAG_NONE,
    make_new, mp_serial_link_make_new,
    locals_dict, &mp_serial_link_locals_dict
);

static mp_obj_t mp_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false); // strict 0 arguments

    mp_timer_obj_t *self = mp_obj_malloc(mp_timer_obj_t, type);
    self->cpp_handle = vex_timer_create();
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_timer_clear(mp_obj_t self_ptr){
    mp_timer_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    vex_timer_clear(self -> cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_timer_clear_obj, mp_timer_clear);

static mp_obj_t mp_timer_time(size_t n_args, const mp_obj_t *args){
    mp_timer_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    int units = (n_args > 1) ? mp_obj_get_int(args[1]) : VEX_TIME_UNITS_MSEC;

    int output;
    output = vex_timer_time(self -> cpp_handle, units);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_timer_time_obj, 1, 2, mp_timer_time);

static mp_obj_t mp_timer_event(mp_obj_t self_ptr, mp_obj_t callback_func, mp_obj_t delay_t){
    mp_timer_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int delay = mp_obj_get_int(delay_t);
    mp_obj_t callback_data = mp_vex_callback_data(callback_func, mp_const_none, true, true);

    vex_timer_event(self -> cpp_handle, callback_data, delay);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_timer_event_obj, mp_timer_event);

static const mp_rom_map_elem_t mp_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&mp_timer_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&mp_timer_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_event), MP_ROM_PTR(&mp_timer_event_obj) },
};
static MP_DEFINE_CONST_DICT(mp_timer_locals_dict, mp_timer_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_timer_type,
    MP_QSTR_timer,
    MP_TYPE_FLAG_NONE,
    make_new, mp_timer_make_new,
    locals_dict, &mp_timer_locals_dict
);

static mp_obj_t mp_vex_thread_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    mp_obj_t callback = args[0];
    if (!mp_obj_is_callable(callback)) {
        mp_raise_TypeError("thread callback must be callable");
    }

    mp_vex_thread_obj_t *self = mp_obj_malloc_with_finaliser(mp_vex_thread_obj_t, type);
    self->thread_id = 0;
    self->stopped = false;
    self->done = false;
    bool no_args = n_args == 1;
    mp_obj_t callback_items[] = {
        callback,
        no_args ? mp_const_none : args[1],
        mp_obj_new_bool(no_args),
        mp_const_true,
        MP_OBJ_FROM_PTR(self),
        MP_OBJ_FROM_PTR(mp_locals_get()),
        MP_OBJ_FROM_PTR(mp_globals_get()),
    };
    mp_obj_t callback_data = mp_obj_new_list(MP_ARRAY_SIZE(callback_items), callback_items);
    if (MP_STATE_VM(vex_callback_roots) == MP_OBJ_NULL) {
        MP_STATE_VM(vex_callback_roots) = mp_obj_new_list(0, NULL);
    }
    mp_obj_list_append(MP_STATE_VM(vex_callback_roots), callback_data);
    size_t stack_size = 0;
    self->thread_id = mp_thread_create(mp_vex_thread_entry, MP_OBJ_TO_PTR(callback_data), &stack_size);
    if (self->thread_id == 0) {
        mp_obj_list_remove(MP_STATE_VM(vex_callback_roots), callback_data);
        mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("could not create thread"));
    }
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_vex_thread_is_done(mp_obj_t self_ptr) {
    mp_vex_thread_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_bool(self->done);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_thread_is_done_obj, mp_vex_thread_is_done);

static mp_obj_t mp_vex_thread_stop(mp_obj_t self_ptr) {
    mp_vex_thread_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    self->stopped = true;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_thread_stop_obj, mp_vex_thread_stop);

static mp_obj_t mp_vex_thread_del(mp_obj_t self_ptr) {
    mp_vex_thread_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    self->stopped = true;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_thread_del_obj, mp_vex_thread_del);

static const mp_rom_map_elem_t mp_vex_thread_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_is_done), MP_ROM_PTR(&mp_vex_thread_is_done_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&mp_vex_thread_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_vex_thread_del_obj) },
};
static MP_DEFINE_CONST_DICT(mp_vex_thread_locals_dict, mp_vex_thread_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_vex_thread_type,
    MP_QSTR_Thread,
    MP_TYPE_FLAG_NONE,
    make_new, mp_vex_thread_make_new,
    locals_dict, &mp_vex_thread_locals_dict
);

/* Defining class brain.battery for micropython module. */
static mp_obj_t mp_brain_battery_voltage(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_brain_battery_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int units = mp_obj_get_int(units_t);

    float output;
    output = brain_battery_voltage(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_battery_voltage_obj, mp_brain_battery_voltage);

static mp_obj_t mp_brain_battery_current(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_brain_battery_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int units = mp_obj_get_int(units_t);

    float output;
    output = brain_battery_current(self -> cpp_handle, units);
    return mp_obj_new_float(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_battery_current_obj, mp_brain_battery_current);

static mp_obj_t mp_brain_battery_capacity(mp_obj_t self_ptr){
    mp_brain_battery_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int output;
    output = brain_battery_capacity(self -> cpp_handle);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_brain_battery_capacity_obj, mp_brain_battery_capacity);

static mp_obj_t mp_brain_battery_temperature(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_brain_battery_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int units = mp_obj_get_int(units_t);

    int output;
    output = brain_battery_temperature(self -> cpp_handle, units);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_battery_temperature_obj, mp_brain_battery_temperature);

static const mp_rom_map_elem_t mp_brain_battery_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_PythonMethodName), MP_ROM_PTR(&C_Function_Object) }
    { MP_ROM_QSTR(MP_QSTR_voltage), MP_ROM_PTR(&mp_brain_battery_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_current), MP_ROM_PTR(&mp_brain_battery_current_obj) },
    { MP_ROM_QSTR(MP_QSTR_capacity), MP_ROM_PTR(&mp_brain_battery_capacity_obj) },
    { MP_ROM_QSTR(MP_QSTR_temperature), MP_ROM_PTR(&mp_brain_battery_temperature_obj) },
};
static MP_DEFINE_CONST_DICT(mp_brain_battery_locals_dict, mp_brain_battery_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_brain_battery_type,
    MP_QSTR_battery,
    MP_TYPE_FLAG_NONE,
    locals_dict, &mp_brain_battery_locals_dict
);

/* Defining class brain.screen for micropython module. */

static mp_obj_t mp_vex_stringify(mp_obj_t value) {
    vstr_t buffer;
    mp_print_t print;
    vstr_init_print(&buffer, 16, &print);
    mp_obj_print_helper(&print, value, PRINT_STR);
    return mp_obj_new_str_from_utf8_vstr(&buffer);
}

static mp_obj_t mp_brain_screen_print(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_text, ARG_sep, ARG_precision };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_text, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_sep, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_precision, MP_ARG_INT, {.u_int = 2} },
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, values);
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t text_obj = mp_vex_stringify(values[ARG_text].u_obj);
    const char *text = mp_obj_str_get_str(text_obj);
    const char *sep = values[ARG_sep].u_obj == MP_OBJ_NULL
        ? ""
        : mp_obj_str_get_str(values[ARG_sep].u_obj);
    int precision = values[ARG_precision].u_int;

    brain_screen_print(self -> cpp_handle, text, sep, precision);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mp_brain_screen_print_obj, 1, mp_brain_screen_print);

static mp_obj_t mp_brain_screen_set_cursor(mp_obj_t self_ptr, mp_obj_t row_t, mp_obj_t col_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int row = mp_obj_get_int(row_t);
    int col = mp_obj_get_int(col_t);

    brain_screen_set_cursor(self -> cpp_handle, row, col);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_brain_screen_set_cursor_obj, mp_brain_screen_set_cursor);

static mp_obj_t mp_brain_screen_next_row(mp_obj_t self_ptr){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    brain_screen_next_row(self -> cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_brain_screen_next_row_obj, mp_brain_screen_next_row);

static mp_obj_t mp_brain_screen_clear_row(size_t n_args, const mp_obj_t *args){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    int row = mp_obj_get_int(args[1]);
    int color = (n_args > 2) ? mp_color_get_value(args[2]) : 0;

    brain_screen_clear_row(self -> cpp_handle, row, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_screen_clear_row_obj, 2, 3 , mp_brain_screen_clear_row);

static mp_obj_t mp_brain_screen_row(mp_obj_t self_ptr){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int output = 0;

    output = brain_screen_row(self -> cpp_handle);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_brain_screen_row_obj, mp_brain_screen_row);

static mp_obj_t mp_brain_screen_column(mp_obj_t self_ptr){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int output = 0;

    output = brain_screen_column(self -> cpp_handle);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_brain_screen_column_obj, mp_brain_screen_column);

static mp_obj_t mp_brain_screen_print_at(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    enum { ARG_text, ARG_x, ARG_y, ARG_sep, ARG_precision, ARG_opaque };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_text, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_x, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_y, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_sep, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_precision, MP_ARG_INT, {.u_int = 2} },
        { MP_QSTR_opaque, MP_ARG_BOOL, {.u_bool = true} },
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, values);
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t text_obj = mp_vex_stringify(values[ARG_text].u_obj);
    const char *text = mp_obj_str_get_str(text_obj);
    int x = values[ARG_x].u_int;
    int y = values[ARG_y].u_int;
    const char *sep = values[ARG_sep].u_obj == MP_OBJ_NULL
        ? " "
        : mp_obj_str_get_str(values[ARG_sep].u_obj);
    int precision = values[ARG_precision].u_int;
    bool opaque = values[ARG_opaque].u_bool;

    brain_screen_print_at(self -> cpp_handle, text, x, y, sep, precision, opaque);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mp_brain_screen_print_at_obj, 1, mp_brain_screen_print_at);

static mp_obj_t mp_brain_screen_get_string_width(mp_obj_t self_ptr, mp_obj_t string_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    const char* text = mp_obj_str_get_str(string_t);

    int output = 0;

    output = brain_screen_get_string_width(self -> cpp_handle, text);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_screen_get_string_width_obj, mp_brain_screen_get_string_width);

static mp_obj_t mp_brain_screen_get_string_height(mp_obj_t self_ptr, mp_obj_t string_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    const char* text = mp_obj_str_get_str(string_t);

    int output = 0;

    output = brain_screen_get_string_height(self -> cpp_handle, text);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_screen_get_string_height_obj, mp_brain_screen_get_string_height);

static mp_obj_t mp_brain_screen_clear_line(mp_obj_t self_ptr, mp_obj_t line_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int line = mp_obj_get_int(line_t);

    brain_screen_clear_line(self -> cpp_handle, line);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_screen_clear_line_obj, mp_brain_screen_clear_line);

static mp_obj_t mp_brain_screen_new_line(mp_obj_t self_ptr){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    brain_screen_new_line(self -> cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_brain_screen_new_line_obj, mp_brain_screen_new_line);

static mp_obj_t mp_brain_screen_clear_screen(size_t n_args, const mp_obj_t *args){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int color = (n_args > 1) ? mp_color_get_value(args[1]) : 0; // Default to BLACK (0)

    brain_screen_clear_screen(self->cpp_handle, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_screen_clear_screen_obj, 1, 2, mp_brain_screen_clear_screen);

static mp_obj_t mp_brain_screen_set_font(mp_obj_t self_ptr, mp_obj_t font_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int font = mp_obj_get_int(font_t);

    brain_screen_set_font(self -> cpp_handle, font);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_screen_set_font_obj, mp_brain_screen_set_font);

static mp_obj_t mp_brain_screen_set_pen_width(mp_obj_t self_ptr, mp_obj_t value_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int value = mp_obj_get_int(value_t);

    brain_screen_set_pen_width(self -> cpp_handle, value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_screen_set_pen_width_obj, mp_brain_screen_set_pen_width);

static mp_obj_t mp_brain_screen_set_pen_color(mp_obj_t self_ptr, mp_obj_t color_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int color = mp_color_get_value(color_t);

    brain_screen_set_pen_color(self -> cpp_handle, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_screen_set_pen_color_obj, mp_brain_screen_set_pen_color);

static mp_obj_t mp_brain_screen_set_fill_color(mp_obj_t self_ptr, mp_obj_t color_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int color = mp_color_get_value(color_t);

    brain_screen_set_fill_color(self -> cpp_handle, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_screen_set_fill_color_obj, mp_brain_screen_set_fill_color);

static mp_obj_t mp_brain_screen_set_origin(mp_obj_t self_ptr, mp_obj_t x_t, mp_obj_t y_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int x = mp_obj_get_int(x_t);
    int y = mp_obj_get_int(y_t);

    brain_screen_set_origin(self -> cpp_handle, x, y);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_brain_screen_set_origin_obj, mp_brain_screen_set_origin);

static mp_obj_t mp_brain_screen_draw_pixel(mp_obj_t self_ptr, mp_obj_t x_t, mp_obj_t y_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int x = mp_obj_get_int(x_t);
    int y = mp_obj_get_int(y_t);

    brain_screen_draw_pixel(self -> cpp_handle, x, y);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_brain_screen_draw_pixel_obj, mp_brain_screen_draw_pixel);

static mp_obj_t mp_brain_screen_draw_line(size_t n_args, const mp_obj_t *args){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    int start_x = mp_obj_get_int(args[1]);
    int start_y = mp_obj_get_int(args[2]);
    int end_x = mp_obj_get_int(args[3]);
    int end_y = mp_obj_get_int(args[4]);

    brain_screen_draw_line(self -> cpp_handle, start_x, start_y, end_x, end_y);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_screen_draw_line_obj, 5, 5, mp_brain_screen_draw_line);

static mp_obj_t mp_brain_screen_draw_rectangle(size_t n_args, const mp_obj_t *args){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int width = mp_obj_get_int(args[3]);
    int height = mp_obj_get_int(args[4]);
    int color = (n_args > 5) ? mp_color_get_value(args[5]) : 0; // Default color is BLACK

    brain_screen_draw_rectangle(self -> cpp_handle, x, y, width, height, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_screen_draw_rectangle_obj, 5, 6,mp_brain_screen_draw_rectangle);

static mp_obj_t mp_brain_screen_draw_circle(size_t n_args, const mp_obj_t *args){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int radius = mp_obj_get_int(args[3]);
    int color = mp_color_get_value(args[4]);

    brain_screen_draw_circle(self->cpp_handle, x, y, radius, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_screen_draw_circle_obj, 5, 5, mp_brain_screen_draw_circle);

static mp_obj_t mp_brain_screen_render(mp_obj_t self_ptr){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    brain_screen_render(self -> cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_brain_screen_render_obj, mp_brain_screen_render);

static mp_obj_t mp_brain_screen_set_clip_region(size_t n_args, const mp_obj_t *args){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int width = mp_obj_get_int(args[3]);
    int height = mp_obj_get_int(args[4]);

    brain_screen_set_clip_region(self -> cpp_handle, x, y, width, height);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_screen_set_clip_region_obj, 5, 5, mp_brain_screen_set_clip_region);

static mp_obj_t mp_brain_screen_pressing(mp_obj_t self_ptr){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    bool output = false;

    output = brain_screen_pressing(self -> cpp_handle);
    return output ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_brain_screen_pressing_obj, mp_brain_screen_pressing);

static mp_obj_t mp_brain_screen_x_position(mp_obj_t self_ptr){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int output = 0;

    output = brain_screen_x_position(self -> cpp_handle);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_brain_screen_x_position_obj, mp_brain_screen_x_position);

static mp_obj_t mp_brain_screen_y_position(mp_obj_t self_ptr){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int output = 0;

    output = brain_screen_y_position(self -> cpp_handle);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_brain_screen_y_position_obj, mp_brain_screen_y_position);

static mp_obj_t mp_brain_screen_pressed(mp_obj_t self_ptr, mp_obj_t callback, mp_obj_t args){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    mp_obj_t callback_data = mp_vex_callback_data(callback, args, false, false);

    brain_screen_pressed(self -> cpp_handle, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_brain_screen_pressed_obj, mp_brain_screen_pressed);

static mp_obj_t mp_brain_screen_released(mp_obj_t self_ptr, mp_obj_t callback, mp_obj_t args){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    mp_obj_t callback_data = mp_vex_callback_data(callback, args, false, false);

    brain_screen_released(self -> cpp_handle, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_brain_screen_released_obj, mp_brain_screen_released);

static const mp_rom_map_elem_t mp_brain_screen_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_PythonMethodName), MP_ROM_PTR(&C_Function_Object) }
    { MP_ROM_QSTR(MP_QSTR_print), MP_ROM_PTR(&mp_brain_screen_print_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_cursor), MP_ROM_PTR(&mp_brain_screen_set_cursor_obj) },
    { MP_ROM_QSTR(MP_QSTR_next_row), MP_ROM_PTR(&mp_brain_screen_next_row_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear_row), MP_ROM_PTR(&mp_brain_screen_clear_row_obj) },
    { MP_ROM_QSTR(MP_QSTR_row), MP_ROM_PTR(&mp_brain_screen_row_obj) },
    { MP_ROM_QSTR(MP_QSTR_column), MP_ROM_PTR(&mp_brain_screen_column_obj) },
    { MP_ROM_QSTR(MP_QSTR_print_at), MP_ROM_PTR(&mp_brain_screen_print_at_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_string_width), MP_ROM_PTR(&mp_brain_screen_get_string_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_string_height), MP_ROM_PTR(&mp_brain_screen_get_string_height_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear_line), MP_ROM_PTR(&mp_brain_screen_clear_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_new_line), MP_ROM_PTR(&mp_brain_screen_new_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear_screen), MP_ROM_PTR(&mp_brain_screen_clear_screen_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_font), MP_ROM_PTR(&mp_brain_screen_set_font_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pen_width), MP_ROM_PTR(&mp_brain_screen_set_pen_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pen_color), MP_ROM_PTR(&mp_brain_screen_set_pen_color_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_fill_color), MP_ROM_PTR(&mp_brain_screen_set_fill_color_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_origin), MP_ROM_PTR(&mp_brain_screen_set_origin_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_pixel), MP_ROM_PTR(&mp_brain_screen_draw_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_line), MP_ROM_PTR(&mp_brain_screen_draw_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_rectangle), MP_ROM_PTR(&mp_brain_screen_draw_rectangle_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_circle), MP_ROM_PTR(&mp_brain_screen_draw_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_render), MP_ROM_PTR(&mp_brain_screen_render_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_clip_region), MP_ROM_PTR(&mp_brain_screen_set_clip_region_obj) },
    { MP_ROM_QSTR(MP_QSTR_pressing), MP_ROM_PTR(&mp_brain_screen_pressing_obj) },
    { MP_ROM_QSTR(MP_QSTR_x_position), MP_ROM_PTR(&mp_brain_screen_x_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_y_position), MP_ROM_PTR(&mp_brain_screen_y_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_pressed), MP_ROM_PTR(&mp_brain_screen_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_released), MP_ROM_PTR(&mp_brain_screen_released_obj) },
};
static MP_DEFINE_CONST_DICT(mp_brain_screen_locals_dict, mp_brain_screen_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_brain_screen_type,
    MP_QSTR_screen,
    MP_TYPE_FLAG_NONE,
    locals_dict, &mp_brain_screen_locals_dict 
);

static mp_obj_t mp_brain_sdcard_loadfile(size_t n_args, const mp_obj_t *args){
    mp_brain_sdcard_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    const char* filename = mp_obj_str_get_str(args[1]);
    char* buffer = NULL;
    size_t buffer_size = 0;
    mp_buffer_info_t buffer_info;
    if (n_args > 2) {
        mp_get_buffer_raise(args[2], &buffer_info, MP_BUFFER_WRITE);
        buffer = buffer_info.buf;
        buffer_size = buffer_info.len;
    }

    size_t result_size = 0;
    MP_THREAD_GIL_EXIT();
    char* result = brain_sdcard_loadfile(self->cpp_handle, filename, buffer, buffer_size, &result_size);
    MP_THREAD_GIL_ENTER();

    const char* output_data = result_size == 0 ? "" : result;
    mp_obj_t output = mp_obj_new_bytearray(result_size, output_data);
    if (buffer == NULL) {
        brain_sdcard_freefile(result);
    }
    return output;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_sdcard_loadfile_obj, 2, 3, mp_brain_sdcard_loadfile);

static mp_obj_t mp_brain_sdcard_savefile(size_t n_args, const mp_obj_t *args){
    mp_brain_sdcard_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    const char* filename = mp_obj_str_get_str(args[1]);
    const char* buffer = NULL;
    size_t buffer_size = 0;
    if (n_args > 2 && args[2] != mp_const_none) {
        mp_buffer_info_t buffer_info;
        mp_get_buffer_raise(args[2], &buffer_info, MP_BUFFER_READ);
        buffer = buffer_info.buf;
        buffer_size = buffer_info.len;
    }

    MP_THREAD_GIL_EXIT();
    brain_sdcard_savefile(self->cpp_handle, filename, buffer, buffer_size);
    MP_THREAD_GIL_ENTER();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_sdcard_savefile_obj, 2, 3, mp_brain_sdcard_savefile);

static mp_obj_t mp_brain_sdcard_appendfile(mp_obj_t self_ptr, mp_obj_t filename_t, mp_obj_t buffer_t){
    mp_brain_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    const char* filename = mp_obj_str_get_str(filename_t);
    mp_buffer_info_t buffer_info;
    mp_get_buffer_raise(buffer_t, &buffer_info, MP_BUFFER_READ);

    MP_THREAD_GIL_EXIT();
    brain_sdcard_appendfile(self->cpp_handle, filename, buffer_info.buf, buffer_info.len);
    MP_THREAD_GIL_ENTER();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_brain_sdcard_appendfile_obj, mp_brain_sdcard_appendfile);

static mp_obj_t mp_brain_sdcard_draw_image_from_file(size_t n_args, const mp_obj_t *args){
    mp_brain_sdcard_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    const char* filename = mp_obj_str_get_str(args[1]);
    int x = mp_obj_get_int(args[2]);
    int y = mp_obj_get_int(args[3]);

    brain_sdcard_draw_image_from_file(self -> cpp_handle, filename, x, y);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_sdcard_draw_image_from_file_obj, 4, 4, mp_brain_sdcard_draw_image_from_file);

static mp_obj_t mp_brain_sdcard_is_inserted(mp_obj_t self_ptr){
    mp_brain_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    bool output = false;
    output = brain_sdcard_is_inserted(self -> cpp_handle);
    return output ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_brain_sdcard_is_inserted_obj, mp_brain_sdcard_is_inserted);

static mp_obj_t mp_brain_sdcard_filesize(mp_obj_t self_ptr, mp_obj_t filename_t){
    mp_brain_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    const char* filename = mp_obj_str_get_str(filename_t);
    size_t output = 0;
    output = brain_sdcard_filesize(self -> cpp_handle, filename);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_sdcard_filesize_obj, mp_brain_sdcard_filesize);

static mp_obj_t mp_brain_sdcard_size(mp_obj_t self_ptr, mp_obj_t filename_t){
    mp_brain_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    const char* filename = mp_obj_str_get_str(filename_t);
    size_t output = 0;
    output = brain_sdcard_size(self -> cpp_handle, filename);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_sdcard_size_obj, mp_brain_sdcard_size);

static mp_obj_t mp_brain_sdcard_exists(mp_obj_t self_ptr, mp_obj_t filename_t){
    mp_brain_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    const char* filename = mp_obj_str_get_str(filename_t);
    bool output = false;
    output = brain_sdcard_exists(self -> cpp_handle, filename);
    return output ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_sdcard_exists_obj, mp_brain_sdcard_exists);

/* Defining class brain.sdcard for micropython module. */
static const mp_rom_map_elem_t mp_brain_sdcard_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_PythonMethodName), MP_ROM_PTR(&C_Function_Object) }
    { MP_ROM_QSTR(MP_QSTR_loadfile), MP_ROM_PTR(&mp_brain_sdcard_loadfile_obj) },
    { MP_ROM_QSTR(MP_QSTR_savefile), MP_ROM_PTR(&mp_brain_sdcard_savefile_obj) },
    { MP_ROM_QSTR(MP_QSTR_appendfile), MP_ROM_PTR(&mp_brain_sdcard_appendfile_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_image_from_file), MP_ROM_PTR(&mp_brain_sdcard_draw_image_from_file_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_inserted), MP_ROM_PTR(&mp_brain_sdcard_is_inserted_obj) },
    { MP_ROM_QSTR(MP_QSTR_filesize), MP_ROM_PTR(&mp_brain_sdcard_filesize_obj) },
    { MP_ROM_QSTR(MP_QSTR_size), MP_ROM_PTR(&mp_brain_sdcard_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_exists), MP_ROM_PTR(&mp_brain_sdcard_exists_obj) },
};
static MP_DEFINE_CONST_DICT(mp_brain_sdcard_locals_dict, mp_brain_sdcard_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_brain_sdcard_type,
    MP_QSTR_sdcard,
    MP_TYPE_FLAG_NONE,
    locals_dict, &mp_brain_sdcard_locals_dict
);

static void mp_brain_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    // CRITICAL SAFETY GUARD: If self_in is uninstantiated metadata type, reject immediately
    if (mp_obj_get_type(self_in) == &mp_type_type) {
        dest[0] = MP_OBJ_NULL;
        return;
    }

    mp_brain_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (dest[0] == MP_OBJ_NULL) {
        if (attr == MP_QSTR_screen) {
            dest[0] = self->screen;
            return;
        } else if (attr == MP_QSTR_sdcard) {
            dest[0] = self->sdcard;
            return;
        } else if (attr == MP_QSTR_battery) {
            dest[0] = self->battery;
            return;
        } else if (attr == MP_QSTR_three_wire_port) {
            dest[0] = self->three_wire_port;
            return;
        }
    }
    dest[0] = MP_OBJ_NULL; 
}


static mp_obj_t mp_brain_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false); // strict 0 arguments

    void *cpp_brain = brain_create();

    // 2. Allocate parent wrapper struct
    mp_brain_obj_t *self = mp_obj_malloc(mp_brain_obj_t, type);
    self->cpp_handle = cpp_brain;

    // 3. Allocate sub-structures, linking their pointers cleanly to internal segments
    mp_brain_screen_obj_t *scr = mp_obj_malloc(mp_brain_screen_obj_t, &mp_brain_screen_type);
    scr->cpp_handle = brain_get_screen(cpp_brain);

    mp_brain_sdcard_obj_t *sd = mp_obj_malloc(mp_brain_sdcard_obj_t, &mp_brain_sdcard_type);
    sd->cpp_handle = brain_get_sdcard(cpp_brain);

    mp_brain_battery_obj_t *bat = mp_obj_malloc(mp_brain_battery_obj_t, &mp_brain_battery_type);
    bat->cpp_handle = brain_get_battery(cpp_brain);

    mp_three_wire_ports_obj_t *three_wire_ports = mp_obj_malloc(mp_three_wire_ports_obj_t, &mp_three_wire_ports_type);
    self->three_wire_port = MP_OBJ_FROM_PTR(three_wire_ports);
    for (int i = 0; i < 8; ++i){
        three_wire_ports->ports[i] = mp_const_none;
    }
    for (int i = 0; i < 8; ++i){
        mp_three_wire_port_obj_t *port = mp_obj_malloc(mp_three_wire_port_obj_t, &mp_three_wire_port_type);
        port->port = i + 1;
        three_wire_ports->ports[i] = MP_OBJ_FROM_PTR(port);
    }

    // 4. Assign Python reference bindings
    self->screen = MP_OBJ_FROM_PTR(scr);
    self->sdcard = MP_OBJ_FROM_PTR(sd);
    self->battery = MP_OBJ_FROM_PTR(bat);
    return MP_OBJ_FROM_PTR(self);
}

static const mp_rom_map_elem_t mp_brain_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_PythonMethodName), MP_ROM_PTR(&C_Function_Object) }
};
static MP_DEFINE_CONST_DICT(mp_brain_locals_dict, mp_brain_locals_dict_table);

// Register the finalized Motor class metadata type
MP_DEFINE_CONST_OBJ_TYPE(
    mp_brain_type,
    MP_QSTR_brain,
    MP_TYPE_FLAG_NONE,
    make_new, mp_brain_make_new,
    attr, mp_brain_attr,
    locals_dict, &mp_brain_locals_dict
);

enum {
    MP_VEX_PORT1 = 1,
    MP_VEX_PORT2,
    MP_VEX_PORT3,
    MP_VEX_PORT4,
    MP_VEX_PORT5,
    MP_VEX_PORT6,
    MP_VEX_PORT7,
    MP_VEX_PORT8,
    MP_VEX_PORT9,
    MP_VEX_PORT10,
    MP_VEX_PORT11,
    MP_VEX_PORT12,
    MP_VEX_PORT13,
    MP_VEX_PORT14,
    MP_VEX_PORT15,
    MP_VEX_PORT16,
    MP_VEX_PORT17,
    MP_VEX_PORT18,
    MP_VEX_PORT19,
    MP_VEX_PORT20,
    MP_VEX_PORT21,
    MP_VEX_PORT22,
    MP_VEX_PERCENT = 23,
    MP_VEX_RPM = 24,
    MP_VEX_DPS = 26,
    MP_VEX_DEGREES = 29,
    MP_VEX_TURNS = 30,
    MP_VEX_AXIS_X = 52,
    MP_VEX_AXIS_Y = 53,
    MP_VEX_AXIS_Z = 54,
    MP_VEX_PITCH = 55,
    MP_VEX_ROLL = 56,
    MP_VEX_YAW = 57,
    MP_VEX_RIGHT = 58,
    MP_VEX_LEFT = 59,
    MP_VEX_MM = 40,
    MP_VEX_INCHES = 41,
    MP_VEX_ANALOG_EIGHTBIT = 42,
    MP_VEX_ANALOG_TENBIT = 43,
    MP_VEX_ANALOG_TWELVEBIT = 44
};

static void mp_three_wire_ports_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest){
    mp_three_wire_ports_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] == MP_OBJ_NULL){
        if (attr == MP_QSTR_a) dest[0] = self->ports[0];
        else if (attr == MP_QSTR_b) dest[0] = self->ports[1];
        else if (attr == MP_QSTR_c) dest[0] = self->ports[2];
        else if (attr == MP_QSTR_d) dest[0] = self->ports[3];
        else if (attr == MP_QSTR_e) dest[0] = self->ports[4];
        else if (attr == MP_QSTR_f) dest[0] = self->ports[5];
        else if (attr == MP_QSTR_g) dest[0] = self->ports[6];
        else if (attr == MP_QSTR_h) dest[0] = self->ports[7];
        else dest[0] = MP_OBJ_NULL;
        if (dest[0] != MP_OBJ_NULL){
            return;
        }
    }
    dest[0] = MP_OBJ_NULL;
}

MP_DEFINE_CONST_OBJ_TYPE(
    mp_three_wire_port_type,
    MP_QSTR_threewireport,
    MP_TYPE_FLAG_NONE
);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_three_wire_ports_type,
    MP_QSTR_three_wire_port,
    MP_TYPE_FLAG_NONE,
    attr, mp_three_wire_ports_attr
);

static mp_obj_t mp_digital_out_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    if (mp_obj_get_type(args[0]) != &mp_three_wire_port_type){
        mp_raise_TypeError("port must be a Brain three_wire_port");
    }

    mp_three_wire_port_obj_t *port = MP_OBJ_TO_PTR(args[0]);
    mp_digital_out_obj_t *self = mp_obj_malloc(mp_digital_out_obj_t, type);
    self->cpp_handle = vex_digital_out_create(port->port);
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_digital_out_set(mp_obj_t self_ptr, mp_obj_t value_obj){
    mp_digital_out_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    vex_digital_out_set(self->cpp_handle, mp_obj_is_true(value_obj));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_digital_out_set_obj, mp_digital_out_set);

static const mp_rom_map_elem_t mp_digital_out_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&mp_digital_out_set_obj) },
};
static MP_DEFINE_CONST_DICT(mp_digital_out_locals_dict, mp_digital_out_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_digital_out_type,
    MP_QSTR_digitalout,
    MP_TYPE_FLAG_NONE,
    make_new, mp_digital_out_make_new,
    locals_dict, &mp_digital_out_locals_dict
);

static void mp_vex_check_angle_units(int units);
static void mp_vex_check_velocity_units(int units, bool allow_percent);

static mp_obj_t mp_three_wire_make_new_kind(const mp_obj_type_t *type, size_t n_args, size_t n_kw,
                                             const mp_obj_t *args, int kind, int extra, int option){
    mp_arg_check_num(n_args, n_kw, 1 + extra, 1 + extra, false);
    if (mp_obj_get_type(args[0]) != &mp_three_wire_port_type){
        mp_raise_TypeError("port must be a Brain three_wire_port");
    }
    mp_three_wire_port_obj_t *port = MP_OBJ_TO_PTR(args[0]);
    mp_three_wire_device_obj_t *self = mp_obj_malloc(mp_three_wire_device_obj_t, type);
    int config = option;
    if (extra){
        config = mp_obj_get_int(args[1]);
    }
    self->cpp_handle = vex_three_wire_create(kind, port->port, config);
    return MP_OBJ_FROM_PTR(self);
}

#define MP_THREE_WIRE_MAKE_NEW(name, kind, extra, option) \
static mp_obj_t name(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){ \
    return mp_three_wire_make_new_kind(type, n_args, n_kw, args, kind, extra, option); \
}

MP_THREE_WIRE_MAKE_NEW(mp_analog_in_make_new, VEX_THREE_WIRE_ANALOG_IN, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_bumper_make_new, VEX_THREE_WIRE_BUMPER, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_digital_in_make_new, VEX_THREE_WIRE_DIGITAL_IN, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_encoder_make_new, VEX_THREE_WIRE_ENCODER, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_gyro_make_new, VEX_THREE_WIRE_GYRO, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_led_make_new, VEX_THREE_WIRE_LED_INDICATOR, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_light_make_new, VEX_THREE_WIRE_LIGHT, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_limit_make_new, VEX_THREE_WIRE_LIMIT, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_line_make_new, VEX_THREE_WIRE_LINE, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_potentiometer_make_new, VEX_THREE_WIRE_POTENTIOMETER, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_potentiometer_v2_make_new, VEX_THREE_WIRE_POTENTIOMETER, 0, 1)
MP_THREE_WIRE_MAKE_NEW(mp_pwm_make_new, VEX_THREE_WIRE_PWM, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_sonar_make_new, VEX_THREE_WIRE_RANGE_FINDER, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_accelerometer_make_new, VEX_THREE_WIRE_ACCELEROMETER, 0, 0)
MP_THREE_WIRE_MAKE_NEW(mp_addressable_led_make_new, VEX_THREE_WIRE_ADDRESSABLE_LED, 0, 0)

static mp_obj_t mp_object_sensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    if (mp_obj_get_type(args[0]) != &mp_three_wire_port_type){
        mp_raise_TypeError("port must be a Brain three_wire_port");
    }
    mp_three_wire_port_obj_t *port = MP_OBJ_TO_PTR(args[0]);
    mp_three_wire_device_obj_t *self = mp_obj_malloc(mp_three_wire_device_obj_t, type);
    self->cpp_handle = vex_three_wire_create(VEX_THREE_WIRE_OBJECT_SENSOR, port->port,
                                              n_args > 1 && mp_obj_is_true(args[1]));
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_three_wire_value(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : MP_VEX_ANALOG_TWELVEBIT;
    if (units != MP_VEX_PERCENT && units != MP_VEX_ANALOG_EIGHTBIT &&
        units != MP_VEX_ANALOG_TENBIT && units != MP_VEX_ANALOG_TWELVEBIT){
        mp_raise_ValueError("invalid analog units");
    }
    return mp_obj_new_int(vex_three_wire_value(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_value_obj, 1, 2, mp_three_wire_value);

static mp_obj_t mp_three_wire_digital_value(mp_obj_t self_ptr){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_int(vex_three_wire_value(self->cpp_handle, MP_VEX_ANALOG_TWELVEBIT));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_three_wire_digital_value_obj, mp_three_wire_digital_value);

static mp_obj_t mp_three_wire_pwm_value(mp_obj_t self_ptr){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_float(vex_three_wire_pwm_value(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_three_wire_pwm_value_obj, mp_three_wire_pwm_value);

static mp_obj_t mp_three_wire_watch(mp_obj_t self_ptr, mp_obj_t callback, mp_obj_t callback_args, int event, bool args_provided){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    mp_obj_t callback_data = mp_vex_callback_data(callback, callback_args, !args_provided, false);
    vex_three_wire_watch(self->cpp_handle, event, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));
    return mp_const_none;
}

#define MP_THREE_WIRE_CALLBACK(name, event) \
static mp_obj_t name(size_t n_args, const mp_obj_t *args){ \
    return mp_three_wire_watch(args[0], args[1], n_args > 2 ? args[2] : mp_const_none, event, n_args > 2); \
} \
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(name##_obj, 2, 3, name);

MP_THREE_WIRE_CALLBACK(mp_three_wire_changed, 0)
MP_THREE_WIRE_CALLBACK(mp_three_wire_high, 1)
MP_THREE_WIRE_CALLBACK(mp_three_wire_low, 2)
MP_THREE_WIRE_CALLBACK(mp_three_wire_pressed, 3)
MP_THREE_WIRE_CALLBACK(mp_three_wire_released, 4)
MP_THREE_WIRE_CALLBACK(mp_three_wire_light_changed, 5)
MP_THREE_WIRE_CALLBACK(mp_three_wire_pot_changed, 6)
MP_THREE_WIRE_CALLBACK(mp_three_wire_object_detected, 7)
MP_THREE_WIRE_CALLBACK(mp_three_wire_object_lost, 8)
MP_THREE_WIRE_CALLBACK(mp_three_wire_object_changed, 9)
MP_THREE_WIRE_CALLBACK(mp_three_wire_accelerometer_changed, 10)

static mp_obj_t mp_three_wire_pressing(mp_obj_t self_ptr){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_bool(vex_three_wire_pressing(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_three_wire_pressing_obj, mp_three_wire_pressing);

static mp_obj_t mp_three_wire_acceleration(mp_obj_t self_ptr){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_float(vex_three_wire_acceleration(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_three_wire_acceleration_obj, mp_three_wire_acceleration);

static mp_obj_t mp_three_wire_brightness(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args > 1 && mp_obj_get_int(args[1]) != MP_VEX_PERCENT){
        mp_raise_ValueError("units must be PERCENT");
    }
    return mp_obj_new_float(vex_three_wire_brightness(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_brightness_obj, 1, 2, mp_three_wire_brightness);

static mp_obj_t mp_three_wire_reflectivity(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args > 1 && mp_obj_get_int(args[1]) != MP_VEX_PERCENT){
        mp_raise_ValueError("units must be PERCENT");
    }
    return mp_obj_new_int(vex_three_wire_reflectivity(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_reflectivity_obj, 1, 2, mp_three_wire_reflectivity);

static mp_obj_t mp_three_wire_object_reflectivity(mp_obj_t self_ptr){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_int(vex_three_wire_reflectivity(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_three_wire_object_reflectivity_obj, mp_three_wire_object_reflectivity);

static mp_obj_t mp_three_wire_is_object_detected(mp_obj_t self_ptr){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_bool(vex_three_wire_is_object_detected(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_three_wire_is_object_detected_obj, mp_three_wire_is_object_detected);

static mp_obj_t mp_three_wire_set_threshold(mp_obj_t self_ptr, mp_obj_t value_obj){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int threshold = mp_obj_get_int(value_obj);
    if (threshold < 0 || threshold > 100){
        mp_raise_ValueError("threshold must be from 0 to 100");
    }
    vex_three_wire_set_threshold(self->cpp_handle, threshold);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_three_wire_set_threshold_obj, mp_three_wire_set_threshold);

static mp_obj_t mp_three_wire_angle(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : MP_VEX_DEGREES;
    if (units != MP_VEX_DEGREES && units != MP_VEX_PERCENT){
        mp_raise_ValueError("units must be DEGREES or PERCENT");
    }
    return mp_obj_new_float(vex_three_wire_angle(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_angle_obj, 1, 2, mp_three_wire_angle);

static mp_obj_t mp_three_wire_heading(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : MP_VEX_DEGREES;
    if (units != MP_VEX_DEGREES) mp_raise_ValueError("units must be DEGREES");
    return mp_obj_new_float(vex_three_wire_heading(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_heading_obj, 1, 2, mp_three_wire_heading);

static mp_obj_t mp_three_wire_rotation(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : MP_VEX_DEGREES;
    if (units != MP_VEX_DEGREES) mp_raise_ValueError("units must be DEGREES");
    return mp_obj_new_float(vex_three_wire_rotation(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_rotation_obj, 1, 2, mp_three_wire_rotation);

static mp_obj_t mp_three_wire_set_heading(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    float value = mp_obj_get_float(args[1]);
    int units = n_args > 2 ? mp_obj_get_int(args[2]) : MP_VEX_DEGREES;
    if (units != MP_VEX_DEGREES) mp_raise_ValueError("units must be DEGREES");
    vex_three_wire_set_heading(self->cpp_handle, value, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_set_heading_obj, 2, 3, mp_three_wire_set_heading);

static mp_obj_t mp_three_wire_set_rotation(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    float value = mp_obj_get_float(args[1]);
    int units = n_args > 2 ? mp_obj_get_int(args[2]) : MP_VEX_DEGREES;
    if (units != MP_VEX_DEGREES) mp_raise_ValueError("units must be DEGREES");
    vex_three_wire_set_rotation(self->cpp_handle, value, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_set_rotation_obj, 2, 3, mp_three_wire_set_rotation);

static mp_obj_t mp_three_wire_calibrate(mp_obj_t self_ptr){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    vex_three_wire_calibrate(self->cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_three_wire_calibrate_obj, mp_three_wire_calibrate);

static mp_obj_t mp_three_wire_set_position(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    float value = mp_obj_get_float(args[1]);
    int units = n_args > 2 ? mp_obj_get_int(args[2]) : MP_VEX_DEGREES;
    mp_vex_check_angle_units(units);
    vex_three_wire_set_position(self->cpp_handle, value, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_set_position_obj, 2, 3, mp_three_wire_set_position);

static mp_obj_t mp_three_wire_reset_position(mp_obj_t self_ptr){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    vex_three_wire_reset_position(self->cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_three_wire_reset_position_obj, mp_three_wire_reset_position);

static mp_obj_t mp_three_wire_position(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : MP_VEX_DEGREES;
    mp_vex_check_angle_units(units);
    return mp_obj_new_float(vex_three_wire_position(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_position_obj, 1, 2, mp_three_wire_position);

static mp_obj_t mp_three_wire_velocity(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : MP_VEX_RPM;
    mp_vex_check_velocity_units(units, false);
    return mp_obj_new_float(vex_three_wire_velocity(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_velocity_obj, 1, 2, mp_three_wire_velocity);

static mp_obj_t mp_three_wire_found_object(mp_obj_t self_ptr){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_bool(vex_three_wire_found_object(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_three_wire_found_object_obj, mp_three_wire_found_object);

static mp_obj_t mp_three_wire_distance(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_arg_check_num(n_args, 0, 2, 2, false);
    int units = mp_obj_get_int(args[1]);
    if (units != MP_VEX_MM && units != MP_VEX_INCHES){
        mp_raise_ValueError("units must be MM or INCHES");
    }
    return mp_obj_new_float(vex_three_wire_distance(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_distance_obj, 2, 2, mp_three_wire_distance);

static mp_obj_t mp_three_wire_set(mp_obj_t self_ptr, mp_obj_t value_obj){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    vex_three_wire_set(self->cpp_handle, mp_obj_is_true(value_obj));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_three_wire_set_obj, mp_three_wire_set);

static mp_obj_t mp_three_wire_set_state(mp_obj_t self_ptr, mp_obj_t value_obj){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    float value = mp_obj_get_float(value_obj);
    if (value < -100.0f || value > 100.0f || value != value){
        mp_raise_ValueError("PWM value must be from -100 to 100");
    }
    vex_three_wire_set_state(self->cpp_handle, value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_three_wire_set_state_obj, mp_three_wire_set_state);

static mp_obj_t mp_three_wire_on(mp_obj_t self_ptr){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    vex_three_wire_on(self->cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_three_wire_on_obj, mp_three_wire_on);

static mp_obj_t mp_three_wire_off(mp_obj_t self_ptr){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    vex_three_wire_off(self->cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_three_wire_off_obj, mp_three_wire_off);

static mp_obj_t mp_three_wire_set_colors(size_t n_args, const mp_obj_t *args){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    size_t count;
    mp_obj_t *items;
    mp_obj_get_array(args[1], &count, &items);
    mp_int_t offset_arg = n_args > 2 ? mp_obj_get_int(args[2]) : 0;
    if (offset_arg < 0){
        mp_raise_ValueError("offset must be non-negative");
    }
    const size_t offset = offset_arg;
    uint32_t *colors = m_new(uint32_t, count);
    for (size_t i = 0; i < count; ++i){
        if (mp_obj_get_type(items[i]) == &mp_color_type){
            colors[i] = ((mp_color_obj_t *)MP_OBJ_TO_PTR(items[i]))->value;
        } else {
            colors[i] = mp_obj_get_int(items[i]);
        }
    }
    vex_three_wire_set_colors(self->cpp_handle, colors, count, offset);
    m_del(uint32_t, colors, count);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_three_wire_set_colors_obj, 2, 3, mp_three_wire_set_colors);

static mp_obj_t mp_three_wire_clear(mp_obj_t self_ptr){
    mp_three_wire_device_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    vex_three_wire_clear(self->cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_three_wire_clear_obj, mp_three_wire_clear);

#define MP_THREE_WIRE_DICT(name, ...) \
static const mp_rom_map_elem_t name##_table[] = { __VA_ARGS__ }; \
static MP_DEFINE_CONST_DICT(name, name##_table)

MP_THREE_WIRE_DICT(mp_analog_in_locals,
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&mp_three_wire_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_changed), MP_ROM_PTR(&mp_three_wire_changed_obj) });
MP_THREE_WIRE_DICT(mp_digital_in_locals,
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&mp_three_wire_digital_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_high), MP_ROM_PTR(&mp_three_wire_high_obj) },
    { MP_ROM_QSTR(MP_QSTR_low), MP_ROM_PTR(&mp_three_wire_low_obj) });
MP_THREE_WIRE_DICT(mp_switch_locals,
    { MP_ROM_QSTR(MP_QSTR_pressing), MP_ROM_PTR(&mp_three_wire_pressing_obj) },
    { MP_ROM_QSTR(MP_QSTR_pressed), MP_ROM_PTR(&mp_three_wire_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_released), MP_ROM_PTR(&mp_three_wire_released_obj) });
MP_THREE_WIRE_DICT(mp_encoder_locals,
    { MP_ROM_QSTR(MP_QSTR_set_position), MP_ROM_PTR(&mp_three_wire_set_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_position), MP_ROM_PTR(&mp_three_wire_reset_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_position), MP_ROM_PTR(&mp_three_wire_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_velocity), MP_ROM_PTR(&mp_three_wire_velocity_obj) });
MP_THREE_WIRE_DICT(mp_gyro_locals,
    { MP_ROM_QSTR(MP_QSTR_calibrate), MP_ROM_PTR(&mp_three_wire_calibrate_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_heading), MP_ROM_PTR(&mp_three_wire_set_heading_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_rotation), MP_ROM_PTR(&mp_three_wire_set_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_heading), MP_ROM_PTR(&mp_three_wire_heading_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotation), MP_ROM_PTR(&mp_three_wire_rotation_obj) });
MP_THREE_WIRE_DICT(mp_led_locals,
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&mp_three_wire_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&mp_three_wire_off_obj) });
MP_THREE_WIRE_DICT(mp_light_locals,
    { MP_ROM_QSTR(MP_QSTR_brightness), MP_ROM_PTR(&mp_three_wire_brightness_obj) },
    { MP_ROM_QSTR(MP_QSTR_changed), MP_ROM_PTR(&mp_three_wire_light_changed_obj) });
MP_THREE_WIRE_DICT(mp_line_locals, { MP_ROM_QSTR(MP_QSTR_reflectivity), MP_ROM_PTR(&mp_three_wire_reflectivity_obj) });
MP_THREE_WIRE_DICT(mp_object_sensor_locals,
    { MP_ROM_QSTR(MP_QSTR_reflectivity), MP_ROM_PTR(&mp_three_wire_object_reflectivity_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_object_detected), MP_ROM_PTR(&mp_three_wire_is_object_detected_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_threshold), MP_ROM_PTR(&mp_three_wire_set_threshold_obj) },
    { MP_ROM_QSTR(MP_QSTR_object_detected), MP_ROM_PTR(&mp_three_wire_object_detected_obj) },
    { MP_ROM_QSTR(MP_QSTR_object_lost), MP_ROM_PTR(&mp_three_wire_object_lost_obj) },
    { MP_ROM_QSTR(MP_QSTR_changed), MP_ROM_PTR(&mp_three_wire_object_changed_obj) });
MP_THREE_WIRE_DICT(mp_potentiometer_locals,
    { MP_ROM_QSTR(MP_QSTR_angle), MP_ROM_PTR(&mp_three_wire_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_changed), MP_ROM_PTR(&mp_three_wire_pot_changed_obj) });
MP_THREE_WIRE_DICT(mp_pwm_locals,
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&mp_three_wire_pwm_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_state), MP_ROM_PTR(&mp_three_wire_set_state_obj) });
MP_THREE_WIRE_DICT(mp_sonar_locals,
    { MP_ROM_QSTR(MP_QSTR_found_object), MP_ROM_PTR(&mp_three_wire_found_object_obj) },
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&mp_three_wire_distance_obj) });
MP_THREE_WIRE_DICT(mp_accelerometer_locals,
    { MP_ROM_QSTR(MP_QSTR_acceleration), MP_ROM_PTR(&mp_three_wire_acceleration_obj) },
    { MP_ROM_QSTR(MP_QSTR_changed), MP_ROM_PTR(&mp_three_wire_accelerometer_changed_obj) });
MP_THREE_WIRE_DICT(mp_addressable_led_locals,
    { MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&mp_three_wire_set_colors_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&mp_three_wire_clear_obj) },
);

#define MP_DEFINE_THREE_WIRE_TYPE(type_name, qstr_name, constructor, locals) \
MP_DEFINE_CONST_OBJ_TYPE(type_name, qstr_name, MP_TYPE_FLAG_NONE, make_new, constructor, locals_dict, &locals)

MP_DEFINE_THREE_WIRE_TYPE(mp_analog_in_type, MP_QSTR_analogin, mp_analog_in_make_new, mp_analog_in_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_bumper_type, MP_QSTR_bumper, mp_bumper_make_new, mp_switch_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_digital_in_type, MP_QSTR_digitalin, mp_digital_in_make_new, mp_digital_in_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_encoder_type, MP_QSTR_encoder, mp_encoder_make_new, mp_encoder_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_gyro_type, MP_QSTR_gyro, mp_gyro_make_new, mp_gyro_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_led_type, MP_QSTR_led, mp_led_make_new, mp_led_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_light_type, MP_QSTR_light, mp_light_make_new, mp_light_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_limit_type, MP_QSTR_limit, mp_limit_make_new, mp_switch_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_line_type, MP_QSTR_line, mp_line_make_new, mp_line_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_object_sensor_type, MP_QSTR_objectdetector, mp_object_sensor_make_new, mp_object_sensor_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_potentiometer_type, MP_QSTR_potentiometer, mp_potentiometer_make_new, mp_potentiometer_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_potentiometer_v2_type, MP_QSTR_potentiometerv2, mp_potentiometer_v2_make_new, mp_potentiometer_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_pwm_type, MP_QSTR_pwm, mp_pwm_make_new, mp_pwm_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_sonar_type, MP_QSTR_sonar, mp_sonar_make_new, mp_sonar_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_accelerometer_type, MP_QSTR_accelerometer, mp_accelerometer_make_new, mp_accelerometer_locals);
MP_DEFINE_THREE_WIRE_TYPE(mp_addressable_led_type, MP_QSTR_addressableled, mp_addressable_led_make_new, mp_addressable_led_locals);

static void mp_vex_check_angle_units(int units){
    if (units != MP_VEX_DEGREES && units != MP_VEX_TURNS){
        mp_raise_ValueError("units must be DEGREES or TURNS");
    }
}

static void mp_vex_check_velocity_units(int units, bool allow_percent){
    if (units != MP_VEX_RPM && units != MP_VEX_DPS && (!allow_percent || units != MP_VEX_PERCENT)){
        mp_raise_ValueError("invalid velocity units");
    }
}

static mp_obj_t mp_rotation_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    int port = mp_obj_get_int(args[0]);
    bool reversed = n_args > 1 ? mp_obj_is_true(args[1]) : false;
    mp_rotation_obj_t *self = mp_obj_malloc(mp_rotation_obj_t, type);
    self->cpp_handle = vex_rotation_create(port, reversed);
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_rotation_set_position(size_t n_args, const mp_obj_t *args){
    mp_rotation_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    float position = mp_obj_get_float(args[1]);
    int units = n_args > 2 ? mp_obj_get_int(args[2]) : MP_VEX_DEGREES;
    mp_vex_check_angle_units(units);
    vex_rotation_set_position(self->cpp_handle, position, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_rotation_set_position_obj, 2, 3, mp_rotation_set_position);

static mp_obj_t mp_rotation_angle(size_t n_args, const mp_obj_t *args){
    mp_rotation_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : MP_VEX_DEGREES;
    mp_vex_check_angle_units(units);
    return mp_obj_new_float(vex_rotation_angle(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_rotation_angle_obj, 1, 2, mp_rotation_angle);

static mp_obj_t mp_rotation_position(size_t n_args, const mp_obj_t *args){
    mp_rotation_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : MP_VEX_DEGREES;
    mp_vex_check_angle_units(units);
    return mp_obj_new_float(vex_rotation_position(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_rotation_position_obj, 1, 2, mp_rotation_position);

static mp_obj_t mp_rotation_velocity(mp_obj_t self_ptr, mp_obj_t units_obj){
    mp_rotation_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int units = mp_obj_get_int(units_obj);
    mp_vex_check_velocity_units(units, false);
    return mp_obj_new_float(vex_rotation_velocity(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_rotation_velocity_obj, mp_rotation_velocity);

static mp_obj_t mp_rotation_reset_position(mp_obj_t self_ptr){
    mp_rotation_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    vex_rotation_reset_position(self->cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_rotation_reset_position_obj, mp_rotation_reset_position);

static mp_obj_t mp_rotation_set_reversed(mp_obj_t self_ptr, mp_obj_t value_obj){
    mp_rotation_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    vex_rotation_set_reversed(self->cpp_handle, mp_obj_is_true(value_obj));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_rotation_set_reversed_obj, mp_rotation_set_reversed);

static mp_obj_t mp_rotation_changed(size_t n_args, const mp_obj_t *args){
    mp_rotation_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t callback = args[1];
    mp_obj_t callback_args = n_args > 2 ? args[2] : mp_const_none;
    mp_obj_t callback_data = mp_vex_callback_data(callback, callback_args, n_args == 2, false);
    vex_rotation_changed(self->cpp_handle, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_rotation_changed_obj, 2, 3, mp_rotation_changed);

static mp_obj_t mp_rotation_installed(mp_obj_t self_ptr){
    mp_rotation_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return vex_rotation_installed(self->cpp_handle) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_rotation_installed_obj, mp_rotation_installed);

static const mp_rom_map_elem_t mp_rotation_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_position), MP_ROM_PTR(&mp_rotation_set_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_angle), MP_ROM_PTR(&mp_rotation_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_position), MP_ROM_PTR(&mp_rotation_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_velocity), MP_ROM_PTR(&mp_rotation_velocity_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_position), MP_ROM_PTR(&mp_rotation_reset_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_reversed), MP_ROM_PTR(&mp_rotation_set_reversed_obj) },
    { MP_ROM_QSTR(MP_QSTR_changed), MP_ROM_PTR(&mp_rotation_changed_obj) },
    { MP_ROM_QSTR(MP_QSTR_installed), MP_ROM_PTR(&mp_rotation_installed_obj) },
};
static MP_DEFINE_CONST_DICT(mp_rotation_locals_dict, mp_rotation_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_rotation_type,
    MP_QSTR_rotation,
    MP_TYPE_FLAG_NONE,
    make_new, mp_rotation_make_new,
    locals_dict, &mp_rotation_locals_dict
);

static mp_obj_t mp_inertial_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    mp_inertial_obj_t *self = mp_obj_malloc(mp_inertial_obj_t, type);
    self->cpp_handle = inertial_create(mp_obj_get_int(args[0]));
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_inertial_heading(size_t n_args, const mp_obj_t *args){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : MP_VEX_DEGREES;
    mp_vex_check_angle_units(units);
    return mp_obj_new_float(inertial_heading(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_inertial_heading_obj, 1, 2, mp_inertial_heading);

static mp_obj_t mp_inertial_rotation(size_t n_args, const mp_obj_t *args){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : MP_VEX_DEGREES;
    mp_vex_check_angle_units(units);
    return mp_obj_new_float(inertial_rotation(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_inertial_rotation_obj, 1, 2, mp_inertial_rotation);

static mp_obj_t mp_inertial_gyro_rate(size_t n_args, const mp_obj_t *args){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int axis = mp_obj_get_int(args[1]);
    int units = n_args > 2 ? mp_obj_get_int(args[2]) : MP_VEX_DPS;
    if (axis != MP_VEX_AXIS_X && axis != MP_VEX_AXIS_Y && axis != MP_VEX_AXIS_Z){
        mp_raise_ValueError("axis must be XAXIS, YAXIS, or ZAXIS");
    }
    mp_vex_check_velocity_units(units, true);
    return mp_obj_new_float(inertial_gyro_rate(self->cpp_handle, axis, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_inertial_gyro_rate_obj, 2, 3, mp_inertial_gyro_rate);

static mp_obj_t mp_inertial_orientation(size_t n_args, const mp_obj_t *args){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int axis = mp_obj_get_int(args[1]);
    int units = n_args > 2 ? mp_obj_get_int(args[2]) : MP_VEX_DEGREES;
    if (axis != MP_VEX_PITCH && axis != MP_VEX_ROLL && axis != MP_VEX_YAW){
        mp_raise_ValueError("axis must be PITCH, ROLL, or YAW");
    }
    mp_vex_check_angle_units(units);
    return mp_obj_new_float(inertial_orientation(self->cpp_handle, axis, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_inertial_orientation_obj, 2, 3, mp_inertial_orientation);

static mp_obj_t mp_inertial_set_heading(size_t n_args, const mp_obj_t *args){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    float value = mp_obj_get_float(args[1]);
    int units = n_args > 2 ? mp_obj_get_int(args[2]) : MP_VEX_DEGREES;
    mp_vex_check_angle_units(units);
    inertial_set_heading(self->cpp_handle, value, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_inertial_set_heading_obj, 2, 3, mp_inertial_set_heading);

static mp_obj_t mp_inertial_set_rotation(size_t n_args, const mp_obj_t *args){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    float value = mp_obj_get_float(args[1]);
    int units = n_args > 2 ? mp_obj_get_int(args[2]) : MP_VEX_DEGREES;
    mp_vex_check_angle_units(units);
    inertial_set_rotation(self->cpp_handle, value, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_inertial_set_rotation_obj, 2, 3, mp_inertial_set_rotation);

static mp_obj_t mp_inertial_calibrate(mp_obj_t self_ptr){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    inertial_calibrate(self->cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_inertial_calibrate_obj, mp_inertial_calibrate);

static mp_obj_t mp_inertial_reset_heading(mp_obj_t self_ptr){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    inertial_reset_heading(self->cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_inertial_reset_heading_obj, mp_inertial_reset_heading);

static mp_obj_t mp_inertial_reset_rotation(mp_obj_t self_ptr){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    inertial_reset_rotation(self->cpp_handle);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_inertial_reset_rotation_obj, mp_inertial_reset_rotation);

static mp_obj_t mp_inertial_is_calibrating(mp_obj_t self_ptr){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return inertial_is_calibrating(self->cpp_handle) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_inertial_is_calibrating_obj, mp_inertial_is_calibrating);

static mp_obj_t mp_inertial_changed(size_t n_args, const mp_obj_t *args){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t callback = args[1];
    mp_obj_t callback_args = n_args > 2 ? args[2] : mp_const_none;
    mp_obj_t callback_data = mp_vex_callback_data(callback, callback_args, n_args == 2, false);
    inertial_changed(self->cpp_handle, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_inertial_changed_obj, 2, 3, mp_inertial_changed);

static mp_obj_t mp_inertial_installed(mp_obj_t self_ptr){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return inertial_installed(self->cpp_handle) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_inertial_installed_obj, mp_inertial_installed);

static mp_obj_t mp_inertial_acceleration(mp_obj_t self_ptr, mp_obj_t axis_obj){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int axis = mp_obj_get_int(axis_obj);
    if (axis != MP_VEX_AXIS_X && axis != MP_VEX_AXIS_Y && axis != MP_VEX_AXIS_Z){
        mp_raise_ValueError("axis must be XAXIS, YAXIS, or ZAXIS");
    }
    return mp_obj_new_float(inertial_acceleration(self->cpp_handle, axis));
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_inertial_acceleration_obj, mp_inertial_acceleration);

static mp_obj_t mp_inertial_collision(size_t n_args, const mp_obj_t *args){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t callback = args[1];
    mp_obj_t callback_args = n_args > 2 ? args[2] : mp_const_none;
    mp_obj_t callback_data = mp_vex_callback_data(callback, callback_args, n_args == 2, false);
    inertial_collision(self->cpp_handle, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_inertial_collision_obj, 2, 3, mp_inertial_collision);

static mp_obj_t mp_inertial_set_turn_type(mp_obj_t self_ptr, mp_obj_t turn_type_obj){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int turn_type = mp_obj_get_int(turn_type_obj);
    if (turn_type != MP_VEX_RIGHT && turn_type != MP_VEX_LEFT){
        mp_raise_ValueError("turntype must be RIGHT or LEFT");
    }
    inertial_set_turn_type(self->cpp_handle, turn_type);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_inertial_set_turn_type_obj, mp_inertial_set_turn_type);

static mp_obj_t mp_inertial_get_turn_type(mp_obj_t self_ptr){
    mp_inertial_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_int(inertial_get_turn_type(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_inertial_get_turn_type_obj, mp_inertial_get_turn_type);

static const mp_rom_map_elem_t mp_inertial_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_heading), MP_ROM_PTR(&mp_inertial_heading_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotation), MP_ROM_PTR(&mp_inertial_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_gyro_rate), MP_ROM_PTR(&mp_inertial_gyro_rate_obj) },
    { MP_ROM_QSTR(MP_QSTR_orientation), MP_ROM_PTR(&mp_inertial_orientation_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_heading), MP_ROM_PTR(&mp_inertial_set_heading_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_rotation), MP_ROM_PTR(&mp_inertial_set_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_calibrate), MP_ROM_PTR(&mp_inertial_calibrate_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_heading), MP_ROM_PTR(&mp_inertial_reset_heading_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_rotation), MP_ROM_PTR(&mp_inertial_reset_rotation_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_calibrating), MP_ROM_PTR(&mp_inertial_is_calibrating_obj) },
    { MP_ROM_QSTR(MP_QSTR_changed), MP_ROM_PTR(&mp_inertial_changed_obj) },
    { MP_ROM_QSTR(MP_QSTR_installed), MP_ROM_PTR(&mp_inertial_installed_obj) },
    { MP_ROM_QSTR(MP_QSTR_acceleration), MP_ROM_PTR(&mp_inertial_acceleration_obj) },
    { MP_ROM_QSTR(MP_QSTR_collision), MP_ROM_PTR(&mp_inertial_collision_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_turn_type), MP_ROM_PTR(&mp_inertial_set_turn_type_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_turn_type), MP_ROM_PTR(&mp_inertial_get_turn_type_obj) },
};
static MP_DEFINE_CONST_DICT(mp_inertial_locals_dict, mp_inertial_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_inertial_type,
    MP_QSTR_inertial,
    MP_TYPE_FLAG_NONE,
    make_new, mp_inertial_make_new,
    locals_dict, &mp_inertial_locals_dict
);

static mp_obj_t mp_optical_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    mp_optical_obj_t *self = mp_obj_malloc(mp_optical_obj_t, type);
    self->cpp_handle = optical_create(mp_obj_get_int(args[0]));
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_optical_set_light(mp_obj_t self_ptr, mp_obj_t state_obj){
    mp_optical_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int state = mp_obj_get_int(state_obj);
    if (state != 0 && state != 1){
        mp_raise_ValueError("state must be LEDStateType.ON or LEDStateType.OFF");
    }
    optical_set_light(self->cpp_handle, state != 0);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_optical_set_light_obj, mp_optical_set_light);

static mp_obj_t mp_optical_set_light_power(size_t n_args, const mp_obj_t *args){
    mp_optical_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int percent = mp_obj_get_int(args[1]);
    int units = n_args > 2 ? mp_obj_get_int(args[2]) : MP_VEX_PERCENT;
    if (units != MP_VEX_PERCENT){
        mp_raise_ValueError("units must be PERCENT");
    }
    if (percent < 0 || percent > 100){
        mp_raise_ValueError("percent must be from 0 to 100");
    }
    optical_set_light_power(self->cpp_handle, percent);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_optical_set_light_power_obj, 2, 3, mp_optical_set_light_power);

static mp_obj_t mp_optical_is_near_object(mp_obj_t self_ptr){
    mp_optical_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return optical_is_near_object(self->cpp_handle) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_optical_is_near_object_obj, mp_optical_is_near_object);

static mp_obj_t mp_optical_color(mp_obj_t self_ptr){
    mp_optical_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_int(optical_color(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_optical_color_obj, mp_optical_color);

static mp_obj_t mp_optical_brightness(mp_obj_t self_ptr){
    mp_optical_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_float(optical_brightness(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_optical_brightness_obj, mp_optical_brightness);

static mp_obj_t mp_optical_hue(mp_obj_t self_ptr){
    mp_optical_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_float(optical_hue(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_optical_hue_obj, mp_optical_hue);

static mp_obj_t mp_optical_object_detected(size_t n_args, const mp_obj_t *args){
    mp_optical_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t callback_args = n_args > 2 ? args[2] : mp_const_none;
    mp_obj_t callback_data = mp_vex_callback_data(args[1], callback_args, n_args == 2, false);
    optical_object_detected(self->cpp_handle, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_optical_object_detected_obj, 2, 3, mp_optical_object_detected);

static mp_obj_t mp_optical_object_lost(size_t n_args, const mp_obj_t *args){
    mp_optical_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t callback_args = n_args > 2 ? args[2] : mp_const_none;
    mp_obj_t callback_data = mp_vex_callback_data(args[1], callback_args, n_args == 2, false);
    optical_object_lost(self->cpp_handle, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_optical_object_lost_obj, 2, 3, mp_optical_object_lost);

static const mp_rom_map_elem_t mp_optical_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_light), MP_ROM_PTR(&mp_optical_set_light_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_light_power), MP_ROM_PTR(&mp_optical_set_light_power_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_near_object), MP_ROM_PTR(&mp_optical_is_near_object_obj) },
    { MP_ROM_QSTR(MP_QSTR_color), MP_ROM_PTR(&mp_optical_color_obj) },
    { MP_ROM_QSTR(MP_QSTR_brightness), MP_ROM_PTR(&mp_optical_brightness_obj) },
    { MP_ROM_QSTR(MP_QSTR_hue), MP_ROM_PTR(&mp_optical_hue_obj) },
    { MP_ROM_QSTR(MP_QSTR_object_detected), MP_ROM_PTR(&mp_optical_object_detected_obj) },
    { MP_ROM_QSTR(MP_QSTR_object_lost), MP_ROM_PTR(&mp_optical_object_lost_obj) },
};
static MP_DEFINE_CONST_DICT(mp_optical_locals_dict, mp_optical_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_optical_type,
    MP_QSTR_optical,
    MP_TYPE_FLAG_NONE,
    make_new, mp_optical_make_new,
    locals_dict, &mp_optical_locals_dict
);

static mp_obj_t mp_distance_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    mp_distance_obj_t *self = mp_obj_malloc(mp_distance_obj_t, type);
    self->cpp_handle = distance_create(mp_obj_get_int(args[0]));
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_distance_object_distance(size_t n_args, const mp_obj_t *args){
    mp_distance_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int units = n_args > 1 ? mp_obj_get_int(args[1]) : MP_VEX_MM;
    if (units != MP_VEX_MM && units != MP_VEX_INCHES){
        mp_raise_ValueError("units must be MM or INCHES");
    }
    return mp_obj_new_float(distance_object_distance(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_distance_object_distance_obj, 1, 2, mp_distance_object_distance);

static mp_obj_t mp_distance_object_velocity(mp_obj_t self_ptr){
    mp_distance_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_float(distance_object_velocity(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_distance_object_velocity_obj, mp_distance_object_velocity);

static mp_obj_t mp_distance_object_size(mp_obj_t self_ptr){
    mp_distance_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_int(distance_object_size(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_distance_object_size_obj, mp_distance_object_size);

static mp_obj_t mp_distance_is_object_detected(mp_obj_t self_ptr){
    mp_distance_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return distance_is_object_detected(self->cpp_handle) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_distance_is_object_detected_obj, mp_distance_is_object_detected);

static mp_obj_t mp_distance_changed(size_t n_args, const mp_obj_t *args){
    mp_distance_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t callback_args = n_args > 2 ? args[2] : mp_const_none;
    mp_obj_t callback_data = mp_vex_callback_data(args[1], callback_args, n_args == 2, false);
    distance_changed(self->cpp_handle, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_distance_changed_obj, 2, 3, mp_distance_changed);

static const mp_rom_map_elem_t mp_distance_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_object_distance), MP_ROM_PTR(&mp_distance_object_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_object_velocity), MP_ROM_PTR(&mp_distance_object_velocity_obj) },
    { MP_ROM_QSTR(MP_QSTR_object_size), MP_ROM_PTR(&mp_distance_object_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_object_detected), MP_ROM_PTR(&mp_distance_is_object_detected_obj) },
    { MP_ROM_QSTR(MP_QSTR_changed), MP_ROM_PTR(&mp_distance_changed_obj) },
};
static MP_DEFINE_CONST_DICT(mp_distance_locals_dict, mp_distance_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_distance_type,
    MP_QSTR_distance,
    MP_TYPE_FLAG_NONE,
    make_new, mp_distance_make_new,
    locals_dict, &mp_distance_locals_dict
);

static mp_obj_t mp_gps_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    mp_gps_obj_t *self = mp_obj_malloc(mp_gps_obj_t, type);
    self->cpp_handle = vex_gps_create(mp_obj_get_int(args[0]));
    return MP_OBJ_FROM_PTR(self);
}

static void mp_gps_check_distance_units(int units){
    if (units != MP_VEX_MM && units != MP_VEX_INCHES){
        mp_raise_ValueError("units must be MM or INCHES");
    }
}

static mp_obj_t mp_gps_set_origin(size_t n_args, const mp_obj_t *args){
    mp_gps_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    const int units = mp_obj_get_int(args[3]);
    mp_gps_check_distance_units(units);
    vex_gps_set_origin(self->cpp_handle, mp_obj_get_float(args[1]), mp_obj_get_float(args[2]), units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_gps_set_origin_obj, 4, 4, mp_gps_set_origin);

static mp_obj_t mp_gps_set_location(size_t n_args, const mp_obj_t *args){
    mp_gps_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    const int distance_units = mp_obj_get_int(args[3]);
    const int heading_units = mp_obj_get_int(args[5]);
    mp_gps_check_distance_units(distance_units);
    if (heading_units != MP_VEX_DEGREES){
        mp_raise_ValueError("heading_units must be DEGREES");
    }
    vex_gps_set_location(self->cpp_handle, mp_obj_get_float(args[1]), mp_obj_get_float(args[2]),
                         distance_units, mp_obj_get_float(args[4]), heading_units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_gps_set_location_obj, 6, 6, mp_gps_set_location);

static mp_obj_t mp_gps_x_position(mp_obj_t self_ptr, mp_obj_t units_obj){
    mp_gps_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    const int units = mp_obj_get_int(units_obj);
    mp_gps_check_distance_units(units);
    return mp_obj_new_float(vex_gps_x_position(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_gps_x_position_obj, mp_gps_x_position);

static mp_obj_t mp_gps_y_position(mp_obj_t self_ptr, mp_obj_t units_obj){
    mp_gps_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    const int units = mp_obj_get_int(units_obj);
    mp_gps_check_distance_units(units);
    return mp_obj_new_float(vex_gps_y_position(self->cpp_handle, units));
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_gps_y_position_obj, mp_gps_y_position);

static mp_obj_t mp_gps_heading(mp_obj_t self_ptr){
    mp_gps_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    return mp_obj_new_float(vex_gps_heading(self->cpp_handle));
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_gps_heading_obj, mp_gps_heading);

static mp_obj_t mp_gps_acceleration(mp_obj_t self_ptr, mp_obj_t axis_obj){
    mp_gps_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    const int axis = mp_obj_get_int(axis_obj);
    if (axis != MP_VEX_AXIS_X && axis != MP_VEX_AXIS_Y && axis != MP_VEX_AXIS_Z){
        mp_raise_ValueError("axis must be XAXIS, YAXIS, or ZAXIS");
    }
    return mp_obj_new_float(vex_gps_acceleration(self->cpp_handle, axis));
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_gps_acceleration_obj, mp_gps_acceleration);

static mp_obj_t mp_gps_gyro_rate(mp_obj_t self_ptr, mp_obj_t axis_obj, mp_obj_t units_obj){
    mp_gps_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    const int axis = mp_obj_get_int(axis_obj);
    const int units = mp_obj_get_int(units_obj);
    if (axis != MP_VEX_AXIS_X && axis != MP_VEX_AXIS_Y && axis != MP_VEX_AXIS_Z){
        mp_raise_ValueError("axis must be XAXIS, YAXIS, or ZAXIS");
    }
    if (units != MP_VEX_DPS && units != MP_VEX_RPM){
        mp_raise_ValueError("units must be DPS or RPM");
    }
    return mp_obj_new_float(vex_gps_gyro_rate(self->cpp_handle, axis, units));
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_gps_gyro_rate_obj, mp_gps_gyro_rate);

static mp_obj_t mp_gps_orientation(mp_obj_t self_ptr, mp_obj_t axis_obj, mp_obj_t units_obj){
    mp_gps_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    const int axis = mp_obj_get_int(axis_obj);
    const int units = mp_obj_get_int(units_obj);
    if (axis != MP_VEX_PITCH && axis != MP_VEX_ROLL && axis != MP_VEX_YAW){
        mp_raise_ValueError("axis must be PITCH, ROLL, or YAW");
    }
    if (units != MP_VEX_DEGREES){
        mp_raise_ValueError("units must be DEGREES");
    }
    return mp_obj_new_float(vex_gps_orientation(self->cpp_handle, axis, units));
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_gps_orientation_obj, mp_gps_orientation);

static mp_obj_t mp_gps_changed(size_t n_args, const mp_obj_t *args){
    mp_gps_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t callback_args = n_args > 2 ? args[2] : mp_const_none;
    mp_obj_t callback_data = mp_vex_callback_data(args[1], callback_args, n_args == 2, false);
    vex_gps_changed(self->cpp_handle, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_gps_changed_obj, 2, 3, mp_gps_changed);

static const mp_rom_map_elem_t mp_gps_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_origin), MP_ROM_PTR(&mp_gps_set_origin_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_location), MP_ROM_PTR(&mp_gps_set_location_obj) },
    { MP_ROM_QSTR(MP_QSTR_x_position), MP_ROM_PTR(&mp_gps_x_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_y_position), MP_ROM_PTR(&mp_gps_y_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_heading), MP_ROM_PTR(&mp_gps_heading_obj) },
    { MP_ROM_QSTR(MP_QSTR_acceleration), MP_ROM_PTR(&mp_gps_acceleration_obj) },
    { MP_ROM_QSTR(MP_QSTR_gyro_rate), MP_ROM_PTR(&mp_gps_gyro_rate_obj) },
    { MP_ROM_QSTR(MP_QSTR_orientation), MP_ROM_PTR(&mp_gps_orientation_obj) },
    { MP_ROM_QSTR(MP_QSTR_changed), MP_ROM_PTR(&mp_gps_changed_obj) },
};
static MP_DEFINE_CONST_DICT(mp_gps_locals_dict, mp_gps_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_gps_type,
    MP_QSTR_gps,
    MP_TYPE_FLAG_NONE,
    make_new, mp_gps_make_new,
    locals_dict, &mp_gps_locals_dict
);

static mp_obj_t mp_controller_button_pressing(mp_obj_t self_ptr){
    mp_controller_button_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    bool output = false;

    output = controller_button_pressing(self -> cpp_handle);
    return output ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_controller_button_pressing_obj, mp_controller_button_pressing);

static mp_obj_t mp_controller_button_pressed(mp_obj_t self_ptr, mp_obj_t callback, mp_obj_t args){
    mp_controller_button_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    mp_obj_t callback_data = mp_vex_callback_data(callback, args, false, false);

    controller_button_pressed(self -> cpp_handle, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_controller_button_pressed_obj, mp_controller_button_pressed);

static mp_obj_t mp_controller_button_released(mp_obj_t self_ptr, mp_obj_t callback, mp_obj_t args){
    mp_controller_button_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    mp_obj_t callback_data = mp_vex_callback_data(callback, args, false, false);

    controller_button_released(self -> cpp_handle, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_controller_button_released_obj, mp_controller_button_released);

static const mp_rom_map_elem_t mp_controller_button_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_PythonMethodName), MP_ROM_PTR(&C_Function_Object) }
    { MP_ROM_QSTR(MP_QSTR_pressing), MP_ROM_PTR(&mp_controller_button_pressing_obj) },
    { MP_ROM_QSTR(MP_QSTR_pressed), MP_ROM_PTR(&mp_controller_button_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_released), MP_ROM_PTR(&mp_controller_button_released_obj) }

};
static MP_DEFINE_CONST_DICT(mp_controller_button_locals_dict, mp_controller_button_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_controller_button_type,
    MP_QSTR_button,
    MP_TYPE_FLAG_NONE,
    locals_dict, &mp_controller_button_locals_dict
);

static mp_obj_t mp_controller_axis_position(mp_obj_t self_ptr){
    mp_controller_axis_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int output = 0;

    output = controller_axis_position(self -> cpp_handle);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_controller_axis_position_obj, mp_controller_axis_position);

static mp_obj_t mp_controller_axis_changed(mp_obj_t self_ptr, mp_obj_t callback, mp_obj_t args){
    mp_controller_axis_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    mp_obj_t callback_data = mp_vex_callback_data(callback, args, false, false);

    controller_axis_changed(self -> cpp_handle, mp_vex_callback_trampoline, MP_OBJ_TO_PTR(callback_data));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_controller_axis_changed_obj, mp_controller_axis_changed);

static const mp_rom_map_elem_t mp_controller_axis_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_PythonMethodName), MP_ROM_PTR(&C_Function_Object) }
    { MP_ROM_QSTR(MP_QSTR_position), MP_ROM_PTR(&mp_controller_axis_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_changed), MP_ROM_PTR(&mp_controller_axis_changed_obj) },

};
static MP_DEFINE_CONST_DICT(mp_controller_axis_locals_dict, mp_controller_axis_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_controller_axis_type,
    MP_QSTR_axis,
    MP_TYPE_FLAG_NONE,
    locals_dict, &mp_controller_axis_locals_dict
);

static mp_obj_t mp_controller_screen_print(size_t n_args, const mp_obj_t* args){
    mp_controller_screen_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    const char* value = mp_obj_str_get_str(args[1]);
    const char* sep = (n_args > 2) ? mp_obj_str_get_str(args[2]) : "";
    int precision = (n_args > 3) ? mp_obj_get_int(args[3]) : 2;

    controller_screen_print(self->cpp_handle, value, sep, precision);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_controller_screen_print_obj, 2, 4, mp_controller_screen_print);

static mp_obj_t mp_controller_screen_set_cursor(mp_obj_t screen_ptr, mp_obj_t x_t, mp_obj_t y_t){
    mp_controller_screen_obj_t *self = MP_OBJ_TO_PTR(screen_ptr);

    int x = mp_obj_get_int(x_t);
    int y = mp_obj_get_int(y_t);

    controller_screen_set_cursor(self->cpp_handle, x, y);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_controller_screen_set_cursor_obj, mp_controller_screen_set_cursor);

static mp_obj_t mp_controller_screen_column(mp_obj_t screen_ptr){
    mp_controller_screen_obj_t *self = MP_OBJ_TO_PTR(screen_ptr);

    int output = 0;

    output = controller_screen_column(self->cpp_handle);

    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_controller_screen_column_obj, mp_controller_screen_column);

static mp_obj_t mp_controller_screen_row(mp_obj_t screen_ptr){
    mp_controller_screen_obj_t *self = MP_OBJ_TO_PTR(screen_ptr);

    int output = 0;

    output = controller_screen_row(self->cpp_handle);

    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_controller_screen_row_obj, mp_controller_screen_row);

static mp_obj_t mp_controller_screen_clear_screen(mp_obj_t screen_ptr){
    mp_controller_screen_obj_t *self = MP_OBJ_TO_PTR(screen_ptr);

    controller_screen_clear_screen(self->cpp_handle);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_controller_screen_clear_screen_obj, mp_controller_screen_clear_screen);

static mp_obj_t mp_controller_screen_clear_row(mp_obj_t screen_ptr, mp_obj_t row_t){
    mp_controller_screen_obj_t *self = MP_OBJ_TO_PTR(screen_ptr);

    int row = mp_obj_get_int(row_t);

    controller_screen_clear_row(self->cpp_handle, row);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_controller_screen_clear_row_obj, mp_controller_screen_clear_row);

static mp_obj_t mp_controller_screen_next_row(mp_obj_t screen_ptr){
    mp_controller_screen_obj_t *self = MP_OBJ_TO_PTR(screen_ptr);

    controller_screen_next_row(self->cpp_handle);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_controller_screen_next_row_obj, mp_controller_screen_next_row);

static const mp_rom_map_elem_t mp_controller_screen_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_PythonMethodName), MP_ROM_PTR(&C_Function_Object) }
    { MP_ROM_QSTR(MP_QSTR_print), MP_ROM_PTR(&mp_controller_screen_print_obj)},
    { MP_ROM_QSTR(MP_QSTR_set_cursor), MP_ROM_PTR(&mp_controller_screen_set_cursor_obj)},
    { MP_ROM_QSTR(MP_QSTR_column), MP_ROM_PTR(&mp_controller_screen_column_obj)},
    { MP_ROM_QSTR(MP_QSTR_row), MP_ROM_PTR(&mp_controller_screen_row_obj)},
    { MP_ROM_QSTR(MP_QSTR_clear_screen), MP_ROM_PTR(&mp_controller_screen_clear_screen_obj)},
    { MP_ROM_QSTR(MP_QSTR_clear_row), MP_ROM_PTR(&mp_controller_screen_clear_row_obj)},
    { MP_ROM_QSTR(MP_QSTR_next_row), MP_ROM_PTR(&mp_controller_screen_next_row_obj)}
};
static MP_DEFINE_CONST_DICT(mp_controller_screen_locals_dict, mp_controller_screen_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_controller_screen_type,
    MP_QSTR_screen,
    MP_TYPE_FLAG_NONE,
    locals_dict, &mp_controller_screen_locals_dict
);

static void mp_controller_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    // CRITICAL SAFETY GUARD: Catch and prevent raw reflection evaluations
    if (mp_obj_get_type(self_in) == &mp_type_type) {
        dest[0] = MP_OBJ_NULL;
        return;
    }

    mp_controller_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (dest[0] == MP_OBJ_NULL) {
        switch (attr) {
            case MP_QSTR_screen: dest[0] = self->screen; return;
            case MP_QSTR_buttonA: dest[0] = self->buttonA; return;
            case MP_QSTR_buttonB: dest[0] = self->buttonB; return;
            case MP_QSTR_buttonX: dest[0] = self->buttonX; return;
            case MP_QSTR_buttonY: dest[0] = self->buttonY; return;
            case MP_QSTR_buttonUp: dest[0] = self->buttonUp; return;
            case MP_QSTR_buttonDown: dest[0] = self->buttonDown; return;
            case MP_QSTR_buttonLeft: dest[0] = self->buttonLeft; return;
            case MP_QSTR_buttonRight: dest[0] = self->buttonRight; return;
            case MP_QSTR_buttonL1: dest[0] = self->buttonL1; return;
            case MP_QSTR_buttonL2: dest[0] = self->buttonL2; return;
            case MP_QSTR_buttonR1: dest[0] = self->buttonR1; return;
            case MP_QSTR_buttonR2: dest[0] = self->buttonR2; return;
            case MP_QSTR_axis1: dest[0] = self->axis1; return;
            case MP_QSTR_axis2: dest[0] = self->axis2; return;
            case MP_QSTR_axis3: dest[0] = self->axis3; return;
            case MP_QSTR_axis4: dest[0] = self->axis4; return;    
        }
    }
    dest[0] = MP_OBJ_NULL; 
}


static mp_obj_t mp_controller_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false); // strict 1 argument

    int controller_type = mp_obj_get_int(args[0]);
    void *cpp_controller = create_controller(controller_type);

    // 2. Allocate parent wrapper struct
    mp_controller_obj_t *self = mp_obj_malloc(mp_controller_obj_t, type);
    self->cpp_handle = cpp_controller;

    mp_controller_screen_obj_t *scr = mp_obj_malloc(mp_controller_screen_obj_t, &mp_controller_screen_type);
    scr->cpp_handle = get_controller_screen(cpp_controller);

    mp_controller_button_obj_t *butA = mp_obj_malloc(mp_controller_button_obj_t, &mp_controller_button_type);
    butA->cpp_handle = get_controller_button_A(cpp_controller);

    mp_controller_button_obj_t *butB = mp_obj_malloc(mp_controller_button_obj_t, &mp_controller_button_type);
    butB->cpp_handle = get_controller_button_B(cpp_controller);

    mp_controller_button_obj_t *butX = mp_obj_malloc(mp_controller_button_obj_t, &mp_controller_button_type);
    butX->cpp_handle = get_controller_button_X(cpp_controller);

    mp_controller_button_obj_t *butY = mp_obj_malloc(mp_controller_button_obj_t, &mp_controller_button_type);
    butY->cpp_handle = get_controller_button_Y(cpp_controller);

    mp_controller_button_obj_t *butUp = mp_obj_malloc(mp_controller_button_obj_t, &mp_controller_button_type);
    butUp->cpp_handle = get_controller_button_Up(cpp_controller);

    mp_controller_button_obj_t *butDown = mp_obj_malloc(mp_controller_button_obj_t, &mp_controller_button_type);
    butDown->cpp_handle = get_controller_button_Down(cpp_controller);

    mp_controller_button_obj_t *butLeft = mp_obj_malloc(mp_controller_button_obj_t, &mp_controller_button_type);
    butLeft->cpp_handle = get_controller_button_Left(cpp_controller);

    mp_controller_button_obj_t *butRight = mp_obj_malloc(mp_controller_button_obj_t, &mp_controller_button_type);
    butRight->cpp_handle = get_controller_button_Right(cpp_controller);

    mp_controller_button_obj_t *butL1 = mp_obj_malloc(mp_controller_button_obj_t, &mp_controller_button_type);
    butL1->cpp_handle = get_controller_button_L1(cpp_controller);

    mp_controller_button_obj_t *butL2 = mp_obj_malloc(mp_controller_button_obj_t, &mp_controller_button_type);
    butL2->cpp_handle = get_controller_button_L2(cpp_controller);

    mp_controller_button_obj_t *butR1 = mp_obj_malloc(mp_controller_button_obj_t, &mp_controller_button_type);
    butR1->cpp_handle = get_controller_button_R1(cpp_controller);

    mp_controller_button_obj_t *butR2 = mp_obj_malloc(mp_controller_button_obj_t, &mp_controller_button_type);
    butR2->cpp_handle = get_controller_button_R2(cpp_controller);

    mp_controller_axis_obj_t *axis1 = mp_obj_malloc(mp_controller_axis_obj_t, &mp_controller_axis_type);
    axis1->cpp_handle = get_controller_axis_1(cpp_controller);

    mp_controller_axis_obj_t *axis2 = mp_obj_malloc(mp_controller_axis_obj_t, &mp_controller_axis_type);
    axis2->cpp_handle = get_controller_axis_2(cpp_controller);

    mp_controller_axis_obj_t *axis3 = mp_obj_malloc(mp_controller_axis_obj_t, &mp_controller_axis_type);
    axis3->cpp_handle = get_controller_axis_3(cpp_controller);

    mp_controller_axis_obj_t *axis4 = mp_obj_malloc(mp_controller_axis_obj_t, &mp_controller_axis_type);
    axis4->cpp_handle = get_controller_axis_4(cpp_controller);

    self->screen = MP_OBJ_FROM_PTR(scr);
    self->buttonA = MP_OBJ_FROM_PTR(butA);
    self->buttonB = MP_OBJ_FROM_PTR(butB);
    self->buttonX = MP_OBJ_FROM_PTR(butX);
    self->buttonY = MP_OBJ_FROM_PTR(butY);
    self->buttonUp = MP_OBJ_FROM_PTR(butUp);
    self->buttonDown = MP_OBJ_FROM_PTR(butDown);
    self->buttonLeft = MP_OBJ_FROM_PTR(butLeft);
    self->buttonRight = MP_OBJ_FROM_PTR(butRight);
    self->buttonL1 = MP_OBJ_FROM_PTR(butL1);
    self->buttonL2 = MP_OBJ_FROM_PTR(butL2);
    self->buttonR1 = MP_OBJ_FROM_PTR(butR1);
    self->buttonR2 = MP_OBJ_FROM_PTR(butR2);
    self->axis1 = MP_OBJ_FROM_PTR(axis1);
    self->axis2 = MP_OBJ_FROM_PTR(axis2);
    self->axis3 = MP_OBJ_FROM_PTR(axis3);
    self->axis4 = MP_OBJ_FROM_PTR(axis4);

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t mp_controller_rumble(mp_obj_t self_ptr, mp_obj_t pattern_t){
    mp_controller_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    const char* pattern = mp_obj_str_get_str(pattern_t);

    controller_rumble_c(self -> cpp_handle, pattern);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_controller_rumble_obj, mp_controller_rumble);

static const mp_rom_map_elem_t mp_controller_locals_dict_table[] = {
    // { MP_ROM_QSTR(MP_QSTR_PythonMethodName), MP_ROM_PTR(&C_Function_Object) }
    { MP_ROM_QSTR(MP_QSTR_rumble), MP_ROM_PTR(&mp_controller_rumble_obj) },
};
static MP_DEFINE_CONST_DICT(mp_controller_locals_dict, mp_controller_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_controller_type,
    MP_QSTR_controller,
    MP_TYPE_FLAG_NONE,
    make_new, mp_controller_make_new,
    attr, mp_controller_attr,
    locals_dict, &mp_controller_locals_dict
);

static void mp_vex_event_add_handler(mp_vex_event_obj_t *self, size_t n_args, const mp_obj_t *args) {
    mp_obj_t callback = args[0];
    if (!mp_obj_is_callable(callback)) {
        mp_raise_TypeError(MP_ERROR_TEXT("event callback must be callable"));
    }
    bool no_args = (n_args == 1);
    mp_obj_t entry[] = {
        callback,
        no_args ? mp_const_none : args[1],
        mp_obj_new_bool(no_args),
    };
    mp_obj_list_append(self->handlers, mp_obj_new_tuple(3, entry));
}

static mp_obj_t mp_vex_event_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 2, false);
    mp_vex_event_obj_t *self = mp_obj_malloc(mp_vex_event_obj_t, type);
    self->handlers = mp_obj_new_list(0, NULL);
    if (n_args >= 1 && args[0] != mp_const_none) {
        mp_vex_event_add_handler(self, n_args, args);
    }
    return MP_OBJ_FROM_PTR(self);
}

// Allows calling event(callback, arg) directly or using @event as a decorator
static mp_obj_t mp_vex_event_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    mp_vex_event_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_vex_event_add_handler(self, n_args, args);
    return args[0];
}

static mp_obj_t mp_vex_event_set(size_t n_args, const mp_obj_t *args) {
    mp_vex_event_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_vex_event_add_handler(self, n_args - 1, args + 1);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_vex_event_set_obj, 2, 3, mp_vex_event_set);

static mp_obj_t mp_vex_event_clear(mp_obj_t self_in) {
    mp_vex_event_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_list_set_len(self->handlers, 0);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_event_clear_obj, mp_vex_event_clear);

static mp_obj_t mp_vex_event_spawn_all(mp_vex_event_obj_t *self) {
    size_t count = 0;
    mp_obj_t *handler_items = NULL;
    mp_obj_get_array(self->handlers, &count, &handler_items);

    mp_obj_t thread_list = mp_obj_new_list(0, NULL);
    if (MP_STATE_VM(vex_callback_roots) == MP_OBJ_NULL) {
        MP_STATE_VM(vex_callback_roots) = mp_obj_new_list(0, NULL);
    }

    for (size_t i = 0; i < count; ++i) {
        size_t h_len = 0;
        mp_obj_t *h_items = NULL;
        mp_obj_get_array(handler_items[i], &h_len, &h_items);

        mp_vex_thread_obj_t *thread_obj = mp_obj_malloc_with_finaliser(mp_vex_thread_obj_t, &mp_vex_thread_type);
        thread_obj->thread_id = 0;
        thread_obj->stopped = false;
        thread_obj->done = false;

        mp_obj_t callback_items[] = {
            h_items[0],
            h_items[1],
            h_items[2],
            mp_const_true,
            MP_OBJ_FROM_PTR(thread_obj),
            MP_OBJ_FROM_PTR(mp_locals_get()),
            MP_OBJ_FROM_PTR(mp_globals_get()),
        };
        mp_obj_t callback_data = mp_obj_new_list(MP_ARRAY_SIZE(callback_items), callback_items);
        mp_obj_list_append(MP_STATE_VM(vex_callback_roots), callback_data);

        size_t stack_size = 0;
        thread_obj->thread_id = mp_thread_create(mp_vex_thread_entry, MP_OBJ_TO_PTR(callback_data), &stack_size);
        if (thread_obj->thread_id == 0) {
            mp_obj_list_remove(MP_STATE_VM(vex_callback_roots), callback_data);
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("could not create event thread"));
        }
        mp_obj_list_append(thread_list, MP_OBJ_FROM_PTR(thread_obj));
    }
    return thread_list;
}

static mp_obj_t mp_vex_event_broadcast(mp_obj_t self_in) {
    mp_vex_event_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_vex_event_spawn_all(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_event_broadcast_obj, mp_vex_event_broadcast);

static mp_obj_t mp_vex_event_broadcast_and_wait(mp_obj_t self_in) {
    mp_vex_event_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t thread_list = mp_vex_event_spawn_all(self);

    size_t count = 0;
    mp_obj_t *threads = NULL;
    mp_obj_get_array(thread_list, &count, &threads);

    while (true) {
        bool all_done = true;
        for (size_t i = 0; i < count; ++i) {
            mp_vex_thread_obj_t *t = MP_OBJ_TO_PTR(threads[i]);
            if (!t->done) {
                all_done = false;
                break;
            }
        }
        if (all_done) {
            break;
        }
        MP_THREAD_GIL_EXIT();
        vex_logic_yield_ms(2);
        MP_THREAD_GIL_ENTER();
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mp_vex_event_broadcast_and_wait_obj, mp_vex_event_broadcast_and_wait);

static const mp_rom_map_elem_t mp_vex_event_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_broadcast), MP_ROM_PTR(&mp_vex_event_broadcast_obj) },
    { MP_ROM_QSTR(MP_QSTR_broadcast_and_wait), MP_ROM_PTR(&mp_vex_event_broadcast_and_wait_obj) },
    { MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&mp_vex_event_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&mp_vex_event_clear_obj) },
};
static MP_DEFINE_CONST_DICT(mp_vex_event_locals_dict, mp_vex_event_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_vex_event_type,
    MP_QSTR_Event,
    MP_TYPE_FLAG_NONE,
    make_new, mp_vex_event_make_new,
    call, mp_vex_event_call,
    locals_dict, &mp_vex_event_locals_dict
);

static const mp_rom_map_elem_t mp_ports_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_PORT1), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_PORT2), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_PORT3), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_PORT4), MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_PORT5), MP_ROM_INT(5) },
    { MP_ROM_QSTR(MP_QSTR_PORT6), MP_ROM_INT(6) },
    { MP_ROM_QSTR(MP_QSTR_PORT7), MP_ROM_INT(7) },
    { MP_ROM_QSTR(MP_QSTR_PORT8), MP_ROM_INT(8) },
    { MP_ROM_QSTR(MP_QSTR_PORT9), MP_ROM_INT(9) },
    { MP_ROM_QSTR(MP_QSTR_PORT10), MP_ROM_INT(10) },
    { MP_ROM_QSTR(MP_QSTR_PORT11), MP_ROM_INT(11) },
    { MP_ROM_QSTR(MP_QSTR_PORT12), MP_ROM_INT(12) },
    { MP_ROM_QSTR(MP_QSTR_PORT13), MP_ROM_INT(13) },
    { MP_ROM_QSTR(MP_QSTR_PORT14), MP_ROM_INT(14) },
    { MP_ROM_QSTR(MP_QSTR_PORT15), MP_ROM_INT(15) },
    { MP_ROM_QSTR(MP_QSTR_PORT16), MP_ROM_INT(16) },
    { MP_ROM_QSTR(MP_QSTR_PORT17), MP_ROM_INT(17) },
    { MP_ROM_QSTR(MP_QSTR_PORT18), MP_ROM_INT(18) },
    { MP_ROM_QSTR(MP_QSTR_PORT19), MP_ROM_INT(19) },
    { MP_ROM_QSTR(MP_QSTR_PORT20), MP_ROM_INT(20) },
    { MP_ROM_QSTR(MP_QSTR_PORT21), MP_ROM_INT(21) },
    { MP_ROM_QSTR(MP_QSTR_PORT22), MP_ROM_INT(22) },
};
static MP_DEFINE_CONST_DICT(mp_ports_locals_dict, mp_ports_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_ports_type, MP_QSTR_ports, MP_TYPE_FLAG_NONE, locals_dict, &mp_ports_locals_dict);

static const mp_rom_map_elem_t mp_velocity_units_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_PERCENT), MP_ROM_INT(MP_VEX_PERCENT) },
    { MP_ROM_QSTR(MP_QSTR_RPM), MP_ROM_INT(MP_VEX_RPM) },
    { MP_ROM_QSTR(MP_QSTR_DPS), MP_ROM_INT(MP_VEX_DPS) },
    { MP_ROM_QSTR(MP_QSTR_VOLT), MP_ROM_INT(39) },
};
static MP_DEFINE_CONST_DICT(mp_velocity_units_locals_dict, mp_velocity_units_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_velocity_units_type, MP_QSTR_velocityunits, MP_TYPE_FLAG_NONE, locals_dict, &mp_velocity_units_locals_dict);

static const mp_rom_map_elem_t mp_angle_units_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_DEGREES), MP_ROM_INT(MP_VEX_DEGREES) },
    { MP_ROM_QSTR(MP_QSTR_TURNS), MP_ROM_INT(MP_VEX_TURNS) },
};
static MP_DEFINE_CONST_DICT(mp_angle_units_locals_dict, mp_angle_units_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_angle_units_type, MP_QSTR_angleunits, MP_TYPE_FLAG_NONE, locals_dict, &mp_angle_units_locals_dict);

static const mp_rom_map_elem_t mp_font_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_MONO12), MP_ROM_INT(40) },
    { MP_ROM_QSTR(MP_QSTR_MONO15), MP_ROM_INT(41) },
    { MP_ROM_QSTR(MP_QSTR_MONO20), MP_ROM_INT(42) },
    { MP_ROM_QSTR(MP_QSTR_MONO30), MP_ROM_INT(43) },
    { MP_ROM_QSTR(MP_QSTR_MONO40), MP_ROM_INT(44) },
    { MP_ROM_QSTR(MP_QSTR_MONO60), MP_ROM_INT(45) },
    { MP_ROM_QSTR(MP_QSTR_PROP20), MP_ROM_INT(46) },
    { MP_ROM_QSTR(MP_QSTR_PROP30), MP_ROM_INT(47) },
    { MP_ROM_QSTR(MP_QSTR_PROP40), MP_ROM_INT(48) },
    { MP_ROM_QSTR(MP_QSTR_PROP60), MP_ROM_INT(49) },
};
static MP_DEFINE_CONST_DICT(mp_font_type_locals_dict, mp_font_type_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_font_type, MP_QSTR_fonttype, MP_TYPE_FLAG_NONE, locals_dict, &mp_font_type_locals_dict);

static const mp_rom_map_elem_t mp_axis_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_XAXIS), MP_ROM_INT(MP_VEX_AXIS_X) },
    { MP_ROM_QSTR(MP_QSTR_YAXIS), MP_ROM_INT(MP_VEX_AXIS_Y) },
    { MP_ROM_QSTR(MP_QSTR_ZAXIS), MP_ROM_INT(MP_VEX_AXIS_Z) },
    { MP_ROM_QSTR(MP_QSTR_PITCH), MP_ROM_INT(MP_VEX_PITCH) },
    { MP_ROM_QSTR(MP_QSTR_ROLL), MP_ROM_INT(MP_VEX_ROLL) },
    { MP_ROM_QSTR(MP_QSTR_YAW), MP_ROM_INT(MP_VEX_YAW) },
};
static MP_DEFINE_CONST_DICT(mp_axis_type_locals_dict, mp_axis_type_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_axis_type, MP_QSTR_axistype, MP_TYPE_FLAG_NONE, locals_dict, &mp_axis_type_locals_dict);

static const mp_rom_map_elem_t mp_turn_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_RIGHT), MP_ROM_INT(MP_VEX_RIGHT) },
    { MP_ROM_QSTR(MP_QSTR_LEFT), MP_ROM_INT(MP_VEX_LEFT) },
};
static MP_DEFINE_CONST_DICT(mp_turn_type_locals_dict, mp_turn_type_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_turn_type, MP_QSTR_turntype, MP_TYPE_FLAG_NONE, locals_dict, &mp_turn_type_locals_dict);

static const mp_rom_map_elem_t mp_led_state_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_ON), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_OFF), MP_ROM_INT(0) },
};
static MP_DEFINE_CONST_DICT(mp_led_state_type_locals_dict, mp_led_state_type_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_led_state_type, MP_QSTR_ledstatetype, MP_TYPE_FLAG_NONE, locals_dict, &mp_led_state_type_locals_dict);

static const mp_rom_map_elem_t mp_object_size_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_NONE), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_SMALL), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_MEDIUM), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_LARGE), MP_ROM_INT(3) },
};
static MP_DEFINE_CONST_DICT(mp_object_size_type_locals_dict, mp_object_size_type_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_object_size_type, MP_QSTR_objectsizetype, MP_TYPE_FLAG_NONE, locals_dict, &mp_object_size_type_locals_dict);

static const mp_rom_map_elem_t mp_GearSetting_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_RATIO_6_1),  MP_ROM_INT(8) },
    { MP_ROM_QSTR(MP_QSTR_RATIO_18_1), MP_ROM_INT(18) },
    { MP_ROM_QSTR(MP_QSTR_RATIO_36_1), MP_ROM_INT(36) },
};
static MP_DEFINE_CONST_DICT(mp_GearSetting_locals_dict, mp_GearSetting_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_gear_setting_type, MP_QSTR_gearsetting, MP_TYPE_FLAG_NONE, locals_dict, &mp_GearSetting_locals_dict);

static const mp_rom_map_elem_t mp_color_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_BLACK), MP_ROM_INT(0x000000) },
    { MP_ROM_QSTR(MP_QSTR_WHITE), MP_ROM_INT(0xFFFFFF) },
    { MP_ROM_QSTR(MP_QSTR_RED), MP_ROM_INT(0xFF0000) },
    { MP_ROM_QSTR(MP_QSTR_GREEN), MP_ROM_INT(0x00FF00) },
    { MP_ROM_QSTR(MP_QSTR_BLUE), MP_ROM_INT(0x0000FF) },
    { MP_ROM_QSTR(MP_QSTR_YELLOW), MP_ROM_INT(0xFFFF00) },
    { MP_ROM_QSTR(MP_QSTR_ORANGE), MP_ROM_INT(0xFFA500) },
    { MP_ROM_QSTR(MP_QSTR_PURPLE), MP_ROM_INT(0x800080) },
    { MP_ROM_QSTR(MP_QSTR_CYAN), MP_ROM_INT(0x00FFFF) },
};
static MP_DEFINE_CONST_DICT(mp_color_locals_dict, mp_color_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_color_type,
    MP_QSTR_color,
    MP_TYPE_FLAG_NONE,
    make_new, mp_color_make_new,
    locals_dict, &mp_color_locals_dict
);

static const mp_rom_map_elem_t mp_distance_units_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_MM), MP_ROM_INT(MP_VEX_MM) },
    { MP_ROM_QSTR(MP_QSTR_INCHES), MP_ROM_INT(MP_VEX_INCHES) },
    { MP_ROM_QSTR(MP_QSTR_CM), MP_ROM_INT(42) },
};
static MP_DEFINE_CONST_DICT(mp_distance_units_locals_dict, mp_distance_units_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_distance_units_type, MP_QSTR_distanceunits, MP_TYPE_FLAG_NONE, locals_dict, &mp_distance_units_locals_dict);

static const mp_rom_map_elem_t mp_analog_units_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_PERCENT), MP_ROM_INT(MP_VEX_PERCENT) },
    { MP_ROM_QSTR(MP_QSTR_EIGHTBIT), MP_ROM_INT(MP_VEX_ANALOG_EIGHTBIT) },
    { MP_ROM_QSTR(MP_QSTR_TENBIT), MP_ROM_INT(MP_VEX_ANALOG_TENBIT) },
    { MP_ROM_QSTR(MP_QSTR_TWELVEBIT), MP_ROM_INT(MP_VEX_ANALOG_TWELVEBIT) },
};
static MP_DEFINE_CONST_DICT(mp_analog_units_locals_dict, mp_analog_units_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_analog_units_type, MP_QSTR_analogunits, MP_TYPE_FLAG_NONE, locals_dict, &mp_analog_units_locals_dict);

static const mp_rom_map_elem_t mp_vex_link_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_MANAGER),        MP_ROM_INT(100) },
    { MP_ROM_QSTR(MP_QSTR_WORKER),         MP_ROM_INT(101) },
};
static MP_DEFINE_CONST_DICT(mp_vex_link_type_locals_dict, mp_vex_link_type_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_vex_link_type, MP_QSTR_vexlinktype, MP_TYPE_FLAG_NONE, locals_dict, &mp_vex_link_type_locals_dict);

// 1. Add OrientationType (PITCH, ROLL, YAW)
static const mp_rom_map_elem_t mp_orientation_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_PITCH), MP_ROM_INT(MP_VEX_PITCH) },
    { MP_ROM_QSTR(MP_QSTR_ROLL), MP_ROM_INT(MP_VEX_ROLL) },
    { MP_ROM_QSTR(MP_QSTR_YAW), MP_ROM_INT(MP_VEX_YAW) },
};
static MP_DEFINE_CONST_DICT(mp_orientation_type_locals_dict, mp_orientation_type_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_orientation_type, MP_QSTR_orientationtype, MP_TYPE_FLAG_NONE, locals_dict, &mp_orientation_type_locals_dict);

// 2. Add CurrentUnits (AMP)
static const mp_rom_map_elem_t mp_current_units_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_AMP), MP_ROM_INT(33) },
};
static MP_DEFINE_CONST_DICT(mp_current_units_locals_dict, mp_current_units_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_current_units_type, MP_QSTR_currentunits, MP_TYPE_FLAG_NONE, locals_dict, &mp_current_units_locals_dict);

// 3. Add PowerUnits (WATT)
static const mp_rom_map_elem_t mp_power_units_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_WATT), MP_ROM_INT(34) },
};
static MP_DEFINE_CONST_DICT(mp_power_units_locals_dict, mp_power_units_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_power_units_type, MP_QSTR_powerunits, MP_TYPE_FLAG_NONE, locals_dict, &mp_power_units_locals_dict);


// This dictionary holds everything inside the top-level namespace: 'import vex'
static const mp_rom_map_elem_t vex_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_vex) },
    { MP_ROM_QSTR(MP_QSTR_wait),        MP_ROM_PTR(&mp_vex_wait_obj) },
    { MP_ROM_QSTR(MP_QSTR_abs),         MP_ROM_PTR(&mp_builtin_abs_obj) },
    { MP_ROM_QSTR(MP_QSTR_Competition), MP_ROM_PTR(&mp_vex_competition_type) },

    // Bind Classes into the module namespace
    { MP_ROM_QSTR(MP_QSTR_Motor), MP_ROM_PTR(&mp_motor_type) },
    { MP_ROM_QSTR(MP_QSTR_MotorGroup), MP_ROM_PTR(&mp_motorgroup_type) },
    { MP_ROM_QSTR(MP_QSTR_DriveTrain), MP_ROM_PTR(&mp_drivetrain_type) },
    { MP_ROM_QSTR(MP_QSTR_MessageLink), MP_ROM_PTR(&mp_message_link_type) },
    { MP_ROM_QSTR(MP_QSTR_SerialLink), MP_ROM_PTR(&mp_serial_link_type) },
    { MP_ROM_QSTR(MP_QSTR_Timer), MP_ROM_PTR(&mp_timer_type) },
    { MP_ROM_QSTR(MP_QSTR_Thread), MP_ROM_PTR(&mp_vex_thread_type) },
    { MP_ROM_QSTR(MP_QSTR_Event), MP_ROM_PTR(&mp_vex_event_type) },
    { MP_ROM_QSTR(MP_QSTR_Brain), MP_ROM_PTR(&mp_brain_type) },
    { MP_ROM_QSTR(MP_QSTR_Controller), MP_ROM_PTR(&mp_controller_type) },
    { MP_ROM_QSTR(MP_QSTR_Rotation), MP_ROM_PTR(&mp_rotation_type) },
    { MP_ROM_QSTR(MP_QSTR_Inertial), MP_ROM_PTR(&mp_inertial_type) },
    { MP_ROM_QSTR(MP_QSTR_Optical), MP_ROM_PTR(&mp_optical_type) },
    { MP_ROM_QSTR(MP_QSTR_Distance), MP_ROM_PTR(&mp_distance_type) },
    { MP_ROM_QSTR(MP_QSTR_Gps), MP_ROM_PTR(&mp_gps_type) },
    { MP_ROM_QSTR(MP_QSTR_DigitalOut), MP_ROM_PTR(&mp_digital_out_type) },
    { MP_ROM_QSTR(MP_QSTR_AnalogIn), MP_ROM_PTR(&mp_analog_in_type) },
    { MP_ROM_QSTR(MP_QSTR_Bumper), MP_ROM_PTR(&mp_bumper_type) },
    { MP_ROM_QSTR(MP_QSTR_DigitalIn), MP_ROM_PTR(&mp_digital_in_type) },
    { MP_ROM_QSTR(MP_QSTR_Encoder), MP_ROM_PTR(&mp_encoder_type) },
    { MP_ROM_QSTR(MP_QSTR_Gyro), MP_ROM_PTR(&mp_gyro_type) },
    { MP_ROM_QSTR(MP_QSTR_Led), MP_ROM_PTR(&mp_led_type) },
    { MP_ROM_QSTR(MP_QSTR_Light), MP_ROM_PTR(&mp_light_type) },
    { MP_ROM_QSTR(MP_QSTR_Limit), MP_ROM_PTR(&mp_limit_type) },
    { MP_ROM_QSTR(MP_QSTR_Line), MP_ROM_PTR(&mp_line_type) },
    { MP_ROM_QSTR(MP_QSTR_ObjectDetector), MP_ROM_PTR(&mp_object_sensor_type) },
    { MP_ROM_QSTR(MP_QSTR_Potentiometer), MP_ROM_PTR(&mp_potentiometer_type) },
    { MP_ROM_QSTR(MP_QSTR_PotentiometerV2), MP_ROM_PTR(&mp_potentiometer_v2_type) },
    { MP_ROM_QSTR(MP_QSTR_Pwm), MP_ROM_PTR(&mp_pwm_type) },
    { MP_ROM_QSTR(MP_QSTR_Sonar), MP_ROM_PTR(&mp_sonar_type) },
    { MP_ROM_QSTR(MP_QSTR_Accelerometer), MP_ROM_PTR(&mp_accelerometer_type) },
    { MP_ROM_QSTR(MP_QSTR_AddressableLed), MP_ROM_PTR(&mp_addressable_led_type) },
    { MP_ROM_QSTR(MP_QSTR_Ports), MP_ROM_PTR(&mp_ports_type) },
    { MP_ROM_QSTR(MP_QSTR_VelocityUnits), MP_ROM_PTR(&mp_velocity_units_type) },
    { MP_ROM_QSTR(MP_QSTR_AngleUnits), MP_ROM_PTR(&mp_angle_units_type) },
    { MP_ROM_QSTR(MP_QSTR_FontType), MP_ROM_PTR(&mp_font_type) },
    { MP_ROM_QSTR(MP_QSTR_AxisType), MP_ROM_PTR(&mp_axis_type) },
    { MP_ROM_QSTR(MP_QSTR_TurnType), MP_ROM_PTR(&mp_turn_type) },
    { MP_ROM_QSTR(MP_QSTR_LEDStateType), MP_ROM_PTR(&mp_led_state_type) },
    { MP_ROM_QSTR(MP_QSTR_ObjectSizeType), MP_ROM_PTR(&mp_object_size_type) },
    { MP_ROM_QSTR(MP_QSTR_Color), MP_ROM_PTR(&mp_color_type) },
    { MP_ROM_QSTR(MP_QSTR_DistanceUnits), MP_ROM_PTR(&mp_distance_units_type) },
    { MP_ROM_QSTR(MP_QSTR_AnalogUnits), MP_ROM_PTR(&mp_analog_units_type) },
    { MP_ROM_QSTR(MP_QSTR_GearSetting), MP_ROM_PTR(&mp_gear_setting_type) },
    { MP_ROM_QSTR(MP_QSTR_VexlinkType), MP_ROM_PTR(&mp_vex_link_type) },
    { MP_ROM_QSTR(MP_QSTR_OrientationType), MP_ROM_PTR(&mp_orientation_type) },
    { MP_ROM_QSTR(MP_QSTR_CurrentUnits), MP_ROM_PTR(&mp_current_units_type) },
    { MP_ROM_QSTR(MP_QSTR_PowerUnits), MP_ROM_PTR(&mp_power_units_type) },

    // Bind Global Enum Constants directly to the module scope
    { MP_ROM_QSTR(MP_QSTR_RATIO_6_1),  MP_ROM_INT(8) },
    { MP_ROM_QSTR(MP_QSTR_RATIO_18_1), MP_ROM_INT(18) },
    { MP_ROM_QSTR(MP_QSTR_RATIO_36_1), MP_ROM_INT(36) },
    { MP_ROM_QSTR(MP_QSTR_PRIMARY),     MP_ROM_INT(50) },
    { MP_ROM_QSTR(MP_QSTR_PARTNER),    MP_ROM_INT(51) },
    { MP_ROM_QSTR(MP_QSTR_COAST),       MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_BRAKE),       MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_HOLD),        MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_PORT1),       MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_PORT2),       MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_PORT3),       MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_PORT4),       MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_PORT5),       MP_ROM_INT(5) },
    { MP_ROM_QSTR(MP_QSTR_PORT6),       MP_ROM_INT(6) },
    { MP_ROM_QSTR(MP_QSTR_PORT7),       MP_ROM_INT(7) },
    { MP_ROM_QSTR(MP_QSTR_PORT8),       MP_ROM_INT(8) },
    { MP_ROM_QSTR(MP_QSTR_PORT9),       MP_ROM_INT(9) },
    { MP_ROM_QSTR(MP_QSTR_PORT10),      MP_ROM_INT(10) },
    { MP_ROM_QSTR(MP_QSTR_PORT11),      MP_ROM_INT(11) },
    { MP_ROM_QSTR(MP_QSTR_PORT12),      MP_ROM_INT(12) },
    { MP_ROM_QSTR(MP_QSTR_PORT13),      MP_ROM_INT(13) },
    { MP_ROM_QSTR(MP_QSTR_PORT14),      MP_ROM_INT(14) },
    { MP_ROM_QSTR(MP_QSTR_PORT15),      MP_ROM_INT(15) },
    { MP_ROM_QSTR(MP_QSTR_PORT16),      MP_ROM_INT(16) },
    { MP_ROM_QSTR(MP_QSTR_PORT17),      MP_ROM_INT(17) },
    { MP_ROM_QSTR(MP_QSTR_PORT18),      MP_ROM_INT(18) },
    { MP_ROM_QSTR(MP_QSTR_PORT19),      MP_ROM_INT(19) },
    { MP_ROM_QSTR(MP_QSTR_PORT20),      MP_ROM_INT(20) },
    { MP_ROM_QSTR(MP_QSTR_PORT21),      MP_ROM_INT(21) },
    { MP_ROM_QSTR(MP_QSTR_PORT22),      MP_ROM_INT(22) },
    { MP_ROM_QSTR(MP_QSTR_PERCENT),     MP_ROM_INT(23) },
    { MP_ROM_QSTR(MP_QSTR_RPM),         MP_ROM_INT(24) },
    { MP_ROM_QSTR(MP_QSTR_VOLT),        MP_ROM_INT(39) },
    { MP_ROM_QSTR(MP_QSTR_DPS),         MP_ROM_INT(26) },
    { MP_ROM_QSTR(MP_QSTR_FORWARD),     MP_ROM_INT(27) },
    { MP_ROM_QSTR(MP_QSTR_REVERSE),     MP_ROM_INT(28) },
    { MP_ROM_QSTR(MP_QSTR_DEGREES),     MP_ROM_INT(29) },
    { MP_ROM_QSTR(MP_QSTR_TURNS),       MP_ROM_INT(30) },
    { MP_ROM_QSTR(MP_QSTR_NM),          MP_ROM_INT(31) },
    { MP_ROM_QSTR(MP_QSTR_INLB),        MP_ROM_INT(32) },
    { MP_ROM_QSTR(MP_QSTR_AMPS),        MP_ROM_INT(33) },
    { MP_ROM_QSTR(MP_QSTR_WATT),        MP_ROM_INT(34) },
    { MP_ROM_QSTR(MP_QSTR_CELSIUS),     MP_ROM_INT(35) },
    { MP_ROM_QSTR(MP_QSTR_FAHRENHEIT),  MP_ROM_INT(36) },
    { MP_ROM_QSTR(MP_QSTR_SECONDS),     MP_ROM_INT(VEX_TIME_UNITS_SECONDS) },
    { MP_ROM_QSTR(MP_QSTR_MSEC),        MP_ROM_INT(VEX_TIME_UNITS_MSEC) },
    { MP_ROM_QSTR(MP_QSTR_MASTER),        MP_ROM_INT(50) },
    { MP_ROM_QSTR(MP_QSTR_PARTNER),        MP_ROM_INT(51) },
    { MP_ROM_QSTR(MP_QSTR_XAXIS),       MP_ROM_INT(MP_VEX_AXIS_X) },
    { MP_ROM_QSTR(MP_QSTR_YAXIS),       MP_ROM_INT(MP_VEX_AXIS_Y) },
    { MP_ROM_QSTR(MP_QSTR_ZAXIS),       MP_ROM_INT(MP_VEX_AXIS_Z) },
    { MP_ROM_QSTR(MP_QSTR_PITCH),       MP_ROM_INT(MP_VEX_PITCH) },
    { MP_ROM_QSTR(MP_QSTR_ROLL),        MP_ROM_INT(MP_VEX_ROLL) },
    { MP_ROM_QSTR(MP_QSTR_YAW),         MP_ROM_INT(MP_VEX_YAW) },
    { MP_ROM_QSTR(MP_QSTR_RIGHT),       MP_ROM_INT(MP_VEX_RIGHT) },
    { MP_ROM_QSTR(MP_QSTR_LEFT),        MP_ROM_INT(MP_VEX_LEFT) },
    { MP_ROM_QSTR(MP_QSTR_ON),          MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_OFF),         MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_NONE),        MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_SMALL),       MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_MEDIUM),      MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_LARGE),       MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_MM),          MP_ROM_INT(MP_VEX_MM) },
    { MP_ROM_QSTR(MP_QSTR_INCHES),      MP_ROM_INT(MP_VEX_INCHES) },
    { MP_ROM_QSTR(MP_QSTR_CM),          MP_ROM_INT(42) },
    { MP_ROM_QSTR(MP_QSTR_BLACK),       MP_ROM_INT(0x000000) },
    { MP_ROM_QSTR(MP_QSTR_WHITE),       MP_ROM_INT(0xFFFFFF) },
    { MP_ROM_QSTR(MP_QSTR_RED),         MP_ROM_INT(0xFF0000) },
    { MP_ROM_QSTR(MP_QSTR_GREEN),       MP_ROM_INT(0x00FF00) },
    { MP_ROM_QSTR(MP_QSTR_BLUE),        MP_ROM_INT(0x0000FF) },
    { MP_ROM_QSTR(MP_QSTR_YELLOW),      MP_ROM_INT(0xFFFF00) },
    { MP_ROM_QSTR(MP_QSTR_ORANGE),      MP_ROM_INT(0xFFA500) },
    { MP_ROM_QSTR(MP_QSTR_PURPLE),      MP_ROM_INT(0x800080) },
    { MP_ROM_QSTR(MP_QSTR_CYAN),        MP_ROM_INT(0x00FFFF) },
    
};
static MP_DEFINE_CONST_DICT(vex_module_globals, vex_module_globals_table);

// Packaging the structural footprint into a Module block
const mp_obj_module_t vex_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&vex_module_globals,
};

// 7. System Entry Hook: Tells the MicroPython parser to index this module
MP_REGISTER_MODULE(MP_QSTR_vex, vex_user_cmodule);