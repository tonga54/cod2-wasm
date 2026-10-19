#!/usr/bin/env python3
"""Run the production 2D triangle command, HUD image materials and world bounds against a recording backend."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
gfx = root / 'src/PC/gfx_d3d'
commands = (gfx / 'r_rendercmds.c').read_text()
backend = (gfx / 'rb_backend.c').read_text()
utils = (gfx / 'r_utils.c').read_text()
images = (gfx / 'r_hud2d.c').read_text()
init = (gfx / 'r_init.c').read_text()
header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()


def function(source, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', source, re.M)
    assert match, name
    brace = source.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


table = backend[backend.index('void (*const RB_RenderCommandTable[34])'):]
entries = [entry.strip() for entry in table[table.index('{') + 1:table.index('};')].split(',') if entry.strip()]
assert len(entries) == 34 and entries[6] == 'RB_DrawTriangles2DCmd', entries[:10]
assert [entries[i] for i in (0, 7, 8, 9, 28, 30, 31, 32)] == ['NULL'] * 8
assert 'return RB_CheckTessOverflow(t, 4, 6);' in function(backend, 'RB_CheckTessOverflow4')
assert 'R_ShutdownHudImages();\n    R_ShutdownImages();' in init

body = (function(utils, 'R_ConvertColorToBytes') + function(commands, 'R_AddCmdDrawTriangles2D') +
        function(backend, 'RB_CheckTessOverflow') + function(backend, 'RB_BeginSurface2D') +
        function(backend, 'RB_DrawTriangles2DCmd') + images[images.index('/* Linear filtering'):])

support = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte; typedef float vec_t; typedef vec_t vec3_t[3];
typedef struct GfxImageFileHeader { char tag[3]; byte version, format, flags; short dimensions[3]; int fileSizeForPicmip[4]; } GfxImageFileHeader;
typedef unsigned int D3DCOLOR; typedef unsigned short r_index_t; typedef int D3DFORMAT; typedef unsigned long DWORD;
#define D3DFMT_A8R8G8B8 21
typedef struct { unsigned short id, byteCount; } GfxCmdHeader;
typedef union { unsigned int packed; byte array[4]; } GfxColor;
typedef struct GfxImage { int mapType; struct { void *map; } texture; int width, height; const char *name; } GfxImage;
typedef struct { const char *name; byte samplerState, semantic, unused_0, unused_1; union { GfxImage *image; } u; } MaterialTextureDef;
typedef struct Material { struct { const char *name; byte gameFlags; } info; int stateBits[2];
    unsigned short textureCount, constantCount; void *techniqueSet; MaterialTextureDef *textures; void *constants; } Material;
typedef Material *MaterialHandle;
typedef struct { int surfaceType; vec3_t bounds[2]; } srfTriangles_t;
typedef struct { const Material *material; unsigned short lightmapIndex, sortGroup; srfTriangles_t *tris; } GfxSurface;
typedef struct { int surfaceCount; GfxSurface *surfaces; } GfxWorld;
static struct { Material *defaultMaterial, *rawMaterial; GfxWorld *world; } rgp;
typedef struct dvar_t { int unused; } dvar_t;
static const dvar_t rendererDvar;
static const dvar_t *r_rendererInUse = &rendererDvar;
typedef struct { const void *cmd; } GfxRenderCommandExecState;
''' + header + r'''
static byte cmdBuffer[1 << 17] __attribute__((aligned(16)));
static int cmdUsed;
static void *R_AllocCmd(int byteCount, int critical, int id) {
    assert(id == 6 && critical == 0 && byteCount > 0 && byteCount < 65536 && byteCount % 4 == 0);
    if (cmdUsed + byteCount + 4 > (int)sizeof(cmdBuffer)) return NULL;
    GfxCmdHeader *cmd = (GfxCmdHeader *)(cmdBuffer + cmdUsed);
    cmd->id = (unsigned short)id; cmd->byteCount = (unsigned short)byteCount;
    cmdUsed += byteCount;
    memset(cmdBuffer + cmdUsed, 0, 4);
    return cmd;
}
#define TESS_VERTICES 0x154a
static struct { int vertexCount, indexCount, optimizedIndexCount, declType, techType, lmapIndex; const Material *material;
    r_index_t indices[0x100000]; } tess;
static struct { int projection2D; } backEnd;
static struct { float x, y, s, t; D3DCOLOR color; } tessVerts[TESS_VERTICES];
static int set2D, surfaceEnds, surfaceBegins;
static void RB_Set2D(void) { backEnd.projection2D = 1; set2D++; }
static void RB_EndSurface(void) { surfaceEnds++; tess.vertexCount = tess.indexCount = 0; }
static void RB_BeginSurface(const Material *material, int techType, int lmapIndex) {
    surfaceBegins++; tess.material = material; tess.techType = techType; tess.lmapIndex = lmapIndex;
    tess.vertexCount = tess.indexCount = 0;
}
static void RB_SetVertex2D(char *t, int index, const dvar_t *renderer, float x, float y, float s, float tt, D3DCOLOR color) {
    assert(t == (char *)&tess && renderer == r_rendererInUse && index >= 0 && index < TESS_VERTICES);
    tessVerts[index].x = x; tessVerts[index].y = y; tessVerts[index].s = s; tessVerts[index].t = tt;
    tessVerts[index].color = color;
}
static int generated, released, unbound, failGenerate, textureIds, lastWidth, lastHeight, lastFormat;
static const byte *lastPixels;
static const GfxImage *lastUnbound;
static void Image_Generate2D(GfxImage *image, byte *pixels, int width, int height, int format) {
    assert(image->name && image->name[0] == '*' && !image->texture.map);
    generated++; lastPixels = pixels; lastWidth = width; lastHeight = height; lastFormat = format;
    if (!failGenerate) image->texture.map = (void *)(intptr_t)++textureIds;
}
static void Image_Release(GfxImage *image) { assert(image->texture.map); released++; image->texture.map = NULL; }
/* Owner pictures have their own test (test-owner-class-pictures.py); none load here. */
static void Image_Setup(GfxImage *image, int width, int height, int depth, int flags, DWORD usage, D3DFORMAT format) {
    (void)image; (void)width; (void)height; (void)depth; (void)flags; (void)usage; (void)format; assert(0);
}
static void Image_UploadData(GfxImage *image, D3DFORMAT format, int face, int level, const byte *src) {
    (void)image; (void)format; (void)face; (void)level; (void)src; assert(0);
}
static void RB_UnbindImage(const GfxImage *image) { unbound++; lastUnbound = image; }
static void ClearBounds(vec3_t mins, vec3_t maxs) {
    for (int i = 0; i < 3; i++) { mins[i] = 131072.0f; maxs[i] = -131072.0f; }
}
static void ExpandBounds(vec3_t addMins, vec3_t addMaxs, vec3_t mins, vec3_t maxs) {
    for (int i = 0; i < 3; i++) {
        if (addMins[i] < mins[i]) mins[i] = addMins[i];
        if (addMaxs[i] > maxs[i]) maxs[i] = addMaxs[i];
    }
}
/* Files are copied to the heap so reads outside them trip the sanitizer. */
static const char *fileNames[3]; static const byte *fileData[3]; static int fileLens[3], reads, frees;
static int FS_ReadFile(const char *path, void **buffer) {
    reads++;
    for (int i = 0; i < 3; i++) {
        if (fileNames[i] && !strcmp(path, fileNames[i])) {
            *buffer = malloc(fileLens[i] ? fileLens[i] : 1); memcpy(*buffer, fileData[i], fileLens[i]); return fileLens[i];
        }
    }
    *buffer = NULL;
    return -1;
}
static void FS_FreeFile(void *buffer) { assert(buffer); frees++; free(buffer); }
static int Com_sprintf(char *dest, int size, const char *fmt, ...) {
    va_list args; va_start(args, fmt); int n = vsnprintf(dest, size, fmt, args); va_end(args);
    return n < size ? n : -1;
}
'''

checks = r'''
static Material mapMaterial, iconMaterial;
static HudVertex2D verts[R_HUD2D_MAX_VERTICES];
static unsigned short indices[R_HUD2D_MAX_INDICES];
static void reset(void) {
    cmdUsed = 0; memset(cmdBuffer, 0, 4); memset(&tess, 0, sizeof(tess)); memset(tessVerts, 0, sizeof(tessVerts));
    backEnd.projection2D = 0; set2D = surfaceEnds = surfaceBegins = 0;
}
static void sameVertex(int slot, const HudVertex2D *vert) {
    GfxColor color;
    R_ConvertColorToBytes(vert->rgba, color.array);
    assert(tessVerts[slot].x == vert->xy[0] && tessVerts[slot].y == vert->xy[1]);
    assert(tessVerts[slot].s == vert->st[0] && tessVerts[slot].t == vert->st[1] && tessVerts[slot].color == color.packed);
}
static const byte *step(GfxRenderCommandExecState *exec) {
    const byte *cmd = exec->cmd;
    assert(((const GfxCmdHeader *)cmd)->id == 6);
    RB_DrawTriangles2DCmd(exec);
    assert((const byte *)exec->cmd == cmd + ((const GfxCmdHeader *)cmd)->byteCount);
    return cmd;
}

static void triangles(void) {
    for (int i = 0; i < R_HUD2D_MAX_VERTICES; i++)
        verts[i] = (HudVertex2D){{10.0f * i, 20.0f + i}, {0.001f * i, 1 - 0.001f * i}, {(i % 5) / 4.0f, 1, 0.5f, 1 - (i % 9) / 8.0f}};
    for (int i = 0; i < R_HUD2D_MAX_INDICES; i++) indices[i] = (unsigned short)((i * 7) % R_HUD2D_MAX_VERTICES);
    /* The last index points past the five vertices. */
    const unsigned short small[9] = {0, 1, 2, 2, 1, 3, 4, 3, 9};
    reset();
    R_AddCmdDrawTriangles2D(verts, 5, small, 9, &mapMaterial);
    R_AddCmdDrawTriangles2D(verts, 5, small, 6, &mapMaterial);
    R_AddCmdDrawTriangles2D(verts, 3, small, 3, &iconMaterial);
    int used = cmdUsed;
    R_AddCmdDrawTriangles2D(verts, 2, small, 3, &mapMaterial);
    R_AddCmdDrawTriangles2D(verts, 5, small, 4, &mapMaterial);
    R_AddCmdDrawTriangles2D(verts, 5, small, 0, &mapMaterial);
    R_AddCmdDrawTriangles2D(verts, R_HUD2D_MAX_VERTICES + 1, indices, 3, &mapMaterial);
    R_AddCmdDrawTriangles2D(verts, 5, indices, R_HUD2D_MAX_INDICES + 3, &mapMaterial);
    R_AddCmdDrawTriangles2D(verts, 5, small, 3, NULL);
    assert(cmdUsed == used);

    GfxRenderCommandExecState exec = {cmdBuffer};
    step(&exec);
    assert(set2D == 1 && backEnd.projection2D && tess.material == &mapMaterial && tess.techType == 3 && tess.lmapIndex == 0x1f);
    assert(tess.vertexCount == 5 && tess.indexCount == 9);
    for (int i = 0; i < 5; i++) sameVertex(i, &verts[i]);
    for (int k = 0; k < 9; k++) assert(tess.indices[k] == (small[k] < 5 ? small[k] : 0));
    /* Colors travel as the engine's A, R, G, B bytes. */
    GfxColor first = {tessVerts[0].color};
    assert(first.array[0] == 255 && first.array[1] == 0 && first.array[2] == 255 && first.array[3] == 127);
    /* The same material appends to the open surface. */
    step(&exec);
    assert(surfaceEnds == 0 && tess.vertexCount == 10 && tess.indexCount == 15);
    for (int i = 0; i < 5; i++) sameVertex(5 + i, &verts[i]);
    for (int k = 0; k < 6; k++) assert(tess.indices[9 + k] == 5 + small[k]);
    /* Another material flushes it. */
    step(&exec);
    assert(surfaceEnds == 1 && set2D == 1 && tess.material == &iconMaterial && tess.vertexCount == 3 && tess.indexCount == 3);
    assert(((const GfxCmdHeader *)exec.cmd)->id == 0 && (const byte *)exec.cmd == cmdBuffer + used);

    /* A nearly full surface is flushed before the new vertices go in. */
    reset();
    tess.material = &mapMaterial; tess.techType = 3; tess.vertexCount = TESS_VERTICES - 4; tess.indexCount = 30;
    R_AddCmdDrawTriangles2D(verts, 5, small, 6, &mapMaterial);
    exec.cmd = cmdBuffer;
    step(&exec);
    assert(surfaceEnds == 1 && tess.vertexCount == 5 && tess.indexCount == 6);
    for (int i = 0; i < 5; i++) sameVertex(i, &verts[i]);

    /* The largest command fits the 16-bit size and lands intact. */
    reset();
    R_AddCmdDrawTriangles2D(verts, R_HUD2D_MAX_VERTICES, indices, R_HUD2D_MAX_INDICES, &mapMaterial);
    assert(cmdUsed > 0);
    exec.cmd = cmdBuffer;
    step(&exec);
    assert(tess.vertexCount == R_HUD2D_MAX_VERTICES && tess.indexCount == R_HUD2D_MAX_INDICES);
    for (int i = 0; i < R_HUD2D_MAX_VERTICES; i += 97) sameVertex(i, &verts[i]);
    for (int k = 0; k < R_HUD2D_MAX_INDICES; k++) assert(tess.indices[k] == indices[k]);
}

static void hudImages(void) {
    static byte pixels[8 * 4 * 4];
    static MaterialTextureDef rawTexdef = {"colorMap", 0x11, 1, 0, 0, {NULL}};
    static Material raw = {{"$raw", 0}, {0x28129165, 7}, 1, 0, (void *)0x1234, &rawTexdef, NULL};
    static Material fallback = {{"$default", 1}};
    rgp.rawMaterial = NULL; rgp.defaultMaterial = &fallback;
    assert(!R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP, pixels, 8, 4));
    rgp.rawMaterial = rgp.defaultMaterial = &raw;
    assert(!R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP, pixels, 8, 4));
    rgp.defaultMaterial = &fallback;
    assert(!R_CreateHudImageMaterial(-1, pixels, 8, 4) && !R_CreateHudImageMaterial(R_HUD_IMAGE_COUNT, pixels, 8, 4));
    assert(!R_CreateHudImageMaterial(0, NULL, 8, 4) && !R_CreateHudImageMaterial(0, pixels, 0, 4));
    assert(generated == 0);

    Material *map = R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP, pixels, 8, 4);
    assert(map && map != &raw && generated == 1 && lastPixels == pixels && lastWidth == 8 && lastHeight == 4);
    assert(lastFormat == D3DFMT_A8R8G8B8);
    /* $raw's alpha-blended 2D technique and state, with the new image,
     * linear filtering and clamping. */
    assert(map->techniqueSet == raw.techniqueSet && map->stateBits[0] == raw.stateBits[0] && map->stateBits[1] == raw.stateBits[1]);
    assert(map->textureCount == 1 && map->textures != raw.textures && !strcmp(map->info.name, "hud_minimap"));
    assert(map->textures[0].samplerState == 0x32 && !strcmp(map->textures[0].name, "colorMap"));
    GfxImage *image = map->textures[0].u.image;
    assert(image && image->texture.map && !strcmp(image->name, "*hud_minimap"));
    assert(raw.textures == &rawTexdef && rawTexdef.samplerState == 0x11 && rawTexdef.u.image == NULL);

    Material *arrow = R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP_ARROW, pixels, 4, 4);
    assert(arrow && arrow != map && arrow->textures[0].u.image != image && !strcmp(arrow->info.name, "hud_minimap_arrow"));
    /* Rebuilding a slot unbinds and releases its old texture first. */
    void *oldTexture = image->texture.map;
    assert(R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP, pixels, 8, 4) == map);
    assert(unbound == 1 && lastUnbound == image && released == 1 && image->texture.map && image->texture.map != oldTexture);
    failGenerate = 1;
    assert(!R_CreateHudImageMaterial(R_HUD_IMAGE_MINIMAP_DOT, pixels, 4, 4));
    failGenerate = 0;
    /* Shutdown releases each live texture once and clears the materials. */
    released = unbound = 0;
    R_ShutdownHudImages();
    assert(released == 2 && unbound == 2 && map->textures == NULL && arrow->textures == NULL);
    R_ShutdownHudImages();
    assert(released == 2);
}

static void worldBounds(void) {
    vec3_t mins, maxs;
    static Material sky = {{"sky_toujane", 0x0c}}, wall = {{"wall", 0}}, caulk = {{"caulk", 0x04}};
    static srfTriangles_t skyTris = {0, {{-20000, -20000, -5000}, {20000, 20000, 9000}}};
    static srfTriangles_t wallTris = {0, {{-100, 50, 0}, {300, 80, 200}}};
    static srfTriangles_t caulkTris = {0, {{-400, -10, -30}, {10, 10, 10}}};
    GfxSurface surfaces[4] = {{&sky, 0, 0, &skyTris}, {&wall, 0, 0, &wallTris}, {&wall, 0, 0, NULL}, {&caulk, 0, 0, &caulkTris}};
    GfxWorld world = {4, surfaces};
    rgp.world = NULL;
    assert(!R_GetWorldDrawBounds(mins, maxs));
    rgp.world = &world;
    assert(R_GetWorldDrawBounds(mins, maxs));
    assert(mins[0] == -400 && mins[1] == -10 && mins[2] == -30 && maxs[0] == 300 && maxs[1] == 80 && maxs[2] == 200);
    world.surfaceCount = 1;
    assert(!R_GetWorldDrawBounds(mins, maxs));
}

static int iwi(byte *out, int version, int format, int w, int h, const byte *top, int topLen) {
    GfxImageFileHeader header = {{'I', 'W', 'i'}, (byte)version, (byte)format, 0, {(short)w, (short)h, 1}, {0}};
    /* A smaller mip comes first; the decoder must skip it. */
    memcpy(out, &header, sizeof(header));
    memset(out + sizeof(header), 0xff, 32);
    memcpy(out + sizeof(header) + 32, top, topLen);
    return (int)sizeof(header) + 32 + topLen;
}
static void dxt5(byte *block, int a0, int a1, const int *index) {
    unsigned long long bits = 0;
    for (int i = 0; i < 16; i++) bits |= (unsigned long long)index[i] << (3 * i);
    block[0] = (byte)a0; block[1] = (byte)a1;
    for (int i = 0; i < 6; i++) block[2 + i] = (byte)(bits >> (8 * i));
    memset(block + 8, 0x5a, 8);
}
static void dxt3(byte *block, int first) {
    memset(block, 0, 16);
    for (int i = 0; i < 16; i++) block[i >> 1] |= (byte)(((first + i) & 15) << ((i & 1) * 4));
    memset(block + 8, 0xa5, 8);
}
static int alphaAt(const byte *bgra, int w, int x, int y) {
    const byte *t = bgra + (y * w + x) * 4;
    assert(t[0] == 255 && t[1] == 255 && t[2] == 255);
    return t[3];
}

static void silhouettes(void) {
    static byte top5[48], top3[64], wideTop[33 * 16], file5[256], file3[256], bad[1024], out[128 * 128 * 4 + 16];
    /* Interpolated alphas in both DXT5 modes, including a0 == a1. */
    const int i0[16] = {0, 1, 2, 7}, i1[16] = {6, 7, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5}, i2[16] = {6, 7, 3};
    dxt5(top5, 255, 0, i0); dxt5(top5 + 16, 0, 200, i1); dxt5(top5 + 32, 100, 100, i2);
    for (int b = 0; b < 4; b++) dxt3(top3 + 16 * b, b);
    int len5 = iwi(file5, 5, 0x0d, 12, 4, top5, 48), len3 = iwi(file3, 5, 0x0c, 6, 6, top3, 64), w = -1, h = -1;

    assert(R_DecodeIconAlpha(file5, len5, out, 128, &w, &h) && w == 12 && h == 4);
    const int row0[12] = {255, 0, 218, 36, 0, 255, 40, 200, 0, 255, 100, 100};
    for (int x = 0; x < 12; x++) assert(alphaAt(out, 12, x, 0) == row0[x]);
    assert(alphaAt(out, 12, 0, 1) == 255 && alphaAt(out, 12, 7, 3) == 160 && alphaAt(out, 12, 6, 3) == 200);
    /* Edge blocks only write the texels inside the image. */
    memset(out, 0xee, sizeof(out));
    assert(R_DecodeIconAlpha(file3, len3, out, 128, &w, &h) && w == 6 && h == 6);
    for (int y = 0; y < 6; y++)
        for (int x = 0; x < 6; x++)
            assert(alphaAt(out, 6, x, y) == (((x / 4 + (y / 4) * 2) + (y % 4) * 4 + x % 4) & 15) * 17);
    assert(out[6 * 6 * 4] == 0xee);

    w = h = -1;
    memcpy(bad, file5, len5); bad[2] = 'j';
    assert(!R_DecodeIconAlpha(bad, len5, out, 128, &w, &h));
    assert(!R_DecodeIconAlpha(bad, iwi(bad, 6, 0x0d, 12, 4, top5, 48), out, 128, &w, &h));
    assert(!R_DecodeIconAlpha(bad, iwi(bad, 5, 0x0b, 12, 4, top5, 48), out, 128, &w, &h));
    assert(!R_DecodeIconAlpha(bad, iwi(bad, 5, 0x0d, 132, 4, wideTop, sizeof(wideTop)), out, 128, &w, &h));
    assert(!R_DecodeIconAlpha(bad, iwi(bad, 5, 0x0d, 4, 132, wideTop, sizeof(wideTop)), out, 128, &w, &h));
    assert(!R_DecodeIconAlpha(bad, iwi(bad, 5, 0x0d, 12, 0, top5, 48), out, 128, &w, &h));
    assert(!R_DecodeIconAlpha(file5, 28 + 40, out, 128, &w, &h) && !R_DecodeIconAlpha(file5, 20, out, 128, &w, &h));
    assert(!R_DecodeIconAlpha(NULL, 100, out, 128, &w, &h) && w == -1 && h == -1);

    /* Cropping keeps one texel around everything above alpha 16 and packs the
     * rows; the image edge bounds the margin. */
    memset(out, 0, 8 * 8 * 4);
    for (int x = 2; x <= 5; x++) out[(3 * 8 + x) * 4 + 3] = 255;
    out[(4 * 8 + 4) * 4 + 3] = 17;
    out[(6 * 8 + 6) * 4 + 3] = 16;
    assert(R_CropToAlpha(out, 8, 8, &w, &h) && w == 6 && h == 4);
    const int cropped[4][6] = {{0}, {0, 255, 255, 255, 255, 0}, {0, 0, 0, 17, 0, 0}, {0}};
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 6; x++) assert(out[(y * 6 + x) * 4 + 3] == cropped[y][x]);
    memset(out, 0, 8 * 8 * 4);
    out[3] = out[(7 * 8 + 7) * 4 + 3] = 255;
    assert(R_CropToAlpha(out, 8, 8, &w, &h) && w == 8 && h == 8 && out[3] == 255 && out[(7 * 8 + 7) * 4 + 3] == 255);
    memset(out, 0, 8 * 8 * 4);
    out[(3 * 8 + 3) * 4 + 3] = 16;
    w = h = -1;
    assert(!R_CropToAlpha(out, 8, 8, &w, &h) && w == -1 && h == -1);

    /* An 8x8 DXT5 icon with two visible texels at (5, 1) and (6, 1). */
    static byte topCrop[64], fileCrop[256];
    const int none[16] = {0};
    int two[16];
    for (int i = 0; i < 16; i++) two[i] = i == 5 || i == 6 ? 0 : 1;
    dxt5(topCrop, 0, 0, none); dxt5(topCrop + 16, 255, 0, two); dxt5(topCrop + 32, 0, 0, none); dxt5(topCrop + 48, 0, 0, none);
    int lenCrop = iwi(fileCrop, 5, 0x0d, 8, 8, topCrop, 64);

    static MaterialTextureDef rawTexdef = {"colorMap", 0x11, 1, 0, 0, {NULL}};
    static Material raw = {{"$raw", 0}, {0x28129165, 7}, 1, 0, (void *)0x1234, &rawTexdef, NULL};
    static Material fallback = {{"$default", 1}};
    static GfxImage testImage = {0, {NULL}, 128, 64, "hud_test"}, missingImage = {0, {NULL}, 64, 64, "hud_missing"};
    static GfxImage failImage = {0, {NULL}, 64, 64, "hud_fail"}, extraImage = {0, {NULL}, 64, 64, "hud_extra"};
    static GfxImage cropImage = {0, {NULL}, 8, 8, "hud_crop"};
    static MaterialTextureDef cropTex = {"colorMap", 0x62, 1, 0, 0, {&cropImage}};
    static Material crop = {{"crop", 0}, {0, 0}, 1, 0, NULL, &cropTex, NULL};
    static char longText[65];
    static GfxImage longImage = {0, {NULL}, 64, 64, longText};
    memset(longText, 'x', 64);
    static MaterialTextureDef testTex = {"colorMap", 0x62, 1, 0, 0, {&testImage}}, missingTex = {"colorMap", 0x62, 1, 0, 0, {&missingImage}};
    static MaterialTextureDef failTex = {"colorMap", 0x62, 1, 0, 0, {&failImage}}, extraTex = {"colorMap", 0x62, 1, 0, 0, {&extraImage}};
    static MaterialTextureDef longTex = {"colorMap", 0x62, 1, 0, 0, {&longImage}};
    static Material icon = {{"gfx/icons/hud@test.tga", 0}, {0, 0}, 1, 0, (void *)0x99, &testTex, NULL};
    static Material alias = {{"gfx/icons/hud@alias.tga", 0}, {0, 0}, 1, 0, (void *)0x98, &testTex, NULL};
    static Material missing = {{"missing", 0}, {0, 0}, 1, 0, NULL, &missingTex, NULL};
    static Material failing = {{"failing", 0}, {0, 0}, 1, 0, NULL, &failTex, NULL};
    static Material extra = {{"extra", 0}, {0, 0}, 1, 0, NULL, &extraTex, NULL};
    static Material longName = {{"long", 0}, {0, 0}, 1, 0, NULL, &longTex, NULL};
    static Material untextured = {{"untextured", 0}};
    assert(strlen(longImage.name) == 64);
    rgp.rawMaterial = &raw; rgp.defaultMaterial = &fallback;
    fileNames[0] = "images/hud_test.iwi"; fileData[0] = file5; fileLens[0] = len5;
    fileNames[1] = "images/hud_fail.iwi"; fileData[1] = file3; fileLens[1] = len3;
    fileNames[2] = "images/hud_crop.iwi"; fileData[2] = fileCrop; fileLens[2] = lenCrop;
    generated = released = unbound = reads = frees = 0;
    float aspect = -1.0f;

    Material *flat = R_GetHudSilhouetteMaterial(&icon, &aspect);
    assert(flat && flat != &icon && reads == 1 && frees == 1 && generated == 1 && aspect == 3.0f);
    assert(lastWidth == 12 && lastHeight == 4 && lastFormat == D3DFMT_A8R8G8B8);
    for (int x = 0; x < 12; x++) assert(alphaAt(lastPixels, 12, x, 0) == row0[x]);
    /* $raw's 2D technique with the white image, clamped and unfiltered by mips. */
    assert(flat->techniqueSet == raw.techniqueSet && flat->stateBits[0] == raw.stateBits[0] && flat->stateBits[1] == raw.stateBits[1]);
    assert(flat->textureCount == 1 && flat->textures[0].samplerState == 0x32 && !strcmp(flat->textures[0].name, "colorMap"));
    assert(!strcmp(flat->textures[0].u.image->name, "*hud_test") && !strcmp(flat->info.name, "hud_test"));
    assert(rawTexdef.u.image == NULL && rawTexdef.samplerState == 0x11 && testTex.u.image == &testImage);
    /* The image is trimmed to what is visible, and its proportions reported. */
    Material *trimmed = R_GetHudSilhouetteMaterial(&crop, &aspect);
    assert(trimmed && trimmed != flat && lastWidth == 4 && lastHeight == 3 && aspect == 4.0f / 3.0f);
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 4; x++) assert(alphaAt(lastPixels, 4, x, y) == (y == 1 && (x == 1 || x == 2) ? 255 : 0));
    /* One silhouette per image, whichever material names it. */
    aspect = -1.0f;
    assert(R_GetHudSilhouetteMaterial(&icon, &aspect) == flat && aspect == 3.0f);
    assert(R_GetHudSilhouetteMaterial(&alias, &aspect) == flat && aspect == 3.0f && reads == 2);
    /* Failures are remembered, so the file is not read on every kill. */
    assert(!R_GetHudSilhouetteMaterial(&missing, &aspect) && aspect == 0.0f && reads == 3 && frees == 2);
    aspect = -1.0f;
    assert(!R_GetHudSilhouetteMaterial(&missing, &aspect) && aspect == 0.0f && reads == 3);
    failGenerate = 1;
    assert(!R_GetHudSilhouetteMaterial(&failing, &aspect) && aspect == 0.0f && reads == 4 && frees == 3 && generated == 3);
    failGenerate = 0;
    aspect = -1.0f;
    assert(!R_GetHudSilhouetteMaterial(&failing, &aspect) && aspect == 0.0f && reads == 4 && generated == 3);
    assert(!R_GetHudSilhouetteMaterial(NULL, &aspect) && !R_GetHudSilhouetteMaterial(&fallback, &aspect));
    assert(!R_GetHudSilhouetteMaterial(&untextured, &aspect) && aspect == 0.0f);
    assert(!R_GetHudSilhouetteMaterial(&longName, &aspect) && reads == 4);
    rgp.rawMaterial = NULL;
    assert(!R_GetHudSilhouetteMaterial(&extra, &aspect) && reads == 4);
    rgp.rawMaterial = &raw;

    /* The cache holds R_HUD_SILHOUETTE_COUNT images, then refuses new ones. */
    static GfxImage many[R_HUD_SILHOUETTE_COUNT]; static char manyNames[R_HUD_SILHOUETTE_COUNT][16];
    static MaterialTextureDef manyTex[R_HUD_SILHOUETTE_COUNT]; static Material manyMat[R_HUD_SILHOUETTE_COUNT];
    for (int i = 0; i < R_HUD_SILHOUETTE_COUNT - 4; i++) {
        snprintf(manyNames[i], 16, "hud_n%02d", i);
        many[i].name = manyNames[i];
        manyTex[i] = (MaterialTextureDef){"colorMap", 0x62, 1, 0, 0, {&many[i]}};
        manyMat[i] = (Material){{"many", 0}, {0, 0}, 1, 0, NULL, &manyTex[i], NULL};
        assert(!R_GetHudSilhouetteMaterial(&manyMat[i], &aspect) && reads == 5 + i);
    }
    int full = reads;
    assert(!R_GetHudSilhouetteMaterial(&extra, &aspect) && reads == full && R_GetHudSilhouetteMaterial(&icon, &aspect) == flat);

    /* Shutdown releases the live silhouettes and empties the cache. */
    R_ShutdownHudImages();
    assert(released == 2 && unbound == 2);
    Material *again = R_GetHudSilhouetteMaterial(&icon, &aspect);
    assert(again && reads == full + 1 && generated == 4);
    assert(!R_GetHudSilhouetteMaterial(&extra, &aspect) && reads == full + 2);
    R_ShutdownHudImages();
    assert(released == 3);

    /* Emblems: an 8x4 DXT5 icon, opaque white to black in the first block and
     * half-transparent red and blue in the second. */
    static byte topLuma[32], fileLuma[256];
    const int flatAlpha[16] = {0};
    dxt5(topLuma, 255, 255, flatAlpha); dxt5(topLuma + 16, 128, 128, flatAlpha);
    const byte grey[8] = {0xff, 0xff, 0x00, 0x00, 0xe4, 0, 0, 0}, redBlue[8] = {0x00, 0xf8, 0x1f, 0x00, 0x04, 0, 0, 0};
    memcpy(topLuma + 8, grey, 8); memcpy(topLuma + 24, redBlue, 8);
    int lenLuma = iwi(fileLuma, 5, 0x0d, 8, 4, topLuma, 32);
    /* Black keeps 96 of 255, white all of it; the two DXT midpoints and the
     * Rec. 709 luma of pure red and blue scale in between. */
    assert(R_DecodeIcon(fileLuma, lenLuma, out, 128, &w, &h, 1) && w == 8 && h == 4);
    const int lumaRow0[8] = {255, 96, 202, 149, 64, 53, 64, 64};
    for (int x = 0; x < 8; x++) assert(alphaAt(out, 8, x, 0) == lumaRow0[x]);
    assert(alphaAt(out, 8, 3, 3) == 255 && alphaAt(out, 8, 7, 3) == 64);
    assert(R_DecodeIconAlpha(fileLuma, lenLuma, out, 128, &w, &h) && alphaAt(out, 8, 1, 0) == 255 && alphaAt(out, 8, 5, 0) == 128);

    fileNames[0] = "images/hud_test.iwi"; fileData[0] = fileLuma; fileLens[0] = lenLuma;
    reads = frees = 0;
    int built = generated;
    Material *emblem = R_GetHudEmblemMaterial(&icon);
    assert(emblem && reads == 1 && frees == 1 && generated == built + 1 && lastWidth == 8 && lastHeight == 4);
    for (int x = 0; x < 8; x++) assert(alphaAt(lastPixels, 8, x, 0) == lumaRow0[x]);
    assert(!strcmp(emblem->textures[0].u.image->name, "*hud_test_emblem") && !strcmp(emblem->info.name, "hud_test_emblem"));
    assert(emblem->techniqueSet == raw.techniqueSet && emblem->textures[0].samplerState == 0x32);
    /* The whole frame is kept, so an emblem stays centred where it is drawn. */
    Material *uncropped = R_GetHudEmblemMaterial(&crop);
    assert(uncropped && uncropped != emblem && lastWidth == 8 && lastHeight == 8 && reads == 2);
    /* One emblem per image, apart from the image's silhouette. */
    assert(R_GetHudEmblemMaterial(&icon) == emblem && R_GetHudEmblemMaterial(&alias) == emblem && reads == 2);
    Material *outline = R_GetHudSilhouetteMaterial(&icon, &aspect);
    assert(outline && outline != emblem && reads == 3 && !strcmp(outline->textures[0].u.image->name, "*hud_test"));
    assert(alphaAt(lastPixels, 8, 1, 0) == 255 && R_GetHudSilhouetteMaterial(&icon, &aspect) == outline);
    assert(R_GetHudEmblemMaterial(&icon) == emblem && reads == 3);
    assert(!R_GetHudEmblemMaterial(&missing) && !R_GetHudEmblemMaterial(&missing) && reads == 4);
    assert(!R_GetHudEmblemMaterial(NULL) && !R_GetHudEmblemMaterial(&fallback));
    R_ShutdownHudImages();
    assert(released == 6);
}

int main(void) {
    triangles(); hudImages(); silhouettes(); worldBounds();
    return 0;
}
'''


def mutate(old, new):
    assert body.count(old) == 1, old
    return body.replace(old, new)


mutants = [
    mutate('tess.indices[ic + i] = (r_index_t)(vc + indices[i]);', 'tess.indices[ic + i] = (r_index_t)indices[i];'),
    mutate('execState->cmd = (const void *)((const byte *)cmd + cmd->header.byteCount);',
           'execState->cmd = (const void *)((const byte *)cmd + sizeof(*cmd));'),
    mutate('cmdIndices[i] = indices[i] < vertexCount ? indices[i] : 0;', 'cmdIndices[i] = indices[i];'),
    mutate('    byteCount = (byteCount + 3) & ~3;\n', ''),
    mutate('vc = RB_CheckTessOverflow(t, cmd->vertexCount, cmd->indexCount);', 'vc = RB_CheckTessOverflow(t, 4, 6);'),
    mutate('R_ConvertColorToBytes(verts[i].rgba, cmdVerts[i].color.array);', 'R_ConvertColorToBytes(verts[0].rgba, cmdVerts[i].color.array);'),
    mutate('    texture->samplerState = HUD_IMAGE_SAMPLER_STATE;\n', ''),
    mutate('    R_ReleaseHudImage(slot);\n    image = &s_hudImages[slot];', '    image = &s_hudImages[slot];'),
    mutate('if (!raw || raw == rgp.defaultMaterial ||', 'if (!raw ||'),
    mutate(' || (surf->material->info.gameFlags & 8)', ''),
    mutate('    block = file + fileLen - blocksW * blocksH * DXT_BLOCK_BYTES;\n', '    block = file + sizeof(*header);\n'),
    mutate('    if (a0 > a1) {', '    if (a0 >= a1) {'),
    mutate('((i & 1) * 4)) & 15) * 17);', '((~i & 1) * 4)) & 15) * 17);'),
    mutate('                if (x >= w || y >= h)\n                    continue;\n', ''),
    mutate('texel[0] = texel[1] = texel[2] = 255;', 'texel[0] = texel[1] = texel[2] = alpha[i];'),
    mutate('w > maxSize || h > maxSize', 'h > maxSize'),
    mutate(' || header->version != IWI_VERSION', ''),
    mutate('    if (fileLen - (int)sizeof(*header) < blocksW * blocksH * DXT_BLOCK_BYTES)\n        return 0;\n', ''),
    mutate('        if (s_silhouettes[i].emblem == emblem && !strcmp(s_silhouettes[i].imageName, image->name))',
           '        if (s_silhouettes[i].emblem == emblem && !strcmp(s_silhouettes[i].imageName, image->name) && '
           's_silhouettes[i].material)'),
    mutate('s_silhouettes[i].emblem == emblem && ', ''),
    mutate('    entry->emblem = emblem;\n', ''),
    mutate('    if (emblem)\n        strcat(entry->name, "_emblem");\n', ''),
    mutate('(!entry->emblem && !R_CropToAlpha(', '(!R_CropToAlpha('),
    mutate('    palette[2] = (2 * palette[0] + palette[1]) / 3;', '    palette[2] = (palette[0] + 2 * palette[1]) / 3;'),
    mutate('palette[i] = (r * 54 + g * 183 + b * 19) >> 8;', 'palette[i] = (r * 19 + g * 183 + b * 54) >> 8;'),
    mutate('luma[i] = (byte)palette[(bits >> (2 * i)) & 3];', 'luma[i] = (byte)palette[(bits >> i) & 3];'),
    mutate('(HUD_EMBLEM_LUMA_FLOOR + luma[i] * (255 - HUD_EMBLEM_LUMA_FLOOR) / 255)', '(luma[i])'),
    mutate('    if (s_silhouetteCount == R_HUD_SILHOUETTE_COUNT)\n        return NULL;\n', ''),
    mutate('        R_ReleaseImage(&s_silhouettes[slot].image);\n', ''),
    mutate('    s_silhouetteCount = 0;\n', ''),
    mutate('<= HUD_SILHOUETTE_ALPHA_MIN)', '< HUD_SILHOUETTE_ALPHA_MIN)'),
    mutate('bounds[0] = minX > 0 ? minX - 1 : 0;', 'bounds[0] = minX;'),
    mutate('bounds[3] = maxY < height - 1 ? maxY + 1 : maxY;', 'bounds[3] = maxY;'),
    mutate('bounds[2] = maxX < width - 1 ? maxX + 1 : maxX;', 'bounds[2] = maxX + 1;'),
    mutate('memmove(bgra + y * *cropWidth * 4,', 'memmove(bgra + y * width * 4,'),
    mutate('    if (!decoded || (!entry->emblem && !R_CropToAlpha(s_silhouettePixels, width, height, &width, &height)))',
           '    if (!decoded)'),
    mutate('    entry->aspect = (float)width / (float)height;\n', ''),
    mutate('*aspect = entry->material ? entry->aspect : 0.0f;', '*aspect = entry->aspect;'),
]
with tempfile.TemporaryDirectory(prefix='cod2-minimap-renderer-') as directory:
    path = Path(directory) / 'renderer.c'
    binary = Path(directory) / 'renderer'
    for index, variant in enumerate([body] + mutants):
        path.write_text(support + variant + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wno-unused-function', str(path), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: render command 6 carries per-vertex positions, texture coordinates and A,R,G,B colors from the builder '
      'to the tessellator, offsets and clamps indices, appends to an open surface, flushes on a material change or '
      'a full surface and rejects bad counts; HUD images copy $raw with their own clamped texture, rebuild and shut '
      'down cleanly; icon silhouettes decode the top DXT3/DXT5 mip as white texels with its alpha, are trimmed to '
      'their visible texels plus one, report their proportions, are built once per image, remember failures, cap '
      'the cache and empty it on shutdown; emblems keep the whole icon with an opacity that follows its Rec. 709 '
      'brightness from a floor, cached apart from silhouettes; world bounds skip the sky; '
      f'{len(mutants)} failing regression mutants')
