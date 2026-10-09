#include "common_types.h"
#include "imports.h"
#include <string.h>
extern server_t sv;
extern clipMap_t cm;

static struct cm_world_t cm_world;

void CM_UnlinkEntity(svEntity_t *ent);
extern void CM_CalcTraceEntents(TraceExtents *extents);
extern gentity_t *SV_GEntityForSvEntity(svEntity_t *svEnt);
extern int SV_PointSightTraceToEntity(const sightpointtrace_t *clip, svEntity_t *check);
extern int XModelGetContents(const XModel *model);
extern qboolean CM_TraceBox(const TraceExtents *extents, const vec_t *mins, const vec_t *maxs, float fraction);
extern qboolean CM_TraceStaticModelComplete(cStaticModel_t *sm, const vec_t *start, const vec_t *end, int contentmask);
extern void CM_TraceStaticModel(cStaticModel_t *sm, trace_t *results, const vec_t *start, const vec_t *end, int contentmask);
extern int SV_ClipSightToEntity(const sightclip_t *clip, svEntity_t *check);
extern void SV_PointTraceToEntity(const pointtrace_t *clip, svEntity_t *check, trace_t *trace);
extern void SV_ClipMoveToEntity(const moveclip_t *clip, svEntity_t *check, trace_t *trace);
extern void CM_ModelBounds(clipHandle_t model, vec_t *mins, vec_t *maxs);
extern cmodel_t *CM_ClipHandleToModel(clipHandle_t handle);
extern void Com_DPrintf(const char *fmt, ...);
static void __attribute_regparm__(2) CM_AreaEntities_r(int nodeIndex, areaParms_t *ap);
int CM_AreaEntities(const vec_t *mins, const vec_t *maxs, int *entityList, int maxcount, int contentmask);
static int __attribute_regparm__(3) CM_PointSightTraceToEntities_r(sightpointtrace_t *clip, int nodeIndex, const vec_t *p1, const vec_t *p2);
int CM_PointSightTraceToEntities(sightpointtrace_t *clip);
static qboolean __attribute_regparm__(3) CM_PointTraceStaticModelsComplete_r(const staticmodeltrace_t *clip, int nodeIndex, const vec_t *p1, const vec_t *p2);
qboolean CM_PointTraceStaticModelsComplete(const vec_t *start, const vec_t *end, int contentmask);
static int __attribute_regparm__(3) CM_ClipSightTraceToEntities_r(sightclip_t *clip, int nodeIndex, const vec_t *p1, const vec_t *p2);
int CM_ClipSightTraceToEntities(sightclip_t *clip);
static void __attribute_regparm__(3) CM_PointTraceStaticModels_r(locTraceWork_t *tw, int nodeIndex, const vec_t *p1, const vec_t *p2, trace_t *trace);
void CM_PointTraceStaticModels(trace_t *results, const vec_t *start, const vec_t *end, int contentmask);
static void __attribute_regparm__(3) CM_PointTraceToEntities_r(pointtrace_t *clip, int nodeIndex, const vec_t *p1, const vec_t *p2, trace_t *trace);
void CM_PointTraceToEntities(pointtrace_t *clip, trace_t *trace);
static void __attribute_regparm__(3) CM_ClipMoveToEntities_r(moveclip_t *clip, int nodeIndex, const vec_t *p1, const vec_t *p2, trace_t *trace);
void CM_ClipMoveToEntities(moveclip_t *clip, trace_t *trace);
static void __attribute_regparm__(3) CM_SortNode(short unsigned int nodeIndex, vec_t *mins, vec_t *maxs);
void CM_LinkWorld(void);
void CM_LinkEntity(svEntity_t *ent, vec_t *absmin, vec_t *absmax, clipHandle_t clipHandle);

