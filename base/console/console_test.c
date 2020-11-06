#include "base/console/console.h"
#include "base/base.h"
#include "base/console/internal.h"
#include "base/testlib/testlib.h"

#include <stdbool.h>

static void test_printf(const char *expect, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

static struct console consoles[2];

static void check_equal(void) {
    ptrdiff_t len[2] = {consoles[0].ptr - consoles[0].chars,
                        consoles[1].ptr - consoles[1].chars};

    if (len[0] != len[1] ||
        memcmp(consoles[0].chars, consoles[1].chars, len[0]) != 0) {
        test_logf("Expect: %s", quote_mem(consoles[0].chars, len[0]));
        test_logf("Actual: %s", quote_mem(consoles[1].chars, len[1]));
        test_fail();
    }
    int nrows[2] = {console_nrows(&consoles[0]), console_nrows(&consoles[1])};
    if (nrows[0] == nrows[1]) {
        for (int i = 0; i < nrows[0]; i++) {
            ptrdiff_t end[2] = {consoles[0].rowends[i] - consoles[0].chars,
                                consoles[1].rowends[i] - consoles[1].chars};
            if (end[0] != end[1]) {
                test_logf("Row %d: expect %td, got %td", i, end[0], end[1]);
                test_fail();
            }
        }
    } else {
        test_logf("Row count: expect %d, got %d", nrows[0], nrows[1]);
        test_fail();
    }
}

static void test_printf(const char *expect, const char *fmt, ...) {
    console_init(&consoles[0]);
    console_puts(&consoles[0], expect);
    console_init(&consoles[1]);
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
    test_printf("hi = 1000", "hi = %hi", (0xff << 16) + 1000);
    test_printf("x = 000000", "x = %#06x", 0);
    test_printf("x = 0x0001", "x = %#06x", 1);
    test_printf("lli = 1234567890123456789", "lli = %lli",
                1234567890123456789ll);
    test_printf("c = a", "c = %c", 'a');
}
