#include "base/testlib/testlib.h"

void test_main(void) {
    test_start("Example 1");
    test_logf("Log string: %s", quote_str("abc\ndef"));
}
