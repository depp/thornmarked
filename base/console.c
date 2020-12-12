#include "base/console_internal.h"

#include "base/base.h"
#include "base/defs.h"

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

void console_init(struct console *cs, console_type ctype) {
    cs->ptr = cs->chars;
    cs->rowstart = cs->chars;
    cs->rowend = cs->chars + CON_COLS;
    cs->row = 0;
    cs->ctype = ctype;
}

struct console *console_new(console_type ctype) {
    struct console *cs = mem_alloc(sizeof(*cs));
    console_init(cs, ctype);
    return cs;
}

static void console_nextrow(struct console *cs) {
    cs->rows[cs->row] = (struct console_row){
        .start = cs->rowstart - cs->chars,
        .end = cs->ptr - cs->chars,
    };
    cs->row++;
    if (cs->ctype == CONSOLE_TRUNCATE) {
        cs->rowstart = cs->ptr;
        if (cs->row < CON_ROWS) {
            cs->rowend = cs->ptr + CON_COLS;
        } else {
            cs->rowend = cs->ptr;
        }
    } else {
        if (cs->row < CON_ROWS) {
            cs->rowstart += CON_COLS;
            cs->rowend += CON_COLS;
        } else {
            cs->rowstart = cs->chars;
            cs->rowend = cs->chars + CON_COLS;
            cs->row = 0;
        }
        cs->ptr = cs->rowstart;
    }
}

void console_newline(struct console *cs) {
    if (cs->rowstart != cs->ptr) {
        console_nextrow(cs);
    }
}

void console_putc(struct console *cs, int c) {
    if (c == '\n') {
        if (cs->row < CON_ROWS) {
            console_nextrow(cs);
        }
    } else if (cs->ptr < cs->rowend) {
        *cs->ptr++ = c;
    } else if (cs->row < CON_ROWS) {
        console_nextrow(cs);
        *cs->ptr++ = c;
    }
}

void console_puts(struct console *cs, const char *s) {
    for (; *s != '\0'; s++) {
        console_putc(cs, *s);
    }
}

// Formatting flags.
enum {
    FMT_LEFTJUSTIFY = 01, // Left-justify result.
    FMT_ALWAYSSIGN = 02,  // Always include + or - sign.
    FMT_SPACE = 04,       // Prefix with space if no sign.
    FMT_ALTERNATE = 010,  // Alternative form.
    FMT_ZEROPAD = 020,    // Pad with zeroes.
    FMT_HASPRECISION = 040,
    FMT_INT = 0100,
};

// Length modifiers.
enum {
    FLEN_NORMAL,
    FLEN_HH,
    FLEN_H,
    FLEN_L,
    FLEN_LL,
    FLEN_J,
    FLEN_Z,
    FLEN_T,
    FLEN_BIGL,
};

static intmax_t read_argi(int lengthmod, va_list *ap) {
    switch (lengthmod) {
    case FLEN_NORMAL:
        return va_arg(*ap, int);
    case FLEN_HH:
        return (signed char)va_arg(*ap, int);
    case FLEN_H:
        return (short)va_arg(*ap, int);
    case FLEN_L:
        return va_arg(*ap, long);
    case FLEN_LL:
        return va_arg(*ap, long long);
    case FLEN_J:
        return va_arg(*ap, intmax_t);
    case FLEN_Z:
        return va_arg(*ap, size_t);
    case FLEN_T:
        return va_arg(*ap, ptrdiff_t);
    default:
        return 0;
    }
}

static uintmax_t read_argu(int lengthmod, va_list *ap) {
    switch (lengthmod) {
    case FLEN_NORMAL:
        return va_arg(*ap, unsigned);
    case FLEN_HH:
        return (unsigned char)va_arg(*ap, int);
    case FLEN_H:
        return (unsigned short)va_arg(*ap, int);
    case FLEN_L:
        return va_arg(*ap, unsigned long);
    case FLEN_LL:
        return va_arg(*ap, unsigned long long);
    case FLEN_J:
        return va_arg(*ap, uintmax_t);
    case FLEN_Z:
        return va_arg(*ap, size_t);
    case FLEN_T:
        return va_arg(*ap, ptrdiff_t);
    default:
        return 0;
    }
}

enum {
    PUTD_LEN = 27,
    PUTO_LEN = 22,
};

