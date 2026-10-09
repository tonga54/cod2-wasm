#include "common_types.h"
#include "imports.h"

vec3_t actorLocationalMaxs = { 64.0f, 64.0f, 72.0f };

extern float floorf(float x);

extern clipHandle_t CM_TempBoxModel(const vec_t *mins, const vec_t *maxs, int contents);
extern void CM_UnlinkEntity(svEntity_t *svEntity);
extern int CM_BoxSightTrace(int oldHitNum, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, int brushmask, int contentmask);
extern int CM_PointSightTraceToEntities(const sightpointtrace_t *clip);
extern int CM_ClipSightTraceToEntities(const sightclip_t *clip);
extern int CM_BoxTrace(trace_t *results, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, int brushmask, int contentmask);
extern void CM_PointTraceStaticModels(trace_t *results, const vec_t *start, const vec_t *end, int contentmask);
extern void CM_CalcTraceEntents(const void *extents);
extern void CM_PointTraceToEntities(const pointtrace_t *clip, trace_t *results);
extern void CM_ClipMoveToEntities(const moveclip_t *clip, trace_t *results);
extern int CM_PointTraceStaticModelsComplete(const vec_t *start, const vec_t *end, int contentmask);
extern float RadiusFromBounds(const vec_t *mins, const vec_t *maxs);
extern float RadiusFromBounds2D(const vec_t *mins, const vec_t *maxs);
extern int CM_BoxLeafnums(const vec_t *mins, const vec_t *maxs, int *leafs, int maxLeafs, int *lastLeaf);
extern int CM_LeafCluster(int leafnum);
extern void CM_LinkEntity(svEntity_t *svEntity, const vec_t *absmin, const vec_t *absmax, int clipHandle);
extern int CM_TraceBox(const void *extents, const vec_t *absmin, const vec_t *absmax, float fraction);
extern int CM_TransformedBoxTrace(trace_t *results, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, int clipHandle, int contentmask, const vec_t *origin, const vec_t *angles);
extern int CM_TransformedBoxSightTrace(int oldHitNum, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, int clipHandle, int contentmask, const vec_t *origin, const vec_t *angles);
extern int CM_AreaEntities(const vec_t *mins, const vec_t *maxs, int *entityList, int maxcount, int contentmask);
extern int CM_PointContents(const vec_t *p, int brushmask);
extern int CM_TransformedPointContents(const vec_t *p, int clipHandle, const vec_t *origin, const vec_t *angles);
extern void Com_Error(int code, const char *fmt, ...);
extern gentity_t *SV_GentityNum(int num);
extern void *Com_GetServerDObj(int entityNum);
extern void DObjGetBounds(void *obj, vec_t *absmin, vec_t *absmax);
extern int DObjHasContents(void *obj, int contentmask);
extern int G_DObjCalcPose(gentity_t *ent);
extern void AnglesToAxis(const vec_t *angles, float (*axis)[3]);
extern void MatrixTransposeTransformVector43(const vec_t *in, const float *mat, vec_t *out);
extern void MatrixTransformVector(const vec_t *in, const float (*axis)[3], vec_t *out);
extern void DObjGeomTraceline(void *obj, const vec_t *start, const vec_t *end, int contentmask, void *objTrace);
extern void DObjTraceline(void *obj, const vec_t *start, const vec_t *end, unsigned char *priorityMap, void *objTrace);
extern byte *SV_SvEntityForGentity(const gentity_t *gEnt);

extern vec3_t actorLocationalMins;
extern vec3_t actorLocationalMaxs;

clipHandle_t SV_ClipHandleForEntity(const gentity_t *ent);
void SV_UnlinkEntity(gentity_t *gEnt);
int SV_SightTrace(int *hitNum, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum0, int passEntityNum1, int contentmask);
void SV_Trace(trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentmask, qboolean locational, unsigned char *priorityMap, qboolean staticmodels);
qboolean SV_TracePassed(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum0, int passEntityNum1, int contentmask, qboolean locational, qboolean staticmodels);
void SV_LinkEntity(gentity_t *gEnt);
void SV_ClipMoveToEntity(const moveclip_t *clip, svEntity_t *check, trace_t *trace);
void SV_PointTraceToEntity(const pointtrace_t *clip, svEntity_t *check, trace_t *trace);
int SV_ClipSightToEntity(const sightclip_t *clip, svEntity_t *check);
int SV_PointSightTraceToEntity(const sightpointtrace_t *clip, svEntity_t *check);
int SV_SightTraceToEntity(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int entityNum, int contentmask);
int SV_PointContents(const vec_t *p, int passEntityNum, int contentmask);

