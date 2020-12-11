#include "game/core/player.h"

#include "base/base.h"
#include "base/vec2.h"
#include "base/vec3.h"
#include "game/core/game.h"

#include <stddef.h>

// Player animations. Corresponds to the order of animations in the player
// model.
enum {
    PANIM_NONE,
    PANIM_ATTACK,
    PANIM_DIE,
    PANIM_HIT,
    PANIM_IDLE,
};

// Player states.
enum {
    PSTATE_INIT,
    PSTATE_ATTACK,
};

void player_init(struct sys_player *restrict psys) {
    (void)psys;
}

struct cp_player *player_new(struct sys_player *restrict msys, int player_index,
                             ent_id ent) {
    if (player_index < 0 || PLAYER_COUNT <= player_index) {
        fatal_error("Bad player index");
    }
    for (int i = 0; i < PLAYER_COUNT; i++) {
        if (msys->players[i].active && msys->players[i].ent.id == ent.id) {
            msys->players[i].active = false;
        }
    }
    struct cp_player *pl = &msys->players[player_index];
    *pl = (struct cp_player){
        .ent = ent,
        .active = true,
    };
    return pl;
}

void player_update(struct game_state *restrict gs, float dt) {
    (void)dt;
    const float idle_speed = 2.0f;
    const float attack_speed = 4.0f;
    for (int i = 0; i < PLAYER_COUNT; i++) {
        struct cp_player *restrict pl = &gs->player.players[i];
        if (!pl->active) {
            continue;
        }
        struct cp_phys *restrict pp = physics_require(&gs->physics, pl->ent);

        // Advance player state.
        switch (pl->state) {
        case PSTATE_INIT:
            break;
        case PSTATE_ATTACK:
            pl->state_time += dt * attack_speed;
            if (pl->state_time >= 1.0f) {
                pl->state = PSTATE_INIT;
            }
            break;
        }

        // Read button inputs.
        const unsigned action_mask = BUTTON_A | BUTTON_B;
        unsigned action = gs->input.input[i].button_press & action_mask;
        // Action fails if multiple buttons are pressed.
        if ((gs->input.input[i].button_state & action_mask) != action) {
            action = -1;
        }
        switch (action) {
        case BUTTON_A:
            if (pl->state == PSTATE_INIT) {
                pl->state = PSTATE_ATTACK;
                pl->state_time = 0.0f;
                vec3 ppos =
                    vec3_vec2(vec2_madd(pp->pos, pp->forward, 1.0f), 2.0f);
                particle_create(&gs->particle, ppos, 1.0f,
                                (color){{255, 0, 0, 255}});
            }
            break;
        }

        // Move player.
        struct cp_walk *restrict wp = walk_get(&gs->walk, pl->ent);
        if (wp != NULL) {
            wp->drive = gs->input.input[i].joystick;
        }

        // Update animation.
        struct cp_model *restrict mp = model_get(&gs->model, pl->ent);
        if (mp != NULL) {
            switch (pl->state) {
            case PSTATE_ATTACK:
                mp->animation_id = PANIM_ATTACK;
                mp->animation_time = pl->state_time;
                break;
            default:
                if (mp->animation_id != PANIM_IDLE) {
                    mp->animation_id = PANIM_IDLE;
                    mp->animation_time = 0.0f;
                } else {
                    mp->animation_time += dt * idle_speed;
                    if (mp->animation_time > 1.0f) {
                        mp->animation_time -= 1.0f;
                    }
                }
                break;
            }
        }
    }
}
