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

enum {
    // Number of model assets which can be loaded at once.
    MODEL_SLOTS = 3,
};

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

// Model data, including buffer.
union model_data {
    struct model_header header;
    uint8_t data[200 * 1024];
};

// Loaded models.
static union model_data model_data[MODEL_SLOTS] ASSET;

// Map from model asset ID to slot number. If the slot maps back to the same
// model asset, then this texture is loaded, otherwise it is not loaded.
static int model_to_slot[PAK_MODEL_COUNT + 1];

// Map from model slot number to model asset ID.
static int model_from_slot[MODEL_SLOTS];

static void *pointer_fixup(void *ptr, uintptr_t base, size_t size) {
    uintptr_t value = (uintptr_t)ptr;
    if (value > size) {
        fatal_error("Bad pointer in asset\nPointer: %p\nBase: %p\nSize: %zu",
                    ptr, (void *)base, size);
    }
    return (void *)(value + base);
}

// Fix the internal pointers in a model after loading.
static void model_fixup(union model_data *p) {
    const uintptr_t base = (uintptr_t)p;
    const size_t size = sizeof(union model_data);
    struct model_header *restrict hdr = &p->header;
    hdr->vertex_data = pointer_fixup(hdr->vertex_data, base, size);
    hdr->display_list = pointer_fixup(hdr->display_list, base, size);
    for (int i = 0; i < hdr->animation_count; i++) {
        struct model_animation *restrict anim = &hdr->animation[i];
        if (anim->frame_count > 0) {
            struct model_frame *restrict frame =
                pointer_fixup(hdr->animation[i].frame, base, size);
            anim->frame = frame;
            for (int j = 0; j < anim->frame_count; j++) {
                frame[j].vertex = pointer_fixup(frame[j].vertex, base, size);
            }
        }
    }
}

// Load a model into the given slot.
static void model_load_slot(pak_model asset, int slot) {
    pak_load_asset_sync(&model_data[slot], sizeof(model_data[slot]),
                        PAK_MODEL_START + asset.id - 1);
    model_fixup(&model_data[slot]);
    model_to_slot[asset.id] = slot;
    model_from_slot[slot] = asset.id;
}

// Load a model and return the slot.
static int model_load(pak_model asset) {
    if (asset.id < 1 || PAK_MODEL_COUNT < asset.id) {
        fatal_error("model_load: invalid model\nModel: %d", asset.id);
    }
    int slot = model_to_slot[asset.id];
    if (model_from_slot[slot] == asset.id) {
        return slot;
    }
    for (slot = 0; slot < MODEL_SLOTS; slot++) {
        if (model_from_slot[slot] == 0) {
            model_load_slot(asset, slot);
            return slot;
        }
    }
    fatal_error("model_load: no slots available");
}

void model_render_init(void) {
    model_load(MODEL_FAIRY);
    model_load(MODEL_BLUEENEMY);
    model_load(MODEL_GREENENEMY);
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
    int current_model = 0;
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
        int slot = model_to_slot[model];
        if (model_from_slot[slot] != model) {
            fatal_error("Model not loaded");
        }
        if (model != current_model) {
            if (model == MODEL_FAIRY.id) {
                gSPDisplayList(dl++, fairy_setup_dl);
                dl = texture_use(dl, IMG_FAIRY);
                gSPSegment(dl++, 1,
                           K0_TO_PHYS(model_data[slot]
                                          .header.animation[anim_id]
                                          .frame[frame_id]
                                          .vertex));
            } else if (model == MODEL_BLUEENEMY.id) {
                gSPDisplayList(dl++, fairy_setup_dl);
                dl = texture_use(dl, IMG_BLUEENEMY);
                gSPSegment(dl++, 1,
                           K0_TO_PHYS(model_data[slot].header.vertex_data));
            } else if (model == MODEL_GREENENEMY.id) {
                gSPDisplayList(dl++, fairy_setup_dl);
                dl = texture_use(dl, IMG_GREENENEMY);
                gSPSegment(dl++, 1,
                           K0_TO_PHYS(model_data[slot].header.vertex_data));
            } else {
                continue;
            }
        }
        current_model = model;
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
        gSPDisplayList(dl++, K0_TO_PHYS(model_data[slot].header.display_list));
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
