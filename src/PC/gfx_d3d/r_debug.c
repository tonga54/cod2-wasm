#include "common_types.h"
#include "imports.h"

#include <string.h>
extern GfxScene scene;

static const int iEdgePairs[12][2] = {
    { 0, 1 }, { 0, 2 }, { 0, 4 }, { 1, 3 }, { 1, 5 }, { 2, 3 }, { 2, 6 }, { 3, 7 }, { 4, 5 }, { 4, 6 }, { 5, 7 }, { 6, 7 }
};
static DebugGlobals debugGlobals;

extern refimport_t ri;
extern GfxBackEndData *frontEndDataOut;
#define frontEndDataOut frontEndDataOut


extern float Vec3Normalize(vec_t *v);

void R_InitDebugEntry(DebugGlobals *debugGlobalsEntry);
void R_ShutdownDebugEntry(DebugGlobals *debugGlobalsEntry);
void R_TransferDebugGlobals(DebugGlobals *debugGlobalsEntry);
void R_ShutdownDebug(void);
void R_LocateDebugStrings(trDebugString_t *strings, int stringCount, int maxStringCount);
void R_LocateDebugLines(trDebugLine_t *lines, int lineCount, int maxLineCount);
void R_AddDebugString(DebugGlobals *debugGlobalsEntry, const vec_t *origin, const vec_t *color, float scale, const char *string);
void R_AddPlume(const vec_t *origin, int score, const vec_t *color, int duration);
void R_AddDebugPolygon(DebugGlobals *debugGlobalsEntry, const vec_t *color, const int pointCount, vec3_t *points);
void R_AddDebugLine(DebugGlobals *debugGlobalsEntry, const vec_t *start, const vec_t *end, const vec_t *color);
void R_InitDebug(void);
void R_AddDebugBox(DebugGlobals *debugGlobalsEntry, const vec_t *mins, const vec_t *maxs, const vec_t *color);
void R_AddScaledDebugString(DebugGlobals *debugGlobalsEntry, const GfxViewParms *viewParms, const vec_t *origin, const vec_t *color, const char *string);

static inline __attribute__((always_inline)) void R_InitDebugEntry_core(DebugGlobals *debugGlobalsEntry)
{
    memset(debugGlobalsEntry, 0, sizeof(DebugGlobals));
    debugGlobalsEntry->vertLimit = 0x1000;
    debugGlobalsEntry->polyLimit = 512;
    debugGlobalsEntry->stringLimit = 0x1000;
    debugGlobalsEntry->lineLimit = 16384;
    debugGlobalsEntry->plumeLimit = 0x1000;
}
void R_InitDebugEntry(DebugGlobals *debugGlobalsEntry)
{
    R_InitDebugEntry_core(debugGlobalsEntry);
}

void R_ShutdownDebugEntry(DebugGlobals *debugGlobalsEntry)
{
    if (debugGlobalsEntry->polys) {
        ri.Z_FreeInternal(debugGlobalsEntry->polys);
        debugGlobalsEntry->polys = NULL;
    }
    if (debugGlobalsEntry->verts) {
        ri.Z_FreeInternal(debugGlobalsEntry->verts);
        debugGlobalsEntry->verts = NULL;
    }
    if (debugGlobalsEntry->strings) {
        ri.Z_FreeInternal(debugGlobalsEntry->strings);
        debugGlobalsEntry->strings = NULL;
    }
    if (debugGlobalsEntry->externStrings) {
        ri.Z_FreeInternal(debugGlobalsEntry->externStrings);
        debugGlobalsEntry->externStrings = NULL;
    }
    if (debugGlobalsEntry->lines) {
        ri.Z_FreeInternal(debugGlobalsEntry->lines);
        debugGlobalsEntry->lines = NULL;
    }
    if (debugGlobalsEntry->externLines) {
        ri.Z_FreeInternal(debugGlobalsEntry->externLines);
        debugGlobalsEntry->externLines = NULL;
    }
    if (debugGlobalsEntry->plumes) {
        ri.Z_FreeInternal(debugGlobalsEntry->plumes);
        debugGlobalsEntry->plumes = NULL;
    }
}

