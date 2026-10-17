#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_playernames.h"
#include "headers/PC/cgame_mp/cg_namefont_data.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <stdlib.h>

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);

/* Capitals 7 units tall, like the stock teammate label's. */
#define NAME_SCALE 0.21f
#define NAME_ALIGN 7
/* The captures' backgrounds are darker than CoD2's walls and sand, and the
 * renderer has no style for the enemy capture's dark halo; the stock
 * one-pixel shadow keeps the letters legible. */
#define NAME_STYLE 3
#define NAME_LEN 32

enum {
    NAME_FONT_UNTRIED,
    NAME_FONT_READY,
    NAME_FONT_FAILED
};

/* The cores of the WWII captures' letters, (98, 141, 126) and (165, 82, 63),
 * each half a level up because R_ConvertColorToBytes truncates. */
static const vec4_t s_teammateColor = { 98.5f / 255.0f, 141.5f / 255.0f, 126.5f / 255.0f, 1.0f };
static const vec3_t s_enemyColor = { 165.5f / 255.0f, 82.5f / 255.0f, 63.5f / 255.0f };

static const dvar_t *cg_hudTeammateNames;
static const dvar_t *cg_hudEnemyNames;
static Glyph s_nameGlyphs[CG_NAME_FONT_GLYPH_COUNT];
static Font s_nameFont;
static int s_nameFontState;

static int CG_NameFontDigit(char c)
{
    return c <= '9' ? c - '0' : c - 'a' + 10;
}

/* Tried once per cgame, which a renderer restart starts again. */
static int CG_NameFontReady(void)
{
    byte *bgra;
    MaterialHandle material;
    int x, y, i;

    if (s_nameFontState != NAME_FONT_UNTRIED)
        return s_nameFontState == NAME_FONT_READY;
    s_nameFontState = NAME_FONT_FAILED;

    bgra = (byte *)malloc(CG_NAME_FONT_ATLAS_WIDTH * CG_NAME_FONT_ATLAS_HEIGHT * 4);
    if (!bgra)
        return 0;
    for (y = 0; y < CG_NAME_FONT_ATLAS_HEIGHT; y++) {
        for (x = 0; x < CG_NAME_FONT_ATLAS_WIDTH; x++) {
            byte *texel = bgra + (y * CG_NAME_FONT_ATLAS_WIDTH + x) * 4;

            texel[0] = texel[1] = texel[2] = 255;
            texel[3] = (byte)(CG_NameFontDigit(cg_nameFontAtlas[y][x]) * CG_NAME_FONT_ALPHA_STEP);
        }
    }
    material = R_CreateHudImageMaterial(R_HUD_IMAGE_NAME_FONT, bgra, CG_NAME_FONT_ATLAS_WIDTH, CG_NAME_FONT_ATLAS_HEIGHT);
    free(bgra);
    if (!material)
        return 0;

    for (i = 0; i < CG_NAME_FONT_GLYPH_COUNT; i++) {
        const CG_NameFontGlyph *src = &cg_nameFontGlyphs[i];
        Glyph *glyph = &s_nameGlyphs[i];

        glyph->letter = src->letter;
        glyph->x0 = src->x0;
        glyph->y0 = src->y0;
        glyph->dx = src->dx;
        glyph->pixelWidth = (byte)(src->width * CG_NAME_FONT_UNITS_PER_TEXEL);
        glyph->pixelHeight = (byte)(src->height * CG_NAME_FONT_UNITS_PER_TEXEL);
        glyph->s0 = (float)src->atlasX / CG_NAME_FONT_ATLAS_WIDTH;
        glyph->t0 = (float)src->atlasY / CG_NAME_FONT_ATLAS_HEIGHT;
        glyph->s1 = (float)(src->atlasX + src->width) / CG_NAME_FONT_ATLAS_WIDTH;
        glyph->t1 = (float)(src->atlasY + src->height) / CG_NAME_FONT_ATLAS_HEIGHT;
    }
    s_nameFont.name = "playerNameFont";
    s_nameFont.pixelHeight = CG_NAME_FONT_PIXEL_HEIGHT;
    s_nameFont.glyphCount = CG_NAME_FONT_GLYPH_COUNT;
    s_nameFont.material = material;
    s_nameFont.glyphs = s_nameGlyphs;
    s_nameFontState = NAME_FONT_READY;
    return 1;
}

/* The label's color covers the whole name, so color codes, a caret and a
 * digit to the renderer, are dropped. */
static void CG_PlayerNameClean(char *dest, const char *src)
{
    int length = 0;

    while (*src && length < NAME_LEN - 1) {
        if (src[0] == '^' && src[1] >= '0' && src[1] <= '9') {
            src += 2;
            continue;
        }
        dest[length++] = *src++;
    }
    dest[length] = 0;
}

static void CG_DrawPlayerName(const char *name, float x, float y, const vec_t *color)
{
    char clean[NAME_LEN];
    float width;

    CG_PlayerNameClean(clean, name);
    if (!clean[0])
        return;
    width = (float)UI_TextWidth(clean, 0x7fffffff, &s_nameFont, NAME_SCALE);
    UI_DrawText(clean, 0x7fffffff, &s_nameFont, x - 0.5f * width, y, NAME_ALIGN, NAME_ALIGN, NAME_SCALE, color,
                NAME_STYLE);
}

void CG_PlayerNamesInit(void)
{
    cg_hudTeammateNames = Dvar_RegisterBool_mac("cg_hudTeammateNames", 1, 0x1001);
    cg_hudEnemyNames = Dvar_RegisterBool_mac("cg_hudEnemyNames", 1, 0x1001);
    s_nameFontState = NAME_FONT_UNTRIED;
}

static int CG_TeammateNamesDrawn(void)
{
    return cg_hudTeammateNames && cg_hudTeammateNames->current.enabled && CG_NameFontReady();
}

int CG_DrawTeammateName(const char *name, float x, float y)
{
    if (!CG_TeammateNamesDrawn())
        return 0;
    CG_DrawPlayerName(name, x, y, s_teammateColor);
    return 1;
}

int CG_GetTeammateNameColor(float *color)
{
    if (!CG_TeammateNamesDrawn())
        return 0;
    color[0] = s_teammateColor[0];
    color[1] = s_teammateColor[1];
    color[2] = s_teammateColor[2];
    return 1;
}

void CG_DrawEnemyName(const char *name, float x, float y, float alpha)
{
    vec4_t color;

    if (!cg_hudEnemyNames || !cg_hudEnemyNames->current.enabled || !CG_NameFontReady())
        return;
    color[0] = s_enemyColor[0];
    color[1] = s_enemyColor[1];
    color[2] = s_enemyColor[2];
    color[3] = alpha;
    CG_DrawPlayerName(name, x, y, color);
}
