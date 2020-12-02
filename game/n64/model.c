#include "game/n64/model.h"

#include "assets/assets.h"
#include "base/base.h"
#include "base/mat4.h"
#include "base/mat4_n64.h"
#include "base/pak/pak.h"
#include "base/vec2.h"
#include "base/vec3.h"
#include "game/defs.h"
#include "game/graphics.h"
#include "game/model.h"
#include "game/n64/texture.h"
#include "game/physics.h"

struct model_header {
    void *vertex_data;
    void *display_list;
};

static union {
    struct model_header header;
    uint8_t data[16 * 1024];
} model_data[2] ASSET;

static void *pointer_fixup(void *base, void *ptr, size_t size) {
    uintptr_t value = (uintptr_t)ptr;
    if (value == 0) {
        return NULL;
    }
    if (value > size) {
        fatal_error("Bad pointer in asset\nPointer: %p\nBase: %p\nSize: %zu",
                    ptr, base, size);
    }
    value += (uintptr_t)base;
    return (void *)value;
}

static void load_model(int slot, int asset) {
    pak_load_asset_sync(model_data[slot].data, sizeof(model_data[slot].data),
                        asset);
    struct model_header *p = &model_data[slot].header;
    size_t sz = sizeof(model_data[slot]);
    p->vertex_data = pointer_fixup(p, p->vertex_data, sz);
    p->display_list = pointer_fixup(p, p->display_list, sz);
}

void model_render_init(void) {
    load_model(0, MODEL_FAIRY);
    load_model(1, MODEL_SPIKE);
}

static const Gfx model_setup_dl[] = {
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsSPTexture(0, 0, 0, 0, G_OFF),
    gsSPGeometryMode(0, G_CULL_BACK | G_SHADE | G_SHADING_SMOOTH | G_LIGHTING),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPEndDisplayList(),
};

static const Gfx fairy_setup_dl[] = {
    gsDPPipeSync(),
    gsSPGeometryMode(G_LIGHTING, G_CULL_BACK | G_SHADE | G_SHADING_SMOOTH),
    gsDPSetCombineMode(G_CC_TRILERP, G_CC_MODULATERGB2),
    gsSPEndDisplayList(),
};

Gfx *model_render(Gfx *dl, struct graphics *restrict gr,
                  struct sys_model *restrict msys,
                  struct sys_phys *restrict psys) {
    int current_model = 0, current_model_index = 0;
    float scale = 0.5f;
    for (unsigned i = 0; i < psys->count; i++) {
        struct cp_phys *restrict cp = &psys->entities[i];
        if (i >= msys->count) {
            continue;
        }
        struct cp_model *restrict mp = &msys->entities[i];
        if (mp->model == 0) {
            continue;
        }
        int model = mp->model;
        if (model != current_model) {
            int index;
            switch (model) {
            case MODEL_FAIRY:
                index = 0;
                gSPDisplayList(dl++, fairy_setup_dl);
                dl = texture_use(dl, IMG_FAIRY);
                break;
            case MODEL_SPIKE:
                index = 1;
                gSPDisplayList(dl++, model_setup_dl);
                break;
            default:
                index = -1;
            }
            if (index < 0) {
                continue;
            }
            current_model_index = index;
            gSPSegment(dl++, 1,
                       K0_TO_PHYS(model_data[index].header.vertex_data));
        }
        Mtx *mtx = gr->mtx_ptr++;
        {
            mat4 mat;
            mat4_translate_rotate_scale(
                &mat, vec3_vec2(vec2_scale(cp->pos, meter), meter),
                cp->orientation, scale);
            mat4_tofixed(mtx, &mat);
        }
        gSPMatrix(dl++, K0_TO_PHYS(mtx),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPDisplayList(
            dl++,
            K0_TO_PHYS(model_data[current_model_index].header.display_list));
        scale *= 0.5f;
    }
    return dl;
}
