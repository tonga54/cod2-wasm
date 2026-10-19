#include "common_types.h"
#include "imports.h"
#include "headers/PC/bgame/bg_weapon_camo.h"
#include "headers/PC/gfx_d3d/r_weapon_camo.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern r_global_permanent_t rgp;
extern GfxBackEndData *frontEndDataOut;
extern void Image_Setup(GfxImage *image, int width, int height, int depth, int imageFlags, DWORD usage,
                        D3DFORMAT imageFormat);
extern D3DCUBEMAP_FACES Image_CubemapFace(int face);
extern void Image_UploadData(GfxImage *image, D3DFORMAT format, int face, int mipLevel, const byte *src);
extern void Image_Generate2D(GfxImage *image, byte *pixels, int width, int height, int imageFormat);
extern void Image_Release(GfxImage *image);
extern void RB_UnbindImage(const GfxImage *image);
extern int FS_ReadFile(const char *qpath, void **buffer);
extern void FS_FreeFile(void *buffer);
extern int DObjGetNumModels(const struct DObj_s *obj);
extern XModel *DObjGetModel(const struct DObj_s *obj, int modelIndex);
extern const struct trXSkin_t *XModelGetSkins(const XModel *model);
extern int XModelGetNumLods(const XModel *model);
extern int XModelGetSurfaces(const XModel *model, struct XSurface_s ***surfaces, int lod, int **partBits);

/* No picmip, with mipmaps (see Image_Setup). */
#define R_CAMO_IMAGE_FLAGS 1
/* Linear filtering between mipmaps, or without them; clamped in U and V
 * (see RB_SetSampler). */
#define R_CAMO_SWATCH_SAMPLER 0x3a
#define R_CAMO_PICTURE_SAMPLER 0x32
#define R_CAMO_IWI_VERSION 5
#define R_CAMO_IWI_ARGB32 0x01
#define R_CAMO_IWI_DXT1 0x0b
#define R_CAMO_IWI_DXT3 0x0c
#define R_CAMO_IWI_DXT5 0x0d
#define R_CAMO_IMAGE_MAX 2048
/* A picture is painted at most this many texels a side; an owner's larger
 * one is halved first. */
#define R_CAMO_PICTURE_MAX 512
/* A mask texel covers a compression block of a gun's color map, 2 by 2
 * texels of the smaller menu pictures. */
#define R_CAMO_MASK_BLOCK 4
#define R_CAMO_PICTURE_BLOCK 2
#define R_CAMO_MASKS 32
#define R_CAMO_PICTURES 16
#define R_CAMO_ENTITY_COUNT ((int)(sizeof(((GfxBackEndData *)0)->entities) / sizeof(GfxEntity)))
/* How much a texel's own shade, against the material's median, shows
 * through the paint: grain, edges and wear. */
#define R_CAMO_WEAR 0.4f
/* Where wood begins: its red over its blue, against its brightness. Blued
 * steel with a warm cast, such as a Kar98k's bolt, stays below a color
 * map's; the retail pictures light their walnut darker. */
#define R_CAMO_MAP_WARMTH 0.17f
#define R_CAMO_PICTURE_WARMTH 0.13f

typedef struct {
    /* 0 until first asked for, then 1 built or -1 failed. */
    int state;
    char name[32];
    /* The pattern on the CPU, for the menus' pictures. */
    byte *texels;
    GfxImage image;
    MaterialTextureDef texture;
    Material material;
} CamoPattern;

typedef struct {
    const Material *material;
    int style;
    int state;
    float reference;
    char name[32];
    GfxImage image;
} CamoMask;

typedef struct {
    const Material *source;
    int silhouette;
    int camo;
    int style;
    int state;
    int frame;
    unsigned int used;
    float aspect;
    char name[40];
    GfxImage image;
    MaterialTextureDef texture;
    Material material;
} CamoPicture;

typedef struct {
    int entity;
    const Material *material;
    CamoPattern *pattern;
    CamoMask *mask;
} CamoTag;

static CamoPattern s_camoPatterns[WEAPON_CAMO_COUNT];
static CamoMask s_camoMasks[R_CAMO_MASKS];
static int s_camoMaskCount;
static CamoPicture s_camoPictures[R_CAMO_PICTURES];
static unsigned int s_camoPictureClock;
static CamoTag s_camoTags[R_WEAPON_CAMO_TAGS];
static int s_camoTagCount;
/* A frame entity's tag, plus one. */
static byte s_camoTagOfEntity[R_CAMO_ENTITY_COUNT];

static float R_CamoStep(float edge0, float edge1, float x)
{
    float t = (x - edge0) / (edge1 - edge0);

    t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
    return t * t * (3.0f - 2.0f * t);
}

