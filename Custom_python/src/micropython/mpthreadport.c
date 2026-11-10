#include <stdint.h>
#include <stdlib.h>

#include "mpthreadport.h"
#include "py/gc.h"
#include "py/mpstate.h"
#include "py/mpthread.h"

typedef struct _mp_thread_record_t {
    task_t task;
    void *(*entry)(void *);
    void *arg;
    mp_state_thread_t *state;
    uintptr_t saved_stack_pointer;
    struct _mp_thread_record_t *next;
} mp_thread_record_t;

static mutex_t thread_records_mutex;
static mp_thread_record_t *thread_records;

static void lock_thread_records(void) {
    if (!mutex_take(thread_records_mutex, TIMEOUT_MAX)) {
        abort();
    }
}

static void unlock_thread_records(void) {
    if (!mutex_give(thread_records_mutex)) {
        abort();
    }
}

static mp_thread_record_t *find_thread_record(task_t task) {
    for (mp_thread_record_t *record = thread_records; record != NULL; record = record->next) {
        if (record->task == task) {
            return record;
        }
    }
    return NULL;
}

static void save_stack_pointer(mp_thread_record_t *record) {
    volatile uintptr_t stack_marker = 0;
    record->saved_stack_pointer = (uintptr_t)&stack_marker;
}

void mp_thread_port_init(void) {
    thread_records_mutex = mutex_create();
    if (thread_records_mutex == NULL) {
        abort();
    }

    mp_thread_record_t *main_record = malloc(sizeof(*main_record));
    if (main_record == NULL) {
        abort();
    }
    main_record->task = task_get_current();
    main_record->entry = NULL;
    main_record->arg = NULL;
    main_record->state = &mp_state_ctx.thread;
    main_record->next = NULL;
    save_stack_pointer(main_record);
    thread_records = main_record;
}

struct _mp_state_thread_t *mp_thread_get_state(void) {
    task_t current_task = task_get_current();
    lock_thread_records();
    mp_thread_record_t *record = find_thread_record(current_task);
    mp_state_thread_t *state = record == NULL ? NULL : record->state;
    unlock_thread_records();
    if (state == NULL) {
        abort();
    }
    return state;
}

void mp_thread_set_state(struct _mp_state_thread_t *state) {
    task_t current_task = task_get_current();
    lock_thread_records();
    mp_thread_record_t *record = find_thread_record(current_task);
    if (record == NULL) {
        record = malloc(sizeof(*record));
        if (record == NULL) {
            unlock_thread_records();
            abort();
        }
        record->task = current_task;
        record->entry = NULL;
        record->arg = NULL;
        record->next = thread_records;
        thread_records = record;
    }
    record->state = state;
    save_stack_pointer(record);
    unlock_thread_records();
}

static void thread_task_entry(void *parameter) {
    mp_thread_record_t *record = parameter;
    lock_thread_records();
    record->task = task_get_current();
    unlock_thread_records();

    record->entry(record->arg);
    mp_thread_finish();

    lock_thread_records();
    mp_thread_record_t **current = &thread_records;
    while (*current != NULL && *current != record) {
        current = &(*current)->next;
    }
    if (*current == record) {
        *current = record->next;
    }
    unlock_thread_records();
    free(record);
}

mp_uint_t mp_thread_create(void *(*entry)(void *), void *arg, size_t *stack_size) {
    size_t stack_bytes = *stack_size;
    uint32_t stack_depth;
    if (stack_bytes == 0) {
        stack_depth = TASK_STACK_DEPTH_DEFAULT;
        stack_bytes = (size_t)stack_depth * sizeof(uint32_t);
    } else {
        if (stack_bytes > (size_t)UINT16_MAX * sizeof(uint32_t)) {
            return 0;
        }
        stack_depth = (uint32_t)((stack_bytes + sizeof(uint32_t) - 1) / sizeof(uint32_t));
        if (stack_depth < TASK_STACK_DEPTH_MIN) {
            stack_depth = TASK_STACK_DEPTH_MIN;
        }
        if (stack_depth > UINT16_MAX) {
            return 0;
        }
        stack_bytes = (size_t)stack_depth * sizeof(uint32_t);
    }
    *stack_size = stack_bytes;

    mp_thread_record_t *record = malloc(sizeof(*record));
    if (record == NULL) {
        return 0;
    }
    record->task = NULL;
    record->entry = entry;
    record->arg = arg;
    record->state = NULL;
    record->saved_stack_pointer = 0;

    lock_thread_records();
    record->next = thread_records;
    thread_records = record;
    unlock_thread_records();

    task_t task = task_create(thread_task_entry, record, TASK_PRIORITY_DEFAULT,
        (uint16_t)stack_depth, "MicroPython Thread");
    if (task == NULL) {
        lock_thread_records();
        mp_thread_record_t **current = &thread_records;
        while (*current != NULL && *current != record) {
            current = &(*current)->next;
        }
        if (*current == record) {
            *current = record->next;
        }
        unlock_thread_records();
        free(record);
        return 0;
    }

    lock_thread_records();
    if (record->task == NULL) {
        record->task = task;
    }
    unlock_thread_records();
    return (mp_uint_t)task;
}

mp_uint_t mp_thread_get_id(void) {
    return (mp_uint_t)task_get_current();
}

void mp_thread_start(void) {
}

void mp_thread_finish(void) {
    lock_thread_records();
    mp_thread_record_t *record = find_thread_record(task_get_current());
    if (record != NULL && record->state != &mp_state_ctx.thread) {
        record->state = NULL;
        record->arg = NULL;
    }
    unlock_thread_records();
}

void mp_thread_mutex_init(mp_thread_mutex_t *mutex) {
    *mutex = mutex_create();
    if (*mutex == NULL) {
        abort();
    }
}

int mp_thread_mutex_lock(mp_thread_mutex_t *mutex, int wait) {
    if (mutex == &MP_STATE_VM(gil_mutex)) {
        lock_thread_records();
        mp_thread_record_t *record = find_thread_record(task_get_current());
        if (record != NULL) {
            save_stack_pointer(record);
        }
        unlock_thread_records();
    }
    return mutex_take(*mutex, wait ? TIMEOUT_MAX : 0) ? 1 : 0;
}

void mp_thread_mutex_unlock(mp_thread_mutex_t *mutex) {
    if (mutex == &MP_STATE_VM(gil_mutex)) {
        lock_thread_records();
        mp_thread_record_t *record = find_thread_record(task_get_current());
        if (record != NULL) {
            save_stack_pointer(record);
        }
        unlock_thread_records();
    }
    if (!mutex_give(*mutex)) {
        abort();
    }
}

void mp_thread_gc_others(void) {
    task_t current_task = task_get_current();
    lock_thread_records();
    for (mp_thread_record_t *record = thread_records; record != NULL; record = record->next) {
        if (record->arg != NULL) {
            void **arg_root = &record->arg;
            gc_collect_root(arg_root, 1);
        }
        if (record->task == current_task || record->state == NULL) {
            continue;
        }

        uintptr_t stack_top = (uintptr_t)record->state->stack_top;
        uintptr_t stack_pointer = record->saved_stack_pointer;
        if (stack_pointer < stack_top) {
            void **stack_roots = (void **)stack_pointer;
            gc_collect_root(stack_roots, (stack_top - stack_pointer) / sizeof(void *));
        }
    }
    unlock_thread_records();
}
