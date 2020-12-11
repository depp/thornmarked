#include "game/core/model.h"

#include "base/base.h"

enum {
    // Maximum number of objects with models.
    MAX_MODEL_OBJS = 16,
};

void model_init(struct sys_model *restrict msys) {
    *msys = (struct sys_model){
        .models = mem_alloc(sizeof(*msys->models) * MAX_MODEL_OBJS),
        .entities = mem_calloc(sizeof(*msys->entities) * ENTITY_COUNT),
    };
}

struct cp_model *model_get(struct sys_model *restrict msys, ent_id ent);

struct cp_model *model_new(struct sys_model *restrict msys, ent_id ent) {
    int index = msys->entities[ent.id];
    struct cp_model *mp = &msys->models[index];
    if (mp->ent.id != ent.id || index >= msys->count) {
        index = msys->count;
        if (index >= MAX_MODEL_OBJS) {
            fatal_error("Too many model objects");
        }
        msys->count = index + 1;
        mp = &msys->models[index];
        msys->entities[ent.id] = index;
    }
    *mp = (struct cp_model){
        .ent = ent,
    };
    return mp;
}

void model_update(struct sys_model *restrict msys) {
    // Clean up destroyed entities.
    struct cp_model *mstart = msys->models, *mend = mstart + msys->count;
    for (struct cp_model *mp = mstart; mp != mend; mp++) {
        if (mp->ent.id == 0) {
            do {
                mend--;
            } while (mp != mend && mend->ent.id == 0);
            if (mp == mend) {
                break;
            }
            *mp = *mend;
            msys->entities[mp->ent.id] = mp - mstart;
        }
    }
    msys->count = mend - mstart;
}