void CM_UnlinkEntity(svEntity_t *ent)
{
    byte *entRaw = (byte *)ent;
    unsigned short nodeIndex;
    worldSector_t *node;

    nodeIndex = ent->worldSector;
    if (nodeIndex == 0)
        return;

    {
        unsigned int idx = (unsigned int)nodeIndex;
        node = &cm_world.sectors[idx];
    }

    ent->worldSector = 0;

    {
        worldSector_t *sector = (worldSector_t *)node;
        unsigned short headEntIdx = sector->contents.entities;
        unsigned int ei = (unsigned int)headEntIdx;
        byte *sv = (byte *)imp_sv;
        svEntity_t *cur = &((server_t *)sv)->svEntities[ei - 1];

        if (cur == ent) {

            sector->contents.entities = ent->nextEntityInWorldSector;
        } else {

            while (1) {
                unsigned short nextIdx = cur->nextEntityInWorldSector;
                unsigned int ni = (unsigned int)nextIdx;
                svEntity_t *next = &((server_t *)sv)->svEntities[ni - 1];
                if (next == ent) {

                    cur->nextEntityInWorldSector = ent->nextEntityInWorldSector;
                    break;
                }
                cur = next;
                sv = (byte *)imp_sv;
            }
        }
    }

check_empty:
    if (node->contents.entities != 0 || node->contents.staticModels != 0)
        goto update_contents;

    if (node->tree.child[0] != 0 || node->tree.child[1] != 0)
        goto update_contents;

    node->contents.contentsEntities = 0;

    {
        unsigned short parentIdx = node->tree.u.parent;
        if (parentIdx == 0)
            goto update_contents;

        {
            unsigned short oldFreeHead = cm_world.freeHead;
            node->tree.u.nextFree = oldFreeHead;
            cm_world.freeHead = nodeIndex;
        }

        {
            unsigned int pi = (unsigned int)parentIdx;
            worldSector_t *parentNode = &cm_world.sectors[pi];

            if (parentNode->tree.child[0] == nodeIndex) {
                parentNode->tree.child[0] = 0;
            } else {
                parentNode->tree.child[1] = 0;
            }

            nodeIndex = parentIdx;
            node = parentNode;
        }
    }

    if (node->contents.entities == 0 && node->contents.staticModels == 0)
        goto check_empty;

update_contents:

    {
        unsigned short child0Idx = node->tree.child[0];
        unsigned short child1Idx = node->tree.child[1];
        int contents;

        {
            unsigned int c0 = (unsigned int)child0Idx;
            unsigned int c1 = (unsigned int)child1Idx;
            contents = cm_world.sectors[c0].contents.contentsEntities;
            contents |= cm_world.sectors[c1].contents.contentsEntities;
        }

        {
            unsigned short entIdx = node->contents.entities;
            if (entIdx != 0) {
                byte *sv = (byte *)imp_sv;
                svEntity_t *svEnt = &((server_t *)sv)->svEntities[(unsigned int)entIdx - 1];

                while (1) {
                    gentity_t *gent = SV_GEntityForSvEntity(svEnt);
                    contents |= gent->r.contents;
                    unsigned short nextIdx = svEnt->nextEntityInWorldSector;
                    if (nextIdx == 0)
                        break;
                    {
                        unsigned int ni = (unsigned int)nextIdx;
                        svEnt = &((server_t *)imp_sv)->svEntities[(ni) - 1];
                    }
                }
            }
        }

        node->contents.contentsEntities = contents;

        {
            unsigned short parentIdx = node->tree.u.parent;
            if (parentIdx == 0)
                return;
            {
                unsigned int pi = (unsigned int)parentIdx;
                node = &cm_world.sectors[pi];
                goto update_contents;
            }
        }
    }
}

static void __attribute_regparm__(2)
    CM_AreaEntities_r(int nodeIndex, areaParms_t *ap)
{
    worldSector_t *node;
    unsigned short entIndex;

    node = &cm_world.sectors[(unsigned short)nodeIndex];
    if ((node->contents.contentsEntities & ap->contentmask) == 0)
        return;

    for (entIndex = node->contents.entities; entIndex != 0;) {
        svEntity_t *check = &((server_t *)imp_sv)->svEntities[((unsigned int)entIndex) - 1];
        gentity_t *gEnt = SV_GEntityForSvEntity(check);

        if ((gEnt->r.contents & ap->contentmask) != 0 &&
            gEnt->r.absmin[0] <= ap->maxs[0] && ap->mins[0] <= gEnt->r.absmax[0] &&
            gEnt->r.absmin[1] <= ap->maxs[1] && ap->mins[1] <= gEnt->r.absmax[1] &&
            gEnt->r.absmin[2] <= ap->maxs[2] && ap->mins[2] <= gEnt->r.absmax[2]) {
            if (ap->count == ap->maxcount) {
                Com_DPrintf("CM_AreaEntities: MAXCOUNT\n");
                return;
            }
            ap->list[ap->count++] = (int)entIndex - 1;
        }

        entIndex = check->nextEntityInWorldSector;
    }

    if (ap->maxs[node->tree.axis] > node->tree.dist)
        CM_AreaEntities_r(node->tree.child[0], ap);

    if (node->tree.dist > ap->mins[node->tree.axis])
        CM_AreaEntities_r(node->tree.child[1], ap);
}

int CM_AreaEntities(const vec_t *mins, const vec_t *maxs, int *entityList, int maxcount, int contentmask)
{
    areaParms_t ap;

    ap.mins = mins;
    ap.maxs = maxs;
    ap.list = entityList;
    ap.count = 0;
    ap.maxcount = maxcount;
    ap.contentmask = contentmask;

    CM_AreaEntities_r(1, &ap);
    return ap.count;
}

