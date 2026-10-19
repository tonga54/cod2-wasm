#include "common_types.h"
#include "imports.h"
#include "headers/PC/bgame/bg_attachments.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_breathprompt.h"
#include "headers/PC/cgame_mp/cg_playernames.h"

extern const dvar_t *cg_drawBreathHint;
extern const dvar_t *player_breath_hold_time;
extern int CL_TextWidth(const char *text, int maxChars, FontHandle font);
extern float CL_NormalizedTextScale(FontHandle font, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float scale, const vec_t *color, int style);
extern void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1, float s2, float t2, const vec_t *color, MaterialHandle material);
extern void Controls_GetConfig(void);
extern int GetKeyBindings(const char *command, char (*bindings)[128]);
extern const char *SEH_StringEd_GetString(const char *pszReference);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern int BG_GetViewmodelWeaponIndex(const playerState_t *ps);
extern void I_strncpyz(char *dest, const char *src, int destsize);

/* Virtual units from the screen's centre, from the WWII capture at 1.2 px a
 * unit with its scope's reticle as the centre. The label's capitals are 6
 * units tall on a baseline 61.2 above the centre, inked from 260.1 right of
 * it; the labels' lettering inks its "H" 0.67 units after the pen. The
 * capture's left-stick glyph is 11 units tall, centred 63 above the centre,
 * and ends 28.5 before the label. The keycap takes its height and its right
 * edge and grows to the left with the key's name. */
#define BREATH_PROMPT_ALIGN 2
#define BREATH_PROMPT_TEXT_X 259.4f
#define BREATH_PROMPT_BASELINE -61.2f
#define BREATH_PROMPT_SCALE 0.18f
#define BREATH_PROMPT_STYLE 3
#define BREATH_PROMPT_KEY_RIGHT 231.6f
#define BREATH_PROMPT_KEY_TOP -68.5f
#define BREATH_PROMPT_KEY_HEIGHT 11.0f
/* The key's name has capitals 5 units tall, centred on the keycap, with 3
 * units each side; a short name leaves the keycap square. */
#define BREATH_PROMPT_KEY_SCALE 0.15f
#define BREATH_PROMPT_KEY_BASELINE 8.0f
#define BREATH_PROMPT_KEY_PAD 3.0f
#define BREATH_PROMPT_SHADOW_OFFSET 1.0f
#define BREATH_PROMPT_SHADOW_ALPHA 0.6f
/* Until a held breath has recovered, the key does nothing. */
#define BREATH_PROMPT_RECOVERING_ALPHA 0.3f
#define BREATH_PROMPT_HOLDING 0x4000
#define BREATH_PROMPT_KEY_LEN 64

/* The other prompts' near-white words and cream icons, the capture's glyph
 * being the cream of its label as theirs are; the key's name is dark, as the
 * glyph's letters. */
static const vec4_t s_breathPromptText = { 0.96f, 0.95f, 0.92f, 1.0f };
static const vec4_t s_breathPromptKeycap = { 0.96f, 0.94f, 0.86f, 1.0f };
static const vec4_t s_breathPromptKeyName = { 0.0f, 0.0f, 0.0f, 1.0f };
static const vec4_t s_breathPromptShadow = { 0.0f, 0.0f, 0.0f, BREATH_PROMPT_SHADOW_ALPHA };

/* Sprinting's key holds the breath too; the stock hint's commands follow. */
static const char *const s_breathPromptCommands[] = { "+holdbreath", "+sprint", "+melee_breath",
                                                      "+breath_binoculars" };

static void CG_BreathPromptColor(vec4_t out, const vec4_t in, float alpha)
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    out[3] = in[3] * alpha;
}

/* Where PM_UpdateHoldBreath lets the key start a hold, and while it would but
 * for the recovery, at a lower alpha; 0 elsewhere and while holding. */
