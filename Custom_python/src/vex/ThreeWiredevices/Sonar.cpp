#include "vex/ThreeWireDevices/Sonar.hpp"

Sonar::Sonar(int port) : ThreeWireDevice(ThreeWireDevice::RangeFinder, port) {}
