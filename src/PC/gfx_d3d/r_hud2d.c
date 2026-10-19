#include "common_types.h"
#include "imports.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern r_global_permanent_t rgp;
extern void Image_Generate2D(GfxImage *image, byte *pixels, int width, int height, int imageFormat);
extern void Image_Setup(GfxImage *image, int width, int height, int depth, int imageFlags, DWORD usage,
                        D3DFORMAT imageFormat);
extern void Image_UploadData(GfxImage *image, D3DFORMAT format, int face, int mipLevel, const byte *src);
extern void Image_Release(GfxImage *image);
extern void RB_UnbindImage(const GfxImage *image);
extern void ClearBounds(vec3_t mins, vec3_t maxs);
extern void ExpandBounds(vec3_t mins, vec3_t maxs, vec3_t boundsMin, vec3_t boundsMax);
extern int FS_ReadFile(const char *path, void **buffer);
extern void FS_FreeFile(void *buffer);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);

/* Linear filtering, no mipmaps, clamped in U and V (see RB_SetSampler). */
#define HUD_IMAGE_SAMPLER_STATE 0x32
/* Linear filtering between linear mipmaps, clamped in U and V. */
#define HUD_PICTURE_SAMPLER_STATE 0x3a
#define IWI_VERSION 5
#define IWI_FORMAT_ARGB32 0x01
/* The image keeps its size whatever the picmip; its mips are in the file. */
#define IWI_FLAG_NOPICMIP 0x01
#define IWI_FORMAT_DXT1 0x0b
#define IWI_FORMAT_DXT3 0x0c
#define IWI_FORMAT_DXT5 0x0d
#define DXT_BLOCK_BYTES 16
#define DXT1_BLOCK_BYTES 8
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
    "*hud_grenade_ring",
    "*hud_grenade_pointer",
    "*hud_headshot",
    "*hud_team_emblems",
    "*hud_killcam",
    "*hud_endgame",
    "*menu_backdrop",
    "*menu_shade",
    "*menu_pill",
    "*menu_cursor",
    "*hud_prematch",
    "*menu_arrow",
    "*menu_roundel",
    "*hud_minimap_overview",
    "*menu_pause_emblems",
    "*hud_scope_lens0",
    "*hud_scope_lens1",
    "*hud_scope_lens2",
    "*hud_scope_lens3",
    "*hud_squad_emblems",
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
static int s_hudImageGeneration;
static byte s_silhouettePixels[R_HUD_SILHOUETTE_MAX_SIZE * R_HUD_SILHOUETTE_MAX_SIZE * 4];

typedef struct HudPicture {
    /* "*" + the image's name. */
    char name[72];
    /* NULL when there is no such picture, so the file is not read again. */
    MaterialHandle material;
    float st[4];
    float aspect;
    GfxImage image;
    MaterialTextureDef texture;
    Material copy;
} HudPicture;

static HudPicture s_pictures[R_HUD_PICTURE_COUNT];
static int s_pictureCount;

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

/* Rec. 709 luma of each texel of a color block. A DXT1 block whose first
 * color is not above its second holds three colors and black; a DXT3 or DXT5
 * block always holds four. */
static void R_DecodeColorLuma(const byte *color, int dxt1, byte *luma)
{
    int palette[4];
    int endpoints[2];
    unsigned int bits;
    int i;

    for (i = 0; i < 2; i++) {
        int r, g, b;

        endpoints[i] = color[2 * i] | (color[2 * i + 1] << 8);
        r = ((endpoints[i] >> 11) & 31) * 255 / 31;
        g = ((endpoints[i] >> 5) & 63) * 255 / 63;
        b = (endpoints[i] & 31) * 255 / 31;
        palette[i] = (r * 54 + g * 183 + b * 19) >> 8;
    }
    if (!dxt1 || endpoints[0] > endpoints[1]) {
        palette[2] = (2 * palette[0] + palette[1]) / 3;
        palette[3] = (palette[0] + 2 * palette[1]) / 3;
    } else {
        palette[2] = (palette[0] + palette[1]) / 2;
        palette[3] = 0;
    }
    bits = (unsigned int)color[4] | ((unsigned int)color[5] << 8) | ((unsigned int)color[6] << 16) |
           ((unsigned int)color[7] << 24);
    for (i = 0; i < 16; i++)
        luma[i] = (byte)palette[(bits >> (2 * i)) & 3];
}

