#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_minimap.h"
#include "headers/PC/cgame_mp/cg_killcam.h"
#include "headers/PC/cgame_mp/cg_scorepanel.h"
#include "headers/PC/cgame_mp/cg_hitmarker.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern float floorf(float x);

extern int CL_GetKeyCatchers(void);
extern Bool CL_GetDisplayHUDWithKeycatchUI(void);
extern float CL_NormalizedTextScale(FontHandle font, float fontScale);
extern void CalcScreenPlacement(float *x, float *y, float *scaleX, float *scaleY, int horzAlign, int vertAlign);
extern void CL_DrawTextPhysical(const char *text, int maxChars, FontHandle font, float x, float y, float scaleX, float scaleY, vec4_t *color, int style);
extern void CL_DrawStretchPicPhysical(float x, float y, float w, float h, float s0, float t0, float s1, float t1, vec4_t *color, void *material);
extern void CG_DrawRotatedPicPhysical(float x, float y, float w, float h, float angle, vec4_t *color, void *material);

extern void CL_DrawSprite(void *material, byte *rgbaColor, const vec_t *pos, float radius, float minScreenRadius, int renderFxFlags);
extern FontHandle UI_GetFontHandle(int scriptIndex, float fontScale);
extern int UI_TextWidth(const char *text, int maxChars, FontHandle font, float fontScale);
extern int UI_TextHeight(FontHandle font, float fontScale);
extern void CG_SafeTranslateHudElemString(int label, char *dst);
extern int CG_ServerMaterialName(int materialIndex, char *buf, int bufSize);
extern void *CL_RegisterMaterial(const char *name, int imageTrack);
extern void *CL_RegisterMaterialNoMip(const char *name, int imageTrack);
extern const char *CL_GetConfigString(int index);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void I_strncat(char *dest, int maxlen, const char *src);
extern int I_stricmp(const char *s1, const char *s2);
extern const char *va(const char *format, ...);
extern float AngleNormalize360(float angle);

Bool CG_AreHudElemsHidden(void);
float CG_AlignHudElemX(int alignOrg, float x, float width);
float CG_AlignHudElemY(int alignOrg, float y, float height);
int compare_hudelems(const inflate_huft *pe0, const inflate_huft *pe1);
static void CG_DrawHudElemString(const char *text, const hudelem_t *elem, cg_hudelem_t *cghe);
static void CG_GetHudElemInfo(const hudelem_t *elem, cg_hudelem_t *cghe, char *hudElemString);
void CG_Draw2dHudElems(qboolean foreground);
void CG_Draw3dHudElems(void);

static inline __attribute__((always_inline)) char *cg_base(void)
{
    return (char *)cg;
}

static int cg_time(void)
{
    return cg->time;
}

static char *cg_snap(void)
{
    return (char *)cg->snap;
}

static float clamp01(float val)
{
    if (val < 0.0f)
        return 0.0f;
    if (val > 1.0f)
        return 1.0f;
    return val;
}

static float CG_GetDimValue(int dimVal, float fontHeight)
{
    if (dimVal == 0)
        return fontHeight;
    return (float)dimVal;
}

static float CG_GetMoveLerpFactor(const hudelem_t *elem)
{
    int moveTime = elem->moveTime;
    if (moveTime <= 0)
        return 1.0f;
    int serverTime = cg_time();
    int elapsed = serverTime - elem->moveStartTime;
    if (elapsed <= 0)
        return 0.0f;
    if (moveTime > elapsed) {
        float frac = (float)elapsed / (float)moveTime;
        return frac;
    }
    return 1.0f;
}

static float CG_AlignX(int alignOrg, float x, float width)
{
    int xAlign = (alignOrg >> 2) & 3;
    switch (xAlign) {
    case 1:
        return x + width * -0.5f;
    case 2:
        return x - width;
    default:
        return x;
    }
}

static float CG_AlignY(int alignOrg, float y, float height)
{
    int yAlign = alignOrg & 3;
    switch (yAlign) {
    case 1:
        return y + height * -0.5f;
    case 2:
        return y - height;
    default:
        return y;
    }
}

Bool CG_AreHudElemsHidden(void)
{
    char *base = cg_base();
    int val = ((cg_t *)base)->drawHud;
    if (!val) {
        return 1;
    }
    if (!(CL_GetKeyCatchers() & 8)) {
        return 0;
    }
    if (!CL_GetDisplayHUDWithKeycatchUI())
        return 1;
    return 0;
}

float CG_AlignHudElemX(int alignOrg, float x, float width)
{
    int xAlign = (alignOrg >> 2) & 3;
    switch (xAlign) {
    case 1:
        return x + width * -0.5f;
    case 2:
        return x - width;
    default:
        return x;
    }
}

float CG_AlignHudElemY(int alignOrg, float y, float height)
{
    int yAlign = alignOrg & 3;
    switch (yAlign) {
    case 1:
        return y + height * -0.5f;
    case 2:
        return y - height;
    default:
        return y;
    }
}

