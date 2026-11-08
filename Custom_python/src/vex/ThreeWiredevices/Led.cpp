#include "vex/ThreeWireDevices/Led.hpp"

Led::Led(int port) : ThreeWireDevice(ThreeWireDevice::LedIndicator, port) {}
