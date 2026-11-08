#ifndef INERTIAL_HPP
#define INERTIAL_HPP

#include <cmath>
#include <tuple>
#include <utility>

#include "vex/motors.hpp"

enum AxisType{
    XAXIS = 52,
    YAXIS = 53,
    ZAXIS = 54,
    PITCH = 55,
    ROLL = 56,
    YAW = 57
};

enum TurnType{
    RIGHT = 58,
    LEFT = 59
};

class Inertial{
    private:
        pros::Imu* Sensor = nullptr;
        TurnType TurnDirection = RIGHT;

    public:
        Inertial(Ports port);

        float heading(AngleUnits units);

        float rotation(AngleUnits units);

        float gyro_rate(AxisType axis, VelocityUnits units);

        float orientation(AxisType axis, AngleUnits units);

        void set_heading(float value, AngleUnits units);
        
        void set_rotation(float value, AngleUnits units);

        void calibrate();

        void reset_heading();

        void reset_rotation();

        bool is_calibrating(); 

        template<typename Func, typename... Args>
        void changed(Func callback, Args... args){
            auto callback_fn = [func = std::forward<Func>(callback),
                                callback_args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply(func, callback_args);
            };
            const float initial_heading = heading(DEGREES);

            pros::Task([this, previous_heading = initial_heading, callback_fn = std::move(callback_fn)]() mutable {
                float previous = previous_heading;
                while (true){
                    const float current = heading(DEGREES);
                    if (current != previous){
                        callback_fn();
                    }
                    previous = current;
                    pros::delay(20);
                }
            });
        }

        bool installed();

        float acceleration(AxisType type);

        template<typename Func, typename... Args>
        void collision(Func callback, Args... args){
            auto callback_fn = [func = std::forward<Func>(callback),
                                callback_args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply(func, callback_args);
            };
            pros::imu_accel_s_t previous = Sensor->get_accel();

            pros::Task([this, previous, callback_fn = std::move(callback_fn)]() mutable {
                pros::imu_accel_s_t last = previous;
                while (true){
                    const pros::imu_accel_s_t current = Sensor->get_accel();
                    const bool current_valid = std::isfinite(current.x) && std::isfinite(current.y)
                        && std::isfinite(current.z) && std::abs(current.x) <= 4.5
                        && std::abs(current.y) <= 4.5 && std::abs(current.z) <= 4.5;
                    const bool last_valid = std::isfinite(last.x) && std::isfinite(last.y)
                        && std::isfinite(last.z) && std::abs(last.x) <= 4.5
                        && std::abs(last.y) <= 4.5 && std::abs(last.z) <= 4.5;
                    if (current_valid && last_valid){
                        const float dx = static_cast<float>(current.x - last.x);
                        const float dy = static_cast<float>(current.y - last.y);
                        const float dz = static_cast<float>(current.z - last.z);
                        if (std::sqrt(dx * dx + dy * dy + dz * dz) > 1.0f){
                            callback_fn();
                        }
                    }
                    if (current_valid){
                        last = current;
                    }
                    pros::delay(20);
                }
            });
        }

        void set_turn_type(TurnType turntype);

        TurnType get_turn_type();
};

#endif // INERTIAL_HPP