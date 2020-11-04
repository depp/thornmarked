#pragma once

#include <ultra64.h>

#include <stdarg.h>
#include <stdint.h>

// A console buffer for printing debugging text on the screen.
struct console;

// Initialize the console.
void console_init(struct console *cs);

// Write a single character to the console.
void console_putc(struct console *cs, int c);

// Write a string to the console.
void console_puts(struct console *cs, const char *s);

// Write a formatted string to the console.
void console_printf(struct console *cs, const char *fmt, ...);

// Write a formatted string to the console.
void console_vprintf(struct console *cs, const char *fmt, va_list ap);

// Draw the console to the framebuffer using the CPU. Does not flush cache.
void console_draw_raw(struct console *cs, uint16_t *restrict framebuffer);

extern struct console console;
