#include "vex/vex.h"
#include "vex/motors.hpp"
#include "vex/Drivetrain.hpp"
#include "vex/VexLink.hpp"
#include "vex/brain.hpp"
#include "vex/timer.hpp"
#include "vex/Thread.hpp"
#include "vex/controller.hpp"
#include "vex/Inerital.hpp"
#include "vex/Sensors/Rotation.hpp"
#include "vex/Sensors/Optical.hpp"
#include "vex/Sensors/Distance.hpp"
#include "vex/Sensors/Gps.hpp"
#include "vex/ThreeWireDevices/DigitalOut.hpp"
#include "vex/ThreeWireDevices/ThreeWireDevice.hpp"
#include "vex/ThreeWireDevices/Accelerometer.hpp"
#include "vex/ThreeWireDevices/AddressableLed.hpp"
#include "vex/ThreeWireDevices/AnalogIn.hpp"
#include "vex/ThreeWireDevices/Bumper.hpp"
#include "vex/ThreeWireDevices/DigitalIn.hpp"
#include "vex/ThreeWireDevices/Encoder.hpp"
#include "vex/ThreeWireDevices/Gyro.hpp"
#include "vex/ThreeWireDevices/Led.hpp"
#include "vex/ThreeWireDevices/Light.hpp"
#include "vex/ThreeWireDevices/Limit.hpp"
#include "vex/ThreeWireDevices/Line.hpp"
#include "vex/ThreeWireDevices/ObjectDetector.hpp"
#include "vex/ThreeWireDevices/Potentiometer.hpp"
#include "vex/ThreeWireDevices/PotentiometerV2.hpp"
#include "vex/ThreeWireDevices/Pwm.hpp"
#include "vex/ThreeWireDevices/Sonar.hpp"

extern "C"{
    #include <cstring>
    #include <vector>
    #include "py/runtime.h"
    #include "py/obj.h"
}

void* vex_thread_create(c_callback_t callback, void* user_data){
    try {
        return new Thread(callback, user_data);
    } catch (const std::bad_alloc&) {
        return nullptr;
    }
}

void vex_thread_destroy(void* thread_ptr){
    delete static_cast<Thread*>(thread_ptr);
}

void vex_thread_stop(void* thread_ptr){
    static_cast<Thread*>(thread_ptr)->stop();
}

bool vex_thread_is_stopped(void* thread_ptr){
    return static_cast<Thread*>(thread_ptr)->is_stopped();
}

/* DEFINE class Motor for C */
void* motor_create(int port, int gears, bool reversed){
    Ports Port = static_cast<Ports>(port);
    GearSetting Gear = static_cast<GearSetting>(gears);
    return static_cast<void*>(new Motor(Port, Gear, reversed));
}

void motor_destroy(void* motor_ptr){
    delete static_cast<Motor*>(motor_ptr);
}

int motor_get_port(void* motor_ptr){
    return static_cast<Motor*>(motor_ptr)->port();
}

void motor_spin(void* motor_ptr, int direction, float velocity, int units){
    static_cast<Motor*>(motor_ptr)->spin(static_cast<Direction>(direction), velocity, static_cast<VelocityUnits>(units));
}

void motor_spin_for(void* motor_ptr, int direction, int angle, int units, float velocity, int units_v, bool wait){
    static_cast<Motor*>(motor_ptr)->spin_for(static_cast<Direction>(direction), angle,static_cast<AngleUnits>(units), velocity, static_cast<VelocityUnits>(units_v), wait);
}

void motor_spin_to_position(void* motor_ptr, int rotation, int units, float velocity, int units_v, bool wait){
    static_cast<Motor*>(motor_ptr)->spin_to_position(rotation, static_cast<AngleUnits>(units), velocity, static_cast<VelocityUnits>(units_v), wait);
}

void motor_stop(void* motor_ptr, int mode){
    static_cast<Motor*>(motor_ptr)->stop(static_cast<BrakeMode>(mode));
}

void motor_set_position(void* motor_ptr, float position, int units){
    static_cast<Motor*>(motor_ptr)->set_position(position, static_cast<AngleUnits>(units));
}

void motor_set_velocity(void* motor_ptr, float velocity, int units){
    static_cast<Motor*>(motor_ptr)->set_velocity(velocity, static_cast<VelocityUnits>(units));
}

void motor_set_stopping(void* motor_ptr, int mode){
    static_cast<Motor*>(motor_ptr)->set_stopping(static_cast<BrakeMode>(mode));
}

void motor_set_max_torque(void* motor_ptr, float value, int units){
    static_cast<Motor*>(motor_ptr)->set_max_torque(value, static_cast<TorqueUnits>(units));
}

void motor_set_timeout(void* motor_ptr, float value, int units){
    static_cast<Motor*>(motor_ptr)->set_timeout(value, static_cast<TimeUnits>(units));
}

void motor_set_reversed(void* motor_ptr, bool value){
    static_cast<Motor*>(motor_ptr)->set_reversed(value);
}

void motor_reset_position(void* motor_ptr){
    static_cast<Motor*>(motor_ptr)->reset_position();
}

bool motor_is_done(void* motor_ptr){
    return static_cast<Motor*>(motor_ptr)->is_done();
}

bool motor_is_spinning(void* motor_ptr){
    return static_cast<Motor*>(motor_ptr)->is_spinning();
}

float motor_position(void* motor_ptr, int units){
    return static_cast<Motor*>(motor_ptr)->position(static_cast<AngleUnits>(units));
}

float motor_velocity(void* motor_ptr, int units){
    return static_cast<Motor*>(motor_ptr)->velocity(static_cast<VelocityUnits>(units));
}

float motor_current(void* motor_ptr, int units){
    return static_cast<Motor*>(motor_ptr)->current(static_cast<CurrentUnits>(units));
}