static void putd(char *restrict buf, uintmax_t x) {
    uint32_t xs[3];
    const uintmax_t radix = 1000000000;
    xs[2] = x % radix;
    x /= radix;
    xs[1] = x % radix;
    xs[0] = x / radix;
    for (int i = 0; i < 3; i++) {
        uint32_t y = xs[i];
        for (int j = 0; j < 9; j++) {
            buf[i * 9 + 8 - j] = '0' + (y % 10);
            y /= 10;
        }
    }
}

static void puto(char *restrict buf, uintmax_t x) {
    for (int i = 0; i < 22; i++) {
        buf[21 - i] = '0' + (x & 7);
        x >>= 3;
    }
}

static const char HEX_DIGIT[2][16] = {{'0', '1', '2', '3', '4', '5', '6', '7',
                                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'},
                                      {'0', '1', '2', '3', '4', '5', '6', '7',
                                       '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'}};

static void putx(char *restrict buf, uint32_t x,
                 const char *restrict hexdigit) {
    for (int i = 0; i < 8; i++) {
        buf[7 - i] = hexdigit[(x >> (4 * i)) & 15];
    }
}

// Powers of 10 which are exact.
static const double POWERS_OF_10[23] = {
    1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
    1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22,
};

enum {
    PUTF_LEN = PUTD_LEN + 1,
};

static bool putf(char *restrict buf, double x, int precision) {
    if (precision > 22) {
        precision = 22;
    } else if (precision < 0) {
        precision = 0;
    }
    double xs = x * POWERS_OF_10[precision];
    if (xs > LLONG_MAX) {
        return false;
    }
    putd(buf, (long long)xs);
    int decimal_pos = PUTD_LEN - precision;
    for (int i = PUTD_LEN; i > decimal_pos; i--) {
        buf[i] = buf[i - 1];
    }
    buf[decimal_pos] = '.';
    return true;
}

static const char *trimzero(const char *ptr, const char *end) {
    while (ptr != end && *ptr == '0') {
        ptr++;
    }
    return ptr;
}

static void console_putptr(struct console *cs, const char *restrict ptr,
                           int n) {
    for (int i = 0; i < n; i++) {
        console_putc(cs, ptr[i]);
    }
}

static void console_putpad(struct console *cs, int ch, int n) {
    for (int i = 0; i < n; i++) {
        console_putc(cs, ch);
    }
}

static const char *console_putitem(struct console *cs, const char *restrict fmt,
                                   va_list *ap) {
    if (*fmt == '%') {
        console_putc(cs, '%');
        return fmt + 1;
    }
    const char *ptr = fmt;
    unsigned flags = 0;
    int width = 0, precision = 0, lengthmod = FLEN_NORMAL;
    // Parse flags.
    for (;; ptr++) {
        int c = (unsigned char)*ptr;
        switch (c) {
        case '-':
            flags |= FMT_LEFTJUSTIFY;
            break;
        case '+':
            flags |= FMT_ALWAYSSIGN;
            break;
        case ' ':
            flags |= FMT_SPACE;
            break;
        case '#':
            flags |= FMT_ALTERNATE;
            break;
        case '0':
            flags |= FMT_ZEROPAD;
            break;
        default:
            goto parse_width;
        }
    }
    // Parse field width.
parse_width:
    if (*ptr == '*') {
        width = va_arg(*ap, int);
        if (width < 0) {
            width = -width;
            flags |= FMT_LEFTJUSTIFY;
        }
    } else if ('0' <= *ptr && *ptr <= '9') {
        int c = (unsigned char)*ptr;
        do {
            width = 10 * width + (c - '0');
            ptr++;
            c = (unsigned char)*ptr;
        } while ('0' <= c && c <= '9');
    }
    // Parse precision.
    if (*ptr == '.') {
        flags |= FMT_HASPRECISION;
        ptr++;
        int c = (unsigned char)*ptr;
        if (c == '*') {
            precision = va_arg(*ap, int);
            if (precision < 0) {
                flags &= ~FMT_HASPRECISION;
            }
            ptr++;
        } else if ('0' <= c && c <= '9') {
            do {
                precision = 10 * precision + (c - '0');
                ptr++;
                c = (unsigned char)*ptr;
            } while ('0' <= c && c <= '9');
        } else {
            goto bad_format;
        }
    }
    // Parse length modifier.
    {
        int c = (unsigned char)ptr[0];
        int adv = 1;
        switch (c) {
        case 'h':
            lengthmod = FLEN_H;
            if (ptr[1] == 'h') {
                lengthmod = FLEN_HH;
                adv = 2;
            }
            break;
        case 'l':
            lengthmod = FLEN_L;
            if (ptr[1] == 'l') {
                lengthmod = FLEN_LL;
                adv = 2;
            }
            break;
        case 'j':
            lengthmod = FLEN_J;
            break;
        case 'z':
            lengthmod = FLEN_Z;
            break;
        case 't':
            lengthmod = FLEN_T;
            break;
        case 'L':
            lengthmod = FLEN_BIGL;
            break;
        default:
            adv = 0;
            break;
        }
        ptr += adv;
    }
    // Parse conversion specifier and do conversion.
    char cbuf[32]; // Conversion buffer, 23 is 1<<63 with '#o' conversion.
    char prefix[2] = {'\0', '\0'};
    const char *cptr, *cend;
    int c = (unsigned char)*ptr;
    switch (c) {
    case 'd':
    case 'i': {
        flags |= FMT_INT;
        intmax_t val = read_argi(lengthmod, ap);
        uintmax_t uval = val;
        if (val < 0) {
            uval = ~uval + 1;
            prefix[0] = '-';
        } else if ((flags & FMT_ALWAYSSIGN) != 0) {
            prefix[0] = '+';
        } else if ((flags & FMT_SPACE) != 0) {
            prefix[0] = ' ';
        }
        putd(cbuf, uval);
        cptr = cbuf;
        cend = cbuf + PUTD_LEN;
    } break;
    // FIXME
    // case 'n':
    //     break;
    case 'o': {
        flags |= FMT_INT;
        uintmax_t val = read_argu(lengthmod, ap);
        puto(cbuf, val);
        cptr = cbuf;
        cend = cbuf + PUTO_LEN;
        if ((flags & FMT_ALTERNATE) != 0) {
            int minprecision = cend - cptr + 1;
            if (precision < minprecision) {
                precision = minprecision;
            }
        }
    } break;
    case 'u': {
        flags |= FMT_INT;
        uintmax_t val = read_argu(lengthmod, ap);
        putd(cbuf, val);
        cptr = cbuf;
        cend = cbuf + PUTD_LEN;
    } break;
    case 'x':
    case 'X': {
        flags |= FMT_INT;
        uintmax_t val = read_argu(lengthmod, ap);
        const char *restrict hexdigit = HEX_DIGIT[c == 'X'];
        putx(cbuf, val >> 32, hexdigit);
        putx(cbuf + 8, val, hexdigit);
        cptr = cbuf;
        cend = cbuf + 16;
        if (val != 0 && (flags & FMT_ALTERNATE) != 0) {
            prefix[0] = '0';
            prefix[1] = c;
        }
    } break;
    case 'c': {
        int cval = va_arg(*ap, int);
        cbuf[0] = cval;
        cptr = cbuf;
        cend = cbuf + 1;
    } break;
    case 'p': {
        void *pval = va_arg(*ap, void *);
        cbuf[0] = '$';
        putx(cbuf + 1, (uintptr_t)pval, HEX_DIGIT[0]);
        cptr = cbuf;
        cend = cbuf + 9;
    } break;
    case 's':
        cptr = va_arg(*ap, const char *);
        if (cptr == NULL) {
            cptr = "(null)";
            cend = cptr + 6;
        } else {
            cend = cptr + strlen(cptr);
        }
        if ((flags & FMT_HASPRECISION) != 0) {
            ptrdiff_t len = cend - cptr;
            if (len > precision) {
                cend = cptr + precision;
            }
        }
        break;
    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G': {
        bool uppercase = (c & 32) == 0;
        union {
            double f;
            uint64_t i;
        } val;
        val.f = va_arg(*ap, double);
        unsigned exponent = (unsigned)(val.i >> 52) & ((1u << 11) - 1);
        if ((val.i >> 63) != 0) {
            val.f = -val.f;
            prefix[0] = '-';
        } else if ((flags & FMT_ALWAYSSIGN) != 0) {
            prefix[0] = '+';
        } else if ((flags & FMT_SPACE) != 0) {
            prefix[0] = ' ';
        }
        if ((flags & FMT_HASPRECISION) == 0) {
            precision = 6;
        }
        if (exponent == (1 << 11) - 1) {
            if ((val.i & ((1ull << 52) - 1)) == 0) {
                cptr = uppercase ? "INF" : "inf";
            } else {
                cptr = uppercase ? "NAN" : "nan";
                prefix[0] = '\0';
            }
            cend = cptr + 3;
        } else {
            putf(cbuf, val.f, precision);
            cptr = cbuf;
            cend = cbuf + PUTF_LEN;
            int dec_pos = PUTD_LEN - precision;
            if (dec_pos > 1) {
                cptr = trimzero(cptr, cptr + dec_pos - 1);
            }
        }
    } break;
    default:
        goto bad_format;
    }
    int clen;
    int plen = (prefix[0] != '\0') + (prefix[1] != '\0');
    int zlen = 0;
    if ((flags & FMT_INT) != 0) {
        cptr = trimzero(cptr, cend);
        clen = cend - cptr;
        // Note: '-' has precedence over '0'.
        if ((flags & (FMT_ZEROPAD | FMT_HASPRECISION)) == FMT_ZEROPAD) {
            zlen = width - clen - plen;
        }
        if ((flags & FMT_HASPRECISION) == 0 && precision == 0) {
            precision = 1;
        }
        if (zlen + clen < precision) {
            zlen = precision - clen;
        }
        if (zlen < 0) {
            zlen = 0;
        }
    } else {
        clen = cend - cptr;
    }
    int slen = width - clen - plen - zlen;
    int llen = 0, rlen = 0;
    if ((flags & FMT_LEFTJUSTIFY) != 0) {
        rlen = slen;
    } else {
        llen = slen;
    }
    console_putpad(cs, ' ', llen);
    console_putptr(cs, prefix, plen);
    console_putpad(cs, '0', zlen);
    console_putptr(cs, cptr, clen);
    console_putpad(cs, ' ', rlen);
    return ptr + 1;

bad_format:
    console_puts(cs, "%ERR");
    return fmt;
}

