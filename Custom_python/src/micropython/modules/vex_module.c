#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"
#include "vex/vex.h"

MP_REGISTER_ROOT_POINTER(mp_obj_t vex_callback_roots);

mp_obj_t mp_vex_callback_executor(mp_obj_t callback_data) {
    size_t count;
    mp_obj_t *items;
    mp_obj_get_array(callback_data, &count, &items);
    if (mp_obj_is_true(items[3])) {
        mp_obj_list_remove(MP_STATE_VM(vex_callback_roots), callback_data);
    }
    if (mp_obj_is_true(items[2])) {
        return mp_call_function_0(items[0]);
    }
    return mp_call_function_1(items[0], items[1]);
}

static void mp_vex_callback_trampoline(void *callback_data) {
    while (!mp_sched_schedule(MP_OBJ_FROM_PTR(mp_vex_callback_executor), MP_OBJ_FROM_PTR(callback_data))) {
        mp_hal_delay_ms(1);
    }
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

typedef struct _mp_motor_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_motor_obj_t;

typedef struct _mp_motorgroup_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_motorgroup_obj_t;

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

typedef struct _mp_digital_out_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_digital_out_obj_t;

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

extern const mp_obj_type_t mp_motor_type;
extern const mp_obj_type_t mp_motorgroup_type;
extern const mp_obj_type_t mp_rotation_type;
extern const mp_obj_type_t mp_inertial_type;
extern const mp_obj_type_t mp_optical_type;
extern const mp_obj_type_t mp_distance_type;
extern const mp_obj_type_t mp_digital_out_type;
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
    float velocity = mp_obj_get_float(args[2]);
    int units = mp_obj_get_int(args[3]);

    motor_spin(self->cpp_handle, direction, velocity, units);
    return mp_const_none;
}
// obj, requred args, total args, function.
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_spin_obj, 4, 4, mp_motor_spin);

static mp_obj_t mp_motor_spin_for(size_t n_args, const mp_obj_t *args) {
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    int direction = mp_obj_get_int(args[1]);
    int angle = mp_obj_get_int(args[2]);

    // Defining default values.
    int units = (n_args > 3) ? mp_obj_get_int(args[3])   : 29;      // DEGREES macro value
    float velocity = (n_args > 4) ? mp_obj_get_float(args[4]) : 601.0f;
    int units_v = (n_args > 5) ? mp_obj_get_int(args[5])   : 24;      // RPM macro value
    bool wait = (n_args > 6) ? mp_obj_is_true(args[6])   : true;

    motor_spin_for(self -> cpp_handle, direction, angle, units, velocity, units_v, wait);
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

    motor_spin_to_position(self -> cpp_handle, rotation, units, velocity, units_v, wait);
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

    motor_set_reversed(self -> cpp_handle, value);
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
};
static MP_DEFINE_CONST_DICT(mp_motor_locals_dict, mp_motor_locals_dict_table);

// Register the finalized Motor class metadata type
MP_DEFINE_CONST_OBJ_TYPE(
    mp_motor_type,
    MP_QSTR_Motor,                  // Name exposed to Python
    MP_TYPE_FLAG_NONE,
    make_new, mp_motor_make_new,    // Constructor attachment
    locals_dict, &mp_motor_locals_dict
);