float motor_power(void* motor_ptr, int units){
    return static_cast<Motor*>(motor_ptr)->power(static_cast<PowerUnits>(units));
}

float motor_torque(void* motor_ptr, int units){
    return static_cast<Motor*>(motor_ptr)->torque(static_cast<TorqueUnits>(units));
}

float motor_efficiency(void* motor_ptr, int units){
    return static_cast<Motor*>(motor_ptr)->efficiency(static_cast<EfficiencyUnits>(units));
}

int motor_temperature(void* motor_ptr, int units){
    return static_cast<Motor*>(motor_ptr)->temperature(static_cast<TemperatureUnits>(units));
}

int motor_get_timeout(void* motor_ptr){
    return static_cast<Motor*>(motor_ptr)->get_timeout();
}

/*DEFINE class MotorGroup for C*/
void* motor_group_create(void** motor_ptrs, int count) {
    Motor** target_motors = reinterpret_cast<Motor**>(motor_ptrs);
    
    return static_cast<void*>(new MotorGroup(target_motors, count));
}

void motor_group_destroy(void* motor_ptr){
    delete static_cast<MotorGroup*>(motor_ptr);
}

void motor_group_spin(void* motor_ptr, int direction, float velocity, int units){
    static_cast<MotorGroup*>(motor_ptr)->spin(static_cast<Direction>(direction), velocity, static_cast<VelocityUnits>(units));
}

void motor_group_spin_for(void* motor_ptr, int direction, int angle, int units, float velocity, int units_v, bool wait){
    static_cast<MotorGroup*>(motor_ptr)->spin_for(static_cast<Direction>(direction), angle,static_cast<AngleUnits>(units), velocity, static_cast<VelocityUnits>(units_v), wait);
}

void motor_group_spin_to_position(void* motor_ptr, int rotation, int units, float velocity, int units_v, bool wait){
    static_cast<MotorGroup*>(motor_ptr)->spin_to_position(rotation, static_cast<AngleUnits>(units), velocity, static_cast<VelocityUnits>(units_v), wait);
}

void motor_group_stop(void* motor_ptr, int mode){
    static_cast<MotorGroup*>(motor_ptr)->stop(static_cast<BrakeMode>(mode));
}

void motor_group_set_position(void* motor_ptr, float position, int units){
    static_cast<MotorGroup*>(motor_ptr)->set_position(position, static_cast<AngleUnits>(units));
}

void motor_group_set_velocity(void* motor_ptr, float velocity, int units){
    static_cast<MotorGroup*>(motor_ptr)->set_velocity(velocity, static_cast<VelocityUnits>(units));
}

void motor_group_set_stopping(void* motor_ptr, int mode){
    static_cast<MotorGroup*>(motor_ptr)->set_stopping(static_cast<BrakeMode>(mode));
}

void motor_group_set_max_torque(void* motor_ptr, float value, int units){
    static_cast<MotorGroup*>(motor_ptr)->set_max_torque(value, static_cast<TorqueUnits>(units));
}

void motor_group_set_timeout(void* motor_ptr, double value, int units){
    static_cast<MotorGroup*>(motor_ptr)->set_timeout(value, static_cast<TimeUnits>(units));
}

void motor_group_set_reversed(void* motor_ptr, bool value){
    static_cast<MotorGroup*>(motor_ptr)->set_reversed(value);
}

void motor_group_reset_position(void* motor_ptr){
    static_cast<MotorGroup*>(motor_ptr)->reset_position();
}

bool motor_group_is_done(void* motor_ptr){
    return static_cast<MotorGroup*>(motor_ptr)->is_done();
}

bool motor_group_is_spinning(void* motor_ptr){
    return static_cast<MotorGroup*>(motor_ptr)->is_spinning();
}

float motor_group_position(void* motor_ptr, int units){
    return static_cast<MotorGroup*>(motor_ptr)->position(static_cast<AngleUnits>(units));
}

float motor_group_velocity(void* motor_ptr, int units){
    return static_cast<MotorGroup*>(motor_ptr)->velocity(static_cast<VelocityUnits>(units));
}

float motor_group_current(void* motor_ptr, int units){
    return static_cast<MotorGroup*>(motor_ptr)->current(static_cast<CurrentUnits>(units));
}

float motor_group_power(void* motor_ptr, int units){
    return static_cast<MotorGroup*>(motor_ptr)->power(static_cast<PowerUnits>(units));
}

float motor_group_torque(void* motor_ptr, int units){
    return static_cast<MotorGroup*>(motor_ptr)->torque(static_cast<TorqueUnits>(units));
}

float motor_group_efficiency(void* motor_ptr, int units){
    return static_cast<MotorGroup*>(motor_ptr)->efficiency(static_cast<EfficiencyUnits>(units));
}

int motor_group_temperature(void* motor_ptr, int units){
    return static_cast<MotorGroup*>(motor_ptr)->temperature(static_cast<TemperatureUnits>(units));
}

int motor_group_get_timeout(void* motor_ptr){
    return static_cast<MotorGroup*>(motor_ptr)->get_timeout();
}

int motor_group_count(void* motor_ptr){
    return static_cast<MotorGroup*>(motor_ptr)->count();
}

/* DEFINE class Drivetrain for C */
void* drivetrain_create(const int8_t* left_ports, int left_count,
                        const int8_t* right_ports, int right_count,
                        float wheel_travel, float track_width, float wheel_base,
                        int units, float external_gear_ratio){
    std::vector<std::int8_t> left(left_ports, left_ports + left_count);
    std::vector<std::int8_t> right(right_ports, right_ports + right_count);
    return new Drivetrain(left, right, wheel_travel, track_width, wheel_base,
                          static_cast<DistanceUnits>(units), external_gear_ratio);
}