static void R_CamoReleaseImage(GfxImage *image)
{
    if (image->texture.map) {
        RB_UnbindImage(image);
        Image_Release(image);
    }
    memset(image, 0, sizeof(*image));
}

/* A copy of $raw, the alpha-blended 2D material, drawing image. */
static MaterialHandle R_CamoCopyRaw(Material *copy, MaterialTextureDef *texture, GfxImage *image, const char *name,
                                    int samplerState)
{
    const Material *raw = rgp.rawMaterial;

    if (!raw || raw == rgp.defaultMaterial || raw->textureCount != 1 || !raw->textures)
        return NULL;
    *texture = raw->textures[0];
    texture->samplerState = (GfxSamplerState)samplerState;
    texture->u.image = image;
    *copy = *raw;
    copy->info.name = name;
    copy->textures = texture;
    return copy;
}

/* Uploads width * height B, G, R, A texels, then mipmaps that each average
 * 2 by 2 of the last, sides halved and rounded down as the device sizes
 * them. The texels are overwritten. */
static int R_CamoUpload(GfxImage *image, const char *name, byte *bgra, int width, int height)
{
    int level;

    memset(image, 0, sizeof(*image));
    image->name = name;
    Image_Setup(image, width, height, 1, R_CAMO_IMAGE_FLAGS, 0, D3DFMT_A8R8G8B8);
    if (!image->texture.map)
        return 0;
    for (level = 0;; level++) {
        int nextWidth = width > 1 ? width / 2 : 1, nextHeight = height > 1 ? height / 2 : 1, x, y, c;

        Image_UploadData(image, D3DFMT_A8R8G8B8, (int)Image_CubemapFace(0), level, bgra);
        if (width == 1 && height == 1)
            break;
        /* In place: each texel is written before any later one is read. */
        for (y = 0; y < nextHeight; y++) {
            int y0 = y * 2 < height ? y * 2 : height - 1, y1 = y * 2 + 1 < height ? y * 2 + 1 : height - 1;

            for (x = 0; x < nextWidth; x++) {
                int x0 = x * 2 < width ? x * 2 : width - 1, x1 = x * 2 + 1 < width ? x * 2 + 1 : width - 1;

                for (c = 0; c < 4; c++) {
                    int sum = bgra[(y0 * width + x0) * 4 + c] + bgra[(y0 * width + x1) * 4 + c] +
                              bgra[(y1 * width + x0) * 4 + c] + bgra[(y1 * width + x1) * 4 + c];

                    bgra[(y * nextWidth + x) * 4 + c] = (byte)((sum + 2) / 4);
                }
            }
        }
        width = nextWidth;
        height = nextHeight;
    }
    return 1;
}

static CamoPattern *R_CamoPattern(int camo)
{
    CamoPattern *pattern;
    byte *upload;
    const size_t bytes = (size_t)R_WEAPON_CAMO_SIZE * R_WEAPON_CAMO_SIZE * 4;

    if (camo <= WEAPON_CAMO_NONE || camo >= WEAPON_CAMO_COUNT)
        return NULL;
    pattern = &s_camoPatterns[camo];
    if (pattern->state)
        return pattern->state > 0 ? pattern : NULL;
    pattern->state = -1;
    pattern->texels = (byte *)malloc(bytes);
    upload = (byte *)malloc(bytes);
    if (pattern->texels && upload && BG_WeaponCamo_Generate(camo, R_WEAPON_CAMO_SIZE, pattern->texels)) {
        snprintf(pattern->name, sizeof(pattern->name), "*weapon_camo_%s", g_weaponCamos[camo].id);
        memcpy(upload, pattern->texels, bytes);
        if (R_CamoUpload(&pattern->image, pattern->name, upload, R_WEAPON_CAMO_SIZE, R_WEAPON_CAMO_SIZE))
            pattern->state = 1;
    }
    free(upload);
    if (pattern->state < 0) {
        free(pattern->texels);
        pattern->texels = NULL;
        return NULL;
    }
    return pattern;
}

MaterialHandle R_GetWeaponCamoMaterial(int camo)
{
    CamoPattern *pattern = R_CamoPattern(camo);

    if (!pattern)
        return NULL;
    if (!pattern->material.textures)
        R_CamoCopyRaw(&pattern->material, &pattern->texture, &pattern->image, pattern->name + 1, R_CAMO_SWATCH_SAMPLER);
    return pattern->material.textures ? &pattern->material : NULL;
}

