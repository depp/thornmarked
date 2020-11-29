#pragma once

#include "base/vectypes.h"

#include <ultra64.h>

inline void mat4_tofixed(Mtx *out, mat4 *in) {
    guMtxF2L((float(*)[4])in->v, out);
}
