#include "py/runtime.h"
#include "py/obj.h"

extern void call_robot_print(const char* text);

extern mp_obj_t vex_motor_make_new(const mp_obj_type_t*, size_t, size_t, const mp_obj_t*);
extern mp_obj_t vex_motor_group_make_new(const mp_obj_type_t*, size_t, size_t, const mp_obj_t*);

/* The QSTR build scans C sources, so keep vex.cpp keyword names here. */
static const qstr vex_argument_qstrs[] = {
    MP_QSTR_port,
    MP_QSTR_gearSetting,
    MP_QSTR_reverse,
    MP_QSTR_direction,
    MP_QSTR_rotation,
    MP_QSTR_units_v,
    MP_QSTR_units,
    MP_QSTR_wait,
    MP_QSTR_brakeType,
    MP_QSTR_time,
    MP_QSTR_value,
};

#define VEX_DECLARE_METHOD(name) extern const mp_obj_fun_builtin_var_t vex_motor_##name##_obj
#define VEX_DECLARE_FIXED_METHOD(name) extern const mp_obj_fun_builtin_fixed_t vex_motor_##name##_obj

VEX_DECLARE_METHOD(spin);
VEX_DECLARE_METHOD(spin_for);
VEX_DECLARE_METHOD(spin_to_position);
VEX_DECLARE_METHOD(stop);
VEX_DECLARE_METHOD(set_position);
VEX_DECLARE_METHOD(set_velocity);
VEX_DECLARE_METHOD(set_stopping);
VEX_DECLARE_METHOD(set_max_torque);
VEX_DECLARE_METHOD(set_timeout);
VEX_DECLARE_FIXED_METHOD(set_reversed);
VEX_DECLARE_FIXED_METHOD(reset_position);
VEX_DECLARE_FIXED_METHOD(is_done);
VEX_DECLARE_FIXED_METHOD(is_spinning);
VEX_DECLARE_METHOD(position);
VEX_DECLARE_METHOD(velocity);
VEX_DECLARE_METHOD(current);
VEX_DECLARE_METHOD(power);
VEX_DECLARE_METHOD(torque);
VEX_DECLARE_METHOD(efficiency);
VEX_DECLARE_METHOD(temperature);
VEX_DECLARE_FIXED_METHOD(get_timeout);
VEX_DECLARE_FIXED_METHOD(count);
VEX_DECLARE_FIXED_METHOD(del);

#undef VEX_DECLARE_METHOD
#undef VEX_DECLARE_FIXED_METHOD

static mp_obj_t brain_print(mp_obj_t text_obj) {
    (void)vex_argument_qstrs;
    const char* str = mp_obj_str_get_str(text_obj);
    call_robot_print(str);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(brain_print_obj, brain_print);

#define VEX_MOTOR_METHODS \
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&vex_motor_del_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_spin), MP_ROM_PTR(&vex_motor_spin_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_spin_for), MP_ROM_PTR(&vex_motor_spin_for_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_spin_to_position), MP_ROM_PTR(&vex_motor_spin_to_position_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&vex_motor_stop_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_set_position), MP_ROM_PTR(&vex_motor_set_position_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_set_velocity), MP_ROM_PTR(&vex_motor_set_velocity_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_set_stopping), MP_ROM_PTR(&vex_motor_set_stopping_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_set_max_torque), MP_ROM_PTR(&vex_motor_set_max_torque_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_set_timeout), MP_ROM_PTR(&vex_motor_set_timeout_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_set_reversed), MP_ROM_PTR(&vex_motor_set_reversed_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_reset_position), MP_ROM_PTR(&vex_motor_reset_position_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_is_done), MP_ROM_PTR(&vex_motor_is_done_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_is_spinning), MP_ROM_PTR(&vex_motor_is_spinning_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_position), MP_ROM_PTR(&vex_motor_position_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_velocity), MP_ROM_PTR(&vex_motor_velocity_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_current), MP_ROM_PTR(&vex_motor_current_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_power), MP_ROM_PTR(&vex_motor_power_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_torque), MP_ROM_PTR(&vex_motor_torque_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_efficiency), MP_ROM_PTR(&vex_motor_efficiency_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_temperature), MP_ROM_PTR(&vex_motor_temperature_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_get_timeout), MP_ROM_PTR(&vex_motor_get_timeout_obj) }

static const mp_rom_map_elem_t vex_motor_locals_table[] = {
    VEX_MOTOR_METHODS,
};
static MP_DEFINE_CONST_DICT(vex_motor_locals, vex_motor_locals_table);

