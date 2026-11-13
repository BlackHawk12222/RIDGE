#include <algorithm>
#include <stdexcept>
#include "vex/safe_value.hpp"
#include <initializer_list>
#include <cmath>
#include "main.h"
#include "vex/motors.hpp"

extern "C"{
    #include "py/runtime.h"
    #include "py/obj.h"
}

// Max RPM of a V5 motor cartridge; 200 (green) when unknown.
static double motor_max_rpm(pros::MotorGears gearing){
    switch (gearing){
        case pros::MotorGears::ratio_6_to_1: return 600.0;
        case pros::MotorGears::ratio_18_to_1: return 200.0;
        case pros::MotorGears::ratio_36_to_1: return 100.0;
        default: return 200.0;
    }
}

// VEX GearSetting values (8, 18, 36) are not PROS gearset values (2, 1, 0).
static pros::v5::MotorGears to_pros_gears(GearSetting gears){
    switch (gears){
        case RATIO_6_1: return pros::v5::MotorGears::ratio_6_to_1;
        case RATIO_36_1: return pros::v5::MotorGears::ratio_36_to_1;
        case RATIO_18_1:
        default: return pros::v5::MotorGears::ratio_18_to_1;
    }
}

// VEX velocity to PROS RPM. False for units that are not a speed.
static bool velocity_to_rpm(double velocity, VelocityUnits units, double max_rpm, double& rpm){
    switch (units){
        case RPM: rpm = velocity; return true;
        case VELOCITY_PERCENT: rpm = velocity * max_rpm / 100.0; return true;
        case DPS: rpm = velocity / 6.0; return true;
        case VELOCITY_VOLT: rpm = velocity * max_rpm / 12.0; return true;
        default: return false;
    }
}

Motor::Motor(Ports port, GearSetting gears, bool reversed){
    PortNum = port;
    Ratio = gears;
    IsReversed = reversed;

    std::int8_t active_port = IsReversed ? -static_cast<std::int8_t>(PortNum) : static_cast<std::int8_t>(PortNum);

    this->motor = new pros::Motor(active_port, to_pros_gears(Ratio));
    this->BASE = this->motor;
}


// Called after every timed command. A later command bumps CommandGen, so an
// old timeout never stops a newer movement.
void Motor::timeout(){
    const std::uint32_t gen = ++CommandGen;
    if (TimeoutTime < 0.0f) {
        return;
    }
    const int ms = get_timeout();
    pros::Task([this, gen, ms]() {
        pros::delay(ms);
        if (CommandGen == gen) {
            motor->brake();
        }
    }, "timeout");
}


void Motor::spin(Direction direction, float velocity, VelocityUnits units){
    // 601 means "no velocity given": use set_velocity()'s value and units.
    if (velocity == 601){
        velocity = Velocity;
        units = DefaultVelocityUnits;
    }
    const double sign = direction == REVERSE ? -1.0 : 1.0;
    ++CommandGen;

    if (units == VELOCITY_VOLT){
        motor->move_voltage(static_cast<std::int32_t>(std::lround(vsafe::num(sign * velocity * 1000.0))));
        return;
    }
    double rpm = 0.0;
    if (!velocity_to_rpm(velocity, units, motor_max_rpm(motor->get_gearing()), rpm)){
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
        return;
    }
    motor->move_velocity(static_cast<std::int32_t>(std::lround(vsafe::num(sign * rpm))));
}

void Motor::spin_for(Direction direction, double angle, AngleUnits units, float velocity, VelocityUnits units_v, bool wait){
    // 601 means "no velocity given": use set_velocity()'s value and units.
    if (velocity == 601){
        velocity = Velocity;
        units_v = DefaultVelocityUnits;
    }

    double degrees = vsafe::num(angle);
    switch (units) {
        case DEGREES: break;
        case TURNS: degrees *= 360.0; break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            return;
    }

    double rpm = 0.0;
    if (!velocity_to_rpm(velocity, units_v, motor_max_rpm(motor->get_gearing()), rpm)){
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
        return;
    }
    rpm = vsafe::num(std::abs(rpm));

    // PROS move_relative takes direction from the sign of the distance, not the speed.
    if (direction == REVERSE){
        degrees = -degrees;
    }

    const double target = vsafe::num(motor->get_position()) + degrees;
    motor->move_relative(degrees, static_cast<std::int32_t>(std::lround(rpm)));
    timeout();

    if (wait && degrees != 0.0 && rpm > 0.0){
        wait_for_position(target);
    }
}

