#include "common_types.h"
#include "imports.h"
#include "headers/PC/bgame/bg_attachments.h"
#include "headers/PC/cgame_mp/cg_attachmentsight.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_wwiiscope.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <math.h>

extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);

#define SIGHT_SEGMENTS 24
#define SIGHT_MAX_RINGS 6
/* Virtual units: the reflex dot and its glow, the lens sight's ring and its
 * centre dot, each fading out over its feather, and the halo the light of
 * each spreads into around it. */
#define SIGHT_REFLEX_RADIUS 0.8f
#define SIGHT_REFLEX_GLOW 0.6f
#define SIGHT_LENS_RING 6.5f
#define SIGHT_LENS_RING_WIDTH 0.45f
#define SIGHT_LENS_DOT 0.45f
#define SIGHT_LENS_FEATHER 0.5f
#define SIGHT_HALO 2.4f
/* The halo's brightness where it leaves the glow, of the reticle's. */
#define SIGHT_HALO_ALPHA 0.28f
/* The reticle shows over the last of the aim-in, as the sight reaches the eye. */
#define SIGHT_FADE_START 0.6f
/* Through a sight's window the reticle fades out over the outer tenth of the
 * opening, as it slips behind the ring. */
#define SIGHT_OPENING_EDGE 0.9f

static const vec4_t s_sightReflex = { 1.0f, 0.16f, 0.1f, 0.95f };
static const vec4_t s_sightLens = { 1.0f, 0.78f, 0.32f, 0.9f };

/* This frame's opening of the sight's ring, in pixels. */
typedef struct SightOpening {
    int on;
    float centre[2];
    float toUnit[4];
} SightOpening;

static SightOpening s_sightOpening;

static float CG_SightOpeningAlpha(float x, float y)
{
    float dx, dy, u, v, r;

    if (!s_sightOpening.on)
        return 1.0f;
    dx = x - s_sightOpening.centre[0];
    dy = y - s_sightOpening.centre[1];
    u = s_sightOpening.toUnit[0] * dx + s_sightOpening.toUnit[1] * dy;
    v = s_sightOpening.toUnit[2] * dx + s_sightOpening.toUnit[3] * dy;
    r = sqrtf(u * u + v * v);
    if (r <= SIGHT_OPENING_EDGE)
        return 1.0f;
    if (!(r < 1.0f))
        return 0.0f;
    r = (1.0f - r) / (1.0f - SIGHT_OPENING_EDGE);
    return r * r * (3.0f - 2.0f * r);
}

static void CG_SightVertex(HudVertex2D *vertex, float x, float y, const vec_t *color, float alpha)
{
    vertex->xy[0] = x;
    vertex->xy[1] = y;
    vertex->st[0] = 0.5f;
    vertex->st[1] = 0.5f;
    vertex->rgba[0] = color[0];
    vertex->rgba[1] = color[1];
    vertex->rgba[2] = color[2];
    vertex->rgba[3] = color[3] * alpha * CG_SightOpeningAlpha(x, y);
}

/* Rings of vertices at the radii in pixels, each with its alpha, the first
 * ring of radius 0 being a single centre vertex; triangles, all wound alike,
 * join neighbours. */
