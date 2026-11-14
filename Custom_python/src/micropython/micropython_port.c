#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include "py/builtin.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/objlist.h"
#include "py/gc.h"
#include "py/nlr.h"
#include "py/stackctrl.h"
#include "py/mpstate.h"
#include "py/mpthread.h"
#include "mpconfigport.h"
#include "pros/rtos.h"

#include "shared/runtime/gchelper.h"

extern void mp_port_status(const char *message);

void init_micropython(void *heap, size_t heap_size) {
    #if MICROPY_PY_THREAD
    mp_thread_port_init();
    #endif
    gc_init(heap, (char *)heap + heap_size);
    mp_stack_ctrl_init();
    mp_init();
#if MICROPY_PY_SYS_PATH
    mp_obj_list_append(mp_sys_path, mp_obj_new_str("/usd", 4));
#endif
}

void gc_collect(void) {
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    gc_collect_end();
}

MP_NORETURN void nlr_jump_fail(void *val) {
    (void)val;
    fputs("MicroPython fatal error: uncaught non-local return\n", stderr);
    abort();
}

void execute_python_string(const char *src) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        // Create a raw runtime string object for the filename instead of a pre-compiled QSTR
        mp_obj_t filename_obj = mp_obj_new_str("main.py", 7);
        qstr source_name = mp_obj_str_get_qstr(filename_obj);

        // Parse and compile raw code strings into execution bytecode tokens
        mp_lexer_t *lex = mp_lexer_new_from_str_len(source_name, src, strlen(src), 0);
        mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, false);
        
        // Execute the compiled script
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // Gracefully catch engine crashes and print them to terminal
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}

// =============================================================================
// CONSOLIDATED MICROPYTHON HARDWARE ABSTRACTION LAYER (HAL) HOOKS
// =============================================================================

// Standard core output stream implementation
// Reference the tracking objects hosted in main.cpp
extern bool capture_traceback_mode;
extern char traceback_buffer[1024];
extern size_t traceback_buf_idx;

// Global helper function to handle exceptions safely inside a native C translation unit
void mp_port_print_last_exception(void) {
    mp_obj_t exc = MP_STATE_THREAD(mp_pending_exception);
    if (exc != MP_OBJ_NULL) {
        // mp_plat_print automatically pipes the stream into your mp_hal_stdout_tx_strn
        mp_obj_print_exception(&mp_plat_print, exc);
    }
}

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "pros/rtos.h"

#define TX_LINE_BUF_SIZE 512

#include <stdio.h>
#include <stddef.h>
#include "pros/rtos.h"

#define TX_LINE_BUF_SIZE 512

static mutex_t stdout_mutex = NULL;

// size_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
//     static char line_buf[TX_LINE_BUF_SIZE];
//     static size_t buf_idx = 0;

//     if (str == NULL || len == 0) {
//         return 0;
//     }

//     if (stdout_mutex == NULL) {
//         stdout_mutex = mutex_create();
//     }
//     if (stdout_mutex != NULL) {
//         mutex_take(stdout_mutex, TIMEOUT_MAX);
//     }

//     for (size_t i = 0; i < len; i++) {
//         line_buf[buf_idx++] = str[i];

//         if (str[i] == '\n' || buf_idx >= TX_LINE_BUF_SIZE) {
//             char flush_copy[TX_LINE_BUF_SIZE];
//             size_t flush_len = buf_idx;
//             memcpy(flush_copy, line_buf, flush_len);
//             buf_idx = 0;

//             if (stdout_mutex != NULL) {
//                 mutex_give(stdout_mutex);
//             }

//             // Release the Python GIL while writing to V5 serial
//             MP_THREAD_GIL_EXIT();
//             fwrite(flush_copy, 1, flush_len, stdout);
//             fflush(stdout);
//             MP_THREAD_GIL_ENTER();

//             if (stdout_mutex != NULL) {
//                 mutex_take(stdout_mutex, TIMEOUT_MAX);
//             }
//         }
//     }

//     if (stdout_mutex != NULL) {
//         mutex_give(stdout_mutex);
//     }

//     return len;
// }

size_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    if (str == NULL || len == 0) {
        return 0;
    }
    size_t written = fwrite(str, 1, len, stdout);
    // stdout is buffered: flush at each line end so print() output and
    // tracebacks reach the terminal right away, even if the program then hangs.
    if (memchr(str, '\n', len) != NULL) {
        fflush(stdout);
    }
    return written;
}

// Concrete implementation mapping to the underlying PROS API delay clock
void mp_hal_delay_ms(unsigned int ms) {
    extern void delay(const uint32_t milliseconds);
    delay((uint32_t)ms);
}

// Concrete implementation mapping to the underlying PROS API runtime clock
unsigned int mp_hal_ticks_ms(void) {
    extern uint32_t millis(void);
    return (unsigned int)millis();
}

// External C hooks pointing straight to PROS's compiled core libraries
void pros_delay(uint32_t ms) {
    extern void delay(const uint32_t milliseconds);
    delay(ms);
}

uint32_t pros_millis(void) {
    extern uint32_t millis(void);
    return millis();
}

// ---- Time HAL used by the time / select / asyncio modules ----
#include "py/mpstate.h"
#include "py/mpthread.h"
#include "py/runtime.h"

extern uint64_t micros(void);

mp_uint_t mp_hal_ticks_us(void) {
    return (mp_uint_t)micros();
}

mp_uint_t mp_hal_ticks_cpu(void) {
    return (mp_uint_t)micros();
}

uint64_t mp_hal_time_ns(void) {
    // No real-time clock: time.time() counts from program start, like VEXcode.
    return micros() * 1000ULL;
}

void mp_hal_delay_us(mp_uint_t us) {
    uint64_t end = micros() + us;
    while (micros() < end) {
    }
}

void mp_port_sleep_ms(mp_uint_t ms) {
    extern void delay(const uint32_t milliseconds);
    MP_THREAD_GIL_EXIT();
    delay(ms);  // delay(0) just yields
    MP_THREAD_GIL_ENTER();
    mp_handle_pending(true);
}

void mp_port_sleep_us(mp_uint_t us) {
    if (us >= 1000) {
        mp_port_sleep_ms(us / 1000);
        us %= 1000;
    }
    mp_hal_delay_us(us);
}

void mp_port_wfe(int timeout_ms) {
    extern void delay(const uint32_t milliseconds);
    MP_THREAD_GIL_EXIT();
    delay(timeout_ms == 0 ? 0 : 1);
    MP_THREAD_GIL_ENTER();
}
