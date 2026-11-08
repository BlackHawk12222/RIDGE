#ifndef ROTATION_HPP
#define ROTATION_HPP

#include <tuple>
#include <utility>

#include "vex/motors.hpp"

class Rotation{
    private:
        pros::Rotation* Sensor = nullptr;

    public:
        Rotation(Ports smartport, bool reverse);

        void set_position(float value, AngleUnits units);

        float angle(AngleUnits units);

        float position(AngleUnits units);

        float velocity(VelocityUnits units);

        void reset_position();

        void set_reversed(bool value);

        template<typename Func, typename... Args>
        void changed(Func callback, Args... args){
            auto callback_fn = [func = std::forward<Func>(callback),
                                callback_args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply(func, callback_args);
            };
            const int initial_position = Sensor->get_position();

            pros::Task([this, previous_position = initial_position, callback_fn = std::move(callback_fn)]() mutable {
                int previous = previous_position;
                while (true){
                    const int current = Sensor->get_position();
                    if (current != previous){
                        callback_fn();
                    }
                    previous = current;
                    pros::delay(20);
                }
            });
        }
};

#endif // ROTATION_HPP