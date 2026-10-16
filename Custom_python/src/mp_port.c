#include <stdio.h>   // Added to fix the printf compiler error!
#include <stdint.h>
#include <string.h>
#include "py/lexer.h"
#include "py/parse.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/misc.h"  // Added to fix the mp_import_stat_t undefined error!
#include "py/obj.h"
#include "py/mpprint.h"

// Reference core module structures compiled inside py/modmath.c, py/modarray.c, etc.
#if MICROPY_PY_MATH
extern const mp_obj_module_t mp_module_math;
#endif
#if MICROPY_PY_CMATH
extern const mp_obj_module_t mp_module_cmath;
#endif
#if MICROPY_PY_ARRAY
extern const mp_obj_module_t mp_module_array;
#endif
#if MICROPY_PY_COLLECTIONS
extern const mp_obj_module_t mp_module_collections;
#endif
#if MICROPY_PY_STRUCT
extern const mp_obj_module_t mp_module_struct;
#endif

// Keep the manually registered module table linkable when optional modules are disabled.
#if !MICROPY_PY_BUILTINS_FLOAT || !MICROPY_PY_MATH
const mp_obj_module_t mp_module_math = { .base = { &mp_type_module } };
#endif
#if !MICROPY_PY_BUILTINS_FLOAT || !MICROPY_PY_BUILTINS_COMPLEX || !MICROPY_PY_CMATH
const mp_obj_module_t mp_module_cmath = { .base = { &mp_type_module } };
#endif
#if !MICROPY_PY_ARRAY
const mp_obj_module_t mp_module_array = { .base = { &mp_type_module } };
#endif
#if !MICROPY_PY_COLLECTIONS
const mp_obj_module_t mp_module_collections = { .base = { &mp_type_module } };
#endif
#if !MICROPY_PY_STRUCT
const mp_obj_module_t mp_module_struct = { .base = { &mp_type_module } };
#endif

#if !MICROPY_PY_GC || !MICROPY_ENABLE_GC
const mp_obj_module_t mp_module_gc = { .base = { &mp_type_module } };
#endif
#if !MICROPY_PY_IO
const mp_obj_module_t mp_module_io = { .base = { &mp_type_module } };
#endif
#if !MICROPY_PY_ERRNO
const mp_obj_module_t mp_module_errno = { .base = { &mp_type_module } };
#endif


int mp_do_str(const char *src, int input_kind) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        qstr source_name = qstr_from_str("<stdin>");
        mp_lexer_t *lex = mp_lexer_new_from_str_len(source_name, src, strlen(src), 0);
        if (lex == NULL) {
            printf("Error: MicroPython failed to allocate lexer for the script string!\n");
            nlr_pop();
            return 0;
        }
        
        // Parse the code tokens into a structural tree
        mp_parse_tree_t parse_tree = mp_parse(lex, (mp_parse_input_kind_t)input_kind);
        
        // Compile the parse tree into runnable bytecode
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, false);

        // Execute the bytecode inside the virtual machine
        mp_call_function_0(module_fun);
        
        nlr_pop(); // Code executed successfully! Pop the safety buffer.
        return 1;
    } else {
        printf("MicroPython script error:\n");
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        return 0;
    }
}

// Bypass enum types by using standard int return types
mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    // Resolve the qstr token back to a human-readable string for the print statement
    printf("FileSystem Import Warning: Script file handling not implemented natively (%s)\n", qstr_str(filename));
    return NULL;
}

int mp_import_stat(const char *path) {
    return 0; // 0 matches MP_IMPORT_STAT_NO_EXIST in MicroPython's core engine
}
