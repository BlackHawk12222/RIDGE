#ifndef THREE_WIRE_SONAR_HPP
#define THREE_WIRE_SONAR_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class Sonar : public ThreeWireDevice {
public:
    explicit Sonar(int port);
};

#endif