void Motor::spin_to_position(double rotation, AngleUnits units, float velocity, VelocityUnits units_v, bool wait){
    // 601 means "no velocity given": use set_velocity()'s value and units.
    if (velocity == 601){
        velocity = Velocity;
        units_v = DefaultVelocityUnits;
    }

    double degrees = vsafe::num(rotation);
    switch (units) {
        case DEGREES: break;
        case TURNS: degrees *= 360.0; break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            return;
    }

    double rpm = 0.0;
    if (!velocity_to_rpm(velocity, units_v, motor_max_rpm(motor->get_gearing()), rpm)){
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
        return;
    }
    rpm = vsafe::num(std::abs(rpm));

    motor->move_absolute(degrees, static_cast<std::int32_t>(std::lround(rpm)));
    timeout();

    if (wait && rpm > 0.0){
        wait_for_position(degrees);
    }
}

// Blocks until the motor is within 3 degrees of target, or has stopped
// moving after starting (stalled, or stopped by its timeout).
void Motor::wait_for_position(double target){
    int elapsed_ms = 0;
    while (std::abs(vsafe::num(motor->get_position()) - target) > 3.0){
        if (elapsed_ms > 150 && std::abs(vsafe::num(motor->get_actual_velocity())) < 1.0){
            break;
        }
        pros::delay(10);
        elapsed_ms += 10;
    }
}

void Motor::stop(BrakeMode mode){
    ++CommandGen;
    const BrakeMode brake_mode = mode != NALL ? mode : DefaultBrakeMode;
    motor->set_brake_mode(static_cast<pros::v5::MotorBrake>(brake_mode));
    motor->brake();
}

void Motor::set_position(float position, AngleUnits units){
    if (units == DEGREES){
        motor->set_zero_position(position);
    }else if (units == TURNS){
        motor->set_zero_position(position * 360.0);
    }else{
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
    }
}

void Motor::set_velocity(float velocity, VelocityUnits units){
    Velocity=velocity;
    DefaultVelocityUnits=units;
}

void Motor::set_stopping(BrakeMode mode){
    DefaultBrakeMode=mode;
}

void Motor::set_max_torque(float value, TorqueUnits units){
    double max_nm = 1.05;
    double max_inlb = 9.29;
    switch (motor->get_gearing()){
        case pros::MotorGear::ratio_6_to_1: max_nm = 0.35; max_inlb = 3.1; break;
        case pros::MotorGear::ratio_36_to_1: max_nm = 2.1; max_inlb = 18.59; break;
        default: break;
    }

    // PROS takes a current limit in mA; 2500 mA is the V5 maximum.
    double limit_ma = 0.0;
    switch (units){
        case TORQUE_AMPS: limit_ma = value * 1000.0; break;
        case NM: limit_ma = vsafe::div(value, max_nm) * 2500.0; break;
        case INLB: limit_ma = vsafe::div(value, max_inlb) * 2500.0; break;
        case TORQUE_PERCENT: limit_ma = value * 25.0; break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            return;
    }
    limit_ma = std::clamp(vsafe::num(limit_ma), 0.0, 2500.0);
    motor->set_current_limit(static_cast<std::int32_t>(std::lround(limit_ma)));
}

void Motor::set_timeout(float value, TimeUnits units){
    TimeoutTime=value;
    TimeoutType=units;
}

void Motor::set_reversed(bool value){
    motor->set_reversed(value);
}

void Motor::reset_position(){
    motor->tare_position();
}

bool Motor::is_done(){
    return std::abs(vsafe::num(motor->get_actual_velocity())) < 1.0f;
}

bool Motor::is_spinning(){
    return std::abs(vsafe::num(motor->get_actual_velocity())) >= 1.0f;
}

float Motor::position(AngleUnits units){
    float output=0.0f;
    switch (units) {
        case DEGREES:
        {
            output= (float)(vsafe::num(motor->get_position()));
            break;
        }
        case TURNS:
        {
            output = (float)(vsafe::num(motor->get_position())) / 360;
        }
    }

    return output;
}