void R_TransferDebugGlobals(DebugGlobals *debugGlobalsEntry)
{
    int plumeIndex = 0;
    int plumeCount = debugGlobals.plumeCount;

    while (plumeIndex < plumeCount) {
        int elapsed = scene.def.time - debugGlobals.plumes[plumeIndex].startTime;
        if (elapsed < 0 || elapsed > debugGlobals.plumes[plumeIndex].duration) {
            plumeCount--;
            debugGlobals.plumeCount = plumeCount;
            debugGlobals.plumes[plumeIndex] = debugGlobals.plumes[plumeCount];
        } else {
            plumeIndex++;
        }
    }

    if (debugGlobals.plumes) {
        if (!debugGlobalsEntry->plumes) {
            debugGlobalsEntry->plumes = ri.Z_MallocInternal(debugGlobals.plumeLimit * sizeof(GfxDebugPlume));
        }
        memcpy(debugGlobalsEntry->plumes, debugGlobals.plumes, plumeCount * sizeof(GfxDebugPlume));
    }
    debugGlobalsEntry->plumeCount = plumeCount;
}

void R_ShutdownDebug(void)
{
    R_ShutdownDebugEntry(&debugGlobals);
}

void R_LocateDebugStrings(trDebugString_t *strings, int stringCount, int maxStringCount)
{
    DebugGlobals *debugGlobalsEntry = &frontEndDataOut->debugGlobals;

    if (strings) {
        if (!debugGlobalsEntry->externStrings) {
            debugGlobalsEntry->externStrings = ri.Z_MallocInternal(maxStringCount * sizeof(trDebugString_t));
        }
        memcpy(debugGlobalsEntry->externStrings, strings, stringCount * sizeof(trDebugString_t));
    }
    debugGlobalsEntry->externStringCount = stringCount;
}

void R_LocateDebugLines(trDebugLine_t *lines, int lineCount, int maxLineCount)
{
    DebugGlobals *debugGlobalsEntry = &frontEndDataOut->debugGlobals;

    if (lines) {
        if (!debugGlobalsEntry->externLines) {
            debugGlobalsEntry->externLines = ri.Z_MallocInternal(maxLineCount * sizeof(trDebugLine_t));
        }
        memcpy(debugGlobalsEntry->externLines, lines, lineCount * sizeof(trDebugLine_t));
    }
    debugGlobalsEntry->externLineCount = lineCount;
}

static inline __attribute__((always_inline)) void R_AddDebugString_core(DebugGlobals *debugGlobalsEntry, const vec_t *origin, const vec_t *color, float scale, const char *string)
{
    if (debugGlobalsEntry->stringCount + 1 > debugGlobalsEntry->stringLimit) {
        return;
    }

    if (!debugGlobalsEntry->strings) {
        debugGlobalsEntry->strings = ri.Z_MallocInternal(debugGlobalsEntry->stringLimit * sizeof(trDebugString_t));
    }

    trDebugString_t *pDebugString = &debugGlobalsEntry->strings[debugGlobalsEntry->stringCount];
    pDebugString->xyz[0] = origin[0];
    pDebugString->xyz[1] = origin[1];
    pDebugString->xyz[2] = origin[2];
    pDebugString->color[0] = color[0];
    pDebugString->color[1] = color[1];
    pDebugString->color[2] = color[2];
    pDebugString->color[3] = color[3];
    pDebugString->scale = scale;
    strncpy(pDebugString->text, string, 95);
    pDebugString->text[95] = '\0';
    debugGlobalsEntry->stringCount++;
}
void R_AddDebugString(DebugGlobals *debugGlobalsEntry, const vec_t *origin, const vec_t *color, float scale, const char *string)
{
    R_AddDebugString_core(debugGlobalsEntry, origin, color, scale, string);
}

void R_AddPlume(const vec_t *origin, int score, const vec_t *color, int duration)
{
    if (!debugGlobals.plumes) {
        debugGlobals.plumes = ri.Z_MallocInternal(debugGlobals.plumeLimit * sizeof(GfxDebugPlume));
        debugGlobals.plumeCount = 0;
    }

    if (debugGlobals.plumeCount == debugGlobals.plumeLimit) {
        return;
    }

    GfxDebugPlume *plume = &debugGlobals.plumes[debugGlobals.plumeCount];
    plume->origin[0] = origin[0];
    plume->origin[1] = origin[1];
    plume->origin[2] = origin[2];
    plume->color[0] = color[0];
    plume->color[1] = color[1];
    plume->color[2] = color[2];
    plume->score = score;
    plume->startTime = scene.def.time;
    plume->duration = duration;
    debugGlobals.plumeCount++;
}

