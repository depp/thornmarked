// Debugging console we can print to.
#pragma once

#include <stdarg.h>

// Types of consoles.
typedef enum {
    CONSOLE_TRUNCATE,
    CONSOLE_SCROLL,
} console_type;

// A console buffer for printing debugging text on the screen.
struct console;

// Initialize the console.
void console_init(struct console *cs, console_type ctype);

// Write a single character to the console.
void console_putc(struct console *cs, int c);

// Write a string to the console.
void console_puts(struct console *cs, const char *s);

// Write a formatted string to the console.
void console_printf(struct console *cs, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

// Write a formatted string to the console.
void console_vprintf(struct console *cs, const char *fmt, va_list ap);

// The main debugging console.
extern struct console console;