static mp_obj_t mp_motor_group_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    size_t count = 0;
    mp_obj_t *items;
    mp_obj_get_array(args[0], &count, &items); // Extracts the Python list array

    void *motor_pointers[count];
    for (size_t i = 0; i < count; i++) {
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

    motor_group_spin_for(self -> cpp_handle, direction, angle, units, velocity, units_v, wait);
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

    motor_group_spin_to_position(self -> cpp_handle, rotation, units, velocity, units_v, wait);
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
    MP_QSTR_MotorGroup,             // Name exposed to Python
    MP_TYPE_FLAG_NONE,
    make_new, mp_motor_group_make_new,
    locals_dict, &mp_motorgroup_locals_dict
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

static mp_obj_t mp_timer_time(mp_obj_t self_ptr, mp_obj_t units_t){
    mp_timer_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int units = mp_obj_get_int(units_t);

    int output;
    output = vex_timer_time(self -> cpp_handle, units);
    return mp_obj_new_int(output);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_timer_time_obj, mp_timer_time);

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
    MP_QSTR_Timer,             // Name exposed to Python
    MP_TYPE_FLAG_NONE,
    make_new, mp_timer_make_new,
    locals_dict, &mp_timer_locals_dict
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

static mp_obj_t mp_brain_screen_print(size_t n_args, const mp_obj_t *args) {
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    const char* text = mp_obj_str_get_str(args[1]);

    // Defining default values.
    const char* sep = (n_args > 2) ? mp_obj_str_get_str(args[2])   : "";      // DEGREES macro value
    int precision = (n_args > 3) ? mp_obj_get_float(args[3]) : 2;

    brain_screen_print(self -> cpp_handle, text, sep, precision);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_screen_print_obj, 2, 4, mp_brain_screen_print);

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

static mp_obj_t mp_brain_screen_clear_row(mp_obj_t self_ptr, mp_obj_t row_t, mp_obj_t color_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int row = mp_obj_get_int(row_t);
    int color = mp_obj_get_int(color_t);

    brain_screen_clear_row(self -> cpp_handle, row, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mp_brain_screen_clear_row_obj, mp_brain_screen_clear_row);

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

static mp_obj_t mp_brain_screen_print_at(size_t n_args, const mp_obj_t *args) {
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    const char* text = mp_obj_str_get_str(args[1]);

    int x = mp_obj_get_int(args[2]);
    int y = mp_obj_get_int(args[3]);

    // Defining default values.
    const char* sep = (n_args > 4) ? mp_obj_str_get_str(args[4])   : "";      // DEGREES macro value
    int precision = (n_args > 5) ? mp_obj_get_float(args[5]) : 2;
    bool opaque = (n_args > 6) ? mp_obj_is_true(args[6])   : true;

    brain_screen_print_at(self -> cpp_handle, text, x, y, sep, precision, opaque);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_screen_print_at_obj, 4, 7, mp_brain_screen_print_at);

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

static mp_obj_t mp_brain_screen_clear_screen(mp_obj_t self_ptr, mp_obj_t color_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int color = mp_obj_get_int(color_t);

    brain_screen_clear_screen(self -> cpp_handle, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_screen_clear_screen_obj, mp_brain_screen_clear_screen);

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

    int color = mp_obj_get_int(color_t);

    brain_screen_set_pen_color(self -> cpp_handle, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_brain_screen_set_pen_color_obj, mp_brain_screen_set_pen_color);

static mp_obj_t mp_brain_screen_set_fill_color(mp_obj_t self_ptr, mp_obj_t color_t){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    int color = mp_obj_get_int(color_t);

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
    int color = (n_args > 5) ? mp_obj_get_int(args[5]) : 0; // Default color is BLACK

    brain_screen_draw_rectangle(self -> cpp_handle, x, y, width, height, color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_screen_draw_rectangle_obj, 5, 6,mp_brain_screen_draw_rectangle);

static mp_obj_t mp_brain_screen_draw_circle(size_t n_args, const mp_obj_t *args){
    mp_brain_screen_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int radius = mp_obj_get_int(args[3]);
    int color = mp_obj_get_int(args[4]);

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
    char* result = brain_sdcard_loadfile(self->cpp_handle, filename, buffer, buffer_size, &result_size);
    mp_obj_t output = mp_obj_new_str(result, result_size);
    if (buffer == NULL) {
        brain_sdcard_freefile(result);
    }
    return output;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_sdcard_loadfile_obj, 2, 3, mp_brain_sdcard_loadfile);

static mp_obj_t mp_brain_sdcard_savefile(size_t n_args, const mp_obj_t *args){
    mp_brain_sdcard_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    const char* filename = mp_obj_str_get_str(args[1]);
    const char* buffer = (n_args > 2 && args[2] != mp_const_none) ? mp_obj_str_get_str(args[2]) : NULL;

    brain_sdcard_savefile(self -> cpp_handle, filename, buffer);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_brain_sdcard_savefile_obj, 2, 3, mp_brain_sdcard_savefile);

static mp_obj_t mp_brain_sdcard_appendfile(mp_obj_t self_ptr, mp_obj_t filename_t, mp_obj_t buffer_t){
    mp_brain_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    const char* filename = mp_obj_str_get_str(filename_t);
    const char* buffer = mp_obj_str_get_str(buffer_t);

    brain_sdcard_appendfile(self -> cpp_handle, filename, buffer);
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
    mp_brain_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // dest[0] == MP_OBJ_NULL means a READ lookup property (e.g. print(brain.screen))
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
    // Fall back to standard method/attribute resolution
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
    MP_QSTR_Brain,
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
    MP_VEX_INCHES = 41
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
    MP_QSTR_ThreeWirePort,
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
    MP_QSTR_DigitalOut,
    MP_TYPE_FLAG_NONE,
    make_new, mp_digital_out_make_new,
    locals_dict, &mp_digital_out_locals_dict
);

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

static const mp_rom_map_elem_t mp_rotation_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_position), MP_ROM_PTR(&mp_rotation_set_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_angle), MP_ROM_PTR(&mp_rotation_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_position), MP_ROM_PTR(&mp_rotation_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_velocity), MP_ROM_PTR(&mp_rotation_velocity_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_position), MP_ROM_PTR(&mp_rotation_reset_position_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_reversed), MP_ROM_PTR(&mp_rotation_set_reversed_obj) },
    { MP_ROM_QSTR(MP_QSTR_changed), MP_ROM_PTR(&mp_rotation_changed_obj) },
};
static MP_DEFINE_CONST_DICT(mp_rotation_locals_dict, mp_rotation_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_rotation_type,
    MP_QSTR_Rotation,
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
    MP_QSTR_Inertial,
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
    MP_QSTR_Optical,
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
    MP_QSTR_Distance,
    MP_TYPE_FLAG_NONE,
    make_new, mp_distance_make_new,
    locals_dict, &mp_distance_locals_dict
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
    mp_controller_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // dest[0] == MP_OBJ_NULL means a READ lookup property (e.g. print(controller.screen))
    if (dest[0] == MP_OBJ_NULL) {
        switch (attr) {
            case MP_QSTR_screen:
                dest[0] = self->screen;
                return;
            case MP_QSTR_buttonA:
                dest[0] = self->buttonA;
                return;
            case MP_QSTR_buttonB:
                dest[0] = self->buttonB;
                return;
            case MP_QSTR_buttonX:
                dest[0] = self->buttonX;
                return;
            case MP_QSTR_buttonY:
                dest[0] = self->buttonY;
                return;
            case MP_QSTR_buttonUp:
                dest[0] = self->buttonUp;
                return;
            case MP_QSTR_buttonDown:
                dest[0] = self->buttonDown;
                return;
            case MP_QSTR_buttonLeft:
                dest[0] = self->buttonLeft;
                return;
            case MP_QSTR_buttonRight:
                dest[0] = self->buttonRight;
                return;
            case MP_QSTR_buttonL1:
                dest[0] = self->buttonL1;
                return;
            case MP_QSTR_buttonL2:
                dest[0] = self->buttonL2;
                return;
            case MP_QSTR_buttonR1:
                dest[0] = self->buttonR1;
                return;
            case MP_QSTR_buttonR2:
                dest[0] = self->buttonR2;
                return;
            case MP_QSTR_axis1:
                dest[0] = self->axis1;
                return;
            case MP_QSTR_axis2:
                dest[0] = self->axis2;
                return;
            case MP_QSTR_axis3:
                dest[0] = self->axis3;
                return;
            case MP_QSTR_axis4:
                dest[0] = self->axis4;
                return;    
        }
    }
    // Fall back to standard method/attribute resolution
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
    MP_QSTR_Controller,
    MP_TYPE_FLAG_NONE,
    make_new, mp_controller_make_new,
    attr, mp_controller_attr,
    locals_dict, &mp_controller_locals_dict
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
MP_DEFINE_CONST_OBJ_TYPE(mp_ports_type, MP_QSTR_Ports, MP_TYPE_FLAG_NONE, locals_dict, &mp_ports_locals_dict);

static const mp_rom_map_elem_t mp_velocity_units_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_PERCENT), MP_ROM_INT(MP_VEX_PERCENT) },
    { MP_ROM_QSTR(MP_QSTR_RPM), MP_ROM_INT(MP_VEX_RPM) },
    { MP_ROM_QSTR(MP_QSTR_DPS), MP_ROM_INT(MP_VEX_DPS) },
    { MP_ROM_QSTR(MP_QSTR_VOLT), MP_ROM_INT(39) },
};
static MP_DEFINE_CONST_DICT(mp_velocity_units_locals_dict, mp_velocity_units_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_velocity_units_type, MP_QSTR_VelocityUnits, MP_TYPE_FLAG_NONE, locals_dict, &mp_velocity_units_locals_dict);

