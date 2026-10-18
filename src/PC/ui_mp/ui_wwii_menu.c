#include "common_types.h"
#include "imports.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include "headers/PC/gfx_d3d/r_font_wwii.h"
#include "headers/PC/ui_mp/ui_wwii_menu.h"
#include <math.h>
#include <string.h>

extern sharedUiInfo_t sharedUiInfo;
extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern Bool Dvar_GetBool(const char *name);
extern const char *Dvar_GetVariantString(const char *dvarName);
extern int I_stricmp(const char *s0, const char *s1);
extern const char *UI_SafeTranslateString(const char *reference);
extern FontHandle UI_GetFontHandle(int fontEnum, float scale);
extern int CL_TextWidth(const char *text, int maxChars, FontHandle font);
extern float CL_NormalizedTextScale(FontHandle font, float scale);
extern const Glyph *R_GetCharacterGlyph(FontHandle font, unsigned int letter);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign,
                        int vertAlign, float scale, const vec_t *color, int style);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material);
extern void Menu_UpdatePosition(menuDef_t *menu);
extern qboolean Item_EnableShowViaDvar(const itemDef_t *item, int flag);
extern float GetRealHeightFromVirtualHeight(float height);
extern void CalcScreenX(float *x, int horzAlign);
extern void CalcScreenY(float *y, int vertAlign);

/* ui/menudefinition.h's alignments, item types and window flags. */
#define WWII_ALIGN_LEFT 1
#define WWII_ALIGN_TOP 1
#define WWII_ALIGN_RIGHT 3
#define WWII_ALIGN_BOTTOM 3
#define WWII_ALIGN_FULLSCREEN 4
#define WWII_ALIGN_NOSCALE 5
#define WWII_ITEM_TEXT 0
#define WWII_ITEM_BUTTON 1
#define WWII_WINDOW_HASFOCUS 2
#define WWII_WINDOW_VISIBLE 4
#define WWII_TEXT_SHADOWED 3

/* The retail main.menu's MAIN_RECT_X: its column of buttons and notes. The
 * buttons from WWII_MENU_RETAIL_EXTRAS down show only now and then, after
 * Quit, which the layout puts under the others, as the capture's Exit Game. */
#define WWII_MENU_RETAIL_X 385.0f
#define WWII_MENU_RETAIL_EXTRAS 350.0f
#define WWII_MENU_QUIT "@MENU_QUIT"
#define WWII_MENU_MAX_ITEMS 24

/* Units from the screen's edges, from the WWII capture at 1728 x 1080, 2.25
 * px a unit. The items are inked from 48.9 right of the left edge, on
 * baselines 185.8, 213.3 and 240.0 down, which rows from 185.95 every 27.1
 * meet within 0.3, their capitals 6.0 tall; the focused one's are 7.6, grown
 * about the same middle, and 8.0 further right. Pens start 0.7
 * left of that ink: Barlow Regular's capitals keep 0.45 (S) to 0.81 (E) of
 * these units before theirs. */
#define WWII_MENU_LEFT 48.2f
#define WWII_MENU_TOP 185.95f
#define WWII_MENU_SPACING 27.1f
#define WWII_MENU_CAP 6.0f
#define WWII_MENU_FOCUS_CAP 7.6f
#define WWII_MENU_FOCUS_INDENT 8.0f
/* Gaps between letters, in capitals: with Barlow Regular's advances the
 * capture's PLAY, SETTINGS and EXIT GAME, inked 23.1, 46.2 and 50.2 wide, come
 * within a third of a unit, and its focused PLAY, 28.9, with Bold's. */
#define WWII_MENU_TRACKING 0.22f
#define WWII_MENU_FOCUS_TRACKING 0.135f
/* The capture's focus slides over 7 frames at 30 fps, slowing as a quadratic
 * ease-out, and has its size and gold within 3. */
#define WWII_MENU_SLIDE_MS 233.0f
#define WWII_MENU_GROW_MS 100.0f
/* A button takes the cursor from this far above its baseline, over its
 * row but clear of the next. */
#define WWII_MENU_HIT_PAD 4.0f
#define WWII_MENU_HIT_ABOVE 15.0f
#define WWII_MENU_HIT_HEIGHT 21.0f
#define WWII_MENU_HIT_WIDTH 240.0f
/* A note is a sentence under or over its button, between two rows. */
#define WWII_MENU_NOTE_CAP 4.0f
#define WWII_MENU_NOTE_BELOW 11.0f
#define WWII_MENU_NOTE_ABOVE 13.0f

/* The capture's CALL OF DUTY lettering is inked from 47.1 to 210.0, and its
 * lockup ends 136.7 down; logo_cod2 is drawn as wide as that lettering and
 * ends there. The subtitle's capitals are 8.0 tall on a baseline 150.2 down,
 * centred under the lettering, 0.705 capitals apart in the UI's SemiBold,
 * which the WWII fonts draw at 95% of Barlow's width. */
#define WWII_LOGO_INK_LEFT 47.1f
#define WWII_LOGO_INK_WIDTH 162.9f
#define WWII_LOGO_INK_BOTTOM 136.7f
#define WWII_TITLE_CAP 8.0f
#define WWII_TITLE_BASELINE 150.2f
#define WWII_TITLE_TRACKING 0.705f
#define WWII_TITLE "MULTIPLAYER"
/* Where logo_cod2's lettering lies in the retail 288 by 72 rectangle, as
 * fractions, measured where it is drawn at 720 lines, and the retail
 * registered mark beside it at 222 and 56 of those units, its text scale
 * .17. */
#define WWII_LOGO_ASPECT 4.0f
#define WWII_LOGO_INK_X0 0.143f
#define WWII_LOGO_INK_X1 0.859f
#define WWII_LOGO_INK_Y1 0.76f
#define WWII_LOGO_MARK_X (222.0f / 288.0f)
#define WWII_LOGO_MARK_Y (56.0f / 72.0f)
#define WWII_LOGO_MARK_SCALE (0.17f / 72.0f)

/* The capture's version: digits 7.3 tall on a baseline 54.4 down, inked to
 * 39.1 left of the right edge, where the pen ends 0.6 further right, as
 * Barlow's digits end. */
#define WWII_VERSION_RIGHT 38.5f
#define WWII_VERSION_BASELINE 54.4f
#define WWII_VERSION_CAP 7.3f

