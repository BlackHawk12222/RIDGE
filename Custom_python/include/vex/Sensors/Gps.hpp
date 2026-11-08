#ifndef VEX_GPS_HPP
#define VEX_GPS_HPP

#include <tuple>
#include <utility>

#include "pros/gps.hpp"
#include "pros/rtos.hpp"
#include "vex/Inerital.hpp"
#include "vex/Sensors/Distance.hpp"
#include "vex/motors.hpp"

class Gps {
    private:
        pros::Gps Sensor;

        static double to_meters(double value, DistanceUnits units);
        static double from_meters(double value, DistanceUnits units);

    public:
        explicit Gps(int port);

        void set_origin(double x, double y, DistanceUnits units);
        void set_location(double x, double y, DistanceUnits distance_units,
                          double heading, AngleUnits heading_units);
        double x_position(DistanceUnits units);
        double y_position(DistanceUnits units);
        double heading();
        double acceleration(AxisType axis);
        double gyro_rate(AxisType axis, VelocityUnits units);
        double orientation(AxisType axis, AngleUnits units);

        template<typename Func, typename... Args>
        void changed(Func callback, Args... args) {
            auto callback_fn = [func = std::forward<Func>(callback),
                                callback_args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply(func, callback_args);
            };
            const double initial_heading = Sensor.get_heading();

            pros::Task([this, previous_heading = initial_heading,
                        callback_fn = std::move(callback_fn)]() mutable {
                double previous = previous_heading;
                while (true) {
                    const double current = Sensor.get_heading();
                    if (current != previous) {
                        callback_fn();
                    }
                    previous = current;
                    pros::delay(20);
                }
            });
        }
};

#endif
