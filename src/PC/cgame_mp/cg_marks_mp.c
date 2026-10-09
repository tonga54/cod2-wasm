#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "headers/PC/cgame_mp/cg_local.h"

extern const dvar_t *cg_marks;

extern float floorf(float x);

extern MarkPoly cg_markPolys[1024];
extern MarkPoly *cg_freeMarkPolys;
extern MarkVertAssemblyBuffer markVerts;

extern const vec_t Vec3NormalizeTo(const vec_t *v, vec_t *out);
extern void PerpendicularVector(const vec_t *src, vec_t *dst);
extern void RotatePointAroundVector(vec_t *dst, const vec_t *dir, const vec_t *point, const float degrees);
extern void Vec3Cross(const vec_t *v0, const vec_t *v1, vec_t *cross);
extern Bool FxHelper_CullSphere(const FxHelper *_this, const vec_t *worldPos, float radius, int planeCount);
extern void CL_AddPolyToScene(MaterialHandle mtlHandle, int lmapIndex, int vertCount, const GfxWorldVertex *verts);
extern int CL_MarkFragments(const vec3_t *points, const vec_t *origin, const vec3_t *axis, float radius, int maxPoints, GfxWorldVertex *verts, int maxFragments, GfxMarkFragment *fragmentBuffer, MaterialHandle markMaterial);
extern void Com_Error(errorParm_t code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);

#define CG_MARKCOUNT_PTR (*(int **)(imp_cg_marksLimit))
#define CG_MARKS_DVAR cg_marks
#define FX_HELPER_PTR (*(FxHelper **)(imp_theFxHelper))

static inline __attribute__((always_inline)) unsigned char ColorFloatToByte(float f)
{
    int val = (int)floorf(f * 255.0f + 0.5f);
    if (val - 255 >= 0)
        return 255;
    if (val <= 0)
        return 0;
    return (unsigned char)val;
}

void CG_InitMarkPolys(void);
void CG_AddMarks(void);
void CG_ImpactMark(MaterialHandle markMaterial, const vec_t *origin, const vec_t *dir, float orientation, const vec_t *color, float radius);

void CG_InitMarkPolys(void)
{
    int lasttrav;
    int i;
    MarkPoly *sentinel;

    lasttrav = CG_MARKCOUNT_PTR[2];

    memset(cg_markPolys, 0, (size_t)lasttrav * sizeof(MarkPoly));

    sentinel = &cg->activeMarkPolys;
    cg->activeMarkPolys.nextMark = (int)(intptr_t)sentinel;
    cg->activeMarkPolys.prevMark = (int)(intptr_t)sentinel;

    cg_freeMarkPolys = &cg_markPolys[0];

    {
        MarkPoly *cur = &cg_markPolys[0];
        MarkPoly *nxt = &cg_markPolys[1];
        for (i = 0; i < lasttrav - 1; i++) {
            ((cur)->nextMark = (int)(intptr_t)(nxt));
            cur = nxt;
            nxt = nxt + 1;
        }
    }
}

void CG_AddMarks(void)
{
    MarkPoly *markPoly;
    FxHelper **fxSlot;

    if (!CG_MARKS_DVAR->current.enabled)
        return;

    markPoly = (MarkPoly *)(intptr_t)cg->activeMarkPolys.nextMark;

    if (markPoly == &cg->activeMarkPolys)
        return;

    fxSlot = (FxHelper **)(imp_theFxHelper);

    do {

        if (!FxHelper_CullSphere(*fxSlot, markPoly->origin, markPoly->radius,
                                 (*fxSlot)->mCamera.numPlanes)) {

            markPoly->lastFrameDrawn = cg->clientFrame;

            CL_AddPolyToScene(markPoly->mtlHandle, (int)markPoly->lmapIndex,
                              (int)markPoly->vertCount, markPoly->verts);
        }

        markPoly = ((MarkPoly *)(intptr_t)(markPoly)->nextMark);
    } while (markPoly != &cg->activeMarkPolys);
}