/* The capture's key prompts: keycaps 10.2 tall ending 48.0 above the bottom,
 * the left one from 44.9, ENTER's 31.6 wide and ESC's 21.3: keys with
 * capitals 5.65 tall in Barlow Regular, as thin as the capture's, and 3.45
 * each side, which meet both within a tenth of a unit; keys on a baseline
 * 50.2 above the bottom, labels with capitals 6.0 tall on one 49.8 above it,
 * a label's pen 3.1 after its keycap; the right prompt's label is inked to
 * 48.9 left of the right edge, its pen ending 0.5 further right. */
#define WWII_PROMPT_LEFT 44.9f
#define WWII_PROMPT_RIGHT 48.4f
#define WWII_PROMPT_BOTTOM 48.0f
#define WWII_PROMPT_HEIGHT 10.2f
#define WWII_PROMPT_KEY_CAP 5.65f
#define WWII_PROMPT_KEY_BASELINE 50.2f
#define WWII_PROMPT_CAP 6.0f
#define WWII_PROMPT_BASELINE 49.8f
#define WWII_PROMPT_PAD 3.45f
#define WWII_PROMPT_GAP 3.1f

/* The capture's background is a live scene in gray. Ours is the retail one in
 * gray, zooming between 1.06 and 1.12 every 53 s and drifting across its
 * spare width and height every 97 s and 71 s; CoD2's art, darker than the
 * capture's sky, is lifted and steepened. A black wash behind the column
 * fades out by 55% of the width. */
#define WWII_BACKDROP_ZOOM 1.09f
#define WWII_BACKDROP_ZOOM_SWING 0.03f
#define WWII_BACKDROP_ZOOM_PERIOD 53.0f
#define WWII_BACKDROP_PAN_X_PERIOD 97.0f
#define WWII_BACKDROP_PAN_Y_PERIOD 71.0f
#define WWII_BACKDROP_GAMMA 0.8f
#define WWII_BACKDROP_CONTRAST 1.15f
#define WWII_SHADE_WIDTH 0.55f
#define WWII_SHADE_ALPHA 0.5f
#define WWII_SHADE_FALLOFF 1.6f

/* The capture's gold arrow is 14.7 units tall, its tip at the cursor. */
#define WWII_CURSOR_SIZE 16.0f

/* Runtime images: the wash's ramp, a keycap whose ends are half circles, and
 * the arrow, in texels. */
#define WWII_SHADE_IMAGE_W 64
#define WWII_SHADE_IMAGE_H 4
#define WWII_PILL_IMAGE_W 64
#define WWII_PILL_IMAGE_H 32
#define WWII_PILL_OUTLINE 2.0f
#define WWII_PILL_FILL 0.05f
#define WWII_PILL_FILL_ALPHA 0.88f
#define WWII_PILL_EDGE 0.79f
#define WWII_CURSOR_IMAGE 64
#define WWII_CURSOR_OUTLINE 2.5f
#define WWII_CURSOR_SAMPLES 4
#define WWII_IMAGE_TEXELS (WWII_CURSOR_IMAGE * WWII_CURSOR_IMAGE)

/* Measured on the capture: the idle items #c6c6c4, the subtitle #d9d7d3, the
 * labels #cfcfcf and keys #c9c9c9. The focused item's gold lies between the
 * capture's thin strokes, which its video's compression washes out, and the
 * solid gold of its logo. The version is the capture's dark gray turned
 * light, over CoD2's darker art. */
static const vec4_t s_wwiiIdle = { 0.776f, 0.776f, 0.769f, 1.0f };
static const vec4_t s_wwiiGold = { 0.80f, 0.72f, 0.47f, 1.0f };
static const vec4_t s_wwiiTitle = { 0.851f, 0.843f, 0.827f, 1.0f };
static const vec4_t s_wwiiLabel = { 0.812f, 0.812f, 0.812f, 1.0f };
static const vec4_t s_wwiiKey = { 0.788f, 0.788f, 0.788f, 1.0f };
static const vec4_t s_wwiiVersion = { 0.78f, 0.78f, 0.78f, 0.85f };
static const vec4_t s_wwiiMark = { 1.0f, 1.0f, 1.0f, 0.6f };
static const vec4_t s_wwiiWhite = { 1.0f, 1.0f, 1.0f, 1.0f };
static const vec4_t s_wwiiShade = { 1.0f, 1.0f, 1.0f, WWII_SHADE_ALPHA };
/* The arrow's light top and darker bottom, and its outline. */
static const float s_wwiiCursorTop[3] = { 1.0f, 0.92f, 0.66f };
static const float s_wwiiCursorBottom[3] = { 0.78f, 0.63f, 0.32f };
static const float s_wwiiCursorEdge[3] = { 0.16f, 0.12f, 0.05f };
/* The arrow's tip, outer wing, notch and lower wing, as fractions of its
 * image. */
static const float s_wwiiCursorShape[4][2] = {
    { 0.06f, 0.04f }, { 0.94f, 0.60f }, { 0.52f, 0.58f }, { 0.40f, 0.96f } };

typedef struct {
    itemDef_t *item;
    int note;
    /* The retail placement, back when the layout is switched off. */
    UiRectangle rect;
    float textalignx;
    float textaligny;
    UiRectangle wwiiRect;
    float wwiiAlignX;
    float wwiiAlignY;
    /* Focus from 0 to 1, slid and grown at their own pace. */
    float slide;
    float grow;
    int time;
} WWIIMenuItem;

static struct {
    const dvar_t *enabled;
    const displayContextDef_t *dc;
    menuDef_t *mainText;
    menuDef_t *bg;
    itemDef_t *bgImage;
    itemDef_t *bgLogo;
    itemDef_t *bgVersion;
    WWIIMenuItem items[WWII_MENU_MAX_ITEMS];
    int itemCount;
    int applied;
    int generation;
    MaterialHandle backdropSource;
    MaterialHandle backdrop;
    float backdropAspect;
    int imagesBuilt;
    MaterialHandle shade;
    MaterialHandle pill;
    MaterialHandle cursor;
    int fontsRegistered;
    FontHandle regular;
} s_wwiiMenu;

