#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "main.h"

class VexMotorHandle {
public:
    VexMotorHandle(int port, int gear_setting, bool reversed);
    explicit VexMotorHandle(const std::vector<VexMotorHandle*>& motors);
    ~VexMotorHandle();

    VexMotorHandle(const VexMotorHandle&) = delete;
    VexMotorHandle& operator=(const VexMotorHandle&) = delete;

    void spin(int direction, float velocity, int units);
    void spin_for(int direction, float rotation, int angle_units, float velocity, int velocity_units, bool wait);
    void spin_to_position(float rotation, int angle_units, float velocity, int velocity_units, bool wait);
    void stop(int brake_mode);
    void set_position(float position, int angle_units);
    void set_velocity(float velocity, int units);
    void set_stopping(int brake_mode);
    void set_max_torque(float value, int units);
    void set_timeout(float value, int units);
    void set_reversed(bool reversed);
    void reset_position();

    bool is_done() const;
    bool is_spinning() const;
    float position(int units) const;
    float velocity(int units) const;
    float current(int units) const;
    float power(int units) const;
    float torque(int units) const;
    float efficiency(int units) const;
    int temperature(int units) const;
    float get_timeout() const;
    int count() const;

private:
    pros::AbstractMotor* device();
    const pros::AbstractMotor* device() const;
    void wait_for_position(float target_position);
    float to_degrees(float rotation, int units) const;
    float to_rpm(float value, int units) const;
    float max_rpm() const;

    std::unique_ptr<pros::Motor> motor_;
    std::unique_ptr<pros::MotorGroup> group_;
    float default_velocity_ = 50.0f;
    int default_velocity_units_ = 1;
    pros::MotorBrake default_brake_mode_ = pros::MotorBrake::brake;
    float timeout_seconds_ = -1.0f;
    int motor_count_ = 1;
};