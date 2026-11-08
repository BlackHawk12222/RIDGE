#include "vex/ThreeWireDevices/ObjectDetector.hpp"

ObjectDetector::ObjectDetector(int port, bool reverse_detection)
    : ThreeWireDevice(ThreeWireDevice::ObjectSensor, port, reverse_detection) {}
