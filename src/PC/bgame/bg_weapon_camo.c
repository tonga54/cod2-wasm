#include "headers/PC/bgame/bg_weapon_camo.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

const WeaponCamo g_weaponCamos[WEAPON_CAMO_COUNT] = {
    { "none", "None", "The gun's own finish" },
    { "frogskin", "Frogskin", "Olive and khaki spots" },
    { "brownspot", "Brownspot", "Sun-faded salmon spots" },
    { "splinter", "Splinter", "Angular shards and rain streaks" },
    { "oakleaf", "Oak Leaf", "Autumn foliage" },
    { "peapattern", "Pea Pattern", "Blotches peppered with dots" },
    { "planetree", "Plane Tree", "Dark forest mottle" },
    { "desert", "Desert", "North African sand" },
    { "winter", "Winter", "Snow whitewash" },
    { "ambush", "Ambush", "Spring green and rust" },
    { "ember", "Ember", "Black with glowing red" },
};

/* Rifles and the PPSh, Thompson and trench gun take paint on their wooden
 * furniture, as the WWII rifles do; guns that are mostly metal, and the
 * pistols, take it everywhere. */
const WeaponCamoGun g_weaponCamoGuns[] = {
    { "thompson_mp", "mtl_weapon_thompson", WEAPON_CAMO_WOOD },
    { "greasegun_mp", "mtl_weapon_greasegun", WEAPON_CAMO_BODY },
    { "sten_mp", "mtl_weapon_sten", WEAPON_CAMO_BODY },
    { "PPS42_mp", "mtl_weapon_pps43", WEAPON_CAMO_BODY },
    { "ppsh_mp", "mtl_weapon_ppsh", WEAPON_CAMO_WOOD },
    { "mp40_mp", "mtl_weapon_mp40", WEAPON_CAMO_BODY },
    { "m1garand_mp", "mtl_weapon_m1garand", WEAPON_CAMO_WOOD },
    { "m1carbine_mp", "mtl_weapon_carbine", WEAPON_CAMO_WOOD },
    { "enfield_mp", "mtl_weapon_enfield", WEAPON_CAMO_WOOD },
    { "mosin_nagant_mp", "mtl_weapon_mosin", WEAPON_CAMO_WOOD },
    { "SVT40_mp", "mtl_weapon_svt40", WEAPON_CAMO_WOOD },
    { "kar98k_mp", "mtl_weapon_kar98", WEAPON_CAMO_WOOD },
    { "g43_mp", "mtl_weapon_g43", WEAPON_CAMO_WOOD },
    { "bar_mp", "mtl_weapon_bar", WEAPON_CAMO_BODY },
    { "bren_mp", "mtl_weapon_bren", WEAPON_CAMO_BODY },
    { "mp44_mp", "mtl_weapon_mp44", WEAPON_CAMO_BODY },
    { "springfield_mp", "mtl_weapon_springfield", WEAPON_CAMO_WOOD },
    { "enfield_scope_mp", "mtl_weapon_enfield", WEAPON_CAMO_WOOD },
    { "mosin_nagant_sniper_mp", "mtl_weapon_mosin", WEAPON_CAMO_WOOD },
    { "kar98k_sniper_mp", "mtl_weapon_kar98", WEAPON_CAMO_WOOD },
    { "shotgun_mp", "mtl_weapon_trenchgun", WEAPON_CAMO_WOOD },
    { "colt_mp", "mtl_weapon_colt45", WEAPON_CAMO_BODY },
    { "webley_mp", "mtl_weapon_webley", WEAPON_CAMO_BODY },
    { "TT30_mp", "mtl_tokarevtt30_pistol", WEAPON_CAMO_BODY },
    { "luger_mp", "mtl_weapon_luger", WEAPON_CAMO_BODY },
};
const int g_weaponCamoGunCount = (int)(sizeof(g_weaponCamoGuns) / sizeof(g_weaponCamoGuns[0]));

