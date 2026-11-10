#ifndef DRIVETRAIN_HPP
#define DRIVETRAIN_HPP

#include <cstdint>
#include <memory>
#include <vector>

#include "pros/motor_group.hpp"
#include "vex/Inerital.hpp"
#include "vex/motors.hpp"
#include "vex/Sensors/Distance.hpp"

class Drivetrain {
    private:
        std::unique_ptr<pros::MotorGroup> left_motors_;
        std::unique_ptr<pros::MotorGroup> right_motors_;
        float wheel_travel_mm_;
        float track_width_mm_;
        float wheel_base_mm_;
        float external_gear_ratio_;
        float drive_velocity_ = 50.0f;
        VelocityUnits drive_velocity_units_ = VELOCITY_PERCENT;
        float turn_velocity_ = 50.0f;
        VelocityUnits turn_velocity_units_ = VELOCITY_PERCENT;
        BrakeMode stopping_mode_ = BRAKE;
        float timeout_ms_ = -1.0f;
        float heading_offset_ = 0.0f;
        float rotation_offset_ = 0.0f;
        double left_origin_ = 0.0;
        double right_origin_ = 0.0;
        double left_target_ = 0.0;
        double right_target_ = 0.0;
        bool target_active_ = false;
        bool continuous_motion_ = false;

        static float to_millimeters(float value, DistanceUnits units);
        static float to_degrees(float value, AngleUnits units);
        static int max_rpm(const pros::MotorGroup& motors);
        static int velocity_to_rpm(const pros::MotorGroup& motors, float velocity, VelocityUnits units);
        static void command_velocity(pros::MotorGroup& motors, float velocity, VelocityUnits units);
        static void command_relative(pros::MotorGroup& motors, double degrees, float velocity, VelocityUnits units);
        float raw_rotation() const;
        void wait_for_target();
        bool target_reached() const;
        double motor_degrees_for_distance(float distance_mm) const;
        void move_relative(double left_degrees, double right_degrees, float velocity, VelocityUnits units, bool wait);

    public:
        Drivetrain(const std::vector<std::int8_t>& left_ports,
                   const std::vector<std::int8_t>& right_ports,
                   float wheel_travel,
                   float track_width,
                   float wheel_base,
                   DistanceUnits units = MM,
                   float external_gear_ratio = 1.0f);

        void drive(Direction direction, float velocity = -1.0f, VelocityUnits units = VELOCITY_PERCENT);
        void drive_for(Direction direction, float distance, DistanceUnits units = MM,
                       float velocity = -1.0f, VelocityUnits velocity_units = VELOCITY_PERCENT, bool wait = true);
        void turn(TurnType direction, float velocity = -1.0f, VelocityUnits units = VELOCITY_PERCENT);
        void turn_for(TurnType direction, float angle, AngleUnits units = DEGREES,
                      float velocity = -1.0f, VelocityUnits velocity_units = VELOCITY_PERCENT, bool wait = true);
        void turn_to_heading(float heading, AngleUnits units = DEGREES,
                             float velocity = -1.0f, VelocityUnits velocity_units = VELOCITY_PERCENT, bool wait = true);
        void turn_to_rotation(float rotation, AngleUnits units = DEGREES,
                              float velocity = -1.0f, VelocityUnits velocity_units = VELOCITY_PERCENT, bool wait = true);
        void stop(BrakeMode mode = NALL);
        void set_drive_velocity(float velocity, VelocityUnits units);
        void set_turn_velocity(float velocity, VelocityUnits units);
        void set_stopping(BrakeMode mode);
        void set_timeout(float value, TimeUnits units);
        void set_heading(float heading, AngleUnits units = DEGREES);
        void set_rotation(float rotation, AngleUnits units = DEGREES);
        bool is_done();
        bool is_moving() const;
        float heading(AngleUnits units = DEGREES) const;
        float rotation(AngleUnits units = DEGREES) const;
        float velocity(VelocityUnits units) const;
        int get_timeout() const;
};

#endif // DRIVETRAIN_HPP
