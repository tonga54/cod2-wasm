#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_teamnames.h"
#include "headers/PC/cgame_mp/cg_namefont_data.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <stdlib.h>

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);

/* Capitals 7 units tall, like the stock label's. */
#define TEAM_NAME_SCALE 0.21f
#define TEAM_NAME_ALIGN 7
/* The capture's letters have no shadow, but its background is darker than
 * CoD2's walls and sand; the stock one-pixel shadow keeps the teal legible. */
#define TEAM_NAME_STYLE 3
#define TEAM_NAME_LEN 32

enum {
    TEAM_NAME_FONT_UNTRIED,
    TEAM_NAME_FONT_READY,
    TEAM_NAME_FONT_FAILED
};

/* The core of the WWII capture's letters, (98, 141, 126), each half a level up
 * because R_ConvertColorToBytes truncates. */
static const vec4_t s_teamNameColor = { 98.5f / 255.0f, 141.5f / 255.0f, 126.5f / 255.0f, 1.0f };

static const dvar_t *cg_hudTeammateNames;
static Glyph s_teamNameGlyphs[CG_NAME_FONT_GLYPH_COUNT];
static Font s_teamNameFont;
static int s_teamNameFontState;

static int CG_TeamNameDigit(char c)
{
    return c <= '9' ? c - '0' : c - 'a' + 10;
}

/* Tried once per cgame, which a renderer restart starts again. */
static int CG_TeamNameFontReady(void)
{
    byte *bgra;
    MaterialHandle material;
    int x, y, i;

    if (s_teamNameFontState != TEAM_NAME_FONT_UNTRIED)
        return s_teamNameFontState == TEAM_NAME_FONT_READY;
    s_teamNameFontState = TEAM_NAME_FONT_FAILED;

    bgra = (byte *)malloc(CG_NAME_FONT_ATLAS_WIDTH * CG_NAME_FONT_ATLAS_HEIGHT * 4);
    if (!bgra)
        return 0;
    for (y = 0; y < CG_NAME_FONT_ATLAS_HEIGHT; y++) {
        for (x = 0; x < CG_NAME_FONT_ATLAS_WIDTH; x++) {
            byte *texel = bgra + (y * CG_NAME_FONT_ATLAS_WIDTH + x) * 4;

            texel[0] = texel[1] = texel[2] = 255;
            texel[3] = (byte)(CG_TeamNameDigit(cg_nameFontAtlas[y][x]) * CG_NAME_FONT_ALPHA_STEP);
        }
    }
    material = R_CreateHudImageMaterial(R_HUD_IMAGE_NAME_FONT, bgra, CG_NAME_FONT_ATLAS_WIDTH, CG_NAME_FONT_ATLAS_HEIGHT);
    free(bgra);
    if (!material)
        return 0;

    for (i = 0; i < CG_NAME_FONT_GLYPH_COUNT; i++) {
        const CG_NameFontGlyph *src = &cg_nameFontGlyphs[i];
        Glyph *glyph = &s_teamNameGlyphs[i];

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
    s_teamNameFont.name = "teamNameFont";
    s_teamNameFont.pixelHeight = CG_NAME_FONT_PIXEL_HEIGHT;
    s_teamNameFont.glyphCount = CG_NAME_FONT_GLYPH_COUNT;
    s_teamNameFont.material = material;
    s_teamNameFont.glyphs = s_teamNameGlyphs;
    s_teamNameFontState = TEAM_NAME_FONT_READY;
    return 1;
}

/* The teal covers the whole name, so color codes, a caret and a digit to the
 * renderer, are dropped. */
static void CG_TeamNameClean(char *dest, const char *src)
{
    int length = 0;

    while (*src && length < TEAM_NAME_LEN - 1) {
        if (src[0] == '^' && src[1] >= '0' && src[1] <= '9') {
            src += 2;
            continue;
        }
        dest[length++] = *src++;
    }
    dest[length] = 0;
}

void CG_TeamNamesInit(void)
{
    cg_hudTeammateNames = Dvar_RegisterBool_mac("cg_hudTeammateNames", 1, 0x1001);
    s_teamNameFontState = TEAM_NAME_FONT_UNTRIED;
}

int CG_DrawTeammateName(const char *name, float x, float y)
{
    char clean[TEAM_NAME_LEN];
    float width;

    if (!cg_hudTeammateNames || !cg_hudTeammateNames->current.enabled || !CG_TeamNameFontReady())
        return 0;
    CG_TeamNameClean(clean, name);
    if (!clean[0])
        return 1;
    width = (float)UI_TextWidth(clean, 0x7fffffff, &s_teamNameFont, TEAM_NAME_SCALE);
    UI_DrawText(clean, 0x7fffffff, &s_teamNameFont, x - 0.5f * width, y, TEAM_NAME_ALIGN, TEAM_NAME_ALIGN,
                TEAM_NAME_SCALE, s_teamNameColor, TEAM_NAME_STYLE);
    return 1;
}
