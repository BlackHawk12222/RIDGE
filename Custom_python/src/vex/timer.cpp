#include "vex/timer.hpp"
#include "main.h"

void Timer::clear(){
    TimeSub=pros::millis();
}

unsigned int Timer::time(TimeUnits units){
    unsigned int elapsed = pros::millis() - TimeSub;
    switch (units){
        case MSEC:
            return elapsed;
        case SECONDS:
            return elapsed / 1000;
        default:
            return elapsed;
    }
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