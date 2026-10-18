#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_headshoticon.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <math.h>
#include <stdlib.h>

extern float GetRealHeightFromVirtualHeight(float height);

/* The symbol on the WWII capture, fitted to its pixels by least squares with
 * their blur, in radii of the ring's outside from its centre, y down. */
#define HEADSHOT_ICON_RING_INNER 0.866f
/* The cranium: |x / 0.484|^2.39 + |(y + 0.314) / 0.314|^2.39 <= 1. */
#define HEADSHOT_ICON_CRANIUM_Y -0.314f
#define HEADSHOT_ICON_CRANIUM_HALF_WIDTH 0.484f
#define HEADSHOT_ICON_CRANIUM_HALF_HEIGHT 0.314f
#define HEADSHOT_ICON_CRANIUM_POWER 2.39f
/* The eye sockets are ellipses whose long axes fall 26 degrees toward the
 * nose. */
#define HEADSHOT_ICON_EYE_X 0.287f
#define HEADSHOT_ICON_EYE_Y 0.1f
#define HEADSHOT_ICON_EYE_LENGTH 0.234f
#define HEADSHOT_ICON_EYE_WIDTH 0.112f
#define HEADSHOT_ICON_EYE_COS 0.896f
#define HEADSHOT_ICON_EYE_SIN 0.445f
/* The nose is a triangle, apex up. */
#define HEADSHOT_ICON_NOSE_TOP 0.191f
#define HEADSHOT_ICON_NOSE_BOTTOM 0.382f
#define HEADSHOT_ICON_NOSE_HALF_WIDTH 0.139f
/* Each texel's alpha from 4 x 4 samples. */
#define HEADSHOT_ICON_SAMPLES 4
#define HEADSHOT_ICON_TEXELS_MIN 8
#define HEADSHOT_ICON_TEXELS_MAX 64

/* A tick runs in from the ring to its end, narrowing from its half-width at
 * the ring to its tip's. */
typedef struct {
    float end;
    float base;
    float tip;
} CG_HeadshotIconTick;

static const CG_HeadshotIconTick s_headshotIconSide = { 0.565f, 0.087f, 0.062f };
static const CG_HeadshotIconTick s_headshotIconTop = { 0.637f, 0.11f, 0.011f };
static const CG_HeadshotIconTick s_headshotIconBottom = { 0.63f, 0.129f, 0.0f };

/* Below the cranium, the face's half-width, linear between rows: cheekbones
 * that meet the side ticks, then the jaw narrowing to the chin. */
static const float s_headshotIconFace[][2] = {
    { -0.08f, 0.503f }, { 0.028f, 0.543f }, { 0.137f, 0.558f }, { 0.245f, 0.548f },
    { 0.353f, 0.314f }, { 0.462f, 0.251f }, { 0.57f, 0.087f },
};

static MaterialHandle s_headshotIcon;
static float s_headshotIconExtent;
static int s_headshotIconTried;

static int CG_HeadshotIconTickCovers(const CG_HeadshotIconTick *tick, float along, float across)
{
    float f;

    if (along < tick->end)
        return 0;
    f = (along - tick->end) / (HEADSHOT_ICON_RING_INNER - tick->end);
    return fabsf(across) <= tick->tip + (tick->base - tick->tip) * f;
}

static float CG_HeadshotIconFaceHalfWidth(float y)
{
    int i;

    for (i = 1; i < (int)(sizeof(s_headshotIconFace) / sizeof(s_headshotIconFace[0])); i++) {
        const float *a = s_headshotIconFace[i - 1];
        const float *b = s_headshotIconFace[i];

        if (y >= a[0] && y <= b[0])
            return a[1] + (b[1] - a[1]) * (y - a[0]) / (b[0] - a[0]);
    }
    return -1.0f;
}

