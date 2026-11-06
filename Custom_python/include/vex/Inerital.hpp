#ifndef INERTIAL_HPP
#define INERTIAL_HPP

#include "vex/motors.hpp"

enum AxisType{
    XAXIS = 52,
    YAXIS = 53,
    ZAXIS = 54,
    PITCH = 55,
    ROLL = 56,
    YAW = 57
};

enum TurnType{
    RIGHT = 58,
    LEFT = 59
};

class Inertial{
    public:
        Inertial(Ports port);

        float heading(AngleUnits units);

        float rotation(AngleUnits units);

        float gyro_rate(AxisType axis, VelocityUnits units);

        float orientation(AxisType axis, AngleUnits units);

        void set_heading(float value, AngleUnits units);
        
        void set_rotation(float value, AngleUnits units);

        void calibrate();

        void reset_heading();

        void reset_rotation();

        bool is_calibrating(); 

        template<typename Func, typename... Args>
        void changed(Func callback, Args... args);

        bool installed();

        float acceleration(AxisType type);

        template<typename Func, typename... Args>
        void collision(Func callback, Args... args);

        void set_turn_type(TurnType turntype);
};

#endif // INERTIAL_HPP