static float UI_WWIIMenu_Clamp01(float value)
{
    return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

static float UI_WWIIMenu_Lerp(float from, float to, float t)
{
    return from + (to - from) * t;
}

static float UI_WWIIMenu_EaseOut(float t)
{
    t = UI_WWIIMenu_Clamp01(t);
    return 1.0f - (1.0f - t) * (1.0f - t);
}

static byte UI_WWIIMenu_Byte(float value)
{
    return (byte)(UI_WWIIMenu_Clamp01(value) * 255.0f + 0.5f);
}

/* Gray levels for the background's luma. */
static void UI_WWIIMenu_Tone(byte *tone)
{
    int i;

    for (i = 0; i < 256; i++) {
        float gray = powf((float)i / 255.0f, WWII_BACKDROP_GAMMA);

        tone[i] = UI_WWIIMenu_Byte((gray - 0.5f) * WWII_BACKDROP_CONTRAST + 0.5f);
    }
}

/* The part of an image of aspect imageAspect that covers a screen of aspect
 * screenAspect at time ms, as texture coordinates s0, t0, s1, t1. */
static void UI_WWIIMenu_BackdropRect(int time, float screenAspect, float imageAspect, float *st)
{
    const float tau = 6.28318531f;
    float seconds = (float)time * 0.001f;
    float zoom = WWII_BACKDROP_ZOOM + WWII_BACKDROP_ZOOM_SWING * sinf(tau * seconds / WWII_BACKDROP_ZOOM_PERIOD);
    float w = 1.0f, h = 1.0f, cx, cy;

    if (screenAspect > 0.0f && imageAspect > 0.0f) {
        if (screenAspect < imageAspect)
            w = screenAspect / imageAspect;
        else
            h = imageAspect / screenAspect;
    }
    w /= zoom;
    h /= zoom;
    cx = 0.5f + 0.5f * (1.0f - w) * sinf(tau * seconds / WWII_BACKDROP_PAN_X_PERIOD);
    cy = 0.5f + 0.5f * (1.0f - h) * sinf(tau * seconds / WWII_BACKDROP_PAN_Y_PERIOD + 1.3f);
    st[0] = cx - 0.5f * w;
    st[1] = cy - 0.5f * h;
    st[2] = cx + 0.5f * w;
    st[3] = cy + 0.5f * h;
}

static void UI_WWIIMenu_ShadeImage(byte *bgra)
{
    int x, y;

    for (y = 0; y < WWII_SHADE_IMAGE_H; y++) {
        for (x = 0; x < WWII_SHADE_IMAGE_W; x++) {
            byte *texel = bgra + (y * WWII_SHADE_IMAGE_W + x) * 4;
            float fade = 1.0f - ((float)x + 0.5f) / (float)WWII_SHADE_IMAGE_W;

            texel[0] = texel[1] = texel[2] = 0;
            texel[3] = UI_WWIIMenu_Byte(powf(fade, WWII_SHADE_FALLOFF));
        }
    }
}

/* A keycap: near-black inside a light edge, its ends half circles that fill
 * the image's first and last quarters. */
static void UI_WWIIMenu_PillImage(byte *bgra)
{
    const float radius = 0.5f * (float)WWII_PILL_IMAGE_H;
    int x, y;

    for (y = 0; y < WWII_PILL_IMAGE_H; y++) {
        for (x = 0; x < WWII_PILL_IMAGE_W; x++) {
            byte *texel = bgra + (y * WWII_PILL_IMAGE_W + x) * 4;
            float px = (float)x + 0.5f, py = (float)y + 0.5f;
            float cx = px < radius ? radius : (px > WWII_PILL_IMAGE_W - radius ? WWII_PILL_IMAGE_W - radius : px);
            float dist = sqrtf((px - cx) * (px - cx) + (py - radius) * (py - radius)) - radius;
            float outer = UI_WWIIMenu_Clamp01(0.5f - dist);
            float inner = UI_WWIIMenu_Clamp01(0.5f - (dist + WWII_PILL_OUTLINE));
            float gray = UI_WWIIMenu_Lerp(WWII_PILL_EDGE, WWII_PILL_FILL, inner);

            texel[0] = texel[1] = texel[2] = UI_WWIIMenu_Byte(gray);
            texel[3] = UI_WWIIMenu_Byte(outer * UI_WWIIMenu_Lerp(1.0f, WWII_PILL_FILL_ALPHA, inner));
        }
    }
}

static int UI_WWIIMenu_InCursor(float x, float y)
{
    int i, j, inside = 0;

    for (i = 0, j = 3; i < 4; j = i++) {
        const float *a = s_wwiiCursorShape[i], *b = s_wwiiCursorShape[j];

        if ((a[1] > y) != (b[1] > y) && x < (b[0] - a[0]) * (y - a[1]) / (b[1] - a[1]) + a[0])
            inside = !inside;
    }
    return inside;
}

/* Distance from the arrow's nearest edge, in fractions of its image. */
static float UI_WWIIMenu_CursorEdge(float x, float y)
{
    float best = 1.0f;
    int i;

    for (i = 0; i < 4; i++) {
        const float *a = s_wwiiCursorShape[i], *b = s_wwiiCursorShape[(i + 1) & 3];
        float dx = b[0] - a[0], dy = b[1] - a[1];
        float t = UI_WWIIMenu_Clamp01(((x - a[0]) * dx + (y - a[1]) * dy) / (dx * dx + dy * dy));
        float ex = a[0] + t * dx - x, ey = a[1] + t * dy - y;
        float dist = sqrtf(ex * ex + ey * ey);

        if (dist < best)
            best = dist;
    }
    return best;
}

/* The gold arrow, light at its top and darker at its bottom, inside a dark
 * edge, its coverage sampled WWII_CURSOR_SAMPLES times a side. */
static void UI_WWIIMenu_CursorImage(byte *bgra)
{
    const float edge = WWII_CURSOR_OUTLINE / (float)WWII_CURSOR_IMAGE;
    const int samples = WWII_CURSOR_SAMPLES * WWII_CURSOR_SAMPLES;
    int x, y, sx, sy, c;

    for (y = 0; y < WWII_CURSOR_IMAGE; y++) {
        for (x = 0; x < WWII_CURSOR_IMAGE; x++) {
            byte *texel = bgra + (y * WWII_CURSOR_IMAGE + x) * 4;
            int covered = 0, outline = 0;
            float rgb[3];

            for (sy = 0; sy < WWII_CURSOR_SAMPLES; sy++) {
                for (sx = 0; sx < WWII_CURSOR_SAMPLES; sx++) {
                    float u = ((float)x + ((float)sx + 0.5f) / WWII_CURSOR_SAMPLES) / WWII_CURSOR_IMAGE;
                    float v = ((float)y + ((float)sy + 0.5f) / WWII_CURSOR_SAMPLES) / WWII_CURSOR_IMAGE;

                    if (!UI_WWIIMenu_InCursor(u, v))
                        continue;
                    covered++;
                    outline += UI_WWIIMenu_CursorEdge(u, v) < edge;
                }
            }
            for (c = 0; c < 3; c++) {
                float fill = UI_WWIIMenu_Lerp(s_wwiiCursorTop[c], s_wwiiCursorBottom[c],
                                              ((float)y + 0.5f) / WWII_CURSOR_IMAGE);

                rgb[c] = covered ? UI_WWIIMenu_Lerp(fill, s_wwiiCursorEdge[c], (float)outline / covered) : 0.0f;
            }
            texel[0] = UI_WWIIMenu_Byte(rgb[2]);
            texel[1] = UI_WWIIMenu_Byte(rgb[1]);
            texel[2] = UI_WWIIMenu_Byte(rgb[0]);
            texel[3] = UI_WWIIMenu_Byte((float)covered / samples);
        }
    }
}

static void UI_WWIIMenu_BuildImages(void)
{
    static byte texels[WWII_IMAGE_TEXELS * 4];

    if (s_wwiiMenu.imagesBuilt)
        return;
    s_wwiiMenu.imagesBuilt = 1;
    UI_WWIIMenu_ShadeImage(texels);
    s_wwiiMenu.shade = R_CreateHudImageMaterial(R_HUD_IMAGE_MENU_SHADE, texels, WWII_SHADE_IMAGE_W, WWII_SHADE_IMAGE_H);
    UI_WWIIMenu_PillImage(texels);
    s_wwiiMenu.pill = R_CreateHudImageMaterial(R_HUD_IMAGE_MENU_PILL, texels, WWII_PILL_IMAGE_W, WWII_PILL_IMAGE_H);
    UI_WWIIMenu_CursorImage(texels);
    s_wwiiMenu.cursor = R_CreateHudImageMaterial(R_HUD_IMAGE_MENU_CURSOR, texels, WWII_CURSOR_IMAGE, WWII_CURSOR_IMAGE);
}

/* The stock fonts' pixel heights, capitals' heights and baselines above the
 * pen, in their units: the scoreboard's table. */
static const struct {
    int pixelHeight;
    float cap;
    float baseline;
} s_wwiiMenuFonts[] = {
    { 12, 7.0f, 2.0f }, { 16, 11.0f, 2.0f }, { 24, 13.12f, 5.0f }, { 30, 17.1f, 6.0f }, { 32, 21.0f, 5.0f },
};

/* The capitals' height and the baseline's height above the pen, in font
 * units, as the scoreboard reads them: a glyph's box also holds a margin and
 * the baseline's offset, and the WWII fonts keep the stock fonts' metrics,
 * told apart by pixel height. */
static void UI_WWIIMenu_FontMetrics(FontHandle font, float *cap, float *rise)
{
    const Glyph *glyph;
    int i;

    for (i = 0; i < (int)(sizeof(s_wwiiMenuFonts) / sizeof(s_wwiiMenuFonts[0])); i++) {
        if (font->pixelHeight == s_wwiiMenuFonts[i].pixelHeight) {
            *cap = s_wwiiMenuFonts[i].cap;
            *rise = s_wwiiMenuFonts[i].baseline;
            return;
        }
    }
    glyph = R_GetCharacterGlyph(font, 'H');
    *cap = glyph && glyph->y0 < 0 ? (float)-glyph->y0 : 0.625f * (float)font->pixelHeight;
    *rise = 0.0f;
}

static float UI_WWIIMenu_CapUnits(FontHandle font)
{
    float cap;
    float rise;

    UI_WWIIMenu_FontMetrics(font, &cap, &rise);
    return cap;
}

/* The text scale that makes a font's capitals cap units tall. */
static float UI_WWIIMenu_Scale(FontHandle font, float cap)
{
    return cap * (float)font->pixelHeight / (48.0f * UI_WWIIMenu_CapUnits(font));
}

/* The smallest of the UI's SemiBold sizes whose capitals are as tall on the
 * screen as the text's, or the largest. */
static FontHandle UI_WWIIMenu_SemiBold(float cap)
{
    FontHandle fonts[4];
    float pixels = GetRealHeightFromVirtualHeight(cap);
    int i;

    fonts[0] = sharedUiInfo.assets.smallFont;
    fonts[1] = sharedUiInfo.assets.textFont;
    fonts[2] = sharedUiInfo.assets.bigFont;
    fonts[3] = sharedUiInfo.assets.extraBigFont;
    for (i = 0; i < 3; i++) {
        if (fonts[i] && UI_WWIIMenu_CapUnits(fonts[i]) >= pixels)
            return fonts[i];
    }
    return fonts[3];
}

static FontHandle UI_WWIIMenu_Regular(float cap)
{
    if (!s_wwiiMenu.fontsRegistered) {
        s_wwiiMenu.fontsRegistered = 1;
        s_wwiiMenu.regular = R_RegisterWwiiFont("$regularfont");
    }
    return s_wwiiMenu.regular ? s_wwiiMenu.regular : UI_WWIIMenu_SemiBold(cap);
}

static FontHandle UI_WWIIMenu_Bold(float cap)
{
    return sharedUiInfo.assets.boldFont ? sharedUiInfo.assets.boldFont : UI_WWIIMenu_SemiBold(cap);
}

/* From the first letter's pen to the last one's advance, with tracking
 * capitals between letters. */
static float UI_WWIIMenu_TextWidth(const char *text, FontHandle font, float cap, float tracking)
{
    float norm = CL_NormalizedTextScale(font, UI_WWIIMenu_Scale(font, cap));
    float width = 0.0f;
    int i;

    for (i = 0; text[i]; i++) {
        width += (float)CL_TextWidth(text + i, 1, font) * norm;
        if (text[i + 1])
            width += tracking * cap;
    }
    return width;
}

/* The scoreboard's placement: the renderer draws text half a unit up and to
 * the left of its pen, and the pen stands under the baseline. */
static void UI_WWIIMenu_DrawText(const char *text, FontHandle font, float x, float baseline, int horzAlign,
                                 int vertAlign, float cap, float tracking, const vec_t *color)
{
    float scale = UI_WWIIMenu_Scale(font, cap);
    float norm = CL_NormalizedTextScale(font, scale);
    float units;
    float rise;
    int i;

    UI_WWIIMenu_FontMetrics(font, &units, &rise);
    x += 0.5f * norm;
    baseline += (rise + 0.5f) * norm;
    if (tracking == 0.0f) {
        UI_DrawText(text, 0x7fffffff, font, x, baseline, horzAlign, vertAlign, scale, color, 0);
        return;
    }
    for (i = 0; text[i]; i++) {
        if (text[i] != ' ')
            UI_DrawText(text + i, 1, font, x, baseline, horzAlign, vertAlign, scale, color, 0);
        x += (float)CL_TextWidth(text + i, 1, font) * norm + tracking * cap;
    }
}

/* Capitals of ASCII and Latin-1 letters. */
static void UI_WWIIMenu_Upper(const char *text, char *out, int size)
{
    int i;

    for (i = 0; text[i] && i < size - 1; i++) {
        unsigned char c = (unsigned char)text[i];

        if ((c >= 'a' && c <= 'z') || (c >= 0xe0 && c <= 0xfe && c != 0xf7))
            c = (unsigned char)(c - 0x20);
        out[i] = (char)c;
    }
    out[i] = '\0';
}

static int UI_WWIIMenu_MainOpen(const displayContextDef_t *dc)
{
    int i;

    if (!s_wwiiMenu.mainText || !(s_wwiiMenu.mainText->window.dynamicFlags[0] & WWII_WINDOW_VISIBLE))
        return 0;
    for (i = 0; i < dc->openMenuCount; i++) {
        if (dc->menuStack[i] == s_wwiiMenu.mainText)
            return 1;
    }
    return 0;
}

static WWIIMenuItem *UI_WWIIMenu_Find(const itemDef_t *item)
{
    int i;

    for (i = 0; i < s_wwiiMenu.itemCount; i++) {
        if (s_wwiiMenu.items[i].item == item)
            return &s_wwiiMenu.items[i];
    }
    return NULL;
}

static int UI_WWIIMenu_IsQuit(const WWIIMenuItem *entry)
{
    return entry->item->text && !I_stricmp(entry->item->text, WWII_MENU_QUIT);
}

/* The buttons' rows in the retail column's order, Quit moved under the
 * others above WWII_MENU_RETAIL_EXTRAS; each note goes under or over the
 * button nearest it in the retail column. */
static void UI_WWIIMenu_Layout(void)
{
    float tops[WWII_MENU_MAX_ITEMS];
    float quitTop = -1.0f;
    int topCount = 0, i, j;

    for (i = 0; i < s_wwiiMenu.itemCount; i++) {
        const WWIIMenuItem *entry = &s_wwiiMenu.items[i];
        float top = entry->rect.y;

        if (entry->note)
            continue;
        if (UI_WWIIMenu_IsQuit(entry) && top < WWII_MENU_RETAIL_EXTRAS)
            quitTop = top;
        for (j = 0; j < topCount && tops[j] < top; j++)
            ;
        if (j < topCount && tops[j] == top)
            continue;
        memmove(&tops[j + 1], &tops[j], (topCount - j) * sizeof(tops[0]));
        tops[j] = top;
        topCount++;
    }
    if (quitTop >= 0.0f) {
        for (i = 0; tops[i] != quitTop; i++)
            ;
        for (; i + 1 < topCount && tops[i + 1] < WWII_MENU_RETAIL_EXTRAS; i++)
            tops[i] = tops[i + 1];
        tops[i] = quitTop;
    }

    for (i = 0; i < s_wwiiMenu.itemCount; i++) {
        WWIIMenuItem *entry = &s_wwiiMenu.items[i];
        int row = 0;
        float baseline;

        for (j = 1; j < topCount; j++) {
            if (fabsf(tops[j] - entry->rect.y) < fabsf(tops[row] - entry->rect.y))
                row = j;
        }
        baseline = WWII_MENU_TOP + (float)row * WWII_MENU_SPACING;
        entry->wwiiRect.x = WWII_MENU_LEFT - WWII_MENU_HIT_PAD;
        entry->wwiiRect.w = WWII_MENU_HIT_WIDTH;
        entry->wwiiRect.horzAlign = WWII_ALIGN_LEFT;
        entry->wwiiRect.vertAlign = WWII_ALIGN_TOP;
        entry->wwiiAlignX = WWII_MENU_HIT_PAD;
        if (entry->note) {
            baseline += entry->rect.y > tops[row] ? WWII_MENU_NOTE_BELOW : -WWII_MENU_NOTE_ABOVE;
            entry->wwiiRect.y = baseline - WWII_MENU_NOTE_CAP - 2.0f;
            entry->wwiiRect.h = WWII_MENU_NOTE_CAP + 4.0f;
            entry->wwiiAlignY = WWII_MENU_NOTE_CAP + 2.0f;
        } else {
            entry->wwiiRect.y = baseline - WWII_MENU_HIT_ABOVE;
            entry->wwiiRect.h = WWII_MENU_HIT_HEIGHT;
            entry->wwiiAlignY = WWII_MENU_HIT_ABOVE;
        }
    }
}

void UI_WWIIMenu_RegisterDvars(void)
{
    s_wwiiMenu.enabled = Dvar_RegisterBool_mac("ui_wwiiMainMenu", 1, 0x1001);
}

void UI_WWIIMenu_Reset(const displayContextDef_t *dc)
{
    const dvar_t *enabled = s_wwiiMenu.enabled;

    if (s_wwiiMenu.dc && dc != s_wwiiMenu.dc)
        return;
    memset(&s_wwiiMenu, 0, sizeof(s_wwiiMenu));
    s_wwiiMenu.enabled = enabled;
    s_wwiiMenu.generation = R_GetHudImageGeneration();
}

void UI_WWIIMenu_AddMenu(const displayContextDef_t *dc, menuDef_t *menu)
{
    const char *name = menu ? menu->window.name : NULL;
    int i;

    if (!name || (I_stricmp(name, "bg") && I_stricmp(name, "main_text")) || (s_wwiiMenu.dc && dc != s_wwiiMenu.dc))
        return;
    s_wwiiMenu.dc = dc;
    if (!I_stricmp(name, "bg")) {
        s_wwiiMenu.bg = menu;
        for (i = 0; i < menu->itemCount; i++) {
            itemDef_t *item = menu->items[i];

            if (item->window.style == 6 && item->dvar)
                s_wwiiMenu.bgImage = item;
            else if (item->window.name && !I_stricmp(item->window.name, "background_cod2_logo"))
                s_wwiiMenu.bgLogo = item;
            else if (item->dvar)
                s_wwiiMenu.bgVersion = item;
        }
        return;
    }
    s_wwiiMenu.mainText = menu;
    s_wwiiMenu.itemCount = 0;
    s_wwiiMenu.applied = 0;
    for (i = 0; i < menu->itemCount && s_wwiiMenu.itemCount < WWII_MENU_MAX_ITEMS; i++) {
        itemDef_t *item = menu->items[i];
        WWIIMenuItem *entry;

        if (item->window.rectClient[0].x != WWII_MENU_RETAIL_X ||
            (item->type != WWII_ITEM_BUTTON && item->type != WWII_ITEM_TEXT))
            continue;
        entry = &s_wwiiMenu.items[s_wwiiMenu.itemCount++];
        memset(entry, 0, sizeof(*entry));
        entry->item = item;
        entry->note = item->type == WWII_ITEM_TEXT;
        entry->rect = item->window.rectClient[0];
        entry->textalignx = item->textalignx;
        entry->textaligny = item->textaligny;
    }
    UI_WWIIMenu_Layout();
}

void UI_WWIIMenu_Update(displayContextDef_t *dc)
{
    int enabled = s_wwiiMenu.enabled && s_wwiiMenu.enabled->current.enabled && s_wwiiMenu.mainText &&
                  s_wwiiMenu.itemCount > 0;
    int i;

    (void)dc;
    if (s_wwiiMenu.generation != R_GetHudImageGeneration()) {
        s_wwiiMenu.generation = R_GetHudImageGeneration();
        s_wwiiMenu.backdropSource = s_wwiiMenu.backdrop = NULL;
        s_wwiiMenu.imagesBuilt = 0;
        s_wwiiMenu.shade = s_wwiiMenu.pill = s_wwiiMenu.cursor = NULL;
        s_wwiiMenu.fontsRegistered = 0;
        s_wwiiMenu.regular = NULL;
    }
    if (enabled == s_wwiiMenu.applied)
        return;
    for (i = 0; i < s_wwiiMenu.itemCount; i++) {
        WWIIMenuItem *entry = &s_wwiiMenu.items[i];
        itemDef_t *item = entry->item;

        item->window.rectClient[0] = enabled ? entry->wwiiRect : entry->rect;
        item->textalignx = enabled ? entry->wwiiAlignX : entry->textalignx;
        item->textaligny = enabled ? entry->wwiiAlignY : entry->textaligny;
        entry->slide = entry->grow = 0.0f;
        entry->time = 0;
    }
    Menu_UpdatePosition(s_wwiiMenu.mainText);
    s_wwiiMenu.applied = enabled;
}

/* The menus stretch the image over the item's rectangle, 896 by 480 for bg's,
 * so that is the aspect it is drawn for, whatever its texels'. */
static int UI_WWIIMenu_PaintBackdrop(displayContextDef_t *dc, itemDef_t *item)
{
    const UiRectangle *rect = &item->window.rectClient[0];
    MaterialHandle source = item->window.background;
    float st[4], width = 640.0f, height = 480.0f, aspect;

    if (source != s_wwiiMenu.backdropSource) {
        byte tone[256];

        s_wwiiMenu.backdropSource = source;
        UI_WWIIMenu_Tone(tone);
        s_wwiiMenu.backdrop = source ? R_CreateHudGrayMaterial(R_HUD_IMAGE_MENU_BACKDROP, source, tone,
                                                               &s_wwiiMenu.backdropAspect)
                                     : NULL;
    }
    if (!s_wwiiMenu.backdrop)
        return 0;
    UI_WWIIMenu_BuildImages();
    CalcScreenX(&width, WWII_ALIGN_FULLSCREEN);
    CalcScreenY(&height, WWII_ALIGN_FULLSCREEN);
    aspect = rect->w > 0.0f && rect->h > 0.0f ? rect->w / rect->h : s_wwiiMenu.backdropAspect;
    UI_WWIIMenu_BackdropRect(dc->realTime, height > 0.0f ? width / height : 0.0f, aspect, st);
    CL_DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, WWII_ALIGN_FULLSCREEN, WWII_ALIGN_FULLSCREEN, st[0], st[1], st[2],
                      st[3], s_wwiiWhite, s_wwiiMenu.backdrop);
    if (s_wwiiMenu.shade)
        CL_DrawStretchPic(0.0f, 0.0f, 640.0f * WWII_SHADE_WIDTH, 480.0f, WWII_ALIGN_FULLSCREEN, WWII_ALIGN_FULLSCREEN,
                          0.0f, 0.0f, 1.0f, 1.0f, s_wwiiShade, s_wwiiMenu.shade);
    return 1;
}

