#pragma once

#include "base/vectypes.h"
#include "game/core/camera.h"
#include "game/core/input.h"
#include "game/core/model.h"
#include "game/core/monster.h"
#include "game/core/particle.h"
#include "game/core/physics.h"
#include "game/core/player.h"
#include "game/core/time.h"
#include "game/core/walk.h"

#include <stdbool.h>

struct graphics;

// Complete state of the game. The core game does not use globals.
struct game_state {
    // Entity freelist.
    struct sys_ent ent;

    // Entity components.
    struct sys_phys physics;
    struct sys_walk walk;
    struct sys_model model;
    struct sys_monster monster;
    struct sys_player player;

    // Other systems.
    struct sys_input input;
    struct sys_camera camera;
    struct sys_particle particle;
    struct game_time time;

    bool show_console;

    // Monster spawn state.
    int spawn_active; // True if spawn_time is counting down.
    float spawn_time; // Time remaining until spawn.
    int spawn_type; // Type of monsters to spawn.
};

// Initialize the game state.
void game_init(struct game_state *restrict gs);

// Advance the game state.
void game_update(struct game_state *restrict gs, float dt);

// Destroy the named entity.
void entity_destroy(struct game_state *restrict gs, ent_id ent);
