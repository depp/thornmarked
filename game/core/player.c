#include "game/core/player.h"

#include "base/base.h"
#include "game/core/game.h"

#include <stddef.h>

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
    for (int i = 0; i < PLAYER_COUNT; i++) {
        struct cp_player *restrict pl = &gs->player.players[i];
        if (!pl->active) {
            continue;
        }
        struct cp_walk *restrict wp = walk_get(&gs->walk, pl->ent);
        if (wp != NULL) {
            wp->drive = gs->input.input[i].joystick;
        }
    }
}
