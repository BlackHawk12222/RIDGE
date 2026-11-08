#ifndef THREE_WIRE_GYRO_HPP
#define THREE_WIRE_GYRO_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class Gyro : public ThreeWireDevice {
public:
    explicit Gyro(int port);
};

#endif
