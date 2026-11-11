#include <cmath>
#include <stdexcept>

#include "vex/Drivetrain.hpp"

namespace {
constexpr float PI = 3.14159265358979323846f;
constexpr double POSITION_TOLERANCE_DEGREES = 3.0;

float normalize_heading(float heading) {
    heading = std::fmod(heading, 360.0f);
    return heading < 0.0f ? heading + 360.0f : heading;
}
}

Drivetrain::Drivetrain(const std::vector<std::int8_t>& left_ports,
                       const std::vector<std::int8_t>& right_ports,
                       float wheel_travel,
                       float track_width,
                       float wheel_base,
                       DistanceUnits units,
                       float external_gear_ratio)
    : wheel_travel_mm_(to_millimeters(wheel_travel, units)),
      track_width_mm_(to_millimeters(track_width, units)),
      wheel_base_mm_(to_millimeters(wheel_base, units)),
      external_gear_ratio_(external_gear_ratio) {
    if (left_ports.empty() || right_ports.empty()) {
        throw std::invalid_argument("Drivetrain requires at least one motor on each side");
    }
    if (wheel_travel_mm_ <= 0.0f || track_width_mm_ <= 0.0f || wheel_base_mm_ <= 0.0f ||
        external_gear_ratio_ <= 0.0f) {
        throw std::invalid_argument("Drivetrain dimensions and external gear ratio must be positive");
    }

    left_motors_ = std::make_unique<pros::MotorGroup>(
        left_ports, pros::v5::MotorGears::invalid, pros::v5::MotorUnits::degrees);
    right_motors_ = std::make_unique<pros::MotorGroup>(
        right_ports, pros::v5::MotorGears::invalid, pros::v5::MotorUnits::degrees);

    left_target_ = left_motors_->get_position();
    right_target_ = right_motors_->get_position();
    left_origin_ = left_target_;
    right_origin_ = right_target_;
}

Drivetrain::Drivetrain(Motor* left_motor, Motor* right_motor,
    float wheel_travel, float track_width, float wheel_base,
    DistanceUnits units, float external_gear_ratio)
: Drivetrain(std::vector<std::int8_t>{static_cast<std::int8_t>(left_motor->port())},
std::vector<std::int8_t>{static_cast<std::int8_t>(right_motor->port())},
wheel_travel, track_width, wheel_base, units, external_gear_ratio) {}

Drivetrain::Drivetrain(MotorGroup* left_group, MotorGroup* right_group,
    float wheel_travel, float track_width, float wheel_base,
    DistanceUnits units, float external_gear_ratio)
: Drivetrain(left_group->get_ports(), right_group->get_ports(),
wheel_travel, track_width, wheel_base, units, external_gear_ratio) {}

Drivetrain::Drivetrain(Motor* left_motor, MotorGroup* right_group,
    float wheel_travel, float track_width, float wheel_base,
    DistanceUnits units, float external_gear_ratio)
: Drivetrain(std::vector<std::int8_t>{static_cast<std::int8_t>(left_motor->port())},
right_group->get_ports(),
wheel_travel, track_width, wheel_base, units, external_gear_ratio) {}

Drivetrain::Drivetrain(MotorGroup* left_group, Motor* right_motor,
    float wheel_travel, float track_width, float wheel_base,
    DistanceUnits units, float external_gear_ratio)
: Drivetrain(left_group->get_ports(),
std::vector<std::int8_t>{static_cast<std::int8_t>(right_motor->port())},
wheel_travel, track_width, wheel_base, units, external_gear_ratio) {}


float Drivetrain::to_millimeters(float value, DistanceUnits units) {
    switch (units) {
        case MM:
            return value;
        case INCHES:
            return value * 25.4f;
        case CM:
            return value * 10.0f;
        default:
            throw std::invalid_argument("Unsupported drivetrain distance unit");
    }
}

float Drivetrain::to_degrees(float value, AngleUnits units) {
    switch (units) {
        case DEGREES:
            return value;
        case TURNS:
            return value * 360.0f;
        default:
            throw std::invalid_argument("Unsupported drivetrain angle unit");
    }
}

int Drivetrain::max_rpm(const pros::MotorGroup& motors) {
    switch (motors.get_gearing()) {
        case pros::MotorGear::ratio_6_to_1:
            return 600;
        case pros::MotorGear::ratio_18_to_1:
            return 200;
        case pros::MotorGear::ratio_36_to_1:
            return 100;
        default:
            return 200;
    }
}

int Drivetrain::velocity_to_rpm(const pros::MotorGroup& motors, float velocity, VelocityUnits units) {
    switch (units) {
        case RPM:
            return static_cast<int>(std::lround(velocity));
        case VELOCITY_PERCENT:
            return static_cast<int>(std::lround(max_rpm(motors) * velocity / 100.0f));
        case DPS:
            return static_cast<int>(std::lround(velocity / 6.0f));
        case VELOCITY_VOLT:
            return static_cast<int>(std::lround(max_rpm(motors) * velocity / 12.0f));
        default:
            throw std::invalid_argument("Drivetrain velocity must be specified in RPM, percent, or DPS");
    }
}

