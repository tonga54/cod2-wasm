#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_minimap.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern const char *CM_EntityString(void);
extern void CM_ModelBounds(clipHandle_t model, vec_t *mins, vec_t *maxs);
extern int CM_BoxTrace(trace_t *results, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, clipHandle_t model, int brushmask);
extern void Com_Printf(const char *fmt, ...);
extern int Sys_Milliseconds(void);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern void CL_DrawStretchPicPhysical(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern qboolean CG_ServerMaterialName(int index, char *materialName, int maxLen);
extern MaterialHandle CL_RegisterMaterial(const char *name, int imageTrack);
extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern const dvar_t *Dvar_RegisterFloat(const char *name, float value, float min, float max, int flags);
extern int CG_CompassUpdateFriendlies(void);
extern const dvar_t **dvar_compassPingFadeTime;

#define MINIMAP_TEXTURE_MAX 1024
/* About 800 texels across the playable area. At the default size and range a
 * pixel of a 1080-line screen spans about 10 units: Toujane's, Carentan's,
 * Dawnville's, Burgundy's and Stalingrad's texels are smaller, Brecourt's,
 * over a larger area, 13.5 units. */
#define MINIMAP_TARGET_TEXELS 800
#define MINIMAP_MIN_TEXEL_SIZE 6.0f
#define MINIMAP_SPAWN_MARGIN 1536.0f
#define MINIMAP_MAX_SPAWNS 256
/* Solid, glass and water: what the player sees from above, without player
 * clip, foliage and the like. */
#define MINIMAP_TRACE_MASK 0x31
#define MINIMAP_SURF_SKY 0x4
#define MINIMAP_CONTENTS_WATER 0x20
#define MINIMAP_WALKABLE_NORMAL 0.7f
/* Anything narrower than twice this radius stands out of the ground level. */
#define MINIMAP_GROUND_RADIUS 768.0f
#define MINIMAP_SPAWN_TOLERANCE 40.0f
#define MINIMAP_SEED_HEIGHT 16.0f
#define MINIMAP_LOW_GROUND 24.0f
#define MINIMAP_MIN_BUILDING_AREA 1024.0f
/* World units of a roof's outline and of the shadow buildings cast to the
 * south-east, darker for the first stretch: a texel and two of Toujane's
 * 15.9-unit texels at 400 across. */
#define MINIMAP_ROOF_EDGE 16.0f
#define MINIMAP_SHADOW_DEEP 16.0f
#define MINIMAP_SHADOW_LENGTH 32.0f
#define MINIMAP_ICON_SIZE 64
#define MINIMAP_MARGIN 6.0f
#define MINIMAP_SEGMENTS 128
#define MINIMAP_TOP_LEFT 9
#define MINIMAP_ACTOR_TIMEOUT 800
#define MINIMAP_MAX_ICONS 72
#define MINIMAP_ENTITYNUM_NONE 1023

/* The pause menu's whole map lets the menu show through around the playable
 * area, with a faint light glow round it, as WWII's: at least this many
 * texels, or a 64th of the map's longer side. */
#define MINIMAP_OVERVIEW_GLOW 8
#define MINIMAP_OVERVIEW_GLOW_SPAN 64
#define MINIMAP_OVERVIEW_GLOW_ALPHA 0.32f
/* A playable area under a tenth of the map is a walk that went wrong: the
 * whole map shows instead. Walls and fences running out of it, under twice
 * this many world units thick, are left out. */
#define MINIMAP_OVERVIEW_MIN_AREA 10
#define MINIMAP_OVERVIEW_OPENING 32.0f
/* Icons' half sizes, of the box's shorter side. */
#define MINIMAP_OVERVIEW_ARROW 0.022f
#define MINIMAP_OVERVIEW_FRIEND 0.018f
#define MINIMAP_OVERVIEW_OBJECTIVE 0.03f

#define MINIMAP_TEXEL_VOID 0x80
#define MINIMAP_TEXEL_WATER 0x40
#define MINIMAP_TEXEL_STEEP 0x20
#define MINIMAP_TEXEL_SURFACE 0x1f

enum {
    MINIMAP_CLASS_VOID,
    MINIMAP_CLASS_GROUND,
    MINIMAP_CLASS_BUILDING,
    MINIMAP_CLASS_UNKNOWN,
    MINIMAP_CLASS_MARKED,
    MINIMAP_CLASS_KEPT,
};

enum {
    MINIMAP_AREA_UNSEEN,
    MINIMAP_AREA_IN,
    MINIMAP_AREA_OUT,
};

/* The texture covers width * texelSize world units east of minX and
 * height * texelSize south of maxY; texels past usedWidth/usedHeight are void. */
typedef struct MinimapLayout {
    float minX;
    float maxY;
    float texelSize;
    int width;
    int height;
    int usedWidth;
    int usedHeight;
} MinimapLayout;

/* Screen placement of the disc and the world position and yaw at its center. */
typedef struct MinimapView {
    float centerX;
    float centerY;
    float radius;
    float scale;
    float originX;
    float originY;
    float sinYaw;
    float cosYaw;
} MinimapView;

typedef struct MinimapBatch {
    HudVertex2D verts[4 * MINIMAP_MAX_ICONS];
    unsigned short indices[6 * MINIMAP_MAX_ICONS];
    int vertexCount;
    int indexCount;
} MinimapBatch;

static const float s_minimapVoid[3] = {14.0f, 13.0f, 12.0f};
static const float s_minimapGround[3] = {44.0f, 41.0f, 35.0f};
static const float s_minimapGrass[3] = {35.0f, 36.0f, 27.0f};
static const float s_minimapDirt[3] = {60.0f, 51.0f, 37.0f};
static const float s_minimapSnow[3] = {62.0f, 62.0f, 61.0f};
static const float s_minimapHard[3] = {50.0f, 48.0f, 44.0f};
static const float s_minimapFloor[3] = {52.0f, 43.0f, 32.0f};
static const float s_minimapWater[3] = {28.0f, 40.0f, 48.0f};
static const float s_minimapRoof[3] = {150.0f, 134.0f, 100.0f};
static const float s_minimapRoofEdge[3] = {88.0f, 77.0f, 58.0f};
static const float s_minimapGlow[3] = {196.0f, 190.0f, 172.0f};

static const dvar_t *cg_minimap;
static const dvar_t *cg_minimapSize;
static const dvar_t *cg_minimapRange;

static struct {
    char mapName[64];
    char builtMapName[64];
    MinimapLayout layout;
    byte *pixels;
    byte *overviewPixels;
    /* The overview's texels, as CG_MinimapOverviewBounds gives them. */
    int overviewBounds[4];
    MaterialHandle mapMaterial;
    MaterialHandle overviewMaterial;
    MaterialHandle arrowMaterial;
    MaterialHandle dotMaterial;
    int attempted;
    int ready;
    float hudOffset;
} s_minimap;

static int CG_MinimapNextPow2(int value)
{
    int pow2 = 1;

    while (pow2 < value)
        pow2 <<= 1;
    return pow2;
}

/* The drawn world, cut down to the spawn area plus a margin: the skybox and
 * the scenery around the playable space only waste texels. */
static void CG_MinimapAreaBounds(const float *drawMins, const float *drawMaxs, const float *worldMins, const float *worldMaxs,
                                 const float (*spawns)[3], int spawnCount, float *mins, float *maxs)
{
    int axis;
    int i;

    for (axis = 0; axis < 2; axis++) {
        mins[axis] = drawMins[axis] > worldMins[axis] ? drawMins[axis] : worldMins[axis];
        maxs[axis] = drawMaxs[axis] < worldMaxs[axis] ? drawMaxs[axis] : worldMaxs[axis];
    }
    if (spawnCount <= 0)
        return;

    for (axis = 0; axis < 2; axis++) {
        float spawnMin = spawns[0][axis];
        float spawnMax = spawns[0][axis];

        for (i = 1; i < spawnCount; i++) {
            if (spawns[i][axis] < spawnMin)
                spawnMin = spawns[i][axis];
            if (spawns[i][axis] > spawnMax)
                spawnMax = spawns[i][axis];
        }
        if (mins[axis] < spawnMin - MINIMAP_SPAWN_MARGIN)
            mins[axis] = spawnMin - MINIMAP_SPAWN_MARGIN;
        if (maxs[axis] > spawnMax + MINIMAP_SPAWN_MARGIN)
            maxs[axis] = spawnMax + MINIMAP_SPAWN_MARGIN;
    }
}

static int CG_MinimapComputeLayout(const float *mins, const float *maxs, MinimapLayout *layout)
{
    float extentX = maxs[0] - mins[0];
    float extentY = maxs[1] - mins[1];
    float texelSize;

    if (!(extentX >= 64.0f && extentY >= 64.0f))
        return 0;

    texelSize = (extentX > extentY ? extentX : extentY) / (float)MINIMAP_TARGET_TEXELS;
    if (texelSize < MINIMAP_MIN_TEXEL_SIZE)
        texelSize = MINIMAP_MIN_TEXEL_SIZE;

    /* One void texel on each side, so clamped sampling past the edge shows
     * the void color. */
    layout->texelSize = texelSize;
    layout->usedWidth = (int)ceilf(extentX / texelSize) + 2;
    layout->usedHeight = (int)ceilf(extentY / texelSize) + 2;
    if (layout->usedWidth > MINIMAP_TEXTURE_MAX)
        layout->usedWidth = MINIMAP_TEXTURE_MAX;
    if (layout->usedHeight > MINIMAP_TEXTURE_MAX)
        layout->usedHeight = MINIMAP_TEXTURE_MAX;
    layout->width = CG_MinimapNextPow2(layout->usedWidth);
    layout->height = CG_MinimapNextPow2(layout->usedHeight);
    layout->minX = mins[0] - texelSize;
    layout->maxY = maxs[1] + texelSize;
    return 1;
}

/* Running minimum or maximum over a window of 2 * radius + 1 samples, in
 * constant time per sample (van Herk / Gil-Werman). Samples past the ends do
 * not count. dst may be src. */
static void CG_MinimapFilterLine(const float *src, float *dst, int count, int stride, int radius, int takeMax, float *prefix, float *suffix)
{
    int window = 2 * radius + 1;
    int padded = count + 2 * radius;
    float neutral = takeMax ? -FLT_MAX : FLT_MAX;
    int i;

    for (i = 0; i < padded; i++) {
        int k = i - radius;
        float value = k >= 0 && k < count ? src[k * stride] : neutral;

        if (i % window == 0)
            prefix[i] = value;
        else
            prefix[i] = takeMax ? fmaxf(prefix[i - 1], value) : fminf(prefix[i - 1], value);
    }
    for (i = padded - 1; i >= 0; i--) {
        int k = i - radius;
        float value = k >= 0 && k < count ? src[k * stride] : neutral;

        if (i == padded - 1 || (i + 1) % window == 0)
            suffix[i] = value;
        else
            suffix[i] = takeMax ? fmaxf(suffix[i + 1], value) : fminf(suffix[i + 1], value);
    }
    for (i = 0; i < count; i++) {
        float a = suffix[i];
        float b = prefix[i + 2 * radius];

        dst[i * stride] = takeMax ? fmaxf(a, b) : fminf(a, b);
    }
}

static void CG_MinimapFilter2D(float *values, int width, int height, int radius, int takeMax, float *prefix, float *suffix)
{
    int i;

    for (i = 0; i < height; i++)
        CG_MinimapFilterLine(values + i * width, values + i * width, width, 1, radius, takeMax, prefix, suffix);
    for (i = 0; i < width; i++)
        CG_MinimapFilterLine(values + i, values + i, height, width, radius, takeMax, prefix, suffix);
}

/* Grey-scale opening of the visible heights over a square of the radius: the
 * ground level around each texel, with everything narrower than the square
 * (buildings, walls) removed. Never above the texel's own height. */
static void CG_MinimapGroundLevel(const float *heights, const byte *flags, int width, int height, int radius, float *ground,
                                  float *prefix, float *suffix)
{
    int count = width * height;
    int i;

    for (i = 0; i < count; i++)
        ground[i] = flags[i] & MINIMAP_TEXEL_VOID ? FLT_MAX : heights[i];
    CG_MinimapFilter2D(ground, width, height, radius, 0, prefix, suffix);
    for (i = 0; i < count; i++) {
        if (ground[i] == FLT_MAX)
            ground[i] = -FLT_MAX;
    }
    CG_MinimapFilter2D(ground, width, height, radius, 1, prefix, suffix);
    for (i = 0; i < count; i++) {
        if (ground[i] == -FLT_MAX || (flags[i] & MINIMAP_TEXEL_VOID))
            ground[i] = heights[i];
    }
}

/* Ground is what a player can walk to from the seeds without climbing a wall,
 * plus low areas the walk cannot reach (closed courtyards); the rest stands
 * above the ground and is drawn as buildings. Without seeds, the texels at
 * ground level start the walk. Returns how many texels the walk reached, the
 * first ones of queue. */
static int CG_MinimapClassify(const float *heights, const byte *flags, const float *ground, int width, int height, float texelSize,
                              const int *seeds, int seedCount, byte *classes, int *queue)
{
    static const int dx[4] = {1, -1, 0, 0};
    static const int dy[4] = {0, 0, 1, -1};
    int count = width * height;
    float step = 20.0f + 0.6f * texelSize;
    int head = 0;
    int tail = 0;
    int i;

    for (i = 0; i < count; i++)
        classes[i] = flags[i] & MINIMAP_TEXEL_VOID ? MINIMAP_CLASS_VOID : MINIMAP_CLASS_UNKNOWN;

    for (i = 0; i < seedCount; i++) {
        int seed = seeds[i];

        if (seed < 0 || seed >= count || classes[seed] != MINIMAP_CLASS_UNKNOWN)
            continue;
        classes[seed] = MINIMAP_CLASS_GROUND;
        queue[tail++] = seed;
    }
    if (!tail) {
        for (i = 0; i < count; i++) {
            if (classes[i] == MINIMAP_CLASS_UNKNOWN && !(flags[i] & MINIMAP_TEXEL_STEEP) &&
                heights[i] - ground[i] <= MINIMAP_SEED_HEIGHT) {
                classes[i] = MINIMAP_CLASS_GROUND;
                queue[tail++] = i;
            }
        }
    }

    while (head < tail) {
        int current = queue[head++];
        int x = current % width;
        int y = current / width;
        int dir;

        for (dir = 0; dir < 4; dir++) {
            int nx = x + dx[dir];
            int ny = y + dy[dir];
            int next;

            if (nx < 0 || ny < 0 || nx >= width || ny >= height)
                continue;
            next = ny * width + nx;
            if (classes[next] != MINIMAP_CLASS_UNKNOWN || (flags[next] & MINIMAP_TEXEL_STEEP) ||
                fabsf(heights[next] - heights[current]) > step)
                continue;
            classes[next] = MINIMAP_CLASS_GROUND;
            queue[tail++] = next;
        }
    }

    for (i = 0; i < count; i++) {
        if (classes[i] == MINIMAP_CLASS_UNKNOWN)
            classes[i] = heights[i] - ground[i] <= MINIMAP_LOW_GROUND ? MINIMAP_CLASS_GROUND : MINIMAP_CLASS_BUILDING;
    }
    return tail;
}

/* Marks the unseen neighbors of the queued texels, and theirs, as value;
 * with onlyBuildings, only through buildings. */
static void CG_MinimapSpreadArea(const byte *classes, int width, int height, int onlyBuildings, byte value, int *queue,
                                 int tail, byte *area)
{
    static const int dx[4] = {1, -1, 0, 0};
    static const int dy[4] = {0, 0, 1, -1};
    int head = 0;

    while (head < tail) {
        int current = queue[head++];
        int x = current % width;
        int y = current / width;
        int dir;

        for (dir = 0; dir < 4; dir++) {
            int nx = x + dx[dir];
            int ny = y + dy[dir];
            int next;

            if (nx < 0 || ny < 0 || nx >= width || ny >= height)
                continue;
            next = ny * width + nx;
            if (area[next] != MINIMAP_AREA_UNSEEN || (onlyBuildings && classes[next] != MINIMAP_CLASS_BUILDING))
                continue;
            area[next] = value;
            queue[tail++] = next;
        }
    }
}

/* Where the pause menu's map shows: the ground walked to from the spawns,
 * the first reachedCount texels of queue, with the buildings standing on it
 * and whatever they close in; the land beyond stays clear. area gets 1 in
 * it, 0 out of it; queue is reused. */
static void CG_MinimapPlayableArea(const byte *classes, int width, int height, int reachedCount, int *queue, byte *area)
{
    int count = width * height;
    int inside = 0;
    int solid = 0;
    int tail = 0;
    int i;

    memset(area, MINIMAP_AREA_UNSEEN, (size_t)count);
    for (i = 0; i < reachedCount; i++)
        area[queue[i]] = MINIMAP_AREA_IN;
    CG_MinimapSpreadArea(classes, width, height, 1, MINIMAP_AREA_IN, queue, reachedCount, area);
    for (i = 0; i < count; i++) {
        int x = i % width;
        int y = i / width;

        if ((x == 0 || y == 0 || x == width - 1 || y == height - 1) && area[i] == MINIMAP_AREA_UNSEEN) {
            area[i] = MINIMAP_AREA_OUT;
            queue[tail++] = i;
        }
    }
    CG_MinimapSpreadArea(classes, width, height, 0, MINIMAP_AREA_OUT, queue, tail, area);
    for (i = 0; i < count; i++) {
        area[i] = area[i] != MINIMAP_AREA_OUT;
        inside += area[i];
        solid += classes[i] != MINIMAP_CLASS_VOID;
    }
    if (inside * MINIMAP_OVERVIEW_MIN_AREA < solid) {
        for (i = 0; i < count; i++)
            area[i] = classes[i] != MINIMAP_CLASS_VOID;
    }
}

/* Posts, trunks and crates would show as specks; only keep blocks of at least
 * minArea texels. */
static void CG_MinimapRemoveSpecks(byte *classes, int width, int height, int minArea, int *queue)
{
    static const int dx[4] = {1, -1, 0, 0};
    static const int dy[4] = {0, 0, 1, -1};
    int count = width * height;
    int start;
    int i;

    for (start = 0; start < count; start++) {
        int head = 0;
        int tail = 0;
        byte value;

        if (classes[start] != MINIMAP_CLASS_BUILDING)
            continue;
        classes[start] = MINIMAP_CLASS_MARKED;
        queue[tail++] = start;
        while (head < tail) {
            int current = queue[head++];
            int x = current % width;
            int y = current / width;
            int dir;

            for (dir = 0; dir < 4; dir++) {
                int nx = x + dx[dir];
                int ny = y + dy[dir];
                int next;

                if (nx < 0 || ny < 0 || nx >= width || ny >= height)
                    continue;
                next = ny * width + nx;
                if (classes[next] != MINIMAP_CLASS_BUILDING)
                    continue;
                classes[next] = MINIMAP_CLASS_MARKED;
                queue[tail++] = next;
            }
        }
        value = tail < minArea ? MINIMAP_CLASS_GROUND : MINIMAP_CLASS_KEPT;
        for (i = 0; i < tail; i++)
            classes[queue[i]] = value;
    }
    for (i = 0; i < count; i++) {
        if (classes[i] == MINIMAP_CLASS_KEPT)
            classes[i] = MINIMAP_CLASS_BUILDING;
    }
}

/* Dirt and gravel paths read lighter than the grass around them, roads and
 * squares are paved, floors are wood. */
static const float *CG_MinimapGroundColor(int flags)
{
    if (flags & MINIMAP_TEXEL_WATER)
        return s_minimapWater;
    switch (flags & MINIMAP_TEXEL_SURFACE) {
    case 8: case 10:
        return s_minimapGrass;
    case 6: case 11: case 14: case 18:
        return s_minimapDirt;
    case 12: case 19:
        return s_minimapSnow;
    case 2: case 5: case 16: case 17: case 22:
        return s_minimapHard;
    case 1: case 3: case 4: case 13: case 15: case 21:
        return s_minimapFloor;
    default:
        return s_minimapGround;
    }
}

/* Height change toward the south-east, per world unit, within one class: the
 * light comes from the north-west. */
static float CG_MinimapSlope(const float *heights, const byte *classes, int width, int height, int x, int y, float texelSize)
{
    int i = y * width + x;
    byte cls = classes[i];
    float center = heights[i];
    float west = x > 0 && classes[i - 1] == cls ? heights[i - 1] : center;
    float east = x + 1 < width && classes[i + 1] == cls ? heights[i + 1] : center;
    float north = y > 0 && classes[i - width] == cls ? heights[i - width] : center;
    float south = y + 1 < height && classes[i + width] == cls ? heights[i + width] : center;

    return (east - west + south - north) / (2.0f * texelSize);
}

static float CG_MinimapClamp(float value, float low, float high)
{
    return value < low ? low : value > high ? high : value;
}

static void CG_MinimapPutTexel(byte *bgra, const float *rgb, float shade)
{
    int c;

    for (c = 0; c < 3; c++)
        bgra[2 - c] = (byte)(CG_MinimapClamp(rgb[c] * shade, 0.0f, 255.0f) + 0.5f);
    bgra[3] = 255;
}

static int CG_MinimapIsBuilding(const byte *classes, int width, int height, int x, int y)
{
    return x >= 0 && y >= 0 && x < width && y < height && classes[y * width + x] == MINIMAP_CLASS_BUILDING;
}

/* Texels spanning about this many world units, at least the given count. */
static int CG_MinimapTexels(float units, float texelSize, int least)
{
    int texels = (int)(units / texelSize + 0.5f);

    return texels < least ? least : texels;
}

/* Whether a building texel has open ground within reach along an axis. */
static int CG_MinimapNearOpen(const byte *classes, int width, int height, int x, int y, int reach)
{
    int k;

    for (k = 1; k <= reach; k++) {
        if (!CG_MinimapIsBuilding(classes, width, height, x - k, y) || !CG_MinimapIsBuilding(classes, width, height, x + k, y) ||
            !CG_MinimapIsBuilding(classes, width, height, x, y - k) || !CG_MinimapIsBuilding(classes, width, height, x, y + k))
            return 1;
    }
    return 0;
}

/* The shade of a building's shadow on ground diagonally south-east of it. */
static float CG_MinimapShadow(const byte *classes, int width, int height, int x, int y, int deep, int length)
{
    int k;

    for (k = 1; k <= length; k++) {
        if (CG_MinimapIsBuilding(classes, width, height, x - k, y - k))
            return k <= deep ? 0.6f : 0.78f;
    }
    return 1.0f;
}

static void CG_MinimapColorize(const float *heights, const byte *flags, const byte *classes, int width, int height, float texelSize, byte *bgra)
{
    int edge = CG_MinimapTexels(MINIMAP_ROOF_EDGE, texelSize, 1);
    int deep = CG_MinimapTexels(MINIMAP_SHADOW_DEEP, texelSize, 1);
    int length = CG_MinimapTexels(MINIMAP_SHADOW_LENGTH, texelSize, deep + 1);
    int x;
    int y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            int i = y * width + x;
            byte *texel = bgra + 4 * i;
            float slope;

            if (classes[i] == MINIMAP_CLASS_VOID) {
                CG_MinimapPutTexel(texel, s_minimapVoid, 1.0f);
            } else if (classes[i] == MINIMAP_CLASS_BUILDING) {
                if (CG_MinimapNearOpen(classes, width, height, x, y, edge)) {
                    CG_MinimapPutTexel(texel, s_minimapRoofEdge, 1.0f);
                } else {
                    slope = CG_MinimapSlope(heights, classes, width, height, x, y, texelSize);
                    CG_MinimapPutTexel(texel, s_minimapRoof, CG_MinimapClamp(1.0f + 0.15f * slope, 0.85f, 1.15f));
                }
            } else {
                float shade;

                slope = CG_MinimapSlope(heights, classes, width, height, x, y, texelSize);
                shade = CG_MinimapClamp(1.0f + 0.3f * slope, 0.7f, 1.3f);
                shade *= CG_MinimapShadow(classes, width, height, x, y, deep, length);
                CG_MinimapPutTexel(texel, CG_MinimapGroundColor(flags[i]), shade);
            }
        }
    }
}

