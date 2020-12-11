#include "base/tool.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>

static int dtoi8(double x) {
    x *= 255;
    if (x > 255.0) {
        return 255;
    }
    if (x < 0.0) {
        return 0;
    }
    return (int)x;
}

int main(int argc, char **argv) {
    FILE *out;
    if (argc == 2) {
        out = fopen(argv[1], "w");
        if (out == NULL) {
            die_errno(errno, "could not open output");
        }
    } else {
        out = stdout;
    }
    const double intensity = 1.5;
    const double alpha = 1;
    for (int i = 0; i < 16; i++) {
        int ii, ia;
        if (i == 0) {
            ii = 0;
            ia = 0;
        } else {
            double x = log((double)i / 15.0);
            double di = exp(x * intensity);
            double da = exp(x * alpha);
            ii = dtoi8(di);
            ia = dtoi8(da);
        }
        int v = (ii << 8) | ia;
        fprintf(out, "0x%04x,\n", v);
    }
    if (out != stdout && fclose(out) != 0) {
        die_errno(errno, "could not write output");
    }
    return 0;
}
