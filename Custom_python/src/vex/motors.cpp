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

Motor::Motor(Ports port, GearSetting gears, bool reversed){
    PortNum = port;
    Ratio = gears;
    IsReversed = reversed;

    std::int8_t active_port = IsReversed ? -static_cast<std::int8_t>(PortNum) : static_cast<std::int8_t>(PortNum);

    this->motor = new pros::Motor(active_port, static_cast<pros::v5::MotorGears>(Ratio));
    this->BASE = this->motor;
}


void Motor::timeout(){
    int target_ms = (TimeoutType == SECONDS) ? static_cast<int>(TimeoutTime * 1000.0f)
                                             : static_cast<int>(TimeoutTime);
    int elapsed_ms = 0;

    while (elapsed_ms < target_ms && std::abs(vsafe::num(motor->get_actual_velocity())) > 1.0) {
        elapsed_ms += 10;
        pros::delay(10);
    }

    motor->brake();
}


void Motor::spin(Direction direction, float velocity, VelocityUnits units){
    if (velocity == 601){
        velocity = Velocity;
    }
    bool reverse = direction == REVERSE;
    velocity = reverse ? -velocity : velocity;

    switch(units) {
        case RPM:
        {
            motor->move_velocity(velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
            break;
        }
        case VELOCITY_VOLT:
        {
            velocity*=10;
            motor->move(velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            }
            break;
        }
        case VELOCITY_PERCENT:
        {
            velocity*= static_cast<int>(motor->get_gearing());
            motor->move_velocity(velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            }
            break;
        }
        case DPS:{
            velocity/=360;
            motor->move_velocity(velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            }
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            break;
        }
    }
    
}

void Motor::spin_for(Direction direction, int angle, AngleUnits units, float velocity, VelocityUnits units_v, bool wait){
    // 601 means "no velocity given": use set_velocity()'s value and units.
    if (velocity == 601){
        velocity = Velocity;
        units_v = DefaultVelocityUnits;
    }

    // Angle units pick the distance; velocity units pick the speed.
    double degrees = static_cast<double>(angle);
    switch (units) {
        case DEGREES: break;
        case TURNS: degrees *= 360.0; break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            return;
    }

    const double max_rpm = motor_max_rpm(motor->get_gearing());
    double rpm = 0.0;
    switch (units_v) {
        case RPM: rpm = velocity; break;
        case VELOCITY_PERCENT: rpm = velocity * max_rpm / 100.0; break;
        case DPS: rpm = velocity / 6.0; break;
        case VELOCITY_VOLT: rpm = vsafe::div(velocity * max_rpm, 12.0); break;
        default:
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
    if (TimeoutTime != -1.0f) {
        pros::Task timeout([this]() {
            this->timeout();
        }, "timeout");
    }

    if (wait && degrees != 0.0 && rpm > 0.0){
        // Done when it reaches the target, or when it has stopped moving
        // after starting (stalled or stopped by the timeout).
        int elapsed_ms = 0;
        while (std::abs(vsafe::num(motor->get_position()) - target) > 3.0){
            if (elapsed_ms > 150 && std::abs(vsafe::num(motor->get_actual_velocity())) < 1.0){
                break;
            }
            pros::delay(10);
            elapsed_ms += 10;
        }
    }
}

void Motor::spin_to_position(int rotation, AngleUnits units, float velocity, VelocityUnits units_v, bool wait){
    /*fix to make units_v work and use switch statments*/
    
    if (velocity > 600.0 || velocity < -600.0) {
        velocity = Velocity;
    }

    switch (units_v){
        case RPM:
        {
            break;
        }
        case VELOCITY_PERCENT:
        {
            float Max_RPM=1.0;
            switch (motor->get_gearing())
            {
            case pros::MotorGear::ratio_6_to_1:
            {
                Max_RPM=6;
                break;
            }   
            case pros::MotorGear::ratio_18_to_1:
            {
                Max_RPM=2;
                break;
            }
            case pros::MotorGear::ratio_36_to_1:
            {
                Max_RPM=1;
                break;
            }
            default:
                break;
            }
            velocity/=Max_RPM;
            break;
        }
        case DPS:
        {
            velocity*=6;  // Includes  * 360 to make DPM then / 60 to make DPS
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::spin_to_position");
            break;
        }
    }

    switch (units){
        case DEGREES:
        {
            motor->move_absolute(rotation, velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
            if (wait){
                while (std::abs(vsafe::num(motor->get_actual_velocity())) > 1){
                    pros::delay(10);
                }
            }
            break;
        }
        case TURNS:
        {
            rotation/=360;
            motor->move_absolute(rotation, velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout();
                }, "timeout");
            } 
            if (wait){
                while (std::abs(vsafe::num(motor->get_actual_velocity())) > 1){
                    pros::delay(10);
                }
            }
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::spin_to_position");
            break;
        }
    }
}

void Motor::stop(BrakeMode mode){
    if (mode != NALL) {
        motor->set_brake_mode((pros::v5::MotorBrake)mode);
        motor->brake();
    }else{
        motor->set_brake_mode((pros::v5::MotorBrake)DefaultBrakeMode);
        motor->brake();
    }
}

void Motor::set_position(float position, AngleUnits units){
    if (units == DEGREES){
        motor->set_zero_position(position);
    }else if (units == TURNS){
        position/=360.0;
        motor->set_zero_position(position);
    }else{
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
        throw std::runtime_error("Given Invalid Input in Motor::set_position");
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
    switch (units){
        case TORQUE_AMPS:
        {
            value/=1000;
            motor->set_current_limit(value);
            break;
        }
        case NM:
        {
            double Max_NM=1.0;
            switch (motor->get_gearing())
            {
            case pros::MotorGear::ratio_6_to_1:
            {
                Max_NM=0.35;
                break;
            }   
            case pros::MotorGear::ratio_18_to_1:
            {
                Max_NM=1.05;
                break;
            }
            case pros::MotorGear::ratio_36_to_1:
            {
                Max_NM=2.1;
                break;
            }
            default:
                break;
            }

            value = static_cast<float>(vsafe::div(value, Max_NM) * 2500);
            motor->set_current_limit(value);
            break;
        }
        case INLB:
        {
            double Max_INLB=1.0;
            switch (motor->get_gearing())
            {
            case pros::MotorGear::ratio_6_to_1:
            {
                Max_INLB=3.1f;
                break;
            }   
            case pros::MotorGear::ratio_18_to_1:
            {
                Max_INLB=9.29;
                break;
            }
            case pros::MotorGear::ratio_36_to_1:
            {
                Max_INLB=18.59;
                break;
            }
            default:
                break;
            }

            value = static_cast<float>(vsafe::div(value, Max_INLB) * 2500);
            motor->set_current_limit(value);
            break;
        }
        case TORQUE_PERCENT:
        {
            value*=25; // (Percent /100) * 2500
            motor->set_current_limit(value);
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::set_max_torque");
            break;
        }
    }
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
    return (float)(vsafe::num(motor->get_actual_velocity())) /1000;
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
            output= (float)(vsafe::num(motor->get_torque())) / NMtoINLB;
            break;
        }
    }
    return output;
}