float Motor::velocity(VelocityUnits units){
    float output=0.0f;
    switch (units){
        case VELOCITY_PERCENT:
        {
            int Max_RPM=1;
            switch (motor->get_gearing())
            {
            case pros::MotorGear::ratio_6_to_1:
            {
                Max_RPM=6;  // Includes / 100 for percentage.
                break;
            }   
            case pros::MotorGear::ratio_18_to_1:
            {
                Max_RPM=2;  // Includes / 100 for percentage.
                break;
            }
            case pros::MotorGear::ratio_36_to_1:
            {
                Max_RPM=1;  // Includes / 100 for percentage. 
                break;
            }
            default:
                break;
            }   

            output= (float)(vsafe::num(motor->get_actual_velocity())) / Max_RPM;
            break;
        }
        case RPM:
        {
            output= (float)(vsafe::num(motor->get_actual_velocity()));
            break;
        }
        case DPS:
        {
            output= (float)(vsafe::num(motor->get_actual_velocity())) * 6;  // Includes  * 360 to make DPM then / 60 to make DPS
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::velocity");
            break;
        }
    }

    return output;
}

float Motor::current(CurrentUnits units){
    // PROS reports mA.
    return static_cast<float>(vsafe::num(motor->get_current_draw()) / 1000.0);
}

float Motor::power(PowerUnits units){
    return (float)(vsafe::num(motor->get_power()));
}

float Motor::torque(TorqueUnits units){
    float output= 0.0f;
    switch(units){
        case NM:
        {
            output= (float)(vsafe::num(motor->get_torque()));
            break;
        }
        case INLB:
        {
            float NMtoINLB = 8.8507f;
            output= (float)(vsafe::num(motor->get_torque())) * NMtoINLB;
            break;
        }
    }
    return output;
}

float Motor::efficiency(EfficiencyUnits units){
    return vsafe::num(motor->get_efficiency());
}

int Motor::temperature(TemperatureUnits units){
    const double celsius = vsafe::num(motor->get_temperature());
    switch(units){
        case CELSIUS: return static_cast<int>(std::lround(celsius));
        case FAHRENHEIT: return static_cast<int>(std::lround(celsius * 1.8 + 32.0));
        case TEMPERATURE_PERCENT: return static_cast<int>(std::lround(std::clamp((celsius - 20.0) * 2.0, 0.0, 100.0)));  // VEX: 0% = 20 C, 100% = 70 C
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            return 0;
    }
}


int Motor::get_timeout(){
    int output = 0;
    switch (TimeoutType)
    {
    case SECONDS:
    {
        output=(int)(TimeoutTime * 1000);
        break;
    }
    case MSEC:
    {
        output=TimeoutTime;
        break;
    }
    default:
    mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::get_timeout");
        break;
    }
    return output; 
}

bool Motor::installed(){
    return motor->is_installed();
}

MotorGroup::MotorGroup(std::initializer_list<Motor> motors){
    this -> motor = new pros::MotorGroup(extract_ports(motors)); 
    
    if (motors.size() > 0) {
        // Get the first motor in the list to use as a configuration template
        const pros::Motor& TemplateMotor = *motors.begin()->BASE;

        // Set the configuration of the group using the template motor's data
        motor->set_gearing(TemplateMotor.get_gearing());
        motor->set_encoder_units(TemplateMotor.get_encoder_units());
    }
    
    this -> Count=motors.size();
}

MotorGroup::MotorGroup(Motor** motor_array, int count){
    this -> motor = new pros::MotorGroup(extract_ports(motor_array, count));
    
    if (count > 0) {
        // Get the first motor in the list to use as a configuration template
        const pros::Motor& TemplateMotor = *motor_array[0]->BASE;

        // Set the configuration of the group using the template motor's data
        motor -> set_gearing(TemplateMotor.get_gearing());
        motor -> set_encoder_units(TemplateMotor.get_encoder_units());
    }
    
    this -> Count=count;
}
    
// Called after every timed command. A later command bumps CommandGen, so an
// old timeout never stops a newer movement.
void MotorGroup::timeout(){
    const std::uint32_t gen = ++CommandGen;
    if (TimeoutTime < 0.0f) {
        return;
    }
    const int ms = get_timeout();
    pros::Task([this, gen, ms]() {
        pros::delay(ms);
        if (CommandGen == gen) {
            motor->brake();
        }
    }, "timeout");
}


