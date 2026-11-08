#ifndef THREE_WIRE_BUMPER_HPP
#define THREE_WIRE_BUMPER_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class Bumper : public ThreeWireDevice {
public:
    explicit Bumper(int port);
};

#endif
