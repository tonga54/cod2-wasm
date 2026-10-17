#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_grenadewarning.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <math.h>
#include <stdlib.h>

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern float vectoyaw(const vec_t *vec);
extern float GetVirtualWidthFromRealWidth(float width);
extern float GetVirtualHeightFromRealHeight(float height);
extern float GetRealHeightFromVirtualHeight(float height);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern void CG_DrawRotatedPic(float x, float y, float width, float height, int horzAlign, int vertAlign, float angle, const vec_t *color, MaterialHandle material);

/* The ring's middle radius on the WWII captures, 21.46 pixels of 576 lines, in
 * virtual units. The warning's other measures are in this radius: the ring is
 * 0.088 thick, and the pointer is a right isosceles triangle whose long side
 * lies 1.25 out and whose tip 0.24 further. */
#define GRENADE_WARNING_RADIUS 17.9f
#define GRENADE_WARNING_RING_WIDTH 0.088f
#define GRENADE_WARNING_POINTER_BASE 1.25f
#define GRENADE_WARNING_POINTER_HEIGHT 0.24f
/* The size at the edge of the warning's range and at the player's feet. The
 * captures' rings are white, which a full-health player's is only beyond the
 * lethal radius; their size falls a sixth of the range in from its edge. */
#define GRENADE_WARNING_SCALE_FAR 0.9f
#define GRENADE_WARNING_SCALE_NEAR 1.5f
/* The captures' rings sit on an ellipse 0.86 of the way from the screen's
 * centre to its edges, toward the grenade. */
#define GRENADE_WARNING_ORBIT 0.86f
/* The grenade is 1.5 long with its head up and to the left, 27 degrees off
 * upright, and its axis passes 0.032 to the upper right of the ring's centre. */
#define GRENADE_WARNING_BODY_LENGTH 1.5f
#define GRENADE_WARNING_BODY_SIN 0.454f
#define GRENADE_WARNING_BODY_COS 0.891f
#define GRENADE_WARNING_BODY_OFFSET 0.032f
/* Zero-coverage texels inside this radius take the grenade's colour, so that
 * filtering does not tint its edges. */
#define GRENADE_WARNING_BODY_REACH 0.87f
/* Units beyond the lethal radius over which the warning turns red. */
#define GRENADE_WARNING_RED_BAND 16.0f
#define GRENADE_WARNING_DEFAULT_HEALTH 100
#define GRENADE_WARNING_SHADOW_OFFSET 1.0f
#define GRENADE_WARNING_SHADOW_ALPHA 0.6f
/* Each texel's alpha from 4 x 4 samples. */
#define GRENADE_WARNING_SAMPLES 4
#define GRENADE_WARNING_TEXELS_MIN 8
#define GRENADE_WARNING_TEXELS_MAX 96

/* Half the grenade's width along its axis from the top of its head, linear
 * between points: the head, the collar, the waist and the grip's end. */
static const float s_grenadeWarningBody[][2] = {
    { 0.0f, 0.181f }, { 0.435f, 0.181f }, { 0.435f, 0.105f }, { 0.76f, 0.097f },
    { 0.84f, 0.074f }, { 1.158f, 0.074f }, { 1.291f, 0.0995f }, { 1.5f, 0.0995f },
};

/* B, G, R. The ring and the pointer are the HUD's text white; the grenade is
 * cream, at the captures' ratio of its colour to the pointer's. */
static const byte s_grenadeWarningRingBgr[3] = { 235, 242, 245 };
static const byte s_grenadeWarningBodyBgr[3] = { 210, 233, 238 };
/* Multiplying the ring's white gives the score panel's red, brightened to it. */
static const vec3_t s_grenadeWarningRed = { 1.0f, 0.37f, 0.24f };
static const vec4_t s_grenadeWarningShadow = { 0.0f, 0.0f, 0.0f, GRENADE_WARNING_SHADOW_ALPHA };

static const dvar_t *cg_hudGrenadeWarning;
static MaterialHandle s_grenadeWarningRing;
static MaterialHandle s_grenadeWarningPointer;
static int s_grenadeWarningTried;
/* Texels per radius in both images, and their sizes in texels. */
static int s_grenadeWarningTexels;
static int s_grenadeWarningRingSize;
static int s_grenadeWarningPointerWidth;
static int s_grenadeWarningPointerHeight;

