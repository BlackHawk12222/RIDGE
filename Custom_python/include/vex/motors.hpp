#ifndef MOTOR_HPP
#define MOTOR_HPP

#include <stdexcept>
#include <initializer_list>
#include "main.h"
#include "vex/timer.hpp"
#include "vex/brain.hpp"

extern "C"{
    #include "py/runtime.h"
    #include "py/obj.h"
}

enum Ports {
    PORT1=1,
    PORT2=2,
    PORT3=3,
    PORT4=4,
    PORT5=5,
    PORT6=6,
    PORT7=7,
    PORT8=8,
    PORT9=9,
    PORT10=10,
    PORT11=11,
    PORT12=12,
    PORT13=13,
    PORT14=14,
    PORT15=15,
    PORT16=16,
    PORT17=17,
    PORT18=18,
    PORT19=19,
    PORT20=20,
    PORT21=21,
    PORT22=22
};

enum GearSetting {
    RATIO_6_1=8,
    RATIO_18_1=18,
    RATIO_36_1=36
};

enum VelocityUnits{
    VELOCITY_PERCENT=23,
    RPM=24,
    VOLT=25,
    DPS=26
};

enum Direction{
    FORWARD=27,
    REVERSE=28 
};

enum AngleUnits{
    DEGREES=29,
    TURNS=30
};

enum BrakeMode{
    BRAKE=static_cast<int>(pros::v5::MotorBrake::brake),
    COAST=static_cast<int>(pros::v5::MotorBrake::coast),
    HOLD=static_cast<int>(pros::v5::MotorBrake::hold),
    NALL=-1
};

enum TorqueUnits{
    TORQUE_PERCENT=23,
    NM=31,
    INLB=32,
    TORQUE_AMPS=33
};

enum PowerUnits{
    WATT=34
};

enum EfficiencyUnits{
    EFFICIENCY_PERCENT=23
};

class Motor {
    private:
        Ports PortNum;
        GearSetting Ratio;
        bool IsReversed;
        pros::Motor* motor=nullptr;
        float Velocity = 50.0f;
        VelocityUnits DefaultVelocityUnits=RPM;
        BrakeMode DefaultBrakeMode=BRAKE;
        float TimeoutTime = -1.0f;
        TimeUnits TimeoutType = MSEC;

        void timeout();

    public:
    
        Motor(Ports port, GearSetting gears=RATIO_18_1, bool reversed=false);

        pros::Motor* BASE=motor;

        void spin(Direction direction, float velocity=601.0, VelocityUnits units=RPM);

        void spin_for(Direction direction, int angle, AngleUnits units=DEGREES, float velocity=601.0f, VelocityUnits units_v=RPM, bool wait=true);

        void spin_to_position(int rotation, AngleUnits units=DEGREES, float velocity=601.0f, VelocityUnits units_v=RPM, bool wait=true);

        void stop(BrakeMode mode= NALL);
        
        void set_position(float position, AngleUnits units);

        void set_velocity(float velocity, VelocityUnits units);

        void set_stopping(BrakeMode mode);

        void set_max_torque(float value, TorqueUnits units);

        void set_timeout(float value, TimeUnits units);

        void set_reversed(bool value);

        void reset_position();

        bool is_done();

        bool is_spinning();

        float position(AngleUnits units);

        float velocity(VelocityUnits units);

        float current(CurrentUnits units);

        float power(PowerUnits units);

        float torque(TorqueUnits units);

        float efficiency(EfficiencyUnits units);

        int temperature(TemperatureUnits units);

        int get_timeout();
};

class MotorGroup{ 
    private:
        Ports PortNum;
        GearSetting Ratio;
        bool IsReversed;
        pros::MotorGroup* motor=nullptr;
        float Velocity=50.0f;
        VelocityUnits DefaultVelocityUnits=RPM;
        BrakeMode DefaultBrakeMode=BRAKE;
        float TimeoutTime=-1.0f;
        TimeUnits TimeoutType=MSEC;
        int Count=0;

        void timeout();

        static std::vector<std::int8_t> extract_ports(std::initializer_list<Motor> motors){
            std::vector<std::int8_t> PortList;
            for (Motor motorloop : motors){
                PortList.push_back(motorloop.BASE->get_port());
            }
            return PortList;
        }

        static std::vector<std::int8_t> extract_ports(Motor** motors, int count){
            std::vector<std::int8_t> PortList;
            for (int i=1; i<=count; i++){
                PortList.push_back(motors[i]->BASE->get_port());
            }
            return PortList;
        }

    public:
        MotorGroup(std::initializer_list<Motor> motors);

        MotorGroup(Motor** motor_array, int count);
            
        void spin(Direction direction, float velocity=601.0, VelocityUnits units=RPM);

        void spin_for(Direction direction, int angle, AngleUnits units=DEGREES, float velocity=601.0, VelocityUnits units_v=RPM, bool wait=true);

        void spin_to_position(int rotation, AngleUnits units=DEGREES, float velocity=601.0f, VelocityUnits units_v=RPM, bool wait=true);

        void stop(BrakeMode mode= NALL);
        
        void set_position(float position, AngleUnits units);

        void set_velocity(float velocity, VelocityUnits units);

        void set_stopping(BrakeMode mode);

        void set_max_torque(float value, TorqueUnits units);

        void set_timeout(float value, TimeUnits units);

        void set_reversed(bool value);

        void reset_position();

        bool is_done();

        bool is_spinning();

        float position(AngleUnits units);

        float velocity(VelocityUnits units);

        float current(CurrentUnits units);

        float power(PowerUnits units);

        float torque(TorqueUnits units);

        float efficiency(EfficiencyUnits units);

        int temperature(TemperatureUnits units);

        int get_timeout();

        int count();
};

#endif //MOTOR_HPP