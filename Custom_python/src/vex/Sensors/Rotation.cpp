#include "vex/Sensors/Rotation.hpp"
#include "vex/safe_value.hpp"

#include <cmath>

Rotation::Rotation(Ports smartport, bool reverse){
    const int port = static_cast<int>(smartport);
    Sensor = new pros::Rotation(static_cast<std::int8_t>(reverse ? -port : port));
}

void Rotation::set_position(float value, AngleUnits units){
    const float centidegrees = units == TURNS ? value * 36000.0f : value * 100.0f;
    Sensor->set_position(static_cast<std::int32_t>(std::lround(centidegrees)));
}

float Rotation::angle(AngleUnits units){
    const float degrees = static_cast<float>(vsafe::num(Sensor->get_angle())) / 100.0f;
    return units == TURNS ? degrees / 360.0f : degrees;
}

float Rotation::position(AngleUnits units){
    const float degrees = static_cast<float>(vsafe::num(Sensor->get_position())) / 100.0f;
    return units == TURNS ? degrees / 360.0f : degrees;
}

float Rotation::velocity(VelocityUnits units){
    const float centidegrees_per_second = static_cast<float>(vsafe::num(Sensor->get_velocity()));
    if (units == RPM){
        return centidegrees_per_second / 600.0f;
    }
    return centidegrees_per_second / 100.0f;
}

void Rotation::reset_position(){
    Sensor->reset_position();
}

void Rotation::set_reversed(bool value){
    Sensor->set_reversed(value);
}

bool Rotation::installed(){
    return Sensor->is_installed(); // Match whatever your pros::Rotation* member is named
}