static inline __attribute__((always_inline))
clipHandle_t
SV_ClipHandleForEntity_core(const gentity_t *ent)
{
    if (ent->r.bmodel)
        return ent->s.index.brushmodel;
    return CM_TempBoxModel(ent->r.mins, ent->r.maxs, ent->r.contents);
}

clipHandle_t SV_ClipHandleForEntity(const gentity_t *ent)
{
    return SV_ClipHandleForEntity_core(ent);
}

void SV_UnlinkEntity(gentity_t *gEnt)
{
    svEntity_t *svEntity = (svEntity_t *)SV_SvEntityForGentity(gEnt);
    gEnt->r.linked = 0;
    CM_UnlinkEntity(svEntity);
    return;
}

int SV_SightTrace(int *hitNum, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum0, int passEntityNum1, int contentmask)
{

    *hitNum = CM_BoxSightTrace(*hitNum, start, end, mins, maxs, 0, contentmask);
    if (*hitNum)
        return *hitNum;

    {
        float extentSum = (maxs[0] - mins[0]) + (maxs[1] - mins[1]) + (maxs[2] - mins[2]);
        if (extentSum == 0.0f) {

            sightpointtrace_t clip;
            clip.start[0] = start[0];
            clip.start[1] = start[1];
            clip.start[2] = start[2];
            clip.end[0] = end[0];
            clip.end[1] = end[1];
            clip.end[2] = end[2];
            clip.passEntityNum[0] = passEntityNum0;
            clip.passEntityNum[1] = passEntityNum1;
            clip.contentmask = contentmask;
            *hitNum = CM_PointSightTraceToEntities(&clip);
        } else {

            sightclip_t clip;
            float halfX = (maxs[0] - mins[0]) * 0.5f;
            float halfY = (maxs[1] - mins[1]) * 0.5f;
            float halfZ = (maxs[2] - mins[2]) * 0.5f;
            float midX = (maxs[0] + mins[0]) * 0.5f;
            float midY = (maxs[1] + mins[1]) * 0.5f;
            float midZ = (maxs[2] + mins[2]) * 0.5f;

            clip.mins[0] = -halfX;
            clip.mins[1] = -halfY;
            clip.mins[2] = -halfZ;

            clip.maxs[0] = halfX;
            clip.maxs[1] = halfY;
            clip.maxs[2] = halfZ;

            clip.outerSize[0] = halfX + 1.0f;
            clip.outerSize[1] = halfY + 1.0f;
            clip.outerSize[2] = halfZ + 1.0f;

            clip.start[0] = midX + start[0];
            clip.start[1] = midY + start[1];
            clip.start[2] = midZ + start[2];

            clip.end[0] = midX + end[0];
            clip.end[1] = midY + end[1];
            clip.end[2] = midZ + end[2];
            clip.passEntityNum[0] = passEntityNum0;
            clip.passEntityNum[1] = passEntityNum1;
            clip.contentmask = contentmask;
            *hitNum = CM_ClipSightTraceToEntities(&clip);
        }
    }

    return *hitNum;
}

