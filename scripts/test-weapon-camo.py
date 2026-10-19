#!/usr/bin/env python3
"""Exercise weapon camouflage: its catalog and patterns, the renderer's paint
masks, painted entities and menu pictures, the client's reading of each
player's camouflage and the server's builtin that publishes it."""
from pathlib import Path
import struct
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
camo = root / 'src/PC/bgame/bg_weapon_camo.c'
renderer = root / 'src/PC/gfx_d3d/r_weapon_camo.c'
cgame = root / 'src/PC/cgame_mp/cg_weaponcamo_mp.c'
server = root / 'src/PC/game_mp/g_weaponcamo_mp.c'

# The few engine types the four files use.
common_types = r'''
#include <stddef.h>
typedef unsigned char byte;
typedef long unsigned int DWORD;
typedef int D3DFORMAT;
typedef int D3DCUBEMAP_FACES;
enum { D3DFMT_A8R8G8B8 = 21 };
typedef byte GfxSamplerState;
typedef byte TextureSemantic;
typedef struct GfxImage {
    int mapType;
    union { void *map; } texture;
    unsigned short width, height, depth;
    const char *name;
} GfxImage;
typedef struct {
    const char *name;
    GfxSamplerState samplerState;
    TextureSemantic semantic;
    byte unused_0, unused_1;
    union { GfxImage *image; } u;
} MaterialTextureDef;
typedef struct Material {
    struct { const char *name; } info;
    unsigned short textureCount;
    MaterialTextureDef *textures;
} Material;
typedef struct Material *MaterialHandle;
typedef struct {
    char tag[3];
    byte version, format, flags;
    short dimensions[3];
    int fileSizeForPicmip[4];
} GfxImageFileHeader;
typedef struct GfxEntity { int reType; float rest[30]; } GfxEntity;
typedef struct { int frameCount; GfxEntity entities[8184]; } GfxBackEndData;
typedef struct { Material *defaultMaterial; Material *rawMaterial; } r_global_permanent_t;
typedef struct XModel XModel;
struct DObj_s;
struct XSurface_s;
struct trXSkin_t;
typedef struct { const char *szInternalName; } WeaponDef;
typedef struct { unsigned short entnum, classnum; } scr_entref_t;
typedef struct { int client; } gclient_t;
typedef struct { int number; gclient_t *client; } gentity_t;
'''
imports = 'extern void *imp_g_entities;\n'

