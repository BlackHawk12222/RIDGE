#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <atomic>
#include <arm_neon.h>
#include "main.h"
#include "pros/apix.h"
#include "vex/font_init.h"

// MicroPython specific headers (must be inside extern "C")
extern "C" {
    #include "py/obj.h"
    #include "py/runtime.h"
    #include "py/mpthread.h"
    bool capture_traceback_mode = false;
    char traceback_buffer[1024];
    extern const mp_print_t *PrintExeption; 
    extern size_t ValueExeptionList;

    // Core shared runtime execution wrappers
    void init_micropython(void* heap, size_t heap_size);
    int mp_do_str(const char* src, int input_kind);
    void vex_competition_callback(int event);

    // Keep ONLY this declaration line here. REMOVE the full function body definition.
    void mp_port_print_last_exception(void);
}

enum {
    VEX_COMPETITION_DRIVER_CONTROL = 0,
    VEX_COMPETITION_AUTONOMOUS = 1,
    VEX_COMPETITION_PRE_AUTON = 2,
    VEX_COMPETITION_DISABLED = 3,
};

#define MP_HEAP_SIZE (1024 * 1024) // 1 MiB Heap
static char mp_heap[MP_HEAP_SIZE];
static std::atomic_flag competition_event_lock = ATOMIC_FLAG_INIT;
static bool python_task_started = false;
static int pending_competition_event = -1;
static constexpr int STATUS_SCREEN_LINES = 8;
static int status_screen_line = 1;

static void report_status(const char* message) {
    printf("%s\n", message);
    if (status_screen_line > STATUS_SCREEN_LINES) {
        pros::screen::scroll(1, 1);
        status_screen_line = STATUS_SCREEN_LINES;
    }
    pros::screen::print(TEXT_SMALL, status_screen_line++, "%s                                                ", message);
}

extern "C" void mp_port_status(const char* message) {
    report_status(message);
}

static void lock_competition_events() {
    while (competition_event_lock.test_and_set(std::memory_order_acquire)) {
    }
}

static void unlock_competition_events() {
    competition_event_lock.clear(std::memory_order_release);
}

static void queue_competition_event(int event) {
    lock_competition_events();
    if (!python_task_started) {
        pending_competition_event = event;
        unlock_competition_events();
        return;
    }
    unlock_competition_events();
    vex_competition_callback(event);
}

static void run_python_script(void* script) {
    char* python_buffer = static_cast<char*>(script);
    init_micropython(mp_heap, sizeof(mp_heap));
    report_status("MP: ready");

    lock_competition_events();
    python_task_started = true;
    const int pending_event = pending_competition_event;
    pending_competition_event = -1;
    unlock_competition_events();
    if (pending_event >= 0) {
        vex_competition_callback(pending_event);
    }

    capture_traceback_mode = false;
    report_status("MicroPython: running script");
    int script_succeeded = mp_do_str(python_buffer, 1);
    free(python_buffer);

    if (script_succeeded) {
        report_status("MicroPython: script finished");
    } else {
        report_status("MicroPython: script failed");
    }

    while (true) {
        MP_THREAD_GIL_EXIT();
        pros::delay(2);
        MP_THREAD_GIL_ENTER();
        mp_handle_pending(MP_HANDLE_PENDING_CALLBACKS_ONLY);
    }
}

void initialize() {

    lv_obj_t* active_screen = lv_screen_active();

    lv_obj_set_style_text_font(active_screen, &mono30, LV_PART_MAIN);

    lv_obj_set_style_text_color(active_screen, lv_color_white(), LV_PART_MAIN);

    pros::screen::erase();
    status_screen_line = 1;
    report_status("MP: initializing");

    report_status("initalize: opening /usd/main.py");
    FILE* py_file = fopen("/usd/main.py", "r");

    if (py_file == NULL) {
        report_status("ERROR: /usd/main.py not found");
        return;
    }

    if (fseek(py_file, 0, SEEK_END) != 0) {
        fclose(py_file);
        report_status("ERROR: cannot seek /usd/main.py");
        return;
    }

    long file_size = ftell(py_file);
    if (file_size < 0 || fseek(py_file, 0, SEEK_SET) != 0) {
        fclose(py_file);
        report_status("ERROR: cannot size /usd/main.py");
        return;
    }

    char* python_buffer = (char*)malloc((size_t)file_size + 1);
    if (python_buffer == NULL) {
        fclose(py_file);
        report_status("ERROR: not enough memory for script");
        return;
    }

    size_t bytes_read = fread(python_buffer, 1, (size_t)file_size, py_file);
    int read_error = ferror(py_file);
    fclose(py_file);

    if (read_error || bytes_read != (size_t)file_size) {
        free(python_buffer);
        report_status("ERROR: failed reading /usd/main.py");
        return;
    }
    python_buffer[bytes_read] = '\0';

    pros::task_t task = pros::c::task_create(
        run_python_script, python_buffer, TASK_PRIORITY_DEFAULT, 0x4000,
        "MicroPython Script"
    );
    if (task == nullptr) {
        free(python_buffer);
        report_status("ERROR: cannot start MicroPython task");
        return;
    }
    report_status("MP: script task started");
}

void autonomous() {
    queue_competition_event(VEX_COMPETITION_AUTONOMOUS);
    while (!pros::competition::is_disabled()) {
        pros::delay(10);
    }
}

void opcontrol() {
    queue_competition_event(VEX_COMPETITION_DRIVER_CONTROL);
    while (!pros::competition::is_disabled()) {
        pros::delay(10);
    }
}

void competition_initialize() {
    queue_competition_event(VEX_COMPETITION_PRE_AUTON);
}

void disabled() {
    queue_competition_event(VEX_COMPETITION_DISABLED);
}