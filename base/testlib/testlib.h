// Test harness.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdnoreturn.h>

// Quote a bytestring. The result is only valid until the next call to quote_str
// or quote_mem.
char *quote_mem(const void *mem, size_t len);

// Quote a string. The result is only valid until the next call to quote_str or
// quote_mem.
char *quote_str(const char *str);

// Start running a test with the given name.
void test_start(const char *name);

// Log messages for the current test.
void test_logf_func(const char *file, int line, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));
#define test_logf(...) test_logf_func(__FILE__, __LINE__, __VA_ARGS__)

// Fail the current test and show an error message.
noreturn void test_fail_func(const char *file, int line);
#define test_fail() test_fail_func(__FILE__, __LINE__)

// Test entry point, must be defined by test.
void test_main(void);