static int __attribute_regparm__(3)
    CM_PointSightTraceToEntities_r(sightpointtrace_t *clip, int nodeIndex, const vec_t *p1, const vec_t *p2)
{
    worldSector_t *node = &cm_world.sectors[(unsigned short)nodeIndex];
    float dist1;
    float dist2;
    vec3_t mid;
    int side;
    unsigned short entIndex;

    if ((node->contents.contentsEntities & clip->contentmask) == 0)
        return 0;

    dist1 = p1[node->tree.axis] - node->tree.dist;
    dist2 = p2[node->tree.axis] - node->tree.dist;

    if (dist1 * dist2 >= 0.0f) {
        float minDist = (dist2 < dist1) ? dist2 : dist1;
        int childIndex = 1 - (0.0f <= minDist);

        if (CM_PointSightTraceToEntities_r(clip, node->tree.child[childIndex], p1, p2))
            return 1;
    } else {
        float frac = dist1 / (dist1 - dist2);

        mid[0] = p1[0] + (p2[0] - p1[0]) * frac;
        mid[1] = p1[1] + (p2[1] - p1[1]) * frac;
        mid[2] = p1[2] + (p2[2] - p1[2]) * frac;

        side = (0.0f <= dist2);
        if (CM_PointSightTraceToEntities_r(clip, node->tree.child[side], p1, mid))
            return 1;
        if (CM_PointSightTraceToEntities_r(clip, node->tree.child[1 - side], mid, p2))
            return 1;
    }

    for (entIndex = node->contents.entities; entIndex != 0;) {
        svEntity_t *check = &((server_t *)imp_sv)->svEntities[((unsigned int)entIndex) - 1];

        if (SV_PointSightTraceToEntity(clip, check))
            return 1;

        entIndex = check->nextEntityInWorldSector;
    }

    return 0;
}

int CM_PointSightTraceToEntities(sightpointtrace_t *clip)
{
    return CM_PointSightTraceToEntities_r(clip, 1, clip->start, clip->end);
}

static qboolean __attribute_regparm__(3)
    CM_PointTraceStaticModelsComplete_r(const staticmodeltrace_t *clip, int nodeIndex, const vec_t *p1, const vec_t *p2)
{
    vec3_t p1Local;

    p1Local[0] = p1[0];
    p1Local[1] = p1[1];
    p1Local[2] = p1[2];

    for (;;) {
        worldSector_t *node = &cm_world.sectors[(unsigned short)nodeIndex];

        if ((node->contents.contentsStaticModels & clip->contents) == 0)
            return 1;

        if (node->contents.staticModels) {
            unsigned short modelIndex = node->contents.staticModels;

            do {
                cStaticModel_t *check = &cm.staticModelList[modelIndex - 1];

                if (XModelGetContents(check->xmodel) & clip->contents) {
                    if (!CM_TraceBox(&clip->extents, check->absmin, check->absmax, 1.0f) &&
                        !CM_TraceStaticModelComplete(check, clip->extents.start, clip->extents.end, clip->contents)) {
                        return 0;
                    }
                }

                modelIndex = check->writable.nextModelInWorldSector;
            } while (modelIndex);
        }

        {
            int axis = node->tree.axis;
            float dist1 = p1Local[axis] - node->tree.dist;
            float dist2 = p2[axis] - node->tree.dist;

            if (dist1 * dist2 >= 0.0f) {
                float minDist = (dist2 < dist1) ? dist2 : dist1;
                int side = (minDist < 0.0f);
                nodeIndex = node->tree.child[side];
            } else {
                float frac = dist1 / (dist1 - dist2);
                vec3_t mid;
                int side;

                mid[0] = p1Local[0] + (p2[0] - p1Local[0]) * frac;
                mid[1] = p1Local[1] + (p2[1] - p1Local[1]) * frac;
                mid[2] = p1Local[2] + (p2[2] - p1Local[2]) * frac;

                side = (0.0f <= dist2);
                if (!CM_PointTraceStaticModelsComplete_r(clip, node->tree.child[side], p1Local, mid))
                    return 0;

                nodeIndex = node->tree.child[1 - side];
                p1Local[0] = mid[0];
                p1Local[1] = mid[1];
                p1Local[2] = mid[2];
            }
        }
    }
}

qboolean CM_PointTraceStaticModelsComplete(const vec_t *start, const vec_t *end, int contentmask)
{
    staticmodeltrace_t clip;

    clip.contents = contentmask;
    clip.extents.start[0] = start[0];
    clip.extents.start[1] = start[1];
    clip.extents.start[2] = start[2];
    clip.extents.end[0] = end[0];
    clip.extents.end[1] = end[1];
    clip.extents.end[2] = end[2];

    CM_CalcTraceEntents(&clip.extents);
    return CM_PointTraceStaticModelsComplete_r(&clip, 1, clip.extents.start, clip.extents.end);
}