static void CG_DrawSightRings(float x, float y, const float *radii, const float *alphas, int rings,
                              const vec_t *color, float alpha)
{
    HudVertex2D verts[SIGHT_MAX_RINGS * SIGHT_SEGMENTS];
    unsigned short indices[(6 * SIGHT_MAX_RINGS - 3) * SIGHT_SEGMENTS];
    int centre = radii[0] <= 0.0f, vertexCount = 0, indexCount = 0, ring, i;

    if (centre)
        CG_SightVertex(&verts[vertexCount++], x, y, color, alpha * alphas[0]);
    for (ring = centre; ring < rings; ring++) {
        for (i = 0; i < SIGHT_SEGMENTS; i++) {
            float angle = 6.28318531f * (float)i / (float)SIGHT_SEGMENTS;

            CG_SightVertex(&verts[vertexCount++], x + cosf(angle) * radii[ring], y + sinf(angle) * radii[ring], color,
                           alpha * alphas[ring]);
        }
    }
    for (i = 0; i < SIGHT_SEGMENTS; i++) {
        int next = (i + 1) % SIGHT_SEGMENTS;

        if (centre) {
            indices[indexCount++] = 0;
            indices[indexCount++] = (unsigned short)(1 + next);
            indices[indexCount++] = (unsigned short)(1 + i);
        }
        for (ring = 0; ring + 1 < rings - centre; ring++) {
            unsigned short a = (unsigned short)(centre + ring * SIGHT_SEGMENTS + i);
            unsigned short b = (unsigned short)(centre + ring * SIGHT_SEGMENTS + next);

            indices[indexCount++] = a;
            indices[indexCount++] = b;
            indices[indexCount++] = (unsigned short)(b + SIGHT_SEGMENTS);
            indices[indexCount++] = a;
            indices[indexCount++] = (unsigned short)(b + SIGHT_SEGMENTS);
            indices[indexCount++] = (unsigned short)(a + SIGHT_SEGMENTS);
        }
    }
    R_AddCmdDrawTriangles2D(verts, vertexCount, indices, indexCount, cgs->media.whiteMaterial);
}

void CG_DrawAttachmentSight(const playerState_t *ps, int weapon, float x, float y)
{
    unsigned mask = BG_GetWeaponAttachments(ps, weapon);
    float w = 1.0f, scale = 1.0f, alpha;

    if (!(mask & (ATT_BIT(ATT_REFLEX) | ATT_BIT(ATT_LENS))) || ps->fWeaponPosFrac <= SIGHT_FADE_START)
        return;
    alpha = (ps->fWeaponPosFrac - SIGHT_FADE_START) / (1.0f - SIGHT_FADE_START);
    alpha = alpha * alpha * (3.0f - 2.0f * alpha);
    CalcScreenPlacement(&x, &y, &w, &scale, 2, 2);
    s_sightOpening.on = CG_WwiiScopeSightOpening(s_sightOpening.centre, s_sightOpening.toUnit);
    if (mask & ATT_BIT(ATT_REFLEX)) {
        const float radii[4] = { 0.0f, SIGHT_REFLEX_RADIUS * scale, (SIGHT_REFLEX_RADIUS + SIGHT_REFLEX_GLOW) * scale,
                                 (SIGHT_REFLEX_RADIUS + SIGHT_REFLEX_GLOW + SIGHT_HALO) * scale };
        const float alphas[4] = { 1.0f, 1.0f, SIGHT_HALO_ALPHA, 0.0f };

        CG_DrawSightRings(x, y, radii, alphas, 4, s_sightReflex, alpha);
    } else {
        const float inner = SIGHT_LENS_RING - 0.5f * SIGHT_LENS_RING_WIDTH, outer = inner + SIGHT_LENS_RING_WIDTH;
        const float ring[6] = {
            (inner - SIGHT_LENS_FEATHER - SIGHT_HALO) * scale, (inner - SIGHT_LENS_FEATHER) * scale, inner * scale,
            outer * scale, (outer + SIGHT_LENS_FEATHER) * scale, (outer + SIGHT_LENS_FEATHER + SIGHT_HALO) * scale,
        };
        const float ringAlphas[6] = { 0.0f, SIGHT_HALO_ALPHA, 1.0f, 1.0f, SIGHT_HALO_ALPHA, 0.0f };
        const float dot[4] = { 0.0f, SIGHT_LENS_DOT * scale, (SIGHT_LENS_DOT + SIGHT_LENS_FEATHER) * scale,
                               (SIGHT_LENS_DOT + SIGHT_LENS_FEATHER + SIGHT_HALO) * scale };
        const float dotAlphas[4] = { 1.0f, 1.0f, SIGHT_HALO_ALPHA, 0.0f };

        CG_DrawSightRings(x, y, ring, ringAlphas, 6, s_sightLens, alpha);
        CG_DrawSightRings(x, y, dot, dotAlphas, 4, s_sightLens, alpha);
    }
}
