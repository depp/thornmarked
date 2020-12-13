#include "game/core/spawn.h"

#include "assets/model.h"
#include "assets/texture.h"
#include "base/base.h"
#include "game/core/game.h"
#include "game/core/random.h"

void spawn_player(struct game_state *restrict gs, int player_index) {
    ent_id ent = entity_newid(&gs->ent);
    if (ent.id == 0) {
        fatal_error("spawn_player: no entity");
    }
    struct cp_phys *pp = physics_new(&gs->physics, ent);
    pp->radius = 0.75f;
    pp->height = -1.0f;
    pp->team = TEAM_PLAYER;
    walk_new(&gs->walk, ent);
    struct cp_model *mp = model_new(&gs->model, ent);
    pak_texture texture = player_index == 0 ? IMG_FAIRY1 : IMG_FAIRY2;
    mp->model_id = MODEL_FAIRY;
    mp->material[0] = (struct material){
        .flags = MAT_ENABLED | MAT_CULL_BACK | MAT_VERTEX_COLOR,
        .texture_id = texture,
    };
    mp->material[1] = (struct material){
        .flags = MAT_ENABLED | MAT_VERTEX_COLOR,
        .texture_id = texture,
    };
    mp->animation_id = 4;
    player_new(&gs->player, player_index, ent);
}

void spawn_monster(struct game_state *restrict gs, pak_model model,
                   pak_texture texture) {
    ent_id ent = entity_newid(&gs->ent);
    if (ent.id == 0) {
        fatal_error("spawn_monster: no entity");
    }
    struct cp_phys *pp = physics_new(&gs->physics, ent);
    const float radius = 0.75f, wall = 4.0f, r = wall - radius;
    pp->pos = (vec2){{
        rand_frange(&grand, -r, r),
        rand_frange(&grand, -r, r),
    }};
    pp->radius = radius;
    pp->team = TEAM_MONSTER;
    walk_new(&gs->walk, ent);
    struct cp_model *mp = model_new(&gs->model, ent);
    mp->model_id = model;
    mp->material[0] = (struct material){
        .flags = MAT_ENABLED | MAT_CULL_BACK | MAT_VERTEX_COLOR,
        .texture_id = texture,
    };
    monster_new(&gs->monster, ent);
}
