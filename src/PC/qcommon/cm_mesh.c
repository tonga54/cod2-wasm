#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

extern float fabsf(float x);
extern float sqrtf(float x);

extern clipMap_t cm;
#define cm_ptr (&cm)

extern float Vec3Normalize(vec3_t v);

#define CM_MESH_REGPARM3_ABI COD2_REGPARM(3)
#define CM_MESH_REGPARM3_SSE_ABI COD2_REGPARM(3) COD2_SSEREGPARM

static Bool CM_MESH_REGPARM3_ABI CM_CullBox(const traceWork_t *tw, const vec3_t origin, const vec3_t halfSize);
static void CM_MESH_REGPARM3_ABI CM_PositionTestCapsuleInTriangle(const traceWork_t *tw, const CollisionTriangle *triangle, trace_t *trace);
static void CM_MESH_REGPARM3_SSE_ABI CM_TraceCapsuleThroughTriangle(const traceWork_t *tw, const CollisionTriangle *triangle, trace_t *trace, float offsetZ);
static void CM_MESH_REGPARM3_ABI CM_TraceThroughAabbTree_r(const traceWork_t *tw, const CollisionAabbTree *aabbTree, trace_t *trace);
void CM_TraceThroughAabbTree(const traceWork_t *tw, CollisionAabbTree *aabbTree, trace_t *trace);
void CM_SightTraceThroughAabbTree(const traceWork_t *tw, CollisionAabbTree *aabbTree, trace_t *trace);
static void CM_MESH_REGPARM3_ABI CM_PositionTestInAabbTree_r(const traceWork_t *tw, const CollisionAabbTree *aabbTree, trace_t *trace);
short int CM_MeshTestInLeaf(const traceWork_t *tw, cLeaf_t *leaf, trace_t *trace);

static Bool CM_MESH_REGPARM3_ABI CM_CullBox(const traceWork_t *tw, const vec3_t origin, const vec3_t halfSize)
{
    float dx, dy, dz;
    float ex, ey, ez;

    dx = tw->midpoint[0] - origin[0];
    dy = tw->midpoint[1] - origin[1];
    dz = tw->midpoint[2] - origin[2];

    ex = halfSize[0] + tw->size[0];
    ey = halfSize[1] + tw->size[1];
    ez = halfSize[2] + tw->size[2];

    if (fabsf(dx) > tw->halfDeltaAbs[0] + ex)
        return 1;
    if (fabsf(dy) > tw->halfDeltaAbs[1] + ey)
        return 1;
    if (fabsf(dz) > tw->halfDeltaAbs[2] + ez)
        return 1;

    if (tw->axialCullOnly)
        return 0;

    {
        float hdy = tw->halfDelta[1];
        float hdz = tw->halfDelta[2];
        float cross;

        cross = hdy * dz - hdz * dy;
        if (fabsf(cross) > tw->halfDeltaAbs[2] * ey + tw->halfDeltaAbs[1] * ez)
            return 1;

        {
            float hdx = tw->halfDelta[0];
            float cross1 = dx * hdz - hdx * dz;
            if (fabsf(cross1) > tw->halfDeltaAbs[0] * ez + tw->halfDeltaAbs[2] * ex)
                return 1;

            {
                float cross2 = hdx * dy - hdy * dx;
                if (fabsf(cross2) > tw->halfDeltaAbs[1] * ex + tw->halfDeltaAbs[0] * ey)
                    return 1;
            }
        }
    }

    return 0;
}

