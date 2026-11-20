#ifndef THREE_WIRE_LINE_HPP
#define THREE_WIRE_LINE_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class Line : public ThreeWireDevice {
public:
    explicit Line(int port);
};

#endif
