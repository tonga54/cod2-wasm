#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_mantleprompt.h"
#include "headers/PC/cgame_mp/cg_playernames.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <stdlib.h>

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern int CL_TextWidth(const char *text, int maxChars, FontHandle font);
extern float CL_NormalizedTextScale(FontHandle font, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);

/* Virtual units from the stock hint's anchor, 130 below the screen centre,
 * matched to the WWII capture in cap heights: capitals 7 units tall centred on
 * the anchor, an ink gap of 0.59 after the key and half one before the arrow's
 * tail, whose tip fades under a pixel at 720p, and the arrow's box from 1.7
 * cap heights above the baseline to 1 below. The line is centred on the
 * anchor. The menu's text style shadows by two pixels, heavier than the
 * capture's letters; the labels' one-pixel shadow keeps them legible on sand. */
#define MANTLE_PROMPT_SCALE 0.21f
#define MANTLE_PROMPT_STYLE 3
#define MANTLE_PROMPT_BASELINE 3.5f
#define MANTLE_PROMPT_KEY_GAP 3.0f
#define MANTLE_PROMPT_ARROW_GAP 2.0f
#define MANTLE_PROMPT_ARROW_TOP -12.1f
#define MANTLE_PROMPT_ARROW_WIDTH 31.2f
#define MANTLE_PROMPT_ARROW_HEIGHT 19.3f
#define MANTLE_PROMPT_SHADOW_OFFSET 1.0f
#define MANTLE_PROMPT_SHADOW_ALPHA 0.6f
/* Two texels for each pixel of the arrow's 60 x 37 box on the capture, each
 * texel's alpha from 4 x 4 samples. */
#define MANTLE_ARROW_TEXELS 2
#define MANTLE_ARROW_IMAGE_WIDTH (60 * MANTLE_ARROW_TEXELS)
#define MANTLE_ARROW_IMAGE_HEIGHT (37 * MANTLE_ARROW_TEXELS)
#define MANTLE_ARROW_SAMPLES 4

/* The capture's colours, brightened to the HUD's text as for the weapon
 * prompt: a gold key, near-white words and a white arrow. */
static const vec4_t s_mantlePromptKey = { 0.95f, 0.84f, 0.34f, 1.0f };
static const vec4_t s_mantlePromptText = { 0.96f, 0.95f, 0.92f, 1.0f };
static const vec4_t s_mantlePromptArrow = { 1.0f, 1.0f, 1.0f, 1.0f };
static const vec4_t s_mantlePromptShadow = { 0.0f, 0.0f, 0.0f, MANTLE_PROMPT_SHADOW_ALPHA };

/* The capture's arrow in pixels of its box, fitted to the capture's pixels
 * with its tail's end held where the capture's ends: a band inside one circle
 * and outside another, thin at its left tip and widening to the right, cut
 * where a triangular head takes over. A circle is its centre and radius; the
 * head is its left barb, its right barb and its tip. */
static const float s_mantleArrowOuter[3] = { 26.56f, 31.23f, 29.66f };
static const float s_mantleArrowInner[3] = { 19.62f, 35.85f, 27.5f };
static const float s_mantleArrowHead[3][2] = { { 38.55f, 23.8f }, { 58.25f, 18.03f }, { 54.55f, 35.48f } };

static const dvar_t *cg_hudMantlePrompt;
static MaterialHandle s_mantleArrow;
static int s_mantleArrowTried;

/* Not negative inside the head, for each of its edges in turn. */
static float CG_MantleArrowSide(const float *a, const float *b, float x, float y)
{
    return (b[0] - a[0]) * (y - a[1]) - (b[1] - a[1]) * (x - a[0]);
}

static int CG_MantleArrowCovers(float x, float y)
{
    const float (*head)[2] = s_mantleArrowHead;
    float ox = x - s_mantleArrowOuter[0];
    float oy = y - s_mantleArrowOuter[1];
    float ix = x - s_mantleArrowInner[0];
    float iy = y - s_mantleArrowInner[1];

    if (CG_MantleArrowSide(head[0], head[1], x, y) >= 0.0f)
        return CG_MantleArrowSide(head[1], head[2], x, y) >= 0.0f && CG_MantleArrowSide(head[2], head[0], x, y) >= 0.0f;
    return ox * ox + oy * oy <= s_mantleArrowOuter[2] * s_mantleArrowOuter[2] &&
           ix * ix + iy * iy > s_mantleArrowInner[2] * s_mantleArrowInner[2];
}