int UI_WWIIMenu_PaintItem(displayContextDef_t *dc, itemDef_t *item)
{
    if (!s_wwiiMenu.applied || !item || !s_wwiiMenu.bg || item->parent != s_wwiiMenu.bg)
        return 0;
    if (item == s_wwiiMenu.bgImage)
        return UI_WWIIMenu_PaintBackdrop(dc, item);
    return UI_WWIIMenu_MainOpen(dc);
}

static void UI_WWIIMenu_Animate(WWIIMenuItem *entry, int time, int focused)
{
    float elapsed = entry->time ? (float)(time - entry->time) : 0.0f;
    float step;

    if (elapsed < 0.0f || elapsed > 100.0f)
        elapsed = elapsed < 0.0f ? 0.0f : 100.0f;
    entry->time = time ? time : 1;
    step = focused ? 1.0f : -1.0f;
    entry->slide = UI_WWIIMenu_Clamp01(entry->slide + step * elapsed / WWII_MENU_SLIDE_MS);
    entry->grow = UI_WWIIMenu_Clamp01(entry->grow + step * elapsed / WWII_MENU_GROW_MS);
}

static void UI_WWIIMenu_PaintButton(displayContextDef_t *dc, WWIIMenuItem *entry, const char *text)
{
    itemDef_t *item = entry->item;
    const menuDef_t *parent = item->parent;
    char upper[256];
    float slide, grow, cap;
    vec4_t color;
    int i;

    UI_WWIIMenu_Animate(entry, dc->realTime, (item->window.dynamicFlags[0] & WWII_WINDOW_HASFOCUS) != 0);
    slide = UI_WWIIMenu_EaseOut(entry->slide);
    grow = UI_WWIIMenu_EaseOut(entry->grow);
    cap = UI_WWIIMenu_Lerp(WWII_MENU_CAP, WWII_MENU_FOCUS_CAP, grow);
    for (i = 0; i < 4; i++)
        color[i] = UI_WWIIMenu_Lerp(s_wwiiIdle[i], s_wwiiGold[i], grow);
    if ((item->dvarFlags & 3) && !Item_EnableShowViaDvar(item, 1)) {
        for (i = 0; i < 4; i++)
            color[i] = parent->disableColor[i];
    }
    UI_WWIIMenu_Upper(text, upper, sizeof(upper));
    UI_WWIIMenu_DrawText(upper, grow >= 0.5f ? UI_WWIIMenu_Bold(cap) : UI_WWIIMenu_Regular(cap),
                         item->window.rect[0].x + item->textalignx + WWII_MENU_FOCUS_INDENT * slide,
                         item->window.rect[0].y + item->textaligny + 0.5f * (cap - WWII_MENU_CAP),
                         item->window.rect[0].horzAlign, item->window.rect[0].vertAlign, cap,
                         UI_WWIIMenu_Lerp(WWII_MENU_TRACKING, WWII_MENU_FOCUS_TRACKING, grow), color);
}