static const mp_rom_map_elem_t mp_angle_units_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_DEGREES), MP_ROM_INT(MP_VEX_DEGREES) },
    { MP_ROM_QSTR(MP_QSTR_TURNS), MP_ROM_INT(MP_VEX_TURNS) },
};
static MP_DEFINE_CONST_DICT(mp_angle_units_locals_dict, mp_angle_units_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_angle_units_type, MP_QSTR_AngleUnits, MP_TYPE_FLAG_NONE, locals_dict, &mp_angle_units_locals_dict);

static const mp_rom_map_elem_t mp_axis_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_XAXIS), MP_ROM_INT(MP_VEX_AXIS_X) },
    { MP_ROM_QSTR(MP_QSTR_YAXIS), MP_ROM_INT(MP_VEX_AXIS_Y) },
    { MP_ROM_QSTR(MP_QSTR_ZAXIS), MP_ROM_INT(MP_VEX_AXIS_Z) },
    { MP_ROM_QSTR(MP_QSTR_PITCH), MP_ROM_INT(MP_VEX_PITCH) },
    { MP_ROM_QSTR(MP_QSTR_ROLL), MP_ROM_INT(MP_VEX_ROLL) },
    { MP_ROM_QSTR(MP_QSTR_YAW), MP_ROM_INT(MP_VEX_YAW) },
};
static MP_DEFINE_CONST_DICT(mp_axis_type_locals_dict, mp_axis_type_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_axis_type, MP_QSTR_AxisType, MP_TYPE_FLAG_NONE, locals_dict, &mp_axis_type_locals_dict);

