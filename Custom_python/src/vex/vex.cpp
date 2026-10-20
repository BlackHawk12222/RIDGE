#include <vector>
#include <cmath>
#include "main.h"
#include "vex/vex.hpp"

extern "C" {
#include "py/runtime.h"
#include "py/obj.h"
}

extern "C" const mp_obj_type_t vex_motor_type;

extern "C" {
extern const mp_obj_fun_builtin_var_t vex_motor_spin_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_spin_for_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_spin_to_position_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_stop_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_set_position_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_set_velocity_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_set_stopping_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_set_max_torque_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_set_timeout_obj;
extern const mp_obj_fun_builtin_fixed_t vex_motor_set_reversed_obj;
extern const mp_obj_fun_builtin_fixed_t vex_motor_reset_position_obj;
extern const mp_obj_fun_builtin_fixed_t vex_motor_is_done_obj;
extern const mp_obj_fun_builtin_fixed_t vex_motor_is_spinning_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_position_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_velocity_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_current_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_power_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_torque_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_efficiency_obj;
extern const mp_obj_fun_builtin_var_t vex_motor_temperature_obj;
extern const mp_obj_fun_builtin_fixed_t vex_motor_get_timeout_obj;
extern const mp_obj_fun_builtin_fixed_t vex_motor_count_obj;
extern const mp_obj_fun_builtin_fixed_t vex_motor_del_obj;
}

namespace {

constexpr int kForward = 0;
constexpr int kReverse = 1;
constexpr int kPercent = 1;
constexpr int kRpm = 2;
constexpr int kVolt = 3;
constexpr int kDps = 4;
constexpr int kDegrees = 1;
constexpr int kTurns = 2;
constexpr int kTorquePercent = 1;
constexpr int kNm = 2;
constexpr int kInLb = 3;
constexpr int kTorqueAmps = 4;
constexpr int kSeconds = 1;
constexpr int kMilliseconds = 2;
constexpr int kCurrentAmps = 5;
constexpr int kWatts = 1;
constexpr int kEfficiencyPercent = 6;
constexpr int kCelsius = 101;
constexpr int kFahrenheit = 102;
constexpr int kTemperaturePercent = 7;

pros::MotorGears parse_gear_setting(int gear_setting) {
    switch (gear_setting) {
        case 6:
            return pros::MotorGears::ratio_6_to_1;
        case 18:
            return pros::MotorGears::ratio_18_to_1;
        case 36:
            return pros::MotorGears::ratio_36_to_1;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid gear setting"));
    }
}

void require_unit(bool valid) {
    if (!valid) {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid unit"));
    }
}

float numeric_value(mp_obj_t value) {
    return mp_obj_get_float_to_d(value);
}

struct mp_vex_motor_obj_t {
    mp_obj_base_t base;
    VexMotorHandle* handle;
};

mp_vex_motor_obj_t* motor_object(mp_obj_t self_in) {
    auto* self = static_cast<mp_vex_motor_obj_t*>(MP_OBJ_TO_PTR(self_in));
    if (self->handle == nullptr) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Motor object has been finalized"));
    }
    return self;
}

void parse_method_args(
    size_t n_args,
    const mp_obj_t* args,
    mp_map_t* kwargs,
    size_t n_allowed,
    const mp_arg_t* allowed,
    mp_arg_val_t* values
) {
    mp_arg_parse_all(n_args - 1, args + 1, kwargs, n_allowed, allowed, values);
}

mp_obj_t new_motor_object(const mp_obj_type_t* type, VexMotorHandle* handle) {
    mp_vex_motor_obj_t* self = mp_obj_malloc_with_finaliser(mp_vex_motor_obj_t, type);
    self->handle = handle;
    return MP_OBJ_FROM_PTR(self);
}

}  // namespace