void drivetrain_destroy(void* drivetrain_ptr){
    delete static_cast<Drivetrain*>(drivetrain_ptr);
}

void drivetrain_drive(void* drivetrain_ptr, int direction, float velocity, int units){
    static_cast<Drivetrain*>(drivetrain_ptr)->drive(
        static_cast<Direction>(direction), velocity, static_cast<VelocityUnits>(units));
}

void drivetrain_drive_for(void* drivetrain_ptr, int direction, float distance, int units,
                          float velocity, int velocity_units, bool wait){
    static_cast<Drivetrain*>(drivetrain_ptr)->drive_for(
        static_cast<Direction>(direction), distance, static_cast<DistanceUnits>(units),
        velocity, static_cast<VelocityUnits>(velocity_units), wait);
}

void drivetrain_turn(void* drivetrain_ptr, int direction, float velocity, int units){
    static_cast<Drivetrain*>(drivetrain_ptr)->turn(
        static_cast<TurnType>(direction), velocity, static_cast<VelocityUnits>(units));
}

void drivetrain_turn_for(void* drivetrain_ptr, int direction, float angle, int units,
                         float velocity, int velocity_units, bool wait){
    static_cast<Drivetrain*>(drivetrain_ptr)->turn_for(
        static_cast<TurnType>(direction), angle, static_cast<AngleUnits>(units),
        velocity, static_cast<VelocityUnits>(velocity_units), wait);
}

void drivetrain_turn_to_heading(void* drivetrain_ptr, float heading, int units,
                                float velocity, int velocity_units, bool wait){
    static_cast<Drivetrain*>(drivetrain_ptr)->turn_to_heading(
        heading, static_cast<AngleUnits>(units), velocity,
        static_cast<VelocityUnits>(velocity_units), wait);
}

void drivetrain_turn_to_rotation(void* drivetrain_ptr, float rotation, int units,
                                 float velocity, int velocity_units, bool wait){
    static_cast<Drivetrain*>(drivetrain_ptr)->turn_to_rotation(
        rotation, static_cast<AngleUnits>(units), velocity,
        static_cast<VelocityUnits>(velocity_units), wait);
}

void drivetrain_stop(void* drivetrain_ptr, int mode){
    static_cast<Drivetrain*>(drivetrain_ptr)->stop(static_cast<BrakeMode>(mode));
}

void drivetrain_set_drive_velocity(void* drivetrain_ptr, float velocity, int units){
    static_cast<Drivetrain*>(drivetrain_ptr)->set_drive_velocity(
        velocity, static_cast<VelocityUnits>(units));
}

void drivetrain_set_turn_velocity(void* drivetrain_ptr, float velocity, int units){
    static_cast<Drivetrain*>(drivetrain_ptr)->set_turn_velocity(
        velocity, static_cast<VelocityUnits>(units));
}

void drivetrain_set_stopping(void* drivetrain_ptr, int mode){
    static_cast<Drivetrain*>(drivetrain_ptr)->set_stopping(static_cast<BrakeMode>(mode));
}

void drivetrain_set_timeout(void* drivetrain_ptr, float value, int units){
    static_cast<Drivetrain*>(drivetrain_ptr)->set_timeout(value, static_cast<TimeUnits>(units));
}

void drivetrain_set_heading(void* drivetrain_ptr, float heading, int units){
    static_cast<Drivetrain*>(drivetrain_ptr)->set_heading(heading, static_cast<AngleUnits>(units));
}

void drivetrain_set_rotation(void* drivetrain_ptr, float rotation, int units){
    static_cast<Drivetrain*>(drivetrain_ptr)->set_rotation(rotation, static_cast<AngleUnits>(units));
}

bool drivetrain_is_done(void* drivetrain_ptr){
    return static_cast<Drivetrain*>(drivetrain_ptr)->is_done();
}

bool drivetrain_is_moving(void* drivetrain_ptr){
    return static_cast<Drivetrain*>(drivetrain_ptr)->is_moving();
}

float drivetrain_heading(void* drivetrain_ptr, int units){
    return static_cast<Drivetrain*>(drivetrain_ptr)->heading(static_cast<AngleUnits>(units));
}

float drivetrain_rotation(void* drivetrain_ptr, int units){
    return static_cast<Drivetrain*>(drivetrain_ptr)->rotation(static_cast<AngleUnits>(units));
}

float drivetrain_velocity(void* drivetrain_ptr, int units){
    return static_cast<Drivetrain*>(drivetrain_ptr)->velocity(static_cast<VelocityUnits>(units));
}

int drivetrain_get_timeout(void* drivetrain_ptr){
    return static_cast<Drivetrain*>(drivetrain_ptr)->get_timeout();
}

/* DEFINE classes MessageLink and SerialLink for C */
void* message_link_create(int port, const char* name, int type, bool wired){
    return new MessageLink(static_cast<Ports>(port), name, static_cast<VexLinkType>(type), wired);
}

void message_link_destroy(void* link_ptr){
    delete static_cast<MessageLink*>(link_ptr);
}

bool message_link_is_linked(void* link_ptr){
    return static_cast<MessageLink*>(link_ptr)->is_linked();
}

void message_link_send(void* link_ptr, const char* message, int integer_input, float float_input){
    static_cast<MessageLink*>(link_ptr)->send(message, integer_input, float_input);
}

int message_link_receive(void* link_ptr, int timeout, char* buffer, size_t capacity,
                         int* integer_output, float* float_output){
    MessageLink* link = static_cast<MessageLink*>(link_ptr);
    char* message = link->receive(timeout);
    if (message == nullptr) {
        return -1;
    }
    const size_t length = std::strlen(message);
    if (capacity <= length) {
        return -2;
    }
    std::memcpy(buffer, message, length + 1);
    if (integer_output != nullptr) {
        *integer_output = link->last_integer();
    }
    if (float_output != nullptr) {
        *float_output = link->last_float();
    }
    return static_cast<int>(length);
}

