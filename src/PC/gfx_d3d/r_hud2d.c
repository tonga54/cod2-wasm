#include "common_types.h"
#include "imports.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <string.h>

extern r_global_permanent_t rgp;
extern void Image_Generate2D(GfxImage *image, byte *pixels, int width, int height, int imageFormat);
extern void Image_Release(GfxImage *image);
extern void RB_UnbindImage(const GfxImage *image);
extern void ClearBounds(vec3_t mins, vec3_t maxs);
extern void ExpandBounds(vec3_t mins, vec3_t maxs, vec3_t boundsMin, vec3_t boundsMax);
extern int FS_ReadFile(const char *path, void **buffer);
extern void FS_FreeFile(void *buffer);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);

/* Linear filtering, no mipmaps, clamped in U and V (see RB_SetSampler). */
#define HUD_IMAGE_SAMPLER_STATE 0x32
#define IWI_VERSION 5
#define IWI_FORMAT_DXT3 0x0c
#define IWI_FORMAT_DXT5 0x0d
#define DXT_BLOCK_BYTES 16
/* Fainter texels do not count as part of a silhouette's shape. */
#define HUD_SILHOUETTE_ALPHA_MIN 16
/* The darkest parts of an emblem keep this much of their alpha, so the whole
 * shape stays faintly visible around its brighter design. */
#define HUD_EMBLEM_LUMA_FLOOR 96

static const char *const s_hudImageNames[R_HUD_IMAGE_COUNT] = {
    "*hud_minimap",
    "*hud_minimap_arrow",
    "*hud_minimap_dot",
    "*hud_name_font",
    "*hud_mantle_arrow",
};

static GfxImage s_hudImages[R_HUD_IMAGE_COUNT];
static MaterialTextureDef s_hudTextures[R_HUD_IMAGE_COUNT];
static Material s_hudMaterials[R_HUD_IMAGE_COUNT];

typedef struct HudSilhouette {
    char imageName[64];
    /* The generated image: "*" + imageName, then "_emblem" for an emblem. */
    char name[72];
    int emblem;
    /* NULL when the icon could not be decoded, so the file is not read again. */
    MaterialHandle material;
    float aspect;
    GfxImage image;
    MaterialTextureDef texture;
    Material copy;
} HudSilhouette;

static HudSilhouette s_silhouettes[R_HUD_SILHOUETTE_COUNT];
static int s_silhouetteCount;
static byte s_silhouettePixels[R_HUD_SILHOUETTE_MAX_SIZE * R_HUD_SILHOUETTE_MAX_SIZE * 4];

static void R_ReleaseImage(GfxImage *image)
{
    if (image->texture.map) {
        RB_UnbindImage(image);
        Image_Release(image);
    }
}

static void R_ReleaseHudImage(int slot)
{
    R_ReleaseImage(&s_hudImages[slot]);
    memset(&s_hudMaterials[slot], 0, sizeof(s_hudMaterials[slot]));
}

/* $raw is the alpha-blended 2D material; without it a copy would draw with
 * the wrong technique. */
static const Material *R_HudRawMaterial(void)
{
    const Material *raw = rgp.rawMaterial;

    if (!raw || raw == rgp.defaultMaterial || raw->textureCount != 1 || !raw->textures)
        return NULL;
    return raw;
}

static MaterialHandle R_CopyRawMaterial(const Material *raw, Material *copy, MaterialTextureDef *texture,
                                        GfxImage *image, const char *name)
{
    *texture = raw->textures[0];
    texture->samplerState = HUD_IMAGE_SAMPLER_STATE;
    texture->u.image = image;
    *copy = *raw;
    copy->info.name = name + 1;
    copy->textures = texture;
    return copy;
}

MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height)
{
    const Material *raw = R_HudRawMaterial();
    GfxImage *image;

    if (slot < 0 || slot >= R_HUD_IMAGE_COUNT || !bgra || width <= 0 || height <= 0 || !raw)
        return NULL;

    R_ReleaseHudImage(slot);
    image = &s_hudImages[slot];
    memset(image, 0, sizeof(*image));
    image->name = s_hudImageNames[slot];
    Image_Generate2D(image, (byte *)bgra, width, height, D3DFMT_A8R8G8B8);
    if (!image->texture.map)
        return NULL;
    return R_CopyRawMaterial(raw, &s_hudMaterials[slot], &s_hudTextures[slot], image, s_hudImageNames[slot]);
}

