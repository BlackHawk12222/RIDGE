// Included by extmod/modtime.c. time.sleep / sleep_ms / sleep_us must give up the
// GIL while waiting (like vex wait()), otherwise every other thread stops.
// mp_hal_delay_ms itself stays a plain delay: it is also called from non-Python
// PROS tasks that don't hold the GIL.
void mp_port_sleep_ms(mp_uint_t ms);
void mp_port_sleep_us(mp_uint_t us);
#define mp_hal_delay_ms mp_port_sleep_ms
#define mp_hal_delay_us mp_port_sleep_us

// time.time(): whole seconds since the program started (the brain has no clock).
static mp_obj_t mp_time_time_get(void) {
    extern uint64_t micros(void);
    return mp_obj_new_int_from_ull(micros() / 1000000ULL);
}
