// Asset loading.
#pragma once

#include <stdint.h>

#include <ultra64.h>

// Handle to access ROM data, from osCartRomInit.
extern OSPiHandle *rom_handle;

// Descriptor for an object in the pak.
struct pak_object {
    uint32_t offset;
    uint32_t size;
};

// Asset table. This must be defined in the client somewhere in order to have
// the correct size.
extern struct pak_object pak_objects[];

// Initialize the asset loader.
void pak_init(int asset_count);

// Synchronously load the asset with the given ID. The destsize argument is just
// the size of the buffer pointed by dest, and it is used for checking that the
// transfer is valid. If the asset is larger than destsize, this will abort.
void pak_load_asset_sync(void *dest, size_t destsize, int asset_id);
