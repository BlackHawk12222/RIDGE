#ifndef THREE_WIRE_POTENTIOMETER_V2_HPP
#define THREE_WIRE_POTENTIOMETER_V2_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class PotentiometerV2 : public ThreeWireDevice {
public:
    explicit PotentiometerV2(int port);
};

#endif
