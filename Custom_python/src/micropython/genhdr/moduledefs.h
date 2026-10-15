#ifndef MICROPY_INCLUDED_GENHDR_MODULEDEFS_H
#define MICROPY_INCLUDED_GENHDR_MODULEDEFS_H

/* Forward declarations to tell the compiler these external pointers exist */
extern const struct _mp_obj_module_t mp_module_builtins;
extern const struct _mp_obj_module_t mp_module_sys;
extern const struct _mp_obj_module_t mp_module_collections;
extern const struct _mp_obj_module_t mp_module_math;
extern const struct _mp_obj_module_t mp_module_cmath;
extern const struct _mp_obj_module_t mp_module_gc;
extern const struct _mp_obj_module_t mp_module_array;
extern const struct _mp_obj_module_t mp_module_io;
extern const struct _mp_obj_module_t mp_module_errno;
extern const struct _mp_obj_module_t mp_module_struct;
extern const struct _mp_obj_module_t pros_user_module;

/* This table maps your compiled C modules directly into Python namespaces */
#define MICROPY_REGISTERED_MODULES \
    { MP_ROM_QSTR(MP_QSTR_builtins), MP_ROM_PTR(&mp_module_builtins) }, \
    { MP_ROM_QSTR(MP_QSTR_sys), MP_ROM_PTR(&mp_module_sys) }, \
    { MP_ROM_QSTR(MP_QSTR_collections), MP_ROM_PTR(&mp_module_collections) }, \
    { MP_ROM_QSTR(MP_QSTR_math), MP_ROM_PTR(&mp_module_math) }, \
    { MP_ROM_QSTR(MP_QSTR_cmath), MP_ROM_PTR(&mp_module_cmath) }, \
    { MP_ROM_QSTR(MP_QSTR_gc), MP_ROM_PTR(&mp_module_gc) }, \
    { MP_ROM_QSTR(MP_QSTR_array), MP_ROM_PTR(&mp_module_array) }, \
    { MP_ROM_QSTR(MP_QSTR_io), MP_ROM_PTR(&mp_module_io) }, \
    { MP_ROM_QSTR(MP_QSTR_errno), MP_ROM_PTR(&mp_module_errno) }, \
    { MP_ROM_QSTR(MP_QSTR_struct), MP_ROM_PTR(&mp_module_struct) }, \
    { MP_ROM_QSTR(MP_QSTR_pros), MP_ROM_PTR(&pros_user_module) }, \

#endif // MICROPY_INCLUDED_GENHDR_MODULEDEFS_H