static void R_CamoColorBlock(const byte *block, int dxt1, byte *rgba)
{
    unsigned int endpoint[2], bits;
    byte palette[4][4];
    int i, c;

    endpoint[0] = block[0] | (block[1] << 8);
    endpoint[1] = block[2] | (block[3] << 8);
    for (i = 0; i < 2; i++) {
        palette[i][0] = (byte)(((endpoint[i] >> 11) & 31) * 255 / 31);
        palette[i][1] = (byte)(((endpoint[i] >> 5) & 63) * 255 / 63);
        palette[i][2] = (byte)((endpoint[i] & 31) * 255 / 31);
        palette[i][3] = 255;
    }
    /* A DXT1 block whose first color is not above its second holds three
     * colors and transparent black. */
    for (c = 0; c < 3; c++) {
        if (!dxt1 || endpoint[0] > endpoint[1]) {
            palette[2][c] = (byte)((2 * palette[0][c] + palette[1][c]) / 3);
            palette[3][c] = (byte)((palette[0][c] + 2 * palette[1][c]) / 3);
        } else {
            palette[2][c] = (byte)((palette[0][c] + palette[1][c]) / 2);
            palette[3][c] = 0;
        }
    }
    palette[2][3] = 255;
    palette[3][3] = (byte)(!dxt1 || endpoint[0] > endpoint[1] ? 255 : 0);
    bits = (unsigned int)block[4] | ((unsigned int)block[5] << 8) | ((unsigned int)block[6] << 16) |
           ((unsigned int)block[7] << 24);
    for (i = 0; i < 16; i++)
        memcpy(rgba + i * 4, palette[(bits >> (2 * i)) & 3], 4);
}

static void R_CamoAlphaBlock(const byte *block, int format, byte *rgba)
{
    byte palette[8];
    unsigned long long bits = 0;
    int i;

    if (format == R_CAMO_IWI_DXT3) {
        for (i = 0; i < 16; i++)
            rgba[i * 4 + 3] = (byte)(((block[i >> 1] >> ((i & 1) * 4)) & 15) * 17);
        return;
    }
    palette[0] = block[0];
    palette[1] = block[1];
    for (i = 2; i < 8; i++) {
        if (block[0] > block[1])
            palette[i] = (byte)(((8 - i) * block[0] + (i - 1) * block[1]) / 7);
        else
            palette[i] = (byte)(i < 6 ? ((6 - i) * block[0] + (i - 1) * block[1]) / 5 : i == 6 ? 0 : 255);
    }
    for (i = 0; i < 6; i++)
        bits |= (unsigned long long)block[2 + i] << (8 * i);
    for (i = 0; i < 16; i++)
        rgba[i * 4 + 3] = palette[(bits >> (3 * i)) & 7];
}

/* The top mip of an image's DXT1, DXT3, DXT5 or A8R8G8B8 IWi as R, G, B, A
 * texels to free, or NULL. A generated image's name starts with '*': the
 * owner's pictures are such copies of their files. */
static byte *R_CamoDecode(const char *imageName, int *width, int *height)
{
    const GfxImageFileHeader *header;
    char path[96];
    void *file = NULL;
    byte *rgba = NULL;
    int length, w, h, blockBytes, blocksW, blocksH, bx, by, i;

    if (imageName && imageName[0] == '*')
        imageName++;
    if (!imageName || snprintf(path, sizeof(path), "images/%s.iwi", imageName) >= (int)sizeof(path))
        return NULL;
    length = FS_ReadFile(path, &file);
    if (length < (int)sizeof(*header) || !file) {
        if (file)
            FS_FreeFile(file);
        return NULL;
    }
    header = (const GfxImageFileHeader *)file;
    w = header->dimensions[0];
    h = header->dimensions[1];
    if (!memcmp(header->tag, "IWi", 3) && header->version == R_CAMO_IWI_VERSION &&
        header->format == R_CAMO_IWI_ARGB32 && w >= 1 && h >= 1 && w <= R_CAMO_IMAGE_MAX && h <= R_CAMO_IMAGE_MAX &&
        length - (int)sizeof(*header) >= w * h * 4) {
        /* B, G, R, A; mips are stored smallest first, so the top mip ends the file. */
        const byte *texel = (const byte *)file + length - w * h * 4;

        rgba = (byte *)malloc((size_t)w * h * 4);
        for (i = 0; rgba && i < w * h; i++, texel += 4) {
            rgba[i * 4] = texel[2];
            rgba[i * 4 + 1] = texel[1];
            rgba[i * 4 + 2] = texel[0];
            rgba[i * 4 + 3] = texel[3];
        }
        if (rgba) {
            *width = w;
            *height = h;
        }
        FS_FreeFile(file);
        return rgba;
    }
    blockBytes = header->format == R_CAMO_IWI_DXT1 ? 8 : header->format == R_CAMO_IWI_DXT3 ||
                                                         header->format == R_CAMO_IWI_DXT5 ? 16 : 0;
    blocksW = (w + 3) / 4;
    blocksH = (h + 3) / 4;
    if (!memcmp(header->tag, "IWi", 3) && header->version == R_CAMO_IWI_VERSION && blockBytes && w >= 1 && h >= 1 &&
        w <= R_CAMO_IMAGE_MAX && h <= R_CAMO_IMAGE_MAX &&
        length - (int)sizeof(*header) >= blocksW * blocksH * blockBytes)
        rgba = (byte *)malloc((size_t)w * h * 4);
    if (rgba) {
        const byte *block = (const byte *)file + length - blocksW * blocksH * blockBytes;

        for (by = 0; by < blocksH; by++) {
            for (bx = 0; bx < blocksW; bx++, block += blockBytes) {
                byte texels[16 * 4];

                R_CamoColorBlock(blockBytes == 8 ? block : block + 8, blockBytes == 8, texels);
                if (blockBytes == 16)
                    R_CamoAlphaBlock(block, header->format, texels);
                for (i = 0; i < 16; i++) {
                    int x = bx * 4 + (i & 3), y = by * 4 + (i >> 2);

                    if (x < w && y < h)
                        memcpy(rgba + ((size_t)y * w + x) * 4, texels + i * 4, 4);
                }
            }
        }
        *width = w;
        *height = h;
    }
    FS_FreeFile(file);
    return rgba;
}

