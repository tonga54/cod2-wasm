#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "headers/PC/cgame_mp/cg_local.h"

#include <string.h>

extern float fabsf(float x);
extern float sinf(float x);
extern float cosf(float x);
extern double atof(const char *s);

static vec4_t color;
static vec2_t sign[4] = {
                          { -1.0f, -1.0f },
                          { 1.0f, -1.0f },
                          { 1.0f, 1.0f },
                          { -1.0f, 1.0f }
};

extern char **compPointerConfig;
extern const vec_t *colorWhite;

extern FontHandle UI_GetFontHandle(int type, float scale);
extern void UI_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float fontScale, const vec_t *color, int style);
extern int CL_TextWidth(const char *text, int maxChars, FontHandle font);
extern int CL_TextHeight(FontHandle font);
extern void CL_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, int horzAlign, int vertAlign, float xScale, float yScale, const vec_t *color, int style);
extern const char *CL_GetConfigString(int index);
extern float AngleNormalize360(float angle);
extern float AngleNormalize180(float angle);
extern float AngleSubtract(float a1, float a2);
extern void CL_SetFullScreenViewport(void);
extern void CL_SetViewport(int x, int y, int width, int height);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern void CL_DrawQuadPic(vec2_t *verts, const vec_t *color, MaterialHandle material);

void CG_DrawStringExt(float x, float y, const char *string, const vec_t *setColor, qboolean forceColor, qboolean shadow, float charHeight, qboolean adjust);
int CG_DrawDevString(float x, float y, const char *s, const vec_t *color, int align, FontHandle font, int style);
int CG_DrawBigDevStringColor(float x, float y, const char *s, const vec_t *color, int align);
int CG_DrawSmallDevStringColor(float x, float y, const char *s, const vec_t *color, int align);
float *CG_FadeColor(int startMsec, int totalMsec, int fadeMsec);
void CG_NorthDirectionChanged(void);
void CG_UpdateCompassOrientation(void);
void CG_UpdateCompPointerOrientation(void);
void CG_TileClear(void);
int CG_DrawBigDevString(float x, float y, const char *s, float alpha, int align);
void CG_DrawRotatedPicPhysical(float x, float y, float width, float height, float angle, const vec_t *color, MaterialHandle material);
void CG_DrawRotatedPic(float x, float y, float width, float height, int horzAlign, int vertAlign, float angle, const vec_t *color, MaterialHandle material);
void CG_DrawRotatedQuadPic(float x, float y, vec2_t *verts, float angle, const vec_t *color, MaterialHandle material);

void CG_DrawStringExt(float x, float y, const char *string, const vec_t *setColor, qboolean forceColor, qboolean shadow, float charHeight, qboolean adjust)
{
    float baselineY = y + charHeight * 0.8f;
    float fontScale = charHeight / 48.0f;

    if (!setColor) {
        setColor = colorWhite;
    }

    FontHandle font = UI_GetFontHandle(5, fontScale);
    int style = (shadow >= 1) ? 3 : 0;

    UI_DrawText(string, 0x7fffffff, font, x, baselineY, 1, 1, fontScale, setColor, style);
}

int CG_DrawDevString(float x, float y, const char *s, const vec_t *color, int align_in, FontHandle font, int style)
{
    int align = align_in;
    if ((align & 3) != 1) {
        CL_TextWidth(s, 0, font);
    }

    int step = CL_TextHeight(font);
    float fStep = (float)step;

    int vertAlign = align & 0xC;
    if (vertAlign != 8) {
        if (vertAlign == 0xC) {
            fStep *= 0.5f;
        }
        y += fStep;
    }

    int textWidth = CL_TextWidth(s, 0, font);
    CL_DrawText(s, 0x7fffffff, font, -(float)textWidth, y, 3, 1, 1.0f, 1.0f, color, style);

    return step;
}

int CG_DrawBigDevStringColor(float x, float y, const char *s, const vec_t *color, int align)
{
    return CG_DrawDevString(x, y, s, color, align, cgs->media.bigDevFont, 0);
}

int CG_DrawSmallDevStringColor(float x, float y, const char *s, const vec_t *color, int align)
{
    return CG_DrawDevString(x, y, s, color, align, cgs->media.smallDevFont, 0);
}