/* Each texel's distance in texels to the nearest one at 0, a step one
 * across and the square root of two diagonally: a two-pass chamfer. */
static void CG_MinimapChamfer(float *dist, int width, int height)
{
    const float diagonal = 1.41421356f;
    int x;
    int y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            float *d = &dist[y * width + x];
            if (x > 0)
                *d = fminf(*d, d[-1] + 1.0f);
            if (y > 0) {
                *d = fminf(*d, d[-width] + 1.0f);
                if (x > 0)
                    *d = fminf(*d, d[-width - 1] + diagonal);
                if (x + 1 < width)
                    *d = fminf(*d, d[-width + 1] + diagonal);
            }
        }
    }
    for (y = height - 1; y >= 0; y--) {
        for (x = width - 1; x >= 0; x--) {
            float *d = &dist[y * width + x];
            if (x + 1 < width)
                *d = fminf(*d, d[1] + 1.0f);
            if (y + 1 < height) {
                *d = fminf(*d, d[width] + 1.0f);
                if (x + 1 < width)
                    *d = fminf(*d, d[width + 1] + diagonal);
                if (x > 0)
                    *d = fminf(*d, d[width - 1] + diagonal);
            }
        }
    }
}

/* Without the walls and fences that run out of it: whatever is under twice
 * radius texels across goes, the rest keeps its outline but for rounder
 * corners. dist is scratch. */