void console_printf(struct console *cs, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    console_vprintf(cs, fmt, ap);
    va_end(ap);
}

void console_vprintf(struct console *cs, const char *fmt, va_list ap) {
    va_list aq;
    va_copy(aq, ap);
    for (;;) {
        unsigned c = (unsigned char)*fmt++;
        if (c == '\0') {
            break;
        } else if (c == '%') {
            fmt = console_putitem(cs, fmt, &aq);
        } else {
            console_putc(cs, c);
        }
    }
    va_end(aq);
}

int console_rows(struct console *cs, struct console_rowptr *restrict rows) {
    int pos = cs->row;
    if (cs->ptr != cs->rowstart) {
        cs->rows[pos] = (struct console_row){
            .start = cs->rowstart - cs->chars,
            .end = cs->ptr - cs->chars,
        };
        pos++;
    }
    if (cs->ctype == CONSOLE_TRUNCATE) {
        for (int i = 0; i < pos; i++) {
            rows[i] = (struct console_rowptr){
                .start = cs->chars + cs->rows[i].start,
                .end = cs->chars + cs->rows[i].end,
            };
        }
        return pos;
    } else {
        int i = 0;
        for (; i < pos; i++) {
            rows[i + CON_ROWS - pos] = (struct console_rowptr){
                .start = cs->chars + cs->rows[i].start,
                .end = cs->chars + cs->rows[i].end,
            };
        }
        for (; i < CON_ROWS; i++) {
            rows[i - pos] = (struct console_rowptr){
                .start = cs->chars + cs->rows[i].start,
                .end = cs->chars + cs->rows[i].end,
            };
        }
        return CON_ROWS;
    }
}

void (*console_vfatal_func)(struct console *cs, const char *fmt, va_list ap);

noreturn void console_fatal(struct console *cs, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    console_vfatal(cs, fmt, ap);
    va_end(ap);
}

noreturn void console_vfatal(struct console *cs, const char *fmt, va_list ap) {
    if (console_vfatal_func != NULL) {
        console_vfatal_func(cs, fmt, ap);
    }
    __builtin_trap();
}