static int __attribute_regparm__(3)
    CM_ClipSightTraceToEntities_r(sightclip_t *clip, int nodeIndex, const vec_t *p1, const vec_t *p2)
{
    vec3_t p;

    p[0] = p1[0];
    p[1] = p1[1];
    p[2] = p1[2];

    for (;;) {
        worldSector_t *node = &cm_world.sectors[(unsigned short)nodeIndex];
        unsigned short entIndex;
        int axis;
        float dist1;
        float dist2;
        float offset;
        float delta;
        float minDist;
        float maxDist;
        int firstSide;
        float firstFrac;
        float secondFrac;
        vec3_t mid;

        if ((node->contents.contentsEntities & clip->contentmask) == 0)
            return 0;

        for (entIndex = node->contents.entities; entIndex != 0;) {
            svEntity_t *check = &((server_t *)imp_sv)->svEntities[((unsigned int)entIndex) - 1];

            if (SV_ClipSightToEntity(clip, check))
                return 1;

            entIndex = check->nextEntityInWorldSector;
        }

        axis = node->tree.axis;
        dist1 = p[axis] - node->tree.dist;
        dist2 = p2[axis] - node->tree.dist;
        offset = clip->outerSize[axis];
        delta = dist2 - dist1;

        minDist = (delta >= 0.0f) ? dist1 : dist2;
        if (minDist >= offset) {
            nodeIndex = node->tree.child[0];
            continue;
        }

        maxDist = (delta >= 0.0f) ? dist2 : dist1;
        if (maxDist <= -offset) {
            nodeIndex = node->tree.child[1];
            continue;
        }

        if (delta == 0.0f) {
            firstSide = 0;
            firstFrac = 1.0f;
            secondFrac = 0.0f;
        } else {
            float absDelta = (delta < 0.0f) ? -delta : delta;
            float base = (delta >= 0.0f) ? -dist1 : dist1;
            float exitFrac = (base + offset) / absDelta;
            float enterFrac = (base - offset) / absDelta;

            firstSide = (delta >= 0.0f);
            firstFrac = (exitFrac <= 1.0f) ? exitFrac : 1.0f;
            secondFrac = (enterFrac >= 0.0f) ? enterFrac : 0.0f;
        }

        mid[0] = p[0] + (p2[0] - p[0]) * firstFrac;
        mid[1] = p[1] + (p2[1] - p[1]) * firstFrac;
        mid[2] = p[2] + (p2[2] - p[2]) * firstFrac;

        if (CM_ClipSightTraceToEntities_r(clip, node->tree.child[firstSide], p, mid))
            return 1;

        p[0] = p[0] + (p2[0] - p[0]) * secondFrac;
        p[1] = p[1] + (p2[1] - p[1]) * secondFrac;
        p[2] = p[2] + (p2[2] - p[2]) * secondFrac;
        nodeIndex = node->tree.child[1 - firstSide];
    }
}

int CM_ClipSightTraceToEntities(sightclip_t *clip)
{
    return CM_ClipSightTraceToEntities_r(clip, 1, clip->start, clip->end);
}

static void __attribute_regparm__(3)
    CM_PointTraceStaticModels_r(locTraceWork_t *tw, int nodeIndex, const vec_t *p1, const vec_t *p2, trace_t *trace)
{
    vec4_t p;

    p[0] = p1[0];
    p[1] = p1[1];
    p[2] = p1[2];
    p[3] = p1[3];

    for (;;) {
        worldSector_t *node = &cm_world.sectors[(unsigned short)nodeIndex];
        unsigned short modelIndex;
        int axis;
        float dist1;
        float dist2;

        if ((node->contents.contentsStaticModels & tw->contents) == 0)
            return;

        for (modelIndex = node->contents.staticModels; modelIndex != 0;) {
            cStaticModel_t *check = &cm.staticModelList[modelIndex - 1];

            if (XModelGetContents(check->xmodel) & tw->contents) {
                if (!CM_TraceBox(&tw->extents, check->absmin, check->absmax, trace->fraction))
                    CM_TraceStaticModel(check, trace, tw->extents.start, tw->extents.end, tw->contents);
            }

            modelIndex = check->writable.nextModelInWorldSector;
        }

        axis = node->tree.axis;
        dist1 = p[axis] - node->tree.dist;
        dist2 = p2[axis] - node->tree.dist;

        if (dist1 * dist2 >= 0.0f) {
            float minDist = (dist2 < dist1) ? dist2 : dist1;
            int side = (minDist < 0.0f);

            nodeIndex = node->tree.child[side];
            continue;
        }

        if (p[3] >= trace->fraction)
            return;

        {
            float frac = dist1 / (dist1 - dist2);
            vec4_t mid;
            int side = (0.0f <= dist2);

            mid[0] = p[0] + (p2[0] - p[0]) * frac;
            mid[1] = p[1] + (p2[1] - p[1]) * frac;
            mid[2] = p[2] + (p2[2] - p[2]) * frac;
            mid[3] = p[3] + (p2[3] - p[3]) * frac;

            CM_PointTraceStaticModels_r(tw, node->tree.child[side], p, mid, trace);

            p[0] = mid[0];
            p[1] = mid[1];
            p[2] = mid[2];
            p[3] = mid[3];
            nodeIndex = node->tree.child[1 - side];
        }
    }
}