int compare_hudelems(const inflate_huft *pe0, const inflate_huft *pe1)
{
    const hudelem_t *e0 = *(const hudelem_t **)pe0;
    const hudelem_t *e1 = *(const hudelem_t **)pe1;
    float delta = e0->sort - e1->sort;
    if (delta < 0.0f) {
        return -1;
    }
    if (delta > 0.0f) {
        return 1;
    }
    return 0;
}

static const char *CG_FormatTimerString(const hudelem_t *elem, int isTenths)
{
    int serverTime = cg_time();
    int msec;
    int hours, minutes, seconds, tenths;

    switch (elem->type) {
    case HE_TYPE_TIMER_DOWN:
    case HE_TYPE_TENTHS_TIMER_DOWN:
        msec = elem->time - serverTime + 999;
        break;
    case HE_TYPE_CLOCK_DOWN:
        msec = elem->time - serverTime + 99;
        break;
    case HE_TYPE_TIMER_UP:
    case HE_TYPE_TENTHS_TIMER_UP:
        msec = serverTime - elem->time;
        break;
    case HE_TYPE_CLOCK_UP:
        msec = serverTime - elem->time;
        break;
    default:
        return NULL;
    }

    if (isTenths) {

        if (msec < 0) {
            hours = 0;
            minutes = 0;
            seconds = 0;
            tenths = 0;
        } else {
            int total = msec / 100;
            hours = total / 36000;
            int rem = total - hours * 36000;
            minutes = rem / 600;
            rem = rem - minutes * 600;
            seconds = rem / 10;
            tenths = rem - seconds * 10;
        }
        if (hours) {
            return va("%i:%02i:%02i.%i", hours, minutes, seconds, tenths);
        } else {
            return va("%i:%02i.%i", minutes, seconds, tenths);
        }
    } else {

        if (msec < 0) {
            hours = 0;
            minutes = 0;
            seconds = 0;
        } else {
            int total = msec / 1000;
            hours = total / 3600;
            int rem = total - hours * 3600;
            minutes = rem / 60;
            seconds = rem - minutes * 60;
        }
        if (hours) {
            return va("%i:%02i:%02i", hours, minutes, seconds);
        } else {
            return va("%i:%02i", minutes, seconds);
        }
    }
}

static float CG_GetWidth(const hudelem_t *elem, float fontHeight)
{
    float width = CG_GetDimValue(elem->width, fontHeight);
    int scaleTime = elem->scaleTime;
    if (scaleTime > 0) {
        int serverTime = cg_time();
        int elapsed = serverTime - elem->scaleStartTime;
        if (scaleTime > elapsed) {
            float toWidth = CG_GetDimValue(elem->fromWidth, fontHeight);
            float lerp = (float)elapsed / (float)scaleTime;
            lerp = clamp01(lerp);
            width = (width - toWidth) * lerp + toWidth;
        }
    }
    return width;
}

static float CG_GetHeight(const hudelem_t *elem, float fontHeight)
{
    float height = CG_GetDimValue(elem->height, fontHeight);
    int scaleTime = elem->scaleTime;
    if (scaleTime > 0) {
        int serverTime = cg_time();
        int elapsed = serverTime - elem->scaleStartTime;
        if (scaleTime > elapsed) {
            float toHeight = CG_GetDimValue(elem->fromHeight, fontHeight);
            float lerp = (float)elapsed / (float)scaleTime;
            lerp = clamp01(lerp);
            height = (height - toHeight) * lerp + toHeight;
        }
    }
    return height;
}

static void CG_DrawHudElemString(const char *text, const hudelem_t *elem, cg_hudelem_t *cghe)
{
    if (!text || !text[0] || !cghe->font) {
        return;
    }

    float negHeight = -(cghe->height - cghe->fontHeight);

    float moveLerp = CG_GetMoveLerpFactor(elem);
    float yPos;
    if (moveLerp == 1.0f) {
        yPos = cghe->y;
        yPos = CG_AlignY(elem->alignOrg, yPos, negHeight);
    } else {

        float fromY = cghe->y;
        float toY = cghe->y;

        int fromYAlign = elem->fromAlignOrg & 3;
        if (fromYAlign == 1)
            fromY = fromY + negHeight * -0.5f;
        else if (fromYAlign == 2)
            fromY = fromY - negHeight;

        int yAlign = elem->alignOrg & 3;
        if (yAlign == 1)
            toY = toY + negHeight * -0.5f;
        else if (yAlign == 2)
            toY = toY - negHeight;

        yPos = (toY - fromY) * moveLerp + fromY;
    }

    float scaleX, scaleY;
    float scale = CL_NormalizedTextScale(cghe->font, cghe->fontScale);
    scaleX = scale;
    scaleY = scale;

    float dummyX = 0.0f;
    float dummyY = 0.0f;
    int alignScreen = elem->alignScreen;
    CalcScreenPlacement(&dummyX, &dummyY, &scaleX, &scaleY, (alignScreen >> 3) & 7, alignScreen & 7);

    float dx = 0.0f;
    float dy = cghe->fontHeight;
    CalcScreenPlacement(&dummyX, &dummyY, &dx, &dy, (alignScreen >> 3) & 7, alignScreen & 7);

    CL_DrawTextPhysical(text, 0x7fffffff, cghe->font, cghe->x, yPos + dy, scaleX, scaleY, &cghe->color, 3);
}

