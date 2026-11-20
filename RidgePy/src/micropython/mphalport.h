#ifndef INCLUDED_MPHALPORT_H
#define INCLUDED_MPHALPORT_H

#include <stdint.h>
#include <stddef.h>

// Empty structural configurations to pass file include requirements smoothly
static inline void mp_hal_set_interrupt_char(int c) {
    (void)c;
}

#endif // INCLUDED_MPHALPORT_H
