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
void motor_set_timeout(void* motor_ptr, double value, int units);
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

/*DEFINE class Timer for C*/
void* timer_create();
void timer_destroy(void* timer_ptr);
void timer_clear(void* timer_ptr);
int timer_time(void* timer_ptr, int units);
void timer_event(void* timer_ptr, mp_obj_t callback_func, int delay_ms);

#ifdef __cplusplus
}
#endif

#endif // VEX_H