static void CG_MinimapOpenArea(byte *area, int width, int height, float radius, float *dist)
{
    int count = width * height;
    int i;

    for (i = 0; i < count; i++)
        dist[i] = area[i] ? FLT_MAX : 0.0f;
    CG_MinimapChamfer(dist, width, height);
    for (i = 0; i < count; i++)
        area[i] = area[i] ? (dist[i] > radius ? 2 : 1) : 0;
    for (i = 0; i < count; i++)
        dist[i] = area[i] == 2 ? 0.0f : FLT_MAX;
    CG_MinimapChamfer(dist, width, height);
    for (i = 0; i < count; i++)
        area[i] = area[i] && dist[i] <= radius + 0.5f;
}

/* The map for the pause menu: the texels in area, the rest clear but for a
 * glow that fades over glow texels from it. dist is scratch. */
static void CG_MinimapOverviewImage(const byte *bgra, const byte *area, int width, int height, int glow, float *dist,
                                    byte *out)
{
    int count = width * height;
    int i;

    for (i = 0; i < count; i++)
        dist[i] = area[i] ? 0.0f : FLT_MAX;
    CG_MinimapChamfer(dist, width, height);
    for (i = 0; i < count; i++) {
        byte *texel = out + 4 * i;
        float alpha;

        if (area[i]) {
            memcpy(texel, bgra + 4 * i, 4);
            continue;
        }
        alpha = 1.0f - dist[i] / (float)glow;
        alpha = alpha > 0.0f ? alpha * alpha * MINIMAP_OVERVIEW_GLOW_ALPHA : 0.0f;
        CG_MinimapPutTexel(texel, s_minimapGlow, 1.0f);
        texel[3] = (byte)(alpha * 255.0f + 0.5f);
    }
}

