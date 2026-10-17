#include "common_types.h"
#include "imports.h"
#include "headers/PC/gfx_d3d/r_font_wwii.h"
#include "headers/PC/gfx_d3d/r_font_wwii_data.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../../third_party/stb/stb_truetype.h"

extern r_global_permanent_t rgp;
extern refimport_t ri;
extern vidConfig_t vidConfig;
extern int I_stricmp(const char *s1, const char *s2);
extern void Image_Setup(GfxImage *image, int width, int height, int depth, int imageFlags, DWORD usage, D3DFORMAT imageFormat);
extern D3DCUBEMAP_FACES Image_CubemapFace(int face);
extern void Image_UploadData(GfxImage *image, D3DFORMAT format, int face, int mipLevel, const byte *src);
extern void Image_Release(GfxImage *image);
extern void RB_UnbindImage(const GfxImage *image);

/* Letters 0x20 to 0x7f, which R_GetCharacterGlyph indexes directly, then the
 * rest of Windows-1252 ascending, which it searches. */
#define R_WWII_FONT_GLYPHS 219
/* Texels of the atlas to a font unit: the screen's lines over the 480 that UI
 * text is laid out for. The console draws its own font a unit to a pixel. */
#define R_WWII_FONT_TEXELS_MAX 3
/* Transparent texels inside each glyph's quad, and between glyphs so that
 * filtering and the first mipmaps never reach a neighbour. */
#define R_WWII_FONT_MARGIN 1
#define R_WWII_FONT_GAP 2
/* Power-of-two widths; heights in whole 4-texel rows. */
#define R_WWII_FONT_ATLAS_MIN 64
#define R_WWII_FONT_ATLAS_MAX 2048
#define R_WWII_FONT_ATLAS_ROW 4
/* No picmip, with mipmaps (see Image_Setup); linear filtering between
 * mipmaps, clamped in U and V (see RB_SetSampler). */
#define R_WWII_FONT_IMAGE_FLAGS 1
#define R_WWII_FONT_SAMPLER_STATE 0x3a
#define R_WWII_FONT_ATLASES 12
#define R_WWII_FONT_FACES 12
#define R_WWII_FONT_NAMES 16
#define R_WWII_FONT_NAME_LEN 64

enum {
    R_WWII_MEDIUM,
    R_WWII_SEMIBOLD,
    R_WWII_BOLD
};

typedef struct {
    /* The stock font's file in fonts/. */
    const char *file;
    int face;
    /* The stock font's units: its pixel height, the height of its "H", and
     * the units from the pen up to its baseline. */
    int pixelHeight;
    float capHeight;
    int baseline;
    /* Horizontal scale beyond the cap height's. */
    float width;
    /* Drawn a unit to a pixel when registered by its file, as the console
     * draws its own. */
    int rawPixels;
} R_WwiiFontProfile;

/* The UI picks small, normal, big or extra big by the text's height on the
 * screen, so they share the captures' SemiBold, at their width: 95% of
 * Barlow's. Bold text is the scoreboard title's Bold. */
static const R_WwiiFontProfile s_wwiiFontProfiles[] = {
    { "smallFont", R_WWII_SEMIBOLD, 12, 7.0f, 2, 0.95f, 0 },
    { "normalFont", R_WWII_SEMIBOLD, 16, 11.0f, 2, 0.95f, 0 },
    { "bigFont", R_WWII_SEMIBOLD, 24, 13.12f, 5, 0.95f, 0 },
    { "extraBigFont", R_WWII_SEMIBOLD, 32, 21.0f, 5, 0.95f, 0 },
    { "boldFont", R_WWII_BOLD, 30, 17.1f, 6, 1.0f, 0 },
    { "consoleFont", R_WWII_MEDIUM, 16, 9.0f, 3, 1.0f, 1 },
    { "smallDevFont", R_WWII_SEMIBOLD, 16, 9.17f, 2, 0.95f, 0 },
    { "bigDevFont", R_WWII_SEMIBOLD, 24, 13.12f, 3, 0.95f, 0 },
    { "qerFont", R_WWII_SEMIBOLD, 12, 7.0f, 3, 0.95f, 0 },
};

