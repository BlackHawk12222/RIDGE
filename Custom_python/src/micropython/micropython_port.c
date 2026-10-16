#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include "py/builtin.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/nlr.h"
#include "py/stackctrl.h"
#include "mpconfigport.h"

#include "shared/runtime/gchelper.h"

extern void mp_port_status(const char *message);

void init_micropython(void *heap, size_t heap_size) {
    mp_port_status("MP: GC init");
    gc_init(heap, (char *)heap + heap_size);
    mp_port_status("MP: GC ready");
    mp_port_status("MP: stack init");
    mp_stack_ctrl_init();
    mp_port_status("MP: core init");
    mp_init();
    mp_port_status("MP: core ready");
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
unsigned int mp_hal_stdout_tx_strn(const char *str, size_t len) {
    printf("%.*s", (int)len, str);
    return len;
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
