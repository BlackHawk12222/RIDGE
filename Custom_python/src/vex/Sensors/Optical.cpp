#include "vex/Sensors/Optical.hpp"

Optical::Optical(Ports port){
    Sensor = new pros::Optical(static_cast<std::uint8_t>(port));
}

void Optical::set_light(bool state){
    Sensor->set_led_pwm(state ? 100 : 0);
}

void Optical::set_light_power(int percent){
    Sensor->set_led_pwm(static_cast<std::uint8_t>(percent));
}

bool Optical::is_near_object(){
    return Sensor->get_proximity() > 0;
}

OpticalColor Optical::color(){
    if (!is_near_object()){
        return OpticalColor::BLACK;
    }

    const float brightness_value = static_cast<float>(Sensor->get_brightness());
    const float saturation = static_cast<float>(Sensor->get_saturation());
    const float hue_value = static_cast<float>(Sensor->get_hue());
    if (brightness_value < 0.1f){
        return OpticalColor::BLACK;
    }
    if (brightness_value > 0.9f && saturation < 0.2f){
        return OpticalColor::WHITE;
    }
    if (hue_value >= 340.0f || hue_value <= 20.0f){
        return OpticalColor::RED;
    }
    if (hue_value <= 40.0f){
        return OpticalColor::ORANGE;
    }
    if (hue_value <= 60.0f){
        return OpticalColor::YELLOW;
    }
    if (hue_value >= 80.0f && hue_value <= 140.0f){
        return OpticalColor::GREEN;
    }
    if (hue_value <= 200.0f && hue_value > 140.0f){
        return OpticalColor::CYAN;
    }
    if (hue_value <= 240.0f && hue_value > 200.0f){
        return OpticalColor::BLUE;
    }
    if (hue_value <= 280.0f && hue_value > 240.0f){
        return OpticalColor::PURPLE;
    }
    return OpticalColor::BLACK;
}

float Optical::brightness(){
    return static_cast<float>(Sensor->get_brightness());
}

float Optical::hue(){
    return static_cast<float>(Sensor->get_hue());
}