/* Clears painted regions with fewer solid cells than a 400th of the mask,
 * their soft rims included: specks of warm grime on metal, not a gun's
 * furniture. queue holds an int per cell. */
static void R_CamoDropSpecks(float *mask, int mw, int mh, int *queue)
{
    static const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
    int minimum = mw * mh / 400 > 4 ? mw * mh / 400 : 4, start, i;

    /* Visited cells are negative until the end. */
    for (start = 0; start < mw * mh; start++) {
        int head = 0, tail = 0, solid = 0;

        if (mask[start] <= 0.0f)
            continue;
        queue[tail++] = start;
        mask[start] = -mask[start];
        while (head < tail) {
            int cell = queue[head++], x = cell % mw, y = cell / mw, d;

            solid += mask[cell] < -0.5f;
            for (d = 0; d < 4; d++) {
                int nx = x + dx[d], ny = y + dy[d], next = ny * mw + nx;

                if (nx < 0 || nx >= mw || ny < 0 || ny >= mh || mask[next] <= 0.0f)
                    continue;
                mask[next] = -mask[next];
                queue[tail++] = next;
            }
        }
        if (solid < minimum) {
            for (i = 0; i < tail; i++)
                mask[queue[i]] = 0.0f;
        }
    }
    for (i = 0; i < mw * mh; i++)
        mask[i] = mask[i] < 0.0f ? -mask[i] : 0.0f;
}

/* Where texels take paint, one value per block of them: block averages,
 * weighted by alpha, hide the compression's noise; a majority over
 * neighbors drops stray grime, a closing fills holes in the grain and small
 * regions go. Wood is warmer than warmth; for the body, any lit texel takes
 * paint. Returns the median brightness of the painted blocks, 0 when there
 * are none. scratch and luma hold a value per block, queue an int. */
static float R_CamoMask(const byte *rgba, int width, int height, int block, int style, float warmth, float *mask,
                        float *scratch, float *luma, int *queue)
{
    int mw = width / block, mh = height / block, histogram[64], painted = 0, x, y, pass, i;
    float *from = mask, *to = scratch;

    memset(histogram, 0, sizeof(histogram));
    for (y = 0; y < mh; y++) {
        for (x = 0; x < mw; x++) {
            float sum[4] = { 0.0f, 0.0f, 0.0f, 0.0f }, r = 0.0f, g = 0.0f, b = 0.0f, hi, lit;
            int sx, sy;

            for (sy = y * block; sy < y * block + block; sy++) {
                for (sx = x * block; sx < x * block + block; sx++) {
                    const byte *texel = rgba + ((size_t)sy * width + sx) * 4;
                    float a = (float)texel[3];

                    sum[0] += texel[0] * a;
                    sum[1] += texel[1] * a;
                    sum[2] += texel[2] * a;
                    sum[3] += a;
                }
            }
            if (sum[3] > 0.0f) {
                r = sum[0] / (255.0f * sum[3]);
                g = sum[1] / (255.0f * sum[3]);
                b = sum[2] / (255.0f * sum[3]);
            }
            hi = r > g ? (r > b ? r : b) : (g > b ? g : b);
            lit = R_CamoStep(0.035f, 0.09f, hi) * R_CamoStep(0.3f, 0.7f, sum[3] / (255.0f * (float)(block * block)));
            i = y * mw + x;
            luma[i] = 0.299f * r + 0.587f * g + 0.114f * b;
            mask[i] = style == WEAPON_CAMO_WOOD ? R_CamoStep(warmth, warmth + 0.12f, (r - b) / (hi + 0.03f)) * lit
                                                : lit;
        }
    }
    /* Two majorities, then a dilation and an erosion: four passes between
     * the two buffers, which leave the result in mask. */
    for (pass = 0; pass < 4; pass++) {
        for (y = 0; y < mh; y++) {
            for (x = 0; x < mw; x++) {
                float total = 0.0f, extreme = pass == 2 ? 0.0f : 1.0f;
                int count = 0, dx, dy;

                for (dy = -1; dy <= 1; dy++) {
                    for (dx = -1; dx <= 1; dx++) {
                        float value;

                        if (x + dx < 0 || x + dx >= mw || y + dy < 0 || y + dy >= mh)
                            continue;
                        value = from[(y + dy) * mw + x + dx];
                        total += value;
                        count++;
                        if (pass == 2 ? value > extreme : value < extreme)
                            extreme = value;
                    }
                }
                to[y * mw + x] = pass < 2 ? R_CamoStep(0.35f, 0.65f, total / (float)count) : extreme;
            }
        }
        from = to;
        to = to == mask ? scratch : mask;
    }
    R_CamoDropSpecks(mask, mw, mh, queue);
    for (i = 0; i < mw * mh; i++) {
        if (mask[i] > 0.5f) {
            int bin = (int)(luma[i] * 64.0f);

            histogram[bin < 0 ? 0 : bin > 63 ? 63 : bin]++;
            painted++;
        }
    }
    if (!painted)
        return 0.0f;
    for (i = 0, x = 0; i < 64; i++) {
        x += histogram[i];
        if (x * 2 >= painted)
            return ((float)i + 0.5f) / 64.0f;
    }
    return 1.0f;
}