static void CM_MESH_REGPARM3_ABI CM_PositionTestCapsuleInTriangle(const traceWork_t *tw, const CollisionTriangle *triangle, trace_t *trace)
{
    float height;
    float startX, startY, startZ;
    float dist, distAlt;
    float cullEndRadius;
    float s0, t0;
    float normalZ;
    float scale;
    int sideFlags;
    int sideIndex;

    height = tw->offsetZ;
    normalZ = triangle->plane[2];

    if (normalZ < 0.0f) {
        height = -height;
    }

    startX = tw->extents.start[0];
    startY = tw->extents.start[1];
    startZ = tw->extents.start[2] - height;

    cullEndRadius = tw->radius;

    dist = startX * triangle->plane[0] + startY * triangle->plane[1] +
           startZ * normalZ - triangle->plane[3];

    if (dist >= cullEndRadius)
        return;

    if (-cullEndRadius < dist) {

        float negDist = -dist;
        float projX, projY, projZ;

        projX = negDist * triangle->plane[0] + startX;
        projY = negDist * triangle->plane[1] + startY;
        projZ = negDist * normalZ + startZ;

        s0 = projX * triangle->svec[0] + projY * triangle->svec[1] +
             projZ * triangle->svec[2] - triangle->svec[3];
        t0 = projX * triangle->tvec[0] + projY * triangle->tvec[1] +
             projZ * triangle->tvec[2] - triangle->tvec[3];

        sideFlags = 0;
        if (s0 + t0 > 1.0f)
            sideFlags |= 1;
        if (0.0f > s0)
            sideFlags |= 2;
        if (0.0f > t0)
            sideFlags |= 4;

        if (sideFlags == 0) {
            trace->startsolid = 1;
            trace->allsolid = 1;
            trace->fraction = s0 + t0;
            return;
        }

        {
            clipMap_t *cmLocal = cm_ptr;
            CollisionVertex *verts = cmLocal->verts;
            CollisionEdge *edges = cmLocal->edges;
            float radiusSq = cullEndRadius * cullEndRadius;
            const byte *edgeBase = (const byte *)triangle;

            for (sideIndex = 0; sideIndex < 3; sideIndex++) {
                if ((sideFlags >> sideIndex) & 1) {

                    int edgeIdx = triangle->edges[sideIndex];
                    if (edgeIdx >= 0) {
                        CollisionEdge *edge = &edges[edgeIdx];
                        float ex = startX - edge->origin[0];
                        float ey = startY - edge->origin[1];
                        float ez = startZ - edge->origin[2];
                        float edgeAxisX = edge->axis[2][0];
                        float edgeAxisY = edge->axis[2][1];
                        float edgeAxisZ = edge->axis[2][2];

                        float proj = ex * edgeAxisX + ey * edgeAxisY + ez * edgeAxisZ;

                        if (fabsf(proj - 0.5f) <= 0.5f) {
                            float axisLenSq = edgeAxisX * edgeAxisX +
                                              edgeAxisY * edgeAxisY +
                                              edgeAxisZ * edgeAxisZ;
                            float t_val = proj / axisLenSq;
                            float negT = -t_val;

                            ex += negT * edgeAxisX;
                            ey += negT * edgeAxisY;
                            ez += negT * edgeAxisZ;

                            if (ex * ex + ey * ey + ez * ez < radiusSq) {
                                trace->startsolid = 1;
                                trace->allsolid = 1;
                                trace->fraction = 0;
                                return;
                            }
                        }
                    }
                } else {

                    int vertIdx = triangle->verts[sideIndex];
                    if (vertIdx >= 0) {
                        CollisionVertex *vert = &verts[vertIdx];
                        float vx = startX - vert->xyz[0];
                        float vy = startY - vert->xyz[1];
                        float vz = startZ - vert->xyz[2];

                        if (vx * vx + vy * vy + vz * vz < radiusSq) {
                            trace->startsolid = 1;
                            trace->allsolid = 1;
                            trace->fraction = 0;
                            return;
                        }
                    }
                }
            }
        }
        return;
    }

    {
        float doubleHeight = height * 2.0f;
        distAlt = doubleHeight * normalZ + dist;

        if (-cullEndRadius >= distAlt)
            return;

        scale = (-cullEndRadius - dist) / normalZ;

        s0 = startX * triangle->svec[0] + startY * triangle->svec[1] +
             startZ * triangle->svec[2] - triangle->svec[3];
        t0 = startX * triangle->tvec[0] + startY * triangle->tvec[1] +
             startZ * triangle->tvec[2] - triangle->tvec[3];

        {
            float s_scaled = scale * triangle->svec[2] + s0;
            float t_scaled;

            if (s_scaled < 0.0f)
                goto edge_test_neg;

            t_scaled = scale * triangle->tvec[2] + t0;
            if (t_scaled < 0.0f)
                goto edge_test_neg;

            if (s_scaled + t_scaled <= 1.0f) {
                trace->startsolid = 1;
                trace->allsolid = 1;
                trace->fraction = 0.0f;
                return;
            }
        }

    edge_test_neg:
        if (cullEndRadius <= distAlt) {

            float h_scale = (cullEndRadius - dist) / normalZ;
            float s_h = h_scale * triangle->svec[2] + s0;
            if (0.0f > s_h)
                return;
            {
                float t_h = h_scale * triangle->tvec[2] + t0;
                if (0.0f > t_h)
                    return;
                if (s_h + t_h > 1.0f)
                    return;
            }
            trace->startsolid = 1;
            trace->allsolid = 1;
            trace->fraction = 0;
            return;
        }

        {
            float h_scale = doubleHeight;
            float s_h = h_scale * triangle->svec[2] + s0;
            if (0.0f > s_h)
                return;
            {
                float t_h = h_scale * triangle->tvec[2] + t0;
                if (0.0f > t_h)
                    return;
                if (s_h + t_h > 1.0f)
                    return;
            }
            trace->startsolid = 1;
            trace->allsolid = 1;
            trace->fraction = 0;
            return;
        }
    }
}