/* fadeOverTime stores the starting color in fromColor and the script then sets
 * the target color, so a fade runs from fromColor to color. */
static void CG_GetHudElemColor(const hudelem_t *elem, vec4_t color)
{
    int serverTime = cg_time();
    int fadeElapsed = serverTime - elem->fadeStartTime;
    int fadeTime = elem->fadeTime;
    byte *toColor = (byte *)&elem->color;

    if (fadeTime > 0 && fadeTime > fadeElapsed) {

        float lerp;
        if (fadeElapsed <= 0) {
            lerp = 0.0f;
        } else {
            lerp = (float)fadeElapsed / (float)fadeTime;
        }

        byte *fromColor = (byte *)&elem->fromColor;

        color[0] = ((float)(int)fromColor[0] + ((float)(int)toColor[0] - (float)(int)fromColor[0]) * lerp) * (1.0f / 255.0f);
        color[1] = ((float)(int)fromColor[1] + ((float)(int)toColor[1] - (float)(int)fromColor[1]) * lerp) * (1.0f / 255.0f);
        color[2] = ((float)(int)fromColor[2] + ((float)(int)toColor[2] - (float)(int)fromColor[2]) * lerp) * (1.0f / 255.0f);
        color[3] = ((float)(int)fromColor[3] + ((float)(int)toColor[3] - (float)(int)fromColor[3]) * lerp) * (1.0f / 255.0f);
    } else {
        color[0] = (float)(int)toColor[0] * (1.0f / 255.0f);
        color[1] = (float)(int)toColor[1] * (1.0f / 255.0f);
        color[2] = (float)(int)toColor[2] * (1.0f / 255.0f);
        color[3] = (float)(int)toColor[3] * (1.0f / 255.0f);
    }
}

/* The text and color a timer element would be drawn with, or NULL for any
 * other element. */
const char *CG_GetHudElemTimer(const hudelem_t *elem, vec4_t color)
{
    if (elem->type < HE_TYPE_TIMER_DOWN || elem->type > HE_TYPE_TENTHS_TIMER_UP)
        return NULL;
    CG_GetHudElemColor(elem, color);
    return CG_FormatTimerString(elem, elem->type >= HE_TYPE_TENTHS_TIMER_DOWN);
}