static const GfxImage *R_CamoColorMap(const Material *material)
{
    int i;

    for (i = 0; material->textures && i < material->textureCount; i++) {
        const MaterialTextureDef *def = &material->textures[i];

        if (def->semantic == 2 && def->name && !strcmp(def->name, "colorMap"))
            return def->u.image;
    }
    return NULL;
}

static CamoMask *R_CamoGunMask(const Material *material, int style)
{
    CamoMask *entry;
    const GfxImage *colorMap;
    byte *rgba, *bgra;
    float *mask, *scratch, *luma;
    int width = 0, height = 0, mw, mh, i;

    for (i = 0; i < s_camoMaskCount; i++) {
        if (s_camoMasks[i].material == material && s_camoMasks[i].style == style)
            return s_camoMasks[i].state > 0 ? &s_camoMasks[i] : NULL;
    }
    if (s_camoMaskCount == R_CAMO_MASKS)
        return NULL;
    entry = &s_camoMasks[s_camoMaskCount++];
    memset(entry, 0, sizeof(*entry));
    entry->material = material;
    entry->style = style;
    entry->state = -1;
    colorMap = R_CamoColorMap(material);
    rgba = colorMap ? R_CamoDecode(colorMap->name, &width, &height) : NULL;
    if (!rgba)
        return NULL;
    mw = width / R_CAMO_MASK_BLOCK;
    mh = height / R_CAMO_MASK_BLOCK;
    mask = mw > 0 && mh > 0 ? (float *)malloc((sizeof(float) * 3 + sizeof(int)) * (size_t)mw * mh) : NULL;
    bgra = mask ? (byte *)malloc((size_t)mw * mh * 4) : NULL;
    if (bgra) {
        scratch = mask + mw * mh;
        luma = scratch + mw * mh;
        entry->reference = R_CamoMask(rgba, width, height, R_CAMO_MASK_BLOCK, style, R_CAMO_MAP_WARMTH, mask, scratch,
                                      luma, (int *)(luma + mw * mh));
        for (i = 0; i < mw * mh; i++) {
            byte value = (byte)(mask[i] * 255.0f + 0.5f);

            bgra[i * 4] = bgra[i * 4 + 1] = bgra[i * 4 + 2] = value;
            bgra[i * 4 + 3] = 255;
        }
        snprintf(entry->name, sizeof(entry->name), "*weapon_camo_mask%d", s_camoMaskCount - 1);
        if (entry->reference > 0.0f && R_CamoUpload(&entry->image, entry->name, bgra, mw, mh))
            entry->state = 1;
    }
    free(bgra);
    free(mask);
    free(rgba);
    return entry->state > 0 ? entry : NULL;
}

/* A material of the object named name, at any level of detail. */
static const Material *R_CamoFindMaterial(const struct DObj_s *obj, const char *name)
{
    int models = DObjGetNumModels(obj), m, lod, i;

    for (m = 0; m < models; m++) {
        const XModel *model = DObjGetModel(obj, m);
        const Material *const *const *skins = model ? (const Material *const *const *)XModelGetSkins(model) : NULL;
        int lods = skins ? XModelGetNumLods(model) : 0;

        for (lod = 0; lod < lods; lod++) {
            struct XSurface_s **surfaces = NULL;
            int *partBits = NULL;
            int count = XModelGetSurfaces(model, &surfaces, lod, &partBits);

            for (i = 0; skins[lod] && i < count; i++) {
                const Material *material = skins[lod][i];

                if (material && material->info.name && !strcmp(material->info.name, name))
                    return material;
            }
        }
    }
    return NULL;
}