void SV_Trace(trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentmask, qboolean locational, unsigned char *priorityMap, qboolean staticmodels)
{

    CM_BoxTrace(results, start, end, mins, maxs, 0, contentmask);

    {
        int isOne = (results->fraction == 1.0f) ? 1 : 0;
        results->entityNum = (unsigned short)(isOne + 0x3fe);
    }

    if (results->fraction == 0.0f)
        return;

    if (staticmodels) {
        CM_PointTraceStaticModels(results, start, end, contentmask);
        if (results->fraction == 0.0f)
            return;
    }

    {
        float extentSum = (maxs[0] - mins[0]) + (maxs[1] - mins[1]) + (maxs[2] - mins[2]);

        if (extentSum == 0.0f) {

            byte clip[0x50];
            int ownerNum;

            ((pointtrace_t *)clip)->extents.start[0] = start[0];
            ((pointtrace_t *)clip)->extents.start[1] = start[1];
            ((pointtrace_t *)clip)->extents.start[2] = start[2];
            ((pointtrace_t *)clip)->extents.end[0] = end[0];
            ((pointtrace_t *)clip)->extents.end[1] = end[1];
            ((pointtrace_t *)clip)->extents.end[2] = end[2];

            CM_CalcTraceEntents((const void *)clip);

            ((pointtrace_t *)clip)->passEntityNum = passEntityNum;

            ((pointtrace_t *)clip)->bLocational = locational;

            ((pointtrace_t *)clip)->priorityMap = priorityMap;

            if (passEntityNum == 0x3ff) {
                ownerNum = -1;
            } else {
                gentity_t *passEnt = SV_GentityNum(passEntityNum);
                ownerNum = passEnt->r.ownerNum;
                if (ownerNum == 0x3ff)
                    ownerNum = -1;
            }
            ((pointtrace_t *)clip)->passOwnerNum = ownerNum;

            ((pointtrace_t *)clip)->contentmask = contentmask;

            CM_PointTraceToEntities((const pointtrace_t *)clip, results);
        } else {

            byte clip[0x70];
            moveclip_t *moveClip = (moveclip_t *)clip;
            int ownerNum;
            float halfX, halfY, halfZ;
            float midX, midY, midZ;

            halfX = (maxs[0] - mins[0]) * 0.5f;
            halfY = (maxs[1] - mins[1]) * 0.5f;
            halfZ = (maxs[2] - mins[2]) * 0.5f;

            midX = (maxs[0] + mins[0]) * 0.5f;
            midY = (maxs[1] + mins[1]) * 0.5f;
            midZ = (maxs[2] + mins[2]) * 0.5f;

            moveClip->contentmask = contentmask;

            moveClip->passEntityNum = passEntityNum;

            if (passEntityNum == 0x3ff) {
                ownerNum = -1;
            } else {
                gentity_t *passEnt = SV_GentityNum(passEntityNum);
                ownerNum = passEnt->r.ownerNum;
                if (ownerNum == 0x3ff)
                    ownerNum = -1;
            }
            moveClip->passOwnerNum = ownerNum;

            moveClip->mins[0] = -halfX;
            moveClip->mins[1] = -halfY;
            moveClip->mins[2] = -halfZ;

            moveClip->maxs[0] = halfX;
            moveClip->maxs[1] = halfY;
            moveClip->maxs[2] = halfZ;

            moveClip->outerSize[0] = halfX + 1.0f;
            moveClip->outerSize[1] = halfY + 1.0f;
            moveClip->outerSize[2] = halfZ + 1.0f;

            moveClip->extents.start[0] = midX + start[0];
            moveClip->extents.start[1] = midY + start[1];
            moveClip->extents.start[2] = midZ + start[2];

            moveClip->extents.end[0] = midX + end[0];
            moveClip->extents.end[1] = midY + end[1];
            moveClip->extents.end[2] = midZ + end[2];

            CM_CalcTraceEntents(&moveClip->extents);

            CM_ClipMoveToEntities(moveClip, results);
        }
    }
}