static void CG_MantleArrowRaster(byte *bgra)
{
    int x, y, sx, sy;

    for (y = 0; y < MANTLE_ARROW_IMAGE_HEIGHT; y++) {
        for (x = 0; x < MANTLE_ARROW_IMAGE_WIDTH; x++) {
            byte *texel = bgra + (y * MANTLE_ARROW_IMAGE_WIDTH + x) * 4;
            int covered = 0;

            for (sy = 0; sy < MANTLE_ARROW_SAMPLES; sy++) {
                for (sx = 0; sx < MANTLE_ARROW_SAMPLES; sx++) {
                    covered += CG_MantleArrowCovers(
                        ((float)x + ((float)sx + 0.5f) / MANTLE_ARROW_SAMPLES) / MANTLE_ARROW_TEXELS,
                        ((float)y + ((float)sy + 0.5f) / MANTLE_ARROW_SAMPLES) / MANTLE_ARROW_TEXELS);
                }
            }
            texel[0] = texel[1] = texel[2] = 255;
            texel[3] = (byte)(covered * 255 / (MANTLE_ARROW_SAMPLES * MANTLE_ARROW_SAMPLES));
        }
    }
}

/* Tried once per cgame, which a renderer restart starts again. */
static MaterialHandle CG_MantleArrowMaterial(void)
{
    byte *bgra;

    if (s_mantleArrowTried)
        return s_mantleArrow;
    s_mantleArrowTried = 1;
    bgra = (byte *)malloc(MANTLE_ARROW_IMAGE_WIDTH * MANTLE_ARROW_IMAGE_HEIGHT * 4);
    if (!bgra)
        return NULL;
    CG_MantleArrowRaster(bgra);
    s_mantleArrow = R_CreateHudImageMaterial(R_HUD_IMAGE_MANTLE_ARROW, bgra, MANTLE_ARROW_IMAGE_WIDTH,
                                             MANTLE_ARROW_IMAGE_HEIGHT);
    free(bgra);
    return s_mantleArrow;
}

void CG_MantlePromptInit(void)
{
    cg_hudMantlePrompt = Dvar_RegisterBool_mac("cg_hudMantlePrompt", 1, 0x1001);
    s_mantleArrow = NULL;
    s_mantleArrowTried = 0;
}

int CG_MantlePromptActive(void)
{
    return cg_hudMantlePrompt && cg_hudMantlePrompt->current.enabled;
}

/* The advance the renderer draws, which UI_TextWidth rounds down to a unit. */
static float CG_MantlePromptWidth(const char *text, FontHandle font)
{
    return (float)CL_TextWidth(text, 0, font) * CL_NormalizedTextScale(font, MANTLE_PROMPT_SCALE);
}

int CG_DrawMantlePrompt(const rectDef_t *rect, const char *key)
{
    static const char words[] = "Press to";
    FontHandle font = CG_GetPlayerNameFont();
    MaterialHandle arrow = font ? CG_MantleArrowMaterial() : NULL;
    float keyWidth, wordsWidth, x, y;

    if (!arrow)
        return 0;
    keyWidth = CG_MantlePromptWidth(key, font);
    wordsWidth = CG_MantlePromptWidth(words, font);
    x = rect->x - 0.5f * (keyWidth + MANTLE_PROMPT_KEY_GAP + wordsWidth + MANTLE_PROMPT_ARROW_GAP +
                          MANTLE_PROMPT_ARROW_WIDTH);
    y = rect->y + MANTLE_PROMPT_BASELINE;
    UI_DrawText(key, 0x7fffffff, font, x, y, rect->horzAlign, rect->vertAlign, MANTLE_PROMPT_SCALE,
                s_mantlePromptKey, MANTLE_PROMPT_STYLE);
    x += keyWidth + MANTLE_PROMPT_KEY_GAP;
    UI_DrawText(words, 0x7fffffff, font, x, y, rect->horzAlign, rect->vertAlign, MANTLE_PROMPT_SCALE,
                s_mantlePromptText, MANTLE_PROMPT_STYLE);
    x += wordsWidth + MANTLE_PROMPT_ARROW_GAP;
    y += MANTLE_PROMPT_ARROW_TOP;
    CL_DrawStretchPic(x + MANTLE_PROMPT_SHADOW_OFFSET, y + MANTLE_PROMPT_SHADOW_OFFSET, MANTLE_PROMPT_ARROW_WIDTH,
                      MANTLE_PROMPT_ARROW_HEIGHT, rect->horzAlign, rect->vertAlign, 0.0f, 0.0f, 1.0f, 1.0f,
                      s_mantlePromptShadow, arrow);
    CL_DrawStretchPic(x, y, MANTLE_PROMPT_ARROW_WIDTH, MANTLE_PROMPT_ARROW_HEIGHT, rect->horzAlign,
                      rect->vertAlign, 0.0f, 0.0f, 1.0f, 1.0f, s_mantlePromptArrow, arrow);
    return 1;
}