static void CG_GetHudElemInfo(const hudelem_t *elem, cg_hudelem_t *cghe, char *hudElemString)
{
    const char *strResult;
    int fontIndex;

    cghe->font = 0;
    cghe->fontHeight = 0.0f;

    switch (elem->font) {
    case 0:
        cghe->fontScale = 0.25f * elem->fontScale;
        fontIndex = 0;
        break;
    case 1:
        cghe->fontScale = 0.5f * elem->fontScale;
        fontIndex = 4;
        break;
    case 2:
        cghe->fontScale = 0.3333333f * elem->fontScale;
        fontIndex = 5;
        break;
    default:
        cghe->fontScale = 0.25f * elem->fontScale;
        fontIndex = 0;
        break;
    }
    cghe->font = UI_GetFontHandle(fontIndex, cghe->fontScale);
    if (!cghe->font && fontIndex != 0) {
        cghe->font = UI_GetFontHandle(0, cghe->fontScale);
    }
    if (cghe->font) {
        cghe->fontHeight = (float)UI_TextHeight(cghe->font, cghe->fontScale);
    }

    cghe->hudElemLabel[0] = 0;

    if (elem->label) {
        CG_SafeTranslateHudElemString(elem->label, cghe->hudElemLabel);
    }

    cghe->hudElemText[0] = 0;
    strResult = NULL;

    switch (elem->type) {
    case HE_TYPE_TEXT:

        if (elem->text) {
            CG_SafeTranslateHudElemString(elem->text, cghe->hudElemText);
        }
        break;
    case HE_TYPE_VALUE: {

        double val = (double)elem->value;
        strResult = va("%g", val);
        break;
    }
    case HE_TYPE_PLAYERNAME: {

        int idx = (int)(floorf(elem->value + 0.5f));
        if (idx > 63)
            break;

        char *snap = cg_snap();
        I_strncpyz(cghe->hudElemText, (char *)snap + 0xe0920 + idx * 1208, 256);
        break;
    }
    case HE_TYPE_MAPNAME:
        strResult = CL_GetConfigString(0x14);
        break;
    case HE_TYPE_GAMETYPE:
        strResult = CL_GetConfigString(0x13);
        break;
    case HE_TYPE_MATERIAL:
        break;
    case HE_TYPE_TIMER_DOWN:
    case HE_TYPE_TIMER_UP:
        strResult = CG_FormatTimerString(elem, 0);
        break;
    case HE_TYPE_TENTHS_TIMER_DOWN:
    case HE_TYPE_TENTHS_TIMER_UP:
        strResult = CG_FormatTimerString(elem, 1);
        break;
    case HE_TYPE_CLOCK_DOWN:
    case HE_TYPE_CLOCK_UP:
        strResult = CG_FormatTimerString(elem, 0);
        break;
    default:
        break;
    }

    if (strResult) {
        I_strncpyz(cghe->hudElemText, strResult, 256);
    }

    if (cghe->hudElemLabel[0] && cghe->hudElemText[0]) {

        int outIdx = 0;
        int labelIdx = 0;

        while (outIdx < 255) {
            char ch = cghe->hudElemLabel[labelIdx];
            if (ch == 0)
                break;
            if (ch == '&') {
                if (cghe->hudElemLabel[labelIdx + 1] == '&' && cghe->hudElemLabel[labelIdx + 2] == '1') {
                    labelIdx += 3;
                    break;
                }
            }
            hudElemString[outIdx++] = ch;
            labelIdx++;
        }

        if (outIdx <= 254) {
            int textIdx = 0;
            while (outIdx < 255 && cghe->hudElemText[textIdx]) {
                hudElemString[outIdx++] = cghe->hudElemText[textIdx++];
            }
        }

        if (outIdx <= 254) {
            while (outIdx < 255 && cghe->hudElemLabel[labelIdx]) {
                hudElemString[outIdx++] = cghe->hudElemLabel[labelIdx++];
            }
        }

        if (outIdx > 255)
            outIdx = 255;
        hudElemString[outIdx] = 0;

        memcpy(cghe->hudElemText, hudElemString, 256);

        cghe->textWidth = (float)UI_TextWidth(cghe->hudElemText, 0, cghe->font, cghe->fontScale);
        cghe->hudElemLabel[0] = 0;
        cghe->labelWidth = 0.0f;
    } else if (cghe->hudElemLabel[0] && !cghe->hudElemText[0]) {

        cghe->labelWidth = (float)UI_TextWidth(cghe->hudElemLabel, 0, cghe->font, cghe->fontScale);
    }

    if (cghe->hudElemText[0]) {
        cghe->textWidth = (float)UI_TextWidth(cghe->hudElemText, 0, cghe->font, cghe->fontScale);
    } else {
        cghe->textWidth = 0.0f;
    }

    {
        int typeMask;
        float elemWidth = 0.0f;

        if (elem->type <= 13) {
            typeMask = 1 << elem->type;
        } else {
            typeMask = 0;
        }

        if (typeMask & 0x7be) {

            elemWidth = cghe->labelWidth + cghe->textWidth;
        } else if (typeMask & 0x1840) {

            float w = CG_GetDimValue(elem->width, cghe->fontHeight);
            w = CG_GetWidth(elem, cghe->fontHeight);
            elemWidth = w + cghe->labelWidth;
        } else if (typeMask & 0x2000) {

            elemWidth = 0.0f;
        } else if (typeMask & 0x2040) {

            elemWidth = (float)elem->height;
        } else if (typeMask & 0x1800) {

            float h = CG_GetDimValue(elem->height, cghe->fontHeight);
            float hResult = h;
            int scaleTime = elem->scaleTime;
            if (scaleTime > 0) {
                int serverTime = cg_time();
                int elapsed = serverTime - elem->scaleStartTime;
                if (scaleTime > elapsed) {
                    float toH = CG_GetDimValue(elem->fromHeight, cghe->fontHeight);
                    float lerp = (float)elapsed / (float)scaleTime;
                    lerp = clamp01(lerp);
                    hResult = (h - toH) * lerp + toH;
                }
            }
            elemWidth = hResult;
        } else {
            elemWidth = 0.0f;
        }

        cghe->width = elemWidth;
    }

    {
        int typeMask;
        float elemHeight = 0.0f;

        if (elem->type <= 13) {
            typeMask = 1 << elem->type;
        } else {
            typeMask = 0;
        }

        if (typeMask & 0x7be) {

            elemHeight = cghe->fontHeight;
            if (cghe->fontHeight > elemHeight)
                elemHeight = cghe->fontHeight;
        } else if (typeMask & 0x2040) {

            elemHeight = (float)elem->height;
        } else if (typeMask & 0x1800) {

            float h = CG_GetDimValue(elem->height, cghe->fontHeight);
            float hResult = h;
            int scaleTime = elem->scaleTime;
            if (scaleTime > 0) {
                int serverTime = cg_time();
                int elapsed = serverTime - elem->scaleStartTime;
                if (scaleTime > elapsed) {
                    float toH = CG_GetDimValue(elem->fromHeight, cghe->fontHeight);
                    float lerp = (float)elapsed / (float)scaleTime;
                    lerp = clamp01(lerp);
                    hResult = (h - toH) * lerp + toH;
                }
            }
            elemHeight = hResult;
        } else {
            elemHeight = 0.0f;
        }

        cghe->height = elemHeight;
    }

    cghe->z = elem->z;

    {
        float moveLerp = CG_GetMoveLerpFactor(elem);

        if (moveLerp == 1.0f) {

            float x = elem->x;
            float y = elem->y;
            float width = cghe->width;
            float height = cghe->height;

            x = CG_AlignX(elem->alignOrg, x, width);
            y = CG_AlignY(elem->alignOrg, y, height) + CG_MinimapHudElemOffset(elem->alignScreen, elem->x);

            cghe->x = x;
            cghe->y = y;

            float dummyWidth = 0.0f;
            float dummyHeight = 0.0f;
            int alignScreen = elem->alignScreen;
            CalcScreenPlacement(&cghe->x, &cghe->y, &dummyWidth, &dummyHeight, (alignScreen >> 3) & 7, alignScreen & 7);
        } else {

            float width = cghe->width;
            float height = cghe->height;

            float fromX = elem->fromX;
            float fromY = elem->fromY;
            fromX = CG_AlignX(elem->fromAlignOrg, fromX, width);
            fromY = CG_AlignY(elem->fromAlignOrg, fromY, height) + CG_MinimapHudElemOffset(elem->fromAlignScreen, elem->fromX);

            float dummyWidth = 0.0f;
            float dummyHeight = 0.0f;
            int fromAlignScreen = elem->fromAlignScreen;
            CalcScreenPlacement(&fromX, &fromY, &dummyWidth, &dummyHeight, (fromAlignScreen >> 3) & 7, fromAlignScreen & 7);

            float toX = elem->x;
            float toY = elem->y;
            toX = CG_AlignX(elem->alignOrg, toX, width);
            toY = CG_AlignY(elem->alignOrg, toY, height) + CG_MinimapHudElemOffset(elem->alignScreen, elem->x);

            dummyWidth = 0.0f;
            dummyHeight = 0.0f;
            int alignScreen = elem->alignScreen;
            CalcScreenPlacement(&toX, &toY, &dummyWidth, &dummyHeight, (alignScreen >> 3) & 7, alignScreen & 7);

            cghe->x = fromX + (toX - fromX) * moveLerp;
            cghe->y = fromY + (toY - fromY) * moveLerp;
        }

        cghe->x = (float)(int)(floorf(cghe->x + 0.5f));
        cghe->y = (float)(int)(floorf(cghe->y + 0.5f));
    }

    CG_GetHudElemColor(elem, cghe->color);
}