/* The texels the pause menu draws, x0, y0, x1, y1 from the texture's corner:
 * the area's with room for its glow, else all used. */
static void CG_MinimapOverviewBounds(const byte *area, const MinimapLayout *layout, int glow, int *bounds)
{
    int x0 = layout->usedWidth;
    int y0 = layout->usedHeight;
    int x1 = 0;
    int y1 = 0;
    int x;
    int y;

    for (y = 0; y < layout->usedHeight; y++) {
        for (x = 0; x < layout->usedWidth; x++) {
            if (!area[y * layout->width + x])
                continue;
            x0 = x < x0 ? x : x0;
            y0 = y < y0 ? y : y0;
            x1 = x + 1 > x1 ? x + 1 : x1;
            y1 = y + 1 > y1 ? y + 1 : y1;
        }
    }
    if (x1 <= x0 || y1 <= y0) {
        x0 = y0 = glow;
        x1 = layout->usedWidth - glow;
        y1 = layout->usedHeight - glow;
    }
    bounds[0] = x0 - glow > 0 ? x0 - glow : 0;
    bounds[1] = y0 - glow > 0 ? y0 - glow : 0;
    bounds[2] = x1 + glow < layout->usedWidth ? x1 + glow : layout->usedWidth;
    bounds[3] = y1 + glow < layout->usedHeight ? y1 + glow : layout->usedHeight;
}

static int CG_MinimapInsidePolygon(const float (*points)[2], int count, float x, float y)
{
    int inside = 0;
    int i;
    int j;

    for (i = 0, j = count - 1; i < count; j = i++) {
        if ((points[i][1] > y) != (points[j][1] > y) &&
            x < (points[j][0] - points[i][0]) * (y - points[i][1]) / (points[j][1] - points[i][1]) + points[i][0])
            inside = !inside;
    }
    return inside;
}

static float CG_MinimapPolygonDistance(const float (*points)[2], int count, float x, float y)
{
    float best = FLT_MAX;
    int i;

    for (i = 0; i < count; i++) {
        const float *a = points[i];
        const float *b = points[(i + 1) % count];
        float abX = b[0] - a[0];
        float abY = b[1] - a[1];
        float t = CG_MinimapClamp(((x - a[0]) * abX + (y - a[1]) * abY) / (abX * abX + abY * abY), 0.0f, 1.0f);
        float dX = a[0] + t * abX - x;
        float dY = a[1] + t * abY - y;

        if (dX * dX + dY * dY < best)
            best = dX * dX + dY * dY;
    }
    return sqrtf(best);
}

/* A white arrow pointing up, or a white dot, with a dark outline; the vertex
 * color tints the white. Antialiased with 4x4 samples per texel. */
static void CG_MinimapRasterIcon(byte *bgra, int size, int dot)
{
    static const float arrow[4][2] = {{0.5f, 0.08f}, {0.86f, 0.88f}, {0.5f, 0.68f}, {0.14f, 0.88f}};
    const float outline = 0.08f;
    const float dotRadius = 0.3f;
    int x;
    int y;

    for (y = 0; y < size; y++) {
        for (x = 0; x < size; x++) {
            byte *texel = bgra + 4 * (y * size + x);
            float alpha = 0.0f;
            float value = 0.0f;
            int sx;
            int sy;

            for (sy = 0; sy < 4; sy++) {
                for (sx = 0; sx < 4; sx++) {
                    float px = ((float)x + ((float)sx + 0.5f) * 0.25f) / (float)size;
                    float py = ((float)y + ((float)sy + 0.5f) * 0.25f) / (float)size;
                    float edge;
                    int inside;

                    if (dot) {
                        edge = sqrtf((px - 0.5f) * (px - 0.5f) + (py - 0.5f) * (py - 0.5f)) - dotRadius;
                        inside = edge <= 0.0f;
                    } else {
                        inside = CG_MinimapInsidePolygon(arrow, 4, px, py);
                        edge = inside ? 0.0f : CG_MinimapPolygonDistance(arrow, 4, px, py);
                    }
                    if (inside) {
                        alpha += 1.0f;
                        value += 1.0f;
                    } else if (edge <= outline) {
                        alpha += 0.9f;
                        value += 0.9f * 0.05f;
                    }
                }
            }
            alpha *= 1.0f / 16.0f;
            value = alpha > 0.0f ? value / (16.0f * alpha) : 0.0f;
            texel[0] = texel[1] = texel[2] = (byte)(value * 255.0f + 0.5f);
            texel[3] = (byte)(alpha * 255.0f + 0.5f);
        }
    }
}

/* Forward is up on screen: the map turns with the view. */
static void CG_MinimapWorldToScreen(const MinimapView *view, float worldX, float worldY, float *screenX, float *screenY)
{
    float dx = worldX - view->originX;
    float dy = worldY - view->originY;

    *screenX = view->centerX + view->scale * (dx * view->sinYaw - dy * view->cosYaw);
    *screenY = view->centerY - view->scale * (dx * view->cosYaw + dy * view->sinYaw);
}

