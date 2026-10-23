#include "py/runtime.h"
#include "py/obj.h"
#include "vex/vex.h"

typedef struct _mp_motor_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_motor_obj_t;

typedef struct _mp_motorgroup_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_motorgroup_obj_t;


typedef struct _mp_timer_obj_t{
    mp_obj_base_t base;
    void *cpp_handle;
} mp_timer_obj_t;

extern const mp_obj_type_t mp_motor_type;
extern const mp_obj_type_t mp_motorgroup_type;
extern const mp_obj_type_t mp_timer_type;

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
    int rotation = mp_obj_get_int(args[2]);

    int units = (n_args > 3) ? mp_obj_get_int(args[3])   : 29;      // DEGREES macro value
    float velocity = (n_args > 4) ? mp_obj_get_float(args[4]) : 601.0f;
    int units_v = (n_args > 5) ? mp_obj_get_int(args[5])   : 24;      // RPM macro value
    bool wait = (n_args > 6) ? mp_obj_is_true(args[6])   : true;

    motor_spin_to_position(self -> cpp_handle, rotation, units, velocity, units_v, wait);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_spin_to_position_obj, 2, 7, mp_motor_spin_to_position);

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
    return mp_obj_new_float(output);
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
    int rotation = mp_obj_get_int(args[2]);

    int units = (n_args > 3) ? mp_obj_get_int(args[3])   : 29;      // DEGREES macro value
    float velocity = (n_args > 4) ? mp_obj_get_float(args[4]) : 601.0f;
    int units_v = (n_args > 5) ? mp_obj_get_int(args[5])   : 24;      // RPM macro value
    bool wait = (n_args > 6) ? mp_obj_is_true(args[6])   : true;

    motor_group_spin_to_position(self -> cpp_handle, rotation, units, velocity, units_v, wait);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_group_spin_to_position_obj, 2, 7, mp_motor_group_spin_to_position);

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
    return mp_obj_new_float(output);
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

    vex_timer_event(self -> cpp_handle, callback_func, delay);
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

// This dictionary holds everything inside the top-level namespace: 'import vex'
static const mp_rom_map_elem_t vex_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_vex) },
    
    // Bind Classes into the module namespace
    { MP_ROM_QSTR(MP_QSTR_Motor), MP_ROM_PTR(&mp_motor_type) },
    { MP_ROM_QSTR(MP_QSTR_MotorGroup), MP_ROM_PTR(&mp_motorgroup_type) },
    { MP_ROM_QSTR(MP_QSTR_Timer), MP_ROM_PTR(&mp_timer_type)},

    // Bind Global Enum Constants directly to the module scope
    { MP_ROM_QSTR(MP_QSTR_NALL),         MP_ROM_INT(-1) },
    { MP_ROM_QSTR(MP_QSTR_COAST),         MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_BRAKE),         MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_HOLD),         MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_PORT1),       MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_PORT2),       MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_PORT3),       MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_PORT4),       MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_PORT5),       MP_ROM_INT(5) },
    { MP_ROM_QSTR(MP_QSTR_PORT6),       MP_ROM_INT(6) },
    { MP_ROM_QSTR(MP_QSTR_PORT7),       MP_ROM_INT(7) },
    { MP_ROM_QSTR(MP_QSTR_PORT8),       MP_ROM_INT(8) },
    { MP_ROM_QSTR(MP_QSTR_PORT9),       MP_ROM_INT(9) },
    { MP_ROM_QSTR(MP_QSTR_PORT10),       MP_ROM_INT(10) },
    { MP_ROM_QSTR(MP_QSTR_PORT11),       MP_ROM_INT(11) },
    { MP_ROM_QSTR(MP_QSTR_PORT12),       MP_ROM_INT(12) },
    { MP_ROM_QSTR(MP_QSTR_PORT13),       MP_ROM_INT(13) },
    { MP_ROM_QSTR(MP_QSTR_PORT14),       MP_ROM_INT(14) },
    { MP_ROM_QSTR(MP_QSTR_PORT15),       MP_ROM_INT(15) },
    { MP_ROM_QSTR(MP_QSTR_PORT16),       MP_ROM_INT(16) },
    { MP_ROM_QSTR(MP_QSTR_PORT17),       MP_ROM_INT(17) },
    { MP_ROM_QSTR(MP_QSTR_PORT18),       MP_ROM_INT(18) },
    { MP_ROM_QSTR(MP_QSTR_PORT19),       MP_ROM_INT(19) },
    { MP_ROM_QSTR(MP_QSTR_PORT20),       MP_ROM_INT(20) },
    { MP_ROM_QSTR(MP_QSTR_PORT21),       MP_ROM_INT(21) },
    { MP_ROM_QSTR(MP_QSTR_PORT22),       MP_ROM_INT(22) }, 
    { MP_ROM_QSTR(MP_QSTR_PERCENT),     MP_ROM_INT(23) },
    { MP_ROM_QSTR(MP_QSTR_RPM),         MP_ROM_INT(24) },
    { MP_ROM_QSTR(MP_QSTR_VOLT),         MP_ROM_INT(25) },
    { MP_ROM_QSTR(MP_QSTR_DPS),         MP_ROM_INT(26) },
    { MP_ROM_QSTR(MP_QSTR_FORWARD),         MP_ROM_INT(27) },
    { MP_ROM_QSTR(MP_QSTR_REVERSE),         MP_ROM_INT(28) },
    { MP_ROM_QSTR(MP_QSTR_DEGREES),     MP_ROM_INT(29) },
    { MP_ROM_QSTR(MP_QSTR_TURNS),         MP_ROM_INT(30) },
    { MP_ROM_QSTR(MP_QSTR_NM),         MP_ROM_INT(31) },
    { MP_ROM_QSTR(MP_QSTR_INLB),         MP_ROM_INT(32) },
    { MP_ROM_QSTR(MP_QSTR_AMPS),         MP_ROM_INT(33) },
    { MP_ROM_QSTR(MP_QSTR_WATT),         MP_ROM_INT(34) },
    { MP_ROM_QSTR(MP_QSTR_CELSIUS),         MP_ROM_INT(35) },
    { MP_ROM_QSTR(MP_QSTR_FAHRENHEIT),         MP_ROM_INT(36) },
    { MP_ROM_QSTR(MP_QSTR_SECONDS),         MP_ROM_INT(37) },
    { MP_ROM_QSTR(MP_QSTR_MSEC),         MP_ROM_INT(38) },
};
static MP_DEFINE_CONST_DICT(vex_module_globals, vex_module_globals_table);

// Packaging the structural footprint into a Module block
const mp_obj_module_t vex_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&vex_module_globals,
};

// 7. System Entry Hook: Tells the MicroPython parser to index this module
MP_REGISTER_MODULE(MP_QSTR_vex, vex_user_cmodule);