void* serial_link_create(int port, const char* name, int type, bool wired){
    return new SerialLink(static_cast<Ports>(port), name, static_cast<VexLinkType>(type), wired);
}

void serial_link_destroy(void* link_ptr){
    delete static_cast<SerialLink*>(link_ptr);
}

bool serial_link_is_linked(void* link_ptr){
    return static_cast<SerialLink*>(link_ptr)->is_linked();
}

void serial_link_send(void* link_ptr, const char* message, int integer_input, float float_input){
    static_cast<SerialLink*>(link_ptr)->send(message, integer_input, float_input);
}

int serial_link_receive(void* link_ptr, int timeout, char* buffer, size_t capacity,
                        int* integer_output, float* float_output){
    SerialLink* link = static_cast<SerialLink*>(link_ptr);
    char* message = link->receive(timeout);
    if (message == nullptr) {
        return -1;
    }
    const size_t length = std::strlen(message);
    if (capacity <= length) {
        return -2;
    }
    std::memcpy(buffer, message, length + 1);
    if (integer_output != nullptr) {
        *integer_output = link->last_integer();
    }
    if (float_output != nullptr) {
        *float_output = link->last_float();
    }
    return static_cast<int>(length);
}

/*DEFINE class Timer for C*/
void* vex_timer_create(){
    return static_cast<void*>(new Timer());
}

void vex_timer_destroy(void* timer_ptr){
    delete static_cast<Timer*>(timer_ptr);
}

void vex_timer_clear(void* timer_ptr){
    static_cast<Timer*>(timer_ptr)->clear();
}

int vex_timer_time(void* timer_ptr, int units){
    return static_cast<Timer*>(timer_ptr)->time(static_cast<TimeUnits>(units));
}
#include <map>

extern "C" {
    #include "py/runtime.h"
}


struct TaskParams {
    int delay;
    mp_obj_t callback_data;
};

void timer_rtos_task(void* params) {
    TaskParams* p = static_cast<TaskParams*>(params);
    
    pros::delay(p->delay); 

    // Inject the Python callback back into MicroPython's execution engine safely
    while (!mp_sched_schedule(MP_OBJ_FROM_PTR(mp_vex_callback_executor), p->callback_data)) {
        pros::delay(1);
    }

    delete p;
}

void vex_timer_event(void* timer_ptr, mp_obj_t callback_data, int delay_ms) {
    TaskParams* params = new TaskParams{delay_ms, callback_data};

    // Spin up an asynchronous background task on your microcontroller
    pros::Task task(timer_rtos_task, params, "PythonTimerTask");
}

/*DEFINE class Brain for C*/
void* brain_create(){
    return static_cast<void*>(new Brain());
}

void brain_destroy(void* brain_ptr){
    delete static_cast<Brain*>(brain_ptr);
}

void* brain_get_screen(void* brain_ptr){
    return &(static_cast<Brain*>(brain_ptr) -> screen);
}

void* brain_get_sdcard(void* brain_ptr){
    return &(static_cast<Brain*>(brain_ptr) -> sdcard);
}

void* brain_get_battery(void* brain_ptr){
    return &(static_cast<Brain*>(brain_ptr) -> battery);
}

float brain_battery_voltage(void* battery_ptr, int units){
    return static_cast<Battery*>(battery_ptr) -> voltage(static_cast<VoltageUnits>(units));
}

float brain_battery_current(void* battery_ptr, int units){
    return static_cast<Battery*>(battery_ptr) -> current(static_cast<CurrentUnits>(units));
}

int brain_battery_capacity(void* battery_ptr){
    return static_cast<Battery*>(battery_ptr) -> capacity();
}
    
int brain_battery_temperature(void* battery_ptr, int units){
    return static_cast<Battery*>(battery_ptr) -> temperature(static_cast<TemperatureUnits>(units));
}

void brain_screen_print(void* screen_ptr, const char* text, const char* sep, int precision){
    static_cast<Screen*>(screen_ptr) -> print(text, sep, precision);
}

void brain_screen_set_cursor(void* screen_ptr, int row, int col){
    static_cast<Screen*>(screen_ptr) -> set_cursor(row, col);
}

void brain_screen_next_row(void* screen_ptr){
    static_cast<Screen*>(screen_ptr) -> next_row();
}

void brain_screen_clear_row(void* screen_ptr, int row, int color){
    static_cast<Screen*>(screen_ptr) -> clear_row(row, static_cast<Color>(color));
}

int brain_screen_row(void* screen_ptr){
    return static_cast<Screen*>(screen_ptr) -> row();
}

int brain_screen_column(void* screen_ptr){
    return static_cast<Screen*>(screen_ptr) -> column();
}

void brain_screen_print_at(void* screen_ptr, const char* text, int x, int y, const char* sep, int precision, bool opaque){
    static_cast<Screen*>(screen_ptr) -> print_at(text, x, y, sep, precision, opaque);
}

int brain_screen_get_string_width(void* screen_ptr, const char* string){
    return static_cast<Screen*>(screen_ptr) -> get_string_width(string);
}

int brain_screen_get_string_height(void* screen_ptr, const char* string){
    return static_cast<Screen*>(screen_ptr) -> get_string_height(string);
}

void brain_screen_clear_line(void* screen_ptr, int line){
    static_cast<Screen*>(screen_ptr) -> clear_line(line);
}

void brain_screen_new_line(void* screen_ptr){
    static_cast<Screen*>(screen_ptr) -> new_line();
}

void brain_screen_clear_screen(void* screen_ptr, int color){
    static_cast<Screen*>(screen_ptr) -> clear_screen(static_cast<Color>(color));
}

