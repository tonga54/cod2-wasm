#include "common_types.h"
#include "imports.h"
#include "cod2_grenade.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_explosives.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern int BG_GetNumWeapons(void);
extern int BG_GetFirstAvailableOffhand(const playerState_t *ps, int offhandClass);
extern int BG_PlayerDetonatorWeapon(const playerState_t *ps);
extern float GetVirtualWidthFromRealWidth(float width);
extern float GetVirtualHeightFromRealHeight(float height);
extern float GetRealHeightFromVirtualHeight(float height);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material);
extern int CL_GetCurrentCmdNumber(void);
extern qboolean CL_GetUserCmd(int cmdNumber, usercmd_t *ucmd);
extern Bool CG_AreHudMenusHidden(void);
extern void CG_SelectWeaponIndex(int weaponIndex);

/* CoD WWII's squad emblem over a placed mine or charge: a disc this wide in
 * the world, this high above it, never smaller on screen than the least size
 * (virtual units at 480 lines) nor larger than the greatest, pulsing slowly
 * from faint to nearly full, and fading out over the last quarter of the
 * range. */
#define EXPLOSIVE_EMBLEM_RANGE PLACED_EMBLEM_RANGE
#define EXPLOSIVE_EMBLEM_FADE 0.25f
#define EXPLOSIVE_EMBLEM_WORLD 16.0f
#define EXPLOSIVE_EMBLEM_LIFT 17.0f
#define EXPLOSIVE_EMBLEM_MIN 22.0f
#define EXPLOSIVE_EMBLEM_MAX 96.0f
#define EXPLOSIVE_EMBLEM_PERIOD 1800
#define EXPLOSIVE_EMBLEM_FAINT 0.3f
#define EXPLOSIVE_EMBLEM_FULL 0.95f
#define EXPLOSIVE_BUTTON_FRAG 0x10000
/* The emblems' shapes in radii of the ring's outside, y down: the ring, the
 * Allies' five-pointed star or the Axis' cross reaching into it, the dark
 * gap that parts them, and a dark edge around everything. Each image cell
 * spans EXPLOSIVE_EMBLEM_CELL radii from its centre. */
#define EXPLOSIVE_EMBLEM_CELL 1.1f
#define EXPLOSIVE_EMBLEM_RING_INNER 0.84f
#define EXPLOSIVE_EMBLEM_STAR_OUTER 0.95f
#define EXPLOSIVE_EMBLEM_STAR_INNER 0.363f
#define EXPLOSIVE_EMBLEM_CROSS_LENGTH 0.8f
#define EXPLOSIVE_EMBLEM_CROSS_END 0.34f
#define EXPLOSIVE_EMBLEM_CROSS_MIDDLE 0.11f
#define EXPLOSIVE_EMBLEM_GAP 0.07f
#define EXPLOSIVE_EMBLEM_EDGE 0.06f
#define EXPLOSIVE_EMBLEM_EDGE_ALPHA 0.45f
#define EXPLOSIVE_EMBLEM_SAMPLES 4
#define EXPLOSIVE_EMBLEM_TEXELS_MIN 32
#define EXPLOSIVE_EMBLEM_TEXELS_MAX 128

enum { EXPLOSIVE_EMBLEM_ALLIES, EXPLOSIVE_EMBLEM_AXIS, EXPLOSIVE_EMBLEM_COUNT };

static const dvar_t *cg_hudExplosiveIcons;
static MaterialHandle s_explosiveEmblems;
static int s_explosiveEmblemsTried;
static int s_detonatorReturn;
static int s_fragDown;

void CG_ExplosivesInit(void)
{
    cg_hudExplosiveIcons = Dvar_RegisterBool_mac("cg_hudExplosiveIcons", 1, 0x1001);
    s_explosiveEmblems = NULL;
    s_explosiveEmblemsTried = 0;
    s_detonatorReturn = 0;
    s_fragDown = 0;
}

static float CG_EmblemSegmentDistance(float x, float y, float ax, float ay, float bx, float by)
{
    float dx = bx - ax, dy = by - ay;
    float t = ((x - ax) * dx + (y - ay) * dy) / (dx * dx + dy * dy);

    t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
    dx = x - ax - t * dx;
    dy = y - ay - t * dy;
    return sqrtf(dx * dx + dy * dy);
}

