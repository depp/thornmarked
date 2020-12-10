#include "game/n64/material.h"

#include "assets/pak.h"
#include "assets/texture.h"
#include "base/base.h"
#include "base/pak/pak.h"
#include "game/n64/defs.h"
#include "game/n64/texture_dl.h"

enum {
    // Number of texture assets which can be loaded at once.
    TEXTURE_SLOTS = 8,
};

// Loaded texture data.
static uint8_t texture_data[TEXTURE_SLOTS][4 * 1024] ASSET;

// Map from texture asset ID to slot number. If the slot maps back to the same
// texture asset, then this texture is loaded, otherwise it is not loaded.
static int texture_to_slot[PAK_TEXTURE_COUNT + 1];

// Map from texture slot number to texture asset ID.
static int texture_from_slot[TEXTURE_SLOTS];

// Load a texture into the given slot. Ignores any texture in the slot, and does
// not check if the texture is already loaded into another slot.
static void texture_load_slot(pak_texture asset, int slot) {
    pak_load_asset_sync(texture_data[slot], sizeof(texture_data[slot]),
                        pak_texture_object(asset));
    texture_to_slot[asset.id] = slot;
    texture_from_slot[slot] = asset.id;
}

// Load a texture, and return the slot.
static int texture_load(pak_texture asset) {
    if (asset.id < 1 || PAK_TEXTURE_COUNT < asset.id) {
        fatal_error("texture_load: invalid texture\nTexture: %d", asset.id);
    }
    int slot = texture_to_slot[asset.id];
    if (texture_from_slot[slot] == asset.id) {
        return slot;
    }
    for (slot = 0; slot < TEXTURE_SLOTS; slot++) {
        if (texture_from_slot[slot] == 0) {
            texture_load_slot(asset, slot);
            return slot;
        }
    }
    fatal_error("texture_load: no slots available");
}

void material_init(void) {
    texture_load(IMG_GROUND);
    texture_load(IMG_FAIRY1);
    texture_load(IMG_FAIRY2);
    texture_load(IMG_BLUEENEMY);
    texture_load(IMG_GREENENEMY);
}

static Gfx *texture_use(Gfx *dl, pak_texture texture_id) {
    if (texture_id.id < 1 || PAK_TEXTURE_COUNT < texture_id.id) {
        fatal_error("Invalid texture");
    }
    int slot = texture_to_slot[texture_id.id];
    if (texture_from_slot[slot] != texture_id.id) {
        fatal_error("Texture not loaded");
    }
    gDPSetTextureImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1,
                       texture_data[slot]);
    gSPDisplayList(dl++, texture_dl);
    return dl;
}

// RDP modes.
enum {
    RDP_NONE,
    RDP_FLAT,
    RDP_SHADE,
    RDP_MIPMAP_FLAT,
    RDP_MIPMAP_SHADE,
};

Gfx *material_use(struct material_state *restrict mst, Gfx *dl,
                  struct material mat) {
    int rdp_mode;

    // Load texture.
    if (mat.texture_id.id == 0) {
        rdp_mode = (mat.flags & MAT_VERTEX_COLOR) != 0 ? RDP_SHADE : RDP_FLAT;
    } else {
        if (mat.texture_id.id != mst->texture_id.id) {
            dl = texture_use(dl, mat.texture_id);
            mst->texture_id = mat.texture_id;
        }
        rdp_mode = (mat.flags & MAT_VERTEX_COLOR) != 0 ? RDP_MIPMAP_SHADE
                                                       : RDP_MIPMAP_FLAT;
    }

    // Set the RDP mode.
    if (rdp_mode != mst->rdp_mode) {
        gDPPipeSync(dl++);
        switch (rdp_mode) {
        case RDP_FLAT:
            gDPSetCycleType(dl++, G_CYC_1CYCLE);
            gDPSetRenderMode(dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);
            gDPSetCombineMode(dl++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
            gDPSetTexturePersp(dl++, G_TP_NONE);
            break;
        case RDP_SHADE:
            gDPSetCycleType(dl++, G_CYC_1CYCLE);
            gDPSetRenderMode(dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);
            gDPSetCombineMode(dl++, G_CC_SHADE, G_CC_SHADE);
            gDPSetTexturePersp(dl++, G_TP_NONE);
            break;
        case RDP_MIPMAP_FLAT:
            gDPSetCycleType(dl++, G_CYC_2CYCLE);
            gDPSetRenderMode(dl++, G_RM_PASS, G_RM_ZB_OPA_SURF2);
            gDPSetCombineMode(dl++, G_CC_TRILERP, G_CC_PASS2);
            gDPSetTexturePersp(dl++, G_TP_PERSP);
            break;
        case RDP_MIPMAP_SHADE:
            gDPSetCycleType(dl++, G_CYC_2CYCLE);
            gDPSetRenderMode(dl++, G_RM_PASS, G_RM_ZB_OPA_SURF2);
            gDPSetCombineMode(dl++, G_CC_TRILERP, G_CC_MODULATERGB2);
            gDPSetTexturePersp(dl++, G_TP_PERSP);
            break;
        default:
            fatal_error("Unknown RDP mode\nMode: %d", rdp_mode);
        }
        mst->rdp_mode = rdp_mode;
    }

    return dl;
}