/* A pattern is a base color under layers painted in order. Every layer's
 * cells divide the tile, so the pattern repeats without a seam. */
enum { CAMO_BLOTCH, CAMO_SHARDS, CAMO_DOTS, CAMO_RAIN };

typedef struct {
    int kind;
    unsigned char color[3];
    /* Lattice cells across and down the tile. */
    int cells[2];
    /* Blotches: the noise level above which they paint. Shards, dots and
     * rain: the share of cells that carry one. */
    float level;
    /* Blotches: how far the outline wanders, in tiles. Dots: their radius,
     * and rain its length, in cells. */
    float size;
    /* The width of an edge's blend, in noise units for blotches and tiles
     * otherwise; for rain, half a streak's width. */
    float edge;
    unsigned int seed;
} CamoLayer;

typedef struct {
    unsigned char base[3];
    int layerCount;
    CamoLayer layers[5];
} CamoPattern;

static const CamoPattern s_camoPatterns[WEAPON_CAMO_COUNT] = {
    { { 0, 0, 0 }, 0, { { 0 } } },
    /* Frogskin */
    { { 120, 132, 82 }, 3, {
        { CAMO_BLOTCH, { 80, 94, 54 }, { 5, 5 }, 0.52f, 0.22f, 0.012f, 11 },
        { CAMO_BLOTCH, { 108, 82, 54 }, { 6, 6 }, 0.555f, 0.26f, 0.012f, 23 },
        { CAMO_BLOTCH, { 164, 150, 106 }, { 7, 7 }, 0.585f, 0.2f, 0.012f, 37 } } },
    /* Brownspot */
    { { 206, 142, 108 }, 2, {
        { CAMO_BLOTCH, { 232, 184, 150 }, { 4, 4 }, 0.53f, 0.24f, 0.012f, 41 },
        { CAMO_BLOTCH, { 166, 98, 70 }, { 5, 5 }, 0.575f, 0.24f, 0.012f, 53 } } },
    /* Splinter */
    { { 146, 148, 124 }, 3, {
        { CAMO_SHARDS, { 112, 88, 62 }, { 7, 5 }, 0.36f, 0.0f, 0.004f, 61 },
        { CAMO_SHARDS, { 86, 104, 66 }, { 6, 4 }, 0.34f, 0.0f, 0.004f, 67 },
        { CAMO_RAIN, { 50, 62, 40 }, { 40, 10 }, 0.42f, 0.55f, 0.0035f, 71 } } },
    /* Oak Leaf */
    { { 150, 124, 80 }, 3, {
        { CAMO_BLOTCH, { 94, 104, 56 }, { 8, 8 }, 0.5f, 0.32f, 0.01f, 83 },
        { CAMO_BLOTCH, { 88, 62, 40 }, { 9, 9 }, 0.565f, 0.3f, 0.01f, 97 },
        { CAMO_BLOTCH, { 36, 34, 28 }, { 10, 10 }, 0.61f, 0.28f, 0.01f, 101 } } },
    /* Pea Pattern */
    { { 116, 96, 64 }, 4, {
        { CAMO_BLOTCH, { 84, 98, 56 }, { 5, 5 }, 0.5f, 0.24f, 0.012f, 107 },
        { CAMO_BLOTCH, { 160, 136, 92 }, { 6, 6 }, 0.585f, 0.22f, 0.012f, 113 },
        { CAMO_DOTS, { 38, 34, 26 }, { 18, 18 }, 0.62f, 0.24f, 0.004f, 127 },
        { CAMO_DOTS, { 146, 156, 94 }, { 22, 22 }, 0.45f, 0.22f, 0.004f, 131 } } },
    /* Plane Tree */
    { { 56, 70, 42 }, 3, {
        { CAMO_BLOTCH, { 30, 36, 24 }, { 6, 6 }, 0.52f, 0.26f, 0.012f, 137 },
        { CAMO_BLOTCH, { 88, 108, 60 }, { 7, 7 }, 0.575f, 0.24f, 0.012f, 139 },
        { CAMO_DOTS, { 124, 136, 80 }, { 20, 20 }, 0.4f, 0.2f, 0.004f, 149 } } },
    /* Desert */
    { { 196, 166, 116 }, 2, {
        { CAMO_BLOTCH, { 156, 124, 82 }, { 4, 4 }, 0.52f, 0.24f, 0.012f, 151 },
        { CAMO_BLOTCH, { 114, 86, 56 }, { 5, 5 }, 0.59f, 0.24f, 0.012f, 157 } } },
    /* Winter */
    { { 222, 224, 218 }, 2, {
        { CAMO_BLOTCH, { 178, 184, 180 }, { 4, 4 }, 0.545f, 0.26f, 0.035f, 163 },
        { CAMO_BLOTCH, { 116, 124, 120 }, { 6, 6 }, 0.6f, 0.24f, 0.02f, 167 } } },
    /* Ambush */
    { { 70, 134, 44 }, 3, {
        { CAMO_BLOTCH, { 176, 100, 40 }, { 5, 5 }, 0.53f, 0.24f, 0.012f, 173 },
        { CAMO_BLOTCH, { 24, 28, 18 }, { 6, 6 }, 0.575f, 0.24f, 0.012f, 179 },
        { CAMO_BLOTCH, { 132, 184, 70 }, { 7, 7 }, 0.61f, 0.2f, 0.012f, 181 } } },
    /* Ember */
    { { 30, 26, 24 }, 2, {
        { CAMO_BLOTCH, { 112, 32, 22 }, { 5, 5 }, 0.55f, 0.26f, 0.012f, 191 },
        { CAMO_BLOTCH, { 200, 78, 34 }, { 6, 6 }, 0.6f, 0.22f, 0.012f, 193 } } },
};

