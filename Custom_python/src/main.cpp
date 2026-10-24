#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <arm_neon.h>
#include "main.h"

// MicroPython specific headers (must be inside extern "C")
extern "C" {
    #include "py/obj.h"
    bool capture_traceback_mode = false;
    char traceback_buffer[1024];
    extern const mp_print_t *PrintExeption; 
    extern size_t ValueExeptionList;

    // Core shared runtime execution wrappers
    void init_micropython(void* heap, size_t heap_size);
    int mp_do_str(const char* src, int input_kind);

    // Keep ONLY this declaration line here. REMOVE the full function body definition.
    void mp_port_print_last_exception(void);
}

#define MP_HEAP_SIZE (1024 * 1024) // 1 MiB Heap
static char mp_heap[MP_HEAP_SIZE];
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

void initialize() {
    pros::screen::erase();
    status_screen_line = 1;
    report_status("MP: initializing");

    init_micropython(mp_heap, sizeof(mp_heap));
    report_status("MP: ready");
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

    report_status("MicroPython: running script");
    
    // Clear buffer before running
    capture_traceback_mode = false;

    // NOTE: Depending on your port's `mp_do_str` implementation, it might print the 
    // traceback *during* this function call. If it does, change `capture_traceback_mode = false;` 
    // above to `true` so it catches it live.
    int script_succeeded = mp_do_str(python_buffer, 1);
    free(python_buffer);
    
    report_status("MicroPython: script finished");
}

void autonomous() {
    report_status("Autonomous: no script configured");
}

void opcontrol() {
    while (true) {
        pros::delay(20);
    }
}