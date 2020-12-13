#include "game/core/stage.h"

#include "assets/model.h"
#include "assets/texture.h"
#include "game/core/game.h"
#include "game/core/spawn.h"

enum {
    // Number of monsters to spawn.
    MONSTER_SPAWN_COUNT = 5,
};

// Initialize stage state.
void stage_init(struct sys_stage *restrict ssys) {
    ssys->spawn_active = false;
}

// Update the stage state.
void stage_update(struct game_state *restrict gs, float dt) {
    struct sys_stage *ssys = &gs->stage;
    if (ssys->spawn_active) {
        ssys->spawn_time -= dt;
        if (ssys->spawn_time < 0.0f) {
            ssys->spawn_active = false;
            ssys->spawn_time = 0.0f;
            int n = MONSTER_SPAWN_COUNT - gs->monster.count;
            if (n > 0) {
                pak_texture texture =
                    ssys->spawn_type ? IMG_BLUEENEMY : IMG_GREENENEMY;
                pak_model model =
                    ssys->spawn_type ? MODEL_BLUEENEMY : MODEL_GREENENEMY;
                for (int i = 0; i < n; i++) {
                    spawn_monster(gs, model, texture);
                }
            }
        }
    } else if (gs->monster.count < MONSTER_SPAWN_COUNT) {
        ssys->spawn_active = true;
        ssys->spawn_time = 3.0f;
    }
}
