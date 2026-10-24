#include <stdexcept>
#include <initializer_list>
#include "main.h"
#include "vex/motors.hpp"

extern "C"{
    #include "py/runtime.h"
    #include "py/obj.h"
}

Motor::Motor(Ports port, GearSetting gears, bool reversed){
    PortNum=port;
    Ratio=gears;
    IsReversed=reversed;

    // 2. Cast your custom enum to the correct PROS 4 type: pros::v5::MotorGears
    // 3. Apply the reversal logic to the port number directly
    std::int8_t active_port = IsReversed ? -static_cast<std::int8_t>(PortNum) : static_cast<std::int8_t>(PortNum);
    
    pros::Motor motor(active_port, static_cast<pros::v5::MotorGears>(Ratio));
}

void Motor::timeout(){
    int TimeTo_MSEC=0;
    int ExpenedTime_MSEC;
    if (TimeoutType == SECONDS){
        TimeTo_MSEC=(int)(TimeoutTime * 1000);
    }else{
        TimeTo_MSEC=TimeoutTime;
    }

    while (ExpenedTime_MSEC >= TimeTo_MSEC && std::abs(motor->get_actual_velocity()) > 1){
        ExpenedTime_MSEC+=10;
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
        }
        case DPS:{
            velocity/=360;
            motor->move_velocity(velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::spin");
        }
    }
    
}

