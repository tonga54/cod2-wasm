#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <math.h>
#include <string.h>

extern void *Sys_GetValue(int key);
cmodel_t *CM_ClipHandleToModel(clipHandle_t handle);
static void __attribute_regparm__(3) CM_TestInLeafBrushNode_r(traceWork_t *tw, byte *node, trace_t *trace);
static int CM_SightTraceThroughBrush(cbrush_t *brush);
static int CM_SightTraceThroughLeafBrushNode_r(const vec_t *p2);
static int CM_SightTraceThroughLeaf(trace_t *trace);
static int CM_TraceThroughLeafBrushNode_r(const vec_t *p2, trace_t *trace);
static int CM_SightTraceThroughTree(const traceWork_t *tw, const vec_t *p2, trace_t *trace);
static Bool CM_TraceThroughLeafBrushNode(void);
static qboolean CM_TraceSphereThroughSphere(const vec_t *vStationary, trace_t *trace);
static qboolean CM_SightTraceSphereThroughSphere(const vec_t *vStationary, trace_t *trace);
clipHandle_t CM_TempBoxModel(const vec_t *mins, const vec_t *maxs, int contents);
static int CM_TraceThroughTree(const vec_t *p2, trace_t *trace);
int CM_ContentsOfModel(clipHandle_t handle);
float CM_RadiusOfModel(clipHandle_t handle);
static int __attribute_regparm__(3) CM_Trace(trace_t *results, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, clipHandle_t model, int brushmask);
int CM_BoxTrace(trace_t *results, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, clipHandle_t model, int brushmask);
int CM_BoxSightTrace(int oldHitNum, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, clipHandle_t model, int brushmask);
int CM_TransformedBoxTrace(trace_t *results, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, clipHandle_t model, int brushmask, const vec_t *origin, const vec_t *angles);
int CM_TransformedBoxTraceExternal(trace_t *results, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, clipHandle_t model, int brushmask, const vec_t *origin, const vec_t *angles);
int CM_TransformedBoxSightTrace(int hitNum, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, clipHandle_t model, int brushmask, const vec_t *origin, const vec_t *angles);

cmodel_t *CM_ClipHandleToModel(clipHandle_t handle)
{
    clipMap_t *cm = (clipMap_t *)imp_cm;
    if (handle < cm->numSubModels) {
        return &cm->cmodels[handle];
    }
#if defined(__x86_64__) || defined(_M_X64)

    return ((struct TraceThreadInfo *)Sys_GetValue(3))->box_model;
#else
    return *(cmodel_t **)((char *)Sys_GetValue(3) + 0x14);
#endif
}

extern clipMap_t cm;
extern void CM_CalcTraceEntents(TraceExtents *extents);
extern int CM_BoxLeafnums(const vec_t *mins, const vec_t *maxs, int *list, int listsize, int *lastLeaf);
extern short int CM_MeshTestInLeaf(const traceWork_t *tw, cLeaf_t *leaf, trace_t *trace);
extern void CM_TraceThroughAabbTree(const traceWork_t *tw, CollisionAabbTree *aabbTree, trace_t *trace);
extern void AnglesToAxis(vec3_t angles, float *axis);
extern void MatrixTransformVector(const vec_t *in, const vec_t *matrix, vec_t *out);
extern void MatrixTransposeTransformVector(const vec_t *in, const vec_t *matrix, vec_t *out);

#define CM_TEMP_BOX_MODEL ((clipHandle_t)0x3ff)

static cbrush_t cm_fallbackBoxBrush;
static cmodel_t cm_fallbackBoxModel;