/* x and y in radii from the ring's centre, y down. */
static int CG_HeadshotIconCovers(float x, float y)
{
    float r2 = x * x + y * y;
    float ax = fabsf(x);
    float dx, dy, along, across;

    if (r2 > 1.0f)
        return 0;
    if (r2 >= HEADSHOT_ICON_RING_INNER * HEADSHOT_ICON_RING_INNER)
        return 1;
    if (CG_HeadshotIconTickCovers(&s_headshotIconSide, ax, y) || CG_HeadshotIconTickCovers(&s_headshotIconTop, -y, x) ||
        CG_HeadshotIconTickCovers(&s_headshotIconBottom, y, x))
        return 1;
    if (powf(ax / HEADSHOT_ICON_CRANIUM_HALF_WIDTH, HEADSHOT_ICON_CRANIUM_POWER) +
                powf(fabsf(y - HEADSHOT_ICON_CRANIUM_Y) / HEADSHOT_ICON_CRANIUM_HALF_HEIGHT,
                     HEADSHOT_ICON_CRANIUM_POWER) > 1.0f &&
        ax > CG_HeadshotIconFaceHalfWidth(y))
        return 0;
    dx = ax - HEADSHOT_ICON_EYE_X;
    dy = y - HEADSHOT_ICON_EYE_Y;
    along = (dx * HEADSHOT_ICON_EYE_COS - dy * HEADSHOT_ICON_EYE_SIN) / HEADSHOT_ICON_EYE_LENGTH;
    across = (dx * HEADSHOT_ICON_EYE_SIN + dy * HEADSHOT_ICON_EYE_COS) / HEADSHOT_ICON_EYE_WIDTH;
    if (along * along + across * across <= 1.0f)
        return 0;
    if (y >= HEADSHOT_ICON_NOSE_TOP && y <= HEADSHOT_ICON_NOSE_BOTTOM &&
        ax <= HEADSHOT_ICON_NOSE_HALF_WIDTH * (y - HEADSHOT_ICON_NOSE_TOP) /
                  (HEADSHOT_ICON_NOSE_BOTTOM - HEADSHOT_ICON_NOSE_TOP))
        return 0;
    return 1;
}

/* The sample's position in radii from the image's centre. */
static float CG_HeadshotIconSample(int texel, int sample, int size, int texels)
{
    return ((float)texel + ((float)sample + 0.5f) / HEADSHOT_ICON_SAMPLES - 0.5f * (float)size) / (float)texels;
}

void CG_HeadshotIconRaster(byte *bgra, int size, int texels)
{
    int x, y, sx, sy;

    for (y = 0; y < size; y++) {
        for (x = 0; x < size; x++) {
            byte *texel = bgra + (y * size + x) * 4;
            int covered = 0;

            for (sy = 0; sy < HEADSHOT_ICON_SAMPLES; sy++) {
                for (sx = 0; sx < HEADSHOT_ICON_SAMPLES; sx++) {
                    covered += CG_HeadshotIconCovers(CG_HeadshotIconSample(x, sx, size, texels),
                                                     CG_HeadshotIconSample(y, sy, size, texels));
                }
            }
            texel[0] = 255;
            texel[1] = 255;
            texel[2] = 255;
            texel[3] = (byte)(covered * 255 / (HEADSHOT_ICON_SAMPLES * HEADSHOT_ICON_SAMPLES));
        }
    }
}

void CG_HeadshotIconInit(void)
{
    s_headshotIcon = NULL;
    s_headshotIconExtent = 1.0f;
    s_headshotIconTried = 0;
}

/* Built once per cgame, which a renderer restart starts again, with a texel
 * per screen pixel and a clear texel around the ring. Nothing is built before
 * the screen has a size. */
MaterialHandle CG_HeadshotIconMaterial(float diameter, float *extent)
{
    float radius = GetRealHeightFromVirtualHeight(0.5f * diameter);
    byte *bgra;
    int texels, size;

    if (!s_headshotIconTried && radius > 0.0f) {
        s_headshotIconTried = 1;
        texels = (int)ceilf(radius);
        if (texels < HEADSHOT_ICON_TEXELS_MIN)
            texels = HEADSHOT_ICON_TEXELS_MIN;
        if (texels > HEADSHOT_ICON_TEXELS_MAX)
            texels = HEADSHOT_ICON_TEXELS_MAX;
        size = 2 * (texels + 1);
        bgra = (byte *)malloc(size * size * 4);
        if (bgra) {
            CG_HeadshotIconRaster(bgra, size, texels);
            s_headshotIcon = R_CreateHudImageMaterial(R_HUD_IMAGE_HEADSHOT, bgra, size, size);
            s_headshotIconExtent = (float)size / (float)(2 * texels);
            free(bgra);
        }
    }
    *extent = s_headshotIconExtent;
    return s_headshotIcon;
}
