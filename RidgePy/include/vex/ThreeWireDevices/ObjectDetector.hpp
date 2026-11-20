#ifndef THREE_WIRE_OBJECT_DETECTOR_HPP
#define THREE_WIRE_OBJECT_DETECTOR_HPP

#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

class ObjectDetector : public ThreeWireDevice {
public:
    explicit ObjectDetector(int port, bool reverse_detection = false);
};

#endif
