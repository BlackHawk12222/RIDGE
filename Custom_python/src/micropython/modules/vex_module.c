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

extern const mp_obj_type_t mp_motor_type;
extern const mp_obj_type_t mp_motorgroup_type;

static mp_obj_t mp_motor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 3, 3, false); // strict 3 arguments
    int port = mp_obj_get_int(args[0]);
    int gears = mp_obj_get_int(args[1]);
    bool reversed = mp_obj_is_true(args[2]);

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
    int units = (n_args > 3) ? mp_obj_get_int(args[3])   : 3;      // DEGREES macro value
    float velocity = (n_args > 4) ? mp_obj_get_float(args[4]) : 601.0f;
    int units_v = (n_args > 5) ? mp_obj_get_int(args[5])   : 2;      // RPM macro value
    bool wait = (n_args > 6) ? mp_obj_is_true(args[6])   : true;

    motor_spin_for(self -> cpp_handle, direction, angle, units, velocity, units_v, wait);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_spin_for_obj, 3, 7, mp_motor_spin_for);

static mp_obj_t mp_motor_spin_to_position(size_t n_args, const mp_obj_t *args){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int rotation = mp_obj_get_int(args[2]);

    int units = (n_args > 3) ? mp_obj_get_int(args[3])   : 3;      // DEGREES macro value
    float velocity = (n_args > 4) ? mp_obj_get_float(args[4]) : 601.0f;
    int units_v = (n_args > 5) ? mp_obj_get_int(args[5])   : 2;      // RPM macro value
    bool wait = (n_args > 6) ? mp_obj_is_true(args[6])   : true;

    motor_spin_to_position(self -> cpp_handle, rotation, units, velocity, units_v, wait);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motor_spin_to_position_obj, 2, 7, mp_motor_spin_to_position);

static mp_obj_t mp_motor_stop(size_t n_args, const mp_obj_t *args){
    mp_motor_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    int mode = (n_args > 1) ? mp_obj_get_int(args[1])   : -1;

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
    { MP_ROM_QSTR(MP_QSTR_set_timeout), MP_ROM_PTR(&mp_motor_set_timeout_obj) }
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

static mp_obj_t mp_motorgroup_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
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

// Instance Method: maps to 'mg.spin(...)'
static mp_obj_t mp_motorgroup_spin(size_t n_args, const mp_obj_t *args) {
    mp_motorgroup_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    int direction = mp_obj_get_int(args[1]);
    float velocity = mp_obj_get_float(args[2]);
    int units = mp_obj_get_int(args[3]);

    motor_group_spin(self->cpp_handle, direction, velocity, units);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_motorgroup_spin_obj, 4, 4, mp_motorgroup_spin);

static const mp_rom_map_elem_t mp_motorgroup_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_spin), MP_ROM_PTR(&mp_motorgroup_spin_obj) },
};
static MP_DEFINE_CONST_DICT(mp_motorgroup_locals_dict, mp_motorgroup_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_motorgroup_type,
    MP_QSTR_MotorGroup,             // Name exposed to Python
    MP_TYPE_FLAG_NONE,
    make_new, mp_motorgroup_make_new,
    locals_dict, &mp_motorgroup_locals_dict
);

// This dictionary holds everything inside the top-level namespace: 'import vex'
static const mp_rom_map_elem_t vex_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_vex) },
    
    // Bind Classes into the module namespace
    { MP_ROM_QSTR(MP_QSTR_Motor),       MP_ROM_PTR(&mp_motor_type) },
    { MP_ROM_QSTR(MP_QSTR_MotorGroup),  MP_ROM_PTR(&mp_motorgroup_type) },

    // Bind Global Enum Constants directly to the module scope
    { MP_ROM_QSTR(MP_QSTR_PORT1),       MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_RPM),         MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_DEGREES),     MP_ROM_INT(3) },
};
static MP_DEFINE_CONST_DICT(vex_module_globals, vex_module_globals_table);

// Packaging the structural footprint into a Module block
const mp_obj_module_t vex_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&vex_module_globals,
};

// 7. System Entry Hook: Tells the MicroPython parser to index this module
MP_REGISTER_MODULE(MP_QSTR_vex, vex_user_cmodule);