#define CG_HUD_LINES_MAX 4

void CG_DrawHudLines(const float lines[][4], int count, float width, float feather, const vec_t *color)
{
    HudVertex2D verts[CG_HUD_LINES_MAX * 16];
    unsigned short indices[CG_HUD_LINES_MAX * 54];
    float alpha = color[3];
    float across[4];
    int vertexCount = 0;
    int indexCount = 0;
    int line;

    /* Thinner than a pixel, a line is a pixel wide and fainter. */
    if (width < 1.0f) {
        alpha *= width;
        width = 1.0f;
    }
    if (alpha <= 0.0f || count > CG_HUD_LINES_MAX || !(feather > 0.0f && feather <= 1.0f))
        return;
    across[0] = (width + feather) * -0.5f;
    across[1] = (width - feather) * -0.5f;
    across[2] = (width - feather) * 0.5f;
    across[3] = (width + feather) * 0.5f;
    for (line = 0; line < count; ++line) {
        float dx = lines[line][2] - lines[line][0];
        float dy = lines[line][3] - lines[line][1];
        float length = sqrtf(dx * dx + dy * dy);
        float along[4];
        int i;
        int j;

        if (length < 1.0f)
            continue;
        dx /= length;
        dy /= length;
        along[0] = -0.5f;
        along[1] = 0.5f;
        along[2] = length - 0.5f;
        along[3] = length + 0.5f;
        /* A grid four vertices across and four along, opaque inside. */
        for (j = 0; j < 4; ++j) {
            for (i = 0; i < 4; ++i) {
                HudVertex2D *vertex = &verts[vertexCount + j * 4 + i];

                memset(vertex, 0, sizeof(*vertex));
                vertex->xy[0] = lines[line][0] + dx * along[j] - dy * across[i];
                vertex->xy[1] = lines[line][1] + dy * along[j] + dx * across[i];
                vertex->rgba[0] = color[0];
                vertex->rgba[1] = color[1];
                vertex->rgba[2] = color[2];
                vertex->rgba[3] = (i == 0 || i == 3 || j == 0 || j == 3) ? 0.0f : alpha;
            }
        }
        for (j = 0; j < 3; ++j) {
            for (i = 0; i < 3; ++i) {
                unsigned short corner = (unsigned short)(vertexCount + j * 4 + i);

                indices[indexCount++] = corner;
                indices[indexCount++] = (unsigned short)(corner + 1);
                indices[indexCount++] = (unsigned short)(corner + 5);
                indices[indexCount++] = corner;
                indices[indexCount++] = (unsigned short)(corner + 5);
                indices[indexCount++] = (unsigned short)(corner + 4);
            }
        }
        vertexCount += 16;
    }
    if (indexCount)
        R_AddCmdDrawTriangles2D(verts, vertexCount, indices, indexCount, cgs->media.whiteMaterial);
}

