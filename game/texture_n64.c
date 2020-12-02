#include "game/texture_n64.h"

#include "assets/assets.h"
#include "base/base.h"
#include "base/pak/pak.h"
#include "game/defs.h"
#include "game/texture_n64_dl.h"

static uint8_t texture[2][4 * 1024] ASSET;

void texture_init(void) {
    pak_load_asset_sync(texture[0], sizeof(texture[0]), IMG_GROUND);
    pak_load_asset_sync(texture[1], sizeof(texture[1]), IMG_FAIRY);
}

Gfx *texture_use(Gfx *dl, int asset_id) {
    int index;
    switch (asset_id) {
    case IMG_GROUND:
        index = 0;
        break;
    case IMG_FAIRY:
        index = 1;
        break;
    default:
        fatal_error("Texture not loaded\nAsset ID: %d", asset_id);
    }
    gDPSetTextureImage(dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture[index]);
    gSPDisplayList(dl++, texture_dl);
    return dl;
}