int BG_WeaponCamo_Find(const char *id)
{
    int i;

    for (i = 0; id && i < WEAPON_CAMO_COUNT; i++) {
        if (!strcmp(g_weaponCamos[i].id, id))
            return i;
    }
    return -1;
}

const WeaponCamoGun *BG_WeaponCamo_Gun(const char *weapon)
{
    int i;

    for (i = 0; weapon && i < g_weaponCamoGunCount; i++) {
        if (!strcmp(g_weaponCamoGuns[i].weapon, weapon))
            return &g_weaponCamoGuns[i];
    }
    return NULL;
}

static float Camo_Step(float edge0, float edge1, float x)
{
    float t = (x - edge0) / (edge1 - edge0);

    t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
    return t * t * (3.0f - 2.0f * t);
}

static unsigned int Camo_Hash(int x, int y, unsigned int seed)
{
    unsigned int h = (unsigned int)x * 0x8da6b343u + (unsigned int)y * 0xd8163841u + seed * 0xcb1ab31fu;

    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    h ^= h >> 16;
    return h;
}

/* 0 to 1 from 24 bits of a hash. */
static float Camo_Unit(unsigned int h)
{
    return (float)(h >> 8) * (1.0f / 16777216.0f);
}

static int Camo_Wrap(int i, int period)
{
    i %= period;
    return i < 0 ? i + period : i;
}

static float Camo_Value(float u, float v, int period, unsigned int seed)
{
    float x = u * (float)period, y = v * (float)period;
    float fx = floorf(x), fy = floorf(y);
    float tx = x - fx, ty = y - fy;
    int x0 = Camo_Wrap((int)fx, period), y0 = Camo_Wrap((int)fy, period);
    int x1 = (x0 + 1) % period, y1 = (y0 + 1) % period;
    float a = Camo_Unit(Camo_Hash(x0, y0, seed)), b = Camo_Unit(Camo_Hash(x1, y0, seed));
    float c = Camo_Unit(Camo_Hash(x0, y1, seed)), d = Camo_Unit(Camo_Hash(x1, y1, seed));

    tx = tx * tx * (3.0f - 2.0f * tx);
    ty = ty * ty * (3.0f - 2.0f * ty);
    return a + (b - a) * tx + (c - a) * ty + (a - b - c + d) * tx * ty;
}

