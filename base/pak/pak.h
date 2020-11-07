// Asset loading.
#pragma once

#include <stdint.h>

#include <ultra64.h>

// Descriptor for an object in the pak.
struct pak_object {
    uint32_t offset;
    uint32_t size;
};

// Asset table. This is a 1-based array. This must be defined in the client
// somewhere in order to have the correct size.
extern struct pak_object pak_objects[];

// Initialize the asset loader.
void pak_init(int asset_count);

// Synchronously load the asset with the given ID.
void pak_load_asset_sync(void *dest, int asset_id);
