#include "base/pak/pak.h"

#include "base/base.h"

#include <ultra64.h>

OSPiHandle *rom_handle;

static OSMesgQueue dma_message_queue;
static OSMesg dma_message_buffer;
static OSIoMesg dma_io_message_buffer;

// Offset in cartridge where data is stored.
extern uint8_t _pakdata_offset[];

void pak_load_data_sync(void *dest, uint32_t offset, uint32_t size) {
    osWritebackDCache(dest, size);
    osInvalDCache(dest, size);
    dma_io_message_buffer = (OSIoMesg){
        .hdr =
            {
                .pri = OS_MESG_PRI_NORMAL,
                .retQueue = &dma_message_queue,
            },
        .dramAddr = dest,
        .devAddr = offset,
        .size = size,
    };
    osEPiStartDma(rom_handle, &dma_io_message_buffer, OS_READ);
    osRecvMesg(&dma_message_queue, NULL, OS_MESG_BLOCK);
    osInvalDCache(dest, size);
}

void pak_init(unsigned asset_count) {
    rom_handle = osCartRomInit();
    osCreateMesgQueue(&dma_message_queue, &dma_message_buffer, 1);
    if (asset_count > 0) {
        uint32_t offset = (uintptr_t)_pakdata_offset;
        pak_load_data_sync(pak_objects + 1, offset,
                           (asset_count - 1) * sizeof(*pak_objects));
        for (unsigned i = 1; i < asset_count; i++) {
            pak_objects[i].offset += offset;
        }
    }
}

void pak_load_asset_sync(void *dest, size_t destsize, int asset_id) {
    struct pak_object obj = pak_objects[asset_id];
    if (obj.size > destsize) {
        fatal_error(
            "Buffer too small to load asset\n\n"
            "Asset ID: %d\nAsset size: %lu\nDest size = %zu\nDest = %p\n",
            asset_id, obj.size, destsize, dest);
    }
    pak_load_data_sync(dest, obj.offset, obj.size);
}

int pak_get_region(void) {
    uint8_t header[32];
    uint8_t *ptr = header + ((~(uintptr_t)header + 1) & 15);
    pak_load_data_sync(ptr, 0x30, 16);
    return ptr[0xe];
}