VexMotorHandle::VexMotorHandle(int port, int gear_setting, bool reversed) {
    if (port < 1 || port > 21) {
        mp_raise_ValueError(MP_ERROR_TEXT("V5 motor port must be between 1 and 21"));
    }
    const auto gear = parse_gear_setting(gear_setting);
    const auto signed_port = static_cast<std::int8_t>(reversed ? -port : port);
    motor_ = std::make_unique<pros::Motor>(signed_port, gear, pros::MotorUnits::degrees);
}

VexMotorHandle::VexMotorHandle(const std::vector<VexMotorHandle*>& motors) {
    if (motors.empty()) {
        mp_raise_ValueError(MP_ERROR_TEXT("MotorGroup requires at least one Motor"));
    }

    std::vector<std::int8_t> ports;
    ports.reserve(motors.size());
    for (const VexMotorHandle* motor : motors) {
        if (motor == nullptr) {
            mp_raise_TypeError(MP_ERROR_TEXT("MotorGroup arguments must be Motor objects"));
        }
        const auto motor_ports = motor->device()->get_port_all();
        ports.insert(ports.end(), motor_ports.begin(), motor_ports.end());
    }
    if (ports.size() > 21) {
        mp_raise_ValueError(MP_ERROR_TEXT("MotorGroup has too many motors"));
    }

    const auto first_gearing = motors.front()->device()->get_gearing();
    group_ = std::make_unique<pros::MotorGroup>(ports, first_gearing, pros::MotorUnits::degrees);
    motor_count_ = static_cast<int>(ports.size());
}

VexMotorHandle::~VexMotorHandle() = default;

pros::AbstractMotor* VexMotorHandle::device() {
    return group_ != nullptr ? static_cast<pros::AbstractMotor*>(group_.get())
                             : static_cast<pros::AbstractMotor*>(motor_.get());
}

const pros::AbstractMotor* VexMotorHandle::device() const {
    return group_ != nullptr ? static_cast<const pros::AbstractMotor*>(group_.get())
                             : static_cast<const pros::AbstractMotor*>(motor_.get());
}

float VexMotorHandle::max_rpm() const {
    switch (device()->get_gearing()) {
        case pros::MotorGears::ratio_6_to_1:
            return 600.0f;
        case pros::MotorGears::ratio_18_to_1:
            return 200.0f;
        case pros::MotorGears::ratio_36_to_1:
            return 100.0f;
        default:
            return 100.0f;
    }
}

float VexMotorHandle::to_degrees(float rotation, int units) const {
    require_unit(units == kDegrees || units == kTurns);
    return units == kTurns ? rotation * 360.0 : rotation;
}

float VexMotorHandle::to_rpm(float value, int units) const {
    switch (units) {
        case kPercent:
            return static_cast<float>(value) * max_rpm() / 100.0f;
        case kRpm:
            return value;
        case kDps:
            return static_cast<float>(value) / 6.0f;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid velocity unit"));
    }
}

void VexMotorHandle::wait_for_position(float target_position) {
    const std::uint32_t start = pros::millis();
    while (std::fabs(device()->get_position() - target_position) > 1.0) {
        if (timeout_seconds_ > 0.0f &&
            static_cast<float>(pros::millis() - start) >= timeout_seconds_ * 1000.0f) {
            break;
        }
        pros::delay(10);
    }
}

void VexMotorHandle::spin(int direction, float velocity, int units) {
    require_unit(direction == kForward || direction == kReverse);
    if (velocity == 601.0f) {
        velocity = default_velocity_;
        units = default_velocity_units_;
    }
    const double sign = direction == kReverse ? -1.0 : 1.0;
    if (units == kVolt) {
        const double voltage = sign * velocity * 1000.0;
        if (std::fabs(voltage) > 12000.0) {
            mp_raise_ValueError(MP_ERROR_TEXT("Voltage must be between -12 and 12"));
        }
        device()->move_voltage(static_cast<std::int32_t>(std::lround(voltage)));
        return;
    }
    device()->move_velocity(static_cast<std::int32_t>(
        std::lround(sign * to_rpm(velocity, units))));
}

