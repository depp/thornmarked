#include "game/n64/terrain.h"

#include "assets/texture.h"
#include "game/n64/graphics.h"
#include "game/n64/material.h"

// Vertex data for the ground.
#define X0 (-8)
#define Y0 (-4)
#define X1 8
#define Y1 14
#define V (1 << 7)
#define T (1 << 11)
#define Z (-V)
static const Vtx ground_vtx[] = {
    {{.ob = {X0 * V, Y0 *V, Z}, .tc = {X0 * T, -Y0 *T}}},
    {{.ob = {X1 * V, Y0 *V, Z}, .tc = {X1 * T, -Y0 *T}}},
    {{.ob = {X0 * V, Y1 *V, Z}, .tc = {X0 * T, -Y1 *T}}},
    {{.ob = {X1 * V, Y1 *V, Z}, .tc = {X1 * T, -Y1 *T}}},
};
#undef X0
#undef Y0
#undef X1
#undef Y1
#undef V
#undef T
#undef Z

// Display list to draw the ground, once the texture is loaded.
static const Gfx ground_dl[] = {
    gsSPVertex(ground_vtx, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 2, 1, 3, 0),
    gsSPEndDisplayList(),
};

void terrain_init(void) {}

Gfx *terrain_render(Gfx *dl, struct graphics *restrict gr) {
    dl = material_use(&gr->material, dl,
                      (struct material){
                          .texture_id = IMG_GROUND,
                      });
    gSPDisplayList(dl++, ground_dl);
    return dl;
}