float Motor::efficiency(EfficiencyUnits units){
    return vsafe::num(motor->get_efficiency());
}

int Motor::temperature(TemperatureUnits units){
    double raw_temp = vsafe::num(motor->get_temperature());
    if (!std::isfinite(raw_temp) || raw_temp < 0.0 || raw_temp > 200.0) {
        return 2;
    }

    int output = 0;
    switch(units){
        case CELSIUS:
            output = (int)std::round(raw_temp);
            break;
        case FAHRENHEIT:
            output = (int)std::round((raw_temp * 1.8) + 32.0);
            break;
        case TEMPERATURE_PERCENT:
            output = (int)std::round(raw_temp * 0.57143);
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::temperature");
    }
    return output;
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
    
void MotorGroup::timeout(){
    int target_ms = (TimeoutType == SECONDS) ? static_cast<int>(TimeoutTime * 1000.0f)
                                             : static_cast<int>(TimeoutTime);
    int elapsed_ms = 0;

    while (elapsed_ms < target_ms && std::abs(vsafe::num(motor->get_actual_velocity())) > 1.0) {
        elapsed_ms += 10;
        pros::delay(10);
    }

    motor->brake();
}


void MotorGroup::spin(Direction direction, float velocity, VelocityUnits units){
    if (velocity == 601){
        velocity = Velocity;
    }
    bool reverse = direction == REVERSE;
    velocity = reverse ? -velocity : velocity;

    switch(units) {
        case RPM:
        {
            motor->move_velocity(velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
            break;
        }
        case VOLT:
        {
            velocity*=10;
            motor->move(velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            }
            break;        
        }
        case VELOCITY_PERCENT:
        {
            velocity*= static_cast<int>(motor->get_gearing());
            motor->move_velocity(velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout();
                }, "timeout");
            }  
            break;
        }
        case DPS:{
            velocity/=360;
            motor->move_velocity(velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            break;
        }
    }
    
}

void MotorGroup::spin_for(Direction direction, int angle, AngleUnits units, float velocity, VelocityUnits units_v, bool wait){
    // 601 means "no velocity given": use set_velocity()'s value and units.
    if (velocity == 601){
        velocity = Velocity;
        units_v = DefaultVelocityUnits;
    }

    // Angle units pick the distance; velocity units pick the speed.
    double degrees = static_cast<double>(angle);
    switch (units) {
        case DEGREES: break;
        case TURNS: degrees *= 360.0; break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            return;
    }

    const double max_rpm = motor_max_rpm(motor->get_gearing());
    double rpm = 0.0;
    switch (units_v) {
        case RPM: rpm = velocity; break;
        case VELOCITY_PERCENT: rpm = velocity * max_rpm / 100.0; break;
        case DPS: rpm = velocity / 6.0; break;
        case VELOCITY_VOLT: rpm = vsafe::div(velocity * max_rpm, 12.0); break;
        default:
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
    if (TimeoutTime != -1.0f) {
        pros::Task timeout([this]() {
            this->timeout();
        }, "timeout");
    }

    if (wait && degrees != 0.0 && rpm > 0.0){
        // Done when it reaches the target, or when it has stopped moving
        // after starting (stalled or stopped by the timeout).
        int elapsed_ms = 0;
        while (std::abs(vsafe::num(motor->get_position()) - target) > 3.0){
            if (elapsed_ms > 150 && std::abs(vsafe::num(motor->get_actual_velocity())) < 1.0){
                break;
            }
            pros::delay(10);
            elapsed_ms += 10;
        }
    }
}

void MotorGroup::spin_to_position(int rotation, AngleUnits units, float velocity, VelocityUnits units_v, bool wait){
    if (velocity > 600.0 || velocity < -600.0) {
        velocity = Velocity;
    }

    switch (units_v){
        case RPM:
        {
            break;
        }
        case VELOCITY_PERCENT:
        {
            float Max_RPM=1.0;
            switch (motor->get_gearing())
            {
            case pros::MotorGear::ratio_6_to_1:
            {
                Max_RPM=6;
                break;
            }   
            case pros::MotorGear::ratio_18_to_1:
            {
                Max_RPM=2;
                break;
            }
            case pros::MotorGear::ratio_36_to_1:
            {
                Max_RPM=1;
                break;
            }
            default:
                break;
            }
            velocity/=Max_RPM;
            break;
        }
        case DPS:
        {
            velocity*=6;  // Includes  * 360 to make DPM then / 60 to make DPS
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::spin_to_position");
            break;
        }
    }

    switch (units){
        case DEGREES:
        {
            motor->move_absolute(rotation, velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
            if (wait){
                while (std::abs(vsafe::num(motor->get_actual_velocity())) > 1){
                    pros::delay(10);
                }
            }
            break;
        }
        case TURNS:
        {
            rotation/=360;
            motor->move_absolute(rotation, velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
            if (wait){
                while (std::abs(vsafe::num(motor->get_actual_velocity())) > 1){
                    pros::delay(10);
                }
            }
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::spin_to_position");
            break;
        }
    }
}

void MotorGroup::stop(BrakeMode mode){
    if (mode != NALL) { 
        motor->set_brake_mode((pros::v5::MotorBrake)mode);
        motor->brake();
    }else{
        motor->set_brake_mode((pros::v5::MotorBrake)DefaultBrakeMode);
        motor->brake();
    }
}

void MotorGroup::set_position(float position, AngleUnits units){
    if (units == DEGREES){
        motor->set_zero_position(position);
    }else if (units == TURNS){
        position/=360.0;
        motor->set_zero_position(position);
    }else{
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
        throw std::runtime_error("Given Invalid Input in Motor::set_position");
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
    switch (units){
        case TORQUE_AMPS:
        {
            value/=1000;
            motor->set_current_limit(value);
            break;
        }
        case NM:
        {
            double Max_NM=1.0;
            switch (motor->get_gearing())
            {
            case pros::MotorGear::ratio_6_to_1:
            {
                Max_NM=0.35;
                break;
            }   
            case pros::MotorGear::ratio_18_to_1:
            {
                Max_NM=1.05;
                break;
            }
            case pros::MotorGear::ratio_36_to_1:
            {
                Max_NM=2.1;
                break;
            }
            default:
                break;
            }

            value = static_cast<float>(vsafe::div(value, Max_NM) * 2500);
            motor->set_current_limit(value);
            break;
        }
        case INLB:
        {
            double Max_INLB=1.0;
            switch (motor->get_gearing())
            {
            case pros::MotorGear::ratio_6_to_1:
            {
                Max_INLB=3.1f;
                break;
            }   
            case pros::MotorGear::ratio_18_to_1:
            {
                Max_INLB=9.29;
                break;
            }
            case pros::MotorGear::ratio_36_to_1:
            {
                Max_INLB=18.59;
                break;
            }
            default:
                break;
            }

            value = static_cast<float>(vsafe::div(value, Max_INLB) * 2500);
            motor->set_current_limit(value);
            break;
        }
        case TORQUE_PERCENT:
        {
            value*=25; // (Percent /100) * 2500
            motor->set_current_limit(value);
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::set_max_torque");
            break;
        }
    }
}

void MotorGroup::set_timeout(float value, TimeUnits units){
    // Techinical difficulties plese stand by...
}

void MotorGroup::set_reversed(bool value){
    motor->set_reversed(value);
}

void MotorGroup::reset_position(){
    motor->tare_position();
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
    return (float)(vsafe::num(motor->get_actual_velocity())) /1000;
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
            output= (float)(vsafe::num(motor->get_torque())) / NMtoINLB;
            break;
        }
    }
    return output;
}

float MotorGroup::efficiency(EfficiencyUnits units){
    return vsafe::num(motor->get_efficiency());
}

int MotorGroup::temperature(TemperatureUnits units){
    unsigned short int output=0;
    switch(units){
        case CELSIUS:
        {
            output= (int)std::round((float)(vsafe::num(motor->get_temperature())));
            break;
        }
        case FAHRENHEIT:
        {
            output= (int)std::round((float)(vsafe::num(motor->get_temperature())) / 1.8f) + 32;
            break;
        }
        case TEMPERATURE_PERCENT:
        {
            output= (int)std::round((float)(vsafe::num(motor->get_temperature())) * 0.57143f);
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::temperature");
            break;
        }
    }
    return output;
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