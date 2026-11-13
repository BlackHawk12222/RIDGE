#ifndef VEX_H
#define VEX_H

#ifdef __cplusplus
extern "C"{  // Wrapper to make sure it is defined using C standerds and not C++
#endif

#include "py/runtime.h"
#include "py/obj.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef void (*c_callback_t)(void* user_data);  // Used for callbacks.
extern const mp_obj_fun_builtin_fixed_t mp_vex_callback_executor_obj;

enum {
    VEX_TIME_UNITS_SECONDS = 37,
    VEX_TIME_UNITS_MSEC = 38,
};

enum {
    VEX_COMPETITION_DRIVER_CONTROL = 0,
    VEX_COMPETITION_AUTONOMOUS = 1,
    VEX_COMPETITION_PRE_AUTON = 2,
    VEX_COMPETITION_DISABLED = 3,
};

void vex_competition_callback(int event);
bool vex_competition_is_autonomous(void);
bool vex_competition_is_driver_control(void);
bool vex_competition_is_enabled(void);
bool vex_competition_is_disabled(void);
bool vex_competition_is_competition_switch(void);
bool vex_competition_is_field_control(void);

/* DEFINE class Motor for C */
void* motor_create(int port, int gears, bool reversed);
void motor_destroy(void* motor_ptr);
int motor_get_port(void* motor_ptr);

void motor_spin(void* motor_ptr, int direction, float velocity, int units);
void motor_spin_for(void* motor_ptr, int direction, int angle, int units, float velocity, int units_v, bool wait);
void motor_spin_to_position(void* motor_ptr, int rotation, int units, float velocity, int units_v, bool wait);
void motor_stop(void* motor_ptr, int mode);
void motor_set_position(void* motor_ptr, float position, int units);
void motor_set_velocity(void* motor_ptr, float velocity, int units);
void motor_set_stopping(void* motor_ptr, int mode);
void motor_set_max_torque(void* motor_ptr, float value, int units);
void motor_set_timeout(void* motor_ptr, float value, int units);
void vex_motor_set_reversed(void* motor_ptr, bool value);
void motor_reset_position(void* motor_ptr);
bool motor_is_done(void* motor_ptr);
bool motor_is_spinning(void* motor_ptr);
float motor_position(void* motor_ptr, int units);
float motor_velocity(void* motor_ptr, int units);
float motor_current(void* motor_ptr, int units);
float motor_power(void* motor_ptr, int units);
float motor_torque(void* motor_ptr, int units);
float motor_efficiency(void* motor_ptr, int units);
int motor_temperature(void* motor_ptr, int units);
int motor_get_timeout(void* motor_ptr);
bool motor_installed(void* motor_ptr);


/*DEFINE class MotorGroup for C*/
void* motor_group_create(void** motor_ptrs, int count);
void motor_group_destroy(void* motor_ptr);

void motor_group_spin(void* motor_ptr, int direction, float velocity, int units);
void motor_group_spin_for(void* motor_ptr, int direction, int angle, int units, float velocity, int units_v, bool wait);
void motor_group_spin_to_position(void* motor_ptr, int rotation, int units, float velocity, int units_v, bool wait);
void motor_group_stop(void* motor_ptr, int mode);
void motor_group_set_position(void* motor_ptr, float position, int units);
void motor_group_set_velocity(void* motor_ptr, float velocity, int units);
void motor_group_set_stopping(void* motor_ptr, int mode);
void motor_group_set_max_torque(void* motor_ptr, float value, int units);
void motor_group_set_timeout(void* motor_ptr, double value, int units);
void motor_group_set_reversed(void* motor_ptr, bool value);
void motor_group_reset_position(void* motor_ptr);
bool motor_group_is_done(void* motor_ptr);
bool motor_group_is_spinning(void* motor_ptr);
float motor_group_position(void* motor_ptr, int units);
float motor_group_velocity(void* motor_ptr, int units);
float motor_group_current(void* motor_ptr, int units);
float motor_group_power(void* motor_ptr, int units);
float motor_group_torque(void* motor_ptr, int units);
float motor_group_efficiency(void* motor_ptr, int units);
int motor_group_temperature(void* motor_ptr, int units);
int motor_group_get_timeout(void* motor_ptr);
int motor_group_count(void* motor_ptr);

/* DEFINE class Drivetrain for C */
void* drivetrain_create(void* left_ptr, bool left_is_group,
    void* right_ptr, bool right_is_group,
    float wheel_travel, float track_width, float wheel_base,
    int units, float external_gear_ratio);

void drivetrain_destroy(void* drivetrain_ptr);
void drivetrain_drive(void* drivetrain_ptr, int direction, float velocity, int units);
void drivetrain_drive_for(void* drivetrain_ptr, int direction, float distance, int units,
                          float velocity, int velocity_units, bool wait);
void drivetrain_turn(void* drivetrain_ptr, int direction, float velocity, int units);
void drivetrain_turn_for(void* drivetrain_ptr, int direction, float angle, int units,
                         float velocity, int velocity_units, bool wait);
void drivetrain_turn_to_heading(void* drivetrain_ptr, float heading, int units,
                                float velocity, int velocity_units, bool wait);
void drivetrain_turn_to_rotation(void* drivetrain_ptr, float rotation, int units,
                                 float velocity, int velocity_units, bool wait);
void drivetrain_stop(void* drivetrain_ptr, int mode);
void drivetrain_set_drive_velocity(void* drivetrain_ptr, float velocity, int units);
void drivetrain_set_turn_velocity(void* drivetrain_ptr, float velocity, int units);
void drivetrain_set_stopping(void* drivetrain_ptr, int mode);
void drivetrain_set_timeout(void* drivetrain_ptr, float value, int units);
void drivetrain_set_heading(void* drivetrain_ptr, float heading, int units);
void drivetrain_set_rotation(void* drivetrain_ptr, float rotation, int units);
bool drivetrain_is_done(void* drivetrain_ptr);
bool drivetrain_is_moving(void* drivetrain_ptr);
float drivetrain_heading(void* drivetrain_ptr, int units);
float drivetrain_rotation(void* drivetrain_ptr, int units);
float drivetrain_velocity(void* drivetrain_ptr, int units);
int drivetrain_get_timeout(void* drivetrain_ptr);

/* DEFINE classes MessageLink and SerialLink for C */
void* message_link_create(int port, const char* name, int type, bool wired);
void message_link_destroy(void* link_ptr);
bool message_link_is_linked(void* link_ptr);
void message_link_send(void* link_ptr, const char* message, int integer_input, float float_input);
int message_link_receive(void* link_ptr, int timeout, char* buffer, size_t capacity,
                         int* integer_output, float* float_output);
void* serial_link_create(int port, const char* name, int type, bool wired);
void serial_link_destroy(void* link_ptr);
bool serial_link_is_linked(void* link_ptr);
void serial_link_send(void* link_ptr, const char* message, int integer_input, float float_input);
int serial_link_receive(void* link_ptr, int timeout, char* buffer, size_t capacity,
                        int* integer_output, float* float_output);

/*DEFINE class Timer for C*/
void* vex_timer_create();
void vex_timer_destroy(void* timer_ptr);
void vex_timer_clear(void* timer_ptr);
int vex_timer_time(void* timer_ptr, int units);
void vex_timer_event(void* timer_ptr, mp_obj_t callback_data, int delay_ms);
mp_obj_t mp_vex_callback_executor(mp_obj_t callback_data);

/* VEX global functions */
bool vex_logic_wait(double duration, int units);
void vex_logic_yield_ms(uint32_t ms);

/* DEFINE class Thread for C */
void* vex_thread_create(c_callback_t callback, void* user_data);
void vex_thread_destroy(void* thread_ptr);
void vex_thread_stop(void* thread_ptr);
bool vex_thread_is_stopped(void* thread_ptr);

/*DEFINE class Brain for c*/
void* brain_create();
void brain_destroy(void* brain_ptr);

void* brain_get_screen(void* brain_ptr);
void* brain_get_sdcard(void* brain_ptr);
void* brain_get_battery(void* brain_ptr);

float brain_battery_voltage(void* battery_ptr, int units);
float brain_battery_current(void* battery_ptr, int units);
int brain_battery_capacity(void* battery_ptr);
int brain_battery_temperature(void* battery_ptr, int units);

void brain_screen_print(void* screen_ptr, const char* text, const char* sep, int precision);
void brain_screen_set_cursor(void* screen_ptr, int row, int col);
void brain_screen_next_row(void* screen_ptr);
void brain_screen_clear_row(void* screen_ptr, int row, int color); // Default color is BLACK.
int brain_screen_row(void* screen_ptr);
int brain_screen_column(void* screen_ptr);
void brain_screen_print_at(void* screen_ptr, const char* text, int x, int y, const char* sep, int precision, bool opaque);
int brain_screen_get_string_width(void* screen_ptr, const char* string);
int brain_screen_get_string_height(void* screen_ptr, const char* string);
void brain_screen_clear_line(void* screen_ptr, int line);
void brain_screen_new_line(void* screen_ptr);
void brain_screen_clear_screen(void* screen_ptr, int color); // Default color is BLACK.
void brain_screen_set_font(void* screen_ptr, int font);
void brain_screen_set_pen_width(void* screen_ptr, int value);
void brain_screen_set_pen_color(void* screen_ptr, int color);
void brain_screen_set_fill_color(void* screen_ptr, int color);
void brain_screen_set_origin(void* screen_ptr, int x, int y);
void brain_screen_draw_pixel(void* screen_ptr, int x, int y);
void brain_screen_draw_line(void* screen_ptr, int start_x, int start_y, int end_x, int end_y);
void brain_screen_draw_rectangle(void* screen_ptr, int x, int y, int width, int height, int color);
void brain_screen_draw_circle(void* screen_ptr, int x, int y, int radius, int color);
void brain_screen_render(void* screen_ptr);
// Red error popup on the brain screen (see Screen::show_error).
void vex_show_error_popup(const char* text);
void brain_screen_set_clip_region(void* screen_ptr, int x, int y, int width, int height);
bool brain_screen_pressing(void* screen_ptr);
int brain_screen_x_position(void* screen_ptr);
int brain_screen_y_position(void* screen_ptr);


void brain_screen_pressed(void* screen_ptr, c_callback_t callback, void* args);
void brain_screen_released(void* screen_ptr, c_callback_t callback, void* args);

char* brain_sdcard_loadfile(void* sdcard_ptr, const char* filename, char* buffer, size_t buffer_size, size_t* result_size);
void brain_sdcard_freefile(char* buffer);
void brain_sdcard_savefile(void* sdcard_ptr, const char* filename, const char* buffer, size_t buffer_size);
void brain_sdcard_appendfile(void* sdcard_ptr, const char* filename, const char* buffer, size_t buffer_size);
void brain_sdcard_draw_image_from_file(void* sdcard_ptr, const char* filename, int x, int y);
bool brain_sdcard_is_inserted(void* sdcard_ptr);
int brain_sdcard_filesize(void* sdcard_ptr, const char* filename);
int brain_sdcard_size(void* sdcard_ptr, const char* filename);
bool brain_sdcard_exists(void* sdcard_ptr, const char* filename);

/*DEFINE class Controller in C*/
void* create_controller(int controller_type);
void destroy_controller(void* controller_ptr);

void* get_controller_screen(void* controller_ptr);

void* get_controller_button_A(void* controller_ptr);
void* get_controller_button_B(void* controller_ptr);
void* get_controller_button_X(void* controller_ptr);
void* get_controller_button_Y(void* controller_ptr);
void* get_controller_button_Up(void* controller_ptr);
void* get_controller_button_Down(void* controller_ptr);
void* get_controller_button_Left(void* controller_ptr);
void* get_controller_button_Right(void* controller_ptr);
void* get_controller_button_R1(void* controller_ptr);
void* get_controller_button_R2(void* controller_ptr);
void* get_controller_button_L1(void* controller_ptr);
void* get_controller_button_L2(void* controller_ptr);

void* get_controller_axis_1(void* controller_ptr);
void* get_controller_axis_2(void* controller_ptr);
void* get_controller_axis_3(void* controller_ptr);
void* get_controller_axis_4(void* controller_ptr);

void controller_screen_print(void* screen_ptr, const char* value, const char* sep, int precision);
void controller_screen_set_cursor(void* screen_ptr,int row, int column);
int controller_screen_column(void* screen_ptr);
int controller_screen_row(void* screen_ptr);
void controller_screen_clear_screen(void* screen_ptr);
void controller_screen_clear_row(void* screen_ptr,int row);
void controller_screen_next_row(void* screen_ptr);

bool controller_button_pressing(void* button_ptr);
void controller_button_pressed(void* button_ptr, c_callback_t func, void *args);
void controller_button_released(void* button_ptr, c_callback_t func, void *args);

int controller_axis_position(void* axis_ptr);
void controller_axis_changed(void* axis_ptr, c_callback_t func, void *args);

void controller_rumble_c(void* controller_ptr, const char* pattern);

/*DEFINE class Inertial for C*/
void* inertial_create(int port);
void inertial_destroy(void* inertial_ptr);
float inertial_heading(void* inertial_ptr, int units);
float inertial_rotation(void* inertial_ptr, int units);
float inertial_gyro_rate(void* inertial_ptr, int axis, int units);
float inertial_orientation(void* inertial_ptr, int axis, int units);
void inertial_set_heading(void* inertial_ptr, float value, int units);
void inertial_set_rotation(void* inertial_ptr, float value, int units);
void inertial_calibrate(void* inertial_ptr);
void inertial_reset_heading(void* inertial_ptr);
void inertial_reset_rotation(void* inertial_ptr);
bool inertial_is_calibrating(void* inertial_ptr);
void inertial_changed(void* inertial_ptr, c_callback_t callback, void* args);
bool inertial_installed(void* inertial_ptr);
float inertial_acceleration(void* inertial_ptr, int axis);
void inertial_collision(void* inertial_ptr, c_callback_t callback, void* args);
void inertial_set_turn_type(void* inertial_ptr, int turn_type);
int inertial_get_turn_type(void* inertial_ptr);

/*DEFINE class Rotation for C*/
void* vex_rotation_create(int port, bool reversed);
void vex_rotation_destroy(void* rotation_ptr);
void vex_rotation_set_position(void* rotation_ptr, float value, int units);
float vex_rotation_angle(void* rotation_ptr, int units);
float vex_rotation_position(void* rotation_ptr, int units);
float vex_rotation_velocity(void* rotation_ptr, int units);
void vex_rotation_reset_position(void* rotation_ptr);
void vex_rotation_set_reversed(void* rotation_ptr, bool value);
void vex_rotation_changed(void* rotation_ptr, c_callback_t callback, void* args);
bool vex_rotation_installed(void* rotation_ptr);

/*DEFINE class Optical for C*/
void* optical_create(int port);
void optical_destroy(void* optical_ptr);
void optical_set_light(void* optical_ptr, bool state);
void optical_set_light_power(void* optical_ptr, int percent);
bool optical_is_near_object(void* optical_ptr);
int optical_color(void* optical_ptr);
float optical_brightness(void* optical_ptr);
float optical_hue(void* optical_ptr);
void optical_object_detected(void* optical_ptr, c_callback_t callback, void* args);
void optical_object_lost(void* optical_ptr, c_callback_t callback, void* args);

/*DEFINE class Distance for C*/
void* distance_create(int port);
void distance_destroy(void* distance_ptr);
float distance_object_distance(void* distance_ptr, int units);
float distance_object_velocity(void* distance_ptr);
int distance_object_size(void* distance_ptr);
bool distance_is_object_detected(void* distance_ptr);
void distance_changed(void* distance_ptr, c_callback_t callback, void* args);

/*DEFINE class Gps for C*/
void* vex_gps_create(int port);
void vex_gps_set_origin(void* gps_ptr, double x, double y, int units);
void vex_gps_set_location(void* gps_ptr, double x, double y, int distance_units, double heading, int heading_units);
double vex_gps_x_position(void* gps_ptr, int units);
double vex_gps_y_position(void* gps_ptr, int units);
double vex_gps_heading(void* gps_ptr);
double vex_gps_acceleration(void* gps_ptr, int axis);
double vex_gps_gyro_rate(void* gps_ptr, int axis, int units);
double vex_gps_orientation(void* gps_ptr, int axis, int units);
void vex_gps_changed(void* gps_ptr, c_callback_t callback, void* args);

/*DEFINE class DigitalOut for C*/
void* vex_digital_out_create(int port);
void vex_digital_out_destroy(void* output_ptr);
void vex_digital_out_set(void* output_ptr, bool value);

enum vex_three_wire_kind {
    VEX_THREE_WIRE_ANALOG_IN,
    VEX_THREE_WIRE_DIGITAL_IN,
    VEX_THREE_WIRE_DIGITAL_OUT,
    VEX_THREE_WIRE_BUMPER,
    VEX_THREE_WIRE_ENCODER,
    VEX_THREE_WIRE_GYRO,
    VEX_THREE_WIRE_LED_INDICATOR,
    VEX_THREE_WIRE_LIGHT,
    VEX_THREE_WIRE_LIMIT,
    VEX_THREE_WIRE_LINE,
    VEX_THREE_WIRE_OBJECT_SENSOR,
    VEX_THREE_WIRE_POTENTIOMETER,
    VEX_THREE_WIRE_PWM,
    VEX_THREE_WIRE_RANGE_FINDER,
    VEX_THREE_WIRE_ACCELEROMETER,
    VEX_THREE_WIRE_ADDRESSABLE_LED
};

void* vex_three_wire_create(int kind, int port, int option);
void vex_three_wire_destroy(void* device_ptr);
int vex_three_wire_value(void* device_ptr, int units);
float vex_three_wire_pwm_value(void* device_ptr);
bool vex_three_wire_pressing(void* device_ptr);
float vex_three_wire_acceleration(void* device_ptr);
float vex_three_wire_brightness(void* device_ptr);
int vex_three_wire_reflectivity(void* device_ptr);
bool vex_three_wire_is_object_detected(void* device_ptr);
void vex_three_wire_set_threshold(void* device_ptr, int threshold);
float vex_three_wire_angle(void* device_ptr, int units);
float vex_three_wire_heading(void* device_ptr, int units);
float vex_three_wire_rotation(void* device_ptr, int units);
void vex_three_wire_set_heading(void* device_ptr, float value, int units);
void vex_three_wire_set_rotation(void* device_ptr, float value, int units);
void vex_three_wire_calibrate(void* device_ptr);
void vex_three_wire_set_position(void* device_ptr, float value, int units);
void vex_three_wire_reset_position(void* device_ptr);
float vex_three_wire_position(void* device_ptr, int units);
float vex_three_wire_velocity(void* device_ptr, int units);
bool vex_three_wire_found_object(void* device_ptr);
float vex_three_wire_distance(void* device_ptr, int units);
void vex_three_wire_set(void* device_ptr, bool state);
void vex_three_wire_set_state(void* device_ptr, float value);
void vex_three_wire_on(void* device_ptr);
void vex_three_wire_off(void* device_ptr);
int vex_three_wire_set_colors(void* device_ptr, const uint32_t* colors, size_t count, size_t offset);
int vex_three_wire_clear(void* device_ptr);
void vex_three_wire_watch(void* device_ptr, int event, c_callback_t callback, void* user_data);


#ifdef __cplusplus
}
#endif

#endif // VEX_H
