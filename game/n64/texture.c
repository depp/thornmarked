#include "game/n64/texture.h"

#include "assets/pak.h"
#include "assets/texture.h"
#include "base/base.h"
#include "base/pak/pak.h"
#include "game/n64/defs.h"
#include "game/n64/texture_dl/dl.h"

enum {
    // Number of texture assets which can be loaded at once.
    TEXTURE_SLOTS = 8,
};

struct texture_slot {
    int display_list;
    unsigned pad;
    uint8_t data[4 * 1024];
};

// Loaded texture data.
static struct texture_slot texture_data[TEXTURE_SLOTS] ASSET;

// Map from texture asset ID to slot number. If the slot maps back to the same
// texture asset, then this texture is loaded, otherwise it is not loaded.
static int texture_to_slot[PAK_TEXTURE_COUNT + 1];

// Map from texture slot number to texture asset ID.
static int texture_from_slot[TEXTURE_SLOTS];

// Load a texture into the given slot. Ignores any texture in the slot, and does
// not check if the texture is already loaded into another slot.
static void texture_load_slot(pak_texture asset, int slot) {
    pak_load_asset_sync(&texture_data[slot], sizeof(texture_data[slot]),
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

void texture_init(void) {
    texture_load(IMG_GROUND1);
    texture_load(IMG_GROUND2);
    texture_load(IMG_FAIRY1);
    texture_load(IMG_FAIRY2);
    texture_load(IMG_BLUEENEMY);
    texture_load(IMG_GREENENEMY);
    texture_load(IMG_STAR1);
}

Gfx *texture_use(Gfx *dl, pak_texture texture_id) {
    if (texture_id.id < 1 || PAK_TEXTURE_COUNT < texture_id.id) {
        fatal_error("Invalid texture");
    }
    int slot = texture_to_slot[texture_id.id];
    if (texture_from_slot[slot] != texture_id.id) {
        fatal_error("Texture not loaded");
    }
    const struct texture_slot *tex = &texture_data[slot];
    gDPSetTextureImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, tex->data);
    const Gfx *tex_dl = NULL;
    if ((size_t)tex->display_list < ARRAY_COUNT(texture_dls)) {
        tex_dl = texture_dls[tex->display_list];
    }
    if (tex_dl == NULL) {
        fatal_error("Unknown texture type\nType: %d", tex->display_list);
    }
    gSPDisplayList(dl++, K0_TO_PHYS(tex_dl));
    return dl;
}