static void R_DecodeBlockAlpha(const byte *block, int format, byte *alpha)
{
    byte palette[8];
    unsigned long long bits = 0;
    int a0, a1, i;

    if (format == IWI_FORMAT_DXT3) {
        for (i = 0; i < 16; i++)
            alpha[i] = (byte)(((block[i >> 1] >> ((i & 1) * 4)) & 15) * 17);
        return;
    }

    a0 = block[0];
    a1 = block[1];
    palette[0] = (byte)a0;
    palette[1] = (byte)a1;
    if (a0 > a1) {
        for (i = 1; i < 7; i++)
            palette[i + 1] = (byte)(((7 - i) * a0 + i * a1) / 7);
    } else {
        for (i = 1; i < 5; i++)
            palette[i + 1] = (byte)(((5 - i) * a0 + i * a1) / 5);
        palette[6] = 0;
        palette[7] = 255;
    }
    for (i = 0; i < 6; i++)
        bits |= (unsigned long long)block[2 + i] << (8 * i);
    for (i = 0; i < 16; i++)
        alpha[i] = palette[(bits >> (3 * i)) & 7];
}

/* Rec. 709 luma of each texel of a DXT3 or DXT5 color block, which always
 * holds four colors. */
static void R_DecodeBlockLuma(const byte *block, byte *luma)
{
    int palette[4];
    unsigned int bits;
    int i;

    for (i = 0; i < 2; i++) {
        int color = block[8 + 2 * i] | (block[9 + 2 * i] << 8);
        int r = ((color >> 11) & 31) * 255 / 31;
        int g = ((color >> 5) & 63) * 255 / 63;
        int b = (color & 31) * 255 / 31;
        palette[i] = (r * 54 + g * 183 + b * 19) >> 8;
    }
    palette[2] = (2 * palette[0] + palette[1]) / 3;
    palette[3] = (palette[0] + 2 * palette[1]) / 3;
    bits = (unsigned int)block[12] | ((unsigned int)block[13] << 8) | ((unsigned int)block[14] << 16) |
           ((unsigned int)block[15] << 24);
    for (i = 0; i < 16; i++)
        luma[i] = (byte)palette[(bits >> (2 * i)) & 3];
}

/* luminance scales each texel's alpha by its brightness, from
 * HUD_EMBLEM_LUMA_FLOOR for black up to 255 for white. */
static int R_DecodeIcon(const byte *file, int fileLen, byte *bgra, int maxSize, int *width, int *height,
                        int luminance)
{
    const GfxImageFileHeader *header = (const GfxImageFileHeader *)file;
    const byte *block;
    int w, h, blocksW, blocksH, bx, by, i;

    if (!file || fileLen < (int)sizeof(*header) || memcmp(header->tag, "IWi", 3) || header->version != IWI_VERSION)
        return 0;
    if (header->format != IWI_FORMAT_DXT3 && header->format != IWI_FORMAT_DXT5)
        return 0;
    w = header->dimensions[0];
    h = header->dimensions[1];
    if (w < 1 || h < 1 || w > maxSize || h > maxSize)
        return 0;
    blocksW = (w + 3) / 4;
    blocksH = (h + 3) / 4;
    /* Mips are stored smallest first, so the top mip ends the file. */
    if (fileLen - (int)sizeof(*header) < blocksW * blocksH * DXT_BLOCK_BYTES)
        return 0;
    block = file + fileLen - blocksW * blocksH * DXT_BLOCK_BYTES;

    for (by = 0; by < blocksH; by++) {
        for (bx = 0; bx < blocksW; bx++, block += DXT_BLOCK_BYTES) {
            byte alpha[16];
            byte luma[16];

            R_DecodeBlockAlpha(block, header->format, alpha);
            if (luminance)
                R_DecodeBlockLuma(block, luma);
            for (i = 0; i < 16; i++) {
                int x = bx * 4 + (i & 3);
                int y = by * 4 + (i >> 2);
                byte *texel;

                if (x >= w || y >= h)
                    continue;
                texel = bgra + (y * w + x) * 4;
                texel[0] = texel[1] = texel[2] = 255;
                texel[3] = alpha[i];
                if (luminance)
                    texel[3] = (byte)(alpha[i] *
                                      (HUD_EMBLEM_LUMA_FLOOR + luma[i] * (255 - HUD_EMBLEM_LUMA_FLOOR) / 255) / 255);
            }
        }
    }
    *width = w;
    *height = h;
    return 1;
}

int R_DecodeIconAlpha(const byte *file, int fileLen, byte *bgra, int maxSize, int *width, int *height)
{
    return R_DecodeIcon(file, fileLen, bgra, maxSize, width, height, 0);
}

/* Trims the transparent margin but one texel and packs the remaining rows at
 * the start of bgra. Returns 0 when nothing is visible. */
