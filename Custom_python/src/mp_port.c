#include <stdio.h>   // Added to fix the printf compiler error!
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include "py/builtin.h"
#include "py/lexer.h"
#include "py/parse.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mpprint.h"
#include "py/stream.h"

typedef struct _mp_vex_file_obj_t {
    mp_obj_base_t base;
    FILE *file;
} mp_vex_file_obj_t;

static mp_uint_t mp_vex_file_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    mp_vex_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->file == NULL) {
        *errcode = MP_EBADF;
        return MP_STREAM_ERROR;
    }
    size_t read_size = fread(buf, 1, size, self->file);
    if (read_size < size && ferror(self->file)) {
        *errcode = errno;
        return MP_STREAM_ERROR;
    }
    return read_size;
}

static mp_uint_t mp_vex_file_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    mp_vex_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->file == NULL) {
        *errcode = MP_EBADF;
        return MP_STREAM_ERROR;
    }
    size_t write_size = fwrite(buf, 1, size, self->file);
    if (write_size < size) {
        *errcode = errno;
        return MP_STREAM_ERROR;
    }
    return write_size;
}

static mp_uint_t mp_vex_file_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    mp_vex_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (request == MP_STREAM_CLOSE) {
        if (self->file != NULL) {
            int result = fclose(self->file);
            self->file = NULL;
            if (result != 0) {
                *errcode = errno;
                return MP_STREAM_ERROR;
            }
        }
        return 0;
    }
    if (self->file == NULL) {
        *errcode = MP_EBADF;
        return MP_STREAM_ERROR;
    }
    switch (request) {
        case MP_STREAM_FLUSH:
            if (fflush(self->file) != 0) {
                *errcode = errno;
                return MP_STREAM_ERROR;
            }
            return 0;
        case MP_STREAM_SEEK: {
            struct mp_stream_seek_t *seek = (struct mp_stream_seek_t *)arg;
            int whence;
            switch (seek->whence) {
                case MP_SEEK_SET:
                    whence = SEEK_SET;
                    break;
                case MP_SEEK_CUR:
                    whence = SEEK_CUR;
                    break;
                case MP_SEEK_END:
                    whence = SEEK_END;
                    break;
                default:
                    *errcode = MP_EINVAL;
                    return MP_STREAM_ERROR;
            }
            if (fseek(self->file, seek->offset, whence) != 0) {
                *errcode = errno;
                return MP_STREAM_ERROR;
            }
            long position = ftell(self->file);
            if (position < 0) {
                *errcode = errno;
                return MP_STREAM_ERROR;
            }
            seek->offset = position;
            return 0;
        }
        case MP_STREAM_GET_FILENO: {
            int fd = fileno(self->file);
            if (fd < 0) {
                *errcode = errno;
                return MP_STREAM_ERROR;
            }
            return fd;
        }
        default:
            *errcode = MP_EINVAL;
            return MP_STREAM_ERROR;
    }
}

static const mp_stream_p_t mp_vex_text_file_stream = {
    .read = mp_vex_file_read,
    .write = mp_vex_file_write,
    .ioctl = mp_vex_file_ioctl,
    .is_text = true,
};

static const mp_stream_p_t mp_vex_binary_file_stream = {
    .read = mp_vex_file_read,
    .write = mp_vex_file_write,
    .ioctl = mp_vex_file_ioctl,
};

static const mp_rom_map_elem_t mp_vex_file_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&mp_stream_ioctl_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&mp_stream___exit___obj) },
};
static MP_DEFINE_CONST_DICT(mp_vex_file_locals, mp_vex_file_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_vex_text_file_type,
    MP_QSTR_TextIOWrapper,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    protocol, &mp_vex_text_file_stream,
    locals_dict, &mp_vex_file_locals
);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_vex_binary_file_type,
    MP_QSTR_FileIO,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    protocol, &mp_vex_binary_file_stream,
    locals_dict, &mp_vex_file_locals
);

static mp_obj_t mp_vex_open(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_file, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_QSTR(MP_QSTR_r)} },
    };
    mp_arg_val_t parsed_args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed_args);

    const char *path = mp_obj_str_get_str(parsed_args[0].u_obj);
    const char *mode = mp_obj_str_get_str(parsed_args[1].u_obj);
    FILE *file = fopen(path, mode);
    if (file == NULL) {
        mp_raise_OSError(errno);
    }

    bool binary = strchr(mode, 'b') != NULL;
    mp_vex_file_obj_t *self = mp_obj_malloc(mp_vex_file_obj_t,
        binary ? &mp_vex_binary_file_type : &mp_vex_text_file_type);
    self->file = file;
    return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_vex_open);

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

mp_import_stat_t mp_import_stat(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) == 0) {
        if (S_ISDIR(path_stat.st_mode)) {
            return MP_IMPORT_STAT_DIR;
        }
        if (S_ISREG(path_stat.st_mode)) {
            return MP_IMPORT_STAT_FILE;
        }
    }

    // PROS can open SD files but may not report SD directories through stat.
    // Treat a path as a package directory when its initializer is readable.
    size_t path_len = strlen(path);
    bool has_trailing_separator = path_len > 0 && path[path_len - 1] == '/';
    const char *package_init = "__init__.py";
    char init_path[MICROPY_ALLOC_PATH_MAX + sizeof("/__init__.py")];
    int init_path_len = snprintf(init_path, sizeof(init_path), "%s%s%s",
        path, has_trailing_separator ? "" : "/", package_init);
    if (init_path_len < 0 || (size_t)init_path_len >= sizeof(init_path)) {
        return MP_IMPORT_STAT_NO_EXIST;
    }

    FILE *file = fopen(init_path, "rb");
    if (file == NULL) {
        file = fopen(path, "rb");
        if (file == NULL) {
            return MP_IMPORT_STAT_NO_EXIST;
        }
        fclose(file);
        return MP_IMPORT_STAT_FILE;
    }
    fclose(file);
    return MP_IMPORT_STAT_DIR;
}