static const mp_rom_map_elem_t vex_motor_group_locals_table[] = {
    VEX_MOTOR_METHODS,
    { MP_ROM_QSTR(MP_QSTR_count), MP_ROM_PTR(&vex_motor_count_obj) },
};
static MP_DEFINE_CONST_DICT(vex_motor_group_locals, vex_motor_group_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    vex_motor_type,
    MP_QSTR_Motor,
    MP_TYPE_FLAG_NONE,
    make_new, vex_motor_make_new,
    locals_dict, (mp_obj_dict_t*)&vex_motor_locals
);

MP_DEFINE_CONST_OBJ_TYPE(
    vex_motor_group_type,
    MP_QSTR_MotorGroup,
    MP_TYPE_FLAG_NONE,
    make_new, vex_motor_group_make_new,
    locals_dict, (mp_obj_dict_t*)&vex_motor_group_locals
);

#define VEX_DEFINE_CONSTANT_TYPE(type_name, qstr_name, table_name, dict_name, ...) \
    static const mp_rom_map_elem_t table_name[] = { __VA_ARGS__ }; \
    static MP_DEFINE_CONST_DICT(dict_name, table_name); \
    MP_DEFINE_CONST_OBJ_TYPE(type_name, qstr_name, MP_TYPE_FLAG_NONE, \
        locals_dict, (mp_obj_dict_t*)&dict_name)

VEX_DEFINE_CONSTANT_TYPE(
    vex_ports_type, MP_QSTR_Ports, vex_ports_table, vex_ports_dict,
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
    { MP_ROM_QSTR(MP_QSTR_PORT21), MP_ROM_INT(21) }
);

VEX_DEFINE_CONSTANT_TYPE(
    vex_gear_setting_type, MP_QSTR_GearSetting, vex_gear_setting_table, vex_gear_setting_dict,
    { MP_ROM_QSTR(MP_QSTR_RATIO_6_1), MP_ROM_INT(6) },
    { MP_ROM_QSTR(MP_QSTR_RATIO_18_1), MP_ROM_INT(18) },
    { MP_ROM_QSTR(MP_QSTR_RATIO_36_1), MP_ROM_INT(36) }
);

VEX_DEFINE_CONSTANT_TYPE(
    vex_velocity_units_type, MP_QSTR_VelocityUnits, vex_velocity_units_table, vex_velocity_units_dict,
    { MP_ROM_QSTR(MP_QSTR_PERCENT), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_VELOCITY_PERCENT), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_RPM), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_VOLT), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_DPS), MP_ROM_INT(4) }
);

VEX_DEFINE_CONSTANT_TYPE(
    vex_direction_type, MP_QSTR_DirectionType, vex_direction_table, vex_direction_dict,
    { MP_ROM_QSTR(MP_QSTR_FORWARD), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_REVERSE), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_FWD), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_REV), MP_ROM_INT(1) }
);

VEX_DEFINE_CONSTANT_TYPE(
    vex_angle_units_type, MP_QSTR_AngleUnits, vex_angle_units_table, vex_angle_units_dict,
    { MP_ROM_QSTR(MP_QSTR_DEGREES), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_DEG), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_TURNS), MP_ROM_INT(2) }
);

MP_DEFINE_CONST_OBJ_TYPE(
    vex_rotation_units_type,
    MP_QSTR_RotationUnits,
    MP_TYPE_FLAG_NONE,
    locals_dict, (mp_obj_dict_t*)&vex_angle_units_dict
);

VEX_DEFINE_CONSTANT_TYPE(
    vex_brake_type, MP_QSTR_BrakeType, vex_brake_table, vex_brake_dict,
    { MP_ROM_QSTR(MP_QSTR_COAST), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_BRAKE), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_HOLD), MP_ROM_INT(2) }
);

VEX_DEFINE_CONSTANT_TYPE(
    vex_torque_units_type, MP_QSTR_TorqueUnits, vex_torque_units_table, vex_torque_units_dict,
    { MP_ROM_QSTR(MP_QSTR_PERCENT), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_NM), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_INLB), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_IN_LB), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_TORQUE_PERCENT), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_TORQUE_AMPS), MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_AMP), MP_ROM_INT(4) }
);

VEX_DEFINE_CONSTANT_TYPE(
    vex_time_units_type, MP_QSTR_TimeUnits, vex_time_units_table, vex_time_units_dict,
    { MP_ROM_QSTR(MP_QSTR_SECONDS), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_MSEC), MP_ROM_INT(2) }
);

VEX_DEFINE_CONSTANT_TYPE(
    vex_current_units_type, MP_QSTR_CurrentUnits, vex_current_units_table, vex_current_units_dict,
    { MP_ROM_QSTR(MP_QSTR_AMP), MP_ROM_INT(5) },
    { MP_ROM_QSTR(MP_QSTR_CURRENT_AMPS), MP_ROM_INT(5) }
);

