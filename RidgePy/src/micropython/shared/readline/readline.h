#ifndef MICROPY_INCLUDED_SHARED_READLINE_READLINE_H
#define MICROPY_INCLUDED_SHARED_READLINE_READLINE_H

#include "py/misc.h"

// Define standard terminal control character codes needed by modbuiltins.c
#define CHAR_CTRL_A (1)
#define CHAR_CTRL_B (2)
#define CHAR_CTRL_C (3)
#define CHAR_CTRL_D (4)
#define CHAR_CTRL_E (5)

void readline_init0(void);
int readline(vstr_t *line, const char *prompt);
void readline_push_history(const char *line);
void readline_init(vstr_t *line, const char *prompt);
void readline_note_newline(const char *prompt);
int readline_process_char(int c);

#endif // MICROPY_INCLUDED_SHARED_READLINE_READLINE_H
