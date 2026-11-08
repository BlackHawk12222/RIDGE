#include "vex/Sensors/Distance.hpp"

Distance::Distance(Ports port){
    Sensor = new pros::Distance(static_cast<std::uint8_t>(port));
}

float Distance::object_distance(DistanceUnits units){
    const float millimeters = static_cast<float>(Sensor->get_distance());
    return units == INCHES ? millimeters / 25.4f : millimeters;
}

float Distance::object_velocity(){
    return static_cast<float>(Sensor->get_object_velocity());
}

ObjectSize Distance::object_size(){
    const int size = Sensor->get_object_size();
    if (size < 0){
        return static_cast<ObjectSize>(size);
    }
    if (size == 0){
        return ObjectSize::NONE;
    }
    if (size < 134){
        return ObjectSize::SMALL;
    }
    if (size < 267){
        return ObjectSize::MEDIUM;
    }
    return ObjectSize::LARGE;
}

bool Distance::is_object_detected(){
    const int millimeters = Sensor->get_distance();
    return millimeters >= 20 && millimeters <= 2000;
}