static int R_CropToAlpha(byte *bgra, int width, int height, int *cropWidth, int *cropHeight)
{
    int minX = width, minY = height, maxX = -1, maxY = -1;
    int x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (bgra[(y * width + x) * 4 + 3] <= HUD_SILHOUETTE_ALPHA_MIN)
                continue;
            if (x < minX)
                minX = x;
            if (x > maxX)
                maxX = x;
            if (y < minY)
                minY = y;
            if (y > maxY)
                maxY = y;
        }
    }
    if (maxX < 0)
        return 0;

    minX = minX > 0 ? minX - 1 : 0;
    minY = minY > 0 ? minY - 1 : 0;
    maxX = maxX < width - 1 ? maxX + 1 : maxX;
    maxY = maxY < height - 1 ? maxY + 1 : maxY;
    *cropWidth = maxX - minX + 1;
    *cropHeight = maxY - minY + 1;
    for (y = 0; y < *cropHeight; y++)
        memmove(bgra + y * *cropWidth * 4, bgra + ((minY + y) * width + minX) * 4, *cropWidth * 4);
    return 1;
}

static MaterialHandle R_BuildHudSilhouette(HudSilhouette *entry, const Material *raw)
{
    char path[96];
    void *file;
    int fileLen, width, height, decoded;

    if (Com_sprintf(path, sizeof(path), "images/%s.iwi", entry->imageName) < 0)
        return NULL;
    fileLen = FS_ReadFile(path, &file);
    if (fileLen < 0)
        return NULL;
    decoded = R_DecodeIcon((const byte *)file, fileLen, s_silhouettePixels, R_HUD_SILHOUETTE_MAX_SIZE, &width, &height,
                           entry->emblem);
    FS_FreeFile(file);
    /* An emblem keeps its whole frame, so it stays centred where it is drawn. */
    if (!decoded || (!entry->emblem && !R_CropToAlpha(s_silhouettePixels, width, height, &width, &height)))
        return NULL;

    entry->aspect = (float)width / (float)height;
    entry->image.name = entry->name;
    Image_Generate2D(&entry->image, s_silhouettePixels, width, height, D3DFMT_A8R8G8B8);
    if (!entry->image.texture.map)
        return NULL;
    return R_CopyRawMaterial(raw, &entry->copy, &entry->texture, &entry->image, entry->name);
}

static MaterialHandle R_HudSilhouetteResult(const HudSilhouette *entry, float *aspect)
{
    *aspect = entry->material ? entry->aspect : 0.0f;
    return entry->material;
}

static MaterialHandle R_GetHudIconCopy(MaterialHandle icon, int emblem, float *aspect)
{
    const Material *material = (const Material *)icon;
    const Material *raw = R_HudRawMaterial();
    const GfxImage *image;
    HudSilhouette *entry;
    int i;

    *aspect = 0.0f;
    if (!material || material == rgp.defaultMaterial || !raw || material->textureCount < 1 || !material->textures)
        return NULL;
    image = material->textures[0].u.image;
    if (!image || !image->name || strlen(image->name) >= sizeof(entry->imageName))
        return NULL;

    for (i = 0; i < s_silhouetteCount; i++) {
        if (s_silhouettes[i].emblem == emblem && !strcmp(s_silhouettes[i].imageName, image->name))
            return R_HudSilhouetteResult(&s_silhouettes[i], aspect);
    }
    if (s_silhouetteCount == R_HUD_SILHOUETTE_COUNT)
        return NULL;

    entry = &s_silhouettes[s_silhouetteCount++];
    memset(entry, 0, sizeof(*entry));
    strcpy(entry->imageName, image->name);
    entry->emblem = emblem;
    entry->name[0] = '*';
    strcpy(entry->name + 1, image->name);
    if (emblem)
        strcat(entry->name, "_emblem");
    entry->material = R_BuildHudSilhouette(entry, raw);
    return R_HudSilhouetteResult(entry, aspect);
}

MaterialHandle R_GetHudSilhouetteMaterial(MaterialHandle icon, float *aspect)
{
    return R_GetHudIconCopy(icon, 0, aspect);
}

MaterialHandle R_GetHudEmblemMaterial(MaterialHandle icon)
{
    float aspect;

    return R_GetHudIconCopy(icon, 1, &aspect);
}

void R_ShutdownHudImages(void)
{
    int slot;

    for (slot = 0; slot < R_HUD_IMAGE_COUNT; slot++)
        R_ReleaseHudImage(slot);
    for (slot = 0; slot < s_silhouetteCount; slot++)
        R_ReleaseImage(&s_silhouettes[slot].image);
    memset(s_silhouettes, 0, sizeof(s_silhouettes));
    s_silhouetteCount = 0;
}

int R_GetWorldDrawBounds(vec_t *mins, vec_t *maxs)
{
    const GfxWorld *world = rgp.world;
    int i;

    ClearBounds(mins, maxs);
    if (!world)
        return 0;

    for (i = 0; i < world->surfaceCount; i++) {
        const GfxSurface *surf = &world->surfaces[i];

        if (!surf->tris || !surf->material || (surf->material->info.gameFlags & 8))
            continue;
        ExpandBounds(surf->tris->bounds[0], surf->tris->bounds[1], mins, maxs);
    }
    return mins[0] <= maxs[0] && mins[1] <= maxs[1] && mins[2] <= maxs[2];
}
