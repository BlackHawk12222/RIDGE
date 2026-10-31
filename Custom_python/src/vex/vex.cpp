#include "vex/vex.h"
#include "main.h"
#include "vex/motors.hpp"
#include "vex/brain.hpp"
#include "vex/timer.hpp"
#include "vex/controller.hpp"

extern "C"{
    #include "py/runtime.h"
    #include "py/obj.h"
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


static mp_obj_t execute_scheduled_callback(mp_obj_t callback_func) {
    mp_call_function_0(callback_func);
    return mp_const_none;
}
struct TaskParams {
    int delay;
    mp_obj_t callback;
};

void timer_rtos_task(void* params) {
    TaskParams* p = static_cast<TaskParams*>(params);
    
    pros::delay(p->delay); 

    // Inject the Python callback back into MicroPython's execution engine safely
    mp_sched_schedule(MP_OBJ_FROM_PTR(execute_scheduled_callback), p->callback);

    delete p;
}

void vex_timer_event(void* timer_ptr, mp_obj_t callback_func, int delay_ms) {
    mp_obj_list_append(mp_sys_path, callback_func); // Simple workaround to pin memory

    // Package parameters for our tracking thread
    TaskParams* params = new TaskParams{delay_ms, callback_func};

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

float brain_battery_voltage(void* brain_ptr, int units){
    return static_cast<Brain*>(brain_ptr) -> battery.voltage(static_cast<VoltageUnits>(units));
}

float brain_battery_current(void* brain_ptr, int units){
    return static_cast<Brain*>(brain_ptr) -> battery.current(static_cast<CurrentUnits>(units));
}

int brain_battery_capacity(void* brain_ptr){
    return static_cast<Brain*>(brain_ptr) -> battery.capacity();
}
    
int brain_battery_temperature(void* brain_ptr, int units){
    return static_cast<Brain*>(brain_ptr) -> battery.temperature(static_cast<TemperatureUnits>(units));
}

void brain_screen_print(void* brain_ptr, const char* text, const char* sep, int precision){
    static_cast<Brain*>(brain_ptr) -> screen.print(text, sep, precision);
}

void brain_screen_set_cursor(void* brain_ptr, int row, int col){
    static_cast<Brain*>(brain_ptr) -> screen.set_cursor(row, col);
}

void brain_screen_next_row(void* brain_ptr){
    static_cast<Brain*>(brain_ptr) -> screen.next_row();
}

void brain_screen_clear_row(void* brain_ptr, int row, int color){
    static_cast<Brain*>(brain_ptr) -> screen.clear_row(row, static_cast<Color>(color));
}

int brain_screen_row(void* brain_ptr){
    static_cast<Brain*>(brain_ptr) -> screen.row();
}

int brain_screen_column(void* brain_ptr){
    return static_cast<Brain*>(brain_ptr) -> screen.column();
}

void brain_screen_print_at(void* brain_ptr, const char* text, int x, int y, const char* sep, int precision, bool opaque){
    static_cast<Brain*>(brain_ptr) -> screen.print_at(text, x, y, sep, precision, opaque);
}

int brain_screen_get_string_width(void* brain_ptr, const char* string){
    return static_cast<Brain*>(brain_ptr) -> screen.get_string_width(string);
}

int brain_screen_get_string_height(void* brain_ptr, const char* string){
    return static_cast<Brain*>(brain_ptr) -> screen.get_string_height(string);
}

void brain_screen_clear_line(void* brain_ptr, int line){
    static_cast<Brain*>(brain_ptr) -> screen.clear_line(line);
}

void brain_screen_new_line(void* brain_ptr){
    static_cast<Brain*>(brain_ptr) -> screen.new_line();
}

void brain_screen_clear_screen(void* brain_ptr, int color){
    static_cast<Brain*>(brain_ptr) -> screen.clear_screen(static_cast<Color>(color));
}

void brain_screen_set_font(void* brain_ptr, int font){
    static_cast<Brain*>(brain_ptr) -> screen.set_font(static_cast<Font>(font));
}

void brain_screen_set_pen_width(void* brain_ptr, int value){
    static_cast<Brain*>(brain_ptr) -> screen.set_pen_width(value);
}

void brain_screen_set_pen_color(void* brain_ptr, int color){
    static_cast<Brain*>(brain_ptr) -> screen.set_pen_color(static_cast<Color>(color));
}

void brain_screen_set_fill_color(void* brain_ptr, int color){
    static_cast<Brain*>(brain_ptr) -> screen.set_fill_color(static_cast<Color>(color));
}

void brain_screen_set_origin(void* brain_ptr, int x, int y){
    static_cast<Brain*>(brain_ptr) -> screen.set_origin(x, y);
}

void brain_screen_draw_pixel(void* brain_ptr, int x, int y){
    static_cast<Brain*>(brain_ptr) -> screen.draw_pixel(x, y);
}

void brain_screen_draw_line(void* brain_ptr, int start_x, int start_y, int end_x, int end_y){
    static_cast<Brain*>(brain_ptr) -> screen.draw_line(start_x, start_y, end_x, end_y);
}

void brain_screen_draw_rectangle(void* brain_ptr, int x, int y, int width, int height, int color){
    static_cast<Brain*>(brain_ptr) -> screen.draw_rectangle(x, y, width, height, static_cast<Color>(color));
}

void brain_screen_draw_circle(void* brain_ptr, int x, int y, int radius, int color);
void brain_screen_render(void* brain_ptr);
void brain_screen_set_clip_region(void* brain_ptr, int x, int y, int width, int height);
bool brain_screen_pressing(void* brain_ptr);
int brain_screen_x_position(void* brain_ptr);
int brain_screen_y_position(void* brain_ptr);

typedef void (*c_callback_t)(void* user_data);
void brain_screen_pressed(void* brain_ptr, c_callback_t callback, void* args);
void brain_screen_released(void* brain_ptr, c_callback_t callback, void* args);

char* brain_sdcard_loadfile(void* brain_ptr, const char* filename, char* buffer = nullptr);
void brain_sdcard_savefile(void* brain_ptr, const char* filename, char* buffer = nullptr);
void brain_sdcard_appendfile(void* brain_ptr, const char* filename, char* buffer);
void brain_sdcard_draw_image_from_file(void* brain_ptr, const char* filename, int x, int y);
bool brain_sdcard_is_inserted(void* brain_ptr);
int brain_sdcard_filesize(void* brain_ptr, const char* filename);
int brain_sdcard_size(void* brain_ptr, const char* filename);
bool brain_sdcard_exists(void* brain_ptr, const char* filename);