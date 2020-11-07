#include "base/pak/pak.h"

#include <ultra64.h>

// Handle to access ROM data, from osCartRomInit.
static OSPiHandle *rom_handle;

static OSMesgQueue dma_message_queue;
static OSMesg dma_message_buffer;
static OSIoMesg dma_io_message_buffer;

// Offset in cartridge where data is stored.
extern uint8_t _pakdata_offset[];

static void load_pak_data(void *dest, uint32_t offset, uint32_t size) {
    osWritebackDCache(dest, size);
    osInvalDCache(dest, size);
    dma_io_message_buffer = (OSIoMesg){
        .hdr =
            {
                .pri = OS_MESG_PRI_NORMAL,
                .retQueue = &dma_message_queue,
            },
        .dramAddr = dest,
        .devAddr = (uint32_t)_pakdata_offset + offset,
        .size = size,
    };
    osEPiStartDma(rom_handle, &dma_io_message_buffer, OS_READ);
    osRecvMesg(&dma_message_queue, NULL, OS_MESG_BLOCK);
    osInvalDCache(dest, size);
}

void pak_init(int asset_count) {
    rom_handle = osCartRomInit();
    osCreateMesgQueue(&dma_message_queue, &dma_message_buffer, 1);
    load_pak_data(pak_objects, 0, asset_count * sizeof(*pak_objects));
}

void pak_load_asset_sync(void *dest, int asset_id) {
    struct pak_object obj = pak_objects[asset_id - 1];
    load_pak_data(dest, obj.offset, obj.size);
}
