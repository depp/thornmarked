#include "base/pak/pak.h"

#include <ultra64.h>

OSPiHandle *rom_handle;

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
        .devAddr = offset,
        .size = size,
    };
    osEPiStartDma(rom_handle, &dma_io_message_buffer, OS_READ);
    osRecvMesg(&dma_message_queue, NULL, OS_MESG_BLOCK);
    osInvalDCache(dest, size);
}

void pak_init(int asset_count) {
    rom_handle = osCartRomInit();
    osCreateMesgQueue(&dma_message_queue, &dma_message_buffer, 1);
    uint32_t offset = (uintptr_t)_pakdata_offset;
    load_pak_data(pak_objects + 1, offset,
                  (asset_count - 1) * sizeof(*pak_objects));
    for (int i = 1; i < asset_count; i++) {
        pak_objects[i].offset += offset;
    }
}

void pak_load_asset_sync(void *dest, int asset_id) {
    struct pak_object obj = pak_objects[asset_id];
    load_pak_data(dest, obj.offset, obj.size);
}