static void R_DecodeBlockLuma(const byte *block, byte *luma)
{
    R_DecodeColorLuma(block + 8, 0, luma);
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

/* The bytes of a DXT1, DXT3 or DXT5 block, or 0 for another format. */
static int R_GrayBlockBytes(int format)
{
    if (format == IWI_FORMAT_DXT1)
        return DXT1_BLOCK_BYTES;
    if (format == IWI_FORMAT_DXT3 || format == IWI_FORMAT_DXT5)
        return DXT_BLOCK_BYTES;
    return 0;
}

int R_GetGrayImageSize(const byte *file, int fileLen, int *width, int *height)
{
    const GfxImageFileHeader *header = (const GfxImageFileHeader *)file;
    int blockBytes, w, h;

    if (!file || fileLen < (int)sizeof(*header) || memcmp(header->tag, "IWi", 3))
        return 0;
    if (header->version != IWI_VERSION)
        return 0;
    blockBytes = R_GrayBlockBytes(header->format);
    w = header->dimensions[0];
    h = header->dimensions[1];
    if (!blockBytes || w < 1 || h < 1 || w > R_HUD_GRAY_MAX_SIZE || h > R_HUD_GRAY_MAX_SIZE)
        return 0;
    /* Mips are stored smallest first, so the top mip ends the file. */
    if (fileLen - (int)sizeof(*header) < ((w + 3) / 4) * ((h + 3) / 4) * blockBytes)
        return 0;
    *width = w;
    *height = h;
    return 1;
}

int R_DecodeGrayImage(const byte *file, int fileLen, byte *bgra, const byte *tone)
{
    const GfxImageFileHeader *header = (const GfxImageFileHeader *)file;
    const byte *block;
    int w, h, blockBytes, blocksW, blocksH, bx, by, i;

    if (!tone || !R_GetGrayImageSize(file, fileLen, &w, &h))
        return 0;
    blockBytes = R_GrayBlockBytes(header->format);
    blocksW = (w + 3) / 4;
    blocksH = (h + 3) / 4;
    block = file + fileLen - blocksW * blocksH * blockBytes;

    for (by = 0; by < blocksH; by++) {
        for (bx = 0; bx < blocksW; bx++, block += blockBytes) {
            byte luma[16];

            R_DecodeColorLuma(blockBytes == DXT1_BLOCK_BYTES ? block : block + 8, blockBytes == DXT1_BLOCK_BYTES,
                              luma);
            for (i = 0; i < 16; i++) {
                int x = bx * 4 + (i & 3);
                int y = by * 4 + (i >> 2);

                if (x < w && y < h) {
                    byte *texel = bgra + (y * w + x) * 4;

                    texel[0] = texel[1] = texel[2] = tone[luma[i]];
                    texel[3] = 255;
                }
            }
        }
    }
    return 1;
}

MaterialHandle R_CreateHudGrayMaterial(int slot, MaterialHandle source, const byte *tone, float *aspect)
{
    const Material *material = (const Material *)source;
    const GfxImage *image;
    MaterialHandle gray = NULL;
    char path[96];
    void *file;
    byte *bgra;
    int fileLen, width, height;

    *aspect = 0.0f;
    if (!material || material == rgp.defaultMaterial || material->textureCount < 1 || !material->textures)
        return NULL;
    image = material->textures[0].u.image;
    if (!image || !image->name || Com_sprintf(path, sizeof(path), "images/%s.iwi", image->name) < 0)
        return NULL;
    fileLen = FS_ReadFile(path, &file);
    if (fileLen < 0)
        return NULL;
    if (R_GetGrayImageSize((const byte *)file, fileLen, &width, &height)) {
        bgra = (byte *)malloc((size_t)width * height * 4);
        if (bgra && R_DecodeGrayImage((const byte *)file, fileLen, bgra, tone))
            gray = R_CreateHudImageMaterial(slot, bgra, width, height);
        free(bgra);
    }
    FS_FreeFile(file);
    if (gray)
        *aspect = (float)width / (float)height;
    return gray;
}

/* Each texel of a color block as B, G, R. A DXT1 block whose first color is
 * not above its second holds three colors and transparent black. */
static void R_DecodeColorBlock(const byte *color, int dxt1, byte (*bgr)[3], byte *alpha)
{
    int palette[4][3];
    int endpoints[2];
    unsigned int bits;
    int i, c;

    for (i = 0; i < 2; i++) {
        endpoints[i] = color[2 * i] | (color[2 * i + 1] << 8);
        palette[i][0] = (endpoints[i] & 31) * 255 / 31;
        palette[i][1] = ((endpoints[i] >> 5) & 63) * 255 / 63;
        palette[i][2] = ((endpoints[i] >> 11) & 31) * 255 / 31;
    }
    for (c = 0; c < 3; c++) {
        if (!dxt1 || endpoints[0] > endpoints[1]) {
            palette[2][c] = (2 * palette[0][c] + palette[1][c]) / 3;
            palette[3][c] = (palette[0][c] + 2 * palette[1][c]) / 3;
        } else {
            palette[2][c] = (palette[0][c] + palette[1][c]) / 2;
            palette[3][c] = 0;
        }
    }
    bits = (unsigned int)color[4] | ((unsigned int)color[5] << 8) | ((unsigned int)color[6] << 16) |
           ((unsigned int)color[7] << 24);
    for (i = 0; i < 16; i++) {
        int index = (bits >> (2 * i)) & 3;

        for (c = 0; c < 3; c++)
            bgr[i][c] = (byte)palette[index][c];
        if (dxt1)
            alpha[i] = endpoints[0] <= endpoints[1] && index == 3 ? 0 : 255;
    }
}

int R_DecodeLensImage(const byte *file, int fileLen, byte *bgra, int *width, int *height)
{
    const GfxImageFileHeader *header = (const GfxImageFileHeader *)file;
    const byte *block;
    byte *full;
    int w, h, blockBytes, blocksW, blocksH, bx, by, i, shift, outW, outH, x, y;

    if (!R_GetGrayImageSize(file, fileLen, &w, &h))
        return 0;
    blockBytes = R_GrayBlockBytes(header->format);
    blocksW = (w + 3) / 4;
    blocksH = (h + 3) / 4;
    block = file + fileLen - blocksW * blocksH * blockBytes;
    full = (byte *)malloc((size_t)w * h * 4);
    if (!full)
        return 0;

    for (by = 0; by < blocksH; by++) {
        for (bx = 0; bx < blocksW; bx++, block += blockBytes) {
            byte bgr[16][3];
            byte alpha[16];

            if (blockBytes == DXT1_BLOCK_BYTES) {
                R_DecodeColorBlock(block, 1, bgr, alpha);
            } else {
                R_DecodeBlockAlpha(block, header->format, alpha);
                R_DecodeColorBlock(block + 8, 0, bgr, alpha);
            }
            for (i = 0; i < 16; i++) {
                x = bx * 4 + (i & 3);
                y = by * 4 + (i >> 2);
                if (x < w && y < h) {
                    byte *texel = full + (y * w + x) * 4;

                    texel[0] = bgr[i][0];
                    texel[1] = bgr[i][1];
                    texel[2] = bgr[i][2];
                    texel[3] = alpha[i];
                }
            }
        }
    }

    /* Each halving averages 2x2 texels, their colors weighted by alpha so the
     * reticle's edges do not take on the color of clear texels. */
    for (shift = 0; (w >> shift) > R_HUD_LENS_MAX_SIZE || (h >> shift) > R_HUD_LENS_MAX_SIZE; shift++)
        ;
    outW = w >> shift > 0 ? w >> shift : 1;
    outH = h >> shift > 0 ? h >> shift : 1;
    for (y = 0; y < outH; y++) {
        for (x = 0; x < outW; x++) {
            unsigned int sum[4] = { 0, 0, 0, 0 }, plain[3] = { 0, 0, 0 };
            int n = 0, sx, sy, c;
            byte *out = bgra + (y * outW + x) * 4;

            for (sy = y << shift; sy < ((y + 1) << shift) && sy < h; sy++) {
                for (sx = x << shift; sx < ((x + 1) << shift) && sx < w; sx++) {
                    const byte *texel = full + (sy * w + sx) * 4;

                    for (c = 0; c < 3; c++) {
                        sum[c] += texel[c] * texel[3];
                        plain[c] += texel[c];
                    }
                    sum[3] += texel[3];
                    n++;
                }
            }
            for (c = 0; c < 3; c++)
                out[c] = (byte)(sum[3] ? sum[c] / sum[3] : plain[c] / (unsigned int)n);
            out[3] = (byte)(sum[3] / (unsigned int)n);
        }
    }
    free(full);
    *width = outW;
    *height = outH;
    return 1;
}

/* Fainter texels are still part of the clear lens or its darkening rim. */
#define LENS_OPAQUE_ALPHA 230
#define LENS_DIRECTIONS 16

float R_ClearOutsideLens(byte *bgra, int width, int height)
{
    float half = 0.5f * (float)(width < height ? width : height);
    float cx = 0.5f * (float)width, cy = 0.5f * (float)height;
    float edges[LENS_DIRECTIONS], edge;
    int found = 0, d, x, y;

    /* Between the axes, clear of a reticle's posts and wires. */
    for (d = 0; d < LENS_DIRECTIONS; d++) {
        float angle = ((float)d + 0.5f) * (6.2831853f / LENS_DIRECTIONS);
        float dx = cosf(angle), dy = sinf(angle), r;

        for (r = 0.3f * half; r < half; r += 0.5f) {
            int tx = (int)floorf(cx + dx * r), ty = (int)floorf(cy + dy * r);

            if (tx < 0 || ty < 0 || tx >= width || ty >= height)
                break;
            if (bgra[(ty * width + tx) * 4 + 3] >= LENS_OPAQUE_ALPHA) {
                int k = found++;

                while (k > 0 && edges[k - 1] > r) {
                    edges[k] = edges[k - 1];
                    k--;
                }
                edges[k] = r;
                break;
            }
        }
    }
    edge = found ? edges[found / 2] : half;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            float fx = (float)x + 0.5f - cx, fy = (float)y + 0.5f - cy;
            float cover = edge + 0.5f - sqrtf(fx * fx + fy * fy);
            byte *alpha = bgra + (y * width + x) * 4 + 3;

            if (cover <= 0.0f)
                *alpha = 0;
            else if (cover < 1.0f)
                *alpha = (byte)((float)*alpha * cover + 0.5f);
        }
    }
    return edge / half;
}