/* Distance outside a polygon, negative within it. */
static float CG_EmblemPolygon(float x, float y, const float *px, const float *py, int count)
{
    float nearest = 1e9f;
    int inside = 0, i, j;

    for (i = 0, j = count - 1; i < count; j = i++) {
        float d = CG_EmblemSegmentDistance(x, y, px[j], py[j], px[i], py[i]);

        if (d < nearest)
            nearest = d;
        if ((py[i] > y) != (py[j] > y) && x < (px[j] - px[i]) * (y - py[i]) / (py[j] - py[i]) + px[i])
            inside = !inside;
    }
    return inside ? -nearest : nearest;
}

/* The star, one point straight up. */
static float CG_EmblemStar(float x, float y)
{
    float px[10], py[10];
    int i;

    for (i = 0; i < 10; i++) {
        float angle = (float)i * 0.628318531f;
        float radius = (i & 1) ? EXPLOSIVE_EMBLEM_STAR_INNER : EXPLOSIVE_EMBLEM_STAR_OUTER;

        px[i] = sinf(angle) * radius;
        py[i] = -cosf(angle) * radius;
    }
    return CG_EmblemPolygon(x, y, px, py, 10);
}

/* The iron cross: four arms flaring from a narrow middle to wide ends. */
static float CG_EmblemCross(float x, float y)
{
    static const float arm[3][2] = {
        { -EXPLOSIVE_EMBLEM_CROSS_END, -EXPLOSIVE_EMBLEM_CROSS_LENGTH },
        { EXPLOSIVE_EMBLEM_CROSS_END, -EXPLOSIVE_EMBLEM_CROSS_LENGTH },
        { EXPLOSIVE_EMBLEM_CROSS_MIDDLE, -EXPLOSIVE_EMBLEM_CROSS_MIDDLE },
    };
    float px[12], py[12];
    int i, k;

    /* Each arm a quarter turn from the last, y down: up, right, down, left. */
    for (k = 0; k < 4; k++) {
        for (i = 0; i < 3; i++) {
            float ax = arm[i][0], ay = arm[i][1];

            px[k * 3 + i] = k == 0 ? ax : k == 1 ? -ay : k == 2 ? -ax : ay;
            py[k * 3 + i] = k == 0 ? ay : k == 1 ? ax : k == 2 ? -ay : -ax;
        }
    }
    return CG_EmblemPolygon(x, y, px, py, 12);
}

/* 2 for the white emblem, 1 for its dark edge or gap, 0 for neither. */
static int CG_EmblemSample(int kind, float x, float y)
{
    float r = sqrtf(x * x + y * y);
    float figure = kind == EXPLOSIVE_EMBLEM_AXIS ? CG_EmblemCross(x, y) : CG_EmblemStar(x, y);
    float ringOutside = r - 1.0f, ringInside = EXPLOSIVE_EMBLEM_RING_INNER - r;
    float ring = ringOutside > ringInside ? ringOutside : ringInside;

    if (figure <= 0.0f || (ring <= 0.0f && figure > EXPLOSIVE_EMBLEM_GAP))
        return 2;
    if (ring <= EXPLOSIVE_EMBLEM_EDGE || figure <= EXPLOSIVE_EMBLEM_EDGE)
        return 1;
    return 0;
}

/* A worn grain in [0, 1], the same for every build. */
static float CG_EmblemGrain(int x, int y)
{
    unsigned int h = (unsigned int)x * 374761393u + (unsigned int)y * 668265263u;

    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 1023u) / 1023.0f;
}

/* Both emblems side by side, each in a size x size cell of B, G, R, A texels. */
void CG_ExplosiveEmblemRaster(byte *bgra, int size)
{
    int kind, x, y, sx, sy;

    for (kind = 0; kind < EXPLOSIVE_EMBLEM_COUNT; kind++) {
        for (y = 0; y < size; y++) {
            for (x = 0; x < size; x++) {
                byte *texel = bgra + (y * size * EXPLOSIVE_EMBLEM_COUNT + kind * size + x) * 4;
                int white = 0, dark = 0;
                float alpha, shade;

                for (sy = 0; sy < EXPLOSIVE_EMBLEM_SAMPLES; sy++) {
                    for (sx = 0; sx < EXPLOSIVE_EMBLEM_SAMPLES; sx++) {
                        float u = (((float)x + ((float)sx + 0.5f) / EXPLOSIVE_EMBLEM_SAMPLES) / (float)size * 2.0f -
                                   1.0f) * EXPLOSIVE_EMBLEM_CELL;
                        float v = (((float)y + ((float)sy + 0.5f) / EXPLOSIVE_EMBLEM_SAMPLES) / (float)size * 2.0f -
                                   1.0f) * EXPLOSIVE_EMBLEM_CELL;
                        int sample = CG_EmblemSample(kind, u, v);

                        white += sample == 2;
                        dark += sample == 1;
                    }
                }
                alpha = ((float)white * (0.84f + 0.16f * CG_EmblemGrain(x + kind * size, y)) +
                         (float)dark * EXPLOSIVE_EMBLEM_EDGE_ALPHA) /
                        (float)(EXPLOSIVE_EMBLEM_SAMPLES * EXPLOSIVE_EMBLEM_SAMPLES);
                shade = white + dark ? (float)white / (float)(white + dark) : 0.0f;
                texel[0] = texel[1] = texel[2] = (byte)(shade * 255.0f + 0.5f);
                texel[3] = (byte)(alpha * 255.0f + 0.5f);
            }
        }
    }
}

