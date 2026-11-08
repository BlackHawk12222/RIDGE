#ifndef OPTICAL_HPP
#define OPTICAL_HPP

#include <tuple>
#include <utility>

#include "vex/motors.hpp"

enum class OpticalColor : int {
    BLACK = 0x000000,
    WHITE = 0xFFFFFF,
    RED = 0xFF0000,
    GREEN = 0x00FF00,
    BLUE = 0x0000FF,
    YELLOW = 0xFFFF00,
    ORANGE = 0xFFA500,
    PURPLE = 0x800080,
    CYAN = 0x00FFFF
};

class Optical{
    private:
        pros::Optical* Sensor = nullptr;

    public:
        explicit Optical(Ports port);

        void set_light(bool state);

        void set_light_power(int percent);

        bool is_near_object();

        OpticalColor color();

        float brightness();

        float hue();

        template<typename Func, typename... Args>
        void object_detected(Func callback, Args... args){
            auto callback_fn = [func = std::forward<Func>(callback),
                                callback_args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply(func, callback_args);
            };
            const bool initial_state = is_near_object();

            pros::Task([this, previous_state = initial_state, callback_fn = std::move(callback_fn)]() mutable {
                bool previous = previous_state;
                while (true){
                    const bool current = is_near_object();
                    if (current && !previous){
                        callback_fn();
                    }
                    previous = current;
                    pros::delay(20);
                }
            });
        }

        template<typename Func, typename... Args>
        void object_lost(Func callback, Args... args){
            auto callback_fn = [func = std::forward<Func>(callback),
                                callback_args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply(func, callback_args);
            };
            const bool initial_state = is_near_object();

            pros::Task([this, previous_state = initial_state, callback_fn = std::move(callback_fn)]() mutable {
                bool previous = previous_state;
                while (true){
                    const bool current = is_near_object();
                    if (!current && previous){
                        callback_fn();
                    }
                    previous = current;
                    pros::delay(20);
                }
            });
        }
};

#endif // OPTICAL_HPP