qboolean SV_TracePassed(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum0, int passEntityNum1, int contentmask, qboolean locational, qboolean staticmodels)
{
    if (CM_BoxSightTrace(0, start, end, mins, maxs, 0, contentmask)) {
        return 0;
    }

    if (staticmodels && !CM_PointTraceStaticModelsComplete(start, end, contentmask)) {
        return 0;
    }

    if ((maxs[0] - mins[0]) + (maxs[1] - mins[1]) + (maxs[2] - mins[2]) == 0.0f) {
        sightpointtrace_t clip;

        clip.start[0] = start[0];
        clip.start[1] = start[1];
        clip.start[2] = start[2];
        clip.end[0] = end[0];
        clip.end[1] = end[1];
        clip.end[2] = end[2];
        clip.passEntityNum[0] = passEntityNum0;
        clip.passEntityNum[1] = passEntityNum1;
        clip.contentmask = contentmask;
        clip.locational = locational;
        return CM_PointSightTraceToEntities(&clip) == 0;
    } else {
        sightclip_t clip;
        float halfX = (maxs[0] - mins[0]) * 0.5f;
        float halfY = (maxs[1] - mins[1]) * 0.5f;
        float halfZ = (maxs[2] - mins[2]) * 0.5f;
        float midX = (maxs[0] + mins[0]) * 0.5f;
        float midY = (maxs[1] + mins[1]) * 0.5f;
        float midZ = (maxs[2] + mins[2]) * 0.5f;

        clip.mins[0] = -halfX;
        clip.mins[1] = -halfY;
        clip.mins[2] = -halfZ;
        clip.maxs[0] = halfX;
        clip.maxs[1] = halfY;
        clip.maxs[2] = halfZ;
        clip.outerSize[0] = halfX + 1.0f;
        clip.outerSize[1] = halfY + 1.0f;
        clip.outerSize[2] = halfZ + 1.0f;
        clip.start[0] = midX + start[0];
        clip.start[1] = midY + start[1];
        clip.start[2] = midZ + start[2];
        clip.end[0] = midX + end[0];
        clip.end[1] = midY + end[1];
        clip.end[2] = midZ + end[2];
        clip.passEntityNum[0] = passEntityNum0;
        clip.passEntityNum[1] = passEntityNum1;
        clip.contentmask = contentmask;
        return CM_ClipSightTraceToEntities(&clip) == 0;
    }
}