int UI_WWIIMenu_PaintText(displayContextDef_t *dc, itemDef_t *item, const char *text)
{
    WWIIMenuItem *entry;

    if (!s_wwiiMenu.applied || !item || item->parent != s_wwiiMenu.mainText || !(entry = UI_WWIIMenu_Find(item)))
        return 0;
    if (entry->note) {
        UI_WWIIMenu_DrawText(text, UI_WWIIMenu_Regular(WWII_MENU_NOTE_CAP), item->window.rect[0].x + item->textalignx,
                             item->window.rect[0].y + item->textaligny, item->window.rect[0].horzAlign,
                             item->window.rect[0].vertAlign, WWII_MENU_NOTE_CAP, 0.0f, item->window.foreColor);
        return 1;
    }
    UI_WWIIMenu_PaintButton(dc, entry, text);
    return 1;
}

static void UI_WWIIMenu_PaintLogo(void)
{
    FontHandle font;
    float width, height, x, y, scale;

    if (s_wwiiMenu.bgLogo && s_wwiiMenu.bgLogo->window.background) {
        width = WWII_LOGO_INK_WIDTH / (WWII_LOGO_INK_X1 - WWII_LOGO_INK_X0);
        height = width / WWII_LOGO_ASPECT;
        x = WWII_LOGO_INK_LEFT - WWII_LOGO_INK_X0 * width;
        y = WWII_LOGO_INK_BOTTOM - WWII_LOGO_INK_Y1 * height;
        CL_DrawStretchPic(x, y, width, height, WWII_ALIGN_LEFT, WWII_ALIGN_TOP, 0.0f, 0.0f, 1.0f, 1.0f, s_wwiiWhite,
                          s_wwiiMenu.bgLogo->window.background);
        scale = WWII_LOGO_MARK_SCALE * height;
        font = UI_GetFontHandle(0, scale);
        UI_DrawText(UI_SafeTranslateString("MENU_R_CIRCLE"), 0x7fffffff, font, x + WWII_LOGO_MARK_X * width,
                    y + WWII_LOGO_MARK_Y * height, WWII_ALIGN_LEFT, WWII_ALIGN_TOP, scale, s_wwiiMark,
                    WWII_TEXT_SHADOWED);
    }
    font = UI_WWIIMenu_SemiBold(WWII_TITLE_CAP);
    width = UI_WWIIMenu_TextWidth(WWII_TITLE, font, WWII_TITLE_CAP, WWII_TITLE_TRACKING);
    UI_WWIIMenu_DrawText(WWII_TITLE, font, WWII_LOGO_INK_LEFT + 0.5f * (WWII_LOGO_INK_WIDTH - width),
                         WWII_TITLE_BASELINE, WWII_ALIGN_LEFT, WWII_ALIGN_TOP, WWII_TITLE_CAP, WWII_TITLE_TRACKING,
                         s_wwiiTitle);
}