static void CG_MinimapScreenToTexture(const MinimapView *view, const MinimapLayout *layout, float screenX, float screenY, float *s, float *t)
{
    float ox = screenX - view->centerX;
    float oy = screenY - view->centerY;
    float worldX = view->originX + (ox * view->sinYaw - oy * view->cosYaw) / view->scale;
    float worldY = view->originY - (ox * view->cosYaw + oy * view->sinYaw) / view->scale;

    *s = (worldX - layout->minX) / ((float)layout->width * layout->texelSize);
    *t = (layout->maxY - worldY) / ((float)layout->height * layout->texelSize);
}

/* Pulls a marker in to limit pixels from the center; returns 1 if it moved. */
static int CG_MinimapClampToDisc(const MinimapView *view, float limit, float *x, float *y)
{
    float dx = *x - view->centerX;
    float dy = *y - view->centerY;
    float distSq = dx * dx + dy * dy;
    float k;

    if (distSq <= limit * limit)
        return 0;
    k = limit / sqrtf(distSq);
    *x = view->centerX + dx * k;
    *y = view->centerY + dy * k;
    return 1;
}

static void CG_MinimapSetVertex(HudVertex2D *vert, float x, float y, float s, float t, const float *rgba)
{
    vert->xy[0] = x;
    vert->xy[1] = y;
    vert->st[0] = s;
    vert->st[1] = t;
    vert->rgba[0] = rgba[0];
    vert->rgba[1] = rgba[1];
    vert->rgba[2] = rgba[2];
    vert->rgba[3] = rgba[3];
}

/* The map disc: a fan around the center with a one-pixel fringe that fades
 * out, so the rim is antialiased. Triangles wind like the engine's quads. */
static int CG_MinimapBuildDisc(const MinimapView *view, const MinimapLayout *layout, float alpha, HudVertex2D *verts,
                               unsigned short *indices, int *indexCount)
{
    const float opaque[4] = {1.0f, 1.0f, 1.0f, alpha};
    const float clear[4] = {1.0f, 1.0f, 1.0f, 0.0f};
    float inner = view->radius - 0.5f;
    float outer = view->radius + 0.5f;
    float s;
    float t;
    int ic = 0;
    int i;

    CG_MinimapScreenToTexture(view, layout, view->centerX, view->centerY, &s, &t);
    CG_MinimapSetVertex(&verts[0], view->centerX, view->centerY, s, t, opaque);
    for (i = 0; i < MINIMAP_SEGMENTS; i++) {
        float angle = (float)i * (6.28318531f / (float)MINIMAP_SEGMENTS);
        float c = cosf(angle);
        float sn = sinf(angle);
        float x = view->centerX + inner * c;
        float y = view->centerY + inner * sn;
        int next = (i + 1) % MINIMAP_SEGMENTS;

        CG_MinimapScreenToTexture(view, layout, x, y, &s, &t);
        CG_MinimapSetVertex(&verts[1 + i], x, y, s, t, opaque);
        x = view->centerX + outer * c;
        y = view->centerY + outer * sn;
        CG_MinimapScreenToTexture(view, layout, x, y, &s, &t);
        CG_MinimapSetVertex(&verts[1 + MINIMAP_SEGMENTS + i], x, y, s, t, clear);

        indices[ic++] = 0;
        indices[ic++] = (unsigned short)(1 + i);
        indices[ic++] = (unsigned short)(1 + next);
        indices[ic++] = (unsigned short)(1 + i);
        indices[ic++] = (unsigned short)(1 + MINIMAP_SEGMENTS + i);
        indices[ic++] = (unsigned short)(1 + MINIMAP_SEGMENTS + next);
        indices[ic++] = (unsigned short)(1 + i);
        indices[ic++] = (unsigned short)(1 + MINIMAP_SEGMENTS + next);
        indices[ic++] = (unsigned short)(1 + next);
    }
    *indexCount = ic;
    return 1 + 2 * MINIMAP_SEGMENTS;
}

/* An icon turned clockwise by angle degrees, as a quad in the engine's corner
 * and index order. */
static void CG_MinimapAddIcon(MinimapBatch *batch, float x, float y, float halfSize, float angle, const float *rgba)
{
    static const float corners[4][2] = {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};
    float radians = angle * (3.14159265f / 180.0f);
    float s = sinf(radians);
    float c = cosf(radians);
    int vc = batch->vertexCount;
    int ic = batch->indexCount;
    int k;

    if (vc + 4 > 4 * MINIMAP_MAX_ICONS)
        return;
    for (k = 0; k < 4; k++) {
        float lx = corners[k][0] * halfSize;
        float ly = corners[k][1] * halfSize;

        CG_MinimapSetVertex(&batch->verts[vc + k], x + lx * c - ly * s, y + lx * s + ly * c, corners[k][0] > 0.0f ? 1.0f : 0.0f,
                            corners[k][1] > 0.0f ? 1.0f : 0.0f, rgba);
    }
    batch->indices[ic + 0] = (unsigned short)(vc + 3);
    batch->indices[ic + 1] = (unsigned short)vc;
    batch->indices[ic + 2] = (unsigned short)(vc + 2);
    batch->indices[ic + 3] = (unsigned short)(vc + 2);
    batch->indices[ic + 4] = (unsigned short)vc;
    batch->indices[ic + 5] = (unsigned short)(vc + 1);
    batch->vertexCount = vc + 4;
    batch->indexCount = ic + 6;
}

static float CG_MinimapComputeHudOffset(int visible, float size)
{
    return visible ? MINIMAP_MARGIN + size : 0.0f;
}

static int CG_MinimapReadToken(const char **cursor, const char *end, char *token, int size)
{
    const char *p = *cursor;
    int length = 0;

    while (p < end && *p != '"')
        p++;
    if (p >= end)
        return 0;
    p++;
    while (p < end && *p != '"') {
        if (length < size - 1)
            token[length++] = *p;
        p++;
    }
    if (p >= end)
        return 0;
    token[length] = 0;
    *cursor = p + 1;
    return 1;
}

/* Origins of the multiplayer spawn points (mp_dm_spawn, mp_tdm_spawn,
 * mp_ctf_spawn_allied, mp_sd_spawn_attacker, ...). */
static int CG_MinimapParseSpawns(const char *entities, float (*spawns)[3], int maxSpawns)
{
    const char *p = entities;
    int count = 0;

    while (p && count < maxSpawns && (p = strchr(p, '{')) != NULL) {
        const char *end = strchr(p, '}');
        const char *cursor = p + 1;
        char classname[64] = "";
        float origin[3];
        int hasOrigin = 0;
        char key[64];
        char value[256];

        if (!end)
            break;
        while (CG_MinimapReadToken(&cursor, end, key, sizeof(key)) && CG_MinimapReadToken(&cursor, end, value, sizeof(value))) {
            if (!strcmp(key, "classname")) {
                strncpy(classname, value, sizeof(classname) - 1);
                classname[sizeof(classname) - 1] = 0;
            } else if (!strcmp(key, "origin")) {
                char *next;
                int axis;

                hasOrigin = 1;
                next = value;
                for (axis = 0; axis < 3; axis++) {
                    char *parsed;

                    origin[axis] = strtof(next, &parsed);
                    if (parsed == next)
                        hasOrigin = 0;
                    next = parsed;
                }
            }
        }
        if (hasOrigin && !strncmp(classname, "mp_", 3) && strstr(classname, "_spawn")) {
            spawns[count][0] = origin[0];
            spawns[count][1] = origin[1];
            spawns[count][2] = origin[2];
            count++;
        }
        p = end + 1;
    }
    return count;
}

static void CG_MinimapTraceTexel(float x, float y, float topZ, float bottomZ, float *height, byte *flags)
{
    static const vec3_t zero = {0.0f, 0.0f, 0.0f};
    trace_t trace;
    vec3_t start;
    vec3_t end;

    start[0] = end[0] = x;
    start[1] = end[1] = y;
    start[2] = topZ;
    end[2] = bottomZ;
    CM_BoxTrace(&trace, start, end, zero, zero, 0, MINIMAP_TRACE_MASK);
    if (trace.startsolid || trace.allsolid || trace.fraction >= 1.0f || (trace.surfaceFlags & MINIMAP_SURF_SKY)) {
        *flags = MINIMAP_TEXEL_VOID;
        return;
    }
    *height = topZ + (bottomZ - topZ) * trace.fraction;
    *flags = (byte)((trace.surfaceFlags >> 20) & MINIMAP_TEXEL_SURFACE);
    if (trace.contents & MINIMAP_CONTENTS_WATER)
        *flags |= MINIMAP_TEXEL_WATER;
    if (trace.normal[2] < MINIMAP_WALKABLE_NORMAL)
        *flags |= MINIMAP_TEXEL_STEEP;
}