void Drivetrain::command_velocity(pros::MotorGroup& motors, float velocity, VelocityUnits units) {
    if (units == VELOCITY_VOLT) {
        motors.move_voltage(static_cast<std::int32_t>(std::lround(velocity * 1000.0f)));
        return;
    }
    motors.move_velocity(velocity_to_rpm(motors, velocity, units));
}

void Drivetrain::command_relative(pros::MotorGroup& motors, double degrees, float velocity, VelocityUnits units) {
    motors.move_relative(degrees, std::abs(velocity_to_rpm(motors, velocity, units)));
}

double Drivetrain::motor_degrees_for_distance(float distance_mm) const {
    return static_cast<double>(distance_mm) * 360.0 * external_gear_ratio_ / wheel_travel_mm_;
}

void Drivetrain::move_relative(double left_degrees, double right_degrees,
                               float velocity, VelocityUnits units, bool wait) {
    left_target_ = left_motors_->get_position() + left_degrees;
    right_target_ = right_motors_->get_position() + right_degrees;
    target_active_ = true;
    continuous_motion_ = false;
    command_relative(*left_motors_, left_degrees, velocity, units);
    command_relative(*right_motors_, right_degrees, velocity, units);
    if (wait) {
        wait_for_target();
    }
}

bool Drivetrain::target_reached() const {
    return std::abs(left_motors_->get_position() - left_target_) <= POSITION_TOLERANCE_DEGREES &&
           std::abs(right_motors_->get_position() - right_target_) <= POSITION_TOLERANCE_DEGREES;
}

void Drivetrain::wait_for_target() {
    const std::uint32_t started = pros::millis();
    while (!target_reached()) {
        if (timeout_ms_ >= 0.0f &&
            static_cast<std::uint32_t>(pros::millis() - started) >= static_cast<std::uint32_t>(timeout_ms_)) {
            stop();
            return;
        }
        pros::delay(5);
    }
    target_active_ = false;
}

void Drivetrain::drive(Direction direction, float velocity, VelocityUnits units) {
    const float speed = velocity < 0.0f ? drive_velocity_ : velocity;
    const VelocityUnits speed_units = velocity < 0.0f ? drive_velocity_units_ : units;
    const float signed_speed = direction == REVERSE ? -speed : speed;
    target_active_ = false;
    continuous_motion_ = true;
    command_velocity(*left_motors_, signed_speed, speed_units);
    command_velocity(*right_motors_, signed_speed, speed_units);
}

void Drivetrain::drive_for(Direction direction, float distance, DistanceUnits units,
                           float velocity, VelocityUnits velocity_units, bool wait) {
    const float speed = velocity < 0.0f ? drive_velocity_ : velocity;
    const VelocityUnits speed_units = velocity < 0.0f ? drive_velocity_units_ : velocity_units;
    const float signed_distance = direction == REVERSE ? -to_millimeters(distance, units)
                                                        : to_millimeters(distance, units);
    const double degrees = motor_degrees_for_distance(signed_distance);
    move_relative(degrees, degrees, speed, speed_units, wait);
}

void Drivetrain::turn(TurnType direction, float velocity, VelocityUnits units) {
    const float speed = velocity < 0.0f ? turn_velocity_ : velocity;
    const VelocityUnits speed_units = velocity < 0.0f ? turn_velocity_units_ : units;
    const float sign = direction == RIGHT ? 1.0f : -1.0f;
    target_active_ = false;
    continuous_motion_ = true;
    command_velocity(*left_motors_, sign * speed, speed_units);
    command_velocity(*right_motors_, -sign * speed, speed_units);
}

void Drivetrain::turn_for(TurnType direction, float angle, AngleUnits units,
                          float velocity, VelocityUnits velocity_units, bool wait) {
    const float speed = velocity < 0.0f ? turn_velocity_ : velocity;
    const VelocityUnits speed_units = velocity < 0.0f ? turn_velocity_units_ : velocity_units;
    const float degrees = to_degrees(angle, units);
    const float sign = direction == RIGHT ? 1.0f : -1.0f;
    const float wheel_distance = sign * PI * track_width_mm_ * degrees / 360.0f;
    const double motor_degrees = motor_degrees_for_distance(wheel_distance);
    move_relative(-motor_degrees, motor_degrees, speed, speed_units, wait);
}