static float CG_GrenadeWarningBodyHalfWidth(float along)
{
    int i;

    if (along < 0.0f)
        return -1.0f;
    for (i = 1; i < (int)(sizeof(s_grenadeWarningBody) / sizeof(s_grenadeWarningBody[0])); i++) {
        const float *a = s_grenadeWarningBody[i - 1];
        const float *b = s_grenadeWarningBody[i];

        if (along <= b[0] && b[0] > a[0])
            return a[1] + (b[1] - a[1]) * (along - a[0]) / (b[0] - a[0]);
    }
    return -1.0f;
}

/* x and y in radii from the ring's centre, y down. */
static int CG_GrenadeWarningBodyCovers(float x, float y)
{
    float qx = x - GRENADE_WARNING_BODY_OFFSET * GRENADE_WARNING_BODY_COS;
    float qy = y + GRENADE_WARNING_BODY_OFFSET * GRENADE_WARNING_BODY_SIN;
    float along = qx * GRENADE_WARNING_BODY_SIN + qy * GRENADE_WARNING_BODY_COS + 0.5f * GRENADE_WARNING_BODY_LENGTH;
    float across = qx * GRENADE_WARNING_BODY_COS - qy * GRENADE_WARNING_BODY_SIN;

    return fabsf(across) <= CG_GrenadeWarningBodyHalfWidth(along);
}

static int CG_GrenadeWarningRingCovers(float x, float y)
{
    float inner = 1.0f - 0.5f * GRENADE_WARNING_RING_WIDTH;
    float outer = 1.0f + 0.5f * GRENADE_WARNING_RING_WIDTH;
    float r2 = x * x + y * y;

    return r2 >= inner * inner && r2 <= outer * outer;
}

/* x and y in radii from the centre of the triangle's box, tip up. */
static int CG_GrenadeWarningPointerCovers(float x, float y)
{
    float half = 0.5f * GRENADE_WARNING_POINTER_HEIGHT;

    return y <= half && fabsf(x) <= y + half;
}

/* The sample's position in radii from the image's centre. */
static float CG_GrenadeWarningSample(int texel, int sample, int size)
{
    return ((float)texel + ((float)sample + 0.5f) / GRENADE_WARNING_SAMPLES - 0.5f * (float)size) /
           (float)s_grenadeWarningTexels;
}

static void CG_GrenadeWarningRingRaster(byte *bgra)
{
    int size = s_grenadeWarningRingSize;
    int x, y, sx, sy;

    for (y = 0; y < size; y++) {
        for (x = 0; x < size; x++) {
            byte *texel = bgra + (y * size + x) * 4;
            float cx = ((float)x + 0.5f - 0.5f * (float)size) / (float)s_grenadeWarningTexels;
            float cy = ((float)y + 0.5f - 0.5f * (float)size) / (float)s_grenadeWarningTexels;
            const byte *color;
            int ring = 0, body = 0;

            for (sy = 0; sy < GRENADE_WARNING_SAMPLES; sy++) {
                for (sx = 0; sx < GRENADE_WARNING_SAMPLES; sx++) {
                    float px = CG_GrenadeWarningSample(x, sx, size);
                    float py = CG_GrenadeWarningSample(y, sy, size);

                    ring += CG_GrenadeWarningRingCovers(px, py);
                    body += CG_GrenadeWarningBodyCovers(px, py);
                }
            }
            if (body || (!ring && cx * cx + cy * cy < GRENADE_WARNING_BODY_REACH * GRENADE_WARNING_BODY_REACH))
                color = s_grenadeWarningBodyBgr;
            else
                color = s_grenadeWarningRingBgr;
            texel[0] = color[0];
            texel[1] = color[1];
            texel[2] = color[2];
            texel[3] = (byte)((ring + body) * 255 / (GRENADE_WARNING_SAMPLES * GRENADE_WARNING_SAMPLES));
        }
    }
}

static void CG_GrenadeWarningPointerRaster(byte *bgra)
{
    int width = s_grenadeWarningPointerWidth;
    int height = s_grenadeWarningPointerHeight;
    int x, y, sx, sy;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            byte *texel = bgra + (y * width + x) * 4;
            int covered = 0;

            for (sy = 0; sy < GRENADE_WARNING_SAMPLES; sy++) {
                for (sx = 0; sx < GRENADE_WARNING_SAMPLES; sx++) {
                    covered += CG_GrenadeWarningPointerCovers(CG_GrenadeWarningSample(x, sx, width),
                                                              CG_GrenadeWarningSample(y, sy, height));
                }
            }
            texel[0] = s_grenadeWarningRingBgr[0];
            texel[1] = s_grenadeWarningRingBgr[1];
            texel[2] = s_grenadeWarningRingBgr[2];
            texel[3] = (byte)(covered * 255 / (GRENADE_WARNING_SAMPLES * GRENADE_WARNING_SAMPLES));
        }
    }
}

