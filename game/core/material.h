// Material definitions.
#pragma once

enum {
    // Number of material slots.
    MAT_SLOT_COUNT = 4,
};

// Material flags.
enum {
    MAT_ENABLED = 01,
    MAT_CULL_BACK = 02,
    MAT_VERTEX_COLOR = 04,
};

// A rendering material.
struct material {
    unsigned flags;
    pak_texture texture_id;
};