/* Each octave doubles the lattice, so all of them tile with the first. */
static float Camo_Fbm(float u, float v, int period, int octaves, unsigned int seed)
{
    float sum = 0.0f, weight = 0.5f, total = 0.0f;
    int i;

    for (i = 0; i < octaves; i++, period *= 2, weight *= 0.5f) {
        sum += weight * Camo_Value(u, v, period, seed + 0x9e3779b9u * (unsigned int)i);
        total += weight;
    }
    return sum / total;
}

static float Camo_Blotch(const CamoLayer *layer, float u, float v)
{
    int period = layer->cells[0];
    float wu = u + layer->size * (Camo_Fbm(u, v, period, 2, layer->seed ^ 0x68e31da4u) - 0.5f);
    float wv = v + layer->size * (Camo_Fbm(u, v, period, 2, layer->seed ^ 0xb5297a4du) - 0.5f);

    return Camo_Step(layer->level - layer->edge, layer->level + layer->edge, Camo_Fbm(wu, wv, period, 3, layer->seed));
}

/* The feature point of a wrapped cell, in cell units from the cell's corner. */
static void Camo_Point(int wx, int wy, unsigned int seed, float *px, float *py)
{
    unsigned int h = Camo_Hash(wx, wy, seed);

    *px = 0.15f + 0.7f * Camo_Unit(h);
    *py = 0.15f + 0.7f * Camo_Unit(Camo_Hash((int)h, wy, seed ^ 0x2545f491u));
}

static int Camo_Carries(int wx, int wy, const CamoLayer *layer)
{
    return Camo_Unit(Camo_Hash(wx, wy, layer->seed ^ 0x94d049bbu)) < layer->level;
}

/* Voronoi cells: straight-edged shards. Where a shard meets a cell without
 * one, the edge blends over the distance between their two feature points. */
static float Camo_Shards(const CamoLayer *layer, float u, float v)
{
    int cu = layer->cells[0], cv = layer->cells[1];
    float x = u * (float)cu, y = v * (float)cv;
    int cx = (int)floorf(x), cy = (int)floorf(y), dx, dy;
    float best = 1e9f, second = 1e9f;
    int bestCarries = 0, secondCarries = 0;

    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            int gx = cx + dx, gy = cy + dy, wx = Camo_Wrap(gx, cu), wy = Camo_Wrap(gy, cv);
            float px, py, ex, ey, d;

            Camo_Point(wx, wy, layer->seed, &px, &py);
            ex = ((float)gx + px - x) / (float)cu;
            ey = ((float)gy + py - y) / (float)cv;
            d = sqrtf(ex * ex + ey * ey);
            if (d < best) {
                second = best;
                secondCarries = bestCarries;
                best = d;
                bestCarries = Camo_Carries(wx, wy, layer);
            } else if (d < second) {
                second = d;
                secondCarries = Camo_Carries(wx, wy, layer);
            }
        }
    }
    if (bestCarries == secondCarries)
        return (float)bestCarries;
    {
        float inside = 0.5f + 0.5f * Camo_Step(0.0f, layer->edge, 0.5f * (second - best));

        return bestCarries ? inside : 1.0f - inside;
    }
}

static float Camo_Dots(const CamoLayer *layer, float u, float v)
{
    int cells = layer->cells[0];
    float x = u * (float)cells, y = v * (float)cells, coverage = 0.0f;
    int cx = (int)floorf(x), cy = (int)floorf(y), dx, dy;

    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            int gx = cx + dx, gy = cy + dy, wx = Camo_Wrap(gx, cells), wy = Camo_Wrap(gy, cells);
            float px, py, ex, ey, radius, dot;

            if (!Camo_Carries(wx, wy, layer))
                continue;
            Camo_Point(wx, wy, layer->seed, &px, &py);
            ex = ((float)gx + px - x) / (float)cells;
            ey = ((float)gy + py - y) / (float)cells;
            radius = layer->size * (0.6f + 0.4f * Camo_Unit(Camo_Hash(wx, wy, layer->seed ^ 0x165667b1u))) /
                     (float)cells;
            dot = Camo_Step(radius + layer->edge, radius - layer->edge, sqrtf(ex * ex + ey * ey));
            if (dot > coverage)
                coverage = dot;
        }
    }
    return coverage;
}

