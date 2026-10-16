#ifndef INCLUDED_MPCONFIGPORT_H
#define INCLUDED_MPCONFIGPORT_H

#include <stdint.h>
#include <stddef.h>

// Handle stack allocations natively inside the toolchain compiler cross-layers
#define alloca(size) __builtin_alloca(size)

typedef intptr_t mp_int_t;
typedef uintptr_t mp_uint_t;
typedef long mp_off_t;

// Set baseline feature preset level to Everything core
#define MICROPY_CONFIG_ROM_LEVEL        (MICROPY_CONFIG_ROM_LEVEL_EVERYTHING)
#define MICROPY_ALLOC_PATH_MAX (512) // Default is often 128
#define MICROPY_ALLOC_LEXER_CHUNK_SIZE (256) // Speeds up on-device compilation
#define MICROPY_OBJ_REPR (MICROPY_OBJ_REPR_B)

// Force the internal VM runtime structures to look at QSTRs as 16-bit integers
#define MICROPY_QSTR_BYTES_IN_LEN                  (2)
#define MICROPY_QSTR_BYTES_IN_HASH                 (2)

#ifndef MICROPY_QSTR_BIT_LEN
#define MICROPY_QSTR_BIT_LEN                       (16)
#endif

// Performance & Execution Optimizations
#define MICROPY_EMIT_THUMB              (1) // Enable direct Thumb assembly generation for V5
#define MICROPY_EMIT_INLINE_THUMB       (1) // Allows inline assembly inside script functions
#define MICROPY_COMP_CONST              (1) // Enables compile-time constant optimization
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE (1) // Speeds up object dictionary lookups

// Garbage Collection & Core Subsystems
#define MICROPY_ENABLE_GC               (1)
#define MICROPY_STACK_CHECK             (0) // Turn off runtime overhead stack boundary checking
#define MICROPY_HELPER_REPL             (0) // Strip out terminal interactive components
#define MICROPY_HELPER_LEXER_UNIX       (0) 
#define MICROPY_ENABLE_EXTERNAL_IMPORT  (1) // Required to load custom motor/sensor user modules
#define MICROPY_ENABLE_COMPILER (1)
#define MICROPY_FLOAT_IMPL              (MICROPY_FLOAT_IMPL_FLOAT)

// These features require port implementations that are not present yet.
#define MICROPY_PY_BUILTINS_INPUT       (0)
#define MICROPY_PY_IO                   (0)
#define MICROPY_PY_SYS_STDFILES         (0)
#define MICROPY_PY_SYS_STDIO_BUFFER     (0)
#define MICROPY_PY_UCTYPES              (0)

// If you don't need a REPL, you can safely strip standard stream histories
#define MICROPY_REPL_EMACS_KEYS          (0)
#define MICROPY_REPL_AUTO_INDENT         (0)


/* Allow baseline system tables to align tracking structs */
#define MICROPY_PY_SYS_ARGV                        (0)
#define MICROPY_PY_SYS_MUTABLE                     (1) // SG matching wrapper tracking

// Force using simple string configurations to remove hex brace error routes
#define MICROPY_PY_BUILTINS_STR_OP_MODULO         (1)
#define MICROPY_PY_BUILTINS_STR_FORMAT            (0)

// Disable weak references to fix mp_weakref_map compilation mismatch
#define MICROPY_PY_WEAKREF               (0)
#define MICROPY_PY_COLLECTIONS_DEQUE     (0) // Safe to turn off if you don't need double-ended queues

#define MICROPY_HW_BOARD_NAME "VEX V5 Brain"
#define MICROPY_HW_MCU_NAME   "Cortex-A9"

// PROS System Traps & Standard Output Redirects
unsigned int mp_hal_stdout_tx_strn(const char *str, size_t len);

#endif // INCLUDED_MPCONFIGPORT_H
