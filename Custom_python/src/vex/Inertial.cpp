#include "vex/Inerital.hpp"
#include "vex/safe_value.hpp"

#include <cmath>

Inertial::Inertial(Ports port){
    this -> Sensor = new pros::Imu(static_cast<std::uint8_t>(port));
}

float Inertial::heading(AngleUnits units){
    float degrees = static_cast<float>(vsafe::num(Sensor->get_heading()));
    if (TurnDirection == LEFT){
        degrees = std::fmod(360.0f - degrees, 360.0f);
    }
    return units == TURNS ? degrees / 360.0f : degrees;
}

float Inertial::rotation(AngleUnits units){
    float degrees = static_cast<float>(vsafe::num(Sensor->get_rotation()));
    if (TurnDirection == LEFT){
        degrees = -degrees;
    }
    return units == TURNS ? degrees / 360.0f : degrees;
}

float Inertial::gyro_rate(AxisType axis, VelocityUnits units){
    const pros::imu_gyro_s_t rate = Sensor->get_gyro_rate();
    float degrees_per_second;
    switch (axis){
        case XAXIS: degrees_per_second = vsafe::fnum(rate.x); break;
        case YAXIS: degrees_per_second = vsafe::fnum(rate.y); break;
        case ZAXIS: degrees_per_second = vsafe::fnum(rate.z); break;
        default: return 0.0f;
    }

    switch (units){
        case RPM: return degrees_per_second / 6.0f;
        case VELOCITY_PERCENT: return degrees_per_second / 10.0f;
        default: return degrees_per_second;
    }
}

float Inertial::orientation(AxisType axis, AngleUnits units){
    const pros::euler_s_t orientation = Sensor->get_euler();
    float degrees;
    switch (axis){
        case PITCH: degrees = vsafe::fnum(orientation.pitch); break;
        case ROLL: degrees = vsafe::fnum(orientation.roll); break;
        case YAW: degrees = vsafe::fnum(orientation.yaw); break;
        default: return 0.0f;
    }
    return units == TURNS ? degrees / 360.0f : degrees;
}

void Inertial::set_heading(float value, AngleUnits units){
    float degrees = units == TURNS ? value * 360.0f : value;
    if (TurnDirection == LEFT){
        degrees = std::fmod(360.0f - degrees, 360.0f);
    }
    Sensor->set_heading(degrees);
}

void Inertial::set_rotation(float value, AngleUnits units){
    float degrees = units == TURNS ? value * 360.0f : value;
    if (TurnDirection == LEFT){
        degrees = -degrees;
    }
    Sensor->set_rotation(degrees);
}

void Inertial::calibrate(){
    Sensor->reset(true);
}

void Inertial::reset_heading(){
    Sensor->tare_heading();
}

void Inertial::reset_rotation(){
    Sensor->tare_rotation();
}

bool Inertial::is_calibrating(){
    return Sensor->get_status() == pros::ImuStatus::calibrating;
}

bool Inertial::installed(){
    return Sensor->is_installed();
}

float Inertial::acceleration(AxisType type){
    const pros::imu_accel_s_t value = Sensor->get_accel();
    switch (type){
        case XAXIS: return vsafe::fnum(value.x);
        case YAXIS: return vsafe::fnum(value.y);
        case ZAXIS: return vsafe::fnum(value.z);
        default: return 0.0f;
    }
}

void Inertial::set_turn_type(TurnType turntype){
    TurnDirection = turntype;
}

TurnType Inertial::get_turn_type(){
    return TurnDirection;
}