static void CM_SetTraceNormalToPlane(trace_t *trace, const CollisionTriangle *triangle,
                                     float startZ, float fromZ)
{
    if (triangle->plane[2] >= 0.7f &&
        trace->normal[2] >= 0.0f &&
        trace->normal[2] < 0.7f &&
        startZ > fromZ) {
        trace->normal[0] = triangle->plane[0];
        trace->normal[1] = triangle->plane[1];
        trace->normal[2] = triangle->plane[2];
    }
}

static void CM_MESH_REGPARM3_SSE_ABI CM_TraceCapsuleThroughTriangle(const traceWork_t *tw, const CollisionTriangle *triangle,
                                                                    trace_t *trace, float offsetZ)
{
    float fromZ;
    float startX, startY, startZ;
    float cullEndRadius;
    float normalZ;
    float dist, endDist, enterDist;
    float s0, t0;
    float fraction;
    float height;
    int sideFlags;
    int sideIndex;

    fromZ = tw->extents.end[2] - offsetZ;
    cullEndRadius = 0.125f + tw->radius;
    normalZ = triangle->plane[2];

    endDist = tw->extents.end[0] * triangle->plane[0] +
              tw->extents.end[1] * triangle->plane[1] +
              fromZ * normalZ - triangle->plane[3];

    if (endDist >= cullEndRadius)
        return;

    startX = tw->extents.start[0];
    startY = tw->extents.start[1];
    startZ = tw->extents.start[2] - offsetZ;

    dist = startX * triangle->plane[0] + startY * triangle->plane[1] +
           startZ * normalZ - triangle->plane[3];

    enterDist = dist - endDist;
    if (enterDist <= 0.0001f)
        return;

    if (-cullEndRadius < dist) {

        float surfDist = dist - cullEndRadius;
        float hitX, hitY, hitZ;

        if (0.0f >= surfDist) {
            fraction = 0.0f;
            hitX = startX;
            hitY = startY;
            hitZ = startZ;
            goto compute_bary_trace;
        }

        fraction = surfDist / enterDist;
        if (fraction > trace->fraction)
            return;

        hitX = fraction * tw->delta[0] + startX;
        hitY = fraction * tw->delta[1] + startY;
        hitZ = fraction * tw->delta[2] + startZ;

    compute_bary_trace:
        s0 = hitX * triangle->svec[0] + hitY * triangle->svec[1] +
             hitZ * triangle->svec[2] - triangle->svec[3];
        t0 = hitX * triangle->tvec[0] + hitY * triangle->tvec[1] +
             hitZ * triangle->tvec[2] - triangle->tvec[3];

        sideFlags = 0;
        if (s0 + t0 > 1.0f)
            sideFlags |= 1;
        if (0.0f > s0)
            sideFlags |= 2;
        if (0.0f > t0)
            sideFlags |= 4;

        if (sideFlags == 0) {
            trace->normal[0] = triangle->plane[0];
            trace->normal[1] = triangle->plane[1];
            trace->normal[2] = triangle->plane[2];
            trace->fraction = fraction;
            if (tw->radius > dist) {
                trace->startsolid = 1;
            }
            return;
        }

        goto edge_trace_tests;
    }

    height = offsetZ * 2.0f;

    {
        float altEndDist = height * normalZ + dist;

        if (-cullEndRadius >= altEndDist)
            return;

        {
            float enterFrac = (-cullEndRadius - dist) / normalZ;

            s0 = startX * triangle->svec[0] + startY * triangle->svec[1] +
                 startZ * triangle->svec[2] - triangle->svec[3];
            t0 = startX * triangle->tvec[0] + startY * triangle->tvec[1] +
                 startZ * triangle->tvec[2] - triangle->tvec[3];

            {
                float s_enter = enterFrac * triangle->svec[2] + s0;

                if (0.0f > s_enter)
                    goto height_adjusted_test;

                {
                    float t_enter = enterFrac * triangle->tvec[2] + t0;
                    if (0.0f > t_enter)
                        goto height_adjusted_test;

                    if (s_enter + t_enter <= 1.0f) {
                        trace->normal[0] = triangle->plane[0];
                        trace->normal[1] = triangle->plane[1];
                        trace->normal[2] = triangle->plane[2];
                        trace->fraction = 0.0f;
                        trace->startsolid = 1;
                        return;
                    }
                }
            }

        height_adjusted_test:
            if (cullEndRadius <= altEndDist) {
                float adjScale = (cullEndRadius - dist) / normalZ;
                float s_adj = triangle->svec[2] * adjScale + s0;
                if (0.0f > s_adj)
                    return;
                {
                    float t_adj = triangle->tvec[2] * adjScale + t0;
                    if (0.0f > t_adj)
                        return;
                    if (s_adj + t_adj <= 1.0f) {
                        trace->normal[0] = triangle->plane[0];
                        trace->normal[1] = triangle->plane[1];
                        trace->normal[2] = triangle->plane[2];
                        trace->fraction = 0;
                        trace->startsolid = 1;
                        return;
                    }
                }
                return;
            } else {
                float adjScale = height;
                float s_adj = triangle->svec[2] * adjScale + s0;
                if (0.0f > s_adj)
                    return;
                {
                    float t_adj = triangle->tvec[2] * adjScale + t0;
                    if (0.0f > t_adj)
                        return;
                    if (s_adj + t_adj > 1.0f)
                        return;
                }
            }
        }
    }

    return;

edge_trace_tests: {
    float radiusSq = cullEndRadius * cullEndRadius;
    int triOffset = 0;

    for (sideIndex = 0; sideIndex < 3; sideIndex++, triOffset += 4) {
        if (!((sideFlags >> sideIndex) & 1)) {

            int vertIdx = triangle->verts[sideIndex];
            if (vertIdx < 0)
                continue;

            {
                int *vertCheck = &tw->threadInfo.checkcount.verts[vertIdx];
                if (*vertCheck == tw->threadInfo.checkcount.global)
                    continue;
                *vertCheck = tw->threadInfo.checkcount.global;
            }

            {
                CollisionVertex *vert = &cm_ptr->verts[vertIdx];
                float dx = startX - vert->xyz[0];
                float dy = startY - vert->xyz[1];
                float dz = startZ - vert->xyz[2];
                float dirDotDelta = dx * tw->delta[0] + dy * tw->delta[1] + dz * tw->delta[2];

                if (dirDotDelta >= 0.0f)
                    continue;

                {
                    float deltaLenSq = dx * dx + dy * dy + dz * dz;
                    float discrim = deltaLenSq - radiusSq;

                    if (0.0f >= discrim) {
                        float invLen = 1.0f / sqrtf(deltaLenSq);
                        trace->normal[0] = dx * invLen;
                        trace->normal[1] = dy * invLen;
                        trace->normal[2] = dz * invLen;

                        CM_SetTraceNormalToPlane(trace, triangle, startZ, fromZ);

                        trace->fraction = 0;
                        if (tw->radius * tw->radius > deltaLenSq)
                            trace->startsolid = 1;
                        continue;
                    }

                    {
                        float a = tw->deltaLenSq;
                        float b2 = dirDotDelta * dirDotDelta - a * discrim;
                        if (0.0f > b2)
                            continue;

                        {
                            float t_hit = (-dirDotDelta - sqrtf(b2)) / a;
                            if (t_hit >= trace->fraction)
                                continue;

                            trace->normal[0] = t_hit * tw->delta[0] + dx;
                            trace->normal[1] = t_hit * tw->delta[1] + dy;
                            trace->normal[2] = t_hit * tw->delta[2] + dz;

                            {
                                float invR = 1.0f / cullEndRadius;
                                trace->normal[0] *= invR;
                                trace->normal[1] *= invR;
                                trace->normal[2] *= invR;
                            }

                            CM_SetTraceNormalToPlane(trace, triangle, startZ, fromZ);
                            trace->fraction = t_hit;
                        }
                    }
                }
            }
            continue;
        }

        {
            int edgeIdx = triangle->edges[sideIndex];
            if (edgeIdx < 0)
                continue;

            {
                int *edgeCheck = &tw->threadInfo.checkcount.edges[edgeIdx];
                if (*edgeCheck == tw->threadInfo.checkcount.global)
                    continue;
                *edgeCheck = tw->threadInfo.checkcount.global;
            }

            {
                CollisionEdge *edge = &cm_ptr->edges[edgeIdx];
                float ex = startX - edge->origin[0];
                float ey = startY - edge->origin[1];
                float ez = startZ - edge->origin[2];

                /* Axis 2 maps the finite edge to [0, 1]. The other two
                 * axes span the perpendicular plane of its cylinder. */
                float edgeAx0 = edge->axis[0][0];
                float edgeAx1 = edge->axis[0][1];
                float edgeAx2 = edge->axis[0][2];
                float edgeBx0 = edge->axis[1][0];
                float edgeBx1 = edge->axis[1][1];
                float edgeBx2 = edge->axis[1][2];

                float projA = ex * edgeAx0 + ey * edgeAx1 + ez * edgeAx2;
                float projB = ex * edgeBx0 + ey * edgeBx1 + ez * edgeBx2;

                float dirA = tw->delta[0] * edgeAx0 + tw->delta[1] * edgeAx1 + tw->delta[2] * edgeAx2;
                float dirB = tw->delta[0] * edgeBx0 + tw->delta[1] * edgeBx1 + tw->delta[2] * edgeBx2;

                float approach = projA * dirA + projB * dirB;
                if (approach >= 0.0f)
                    continue;

                {
                    float perpDistSq = projA * projA + projB * projB;
                    float perpDiscrim = perpDistSq - radiusSq;

                    if (0.0f >= perpDiscrim) {

                        float edgeProj = ex * edge->axis[2][0] + ey * edge->axis[2][1] + ez * edge->axis[2][2];
                        if (fabsf(edgeProj - 0.5f) > 0.5f)
                            continue;

                        trace->normal[0] = projA * edgeAx0;
                        trace->normal[1] = projA * edgeAx1;
                        trace->normal[2] = projA * edgeAx2;

                        trace->normal[0] += projB * edgeBx0;
                        trace->normal[1] += projB * edgeBx1;
                        trace->normal[2] += projB * edgeBx2;

                        Vec3Normalize(trace->normal);

                        CM_SetTraceNormalToPlane(trace, triangle, startZ, fromZ);

                        trace->fraction = 0;
                        if (tw->radius * tw->radius > perpDistSq)
                            trace->startsolid = 1;
                        continue;
                    }

                    {
                        float perpLenSq = dirA * dirA + dirB * dirB;
                        float b2 = approach * approach - perpLenSq * perpDiscrim;
                        if (0.0f >= b2)
                            continue;

                        {
                            float t_hit;
                            float sqrtVal = sqrtf(b2);
                            t_hit = (-approach - sqrtVal) / perpLenSq;

                            if (t_hit >= trace->fraction)
                                continue;

                            {
                                float hitEx = tw->delta[0] * t_hit + ex;
                                float hitEy = tw->delta[1] * t_hit + ey;
                                float hitEz = tw->delta[2] * t_hit + ez;
                                float hitProj = hitEx * edge->axis[2][0] + hitEy * edge->axis[2][1] + hitEz * edge->axis[2][2];
                                hitProj -= 0.5f;
                                if (fabsf(hitProj) > 0.5f)
                                    continue;

                                {
                                    float normA = (t_hit * dirA + projA) / cullEndRadius;
                                    float normB = (t_hit * dirB + projB) / cullEndRadius;

                                    trace->normal[0] = normA * edgeAx0;
                                    trace->normal[1] = normA * edgeAx1;
                                    trace->normal[2] = normA * edgeAx2;

                                    trace->normal[0] += normB * edgeBx0;
                                    trace->normal[1] += normB * edgeBx1;
                                    trace->normal[2] += normB * edgeBx2;
                                }

                                CM_SetTraceNormalToPlane(trace, triangle, startZ, fromZ);
                                trace->fraction = t_hit;
                            }
                        }
                    }
                }
            }
        }
    }
}
}

