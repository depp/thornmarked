#include "base/testlib/testlib.h"

#include "base/base.h"
#include "base/defs.h"

#include <string.h>

static char quote_buf[1024];

static const char HEX_DIGIT[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                   '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

char *quote_mem(const void *mem, size_t len) {
    const unsigned char *iptr = mem, *iend = iptr + len;
    char *ostart = quote_buf, *optr = ostart,
         *oend = quote_buf + ARRAY_COUNT(quote_buf);
    *optr++ = '"';
    for (; iptr != iend; iptr++) {
        if (oend - optr < 5) {
            break;
        }
        int c = *iptr;
        if (32 <= c && c <= 126) {
            if (c == '"' || c == '\\') {
                *optr++ = '\\';
            }
            *optr++ = c;
        } else {
            *optr++ = '\\';
            switch (c) {
            case '\n':
                *optr++ = 'n';
                break;
            case '\r':
                *optr++ = 'r';
                break;
            case '\t':
                *optr++ = 't';
                break;
            default:
                optr[0] = 'x';
                optr[1] = HEX_DIGIT[c >> 4];
                optr[2] = HEX_DIGIT[c & 15];
                break;
            }
        }
    }
    *optr++ = '"';
    if (iptr != iend) {
        optr[0] = '.';
        optr[1] = '.';
        optr[2] = '.';
        optr += 3;
    }
    *optr = '\0';
    return ostart;
}

char *quote_str(const char *str) {
    return quote_mem(str, strlen(str));
}
