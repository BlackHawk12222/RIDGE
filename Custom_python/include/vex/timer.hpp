#ifndef TIMER_HPP
#define TIMER_HPP

#include <tuple>
#include "main.h"

enum TimeUnits{
    SECONDS=1,
    MSEC=2
};

class Timer{
    private:
        unsigned int TimeSub=pros::millis();
        TimeUnits Units=MSEC;

    public:
        void clear();

        unsigned int time(TimeUnits units);

        template <typename Func, typename... Args>
        void event(Func callback, int delay=0, std::tuple<Args...> args={});
};

#endif //TIMER_HPP