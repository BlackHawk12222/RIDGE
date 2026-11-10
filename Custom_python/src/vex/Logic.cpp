#include <cmath>
#include <cstdint>
#include <limits>

#include "main.h"
#include "vex/Logic.hpp"
#include "vex/vex.h"

bool VexLogic::wait(double duration, int units) {
    if (!std::isfinite(duration) || duration < 0) {
        return false;
    }

    double milliseconds;
    switch (units) {
        case VEX_TIME_UNITS_SECONDS:
            milliseconds = duration * 1000.0;
            break;
        case VEX_TIME_UNITS_MSEC:
            milliseconds = duration;
            break;
        default:
            return false;
    }

    if (!std::isfinite(milliseconds)
        || milliseconds >= static_cast<double>(std::numeric_limits<std::uint32_t>::max()) + 1.0) {
        return false;
    }

    pros::delay(static_cast<std::uint32_t>(milliseconds));
    return true;
}

extern "C" bool vex_logic_wait(double duration, int units) {
    return VexLogic::wait(duration, units);
}