void brain_screen_set_font(void* screen_ptr, int font){
    static_cast<Screen*>(screen_ptr) -> set_font(static_cast<Font>(font));
}

void brain_screen_set_pen_width(void* screen_ptr, int value){
    static_cast<Screen*>(screen_ptr) -> set_pen_width(value);
}

void brain_screen_set_pen_color(void* screen_ptr, int color){
    static_cast<Screen*>(screen_ptr) -> set_pen_color(static_cast<Color>(color));
}

void brain_screen_set_fill_color(void* screen_ptr, int color){
    static_cast<Screen*>(screen_ptr) -> set_fill_color(static_cast<Color>(color));
}

void brain_screen_set_origin(void* screen_ptr, int x, int y){
    static_cast<Screen*>(screen_ptr) -> set_origin(x, y);
}

void brain_screen_draw_pixel(void* screen_ptr, int x, int y){
    static_cast<Screen*>(screen_ptr) -> draw_pixel(x, y);
}

void brain_screen_draw_line(void* screen_ptr, int start_x, int start_y, int end_x, int end_y){
    static_cast<Screen*>(screen_ptr) -> draw_line(start_x, start_y, end_x, end_y);
}

void brain_screen_draw_rectangle(void* screen_ptr, int x, int y, int width, int height, int color){
    static_cast<Screen*>(screen_ptr) -> draw_rectangle(x, y, width, height, static_cast<Color>(color));
}

void brain_screen_draw_circle(void* screen_ptr, int x, int y, int radius, int color){
    static_cast<Screen*>(screen_ptr) -> draw_circle(x, y, radius, static_cast<Color>(color));
}

void brain_screen_render(void* screen_ptr){
    static_cast<Screen*>(screen_ptr) -> render();
}

void brain_screen_set_clip_region(void* screen_ptr, int x, int y, int width, int height){
    static_cast<Screen*>(screen_ptr) -> set_clip_region(x, y, width, height);
}

bool brain_screen_pressing(void* screen_ptr){
    return static_cast<Screen*>(screen_ptr) -> pressing();
}

int brain_screen_x_position(void* screen_ptr){
    return static_cast<Screen*>(screen_ptr) -> x_position();
}

int brain_screen_y_position(void* screen_ptr){
    return static_cast<Screen*>(screen_ptr) -> y_position();
}

typedef void (*c_callback_t)(void* user_data);

void brain_screen_pressed(void* screen_ptr, c_callback_t callback, void* args) {
    if (!callback || !screen_ptr) return;

    // By explicitly capturing the raw pointers by value, they are safely copied 
    // into the internal storage of the lambda object managed by the PROS Event system.
    static_cast<Screen*>(screen_ptr)->pressed([callback, args]() {
        if (callback) {
            callback(args);
        }
    });
}

void brain_screen_released(void* screen_ptr, c_callback_t callback, void* args) {
    if (!callback || !screen_ptr) return;

    // Explicit capture prevents stack lifetime issues
    static_cast<Screen*>(screen_ptr)->released([callback, args]() {
        if (callback) {
            callback(args);
        }
    });
}


char* brain_sdcard_loadfile(void* sdcard_ptr, const char* filename, char* buffer, size_t buffer_size, size_t* result_size){
    return static_cast<SdCard*>(sdcard_ptr) -> loadfile(filename, buffer, buffer_size, result_size);
}

void brain_sdcard_freefile(char* buffer){
    delete[] buffer;
}

void brain_sdcard_savefile(void* sdcard_ptr, const char* filename, const char* buffer, size_t buffer_size){
    static_cast<SdCard*>(sdcard_ptr) -> savefile(filename, buffer, buffer_size);
}

void brain_sdcard_appendfile(void* sdcard_ptr, const char* filename, const char* buffer, size_t buffer_size){
    static_cast<SdCard*>(sdcard_ptr) -> appendfile(filename, buffer, buffer_size);
}

void brain_sdcard_draw_image_from_file(void* sdcard_ptr, const char* filename, int x, int y){
    static_cast<SdCard*>(sdcard_ptr) -> draw_image_from_file(filename, x, y);
}

bool brain_sdcard_is_inserted(void* sdcard_ptr){
    return static_cast<SdCard*>(sdcard_ptr) -> is_inserted();
}

int brain_sdcard_filesize(void* sdcard_ptr, const char* filename){
    return static_cast<SdCard*>(sdcard_ptr) -> filesize(filename);
}

int brain_sdcard_size(void* sdcard_ptr, const char* filename){
    return static_cast<SdCard*>(sdcard_ptr) -> size(filename);
}

bool brain_sdcard_exists(void* sdcard_ptr, const char* filename){
    return static_cast<SdCard*>(sdcard_ptr) -> exists(filename);
}

/*DEFINE class Controller in C*/
void* create_controller(int controller_type){
    return static_cast<void*>(new Controller(static_cast<ControllerType>(controller_type)));
}

void destroy_controller(void* controller_ptr){
    delete static_cast<Controller*>(controller_ptr);
}

void* get_controller_screen(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->screen);
}

void* get_controller_button_A(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->buttonA);
}

void* get_controller_button_B(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->buttonB);
}

void* get_controller_button_X(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->buttonX);
}

void* get_controller_button_Y(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->buttonY);
}

void* get_controller_button_Up(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->buttonUp);
}

void* get_controller_button_Down(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->buttonDown);
}

void* get_controller_button_Left(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->buttonLeft);
}

void* get_controller_button_Right(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->buttonRight);
}

void* get_controller_button_R1(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->buttonR1);
}

void* get_controller_button_R2(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->buttonR2);
}

void* get_controller_button_L1(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->buttonL1);
}

void* get_controller_button_L2(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->buttonL2);
}

void* get_controller_axis_1(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->axis1);
}

void* get_controller_axis_2(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->axis2);
}