static int CG_MinimapGenerate(void)
{
    vec3_t worldMins;
    vec3_t worldMaxs;
    vec3_t drawMins;
    vec3_t drawMaxs;
    float areaMins[2];
    float areaMaxs[2];
    float (*spawns)[3];
    float *heights = NULL;
    float *ground = NULL;
    float *prefix = NULL;
    float *suffix = NULL;
    byte *flags = NULL;
    byte *classes = NULL;
    byte *pixels = NULL;
    byte *area = NULL;
    byte *overview = NULL;
    int *queue = NULL;
    int *seeds = NULL;
    MinimapLayout layout;
    int bounds[4] = {0, 0, 0, 0};
    int startTime = Sys_Milliseconds();
    int spawnCount;
    int seedCount = 0;
    int reached;
    int glow;
    int count;
    int radius;
    int lineLength;
    int ok = 0;
    int x;
    int y;
    int i;
    float topZ;
    float bottomZ;

    spawns = malloc(sizeof(*spawns) * MINIMAP_MAX_SPAWNS);
    if (!spawns)
        return 0;
    CM_ModelBounds(0, worldMins, worldMaxs);
    if (!R_GetWorldDrawBounds(drawMins, drawMaxs)) {
        memcpy(drawMins, worldMins, sizeof(vec3_t));
        memcpy(drawMaxs, worldMaxs, sizeof(vec3_t));
    }
    spawnCount = CG_MinimapParseSpawns(CM_EntityString(), spawns, MINIMAP_MAX_SPAWNS);
    CG_MinimapAreaBounds(drawMins, drawMaxs, worldMins, worldMaxs, (const float (*)[3])spawns, spawnCount, areaMins, areaMaxs);
    if (!CG_MinimapComputeLayout(areaMins, areaMaxs, &layout))
        goto done;

    count = layout.width * layout.height;
    radius = (int)(MINIMAP_GROUND_RADIUS / layout.texelSize + 0.5f);
    lineLength = (layout.width > layout.height ? layout.width : layout.height) + 2 * radius;
    heights = malloc(sizeof(float) * count);
    ground = malloc(sizeof(float) * count);
    flags = malloc(count);
    classes = malloc(count);
    queue = malloc(sizeof(int) * count);
    seeds = malloc(sizeof(int) * (spawnCount + 1));
    prefix = malloc(sizeof(float) * lineLength);
    suffix = malloc(sizeof(float) * lineLength);
    pixels = malloc(4 * count);
    if (!heights || !ground || !flags || !classes || !queue || !seeds || !prefix || !suffix || !pixels)
        goto done;

    topZ = drawMaxs[2] + 16.0f;
    bottomZ = worldMins[2] - 16.0f;
    for (y = 0; y < layout.height; y++) {
        for (x = 0; x < layout.width; x++) {
            i = y * layout.width + x;
            heights[i] = bottomZ;
            flags[i] = MINIMAP_TEXEL_VOID;
            if (x == 0 || y == 0 || x >= layout.usedWidth - 1 || y >= layout.usedHeight - 1)
                continue;
            CG_MinimapTraceTexel(layout.minX + ((float)x + 0.5f) * layout.texelSize, layout.maxY - ((float)y + 0.5f) * layout.texelSize,
                                 topZ, bottomZ, &heights[i], &flags[i]);
        }
    }

    /* Spawns standing on the traced surface are on the ground. */
    for (i = 0; i < spawnCount; i++) {
        x = (int)floorf((spawns[i][0] - layout.minX) / layout.texelSize);
        y = (int)floorf((layout.maxY - spawns[i][1]) / layout.texelSize);
        if (x < 0 || y < 0 || x >= layout.width || y >= layout.height)
            continue;
        if (!(flags[y * layout.width + x] & MINIMAP_TEXEL_VOID) &&
            fabsf(heights[y * layout.width + x] - spawns[i][2]) <= MINIMAP_SPAWN_TOLERANCE)
            seeds[seedCount++] = y * layout.width + x;
    }

    CG_MinimapGroundLevel(heights, flags, layout.width, layout.height, radius, ground, prefix, suffix);
    reached = CG_MinimapClassify(heights, flags, ground, layout.width, layout.height, layout.texelSize, seeds, seedCount,
                                 classes, queue);
    /* The playable area reads the walk off the queue before the specks reuse it. */
    area = malloc(count);
    if (area)
        CG_MinimapPlayableArea(classes, layout.width, layout.height, reached, queue, area);
    CG_MinimapRemoveSpecks(classes, layout.width, layout.height,
                           (int)(MINIMAP_MIN_BUILDING_AREA / (layout.texelSize * layout.texelSize) + 0.5f), queue);
    CG_MinimapColorize(heights, flags, classes, layout.width, layout.height, layout.texelSize, pixels);
    /* The heights are no longer needed: they hold the glow's distances. */
    overview = area ? malloc(4 * count) : NULL;
    if (overview) {
        glow = (layout.usedWidth > layout.usedHeight ? layout.usedWidth : layout.usedHeight) / MINIMAP_OVERVIEW_GLOW_SPAN;
        if (glow < MINIMAP_OVERVIEW_GLOW)
            glow = MINIMAP_OVERVIEW_GLOW;
        CG_MinimapOpenArea(area, layout.width, layout.height, MINIMAP_OVERVIEW_OPENING / layout.texelSize, heights);
        CG_MinimapOverviewImage(pixels, area, layout.width, layout.height, glow, heights, overview);
        CG_MinimapOverviewBounds(area, &layout, glow, bounds);
    }
    ok = 1;

    Com_Printf("Minimap: %ix%i texels of %.1f units, %i of %i spawns on the ground, built in %i ms\n", layout.usedWidth,
               layout.usedHeight, layout.texelSize, seedCount, spawnCount, Sys_Milliseconds() - startTime);

done:
    free(spawns);
    free(heights);
    free(ground);
    free(flags);
    free(classes);
    free(area);
    free(queue);
    free(seeds);
    free(prefix);
    free(suffix);
    if (!ok) {
        free(pixels);
        free(overview);
        return 0;
    }
    free(s_minimap.pixels);
    free(s_minimap.overviewPixels);
    s_minimap.pixels = pixels;
    s_minimap.overviewPixels = overview;
    memcpy(s_minimap.overviewBounds, bounds, sizeof(bounds));
    s_minimap.layout = layout;
    memcpy(s_minimap.builtMapName, s_minimap.mapName, sizeof(s_minimap.builtMapName));
    return 1;
}

static int CG_MinimapUpload(void)
{
    static byte icon[MINIMAP_ICON_SIZE * MINIMAP_ICON_SIZE * 4];

    s_minimap.mapMaterial = R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP, s_minimap.pixels, s_minimap.layout.width, s_minimap.layout.height);
    s_minimap.overviewMaterial = s_minimap.overviewPixels
                                     ? R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP_OVERVIEW, s_minimap.overviewPixels,
                                                                s_minimap.layout.width, s_minimap.layout.height)
                                     : NULL;
    CG_MinimapRasterIcon(icon, MINIMAP_ICON_SIZE, 0);
    s_minimap.arrowMaterial = R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP_ARROW, icon, MINIMAP_ICON_SIZE, MINIMAP_ICON_SIZE);
    CG_MinimapRasterIcon(icon, MINIMAP_ICON_SIZE, 1);
    s_minimap.dotMaterial = R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP_DOT, icon, MINIMAP_ICON_SIZE, MINIMAP_ICON_SIZE);
    return s_minimap.mapMaterial && s_minimap.arrowMaterial && s_minimap.dotMaterial;
}

