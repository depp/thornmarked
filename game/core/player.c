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
    for (int i = 0; i < PLAYER_COUNT; i++) {
        psys->players[i].ent = ENTITY_DESTROY;
    }
}

struct cp_player *player_get(struct sys_player *restrict psys, ent_id ent) {
    for (int i = 0; i < PLAYER_COUNT; i++) {
        struct cp_player *pl = &psys->players[i];
        if (pl->ent.id == ent.id) {
            return pl;
        }
    }
    return NULL;
}

struct cp_player *player_new(struct sys_player *restrict msys, int player_index,
                             ent_id ent) {
    if (player_index < 0 || PLAYER_COUNT <= player_index) {
        fatal_error("Bad player index");
    }
    for (int i = 0; i < PLAYER_COUNT; i++) {
        if (msys->players[i].ent.id == ent.id) {
            msys->players[i].ent = ENTITY_DESTROY;
        }
    }
    struct cp_player *pl = &msys->players[player_index];
    *pl = (struct cp_player){
        .ent = ent,
    };
    return pl;
}

void player_update(struct game_state *restrict gs, float dt) {
    (void)dt;
    const float idle_speed = 2.0f;
    const float attack_speed = 4.0f;
    for (int i = 0; i < PLAYER_COUNT; i++) {
        struct cp_player *restrict pl = &gs->player.players[i];
        if (pl->ent.id == 0) {
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
                vec2 ppos = vec2_madd(pp->pos, pp->forward, 1.0f);
                struct cp_phys *target =
                    physics_find(&gs->physics, pl->ent, ppos, 0.5f);
                if (target != NULL && target->team == TEAM_MONSTER) {
                    entity_destroy(gs, target->ent);
                    particle_create(&gs->particle, vec3_vec2(target->pos, 2.0f),
                                    2.0f, (color){{255, 0, 0, 255}});
                }
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
