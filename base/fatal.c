#include "base/base.h"

#include "base/console.h"

#include <stdarg.h>

noreturn void fatal_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    console_vfatal(NULL, fmt, ap);
    va_end(ap);
}

noreturn void assert_fail(const char *file, int line, const char *pred) {
    fatal_error("Assertion failed\n%s:%d\n%s", file, line, pred);
}