/* Builds the map once per level; a renderer restart only uploads it again. */
static int CG_MinimapPrepare(void)
{
    if (s_minimap.ready)
        return 1;
    if (s_minimap.attempted)
        return 0;
    s_minimap.attempted = 1;
    if (!s_minimap.pixels || strcmp(s_minimap.builtMapName, s_minimap.mapName)) {
        if (!CG_MinimapGenerate())
            return 0;
    }
    s_minimap.ready = CG_MinimapUpload();
    return s_minimap.ready;
}

void CG_MinimapInit(const char *mapName)
{
    cg_minimap = Dvar_RegisterBool_mac("cg_minimap", 1, 0x1001);
    cg_minimapSize = Dvar_RegisterFloat("cg_minimapSize", 120.0f, 64.0f, 200.0f, 0x1001);
    cg_minimapRange = Dvar_RegisterFloat("cg_minimapRange", 1400.0f, 400.0f, 4000.0f, 0x1001);

    strncpy(s_minimap.mapName, mapName, sizeof(s_minimap.mapName) - 1);
    s_minimap.mapName[sizeof(s_minimap.mapName) - 1] = 0;
    s_minimap.attempted = 0;
    s_minimap.ready = 0;
    s_minimap.hudOffset = 0.0f;
    s_minimap.mapMaterial = NULL;
    s_minimap.overviewMaterial = NULL;
    s_minimap.arrowMaterial = NULL;
    s_minimap.dotMaterial = NULL;
    if (cg_minimap->current.enabled)
        CG_MinimapPrepare();
}

int CG_MinimapIsActive(void)
{
    return cg_minimap && cg_minimap->current.enabled && CG_MinimapPrepare();
}

void CG_MinimapSetHudVisible(int visible)
{
    s_minimap.hudOffset = CG_MinimapComputeHudOffset(visible && CG_MinimapIsActive(), cg_minimapSize ? cg_minimapSize->current.value : 0.0f);
}

float CG_MinimapTopLeftOffset(void)
{
    return s_minimap.hudOffset;
}

/* Script elements placed in the minimap's column of the top-left corner. */
float CG_MinimapHudElemOffset(int alignScreen, float x)
{
    if (alignScreen != MINIMAP_TOP_LEFT || x >= s_minimap.hudOffset)
        return 0.0f;
    return s_minimap.hudOffset;
}

static void CG_MinimapFlush(MinimapBatch *batch, MaterialHandle material)
{
    if (batch->vertexCount)
        R_AddCmdDrawTriangles2D(batch->verts, batch->vertexCount, batch->indices, batch->indexCount, material);
    batch->vertexCount = 0;
    batch->indexCount = 0;
}

static void CG_MinimapDrawObjectives(const MinimapView *view, int team, float alpha)
{
    float half = view->radius * 0.14f;
    vec4_t color = {1.0f, 1.0f, 1.0f, alpha};
    int i;

    for (i = 0; i < 16; i++) {
        const objective_t *objective = &cg->snap->ps.objective[i];
        char materialName[64];
        const float *origin = objective->origin;
        float x;
        float y;

        if (objective->state != OBJST_ACTIVE && objective->state != OBJST_CURRENT)
            continue;
        if (objective->teamNum && objective->teamNum != team)
            continue;
        if (!CG_ServerMaterialName(objective->icon, materialName, sizeof(materialName)))
            continue;
        if (objective->entNum >= 0 && objective->entNum < MINIMAP_ENTITYNUM_NONE && cg_entities[objective->entNum].nextValid)
            origin = cg_entities[objective->entNum].lerpOrigin;
        CG_MinimapWorldToScreen(view, origin[0], origin[1], &x, &y);
        CG_MinimapClampToDisc(view, view->radius - half, &x, &y);
        CL_DrawStretchPicPhysical(x - half, y - half, 2.0f * half, 2.0f * half, 0.0f, 0.0f, 1.0f, 1.0f, color,
                                  CL_RegisterMaterial(materialName, 7));
    }
}

static void CG_MinimapDrawActors(const MinimapView *view, int team, float alpha)
{
    static MinimapBatch dots;
    static MinimapBatch arrows;
    float dotHalf = view->radius * 0.06f;
    float friendHalf = view->radius * 0.085f;
    float playerHalf = view->radius * 0.095f;
    float fadeMs = (*dvar_compassPingFadeTime)->current.value * 1000.0f;
    float refYaw = cg->refdefViewAngles[1];
    int self = cg->snap->ps.clientNum;
    int i;

    /* Enemies show where they fired, fading like the compass pings. */
    for (i = 0; i < 64; i++) {
        const compassactor_t *actor = &cg->compassActors[i];
        float pingAlpha;
        float rgba[4] = {1.0f, 0.25f, 0.2f, 0.0f};
        float x;
        float y;

        if (!actor->enemy || !actor->beginFadeTime || fadeMs <= 0.0f || (float)cg->time > (float)actor->beginFadeTime + fadeMs)
            continue;
        pingAlpha = cg->time <= actor->beginFadeTime ? 1.0f : 1.0f - (float)(cg->time - actor->beginFadeTime) / fadeMs;
        rgba[3] = alpha * CG_MinimapClamp(pingAlpha, 0.0f, 1.0f);
        CG_MinimapWorldToScreen(view, actor->vLastPos[0], actor->vLastPos[1], &x, &y);
        CG_MinimapClampToDisc(view, view->radius - dotHalf, &x, &y);
        CG_MinimapAddIcon(&dots, x, y, dotHalf, 0.0f, rgba);
    }
    CG_MinimapFlush(&dots, s_minimap.dotMaterial);

    if (team > 0) {
        for (i = 0; i < 64; i++) {
            compassactor_t *actor = &cg->compassActors[i];
            float rgba[4] = {0.3f, 0.82f, 1.0f, alpha};
            float x;
            float y;

            if (actor->iLastUpdate > cg->time)
                actor->iLastUpdate = 0;
            if (actor->iLastUpdate < cg->time - MINIMAP_ACTOR_TIMEOUT || i == self)
                continue;
            if (fabsf(actor->vLastPos[0]) <= 1.0f && fabsf(actor->vLastPos[1]) <= 1.0f) {
                /* Out of the server's packing range: only the direction is known. */
                CG_MinimapWorldToScreen(view, view->originX + actor->vLastPos[0] * 1e5f, view->originY + actor->vLastPos[1] * 1e5f, &x, &y);
            } else {
                CG_MinimapWorldToScreen(view, actor->vLastPos[0], actor->vLastPos[1], &x, &y);
            }
            CG_MinimapClampToDisc(view, view->radius - friendHalf, &x, &y);
            if (actor->pingTime > cg->time && (actor->pingTime - cg->time) % 500 > 249) {
                rgba[0] = 0.8f;
                rgba[1] = 0.95f;
            }
            CG_MinimapAddIcon(&arrows, x, y, friendHalf, refYaw - actor->fLastYaw, rgba);
        }
    }
    {
        const float white[4] = {1.0f, 0.97f, 0.88f, alpha};

        CG_MinimapAddIcon(&arrows, view->centerX, view->centerY, playerHalf, 0.0f, white);
    }
    CG_MinimapFlush(&arrows, s_minimap.arrowMaterial);
}

void CG_DrawMinimap(float alpha)
{
    static HudVertex2D verts[1 + 2 * MINIMAP_SEGMENTS];
    static unsigned short indices[9 * MINIMAP_SEGMENTS];
    MinimapView view;
    float x = MINIMAP_MARGIN;
    float y = MINIMAP_MARGIN;
    float w;
    float h;
    float yaw;
    int vertexCount;
    int indexCount;
    int team;

    if (alpha <= 0.0f || !CG_MinimapIsActive())
        return;

    w = h = cg_minimapSize->current.value;
    CalcScreenPlacement(&x, &y, &w, &h, 1, 1);
    view.radius = 0.5f * (w < h ? w : h);
    view.centerX = x + 0.5f * w;
    view.centerY = y + 0.5f * h;
    view.scale = view.radius / cg_minimapRange->current.value;
    view.originX = cg->refdef.vieworg[0];
    view.originY = cg->refdef.vieworg[1];
    yaw = cg->refdefViewAngles[1] * (3.14159265f / 180.0f);
    view.sinYaw = sinf(yaw);
    view.cosYaw = cosf(yaw);

    vertexCount = CG_MinimapBuildDisc(&view, &s_minimap.layout, alpha, verts, indices, &indexCount);
    R_AddCmdDrawTriangles2D(verts, vertexCount, indices, indexCount, s_minimap.mapMaterial);

    team = CG_CompassUpdateFriendlies();
    CG_MinimapDrawObjectives(&view, team, alpha);
    CG_MinimapDrawActors(&view, team, alpha);
}

