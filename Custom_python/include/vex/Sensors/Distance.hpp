#ifndef DISTANCE_HPP
#define DISTANCE_HPP

#include <tuple>
#include <utility>

#include "vex/motors.hpp"

enum class ObjectSize : int {
    NONE = 0,
    SMALL = 1,
    MEDIUM = 2,
    LARGE = 3
};

enum DistanceUnits {
    MM = 40,
    INCHES = 41,
    CM = 42
};

class Distance{
    private:
        pros::Distance* Sensor = nullptr;

    public:
        explicit Distance(Ports port);

        float object_distance(DistanceUnits units);

        float object_velocity();

        ObjectSize object_size();

        bool is_object_detected();

        template<typename Func, typename... Args>
        void changed(Func callback, Args... args){
            auto callback_fn = [func = std::forward<Func>(callback),
                                callback_args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply(func, callback_args);
            };
            const int initial_distance = Sensor->get_distance();

            pros::Task([this, previous_distance = initial_distance, callback_fn = std::move(callback_fn)]() mutable {
                int previous = previous_distance;
                while (true){
                    const int current = Sensor->get_distance();
                    if (current != previous){
                        callback_fn();
                    }
                    previous = current;
                    pros::delay(20);
                }
            });
        }
};

#endif // DISTANCE_HPP