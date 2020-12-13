#include "game/core/stage.h"

#include "assets/model.h"
#include "assets/texture.h"
#include "base/base.h"
#include "game/core/game.h"

enum {
    // Number of monsters to spawn.
    MONSTER_SPAWN_COUNT = 5,
};

void stage_init(struct sys_stage *restrict ssys) {
    ssys->spawn_active = false;
}

void stage_stop(struct game_state *restrict gs) {
    entity_destroyall(gs);
    gs->stage = (struct sys_stage){
        .active = false,
    };
}

void stage_start(struct game_state *restrict gs, int player_count) {
    if (player_count < 1 || gs->input.count < player_count) {
        fatal_error("Bad player count: %d", player_count);
    }
    entity_destroyall(gs);
    gs->stage = (struct sys_stage){
        .active = true,
    };
    for (int i = 0; i < player_count; i++) {
        player_spawn(gs, i);
    }
}

void stage_update(struct game_state *restrict gs, float dt) {
    if (!gs->stage.active) {
        return;
    }
    struct sys_stage *ssys = &gs->stage;
    if (ssys->spawn_active) {
        ssys->spawn_time -= dt;
        if (ssys->spawn_time < 0.0f) {
            ssys->spawn_active = false;
            ssys->spawn_time = 0.0f;
            int n = MONSTER_SPAWN_COUNT - gs->monster.count;
            if (n > 0) {
                monster_type type =
                    ssys->spawn_type ? MONSTER_BLUE : MONSTER_GREEN;
                for (int i = 0; i < n; i++) {
                    monster_spawn(gs, type);
                }
                ssys->spawn_type ^= 1;
            }
        }
    } else if (gs->monster.count < MONSTER_SPAWN_COUNT) {
        ssys->spawn_active = true;
        ssys->spawn_time = 1.5f;
    }
}