void SV_LinkEntity(gentity_t *gEnt)
{
    svEntity_t *ent = (svEntity_t *)SV_SvEntityForGentity(gEnt);
    vec_t *mins = gEnt->r.mins;
    vec_t *maxs = gEnt->r.maxs;
    vec_t *absmin = gEnt->r.absmin;
    vec_t *absmax = gEnt->r.absmax;
    const vec_t *origin = gEnt->r.currentOrigin;
    const vec_t *angles = gEnt->r.currentAngles;
    int leafs[128];
    int lastLeaf;
    int num_leafs;
    clipHandle_t clipHandle;
    void *obj;
    int i;

    if (gEnt->r.bmodel) {
        gEnt->s.solid = 0x00ffffff;
    } else if (gEnt->r.contents & 0x02000001) {
        int x;
        int zd;
        int zu;

        x = (int)maxs[0];
        if (x <= 0)
            x = 1;
        else if (x >= 0x100)
            x = 0xff;

        zd = (int)(1.0f - mins[2]);
        if (zd <= 0)
            zd = 0x100;
        else if (zd > 0xff)
            zd = 0xff00;
        else
            zd <<= 8;

        zu = (int)(maxs[2] + 32.0f);
        if (zu <= 0)
            zu = 0x10000;
        else if (zu > 0xff)
            zu = 0x00ff0000;
        else
            zu <<= 16;

        gEnt->s.solid = x | zd | zu;
    } else {
        gEnt->s.solid = 0;
    }

    for (i = 0; i < 3; ++i) {
        float rounded = (float)(int)floorf(angles[i] + 0.5f);
        float delta = rounded - angles[i];
        if (delta * delta < 1.0000001111620804e-06f)
            gEnt->r.currentAngles[i] = rounded;
    }

    if (gEnt->r.bmodel && (angles[0] != 0.0f || angles[1] != 0.0f || angles[2] != 0.0f)) {
        if (angles[0] == 0.0f && angles[2] == 0.0f) {
            float radius = RadiusFromBounds2D(mins, maxs);
            absmin[0] = origin[0] - radius;
            absmin[1] = origin[1] - radius;
            absmax[0] = origin[0] + radius;
            absmax[1] = origin[1] + radius;
            absmin[2] = origin[2] + mins[2];
            absmax[2] = origin[2] + maxs[2];
        } else {
            float radius = RadiusFromBounds(mins, maxs);
            absmin[0] = origin[0] - radius;
            absmin[1] = origin[1] - radius;
            absmin[2] = origin[2] - radius;
            absmax[0] = origin[0] + radius;
            absmax[1] = origin[1] + radius;
            absmax[2] = origin[2] + radius;
        }
    } else {
        absmin[0] = origin[0] + mins[0];
        absmin[1] = origin[1] + mins[1];
        absmin[2] = origin[2] + mins[2];
        absmax[0] = origin[0] + maxs[0];
        absmax[1] = origin[1] + maxs[1];
        absmax[2] = origin[2] + maxs[2];
    }

    absmin[0] -= 1.0f;
    absmin[1] -= 1.0f;
    absmin[2] -= 1.0f;
    absmax[0] += 1.0f;
    absmax[1] += 1.0f;
    absmax[2] += 1.0f;

    ent->numClusters = 0;
    ent->lastCluster = 0;

    if ((gEnt->r.svFlags & 0x19) == 0) {
        num_leafs = CM_BoxLeafnums(absmin, absmax, leafs, 128, &lastLeaf);
        if (num_leafs == 0) {
            CM_UnlinkEntity(ent);
            return;
        }

        if (num_leafs > 0) {
            for (i = 0; i < num_leafs; ++i) {
                int cluster = CM_LeafCluster(leafs[i]);
                if (cluster == -1)
                    continue;

                ent->clusternums[ent->numClusters++] = cluster;
                if (ent->numClusters == 16) {
                    ent->lastCluster = CM_LeafCluster(lastLeaf);
                    break;
                }
            }
        } else {
            ent->lastCluster = CM_LeafCluster(lastLeaf);
        }
    }

    gEnt->r.linked = 1;

    if (gEnt->r.contents == 0) {
        CM_UnlinkEntity(ent);
        return;
    }

    if (gEnt->r.bmodel)
        clipHandle = gEnt->s.index.brushmodel;
    else
        clipHandle = CM_TempBoxModel(mins, maxs, gEnt->r.contents);

    obj = Com_GetServerDObj(gEnt->s.number);
    if (obj && (gEnt->r.svFlags & 0x06)) {
        vec3_t modelMins;
        vec3_t modelMaxs;

        if (gEnt->r.svFlags & 0x02) {
            modelMins[0] = origin[0] + actorLocationalMins[0];
            modelMins[1] = origin[1] + actorLocationalMins[1];
            modelMins[2] = origin[2] + actorLocationalMins[2];
            modelMaxs[0] = origin[0] + actorLocationalMaxs[0];
            modelMaxs[1] = origin[1] + actorLocationalMaxs[1];
            modelMaxs[2] = origin[2] + actorLocationalMaxs[2];
        } else {
            DObjGetBounds(obj, modelMins, modelMaxs);
            modelMins[0] += origin[0];
            modelMins[1] += origin[1];
            modelMins[2] += origin[2];
            modelMaxs[0] += origin[0];
            modelMaxs[1] += origin[1];
            modelMaxs[2] += origin[2];
        }

        CM_LinkEntity(ent, modelMins, modelMaxs, clipHandle);
    } else {
        CM_LinkEntity(ent, absmin, absmax, clipHandle);
    }
}

void SV_ClipMoveToEntity(const moveclip_t *clip, svEntity_t *check, trace_t *trace)
{
    server_t *sv = (server_t *)imp_sv;
    int entityNum = (int)(check - sv->svEntities);
    gentity_t *touch = SV_GentityNum(entityNum);
    int contentmask = clip->contentmask;
    vec3_t absmin;
    vec3_t absmax;
    float oldFraction;
    clipHandle_t clipHandle;
    const vec_t *angles;

    if ((touch->r.contents & contentmask) == 0) {
        return;
    }

    if (clip->passEntityNum != 1023) {
        if (entityNum == clip->passEntityNum) {
            return;
        }
        if (touch->r.ownerNum == clip->passEntityNum) {
            return;
        }
        if (touch->r.ownerNum == clip->passOwnerNum) {
            return;
        }
    }

    absmin[0] = touch->r.absmin[0] + clip->mins[0];
    absmin[1] = touch->r.absmin[1] + clip->mins[1];
    absmin[2] = touch->r.absmin[2] + clip->mins[2];

    absmax[0] = touch->r.absmax[0] + clip->maxs[0];
    absmax[1] = touch->r.absmax[1] + clip->maxs[1];
    absmax[2] = touch->r.absmax[2] + clip->maxs[2];

    if (CM_TraceBox(&clip->extents, absmin, absmax, trace->fraction)) {
        return;
    }

    clipHandle = SV_ClipHandleForEntity(touch);
    angles = touch->r.bmodel ? touch->r.currentAngles : (const vec_t *)imp_vec3_origin;

    oldFraction = trace->fraction;
    CM_TransformedBoxTrace(trace, clip->extents.start, clip->extents.end, clip->mins, clip->maxs,
                           clipHandle, contentmask, touch->r.currentOrigin, angles);

    if (trace->fraction < oldFraction) {
        trace->entityNum = touch->s.number;
    }
}