void CG_ImpactMark(MaterialHandle markMaterial, const vec_t *origin, const vec_t *dir, float orientation, const vec_t *color, float radius)
{
    vec3_t axis[3];
    vec_t *axisDir = axis[0];
    vec_t *axisRight = axis[1];
    vec_t *axisCross = axis[2];
    vec3_t originalPoints[4];
    GfxMarkFragment markFragments[384];
    int fragmentCount;
    int fragmentIndex;
    GfxColor nativeColor;
    int j;
    MarkPoly *sentinel;
    MarkPoly *markPoly;
    MarkPoly *oldest;
    MarkPoly *current;
    float a, b;

    if (!markMaterial || radius <= 0.0f || !CG_MARKS_DVAR->current.enabled)
        return;

    if (((cg_t *)cg)->nomarks != 0)
        return;

    Vec3NormalizeTo(dir, axisDir);
    PerpendicularVector(axisDir, axisCross);
    RotatePointAroundVector(axisRight, axisDir, axisCross, orientation);
    Vec3Cross(axisDir, axisRight, axisCross);

    for (j = 0; j < 3; j++) {
        a = radius * axisCross[j];
        b = radius * axisRight[j];
        originalPoints[0][j] = origin[j] - a - b;
        originalPoints[1][j] = origin[j] + a - b;
        originalPoints[2][j] = origin[j] + a + b;
        originalPoints[3][j] = origin[j] - a + b;
    }

    fragmentCount = CL_MarkFragments(
        originalPoints,
        origin,
        axis,
        radius,
        1024,
        (GfxWorldVertex *)&markVerts,
        384,
        markFragments,
        markMaterial);

    /* World vertices use RGBA; the complete basis above supplies decal UVs. */
    for (j = 0; j < 4; j++)
        nativeColor.array[j] = ColorFloatToByte(color[j]);

    if (getenv("PTRACE"))
        Com_Printf("[mark] material=%s fragments=%d radius=%.2f color=%.2f,%.2f,%.2f,%.2f\n",
            markMaterial->info.name, fragmentCount, radius,
            color[0], color[1], color[2], color[3]);

    if (fragmentCount <= 0)
        return;

    for (fragmentIndex = 0; fragmentIndex < fragmentCount; fragmentIndex++) {
        GfxMarkFragment *frag = &markFragments[fragmentIndex];
        GfxWorldVertex *verts = (GfxWorldVertex *)((char *)&markVerts + frag->firstPoint * (int)sizeof(GfxWorldVertex));
        int vertCount = (int)frag->pointCount;

        for (j = 0; j < vertCount; j++) {
            verts[j].color.packed = *(unsigned int *)nativeColor.array;
        }

        if (cg_freeMarkPolys == NULL) {

            sentinel = &cg->activeMarkPolys;
            oldest = ((MarkPoly *)(intptr_t)(sentinel)->prevMark);

            if (oldest != sentinel) {
                current = oldest;
                while (1) {
                    if (current->lastFrameDrawn < oldest->lastFrameDrawn) {
                        oldest = current;
                    }
                    current = ((MarkPoly *)(intptr_t)(current)->prevMark);
                    if (current == sentinel)
                        break;
                }
            }

            if (((MarkPoly *)(intptr_t)(oldest)->prevMark) == NULL) {
                Com_Error(ERR_DROP, "CG_FreeLocalEntity: not active");

            }

            {
                MarkPoly *prev = ((MarkPoly *)(intptr_t)(oldest)->prevMark);
                MarkPoly *next = ((MarkPoly *)(intptr_t)(oldest)->nextMark);
                ((prev)->nextMark = (int)(intptr_t)(next));
                ((next)->prevMark = (int)(intptr_t)(prev));
            }

            ((oldest)->nextMark = (int)(intptr_t)(cg_freeMarkPolys));
            cg_freeMarkPolys = oldest;
        }

        markPoly = cg_freeMarkPolys;
        cg_freeMarkPolys = ((MarkPoly *)(intptr_t)(markPoly)->nextMark);

        sentinel = &cg->activeMarkPolys;
        {
            MarkPoly *oldFirst = ((MarkPoly *)(intptr_t)(sentinel)->nextMark);
            ((markPoly)->nextMark = (int)(intptr_t)(oldFirst));
            ((markPoly)->prevMark = (int)(intptr_t)(sentinel));
            ((oldFirst)->prevMark = (int)(intptr_t)(markPoly));
            ((sentinel)->nextMark = (int)(intptr_t)(markPoly));
        }

        markPoly->origin[0] = origin[0];
        markPoly->origin[1] = origin[1];
        markPoly->origin[2] = origin[2];
        markPoly->radius = radius;
        markPoly->mtlHandle = frag->markMaterial;
        markPoly->lmapIndex = frag->lmapIndex;
        markPoly->vertCount = frag->pointCount;
        markPoly->lastFrameDrawn = *(int *)cg;

        memcpy(markPoly->verts, verts, (size_t)frag->pointCount * sizeof(GfxWorldVertex));
    }
}