/* The frame entity ent is, or -1. */
static int R_CamoEntityIndex(const GfxEntity *ent)
{
    const GfxBackEndData *data = frontEndDataOut;
    uintptr_t offset;

    if (!ent || !data || (uintptr_t)ent < (uintptr_t)data->entities)
        return -1;
    offset = (uintptr_t)ent - (uintptr_t)data->entities;
    if (offset % sizeof(GfxEntity) || offset / sizeof(GfxEntity) >= (uintptr_t)R_CAMO_ENTITY_COUNT)
        return -1;
    return (int)(offset / sizeof(GfxEntity));
}

void R_SetEntityWeaponCamo(GfxEntity *ent, const struct DObj_s *obj, const char *materialName, int camo, int style)
{
    int index = R_CamoEntityIndex(ent);
    const Material *material;
    CamoPattern *pattern;
    CamoMask *mask;
    CamoTag *tag;

    if (index < 0 || !obj || !materialName || s_camoTagOfEntity[index] || s_camoTagCount == R_WEAPON_CAMO_TAGS)
        return;
    pattern = R_CamoPattern(camo);
    material = pattern ? R_CamoFindMaterial(obj, materialName) : NULL;
    mask = material ? R_CamoGunMask(material, style) : NULL;
    if (!mask)
        return;
    tag = &s_camoTags[s_camoTagCount++];
    tag->entity = index;
    tag->material = material;
    tag->pattern = pattern;
    tag->mask = mask;
    s_camoTagOfEntity[index] = (byte)s_camoTagCount;
}

int R_GetDrawWeaponCamo(const GfxEntity *ent, const Material *material, R_WeaponCamoDraw *draw)
{
    int index, slot;
    const CamoTag *tag;

    if (!s_camoTagCount || !material)
        return 0;
    index = R_CamoEntityIndex(ent);
    slot = index < 0 ? 0 : s_camoTagOfEntity[index];
    tag = slot ? &s_camoTags[slot - 1] : NULL;
    if (!tag || tag->material != material)
        return 0;
    draw->pattern = &tag->pattern->image;
    draw->mask = &tag->mask->image;
    draw->tiles = R_WEAPON_CAMO_TILES;
    draw->reference = tag->mask->reference;
    return 1;
}

void R_ClearWeaponCamoEntities(void)
{
    int i;

    for (i = 0; i < s_camoTagCount; i++)
        s_camoTagOfEntity[s_camoTags[i].entity] = 0;
    s_camoTagCount = 0;
}

/* Trims the transparent margin but one texel and packs the remaining rows
 * at the start, as the HUD's silhouettes are trimmed. */
static int R_CamoCrop(byte *rgba, int *width, int *height)
{
    int w = *width, h = *height, minX = w, minY = h, maxX = -1, maxY = -1, x, y;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (rgba[((size_t)y * w + x) * 4 + 3] <= 16)
                continue;
            minX = x < minX ? x : minX;
            maxX = x > maxX ? x : maxX;
            minY = y < minY ? y : minY;
            maxY = y > maxY ? y : maxY;
        }
    }
    if (maxX < 0)
        return 0;
    minX = minX > 0 ? minX - 1 : 0;
    minY = minY > 0 ? minY - 1 : 0;
    maxX = maxX < w - 1 ? maxX + 1 : maxX;
    maxY = maxY < h - 1 ? maxY + 1 : maxY;
    for (y = minY; y <= maxY; y++)
        memmove(rgba + (size_t)(y - minY) * (maxX - minX + 1) * 4, rgba + ((size_t)y * w + minX) * 4,
                (size_t)(maxX - minX + 1) * 4);
    *width = maxX - minX + 1;
    *height = maxY - minY + 1;
    return 1;
}

static float R_CamoSampleMask(const float *mask, int mw, int mh, float x, float y)
{
    int x0, y0, x1, y1;
    float fx, fy, top, bottom;

    x = x < 0.0f ? 0.0f : x > (float)(mw - 1) ? (float)(mw - 1) : x;
    y = y < 0.0f ? 0.0f : y > (float)(mh - 1) ? (float)(mh - 1) : y;
    x0 = (int)x;
    y0 = (int)y;
    x1 = x0 + 1 < mw ? x0 + 1 : x0;
    y1 = y0 + 1 < mh ? y0 + 1 : y0;
    fx = x - (float)x0;
    fy = y - (float)y0;
    top = mask[y0 * mw + x0] + (mask[y0 * mw + x1] - mask[y0 * mw + x0]) * fx;
    bottom = mask[y1 * mw + x0] + (mask[y1 * mw + x1] - mask[y1 * mw + x0]) * fx;
    return top + (bottom - top) * fy;
}

