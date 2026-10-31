#ifndef VEX_H
#define VEX_H

#ifdef __cplusplus
extern "C"{
#endif

#include "py/runtime.h"
#include "py/obj.h"

/* DEFINE class Motor for C */
void* motor_create(int port, int gears, bool reversed);
void motor_destroy(void* motor_ptr);

void motor_spin(void* motor_ptr, int direction, float velocity, int units);
void motor_spin_for(void* motor_ptr, int direction, int angle, int units, float velocity, int units_v, bool wait);
void motor_spin_to_position(void* motor_ptr, int rotation, int units, float velocity, int units_v, bool wait);
void motor_stop(void* motor_ptr, int mode);
void motor_set_position(void* motor_ptr, float position, int units);
void motor_set_velocity(void* motor_ptr, float velocity, int units);
void motor_set_stopping(void* motor_ptr, int mode);
void motor_set_max_torque(void* motor_ptr, float value, int units);
void motor_set_timeout(void* motor_ptr, float value, int units);
void motor_set_reversed(void* motor_ptr, bool value);
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

/*DEFINE class Timer for C*/
void* vex_timer_create();
void vex_timer_destroy(void* timer_ptr);
void vex_timer_clear(void* timer_ptr);
int vex_timer_time(void* timer_ptr, int units);
void vex_timer_event(void* timer_ptr, mp_obj_t callback_func, int delay_ms);

/*DEFINE class Brain for c*/
void* brain_create();
void brain_destroy(void* brain_ptr);

float brain_battery_voltage(void* brain_ptr, int units);
float brain_battery_current(void* brain_ptr, int units);
int brain_battery_capacity(void* brain_ptr);
int brain_battery_temperature(void* brain_ptr, int units);

void brain_screen_print(void* brain_ptr, const char* text, const char* sep, int precision);
void brain_screen_set_cursor(void* brain_ptr, int row, int col);
void brain_screen_next_row(void* brain_ptr);
void brain_screen_clear_row(void* brain_ptr, int row=-1, int color); // Default color is BLACK.
int brain_screen_row(void* brain_ptr);
int brain_screen_column(void* brain_ptr);
void brain_screen_print_at(void* brain_ptr, const char* text, int x, int y, const char* sep, int precision, bool opaque);
int brain_screen_get_string_width(void* brain_ptr, const char* string);
int brain_screen_get_string_height(void* brain_ptr, const char* string);
void brain_screen_clear_line(void* brain_ptr, int line);
void brain_screen_new_line(void* brain_ptr);
void brain_screen_clear_screen(void* brain_ptr, int color); // Default color is BLACK.
void brain_screen_set_font(void* brain_ptr, int font);
void brain_screen_set_pen_width(void* brain_ptr, int value);
void brain_screen_set_pen_color(void* brain_ptr, int color);
void brain_screen_set_fill_color(void* brain_ptr, int color);
void brain_screen_set_origin(void* brain_ptr, int x, int y);
void brain_screen_draw_pixel(void* brain_ptr, int x, int y);
void brain_screen_draw_line(void* brain_ptr, int start_x, int start_y, int end_x, int end_y);
void brain_screen_draw_rectangle(void* brain_ptr, int x, int y, int width, int height, int color);
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



#ifdef __cplusplus
}
#endif

#endif // VEX_H
