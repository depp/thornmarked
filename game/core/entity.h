#pragma once

enum {
    // Maximum number of entities. This includes entity 0, which is invalid.
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

// Entity tracking system. Tracks which entities exist, and hands out entity IDs
// for new entities.
struct sys_ent {
    // Index of the first and last entity in the freelist, or 0 if the freelist
    // is empty. The freelist itself is a singly linked list.
    unsigned short free_start, free_end;

    // If an entity is in the freelist, its entry is the next ID in the
    // freelist, or points to itself if that is the last ID in the freelist. If
    // an entity is not in the freelist, the entry is 0.
    unsigned short *entities;
};

// Initialize the entity system.
void entity_init(struct sys_ent *restrict esys);

// Create a new entity ID. Returns 0 if no IDs are available.
ent_id entity_newid(struct sys_ent *restrict esys);

// Return an entity ID to the freelist so it can be returned from entity_newid()
// again.
void entity_freeid(struct sys_ent *restrict esys, ent_id ent);

// Mark all entities as free.
void entity_freeall(struct sys_ent *restrict esys);
