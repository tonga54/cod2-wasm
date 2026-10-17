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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte; typedef float vec_t; typedef vec_t vec3_t[3];
typedef unsigned int D3DCOLOR; typedef unsigned short r_index_t;
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

int main(void) {
    triangles(); hudImages(); worldBounds();
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
    mutate('    s_hudTextures[slot].samplerState = HUD_IMAGE_SAMPLER_STATE;\n', ''),
    mutate('    R_ReleaseHudImage(slot);\n    image = &s_hudImages[slot];', '    image = &s_hudImages[slot];'),
    mutate('if (!raw || raw == rgp.defaultMaterial ||', 'if (!raw ||'),
    mutate(' || (surf->material->info.gameFlags & 8)', ''),
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
      f'down cleanly; world bounds skip the sky; {len(mutants)} failing regression mutants')