/* The gametype scripts confirm each hit by showing the damage_feedback X in a
 * 24-unit box at screen center and fading it out over a second. Draw CoD
 * WWII's instead, measured on its captures in units of that box: four thin
 * lines on the diagonals, white for a hit and longer and red for the hit that
 * kills, shown only briefly. */
#define CG_HIT_MARKER_MATERIAL "damage_feedback"
#define CG_HIT_MARKER_BOX 24.0f
#define CG_HIT_MARKER_INNER 5.8f
#define CG_HIT_MARKER_OUTER 13.0f
#define CG_HIT_MARKER_KILL_OUTER 14.6f
#define CG_HIT_MARKER_WIDTH 0.85f
#define CG_HIT_MARKER_HOLD_MS 80
#define CG_HIT_MARKER_FADE_MS 220
/* The scripts show the marker in the server frame of the obituary, but a
 * snapshot that is late may bring the obituary after it. */
#define CG_HIT_MARKER_KILL_WINDOW_MS 250

static const float s_hitMarkerKillColor[3] = {0.86f, 0.16f, 0.13f};
/* The server time of the snapshot that brought each player's latest kill. */
static int s_hitMarkerKillTime[64];
static int s_hitMarkerKilled[64];

void CG_HitMarkerInit(void)
{
    memset(s_hitMarkerKillTime, 0, sizeof(s_hitMarkerKillTime));
    memset(s_hitMarkerKilled, 0, sizeof(s_hitMarkerKilled));
}

void CG_HitMarkerObituary(int attacker, int target)
{
    if (attacker < 0 || attacker >= 64 || attacker == target || !cg->nextSnap)
        return;
    s_hitMarkerKillTime[attacker] = cg->nextSnap->serverTime;
    s_hitMarkerKilled[attacker] = 1;
}

/* Whether the viewed player's marker shows their latest kill, once the
 * snapshot that brought it is the one drawn. */
static int CG_HitMarkerShowsKill(const hudelem_t *elem)
{
    int viewer = cg->snap->ps.clientNum;
    int killTime;

    if (viewer < 0 || viewer >= 64 || !s_hitMarkerKilled[viewer])
        return 0;
    killTime = s_hitMarkerKillTime[viewer];
    return cg->snap->serverTime - killTime >= 0 && killTime - elem->fadeStartTime >= 0 &&
           killTime - elem->fadeStartTime <= CG_HIT_MARKER_KILL_WINDOW_MS;
}

static void CG_DrawHitMarker(const hudelem_t *elem, float x, float y, float width, float height, float alpha)
{
    static const float diagonals[4][2] = {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};
    vec4_t color = {1.0f, 1.0f, 1.0f, alpha};
    float lines[4][4];
    float unit = height / CG_HIT_MARKER_BOX * 0.70710678f;
    float outer = CG_HIT_MARKER_OUTER;
    float centerX;
    float centerY;
    int fadeElapsed = cg_time() - elem->fadeStartTime;
    int i;

    /* While the script's fade runs the hit is fresh: hold, then fade out early. */
    if (fadeElapsed < elem->fadeTime) {
        fadeElapsed -= CG_HIT_MARKER_HOLD_MS;
        if (fadeElapsed >= CG_HIT_MARKER_FADE_MS)
            return;
        color[3] = fadeElapsed > 0 ? 1.0f - (float)fadeElapsed / (float)CG_HIT_MARKER_FADE_MS : 1.0f;
    }
    if (CG_HitMarkerShowsKill(elem)) {
        color[0] = s_hitMarkerKillColor[0];
        color[1] = s_hitMarkerKillColor[1];
        color[2] = s_hitMarkerKillColor[2];
        outer = CG_HIT_MARKER_KILL_OUTER;
    }

    /* Centered on the box's middle pixel, where the crosshair's lines meet. */
    centerX = floorf(x + width * 0.5f) + 0.5f;
    centerY = floorf(y + height * 0.5f) + 0.5f;
    for (i = 0; i < 4; ++i) {
        lines[i][0] = centerX + diagonals[i][0] * unit * CG_HIT_MARKER_INNER;
        lines[i][1] = centerY + diagonals[i][1] * unit * CG_HIT_MARKER_INNER;
        lines[i][2] = centerX + diagonals[i][0] * unit * outer;
        lines[i][3] = centerY + diagonals[i][1] * unit * outer;
    }
    CG_DrawHudLines(lines, 4, height / CG_HIT_MARKER_BOX * CG_HIT_MARKER_WIDTH, 1.0f, color);
}

