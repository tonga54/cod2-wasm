#!/usr/bin/env python3
"""Run the production "Kill +100" popup: accumulation, pop, fade and layout."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_scorepopup_mp.c').read_text()
header = (root / 'src/headers/PC/game_mp/g_points.h').read_text()
commands = (root / 'src/PC/cgame_mp/cg_servercmds_mp.c').read_text()
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()


def block(text, start_text, declaration=False):
    assert text.count(start_text) == 1, start_text
    start = text.index(start_text)
    brace = text.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    if declaration:
        end = text.index(';', end) + 1
    return text[start:end]


defines = '\n'.join(re.findall(r'^#define SCORE_POPUP_\w+ .*$', source, re.M))
state = block(source, 'typedef struct {\n    int active;', True)
labels = block(source, 'static const char *const s_scorePopupLabels[POINTS_EVENT_COUNT] = {', True)
body = '\n'.join([
    block(source, 'static void CG_ScorePopupPush('),
    block(source, 'static int CG_ScorePopupFrame('),
    block(source, 'void CG_DrawScorePopup(void)'),
])

# The server command reaches the popup and the HUD draws it after the menus.
assert commands.count('if (!strcmp(cmd, "cod2_points")) {\n        if (Cmd_Argc() == 3)\n'
                      '            CG_ScorePopupAdd(atoi(CG_Argv(1)), atoi(CG_Argv(2)));\n        return;\n    }') == 1
assert draw.count('        CG_Draw2dHudElems(1);\n        if (drawHudMenus) {\n            CG_DrawScorePanel();\n'
                  '            CG_DrawKillFeed();\n            CG_DrawScorePopup();\n        }\n    }') == 1
assert main.count('    CG_MinimapInit(cgs->mapname);\n    CG_ScorePopupInit();\n') == 1

support = r'''
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define CG_POINTS_CLIENT_ONLY
''' + header + r'''
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef const float *FontHandle;
typedef struct { struct { int enabled; } current; } dvar_t;
static struct { int time; } cgState;
#define cg (&cgState)
static dvar_t popupDvar = { { 1 } };
static const dvar_t *cg_hudScorePopup = &popupDvar;
static float fontScales[8];
static int fontCount;
FontHandle UI_GetFontHandle(int fontEnum, float scale) {
    assert(fontEnum == 0 && fontCount < 8);
    fontScales[fontCount] = scale;
    return &fontScales[fontCount++];
}
int UI_TextWidth(const char *text, int maxChars, FontHandle font, float scale) {
    assert(maxChars == 0);
    return (int)(strlen(text) * 20.0f * scale);
}
static struct { char text[32]; float fontScale, x, y, scale, alpha; int horz, vert, style; } drawn[4];
static int drawCount;
void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign,
                 float scale, const vec_t *color, int style) {
    assert(drawCount < 4 && maxChars == 0x7fffffff);
    snprintf(drawn[drawCount].text, sizeof(drawn[0].text), "%s", text);
    drawn[drawCount].fontScale = *font;
    drawn[drawCount].x = x; drawn[drawCount].y = y; drawn[drawCount].scale = scale; drawn[drawCount].alpha = color[3];
    drawn[drawCount].horz = horzAlign; drawn[drawCount].vert = vertAlign; drawn[drawCount].style = style;
    ++drawCount;
}
char *va(const char *fmt, ...) {
    static char buffer[64]; va_list ap;
    va_start(ap, fmt); vsnprintf(buffer, sizeof(buffer), fmt, ap); va_end(ap);
    return buffer;
}
''' + defines + '\n' + state + '\n' + labels + '\nstatic CG_ScorePopupState s_scorePopup;\n'

checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 1e-4f; }
static void frame(CG_ScorePopupState *s, int now, int visible, float scale, float alpha) {
    float gotScale = -1.0f, gotAlpha = -1.0f;
    int shown = CG_ScorePopupFrame(s, now, &gotScale, &gotAlpha);
    if (shown != visible || (visible && (!near(gotScale, scale) || !near(gotAlpha, alpha)))) {
        fprintf(stderr, "t=%d shown=%d scale=%f alpha=%f\n", now, shown, gotScale, gotAlpha);
        assert(0);
    }
}
int main(void) {
    CG_ScorePopupState s = { 0 };
    static const char *expected[POINTS_EVENT_COUNT] = {
        "Kill", "Kill", "Headshot", "Assist", "Flag Captured", "Flag Returned", "Bomb Planted", "Bomb Defused" };
    for (int i = 0; i < POINTS_EVENT_COUNT; ++i) assert(!strcmp(s_scorePopupLabels[i], expected[i]));

    /* Pops in big, settles to its size, holds, then fades out. */
    CG_ScorePopupPush(&s, POINTS_KILL, 100, 1000);
    frame(&s, 1000, 1, 1.45f, 0.0f);
    frame(&s, 1030, 1, 1.0f + 0.45f * (110.0f / 140.0f) * (110.0f / 140.0f), 0.5f);
    frame(&s, 1070, 1, 1.0f + 0.45f * 0.25f, 1.0f);
    frame(&s, 1140, 1, 1.0f, 1.0f);
    frame(&s, 2250, 1, 1.0f, 1.0f);
    frame(&s, 2425, 1, 1.0f, 0.5f);
    frame(&s, 2599, 1, 1.0f, 1.0f / 350.0f);
    frame(&s, 2600, 0, 0, 0);
    assert(!s.active);

    /* Points that arrive while it is up add up and replay the pop. */
    CG_ScorePopupPush(&s, POINTS_KILL, 100, 3000);
    CG_ScorePopupPush(&s, POINTS_HEADSHOT, 150, 4500);
    assert(s.total == 250 && s.event == POINTS_HEADSHOT && s.startTime == 4500);
    frame(&s, 4500, 1, 1.45f, 0.0f);
    CG_ScorePopupPush(&s, POINTS_ASSIST, 25, 6100);
    assert(s.total == 25 && s.event == POINTS_ASSIST);

    /* A new map restarts the clock: a popup from the old one is gone. */
    frame(&s, 100, 0, 0, 0);
    CG_ScorePopupPush(&s, POINTS_ASSIST, 25, 6100);
    CG_ScorePopupPush(&s, POINTS_BOMB_PLANT, 500, 200);
    assert(s.total == 500 && s.startTime == 200);

    /* Unknown events and empty awards are ignored. */
    CG_ScorePopupPush(&s, POINTS_EVENT_COUNT, 100, 300);
    CG_ScorePopupPush(&s, -1, 100, 300);
    CG_ScorePopupPush(&s, POINTS_KILL, 0, 300);
    assert(s.total == 500 && s.event == POINTS_BOMB_PLANT && s.startTime == 200);

    /* Layout: "Headshot +250" centred left of the crosshair, glyphs from the
     * resting font size while the text scales. */
    s_scorePopup = (CG_ScorePopupState){ 0 };
    CG_ScorePopupPush(&s_scorePopup, POINTS_KILL, 100, 10000);
    CG_ScorePopupPush(&s_scorePopup, POINTS_HEADSHOT, 150, 10500);
    cgState.time = 10570;
    CG_DrawScorePopup();
    assert(drawCount == 2);
    {
        float scale = 1.0f + 0.45f * 0.25f;
        float labelWidth = (float)(int)(8 * 20.0f * (0.42f * scale));
        float pointsWidth = (float)(int)(4 * 20.0f * (0.34f * scale));
        float x = -128.0f - (labelWidth + 5.0f * scale + pointsWidth) * 0.5f;
        assert(!strcmp(drawn[0].text, "Headshot") && !strcmp(drawn[1].text, "+250"));
        assert(fontCount == 1 && near(drawn[0].fontScale, 0.42f) && near(drawn[1].fontScale, 0.42f));
        assert(near(drawn[0].scale, 0.42f * scale) && near(drawn[1].scale, 0.34f * scale));
        assert(near(drawn[0].x, x) && near(drawn[1].x, x + labelWidth + 5.0f * scale));
        assert(near(drawn[0].y, 70.0f) && near(drawn[1].y, 70.0f));
        for (int i = 0; i < 2; ++i)
            assert(drawn[i].horz == 2 && drawn[i].vert == 2 && drawn[i].style == 3 && near(drawn[i].alpha, 1.0f));
    }
    drawCount = 0;
    cgState.time = 10500 + 1425;
    CG_DrawScorePopup();
    assert(drawCount == 2 && near(drawn[0].alpha, 0.5f) && near(drawn[1].alpha, 0.5f));
    drawCount = 0;
    cgState.time = 12100;
    CG_DrawScorePopup();
    assert(drawCount == 0);
    CG_ScorePopupPush(&s_scorePopup, POINTS_KILL, 100, 12100);
    popupDvar.current.enabled = 0;
    CG_DrawScorePopup();
    assert(drawCount == 0);
    return 0;
}
'''


def mutate(old, new):
    assert body.count(old) + support.count(old) == 1, old
    return (support.replace(old, new), body) if old in support else (support, body.replace(old, new))


mutants = [
    mutate('        state->total += points;', '        state->total = points;'),
    mutate('if (state->active && now >= state->startTime && now - state->startTime < SCORE_POPUP_MSEC)',
           'if (state->active && now - state->startTime < SCORE_POPUP_MSEC)'),
    mutate('if (!state->active || elapsed < 0 || elapsed >= SCORE_POPUP_MSEC)', 'if (!state->active || elapsed >= SCORE_POPUP_MSEC)'),
    mutate('settle * settle', 'settle'),
    mutate('#define SCORE_POPUP_START_SCALE 1.45f', '#define SCORE_POPUP_START_SCALE 1.0f'),
    mutate('else if (elapsed > SCORE_POPUP_MSEC - SCORE_POPUP_FADE_OUT_MSEC)', 'else if (0)'),
    mutate('    if (elapsed < SCORE_POPUP_FADE_IN_MSEC)\n        *alpha = (float)elapsed / (float)SCORE_POPUP_FADE_IN_MSEC;\n    else ',
           '    '),
    mutate('if (event < 0 || event >= POINTS_EVENT_COUNT || points <= 0)', 'if (event < 0 || points <= 0)'),
    mutate('font = UI_GetFontHandle(0, SCORE_POPUP_LABEL_SCALE);', 'font = UI_GetFontHandle(0, SCORE_POPUP_LABEL_SCALE * scale);'),
    mutate('pointsScale = SCORE_POPUP_POINTS_SCALE * scale;', 'pointsScale = SCORE_POPUP_POINTS_SCALE;'),
    mutate('x = SCORE_POPUP_X - width * 0.5f;', 'x = SCORE_POPUP_X;'),
    mutate('    color[3] = alpha;\n', ''),
    mutate('if (!cg_hudScorePopup || !cg_hudScorePopup->current.enabled)', 'if (!cg_hudScorePopup)'),
    mutate('#define SCORE_POPUP_X -128.0f', '#define SCORE_POPUP_X 0.0f'),
]
with tempfile.TemporaryDirectory(prefix='cod2-popup-') as directory:
    path = Path(directory) / 'popup.c'
    binary = Path(directory) / 'popup'
    for index, (support_source, popup_source) in enumerate([(support, body)] + mutants):
        path.write_text(support_source + popup_source + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        str(path), '-o', str(binary), '-lm'], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: the points popup pops in at 1.45x, settles in 140 ms, fades out over the last 350 ms of 1.6 s, adds up '
      'points that land while it is up, drops popups from the previous map, ignores unknown events, and draws '
      f'"Headshot +250" centred left of the crosshair from the resting font; {len(mutants)} failing regression mutants')
