#ifndef ROTATION_HPP
#define ROTATION_HPP

#include "vex/motors.hpp"

class Rotation{
    public:
        Rotation(Ports smartport, bool reverse);

        void set_position(float value, AngleUnits units);

        float angle(AngleUnits units);

        float position(AngleUnits units);

        float velosity(VelocityUnits units);

        void reset_position();

        void set_reversed(bool value);

        template<typename Func, typename... Args>
        void changed(Func callback, Args... args);
};

#endif // ROTATION_HPP