/* Built once per cgame, which a renderer restart starts again, with a texel per
 * screen pixel at the largest size, so smaller sizes minify less than twice. */
static int CG_GrenadeWarningMaterials(void)
{
    byte *bgra;
    int texels;

    if (s_grenadeWarningTried)
        return s_grenadeWarningRing && s_grenadeWarningPointer;
    s_grenadeWarningTried = 1;
    texels = (int)ceilf(GetRealHeightFromVirtualHeight(GRENADE_WARNING_RADIUS * GRENADE_WARNING_SCALE_NEAR));
    if (texels < GRENADE_WARNING_TEXELS_MIN)
        texels = GRENADE_WARNING_TEXELS_MIN;
    if (texels > GRENADE_WARNING_TEXELS_MAX)
        texels = GRENADE_WARNING_TEXELS_MAX;
    s_grenadeWarningTexels = texels;
    s_grenadeWarningRingSize = 2 * (int)ceilf((1.0f + 0.5f * GRENADE_WARNING_RING_WIDTH) * (float)texels + 1.0f);
    s_grenadeWarningPointerWidth = 2 * (int)ceilf(GRENADE_WARNING_POINTER_HEIGHT * (float)texels + 1.0f);
    s_grenadeWarningPointerHeight = 2 * (int)ceilf(0.5f * GRENADE_WARNING_POINTER_HEIGHT * (float)texels + 1.0f);
    bgra = (byte *)malloc(s_grenadeWarningRingSize * s_grenadeWarningRingSize * 4);
    if (!bgra)
        return 0;
    CG_GrenadeWarningRingRaster(bgra);
    s_grenadeWarningRing = R_CreateHudImageMaterial(R_HUD_IMAGE_GRENADE_RING, bgra, s_grenadeWarningRingSize,
                                                   s_grenadeWarningRingSize);
    CG_GrenadeWarningPointerRaster(bgra);
    s_grenadeWarningPointer = R_CreateHudImageMaterial(R_HUD_IMAGE_GRENADE_POINTER, bgra,
                                                      s_grenadeWarningPointerWidth, s_grenadeWarningPointerHeight);
    free(bgra);
    return s_grenadeWarningRing && s_grenadeWarningPointer;
}

void CG_GrenadeWarningInit(void)
{
    cg_hudGrenadeWarning = Dvar_RegisterBool_mac("cg_hudGrenadeWarning", 1, 0x1001);
    s_grenadeWarningRing = NULL;
    s_grenadeWarningPointer = NULL;
    s_grenadeWarningTried = 0;
}

int CG_GrenadeWarningActive(void)
{
    return cg_hudGrenadeWarning && cg_hudGrenadeWarning->current.enabled;
}

/* Where G_RadiusDamage's blast, falling linearly from the inner damage at the
 * centre to the outer damage at the radius, takes all of health. */
static float CG_GrenadeWarningLethalDistance(const WeaponDef *weapon, int health)
{
    if (weapon->iExplosionOuterDamage >= health)
        return (float)weapon->iExplosionRadius;
    if (weapon->iExplosionInnerDamage <= health)
        return 0.0f;
    return (float)weapon->iExplosionRadius * (float)(weapon->iExplosionInnerDamage - health) /
           (float)(weapon->iExplosionInnerDamage - weapon->iExplosionOuterDamage);
}

static float CG_GrenadeWarningClamp(float value)
{
    return value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
}