static const mp_rom_map_elem_t mp_turn_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_RIGHT), MP_ROM_INT(MP_VEX_RIGHT) },
    { MP_ROM_QSTR(MP_QSTR_LEFT), MP_ROM_INT(MP_VEX_LEFT) },
};
static MP_DEFINE_CONST_DICT(mp_turn_type_locals_dict, mp_turn_type_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_turn_type, MP_QSTR_TurnType, MP_TYPE_FLAG_NONE, locals_dict, &mp_turn_type_locals_dict);

static const mp_rom_map_elem_t mp_led_state_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_ON), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_OFF), MP_ROM_INT(0) },
};
static MP_DEFINE_CONST_DICT(mp_led_state_type_locals_dict, mp_led_state_type_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_led_state_type, MP_QSTR_LEDStateType, MP_TYPE_FLAG_NONE, locals_dict, &mp_led_state_type_locals_dict);

static const mp_rom_map_elem_t mp_object_size_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_NONE), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_SMALL), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_MEDIUM), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_LARGE), MP_ROM_INT(3) },
};
static MP_DEFINE_CONST_DICT(mp_object_size_type_locals_dict, mp_object_size_type_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_object_size_type, MP_QSTR_ObjectSizeType, MP_TYPE_FLAG_NONE, locals_dict, &mp_object_size_type_locals_dict);

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
MP_DEFINE_CONST_OBJ_TYPE(mp_color_type, MP_QSTR_Color, MP_TYPE_FLAG_NONE, locals_dict, &mp_color_locals_dict);

