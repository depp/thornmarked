#pragma once

enum {
    // Maximum number of entities.
    ENTITY_COUNT = 16,

    // Maximum number of supported players.
    PLAYER_COUNT = 2,
};

// An entity ID. The ID must be in the range 0..ENTITY_COUNT-1. The value 0
// refers to a destroyed entity.
typedef struct ent_id {
    int id;
} ent_id;

#define ENTITY_DESTROY ((ent_id){0})