void SV_PointTraceToEntity(const pointtrace_t *clip, svEntity_t *check, trace_t *trace)
{
    server_t *sv = (server_t *)imp_sv;
    int entityNum = (int)(check - sv->svEntities);
    gentity_t *touch = SV_GentityNum(entityNum);
    int contentmask = clip->contentmask;
    const vec_t *zero;
    clipHandle_t clipHandle;
    const vec_t *angles;
    float oldFraction;

    if ((touch->r.contents & contentmask) == 0) {
        return;
    }

    if (clip->passEntityNum != 1023) {
        if (entityNum == clip->passEntityNum) {
            return;
        }
        if (touch->r.ownerNum == clip->passEntityNum) {
            return;
        }
        if (touch->r.ownerNum == clip->passOwnerNum) {
            return;
        }
    }

    if (clip->bLocational) {
        void *obj = Com_GetServerDObj(touch->s.number);

        if (obj && (touch->r.svFlags & 6)) {
            vec3_t origin;
            vec3_t absmin;
            vec3_t absmax;
            float entAxis[4][3];
            vec3_t localStart;
            vec3_t localEnd;
            DObjTrace objTrace;

            origin[0] = touch->r.currentOrigin[0];
            origin[1] = touch->r.currentOrigin[1];
            origin[2] = touch->r.currentOrigin[2];

            if (touch->r.svFlags & 4) {
                if (!DObjHasContents(obj, contentmask)) {
                    return;
                }

                DObjGetBounds(obj, absmin, absmax);
                absmin[0] += origin[0];
                absmin[1] += origin[1];
                absmin[2] += origin[2];
                absmax[0] += origin[0];
                absmax[1] += origin[1];
                absmax[2] += origin[2];
            } else {
                if (!clip->priorityMap) {
                    goto non_locational_trace;
                }

                absmin[0] = origin[0] + actorLocationalMins[0];
                absmin[1] = origin[1] + actorLocationalMins[1];
                absmin[2] = origin[2] + actorLocationalMins[2];
                absmax[0] = origin[0] + actorLocationalMaxs[0];
                absmax[1] = origin[1] + actorLocationalMaxs[1];
                absmax[2] = origin[2] + actorLocationalMaxs[2];
            }

            if (CM_TraceBox(&clip->extents, absmin, absmax, trace->fraction)) {
                return;
            }

            G_DObjCalcPose(touch);
            AnglesToAxis(touch->r.currentAngles, entAxis);
            /* The inverse affine transform also reads the origin row. */
            memcpy(entAxis[3], origin, sizeof(origin));
            MatrixTransposeTransformVector43(clip->extents.start, (const float *)entAxis, localStart);
            MatrixTransposeTransformVector43(clip->extents.end, (const float *)entAxis, localEnd);

            objTrace.fraction = trace->fraction;
            if (touch->r.svFlags & 4) {
                DObjGeomTraceline(obj, localStart, localEnd, contentmask, &objTrace);
            } else {
                DObjTraceline(obj, localStart, localEnd, clip->priorityMap, &objTrace);
            }

            if (objTrace.fraction < trace->fraction) {
                trace->fraction = objTrace.fraction;
                trace->surfaceFlags = objTrace.surfaceflags;
                trace->partName = objTrace.partName;
                trace->partGroup = objTrace.partGroup;
                MatrixTransformVector(objTrace.normal, entAxis, trace->normal);
                trace->entityNum = touch->s.number;
                trace->contents = touch->r.contents;
                trace->material = NULL;
            }

            return;
        }
    }

non_locational_trace:
    if ((check->linkcontents & contentmask) == 0) {
        return;
    }

    if (CM_TraceBox(&clip->extents, touch->r.absmin, touch->r.absmax, trace->fraction)) {
        return;
    }

    zero = (const vec_t *)imp_vec3_origin;
    if (touch->r.bmodel) {
        clipHandle = touch->s.index.brushmodel;
        angles = touch->r.currentAngles;
    } else {
        clipHandle = CM_TempBoxModel(touch->r.mins, touch->r.maxs, touch->r.contents);
        angles = zero;
    }

    oldFraction = trace->fraction;
    CM_TransformedBoxTrace(trace, clip->extents.start, clip->extents.end, zero, zero,
                           clipHandle, contentmask, touch->r.currentOrigin, angles);

    if (trace->fraction < oldFraction) {
        trace->surfaceFlags = 0;
        trace->partName = 0;
        trace->partGroup = 0;
        trace->entityNum = touch->s.number;
        trace->contents = touch->r.contents;
        trace->material = NULL;
    }
}