static void UI_WWIIMenu_PaintVersion(void)
{
    const char *text;
    FontHandle font;

    if (!s_wwiiMenu.bgVersion)
        return;
    text = Dvar_GetVariantString(s_wwiiMenu.bgVersion->dvar);
    if (!text || !text[0])
        return;
    font = UI_WWIIMenu_Regular(WWII_VERSION_CAP);
    UI_WWIIMenu_DrawText(text, font, -WWII_VERSION_RIGHT - UI_WWIIMenu_TextWidth(text, font, WWII_VERSION_CAP, 0.0f),
                         WWII_VERSION_BASELINE, WWII_ALIGN_RIGHT, WWII_ALIGN_TOP, WWII_VERSION_CAP, 0.0f,
                         s_wwiiVersion);
}

/* The prompt's width from its keycap's left to its label's end; keycap gets
 * the keycap's. */
static float UI_WWIIMenu_PromptWidth(const char *key, const char *label, float *keycap)
{
    float keyWidth =
        UI_WWIIMenu_TextWidth(key, UI_WWIIMenu_Regular(WWII_PROMPT_KEY_CAP), WWII_PROMPT_KEY_CAP, 0.0f);

    *keycap = keyWidth + 2.0f * WWII_PROMPT_PAD;
    if (*keycap < WWII_PROMPT_HEIGHT)
        *keycap = WWII_PROMPT_HEIGHT;
    return *keycap + WWII_PROMPT_GAP +
           UI_WWIIMenu_TextWidth(label, UI_WWIIMenu_Regular(WWII_PROMPT_CAP), WWII_PROMPT_CAP, 0.0f);
}