catalog_checks = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "@CAMO@"
static unsigned char a[128 * 128 * 4], b[128 * 128 * 4];
static int channel(const unsigned char *t, int size, int x, int y, int c) { return t[((y & (size - 1)) * size + (x & (size - 1))) * 4 + c]; }
/* The mean difference across a column or row boundary, over every channel. */
static double across(const unsigned char *t, int size, int x0, int y0, int horizontal)
{
    double sum = 0;
    for (int i = 0; i < size; i++)
        for (int c = 0; c < 3; c++)
            sum += horizontal ? abs(channel(t, size, x0, i, c) - channel(t, size, x0 + 1, i, c))
                              : abs(channel(t, size, i, y0, c) - channel(t, size, i, y0 + 1, c));
    return sum / (size * 3);
}
int main(void)
{
    assert(WEAPON_CAMO_COUNT == 11 && !strcmp(g_weaponCamos[WEAPON_CAMO_NONE].id, "none"));
    for (int i = 0; i < WEAPON_CAMO_COUNT; i++) {
        const char *id = g_weaponCamos[i].id;
        assert(strlen(id) && strlen(id) <= WEAPON_CAMO_ID_MAX && g_weaponCamos[i].name[0] && g_weaponCamos[i].note[0]);
        for (int c = 0; id[c]; c++)
            assert(id[c] >= 'a' && id[c] <= 'z');
        for (int j = 0; j < i; j++)
            assert(strcmp(g_weaponCamos[j].id, id) && strcmp(g_weaponCamos[j].name, g_weaponCamos[i].name));
        assert(BG_WeaponCamo_Find(id) == i);
    }
    assert(BG_WeaponCamo_Find("Frogskin") < 0 && BG_WeaponCamo_Find("") < 0 && BG_WeaponCamo_Find(NULL) < 0);
    assert(g_weaponCamoGunCount == 25);
    {
        const WeaponCamoGun *kar = BG_WeaponCamo_Gun("kar98k_mp"), *luger = BG_WeaponCamo_Gun("luger_mp");
        assert(kar && !strcmp(kar->material, "mtl_weapon_kar98") && kar->style == WEAPON_CAMO_WOOD);
        assert(luger && luger->style == WEAPON_CAMO_BODY && BG_WeaponCamo_Gun("TT30_mp"));
        assert(!BG_WeaponCamo_Gun("frag_grenade_german_mp") && !BG_WeaponCamo_Gun("KAR98K_mp") && !BG_WeaponCamo_Gun(NULL));
    }

    /* Patterns: opaque, the same every time, different for every camouflage,
     * and no seam where a tile meets the next. */
    assert(!BG_WeaponCamo_Generate(WEAPON_CAMO_NONE, 64, a) && !BG_WeaponCamo_Generate(WEAPON_CAMO_COUNT, 64, a));
    assert(!BG_WeaponCamo_Generate(1, 7, a) && !BG_WeaponCamo_Generate(1, 96, a) && !BG_WeaponCamo_Generate(1, 2048, a));
    assert(!BG_WeaponCamo_Generate(1, 64, NULL) && !BG_WeaponCamo_Generate(-1, 64, a));
    for (int i = 1; i < WEAPON_CAMO_COUNT; i++) {
        double seam, interior = 0;
        assert(BG_WeaponCamo_Generate(i, 128, a) && BG_WeaponCamo_Generate(i, 128, b) && !memcmp(a, b, sizeof(a)));
        for (int t = 0; t < 128 * 128; t++)
            assert(a[t * 4 + 3] == 255);
        for (int x = 8; x < 120; x += 8)
            interior += across(a, 128, x, 0, 1) + across(a, 128, 0, x, 0);
        interior /= 28.0;
        seam = (across(a, 128, 127, 0, 1) + across(a, 128, 0, 127, 0)) / 2.0;
        assert(seam < 2.0 * interior + 3.0);
        for (int j = 1; j < i; j++) {
            BG_WeaponCamo_Generate(j, 128, b);
            assert(memcmp(a, b, sizeof(a)));
        }
    }
    /* Each in its palette, in B, G, R order: olive and khaki frogskin, the
     * plane tree's greens, salmon brownspot, red ember, white winter. */
    {
        static const struct { const char *id; double r, g, b; } palettes[] = {
            { "frogskin", 1.4, 1.4, 1.0 }, { "planetree", 1.0, 1.15, 1.0 }, { "brownspot", 1.3, 1.0, 0.0 },
            { "ember", 2.0, 1.0, 0.0 },
        };
        double r, g, bl;
        for (int p = 0; p < 4; p++) {
            r = g = bl = 0;
            BG_WeaponCamo_Generate(BG_WeaponCamo_Find(palettes[p].id), 128, a);
            for (int t = 0; t < 128 * 128; t++) { bl += a[t * 4]; g += a[t * 4 + 1]; r += a[t * 4 + 2]; }
            if (p == 0) assert(r > palettes[p].r * bl && g > palettes[p].g * bl);
            if (p == 1) assert(g > palettes[p].g * r && g > palettes[p].g * bl);
            if (p >= 2) assert(r > palettes[p].r * g && g > bl);
        }
        BG_WeaponCamo_Generate(BG_WeaponCamo_Find("winter"), 128, a);
        r = 0;
        for (int t = 0; t < 128 * 128; t++) r += a[t * 4] + a[t * 4 + 1] + a[t * 4 + 2];
        assert(r / (128.0 * 128.0 * 3.0) > 150.0);
    }
    puts("PASS: 11 camouflage ids, 25 class guns with their painted material, opaque deterministic seamless "
         "patterns in their palettes");
    return 0;
}
'''

renderer_checks = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "@SOURCE@"
r_global_permanent_t rgp;
static GfxBackEndData s_data;
GfxBackEndData *frontEndDataOut = &s_data;
typedef struct { int width, height, levels; byte *level0; } Fake;
static int setups, uploads, generated, releases, unbinds, reads;
void Image_Setup(GfxImage *image, int width, int height, int depth, int imageFlags, DWORD usage, D3DFORMAT format)
{
    Fake *fake = calloc(1, sizeof(Fake));
    assert(depth == 1 && imageFlags == 1 && !usage && format == D3DFMT_A8R8G8B8 && image->name && image->name[0] == '*');
    fake->width = width; fake->height = height;
    image->width = (unsigned short)width; image->height = (unsigned short)height;
    image->texture.map = fake;
    setups++;
}
D3DCUBEMAP_FACES Image_CubemapFace(int face) { return face; }
void Image_UploadData(GfxImage *image, D3DFORMAT format, int face, int level, const byte *src)
{
    Fake *fake = image->texture.map;
    int w = fake->width >> level, h = fake->height >> level;
    assert(format == D3DFMT_A8R8G8B8 && !face && level == fake->levels);
    w = w ? w : 1; h = h ? h : 1;
    if (!level) {
        fake->level0 = malloc((size_t)w * h * 4);
        memcpy(fake->level0, src, (size_t)w * h * 4);
    }
    fake->levels++;
    uploads++;
}
void Image_Generate2D(GfxImage *image, byte *pixels, int width, int height, int format)
{
    Fake *fake = calloc(1, sizeof(Fake));
    assert(format == D3DFMT_A8R8G8B8 && image->name && image->name[0] == '*');
    fake->width = width; fake->height = height; fake->levels = 1;
    fake->level0 = malloc((size_t)width * height * 4);
    memcpy(fake->level0, pixels, (size_t)width * height * 4);
    image->width = (unsigned short)width; image->height = (unsigned short)height;
    image->texture.map = fake;
    generated++;
}
void Image_Release(GfxImage *image)
{
    Fake *fake = image->texture.map;
    free(fake->level0);
    free(fake);
    image->texture.map = NULL;
    releases++;
}
void RB_UnbindImage(const GfxImage *image) { (void)image; unbinds++; }

/* Files by path, built by the test. */
static struct { const char *path; byte *data; int length; } files[16];
static int fileCount;
static void addFile(const char *path, byte *data, int length) { files[fileCount].path = path; files[fileCount].data = data; files[fileCount++].length = length; }
int FS_ReadFile(const char *path, void **buffer)
{
    reads++;
    for (int i = 0; i < fileCount; i++) {
        if (!strcmp(files[i].path, path)) {
            *buffer = malloc((size_t)files[i].length);
            memcpy(*buffer, files[i].data, (size_t)files[i].length);
            return files[i].length;
        }
    }
    *buffer = NULL;
    return -1;
}
void FS_FreeFile(void *buffer) { free(buffer); }

/* A gun of one model, with its material at every level of detail. */
struct XModel { Material **lods[2]; int count; };
static XModel gunModel, handsModel;
static Material *handsSkin[1], *gunSkin[2], *gunLod1[2];
int DObjGetNumModels(const struct DObj_s *obj) { (void)obj; return 2; }
XModel *DObjGetModel(const struct DObj_s *obj, int index) { (void)obj; return index ? &gunModel : &handsModel; }
const struct trXSkin_t *XModelGetSkins(const XModel *model) { return (const struct trXSkin_t *)model->lods; }
int XModelGetNumLods(const XModel *model) { return model == &gunModel ? 2 : 1; }
int XModelGetSurfaces(const XModel *model, struct XSurface_s ***surfaces, int lod, int **partBits)
{ (void)surfaces; (void)lod; (void)partBits; return model->count; }

static byte *iwi(int format, int width, int height, int *length)
{
    int bytes = format == 0x01 ? width * height * 4 : ((width + 3) / 4) * ((height + 3) / 4) * (format == 0x0b ? 8 : 16);
    byte *file = calloc(1, 28 + (size_t)bytes);
    memcpy(file, "IWi", 3);
    file[3] = 5; file[4] = (byte)format;
    ((short *)(file + 6))[0] = (short)width; ((short *)(file + 6))[1] = (short)height; ((short *)(file + 6))[2] = 1;
    *length = 28 + bytes;
    return file;
}
static unsigned short rgb565(int r, int g, int b) { return (unsigned short)(((r * 31 + 127) / 255) << 11 | ((g * 63 + 127) / 255) << 5 | ((b * 31 + 127) / 255)); }
/* A channel as a DXT block decodes it. */
static int decoded(int value, int bits) { int top = (1 << bits) - 1; return ((value * top + 127) / 255) * 255 / top; }
/* One solid color per block, from a function of the block's position. */
typedef void (*BlockColor)(int bx, int by, int *rgb, int *alpha);
static void fillDxt(byte *file, int format, int width, int height, BlockColor color)
{
    byte *block = file + 28;
    for (int by = 0; by < height / 4; by++) {
        for (int bx = 0; bx < width / 4; bx++) {
            int rgb[3], alpha = 255;
            unsigned short c;
            color(bx, by, rgb, &alpha);
            c = rgb565(rgb[0], rgb[1], rgb[2]);
            if (format != 0x0b) {
                if (format == 0x0c) memset(block, alpha == 255 ? 0xff : alpha ? 0x77 : 0, 8);
                else { block[0] = block[1] = (byte)alpha; memset(block + 2, 0, 6); }
                block += 8;
            }
            block[0] = block[2] = (byte)c; block[1] = block[3] = (byte)(c >> 8);
            memset(block + 4, 0, 4);
            block += 8;
        }
    }
}
static const int wood[3] = { 140, 80, 45 }, metal[3] = { 70, 72, 75 };
/* 512 square: wood on the left, steel on the right with a speck of warm
 * grime, black along the bottom. */
static void gunColor(int bx, int by, int *rgb, int *alpha)
{
    const int *c = bx < 64 ? wood : metal;
    (void)alpha;
    if (bx >= 100 && bx < 104 && by >= 20 && by < 24) c = wood;
    memcpy(rgb, c, sizeof(int) * 3);
    if (by >= 120) rgb[0] = rgb[1] = rgb[2] = 0;
}
/* 64 by 32: transparent above, wood on the left and steel on the right. */
static void pictureColor(int bx, int by, int *rgb, int *alpha)
{
    memcpy(rgb, bx < 8 ? wood : metal, sizeof(int) * 3);
    *alpha = by < 2 ? 0 : 255;
}
/* 64 square: an icon's opaque 32 by 16 shape in white. */
static void iconColor(int bx, int by, int *rgb, int *alpha)
{
    rgb[0] = rgb[1] = rgb[2] = 255;
    *alpha = bx >= 4 && bx < 12 && by >= 6 && by < 10 ? 255 : 0;
}
static Material *material(const char *name, const char *image)
{
    Material *m = calloc(1, sizeof(Material));
    MaterialTextureDef *t = calloc(2, sizeof(MaterialTextureDef));
    GfxImage *colorMap = calloc(1, sizeof(GfxImage)), *other = calloc(1, sizeof(GfxImage));
    other->name = "detail_image";
    colorMap->name = image;
    t[0].name = "detailMap"; t[0].semantic = 2; t[0].u.image = other;
    t[1].name = "colorMap"; t[1].semantic = 2; t[1].u.image = colorMap;
    m->info.name = name; m->textureCount = 2; m->textures = t;
    return m;
}
static Material *picture(const char *image)
{
    Material *m = calloc(1, sizeof(Material));
    MaterialTextureDef *t = calloc(1, sizeof(MaterialTextureDef));
    GfxImage *img = calloc(1, sizeof(GfxImage));
    img->name = image; t->name = "colorMap"; t->u.image = img;
    m->info.name = image; m->textureCount = 1; m->textures = t;
    return m;
}
static const byte *texel(const GfxImage *image, int x, int y) { const Fake *f = image->texture.map; return f->level0 + (y * f->width + x) * 4; }
static int images(void) { return setups + generated - releases; }
int main(void)
{
    static Material raw;
    static MaterialTextureDef rawTexture;
    const struct DObj_s *obj = (const struct DObj_s *)&gunModel;
    Material *kar = material("mtl_weapon_kar98", "weapon_kar98_c"), *round = material("mtl_k98round", "k98round");
    Material *hands = material("mtl_hands", "hands");
    R_WeaponCamoDraw draw;
    int length, i;
    byte *file;

    rgp.rawMaterial = &raw; raw.textureCount = 1; raw.textures = &rawTexture;
    handsSkin[0] = hands; handsModel.lods[0] = handsSkin; handsModel.count = 1;
    gunSkin[0] = round; gunSkin[1] = kar; gunLod1[0] = round; gunLod1[1] = kar;
    gunModel.lods[0] = gunSkin; gunModel.lods[1] = gunLod1; gunModel.count = 2;
    file = iwi(0x0b, 512, 512, &length);
    fillDxt(file, 0x0b, 512, 512, gunColor);
    addFile("images/weapon_kar98_c.iwi", file, length);

    /* A painted gun: the material named, of this entity, this frame. */
    R_SetEntityWeaponCamo(&s_data.entities[7], obj, "mtl_weapon_kar98", 1, WEAPON_CAMO_WOOD);
    assert(R_GetDrawWeaponCamo(&s_data.entities[7], kar, &draw));
    assert(draw.pattern && draw.mask && draw.tiles == R_WEAPON_CAMO_TILES);
    assert(!R_GetDrawWeaponCamo(&s_data.entities[7], round, &draw) && !R_GetDrawWeaponCamo(&s_data.entities[7], hands, &draw));
    assert(!R_GetDrawWeaponCamo(&s_data.entities[6], kar, &draw) && !R_GetDrawWeaponCamo(NULL, kar, &draw));
    {
        static GfxEntity world;
        assert(!R_GetDrawWeaponCamo(&world, kar, &draw));
        assert(!R_GetDrawWeaponCamo((const GfxEntity *)((byte *)&s_data.entities[7] + 4), kar, &draw));
    }
    /* The pattern: 256 square with every mip; the mask: a texel per block. */
    {
        const Fake *pattern = draw.pattern->texture.map, *mask = draw.mask->texture.map;
        int r = 0, g = 0, b = 0, painted = 0, partial = 0;
        assert(pattern->width == 256 && pattern->height == 256 && pattern->levels == 9);
        assert(mask->width == 128 && mask->height == 128 && mask->levels == 8);
        for (int y = 0; y < 120; y++) {
            for (int x = 0; x < 128; x++) {
                int value = mask->level0[(y * 128 + x) * 4];
                assert(mask->level0[(y * 128 + x) * 4 + 3] == 255);
                if (x < 62) assert(value == 255);
                if (x > 66) assert(value == 0);
                painted += value == 255;
                partial += value > 0 && value < 255;
            }
        }
        for (int y = 121; y < 128; y++)
            for (int x = 0; x < 128; x++)
                assert(mask->level0[(y * 128 + x) * 4] == 0);
        assert(painted >= 62 * 120 && partial < 4 * 120);
        /* The median brightness of the wood as the block decodes it. */
        r = decoded(wood[0], 5); g = decoded(wood[1], 6); b = decoded(wood[2], 5);
        assert(fabsf(draw.reference - (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f) < 0.25f / 64.0f);
    }
    /* One mask a material and style, one pattern a camouflage. */
    {
        int before = setups, read = reads;
        R_SetEntityWeaponCamo(&s_data.entities[8], obj, "mtl_weapon_kar98", 1, WEAPON_CAMO_WOOD);
        assert(setups == before && reads == read && R_GetDrawWeaponCamo(&s_data.entities[8], kar, &draw));
        R_SetEntityWeaponCamo(&s_data.entities[9], obj, "mtl_weapon_kar98", 2, WEAPON_CAMO_WOOD);
        assert(setups == before + 1 && reads == read);
        R_SetEntityWeaponCamo(&s_data.entities[10], obj, "mtl_weapon_kar98", 2, WEAPON_CAMO_BODY);
        assert(setups == before + 2 && reads == read + 1 && R_GetDrawWeaponCamo(&s_data.entities[10], kar, &draw));
        /* The whole gun takes paint, but black. */
        {
            const Fake *mask = draw.mask->texture.map;
            assert(mask->level0[(10 * 128 + 100) * 4] == 255 && mask->level0[(10 * 128 + 10) * 4] == 255);
            assert(mask->level0[(125 * 128 + 100) * 4] == 0);
        }
    }
    /* An entity is painted once a frame; the first painting stands. */
    R_SetEntityWeaponCamo(&s_data.entities[7], obj, "mtl_weapon_kar98", 3, WEAPON_CAMO_WOOD);
    assert(R_GetDrawWeaponCamo(&s_data.entities[7], kar, &draw) && draw.pattern == &s_camoPatterns[1].image);
    /* No paint: none or an unknown camouflage, a material the object lacks, no object, an entity outside the frame. */
    R_SetEntityWeaponCamo(&s_data.entities[11], obj, "mtl_weapon_kar98", WEAPON_CAMO_NONE, WEAPON_CAMO_WOOD);
    R_SetEntityWeaponCamo(&s_data.entities[12], obj, "mtl_weapon_kar98", WEAPON_CAMO_COUNT, WEAPON_CAMO_WOOD);
    R_SetEntityWeaponCamo(&s_data.entities[13], obj, "mtl_weapon_mp44", 1, WEAPON_CAMO_WOOD);
    R_SetEntityWeaponCamo(&s_data.entities[14], NULL, "mtl_weapon_kar98", 1, WEAPON_CAMO_WOOD);
    R_SetEntityWeaponCamo(&s_data.entities[15], obj, NULL, 1, WEAPON_CAMO_WOOD);
    for (i = 11; i <= 15; i++)
        assert(!R_GetDrawWeaponCamo(&s_data.entities[i], kar, &draw));
    /* A material whose color map cannot be read is not painted, and not read again. */
    {
        Material *missing = material("mtl_missing", "missing_c");
        Material *missingSkin[1] = { missing };
        int read;
        gunModel.lods[0] = missingSkin; gunModel.count = 1;
        R_SetEntityWeaponCamo(&s_data.entities[16], obj, "mtl_missing", 1, WEAPON_CAMO_WOOD);
        read = reads;
        R_SetEntityWeaponCamo(&s_data.entities[17], obj, "mtl_missing", 1, WEAPON_CAMO_WOOD);
        assert(reads == read && !R_GetDrawWeaponCamo(&s_data.entities[16], missing, &draw));
        gunModel.lods[0] = gunSkin; gunModel.count = 2;
    }
    /* A color map wider than tall: the mask has every mip, to one texel. */
    {
        Material *bar = material("mtl_weapon_bar", "weapon_bar_c");
        Material *barSkin[1] = { bar };
        file = iwi(0x0b, 256, 128, &length);
        fillDxt(file, 0x0b, 256, 128, gunColor);
        addFile("images/weapon_bar_c.iwi", file, length);
        gunModel.lods[0] = barSkin; gunModel.count = 1;
        R_SetEntityWeaponCamo(&s_data.entities[18], obj, "mtl_weapon_bar", 1, WEAPON_CAMO_BODY);
        assert(R_GetDrawWeaponCamo(&s_data.entities[18], bar, &draw));
        {
            const Fake *mask = draw.mask->texture.map;
            assert(mask->width == 64 && mask->height == 32 && mask->levels == 7);
        }
        gunModel.lods[0] = gunSkin; gunModel.count = 2;
    }
    /* The frame's tags go with its entities, and are bounded. */
    R_ClearWeaponCamoEntities();
    assert(!R_GetDrawWeaponCamo(&s_data.entities[7], kar, &draw));
    for (i = 0; i < R_WEAPON_CAMO_TAGS + 8; i++)
        R_SetEntityWeaponCamo(&s_data.entities[100 + i], obj, "mtl_weapon_kar98", 1, WEAPON_CAMO_WOOD);
    assert(R_GetDrawWeaponCamo(&s_data.entities[100 + R_WEAPON_CAMO_TAGS - 1], kar, &draw));
    assert(!R_GetDrawWeaponCamo(&s_data.entities[100 + R_WEAPON_CAMO_TAGS], kar, &draw));
    R_ClearWeaponCamoEntities();
    R_SetEntityWeaponCamo(&s_data.entities[8183], obj, "mtl_weapon_kar98", 1, WEAPON_CAMO_WOOD);
    assert(R_GetDrawWeaponCamo(&s_data.entities[8183], kar, &draw));
    R_ClearWeaponCamoEntities();

    /* The swatches' material draws the pattern through a copy of $raw. */
    {
        MaterialHandle swatch = R_GetWeaponCamoMaterial(1);
        assert(swatch && swatch->textures[0].u.image == &s_camoPatterns[1].image && swatch->textures[0].samplerState == 0x3a);
        assert(R_GetWeaponCamoMaterial(1) == swatch && !R_GetWeaponCamoMaterial(WEAPON_CAMO_NONE));
        assert(!strcmp(swatch->info.name, "weapon_camo_frogskin"));
    }

    /* A retail picture: wood painted, steel and transparency kept. */
    file = iwi(0x0d, 64, 32, &length);
    fillDxt(file, 0x0d, 64, 32, pictureColor);
    addFile("images/weapon_kar98.iwi", file, length);
    {
        Material *source = picture("weapon_kar98");
        float aspect = 0.0f;
        MaterialHandle painted = R_GetWeaponCamoPicture(source, 0, 1, WEAPON_CAMO_WOOD, &aspect);
        const GfxImage *image;
        int changed = 0, read = reads;
        assert(painted && aspect == 2.0f && painted->textures[0].samplerState == 0x32);
        image = painted->textures[0].u.image;
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 64; x++) {
                const byte *t = texel(image, x, y);
                if (y < 8) { assert(t[3] == 0); continue; }
                assert(t[3] == 255);
                if (x < 28)
                    changed += abs(t[2] - decoded(wood[0], 5)) + abs(t[1] - decoded(wood[1], 6)) +
                               abs(t[0] - decoded(wood[2], 5)) > 24;
                if (x > 36)
                    assert(t[2] == decoded(metal[0], 5) && t[1] == decoded(metal[1], 6) && t[0] == decoded(metal[2], 5));
            }
        }
        assert(changed > 28 * 24 / 2);
        assert(R_GetWeaponCamoPicture(source, 0, 1, WEAPON_CAMO_WOOD, &aspect) == painted && reads == read && aspect == 2.0f);
        assert(!R_GetWeaponCamoPicture(source, 0, WEAPON_CAMO_NONE, WEAPON_CAMO_WOOD, &aspect) && aspect == 0.0f);
        assert(!R_GetWeaponCamoPicture(NULL, 0, 1, WEAPON_CAMO_WOOD, &aspect));
    }
    /* An icon's silhouette: filled with the pattern, trimmed as the HUD trims it. */
    file = iwi(0x0c, 64, 64, &length);
    fillDxt(file, 0x0c, 64, 64, iconColor);
    addFile("images/hud_luger.iwi", file, length);
    {
        float aspect = 0.0f;
        MaterialHandle painted = R_GetWeaponCamoPicture(picture("hud_luger"), 1, 10, WEAPON_CAMO_WOOD, &aspect);
        const GfxImage *image = painted ? painted->textures[0].u.image : NULL;
        int white = 0;
        assert(image && image->width == 34 && image->height == 18 && fabsf(aspect - 34.0f / 18.0f) < 1e-6f);
        for (int y = 1; y < 17; y++)
            for (int x = 1; x < 33; x++)
                white += texel(image, x, y)[0] > 240 && texel(image, x, y)[1] > 240 && texel(image, x, y)[2] > 240;
        assert(!white && texel(image, 0, 0)[3] == 0 && texel(image, 5, 5)[3] == 255);
    }
    /* An owner's A8R8G8B8 picture, named as a generated copy, stored B, G, R,
     * A: its wood is painted, its steel kept. All steel is not painted. */
    file = iwi(0x01, 16, 16, &length);
    for (i = 0; i < 256; i++) {
        const int *c = i % 16 < 8 ? wood : metal;
        file[28 + i * 4] = (byte)c[2]; file[29 + i * 4] = (byte)c[1]; file[30 + i * 4] = (byte)c[0]; file[31 + i * 4] = 255;
    }
    addFile("images/browser_class/kar98k_mp.iwi", file, length);
    file = iwi(0x01, 16, 16, &length);
    for (i = 0; i < 256; i++) { file[28 + i * 4] = 0x40; file[29 + i * 4] = 0x44; file[30 + i * 4] = 0x48; file[31 + i * 4] = 255; }
    addFile("images/browser_class/sten_mp.iwi", file, length);
    {
        float aspect = 0.0f;
        MaterialHandle painted = R_GetWeaponCamoPicture(picture("*browser_class/kar98k_mp"), 0, 4, WEAPON_CAMO_WOOD, &aspect);
        const byte *steel;
        assert(painted && aspect == 1.0f);
        steel = texel(painted->textures[0].u.image, 14, 8);
        assert(steel[0] == metal[2] && steel[1] == metal[1] && steel[2] == metal[0] && steel[3] == 255);
        assert(memcmp(texel(painted->textures[0].u.image, 2, 8), (byte[]){ (byte)wood[2], (byte)wood[1], (byte)wood[0] }, 3));
        Material *sten = picture("*browser_class/sten_mp");
        int read;
        assert(!R_GetWeaponCamoPicture(sten, 0, 4, WEAPON_CAMO_WOOD, &aspect) && aspect == 0.0f);
        read = reads;
        assert(!R_GetWeaponCamoPicture(sten, 0, 4, WEAPON_CAMO_WOOD, &aspect) && reads == read);
        assert(R_GetWeaponCamoPicture(sten, 0, 4, WEAPON_CAMO_BODY, &aspect) && aspect == 1.0f);
    }
    /* Sixteen pictures at most; one drawn this frame stays for it. */
    {
        Material *source = picture("weapon_kar98");
        float aspect;
        MaterialHandle first;
        int built = generated;
        s_data.frameCount = 50;
        first = R_GetWeaponCamoPicture(source, 0, 1, WEAPON_CAMO_BODY, &aspect);
        for (i = 2; i < WEAPON_CAMO_COUNT; i++)
            assert(R_GetWeaponCamoPicture(source, 0, i, WEAPON_CAMO_BODY, &aspect));
        for (i = 2; i < 8; i++)
            assert(R_GetWeaponCamoPicture(source, 0, i, WEAPON_CAMO_WOOD, &aspect));
        assert(generated == built + 16);
        assert(!R_GetWeaponCamoPicture(source, 0, 8, WEAPON_CAMO_WOOD, &aspect) && aspect == 0.0f);
        s_data.frameCount = 51;
        assert(R_GetWeaponCamoPicture(source, 0, 8, WEAPON_CAMO_WOOD, &aspect) && generated == built + 17);
        assert(R_GetWeaponCamoPicture(source, 0, 1, WEAPON_CAMO_BODY, &aspect) && generated == built + 18);
        (void)first;
    }
    /* Shutting the renderer down releases every image and every tag. */
    R_SetEntityWeaponCamo(&s_data.entities[3], obj, "mtl_weapon_kar98", 1, WEAPON_CAMO_WOOD);
    assert(R_GetDrawWeaponCamo(&s_data.entities[3], kar, &draw));
    R_ShutdownWeaponCamos();
    assert(images() == 0 && unbinds == releases && !R_GetDrawWeaponCamo(&s_data.entities[3], kar, &draw));
    R_SetEntityWeaponCamo(&s_data.entities[3], obj, "mtl_weapon_kar98", 1, WEAPON_CAMO_WOOD);
    assert(R_GetDrawWeaponCamo(&s_data.entities[3], kar, &draw) && images() == 2);
    R_ShutdownWeaponCamos();
    puts("PASS: a painted entity's own material only, for its frame; masks of wood or the whole gun, cleared of "
         "specks, with the painted median; one pattern with its mipmaps and one mask a material, failures "
         "remembered; 128 tags; swatches and painted pictures, silhouettes trimmed, owners' A8R8G8B8 pictures; "
         "16 pictures kept through their frame; shutdown releases everything");
    return 0;
}
'''

