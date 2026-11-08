#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"

#include "pros/rtos.hpp"

#include <algorithm>
#include <cmath>

namespace {
float degrees_from_units(float value, int units) {
    if (units == 30) {
        return value / 360.0f;
    }
    return value;
}

float inches_from_mm(float value, int units) {
    if (units == 41) {
        return value / 25.4f;
    }
    return value;
}
}

ThreeWireDevice::ThreeWireDevice(Kind kind, int port, int option)
    : kind_(kind), port_(port), option_(option) {
    const auto adi_port = static_cast<std::uint8_t>(port);
    switch (kind_) {
        case AnalogIn:
        case Light:
        case Line:
        case ObjectSensor:
        case Accelerometer:
            analog_ = new pros::adi::AnalogIn(adi_port);
            break;
        case DigitalIn:
        case Bumper:
        case Limit:
            digital_in_ = new pros::adi::DigitalIn(adi_port);
            break;
        case DigitalOut:
        case LedIndicator:
            digital_out_ = new pros::adi::DigitalOut(adi_port);
            break;
        case Encoder:
            encoder_ = new pros::adi::Encoder(adi_port, static_cast<std::uint8_t>(port + 1));
            encoder_previous_time_ = pros::millis();
            break;
        case Gyro:
            gyro_ = new pros::adi::Gyro(adi_port);
            break;
        case Potentiometer:
            potentiometer_ = new pros::adi::Potentiometer(
                adi_port,
                option_ ? pros::E_ADI_POT_V2 : pros::E_ADI_POT_EDR);
            break;
        case Pwm:
            pwm_ = new pros::adi::Motor(adi_port);
            break;
        case RangeFinder:
            ultrasonic_ = new pros::adi::Ultrasonic(adi_port, static_cast<std::uint8_t>(port + 1));
            break;
        case AddressableLed:
            led_ = pros::c::adi_led_init(adi_port);
            break;
    }
}

ThreeWireDevice::~ThreeWireDevice() {
    for (auto& task : callback_tasks_) {
        task->remove();
    }
    delete analog_;
    delete digital_in_;
    delete digital_out_;
    delete pwm_;
    delete encoder_;
    delete gyro_;
    delete potentiometer_;
    delete ultrasonic_;
}

int ThreeWireDevice::value(int units) {
    if (kind_ == DigitalIn || kind_ == Bumper || kind_ == Limit) {
        return digital_in_->get_value();
    }
    const int raw = analog_->get_value();
    if (units == 23) {
        return raw * 100 / 4095;
    }
    if (units == 42) {
        return raw >> 4;
    }
    if (units == 43) {
        return raw >> 2;
    }
    return raw;
}

float ThreeWireDevice::pwm_value() const {
    return pwm_value_;
}

bool ThreeWireDevice::pressing() {
    return digital_in_->get_value() != 0;
}

float ThreeWireDevice::acceleration() {
    return (analog_->get_value() * (5.0f / 4095.0f) - 2.5f) / 0.8f;
}

float ThreeWireDevice::brightness() {
    return analog_->get_value() * 100.0f / 4095.0f;
}

int ThreeWireDevice::reflectivity() {
    return analog_->get_value() * 100 / 4095;
}

bool ThreeWireDevice::is_object_detected() {
    const bool detected = reflectivity() >= object_threshold_;
    return option_ ? !detected : detected;
}

void ThreeWireDevice::set_threshold(int threshold) {
    object_threshold_ = threshold;
}

float ThreeWireDevice::angle(int units) {
    const float degrees = static_cast<float>(potentiometer_->get_angle()) / 10.0f;
    if (units == 23) {
        return degrees * 100.0f / (option_ ? 330.0f : 250.0f);
    }
    return degrees;
}

float ThreeWireDevice::heading(int units) {
    const float degrees = static_cast<float>(gyro_->get_value()) / 10.0f + gyro_heading_offset_;
    return degrees_from_units(degrees, units);
}

float ThreeWireDevice::rotation(int units) {
    const float degrees = static_cast<float>(gyro_->get_value()) / 10.0f + gyro_rotation_offset_;
    return degrees_from_units(degrees, units);
}

