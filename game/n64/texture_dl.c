#include "game/n64/texture_dl.h"

const Gfx texture_dl[] = {
    gsDPPipeSync(),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetCycleType(G_CYC_2CYCLE),
    gsDPSetRenderMode(G_RM_PASS, G_RM_ZB_OPA_SURF2),
    gsSPTexture(0x8000, 0x8000, 5, 0, G_ON),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetPrimColor(0, 0, 255, 255, 255, 255),
    gsDPSetTextureLOD(G_TL_LOD),
    gsDPSetTextureFilter(G_TF_BILERP),

    // Load data into TMEM (using G_TX_LOADTILE).

    gsDPSetTile(       //
        G_IM_FMT_RGBA, // format
        G_IM_SIZ_16b,  // size
        0,             // size of one row
        0,             // tmem
        G_TX_LOADTILE, // tile
        0,             // palette
        G_TX_NOMIRROR, // cmt
        0,             // maskt
        G_TX_NOLOD,    // shiftt
        G_TX_NOMIRROR, // cms
        0,             // masks
        G_TX_NOLOD),   // shifts

    gsDPLoadSync(),

    gsDPLoadBlock(     //
        G_TX_LOADTILE, // tile
        0,             // uls
        0,             // ult
        1371,          // lrs
        0),            // dxt

    gsDPPipeSync(),

    // Tile: format, size, length of row in 8-byte units, TMEM address, tile,
    // palette, cmt, maskt, shiftt, cms, masks, shifts.
    //
    // TileSize: tile, uls, ult, lrs, lrt
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0, 0, 5, 0, 0, 5, 0),
    gsDPSetTileSize(0, 0, 0, 31 << G_TEXTURE_IMAGE_FRAC,
                    31 << G_TEXTURE_IMAGE_FRAC),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 4, 256, 1, 0, 0, 4, 1, 0, 4, 1),
    gsDPSetTileSize(1, 0, 0, 15 << G_TEXTURE_IMAGE_FRAC,
                    15 << G_TEXTURE_IMAGE_FRAC),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 2, 320, 2, 0, 0, 3, 2, 0, 3, 2),
    gsDPSetTileSize(2, 0, 0, 7 << G_TEXTURE_IMAGE_FRAC,
                    7 << G_TEXTURE_IMAGE_FRAC),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 336, 3, 0, 0, 2, 3, 0, 2, 3),
    gsDPSetTileSize(3, 0, 0, 3 << G_TEXTURE_IMAGE_FRAC,
                    3 << G_TEXTURE_IMAGE_FRAC),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 340, 4, 0, 0, 1, 4, 0, 1, 4),
    gsDPSetTileSize(4, 0, 0, 1 << G_TEXTURE_IMAGE_FRAC,
                    1 << G_TEXTURE_IMAGE_FRAC),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, 342, 5, 0, 0, 0, 5, 0, 0, 5),
    gsDPSetTileSize(5, 0, 0, 0 << G_TEXTURE_IMAGE_FRAC,
                    0 << G_TEXTURE_IMAGE_FRAC),

    gsSPEndDisplayList(),
};
