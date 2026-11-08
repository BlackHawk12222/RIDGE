#ifndef THREE_WIRE_LIGHT_HPP
#define THREE_WIRE_LIGHT_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class Light : public ThreeWireDevice {
public:
    explicit Light(int port);
};

#endif