/* Built once per cgame, which a renderer restart starts again, with about a
 * texel per pixel at the greatest size. */
static MaterialHandle CG_ExplosiveEmblems(void)
{
    byte *bgra;
    int size;

    if (s_explosiveEmblemsTried)
        return s_explosiveEmblems;
    size = (int)ceilf(GetRealHeightFromVirtualHeight(EXPLOSIVE_EMBLEM_MAX));
    if (size <= 0)
        return NULL;
    s_explosiveEmblemsTried = 1;
    size = size < EXPLOSIVE_EMBLEM_TEXELS_MIN ? EXPLOSIVE_EMBLEM_TEXELS_MIN :
           size > EXPLOSIVE_EMBLEM_TEXELS_MAX ? EXPLOSIVE_EMBLEM_TEXELS_MAX : size;
    bgra = (byte *)malloc((size_t)size * size * 4 * EXPLOSIVE_EMBLEM_COUNT);
    if (bgra) {
        CG_ExplosiveEmblemRaster(bgra, size);
        s_explosiveEmblems = R_CreateHudImageMaterial(R_HUD_IMAGE_SQUAD_EMBLEMS, bgra, size * EXPLOSIVE_EMBLEM_COUNT,
                                                      size);
        free(bgra);
    }
    return s_explosiveEmblems;
}

/* The point in center_safearea units and how many of them a world unit spans
 * at its depth; 0 behind the view or off the screen. */
static int CG_ExplosiveScreenPosition(const vec_t *point, float *x, float *y, float *scale)
{
    float tanX = tanf(cg->refdef.fov_x * 0.00872664626f);
    float tanY = tanf(cg->refdef.fov_y * 0.00872664626f);
    float halfWidth = GetVirtualWidthFromRealWidth((float)cg->refdef.width) * .5f;
    float halfHeight = GetVirtualHeightFromRealHeight((float)cg->refdef.height) * .5f;
    float depth = 0, side = 0, up = 0;
    int axis;

    if (tanX <= 0 || tanY <= 0)
        return 0;
    for (axis = 0; axis < 3; ++axis) {
        float delta = point[axis] - cg->refdef.vieworg[axis];

        depth += delta * cg->refdef.viewaxis[0][axis];
        side += delta * cg->refdef.viewaxis[1][axis];
        up += delta * cg->refdef.viewaxis[2][axis];
    }
    if (depth <= 1)
        return 0;
    *x = -halfWidth * side / (depth * tanX);
    *y = -halfHeight * up / (depth * tanY);
    *scale = halfHeight / (depth * tanY);
    return fabsf(*x) <= halfWidth && fabsf(*y) <= halfHeight;
}

/* Through walls, as teammates' names are: the player's own and, in a team
 * mode, their teammates' mines and charges once they have come to rest. */