void VexMotorHandle::spin_for(
    int direction,
    float rotation,
    int angle_units,
    float velocity,
    int velocity_units,
    bool wait
) {
    require_unit(direction == kForward || direction == kReverse);
    if (velocity == 601.0f) {
        velocity = default_velocity_;
        velocity_units = default_velocity_units_;
    }
    require_unit(velocity_units == kPercent || velocity_units == kRpm || velocity_units == kDps);
    const float angle = to_degrees(rotation, angle_units) * (direction == kReverse ? -1.0 : 1.0);
    const float target_position = device()->get_position() + angle;
    device()->move_relative(angle, static_cast<std::int32_t>(
        std::lround(to_rpm(velocity, velocity_units))));
    if (wait) {
        wait_for_position(target_position);
    }
}

void VexMotorHandle::spin_to_position(
    float rotation,
    int angle_units,
    float velocity,
    int velocity_units,
    bool wait
) {
    if (velocity == 601.0f) {
        velocity = default_velocity_;
        velocity_units = default_velocity_units_;
    }
    require_unit(velocity_units == kPercent || velocity_units == kRpm || velocity_units == kDps);
    const float target_position = to_degrees(rotation, angle_units);
    device()->move_absolute(target_position, static_cast<std::int32_t>(
        std::lround(to_rpm(velocity, velocity_units))));
    if (wait) {
        wait_for_position(target_position);
    }
}

void VexMotorHandle::stop(int brake_mode) {
    const auto mode = brake_mode < 0 ? default_brake_mode_
                                     : static_cast<pros::MotorBrake>(brake_mode);
    require_unit(brake_mode < 0 || brake_mode == 0 || brake_mode == 1 || brake_mode == 2);
    device()->set_brake_mode_all(mode);
    device()->brake();
}

void VexMotorHandle::set_position(float position, int angle_units) {
    device()->set_zero_position_all(to_degrees(position, angle_units));
}

void VexMotorHandle::set_velocity(float velocity, int units) {
    require_unit(units == kPercent || units == kRpm || units == kDps);
    default_velocity_ = velocity;
    default_velocity_units_ = units;
}

void VexMotorHandle::set_stopping(int brake_mode) {
    require_unit(brake_mode == 0 || brake_mode == 1 || brake_mode == 2);
    default_brake_mode_ = static_cast<pros::MotorBrake>(brake_mode);
    device()->set_brake_mode_all(default_brake_mode_);
}

void VexMotorHandle::set_max_torque(float value, int units) {
    if (value <= 0.0f) {
        mp_raise_ValueError(MP_ERROR_TEXT("Torque must be greater than zero"));
    }

    double limit_ma = 0.0;
    switch (units) {
        case kTorquePercent:
            require_unit(value <= 100.0f);
            limit_ma = value * 25.0f;
            break;
        case kNm: {
            double max_nm = 1.0;
            switch (device()->get_gearing()) {
                case pros::MotorGears::ratio_6_to_1: max_nm = 0.35f; break;
                case pros::MotorGears::ratio_18_to_1: max_nm = 1.05f; break;
                case pros::MotorGears::ratio_36_to_1: max_nm = 2.1f; break;
                default: break;
            }
            limit_ma = max_nm / value * 2500.0;
            break;
        }
        case kInLb: {
            double max_in_lb = 1.0;
            switch (device()->get_gearing()) {
                case pros::MotorGears::ratio_6_to_1: max_in_lb = 3.1f; break;
                case pros::MotorGears::ratio_18_to_1: max_in_lb = 9.29f; break;
                case pros::MotorGears::ratio_36_to_1: max_in_lb = 18.59f; break;
                default: break;
            }
            limit_ma = max_in_lb / value * 2500.0f;
            break;
        }
        case kTorqueAmps:
        case kCurrentAmps:
            limit_ma = value * 1000.0f;
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid torque unit"));
    }
    device()->set_current_limit_all(static_cast<std::int32_t>(std::lround(limit_ma)));
}

void VexMotorHandle::set_timeout(float value, int units) {
    require_unit(units == kSeconds || units == kMilliseconds);
    if (value < 0.0f) {
        mp_raise_ValueError(MP_ERROR_TEXT("Timeout cannot be negative"));
    }
    timeout_seconds_ = units == kMilliseconds ? value / 1000.0f : value;
}