static const mp_rom_map_elem_t mp_distance_units_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_MM), MP_ROM_INT(MP_VEX_MM) },
    { MP_ROM_QSTR(MP_QSTR_INCHES), MP_ROM_INT(MP_VEX_INCHES) },
};
static MP_DEFINE_CONST_DICT(mp_distance_units_locals_dict, mp_distance_units_locals_dict_table);
MP_DEFINE_CONST_OBJ_TYPE(mp_distance_units_type, MP_QSTR_DistanceUnits, MP_TYPE_FLAG_NONE, locals_dict, &mp_distance_units_locals_dict);

// This dictionary holds everything inside the top-level namespace: 'import vex'
static const mp_rom_map_elem_t vex_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_vex) },
    
    // Bind Classes into the module namespace
    { MP_ROM_QSTR(MP_QSTR_Motor), MP_ROM_PTR(&mp_motor_type) },
    { MP_ROM_QSTR(MP_QSTR_MotorGroup), MP_ROM_PTR(&mp_motorgroup_type) },
    { MP_ROM_QSTR(MP_QSTR_Timer), MP_ROM_PTR(&mp_timer_type) },
    { MP_ROM_QSTR(MP_QSTR_Brain), MP_ROM_PTR(&mp_brain_type) },
    { MP_ROM_QSTR(MP_QSTR_Controller), MP_ROM_PTR(&mp_controller_type) },
    { MP_ROM_QSTR(MP_QSTR_Rotation), MP_ROM_PTR(&mp_rotation_type) },
    { MP_ROM_QSTR(MP_QSTR_Inertial), MP_ROM_PTR(&mp_inertial_type) },
    { MP_ROM_QSTR(MP_QSTR_Optical), MP_ROM_PTR(&mp_optical_type) },
    { MP_ROM_QSTR(MP_QSTR_Distance), MP_ROM_PTR(&mp_distance_type) },
    { MP_ROM_QSTR(MP_QSTR_DigitalOut), MP_ROM_PTR(&mp_digital_out_type) },
    { MP_ROM_QSTR(MP_QSTR_Ports), MP_ROM_PTR(&mp_ports_type) },
    { MP_ROM_QSTR(MP_QSTR_VelocityUnits), MP_ROM_PTR(&mp_velocity_units_type) },
    { MP_ROM_QSTR(MP_QSTR_AngleUnits), MP_ROM_PTR(&mp_angle_units_type) },
    { MP_ROM_QSTR(MP_QSTR_AxisType), MP_ROM_PTR(&mp_axis_type) },
    { MP_ROM_QSTR(MP_QSTR_TurnType), MP_ROM_PTR(&mp_turn_type) },
    { MP_ROM_QSTR(MP_QSTR_LEDStateType), MP_ROM_PTR(&mp_led_state_type) },
    { MP_ROM_QSTR(MP_QSTR_ObjectSizeType), MP_ROM_PTR(&mp_object_size_type) },
    { MP_ROM_QSTR(MP_QSTR_Color), MP_ROM_PTR(&mp_color_type) },
    { MP_ROM_QSTR(MP_QSTR_DistanceUnits), MP_ROM_PTR(&mp_distance_units_type) },

    // Bind Global Enum Constants directly to the module scope
    { MP_ROM_QSTR(MP_QSTR_RATIO_6_1),  MP_ROM_INT(8) },
    { MP_ROM_QSTR(MP_QSTR_RATIO_18_1), MP_ROM_INT(18) },
    { MP_ROM_QSTR(MP_QSTR_RATIO_36_1), MP_ROM_INT(36) },
    { MP_ROM_QSTR(MP_QSTR_MASTER),     MP_ROM_INT(50) },
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
    { MP_ROM_QSTR(MP_QSTR_SECONDS),     MP_ROM_INT(37) },
    { MP_ROM_QSTR(MP_QSTR_MSEC),        MP_ROM_INT(38) },
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