void CM_PointTraceStaticModels(trace_t *results, const vec_t *start, const vec_t *end, int contentmask)
{
    locTraceWork_t tw;
    vec4_t startWork;
    vec4_t endWork;

    tw.contents = contentmask;
    tw.extents.start[0] = start[0];
    tw.extents.start[1] = start[1];
    tw.extents.start[2] = start[2];
    tw.extents.end[0] = end[0];
    tw.extents.end[1] = end[1];
    tw.extents.end[2] = end[2];

    CM_CalcTraceEntents(&tw.extents);

    startWork[0] = tw.extents.start[0];
    startWork[1] = tw.extents.start[1];
    startWork[2] = tw.extents.start[2];
    startWork[3] = 0.0f;

    endWork[0] = tw.extents.end[0];
    endWork[1] = tw.extents.end[1];
    endWork[2] = tw.extents.end[2];
    endWork[3] = results->fraction;

    CM_PointTraceStaticModels_r(&tw, 1, startWork, endWork, results);
}

static void __attribute_regparm__(3)
    CM_PointTraceToEntities_r(pointtrace_t *clip, int nodeIndex, const vec_t *p1, const vec_t *p2, trace_t *trace)
{
    vec4_t p;

    p[0] = p1[0];
    p[1] = p1[1];
    p[2] = p1[2];
    p[3] = p1[3];

    for (;;) {
        worldSector_t *node = &cm_world.sectors[(unsigned short)nodeIndex];
        unsigned short entIndex;
        int axis;
        float dist1;
        float dist2;

        if ((node->contents.contentsEntities & clip->contentmask) == 0)
            return;

        for (entIndex = node->contents.entities; entIndex != 0;) {
            svEntity_t *check = &((server_t *)imp_sv)->svEntities[((unsigned int)entIndex) - 1];

            SV_PointTraceToEntity(clip, check, trace);
            entIndex = check->nextEntityInWorldSector;
        }

        axis = node->tree.axis;
        dist1 = p[axis] - node->tree.dist;
        dist2 = p2[axis] - node->tree.dist;

        if (dist1 * dist2 >= 0.0f) {
            float minDist = (dist2 < dist1) ? dist2 : dist1;
            int side = (minDist < 0.0f);

            nodeIndex = node->tree.child[side];
            continue;
        }

        if (p[3] >= trace->fraction)
            return;

        {
            float frac = dist1 / (dist1 - dist2);
            vec4_t mid;
            int side = (0.0f <= dist2);

            mid[0] = p[0] + (p2[0] - p[0]) * frac;
            mid[1] = p[1] + (p2[1] - p[1]) * frac;
            mid[2] = p[2] + (p2[2] - p[2]) * frac;
            mid[3] = p[3] + (p2[3] - p[3]) * frac;

            CM_PointTraceToEntities_r(clip, node->tree.child[side], p, mid, trace);

            p[0] = mid[0];
            p[1] = mid[1];
            p[2] = mid[2];
            p[3] = mid[3];
            nodeIndex = node->tree.child[1 - side];
        }
    }
}

void CM_PointTraceToEntities(pointtrace_t *clip, trace_t *trace)
{
    vec4_t start;
    vec4_t end;

    start[0] = clip->extents.start[0];
    start[1] = clip->extents.start[1];
    start[2] = clip->extents.start[2];
    start[3] = 0.0f;

    end[0] = clip->extents.end[0];
    end[1] = clip->extents.end[1];
    end[2] = clip->extents.end[2];
    end[3] = trace->fraction;

    CM_PointTraceToEntities_r(clip, 1, start, end, trace);
}