void MotorGroup::spin(Direction direction, float velocity, VelocityUnits units){
    // 601 means "no velocity given": use set_velocity()'s value and units.
    if (velocity == 601){
        velocity = Velocity;
        units = DefaultVelocityUnits;
    }
    const double sign = direction == REVERSE ? -1.0 : 1.0;
    ++CommandGen;

    if (units == VELOCITY_VOLT){
        motor->move_voltage(static_cast<std::int32_t>(std::lround(vsafe::num(sign * velocity * 1000.0))));
        return;
    }
    double rpm = 0.0;
    if (!velocity_to_rpm(velocity, units, motor_max_rpm(motor->get_gearing()), rpm)){
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
        return;
    }
    motor->move_velocity(static_cast<std::int32_t>(std::lround(vsafe::num(sign * rpm))));
}

void MotorGroup::spin_for(Direction direction, double angle, AngleUnits units, float velocity, VelocityUnits units_v, bool wait){
    // 601 means "no velocity given": use set_velocity()'s value and units.
    if (velocity == 601){
        velocity = Velocity;
        units_v = DefaultVelocityUnits;
    }

    double degrees = vsafe::num(angle);
    switch (units) {
        case DEGREES: break;
        case TURNS: degrees *= 360.0; break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            return;
    }

    double rpm = 0.0;
    if (!velocity_to_rpm(velocity, units_v, motor_max_rpm(motor->get_gearing()), rpm)){
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
        return;
    }
    rpm = vsafe::num(std::abs(rpm));

    // PROS move_relative takes direction from the sign of the distance, not the speed.
    if (direction == REVERSE){
        degrees = -degrees;
    }

    const double target = vsafe::num(motor->get_position()) + degrees;
    motor->move_relative(degrees, static_cast<std::int32_t>(std::lround(rpm)));
    timeout();

    if (wait && degrees != 0.0 && rpm > 0.0){
        wait_for_position(target);
    }
}

void MotorGroup::spin_to_position(double rotation, AngleUnits units, float velocity, VelocityUnits units_v, bool wait){
    // 601 means "no velocity given": use set_velocity()'s value and units.
    if (velocity == 601){
        velocity = Velocity;
        units_v = DefaultVelocityUnits;
    }

    double degrees = vsafe::num(rotation);
    switch (units) {
        case DEGREES: break;
        case TURNS: degrees *= 360.0; break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            return;
    }

    double rpm = 0.0;
    if (!velocity_to_rpm(velocity, units_v, motor_max_rpm(motor->get_gearing()), rpm)){
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
        return;
    }
    rpm = vsafe::num(std::abs(rpm));

    motor->move_absolute(degrees, static_cast<std::int32_t>(std::lround(rpm)));
    timeout();

    if (wait && rpm > 0.0){
        wait_for_position(degrees);
    }
}

// Blocks until the motor is within 3 degrees of target, or has stopped
// moving after starting (stalled, or stopped by its timeout).
void MotorGroup::wait_for_position(double target){
    int elapsed_ms = 0;
    while (std::abs(vsafe::num(motor->get_position()) - target) > 3.0){
        if (elapsed_ms > 150 && std::abs(vsafe::num(motor->get_actual_velocity())) < 1.0){
            break;
        }
        pros::delay(10);
        elapsed_ms += 10;
    }
}

void MotorGroup::stop(BrakeMode mode){
    ++CommandGen;
    const BrakeMode brake_mode = mode != NALL ? mode : DefaultBrakeMode;
    motor->set_brake_mode_all(static_cast<pros::v5::MotorBrake>(brake_mode));
    motor->brake();
}

void MotorGroup::set_position(float position, AngleUnits units){
    if (units == DEGREES){
        motor->set_zero_position_all(position);
    }else if (units == TURNS){
        motor->set_zero_position_all(position * 360.0);
    }else{
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
    }
}

void MotorGroup::set_velocity(float velocity, VelocityUnits units){
    Velocity=velocity;
    DefaultVelocityUnits=units;
}

void MotorGroup::set_stopping(BrakeMode mode){
    DefaultBrakeMode=mode;
}

