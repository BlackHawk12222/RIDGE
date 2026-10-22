#include "vex/timer.hpp"
#include "main.h"

void Timer::clear(){
    TimeSub=pros::millis();
}

int Timer::time(TimeUnits units){
    unsigned int output = 0;
    switch (units){
        case MSEC:
        {
            output = pros::millis();
        }
        case SECONDS:
        {
            output = pros::millis() / 1000;
        }
    }
}

template <typename Func, typename... Args>
void Timer::event(Func callback, int delay=0, std::tuple<Args...> args={}){
    pros::Task([=]() {

        if (delay_ms > 0) {
            pros::delay(delay_ms);
        }
        
        std::apply(callback, args);
    });
}