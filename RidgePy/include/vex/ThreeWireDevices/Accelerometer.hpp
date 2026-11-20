#ifndef THREE_WIRE_ACCELEROMETER_HPP
#define THREE_WIRE_ACCELEROMETER_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class Accelerometer : public ThreeWireDevice {
public:
    explicit Accelerometer(int port);
};

#endif