static const unsigned char *const s_wwiiFontData[] = { r_wwiiFontMedium, r_wwiiFontSemiBold, r_wwiiFontBold };

/* Windows-1252 from 0x80; 0 where it defines no letter. */
static const unsigned short s_wwiiFontWindows1252[32] = {
    0x20ac, 0, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017d, 0,
    0, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0, 0x017e, 0x0178,
};

typedef struct {
    int glyph;
    /* stb_truetype's bitmap box, texels from the pen on the baseline. */
    int ix0, iy0, ix1, iy1;
    /* The glyph's quad in font units from the baseline, and its texels in
     * the atlas. */
    int x0, y0, width, height;
    int atlasX, atlasY;
} R_WwiiFontLayout;

/* The glyphs of one face at one size and density, which fonts that differ
 * only in their baseline share. Its glyphs sit on the baseline. */
typedef struct {
    int face;
    float capHeight;
    float width;
    int texelsPerUnit;
    int atlasWidth;
    int atlasHeight;
    char imageName[R_WWII_FONT_NAME_LEN];
    GfxImage image;
    MaterialTextureDef texture;
    Material material;
    Glyph glyphs[R_WWII_FONT_GLYPHS];
} R_WwiiFontAtlas;

typedef struct {
    const R_WwiiFontProfile *profile;
    R_WwiiFontAtlas *atlas;
    Glyph glyphs[R_WWII_FONT_GLYPHS];
} R_WwiiFontFace;

/* Each name registered gets its own Font, sharing its face's glyphs. */
typedef struct {
    Font font;
    char name[R_WWII_FONT_NAME_LEN];
} R_WwiiFontName;

static const dvar_t *r_wwiiFonts;
static R_WwiiFontAtlas s_wwiiFontAtlases[R_WWII_FONT_ATLASES];
static int s_wwiiFontAtlasCount;
static R_WwiiFontFace s_wwiiFontFaces[R_WWII_FONT_FACES];
static int s_wwiiFontFaceCount;
static R_WwiiFontName s_wwiiFontNames[R_WWII_FONT_NAMES];
static int s_wwiiFontNameCount;
static R_WwiiFontLayout s_wwiiFontLayout[R_WWII_FONT_GLYPHS];

static int R_WwiiFontFloorDiv(int value, int divisor)
{
    return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor);
}

static int R_WwiiFontCeilDiv(int value, int divisor)
{
    return -R_WwiiFontFloorDiv(-value, divisor);
}

/* "$font" is the normal font; "$x" and "fonts/x" are fonts/x. */
static const R_WwiiFontProfile *R_WwiiFontProfileFor(const char *fontName)
{
    const char *file;
    char prefix[7];
    int i;

    if (!I_stricmp(fontName, "$font")) {
        file = "normalFont";
    } else if (fontName[0] == '$') {
        file = fontName + 1;
    } else {
        if (strlen(fontName) <= 6)
            return NULL;
        memcpy(prefix, fontName, 6);
        prefix[6] = '\0';
        if (I_stricmp(prefix, "fonts/"))
            return NULL;
        file = fontName + 6;
    }
    for (i = 0; i < (int)(sizeof(s_wwiiFontProfiles) / sizeof(s_wwiiFontProfiles[0])); i++) {
        if (!I_stricmp(file, s_wwiiFontProfiles[i].file))
            return &s_wwiiFontProfiles[i];
    }
    return NULL;
}

/* The letter's Unicode code point, or -1 where Windows-1252 has none. DEL and
 * the no-break space draw as spaces. */
static int R_WwiiFontUnicode(int letter)
{
    if (letter == 0x7f || letter == 0xa0)
        return 0x20;
    if (letter >= 0x80 && letter < 0xa0)
        return s_wwiiFontWindows1252[letter - 0x80] ? s_wwiiFontWindows1252[letter - 0x80] : -1;
    return letter;
}

