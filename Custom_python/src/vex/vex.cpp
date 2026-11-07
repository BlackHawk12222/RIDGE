#include "vex/vex.h"
#include "vex/motors.hpp"
#include "vex/brain.hpp"
#include "vex/timer.hpp"
#include "vex/controller.hpp"
#include "vex/Inerital.hpp"
#include "vex/Rotation.hpp"

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


char* brain_sdcard_loadfile(void* brain_ptr, const char* filename, char* buffer){
    return static_cast<Brain*>(brain_ptr) -> sdcard.loadfile(filename, buffer);
}

void brain_sdcard_savefile(void* brain_ptr, const char* filename, char* buffer){
    static_cast<Brain*>(brain_ptr) -> sdcard.savefile(filename, buffer);
}

void brain_sdcard_appendfile(void* brain_ptr, const char* filename, char* buffer){
    static_cast<Brain*>(brain_ptr) -> sdcard.appendfile(filename, buffer);
}

void brain_sdcard_draw_image_from_file(void* brain_ptr, const char* filename, int x, int y){
    static_cast<Brain*>(brain_ptr) -> sdcard.draw_image_from_file(filename, x, y);
}

bool brain_sdcard_is_inserted(void* brain_ptr){
    return static_cast<Brain*>(brain_ptr) -> sdcard.is_inserted();
}

int brain_sdcard_filesize(void* brain_ptr, const char* filename){
    return static_cast<Brain*>(brain_ptr) -> sdcard.filesize(filename);
}

int brain_sdcard_size(void* brain_ptr, const char* filename){
    return static_cast<Brain*>(brain_ptr) -> sdcard.size(filename);
}

bool brain_sdcard_exists(void* brain_ptr, const char* filename){
    return static_cast<Brain*>(brain_ptr) -> sdcard.exists(filename);
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

void controller_rumble(void* controller_ptr, const char* pattern){
    static_cast<Controller*>(controller_ptr)->rumble(pattern);
}