void Motor::spin_for(Direction direction, int angle, AngleUnits units, float velocity, VelocityUnits units_v, bool wait){
    if (velocity == 601){
        velocity = Velocity;
    }
    bool reverse = direction == REVERSE;
    velocity = reverse ? -velocity : velocity;

    switch(units) {
        case RPM:
        {
            motor->move_relative(angle, velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
            if (wait){
                while (std::abs(motor->get_actual_velocity()) > 1){
                    pros::delay(10);
                }
            }
            break;
        }
        case VELOCITY_PERCENT:
        {
            velocity*= static_cast<int>(motor->get_gearing());
            motor->move_relative(angle, velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
            if (wait){
                while (std::abs(motor->get_actual_velocity()) > 1){
                    pros::delay(10);
                }
            }
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::spin_for");
            break;
        }
    }
}

void Motor::spin_to_position(int rotation, AngleUnits units, float velocity, VelocityUnits units_v, bool wait){
    /*fix to make units_v work and use switch statments*/
    
    if (velocity > 600.0 || velocity < -600.0) [[unlikely]]{
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
                while (std::abs(motor->get_actual_velocity()) > 1){
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
                while (std::abs(motor->get_actual_velocity()) > 1){
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
    if (mode != NALL) [[likely]]{
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

            value=(Max_NM/value) * 2500;
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

            value=(Max_INLB/value) * 2500;
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
    return std::abs(motor->get_actual_velocity()) < 1.0f;
}

bool Motor::is_spinning(){
    return std::abs(motor->get_actual_velocity()) >= 1.0f;
}

float Motor::position(AngleUnits units){
    float output=0.0f;
    switch (units) {
        case DEGREES:
        {
            output= (float)(motor->get_position());
            break;
        }
        case TURNS:
        {
            output = (float)(motor->get_position()) / 360;
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

            output= (float)(motor->get_actual_velocity()) / Max_RPM;
            break;
        }
        case RPM:
        {
            output= (float)(motor->get_actual_velocity());
            break;
        }
        case DPS:
        {
            output= (float)(motor->get_actual_velocity()) * 6;  // Includes  * 360 to make DPM then / 60 to make DPS
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
    return (float)(motor->get_actual_velocity()) /1000;
}

float Motor::power(PowerUnits units){
    return (float)(motor->get_power());
}

float Motor::torque(TorqueUnits units){
    float output= 0.0f;
    switch(units){
        case NM:
        {
            output= (float)(motor->get_torque());
            break;
        }
        case INLB:
        {
            float NMtoINLB = 8.8507f;
            output= (float)(motor->get_torque()) / NMtoINLB;
            break;
        }
    }
    return output;
}

float Motor::efficiency(EfficiencyUnits units){
    return motor->get_efficiency();
}

int Motor::temperature(TemperatureUnits units){
    uint32_t output=0;
    switch(units){
        case CELSIUS:
        {
            output= (int)std::round((float)(motor->get_temperature()));
            break;
        }
        case FAHRENHEIT:
        {
            output= (int)std::round((float)(motor->get_temperature()) / 1.8f) + 32;
            break;
        }
        case TEMPERATURE_PERCENT:
        {
            output= (int)std::round((float)(motor->get_temperature()) * 0.57143f);
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

MotorGroup::MotorGroup(std::initializer_list<Motor> motors){
    pros::MotorGroup motor(extract_ports(motors)); 
    
    if (motors.size() > 0) {
        // Get the first motor in the list to use as a configuration template
        const pros::Motor& TemplateMotor = *motors.begin()->BASE;

        // Set the configuration of the group using the template motor's data
        motor.set_gearing(TemplateMotor.get_gearing());
        motor.set_encoder_units(TemplateMotor.get_encoder_units());
    }
    
    Count=motors.size();
}

MotorGroup::MotorGroup(Motor** motor_array, int count){
    pros::MotorGroup motor(extract_ports(motor_array, count)); 
    
    if (count > 0) {
        // Get the first motor in the list to use as a configuration template
        const pros::Motor& TemplateMotor = *motor_array[0]->BASE;

        // Set the configuration of the group using the template motor's data
        motor.set_gearing(TemplateMotor.get_gearing());
        motor.set_encoder_units(TemplateMotor.get_encoder_units());
    }
    
    Count=count;
}
    
void MotorGroup::timeout(){
    int TimeTo_MSEC=0;
    int ExpenedTime_MSEC;
    if (TimeoutType == SECONDS){
        TimeTo_MSEC=(int)(TimeoutTime * 1000);
    }else{
        TimeTo_MSEC=TimeoutTime;
    }

    while (ExpenedTime_MSEC >= TimeTo_MSEC && std::abs(motor->get_actual_velocity()) > 1){
        ExpenedTime_MSEC+=10;
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
        }
        case DPS:{
            velocity/=360;
            motor->move_velocity(velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::spin");
        }
    }
    
}

void MotorGroup::spin_for(Direction direction, int angle, AngleUnits units, float velocity, VelocityUnits units_v, bool wait){
    if (velocity == 601){
        velocity = Velocity;
    }
    bool reverse = direction == REVERSE;
    velocity = reverse ? -velocity : velocity;

    switch(units) {
        case RPM:
        {
            motor->move_relative(angle, velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
            if (wait){
                while (std::abs(motor->get_actual_velocity()) > 1){
                    pros::delay(10);
                }
            }
            
            break;
        }
        case VELOCITY_PERCENT:
        {
            velocity*= static_cast<int>(motor->get_gearing());
            motor->move_relative(angle, velocity);
            if (TimeoutTime != -1.0f) {
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
            if (wait){
                while (std::abs(motor->get_actual_velocity()) > 1){
                    pros::delay(10);
                }
            } 
            break;
        }
        default:
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Invalid argument type"));
            throw std::runtime_error("Given Invalid Input in Motor::spin_for");
            break;
        }
    }
}

void MotorGroup::spin_to_position(int rotation, AngleUnits units, float velocity, VelocityUnits units_v, bool wait){
    /*fix to make units_v work and use switch statments*/
    
    if (velocity > 600.0 || velocity < -600.0) [[unlikey]]{
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
                while (std::abs(motor->get_actual_velocity()) > 1){
                    pros::delay(10);
                }
            }
            break;
        }
        case TURNS:
        {
            rotation/=360;
            motor->move_absolute(rotation, velocity);
            if (TimeoutTime != -1.0f) [[likely]]{ 
                pros::Task timeout([this]() {
                    this->timeout(); // Or simply timeout();
                }, "timeout");
            } 
            if (wait){
                while (std::abs(motor->get_actual_velocity()) > 1){
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
    if (mode != NALL) [[likely]] {
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

            value=(Max_NM/value) * 2500;
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

            value=(Max_INLB/value) * 2500;
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
    return std::abs(motor->get_actual_velocity()) < 1.0f;
}

bool MotorGroup::is_spinning(){
    return std::abs(motor->get_actual_velocity()) >= 1.0f;
}

float MotorGroup::position(AngleUnits units){
    float output=0.0f;
    switch (units) {
        case DEGREES:
        {
            output= (float)(motor->get_position());
            break;
        }
        case TURNS:
        {
            output = (float)(motor->get_position()) / 360;
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

            output= (float)(motor->get_actual_velocity()) / Max_RPM;
            break;
        }
        case RPM:
        {
            output= (float)(motor->get_actual_velocity());
            break;
        }
        case DPS:
        {
            output= (float)(motor->get_actual_velocity()) * 6;  // Includes  * 360 to make DPM then / 60 to make DPS
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
    return (float)(motor->get_actual_velocity()) /1000;
}

float MotorGroup::power(PowerUnits units){
    return (float)(motor->get_power());
}

float MotorGroup::torque(TorqueUnits units){
    float output= 0.0f;
    switch(units){
        case NM:
        {
            output= (float)(motor->get_torque());
            break;
        }
        case INLB:
        {
            float NMtoINLB = 8.8507f;
            output= (float)(motor->get_torque()) / NMtoINLB;
            break;
        }
    }
    return output;
}

float MotorGroup::efficiency(EfficiencyUnits units){
    return motor->get_efficiency();
}

int MotorGroup::temperature(TemperatureUnits units){
    unsigned short int output=0;
    switch(units){
        case CELSIUS:
        {
            output= (int)std::round((float)(motor->get_temperature()));
            break;
        }
        case FAHRENHEIT:
        {
            output= (int)std::round((float)(motor->get_temperature()) / 1.8f) + 32;
            break;
        }
        case TEMPERATURE_PERCENT:
        {
            output= (int)std::round((float)(motor->get_temperature()) * 0.57143f);
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