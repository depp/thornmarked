#include "game/n64/image.h"

#include "assets/image.h"
#include "assets/pak.h"
#include "base/base.h"
#include "base/fixup.h"
#include "base/pak/pak.h"

enum {
    // Maximum number of images which can be loaded at once.
    IMAGE_SLOTS = 4,

    // Amount of memory which can be used for images.
    IMAGE_HEAPSIZE = 192 * 1024,
};

// A single rectangle of image data in a larger image.
struct image_rect {
    short x, y, xsz, ysz;
    void *pixels;
};

// Header for a strip-based image.
struct image_header {
    int rect_count;
    struct image_rect rect[];
};

// Heap for allocating images.
struct image_heap {
    uintptr_t pos;
    uintptr_t start;
    uintptr_t end;
};

// Allocate storage for an image.
static void *image_alloc(struct image_heap *restrict hp, size_t size) {
    size = (size + 15) & ~(size_t)15;
    size_t rem = hp->end - hp->pos;
    if (rem < size) {
        fatal_error("image_alloc failed");
    }
    uintptr_t ptr = hp->pos;
    hp->pos = ptr + size;
    return (void *)ptr;
}

// Image system state.
struct image_state {
    struct image_heap heap;
    struct image_header *image[IMAGE_SLOTS];
    int image_from_slot[IMAGE_SLOTS];
    int image_to_slot[PAK_IMAGE_COUNT + 1];
};

// Fix the internal pointers in an image after loading.
static void image_fixup(struct image_header *restrict img, size_t size) {
    uintptr_t base = (uintptr_t)img;
    for (int i = 0; i < img->rect_count; i++) {
        img->rect[i].pixels = pointer_fixup(img->rect[i].pixels, base, size);
    }
}

// Load an image into the given slot.
static void image_load_slot(struct image_state *restrict ist, pak_image asset,
                            int slot) {
    int obj = pak_image_object(asset);
    size_t obj_size = pak_objects[obj].size;
    struct image_header *restrict img = image_alloc(&ist->heap, obj_size);
    pak_load_asset_sync(img, obj_size, obj);
    image_fixup(img, obj_size);
    ist->image[slot] = img;
    ist->image_to_slot[asset.id] = slot;
    ist->image_from_slot[slot] = asset.id;
}

// State of the image system.
static struct image_state image_state;

// Load an image into an available slot.
static void image_load(pak_image asset) {
    struct image_state *restrict ist = &image_state;
    if (asset.id < 1 || PAK_IMAGE_COUNT < asset.id) {
        fatal_error("image_load: invalid image\nImage: %d", asset.id);
    }
    int slot = ist->image_to_slot[asset.id];
    if (ist->image_from_slot[slot] == asset.id) {
        return;
    }
    for (slot = 0; slot < IMAGE_SLOTS; slot++) {
        if (ist->image_from_slot[slot] == 0) {
            image_load_slot(ist, asset, slot);
            return;
        }
    }
    fatal_error("image_load: no slots available");
}

void image_init(void) {
    {
        struct image_state *restrict ist = &image_state;
        uintptr_t heap_base = (uintptr_t)mem_alloc(IMAGE_HEAPSIZE);
        ist->heap = (struct image_heap){
            .pos = heap_base,
            .start = heap_base,
            .end = heap_base + IMAGE_HEAPSIZE,
        };
    }
    image_load(IMG_BIG);
}

static const Gfx image_dl[] = {
    gsDPPipeSync(),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF),
    gsSPGeometryMode(~(unsigned)0, 0),
    gsSPTexture(0x2000, 0x2000, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_DECALRGBA),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetTextureFilter(G_TF_POINT),
    gsSPEndDisplayList(),
};

static Gfx *image_draw(Gfx *dl, Gfx *dl_end, pak_image asset, int x, int y) {
    (void)dl_end;
    struct image_state *restrict ist = &image_state;
    int slot = ist->image_to_slot[asset.id];
    if (ist->image_from_slot[slot] != asset.id) {
        fatal_error("image_draw: not loaded\nImage: %d", asset.id);
    }
    const struct image_header *restrict img = ist->image[slot];
    gSPDisplayList(dl++, image_dl);
    for (int i = 0; i < img->rect_count; i++) {
        struct image_rect r = img->rect[i];
        unsigned xsz = (r.xsz + 3) & ~3u;
        gDPLoadTextureBlock(dl++, r.pixels, G_IM_FMT_RGBA, G_IM_SIZ_16b, xsz,
                            r.ysz, 0, G_TX_NOMIRROR, G_TX_NOMIRROR, 0, 0,
                            G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(dl++, (x + r.x) << 2, (y + r.y) << 2,
                            (x + r.x + xsz) << 2, (y + r.y + r.ysz) << 2, 0,
                            0, 0, 1 << 10, 1 << 10);
    }
    return dl;
}

Gfx *image_render(Gfx *dl, Gfx *dl_end) {
    dl = image_draw(dl, dl_end, IMG_BIG, 16, 16);
    return dl;
}
