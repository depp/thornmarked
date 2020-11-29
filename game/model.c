#include "game/model.h"

#include "base/base.h"

enum {
    // Maximum number of objects with models.
    MAX_MODEL_OBJS = 16,
};

void model_init(struct sys_model *restrict msys) {
    msys->entities = mem_alloc(sizeof(*msys->entities) * MAX_MODEL_OBJS);
    msys->count = 0;
}

struct cp_model *model_new(struct sys_model *restrict msys) {
    if (msys->count >= MAX_MODEL_OBJS) {
        fatal_error("Too many model objects");
    }
    unsigned index = msys->count;
    msys->count++;
    struct cp_model *mp = &msys->entities[index];
    mp->model = 0;
    return mp;
}
