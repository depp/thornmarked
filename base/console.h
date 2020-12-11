// Debugging console we can print to.
#pragma once

#include <stdarg.h>
#include <stdnoreturn.h>

// Types of consoles.
typedef enum {
    CONSOLE_TRUNCATE,
    CONSOLE_SCROLL,
} console_type;

// A console buffer for printing debugging text on the screen.
struct console;

// Initialize the console.
void console_init(struct console *cs, console_type ctype);

// Allocate a new console.
struct console *console_new(console_type ctype);

// Start a new line if we are not at the beginning of a line.
void console_newline(struct console *cs);

// Write a single character to the console.
void console_putc(struct console *cs, int c);

// Write a string to the console.
void console_puts(struct console *cs, const char *s);

// Write a formatted string to the console.
void console_printf(struct console *cs, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

// Write a formatted string to the console.
void console_vprintf(struct console *cs, const char *fmt, va_list ap);

// Callback for fatal errors. This should be initialized once at program
// startup. This is a function pointer in order to prevent backwards linking
// dependencies.
extern void (*console_vfatal_func)(struct console *cs, const char *fmt,
                                   va_list ap);

// Show a "fatal error" screen using the given console. If the console is NULL,
// no console is displayed.
noreturn void console_fatal(struct console *cs, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

// Show a "fatal error" screen using the given console. If the console is NULL,
// no console is displayed.
noreturn void console_vfatal(struct console *cs, const char *fmt, va_list ap);

// The main debugging console.
extern struct console console;

// Print to the main console. This should only be done from the main thread,
// whatever thread that is.
void cprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