void ThreeWireDevice::set_heading(float value, int units) {
    gyro_heading_offset_ = (units == 30 ? value * 360.0f : value) - static_cast<float>(gyro_->get_value()) / 10.0f;
}

void ThreeWireDevice::set_rotation(float value, int units) {
    gyro_rotation_offset_ = (units == 30 ? value * 360.0f : value) - static_cast<float>(gyro_->get_value()) / 10.0f;
}

void ThreeWireDevice::calibrate() {
    gyro_->reset();
    gyro_heading_offset_ = 0.0f;
    gyro_rotation_offset_ = 0.0f;
}

void ThreeWireDevice::set_position(float value, int units) {
    encoder_position_offset_ = units == 30 ? value * 360.0f : value;
    encoder_->reset();
}

void ThreeWireDevice::reset_position() {
    encoder_->reset();
    encoder_position_offset_ = 0.0f;
}

float ThreeWireDevice::position(int units) {
    return degrees_from_units(static_cast<float>(encoder_->get_value()) + encoder_position_offset_, units);
}

float ThreeWireDevice::velocity(int units) {
    const auto now = pros::millis();
    const auto current = encoder_->get_value();
    const auto elapsed = now - encoder_previous_time_;
    const float dps = elapsed == 0 ? 0.0f : (current - encoder_previous_) * 1000.0f / elapsed;
    encoder_previous_ = current;
    encoder_previous_time_ = now;
    return units == 24 ? dps * 60.0f / 360.0f : dps;
}

bool ThreeWireDevice::found_object() {
    return ultrasonic_->get_value() > 0;
}

float ThreeWireDevice::distance(int units) {
    const float mm = static_cast<float>(ultrasonic_->get_value()) / 10.0f;
    return inches_from_mm(mm, units);
}

void ThreeWireDevice::set(bool state) {
    if (kind_ == LedIndicator) {
        digital_out_->set_value(state);
    } else {
        digital_out_->set_value(state);
    }
}

void ThreeWireDevice::set_state(float value) {
    pwm_value_ = value;
    pwm_->set_value(static_cast<std::int8_t>(value * 127.0f / 100.0f));
}

void ThreeWireDevice::on() {
    digital_out_->set_value(true);
}

void ThreeWireDevice::off() {
    digital_out_->set_value(false);
}

int ThreeWireDevice::set_colors(const std::uint32_t* colors, std::size_t count, std::size_t offset) {
    if (led_buffer_.size() < offset + count) {
        led_buffer_.resize(offset + count, 0);
    }
    for (std::size_t i = 0; i < count; ++i) {
        led_buffer_[offset + i] = colors[i];
    }
    return pros::c::adi_led_set(led_, led_buffer_.data(), led_buffer_.size());
}

int ThreeWireDevice::clear() {
    std::fill(led_buffer_.begin(), led_buffer_.end(), 0);
    if (led_buffer_.empty()) {
        return 1;
    }
    return pros::c::adi_led_clear_all(led_, led_buffer_.data(), led_buffer_.size());
}

int ThreeWireDevice::watch_value(int event) {
    switch (event) {
        case 0:
        case 5:
        case 9:
        case 10:
            return analog_->get_value();
        case 1:
        case 2:
        case 3:
        case 4:
            return digital_in_->get_value() != 0;
        case 6:
            return potentiometer_->get_value();
        case 7:
        case 8:
            return is_object_detected();
        default:
            return 0;
    }
}

void ThreeWireDevice::watch(int event, Callback callback, void* user_data) {
    callback_tasks_.emplace_back(new pros::Task([this, event, callback, user_data]() {
        int previous = watch_value(event);
        while (true) {
            pros::delay(10);
            const int current = watch_value(event);
            const bool changed = current != previous;
            const bool should_call =
                (event == 1 || event == 3 || event == 7) ? changed && current :
                (event == 2 || event == 4 || event == 8) ? changed && !current :
                changed;
            if (should_call && callback != nullptr) {
                callback(user_data);
            }
            previous = current;
        }
    }, "VEX 3-wire callback"));
}