static void CM_MESH_REGPARM3_ABI CM_TraceThroughAabbTree_r(const traceWork_t *tw, const CollisionAabbTree *aabbTree, trace_t *trace)
{
    int i;

    if (CM_CullBox(tw, aabbTree->origin, aabbTree->halfSize))
        return;

    if (aabbTree->childCount != 0) {
        CollisionAabbTree *child = &cm_ptr->aabbTrees[aabbTree->u.firstChildIndex];
        for (i = 0; i < aabbTree->childCount; i++) {
            CM_TraceThroughAabbTree_r(tw, &child[i], trace);
        }
        return;
    }

    {
        int partIdx = aabbTree->u.partitionIndex;
        short unsigned int *partCheck = &tw->threadInfo.checkcount.partitions[partIdx];

        if (*partCheck == (short unsigned int)tw->threadInfo.checkcount.global)
            return;
        *partCheck = (short unsigned int)tw->threadInfo.checkcount.global;

        {
            CollisionPartition *partition = &cm_ptr->partitions[partIdx];

            if (tw->isPoint) {
                if (partition->triCount == 0)
                    return;

                {
                    CollisionTriangle *tri = partition->tris;
                    for (i = 0; i < partition->triCount; i++) {
                        float plane0 = tri[i].plane[0];
                        float plane1 = tri[i].plane[1];
                        float plane2 = tri[i].plane[2];
                        float planeD = tri[i].plane[3];

                        float endDist = plane0 * tw->extents.end[0] +
                                        plane1 * tw->extents.end[1] +
                                        plane2 * tw->extents.end[2] - planeD;
                        if (endDist >= 0.0f)
                            continue;

                        float startDist = plane0 * tw->extents.start[0] +
                                          plane1 * tw->extents.start[1] +
                                          plane2 * tw->extents.start[2] - planeD;
                        if (startDist <= 0.0f)
                            continue;

                        float denom = startDist - endDist;
                        float frac = (startDist - 0.125f) / denom;
                        if (frac < 0.0f)
                            frac = 0.0f;

                        if (frac >= trace->fraction)
                            continue;

                        float hitFrac = startDist / denom;
                        float hitX = hitFrac * tw->delta[0] + tw->extents.start[0];
                        float hitY = hitFrac * tw->delta[1] + tw->extents.start[1];
                        float hitZ = hitFrac * tw->delta[2] + tw->extents.start[2];

                        float s_val = hitX * tri[i].svec[0] + hitY * tri[i].svec[1] +
                                      hitZ * tri[i].svec[2] - tri[i].svec[3];
                        if (!(s_val >= -0.001f))
                            continue;
                        if (s_val > 1.001f)
                            continue;

                        float t_val = hitX * tri[i].tvec[0] + hitY * tri[i].tvec[1] +
                                      hitZ * tri[i].tvec[2] - tri[i].tvec[3];
                        if (-0.001f > t_val)
                            continue;
                        if (s_val + t_val > 1.001f)
                            continue;

                        trace->fraction = frac;
                        trace->normal[0] = tri[i].plane[0];
                        trace->normal[1] = tri[i].plane[1];
                        trace->normal[2] = tri[i].plane[2];
                    }
                }
                return;
            }

            if (partition->triCount == 0) {

                if (tw->delta[0] == 0.0f && tw->delta[1] == 0.0f) {
                    if (tw->offsetZ == 0.0f)
                        return;
                }

                if (partition->borderCount == 0)
                    return;

                {
                    CollisionBorder *border = partition->borders;
                    for (i = 0; i < partition->borderCount; i++) {
                        float dirX = tw->delta[0];
                        float dirY = tw->delta[1];
                        float dirZ = tw->delta[2];
                        float bNormX = border[i].distEq[0];
                        float bNormY = border[i].distEq[1];

                        float cross = dirX * bNormX + dirY * bNormY;
                        if (cross >= 0.0f)
                            continue;

                        float radius = 0.125f + tw->radius;
                        float borderDist = tw->extents.start[0] * bNormX +
                                           tw->extents.start[1] * bNormY -
                                           border[i].distEq[2];
                        float enterFrac = (radius - borderDist) / cross;

                        if (enterFrac >= trace->fraction)
                            continue;

                        if (-radius > enterFrac * tw->deltaLen)
                            continue;

                        float startZ = tw->extents.start[2];
                        float hitZ = dirZ * enterFrac + startZ;
                        float hitX = dirX * enterFrac + tw->extents.start[0];
                        float hitY = dirY * enterFrac + tw->extents.start[1];

                        float lateral = hitX * bNormY - hitY * bNormX - border[i].start;

                        if (lateral < 0.0f) {

                            float cornerX_coeff = border[i].start;
                            float cornerProjA = bNormY * cornerX_coeff;
                            float cornerProjB = bNormX * border[i].distEq[2];
                            float cX = tw->extents.start[0] - (cornerProjA + cornerProjB);

                            float bNormY_times_zBase = bNormY * border[i].distEq[2];
                            float bNormX_times_start = bNormX * border[i].start;
                            float cY = tw->extents.start[1] - (bNormY_times_zBase - bNormX_times_start);

                            float cross2d = dirX * cY + dirY * cX;
                            if (cross2d >= 0.0f)
                                continue;

                            float distSq2d = cX * cX + cY * cY;
                            float radiusSq = radius * radius;
                            float discrim2d = distSq2d - radiusSq;
                            if (0.0f >= discrim2d) {

                                float zAtBorder = border[i].zBase;
                                float zDiff = startZ - zAtBorder;
                                if (fabsf(zDiff) > tw->offsetZ)
                                    continue;

                                trace->fraction = 0;
                                trace->normal[0] = bNormX;
                                trace->normal[1] = bNormY;
                                trace->normal[2] = 0.0f;

                                if (tw->radius * tw->radius > distSq2d)
                                    trace->startsolid = 1;
                                continue;
                            }

                            {
                                float a = tw->deltaLenSq;
                                float b2 = cross2d * cross2d - a * discrim2d;
                                if (0.0f > b2)
                                    continue;

                                float t_hit = (-cross2d - sqrtf(b2)) / a;
                                if (t_hit >= trace->fraction)
                                    continue;
                                if (0.0f > t_hit)
                                    continue;

                                float hitZ2 = dirZ * t_hit + startZ;
                                float zSloped = border[i].length;

                                float zAtHit = border[i].length * border[i].zSlope + border[i].zBase;
                                float zDiff2 = hitZ2 - zAtHit;
                                if (fabsf(zDiff2) > tw->offsetZ)
                                    continue;

                                trace->fraction = t_hit;
                                trace->normal[0] = bNormX;
                                trace->normal[1] = bNormY;
                                trace->normal[2] = 0.0f;
                            }
                            continue;
                        }

                        if (lateral > border[i].length) {

                            float cornerX_coeff = border[i].start + border[i].length;
                            float cornerProjA = bNormY * cornerX_coeff;
                            float cornerProjB = bNormX * border[i].distEq[2];
                            float cX = tw->extents.start[0] - (cornerProjA + cornerProjB);

                            float bNormY_times_zBase = bNormY * border[i].distEq[2];
                            float bNormX_times_start = bNormX * cornerX_coeff;
                            float cY = tw->extents.start[1] - (bNormY_times_zBase - bNormX_times_start);

                            float cross2d = dirX * cY + dirY * cX;
                            if (cross2d >= 0.0f)
                                continue;

                            float distSq2d = cX * cX + cY * cY;
                            float radiusSq = radius * radius;
                            float discrim2d = distSq2d - radiusSq;
                            if (0.0f >= discrim2d) {
                                float zAtBorder = lateral * border[i].zSlope + border[i].zBase;
                                float zDiff = startZ - zAtBorder;
                                if (fabsf(zDiff) > tw->offsetZ)
                                    continue;

                                trace->fraction = 0;
                                trace->normal[0] = bNormX;
                                trace->normal[1] = bNormY;
                                trace->normal[2] = 0.0f;

                                if (tw->radius * tw->radius > distSq2d)
                                    trace->startsolid = 1;
                                continue;
                            }

                            {
                                float a = tw->deltaLenSq;
                                float b2 = cross2d * cross2d - a * discrim2d;
                                if (0.0f > b2)
                                    continue;

                                float t_hit = (-cross2d - sqrtf(b2)) / a;
                                if (t_hit >= trace->fraction)
                                    continue;
                                if (0.0f > t_hit)
                                    continue;

                                float hitZ2 = dirZ * t_hit + startZ;
                                float zAtHit = border[i].length * border[i].zSlope + border[i].zBase;
                                float zDiff2 = hitZ2 - zAtHit;
                                if (fabsf(zDiff2) > tw->offsetZ)
                                    continue;

                                trace->fraction = t_hit;
                                trace->normal[0] = bNormX;
                                trace->normal[1] = bNormY;
                                trace->normal[2] = 0.0f;
                            }
                            continue;
                        }

                        {
                            float frac_clamped = enterFrac;
                            if (frac_clamped < 0.0f)
                                frac_clamped = 0.0f;

                            float hitZ2 = dirZ * frac_clamped + startZ;
                            float zAtHit = lateral * border[i].zSlope + border[i].zBase;
                            float zDiff = hitZ2 - zAtHit;
                            if (fabsf(zDiff) > tw->offsetZ)
                                continue;

                            trace->fraction = enterFrac;
                            trace->normal[0] = bNormX;
                            trace->normal[1] = bNormY;
                            trace->normal[2] = 0.0f;
                        }
                    }
                }
                return;
            }

            {
                CollisionTriangle *tri = partition->tris;
                for (i = 0; i < partition->triCount; i++) {
                    CM_TraceCapsuleThroughTriangle(tw, &tri[i], trace, tw->offsetZ);

                    if (tri[i].plane[2] > 0.0f) {
                        CM_TraceCapsuleThroughTriangle(tw, &tri[i], trace, -tw->offsetZ);
                    }
                }
            }
        }
    }
}