cgame_checks = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "@SOURCE@"
static const char *configstrings[0x800];
const char *CL_GetConfigString(int index) { assert(index >= 0 && index < 0x800); return configstrings[index] ? configstrings[index] : ""; }
static WeaponDef defs[8] = { { "none" }, { "kar98k_mp" }, { "luger_mp" }, { "mp40_mp" }, { "frag_grenade_german_mp" },
                             { "kar98k_sniper_mp" } };
static int weaponCount = 5;
WeaponDef *BG_GetWeaponDef(int weapon) { assert(weapon >= 0 && weapon <= weaponCount); return &defs[weapon]; }
int BG_GetNumWeapons(void) { return weaponCount; }
static struct { GfxEntity *ent; const struct DObj_s *obj; const char *material; int camo, style; } painted[8];
static int paints;
void R_SetEntityWeaponCamo(GfxEntity *ent, const struct DObj_s *obj, const char *material, int camo, int style)
{
    painted[paints].ent = ent; painted[paints].obj = obj; painted[paints].material = material;
    painted[paints].camo = camo; painted[paints++].style = style;
}
int main(void)
{
    static GfxEntity ent;
    const struct DObj_s *obj = (const struct DObj_s *)&ent;
    const int frogskin = BG_WeaponCamo_Find("frogskin"), ember = BG_WeaponCamo_Find("ember");
    configstrings[CS_WEAPON_CAMOS + 3] = "kar98k_mp frogskin luger_mp ember";
    assert(CG_GetClientWeaponCamo(3, 1) == frogskin && CG_GetClientWeaponCamo(3, 2) == ember);
    assert(!CG_GetClientWeaponCamo(3, 3) && !CG_GetClientWeaponCamo(4, 1) && !CG_GetClientWeaponCamo(3, 0));
    assert(!CG_GetClientWeaponCamo(3, 6) && !CG_GetClientWeaponCamo(-1, 1) && !CG_GetClientWeaponCamo(64, 1));
    /* The text changes with the next class; a weapon is known by its name. */
    configstrings[CS_WEAPON_CAMOS + 3] = "kar98k_sniper_mp winter  luger_mp none";
    weaponCount = 6;
    assert(CG_GetClientWeaponCamo(3, 5) == BG_WeaponCamo_Find("winter") && !CG_GetClientWeaponCamo(3, 1));
    assert(!CG_GetClientWeaponCamo(3, 2));
    configstrings[CS_WEAPON_CAMOS + 3] = "kar98k_mp tiger luger_mp ember mp40_mp";
    assert(!CG_GetClientWeaponCamo(3, 1) && CG_GetClientWeaponCamo(3, 2) == ember && !CG_GetClientWeaponCamo(3, 3));
    configstrings[CS_WEAPON_CAMOS + 3] = "kar98k_mp_with_a_name_much_longer_than_any_weapon frogskin";
    assert(!CG_GetClientWeaponCamo(3, 1));
    /* A name too long spoils the whole string, not just its first 31 letters. */
    configstrings[CS_WEAPON_CAMOS + 3] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaax luger_mp ember";
    assert(!CG_GetClientWeaponCamo(3, 2));
    configstrings[CS_WEAPON_CAMOS + 63] = "luger_mp frogskin";
    assert(CG_GetClientWeaponCamo(63, 2) == frogskin);
    configstrings[CS_WEAPON_CAMOS + 3] = "";
    assert(!CG_GetClientWeaponCamo(3, 2));
    /* Painting goes to the gun's own material and style; grenades and unpainted guns have none. */
    configstrings[CS_WEAPON_CAMOS + 5] = "kar98k_mp frogskin luger_mp ember frag_grenade_german_mp frogskin";
    CG_SetWeaponCamo(&ent, obj, 5, 1);
    CG_SetWeaponCamo(&ent, obj, 5, 2);
    CG_SetWeaponCamo(&ent, obj, 5, 3);
    CG_SetWeaponCamo(&ent, obj, 5, 4);
    CG_SetWeaponCamo(NULL, obj, 5, 1);
    CG_SetWeaponCamo(&ent, NULL, 5, 1);
    assert(paints == 2);
    assert(painted[0].ent == &ent && painted[0].obj == obj && !strcmp(painted[0].material, "mtl_weapon_kar98") &&
           painted[0].camo == frogskin && painted[0].style == WEAPON_CAMO_WOOD);
    assert(!strcmp(painted[1].material, "mtl_weapon_luger") && painted[1].camo == ember && painted[1].style == WEAPON_CAMO_BODY);
    puts("PASS: each client's configstring read by weapon name, reread when it changes, bounded tokens; "
         "a class gun painted in its own material and style");
    return 0;
}
'''

server_checks = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "@SOURCE@"
static gclient_t clients[64];
static gentity_t entities[80];
void *imp_g_entities = entities;
static int params = 1, errors, objectErrors, paramErrors, sets;
static const char *argument = "", *setValue;
static int setIndex;
unsigned int Scr_GetNumParam(void) { return (unsigned int)params; }
const char *Scr_GetString(unsigned int index) { assert(index == 0); return argument; }
void Scr_Error(const char *msg) { (void)msg; errors++; }
void Scr_ObjectError(const char *msg) { (void)msg; objectErrors++; }
void Scr_ParamError(unsigned int index, const char *msg) { assert(index == 0); (void)msg; paramErrors++; }
void SV_SetConfigstring(int index, const char *val) { setIndex = index; setValue = val; sets++; }
static void call(int entnum, int classnum, const char *text)
{
    scr_entref_t ref = { (unsigned short)entnum, (unsigned short)classnum };
    argument = text;
    PlayerCmd_SetWeaponCamos(ref);
}
int main(void)
{
    for (int i = 0; i < 64; i++) entities[i].client = &clients[i];
    call(3, 0, "kar98k_mp frogskin luger_mp none");
    assert(sets == 1 && setIndex == CS_WEAPON_CAMOS + 3 && !strcmp(setValue, "kar98k_mp frogskin luger_mp none"));
    call(63, 0, "");
    assert(sets == 2 && setIndex == CS_WEAPON_CAMOS + 63 && !strcmp(setValue, ""));
    call(5, 0, "PPS42_mp ember TT30_mp desert");
    assert(sets == 3);
    {
        static char longest[WEAPON_CAMO_STRING_MAX], tooLong[WEAPON_CAMO_STRING_MAX + 1];
        const char *invalid[] = { "kar98k_mp frogskin;quit", "kar98k_mp \"frogskin\"", "a\\b", "a\nb", "^1red", "a\tb",
                                  tooLong };
        memset(longest, 'a', sizeof(longest) - 1);
        memset(tooLong, 'a', sizeof(tooLong) - 1);
        call(3, 0, longest);
        assert(sets == 4 && !strcmp(setValue, longest));
        for (int i = 0; i < (int)(sizeof(invalid) / sizeof(invalid[0])); i++) {
            call(3, 0, invalid[i]);
            assert(paramErrors == i + 1 && sets == 4);
        }
    }
    entities[7].client = NULL;
    call(7, 0, "kar98k_mp frogskin");
    call(64, 0, "kar98k_mp frogskin");
    call(3, 1, "kar98k_mp frogskin");
    assert(objectErrors == 3 && sets == 4);
    params = 2;
    call(3, 0, "kar98k_mp frogskin");
    params = 0;
    call(3, 0, "kar98k_mp frogskin");
    assert(errors == 2 && sets == 4);
    assert(!G_ValidWeaponCamos(NULL) && G_ValidWeaponCamos("") && G_ValidWeaponCamos("abc_09 XYZ"));
    puts("PASS: setWeaponCamos sets its player's configstring, takes names and ids between spaces up to the "
         "clients' copy, refuses other entities and wrong counts");
    return 0;
}
'''