void VexMotorHandle::set_reversed(bool reversed) {
    device()->set_reversed_all(reversed);
}

void VexMotorHandle::reset_position() {
    device()->tare_position_all();
}

bool VexMotorHandle::is_done() const {
    return std::fabs(device()->get_actual_velocity()) < 1.0f;
}

bool VexMotorHandle::is_spinning() const {
    return !is_done();
}

float VexMotorHandle::position(int units) const {
    require_unit(units == kDegrees || units == kTurns);
    const float degrees = device()->get_position();
    return units == kTurns ? degrees / 360.0f : degrees;
}

float VexMotorHandle::velocity(int units) const {
    const double rpm = device()->get_actual_velocity();
    switch (units) {
        case kPercent:
            return rpm * 100.0 / max_rpm();
        case kRpm:
            return rpm;
        case kDps:
            return rpm * 6.0;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid velocity unit"));
    }
}

float VexMotorHandle::current(int units) const {
    require_unit(units == kCurrentAmps);
    return static_cast<double>(device()->get_current_draw()) / 1000.0;
}

float VexMotorHandle::power(int units) const {
    require_unit(units == kWatts);
    return device()->get_power();
}

float VexMotorHandle::torque(int units) const {
    const float torque_nm = device()->get_torque();
    switch (units) {
        case kNm:
            return torque_nm;
        case kInLb:
            return torque_nm * 8.8507f;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid torque unit"));
    }
}

float VexMotorHandle::efficiency(int units) const {
    require_unit(units == kEfficiencyPercent || units == kPercent);
    return device()->get_efficiency();
}

int VexMotorHandle::temperature(int units) const {
    const float celsius = device()->get_temperature();
    switch (units) {
        case kCelsius:
            return static_cast<int>(std::lround(celsius));
        case kFahrenheit:
            return static_cast<int>(std::lround(celsius * 1.8f + 32.0f));
        case kTemperaturePercent:
        case kPercent:
            return static_cast<int>(std::lround(celsius));
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid temperature unit"));
    }
}

float VexMotorHandle::get_timeout() const {
    return timeout_seconds_ < 0.0f ? 0.0f : timeout_seconds_;
}

int VexMotorHandle::count() const {
    return motor_count_;
}

extern "C" mp_obj_t vex_motor_make_new(
    const mp_obj_type_t* type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t* args
) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_port, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_gearSetting, MP_ARG_INT, {.u_int = 18} },
        { MP_QSTR_reverse, MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(
        n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, values);

    auto* handle = new VexMotorHandle(
        values[0].u_int, values[1].u_int, values[2].u_bool);
    return new_motor_object(type, handle);
}

extern "C" mp_obj_t vex_motor_group_make_new(
    const mp_obj_type_t* type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t* args
) {
    if (n_kw != 0 || n_args == 0) {
        mp_raise_TypeError(MP_ERROR_TEXT("MotorGroup expects one or more Motor objects"));
    }
    std::vector<VexMotorHandle*> motors;
    motors.reserve(n_args);
    for (size_t i = 0; i < n_args; ++i) {
        if (!mp_obj_is_type(args[i], &vex_motor_type)) {
            mp_raise_TypeError(MP_ERROR_TEXT("MotorGroup arguments must be Motor objects"));
        }
        motors.push_back(motor_object(args[i])->handle);
    }
    return new_motor_object(type, new VexMotorHandle(motors));
}