/* The pattern averaged over a 3 by 3 grid across a picture texel, which
 * spans several of its texels. */
static void R_CamoSamplePattern(const byte *pattern, float u, float v, float step, float *rgb)
{
    int i, j, c;

    rgb[0] = rgb[1] = rgb[2] = 0.0f;
    for (j = -1; j <= 1; j++) {
        for (i = -1; i <= 1; i++) {
            int x = (int)((u + (float)i * step / 3.0f) * (float)R_WEAPON_CAMO_SIZE) & (R_WEAPON_CAMO_SIZE - 1);
            int y = (int)((v + (float)j * step / 3.0f) * (float)R_WEAPON_CAMO_SIZE) & (R_WEAPON_CAMO_SIZE - 1);
            const byte *texel = pattern + ((size_t)y * R_WEAPON_CAMO_SIZE + x) * 4;

            for (c = 0; c < 3; c++)
                rgb[c] += (float)texel[2 - c] / (255.0f * 9.0f);
        }
    }
}

/* Halves R, G, B, A texels in place: each the mean of the 2 by 2 it covers,
 * colors weighted by their alpha. */
static void R_CamoHalve(byte *rgba, int *width, int *height)
{
    int w = *width, h = *height, nw = w > 1 ? w / 2 : 1, nh = h > 1 ? h / 2 : 1, x, y, c;

    for (y = 0; y < nh; y++) {
        for (x = 0; x < nw; x++) {
            int sx[2] = { x * 2 < w ? x * 2 : w - 1, x * 2 + 1 < w ? x * 2 + 1 : w - 1 };
            int sy[2] = { y * 2 < h ? y * 2 : h - 1, y * 2 + 1 < h ? y * 2 + 1 : h - 1 };
            int sum[3] = { 0, 0, 0 }, alpha = 0, i;

            for (i = 0; i < 4; i++) {
                const byte *texel = rgba + ((size_t)sy[i >> 1] * w + sx[i & 1]) * 4;

                for (c = 0; c < 3; c++)
                    sum[c] += texel[c] * texel[3];
                alpha += texel[3];
            }
            for (c = 0; c < 3; c++)
                rgba[((size_t)y * nw + x) * 4 + c] = (byte)(alpha ? (sum[c] + alpha / 2) / alpha : 0);
            rgba[((size_t)y * nw + x) * 4 + 3] = (byte)((alpha + 2) / 4);
        }
    }
    *width = nw;
    *height = nh;
}

/* Paint over a picture as the guns' shaders paint them: the pattern where
 * the mask is, shaded by each texel against the painted median. A
 * silhouette is white, and painted whole. */
