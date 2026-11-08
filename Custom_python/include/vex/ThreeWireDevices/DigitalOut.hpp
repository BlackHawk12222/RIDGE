#ifndef DIGITAL_OUT_HPP
#define DIGITAL_OUT_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class DigitalOut : public ThreeWireDevice {
public:
    explicit DigitalOut(int port);
};

#endif // DIGITAL_OUT_HPP