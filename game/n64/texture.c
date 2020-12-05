#include "game/n64/texture.h"

#include "assets/pak.h"
#include "assets/texture.h"
#include "base/base.h"
#include "base/pak/pak.h"
#include "game/n64/defs.h"
#include "game/n64/texture_dl.h"

enum {
    // Number of texture assets which can be loaded at once.
    TEXTURE_SLOTS = 4,
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

void texture_init(void) {
    texture_load(IMG_GROUND);
    texture_load(IMG_FAIRY);
    texture_load(IMG_BLUEENEMY);
    texture_load(IMG_GREENENEMY);
}

// Current active texture.
static int texture_current;

void texture_startframe(void) {
    texture_current = 0;
}

Gfx *texture_use(Gfx *dl, pak_texture asset_id) {
    if (asset_id.id == texture_current) {
        return dl;
    }
    if (asset_id.id < 1 || PAK_TEXTURE_COUNT < asset_id.id) {
        fatal_error("Invalid texture");
    }
    int slot = texture_to_slot[asset_id.id];
    if (texture_from_slot[slot] != asset_id.id) {
        fatal_error("Texture not loaded");
    }
    gDPSetTextureImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1,
                       texture_data[slot]);
    gSPDisplayList(dl++, texture_dl);
    return dl;
}
