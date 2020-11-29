#pragma once

// Model component. Used for entities that have a 3D model.
struct cp_model {
    // The model asset index. 0 means "no model".
    int model;
};

// Model system.
struct sys_model {
    struct cp_model *entities;
    unsigned count;
};

// Initialize model system.
void model_init(struct sys_model *restrict msys);

// Create a new model component.
struct cp_model *model_new(struct sys_model *restrict msys);