static void CG_DrawGrenadeWarning(const CG_GrenadeThreat *threat, float orbitX, float orbitY, int health)
{
    const playerState_t *ps = &cg->predictedPlayerState;
    const WeaponDef *weapon = BG_GetWeaponDef(threat->cent->nextState.weapon);
    float distance = sqrtf(threat->distanceSq);
    float closeness = CG_GrenadeWarningClamp(threat->range > 0.0f ? 1.0f - distance / threat->range : 0.0f);
    float radius = GRENADE_WARNING_RADIUS *
                   (GRENADE_WARNING_SCALE_FAR + (GRENADE_WARNING_SCALE_NEAR - GRENADE_WARNING_SCALE_FAR) * closeness);
    float lethal = weapon ? CG_GrenadeWarningLethalDistance(weapon, health) : 0.0f;
    float red = lethal > 0.0f ?
                CG_GrenadeWarningClamp((lethal + GRENADE_WARNING_RED_BAND - distance) / GRENADE_WARNING_RED_BAND) : 0.0f;
    float ringHalf = 0.5f * (float)s_grenadeWarningRingSize / (float)s_grenadeWarningTexels * radius;
    float pointerWidth = (float)s_grenadeWarningPointerWidth / (float)s_grenadeWarningTexels * radius;
    float pointerHeight = (float)s_grenadeWarningPointerHeight / (float)s_grenadeWarningTexels * radius;
    float pointerOut = (GRENADE_WARNING_POINTER_BASE + 0.5f * GRENADE_WARNING_POINTER_HEIGHT) * radius;
    float angle, radians, dx, dy, reach, x, y, px, py;
    vec4_t color;
    vec3_t delta;
    int i;

    delta[0] = threat->cent->lerpOrigin[0] - ps->origin[0];
    delta[1] = threat->cent->lerpOrigin[1] - ps->origin[1];
    delta[2] = 0.0f;
    angle = vectoyaw(delta) - cg->refdefViewAngles[1];
    radians = angle * 0.01745329252f;
    dx = -sinf(radians);
    dy = -cosf(radians);
    /* The tip stays where the captures' size puts it, so a nearer grenade's
     * larger ring grows toward the crosshair rather than off the screen. */
    reach = 1.0f / sqrtf(dx * dx / (orbitX * orbitX) + dy * dy / (orbitY * orbitY)) +
            (GRENADE_WARNING_RADIUS - radius) * (GRENADE_WARNING_POINTER_BASE + GRENADE_WARNING_POINTER_HEIGHT);
    x = dx * reach;
    y = dy * reach;
    for (i = 0; i < 3; i++)
        color[i] = 1.0f + (s_grenadeWarningRed[i] - 1.0f) * red;
    color[3] = 1.0f;

    CL_DrawStretchPic(x - ringHalf + GRENADE_WARNING_SHADOW_OFFSET, y - ringHalf + GRENADE_WARNING_SHADOW_OFFSET,
                      2.0f * ringHalf, 2.0f * ringHalf, 7, 7, 0.0f, 0.0f, 1.0f, 1.0f, s_grenadeWarningShadow,
                      s_grenadeWarningRing);
    CL_DrawStretchPic(x - ringHalf, y - ringHalf, 2.0f * ringHalf, 2.0f * ringHalf, 7, 7, 0.0f, 0.0f, 1.0f, 1.0f,
                      color, s_grenadeWarningRing);
    px = x + dx * pointerOut - 0.5f * pointerWidth;
    py = y + dy * pointerOut - 0.5f * pointerHeight;
    CG_DrawRotatedPic(px + GRENADE_WARNING_SHADOW_OFFSET, py + GRENADE_WARNING_SHADOW_OFFSET, pointerWidth,
                      pointerHeight, 7, 7, -angle, s_grenadeWarningShadow, s_grenadeWarningPointer);
    CG_DrawRotatedPic(px, py, pointerWidth, pointerHeight, 7, 7, -angle, color, s_grenadeWarningPointer);
}

int CG_DrawGrenadeWarnings(const CG_GrenadeThreat *threats, int count)
{
    const playerState_t *ps = &cg->predictedPlayerState;
    float halfWidth = GetVirtualWidthFromRealWidth((float)cg->refdef.width) * 0.5f;
    float halfHeight = GetVirtualHeightFromRealHeight((float)cg->refdef.height) * 0.5f;
    int health = ps->stats[2] > 0 ? ps->stats[2] : GRENADE_WARNING_DEFAULT_HEALTH;
    int i;

    if (halfWidth <= 0.0f || halfHeight <= 0.0f || !CG_GrenadeWarningMaterials())
        return 0;
    /* The nearest last, on top. */
    for (i = count - 1; i >= 0; --i)
        CG_DrawGrenadeWarning(&threats[i], GRENADE_WARNING_ORBIT * halfWidth, GRENADE_WARNING_ORBIT * halfHeight,
                              health);
    return 1;
}