static void __attribute_regparm__(3)
    CM_ClipMoveToEntities_r(moveclip_t *clip, int nodeIndex, const vec_t *p1, const vec_t *p2, trace_t *trace)
{
    vec4_t p;

    p[0] = p1[0];
    p[1] = p1[1];
    p[2] = p1[2];
    p[3] = p1[3];

    for (;;) {
        worldSector_t *node = &cm_world.sectors[(unsigned short)nodeIndex];
        unsigned short entIndex;
        int axis;
        float dist1;
        float dist2;
        float offset;
        float delta;
        float minDist;
        float maxDist;
        int firstSide;
        float firstFrac;
        float secondFrac;
        vec4_t mid;

        if ((node->contents.contentsEntities & clip->contentmask) == 0)
            return;

        for (entIndex = node->contents.entities; entIndex != 0;) {
            svEntity_t *check = &((server_t *)imp_sv)->svEntities[((unsigned int)entIndex) - 1];

            if (check->linkcontents & clip->contentmask)
                SV_ClipMoveToEntity(clip, check, trace);

            entIndex = check->nextEntityInWorldSector;
        }

        axis = node->tree.axis;
        dist1 = p[axis] - node->tree.dist;
        dist2 = p2[axis] - node->tree.dist;
        offset = clip->outerSize[axis];
        delta = dist2 - dist1;

        minDist = (delta >= 0.0f) ? dist1 : dist2;
        if (minDist >= offset) {
            nodeIndex = node->tree.child[0];
            continue;
        }

        maxDist = (delta >= 0.0f) ? dist2 : dist1;
        if (maxDist <= -offset) {
            nodeIndex = node->tree.child[1];
            continue;
        }

        if (p[3] >= trace->fraction)
            return;

        if (delta == 0.0f) {
            firstSide = 0;
            firstFrac = 1.0f;
            secondFrac = 0.0f;
        } else {
            float absDelta = (delta < 0.0f) ? -delta : delta;
            float base = (delta >= 0.0f) ? -dist1 : dist1;
            float exitFrac = (base + offset) / absDelta;
            float enterFrac = (base - offset) / absDelta;

            firstSide = (delta >= 0.0f);
            firstFrac = (exitFrac <= 1.0f) ? exitFrac : 1.0f;
            secondFrac = (enterFrac >= 0.0f) ? enterFrac : 0.0f;
        }

        mid[0] = p[0] + (p2[0] - p[0]) * firstFrac;
        mid[1] = p[1] + (p2[1] - p[1]) * firstFrac;
        mid[2] = p[2] + (p2[2] - p[2]) * firstFrac;
        mid[3] = p[3] + (p2[3] - p[3]) * firstFrac;

        CM_ClipMoveToEntities_r(clip, node->tree.child[firstSide], p, mid, trace);

        p[0] = p[0] + (p2[0] - p[0]) * secondFrac;
        p[1] = p[1] + (p2[1] - p[1]) * secondFrac;
        p[2] = p[2] + (p2[2] - p[2]) * secondFrac;
        p[3] = p[3] + (p2[3] - p[3]) * secondFrac;
        nodeIndex = node->tree.child[1 - firstSide];
    }
}

void CM_ClipMoveToEntities(moveclip_t *clip, trace_t *trace)
{
    vec4_t start;
    vec4_t end;

    start[0] = clip->extents.start[0];
    start[1] = clip->extents.start[1];
    start[2] = clip->extents.start[2];
    start[3] = 0.0f;

    end[0] = clip->extents.end[0];
    end[1] = clip->extents.end[1];
    end[2] = clip->extents.end[2];
    end[3] = trace->fraction;

    CM_ClipMoveToEntities_r(clip, 1, start, end, trace);
}

static unsigned short CM_WorldEntityIndex(const svEntity_t *ent)
{
    server_t *sv = (server_t *)imp_sv;
    /* Zero terminates every world-sector entity list. */
    return (unsigned short)(ent - sv->svEntities + 1);
}

static svEntity_t *CM_WorldEntityForIndex(unsigned short entIndex)
{
    return &sv.svEntities[(unsigned int)entIndex - 1];
}

static unsigned short CM_AllocWorldSector(const vec_t *mins, const vec_t *maxs)
{
    vec2_t size;
    unsigned short axis;
    unsigned short nodeIndex;
    worldSector_t *node;

    nodeIndex = cm_world.freeHead;
    if (nodeIndex == 0)
        return 0;

    size[0] = maxs[0] - mins[0];
    size[1] = maxs[1] - mins[1];
    axis = (size[1] >= size[0]) ? 1 : 0;
    if (size[axis] <= 512.0f)
        return 0;

    node = &cm_world.sectors[nodeIndex];
    cm_world.freeHead = node->tree.u.nextFree;
    node->tree.axis = axis;
    node->tree.dist = (maxs[axis] + mins[axis]) * 0.5f;
    return nodeIndex;
}

static unsigned short CM_EnsureChildNode(unsigned short nodeIndex, int childSide, const vec_t *mins, const vec_t *maxs)
{
    worldSector_t *node = &cm_world.sectors[nodeIndex];
    unsigned short childIndex = node->tree.child[childSide];

    if (childIndex == 0) {
        childIndex = CM_AllocWorldSector(mins, maxs);
        if (childIndex != 0) {
            node->tree.child[childSide] = childIndex;
            cm_world.sectors[childIndex].tree.u.parent = nodeIndex;
        }
    }

    return childIndex;
}

static void CM_InsertEntityIntoWorldSector(unsigned short nodeIndex, svEntity_t *ent)
{
    unsigned short entIndex = CM_WorldEntityIndex(ent);
    worldSector_t *node = &cm_world.sectors[nodeIndex];
    unsigned short *link = &node->contents.entities;

    while (*link && *link <= entIndex) {
        svEntity_t *linkEnt = CM_WorldEntityForIndex(*link);
        link = &linkEnt->nextEntityInWorldSector;
    }

    ent->worldSector = nodeIndex;
    ent->nextEntityInWorldSector = *link;
    *link = entIndex;
}