void CM_TraceThroughAabbTree(const traceWork_t *tw, CollisionAabbTree *aabbTree, trace_t *trace)
{
    dmaterial_t *materialInfo;
    float oldFraction;

    materialInfo = &cm_ptr->materials[aabbTree->materialIndex];

    if (!(tw->contents & materialInfo->contentFlags))
        return;

    oldFraction = trace->fraction;
    CM_TraceThroughAabbTree_r(tw, aabbTree, trace);

    if (oldFraction > trace->fraction) {
        trace->surfaceFlags = materialInfo->surfaceFlags;
        trace->contents = materialInfo->contentFlags;
        trace->material = (const char *)materialInfo;
    }
}

void CM_SightTraceThroughAabbTree(const traceWork_t *tw, CollisionAabbTree *aabbTree, trace_t *trace)
{
    dmaterial_t *materialInfo;
    float oldFraction;

    materialInfo = &cm_ptr->materials[aabbTree->materialIndex];

    if (!(tw->contents & materialInfo->contentFlags))
        return;

    oldFraction = trace->fraction;
    CM_TraceThroughAabbTree_r(tw, aabbTree, trace);

    if (oldFraction > trace->fraction) {
        trace->surfaceFlags = materialInfo->surfaceFlags;
        trace->contents = materialInfo->contentFlags;
        trace->material = (const char *)materialInfo;
    }
}

