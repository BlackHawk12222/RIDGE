#include "vex/ThreeWireDevices/PotentiometerV2.hpp"

PotentiometerV2::PotentiometerV2(int port) : ThreeWireDevice(ThreeWireDevice::Potentiometer, port, 1) {}
