#include "game/core/player.h"

#include "assets/model.h"
#include "assets/texture.h"
#include "assets/track.h"
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

void player_spawn(struct game_state *restrict gs, int player_index) {
    ent_id ent = entity_newid(&gs->ent);
    if (ent.id == 0) {
        fatal_error("player_spawn: no entity");
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
        if ((gs->input.input[i].button_state & action_mask) != action ||
            (action & (action - 1)) != 0) {
            action = 0;
        }
        if (action != 0 && pl->state == PSTATE_INIT) {
            pl->state = PSTATE_ATTACK;
            pl->state_time = 0.0f;
            vec2 ppos = vec2_madd(pp->pos, pp->forward, 0.5f);
            struct cp_phys *target =
                physics_find(&gs->physics, pl->ent, ppos, 1.0f);
            if (target != NULL && target->team == TEAM_MONSTER) {
                bool ok = monster_damage(gs, target->ent, action);
                if (!ok) {
                    sfx_play(&gs->sfx, &(struct sfx_src){
                                           .track_id = SFX_CLANG,
                                           .volume = 1.0f,
                                       });
                }
            }
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
