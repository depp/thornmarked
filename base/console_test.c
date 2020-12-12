#include "base/console.h"
#include "base/base.h"
#include "base/console_internal.h"
#include "base/testlib/testlib.h"

#include <stdbool.h>
#include <string.h>

static void test_printf(const char *expect, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

static struct console consoles[2];

static void check_equal(void) {
    bool ok = true;
    struct console_rowptr rows[2][CON_ROWS];
    int nrows[2] = {console_rows(&consoles[0], rows[0]),
                    console_rows(&consoles[1], rows[1])};
    if (nrows[0] == nrows[1]) {
        for (int i = 0; i < nrows[0]; i++) {
            const uint8_t *ptrs[2] = {rows[0][i].start, rows[1][i].start};
            int lens[2] = {rows[0][i].end - rows[0][i].start,
                           rows[1][i].end - rows[1][i].start};
            if (lens[0] != lens[1] || memcmp(ptrs[0], ptrs[1], lens[0]) != 0) {
                test_logf("Row %d: expect %s", i, quote_mem(ptrs[0], lens[0]));
                test_logf("Row %d: got    %s", i, quote_mem(ptrs[1], lens[1]));
                ok = false;
            }
        }
    } else {
        test_logf("Row count: expect %d, got %d", nrows[0], nrows[1]);
        ok = false;
    }
    if (!ok) {
        test_fail();
    }
}

static void test_printf(const char *expect, const char *fmt, ...) {
    console_init(&consoles[0], CONSOLE_TRUNCATE);
    console_puts(&consoles[0], expect);
    console_init(&consoles[1], CONSOLE_TRUNCATE);
    va_list ap;
    va_start(ap, fmt);
    // va_list aq;
    // va_copy(aq, ap);
    // vfprintf(stderr, fmt, aq);
    // fputc('\n', stderr);
    console_vprintf(&consoles[1], fmt, ap);
    va_end(ap);
    check_equal();
}

void test_main(void) {
    test_printf("Basic string", "Basic string");
    test_printf("i = 1234", "i = %d", 1234);
    test_printf("i = 0", "i = %d", 0);
    test_printf("i = -999000", "i = %d", -999000);
    test_printf("u = 4000000000", "u = %u", 4000000000u);
    test_printf("s = abc", "s = %s", "abc");
    test_printf("s = 000", "s = %s", "000");
    test_printf("s = abcd", "s = %.4s", "abcd9");
    test_printf("s = abcd1", "s = %.*s", (int)5, "abcd1234");
    test_printf("right [   abc]", "right [%6s]", "abc");
    test_printf("left [abc   ]", "left [%-6s]", "abc");
    test_printf("i = 000123", "i = %06d", 123);
    test_printf("i = 123456789", "i = %06d", 123456789);
    test_printf("i = -00123", "i = %06d", -123);
    test_printf("i = +999", "i = %+d", 999);
    test_printf("i = +0", "i = %+d", 0);
    test_printf("i = -555", "i = %+d", -555);
    test_printf("i = [ 123]", "i = [% d]", 123);
    test_printf("i = [-123]", "i = [% d]", -123);
    test_printf("p = $8000ff00", "p = %p", (void *)(uintptr_t)0x8000ff00);
    test_printf("hi = 1000", "hi = %hi", (short)((0xff << 16) + 1000));
    test_printf("x = 000000", "x = %#06x", 0);
    test_printf("x = 0x0001", "x = %#06x", 1);
    test_printf("lli = 1234567890123456789", "lli = %lli",
                1234567890123456789ll);
    test_printf("c = a", "c = %c", 'a');
    test_printf("f = 0.000000", "f = %f", 0.0);
    test_printf("f = 1.000000", "f = %f", 1.0);
    test_printf("f = 9.375000", "f = %f", 9.375);
}
