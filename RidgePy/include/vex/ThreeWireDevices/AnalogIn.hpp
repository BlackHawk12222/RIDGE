#ifndef THREE_WIRE_ANALOG_IN_HPP
#define THREE_WIRE_ANALOG_IN_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class AnalogIn : public ThreeWireDevice {
public:
    explicit AnalogIn(int port);
};

#endif