/* Short vertical streaks, as the rain of a splinter pattern. */
static float Camo_Rain(const CamoLayer *layer, float u, float v)
{
    int cu = layer->cells[0], cv = layer->cells[1];
    float x = u * (float)cu, y = v * (float)cv, coverage = 0.0f;
    int cx = (int)floorf(x), cy = (int)floorf(y), dx, dy;

    for (dy = -1; dy <= 0; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            int gx = cx + dx, gy = cy + dy, wx = Camo_Wrap(gx, cu), wy = Camo_Wrap(gy, cv);
            float px, py, across, along, streak;

            if (!Camo_Carries(wx, wy, layer))
                continue;
            Camo_Point(wx, wy, layer->seed, &px, &py);
            across = fabsf(((float)gx + px - x) / (float)cu);
            along = (y - ((float)gy + py * 0.5f)) / layer->size;
            if (along < 0.0f || along > 1.0f)
                continue;
            streak = Camo_Step(layer->edge * 1.6f, layer->edge * 0.6f, across) *
                     Camo_Step(0.0f, 0.15f, along) * Camo_Step(1.0f, 0.85f, along);
            if (streak > coverage)
                coverage = streak;
        }
    }
    return coverage;
}

static float Camo_Coverage(const CamoLayer *layer, float u, float v)
{
    switch (layer->kind) {
    case CAMO_SHARDS:
        return Camo_Shards(layer, u, v);
    case CAMO_DOTS:
        return Camo_Dots(layer, u, v);
    case CAMO_RAIN:
        return Camo_Rain(layer, u, v);
    default:
        return Camo_Blotch(layer, u, v);
    }
}

int BG_WeaponCamo_Generate(int camo, int size, unsigned char *bgra)
{
    const CamoPattern *pattern;
    int x, y, i, c;

    if (camo <= WEAPON_CAMO_NONE || camo >= WEAPON_CAMO_COUNT || !bgra || size < 8 || size > 1024 ||
        (size & (size - 1)))
        return 0;
    pattern = &s_camoPatterns[camo];
    for (y = 0; y < size; y++) {
        for (x = 0; x < size; x++) {
            float u = ((float)x + 0.5f) / (float)size, v = ((float)y + 0.5f) / (float)size;
            /* A faint mottle, as paint over a gun is never one flat color. */
            float grain = 0.94f + 0.12f * Camo_Fbm(u, v, 32, 2, (unsigned int)camo * 0x51ed27u);
            float rgb[3];
            unsigned char *texel = bgra + ((size_t)y * (size_t)size + (size_t)x) * 4;

            for (c = 0; c < 3; c++)
                rgb[c] = (float)pattern->base[c];
            for (i = 0; i < pattern->layerCount; i++) {
                const CamoLayer *layer = &pattern->layers[i];
                float a = Camo_Coverage(layer, u, v);

                for (c = 0; c < 3; c++)
                    rgb[c] += ((float)layer->color[c] - rgb[c]) * a;
            }
            for (c = 0; c < 3; c++) {
                float value = rgb[c] * grain + 0.5f;

                rgb[c] = value < 0.0f ? 0.0f : value > 255.0f ? 255.0f : value;
            }
            texel[0] = (unsigned char)rgb[2];
            texel[1] = (unsigned char)rgb[1];
            texel[2] = (unsigned char)rgb[0];
            texel[3] = 255;
        }
    }
    return 1;
}
