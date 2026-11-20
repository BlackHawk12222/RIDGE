#ifndef MICROPY_INCLUDED_MPCUSTOM_MPTHREADPORT_H
#define MICROPY_INCLUDED_MPCUSTOM_MPTHREADPORT_H

#include "pros/rtos.h"

#ifdef __cplusplus
typedef pros::mutex_t mp_thread_mutex_t;
#else
typedef mutex_t mp_thread_mutex_t;
#endif

struct _mp_state_thread_t;

// Every Python task (the script task, _thread threads and vex.Thread) runs at
// this one priority, so FreeRTOS time-slices them like VEXcode's equal threads.
// A higher script priority (it was 12) let a busy main loop starve every thread.
#define MP_PYTHON_TASK_PRIORITY (TASK_PRIORITY_DEFAULT)

#ifdef __cplusplus
extern "C" {
#endif
void mp_thread_port_init(void);
void mp_thread_gc_others(void);
// Called by the VM every MICROPY_PY_THREAD_GIL_VM_DIVISOR branches.
void mp_thread_gil_bounce(void);
#ifdef __cplusplus
}
#endif
#define MP_THREAD_GIL_BOUNCE() mp_thread_gil_bounce()

#endif