static int R_CamoBuildPicture(CamoPicture *entry)
{
    const GfxImage *image = entry->source->textures[0].u.image;
    CamoPattern *pattern = R_CamoPattern(entry->camo);
    byte *rgba = NULL, *bgra = NULL;
    float *mask = NULL, reference = 1.0f, tile;
    int width = 0, height = 0, mw = 0, mh = 0, x, y, i, built = 0;

    if (pattern && image)
        rgba = R_CamoDecode(image->name, &width, &height);
    if (!rgba)
        return 0;
    while (width > R_CAMO_PICTURE_MAX || height > R_CAMO_PICTURE_MAX)
        R_CamoHalve(rgba, &width, &height);
    if (entry->silhouette) {
        for (i = 0; i < width * height; i++)
            rgba[i * 4] = rgba[i * 4 + 1] = rgba[i * 4 + 2] = 255;
        if (!R_CamoCrop(rgba, &width, &height))
            goto done;
    } else {
        mw = width / R_CAMO_PICTURE_BLOCK;
        mh = height / R_CAMO_PICTURE_BLOCK;
        mask = mw > 0 && mh > 0 ? (float *)malloc((sizeof(float) * 3 + sizeof(int)) * (size_t)mw * mh) : NULL;
        if (!mask)
            goto done;
        reference = R_CamoMask(rgba, width, height, R_CAMO_PICTURE_BLOCK, entry->style, R_CAMO_PICTURE_WARMTH, mask,
                               mask + mw * mh, mask + 2 * mw * mh, (int *)(mask + 3 * mw * mh));
        if (reference <= 0.0f)
            goto done;
    }
    bgra = (byte *)malloc((size_t)width * height * 4);
    if (!bgra)
        goto done;
    tile = (float)width / (entry->silhouette ? R_WEAPON_CAMO_SILHOUETTE_TILES : R_WEAPON_CAMO_PICTURE_TILES);
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            const byte *texel = rgba + ((size_t)y * width + x) * 4;
            byte *out = bgra + ((size_t)y * width + x) * 4;
            float base[3], camo[3], paint, wear;
            int c;

            for (c = 0; c < 3; c++)
                base[c] = (float)texel[c] / 255.0f;
            paint = mask ? R_CamoSampleMask(mask, mw, mh, ((float)x + 0.5f) / R_CAMO_PICTURE_BLOCK - 0.5f,
                                            ((float)y + 0.5f) / R_CAMO_PICTURE_BLOCK - 0.5f)
                         : 1.0f;
            R_CamoSamplePattern(pattern->texels, ((float)x + 0.5f) / tile, ((float)y + 0.5f) / tile, 1.0f / tile, camo);
            wear = (0.299f * base[0] + 0.587f * base[1] + 0.114f * base[2]) / reference;
            wear = 1.0f + R_CAMO_WEAR * ((wear < 0.5f ? 0.5f : wear > 1.5f ? 1.5f : wear) - 1.0f);
            for (c = 0; c < 3; c++) {
                float value = base[c] + (camo[c] * wear - base[c]) * paint;

                out[2 - c] = (byte)(value <= 0.0f ? 0.0f : value >= 1.0f ? 255.0f : value * 255.0f + 0.5f);
            }
            out[3] = texel[3];
        }
    }
    snprintf(entry->name, sizeof(entry->name), "*weapon_camo_picture%d", (int)(entry - s_camoPictures));
    entry->image.name = entry->name;
    Image_Generate2D(&entry->image, bgra, width, height, D3DFMT_A8R8G8B8);
    if (entry->image.texture.map &&
        R_CamoCopyRaw(&entry->material, &entry->texture, &entry->image, entry->name + 1, R_CAMO_PICTURE_SAMPLER)) {
        entry->aspect = (float)width / (float)height;
        built = 1;
    }
done:
    free(bgra);
    free(mask);
    free(rgba);
    return built;
}

MaterialHandle R_GetWeaponCamoPicture(MaterialHandle picture, int silhouette, int camo, int style, float *aspect)
{
    const Material *source = (const Material *)picture;
    const int frame = frontEndDataOut ? frontEndDataOut->frameCount : 0;
    CamoPicture *entry = NULL;
    int i;

    *aspect = 0.0f;
    if (!source || source == rgp.defaultMaterial || source->textureCount < 1 || !source->textures ||
        camo <= WEAPON_CAMO_NONE || camo >= WEAPON_CAMO_COUNT)
        return NULL;
    silhouette = silhouette != 0;
    style = silhouette ? WEAPON_CAMO_BODY : style;
    s_camoPictureClock++;
    for (i = 0; i < R_CAMO_PICTURES; i++) {
        CamoPicture *candidate = &s_camoPictures[i];

        if (candidate->state && candidate->source == source && candidate->silhouette == silhouette &&
            candidate->camo == camo && candidate->style == style) {
            candidate->used = s_camoPictureClock;
            candidate->frame = frame;
            *aspect = candidate->state > 0 ? candidate->aspect : 0.0f;
            return candidate->state > 0 ? &candidate->material : NULL;
        }
        if (!entry || candidate->used < entry->used)
            entry = candidate;
    }
    /* A picture drawn this frame stays until the frame has drawn it. */
    if (entry->state && entry->frame == frame)
        return NULL;
    R_CamoReleaseImage(&entry->image);
    memset(entry, 0, sizeof(*entry));
    entry->source = source;
    entry->silhouette = silhouette;
    entry->camo = camo;
    entry->style = style;
    entry->used = s_camoPictureClock;
    entry->frame = frame;
    entry->state = R_CamoBuildPicture(entry) ? 1 : -1;
    *aspect = entry->state > 0 ? entry->aspect : 0.0f;
    return entry->state > 0 ? &entry->material : NULL;
}

void R_ShutdownWeaponCamos(void)
{
    int i;

    R_ClearWeaponCamoEntities();
    for (i = 0; i < WEAPON_CAMO_COUNT; i++) {
        R_CamoReleaseImage(&s_camoPatterns[i].image);
        free(s_camoPatterns[i].texels);
    }
    for (i = 0; i < s_camoMaskCount; i++)
        R_CamoReleaseImage(&s_camoMasks[i].image);
    for (i = 0; i < R_CAMO_PICTURES; i++)
        R_CamoReleaseImage(&s_camoPictures[i].image);
    memset(s_camoPatterns, 0, sizeof(s_camoPatterns));
    memset(s_camoMasks, 0, sizeof(s_camoMasks));
    memset(s_camoPictures, 0, sizeof(s_camoPictures));
    s_camoMaskCount = 0;
    s_camoPictureClock = 0;
}