static void CM_InsertStaticModelIntoWorldSector(unsigned short nodeIndex, cStaticModel_t *staticModel)
{
    clipMap_t *clip = &cm;
    unsigned short modelIndex = (unsigned short)(staticModel - clip->staticModelList + 1);
    worldSector_t *node = &cm_world.sectors[nodeIndex];
    unsigned short *link = &node->contents.staticModels;

    while (*link && *link <= modelIndex) {
        cStaticModel_t *linkModel = &clip->staticModelList[(unsigned int)*link - 1];
        link = &linkModel->writable.nextModelInWorldSector;
    }

    staticModel->writable.nextModelInWorldSector = *link;
    *link = modelIndex;
}

static void __attribute_regparm__(3)
    CM_SortNode(short unsigned int nodeIndex, vec_t *mins, vec_t *maxs)
{
    worldSector_t *node;
    unsigned short axis;
    float dist;
    svEntity_t *prevEnt;
    unsigned short entIndex;
    cStaticModel_t *prevStaticModel;
    unsigned short modelIndex;

    if (cm_world.lockTree)
        return;

    node = &cm_world.sectors[(unsigned int)nodeIndex];
    axis = node->tree.axis;
    dist = node->tree.dist;

    prevEnt = NULL;
    entIndex = node->contents.entities;
    while (entIndex != 0) {
        svEntity_t *ent = CM_WorldEntityForIndex(entIndex);
        unsigned short nextEntIndex = ent->nextEntityInWorldSector;
        int childSide = -1;
        unsigned short childIndex = 0;
        vec2_t childMins;
        vec2_t childMaxs;

        if (ent->linkmin[axis] > dist) {
            childSide = 0;
            childMins[0] = mins[0];
            childMins[1] = mins[1];
            childMaxs[0] = maxs[0];
            childMaxs[1] = maxs[1];
            childMins[axis] = dist;
        } else if (dist > ent->linkmax[axis]) {
            childSide = 1;
            childMins[0] = mins[0];
            childMins[1] = mins[1];
            childMaxs[0] = maxs[0];
            childMaxs[1] = maxs[1];
            childMaxs[axis] = dist;
        }

        if (childSide >= 0) {
            childIndex = CM_EnsureChildNode(nodeIndex, childSide, childMins, childMaxs);
        }

        if (childIndex != 0) {
            if (prevEnt != NULL) {
                prevEnt->nextEntityInWorldSector = nextEntIndex;
            } else {
                node->contents.entities = nextEntIndex;
            }

            CM_InsertEntityIntoWorldSector(childIndex, ent);
            cm_world.sectors[childIndex].contents.contentsEntities |= SV_GEntityForSvEntity(ent)->r.contents;
        } else {
            prevEnt = ent;
        }

        entIndex = nextEntIndex;
    }

    prevStaticModel = NULL;
    modelIndex = node->contents.staticModels;
    while (modelIndex != 0) {
        clipMap_t *clip = &cm;
        cStaticModel_t *staticModel = &clip->staticModelList[(unsigned int)modelIndex - 1];
        unsigned short nextModelIndex = staticModel->writable.nextModelInWorldSector;
        int childSide = -1;
        unsigned short childIndex = 0;
        vec2_t childMins;
        vec2_t childMaxs;

        if (staticModel->absmin[axis] > dist) {
            childSide = 0;
            childMins[0] = mins[0];
            childMins[1] = mins[1];
            childMaxs[0] = maxs[0];
            childMaxs[1] = maxs[1];
            childMins[axis] = dist;
        } else if (dist > staticModel->absmax[axis]) {
            childSide = 1;
            childMins[0] = mins[0];
            childMins[1] = mins[1];
            childMaxs[0] = maxs[0];
            childMaxs[1] = maxs[1];
            childMaxs[axis] = dist;
        }

        if (childSide >= 0) {
            childIndex = CM_EnsureChildNode(nodeIndex, childSide, childMins, childMaxs);
        }

        if (childIndex != 0) {
            if (prevStaticModel != NULL) {
                prevStaticModel->writable.nextModelInWorldSector = nextModelIndex;
            } else {
                node->contents.staticModels = nextModelIndex;
            }

            CM_InsertStaticModelIntoWorldSector(childIndex, staticModel);
            cm_world.sectors[childIndex].contents.contentsStaticModels |= XModelGetContents(staticModel->xmodel);
        } else {
            prevStaticModel = staticModel;
        }

        modelIndex = nextModelIndex;
    }
}