float *CG_FadeColor(int startMsec, int totalMsec, int fadeMsec)
{
    if (!startMsec) {
        return NULL;
    }

    int t = cg->time - startMsec;

    if (t >= totalMsec) {
        return NULL;
    }

    int remaining = totalMsec - t;

    if (remaining < fadeMsec) {
        color[3] = (float)remaining / (float)fadeMsec;
    } else {
        color[3] = 1.0f;
    }

    color[2] = 1.0f;
    color[1] = 1.0f;
    color[0] = 1.0f;
    return color;
}

void CG_NorthDirectionChanged(void)
{
    const char *configStr = CL_GetConfigString(11);
    cg->compassNorthYaw = (float)atof(configStr);
}

void CG_UpdateCompassOrientation(void)
{
    float fTargetYaw = AngleNormalize360(cg->refdefViewAngles[1] - cg->compassNorthYaw);

    int lastTime = cg->compassLastTime;
    int currentTime = cg->time;

    if (lastTime > currentTime || (float)(currentTime - lastTime) > 500.0f) {
        cg->compassLastTime = currentTime;
        cg->compassYaw = fTargetYaw;
        cg->compassSpeed = 0.0f;
        return;
    }

    int iTotalTime = currentTime - lastTime;
    cg->compassLastTime = currentTime;
    float fDiff = AngleSubtract(cg->compassYaw, fTargetYaw);

    if (iTotalTime <= 0) {
        cg->compassYaw = AngleNormalize360(fDiff + fTargetYaw);
        return;
    }

    while (iTotalTime > 0) {
        float dt;
        if (iTotalTime > 5) {
            iTotalTime -= 5;
            dt = 5.0f;
        } else {
            dt = (float)iTotalTime;
            iTotalTime = 0;
        }
        float fTimeStep = dt * 0.001f;

        if (fabsf(fDiff) < 0.25f) {
            if (fabsf(cg->compassSpeed) < 1.0f) {
                cg->compassYaw = fTargetYaw;
                cg->compassSpeed = 0.0f;
                return;
            }
        }

        fDiff = AngleNormalize180(fDiff + fTimeStep * cg->compassSpeed);

        if (fDiff > 0.0f) {
            cg->compassSpeed -= fTimeStep * 1000.0f;
        } else if (fDiff < 0.0f) {
            cg->compassSpeed += fTimeStep * 1000.0f;
        }

        float vel = cg->compassSpeed;
        vel -= vel * 2.0f * fTimeStep;
        cg->compassSpeed = vel;

        if (vel > 0.0f) {
            if (fDiff > 0.0f) {
                vel -= vel * 3.5f * fTimeStep;
                cg->compassSpeed = vel;
            }
            vel -= fTimeStep;
            cg->compassSpeed = vel;
            if (vel < 0.0f) {
                cg->compassSpeed = 0.0f;
            }
        } else {
            if (fDiff < 0.0f) {
                vel -= vel * 3.5f * fTimeStep;
                cg->compassSpeed = vel;
            }
            vel += fTimeStep;
            if (vel > 0.0f)
                vel = 0.0f;
            cg->compassSpeed = vel;
        }

        vel = cg->compassSpeed;
        if (vel > 30000.0f) {
            cg->compassSpeed = 30000.0f;
        } else if (vel < -30000.0f) {
            cg->compassSpeed = -30000.0f;
        }
    }

    cg->compassYaw = AngleNormalize360(fDiff + fTargetYaw);
}