void Drivetrain::turn_to_heading(float heading, AngleUnits units,
                                 float velocity, VelocityUnits velocity_units, bool wait) {
    const float target = normalize_heading(to_degrees(heading, units));
    float delta = target - this->heading();
    if (delta > 180.0f) {
        delta -= 360.0f;
    } else if (delta < -180.0f) {
        delta += 360.0f;
    }
    turn_for(delta >= 0.0f ? RIGHT : LEFT, std::abs(delta), DEGREES, velocity, velocity_units, wait);
}

void Drivetrain::turn_to_rotation(float rotation, AngleUnits units,
                                  float velocity, VelocityUnits velocity_units, bool wait) {
    const float delta = to_degrees(rotation, units) - this->rotation();
    turn_for(delta >= 0.0f ? RIGHT : LEFT, std::abs(delta), DEGREES, velocity, velocity_units, wait);
}

void Drivetrain::stop(BrakeMode mode) {
    const BrakeMode brake_mode = mode == NALL ? stopping_mode_ : mode;
    left_motors_->set_brake_mode_all(static_cast<pros::MotorBrake>(brake_mode));
    right_motors_->set_brake_mode_all(static_cast<pros::MotorBrake>(brake_mode));
    left_motors_->brake();
    right_motors_->brake();
    target_active_ = false;
    continuous_motion_ = false;
}

void Drivetrain::set_drive_velocity(float velocity, VelocityUnits units) {
    drive_velocity_ = velocity;
    drive_velocity_units_ = units;
}

void Drivetrain::set_turn_velocity(float velocity, VelocityUnits units) {
    turn_velocity_ = velocity;
    turn_velocity_units_ = units;
}

void Drivetrain::set_stopping(BrakeMode mode) {
    stopping_mode_ = mode;
    left_motors_->set_brake_mode_all(static_cast<pros::MotorBrake>(mode));
    right_motors_->set_brake_mode_all(static_cast<pros::MotorBrake>(mode));
}

void Drivetrain::set_timeout(float value, TimeUnits units) {
    if (value < 0.0f) {
        timeout_ms_ = -1.0f;
    } else if (units == SECONDS) {
        timeout_ms_ = value * 1000.0f;
    } else if (units == MSEC) {
        timeout_ms_ = value;
    } else {
        throw std::invalid_argument("Unsupported drivetrain timeout unit");
    }
}

void Drivetrain::set_heading(float heading, AngleUnits units) {
    heading_offset_ = to_degrees(heading, units) - raw_rotation();
}

void Drivetrain::set_rotation(float rotation, AngleUnits units) {
    rotation_offset_ = to_degrees(rotation, units) - raw_rotation();
    heading_offset_ = rotation_offset_;
}

bool Drivetrain::is_done() {
    if (target_active_) {
        if (!target_reached()) {
            return false;
        }
        target_active_ = false;
        return true;
    }
    return !continuous_motion_ &&
           std::abs(left_motors_->get_actual_velocity()) < 1.0 &&
           std::abs(right_motors_->get_actual_velocity()) < 1.0;
}

bool Drivetrain::is_moving() const {
    if (target_active_) {
        return !target_reached();
    }
    return continuous_motion_ ||
           std::abs(left_motors_->get_actual_velocity()) >= 1.0 ||
           std::abs(right_motors_->get_actual_velocity()) >= 1.0;
}

float Drivetrain::rotation(AngleUnits units) const {
    const float degrees = raw_rotation() + rotation_offset_;
    if (units == DEGREES) {
        return degrees;
    }
    if (units == TURNS) {
        return degrees / 360.0f;
    }
    throw std::invalid_argument("Unsupported drivetrain angle unit");
}

float Drivetrain::raw_rotation() const {
    const double left_delta = left_motors_->get_position() - left_origin_;
    const double right_delta = right_motors_->get_position() - right_origin_;
    return static_cast<float>(
        (right_delta - left_delta) * wheel_travel_mm_ /
        (2.0 * PI * track_width_mm_ * external_gear_ratio_));
}

float Drivetrain::heading(AngleUnits units) const {
    const float degrees = normalize_heading(raw_rotation() + heading_offset_);
    if (units == DEGREES) {
        return degrees;
    }
    if (units == TURNS) {
        return degrees / 360.0f;
    }
    throw std::invalid_argument("Unsupported drivetrain angle unit");
}

float Drivetrain::velocity(VelocityUnits units) const {
    const float average_rpm = static_cast<float>(
        (left_motors_->get_actual_velocity() + right_motors_->get_actual_velocity()) / 2.0);
    if (units == RPM) {
        return average_rpm;
    }
    if (units == DPS) {
        return average_rpm * 6.0f;
    }
    if (units == VELOCITY_PERCENT) {
        return average_rpm * 100.0f /
               ((max_rpm(*left_motors_) + max_rpm(*right_motors_)) / 2.0f);
    }
    throw std::invalid_argument("Unsupported drivetrain velocity unit");
}

int Drivetrain::get_timeout() const {
    return timeout_ms_ < 0.0f ? -1 : static_cast<int>(timeout_ms_);
}
