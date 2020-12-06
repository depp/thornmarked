#pragma once

#include "base/pak/types.h"
#include "game/core/entity.h"

// Model component. Used for entities that have a 3D model.
struct cp_model {
    ent_id ent;
    // The model asset index. 0 means "no model".
    pak_model model_id;
    // The texture to render the model with.
    pak_texture texture_id;
};

// Model system.
struct sys_model {
    struct cp_model *models;
    unsigned short *entities;
    int count;
};

// Initialize model system.
void model_init(struct sys_model *restrict msys);

// Get the model component for the given entity.
inline struct cp_model *model_get(struct sys_model *restrict msys, ent_id ent) {
    int index = msys->entities[ent.id];
    struct cp_model *mp = &msys->models[index];
    return mp->ent.id == ent.id && index < msys->count ? mp : 0;
}

// Create a new model component for the given entity. Overwrite any existing
// model component.
struct cp_model *model_new(struct sys_model *restrict msys, ent_id ent);