void MotorGroup::set_max_torque(float value, TorqueUnits units){
    double max_nm = 1.05;
    double max_inlb = 9.29;
    switch (motor->get_gearing()){
        case pros::MotorGear::ratio_6_to_1: max_nm = 0.35; max_inlb = 3.1; break;
        case pros::MotorGear::ratio_36_to_1: max_nm = 2.1; max_inlb = 18.59; break;
        default: break;
    }

    // PROS takes a current limit in mA; 2500 mA is the V5 maximum.
    double limit_ma = 0.0;
    switch (units){
        case TORQUE_AMPS: limit_ma = value * 1000.0; break;
        case NM: limit_ma = vsafe::div(value, max_nm) * 2500.0; break;
        case INLB: limit_ma = vsafe::div(value, max_inlb) * 2500.0; break;
        case TORQUE_PERCENT: limit_ma = value * 25.0; break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            return;
    }
    limit_ma = std::clamp(vsafe::num(limit_ma), 0.0, 2500.0);
    motor->set_current_limit_all(static_cast<std::int32_t>(std::lround(limit_ma)));
}

void MotorGroup::set_timeout(float value, TimeUnits units){
    // Techinical difficulties plese stand by...
}

void MotorGroup::set_reversed(bool value){
    motor->set_reversed_all(value);
}

void MotorGroup::reset_position(){
    motor->tare_position_all();
}

bool MotorGroup::is_done(){
    return std::abs(vsafe::num(motor->get_actual_velocity())) < 1.0f;
}

bool MotorGroup::is_spinning(){
    return std::abs(vsafe::num(motor->get_actual_velocity())) >= 1.0f;
}

float MotorGroup::position(AngleUnits units){
    float output=0.0f;
    switch (units) {
        case DEGREES:
        {
            output= (float)(vsafe::num(motor->get_position()));
            break;
        }
        case TURNS:
        {
            output = (float)(vsafe::num(motor->get_position())) / 360;
        }
    }

    return output;
}

float MotorGroup::velocity(VelocityUnits units){
    float output=0.0f;
    switch (units){
        case VELOCITY_PERCENT:
        {
            int Max_RPM=1;
            switch (motor->get_gearing())
            {
            case pros::MotorGear::ratio_6_to_1:
            {
                Max_RPM=6;  // Includes / 100 for percentage.
                break;
            }   
            case pros::MotorGear::ratio_18_to_1:
            {
                Max_RPM=2;  // Includes / 100 for percentage.
                break;
            }
            case pros::MotorGear::ratio_36_to_1:
            {
                Max_RPM=1;  // Includes / 100 for percentage. 
                break;
            }
            default:
                break;
            }   

            output= (float)(vsafe::num(motor->get_actual_velocity())) / Max_RPM;
            break;
        }
        case RPM:
        {
            output= (float)(vsafe::num(motor->get_actual_velocity()));
            break;
        }
        case DPS:
        {
            output= (float)(vsafe::num(motor->get_actual_velocity())) * 6;  // Includes  * 360 to make DPM then / 60 to make DPS
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::velocity");
            break;
        }
    }

    return output;
}

float MotorGroup::current(CurrentUnits units){
    // PROS reports mA.
    return static_cast<float>(vsafe::num(motor->get_current_draw()) / 1000.0);
}

float MotorGroup::power(PowerUnits units){
    return (float)(vsafe::num(motor->get_power()));
}

float MotorGroup::torque(TorqueUnits units){
    float output= 0.0f;
    switch(units){
        case NM:
        {
            output= (float)(vsafe::num(motor->get_torque()));
            break;
        }
        case INLB:
        {
            float NMtoINLB = 8.8507f;
            output= (float)(vsafe::num(motor->get_torque())) * NMtoINLB;
            break;
        }
    }
    return output;
}

float MotorGroup::efficiency(EfficiencyUnits units){
    return vsafe::num(motor->get_efficiency());
}

int MotorGroup::temperature(TemperatureUnits units){
    const double celsius = vsafe::num(motor->get_temperature());
    switch(units){
        case CELSIUS: return static_cast<int>(std::lround(celsius));
        case FAHRENHEIT: return static_cast<int>(std::lround(celsius * 1.8 + 32.0));
        case TEMPERATURE_PERCENT: return static_cast<int>(std::lround(std::clamp((celsius - 20.0) * 2.0, 0.0, 100.0)));  // VEX: 0% = 20 C, 100% = 70 C
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            return 0;
    }
}

int MotorGroup::get_timeout(){
    int output = 0;
    switch (TimeoutType)
    {
    case SECONDS:
    {
        output=(int)(TimeoutTime * 1000);
        break;
    }
    case MSEC:
    {
        output=TimeoutTime;
        break;
    }
    default:
    mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::get_timeout");
        break;
    }
    return output; 
}

int MotorGroup::count(){
    return Count;
}