void* get_controller_axis_3(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->axis3);
}

void* get_controller_axis_4(void* controller_ptr){
    return static_cast<void*>(static_cast<Controller*>(controller_ptr)->axis4);
}

void controller_screen_print(void* screen_ptr, const char* value, const char* sep, int precision){
    static_cast<ControllerScreen*>(screen_ptr)->print(value, sep, precision);
}

void controller_screen_set_cursor(void* screen_ptr,int row, int column){
    static_cast<ControllerScreen*>(screen_ptr)->set_cursor(row, column);
}

int controller_screen_column(void* screen_ptr){
    return static_cast<ControllerScreen*>(screen_ptr)->column();
}

int controller_screen_row(void* screen_ptr){
    return static_cast<ControllerScreen*>(screen_ptr)->row();
}

void controller_screen_clear_screen(void* screen_ptr){
    static_cast<ControllerScreen*>(screen_ptr)->clear_screen();
}

void controller_screen_clear_row(void* screen_ptr,int row){
    static_cast<ControllerScreen*>(screen_ptr)->clear_row(row);
}

void controller_screen_next_row(void* screen_ptr){
    static_cast<ControllerScreen*>(screen_ptr)->next_row();
}

bool controller_button_pressing(void* button_ptr){
    return static_cast<ControllerButton*>(button_ptr)->pressing();
}

void controller_button_pressed(void* button_ptr, c_callback_t func, void *args){
    static_cast<ControllerButton*>(button_ptr)->pressed([func, args]() {
        if (func) {
            func(args);
        }
    });
}

void controller_button_released(void* button_ptr, c_callback_t func, void *args){
    static_cast<ControllerButton*>(button_ptr)->released([func, args]() {
        if (func) {
            func(args);
        }
    });
}

int controller_axis_position(void* axis_ptr){
    return static_cast<ControllerAxis*>(axis_ptr)->position();
}

void controller_axis_changed(void* axis_ptr, c_callback_t func, void *args){
    static_cast<ControllerAxis*>(axis_ptr)->changed([func, args]() {
        if (func) {
            func(args);
        }
    });
}

void controller_rumble_c(void* controller_ptr, const char* pattern){
    static_cast<Controller*>(controller_ptr)->rumble(pattern);
}

/*DEFINE class Inertial for C*/
void* inertial_create(int port){
    return static_cast<void*>(new Inertial(static_cast<Ports>(port)));
}

void inertial_destroy(void* inertial_ptr){
    delete static_cast<Inertial*>(inertial_ptr);
}

float inertial_heading(void* inertial_ptr, int units){
    return static_cast<Inertial*>(inertial_ptr)->heading(static_cast<AngleUnits>(units));
}

float inertial_rotation(void* inertial_ptr, int units){
    return static_cast<Inertial*>(inertial_ptr)->rotation(static_cast<AngleUnits>(units));
}

float inertial_gyro_rate(void* inertial_ptr, int axis, int units){
    return static_cast<Inertial*>(inertial_ptr)->gyro_rate(static_cast<AxisType>(axis), static_cast<VelocityUnits>(units));
}

float inertial_orientation(void* inertial_ptr, int axis, int units){
    return static_cast<Inertial*>(inertial_ptr)->orientation(static_cast<AxisType>(axis), static_cast<AngleUnits>(units));
}

void inertial_set_heading(void* inertial_ptr, float value, int units){
    static_cast<Inertial*>(inertial_ptr)->set_heading(value, static_cast<AngleUnits>(units));
}

void inertial_set_rotation(void* inertial_ptr, float value, int units){
    static_cast<Inertial*>(inertial_ptr)->set_rotation(value, static_cast<AngleUnits>(units));
}

void inertial_calibrate(void* inertial_ptr){
    static_cast<Inertial*>(inertial_ptr)->calibrate();
}

void inertial_reset_heading(void* inertial_ptr){
    static_cast<Inertial*>(inertial_ptr)->reset_heading();
}

void inertial_reset_rotation(void* inertial_ptr){
    static_cast<Inertial*>(inertial_ptr)->reset_rotation();
}

bool inertial_is_calibrating(void* inertial_ptr){
    return static_cast<Inertial*>(inertial_ptr)->is_calibrating();
}

void inertial_changed(void* inertial_ptr, c_callback_t callback, void* args){
    static_cast<Inertial*>(inertial_ptr)->changed([callback, args](){
        if (callback){
            callback(args);
        }
    });
}

bool inertial_installed(void* inertial_ptr){
    return static_cast<Inertial*>(inertial_ptr)->installed();
}

float inertial_acceleration(void* inertial_ptr, int axis){
    return static_cast<Inertial*>(inertial_ptr)->acceleration(static_cast<AxisType>(axis));
}

void inertial_collision(void* inertial_ptr, c_callback_t callback, void* args){
    static_cast<Inertial*>(inertial_ptr)->collision([callback, args](){
        if (callback){
            callback(args);
        }
    });
}

void inertial_set_turn_type(void* inertial_ptr, int turn_type){
    static_cast<Inertial*>(inertial_ptr)->set_turn_type(static_cast<TurnType>(turn_type));
}

int inertial_get_turn_type(void* inertial_ptr){
    return static_cast<int>(static_cast<Inertial*>(inertial_ptr)->get_turn_type());
}

/*DEFINE class Rotation for C*/
void* vex_rotation_create(int port, bool reversed){
    return static_cast<void*>(new Rotation(static_cast<Ports>(port), reversed));
}

void vex_rotation_destroy(void* rotation_ptr){
    delete static_cast<Rotation*>(rotation_ptr);
}

void vex_rotation_set_position(void* rotation_ptr, float value, int units){
    static_cast<Rotation*>(rotation_ptr)->set_position(value, static_cast<AngleUnits>(units));
}