extern "C" mp_obj_t vex_motor_del(mp_obj_t self_in) {
    auto* self = static_cast<mp_vex_motor_obj_t*>(MP_OBJ_TO_PTR(self_in));
    delete self->handle;
    self->handle = nullptr;
    return mp_const_none;
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_1(vex_motor_del_obj, vex_motor_del);

extern "C" mp_obj_t vex_motor_spin(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {
        { MP_QSTR_direction, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_velocity, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(601)} },
        { MP_QSTR_units, MP_ARG_INT, {.u_int = kPercent} },
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    motor_object(args[0])->handle->spin(values[0].u_int,
        static_cast<float>(numeric_value(values[1].u_obj)), values[2].u_int);
    return mp_const_none;
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_spin_obj, 1, vex_motor_spin);

extern "C" mp_obj_t vex_motor_spin_for(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {
        { MP_QSTR_direction, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_rotation, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_units, MP_ARG_INT, {.u_int = kDegrees} },
        { MP_QSTR_velocity, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(601)} },
        { MP_QSTR_units_v, MP_ARG_INT, {.u_int = kPercent} },
        { MP_QSTR_wait, MP_ARG_BOOL, {.u_bool = true} },
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    motor_object(args[0])->handle->spin_for(values[0].u_int,
        numeric_value(values[1].u_obj), values[2].u_int,
        static_cast<float>(numeric_value(values[3].u_obj)),
        values[4].u_int, values[5].u_bool);
    return mp_const_none;
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_spin_for_obj, 1, vex_motor_spin_for);

extern "C" mp_obj_t vex_motor_spin_to_position(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {
        { MP_QSTR_rotation, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_units, MP_ARG_INT, {.u_int = kDegrees} },
        { MP_QSTR_velocity, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(601)} },
        { MP_QSTR_units_v, MP_ARG_INT, {.u_int = kPercent} },
        { MP_QSTR_wait, MP_ARG_BOOL, {.u_bool = true} },
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    motor_object(args[0])->handle->spin_to_position(
        numeric_value(values[0].u_obj), values[1].u_int,
        static_cast<float>(numeric_value(values[2].u_obj)),
        values[3].u_int, values[4].u_bool);
    return mp_const_none;
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_spin_to_position_obj, 1, vex_motor_spin_to_position);

extern "C" mp_obj_t vex_motor_stop(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {
        { MP_QSTR_brakeType, MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    const int brake_mode = values[0].u_obj == mp_const_none
        ? -1
        : mp_obj_get_int(values[0].u_obj);
    motor_object(args[0])->handle->stop(brake_mode);
    return mp_const_none;
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_stop_obj, 1, vex_motor_stop);

extern "C" mp_obj_t vex_motor_set_position(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {
        { MP_QSTR_position, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_units, MP_ARG_INT, {.u_int = kDegrees} },
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    motor_object(args[0])->handle->set_position(numeric_value(values[0].u_obj), values[1].u_int);
    return mp_const_none;
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_set_position_obj, 1, vex_motor_set_position);

extern "C" mp_obj_t vex_motor_set_velocity(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {
        { MP_QSTR_velocity, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_units, MP_ARG_INT, {.u_int = kPercent} },
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    motor_object(args[0])->handle->set_velocity(
        static_cast<float>(numeric_value(values[0].u_obj)), values[1].u_int);
    return mp_const_none;
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_set_velocity_obj, 1, vex_motor_set_velocity);

extern "C" mp_obj_t vex_motor_set_stopping(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {
        { MP_QSTR_brakeType, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 1} },
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    motor_object(args[0])->handle->set_stopping(values[0].u_int);
    return mp_const_none;
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_set_stopping_obj, 1, vex_motor_set_stopping);

extern "C" mp_obj_t vex_motor_set_max_torque(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {
        { MP_QSTR_value, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_units, MP_ARG_INT, {.u_int = kTorquePercent} },
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    motor_object(args[0])->handle->set_max_torque(
        static_cast<float>(numeric_value(values[0].u_obj)), values[1].u_int);
    return mp_const_none;
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_set_max_torque_obj, 1, vex_motor_set_max_torque);

extern "C" mp_obj_t vex_motor_set_timeout(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {
        { MP_QSTR_time, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_units, MP_ARG_INT, {.u_int = kSeconds} },
    };
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    motor_object(args[0])->handle->set_timeout(
        static_cast<float>(numeric_value(values[0].u_obj)), values[1].u_int);
    return mp_const_none;
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_set_timeout_obj, 1, vex_motor_set_timeout);

extern "C" mp_obj_t vex_motor_set_reversed(mp_obj_t self_in, mp_obj_t value) {
    motor_object(self_in)->handle->set_reversed(mp_obj_is_true(value));
    return mp_const_none;
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_2(vex_motor_set_reversed_obj, vex_motor_set_reversed);

extern "C" mp_obj_t vex_motor_reset_position(mp_obj_t self_in) {
    motor_object(self_in)->handle->reset_position();
    return mp_const_none;
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_1(vex_motor_reset_position_obj, vex_motor_reset_position);

extern "C" mp_obj_t vex_motor_is_done(mp_obj_t self_in) {
    return mp_obj_new_bool(motor_object(self_in)->handle->is_done());
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_1(vex_motor_is_done_obj, vex_motor_is_done);

extern "C" mp_obj_t vex_motor_is_spinning(mp_obj_t self_in) {
    return mp_obj_new_bool(motor_object(self_in)->handle->is_spinning());
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_1(vex_motor_is_spinning_obj, vex_motor_is_spinning);

extern "C" mp_obj_t vex_motor_position(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {{MP_QSTR_units, MP_ARG_INT, {.u_int = kDegrees}}};
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    return mp_obj_new_float(motor_object(args[0])->handle->position(values[0].u_int));
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_position_obj, 1, vex_motor_position);

extern "C" mp_obj_t vex_motor_velocity(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {{MP_QSTR_units, MP_ARG_INT, {.u_int = kPercent}}};
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    return mp_obj_new_float(motor_object(args[0])->handle->velocity(values[0].u_int));
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_velocity_obj, 1, vex_motor_velocity);

extern "C" mp_obj_t vex_motor_current(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {{MP_QSTR_units, MP_ARG_INT, {.u_int = kCurrentAmps}}};
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    return mp_obj_new_float(motor_object(args[0])->handle->current(values[0].u_int));
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_current_obj, 1, vex_motor_current);

extern "C" mp_obj_t vex_motor_power(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {{MP_QSTR_units, MP_ARG_INT, {.u_int = kWatts}}};
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    return mp_obj_new_float(motor_object(args[0])->handle->power(values[0].u_int));
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_power_obj, 1, vex_motor_power);

extern "C" mp_obj_t vex_motor_torque(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {{MP_QSTR_units, MP_ARG_INT, {.u_int = kNm}}};
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    return mp_obj_new_float(motor_object(args[0])->handle->torque(values[0].u_int));
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_torque_obj, 1, vex_motor_torque);

extern "C" mp_obj_t vex_motor_efficiency(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {{MP_QSTR_units, MP_ARG_INT, {.u_int = kEfficiencyPercent}}};
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    return mp_obj_new_float(motor_object(args[0])->handle->efficiency(values[0].u_int));
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_efficiency_obj, 1, vex_motor_efficiency);

extern "C" mp_obj_t vex_motor_temperature(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    static const mp_arg_t allowed[] = {{MP_QSTR_units, MP_ARG_INT, {.u_int = kTemperaturePercent}}};
    mp_arg_val_t values[MP_ARRAY_SIZE(allowed)];
    parse_method_args(n_args, args, kwargs, MP_ARRAY_SIZE(allowed), allowed, values);
    return mp_obj_new_int(motor_object(args[0])->handle->temperature(values[0].u_int));
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(vex_motor_temperature_obj, 1, vex_motor_temperature);

extern "C" mp_obj_t vex_motor_get_timeout(mp_obj_t self_in) {
    return mp_obj_new_float(motor_object(self_in)->handle->get_timeout());
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_1(vex_motor_get_timeout_obj, vex_motor_get_timeout);

extern "C" mp_obj_t vex_motor_count(mp_obj_t self_in) {
    return mp_obj_new_int(motor_object(self_in)->handle->count());
}
extern "C" MP_DEFINE_CONST_FUN_OBJ_1(vex_motor_count_obj, vex_motor_count);
