#include "vex/ThreeWireDevices/DigitalOut.hpp"

DigitalOut::DigitalOut(int port) : ThreeWireDevice(ThreeWireDevice::DigitalOut, port) {}