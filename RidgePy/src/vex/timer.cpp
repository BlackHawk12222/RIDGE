#include "vex/timer.hpp"
#include "main.h"

void Timer::clear(){
    TimeSub=pros::millis();
}

double Timer::time(TimeUnits units){
    const unsigned int elapsed = pros::millis() - TimeSub;
    switch (units){
        case MSEC:
            return elapsed;
        case SECONDS:
            return elapsed / 1000.0;
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