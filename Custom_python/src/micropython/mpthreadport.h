#ifndef MICROPY_INCLUDED_MPCUSTOM_MPTHREADPORT_H
#define MICROPY_INCLUDED_MPCUSTOM_MPTHREADPORT_H

#include "pros/rtos.h"

#ifdef __cplusplus
typedef pros::mutex_t mp_thread_mutex_t;
#else
typedef mutex_t mp_thread_mutex_t;
#endif

struct _mp_state_thread_t;

void mp_thread_port_init(void);
void mp_thread_gc_others(void);

#endif