static void CM_MESH_REGPARM3_ABI CM_PositionTestInAabbTree_r(const traceWork_t *tw, const CollisionAabbTree *aabbTree, trace_t *trace)
{
    int i;

    if (CM_CullBox(tw, aabbTree->origin, aabbTree->halfSize))
        return;

    if (aabbTree->childCount != 0) {
        CollisionAabbTree *child = &cm_ptr->aabbTrees[aabbTree->u.firstChildIndex];
        for (i = 0; i < aabbTree->childCount; i++) {
            CM_PositionTestInAabbTree_r(tw, &child[i], trace);
        }
        return;
    }

    {
        int partIdx = aabbTree->u.partitionIndex;
        short unsigned int *partCheck = &tw->threadInfo.checkcount.partitions[partIdx];

        if (*partCheck == (short unsigned int)tw->threadInfo.checkcount.global)
            return;
        *partCheck = (short unsigned int)tw->threadInfo.checkcount.global;

        {
            CollisionPartition *partition = &cm_ptr->partitions[partIdx];

            if (partition->triCount == 0)
                return;

            {
                CollisionTriangle *tri = partition->tris;
                for (i = 0; i < partition->triCount; i++) {
                    CM_PositionTestCapsuleInTriangle(tw, &tri[i], trace);
                }
            }
        }
    }
}