static void UI_WWIIMenu_PaintPrompt(float x, int horzAlign, const char *key, const char *label)
{
    const float radius = 0.5f * WWII_PROMPT_HEIGHT;
    const float top = -(WWII_PROMPT_BOTTOM + WWII_PROMPT_HEIGHT);
    FontHandle keyFont = UI_WWIIMenu_Regular(WWII_PROMPT_KEY_CAP);
    float keycap, keyWidth;

    UI_WWIIMenu_PromptWidth(key, label, &keycap);
    if (s_wwiiMenu.pill) {
        CL_DrawStretchPic(x, top, radius, WWII_PROMPT_HEIGHT, horzAlign, WWII_ALIGN_BOTTOM, 0.0f, 0.0f, 0.25f, 1.0f,
                          s_wwiiWhite, s_wwiiMenu.pill);
        CL_DrawStretchPic(x + radius, top, keycap - 2.0f * radius, WWII_PROMPT_HEIGHT, horzAlign, WWII_ALIGN_BOTTOM,
                          0.25f, 0.0f, 0.75f, 1.0f, s_wwiiWhite, s_wwiiMenu.pill);
        CL_DrawStretchPic(x + keycap - radius, top, radius, WWII_PROMPT_HEIGHT, horzAlign, WWII_ALIGN_BOTTOM, 0.75f,
                          0.0f, 1.0f, 1.0f, s_wwiiWhite, s_wwiiMenu.pill);
    }
    keyWidth = UI_WWIIMenu_TextWidth(key, keyFont, WWII_PROMPT_KEY_CAP, 0.0f);
    UI_WWIIMenu_DrawText(key, keyFont, x + 0.5f * (keycap - keyWidth), -WWII_PROMPT_KEY_BASELINE, horzAlign,
                         WWII_ALIGN_BOTTOM, WWII_PROMPT_KEY_CAP, 0.0f, s_wwiiKey);
    UI_WWIIMenu_DrawText(label, UI_WWIIMenu_Regular(WWII_PROMPT_CAP), x + keycap + WWII_PROMPT_GAP,
                         -WWII_PROMPT_BASELINE, horzAlign, WWII_ALIGN_BOTTOM, WWII_PROMPT_CAP, 0.0f, s_wwiiLabel);
}