int SV_ClipSightToEntity(const sightclip_t *clip, svEntity_t *check)
{
    server_t *sv = (server_t *)imp_sv;
    int entityNum = (int)(check - sv->svEntities);
    gentity_t *touch = SV_GentityNum(entityNum);
    int contentmask = clip->contentmask;
    int passEntityNum;
    clipHandle_t clipHandle;
    const vec_t *angles;

    if (!(touch->r.contents & contentmask)) {
        return 0;
    }

    passEntityNum = clip->passEntityNum[0];
    if (passEntityNum != 1023) {
        if (entityNum == passEntityNum || touch->r.ownerNum == passEntityNum) {
            return 0;
        }
    }

    passEntityNum = clip->passEntityNum[1];
    if (passEntityNum != 1023) {
        if (entityNum == passEntityNum || touch->r.ownerNum == passEntityNum) {
            return 0;
        }
    }

    clipHandle = SV_ClipHandleForEntity_core(touch);
    angles = touch->r.bmodel ? touch->r.currentAngles : (const vec_t *)imp_vec3_origin;

    if (CM_TransformedBoxSightTrace(0, clip->start, clip->end, clip->mins, clip->maxs,
                                    clipHandle, contentmask, touch->r.currentOrigin, angles)) {
        return -1;
    }

    return 0;
}

