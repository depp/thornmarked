#include "game/core/entity.h"

#include "base/base.h"

#include <stdnoreturn.h>

// I mean... it's only sensible.
static_assert(ENTITY_COUNT >= 2);

void entity_init(struct sys_ent *restrict esys) {
    *esys = (struct sys_ent){
        .free_start = 1,
        .free_end = ENTITY_COUNT - 1,
        .entities = mem_alloc(ENTITY_COUNT * sizeof(*esys->entities)),
    };
    // Entity 0 is invalid, and not in the freelist.
    esys->entities[0] = 0;
    // Entities 1..(N-2) are in the freelist, pointing to the next entity.
    for (int i = 0; i < ENTITY_COUNT - 1; i++) {
        esys->entities[i] = i + 1;
    }
    // Entity N-1 is at the end of the freelist, points to itself.
    esys->entities[ENTITY_COUNT - 1] = ENTITY_COUNT - 1;
}

ent_id entity_newid(struct sys_ent *restrict esys) {
    int id = esys->free_start;
    if (id != 0) {
        // Remove from freelist.
        int next = esys->entities[id];
        esys->entities[id] = 0;
        if (next == id) {
            // Last entity in freelist.
            esys->free_start = 0;
            esys->free_end = 0;
        } else {
            // Update freelist.
            esys->free_start = next;
        }
    }
    return (ent_id){id};
}

static noreturn void entity_err(const char *msg, ent_id ent) {
    fatal_error("entity_freeid: %s\nent: %d", msg, ent.id);
}

void entity_freeid(struct sys_ent *restrict esys, ent_id ent) {
    // Sanity check.
    if (ent.id <= 0 || ENTITY_COUNT <= ent.id) {
        entity_err("invalid ent", ent);
    }
    // Check that entity is not already in freelist.
    int next = esys->entities[ent.id];
    if (next != 0) {
        entity_err("double free", ent);
    }
    // Add entity to end of freelist.
    esys->entities[ent.id] = ent.id;
    if (esys->free_end == 0) {
        esys->free_start = ent.id;
    } else {
        esys->entities[esys->free_end] = ent.id;
    }
    esys->free_end = ent.id;
}