void CG_DrawExplosiveIcons(void)
{
    const snapshot_t *snap = cg->nextSnap ? cg->nextSnap : cg->snap;
    int self = cg->predictedPlayerState.clientNum;
    MaterialHandle emblems;
    float pulse;
    int team, entities, i;

    if (!cg_hudExplosiveIcons || !cg_hudExplosiveIcons->current.enabled || !snap || !cg_entities ||
        cg->renderingThirdPerson || CG_AreHudMenusHidden() || (unsigned)self >= 64)
        return;
    team = cg->bgs.clientinfo[self].team;
    entities = snap->numEntities;
    if (entities > (int)(sizeof(snap->entities) / sizeof(snap->entities[0])))
        entities = sizeof(snap->entities) / sizeof(snap->entities[0]);
    pulse = 0.5f + 0.5f * cosf((float)(cg->time % EXPLOSIVE_EMBLEM_PERIOD) * 6.28318531f / EXPLOSIVE_EMBLEM_PERIOD);
    pulse = EXPLOSIVE_EMBLEM_FAINT + (EXPLOSIVE_EMBLEM_FULL - EXPLOSIVE_EMBLEM_FAINT) * pulse;
    emblems = NULL;
    for (i = 0; i < entities; ++i) {
        int number = snap->entities[i].number;
        const centity_t *cent;
        const WeaponDef *weapDef;
        vec3_t point, delta;
        vec4_t color;
        float x, y, scale, size, distance, s0;
        int weapon, owner;

        if (snap->entities[i].eType != 4 || (unsigned)number >= 1022)
            continue;
        cent = &cg_entities[number];
        weapon = cent->nextState.weapon;
        if (!cent->nextValid || cent->nextState.eType != 4 || (cent->nextState.eFlags & 0x20) || weapon <= 0 ||
            weapon > BG_GetNumWeapons() || cent->nextState.pos.trType != 0)
            continue;
        weapDef = BG_GetWeaponDef(weapon);
        if (!weapDef || !BG_IsPlacedExplosive(weapDef))
            continue;
        owner = cent->nextState.otherEntityNum;
        if ((unsigned)owner >= 64)
            continue;
        if (owner != self && ((team != 1 && team != 2) || cg->bgs.clientinfo[owner].team != team))
            continue;
        delta[0] = cent->lerpOrigin[0] - cg->refdef.vieworg[0];
        delta[1] = cent->lerpOrigin[1] - cg->refdef.vieworg[1];
        delta[2] = cent->lerpOrigin[2] - cg->refdef.vieworg[2];
        distance = sqrtf(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]);
        if (distance > EXPLOSIVE_EMBLEM_RANGE)
            continue;
        point[0] = cent->lerpOrigin[0];
        point[1] = cent->lerpOrigin[1];
        point[2] = cent->lerpOrigin[2] + EXPLOSIVE_EMBLEM_LIFT;
        if (!CG_ExplosiveScreenPosition(point, &x, &y, &scale))
            continue;
        if (!emblems && !(emblems = CG_ExplosiveEmblems()))
            return;
        size = EXPLOSIVE_EMBLEM_WORLD * scale;
        size = size < EXPLOSIVE_EMBLEM_MIN ? EXPLOSIVE_EMBLEM_MIN : size > EXPLOSIVE_EMBLEM_MAX ? EXPLOSIVE_EMBLEM_MAX : size;
        color[0] = color[1] = color[2] = 1.0f;
        color[3] = (EXPLOSIVE_EMBLEM_RANGE - distance) / (EXPLOSIVE_EMBLEM_RANGE * EXPLOSIVE_EMBLEM_FADE);
        color[3] = (color[3] > 1.0f ? 1.0f : color[3]) * pulse;
        s0 = (float)(team == 1 ? EXPLOSIVE_EMBLEM_AXIS : EXPLOSIVE_EMBLEM_ALLIES) / EXPLOSIVE_EMBLEM_COUNT;
        CL_DrawStretchPic(x - 0.5f * size, y - 0.5f * size, size, size, 7, 7, s0, 0.0f,
                          s0 + 1.0f / EXPLOSIVE_EMBLEM_COUNT, 1.0f, color, emblems);
    }
}

/* With a charge carried instead of frags, a press of G takes out its
 * detonator, and the next puts it away again (bg_weapons.c keeps that press
 * from showing the empty-offhand notice). A live frag nearby is picked up
 * instead. */
void CG_UpdateDetonatorSelect(void)
{
    const playerState_t *ps = &cg->predictedPlayerState;
    usercmd_t cmd;
    int down, detonator, back;

    if (!CL_GetUserCmd(CL_GetCurrentCmdNumber(), &cmd))
        return;
    down = (cmd.buttons & EXPLOSIVE_BUTTON_FRAG) != 0;
    if (!down || s_fragDown) {
        s_fragDown = down;
        return;
    }
    s_fragDown = 1;
    if (ps->pm_type > 1 || ps->stats[0] <= 0 || ps->cursorHint == GRENADE_THROWBACK_HINT)
        return;
    detonator = BG_PlayerDetonatorWeapon(ps);
    if (!detonator || BG_GetFirstAvailableOffhand(ps, OFFHAND_CLASS_FRAG_GRENADE))
        return;
    if (cg->weaponSelect != detonator) {
        s_detonatorReturn = cg->weaponSelect;
        CG_SelectWeaponIndex(detonator);
        return;
    }
    back = s_detonatorReturn;
    if (!back || back == detonator || !((ps->weapons[back >> 5] >> (back & 0x1f)) & 1))
        back = (signed char)ps->weaponslots[1] ? (signed char)ps->weaponslots[1] : (signed char)ps->weaponslots[2];
    if (back)
        CG_SelectWeaponIndex(back);
}
