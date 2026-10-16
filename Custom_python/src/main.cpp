#include "main.h"
#include <stdio.h>
#include <stdlib.h>

// Enforce uniform standard C naming layouts for ALL shared parameters
extern "C" {
    void init_micropython(void* heap, size_t heap_size);
    int mp_do_str(const char* src, int input_kind);
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
}

void autonomous() {
    report_status("Autonomous: no script configured");
}

void opcontrol() {
    report_status("Driver Control: opening /usd/main.py");
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
    int script_succeeded = mp_do_str(python_buffer, 1);
    free(python_buffer);

    report_status(script_succeeded
        ? "MicroPython: script finished"
        : "MicroPython: script error; check console");
    while (true) {
        pros::delay(20);
    }
}