/* The whole map as the pause menu draws it: physical pixels from its top-left
 * corner, and turned a quarter clockwise (north to the right) when that draws
 * it larger in its box. */
typedef struct MinimapOverview {
    /* The texels drawn, x0, y0, x1, y1 from the texture's corner. */
    int texels[4];
    float x;
    float y;
    float width;
    float height;
    float scale;
    int turned;
} MinimapOverview;

/* The texels as large as they fit in the box (physical pixels), centered,
 * turned when that is larger. */
static void CG_MinimapOverviewFit(const MinimapLayout *layout, const int *texels, float x, float y, float w, float h,
                                  MinimapOverview *overview)
{
    float across = (float)(texels[2] - texels[0]);
    float down = (float)(texels[3] - texels[1]);
    float northScale = fminf(w / across, h / down);
    float turnedScale = fminf(w / down, h / across);

    memcpy(overview->texels, texels, sizeof(overview->texels));
    overview->turned = turnedScale > northScale;
    overview->scale = (overview->turned ? turnedScale : northScale) / layout->texelSize;
    overview->width = (overview->turned ? down : across) * layout->texelSize * overview->scale;
    overview->height = (overview->turned ? across : down) * layout->texelSize * overview->scale;
    overview->x = x + 0.5f * (w - overview->width);
    overview->y = y + 0.5f * (h - overview->height);
}

static void CG_MinimapOverviewPoint(const MinimapLayout *layout, const MinimapOverview *overview, float worldX,
                                    float worldY, float *x, float *y)
{
    float east = worldX - (layout->minX + (float)overview->texels[0] * layout->texelSize);
    float south = layout->maxY - (float)overview->texels[1] * layout->texelSize - worldY;

    if (overview->turned) {
        *x = overview->x +
             ((float)(overview->texels[3] - overview->texels[1]) * layout->texelSize - south) * overview->scale;
        *y = overview->y + east * overview->scale;
    } else {
        *x = overview->x + east * overview->scale;
        *y = overview->y + south * overview->scale;
    }
}

/* An icon's clockwise turn on the overview for a world yaw: up is north,
 * or east when the map is turned. */
static float CG_MinimapOverviewAngle(const MinimapOverview *overview, float yaw)
{
    return 90.0f - yaw + (overview->turned ? 90.0f : 0.0f);
}

static void CG_MinimapOverviewObjectives(const MinimapOverview *overview, int team, float half, float alpha)
{
    vec4_t color = {1.0f, 1.0f, 1.0f, alpha};
    int i;

    for (i = 0; i < 16; i++) {
        const objective_t *objective = &cg->snap->ps.objective[i];
        char materialName[64];
        const float *origin = objective->origin;
        float x;
        float y;

        if (objective->state != OBJST_ACTIVE && objective->state != OBJST_CURRENT)
            continue;
        if (objective->teamNum && objective->teamNum != team)
            continue;
        if (!CG_ServerMaterialName(objective->icon, materialName, sizeof(materialName)))
            continue;
        if (objective->entNum >= 0 && objective->entNum < MINIMAP_ENTITYNUM_NONE && cg_entities[objective->entNum].nextValid)
            origin = cg_entities[objective->entNum].lerpOrigin;
        CG_MinimapOverviewPoint(&s_minimap.layout, overview, origin[0], origin[1], &x, &y);
        CL_DrawStretchPicPhysical(x - half, y - half, 2.0f * half, 2.0f * half, 0.0f, 0.0f, 1.0f, 1.0f, color,
                                  CL_RegisterMaterial(materialName, 7));
    }
}

/* The image's corners clockwise from the top left; turned, the map's
 * south-west corner is the top left. */
static void CG_MinimapOverviewQuad(const MinimapLayout *layout, const MinimapOverview *overview, const float *rgba,
                                   HudVertex2D *verts)
{
    float s0 = (float)overview->texels[0] / (float)layout->width;
    float t0 = (float)overview->texels[1] / (float)layout->height;
    float s1 = (float)overview->texels[2] / (float)layout->width;
    float t1 = (float)overview->texels[3] / (float)layout->height;
    float right = overview->x + overview->width;
    float bottom = overview->y + overview->height;

    if (overview->turned) {
        CG_MinimapSetVertex(&verts[0], overview->x, overview->y, s0, t1, rgba);
        CG_MinimapSetVertex(&verts[1], right, overview->y, s0, t0, rgba);
        CG_MinimapSetVertex(&verts[2], right, bottom, s1, t0, rgba);
        CG_MinimapSetVertex(&verts[3], overview->x, bottom, s1, t1, rgba);
    } else {
        CG_MinimapSetVertex(&verts[0], overview->x, overview->y, s0, t0, rgba);
        CG_MinimapSetVertex(&verts[1], right, overview->y, s1, t0, rgba);
        CG_MinimapSetVertex(&verts[2], right, bottom, s1, t1, rgba);
        CG_MinimapSetVertex(&verts[3], overview->x, bottom, s0, t1, rgba);
    }
}

void CG_DrawMinimapOverview(float x, float y, float w, float h, int horzAlign, int vertAlign, float alpha)
{
    static const unsigned short quad[6] = {3, 0, 2, 2, 0, 1};
    static MinimapBatch arrows;
    const MinimapLayout *layout = &s_minimap.layout;
    const float white[4] = {1.0f, 1.0f, 1.0f, alpha};
    const float viewer[4] = {1.0f, 0.97f, 0.88f, alpha};
    HudVertex2D verts[4];
    MinimapOverview overview;
    float size;
    int team;
    int self;
    int i;

    if (alpha <= 0.0f || !cg->snap || !CG_MinimapPrepare() || !s_minimap.overviewMaterial)
        return;
    CalcScreenPlacement(&x, &y, &w, &h, horzAlign, vertAlign);
    CG_MinimapOverviewFit(layout, s_minimap.overviewBounds, x, y, w, h, &overview);
    CG_MinimapOverviewQuad(layout, &overview, white, verts);
    R_AddCmdDrawTriangles2D(verts, 4, quad, 6, s_minimap.overviewMaterial);

    size = fminf(w, h);
    team = CG_CompassUpdateFriendlies();
    CG_MinimapOverviewObjectives(&overview, team, MINIMAP_OVERVIEW_OBJECTIVE * size, alpha);
    self = cg->snap->ps.clientNum;
    for (i = 0; team > 0 && i < 64; i++) {
        const compassactor_t *actor = &cg->compassActors[i];
        float rgba[4] = {0.3f, 0.82f, 1.0f, alpha};
        float px;
        float py;

        if (i == self || actor->iLastUpdate > cg->time || actor->iLastUpdate < cg->time - MINIMAP_ACTOR_TIMEOUT)
            continue;
        /* Out of the server's packing range only the direction is known. */
        if (fabsf(actor->vLastPos[0]) <= 1.0f && fabsf(actor->vLastPos[1]) <= 1.0f)
            continue;
        CG_MinimapOverviewPoint(layout, &overview, actor->vLastPos[0], actor->vLastPos[1], &px, &py);
        CG_MinimapAddIcon(&arrows, px, py, MINIMAP_OVERVIEW_FRIEND * size,
                          CG_MinimapOverviewAngle(&overview, actor->fLastYaw), rgba);
    }
    {
        float px;
        float py;

        CG_MinimapOverviewPoint(layout, &overview, cg->refdef.vieworg[0], cg->refdef.vieworg[1], &px, &py);
        CG_MinimapAddIcon(&arrows, px, py, MINIMAP_OVERVIEW_ARROW * size,
                          CG_MinimapOverviewAngle(&overview, cg->refdefViewAngles[1]), viewer);
    }
    CG_MinimapFlush(&arrows, s_minimap.arrowMaterial);
}