void CG_Draw2dHudElems(qboolean foreground)
{
    hudelem_t *elems[64];
    cg_hudelem_t cghe;
    char hudElemString[256];
    hudelem_t *current;
    hudelem_t *archival;
    int elemCount;
    int i;

    byte *ps = (byte *)cg_snap() + 0x0C;
    elemCount = 0;
    current = (hudelem_t *)(ps + 0x7a8);
    archival = current + 31;

    for (i = 0; i < 31; i++) {
        if (current[i].type != HE_TYPE_FREE) {
            elems[elemCount++] = &current[i];
        } else {
            break;
        }
    }

    for (i = 0; i < 31; i++) {
        if (archival[i].type != HE_TYPE_FREE) {
            elems[elemCount++] = &archival[i];
        } else {
            break;
        }
    }

    qsort(elems, elemCount, sizeof(elems[0]), (int (*)(const void *, const void *))compare_hudelems);

    if (elemCount <= 0)
        return;

    for (i = 0; i < elemCount; i++) {
        hudelem_t *elem = elems[i];

        if (elem->foreground != foreground)
            continue;

        if (elem->type == HE_TYPE_WAYPOINT)
            continue;

        if (CG_ScorePanelHidesHudElem(elem))
            continue;
        if (CG_KillcamHidesHudElem(elem))
            continue;

        CG_GetHudElemInfo(elem, &cghe, hudElemString);

        if (cghe.color[3] == 0.0f)
            continue;

        if (cghe.hudElemLabel[0]) {
            CG_DrawHudElemString(cghe.hudElemLabel, elem, &cghe);
            cghe.x += cghe.labelWidth;
        }

        if (elem->type <= 12) {
            int typeMask = 1 << elem->type;

            if (typeMask & 0x7be) {

                if (cghe.hudElemText[0]) {
                    CG_DrawHudElemString(cghe.hudElemText, elem, &cghe);
                }
            } else if (typeMask & 0x40) {

                char materialName[64];
                if (!CG_ServerMaterialName(elem->materialIndex, materialName, 64))
                    continue;
                void *material = CL_RegisterMaterial(materialName, 7);

                float width = CG_GetWidth(elem, cghe.fontHeight);
                float height = CG_GetHeight(elem, cghe.fontHeight);
                float negHeight = -(cghe.height - height);

                float moveLerp = CG_GetMoveLerpFactor(elem);
                float yPos;
                if (moveLerp == 1.0f) {
                    yPos = cghe.y;
                    yPos = CG_AlignY(elem->alignOrg, yPos, negHeight);
                } else {
                    float fromY = cghe.y;
                    float toY = cghe.y;
                    int fromYAlign = elem->fromAlignOrg & 3;
                    if (fromYAlign == 1)
                        fromY += negHeight * -0.5f;
                    else if (fromYAlign == 2)
                        fromY -= negHeight;
                    int yAlign = elem->alignOrg & 3;
                    if (yAlign == 1)
                        toY += negHeight * -0.5f;
                    else if (yAlign == 2)
                        toY -= negHeight;
                    yPos = (toY - fromY) * moveLerp + fromY;
                }

                float dummyX = 0.0f;
                float dummyY = 0.0f;
                int alignScreen = elem->alignScreen;
                CalcScreenPlacement(&dummyX, &dummyY, &width, &height, (alignScreen >> 3) & 7, alignScreen & 7);

                if (!I_stricmp(materialName, CG_HIT_MARKER_MATERIAL))
                    CG_DrawHitMarker(elem, cghe.x, yPos, width, height, cghe.color[3]);
                else
                    CL_DrawStretchPicPhysical(cghe.x, yPos, width, height, 0.0f, 0.0f, 1.0f, 1.0f, &cghe.color, material);
            } else if (typeMask & 0x1800) {

                char materialName[64];
                if (!CG_ServerMaterialName(elem->materialIndex, materialName, 58))
                    continue;

                void *faceMaterial = CL_RegisterMaterialNoMip(materialName, 7);
                I_strncat(materialName, 64, "_hand");
                void *handMaterial = CL_RegisterMaterialNoMip(materialName, 7);

                float timeVal = 0.0f;
                int serverTime = cg_time();
                int msec;

                switch (elem->type) {
                case HE_TYPE_TIMER_DOWN:
                case HE_TYPE_TENTHS_TIMER_DOWN:
                    msec = elem->time - serverTime + 999;
                    if (msec >= 0)
                        timeVal = (float)msec;
                    break;
                case HE_TYPE_CLOCK_DOWN:
                    msec = elem->time - serverTime + 99;
                    if (msec >= 0)
                        timeVal = (float)msec;
                    break;
                case HE_TYPE_TIMER_UP:
                case HE_TYPE_TENTHS_TIMER_UP:
                    msec = serverTime - elem->time;
                    if (msec >= 0)
                        timeVal = (float)msec;
                    break;
                case HE_TYPE_CLOCK_UP:
                    msec = serverTime - elem->time;
                    if (msec >= 0)
                        timeVal = (float)msec;
                    break;
                default:
                    break;
                }

                float angle;
                if (elem->duration != 0) {
                    angle = AngleNormalize360(timeVal * 360.0f / (float)elem->duration);
                } else {
                    angle = AngleNormalize360(timeVal * 0.006f);
                }

                float width = CG_GetWidth(elem, cghe.fontHeight);
                float height = CG_GetHeight(elem, cghe.fontHeight);
                float negHeight = -(cghe.height - height);

                float moveLerp2 = CG_GetMoveLerpFactor(elem);
                float yPos2;
                if (moveLerp2 == 1.0f) {
                    yPos2 = cghe.y;
                    yPos2 = CG_AlignY(elem->alignOrg, yPos2, negHeight);
                } else {
                    float fromY = cghe.y;
                    float toY = cghe.y;
                    int fromYAlign = elem->fromAlignOrg & 3;
                    if (fromYAlign == 1)
                        fromY += negHeight * -0.5f;
                    else if (fromYAlign == 2)
                        fromY -= negHeight;
                    int yAlign = elem->alignOrg & 3;
                    if (yAlign == 1)
                        toY += negHeight * -0.5f;
                    else if (yAlign == 2)
                        toY -= negHeight;
                    yPos2 = (toY - fromY) * moveLerp2 + fromY;
                }

                float dummyY2 = 0.0f;
                float dummyW = 0.0f;
                int alignScreen2 = elem->alignScreen;
                CalcScreenPlacement(&dummyY2, &dummyW, &width, &height, (alignScreen2 >> 3) & 7, alignScreen2 & 7);

                CL_DrawStretchPicPhysical(cghe.x, yPos2, width, height, 0.0f, 0.0f, 1.0f, 1.0f, &cghe.color, faceMaterial);

                CG_DrawRotatedPicPhysical(cghe.x, yPos2, width, height, angle, &cghe.color, handMaterial);
            }
        }
    }

    return;
}

