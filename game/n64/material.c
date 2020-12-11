#include "game/n64/material.h"

#include "base/base.h"
#include "game/n64/texture.h"

// RDP modes.
enum {
    RDP_NONE,
    RDP_FLAT,
    RDP_SHADE,
    RDP_MIPMAP_FLAT,
    RDP_MIPMAP_SHADE,
    RDP_PARTICLE,
};

Gfx *material_use(struct material_state *restrict mst, Gfx *dl,
                  struct material mat) {
    // Set the RSP mode first.
    unsigned rsp_mode = G_ZBUFFER;
    if ((mat.flags & MAT_CULL_BACK) != 0) {
        rsp_mode |= G_CULL_BACK;
    }
    if ((mat.flags & MAT_VERTEX_COLOR) != 0) {
        rsp_mode |= G_SHADE | G_SHADING_SMOOTH;
    }
    if (rsp_mode != mst->rsp_mode) {
        gSPGeometryMode(dl++, ~rsp_mode, rsp_mode);
        mst->rsp_mode = rsp_mode;
    }

    // Load texture.
    int rdp_mode;
    if (mat.texture_id.id == 0) {
        if (mst->texture_active) {
            gSPTexture(dl++, 0, 0, 0, 0, G_OFF);
            mst->texture_active = false;
        }
        if ((mat.flags & MAT_VERTEX_COLOR) != 0) {
            rdp_mode = RDP_SHADE;
        } else {
            rdp_mode = RDP_FLAT;
        }
    } else {
        if (mat.texture_id.id != mst->texture_id.id) {
            dl = texture_use(dl, mat.texture_id);
            mst->texture_id = mat.texture_id;
        }
        if (!mst->texture_active) {
            gSPTexture(dl++, 0x8000, 0x8000, 5, 0, G_ON);
            mst->texture_active = true;
        }
        if ((mat.flags & MAT_VERTEX_COLOR) != 0) {
            rdp_mode = RDP_MIPMAP_SHADE;
        } else if ((mat.flags & MAT_PARTICLE)) {
            rdp_mode = RDP_PARTICLE;
        } else {
            rdp_mode = RDP_MIPMAP_FLAT;
        }
    }

    // Set the RDP mode.
    if (rdp_mode != mst->rdp_mode) {
        gDPPipeSync(dl++);
        switch (rdp_mode) {
        case RDP_FLAT:
            gDPSetCycleType(dl++, G_CYC_1CYCLE);
            gDPSetRenderMode(dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);
            gDPSetCombineMode(dl++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
            gDPSetTexturePersp(dl++, G_TP_NONE);
            break;
        case RDP_SHADE:
            gDPSetCycleType(dl++, G_CYC_1CYCLE);
            gDPSetRenderMode(dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);
            gDPSetCombineMode(dl++, G_CC_SHADE, G_CC_SHADE);
            gDPSetTexturePersp(dl++, G_TP_NONE);
            break;
        case RDP_MIPMAP_FLAT:
            gDPSetCycleType(dl++, G_CYC_2CYCLE);
            gDPSetRenderMode(dl++, G_RM_PASS, G_RM_ZB_OPA_SURF2);
            gDPSetCombineMode(dl++, G_CC_TRILERP, G_CC_MODULATERGB_PRIM2);
            gDPSetTexturePersp(dl++, G_TP_PERSP);
            break;
        case RDP_MIPMAP_SHADE:
            gDPSetCycleType(dl++, G_CYC_2CYCLE);
            gDPSetRenderMode(dl++, G_RM_PASS, G_RM_ZB_OPA_SURF2);
            gDPSetCombineMode(dl++, G_CC_TRILERP, G_CC_MODULATERGB2);
            gDPSetTexturePersp(dl++, G_TP_PERSP);
            break;
        case RDP_PARTICLE:
            gDPSetCycleType(dl++, G_CYC_2CYCLE);
            gDPSetRenderMode(dl++, G_RM_PASS, G_RM_ZB_XLU_SURF2);
            gDPSetCombineMode(dl++, G_CC_TRILERP, G_CC_MODULATERGBA_PRIM2);
            gDPSetTexturePersp(dl++, G_TP_PERSP);
            break;
        default:
            fatal_error("Unknown RDP mode\nMode: %d", rdp_mode);
        }
        mst->rdp_mode = rdp_mode;
    }

    return dl;
}