static float CM_DotProduct(const vec_t *a, const vec_t *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static float CM_AbsFloat(float value)
{
    return value < 0.0f ? -value : value;
}

static float CM_MinFloat(float a, float b)
{
    return a < b ? a : b;
}

static inline __attribute__((always_inline)) void CM_CopyVec3(const vec_t *src, vec_t *dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

static inline __attribute__((always_inline)) void CM_ClearTraceResult(trace_t *trace)
{
    memset(trace, 0, sizeof(*trace));
    trace->fraction = 1.0f;
}

static inline __attribute__((always_inline)) TraceThreadInfo *CM_GetThreadInfo(void)
{
    return (TraceThreadInfo *)Sys_GetValue(3);
}

static cmodel_t *CM_ModelForHandle(clipHandle_t handle)
{
    if (handle < cm.numSubModels)
        return &cm.cmodels[handle];

    if (handle == CM_TEMP_BOX_MODEL) {
        TraceThreadInfo *threadInfo = CM_GetThreadInfo();
        if (threadInfo && threadInfo->box_model)
            return threadInfo->box_model;
        return &cm_fallbackBoxModel;
    }

    return CM_ClipHandleToModel(handle);
}

static cbrush_t *CM_BoxBrush(void)
{
    TraceThreadInfo *threadInfo = CM_GetThreadInfo();
    if (threadInfo && threadInfo->box_brush)
        return threadInfo->box_brush;
    return &cm_fallbackBoxBrush;
}

static void CM_InitTraceThreadInfo(traceWork_t *tw)
{
    TraceThreadInfo *threadInfo = CM_GetThreadInfo();

    memset(&tw->threadInfo, 0, sizeof(tw->threadInfo));
    if (!threadInfo)
        return;

    threadInfo->checkcount.global++;
    tw->threadInfo = *threadInfo;
}

static void CM_InitTraceWork(traceWork_t *tw, const vec_t *start, const vec_t *end,
                             const vec_t *mins, const vec_t *maxs, int brushmask)
{
    int axis;
    float sizeSum;
    float radius;

    memset(tw, 0, sizeof(*tw));

    for (axis = 0; axis < 3; axis++) {
        float offset = (mins[axis] + maxs[axis]) * 0.5f;

        tw->size[axis] = maxs[axis] - offset;
        tw->extents.start[axis] = start[axis] + offset;
        tw->extents.end[axis] = end[axis] + offset;
        tw->midpoint[axis] = (tw->extents.start[axis] + tw->extents.end[axis]) * 0.5f;
        tw->delta[axis] = tw->extents.end[axis] - tw->extents.start[axis];
        tw->halfDelta[axis] = tw->delta[axis] * 0.5f;
        tw->halfDeltaAbs[axis] = CM_AbsFloat(tw->halfDelta[axis]);
    }

    CM_CalcTraceEntents(&tw->extents);

    tw->deltaLenSq = CM_DotProduct(tw->delta, tw->delta);
    tw->deltaLen = sqrtf(tw->deltaLenSq);

    radius = CM_MinFloat(CM_AbsFloat(tw->size[0]), CM_AbsFloat(tw->size[2]));
    tw->radius = radius;
    tw->offsetZ = tw->size[2] - radius;

    for (axis = 0; axis < 3; axis++) {
        float expand = axis == 2 ? CM_AbsFloat(tw->size[2]) : radius;
        float startAxis = tw->extents.start[axis];
        float endAxis = tw->extents.end[axis];

        if (endAxis > startAxis) {
            tw->bounds[0][axis] = startAxis - expand;
            tw->bounds[1][axis] = endAxis + expand;
        } else {
            tw->bounds[0][axis] = endAxis - expand;
            tw->bounds[1][axis] = startAxis + expand;
        }
    }

    tw->contents = brushmask;
    sizeSum = CM_AbsFloat(tw->size[0]) + CM_AbsFloat(tw->size[1]) + CM_AbsFloat(tw->size[2]);
    tw->isPoint = (sizeSum == 0.0f);
    tw->axialCullOnly = 0;
    tw->radiusOffset[0] = radius;
    tw->radiusOffset[1] = radius;
    tw->radiusOffset[2] = CM_AbsFloat(tw->size[2]);

    CM_InitTraceThreadInfo(tw);
}

static void CM_SetTraceMaterial(trace_t *trace, int materialNum, int contents)
{
    trace->contents = contents;
    if (materialNum >= 0 && materialNum < cm.numMaterials) {
        trace->surfaceFlags = cm.materials[materialNum].surfaceFlags;
        trace->material = (const char *)&cm.materials[materialNum];
    }
}

static void CM_TraceBrushPlane(const traceWork_t *tw, const vec_t *normal, float dist,
                               int materialNum, float *enterFrac, float *leaveFrac,
                               vec3_t enterNormal, int *enterMaterial, int *startsOut,
                               int *getOut)
{
    float support;
    float expandedDist;
    float startDist;
    float endDist;
    float denominator;
    float fraction;

    /* Map brushes and triangle meshes must sweep the same upright capsule.
     * Expanding by AABB half sizes adds square shoulders on diagonal walls
     * and slopes, making a clear corner overlap the player's box. BSP plane
     * normals are unit length; the sphere radius plus the projected vertical
     * segment is the capsule's support distance. */
    support = tw->radius + CM_AbsFloat(normal[2]) * tw->offsetZ;
    expandedDist = dist + support;

    startDist = CM_DotProduct(tw->extents.start, normal) - expandedDist;
    endDist = CM_DotProduct(tw->extents.end, normal) - expandedDist;

    if (startDist > 0.0f)
        *startsOut = 1;
    if (endDist > 0.0f)
        *getOut = 1;

    if (startDist > 0.0f && endDist > 0.0f) {
        *enterFrac = 1.0f;
        *leaveFrac = 0.0f;
        return;
    }

    if (startDist <= 0.0f && endDist <= 0.0f)
        return;

    denominator = startDist - endDist;
    if (denominator == 0.0f)
        return;

    if (startDist > endDist) {
        fraction = (startDist - 0.125f) / denominator;
        if (fraction > *enterFrac) {
            *enterFrac = fraction;
            enterNormal[0] = normal[0];
            enterNormal[1] = normal[1];
            enterNormal[2] = normal[2];
            *enterMaterial = materialNum;
        }
    } else {
        fraction = (startDist + 0.125f) / denominator;
        if (fraction < *leaveFrac)
            *leaveFrac = fraction;
    }
}

static void CM_TraceThroughBrush(const traceWork_t *tw, cbrush_t *brush, trace_t *trace)
{
    int axis;
    int sideIndex;
    int startsOut = 0;
    int getOut = 0;
    int enterMaterial = -1;
    float enterFrac = -1.0f;
    float leaveFrac = 1.0f;
    vec3_t enterNormal = { 0.0f, 0.0f, 0.0f };

    if (!brush || !(brush->contents & tw->contents))
        return;

    for (axis = 0; axis < 3; axis++) {
        if (tw->bounds[0][axis] > brush->maxs[axis] + 1.0f)
            return;
        if (tw->bounds[1][axis] < brush->mins[axis] - 1.0f)
            return;
    }

    for (axis = 0; axis < 3; axis++) {
        vec3_t normal;

        normal[0] = 0.0f;
        normal[1] = 0.0f;
        normal[2] = 0.0f;
        normal[axis] = 1.0f;
        CM_TraceBrushPlane(tw, normal, brush->maxs[axis],
                           brush->axialMaterialNum[1][axis],
                           &enterFrac, &leaveFrac, enterNormal, &enterMaterial,
                           &startsOut, &getOut);
        if (enterFrac >= leaveFrac)
            return;

        normal[axis] = -1.0f;
        CM_TraceBrushPlane(tw, normal, -brush->mins[axis],
                           brush->axialMaterialNum[0][axis],
                           &enterFrac, &leaveFrac, enterNormal, &enterMaterial,
                           &startsOut, &getOut);
        if (enterFrac >= leaveFrac)
            return;
    }

    for (sideIndex = 0; sideIndex < brush->numsides; sideIndex++) {
        cbrushside_t *side = &brush->sides[sideIndex];
        if (!side->plane)
            continue;

        CM_TraceBrushPlane(tw, side->plane->normal, side->plane->dist,
                           side->materialNum, &enterFrac, &leaveFrac,
                           enterNormal, &enterMaterial, &startsOut, &getOut);
        if (enterFrac >= leaveFrac)
            return;
    }

    if (!startsOut) {
        trace->startsolid = 1;
        CM_SetTraceMaterial(trace, enterMaterial, brush->contents);
        if (!getOut) {
            trace->allsolid = 1;
            trace->fraction = 0.0f;
        }
        return;
    }

    if (enterFrac < 0.0f)
        enterFrac = 0.0f;

    if (enterFrac < leaveFrac && enterFrac < trace->fraction) {
        trace->fraction = enterFrac;
        trace->normal[0] = enterNormal[0];
        trace->normal[1] = enterNormal[1];
        trace->normal[2] = enterNormal[2];
        CM_SetTraceMaterial(trace, enterMaterial, brush->contents);
    }
}

static void CM_TraceLeafBrushNode_r(const traceWork_t *tw, cLeafBrushNode_t *node,
                                    trace_t *trace, int depth)
{
    int i;

    if (!node || depth > 64 || trace->allsolid)
        return;

    if (!(node->contents & tw->contents))
        return;

    if (node->leafBrushCount > 0) {
        for (i = 0; i < node->leafBrushCount; i++) {
            unsigned short brushIndex = node->data.leaf.brushes[i];
            if (brushIndex < cm.numBrushes)
                CM_TraceThroughBrush(tw, &cm.brushes[brushIndex], trace);
            if (trace->allsolid)
                return;
        }
        return;
    }

    if (node->leafBrushCount < 0)
        CM_TraceLeafBrushNode_r(tw, node + 1, trace, depth + 1);

    if (node->data.children.childOffset[0] != 0)
        CM_TraceLeafBrushNode_r(tw, node + node->data.children.childOffset[0], trace, depth + 1);
    if (node->data.children.childOffset[1] != 0 &&
        node->data.children.childOffset[1] != node->data.children.childOffset[0])
        CM_TraceLeafBrushNode_r(tw, node + node->data.children.childOffset[1], trace, depth + 1);
}

static void CM_TraceLeafBrushes(const traceWork_t *tw, cLeaf_t *leaf, trace_t *trace)
{
    if (!leaf || !(leaf->brushContents & tw->contents))
        return;

    if (leaf->leafBrushNode >= 0 && leaf->leafBrushNode < cm.leafbrushNodesCount)
        CM_TraceLeafBrushNode_r(tw, &cm.leafbrushNodes[leaf->leafBrushNode], trace, 0);
}

static void CM_TraceLeafTerrain(const traceWork_t *tw, cLeaf_t *leaf, trace_t *trace)
{
    int k;

    if (!leaf || !(leaf->terrainContents & tw->contents))
        return;

    for (k = 0; k < leaf->collAabbCount; k++) {
        int treeIndex = leaf->firstCollAabbIndex + k;
        if (treeIndex >= 0 && treeIndex < cm.aabbTreeCount)
            CM_TraceThroughAabbTree(tw, &cm.aabbTrees[treeIndex], trace);
        if (trace->allsolid || trace->fraction == 0.0f)
            return;
    }
}

static void CM_TraceLeaf(const traceWork_t *tw, cLeaf_t *leaf, trace_t *trace)
{
    CM_TraceLeafBrushes(tw, leaf, trace);
    if (!trace->allsolid)
        CM_TraceLeafTerrain(tw, leaf, trace);
}

static int __attribute_regparm__(3) CM_Trace(trace_t *results, const vec_t *start, const vec_t *end,
                                                         const vec_t *mins, const vec_t *maxs,
                                                         clipHandle_t model, int brushmask)
{
    traceWork_t tw;
    cmodel_t *cmodel;

    CM_InitTraceWork(&tw, start, end, mins, maxs, brushmask);

    if (model == CM_TEMP_BOX_MODEL) {
        CM_TraceThroughBrush(&tw, CM_BoxBrush(), results);
        return 0;
    }

    if (model != 0) {
        cmodel = CM_ModelForHandle(model);
        if (cmodel)
            CM_TraceLeaf(&tw, &cmodel->leaf, results);
        return 0;
    }

    {
        int leafs[1024];
        int lastLeaf = 0;
        int leafCount;
        int i;

        leafCount = CM_BoxLeafnums(tw.bounds[0], tw.bounds[1], leafs, 1024, &lastLeaf);
        for (i = 0; i < leafCount; i++) {
            int leafIndex = leafs[i];
            if (leafIndex >= 0 && leafIndex < cm.numLeafs)
                CM_TraceLeaf(&tw, &cm.leafs[leafIndex], results);
            if (results->allsolid || results->fraction == 0.0f)
                break;
        }
    }

    return 0;
}

int CM_ContentsOfModel(clipHandle_t handle)
{
    clipMap_t *cm = (clipMap_t *)imp_cm;
    cmodel_t *model;

    if (handle < cm->numSubModels)
        model = &cm->cmodels[handle];
    else
        model = *(cmodel_t **)((byte *)Sys_GetValue(3) + 0x14);

    return model->leaf.brushContents | model->leaf.terrainContents;
}

float CM_RadiusOfModel(clipHandle_t handle)
{
    clipMap_t *cm = (clipMap_t *)imp_cm;
    cmodel_t *model;

    if (handle < cm->numSubModels)
        model = &cm->cmodels[handle];
    else
        model = *(cmodel_t **)((byte *)Sys_GetValue(3) + 0x14);

    return model->radius;
}

clipHandle_t CM_TempBoxModel(const vec_t *mins, const vec_t *maxs, int contents)
{
    TraceThreadInfo *threadInfo = CM_GetThreadInfo();
    cbrush_t *brush = threadInfo->box_brush;
    cmodel_t *model = threadInfo->box_model;

    CM_CopyVec3(mins, model->mins);
    CM_CopyVec3(maxs, model->maxs);
    CM_CopyVec3(mins, brush->mins);
    CM_CopyVec3(maxs, brush->maxs);
    brush->contents = contents;
    /* Entity linking reads the model leaf mask before tracing the brush. */
    model->leaf.brushContents = contents;

    return CM_TEMP_BOX_MODEL;
}

int CM_BoxTrace(trace_t *results, const vec_t *start, const vec_t *end,
                const vec_t *mins, const vec_t *maxs, clipHandle_t model, int brushmask)
{
    CM_ClearTraceResult(results);
    return CM_Trace(results, start, end, mins, maxs, model, brushmask);
}

int CM_BoxSightTrace(int oldHitNum, const vec_t *start, const vec_t *end,
                     const vec_t *mins, const vec_t *maxs, clipHandle_t model, int brushmask)
{
    trace_t trace;

    (void)oldHitNum;
    CM_BoxTrace(&trace, start, end, mins, maxs, model, brushmask);
    return trace.fraction < 1.0f || trace.startsolid || trace.allsolid;
}

extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);

int CM_TransformedBoxTrace(trace_t *results, const vec_t *start, const vec_t *end,
                           const vec_t *mins, const vec_t *maxs, clipHandle_t model,
                           int brushmask, const vec_t *origin, const vec_t *angles)
{
    vec3_t sizeNeg;
    vec3_t sizePos;
    vec3_t localStart;
    vec3_t localEnd;
    vec3_t forward;
    vec3_t right;
    vec3_t up;
    float oldFraction;
    int rotate;
    int i;
    int traceResult;

    for (i = 0; i < 3; i++) {
        float offset = (mins[i] + maxs[i]) * 0.5f;
        sizeNeg[i] = mins[i] - offset;
        sizePos[i] = maxs[i] - offset;
        localStart[i] = start[i] + offset;
        localEnd[i] = end[i] + offset;
    }

    localStart[0] = localStart[0] - origin[0];
    localStart[1] = localStart[1] - origin[1];
    localStart[2] = localStart[2] - origin[2];
    localEnd[0] = localEnd[0] - origin[0];
    localEnd[1] = localEnd[1] - origin[1];
    localEnd[2] = localEnd[2] - origin[2];

    rotate = (angles[0] != 0.0f || angles[1] != 0.0f || angles[2] != 0.0f);
    if (rotate) {
        vec3_t ls;
        vec3_t le;

        AngleVectors(angles, forward, right, up);
        right[0] = -right[0];
        right[1] = -right[1];
        right[2] = -right[2];

        ls[0] = localStart[0];
        ls[1] = localStart[1];
        ls[2] = localStart[2];
        localStart[0] = ls[0] * forward[0] + ls[1] * forward[1] + ls[2] * forward[2];
        localStart[1] = ls[0] * right[0] + ls[1] * right[1] + ls[2] * right[2];
        localStart[2] = ls[0] * up[0] + ls[1] * up[1] + ls[2] * up[2];

        le[0] = localEnd[0];
        le[1] = localEnd[1];
        le[2] = localEnd[2];
        localEnd[0] = le[0] * forward[0] + le[1] * forward[1] + le[2] * forward[2];
        localEnd[1] = le[0] * right[0] + le[1] * right[1] + le[2] * right[2];
        localEnd[2] = le[0] * up[0] + le[1] * up[1] + le[2] * up[2];
    }

    oldFraction = results->fraction;
    traceResult = CM_Trace(results, localStart, localEnd, sizeNeg, sizePos, model, brushmask);

    if (rotate && results->fraction < oldFraction) {
        vec3_t n;
        n[0] = results->normal[0];
        n[1] = results->normal[1];
        n[2] = results->normal[2];
        results->normal[0] = n[0] * forward[0] + n[1] * right[0] + n[2] * up[0];
        results->normal[1] = n[0] * forward[1] + n[1] * right[1] + n[2] * up[1];
        results->normal[2] = n[0] * forward[2] + n[1] * right[2] + n[2] * up[2];
    }

    return traceResult;
}

int CM_TransformedBoxTraceExternal(trace_t *results, const vec_t *start, const vec_t *end,
                                   const vec_t *mins, const vec_t *maxs, clipHandle_t model,
                                   int brushmask, const vec_t *origin, const vec_t *angles)
{
    memset(results, 0, sizeof(*results));
    results->fraction = 1.0f;
    return CM_TransformedBoxTrace(results, start, end, mins, maxs, model, brushmask, origin, angles);
}

int CM_TransformedBoxSightTrace(int hitNum, const vec_t *start, const vec_t *end,
                                const vec_t *mins, const vec_t *maxs, clipHandle_t model,
                                int brushmask, const vec_t *origin, const vec_t *angles)
{
    vec3_t sizeNeg;
    vec3_t sizePos;
    vec3_t localStart;
    vec3_t localEnd;
    vec3_t forward;
    vec3_t right;
    vec3_t up;
    int i;

    for (i = 0; i < 3; i++) {
        float offset = (mins[i] + maxs[i]) * 0.5f;
        sizeNeg[i] = mins[i] - offset;
        sizePos[i] = maxs[i] - offset;
        localStart[i] = start[i] + offset;
        localEnd[i] = end[i] + offset;
    }

    localStart[0] = localStart[0] - origin[0];
    localStart[1] = localStart[1] - origin[1];
    localStart[2] = localStart[2] - origin[2];
    localEnd[0] = localEnd[0] - origin[0];
    localEnd[1] = localEnd[1] - origin[1];
    localEnd[2] = localEnd[2] - origin[2];

    if (angles[0] != 0.0f || angles[1] != 0.0f || angles[2] != 0.0f) {
        vec3_t ls;
        vec3_t le;

        AngleVectors(angles, forward, right, up);
        right[0] = -right[0];
        right[1] = -right[1];
        right[2] = -right[2];

        ls[0] = localStart[0];
        ls[1] = localStart[1];
        ls[2] = localStart[2];
        localStart[0] = ls[0] * forward[0] + ls[1] * forward[1] + ls[2] * forward[2];
        localStart[1] = ls[0] * right[0] + ls[1] * right[1] + ls[2] * right[2];
        localStart[2] = ls[0] * up[0] + ls[1] * up[1] + ls[2] * up[2];

        le[0] = localEnd[0];
        le[1] = localEnd[1];
        le[2] = localEnd[2];
        localEnd[0] = le[0] * forward[0] + le[1] * forward[1] + le[2] * forward[2];
        localEnd[1] = le[0] * right[0] + le[1] * right[1] + le[2] * right[2];
        localEnd[2] = le[0] * up[0] + le[1] * up[1] + le[2] * up[2];
    }

    return CM_BoxSightTrace(hitNum, localStart, localEnd, sizeNeg, sizePos, model, brushmask);
}
