#include "main.h"
#include <stdio.h>
#include <stdlib.h>

// Enforce uniform standard C naming layouts for ALL shared parameters
extern "C" {
    void init_micropython(void* heap, size_t heap_size);
    void mp_do_str(const char* src, int input_kind);
}

#define MP_HEAP_SIZE (1024 * 1024) // 1024 KB Heap
static char mp_heap[MP_HEAP_SIZE];

void initialize() {
    printf("Initializing Custom MicroPython Interpreter...\n");
    init_micropython(mp_heap, sizeof(mp_heap));
}

void autonomous() {
    // You can call specific python subroutines here if mapped
}

void opcontrol() {
    // 1. Open the raw text script directly from the V5 MicroSD card slot
    FILE* py_file = fopen("/usd/main.py", "r");
    
    if (py_file == NULL) {
        printf("Error: Could not find main.py on the MicroSD card (/usd/main.py)!\n");
        return;
    }

    // 2. Read the entire python text file into memory dynamically
    fseek(py_file, 0, SEEK_END);
    long file_size = ftell(py_file);
    fseek(py_file, 0, SEEK_SET);

    char* python_buffer = (char*)malloc(file_size + 1);
    fread(python_buffer, 1, file_size, py_file);
    python_buffer[file_size] = '\0'; // Null-terminate the string buffer
    fclose(py_file);

    // 3. Hand the raw string buffer over to MicroPython to execute live code
    printf("Executing /usd/main.py...\n");
    mp_do_str(python_buffer, 1);

    // 4. Free up memory allocations
    free(python_buffer);

    // Fallback loop to prevent the task from instantly terminating
    while (true) {
        pros::delay(20);
    }
}
