#include "py/mpconfig.h"  // Explicitly include this first to help IntelliSense
#include "py/obj.h"
#include "py/runtime.h"
#include "api.h"          // Core PROS API Header

// 1. Define the custom Motor Object Structure
typedef struct _mp_obj_pros_motor_t {
    mp_obj_base_t base;
    uint8_t port;
} mp_obj_pros_motor_t;

// Forward declaration of the MicroPython type object
const mp_obj_type_t pros_motor_type;

// 2. Motor Constructor: pros.Motor(port_number)
static mp_obj_t pros_motor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    
    mp_obj_pros_motor_t *self = mp_obj_malloc(mp_obj_pros_motor_t, type);
    self->port = (uint8_t)mp_obj_get_int(args[0]);
    
    return MP_OBJ_FROM_PTR(self);
}

// 3. Method: motor.move_velocity(speed)
static mp_obj_t pros_motor_move_velocity(mp_obj_t self_in, mp_obj_t velocity_in) {
    mp_obj_pros_motor_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t velocity = mp_obj_get_int(velocity_in);
    
    // Low latency execution link directly to PROS kernel
    motor_move_velocity(self->port, velocity); 
    
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(pros_motor_move_velocity_obj, pros_motor_move_velocity);

// 4. Register methods inside the object type local directory
static const mp_rom_map_elem_t pros_motor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_move_velocity), MP_ROM_PTR(&pros_motor_move_velocity_obj) },
};
static MP_DEFINE_CONST_DICT(pros_motor_locals_dict, pros_motor_locals_dict_table);

// 5. Define the Class Properties Layout
MP_DEFINE_CONST_OBJ_TYPE(
    pros_motor_type,
    MP_QSTR_Motor,
    MP_TYPE_FLAG_NONE,
    make_new, pros_motor_make_new,
    locals_dict, &pros_motor_locals_dict
);

// 6. Bind the class object to the root 'pros' module namespace
static const mp_rom_map_elem_t pros_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_pros) },
    { MP_ROM_QSTR(MP_QSTR_Motor), MP_ROM_PTR(&pros_motor_type) },
};
static MP_DEFINE_CONST_DICT(pros_module_globals, pros_module_globals_table);

// Define the structural module context configuration
const mp_obj_module_t pros_user_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pros_module_globals,
};

// Register the module directly to MicroPython 1.29 engine rules
MP_REGISTER_MODULE(MP_QSTR_pros, pros_user_module);
