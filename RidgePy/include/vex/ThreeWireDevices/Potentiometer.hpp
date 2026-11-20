#ifndef THREE_WIRE_POTENTIOMETER_HPP
#define THREE_WIRE_POTENTIOMETER_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class Potentiometer : public ThreeWireDevice {
public:
    explicit Potentiometer(int port);
};

#endif
