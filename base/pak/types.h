#pragma once

// Types for identifying game assets. In all cases, the zero ID is not a valid
// asset. These would just be enums, but using a structure prevents accidental
// implicit casts.

// Identifier for a raw data asset.
typedef struct pak_data {
    int id;
} pak_data;

// Identifier for a font asset.
typedef struct pak_font {
    int id;
} pak_font;

// Identifier for a model asset.
typedef struct pak_model {
    int id;
} pak_model;

#define MODEL_NONE ((pak_model){0})

// Identifier for a texture asset.
typedef struct pak_texture {
    int id;
} pak_texture;

#define TEXTURE_NONE ((pak_texture){0})

// Identifier for an audio track asset.
typedef struct pak_track {
    int id;
} pak_track;

#define TRACK_NONE ((pak_track){0})
