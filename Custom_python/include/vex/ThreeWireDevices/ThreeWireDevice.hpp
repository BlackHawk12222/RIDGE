#ifndef THREE_WIRE_DEVICE_HPP
#define THREE_WIRE_DEVICE_HPP

#include "pros/adi.hpp"
#include "pros/rtos.hpp"

#include <memory>
#include <vector>

class ThreeWireDevice {
public:
    using Callback = void (*)(void*);

    enum Kind {
        AnalogIn,
        DigitalIn,
        DigitalOut,
        Bumper,
        Encoder,
        Gyro,
        LedIndicator,
        Light,
        Limit,
        Line,
        ObjectSensor,
        Potentiometer,
        Pwm,
        RangeFinder,
        Accelerometer,
        AddressableLed
    };

    ThreeWireDevice(Kind kind, int port, int option = 0);
    virtual ~ThreeWireDevice();

    int value(int units = 44);
    float pwm_value() const;
    bool pressing();
    float acceleration();
    float brightness();
    int reflectivity();
    bool is_object_detected();
    void set_threshold(int threshold);
    float angle(int units = 29);
    float heading(int units = 29);
    float rotation(int units = 29);
    void set_heading(float value, int units = 29);
    void set_rotation(float value, int units = 29);
    void calibrate();
    void set_position(float value, int units = 29);
    void reset_position();
    float position(int units = 29);
    float velocity(int units = 24);
    bool found_object();
    float distance(int units = 40);
    void set(bool state);
    void set_state(float value);
    void on();
    void off();
    int set_colors(const std::uint32_t* colors, std::size_t count, std::size_t offset);
    int clear();
    void watch(int event, Callback callback, void* user_data);

private:
    int watch_value(int event);

    Kind kind_;
    int port_;
    int option_;
    float gyro_heading_offset_ = 0.0f;
    float gyro_rotation_offset_ = 0.0f;
    int object_threshold_ = 50;
    float pwm_value_ = 0.0f;
    float encoder_position_offset_ = 0.0f;
    std::int32_t encoder_previous_ = 0;
    std::uint32_t encoder_previous_time_ = 0;
    pros::adi::AnalogIn* analog_ = nullptr;
    pros::adi::DigitalIn* digital_in_ = nullptr;
    pros::adi::DigitalOut* digital_out_ = nullptr;
    pros::adi::Motor* pwm_ = nullptr;
    pros::adi::Encoder* encoder_ = nullptr;
    pros::adi::Gyro* gyro_ = nullptr;
    pros::c::adi_led_t led_ = 0;
    std::vector<std::uint32_t> led_buffer_;
    std::vector<std::unique_ptr<pros::Task>> callback_tasks_;
    pros::adi::Potentiometer* potentiometer_ = nullptr;
    pros::adi::Ultrasonic* ultrasonic_ = nullptr;
};

#endif