static int R_WwiiFontTexelsPerUnit(int rawPixels)
{
    int texels;

    if (rawPixels)
        return 1;
    texels = (int)floorf((float)vidConfig.height / 480.0f + 0.5f);
    if (texels < 1)
        return 1;
    return texels > R_WWII_FONT_TEXELS_MAX ? R_WWII_FONT_TEXELS_MAX : texels;
}

/* $raw is the alpha-blended 2D material the stock fonts' technique matches;
 * without it a copy would draw with the wrong technique. */
static const Material *R_WwiiFontRawMaterial(void)
{
    const Material *raw = rgp.rawMaterial;

    if (!raw || raw == rgp.defaultMaterial || raw->textureCount != 1 || !raw->textures)
        return NULL;
    return raw;
}

/* Glyph metrics and quads in font units from the baseline. Returns the glyph
 * count, or 0 when a glyph is missing or does not fit a Glyph. */
static int R_WwiiFontMeasure(R_WwiiFontAtlas *atlas, const stbtt_fontinfo *info, float scaleX, float scaleY, float advanceScale)
{
    int texels = atlas->texelsPerUnit;
    int count = 0;
    int letter;

    for (letter = 0x20; letter < 0x100; letter++) {
        int unicode = R_WwiiFontUnicode(letter);
        R_WwiiFontLayout *layout = &s_wwiiFontLayout[count];
        Glyph *glyph = &atlas->glyphs[count];
        int advance, bearing, dx, x1, y1;

        if (unicode < 0)
            continue;
        if (count == R_WWII_FONT_GLYPHS)
            return 0;
        memset(layout, 0, sizeof(*layout));
        memset(glyph, 0, sizeof(*glyph));
        layout->glyph = stbtt_FindGlyphIndex(info, unicode);
        if (!layout->glyph)
            return 0;
        stbtt_GetGlyphHMetrics(info, layout->glyph, &advance, &bearing);
        dx = (int)floorf((float)advance * advanceScale + 0.5f);
        if (dx < 0 || dx > 255)
            return 0;
        glyph->letter = (unsigned short)letter;
        glyph->dx = (byte)dx;
        stbtt_GetGlyphBitmapBox(info, layout->glyph, scaleX, scaleY, &layout->ix0, &layout->iy0, &layout->ix1,
                                &layout->iy1);
        if (!stbtt_IsGlyphEmpty(info, layout->glyph) && layout->ix1 > layout->ix0 && layout->iy1 > layout->iy0) {
            layout->x0 = R_WwiiFontFloorDiv(layout->ix0 - R_WWII_FONT_MARGIN, texels);
            layout->y0 = R_WwiiFontFloorDiv(layout->iy0 - R_WWII_FONT_MARGIN, texels);
            x1 = R_WwiiFontCeilDiv(layout->ix1 + R_WWII_FONT_MARGIN, texels);
            y1 = R_WwiiFontCeilDiv(layout->iy1 + R_WWII_FONT_MARGIN, texels);
            layout->width = x1 - layout->x0;
            layout->height = y1 - layout->y0;
            if (layout->x0 < -128 || layout->x0 > 127 || layout->y0 < -128 || layout->y0 > 127 ||
                layout->width > 255 || layout->height > 255)
                return 0;
            glyph->x0 = (signed char)layout->x0;
            glyph->y0 = (signed char)layout->y0;
            glyph->pixelWidth = (byte)layout->width;
            glyph->pixelHeight = (byte)layout->height;
        } else {
            layout->glyph = 0;
        }
        count++;
    }
    return count;
}

/* Shelves of glyphs, tallest first, in an atlas width texels wide. Returns
 * the height used. */
