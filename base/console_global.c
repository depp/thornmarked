#include "base/console.h"

#include "base/console_internal.h"

struct console console;

void cprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    console_vprintf(&console, fmt, ap);
    va_end(ap);
}
