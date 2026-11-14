#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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

// ---- Fast thread-state lookup via FreeRTOS thread-local storage ----
// FreeRTOS TLS (exported by libpros.a)
extern void *pvTaskGetThreadLocalStoragePointer(task_t task, int32_t index);
extern void vTaskSetThreadLocalStoragePointer(task_t task, int32_t index, void *value);

#ifndef MP_THREAD_TLS_INDEX
#define MP_THREAD_TLS_INDEX (4)   // PROS uses the low slots
#endif

static inline mp_thread_record_t *tls_get_record(void) {
    return (mp_thread_record_t *)pvTaskGetThreadLocalStoragePointer(NULL, MP_THREAD_TLS_INDEX);
}

static inline void tls_set_record(mp_thread_record_t *record) {
    vTaskSetThreadLocalStoragePointer(NULL, MP_THREAD_TLS_INDEX, record);
}

// Fallback: old locked lookup, then cache it in TLS
static mp_thread_record_t *slow_find_current_record(void) {
    lock_thread_records();
    mp_thread_record_t *record = find_thread_record(task_get_current());
    unlock_thread_records();
    if (record != NULL) {
        tls_set_record(record);
    }
    return record;
}

static inline mp_thread_record_t *current_record(void) {
    mp_thread_record_t *record = tls_get_record();
    return record != NULL ? record : slow_find_current_record();
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
    tls_set_record(main_record);
}

struct _mp_state_thread_t *mp_thread_get_state(void) {
    mp_thread_record_t *record = current_record();
    if (record == NULL || record->state == NULL) {
        abort();
    }
    return record->state;
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
    tls_set_record(record);
    unlock_thread_records();
}

static void thread_task_entry(void *parameter) {
    mp_thread_record_t *record = parameter;
    lock_thread_records();
    record->task = task_get_current();
    unlock_thread_records();
    tls_set_record(record);

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
    tls_set_record(NULL);
    free(record);

    task_delete(NULL); // Required in PROS C API so task never returns to task_clean_up
}

mp_uint_t mp_thread_create(void *(*entry)(void *), void *arg, size_t *stack_size) {
    size_t stack_bytes = *stack_size;
    uint32_t stack_depth = 0x3000;
    *stack_size = (size_t)stack_depth * sizeof(uint32_t);

    mp_thread_record_t *record = malloc(sizeof(*record));
    if (record == NULL) {
        printf("[mp_thread] malloc failed (requested %u bytes)\n", (unsigned)stack_bytes);
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

    task_t task = task_create(thread_task_entry, record, MP_PYTHON_TASK_PRIORITY,
        (uint16_t)stack_depth, "MicroPython Thread");
    if (task == NULL) {
        printf("[mp_thread] task_create failed (requested %u bytes, depth %u)\n",
               (unsigned)stack_bytes, (unsigned)stack_depth);
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

// Number of tasks blocked waiting for the GIL. The VM only hands the GIL over
// when this is non-zero, so a lone thread never pays for a mutex give/take.
static volatile int gil_waiters;

int mp_thread_mutex_lock(mp_thread_mutex_t *mutex, int wait) {
    if (mutex == &MP_STATE_VM(gil_mutex)) {
        mp_thread_record_t *record = current_record();
        if (record != NULL) {
            save_stack_pointer(record);
        }
        if (mutex_take(*mutex, 0)) {
            return 1;
        }
        if (!wait) {
            return 0;
        }
        __atomic_add_fetch(&gil_waiters, 1, __ATOMIC_SEQ_CST);
        bool taken = mutex_take(*mutex, TIMEOUT_MAX);
        __atomic_sub_fetch(&gil_waiters, 1, __ATOMIC_SEQ_CST);
        return taken ? 1 : 0;
    }
    return mutex_take(*mutex, wait ? TIMEOUT_MAX : 0) ? 1 : 0;
}

// FreeRTOS mutexes are not fair: giving the GIL to an equal-priority waiter
// does not switch to it, so the holder took it straight back and the waiter
// starved until a tick happened to land in the gap. Hand off explicitly:
// release, yield (delay(0) == taskYIELD), then queue up behind the waiter.
void mp_thread_gil_bounce(void) {
    if (__atomic_load_n(&gil_waiters, __ATOMIC_RELAXED) == 0) {
        return;
    }
    MP_THREAD_GIL_EXIT();
    delay(0);
    MP_THREAD_GIL_ENTER();
}

void mp_thread_mutex_unlock(mp_thread_mutex_t *mutex) {
    if (mutex == &MP_STATE_VM(gil_mutex)) {
        mp_thread_record_t *record = current_record();
        if (record != NULL) {
            save_stack_pointer(record);
        }
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