/* Enter selects the focused item; Escape returns to a match only from
 * inside one, where main_text's onESC closes the menu. */
static void UI_WWIIMenu_PaintPrompts(void)
{
    const char *back;
    float keycap;

    UI_WWIIMenu_PaintPrompt(WWII_PROMPT_LEFT, WWII_ALIGN_LEFT, "ENTER", "Select");
    if (!Dvar_GetBool("cl_ingame"))
        return;
    back = UI_SafeTranslateString("MENU_BACKTOGAME");
    UI_WWIIMenu_PaintPrompt(-WWII_PROMPT_RIGHT - UI_WWIIMenu_PromptWidth("ESC", back, &keycap), WWII_ALIGN_RIGHT,
                            "ESC", back);
}

void UI_WWIIMenu_PaintMenu(displayContextDef_t *dc, menuDef_t *menu)
{
    if (!s_wwiiMenu.applied || !menu || menu != s_wwiiMenu.bg || !UI_WWIIMenu_MainOpen(dc))
        return;
    UI_WWIIMenu_BuildImages();
    UI_WWIIMenu_PaintLogo();
    UI_WWIIMenu_PaintVersion();
    UI_WWIIMenu_PaintPrompts();
}

int UI_WWIIMenu_DrawCursor(int x, int y)
{
    float px = (float)x, py = (float)y, size;

    if (!s_wwiiMenu.applied)
        return 0;
    UI_WWIIMenu_BuildImages();
    if (!s_wwiiMenu.cursor)
        return 0;
    CalcScreenX(&px, WWII_ALIGN_FULLSCREEN);
    CalcScreenY(&py, WWII_ALIGN_FULLSCREEN);
    size = GetRealHeightFromVirtualHeight(WWII_CURSOR_SIZE);
    CL_DrawStretchPic(px - s_wwiiCursorShape[0][0] * size, py - s_wwiiCursorShape[0][1] * size, size, size,
                      WWII_ALIGN_NOSCALE, WWII_ALIGN_NOSCALE, 0.0f, 0.0f, 1.0f, 1.0f, s_wwiiWhite, s_wwiiMenu.cursor);
    return 1;
}

static int UI_WWIIMenu_LaidOut(const menuDef_t *menu)
{
    return s_wwiiMenu.applied && menu && menu == s_wwiiMenu.mainText;
}

int UI_WWIIMenu_CoversScreen(const menuDef_t *menu)
{
    return UI_WWIIMenu_LaidOut(menu);
}

/* Whether main_text's item a comes before b on the keys: the column by its
 * rows, then in the menu's order, and the items off it after. */
static int UI_WWIIMenu_Before(const menuDef_t *menu, int a, int b)
{
    const WWIIMenuItem *first = UI_WWIIMenu_Find(menu->items[a]);
    const WWIIMenuItem *second = UI_WWIIMenu_Find(menu->items[b]);

    if (!first != !second)
        return first != NULL;
    if (first && first->wwiiRect.y != second->wwiiRect.y)
        return first->wwiiRect.y < second->wwiiRect.y;
    return a < b;
}

int UI_WWIIMenu_NextItem(const menuDef_t *menu, int cursor, int direction)
{
    int count = menu->itemCount, next = -1, wrap = -1, i;

    if (count <= 0)
        return cursor;
    if (!UI_WWIIMenu_LaidOut(menu))
        return (cursor + direction + count) % count;
    for (i = 0; i < count; i++) {
        int *pick;

        if (i == cursor)
            continue;
        pick = cursor < 0 || cursor >= count ||
                       (direction > 0 ? UI_WWIIMenu_Before(menu, cursor, i) : UI_WWIIMenu_Before(menu, i, cursor))
                   ? &next
                   : &wrap;
        if (*pick < 0 || (direction > 0 ? UI_WWIIMenu_Before(menu, i, *pick) : UI_WWIIMenu_Before(menu, *pick, i)))
            *pick = i;
    }
    return next >= 0 ? next : wrap >= 0 ? wrap : cursor;
}
