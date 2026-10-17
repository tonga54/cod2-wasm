#!/usr/bin/env python3
"""Run the production minimap build and drawing math on synthetic levels under ASan/UBSan."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_minimap_mp.c').read_text()
header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()


def function(name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', source, re.M)
    assert match, name
    brace = source.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


definitions = source[source.index('#define MINIMAP_TEXTURE_MAX'):source.index('static const dvar_t *cg_minimap;')]
names = (
    'CG_MinimapNextPow2', 'CG_MinimapAreaBounds', 'CG_MinimapComputeLayout', 'CG_MinimapFilterLine',
    'CG_MinimapFilter2D', 'CG_MinimapGroundLevel', 'CG_MinimapClassify', 'CG_MinimapRemoveSpecks',
    'CG_MinimapGroundColor', 'CG_MinimapSlope', 'CG_MinimapClamp', 'CG_MinimapPutTexel', 'CG_MinimapIsBuilding',
    'CG_MinimapColorize', 'CG_MinimapInsidePolygon', 'CG_MinimapPolygonDistance', 'CG_MinimapRasterIcon',
    'CG_MinimapWorldToScreen', 'CG_MinimapScreenToTexture', 'CG_MinimapClampToDisc', 'CG_MinimapSetVertex',
    'CG_MinimapBuildDisc', 'CG_MinimapBuildRing', 'CG_MinimapAddIcon', 'CG_MinimapComputeHudOffset',
    'CG_MinimapReadToken', 'CG_MinimapParseSpawns', 'CG_MinimapHudElemOffset')
body = definitions + ''.join(function(name) for name in names)

support = r'''
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte; typedef float vec_t;
typedef struct Material Material; typedef Material *MaterialHandle;
typedef struct { unsigned short id, byteCount; } GfxCmdHeader;
typedef union { unsigned int packed; byte array[4]; } GfxColor;
''' + header + r'''
static struct { float hudOffset; } s_minimap;
'''

checks = r'''
static void near(float a, float b, float eps) {
    if (!(fabsf(a - b) <= eps)) { fprintf(stderr, "%.6f != %.6f\n", a, b); abort(); }
}
static float cross(const HudVertex2D *a, const HudVertex2D *b, const HudVertex2D *c) {
    return (b->xy[0] - a->xy[0]) * (c->xy[1] - a->xy[1]) - (b->xy[1] - a->xy[1]) * (c->xy[0] - a->xy[0]);
}
/* The engine's quads (TL, TR, BR, BL drawn as 3,0,2 and 2,0,1) wind with a
 * positive cross product in y-down screen space; every triangle must match. */
static void windsLikeQuads(const HudVertex2D *verts, int vertexCount, const unsigned short *indices, int indexCount) {
    assert(indexCount % 3 == 0);
    for (int i = 0; i < indexCount; i += 3) {
        assert(indices[i] < vertexCount && indices[i + 1] < vertexCount && indices[i + 2] < vertexCount);
        assert(cross(&verts[indices[i]], &verts[indices[i + 1]], &verts[indices[i + 2]]) > 0.0f);
    }
}

static void layout(void) {
    float mins[2] = {-2000, -1500}, maxs[2] = {2000, 1500};
    MinimapLayout l;
    assert(CG_MinimapComputeLayout(mins, maxs, &l));
    near(l.texelSize, MINIMAP_MIN_TEXEL_SIZE, 0);
    assert(l.usedWidth == 336 && l.usedHeight == 252 && l.width == 512 && l.height == 256);
    /* The outermost used texels lie outside the area: a void border that
     * clamped sampling repeats past the edge. */
    assert(l.minX + 0.5f * l.texelSize < mins[0] && l.maxY - 0.5f * l.texelSize > maxs[1]);
    assert(l.minX + (l.usedWidth - 0.5f) * l.texelSize > maxs[0]);
    assert(l.maxY - (l.usedHeight - 0.5f) * l.texelSize < mins[1]);
    assert(l.minX + (l.usedWidth - 1.5f) * l.texelSize < maxs[0] + l.texelSize);
    /* A large level keeps about 400 texels across and fits the texture. */
    float bigMins[2] = {-10000, -3000}, bigMaxs[2] = {10000, 3000};
    assert(CG_MinimapComputeLayout(bigMins, bigMaxs, &l));
    near(l.texelSize, 50, 1e-3f);
    assert(l.usedWidth == 402 && l.width == 512 && l.usedHeight == 122 && l.height == 128);
    float tinyMins[2] = {0, 0}, tinyMaxs[2] = {40, 1000};
    assert(!CG_MinimapComputeLayout(tinyMins, tinyMaxs, &l));
}

static void area(void) {
    float drawMins[3] = {-8000, -8000, -100}, drawMaxs[3] = {8000, 8000, 900};
    float worldMins[3] = {-9000, -6000, -200}, worldMaxs[3] = {9000, 9000, 4000};
    float spawns[3][3] = {{-1000, 200, 0}, {1500, -700, 0}, {300, 2500, 0}};
    float mins[2], maxs[2];
    CG_MinimapAreaBounds(drawMins, drawMaxs, worldMins, worldMaxs, (const float (*)[3])spawns, 3, mins, maxs);
    near(mins[0], -1000 - MINIMAP_SPAWN_MARGIN, 0); near(maxs[0], 1500 + MINIMAP_SPAWN_MARGIN, 0);
    near(mins[1], -700 - MINIMAP_SPAWN_MARGIN, 0); near(maxs[1], 2500 + MINIMAP_SPAWN_MARGIN, 0);
    CG_MinimapAreaBounds(drawMins, drawMaxs, worldMins, worldMaxs, (const float (*)[3])spawns, 0, mins, maxs);
    near(mins[0], -8000, 0); near(mins[1], -6000, 0); near(maxs[0], 8000, 0); near(maxs[1], 8000, 0);
    float edge[1][3] = {{7900, 0, 0}};
    CG_MinimapAreaBounds(drawMins, drawMaxs, worldMins, worldMaxs, (const float (*)[3])edge, 1, mins, maxs);
    near(maxs[0], 8000, 0); near(mins[0], 7900 - MINIMAP_SPAWN_MARGIN, 0);
}

static void filters(void) {
    srand(7);
    for (int trial = 0; trial < 400; trial++) {
        int count = 1 + rand() % 40, radius = rand() % 12, takeMax = rand() & 1, stride = 1 + rand() % 3;
        float src[160], dst[160], prefix[64], suffix[64];
        for (int i = 0; i < count * stride; i++) src[i] = (float)(rand() % 1000) - 500.0f;
        memcpy(dst, src, sizeof(src));
        CG_MinimapFilterLine(dst, dst, count, stride, radius, takeMax, prefix, suffix);
        for (int i = 0; i < count; i++) {
            float best = takeMax ? -FLT_MAX : FLT_MAX;
            for (int k = i - radius; k <= i + radius; k++)
                if (k >= 0 && k < count) best = takeMax ? fmaxf(best, src[k * stride]) : fminf(best, src[k * stride]);
            assert(dst[i * stride] == best);
        }
        for (int i = 0; i < count * stride; i++) if (i % stride) assert(dst[i] == src[i]);
    }
    enum { W = 23, H = 17, R = 3 };
    float grid[W * H], copy[W * H], prefix[W + 2 * R], suffix[W + 2 * R];
    for (int i = 0; i < W * H; i++) grid[i] = copy[i] = (float)(rand() % 300);
    CG_MinimapFilter2D(grid, W, H, R, 0, prefix, suffix);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        float best = FLT_MAX;
        for (int v = y - R; v <= y + R; v++) for (int u = x - R; u <= x + R; u++)
            if (u >= 0 && v >= 0 && u < W && v < H) best = fminf(best, copy[v * W + u]);
        assert(grid[y * W + x] == best);
    }
}

/* A 96 x 64 level of 12-unit texels: flat dirt with a paved road and a lawn,
 * a flat-roof house, a walled courtyard, a terrace wider than the filter, a
 * hill, a post, a long wall, a pool and void past the east edge. */
enum { LW = 96, LH = 64, RADIUS = 10 };
static float heights[LW * LH], ground[LW * LH], prefix[LW + 2 * RADIUS], suffix[LW + 2 * RADIUS];
static byte flags[LW * LH], classes[LW * LH], classes2[LW * LH], pixels[LW * LH * 4];
static int queue[LW * LH];
#define AT(x, y) ((y) * LW + (x))
static void level(void) {
    for (int y = 0; y < LH; y++) for (int x = 0; x < LW; x++) {
        int i = AT(x, y);
        heights[i] = 0; flags[i] = 6;
        if (x < 2 || y == 0 || y == LH - 1 || x >= 88) { flags[i] = MINIMAP_TEXEL_VOID; heights[i] = -500; continue; }
        if (y >= 20 && y < 24 && x < 40) flags[i] = 22;
        if (x >= 24 && x < 30 && y >= 43 && y < 49) flags[i] = 10;
        if (x >= 10 && x < 22 && y >= 6 && y < 14) heights[i] = 220;
        if (x >= 40 && x < 60 && y >= 6 && y < 30 && !(x >= 43 && x < 57 && y >= 9 && y < 27)) heights[i] = 300;
        if (x >= 62 && x < 86 && y >= 2 && y < 26) heights[i] = 60;
        float d = hypotf((float)(x - 75), (float)(y - 45));
        if (d < 18) heights[i] = 150 * (1 - d / 18);
        if (x == 30 && y == 50) heights[i] = 100;
        if (y == 40 && x >= 5 && x < 35) heights[i] = 120;
        if (x >= 15 && x < 25 && y >= 52 && y < 60) { heights[i] = -10; flags[i] = 6 | MINIMAP_TEXEL_WATER; }
    }
}

static void classify(void) {
    level();
    CG_MinimapGroundLevel(heights, flags, LW, LH, RADIUS, ground, prefix, suffix);
    for (int i = 0; i < LW * LH; i++) if (!(flags[i] & MINIMAP_TEXEL_VOID)) assert(ground[i] <= heights[i]);
    /* The opening keeps wide ground and removes narrow blocks. */
    near(ground[AT(16, 10)], 0, 0); near(ground[AT(50, 7)], 0, 0); near(ground[AT(50, 18)], 0, 0);
    near(ground[AT(5, 30)], 0, 0);
    for (int y = 2; y < 26; y++) for (int x = 62; x < 86; x++) near(ground[AT(x, y)], 60, 0);
    int seed = AT(5, 30);
    CG_MinimapClassify(heights, flags, ground, LW, LH, 12.0f, &seed, 1, classes, queue);
    CG_MinimapRemoveSpecks(classes, LW, LH, (int)(MINIMAP_MIN_BUILDING_AREA / 144.0f + 0.5f), queue);
    assert(classes[AT(1, 30)] == MINIMAP_CLASS_VOID && classes[AT(90, 30)] == MINIMAP_CLASS_VOID);
    for (int y = 6; y < 14; y++) for (int x = 10; x < 22; x++) assert(classes[AT(x, y)] == MINIMAP_CLASS_BUILDING);
    assert(classes[AT(41, 15)] == MINIMAP_CLASS_BUILDING && classes[AT(58, 28)] == MINIMAP_CLASS_BUILDING);
    /* The closed courtyard and the terrace cannot be walked to but are at
     * their ground level. */
    for (int y = 9; y < 27; y++) for (int x = 43; x < 57; x++) assert(classes[AT(x, y)] == MINIMAP_CLASS_GROUND);
    for (int y = 2; y < 26; y++) for (int x = 62; x < 86; x++) assert(classes[AT(x, y)] == MINIMAP_CLASS_GROUND);
    /* The whole hill can be climbed, top included. */
    for (int y = 28; y < 62; y++) for (int x = 58; x < 88; x++)
        if (hypotf((float)(x - 75), (float)(y - 45)) < 18) assert(classes[AT(x, y)] == MINIMAP_CLASS_GROUND);
    assert(heights[AT(75, 45)] - ground[AT(75, 45)] > MINIMAP_LOW_GROUND);
    /* The post is dropped, the long wall stays. */
    assert(classes[AT(30, 50)] == MINIMAP_CLASS_GROUND);
    for (int x = 5; x < 35; x++) assert(classes[AT(x, 40)] == MINIMAP_CLASS_BUILDING);
    assert(classes[AT(20, 55)] == MINIMAP_CLASS_GROUND && classes[AT(5, 21)] == MINIMAP_CLASS_GROUND);
    /* Without spawns on the ground, texels at ground level start the walk. */
    CG_MinimapClassify(heights, flags, ground, LW, LH, 12.0f, NULL, 0, classes2, queue);
    CG_MinimapRemoveSpecks(classes2, LW, LH, (int)(MINIMAP_MIN_BUILDING_AREA / 144.0f + 0.5f), queue);
    assert(!memcmp(classes, classes2, sizeof(classes)));
}

static const byte *texel(int x, int y) { return pixels + 4 * AT(x, y); }
static int red(int x, int y) { return texel(x, y)[2]; }
static int blue(int x, int y) { return texel(x, y)[0]; }
static int bright(int x, int y) { return texel(x, y)[0] + texel(x, y)[1] + texel(x, y)[2]; }
static void colors(void) {
    CG_MinimapColorize(heights, flags, classes, LW, LH, 12.0f, pixels);
    for (int i = 0; i < LW * LH; i++) assert(pixels[4 * i + 3] == 255);
    /* Bytes are B, G, R, A. */
    assert(texel(1, 30)[0] == 12 && texel(1, 30)[1] == 13 && texel(1, 30)[2] == 14);
    assert(texel(16, 10)[0] == 100 && texel(16, 10)[1] == 134 && texel(16, 10)[2] == 150);
    assert(texel(10, 10)[0] == 58 && texel(10, 10)[1] == 77 && texel(10, 10)[2] == 88);
    /* The paved road and the lawn read apart from the dirt, the pool is blue. */
    assert(red(30, 21) - blue(30, 21) + 8 < red(30, 30) - blue(30, 30));
    assert(bright(27, 46) < bright(5, 30) * 0.75f && texel(27, 46)[1] >= texel(27, 46)[2]);
    assert(blue(20, 55) > red(20, 55));
    /* The house shades the ground south-east of it, not north-west. */
    assert(bright(22, 14) < bright(9, 5) * 0.7f && bright(9, 5) == bright(5, 30));
    assert(bright(23, 15) < bright(5, 30) && bright(23, 15) > bright(22, 14));
    /* Light from the north-west: the hill's north-west face is brighter. */
    assert(bright(69, 39) > bright(5, 30) && bright(81, 51) < bright(5, 30));
}

static void icons(void) {
    static byte icon[64 * 64 * 4];
#define A(x, y) icon[4 * ((y) * 64 + (x)) + 3]
#define V(x, y) icon[4 * ((y) * 64 + (x))]
    CG_MinimapRasterIcon(icon, 64, 0);
    assert(A(32, 36) == 255 && V(32, 36) == 255);
    assert(A(0, 0) == 0 && A(63, 0) == 0 && A(0, 63) == 0 && A(63, 63) == 0);
    /* A dark outline around the tip, an open notch under the body. */
    assert(A(32, 2) > 150 && V(32, 2) < 40);
    assert(A(32, 51) < 128);
    for (int y = 0; y < 64; y++) for (int x = 0; x < 64; x++) {
        assert(abs(A(x, y) - A(63 - x, y)) <= 16);
        assert(icon[4 * (y * 64 + x)] == icon[4 * (y * 64 + x) + 1] && icon[4 * (y * 64 + x)] == icon[4 * (y * 64 + x) + 2]);
    }
    int top = 64, bottom = -1;
    for (int y = 0; y < 64; y++) for (int x = 0; x < 64; x++) if (A(x, y) == 255 && V(x, y) == 255) { if (y < top) top = y; if (y > bottom) bottom = y; }
    assert(top <= 10 && bottom >= 50);
    CG_MinimapRasterIcon(icon, 64, 1);
    assert(A(32, 32) == 255 && V(32, 32) == 255);
    assert(A(32, 54) > 150 && V(32, 54) < 40);
    assert(A(0, 0) == 0 && A(32, 63) == 0);
    for (int y = 0; y < 64; y++) for (int x = 0; x < 64; x++) assert(abs(A(x, y) - A(y, x)) <= 1);
}

static MinimapView view(float yawDegrees) {
    float yaw = yawDegrees * 3.14159265f / 180.0f;
    MinimapView v = {100, 80, 90, 90.0f / 1400.0f, 500, -300, sinf(yaw), cosf(yaw)};
    return v;
}
static const MinimapLayout mapLayout = {-2012, 1512, 12, 512, 256, 336, 252};
static void mapping(void) {
    const float yaws[] = {0, 30, 90, 180, 247, -45};
    for (int k = 0; k < 6; k++) {
        MinimapView v = view(yaws[k]);
        float yaw = yaws[k] * 3.14159265f / 180.0f, x, y;
        /* Straight ahead is straight up, the view's right is to the right. */
        CG_MinimapWorldToScreen(&v, 500 + 1000 * cosf(yaw), -300 + 1000 * sinf(yaw), &x, &y);
        near(x, 100, 1e-3f); near(y, 80 - 1000 * v.scale, 1e-3f);
        CG_MinimapWorldToScreen(&v, 500 + 1000 * sinf(yaw), -300 - 1000 * cosf(yaw), &x, &y);
        near(x, 100 + 1000 * v.scale, 1e-3f); near(y, 80, 1e-3f);
        for (int i = 0; i < 50; i++) {
            float wx = -1500 + (float)(rand() % 3000), wy = -1200 + (float)(rand() % 2400), s, t;
            CG_MinimapWorldToScreen(&v, wx, wy, &x, &y);
            CG_MinimapScreenToTexture(&v, &mapLayout, x, y, &s, &t);
            near(s, (wx - mapLayout.minX) / (512 * 12.0f), 2e-5f);
            near(t, (mapLayout.maxY - wy) / (256 * 12.0f), 2e-5f);
        }
    }
    /* Facing east, north is on the left. */
    MinimapView east = view(0);
    float x, y;
    CG_MinimapWorldToScreen(&east, 500, 0, &x, &y);
    assert(x < 100 && fabsf(y - 80) < 1e-3f);
    float px = 300, py = 80;
    assert(CG_MinimapClampToDisc(&east, 50, &px, &py) && px == 150 && py == 80);
    px = 110; py = 70;
    assert(!CG_MinimapClampToDisc(&east, 50, &px, &py) && px == 110 && py == 70);
    px = 400; py = 480;
    assert(CG_MinimapClampToDisc(&east, 50, &px, &py)); near(px, 130, 1e-4f); near(py, 120, 1e-4f);
}

static void disc(void) {
    static HudVertex2D verts[1 + 2 * MINIMAP_SEGMENTS];
    static unsigned short indices[9 * MINIMAP_SEGMENTS];
    MinimapView v = view(30);
    int indexCount;
    int vertexCount = CG_MinimapBuildDisc(&v, &mapLayout, 0.8f, verts, indices, &indexCount);
    assert(vertexCount == 1 + 2 * MINIMAP_SEGMENTS && indexCount == 9 * MINIMAP_SEGMENTS);
    assert(vertexCount <= R_HUD2D_MAX_VERTICES && indexCount <= R_HUD2D_MAX_INDICES);
    windsLikeQuads(verts, vertexCount, indices, indexCount);
    float s, t, areaSum = 0;
    CG_MinimapScreenToTexture(&v, &mapLayout, 100, 80, &s, &t);
    near(verts[0].xy[0], 100, 0); near(verts[0].xy[1], 80, 0); near(verts[0].st[0], s, 0); near(verts[0].st[1], t, 0);
    near(s, (500 - mapLayout.minX) / (512 * 12.0f), 1e-6f);
    for (int i = 0; i < vertexCount; i++) {
        float r = hypotf(verts[i].xy[0] - 100, verts[i].xy[1] - 80);
        CG_MinimapScreenToTexture(&v, &mapLayout, verts[i].xy[0], verts[i].xy[1], &s, &t);
        near(verts[i].st[0], s, 1e-6f); near(verts[i].st[1], t, 1e-6f);
        assert(verts[i].rgba[0] == 1 && verts[i].rgba[1] == 1 && verts[i].rgba[2] == 1);
        if (i == 0) continue;
        near(r, i <= MINIMAP_SEGMENTS ? 89.5f : 90.5f, 1e-3f);
        near(verts[i].rgba[3], i <= MINIMAP_SEGMENTS ? 0.8f : 0.0f, 0);
    }
    for (int i = 0; i < 9 * MINIMAP_SEGMENTS; i += 9) areaSum += 0.5f * cross(&verts[indices[i]], &verts[indices[i + 1]], &verts[indices[i + 2]]);
    near(areaSum, 3.14159265f * 89.5f * 89.5f, 3.14159265f * 89.5f * 89.5f * 0.01f);

    static HudVertex2D ring[2 * MINIMAP_SEGMENTS];
    const float inner[4] = {0, 0, 0, 0.55f}, outer[4] = {0, 0, 0, 0};
    vertexCount = CG_MinimapBuildRing(&v, 90, 95.5f, inner, outer, ring, indices, &indexCount);
    assert(vertexCount == 2 * MINIMAP_SEGMENTS && indexCount == 6 * MINIMAP_SEGMENTS);
    windsLikeQuads(ring, vertexCount, indices, indexCount);
    for (int i = 0; i < vertexCount; i++) {
        float r = hypotf(ring[i].xy[0] - 100, ring[i].xy[1] - 80);
        near(r, i < MINIMAP_SEGMENTS ? 90 : 95.5f, 1e-3f);
        near(ring[i].rgba[3], i < MINIMAP_SEGMENTS ? 0.55f : 0, 0);
    }
}

static void markers(void) {
    static MinimapBatch batch;
    const float rgba[4] = {0.3f, 0.8f, 1, 0.5f};
    CG_MinimapAddIcon(&batch, 50, 60, 10, 0, rgba);
    const float expect[4][4] = {{40, 50, 0, 0}, {60, 50, 1, 0}, {60, 70, 1, 1}, {40, 70, 0, 1}};
    for (int k = 0; k < 4; k++) {
        near(batch.verts[k].xy[0], expect[k][0], 1e-4f); near(batch.verts[k].xy[1], expect[k][1], 1e-4f);
        assert(batch.verts[k].st[0] == expect[k][2] && batch.verts[k].st[1] == expect[k][3]);
        assert(!memcmp(batch.verts[k].rgba, rgba, sizeof(rgba)));
    }
    const unsigned short quad[6] = {3, 0, 2, 2, 0, 1};
    assert(!memcmp(batch.indices, quad, sizeof(quad)));
    /* Turned 90 degrees clockwise, the icon's top edge faces right. */
    CG_MinimapAddIcon(&batch, 50, 60, 10, 90, rgba);
    assert(batch.vertexCount == 8 && batch.indexCount == 12 && batch.indices[6] == 7 && batch.indices[11] == 5);
    float topX = 0, topY = 0;
    for (int k = 4; k < 8; k++) if (batch.verts[k].st[1] == 0) { topX += batch.verts[k].xy[0] * 0.5f; topY += batch.verts[k].xy[1] * 0.5f; }
    near(topX, 60, 1e-4f); near(topY, 60, 1e-4f);
    for (int i = 0; i < MINIMAP_MAX_ICONS + 5; i++) CG_MinimapAddIcon(&batch, 30, 30, 4, (float)(i * 37), rgba);
    assert(batch.vertexCount == 4 * MINIMAP_MAX_ICONS && batch.indexCount == 6 * MINIMAP_MAX_ICONS);
    windsLikeQuads(batch.verts, batch.vertexCount, batch.indices, batch.indexCount);
}

static void spawns(void) {
    const char *entities =
        "{\n\"classname\" \"worldspawn\"\n\"origin\" \"1 2 3\"\n}\n"
        "{\n\"origin\" \"-512 1024.5 16\"\n\"classname\" \"mp_tdm_spawn\"\n\"angles\" \"0 90 0\"\n}\n"
        "{\n\"classname\" \"mp_ctf_spawn_allied\"\n\"origin\" \"10 -20 30\"\n}\n"
        "{\n\"classname\" \"mp_global_intermission\"\n\"origin\" \"5 5 5\"\n}\n"
        "{\n\"classname\" \"mp_dm_spawn\"\n\"origin\" \"bad\"\n}\n"
        "{\n\"classname\" \"info_player_start\"\n\"origin\" \"7 7 7\"\n}\n"
        "{\n\"classname\" \"mp_sd_spawn_attacker\"\n\"origin\" \"1 2\"\n}\n"
        "{\n\"classname\" \"mp_dm_spawn\"\n\"origin\" \"100 200 300\"\n}\n"
        "{\n\"classname\" \"mp_dm_spawn\"\n\"origin\" \"9 9 9\"\n";
    float found[8][3];
    assert(CG_MinimapParseSpawns(entities, found, 8) == 3);
    assert(found[0][0] == -512 && found[0][1] == 1024.5f && found[0][2] == 16);
    assert(found[1][0] == 10 && found[1][1] == -20 && found[1][2] == 30);
    assert(found[2][0] == 100 && found[2][1] == 200 && found[2][2] == 300);
    assert(CG_MinimapParseSpawns(entities, found, 2) == 2);
    assert(CG_MinimapParseSpawns(NULL, found, 8) == 0 && CG_MinimapParseSpawns("", found, 8) == 0);
}

static void hud(void) {
    assert(CG_MinimapComputeHudOffset(1, 120) == 126 && CG_MinimapComputeHudOffset(0, 120) == 0);
    s_minimap.hudOffset = CG_MinimapComputeHudOffset(1, 120);
    /* The stock top-left column (clock, team icons and scores, CTF flag, bomb
     * timer) lands below the minimap; other corners and columns stay. */
    const float stock[][2] = {{8, 2}, {6, 28}, {6, 50}, {36, 26}, {49, 48}, {30, 95}, {6, 76}};
    for (int i = 0; i < 7; i++) assert(stock[i][1] + CG_MinimapHudElemOffset(9, stock[i][0]) >= MINIMAP_MARGIN + 120);
    assert(CG_MinimapHudElemOffset(9, 126) == 0 && CG_MinimapHudElemOffset(9, 300) == 0);
    assert(CG_MinimapHudElemOffset((1 << 3) | 3, 8) == 0 && CG_MinimapHudElemOffset((2 << 3) | 1, 8) == 0);
    assert(CG_MinimapHudElemOffset((1 << 3) | 2, 8) == 0);
    s_minimap.hudOffset = 0;
    assert(CG_MinimapHudElemOffset(9, 8) == 0);
}

int main(void) {
    layout(); area(); filters(); classify(); colors(); icons(); mapping(); disc(); markers(); spawns(); hud();
    return 0;
}
'''


def mutate(old, new):
    assert body.count(old) == 1, old
    return body.replace(old, new)


mutants = [
    mutate('float b = prefix[i + 2 * radius];', 'float b = prefix[i + radius];'),
    mutate('    CG_MinimapFilter2D(ground, width, height, radius, 1, prefix, suffix);\n', ''),
    mutate('fabsf(heights[next] - heights[current]) > step', '0'),
    mutate('heights[i] - ground[i] <= MINIMAP_LOW_GROUND ? MINIMAP_CLASS_GROUND : MINIMAP_CLASS_BUILDING',
           'MINIMAP_CLASS_BUILDING'),
    mutate('    while (head < tail) {\n        int current = queue[head++];',
           '    while (0) {\n        int current = queue[head++];'),
    mutate('value = tail < minArea ? MINIMAP_CLASS_GROUND', 'value = 0 ? MINIMAP_CLASS_GROUND'),
    mutate('CG_MinimapIsBuilding(classes, width, height, x - 1, y - 1)',
           'CG_MinimapIsBuilding(classes, width, height, x + 1, y + 1)'),
    mutate('bgra[2 - c]', 'bgra[c]'),
    mutate('case 2: case 5: case 16: case 17: case 22:\n        return s_minimapHard;',
           'case 2: case 5: case 16: case 17: case 22:\n        return s_minimapDirt;'),
    mutate('case 8: case 10:\n        return s_minimapGrass;', 'case 8: case 10:\n        return s_minimapDirt;'),
    mutate('return (east - west + south - north)', 'return (west - east + north - south)'),
    mutate('*screenX = view->centerX + view->scale * (dx * view->sinYaw - dy * view->cosYaw);',
           '*screenX = view->centerX + view->scale * (dx * view->sinYaw + dy * view->cosYaw);'),
    mutate('float worldY = view->originY - (ox * view->cosYaw + oy * view->sinYaw) / view->scale;',
           'float worldY = view->originY + (ox * view->cosYaw + oy * view->sinYaw) / view->scale;'),
    mutate('CG_MinimapSetVertex(&verts[1 + MINIMAP_SEGMENTS + i], x, y, s, t, clear);',
           'CG_MinimapSetVertex(&verts[1 + MINIMAP_SEGMENTS + i], x, y, s, t, opaque);'),
    mutate('        indices[ic++] = 0;\n        indices[ic++] = (unsigned short)(1 + i);\n'
           '        indices[ic++] = (unsigned short)(1 + next);\n',
           '        indices[ic++] = 0;\n        indices[ic++] = (unsigned short)(1 + next);\n'
           '        indices[ic++] = (unsigned short)(1 + i);\n'),
    mutate('x + lx * c - ly * s, y + lx * s + ly * c', 'x + lx * c + ly * s, y - lx * s + ly * c'),
    mutate('    if (vc + 4 > 4 * MINIMAP_MAX_ICONS)\n        return;\n', ''),
    mutate('strstr(classname, "_spawn")', '1'),
    mutate('if (alignScreen != MINIMAP_TOP_LEFT || x >= s_minimap.hudOffset)', 'if (x >= s_minimap.hudOffset)'),
    mutate('layout->minX = mins[0] - texelSize;', 'layout->minX = mins[0];'),
    mutate('                        alpha += 0.9f;\n                        value += 0.9f * 0.05f;\n', ''),
]
with tempfile.TemporaryDirectory(prefix='cod2-minimap-') as directory:
    path = Path(directory) / 'minimap.c'
    binary = Path(directory) / 'minimap'
    for index, variant in enumerate([body] + mutants):
        path.write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: minimap layout keeps a void border and 400 texels; the opening and walk classify a house, walled '
      'courtyard, terrace, climbable hill, post, wall and pool; road, lawn, pool, shadow and hillshade colors in BGRA; arrow and dot '
      'icons; forward-up mapping with an exact texture inverse; antialiased disc, ring and icon quads wind like the '
      f'engine quads; spawn parsing; top-left HUD shift; {len(mutants)} failing regression mutants')
