#include "base/console.h"

#include "base/system_pc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

noreturn void console_vfatal_impl(struct console *cs, const char *fmt,
                                  va_list ap) {
    (void)cs;
    fputs("Error: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    exit(1);
}
