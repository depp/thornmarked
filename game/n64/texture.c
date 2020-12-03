#include "game/n64/texture.h"

#include "assets/pak.h"
#include "assets/texture.h"
#include "base/base.h"
#include "base/pak/pak.h"
#include "game/defs.h"
#include "game/n64/texture_dl.h"

static uint8_t texture[4][4 * 1024] ASSET;

static void texture_load(int slot, pak_texture asset) {
    int obj_id = PAK_TEXTURE_START + asset.id - 1;
    pak_load_asset_sync(texture[slot], sizeof(texture[slot]), obj_id);
}

void texture_init(void) {
    texture_load(0, IMG_GROUND);
    texture_load(1, IMG_FAIRY);
    texture_load(2, IMG_BLUEENEMY);
    texture_load(3, IMG_GREENENEMY);
}

Gfx *texture_use(Gfx *dl, pak_texture asset_id) {
    int index;
    if (asset_id.id == IMG_GROUND.id) {
        index = 0;
    } else if (asset_id.id == IMG_FAIRY.id) {
        index = 1;
    } else if (asset_id.id == IMG_BLUEENEMY.id) {
        index = 2;
    } else if (asset_id.id == IMG_GREENENEMY.id) {
        index = 3;
    } else {
        fatal_error("Texture not loaded\nAsset ID: %d", asset_id.id);
    }
    gDPSetTextureImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture[index]);
    gSPDisplayList(dl++, texture_dl);
    return dl;
}