short int CM_MeshTestInLeaf(const traceWork_t *tw, cLeaf_t *leaf, trace_t *trace)
{
    int k;

    for (k = 0; k < leaf->collAabbCount; k++) {
        CollisionAabbTree *tree = &cm_ptr->aabbTrees[leaf->firstCollAabbIndex + k];
        dmaterial_t *materialInfo = &cm_ptr->materials[tree->materialIndex];

        if (!(tw->contents & materialInfo->contentFlags))
            continue;

        if (CM_CullBox(tw, tree->origin, tree->halfSize))
            goto check_allsolid;

        if (tree->childCount != 0) {
            CollisionAabbTree *child = &cm_ptr->aabbTrees[tree->u.firstChildIndex];
            int ci;
            for (ci = 0; ci < tree->childCount; ci++) {
                CM_PositionTestInAabbTree_r(tw, &child[ci], trace);
            }
            goto check_allsolid;
        }

        {
            int partIdx = tree->u.partitionIndex;
            short unsigned int *partCheck = &tw->threadInfo.checkcount.partitions[partIdx];

            if (*partCheck == (short unsigned int)tw->threadInfo.checkcount.global)
                goto check_allsolid;
            *partCheck = (short unsigned int)tw->threadInfo.checkcount.global;

            {
                CollisionPartition *partition = &cm_ptr->partitions[partIdx];

                if (partition->triCount > 0) {
                    CollisionTriangle *tri = partition->tris;
                    int ti;
                    for (ti = 0; ti < partition->triCount; ti++) {
                        CM_PositionTestCapsuleInTriangle(tw, &tri[ti], trace);
                    }
                }
            }
        }

    check_allsolid:
        if (trace->allsolid) {
            trace->surfaceFlags = materialInfo->surfaceFlags;
            trace->contents = materialInfo->contentFlags;
            trace->material = (const char *)materialInfo;
            return 0;
        }
    }

    return 0;
}
