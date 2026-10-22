#include "vex/timer.hpp"
#include "main.h"

void Timer::clear(){
    TimeSub=pros::millis();
}

unsigned int Timer::time(TimeUnits units){
    unsigned int output = 0;
    switch (units){
        case MSEC:
        {
            output = pros::millis();
            break;
        }
        case SECONDS:
        {
            output = pros::millis() / 1000;
            break;
        }
        default:
        {
            break;
        }
    }
    return output;
}

template <typename Func, typename... Args>
void Timer::event(Func callback, int delay, std::tuple<Args...> args){
    pros::Task([=]() {

        if (delay > 0) {
            pros::delay(delay);
        }
        
        std::apply(callback, args);
    });
}