static int R_WwiiFontPack(const int *order, int count, int texels, int width)
{
    int x = R_WWII_FONT_GAP, y = R_WWII_FONT_GAP, shelf = 0;
    int i;

    for (i = 0; i < count; i++) {
        R_WwiiFontLayout *layout = &s_wwiiFontLayout[order[i]];
        int w = layout->width * texels, h = layout->height * texels;

        if (!layout->glyph)
            continue;
        if (w + 2 * R_WWII_FONT_GAP > width)
            return R_WWII_FONT_ATLAS_MAX + 1;
        if (x + w + R_WWII_FONT_GAP > width) {
            x = R_WWII_FONT_GAP;
            y += shelf + R_WWII_FONT_GAP;
            shelf = 0;
        }
        layout->atlasX = x;
        layout->atlasY = y;
        x += w + R_WWII_FONT_GAP;
        if (h > shelf)
            shelf = h;
    }
    return y + shelf + R_WWII_FONT_GAP;
}

/* The power-of-two width that wastes the fewest texels. */
static int R_WwiiFontLayoutAtlas(R_WwiiFontAtlas *atlas, int count)
{
    static int order[R_WWII_FONT_GLYPHS];
    int texels = atlas->texelsPerUnit;
    int width, best = 0, bestHeight = 0;
    int i, j;

    for (i = 0; i < count; i++) {
        const R_WwiiFontLayout *layout = &s_wwiiFontLayout[i];
        int key = layout->height * 256 + layout->width;

        for (j = i; j > 0; j--) {
            const R_WwiiFontLayout *other = &s_wwiiFontLayout[order[j - 1]];

            if (other->height * 256 + other->width >= key)
                break;
            order[j] = order[j - 1];
        }
        order[j] = i;
    }
    for (width = R_WWII_FONT_ATLAS_MIN; width <= R_WWII_FONT_ATLAS_MAX; width *= 2) {
        int height = R_WwiiFontPack(order, count, texels, width);

        height = (height + R_WWII_FONT_ATLAS_ROW - 1) / R_WWII_FONT_ATLAS_ROW * R_WWII_FONT_ATLAS_ROW;
        if (height > R_WWII_FONT_ATLAS_MAX)
            continue;
        if (!best || (long)width * height < (long)best * bestHeight) {
            best = width;
            bestHeight = height;
        }
    }
    if (!best)
        return 0;
    R_WwiiFontPack(order, count, texels, best);
    atlas->atlasWidth = best;
    atlas->atlasHeight = bestHeight;
    return 1;
}

/* A white image with the atlas's alpha, as luminance and alpha at half the
 * memory of BGRA, and its mipmaps averaged down to a texel, each side halved
 * and rounded down as the device sizes them. */
static int R_WwiiFontUpload(R_WwiiFontAtlas *atlas, byte *alpha)
{
    int width = atlas->atlasWidth, height = atlas->atlasHeight;
    byte *texels = (byte *)malloc((size_t)width * height * 2);
    int level = 0, i, x, y;

    if (!texels)
        return 0;
    memset(&atlas->image, 0, sizeof(atlas->image));
    atlas->image.name = atlas->imageName;
    Image_Setup(&atlas->image, width, height, 1, R_WWII_FONT_IMAGE_FLAGS, 0, D3DFMT_A8L8);
    if (!atlas->image.texture.map) {
        free(texels);
        return 0;
    }
    for (;;) {
        int nextWidth = width > 1 ? width / 2 : 1, nextHeight = height > 1 ? height / 2 : 1;

        for (i = 0; i < width * height; i++) {
            texels[i * 2] = 255;
            texels[i * 2 + 1] = alpha[i];
        }
        Image_UploadData(&atlas->image, D3DFMT_A8L8, (int)Image_CubemapFace(0), level, texels);
        if (width == 1 && height == 1)
            break;
        for (y = 0; y < nextHeight; y++) {
            for (x = 0; x < nextWidth; x++) {
                int x0 = width > 1 ? 2 * x : x, y0 = height > 1 ? 2 * y : y;
                int x1 = width > 1 ? x0 + 1 : x0, y1 = height > 1 ? y0 + 1 : y0;

                alpha[y * nextWidth + x] = (byte)((alpha[y0 * width + x0] + alpha[y0 * width + x1] +
                                                   alpha[y1 * width + x0] + alpha[y1 * width + x1] + 2) / 4);
            }
        }
        width = nextWidth;
        height = nextHeight;
        level++;
    }
    free(texels);
    return 1;
}