static float CG_BreathPromptAlpha(const playerState_t *ps)
{
    const WeaponDef *weapDef;

    /* A replay shows the killer's aim, not the viewer's. */
    if (cg->inKillCam)
        return 0.0f;
    if (!cg_drawBreathHint->current.enabled || cg->renderingThirdPerson || ps->pm_type > 1 || ps->stats[0] <= 0 ||
        (ps->eFlags & 0x300) || (ps->pm_flags & BREATH_PROMPT_HOLDING) || ps->fWeaponPosFrac != 1.0f ||
        (int)(player_breath_hold_time->current.value * 1000.0f) <= 0)
        return 0.0f;
    weapDef = BG_GetPlayerWeaponDef(ps, BG_GetViewmodelWeaponIndex(ps));
    if (!weapDef->overlayReticle || weapDef->weapClass == 9)
        return 0.0f;
    return ps->holdBreathTimer > 0 ? BREATH_PROMPT_RECOVERING_ALPHA : 1.0f;
}

/* The first key of the first bound command, named as the controls menu names
 * it, in capitals. */
static int CG_BreathPromptKey(char *key)
{
    char bindings[2][128];
    const char *name;
    int i, c;

    Controls_GetConfig();
    for (i = 0; i < (int)(sizeof(s_breathPromptCommands) / sizeof(s_breathPromptCommands[0])); i++) {
        if (!GetKeyBindings(s_breathPromptCommands[i], bindings))
            continue;
        name = SEH_StringEd_GetString(bindings[0]);
        I_strncpyz(key, name ? name : bindings[0], BREATH_PROMPT_KEY_LEN);
        for (c = 0; key[c]; c++) {
            if (key[c] >= 'a' && key[c] <= 'z')
                key[c] = (char)(key[c] - 'a' + 'A');
        }
        return 1;
    }
    return 0;
}

void CG_DrawBreathPrompt(void)
{
    float alpha = CG_BreathPromptAlpha(&cg->predictedPlayerState);
    char key[BREATH_PROMPT_KEY_LEN];
    FontHandle font;
    float nameWidth, width, left;
    vec4_t color;

    if (alpha <= 0.0f)
        return;
    font = CG_GetPlayerNameFont();
    if (!font || !CG_BreathPromptKey(key))
        return;
    nameWidth = (float)CL_TextWidth(key, 0, font) * CL_NormalizedTextScale(font, BREATH_PROMPT_KEY_SCALE);
    width = nameWidth + 2.0f * BREATH_PROMPT_KEY_PAD;
    if (width < BREATH_PROMPT_KEY_HEIGHT)
        width = BREATH_PROMPT_KEY_HEIGHT;
    left = BREATH_PROMPT_KEY_RIGHT - width;

    CG_BreathPromptColor(color, s_breathPromptShadow, alpha);
    CL_DrawStretchPic(left + BREATH_PROMPT_SHADOW_OFFSET, BREATH_PROMPT_KEY_TOP + BREATH_PROMPT_SHADOW_OFFSET, width,
                      BREATH_PROMPT_KEY_HEIGHT, BREATH_PROMPT_ALIGN, BREATH_PROMPT_ALIGN, 0.0f, 0.0f, 1.0f, 1.0f, color,
                      cgs->media.whiteMaterial);
    CG_BreathPromptColor(color, s_breathPromptKeycap, alpha);
    CL_DrawStretchPic(left, BREATH_PROMPT_KEY_TOP, width, BREATH_PROMPT_KEY_HEIGHT, BREATH_PROMPT_ALIGN,
                      BREATH_PROMPT_ALIGN, 0.0f, 0.0f, 1.0f, 1.0f, color, cgs->media.whiteMaterial);
    CG_BreathPromptColor(color, s_breathPromptKeyName, alpha);
    UI_DrawText(key, 0x7fffffff, font, left + 0.5f * (width - nameWidth),
                BREATH_PROMPT_KEY_TOP + BREATH_PROMPT_KEY_BASELINE, BREATH_PROMPT_ALIGN, BREATH_PROMPT_ALIGN,
                BREATH_PROMPT_KEY_SCALE, color, 0);
    CG_BreathPromptColor(color, s_breathPromptText, alpha);
    UI_DrawText(UI_SafeTranslateString("MENU_HOLD_BREATH"), 0x7fffffff, font, BREATH_PROMPT_TEXT_X,
                BREATH_PROMPT_BASELINE, BREATH_PROMPT_ALIGN, BREATH_PROMPT_ALIGN, BREATH_PROMPT_SCALE, color,
                BREATH_PROMPT_STYLE);
}