void CG_Draw3dHudElems(void)
{
    hudelem_t *elems[64];
    cg_hudelem_t cghe;
    char hudElemString[256];
    hudelem_t *current;
    hudelem_t *archival;
    int elemCount;
    int i;

    if (CG_AreHudElemsHidden())
        return;

    byte *ps = (byte *)cg_snap() + 0x0C;
    elemCount = 0;
    current = (hudelem_t *)(ps + 0x7a8);
    archival = current + 31;

    for (i = 0; i < 31; i++) {
        if (current[i].type != HE_TYPE_FREE) {
            elems[elemCount++] = &current[i];
        } else {
            break;
        }
    }

    for (i = 0; i < 31; i++) {
        if (archival[i].type != HE_TYPE_FREE) {
            elems[elemCount++] = &archival[i];
        } else {
            break;
        }
    }

    qsort(elems, elemCount, sizeof(elems[0]), (int (*)(const void *, const void *))compare_hudelems);

    if (elemCount <= 0)
        return;

    for (i = 0; i < elemCount; i++) {
        hudelem_t *elem = elems[i];

        if (elem->type != HE_TYPE_WAYPOINT)
            continue;

        CG_GetHudElemInfo(elem, &cghe, hudElemString);

        if (cghe.hudElemLabel[0]) {
            CG_DrawHudElemString(cghe.hudElemLabel, elem, &cghe);
            cghe.x += cghe.labelWidth;
        }

        if (elem->type != HE_TYPE_WAYPOINT)
            continue;

        byte *colorBytes = (byte *)&elem->color;
        if (colorBytes[3] == 0)
            continue;

        char materialName[64];
        if (!CG_ServerMaterialName(elem->materialIndex, materialName, 64))
            continue;

        void *material = CL_RegisterMaterial(materialName, 7);

        float height = CG_GetHeight(elem, cghe.fontHeight);

        if (height == 0.0f)
            continue;

        vec3_t pos;
        pos[0] = elem->x;
        pos[1] = elem->y;
        pos[2] = elem->z;

        float alpha = (float)(int)colorBytes[3] / 255.0f;

        int renderFxFlags = 0;
        if (elem->value > 0.0f) {
            height *= 0.0043f;
            renderFxFlags = 0x3000;
        }

        byte rgbaColor[4];
        rgbaColor[0] = 0xFF;
        rgbaColor[1] = 0xFF;
        rgbaColor[2] = 0xFF;
        rgbaColor[3] = (byte)(int)(floorf(alpha * 255.0f + 0.5f));

        CL_DrawSprite(material, rgbaColor, pos, height, 0.0f, renderFxFlags);
    }

    return;
}

const unsigned char __ZZ25CG_DrawRotatedPicPhysicalfffffPKfP8MaterialE4sign[480] = {
    0x00, 0x00, 0x80, 0xbf, 0x00, 0x00, 0x80, 0xbf, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0xbf,
    0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0xbf, 0x00, 0x00, 0x80, 0x3f,
    0x07, 0x0f, 0x1a, 0x00, 0x07, 0x0f, 0x1a, 0x00, 0x07, 0x0f, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00,
    0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0x91, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00,
    0xde, 0x0f, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0xbc, 0x0f, 0x1a, 0x00, 0x9a, 0x0f, 0x1a, 0x00,
    0x78, 0x0f, 0x1a, 0x00, 0x56, 0x0f, 0x1a, 0x00, 0x34, 0x0f, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00,
    0x40, 0x0e, 0x1a, 0x00, 0x07, 0x0f, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00,
    0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00,
    0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00,
    0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00,
    0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00,
    0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0x40, 0x0e, 0x1a, 0x00, 0x9b, 0x0e, 0x1a, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