static int R_WwiiFontBuildAtlas(R_WwiiFontAtlas *atlas, const R_WwiiFontProfile *profile, int texels)
{
    const unsigned char *data = s_wwiiFontData[profile->face];
    const Material *raw = R_WwiiFontRawMaterial();
    stbtt_fontinfo info;
    float units, scaleX, scaleY;
    int boxX0, boxY0, boxX1, boxY1;
    int count, i;
    byte *alpha;

    memset(atlas, 0, sizeof(*atlas));
    atlas->face = profile->face;
    atlas->capHeight = profile->capHeight;
    atlas->width = profile->width;
    atlas->texelsPerUnit = texels;
    if (!raw || !stbtt_InitFont(&info, data, stbtt_GetFontOffsetForIndex(data, 0)) ||
        !stbtt_GetCodepointBox(&info, 'H', &boxX0, &boxY0, &boxX1, &boxY1) || boxY1 <= 0)
        return 0;
    units = profile->capHeight / (float)boxY1;
    scaleX = units * profile->width * (float)texels;
    scaleY = units * (float)texels;
    count = R_WwiiFontMeasure(atlas, &info, scaleX, scaleY, units * profile->width);
    if (count != R_WWII_FONT_GLYPHS || !R_WwiiFontLayoutAtlas(atlas, count))
        return 0;

    alpha = (byte *)calloc((size_t)atlas->atlasWidth * atlas->atlasHeight, 1);
    if (!alpha)
        return 0;
    for (i = 0; i < count; i++) {
        const R_WwiiFontLayout *layout = &s_wwiiFontLayout[i];
        Glyph *glyph = &atlas->glyphs[i];

        if (!layout->glyph)
            continue;
        stbtt_MakeGlyphBitmapSubpixel(&info,
                                      alpha + (layout->atlasY + layout->iy0 - layout->y0 * texels) * atlas->atlasWidth +
                                          layout->atlasX + layout->ix0 - layout->x0 * texels,
                                      layout->ix1 - layout->ix0, layout->iy1 - layout->iy0, atlas->atlasWidth, scaleX,
                                      scaleY, 0.0f, 0.0f, layout->glyph);
        glyph->s0 = (float)layout->atlasX / (float)atlas->atlasWidth;
        glyph->t0 = (float)layout->atlasY / (float)atlas->atlasHeight;
        glyph->s1 = (float)(layout->atlasX + layout->width * texels) / (float)atlas->atlasWidth;
        glyph->t1 = (float)(layout->atlasY + layout->height * texels) / (float)atlas->atlasHeight;
    }

    snprintf(atlas->imageName, sizeof(atlas->imageName), "*wwii_%s_%d", profile->file, texels);
    if (!R_WwiiFontUpload(atlas, alpha)) {
        free(alpha);
        return 0;
    }
    free(alpha);
    atlas->texture = raw->textures[0];
    atlas->texture.samplerState = R_WWII_FONT_SAMPLER_STATE;
    atlas->texture.u.image = &atlas->image;
    atlas->material = *raw;
    atlas->material.info.name = atlas->imageName + 1;
    atlas->material.textures = &atlas->texture;
    return 1;
}

static void R_WwiiFontReleaseAtlas(R_WwiiFontAtlas *atlas)
{
    if (atlas->image.texture.map) {
        RB_UnbindImage(&atlas->image);
        Image_Release(&atlas->image);
    }
    memset(atlas, 0, sizeof(*atlas));
}