float vex_rotation_angle(void* rotation_ptr, int units){
    return static_cast<Rotation*>(rotation_ptr)->angle(static_cast<AngleUnits>(units));
}

float vex_rotation_position(void* rotation_ptr, int units){
    return static_cast<Rotation*>(rotation_ptr)->position(static_cast<AngleUnits>(units));
}

float vex_rotation_velocity(void* rotation_ptr, int units){
    return static_cast<Rotation*>(rotation_ptr)->velocity(static_cast<VelocityUnits>(units));
}

void vex_rotation_reset_position(void* rotation_ptr){
    static_cast<Rotation*>(rotation_ptr)->reset_position();
}

void vex_rotation_set_reversed(void* rotation_ptr, bool value){
    static_cast<Rotation*>(rotation_ptr)->set_reversed(value);
}

void vex_rotation_changed(void* rotation_ptr, c_callback_t callback, void* args){
    static_cast<Rotation*>(rotation_ptr)->changed([callback, args](){
        if (callback){
            callback(args);
        }
    });
}

/*DEFINE class Optical for C*/
void* optical_create(int port){
    return static_cast<void*>(new Optical(static_cast<Ports>(port)));
}

void optical_destroy(void* optical_ptr){
    delete static_cast<Optical*>(optical_ptr);
}

void optical_set_light(void* optical_ptr, bool state){
    static_cast<Optical*>(optical_ptr)->set_light(state);
}

void optical_set_light_power(void* optical_ptr, int percent){
    static_cast<Optical*>(optical_ptr)->set_light_power(percent);
}

bool optical_is_near_object(void* optical_ptr){
    return static_cast<Optical*>(optical_ptr)->is_near_object();
}

int optical_color(void* optical_ptr){
    return static_cast<int>(static_cast<Optical*>(optical_ptr)->color());
}

float optical_brightness(void* optical_ptr){
    return static_cast<Optical*>(optical_ptr)->brightness();
}

float optical_hue(void* optical_ptr){
    return static_cast<Optical*>(optical_ptr)->hue();
}

void optical_object_detected(void* optical_ptr, c_callback_t callback, void* args){
    static_cast<Optical*>(optical_ptr)->object_detected([callback, args](){
        if (callback){
            callback(args);
        }
    });
}

void optical_object_lost(void* optical_ptr, c_callback_t callback, void* args){
    static_cast<Optical*>(optical_ptr)->object_lost([callback, args](){
        if (callback){
            callback(args);
        }
    });
}

/*DEFINE class Distance for C*/
void* distance_create(int port){
    return static_cast<void*>(new Distance(static_cast<Ports>(port)));
}

void distance_destroy(void* distance_ptr){
    delete static_cast<Distance*>(distance_ptr);
}

float distance_object_distance(void* distance_ptr, int units){
    return static_cast<Distance*>(distance_ptr)->object_distance(static_cast<DistanceUnits>(units));
}

float distance_object_velocity(void* distance_ptr){
    return static_cast<Distance*>(distance_ptr)->object_velocity();
}

int distance_object_size(void* distance_ptr){
    return static_cast<int>(static_cast<Distance*>(distance_ptr)->object_size());
}

bool distance_is_object_detected(void* distance_ptr){
    return static_cast<Distance*>(distance_ptr)->is_object_detected();
}

void distance_changed(void* distance_ptr, c_callback_t callback, void* args){
    static_cast<Distance*>(distance_ptr)->changed([callback, args](){
        if (callback){
            callback(args);
        }
    });
}

/*DEFINE class Gps for C*/
void* vex_gps_create(int port){
    return static_cast<void*>(new Gps(port));
}

void vex_gps_set_origin(void* gps_ptr, double x, double y, int units){
    static_cast<Gps*>(gps_ptr)->set_origin(x, y, static_cast<DistanceUnits>(units));
}

void vex_gps_set_location(void* gps_ptr, double x, double y, int distance_units, double heading, int heading_units){
    static_cast<Gps*>(gps_ptr)->set_location(x, y, static_cast<DistanceUnits>(distance_units),
                                             heading, static_cast<AngleUnits>(heading_units));
}

double vex_gps_x_position(void* gps_ptr, int units){
    return static_cast<Gps*>(gps_ptr)->x_position(static_cast<DistanceUnits>(units));
}

double vex_gps_y_position(void* gps_ptr, int units){
    return static_cast<Gps*>(gps_ptr)->y_position(static_cast<DistanceUnits>(units));
}

double vex_gps_heading(void* gps_ptr){
    return static_cast<Gps*>(gps_ptr)->heading();
}

double vex_gps_acceleration(void* gps_ptr, int axis){
    return static_cast<Gps*>(gps_ptr)->acceleration(static_cast<AxisType>(axis));
}

double vex_gps_gyro_rate(void* gps_ptr, int axis, int units){
    return static_cast<Gps*>(gps_ptr)->gyro_rate(static_cast<AxisType>(axis), static_cast<VelocityUnits>(units));
}

double vex_gps_orientation(void* gps_ptr, int axis, int units){
    return static_cast<Gps*>(gps_ptr)->orientation(static_cast<AxisType>(axis), static_cast<AngleUnits>(units));
}

void vex_gps_changed(void* gps_ptr, c_callback_t callback, void* args){
    static_cast<Gps*>(gps_ptr)->changed([callback, args](){
        if (callback){
            callback(args);
        }
    });
}

/*DEFINE class DigitalOut for C*/
void* vex_digital_out_create(int port){
    return static_cast<void*>(new DigitalOut(port));
}

void vex_digital_out_destroy(void* output_ptr){
    delete static_cast<DigitalOut*>(output_ptr);
}

void vex_digital_out_set(void* output_ptr, bool value){
    static_cast<DigitalOut*>(output_ptr)->set(value);
}