void R_AddDebugPolygon(DebugGlobals *debugGlobalsEntry, const vec_t *color, const int pointCount, vec3_t *points)
{
    int vertCount = debugGlobalsEntry->vertCount;
    if (pointCount + vertCount > debugGlobalsEntry->vertLimit) {
        return;
    }
    int polyCount = debugGlobalsEntry->polyCount;
    if (polyCount + 1 > debugGlobalsEntry->polyLimit) {
        return;
    }

    if (!debugGlobalsEntry->polys) {
        debugGlobalsEntry->polys = ri.Z_MallocInternal(debugGlobalsEntry->polyLimit * sizeof(GfxDebugPoly));
        debugGlobalsEntry->verts = ri.Z_MallocInternal(debugGlobalsEntry->vertLimit * sizeof(vec3_t));
        vertCount = debugGlobalsEntry->vertCount;
        polyCount = debugGlobalsEntry->polyCount;
    }

    debugGlobalsEntry->polys[polyCount].firstVert = vertCount;
    debugGlobalsEntry->polys[polyCount].vertCount = pointCount;
    debugGlobalsEntry->polys[polyCount].color[0] = color[0];
    debugGlobalsEntry->polys[polyCount].color[1] = color[1];
    debugGlobalsEntry->polys[polyCount].color[2] = color[2];
    debugGlobalsEntry->polys[polyCount].color[3] = color[3];
    debugGlobalsEntry->polyCount++;

    memcpy(&debugGlobalsEntry->verts[vertCount], points, pointCount * sizeof(vec3_t));
    debugGlobalsEntry->vertCount += pointCount;
}

static inline __attribute__((always_inline)) void R_AddDebugLine_core(DebugGlobals *debugGlobalsEntry, const vec_t *start, const vec_t *end, const vec_t *color)
{
    if (debugGlobalsEntry->lineCount + 1 > debugGlobalsEntry->lineLimit) {
        return;
    }

    if (!debugGlobalsEntry->lines) {
        debugGlobalsEntry->lines = ri.Z_MallocInternal(debugGlobalsEntry->lineLimit * sizeof(trDebugLine_t));
    }

    trDebugLine_t *line = &debugGlobalsEntry->lines[debugGlobalsEntry->lineCount];
    line->start[0] = start[0];
    line->start[1] = start[1];
    line->start[2] = start[2];
    line->end[0] = end[0];
    line->end[1] = end[1];
    line->end[2] = end[2];
    line->color[0] = color[0];
    line->color[1] = color[1];
    line->color[2] = color[2];
    line->color[3] = color[3];
    line->depthTest = 0;
    debugGlobalsEntry->lineCount++;
}
void R_AddDebugLine(DebugGlobals *debugGlobalsEntry, const vec_t *start, const vec_t *end, const vec_t *color)
{
    R_AddDebugLine_core(debugGlobalsEntry, start, end, color);
}

void R_InitDebug(void)
{
    R_InitDebugEntry_core(&debugGlobals);
}

void R_AddDebugBox(DebugGlobals *debugGlobalsEntry, const vec_t *mins, const vec_t *maxs, const vec_t *color)
{
    vec3_t v[8];
    int i, j;

    for (i = 0; i < 8; i++) {
        for (j = 0; j < 3; j++) {
            if ((i >> j) & 1) {
                v[i][j] = maxs[j];
            } else {
                v[i][j] = mins[j];
            }
        }
    }

    const int *ep = &iEdgePairs[0][0];
    for (i = 12; i != 0; i--) {
        R_AddDebugLine_core(debugGlobalsEntry, v[ep[0]], v[ep[1]], color);
        ep += 2;
    }
}

void R_AddScaledDebugString(DebugGlobals *debugGlobalsEntry, const GfxViewParms *viewParms, const vec_t *origin, const vec_t *color, const char *string)
{
    vec3_t delta;
    float dist, dot, scale;

    delta[0] = origin[0] - viewParms->origin[0];
    delta[1] = origin[1] - viewParms->origin[1];
    delta[2] = origin[2] - viewParms->origin[2];

    dist = Vec3Normalize(delta);
    dot = delta[0] * viewParms->axis[0][0] + delta[1] * viewParms->axis[0][1] + delta[2] * viewParms->axis[0][2];
    scale = dist * (dot - 0.995f);
    if (scale < 1.0f) {
        scale = 1.0f;
    }

    R_AddDebugString_core(debugGlobalsEntry, origin, color, scale, string);
}