MaterialHandle R_CreateHudLensMaterial(int slot, MaterialHandle overlay, float *edge)
{
    const Material *material = (const Material *)overlay;
    const GfxImage *image;
    MaterialHandle lens = NULL;
    char path[96];
    void *file;
    byte *bgra;
    int fileLen, width, height;

    *edge = 0.0f;
    if (!material || material == rgp.defaultMaterial || material->textureCount < 1 || !material->textures)
        return NULL;
    image = material->textures[0].u.image;
    if (!image || !image->name || Com_sprintf(path, sizeof(path), "images/%s.iwi", image->name) < 0)
        return NULL;
    fileLen = FS_ReadFile(path, &file);
    if (fileLen < 0)
        return NULL;
    bgra = (byte *)malloc((size_t)R_HUD_LENS_MAX_SIZE * R_HUD_LENS_MAX_SIZE * 4);
    if (bgra && R_DecodeLensImage((const byte *)file, fileLen, bgra, &width, &height)) {
        float radius = R_ClearOutsideLens(bgra, width, height);

        lens = R_CreateHudImageMaterial(slot, bgra, width, height);
        if (lens)
            *edge = radius;
    }
    free(bgra);
    FS_FreeFile(file);
    return lens;
}

/* The texels above HUD_SILHOUETTE_ALPHA_MIN and one more around them, as
 * bounds[] = { minX, minY, maxX, maxY }. Returns 0 when nothing is visible. */