catalog_mutants = [
    ('    int x1 = (x0 + 1) % period, y1 = (y0 + 1) % period;', '    int x1 = x0 + 1, y1 = y0 + 1;'),
    ('            texel[0] = (unsigned char)rgb[2];\n            texel[1] = (unsigned char)rgb[1];\n            texel[2] = (unsigned char)rgb[0];',
     '            texel[0] = (unsigned char)rgb[0];\n            texel[1] = (unsigned char)rgb[1];\n            texel[2] = (unsigned char)rgb[2];'),
    ('                    rgb[c] += ((float)layer->color[c] - rgb[c]) * a;', '                    rgb[c] += ((float)layer->color[c] - rgb[c]);'),
    ('        (size & (size - 1)))\n        return 0;', '        0)\n        return 0;'),
    ('    { "winter", "Winter", "Snow whitewash" },', '    { "winter", "Ember", "Snow whitewash" },'),
    ('    { "luger_mp", "mtl_weapon_luger", WEAPON_CAMO_BODY },', '    { "luger_mp", "mtl_weapon_luger", WEAPON_CAMO_WOOD },'),
    ('        if (!strcmp(g_weaponCamoGuns[i].weapon, weapon))', '        if (!strcasecmp(g_weaponCamoGuns[i].weapon, weapon))'),
]
renderer_mutants = [
    ('    if (!tag || tag->material != material)\n        return 0;', '    if (!tag)\n        return 0;'),
    ('    R_CamoDropSpecks(mask, mw, mh, queue);\n', ''),
    ('    if (index < 0 || !obj || !materialName || s_camoTagOfEntity[index] || s_camoTagCount == R_WEAPON_CAMO_TAGS)',
     '    if (index < 0 || !obj || !materialName || s_camoTagCount == R_WEAPON_CAMO_TAGS)'),
    ('    if (offset % sizeof(GfxEntity) || offset / sizeof(GfxEntity) >= (uintptr_t)R_CAMO_ENTITY_COUNT)',
     '    if (offset / sizeof(GfxEntity) >= (uintptr_t)R_CAMO_ENTITY_COUNT)'),
    ('        s_camoTagOfEntity[s_camoTags[i].entity] = 0;\n', ''),
    ('    if (entry->state && entry->frame == frame)\n        return NULL;\n', ''),
    ('    if (imageName && imageName[0] == \'*\')\n        imageName++;\n', ''),
    ('            rgba[i * 4] = texel[2];\n            rgba[i * 4 + 1] = texel[1];\n            rgba[i * 4 + 2] = texel[0];',
     '            rgba[i * 4] = texel[0];\n            rgba[i * 4 + 1] = texel[1];\n            rgba[i * 4 + 2] = texel[2];'),
    ('    minX = minX > 0 ? minX - 1 : 0;\n', ''),
    ('        if (def->semantic == 2 && def->name && !strcmp(def->name, "colorMap"))', '        if (def->semantic == 2)'),
    ('    for (i = 0; i < s_camoMaskCount; i++)\n        R_CamoReleaseImage(&s_camoMasks[i].image);\n', ''),
    ('            return ((float)i + 0.5f) / 64.0f;', '            return (float)i / 64.0f;'),
    ('        if (width == 1 && height == 1)\n            break;', '        if (width == 1 || height == 1)\n            break;'),
    ('    entry->state = R_CamoBuildPicture(entry) ? 1 : -1;', '    entry->state = R_CamoBuildPicture(entry) ? 1 : 0;'),
    ('                if (pass == 2 ? value > extreme : value < extreme)', '                if (value > extreme)'),
]
cgame_mutants = [
    ('    if (strcmp(camos->text, text))\n        CG_ParseWeaponCamos(text, camos);', '    if (!camos->text[0])\n        CG_ParseWeaponCamos(text, camos);'),
    ('weapon <= 0 || weapon > BG_GetNumWeapons()', 'weapon <= 0'),
    ('        if (length >= CG_CAMO_WEAPON_NAME)\n            return;', '        if (length >= CG_CAMO_WEAPON_NAME)\n            length = CG_CAMO_WEAPON_NAME - 1;'),
    ('        R_SetEntityWeaponCamo(sceneEnt, obj, gun->material, camo, gun->style);', '        R_SetEntityWeaponCamo(sceneEnt, obj, gun->material, camo, WEAPON_CAMO_WOOD);'),
]
server_mutants = [
    ('    SV_SetConfigstring(CS_WEAPON_CAMOS + entref.entnum, text);', '    SV_SetConfigstring(CS_WEAPON_CAMOS, text);'),
    ("        if (!(c == ' ' || c == '_' ||", "        if (!(c == ' ' || c == '_' || c == ';' ||"),
    ('    if (!text || length >= WEAPON_CAMO_STRING_MAX)', '    if (!text)'),
    ('    if (entref.classnum != 0 || entref.entnum >= WEAPON_CAMO_CLIENTS ||', '    if (entref.entnum >= WEAPON_CAMO_CLIENTS ||'),
    ('    if (Scr_GetNumParam() != 1) {', '    if (Scr_GetNumParam() < 1) {'),
]