int SV_PointSightTraceToEntity(const sightpointtrace_t *clip, svEntity_t *check)
{
    server_t *sv = (server_t *)imp_sv;
    int entityNum = (int)(check - sv->svEntities);
    gentity_t *touch = SV_GentityNum(entityNum);
    int contentmask = clip->contentmask;
    int passEntityNum;
    clipHandle_t clipHandle;
    const vec_t *zero;
    const vec_t *angles;

    if (!(touch->r.contents & contentmask)) {
        return 0;
    }

    passEntityNum = clip->passEntityNum[0];
    if (passEntityNum != 1023) {
        if (entityNum == passEntityNum || touch->r.ownerNum == passEntityNum) {
            return 0;
        }
    }

    passEntityNum = clip->passEntityNum[1];
    if (passEntityNum != 1023) {
        if (entityNum == passEntityNum || touch->r.ownerNum == passEntityNum) {
            return 0;
        }
    }

    if (clip->locational) {
        void *obj = Com_GetServerDObj(touch->s.number);

        if (obj && (touch->r.svFlags & 4)) {
            vec3_t origin;
            vec3_t absmin;
            vec3_t absmax;
            TraceExtents extents;
            float entAxis[4][3];
            vec3_t localStart;
            vec3_t localEnd;
            DObjTrace objTrace;
            int i;

            if (!DObjHasContents(obj, contentmask)) {
                return 0;
            }

            origin[0] = touch->r.currentOrigin[0];
            origin[1] = touch->r.currentOrigin[1];
            origin[2] = touch->r.currentOrigin[2];

            DObjGetBounds(obj, absmin, absmax);
            for (i = 0; i < 3; ++i) {
                absmin[i] += origin[i];
                absmax[i] += origin[i];
                extents.start[i] = clip->start[i];
                extents.end[i] = clip->end[i];
            }

            CM_CalcTraceEntents(&extents);
            if (CM_TraceBox(&extents, absmin, absmax, 1.0f)) {
                return 0;
            }

            G_DObjCalcPose(touch);
            AnglesToAxis(touch->r.currentAngles, entAxis);
            memcpy(entAxis[3], origin, sizeof(origin));
            MatrixTransposeTransformVector43(extents.start, (const float *)entAxis, localStart);
            MatrixTransposeTransformVector43(extents.end, (const float *)entAxis, localEnd);

            objTrace.fraction = 1.0f;
            DObjGeomTraceline(obj, localStart, localEnd, contentmask, &objTrace);
            if (objTrace.fraction < 1.0f) {
                return -1;
            }

            return 0;
        }
    }

    zero = (const vec_t *)imp_vec3_origin;
    if (touch->r.bmodel) {
        clipHandle = touch->s.index.brushmodel;
        angles = touch->r.currentAngles;
    } else {
        clipHandle = CM_TempBoxModel(touch->r.mins, touch->r.maxs, touch->r.contents);
        angles = zero;
    }

    if (CM_TransformedBoxSightTrace(0, clip->start, clip->end, zero, zero,
                                    clipHandle, contentmask, touch->r.currentOrigin, angles)) {
        return -1;
    }

    return 0;
}

int SV_SightTraceToEntity(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end,
                          int entityNum, int contentmask)
{
    gentity_t *ent = SV_GentityNum(entityNum);
    vec3_t boxmins;
    vec3_t boxmaxs;
    int i;
    clipHandle_t clipHandle;
    const vec_t *angles;

    if (!(ent->r.contents & contentmask)) {
        return 0;
    }

    for (i = 0; i < 3; ++i) {
        if (end[i] > start[i]) {
            boxmins[i] = start[i] + mins[i] - 1.0f;
            boxmaxs[i] = end[i] + maxs[i] + 1.0f;
        } else {
            boxmins[i] = end[i] + mins[i] - 1.0f;
            boxmaxs[i] = start[i] + maxs[i] + 1.0f;
        }
    }

    if (ent->r.absmin[0] > boxmaxs[0] || ent->r.absmin[1] > boxmaxs[1] || ent->r.absmin[2] > boxmaxs[2]) {
        return 0;
    }

    if (boxmins[0] > ent->r.absmax[0] || boxmins[1] > ent->r.absmax[1] || boxmins[2] > ent->r.absmax[2]) {
        return 0;
    }

    clipHandle = SV_ClipHandleForEntity_core(ent);
    angles = ent->r.bmodel ? ent->r.currentAngles : (const vec_t *)imp_vec3_origin;

    if (CM_TransformedBoxSightTrace(0, start, end, mins, maxs, clipHandle, contentmask,
                                    ent->r.currentOrigin, angles)) {
        return -1;
    }

    return 0;
}

int SV_PointContents(const vec_t *p, int passEntityNum, int contentmask)
{
    int contents;
    int touch[1024];
    int num;
    int i;

    contents = CM_PointContents(p, 0);
    num = CM_AreaEntities(p, p, touch, 1024, contentmask);

    for (i = 0; i < num; ++i) {
        gentity_t *hit;
        clipHandle_t clipHandle;

        if (touch[i] == passEntityNum)
            continue;

        hit = SV_GentityNum(touch[i]);
        if (hit->r.bmodel)
            clipHandle = hit->s.index.brushmodel;
        else
            clipHandle = CM_TempBoxModel(hit->r.mins, hit->r.maxs, hit->r.contents);

        contents |= CM_TransformedPointContents(p, clipHandle, hit->r.currentOrigin, hit->r.currentAngles);
    }

    return contents & contentmask;
}
