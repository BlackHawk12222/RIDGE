#ifndef TIMER_HPP
#define TIMER_HPP

#include <tuple>
#include "main.h"

enum TimeUnits{
    SECONDS=27,
    MSEC=28
};

class Timer{
    private:
        unsigned int TimeSub=pros::millis();
        TimeUnits Units=MSEC;

    public:
        void clear();

        unsigned int time(TimeUnits units=MSEC);

        template <typename Func, typename... Args>
        void event(Func callback, int delay=0, std::tuple<Args...> args={});
};

#endif //TIMER_HPP