VEX_DEFINE_CONSTANT_TYPE(
    vex_power_units_type, MP_QSTR_PowerUnits, vex_power_units_table, vex_power_units_dict,
    { MP_ROM_QSTR(MP_QSTR_WATT), MP_ROM_INT(1) }
);

VEX_DEFINE_CONSTANT_TYPE(
    vex_efficiency_units_type, MP_QSTR_EfficiencyUnits, vex_efficiency_units_table, vex_efficiency_units_dict,
    { MP_ROM_QSTR(MP_QSTR_PERCENT), MP_ROM_INT(6) },
    { MP_ROM_QSTR(MP_QSTR_EFFICIENCY_PERCENT), MP_ROM_INT(6) }
);

VEX_DEFINE_CONSTANT_TYPE(
    vex_temperature_units_type, MP_QSTR_TemperatureUnits, vex_temperature_units_table, vex_temperature_units_dict,
    { MP_ROM_QSTR(MP_QSTR_CELSIUS), MP_ROM_INT(101) },
    { MP_ROM_QSTR(MP_QSTR_FAHRENHEIT), MP_ROM_INT(102) },
    { MP_ROM_QSTR(MP_QSTR_PERCENT), MP_ROM_INT(7) },
    { MP_ROM_QSTR(MP_QSTR_TEMPERATURE_PERCENT), MP_ROM_INT(7) }
);

#undef VEX_DEFINE_CONSTANT_TYPE
#undef VEX_MOTOR_METHODS

static const mp_rom_map_elem_t vex_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_vex) },
    { MP_ROM_QSTR(MP_QSTR_brain_print), MP_ROM_PTR(&brain_print_obj) },
    { MP_ROM_QSTR(MP_QSTR_Motor), MP_ROM_PTR(&vex_motor_type) },
    { MP_ROM_QSTR(MP_QSTR_MotorGroup), MP_ROM_PTR(&vex_motor_group_type) },
    { MP_ROM_QSTR(MP_QSTR_Ports), MP_ROM_PTR(&vex_ports_type) },
    { MP_ROM_QSTR(MP_QSTR_GearSetting), MP_ROM_PTR(&vex_gear_setting_type) },
    { MP_ROM_QSTR(MP_QSTR_VelocityUnits), MP_ROM_PTR(&vex_velocity_units_type) },
    { MP_ROM_QSTR(MP_QSTR_DirectionType), MP_ROM_PTR(&vex_direction_type) },
    { MP_ROM_QSTR(MP_QSTR_Direction), MP_ROM_PTR(&vex_direction_type) },
    { MP_ROM_QSTR(MP_QSTR_AngleUnits), MP_ROM_PTR(&vex_angle_units_type) },
    { MP_ROM_QSTR(MP_QSTR_RotationUnits), MP_ROM_PTR(&vex_rotation_units_type) },
    { MP_ROM_QSTR(MP_QSTR_BrakeType), MP_ROM_PTR(&vex_brake_type) },
    { MP_ROM_QSTR(MP_QSTR_TorqueUnits), MP_ROM_PTR(&vex_torque_units_type) },
    { MP_ROM_QSTR(MP_QSTR_TimeUnits), MP_ROM_PTR(&vex_time_units_type) },
    { MP_ROM_QSTR(MP_QSTR_CurrentUnits), MP_ROM_PTR(&vex_current_units_type) },
    { MP_ROM_QSTR(MP_QSTR_PowerUnits), MP_ROM_PTR(&vex_power_units_type) },
    { MP_ROM_QSTR(MP_QSTR_EfficiencyUnits), MP_ROM_PTR(&vex_efficiency_units_type) },
    { MP_ROM_QSTR(MP_QSTR_TemperatureUnits), MP_ROM_PTR(&vex_temperature_units_type) },
    { MP_ROM_QSTR(MP_QSTR_PERCENT), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_RPM), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_VOLT), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_DPS), MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_FORWARD), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_REVERSE), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_DEGREES), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_TURNS), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_COAST), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_BRAKE), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_HOLD), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_NM), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_INLB), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_AMP), MP_ROM_INT(5) },
    { MP_ROM_QSTR(MP_QSTR_SECONDS), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_MSEC), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_WATT), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_CELSIUS), MP_ROM_INT(101) },
    { MP_ROM_QSTR(MP_QSTR_FAHRENHEIT), MP_ROM_INT(102) },
    { MP_ROM_QSTR(MP_QSTR_FWD), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_REV), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_IN_LB), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_TORQUE_AMPS), MP_ROM_INT(4) },
};
static MP_DEFINE_CONST_DICT(vex_globals, vex_globals_table);

const mp_obj_module_t vex_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&vex_globals,
};

MP_REGISTER_MODULE(MP_QSTR_vex, vex_user_cmodule);