def build(directory, name, source_text, mutating, *sources):
    path = Path(directory)
    (path / 'common_types.h').write_text(common_types)
    (path / 'imports.h').write_text(imports)
    (path / f'{name}.c').write_text(source_text)
    flags = ['-std=gnu99', '-I', str(path), '-I', str(root / 'src'), '-fsanitize=address,undefined',
             '-fno-sanitize-recover=all', '-g']
    warnings = ['-O0', '-w'] if mutating else ['-O1', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                                               '-Wno-unused-function', '-Wno-missing-field-initializers']
    subprocess.run(['cc', *flags, *warnings, str(path / f'{name}.c'), *map(str, sources), '-lm', '-o',
                    str(path / name)], check=True)
    return subprocess.run([str(path / name)], capture_output=True, text=True)


def check(directory, name, checks, target, mutants):
    """The checks with target included; the catalog, a file of its own, links with the others."""
    separate = () if target == camo else (camo,)
    program = checks.replace('@CAMO@', str(camo)).replace('@SOURCE@', str(target))
    result = build(directory, name, program, False, *separate)
    assert result.returncode == 0, (name, result.stdout, result.stderr[-3000:])
    print(result.stdout.strip())
    original = target.read_text()
    for index, (before, after) in enumerate(mutants):
        assert original.count(before) == 1, (name, before)
        mutant = Path(directory) / f'{name}-source{index}.c'
        mutant.write_text(original.replace(before, after))
        variant = checks.replace('@CAMO@', str(mutant)).replace('@SOURCE@', str(mutant))
        result = build(directory, f'{name}-mutant{index}', variant, True, *separate)
        assert result.returncode != 0, f'mutant survived: {after!r}'
    return len(mutants)


with tempfile.TemporaryDirectory(prefix='cod2-weapon-camo-') as directory:
    killed = check(directory, 'catalog', catalog_checks, camo, catalog_mutants)
    killed += check(directory, 'renderer', renderer_checks, renderer, renderer_mutants)
    killed += check(directory, 'cgame', cgame_checks, cgame, cgame_mutants)
    killed += check(directory, 'server', server_checks, server, server_mutants)
print(f'PASS: {killed} weapon camouflage mutants fail')