static R_WwiiFontAtlas *R_WwiiFontAtlasFor(const R_WwiiFontProfile *profile, int texels)
{
    R_WwiiFontAtlas *atlas;
    int i;

    for (i = 0; i < s_wwiiFontAtlasCount; i++) {
        atlas = &s_wwiiFontAtlases[i];
        if (atlas->face == profile->face && atlas->capHeight == profile->capHeight &&
            atlas->width == profile->width && atlas->texelsPerUnit == texels)
            return atlas;
    }
    if (s_wwiiFontAtlasCount == R_WWII_FONT_ATLASES)
        return NULL;
    atlas = &s_wwiiFontAtlases[s_wwiiFontAtlasCount];
    if (!R_WwiiFontBuildAtlas(atlas, profile, texels)) {
        R_WwiiFontReleaseAtlas(atlas);
        return NULL;
    }
    s_wwiiFontAtlasCount++;
    return atlas;
}

/* The atlas's glyphs moved onto the profile's baseline. */
static R_WwiiFontFace *R_WwiiFontFaceFor(const R_WwiiFontProfile *profile, int texels)
{
    R_WwiiFontAtlas *atlas;
    R_WwiiFontFace *face;
    int i;

    for (i = 0; i < s_wwiiFontFaceCount; i++) {
        face = &s_wwiiFontFaces[i];
        if (face->profile == profile && face->atlas->texelsPerUnit == texels)
            return face;
    }
    if (s_wwiiFontFaceCount == R_WWII_FONT_FACES)
        return NULL;
    atlas = R_WwiiFontAtlasFor(profile, texels);
    if (!atlas)
        return NULL;
    face = &s_wwiiFontFaces[s_wwiiFontFaceCount];
    face->profile = profile;
    face->atlas = atlas;
    memcpy(face->glyphs, atlas->glyphs, sizeof(face->glyphs));
    for (i = 0; i < R_WWII_FONT_GLYPHS; i++) {
        int y0 = face->glyphs[i].y0 - profile->baseline;

        if (!face->glyphs[i].pixelHeight)
            continue;
        if (y0 < -128)
            return NULL;
        face->glyphs[i].y0 = (signed char)y0;
    }
    s_wwiiFontFaceCount++;
    return face;
}

FontHandle R_LoadWwiiFont(const char *fontName)
{
    const R_WwiiFontProfile *profile;
    R_WwiiFontFace *face;
    R_WwiiFontName *entry;
    int rawPixels;

    if (!r_wwiiFonts)
        r_wwiiFonts = ri.Dvar_RegisterBool("r_wwiiFonts", 1, 0x2021);
    if (!fontName || !r_wwiiFonts || !r_wwiiFonts->current.enabled || s_wwiiFontNameCount == R_WWII_FONT_NAMES)
        return NULL;
    profile = R_WwiiFontProfileFor(fontName);
    if (!profile)
        return NULL;
    rawPixels = profile->rawPixels && fontName[0] != '$';
    face = R_WwiiFontFaceFor(profile, R_WwiiFontTexelsPerUnit(rawPixels));
    if (!face)
        return NULL;

    entry = &s_wwiiFontNames[s_wwiiFontNameCount++];
    snprintf(entry->name, sizeof(entry->name), "%s", fontName);
    entry->font.name = entry->name;
    entry->font.pixelHeight = profile->pixelHeight;
    entry->font.glyphCount = R_WWII_FONT_GLYPHS;
    entry->font.material = &face->atlas->material;
    entry->font.glyphs = face->glyphs;
    return &entry->font;
}

void R_ShutdownWwiiFonts(void)
{
    int i;

    for (i = 0; i < s_wwiiFontAtlasCount; i++)
        R_WwiiFontReleaseAtlas(&s_wwiiFontAtlases[i]);
    s_wwiiFontAtlasCount = 0;
    s_wwiiFontFaceCount = 0;
    s_wwiiFontNameCount = 0;
    r_wwiiFonts = NULL;
}