void CM_LinkWorld(void)
{
    clipMap_t *clip = &cm;
    int modelIndex;
    unsigned short freeIndex;
    unsigned short axis;

    memset(&cm_world, 0, sizeof(cm_world));
    CM_ModelBounds(0, cm_world.mins, cm_world.maxs);

    cm_world.freeHead = 2;
    for (freeIndex = 2; freeIndex < 1023; ++freeIndex) {
        cm_world.sectors[freeIndex].tree.u.nextFree = freeIndex + 1;
    }
    cm_world.sectors[1023].tree.u.nextFree = 0;

    axis = (cm_world.maxs[1] - cm_world.mins[1] >= cm_world.maxs[0] - cm_world.mins[0]) ? 1 : 0;
    cm_world.sectors[1].tree.axis = axis;
    cm_world.sectors[1].tree.dist = (cm_world.maxs[axis] + cm_world.mins[axis]) * 0.5f;

    for (modelIndex = 0; modelIndex < clip->numStaticModels; ++modelIndex) {
        cStaticModel_t *staticModel = &clip->staticModelList[modelIndex];
        int contents;
        unsigned short nodeIndex;
        vec2_t mins;
        vec2_t maxs;
        worldSector_t *sector;
        unsigned short linkIndex;
        unsigned short *prevLink;

        if (!XModelGetContents(staticModel->xmodel)) {
            continue;
        }

        contents = XModelGetContents(staticModel->xmodel);
        mins[0] = cm_world.mins[0];
        mins[1] = cm_world.mins[1];
        maxs[0] = cm_world.maxs[0];
        maxs[1] = cm_world.maxs[1];

        nodeIndex = 1;
        while (1) {
            float dist;

            sector = &cm_world.sectors[nodeIndex];
            sector->contents.contentsStaticModels |= contents;

            axis = sector->tree.axis;
            dist = sector->tree.dist;

            if (staticModel->absmin[axis] > dist) {
                mins[axis] = dist;
                if (sector->tree.child[0]) {
                    nodeIndex = sector->tree.child[0];
                    continue;
                }
            } else if (dist > staticModel->absmax[axis]) {
                maxs[axis] = dist;
                if (sector->tree.child[1]) {
                    nodeIndex = sector->tree.child[1];
                    continue;
                }
            }
            break;
        }

        linkIndex = (unsigned short)(modelIndex + 1);
        prevLink = &sector->contents.staticModels;
        while (*prevLink && (unsigned short)(*prevLink - 1) <= modelIndex) {
            prevLink = &clip->staticModelList[*prevLink - 1].writable.nextModelInWorldSector;
        }

        staticModel->writable.nextModelInWorldSector = *prevLink;
        *prevLink = linkIndex;

        CM_SortNode(nodeIndex, mins, maxs);
    }
}

void CM_LinkEntity(svEntity_t *ent, vec_t *absmin, vec_t *absmax, clipHandle_t clipHandle)
{
    cmodel_t *cmodel = CM_ClipHandleToModel(clipHandle);
    int contents = cmodel->leaf.brushContents | cmodel->leaf.terrainContents;
    vec2_t mins;
    vec2_t maxs;
    unsigned short nodeIndex;

    if (contents == 0) {
        CM_UnlinkEntity(ent);
        return;
    }

restart_link:
    mins[0] = cm_world.mins[0];
    mins[1] = cm_world.mins[1];
    maxs[0] = cm_world.maxs[0];
    maxs[1] = cm_world.maxs[1];
    nodeIndex = 1;

    while (1) {
        worldSector_t *node = &cm_world.sectors[(unsigned int)nodeIndex];
        unsigned short axis;
        float dist;

        node->contents.contentsEntities |= contents;

        axis = node->tree.axis;
        dist = node->tree.dist;

        if (absmin[axis] > dist) {
            mins[axis] = dist;
            if (node->tree.child[0]) {
                nodeIndex = node->tree.child[0];
                continue;
            }
        } else if (dist > absmax[axis]) {
            maxs[axis] = dist;
            if (node->tree.child[1]) {
                nodeIndex = node->tree.child[1];
                continue;
            }
        }

        if (ent->worldSector == nodeIndex && (ent->linkcontents & ~contents) == 0) {
            ent->linkcontents = contents;
            ent->linkmin[0] = absmin[0];
            ent->linkmin[1] = absmin[1];
            ent->linkmax[0] = absmax[0];
            ent->linkmax[1] = absmax[1];
            return;
        }

        if (ent->worldSector != 0) {
            CM_UnlinkEntity(ent);
            goto restart_link;
        }

        CM_InsertEntityIntoWorldSector(nodeIndex, ent);

        ent->linkcontents = contents;
        ent->linkmin[0] = absmin[0];
        ent->linkmin[1] = absmin[1];
        ent->linkmax[0] = absmax[0];
        ent->linkmax[1] = absmax[1];

        CM_SortNode(nodeIndex, mins, maxs);
        return;
    }
}