void CG_UpdateCompPointerOrientation(void)
{
    const dvar_t *config = compPointerConfig ? (const dvar_t *)*compPointerConfig : NULL;
    if (!config || config->current.enabled == 0) {
        cg->compPointerYaw = cg->refdefViewAngles[1];
        return;
    }

    int lastTime = cg->compassLastTime;
    int currentTime = cg->time;

    if (lastTime == currentTime) {
        return;
    }

    float fTargetYaw = cg->refdefViewAngles[1];

    if (lastTime > currentTime || (float)(currentTime - lastTime) > 500.0f) {
        cg->compassLastTime = currentTime;
        cg->compPointerYaw = fTargetYaw;
        cg->compPointerSpeed = 0.0f;
        return;
    }

    int iTotalTime = currentTime - lastTime;
    cg->compassLastTime = currentTime;
    float fDiff = AngleSubtract(cg->compPointerYaw, fTargetYaw);

    if (fabsf(fDiff) > 10.0f) {
        fDiff = (fDiff >= 0.0f) ? 10.0f : -10.0f;
    }

    if (iTotalTime <= 0) {
        cg->compPointerYaw = AngleNormalize360(fDiff + fTargetYaw);
        return;
    }

    while (iTotalTime > 0) {
        float dt;
        if (iTotalTime > 5) {
            iTotalTime -= 5;
            dt = 5.0f;
        } else {
            dt = (float)iTotalTime;
            iTotalTime = 0;
        }
        float fTimeStep = dt * 0.001f;

        if (fabsf(fDiff) < 0.5f) {
            if (fabsf(cg->compPointerSpeed) < 2.0f) {
                cg->compPointerYaw = fTargetYaw;
                cg->compPointerSpeed = 0.0f;
                return;
            }
        }

        fDiff = AngleNormalize180(fDiff + fTimeStep * cg->compPointerSpeed);

        if (fDiff > 0.0f) {
            cg->compPointerSpeed -= fTimeStep * 1500.0f;
        } else if (fDiff < 0.0f) {
            cg->compPointerSpeed += fTimeStep * 1500.0f;
        }

        float vel = cg->compPointerSpeed;
        vel -= vel * 3.0f * fTimeStep;
        cg->compPointerSpeed = vel;

        if (vel > 0.0f) {
            if (fDiff > 0.0f) {
                vel -= vel * 5.0f * fTimeStep;
                cg->compPointerSpeed = vel;
            }
            vel -= 2.0f * fTimeStep;
            cg->compPointerSpeed = vel;
            if (vel < 0.0f) {
                cg->compPointerSpeed = 0.0f;
            }
        } else {
            if (fDiff < 0.0f) {
                vel -= vel * 5.0f * fTimeStep;
                cg->compPointerSpeed = vel;
            }
            vel += 2.0f * fTimeStep;
            if (vel > 0.0f)
                vel = 0.0f;
            cg->compPointerSpeed = vel;
        }

        vel = cg->compPointerSpeed;
        if (vel > 2000.0f) {
            cg->compPointerSpeed = 2000.0f;
        } else if (vel < -2000.0f) {
            cg->compPointerSpeed = -2000.0f;
        }
    }

    cg->compPointerYaw = AngleNormalize360(fDiff + fTargetYaw);
}

void CG_TileClear(void)
{

    if (cgs->viewX == cg->refdef.x &&
        cgs->viewY == cg->refdef.y &&
        cgs->viewWidth == cg->refdef.width &&
        cgs->viewHeight == cg->refdef.height) {
        return;
    }

    CL_SetFullScreenViewport();
    CL_SetViewport(
        cg->refdef.x,
        cg->refdef.y,
        cg->refdef.width,
        cg->refdef.height);
}

int CG_DrawBigDevString(float x, float y, const char *s, float alpha, int align)
{
    vec4_t c;
    c[0] = 1.0f;
    c[1] = 1.0f;
    c[2] = 1.0f;
    c[3] = alpha;

    return CG_DrawDevString(x, y, s, c, align, cgs->media.bigDevFont, 0);
}

void CG_DrawRotatedPicPhysical(float x, float y, float width, float height, float angle, const vec_t *color, MaterialHandle material)
{
    float radians = (float)((double)angle * 0.017453292519943295);
    float s = sinf(radians);
    float c = cosf(radians);
    float halfW = width * 0.5f;
    float halfH = height * 0.5f;
    float cx = halfW + x;
    float cy = halfH + y;
    float cw = c * halfW;
    float sw = s * halfW;
    float sh = s * halfH;
    float ch = c * halfH;

    vec2_t verts[4];
    int i;
    for (i = 0; i < 4; i++) {
        verts[i][0] = cx + cw * sign[i][0] - sh * sign[i][1];
        verts[i][1] = cy + sw * sign[i][0] + ch * sign[i][1];
    }

    CL_DrawQuadPic(verts, color, material);
}

void CG_DrawRotatedPic(float x, float y, float width, float height, int horzAlign, int vertAlign, float angle, const vec_t *color, MaterialHandle material)
{
    CalcScreenPlacement(&x, &y, &width, &height, horzAlign, vertAlign);
    CG_DrawRotatedPicPhysical(x, y, width, height, angle, color, material);
}

void CG_DrawRotatedQuadPic(float x, float y, vec2_t *verts, float angle, const vec_t *color, MaterialHandle material)
{
    float radians = (float)((double)angle * 0.017453292519943295);
    float s = sinf(radians);
    float c = cosf(radians);

    vec2_t xy[4];
    int i;
    for (i = 0; i < 4; i++) {
        xy[i][0] = x + c * verts[i][0] - s * verts[i][1];
        xy[i][1] = y + s * verts[i][0] + c * verts[i][1];
    }

    CL_DrawQuadPic(xy, color, material);
}