static int R_AlphaBounds(const byte *bgra, int width, int height, int *bounds)
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

    bounds[0] = minX > 0 ? minX - 1 : 0;
    bounds[1] = minY > 0 ? minY - 1 : 0;
    bounds[2] = maxX < width - 1 ? maxX + 1 : maxX;
    bounds[3] = maxY < height - 1 ? maxY + 1 : maxY;
    return 1;
}

/* Trims the transparent margin but one texel and packs the remaining rows at
 * the start of bgra. Returns 0 when nothing is visible. */
static int R_CropToAlpha(byte *bgra, int width, int height, int *cropWidth, int *cropHeight)
{
    int bounds[4];
    int y;

    if (!R_AlphaBounds(bgra, width, height, bounds))
        return 0;
    *cropWidth = bounds[2] - bounds[0] + 1;
    *cropHeight = bounds[3] - bounds[1] + 1;
    for (y = 0; y < *cropHeight; y++)
        memmove(bgra + y * *cropWidth * 4, bgra + ((bounds[1] + y) * width + bounds[0]) * 4, *cropWidth * 4);
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

static int R_MipSize(int size, int level)
{
    return size >> level > 0 ? size >> level : 1;
}

/* Mips down to 1 x 1, as D3D makes them for a texture of powers of two. */
static int R_PictureLevels(int width, int height)
{
    int levels = 1;

    while (R_MipSize(width, levels - 1) > 1 || R_MipSize(height, levels - 1) > 1)
        levels++;
    return levels;
}

int R_GetPictureSize(const byte *file, int fileLen, int *width, int *height)
{
    const GfxImageFileHeader *header = (const GfxImageFileHeader *)file;
    int w, h, level, levels, size;

    if (!file || fileLen < (int)sizeof(*header) || memcmp(header->tag, "IWi", 3))
        return 0;
    if (header->version != IWI_VERSION || header->format != IWI_FORMAT_ARGB32 || header->flags != IWI_FLAG_NOPICMIP ||
        header->dimensions[2] != 1)
        return 0;
    w = header->dimensions[0];
    h = header->dimensions[1];
    if (w < 1 || h < 1 || w > R_HUD_PICTURE_MAX_SIZE || h > R_HUD_PICTURE_MAX_SIZE || (w & (w - 1)) || (h & (h - 1)))
        return 0;
    levels = R_PictureLevels(w, h);
    size = (int)sizeof(*header);
    for (level = 0; level < levels; level++)
        size += R_MipSize(w, level) * R_MipSize(h, level) * 4;
    if (fileLen != size)
        return 0;
    *width = w;
    *height = h;
    return 1;
}

static MaterialHandle R_BuildHudPicture(HudPicture *entry, const Material *raw)
{
    char path[96];
    void *file;
    const byte *mip;
    MaterialHandle material = NULL;
    int fileLen, width, height, level, bounds[4];

    if (Com_sprintf(path, sizeof(path), "images/%s.iwi", entry->name + 1) < 0)
        return NULL;
    fileLen = FS_ReadFile(path, &file);
    if (fileLen < 0)
        return NULL;
    /* Mips are stored smallest first, so the top mip ends the file. */
    if (R_GetPictureSize((const byte *)file, fileLen, &width, &height) &&
        R_AlphaBounds((const byte *)file + fileLen - width * height * 4, width, height, bounds)) {
        entry->image.name = entry->name;
        Image_Setup(&entry->image, width, height, 1, IWI_FLAG_NOPICMIP, 0, D3DFMT_A8R8G8B8);
        if (entry->image.texture.map) {
            mip = (const byte *)file + sizeof(GfxImageFileHeader);
            for (level = R_PictureLevels(width, height) - 1; level >= 0; level--) {
                Image_UploadData(&entry->image, D3DFMT_A8R8G8B8, 0, level, mip);
                mip += R_MipSize(width, level) * R_MipSize(height, level) * 4;
            }
            entry->st[0] = (float)bounds[0] / (float)width;
            entry->st[1] = (float)bounds[1] / (float)height;
            entry->st[2] = (float)(bounds[2] + 1) / (float)width;
            entry->st[3] = (float)(bounds[3] + 1) / (float)height;
            entry->aspect = (float)(bounds[2] - bounds[0] + 1) / (float)(bounds[3] - bounds[1] + 1);
            material = R_CopyRawMaterial(raw, &entry->copy, &entry->texture, &entry->image, entry->name);
            entry->texture.samplerState = HUD_PICTURE_SAMPLER_STATE;
        }
    }
    FS_FreeFile(file);
    return material;
}

MaterialHandle R_GetHudPictureMaterial(const char *name, float *st, float *aspect)
{
    const Material *raw = R_HudRawMaterial();
    HudPicture *entry;
    int i;

    *aspect = 0.0f;
    if (!name || !raw || strlen(name) + 1 >= sizeof(entry->name))
        return NULL;
    for (i = 0; i < s_pictureCount; i++) {
        if (!strcmp(s_pictures[i].name + 1, name))
            break;
    }
    if (i == s_pictureCount) {
        if (s_pictureCount == R_HUD_PICTURE_COUNT)
            return NULL;
        entry = &s_pictures[s_pictureCount++];
        memset(entry, 0, sizeof(*entry));
        entry->name[0] = '*';
        strcpy(entry->name + 1, name);
        entry->material = R_BuildHudPicture(entry, raw);
    }
    entry = &s_pictures[i];
    if (!entry->material)
        return NULL;
    memcpy(st, entry->st, sizeof(entry->st));
    *aspect = entry->aspect;
    return entry->material;
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
    for (slot = 0; slot < s_pictureCount; slot++)
        R_ReleaseImage(&s_pictures[slot].image);
    memset(s_pictures, 0, sizeof(s_pictures));
    s_pictureCount = 0;
    s_hudImageGeneration++;
}

int R_GetHudImageGeneration(void)
{
    return s_hudImageGeneration;
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
