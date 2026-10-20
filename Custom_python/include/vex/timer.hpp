#include <tuple>
#include "main.h"

extern "C"{
    #include "py/runtime.h"
    #include "py/obj.h"
}

enum TimeUnits{
    SECONDS=1,
    MSEC=2
};

class Timer{
    void clear();

    int time(TimeUnits units);

    template <typename Func>
    void event(Func callback, int delay=0, std::tuple<> args={});
};