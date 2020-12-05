#pragma once

#include <stdarg.h>
#include <stdnoreturn.h>

struct console;

noreturn void console_vfatal_impl(struct console *cs, const char *fmt,
                                  va_list ap);
