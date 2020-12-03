#include "game/n64/model.h"

#include "assets/model.h"
#include "assets/pak.h"
#include "assets/texture.h"
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

// A frame in a model animation.
struct model_frame {
    float time;
    float inv_dt; // Inverse of time delta to next frame.
    Vtx *vertex;
};

// An animation in a model.
struct model_animation {
    float duration;
    int frame_count;
    struct model_frame *frame;
};

// Header for the model data.
struct model_header {
    Vtx *vertex_data;
    Gfx *display_list;
    int animation_count;
    struct model_animation animation[];
};

// Loaded models.
static union {
    struct model_header header;
    uint8_t data[200 * 1024];
} model_data[3] ASSET;

static void *pointer_fixup(void *ptr, uintptr_t base, size_t size) {
    uintptr_t value = (uintptr_t)ptr;
    if (value > size) {
        fatal_error("Bad pointer in asset\nPointer: %p\nBase: %p\nSize: %zu",
                    ptr, (void *)base, size);
    }
    return (void *)(value + base);
}

static void model_fixup(struct model_header *restrict p, uintptr_t base,
                        size_t size) {
    p->vertex_data = pointer_fixup(p->vertex_data, base, size);
    p->display_list = pointer_fixup(p->display_list, base, size);
    for (int i = 0; i < p->animation_count; i++) {
        struct model_animation *restrict anim = &p->animation[i];
        if (anim->frame_count > 0) {
            struct model_frame *restrict frame =
                pointer_fixup(p->animation[i].frame, base, size);
            anim->frame = frame;
            for (int j = 0; j < anim->frame_count; j++) {
                frame[j].vertex = pointer_fixup(frame[j].vertex, base, size);
            }
        }
    }
}

static void model_load(int slot, pak_model asset_id) {
    pak_load_asset_sync(model_data[slot].data, sizeof(model_data[slot].data),
                        PAK_MODEL_START + asset_id.id - 1);
    struct model_header *p = &model_data[slot].header;
    model_fixup(p, (uintptr_t)&model_data[slot], sizeof(model_data[slot]));
}

void model_render_init(void) {
    model_load(0, MODEL_FAIRY);
    model_load(1, MODEL_BLUEENEMY);
    model_load(2, MODEL_GREENENEMY);
}

static const Gfx fairy_setup_dl[] = {
    gsDPPipeSync(),
    gsSPGeometryMode(G_LIGHTING, G_CULL_BACK | G_SHADE | G_SHADING_SMOOTH),
    gsDPSetCombineMode(G_CC_TRILERP, G_CC_MODULATERGB2),
    gsSPEndDisplayList(),
};

static int anim_id, frame_id;

Gfx *model_render(Gfx *dl, struct graphics *restrict gr,
                  struct sys_model *restrict msys,
                  struct sys_phys *restrict psys) {
    int current_model = -1, current_model_index = 0;
    float scale = 0.5f;
    for (unsigned i = 0; i < psys->count; i++) {
        struct cp_phys *restrict cp = &psys->entities[i];
        if (i >= msys->count) {
            continue;
        }
        struct cp_model *restrict mp = &msys->entities[i];
        if (mp->model_id.id == 0) {
            continue;
        }
        int model = mp->model_id.id;
        if (model != current_model) {
            int index;
            if (model == MODEL_FAIRY.id) {
                index = 0;
                gSPDisplayList(dl++, fairy_setup_dl);
                dl = texture_use(dl, IMG_FAIRY);
                gSPSegment(dl++, 1,
                           K0_TO_PHYS(model_data[index]
                                          .header.animation[anim_id]
                                          .frame[frame_id]
                                          .vertex));
            } else if (model == MODEL_BLUEENEMY.id) {
                index = 1;
                gSPDisplayList(dl++, fairy_setup_dl);
                dl = texture_use(dl, IMG_BLUEENEMY);
                gSPSegment(dl++, 1,
                           K0_TO_PHYS(model_data[index].header.vertex_data));
            } else if (model == MODEL_GREENENEMY.id) {
                index = 2;
                gSPDisplayList(dl++, fairy_setup_dl);
                dl = texture_use(dl, IMG_GREENENEMY);
                gSPSegment(dl++, 1,
                           K0_TO_PHYS(model_data[index].header.vertex_data));
            } else {
                continue;
            }
            current_model_index = index;
            current_model = model;
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
    }

    frame_id++;
    if (frame_id >= model_data[0].header.animation[anim_id].frame_count) {
        frame_id = 0;
        anim_id++;
        if (anim_id >= model_data[0].header.animation_count) {
            anim_id = 0;
        }
    }

    return dl;
}
