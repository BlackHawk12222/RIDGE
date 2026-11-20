#ifndef THREE_WIRE_LED_HPP
#define THREE_WIRE_LED_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class Led : public ThreeWireDevice {
public:
    explicit Led(int port);
};

#endif
