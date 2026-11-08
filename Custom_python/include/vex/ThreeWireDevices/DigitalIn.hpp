#ifndef THREE_WIRE_DIGITAL_IN_HPP
#define THREE_WIRE_DIGITAL_IN_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class DigitalIn : public ThreeWireDevice {
public:
    explicit DigitalIn(int port);
};

#endif
