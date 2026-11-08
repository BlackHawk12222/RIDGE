#ifndef THREE_WIRE_PWM_HPP
#define THREE_WIRE_PWM_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class Pwm : public ThreeWireDevice {
public:
    explicit Pwm(int port);
};

#endif