void* vex_three_wire_create(int kind, int port, int option){
    switch (kind){
        case VEX_THREE_WIRE_ANALOG_IN: return new AnalogIn(port);
        case VEX_THREE_WIRE_DIGITAL_IN: return new DigitalIn(port);
        case VEX_THREE_WIRE_DIGITAL_OUT: return new DigitalOut(port);
        case VEX_THREE_WIRE_BUMPER: return new Bumper(port);
        case VEX_THREE_WIRE_ENCODER: return new Encoder(port);
        case VEX_THREE_WIRE_GYRO: return new Gyro(port);
        case VEX_THREE_WIRE_LED_INDICATOR: return new Led(port);
        case VEX_THREE_WIRE_LIGHT: return new Light(port);
        case VEX_THREE_WIRE_LIMIT: return new Limit(port);
        case VEX_THREE_WIRE_LINE: return new Line(port);
        case VEX_THREE_WIRE_OBJECT_SENSOR: return new ObjectDetector(port, option != 0);
        case VEX_THREE_WIRE_POTENTIOMETER: return option ? static_cast<void*>(new PotentiometerV2(port)) : static_cast<void*>(new Potentiometer(port));
        case VEX_THREE_WIRE_PWM: return new Pwm(port);
        case VEX_THREE_WIRE_RANGE_FINDER: return new Sonar(port);
        case VEX_THREE_WIRE_ACCELEROMETER: return new Accelerometer(port);
        case VEX_THREE_WIRE_ADDRESSABLE_LED: return new AddressableLed(port);
        default: return nullptr;
    }
}

void vex_three_wire_destroy(void* device_ptr){
    delete static_cast<ThreeWireDevice*>(device_ptr);
}

int vex_three_wire_value(void* device_ptr, int units){
    return static_cast<ThreeWireDevice*>(device_ptr)->value(units);
}

float vex_three_wire_pwm_value(void* device_ptr){
    return static_cast<ThreeWireDevice*>(device_ptr)->pwm_value();
}

bool vex_three_wire_pressing(void* device_ptr){
    return static_cast<ThreeWireDevice*>(device_ptr)->pressing();
}

float vex_three_wire_acceleration(void* device_ptr){
    return static_cast<ThreeWireDevice*>(device_ptr)->acceleration();
}

float vex_three_wire_brightness(void* device_ptr){
    return static_cast<ThreeWireDevice*>(device_ptr)->brightness();
}

int vex_three_wire_reflectivity(void* device_ptr){
    return static_cast<ThreeWireDevice*>(device_ptr)->reflectivity();
}

bool vex_three_wire_is_object_detected(void* device_ptr){
    return static_cast<ThreeWireDevice*>(device_ptr)->is_object_detected();
}

void vex_three_wire_set_threshold(void* device_ptr, int threshold){
    static_cast<ThreeWireDevice*>(device_ptr)->set_threshold(threshold);
}

float vex_three_wire_angle(void* device_ptr, int units){
    return static_cast<ThreeWireDevice*>(device_ptr)->angle(units);
}

float vex_three_wire_heading(void* device_ptr, int units){
    return static_cast<ThreeWireDevice*>(device_ptr)->heading(units);
}

float vex_three_wire_rotation(void* device_ptr, int units){
    return static_cast<ThreeWireDevice*>(device_ptr)->rotation(units);
}

void vex_three_wire_set_heading(void* device_ptr, float value, int units){
    static_cast<ThreeWireDevice*>(device_ptr)->set_heading(value, units);
}

void vex_three_wire_set_rotation(void* device_ptr, float value, int units){
    static_cast<ThreeWireDevice*>(device_ptr)->set_rotation(value, units);
}

void vex_three_wire_calibrate(void* device_ptr){
    static_cast<ThreeWireDevice*>(device_ptr)->calibrate();
}

void vex_three_wire_set_position(void* device_ptr, float value, int units){
    static_cast<ThreeWireDevice*>(device_ptr)->set_position(value, units);
}

void vex_three_wire_reset_position(void* device_ptr){
    static_cast<ThreeWireDevice*>(device_ptr)->reset_position();
}

float vex_three_wire_position(void* device_ptr, int units){
    return static_cast<ThreeWireDevice*>(device_ptr)->position(units);
}

float vex_three_wire_velocity(void* device_ptr, int units){
    return static_cast<ThreeWireDevice*>(device_ptr)->velocity(units);
}

bool vex_three_wire_found_object(void* device_ptr){
    return static_cast<ThreeWireDevice*>(device_ptr)->found_object();
}

float vex_three_wire_distance(void* device_ptr, int units){
    return static_cast<ThreeWireDevice*>(device_ptr)->distance(units);
}

void vex_three_wire_set(void* device_ptr, bool state){
    static_cast<ThreeWireDevice*>(device_ptr)->set(state);
}

void vex_three_wire_set_state(void* device_ptr, float value){
    static_cast<ThreeWireDevice*>(device_ptr)->set_state(value);
}

void vex_three_wire_on(void* device_ptr){
    static_cast<ThreeWireDevice*>(device_ptr)->on();
}

void vex_three_wire_off(void* device_ptr){
    static_cast<ThreeWireDevice*>(device_ptr)->off();
}

int vex_three_wire_set_colors(void* device_ptr, const uint32_t* colors, size_t count, size_t offset){
    return static_cast<ThreeWireDevice*>(device_ptr)->set_colors(colors, count, offset);
}

int vex_three_wire_clear(void* device_ptr){
    return static_cast<ThreeWireDevice*>(device_ptr)->clear();
}

void vex_three_wire_watch(void* device_ptr, int event, c_callback_t callback, void* user_data){
    static_cast<ThreeWireDevice*>(device_ptr)->watch(event, callback, user_data);
}
