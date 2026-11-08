#include "vex/ThreeWireDevices/DigitalOut.hpp"

DigitalOut::DigitalOut(int port){
    Output = new pros::adi::DigitalOut(static_cast<std::uint8_t>(port));
}

void DigitalOut::set(bool value){
    Output->set_value(value);
}