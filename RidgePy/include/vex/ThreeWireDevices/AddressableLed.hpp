#ifndef THREE_WIRE_ADDRESSABLE_LED_HPP
#define THREE_WIRE_ADDRESSABLE_LED_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class AddressableLed : public ThreeWireDevice {
public:
    explicit AddressableLed(int port);
};

#endif
