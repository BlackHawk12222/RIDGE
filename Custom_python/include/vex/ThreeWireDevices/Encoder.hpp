#ifndef THREE_WIRE_ENCODER_HPP
#define THREE_WIRE_ENCODER_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class Encoder : public ThreeWireDevice {
public:
    explicit Encoder(int port);
};

#endif
