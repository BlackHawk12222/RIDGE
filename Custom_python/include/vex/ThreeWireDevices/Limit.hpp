#ifndef THREE_WIRE_LIMIT_HPP
#define THREE_WIRE_LIMIT_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class Limit : public ThreeWireDevice {
public:
    explicit Limit(int port);
};

#endif
