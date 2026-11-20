#include "vex/Sensors/Gps.hpp"
#include "vex/safe_value.hpp"

#include <stdexcept>

Gps::Gps(int port) : Sensor(static_cast<std::uint8_t>(port)) {}

double Gps::to_meters(double value, DistanceUnits units) {
    if (units == MM) {
        return value / 1000.0;
    }
    return value * 0.0254;
}

double Gps::from_meters(double value, DistanceUnits units) {
    if (units == MM) {
        return value * 1000.0;
    }
    return value / 0.0254;
}

void Gps::set_origin(double x, double y, DistanceUnits units) {
    Sensor.set_offset(to_meters(x, units), to_meters(y, units));
}

void Gps::set_location(double x, double y, DistanceUnits distance_units,
                       double heading_value, AngleUnits heading_units) {
    const double heading_degrees = heading_units == TURNS ? heading_value * 360.0 : heading_value;
    Sensor.set_position(to_meters(x, distance_units), to_meters(y, distance_units), heading_degrees);
}

double Gps::x_position(DistanceUnits units) {
    return from_meters(vsafe::num(Sensor.get_position_x()), units);
}

double Gps::y_position(DistanceUnits units) {
    return from_meters(vsafe::num(Sensor.get_position_y()), units);
}

double Gps::heading() {
    return vsafe::num(Sensor.get_heading());
}

double Gps::acceleration(AxisType axis) {
    const pros::gps_accel_s_t acceleration_value = Sensor.get_accel();
    switch (axis) {
        case XAXIS: return vsafe::num(acceleration_value.x);
        case YAXIS: return vsafe::num(acceleration_value.y);
        case ZAXIS: return vsafe::num(acceleration_value.z);
        default: throw std::invalid_argument("invalid GPS acceleration axis");
    }
}

double Gps::gyro_rate(AxisType axis, VelocityUnits units) {
    const pros::gps_gyro_s_t rate = Sensor.get_gyro_rate();
    double degrees_per_second;
    switch (axis) {
        case XAXIS: degrees_per_second = vsafe::num(rate.x); break;
        case YAXIS: degrees_per_second = vsafe::num(rate.y); break;
        case ZAXIS: degrees_per_second = vsafe::num(rate.z); break;
        default: throw std::invalid_argument("invalid GPS gyroscope axis");
    }
    return units == RPM ? degrees_per_second / 6.0 : degrees_per_second;
}

double Gps::orientation(AxisType axis, AngleUnits units) {
    const pros::gps_orientation_s_t orientation_value = Sensor.get_orientation();
    double degrees;
    switch (axis) {
        case PITCH: degrees = vsafe::num(orientation_value.pitch); break;
        case ROLL: degrees = vsafe::num(orientation_value.roll); break;
        case YAW: degrees = vsafe::num(orientation_value.yaw); break;
        default: throw std::invalid_argument("invalid GPS orientation axis");
    }
    return units == TURNS ? degrees / 360.0 : degrees;
}
