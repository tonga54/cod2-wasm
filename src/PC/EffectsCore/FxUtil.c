#include "common_types.h"

qboolean fxInitialized[1] = { 0x0 };

extern Bool g_rendererExists;
#include "imports.h"
#include "bytematch.h"
#include <math.h>

#define PART_ANCHOR_X(p) (((Effect *)(p))->origin[0])
#define PART_ANCHOR_Y(p) (((Effect *)(p))->origin[1])
#define PART_ANCHOR_Z(p) (((Effect *)(p))->origin[2])

#define QUAT_X(p) (((const DObjAnimMat *)(p))->quat[0])
#define QUAT_Y(p) (((const DObjAnimMat *)(p))->quat[1])
#define QUAT_Z(p) (((const DObjAnimMat *)(p))->quat[2])
#define QUAT_W(p) (((const DObjAnimMat *)(p))->quat[3])
#define QUAT_SCALE(p) (((const DObjAnimMat *)(p))->transWeight)

#define SCH_FX(p) (*(byte **)(p))
#define SCH_PRIM_INDEX(p) (((ScheduledEffect *)(p))->mPrimIndex)
#define SCH_START_TIME(p) (((ScheduledEffect *)(p))->mStartTime)
#define SCH_BOLT_ENTITY(p) (((ScheduledEffect *)(p))->mBolt.dobjHandle)
#define SCH_RAND_SEED(p) (((ScheduledEffect *)(p))->mSeed)
#define SCH_INDEX_IN_BATCH(p) (((ScheduledEffect *)(p))->mIndexInBatch)
#define SCH_NEXT(p) (*(byte **)&((ScheduledEffect *)(p))->mScheduledNext)

#define SCHEDULER_FIRST_P(p) ((byte **)&((FxScheduler *)(p))->mScheduledHead)
#define SCHEDULER_COUNT(p) (((FxScheduler *)(p))->mScheduledCount)

COD2_ASSERT_FIELD(struct DObjAnimMat_s,   quat,            0x00);
COD2_ASSERT_FIELD(struct DObjAnimMat_s,   transWeight,     0x1c);
COD2_ASSERT_FIELD(struct ScheduledEffect, mBolt,           0x0c);
COD2_ASSERT_FIELD(struct ScheduledEffect, mScheduledNext,  0x4c);
COD2_ASSERT_FIELD(struct FxScheduler,     mScheduledHead,  0x04);
COD2_ASSERT_FIELD(struct FxScheduler,     mScheduledCount, 0x08);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mAttributeFlags, 0x90);

extern FxHelper *theFxHelper;
extern int g_effectVisArrayCount;
extern EffectVisInfo g_effectVisArray[1800];
extern int effectActiveCountBolt;
extern int effectActiveCountNonBolt;
extern int privateEffectActiveCountBolt;
extern int privateEffectActiveCountNonBolt;
extern int initialEffectActiveCountBolt;
extern int initialEffectActiveCountNonBolt;
extern int cullEffectCountNonBolt;
extern int cullEffectCountBolt;
extern int visibleEffectCountNonBolt;
extern int visibleEffectCountBolt;
extern EffectCluster *effectClusters;
extern int effectClusterCount;
extern int *clusterSort;
extern Effect **effectListBolt;
extern Effect **effectListNonBolt;
extern int effectActiveCount;
extern int effectBlockSightCount;
extern FxHelper theFxHelpers[1];
extern qboolean fxInitialized[1];
static effectClusterArray_t effectClusterArray;
static effectListArray_t effectListArrayBolt;
static effectListArray_t effectListArrayNonBolt;
static SortedEffect visibleEffectsNonBolt[1800];
static SortedEffect visibleEffectsBolt[1800];

extern Bool FxHelper_IsMaterialRefractive(FxHelper *helper, MaterialHandle material);
extern void FxHelper_FxHelper(FxHelper *helper);
extern void Z_FreeInternal(void *ptr);
extern void *Z_MallocInternal(int size);
extern void __ZdaPv(void *ptr);
extern void Rand_Init(int seed);
extern void FxHelper_Trace(void *helper, void *trace, vec_t *start, vec_t *mins, vec_t *maxs, vec_t *end, int contents, int mask);
extern void AxisCopy(const vec_t *src, vec_t *dst);
extern void CG_GetDObjOrientation(int entityNum, void *axis);
extern void *Com_GetClientDObj(int entityNum, int localClientNum);
extern int DObjNumBones(void *dobj);
extern void CG_DObjCalcBoneGeneric(int entityNum, int localClientNum, int boneIndex);
extern void *DObjGetRotTransArray(void *dobj);
extern void MatrixMultiply(void *a, void *b, void *out);
extern void MatrixTransformVector43(void *trans, void *axis, void *out);
extern void *imp_fx_debugBolt;
extern void AxisTransformVector(void *axis, float x, float y, float z, vec_t *out);
extern void *imp_vec3_origin;
extern void *__Znam(int size);
extern float flrand(float min, float max);
extern void Particle_Particle(void *particle);
extern void Particle_IntegrateTotalVelocity(void *particle, int time, vec_t *velSum);
extern void Cloud_Cloud(void *cloud);
extern void Cylinder_Cylinder(void *cyl);
extern void Light_Light(void *light);
extern void *FxBoltFrame_GetOrientation(void *boltFrame);
extern void OrientationDirFromWorldDir(void *orient, vec_t *normal, vec_t *localNormal);
extern void FxScheduler_Clean(void *scheduler, int bRemoveTemplates, int arg3);
extern void *imp_fxSchedulers;
extern void *imp_theFxScheduler;
extern float Vec3DistanceSq(const vec_t *a, const vec_t *b);
extern float FxRange_GetVal(void *range);
extern void FxHelper_CameraShake(void *helper, vec_t *origin, float intensity, int duration, int fadeTime);
extern void *MediaHandles_GetEffect(void *mediaHandles);
extern void FxScheduler_PlayEffect(void *scheduler, void *effectTemplate, vec_t *origin, vec3_t *ax, void *bolt);
extern void FxScheduler_CreateDecalEffect(void *scheduler, void *primTemp, vec_t *origin, vec3_t *ax);
extern void FxScheduler_FxScheduler(void *scheduler);
extern void FX_InitTemplates(void);
extern void FxHelper_Init(void *helper);
extern void *imp_g_rendererExists;

void FX_InitServer(void);
static int CompareSortedEffects(const void *e0, const void *e1);
static int CompareSortedClusters(const void *e0, const void *e1);
void FX_SetSortGroup(Effect *fx);
int FX_GetCluster(const vec_t *origin);
void FX_CalcOrigin2(const PrimitiveTemplate *primTemp, vec_t *org, vec_t *org2, const vec_t *origin, vec3_t *ax);
Bool FX_GetBoneOrientation(const FxBoltInfo *bolt, orientation_t *orient);
void FX_AddScheduledEffects(void);
float FX_GetServerVisibility(const vec_t *start, const vec_t *end);
static void FX_CalcOriginAndAxis(EffectPrimitive *prim, vec_t *orgOut, const vec_t *origin, vec3_t *ax);
static void FX_InitParticle(EffectPrimitive *prim, Particle *particle, vec_t *newOrigin, const vec_t *origin, vec3_t *ax, int indexInBatch);
void FX_AddCameraShake(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
void FX_AddFxRunner(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
void FX_AddDecal(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
void FX_DrawAll(void);
static void FX_SetMaterialAndSequenceParams(byte *primTemp, Effect *particle, int killTime, int indexInBatch);
static Bool FX_AddPrimitive(EffectPrimitive *prim, Effect *particle, const vec_t *origin);
void FX_AddCloud(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
void FX_AddFlash(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
void FX_AddLight(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
void FX_AddCylinder(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
void FX_AddLine(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
void FX_AddParticle(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
void FX_AddTail(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
void FX_AddEmitter(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
void FX_AddOrientedParticle(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
void FX_UpdateScheduledEffectsNonBolt(void);
void FX_UpdateScheduledEffectsBolt(void);
void FX_UpdateAllBolt(void);
void FX_Rewind(int time);
void FX_UpdateAllNonBolt(void);
void FX_DrawScheduledEffects(void);
int FX_Restore(MemoryFile *memFile);
int FX_Init(int rendererExists);
void FX_Free(int bRemoveTemplates);

void FX_InitServer(void)
{
    g_effectVisArrayCount = 0;
}

static int CompareSortedEffects(const void *e0, const void *e1)
{
    Effect *fx0 = *(Effect **)e0;
    Effect *fx1 = *(Effect **)e1;
    int result;

    result = fx0->mSortGroup - fx1->mSortGroup;
    if (result)
        return result;

    result = clusterSort[fx0->mClusterId] - clusterSort[fx1->mClusterId];
    if (result)
        return result;

    result = (int)fx0->mRefEnt.customMaterial - (int)fx1->mRefEnt.customMaterial;
    if (result)
        return result;

    return (*(float *)((byte *)e1 + 4) > *(float *)((byte *)e0 + 4)) ? 1 : -1;
}

static int CompareSortedClusters(const void *e0, const void *e1)
{
    float v1 = *(float *)((byte *)e1 + 4);
    float v0 = *(float *)((byte *)e0 + 4);
    return (v1 > v0) ? 1 : -1;
}

void FX_SetSortGroup(Effect *fx)
{
    fx->mSortGroup = 0;
    if (fx->mRefEnt.customMaterial && FxHelper_IsMaterialRefractive(theFxHelper, fx->mRefEnt.customMaterial)) {
        fx->mSortGroup = -1;
    }
}

#ifdef _MSC_VER
COD2_CONSTRUCTOR(GLOBAL__I_effectClusters)
#else
static void __attribute__((constructor)) GLOBAL__I_effectClusters(void)
#endif
{
    FxHelper_FxHelper(theFxHelpers);
}

extern float Vec3DistanceSq(const vec_t *a, const vec_t *b);
static inline __attribute__((always_inline)) int FX_GetCluster_core(const vec_t *origin)
{
    int i;

    for (i = 0; i < effectClusterCount; i++) {
        EffectCluster *cluster = &effectClusters[i];
        float distSq = Vec3DistanceSq(origin, cluster->origin);
        if (distSq < 131072.0f) {
            cluster->refCount += 1;
            return i;
        }
    }

    {
        EffectCluster *newCluster = &effectClusters[effectClusterCount];
        newCluster->origin[0] = origin[0];
        newCluster->origin[1] = origin[1];
        newCluster->origin[2] = origin[2];
        newCluster->refCount = 1;
        return effectClusterCount++;
    }
}
int FX_GetCluster(const vec_t *origin)
{
    return FX_GetCluster_core(origin);
}

void FX_CalcOrigin2(const PrimitiveTemplate *primTemp, vec_t *org, vec_t *org2, const vec_t *origin, vec3_t *ax)
{
    byte *pt = (byte *)primTemp;
    int flags = ((PrimitiveTemplate *)pt)->mSpawnFlags;

    if (flags & 0x08) {

        float *axf = (float *)ax;
        vec3_t temp;
        temp[0] = org[0] + axf[0] * 16384.0f;
        temp[1] = org[1] + axf[1] * 16384.0f;
        temp[2] = org[2] + axf[2] * 16384.0f;

        if (flags & 0x20) {

            float z = FxRange_GetVal(pt + 0xe0);
            float y = FxRange_GetVal(pt + 0xd8);
            float x = FxRange_GetVal(pt + 0xd0);
            if (flags & 0x80) {

                org2[0] = x;
                org2[1] = y;
                org2[2] = z;
                temp[0] += x;
                temp[1] += y;
                temp[2] += z;
            } else {

                vec3_t point;
                AxisTransformVector(ax, x, y, z, point);
                temp[0] += point[0];
                temp[1] += point[1];
                temp[2] += point[2];
            }
        }

        byte trace[0x44];
        FxHelper_Trace(*(FxHelper **)imp_theFxHelper, trace, org, (vec_t *)imp_vec3_origin, (vec_t *)imp_vec3_origin, temp, -1, 1);
        float fraction = *(float *)trace;

        org2[0] = org[0] + (temp[0] - org[0]) * fraction;
        org2[1] = org[1] + (temp[1] - org[1]) * fraction;
        org2[2] = org[2] + (temp[2] - org[2]) * fraction;

        if (flags & 0x10) {
            void *effect = MediaHandles_GetEffect(pt + 0x70);
            vec_t *traceNormal = (vec_t *)(trace + 4);
            FxScheduler_PlayEffect(*(void **)imp_theFxScheduler, effect, org2, (vec3_t *)traceNormal, NULL);
        }
    } else {

        float z = FxRange_GetVal(pt + 0xe0);
        float y = FxRange_GetVal(pt + 0xd8);
        float x = FxRange_GetVal(pt + 0xd0);

        if (flags & 0x80) {

            org2[0] = x;
            org2[1] = y;
            org2[2] = z;
        } else {

            AxisTransformVector(ax, x, y, z, org2);
        }
        org2[0] += origin[0];
        org2[1] += origin[1];
        org2[2] += origin[2];
    }
}

extern void CG_GetDObjOrientation(int entityNum, void *axis);
extern void *Com_GetClientDObj(int entityNum, int localClientNum);
extern int DObjNumBones(void *dobj);
extern void CG_DObjCalcBoneGeneric(int entityNum, int localClientNum, int boneIndex);
extern void *DObjGetRotTransArray(void *dobj);
extern void MatrixMultiply(void *a, void *b, void *out);
extern void MatrixTransformVector43(void *trans, void *axis, void *out);
extern void *imp_fx_debugBolt;
extern void *imp_colorRed;
extern void *imp_colorGreen;
extern void *imp_colorBlue;
extern void CL_AddDebugLine(const vec_t *start, const vec_t *end, const vec_t *color, int depthTest, int duration, int fromServer);
Bool FX_GetBoneOrientation(const FxBoltInfo *bolt, orientation_t *orient)
{
    FxBoltInfo *b = (FxBoltInfo *)bolt;
    int entityNum = b->dobjHandle;
    int boneIndex = b->boneIndex;

    float axis[12];
    CG_GetDObjOrientation(entityNum, axis);

    if (boneIndex < 0) {

        orient->origin[0] = axis[9];
        orient->origin[1] = axis[10];
        orient->origin[2] = axis[11];
        AxisCopy((vec_t *)axis, (vec_t *)orient->axis);
        return 1;
    }

    void *dobj = Com_GetClientDObj(entityNum, 0);
    if (!dobj)
        return 0;

    if (boneIndex >= DObjNumBones(dobj))
        return 0;

    CG_DObjCalcBoneGeneric(entityNum, 0, boneIndex);
    byte *rotTransArray = (byte *)DObjGetRotTransArray(dobj);
    if (!rotTransArray)
        return 0;

    byte *mtx = rotTransArray + boneIndex * 32;

    float scale = QUAT_SCALE(mtx);
    float qx = QUAT_X(mtx) * scale;
    float qy = QUAT_Y(mtx) * scale;
    float qz = QUAT_Z(mtx) * scale;
    float qw = QUAT_W(mtx);

    float xx = qx * QUAT_X(mtx);
    float xy = qx * QUAT_Y(mtx);
    float xz = qx * QUAT_Z(mtx);
    float xw = qx * qw;
    float yy = qy * QUAT_Y(mtx);
    float yz = qy * QUAT_Z(mtx);
    float yw = qy * qw;
    float zz = qz * QUAT_Z(mtx);
    float zw = qz * qw;

    float tagAxis[9];
    tagAxis[0] = 1.0f - (yy + zz);
    tagAxis[1] = zw + xy;
    tagAxis[2] = xz - yw;
    tagAxis[3] = xy - zw;
    tagAxis[4] = 1.0f - (xx + zz);
    tagAxis[5] = xw + yz;
    tagAxis[6] = yw + xz;
    tagAxis[7] = yz - xw;
    tagAxis[8] = 1.0f - (xx + yy);

    MatrixMultiply(tagAxis, axis, orient->axis);

    MatrixTransformVector43(mtx + 0x10, axis, orient);

    float debugDist = (*(dvar_t **)imp_fx_debugBolt)->current.value;
    if (debugDist != 0.0f) {
        vec3_t endPt;
        endPt[0] = orient->origin[0] + orient->axis[0][0] * debugDist;
        endPt[1] = orient->origin[1] + orient->axis[0][1] * debugDist;
        endPt[2] = orient->origin[2] + orient->axis[0][2] * debugDist;
        CL_AddDebugLine(orient->origin, endPt, (const vec_t *)imp_colorRed, 1, 0, 0);

        debugDist = (*(dvar_t **)imp_fx_debugBolt)->current.value;
        endPt[0] = orient->origin[0] + orient->axis[1][0] * debugDist;
        endPt[1] = orient->origin[1] + orient->axis[1][1] * debugDist;
        endPt[2] = orient->origin[2] + orient->axis[1][2] * debugDist;
        CL_AddDebugLine(orient->origin, endPt, (const vec_t *)imp_colorGreen, 1, 0, 0);

        debugDist = (*(dvar_t **)imp_fx_debugBolt)->current.value;
        endPt[0] = orient->origin[0] + orient->axis[2][0] * debugDist;
        endPt[1] = orient->origin[1] + orient->axis[2][1] * debugDist;
        endPt[2] = orient->origin[2] + orient->axis[2][2] * debugDist;
        CL_AddDebugLine(orient->origin, endPt, (const vec_t *)imp_colorBlue, 1, 0, 0);
    }

    return 1;
}

extern void Rand_Init(int seed);
extern void FxScheduler_CreateEffect(void *scheduler, void *fx, void *primTemp, void *origin, void *orient, void *ax, int lateTime, int indexInBatch);
void FX_AddScheduledEffects(void)
{
    if (!(*(dvar_t **)imp_fx_enable)->current.enabled)
        return;

    FxScheduler *scheduler = *(FxScheduler **)imp_theFxScheduler;
    byte **prevNext = SCHEDULER_FIRST_P(scheduler);
    byte *scheduled = *prevNext;

    while (scheduled) {

        int startTime = SCH_START_TIME(scheduled);
        if (startTime > theFxHelper->mTime) {

            prevNext = &SCH_NEXT(scheduled);
            scheduled = *prevNext;
            continue;
        }

        byte *fx = SCH_FX(scheduled);
        int primIndex = SCH_PRIM_INDEX(scheduled);
        byte *primTemp = *(byte **)(fx + 8 + primIndex * 4);

        Rand_Init(SCH_RAND_SEED(scheduled));

        *prevNext = SCH_NEXT(scheduled);
        SCHEDULER_COUNT(scheduler) -= 1;

        int boltEntity = SCH_BOLT_ENTITY(scheduled);
        int indexInBatch = SCH_INDEX_IN_BATCH(scheduled);
        int lateTime = theFxHelper->mTime - startTime;

        if (boltEntity >= 0) {

            orientation_t orient;
            Bool ok = FX_GetBoneOrientation((void *)(scheduled + 0xc), &orient);
            if (ok) {
                FxScheduler_CreateEffect(scheduler, fx, primTemp,
                                         scheduled + 0xc, &orient, orient.axis,
                                         lateTime, indexInBatch);
            }
        } else {

            FxScheduler_CreateEffect(scheduler, fx, primTemp,
                                     scheduled + 0xc, scheduled + 0x14, scheduled + 0x20,
                                     lateTime, indexInBatch);
        }

        if (scheduled)
            __ZdaPv(scheduled);

        scheduled = *prevNext;
    }
}

extern void *imp_fx_visMinTraceDist;
extern float Vec3Normalize(vec_t *v);
float FX_GetServerVisibility(const vec_t *start, const vec_t *end)
{
    int count = g_effectVisArrayCount;
    if (count == 0)
        return 1.0f;

    vec3_t dir;
    dir[0] = end[0] - start[0];
    dir[1] = end[1] - start[1];
    dir[2] = end[2] - start[2];
    float len = Vec3Normalize(dir);

    float minDist = (*(dvar_t **)imp_fx_visMinTraceDist)->current.value;
    if (len < minDist)
        return 1.0f;

    float halfLen = len * 0.5f;
    float visibility = 1.0f;

    byte *visArray = (byte *)g_effectVisArray;
    int i;
    for (i = 0; i < count; i++) {
        byte *vis = visArray + i * 0x14;

        float dx = (((EffectVisInfo *)(vis))->origin[0]) - start[0];
        float dy = (((EffectVisInfo *)(vis))->origin[1]) - start[1];
        float dz = (((EffectVisInfo *)(vis))->origin[2]) - start[2];
        float t = dx * dir[0] + dy * dir[1] + dz * dir[2];

        float absDist = t - halfLen;
        if (absDist < 0)
            absDist = -absDist;
        if (absDist > halfLen)
            continue;

        vec3_t projPt;
        projPt[0] = start[0] + dir[0] * t;
        projPt[1] = start[1] + dir[1] * t;
        projPt[2] = start[2] + dir[2] * t;

        float distSq = Vec3DistanceSq((const vec_t *)vis, projPt);
        float radiusSq = (((EffectVisInfo *)(vis))->distSq);
        if (distSq >= radiusSq)
            continue;

        visibility *= (((EffectVisInfo *)(vis))->vis);
    }
    return visibility;
}

extern float sinf(float x);
extern float cosf(float x);
extern void RotatePointAroundVector(vec_t *dst, const vec_t *dir, const vec_t *point, float degrees);
extern void Vec3Cross(const vec_t *a, const vec_t *b, vec_t *out);
extern void MakeNormalVectors(const vec_t *forward, vec_t *right, vec_t *up);
extern void AxisTransformVector(void *axis, float x, float y, float z, vec_t *out);
extern void OrientationPosFromWorldPos(void *orient, vec_t *worldPos, vec_t *localPos);
extern void OrientationPosToWorldPos(void *orient, vec_t *localPos, vec_t *worldPos);
static void FX_CalcOriginAndAxis_impl(byte *prim, vec_t *orgOut, const vec_t *origin, vec3_t *ax)
{
    byte *primTemp = (byte *)((EffectPrimitive *)prim)->primTemp;

    vec3_t up = { 0.0f, 0.0f, 1.0f };
    int flags = (((PrimitiveTemplate *)(primTemp))->mSpawnFlags);
    vec3_t org;

    if (flags & 0x40) {

        float z = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mOrigin1Z)));
        float y = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mOrigin1Y)));
        float x = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mOrigin1X)));
        org[0] = x;
        org[1] = y;
        org[2] = z;
    } else {

        float z = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mOrigin1Z)));
        float y = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mOrigin1Y)));
        float x = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mOrigin1X)));
        AxisTransformVector(ax, x, y, z, org);
    }

    orgOut[0] = org[0];
    orgOut[1] = org[1];
    orgOut[2] = org[2];

    if (flags & 1) {

        float phi = flrand(0.0f, 360.0f) * 0.017453292f;
        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);
        float theta = flrand(0.0f, 180.0f) * 0.017453292f;
        float sinTheta = sinf(theta);
        float cosTheta = cosf(theta);
        float width = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mRadius)));
        float height = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mHeight)));
        vec3_t pt;
        pt[0] = sinPhi * width * sinTheta;
        pt[1] = cosPhi * width * sinTheta;
        pt[2] = height * cosTheta;
        org[0] += pt[0];
        org[1] += pt[1];
        org[2] += pt[2];
        orgOut[0] = org[0];
        orgOut[1] = org[1];
        orgOut[2] = org[2];

        if (flags & 2) {

            float len2 = pt[0] * pt[0] + pt[1] * pt[1] + pt[2] * pt[2];
            float len = __builtin_sqrtf(len2);
            if (len != 0.0f) {
                float invLen = 1.0f / len;
                ((float *)ax)[0] = pt[0] * invLen;
                ((float *)ax)[1] = pt[1] * invLen;
                ((float *)ax)[2] = pt[2] * invLen;

                MakeNormalVectors((vec_t *)ax, ((vec_t *)ax) + 3, ((vec_t *)ax) + 6);
            }
        }
    } else if (flags & 4) {

        float rndHeight = flrand(-0.5f, 0.5f);
        float height = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mHeight)));
        float width = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mRadius)));

        vec3_t pt;
        float *axf = (float *)ax;
        pt[0] = axf[3] * width + axf[0] * rndHeight * height;
        pt[1] = axf[4] * width + axf[1] * rndHeight * height;
        pt[2] = axf[5] * width + axf[2] * rndHeight * height;

        float angle = flrand(0.0f, 360.0f);
        vec3_t rotated;
        RotatePointAroundVector(rotated, (vec_t *)ax, pt, angle);
        org[0] += rotated[0];
        org[1] += rotated[1];
        org[2] += rotated[2];
        orgOut[0] = org[0];
        orgOut[1] = org[1];
        orgOut[2] = org[2];

        if (flags & 2) {
            float len2 = rotated[0] * rotated[0] + rotated[1] * rotated[1] + rotated[2] * rotated[2];
            float len = __builtin_sqrtf(len2);
            if (len != 0.0f) {
                float invLen = 1.0f / len;
                ((float *)ax)[0] = rotated[0] * invLen;
                ((float *)ax)[1] = rotated[1] * invLen;
                ((float *)ax)[2] = rotated[2] * invLen;
                float absZ = ((float *)ax)[2];
                if (absZ < 0)
                    absZ = -absZ;
                if (absZ >= 0.999f) {
                    up[0] = 0;
                    up[1] = 1;
                    up[2] = 0;
                }
                float *axf2 = (float *)ax;
                Vec3Cross((vec_t *)ax, up, (vec_t *)(axf2 + 3));
                Vec3Normalize((vec_t *)(axf2 + 3));
                Vec3Cross((vec_t *)ax, (vec_t *)(axf2 + 3), (vec_t *)(axf2 + 6));
            }
        }
    }

    for (int i = 0; i < 3; ++i)
        orgOut[i] += origin[i];
    byte *bolt = (byte *)((EffectPrimitive *)prim)->boltFrame.value;
    if (bolt) {
        void *orient = FxBoltFrame_GetOrientation(bolt);
        vec3_t localOrg;
        OrientationPosFromWorldPos(orient, orgOut, localOrg);
        orgOut[0] = localOrg[0];
        orgOut[1] = localOrg[1];
        orgOut[2] = localOrg[2];
    }
}

static inline __attribute__((always_inline)) void FX_CalcOriginAndAxis(EffectPrimitive *prim, vec_t *orgOut, const vec_t *origin, vec3_t *ax)
{
    FX_CalcOriginAndAxis_impl((byte *)prim, orgOut, origin, ax);
}

extern void AxisTransformVector(void *axis, float x, float y, float z, vec_t *out);
extern void OrientationPosFromWorldPos(void *orient, vec_t *worldPos, vec_t *localPos);
extern void Particle_SetRandomVelocityWeights(void *particle, float w0, float w1, float w2);
extern void Particle_SetRandomVelocity2Weights(void *particle, float w0, float w1, float w2);
extern void Particle_SetAxis(void *particle, vec3_t *ax);
static void FX_InitParticle_impl(byte *prim, Effect *particle, vec_t *newOrigin, const vec_t *origin, vec3_t *ax, int indexInBatch)
{
    (void)indexInBatch;
    byte *primTemp = *(byte **)(prim + 4);
    int flags = (((PrimitiveTemplate *)(primTemp))->mAttributeFlags);

    if (flags & 0x2000)
        (((Particle *)(particle))->blendWeight[0]) = flrand(0.0f, 1.0f);
    if (flags & 0x4000)
        (((Particle *)(particle))->blendWeight[1]) = flrand(0.0f, 1.0f);
    if ((short)flags < 0)
        (((Particle *)(particle))->blendWeight[2]) = flrand(0.0f, 1.0f);
    if (flags & 0x10000)
        (((Particle *)(particle))->blendWeight[3]) = flrand(0.0f, 1.0f);
    if (flags & 0x40000)
        (((Particle *)(particle))->blendWeight[4]) = flrand(0.0f, 1.0f);
    if (flags & 0x80000) {
        Particle_SetRandomVelocityWeights(particle, flrand(0, 1), flrand(0, 1), flrand(0, 1));
    }
    if (flags & 0x100000) {
        Particle_SetRandomVelocity2Weights(particle, flrand(0, 1), flrand(0, 1), flrand(0, 1));
    }

    (((Particle *)(particle))->gravity) = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mGravity)));
    (((Particle *)(particle))->windModifier) = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mWindModifier)));

    FX_CalcOriginAndAxis_impl(prim, newOrigin, origin, ax);
    Particle_SetAxis(particle, ax);

    (((Particle *)(particle))->nonUniformScale) = ((PrimitiveTemplate *)primTemp)->mNonUniformScale;
    (((Particle *)(particle))->elasticity) = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mElasticity)));
    (((Effect *)(particle))->mRefEnt.rotation) = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mRotation)));
}
static void FX_InitParticle(EffectPrimitive *prim, Particle *particle, vec_t *newOrigin, const vec_t *origin, vec3_t *ax, int indexInBatch)
{
    FX_InitParticle_impl((byte *)prim, &particle->base, newOrigin, origin, ax, indexInBatch);
}

extern float FxRange_GetVal(void *range);
extern void FxHelper_CameraShake(void *helper, vec_t *origin, float intensity, int duration, int fadeTime);
void FX_AddCameraShake(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)
{
    (void)origin;
    (void)lateTime;
    (void)indexInBatch;
    vec3_t newOrigin;
    FX_CalcOriginAndAxis(prim, newOrigin, origin, ax);
    byte *primTemp = (byte *)prim->primTemp;
    float duration = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mLife)));
    float fadeTime = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mRadius)));
    float intensity = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mElasticity)));
    FxHelper_CameraShake(theFxHelper, newOrigin, intensity, (int)duration, (int)fadeTime);
}

extern void *MediaHandles_GetEffect(void *mediaHandles);
extern void FxScheduler_PlayEffect(void *scheduler, void *effectTemplate, vec_t *origin, vec3_t *ax, void *bolt);
void FX_AddFxRunner(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)
{
    (void)origin;
    (void)lateTime;
    (void)indexInBatch;
    vec3_t newOrigin;
    FX_CalcOriginAndAxis(prim, newOrigin, origin, ax);
    byte *primTemp = (byte *)prim->primTemp;
    void *bolt = (void *)prim->boltFrame.value;
    void *effect = MediaHandles_GetEffect(((char *)primTemp + offsetof(PrimitiveTemplate, mPlayFxHandles)));
    void *scheduler = *(void **)imp_theFxScheduler;
    if (bolt) {
        FxScheduler_PlayEffect(scheduler, effect, newOrigin, NULL, (byte *)bolt + 0x3c);
    } else {
        FxScheduler_PlayEffect(scheduler, effect, newOrigin, ax, NULL);
    }
}

extern void *imp_theFxScheduler;
extern void FxScheduler_CreateDecalEffect(void *scheduler, void *primTemp, vec_t *origin, vec3_t *ax);
void FX_AddDecal(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)
{
    (void)origin;
    (void)lateTime;
    (void)indexInBatch;
    vec3_t newOrigin;
    FX_CalcOriginAndAxis(prim, newOrigin, origin, ax);
    FxScheduler_CreateDecalEffect(*(void **)imp_theFxScheduler, (void *)prim->primTemp, newOrigin, ax);
}

extern void *imp_fx_sort;
extern void *imp_fx_draw;
extern void *imp_fx_debug;
extern void FX_Print(const char *fmt, ...);
#ifndef _WIN32
extern void qsort(void *base, int nmemb, int size, void *cmp);
#endif
extern int CompareSortedClusters(const void *a, const void *b);
void FX_DrawAll(void)
{
    typedef void (*DrawFn)(void *);
    typedef Bool (*CullFn)(void *);
    int i;

    if (*(int *)imp_fx_camera_valid) {
        for (i = cullEffectCountNonBolt; i < privateEffectActiveCountNonBolt; i++) {
            Effect *eff = effectListNonBolt[i];
            if ((*(dvar_t **)imp_fx_cull)->current.enabled) {
                if (((CullFn)(*(void ***)eff)[4])(eff))
                    continue;
            }
            int idx = visibleEffectCountNonBolt;
            visibleEffectsNonBolt[idx].effect = (Effect *)eff;
            float dist = Vec3DistanceSq(eff->mRefEnt.origin, theFxHelper->mCamera.vieworg);
            visibleEffectsNonBolt[idx].distSq = dist;
            visibleEffectCountNonBolt++;
        }
        cullEffectCountNonBolt = privateEffectActiveCountNonBolt;
    }

    for (i = cullEffectCountBolt; i < privateEffectActiveCountBolt; i++) {
        Effect *eff = effectListBolt[i];
        if ((*(dvar_t **)imp_fx_cull)->current.enabled) {
            if (((CullFn)(*(void ***)eff)[4])(eff))
                continue;
        }
        int idx = visibleEffectCountBolt;
        visibleEffectsBolt[idx].effect = (Effect *)eff;
        float dist = Vec3DistanceSq(eff->mRefEnt.origin, theFxHelper->mCamera.vieworg);
        visibleEffectsBolt[idx].distSq = dist;
        visibleEffectCountBolt++;
    }
    cullEffectCountBolt = privateEffectActiveCountBolt;

    for (i = 0; i < visibleEffectCountBolt; i++) {
        int dst = visibleEffectCountNonBolt;
        visibleEffectsNonBolt[dst].effect = visibleEffectsBolt[i].effect;
        visibleEffectsNonBolt[dst].distSq = visibleEffectsBolt[i].distSq;
        visibleEffectCountNonBolt++;
    }

    g_effectVisArrayCount = 0;
    if (effectBlockSightCount > 0) {
        for (i = 0; i < effectActiveCountNonBolt; i++) {
            Effect *eff = effectListNonBolt[i];
            if (eff->mFlags & 0x1000)
                ((DrawFn)(*(void ***)eff)[7])(eff);
        }
        for (i = 0; i < effectActiveCountBolt; i++) {
            Effect *eff = effectListBolt[i];
            if (eff->mFlags & 0x1000)
                ((DrawFn)(*(void ***)eff)[7])(eff);
        }
    }

    if ((*(dvar_t **)imp_fx_sort)->current.enabled) {
        int effectCount = visibleEffectCountNonBolt;

        byte sortedClusters[1800 * 8];
        for (i = 0; i < effectClusterCount; i++) {
            *(int *)(sortedClusters + i * 8) = i;
            float dist = Vec3DistanceSq(effectClusters[i].origin, theFxHelper->mCamera.vieworg);
            *(float *)(sortedClusters + i * 8 + 4) = dist;
        }
        qsort(sortedClusters, effectClusterCount, 8, CompareSortedClusters);

        int sortOrder[1800];
        for (i = 0; i < effectClusterCount; i++) {
            int cid = *(int *)(sortedClusters + i * 8);
            sortOrder[cid] = i;
        }
        clusterSort = sortOrder;
        qsort(visibleEffectsNonBolt, effectCount, sizeof(visibleEffectsNonBolt[0]), CompareSortedEffects);
        clusterSort = NULL;
    }

    if ((*(dvar_t **)imp_fx_draw)->current.enabled) {
        for (i = 0; i < visibleEffectCountNonBolt; i++) {
            Effect *eff = visibleEffectsNonBolt[i].effect;
            ((DrawFn)(*(void ***)eff)[5])(eff);
        }
    }

    if ((*(dvar_t **)imp_fx_debug)->current.enabled) {
        FX_Print("Active    FX: %i\n", effectActiveCount);
        FX_Print("Drawn     FX: %i\n", visibleEffectCountNonBolt);
        FX_Print("Scheduled FX: %i\n", (*(FxScheduler **)imp_theFxScheduler)->mScheduledCount);
    }
}

extern void *MediaHandles_GetHandle(void *mediaHandles);
extern int FxHelper_GetMaterialSubimageCount(void *helper, void *material);
extern int irand(int min, int max);
static void FX_SetMaterialAndSequenceParams_impl(byte *primTemp, Effect *particle, int killTime, int indexInBatch)
{
    void *material = MediaHandles_GetHandle(((char *)primTemp + offsetof(PrimitiveTemplate, mMediaHandles)));
    int startFrame = 0;
    float frameRate = 0.0f;

    if (material) {
        int subimageCount = FxHelper_GetMaterialSubimageCount(theFxHelper, material);
        if (subimageCount == 1) {
            startFrame = 0;
            frameRate = 0.0f;
        } else {

            int seqMode = (int)((PrimitiveTemplate *)primTemp)->mSequenceStartFrameMode;
            if (seqMode == 0) {
                startFrame = ((PrimitiveTemplate *)primTemp)->mSequenceFixedFrameValue - 1;
            } else if (seqMode == 1) {
                startFrame = irand(0, subimageCount);
            } else if (seqMode == 2) {
                startFrame = indexInBatch;
            } else {
                startFrame = 0;
            }

            int rateMode = (int)((PrimitiveTemplate *)primTemp)->mSequencePlayRateMode;
            if (rateMode == 0) {
                frameRate = ((PrimitiveTemplate *)primTemp)->mSequenceFixedFpsValue / 1000.0f;
            } else if (rateMode == 1) {
                frameRate = (float)subimageCount / (float)killTime;
            } else {
                frameRate = 0.0f;
            }
        }
    }

    (((Particle *)(particle))->startFrame) = startFrame;
    (((Particle *)(particle))->frameRate) = frameRate;
    (((Particle *)(particle))->sequenceLoopMode) = (int)((PrimitiveTemplate *)primTemp)->mSequenceLoopMode;
    (((Particle *)(particle))->sequenceLoopTimes) = ((PrimitiveTemplate *)primTemp)->mSequenceLoopTimes;
    (*(void **)&((Effect *)(particle))->mRefEnt.customMaterial) = material;
    (((Effect *)(particle))->mSortGroup) = 0;

    if (material && FxHelper_IsMaterialRefractive(theFxHelper, (MaterialHandle)material))
        (((Effect *)(particle))->mSortGroup) = -1;
}

static void FX_SetMaterialAndSequenceParams(byte *primTemp, Effect *particle, int killTime, int indexInBatch)
{
    FX_SetMaterialAndSequenceParams_impl(primTemp, particle, killTime, indexInBatch);
}

extern void Effect_SetTimeStartEnd(void *effect, int startTime, int endTime);
extern void Effect_SetBoltFrame(const void *effect, const void *boltFramePtr);
static Bool FX_AddPrimitive_impl(byte *prim, Effect *particle, const vec_t *origin)
{
    EffectPrimitive *ep = (EffectPrimitive *)prim;
    byte *primTemp = (byte *)ep->primTemp;
    byte *boltInfo = (byte *)ep->boltFrame.value;

    Effect **dstSlot = NULL;
    if (boltInfo) {
        int slot = effectActiveCount + 1;
        effectActiveCount = slot;
        if (slot > 1800) {
            effectActiveCount = slot - 1;
        } else {
            int boltSlot = effectActiveCountBolt + 1;
            effectActiveCountBolt = boltSlot;
            dstSlot = &effectListBolt[boltSlot - 1];
        }
    } else {
        int slot = effectActiveCount + 1;
        effectActiveCount = slot;
        if (slot > 1800) {
            effectActiveCount = slot - 1;
        } else {
            int nbSlot = effectActiveCountNonBolt + 1;
            effectActiveCountNonBolt = nbSlot;
            dstSlot = &effectListNonBolt[nbSlot - 1];
        }
    }

    int clusterId = FX_GetCluster_core(origin);

    if (!dstSlot)
        return 0;

    if (((PrimitiveTemplate *)primTemp)->mAttributeFlags & 0x1000)
        effectBlockSightCount++;

    *dstSlot = (Effect *)particle;

    particle->mFlags = (((PrimitiveTemplate *)(primTemp))->mAttributeFlags);

    int curTime = theFxHelper->mTime;
    float lifeRange = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mLife)));
    int endTime = curTime + (int)lifeRange;
    Effect_SetTimeStartEnd(particle, curTime, endTime);

    (((Effect *)(particle))->impactEffect) = (struct Effect *)(uintptr_t)(int)ep->fx;
    (((Effect *)(particle))->field_0x38) = ((PrimitiveTemplate *)primTemp)->mParentPrimIndex;
    (((Effect *)(particle))->field_0x10) = ((PrimitiveTemplate *)primTemp)->mGroupFlags;

    (((Effect *)(particle))->axis[0][0]) = ((PrimitiveTemplate *)primTemp)->mMin[0];
    (((Effect *)(particle))->axis[0][1]) = ((PrimitiveTemplate *)primTemp)->mMin[1];
    (((Effect *)(particle))->axis[0][2]) = ((PrimitiveTemplate *)primTemp)->mMin[2];
    (((Effect *)(particle))->axis[1][0]) = ((PrimitiveTemplate *)primTemp)->mMax[0];
    (((Effect *)(particle))->axis[1][1]) = ((PrimitiveTemplate *)primTemp)->mMax[1];
    (((Effect *)(particle))->axis[1][2]) = ((PrimitiveTemplate *)primTemp)->mMax[2];

    (((Effect *)(particle))->emitEffect) = (struct Effect *)MediaHandles_GetEffect(((char *)primTemp + offsetof(PrimitiveTemplate, mDeathFxHandles)));
    (((Effect *)(particle))->deathEffect) = (struct Effect *)MediaHandles_GetEffect(((char *)primTemp + offsetof(PrimitiveTemplate, mImpactFxHandles)));

    typedef void (*CreateChFn)(void *, void *);
    ((CreateChFn)(*(void ***)particle)[8])(particle, primTemp);

    Effect_SetBoltFrame((const Effect *)particle, &ep->boltFrame);

    particle->mClusterId = clusterId;

    return 1;
}
static Bool FX_AddPrimitive(EffectPrimitive *prim, Effect *particle, const vec_t *origin)
{
    return FX_AddPrimitive_impl((byte *)prim, particle, origin);
}

extern void Cloud_Cloud(void *cloud);
void FX_AddCloud(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)
{
    Effect *p = (Effect *)__Znam(sizeof(Cloud));
    if (p)
        memset(p, 0, sizeof(Cloud));
    Cloud_Cloud(p);
    if (!p)
        return;
    int added = FX_AddPrimitive(prim, p, origin);
    if (!(byte)added) {
        typedef void (*Fn)(void *);
        ((Fn)(*(void ***)p)[1])(p);
        return;
    }
    vec3_t newOrigin;
    FX_InitParticle(prim, (Particle *)p, newOrigin, origin, ax, indexInBatch);
    byte *primTemp = (byte *)prim->primTemp;
    int killTime = p->mTimeEnd - p->mTimeStart;
    FX_SetMaterialAndSequenceParams_impl(primTemp, p, killTime, indexInBatch);
    if (lateTime > 0) {
        float dt = (float)lateTime * 0.001f;
        vec3_t velSum;
        Particle_IntegrateTotalVelocity(p, lateTime, velSum);
        newOrigin[0] += velSum[0] * dt;
        newOrigin[1] += velSum[1] * dt;
        newOrigin[2] += velSum[2] * dt;
    }
    PART_ANCHOR_X(p) = newOrigin[0];
    PART_ANCHOR_Y(p) = newOrigin[1];
    PART_ANCHOR_Z(p) = newOrigin[2];
    ((Cloud *)p)->randomLengthBlend = flrand(0.0f, 1.0f);
    ((Cloud *)p)->useLengthFlag = ((PrimitiveTemplate *)primTemp)->useLength;
}

extern void FX_SetPrimitiveVTable(void *effect, PrimType type);
extern void Flash_Init(void *flash);
void FX_AddFlash(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)
{
    (void)ax;
    (void)lateTime;
    (void)indexInBatch;
    Effect *p = (Effect *)__Znam(sizeof(Light));
    if (p)
        memset(p, 0, sizeof(Light));
    Light_Light(p);

    FX_SetPrimitiveVTable(p, PT_SCREENFLASH);

    int added = FX_AddPrimitive(prim, p, origin);
    if (!(byte)added) {

        typedef void (*Fn)(void *);
        ((Fn)(*(void ***)p)[1])(p);
        return;
    }

    byte *primTemp = (byte *)prim->primTemp;
    void *material = MediaHandles_GetHandle(((char *)primTemp + offsetof(PrimitiveTemplate, mMediaHandles)));

    if (origin) {
        PART_ANCHOR_X(p) = origin[0];
        PART_ANCHOR_Y(p) = origin[1];
        PART_ANCHOR_Z(p) = origin[2];
    } else {
        PART_ANCHOR_X(p) = 0;
        PART_ANCHOR_Y(p) = 0;
        PART_ANCHOR_Z(p) = 0;
    }

    (*(void **)&((Light *)p)->base.mRefEnt.customMaterial) = material;

    if (((PrimitiveTemplate *)primTemp)->mAttributeFlags & 0x2000)
        ((Light *)p)->colorBlendFactor = flrand(0.0f, 1.0f);

    ((Light *)p)->base.mSortGroup = 0;
    if (material && FxHelper_IsMaterialRefractive(theFxHelper, (MaterialHandle)material))
        ((Light *)p)->base.mSortGroup = -1;

    Flash_Init(p);
}

extern void Light_Light(void *light);
extern float flrand(float min, float max);
extern void *__Znam(int size);
void FX_AddLight(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)
{
    (void)lateTime;
    (void)indexInBatch;
    Light *light = (Light *)__Znam(sizeof(Light));
    if (light)
        memset(light, 0, sizeof(Light));
    Light_Light(light);
    if (!light)
        return;

    int added = FX_AddPrimitive(prim, &light->base, origin);
    if (!(byte)added) {
        typedef void (*Fn)(void *);
        ((Fn)(*(void ***)light)[1])(light);
        return;
    }

    vec3_t newOrigin;
    FX_CalcOriginAndAxis(prim, newOrigin, origin, ax);
    PART_ANCHOR_X(light) = newOrigin[0];
    PART_ANCHOR_Y(light) = newOrigin[1];
    PART_ANCHOR_Z(light) = newOrigin[2];

    byte *primTemp = (byte *)prim->primTemp;
    int flags = (((PrimitiveTemplate *)(primTemp))->mAttributeFlags);
    if (flags & 0x2000)
        light->colorBlendFactor = flrand(0.0f, 1.0f);
    if ((short)flags < 0)
        light->sizeBlendFactor = flrand(0.0f, 1.0f);
}

extern void Cylinder_Cylinder(void *cyl);
extern void *FxBoltFrame_GetOrientation(void *boltFrame);
extern void OrientationDirFromWorldDir(void *orient, vec_t *normal, vec_t *localNormal);
void FX_AddCylinder(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)
{
    (void)lateTime;
    Effect *p = (Effect *)__Znam(0x278);
    if (p)
        memset(p, 0, 0x278);
    Cylinder_Cylinder(p);
    if (!p)
        return;
    int added = FX_AddPrimitive(prim, p, origin);
    if (!(byte)added) {
        typedef void (*Fn)(void *);
        ((Fn)(*(void ***)p)[1])(p);
        return;
    }
    vec3_t newOrigin;
    FX_InitParticle(prim, (Particle *)p, newOrigin, origin, ax, indexInBatch);
    int killTime = p->mTimeEnd - p->mTimeStart;
    FX_SetMaterialAndSequenceParams_impl((byte *)prim->primTemp, p, killTime, indexInBatch);

    vec3_t normal;
    normal[0] = ((float *)ax)[0];
    normal[1] = ((float *)ax)[1];
    normal[2] = ((float *)ax)[2];
    void *bolt = (void *)prim->boltFrame.value;
    if (bolt) {

        void *orient = FxBoltFrame_GetOrientation(bolt);
        vec3_t localNormal;
        OrientationDirFromWorldDir(orient, normal, localNormal);

        p->mRefEnt.axis[0][0] = localNormal[0];
        p->mRefEnt.axis[0][1] = localNormal[1];
        p->mRefEnt.axis[0][2] = localNormal[2];
    } else {
        p->mRefEnt.axis[0][0] = normal[0];
        p->mRefEnt.axis[0][1] = normal[1];
        p->mRefEnt.axis[0][2] = normal[2];
    }
    PART_ANCHOR_X(p) = newOrigin[0];
    PART_ANCHOR_Y(p) = newOrigin[1];
    PART_ANCHOR_Z(p) = newOrigin[2];
}

extern void Line_Line(void *line);
extern void Particle_SetAxis(void *particle, vec3_t *ax);
extern void OrientationPosFromWorldPos(void *orient, vec_t *worldPos, vec_t *localPos);
void FX_AddLine(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)
{
    (void)lateTime;
    (void)indexInBatch;
    Effect *p = (Effect *)__Znam(0x258);
    if (p)
        memset(p, 0, 0x258);
    Line_Line(p);
    if (!p)
        return;

    int added = FX_AddPrimitive(prim, p, origin);
    if (!(byte)added) {
        typedef void (*Fn)(void *);
        ((Fn)(*(void ***)p)[1])(p);
        return;
    }

    vec3_t newOrigin;
    vec3_t org2;
    vec3_t worldEnd;
    FX_CalcOriginAndAxis(prim, newOrigin, origin, ax);

    Particle_SetAxis(p, ax);

    byte *primTemp = (byte *)prim->primTemp;
    FX_CalcOrigin2((const PrimitiveTemplate *)primTemp, newOrigin, org2, origin, ax);
    worldEnd[0] = org2[0];
    worldEnd[1] = org2[1];
    worldEnd[2] = org2[2];

    void *material = MediaHandles_GetHandle(((char *)primTemp + offsetof(PrimitiveTemplate, mMediaHandles)));

    void *bolt = (void *)prim->boltFrame.value;
    if (bolt) {
        void *orient = FxBoltFrame_GetOrientation(bolt);
        vec3_t localEnd;
        OrientationPosFromWorldPos(orient, worldEnd, localEnd);
        (((Tail *)(p))->endpoint[0]) = localEnd[0];
        (((Tail *)(p))->endpoint[1]) = localEnd[1];
        (((Tail *)(p))->endpoint[2]) = localEnd[2];
    } else {
        (((Tail *)(p))->endpoint[0]) = worldEnd[0];
        (((Tail *)(p))->endpoint[1]) = worldEnd[1];
        (((Tail *)(p))->endpoint[2]) = worldEnd[2];
    }

    PART_ANCHOR_X(p) = newOrigin[0];
    PART_ANCHOR_Y(p) = newOrigin[1];
    PART_ANCHOR_Z(p) = newOrigin[2];
    p->mRefEnt.customMaterial = (MaterialHandle)material;

    int flags = (((PrimitiveTemplate *)(primTemp))->mAttributeFlags);
    if (flags & 0x2000)
        (((Particle *)(p))->blendWeight[0]) = flrand(0.0f, 1.0f);
    if (flags & 0x4000)
        (((Particle *)(p))->blendWeight[1]) = flrand(0.0f, 1.0f);
    if ((short)flags < 0)
        (((Particle *)(p))->blendWeight[2]) = flrand(0.0f, 1.0f);
    if (flags & 0x10000)
        (((Particle *)(p))->blendWeight[3]) = flrand(0.0f, 1.0f);

    p->mSortGroup = 0;
    if (material && FxHelper_IsMaterialRefractive(theFxHelper, (MaterialHandle)material))
        p->mSortGroup = -1;
}

extern void Particle_Particle(void *particle);
extern void Particle_IntegrateTotalVelocity(void *particle, int time, vec_t *velSum);
void FX_AddParticle(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)
{
    Particle *p = (Particle *)__Znam(sizeof(Particle));
    if (p)
        memset(p, 0, sizeof(Particle));
    Particle_Particle(p);
    if (!p)
        return;

    int added = FX_AddPrimitive(prim, &p->base, origin);
    if (!(byte)added) {
        typedef void (*Fn)(void *);
        ((Fn)(*(void ***)p)[1])(p);
        return;
    }

    vec3_t newOrigin;
    FX_InitParticle(prim, p, newOrigin, origin, ax, indexInBatch);

    int killTime = p->base.mTimeEnd - p->base.mTimeStart;
    FX_SetMaterialAndSequenceParams_impl((byte *)prim->primTemp, &p->base, killTime, indexInBatch);

    if (lateTime > 0) {
        float dt = (float)lateTime * 0.001f;
        vec3_t velSum;
        Particle_IntegrateTotalVelocity(p, lateTime, velSum);
        newOrigin[0] += velSum[0] * dt;
        newOrigin[1] += velSum[1] * dt;
        newOrigin[2] += velSum[2] * dt;
    }

    PART_ANCHOR_X(p) = newOrigin[0];
    PART_ANCHOR_Y(p) = newOrigin[1];
    PART_ANCHOR_Z(p) = newOrigin[2];
}

extern void Tail_Tail(void *tail);
extern void Tail_InitEndPoint(void *tail);
void FX_AddTail(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)
{
    Effect *p = (Effect *)__Znam(sizeof(Tail));
    if (p)
        memset(p, 0, sizeof(Tail));
    Tail_Tail(p);
    if (!p)
        return;
    int added = FX_AddPrimitive(prim, p, origin);
    if (!(byte)added) {
        typedef void (*Fn)(void *);
        ((Fn)(*(void ***)p)[1])(p);
        return;
    }
    vec3_t newOrigin;
    FX_InitParticle(prim, (Particle *)p, newOrigin, origin, ax, indexInBatch);
    int killTime = p->mTimeEnd - p->mTimeStart;
    FX_SetMaterialAndSequenceParams_impl((byte *)prim->primTemp, p, killTime, indexInBatch);
    if (lateTime > 0) {
        float dt = (float)lateTime * 0.001f;
        vec3_t velSum;
        Particle_IntegrateTotalVelocity(p, lateTime, velSum);
        newOrigin[0] += velSum[0] * dt;
        newOrigin[1] += velSum[1] * dt;
        newOrigin[2] += velSum[2] * dt;
    }

    PART_ANCHOR_X(p) = newOrigin[0];
    PART_ANCHOR_Y(p) = newOrigin[1];
    PART_ANCHOR_Z(p) = newOrigin[2];

    float ny = newOrigin[1];
    (((Tail *)(p))->endpoint[0]) = newOrigin[0] - ((float *)ax)[0];
    (((Tail *)(p))->endpoint[1]) = ny - ((float *)ax)[1];
    (((Tail *)(p))->endpoint[2]) = newOrigin[2] - ((float *)ax)[2];

    ((Tail *)p)->lengthBlendFactor = flrand(0.0f, 1.0f);
    Tail_InitEndPoint(p);
}

extern void Emitter_Emitter(void *emitter);
extern void Particle_GetTotalVelocityAtTime0(void *particle, vec_t *outVector);
extern void vectoangles(const vec_t *dir, vec_t *angles);
extern float crandom(void);
void FX_AddEmitter(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)
{
    Effect *p = (Effect *)__Znam(0x29c);
    if (p)
        memset(p, 0, 0x29c);
    Emitter_Emitter(p);
    if (!p)
        return;
    int added = FX_AddPrimitive(prim, p, origin);
    if (!(byte)added) {
        typedef void (*Fn)(void *);
        ((Fn)(*(void ***)p)[1])(p);
        return;
    }
    vec3_t vecAng;
    vec3_t newOrigin;
    FX_InitParticle(prim, (Particle *)p, newOrigin, origin, ax, indexInBatch);
    byte *primTemp = (byte *)prim->primTemp;
    if (lateTime > 0) {
        float dt = (float)lateTime * 0.001f;
        vec3_t velSum;
        Particle_IntegrateTotalVelocity(p, lateTime, velSum);
        newOrigin[0] += velSum[0] * dt;
        newOrigin[1] += velSum[1] * dt;
        newOrigin[2] += velSum[2] * dt;
    }

    float ang0 = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mAngle1)));
    float ang1 = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mAngle2)));
    float ang2 = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mAngle3)));
    vectoangles((const vec_t *)ax, vecAng);
    ang0 += vecAng[0];
    ang1 += vecAng[1];
    ang2 += vecAng[2];

    float pos0 = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mAngle1Delta)));
    float pos1 = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mAngle2Delta)));
    float pos2 = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mAngle3Delta)));

    void *material = MediaHandles_GetHandle(((char *)primTemp + offsetof(PrimitiveTemplate, mMediaHandles)));
    void *effect = MediaHandles_GetEffect(((char *)primTemp + offsetof(PrimitiveTemplate, mEmitterFxHandles)));
    float stepBase = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mDensity)));
    float stepVar = FxRange_GetVal(((char *)primTemp + offsetof(PrimitiveTemplate, mVariance)));

    if (!material)
        (*(int *)&((Emitter *)p)->_base[168]) &= ~0x10;

    vec3_t vel;
    Particle_GetTotalVelocityAtTime0(p, vel);

    PART_ANCHOR_X(p) = newOrigin[0];
    PART_ANCHOR_Y(p) = newOrigin[1];
    PART_ANCHOR_Z(p) = newOrigin[2];
    /* Trail samples use world positions even when the emitter is bolted. */
    if (prim->boltFrame.value) {
        void *orient = FxBoltFrame_GetOrientation(prim->boltFrame.value);
        if (orient)
            OrientationPosToWorldPos(orient, newOrigin, ((Emitter *)p)->emitPos);
        else
            memcpy(((Emitter *)p)->emitPos, newOrigin, sizeof(vec3_t));
    } else {
        memcpy(((Emitter *)p)->emitPos, newOrigin, sizeof(vec3_t));
    }
    ((Emitter *)p)->initialVel[0] = vel[0];
    ((Emitter *)p)->initialVel[1] = vel[1];
    ((Emitter *)p)->initialVel[2] = vel[2];

    ((Emitter *)p)->spawnSize = ang0;
    (*(float *)&((Emitter *)p)->_gap2[0]) = ang1;
    (*(float *)&((Emitter *)p)->_gap2[4]) = ang2;
    ((Emitter *)p)->spawnDensity = pos0;
    (*(float *)&((Emitter *)p)->_gap3[0]) = pos1;
    ((Emitter *)p)->spawnStep = pos2;

    (*(void **)&((Emitter *)p)->_base[180]) = material;
    ((Emitter *)p)->emitFx = effect;
    ((Emitter *)p)->spawnVariance = stepBase;
    (*(float *)&((Emitter *)p)->_tail[0]) = stepVar;

    ((Emitter *)p)->emitLastTime = theFxHelper->mTime;
    ((Emitter *)p)->emitStep = stepBase + crandom() * (*(float *)&((Emitter *)p)->_tail[0]);
}

extern void OrientedParticle_OrientedParticle(void *op);
void FX_AddOrientedParticle(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch)
{
    Effect *p = (Effect *)__Znam(0x258);
    if (p)
        memset(p, 0, 0x258);
    OrientedParticle_OrientedParticle(p);
    if (!p)
        return;
    int added = FX_AddPrimitive(prim, p, origin);
    if (!(byte)added) {
        typedef void (*Fn)(void *);
        ((Fn)(*(void ***)p)[1])(p);
        return;
    }
    vec3_t newOrigin;
    vec3_t normal;
    vec3_t localNormal;
    FX_InitParticle(prim, (Particle *)p, newOrigin, origin, ax, indexInBatch);
    int killTime = p->mTimeEnd - p->mTimeStart;
    FX_SetMaterialAndSequenceParams_impl((byte *)prim->primTemp, p, killTime, indexInBatch);
    if (lateTime > 0) {
        float dt = (float)lateTime * 0.001f;
        vec3_t velSum;
        Particle_IntegrateTotalVelocity(p, lateTime, velSum);
        newOrigin[0] += velSum[0] * dt;
        newOrigin[1] += velSum[1] * dt;
        newOrigin[2] += velSum[2] * dt;
    }

    normal[0] = ((float *)ax)[0];
    normal[1] = ((float *)ax)[1];
    normal[2] = ((float *)ax)[2];
    void *bolt = (void *)prim->boltFrame.value;
    if (bolt) {
        void *orient = FxBoltFrame_GetOrientation(bolt);
        OrientationDirFromWorldDir(orient, normal, localNormal);
        (((Tail *)(p))->endpoint[0]) = localNormal[0];
        (((Tail *)(p))->endpoint[1]) = localNormal[1];
        (((Tail *)(p))->endpoint[2]) = localNormal[2];
    } else {
        (((Tail *)(p))->endpoint[0]) = normal[0];
        (((Tail *)(p))->endpoint[1]) = normal[1];
        (((Tail *)(p))->endpoint[2]) = normal[2];
    }
    PART_ANCHOR_X(p) = newOrigin[0];
    PART_ANCHOR_Y(p) = newOrigin[1];
    PART_ANCHOR_Z(p) = newOrigin[2];
}

extern void *imp_fx_enable;
extern void *imp_fx_camera_valid;
extern void *imp_fx_cull;
void FX_UpdateScheduledEffectsNonBolt(void)
{
    typedef void (*UpdateFn)(void *);
    typedef Bool (*CullFn)(void *);
    int i;

    if (!(*(dvar_t **)imp_fx_enable)->current.enabled)
        return;

    cullEffectCountNonBolt = 0;
    visibleEffectCountNonBolt = 0;

    int count = privateEffectActiveCountNonBolt;
    initialEffectActiveCountNonBolt = count;

    i = 0;
    while (i < count) {
        Effect *eff = effectListNonBolt[i];
        int curTime = theFxHelper->mTime;
        if (curTime > eff->mTimeEnd) {

            eff->mFlags &= ~0x400;
        } else {

            Bool alive = ((CullFn)(*(void ***)eff)[3])(eff);
            if (alive) {
                i++;
                count = privateEffectActiveCountNonBolt;
                continue;
            }

        }

        {
            Effect **slot = effectListNonBolt + i;
            Effect *dead = *slot;
            count--;
            privateEffectActiveCountNonBolt = count;
            Effect *last = effectListNonBolt[count];
            *slot = last;
            effectListNonBolt[count] = (Effect *)dead;

            ((UpdateFn)(*(void ***)dead)[2])(dead);
            if (dead->mFlags & 0x1000)
                effectBlockSightCount--;
            count = privateEffectActiveCountNonBolt;
        }
    }

    if (!*(int *)imp_fx_camera_valid)
        return;

    for (i = cullEffectCountNonBolt; i < privateEffectActiveCountNonBolt; i++) {
        Effect *eff = effectListNonBolt[i];
        if ((*(dvar_t **)imp_fx_cull)->current.enabled) {
            Bool culled = ((CullFn)(*(void ***)eff)[4])(eff);
            if (culled)
                continue;
        }
        int idx = visibleEffectCountNonBolt;
        visibleEffectsNonBolt[idx].effect = (Effect *)eff;
        float dist = Vec3DistanceSq(eff->mRefEnt.origin, theFxHelper->mCamera.vieworg);
        visibleEffectsNonBolt[idx].distSq = dist;
        visibleEffectCountNonBolt++;
    }
    cullEffectCountNonBolt = privateEffectActiveCountNonBolt;
}

void FX_UpdateScheduledEffectsBolt(void)
{
    typedef void (*UpdateFn)(void *);
    typedef Bool (*CullFn)(void *);
    int i, count;

    if (!(*(dvar_t **)imp_fx_enable)->current.enabled)
        return;

    cullEffectCountBolt = 0;
    visibleEffectCountBolt = 0;

    count = privateEffectActiveCountBolt;
    initialEffectActiveCountBolt = count;

    i = 0;
    while (i < count) {
        Effect *eff = effectListBolt[i];
        int curTime = theFxHelper->mTime;
        if (curTime > eff->mTimeEnd) {
            eff->mFlags &= ~0x400;
        } else {
            Bool alive = ((CullFn)(*(void ***)eff)[3])(eff);
            if (alive) {
                i++;
                count = privateEffectActiveCountBolt;
                continue;
            }

        }
        {
            Effect **slot = effectListBolt + i;
            Effect *dead = *slot;
            count--;
            privateEffectActiveCountBolt = count;
            Effect *last = effectListBolt[count];
            *slot = last;
            effectListBolt[count] = (Effect *)dead;
            ((UpdateFn)(*(void ***)dead)[2])(dead);
            if (dead->mFlags & 0x1000)
                effectBlockSightCount--;
            count = privateEffectActiveCountBolt;
        }
    }

    for (i = cullEffectCountBolt; i < privateEffectActiveCountBolt; i++) {
        Effect *eff = effectListBolt[i];
        if ((*(dvar_t **)imp_fx_cull)->current.enabled) {
            Bool culled = ((CullFn)(*(void ***)eff)[4])(eff);
            if (culled)
                continue;
        }
        int idx = visibleEffectCountBolt;
        visibleEffectsBolt[idx].effect = (Effect *)eff;
        float dist = Vec3DistanceSq(eff->mRefEnt.origin, theFxHelper->mCamera.vieworg);
        visibleEffectsBolt[idx].distSq = dist;
        visibleEffectCountBolt++;
    }
    cullEffectCountBolt = privateEffectActiveCountBolt;
}

static void FX_RemoveCluster(int clusterId)
{
    int lastIdx = effectClusterCount - 1;
    effectClusterCount = lastIdx;
    if (clusterId != lastIdx) {

        effectClusters[clusterId] = effectClusters[lastIdx];

        int i;
        for (i = 0; i < effectActiveCountBolt; i++) {
            Effect *eff = effectListBolt[i];
            if (eff->mClusterId == lastIdx)
                eff->mClusterId = clusterId;
        }
        for (i = 0; i < effectActiveCountNonBolt; i++) {
            Effect *eff = effectListNonBolt[i];
            if (eff->mClusterId == lastIdx)
                eff->mClusterId = clusterId;
        }
    }
}
void FX_UpdateAllBolt(void)
{
    typedef void (*VtFn)(void *);
    typedef Bool (*UpdateFn)(void *);
    int i, count;

    count = effectActiveCountBolt;
    privateEffectActiveCountBolt = count;
    initialEffectActiveCountBolt = count;

    i = 0;
    while (i < count) {
        Effect *eff = effectListBolt[i];
        int curTime = theFxHelper->mTime;
        if (curTime > eff->mTimeEnd) {
            eff->mFlags &= ~0x400;
        } else {
            Bool alive = ((UpdateFn)(*(void ***)eff)[3])(eff);
            if (alive) {
                i++;
                count = privateEffectActiveCountBolt;
                continue;
            }

        }
        {

            Effect *dead = eff;
            count--;
            privateEffectActiveCountBolt = count;
            Effect *last = effectListBolt[count];
            effectListBolt[i] = (Effect *)last;
            effectListBolt[count] = (Effect *)dead;
            ((VtFn)(*(void ***)dead)[2])(dead);
            if (dead->mFlags & 0x1000)
                effectBlockSightCount--;
            count = privateEffectActiveCountBolt;
        }
    }

    for (i = count; i < initialEffectActiveCountBolt; i++) {
        Effect *eff = effectListBolt[i];
        int clusterId = eff->mClusterId;
        EffectCluster *cluster = &effectClusters[clusterId];
        cluster->refCount -= 1;
        if (cluster->refCount <= 0) {
            FX_RemoveCluster(clusterId);
        }
        ((VtFn)(*(void ***)eff)[1])(eff);
        effectActiveCountBolt--;
        effectListBolt[i] = effectListBolt[effectActiveCountBolt];
        effectActiveCount--;
    }
}

void FX_Rewind(int time)
{
    typedef void (*VtFn)(void *);
    int i, count;

    count = effectActiveCountBolt;
    privateEffectActiveCountBolt = count;
    initialEffectActiveCountBolt = count;

    i = 0;
    while (i < count) {
        Effect *eff = effectListBolt[i];
        if (time < eff->mTimeStart) {

            count--;
            privateEffectActiveCountBolt = count;
            Effect **slot = effectListBolt + i;
            Effect *removed = *slot;
            *slot = effectListBolt[count];
            effectListBolt[count] = (Effect *)removed;
            if (removed->mFlags & 0x1000)
                effectBlockSightCount--;
            count = privateEffectActiveCountBolt;
        } else {
            i++;
            count = privateEffectActiveCountBolt;
        }
    }

    for (i = count; i < initialEffectActiveCountBolt; i++) {
        Effect *eff = effectListBolt[i];
        int clusterId = eff->mClusterId;
        EffectCluster *cluster = &effectClusters[clusterId];
        cluster->refCount -= 1;
        if (cluster->refCount <= 0)
            FX_RemoveCluster(clusterId);
        ((VtFn)(*(void ***)eff)[1])(eff);
        effectActiveCountBolt--;
        effectListBolt[i] = effectListBolt[effectActiveCountBolt];
        effectActiveCount--;
    }

    count = effectActiveCountNonBolt;
    privateEffectActiveCountNonBolt = count;
    initialEffectActiveCountNonBolt = count;

    i = 0;
    while (i < count) {
        Effect *eff = effectListNonBolt[i];
        if (time < eff->mTimeStart) {
            count--;
            privateEffectActiveCountNonBolt = count;
            Effect **slot = effectListNonBolt + i;
            Effect *removed = *slot;
            *slot = effectListNonBolt[count];
            effectListNonBolt[count] = (Effect *)removed;
            if (removed->mFlags & 0x1000)
                effectBlockSightCount--;
            count = privateEffectActiveCountNonBolt;
        } else {
            i++;
            count = privateEffectActiveCountNonBolt;
        }
    }

    for (i = count; i < initialEffectActiveCountNonBolt; i++) {
        Effect *eff = effectListNonBolt[i];
        int clusterId = eff->mClusterId;
        EffectCluster *cluster = &effectClusters[clusterId];
        cluster->refCount -= 1;
        if (cluster->refCount <= 0)
            FX_RemoveCluster(clusterId);
        ((VtFn)(*(void ***)eff)[1])(eff);
        effectActiveCountNonBolt--;
        effectListNonBolt[i] = effectListNonBolt[effectActiveCountNonBolt];
        effectActiveCount--;
    }
}

void FX_UpdateAllNonBolt(void)
{
    typedef void (*VtFn)(void *);
    typedef Bool (*UpdateFn)(void *);
    int i, count;

    count = effectActiveCountNonBolt;
    privateEffectActiveCountNonBolt = count;
    initialEffectActiveCountNonBolt = count;

    i = 0;
    while (i < count) {
        Effect *eff = effectListNonBolt[i];
        int curTime = theFxHelper->mTime;
        if (curTime > eff->mTimeEnd) {
            eff->mFlags &= ~0x400;
        } else {
            Bool alive = ((UpdateFn)(*(void ***)eff)[3])(eff);
            if (alive) {
                i++;
                count = privateEffectActiveCountNonBolt;
                continue;
            }

        }
        {
            Effect *dead = eff;
            count--;
            privateEffectActiveCountNonBolt = count;
            Effect *last = effectListNonBolt[count];
            effectListNonBolt[i] = (Effect *)last;
            effectListNonBolt[count] = (Effect *)dead;
            ((VtFn)(*(void ***)dead)[2])(dead);
            if (dead->mFlags & 0x1000)
                effectBlockSightCount--;
            count = privateEffectActiveCountNonBolt;
        }
    }

    for (i = count; i < initialEffectActiveCountNonBolt; i++) {
        Effect *eff = effectListNonBolt[i];
        int clusterId = eff->mClusterId;
        EffectCluster *cluster = &effectClusters[clusterId];
        cluster->refCount -= 1;
        if (cluster->refCount <= 0)
            FX_RemoveCluster(clusterId);
        ((VtFn)(*(void ***)eff)[1])(eff);
        effectActiveCountNonBolt--;
        effectListNonBolt[i] = effectListNonBolt[effectActiveCountNonBolt];
        effectActiveCount--;
    }
}

void FX_DrawScheduledEffects(void)
{
    typedef void (*VtFn)(void *);
    typedef Bool (*UpdateFn)(void *);
    int i, count;
    int firstNewNonBolt, firstNewBolt;

    if (!(*(dvar_t **)imp_fx_enable)->current.enabled)
        return;

    count = privateEffectActiveCountNonBolt;
    for (i = count; i < initialEffectActiveCountNonBolt; i++) {
        Effect *eff = effectListNonBolt[i];
        int clusterId = eff->mClusterId;
        EffectCluster *cluster = &effectClusters[clusterId];
        cluster->refCount -= 1;
        if (cluster->refCount <= 0)
            FX_RemoveCluster(clusterId);
        ((VtFn)(*(void ***)eff)[1])(eff);
        effectActiveCountNonBolt--;
        effectListNonBolt[i] = effectListNonBolt[effectActiveCountNonBolt];
        effectActiveCount--;
    }

    count = privateEffectActiveCountBolt;
    for (i = count; i < initialEffectActiveCountBolt; i++) {
        Effect *eff = effectListBolt[i];
        int clusterId = eff->mClusterId;
        EffectCluster *cluster = &effectClusters[clusterId];
        cluster->refCount -= 1;
        if (cluster->refCount <= 0)
            FX_RemoveCluster(clusterId);
        ((VtFn)(*(void ***)eff)[1])(eff);
        effectActiveCountBolt--;
        effectListBolt[i] = effectListBolt[effectActiveCountBolt];
        effectActiveCount--;
    }

    /* Existing particles were advanced by the non-bolt/bolt update passes.
       Only newly scheduled particles need initialization here. */
    firstNewNonBolt = effectActiveCountNonBolt;
    firstNewBolt = effectActiveCountBolt;
    FX_AddScheduledEffects();

    count = effectActiveCountNonBolt;
    privateEffectActiveCountNonBolt = count;
    initialEffectActiveCountNonBolt = count;
    i = firstNewNonBolt;
    while (i < count) {
        Effect *eff = effectListNonBolt[i];
        int curTime = theFxHelper->mTime;
        if (curTime > eff->mTimeEnd) {
            eff->mFlags &= ~0x400;
        } else {
            Bool alive = ((UpdateFn)(*(void ***)eff)[3])(eff);
            if (alive) {
                i++;
                count = privateEffectActiveCountNonBolt;
                continue;
            }

        }
        {
            Effect *dead = eff;
            count--;
            privateEffectActiveCountNonBolt = count;
            Effect *last = effectListNonBolt[count];
            effectListNonBolt[i] = (Effect *)last;
            effectListNonBolt[count] = (Effect *)dead;
            ((VtFn)(*(void ***)dead)[2])(dead);
            if (dead->mFlags & 0x1000)
                effectBlockSightCount--;
            count = privateEffectActiveCountNonBolt;
        }
    }

    count = effectActiveCountBolt;
    privateEffectActiveCountBolt = count;
    initialEffectActiveCountBolt = count;
    i = firstNewBolt;
    while (i < count) {
        Effect *eff = effectListBolt[i];
        int curTime = theFxHelper->mTime;
        if (curTime > eff->mTimeEnd) {
            eff->mFlags &= ~0x400;
        } else {
            Bool alive = ((UpdateFn)(*(void ***)eff)[3])(eff);
            if (alive) {
                i++;
                count = privateEffectActiveCountBolt;
                continue;
            }

        }
        {
            Effect *dead = eff;
            count--;
            privateEffectActiveCountBolt = count;
            Effect *last = effectListBolt[count];
            effectListBolt[i] = (Effect *)last;
            effectListBolt[count] = (Effect *)dead;
            ((VtFn)(*(void ***)dead)[2])(dead);
            if (dead->mFlags & 0x1000)
                effectBlockSightCount--;
            count = privateEffectActiveCountBolt;
        }
    }

    /* Scheduling and expiry can replace list entries after the earlier update
       commands. Build visibility from the surviving particles before drawing. */
    cullEffectCountNonBolt = cullEffectCountBolt = 0;
    visibleEffectCountNonBolt = visibleEffectCountBolt = 0;
    FX_DrawAll();
}

extern void FxArchive_FxArchive(void *arch);
extern void FxArchive_ReadData(void *arch, void *data, int size);
extern void FxArchive_BeginReading(void *arch, void *memFile);
extern void FxHelper_Archive(void *helper, void *arch);
extern void FxScheduler_Archive(void *scheduler, void *arch);
extern void Line_Line(void *line);
extern void Tail_Tail(void *tail);
extern void Emitter_Emitter(void *emitter);
int FX_Restore(MemoryFile *memFile)
{
    typedef void (*VtFn)(void *);
    typedef void (*ArchFn)(void *, void *);
    typedef void (*FixupFn)(void *, void *);
    int i;
    byte arch[16];

    FxArchive_FxArchive(arch);

    for (i = 0; i < effectActiveCountBolt; i++) {
        Effect *eff = effectListBolt[i];
        if (eff)
            ((VtFn)(*(void ***)eff)[1])(eff);
    }
    for (i = 0; i < effectActiveCountNonBolt; i++) {
        Effect *eff = effectListNonBolt[i];
        if (eff)
            ((VtFn)(*(void ***)eff)[1])(eff);
    }
    effectActiveCountBolt = 0;
    effectActiveCountNonBolt = 0;
    effectActiveCount = 0;
    effectBlockSightCount = 0;
    effectClusterCount = 0;

    void **schedulerPtr = (void **)imp_theFxScheduler;
    if (*schedulerPtr)
        FxScheduler_Clean(*schedulerPtr, 0, 0);

    FxArchive_BeginReading(arch, memFile);
    FxHelper_Archive(theFxHelper, arch);
    FxScheduler_Archive(*schedulerPtr, arch);

    for (;;) {
        byte effectType;
        FxArchive_ReadData(arch, &effectType, 1);
        if (effectType == 0)
            break;

        int size;
        FxArchive_ReadData(arch, &size, 4);

        if (effectType > 12)
            continue;

        static const int sizes[] = { 0, 0x24c, 0xfc, 0x258, 0x278, 0x278, 0x27c, 0x258, 0x278, 0x29c, 0x258, 0x278, 0xfc };
        static const char *ctors[] = { NULL, "Particle", "Light", "Line", "Tail", "Cylinder", "Cloud", "OrientedParticle", "Tail", "Emitter", "Line", "Cylinder", "Flash" };
        (void)ctors;

        int allocSize = (effectType <= 12) ? sizes[effectType] : 0;
        if (allocSize == 0)
            continue;

        Effect *eff = (Effect *)__Znam(allocSize);
        if (eff)
            memset(eff, 0, allocSize);

        switch (effectType) {
        case 1:
            Particle_Particle(eff);
            break;
        case 2:
        case 12:
            Light_Light(eff);
            break;
        case 3:
        case 10:
            Line_Line(eff);
            break;
        case 4:
        case 8:
            Tail_Tail(eff);
            break;
        case 5:
        case 11:
            Cylinder_Cylinder(eff);
            break;
        case 6:
            Cloud_Cloud(eff);
            break;
        case 7:
            OrientedParticle_OrientedParticle(eff);
            break;
        case 9:
            Emitter_Emitter(eff);
            break;
        }

        ((ArchFn)(*(void ***)eff)[10])(eff, arch);

        EffectTemplate *fxTemplate = (EffectTemplate *)(((Effect *)(eff))->impactEffect);
        int primIdx = (((Effect *)(eff))->field_0x38);
        byte *primTemp = NULL;
        if (fxTemplate && primIdx < fxTemplate->mPrimitiveCount)
            primTemp = (byte *)fxTemplate->mPrimitives[primIdx];

        if (!primTemp)
            continue;

        ((FixupFn)(*(void ***)eff)[11])(eff, primTemp);

        if (eff->mFlags & 0x1000)
            effectBlockSightCount++;

        if (eff->mBolt.value) {
            effectListBolt[effectActiveCountBolt] = (Effect *)eff;
            effectActiveCountBolt++;
        } else {
            effectListNonBolt[effectActiveCountNonBolt] = (Effect *)eff;
            effectActiveCountNonBolt++;
        }
        effectActiveCount++;
    }

    int bytesRead = *(int *)&((FxArchive *)arch)->memFile;
    return bytesRead;
}

extern void *Z_MallocInternal(int size);
extern void FxScheduler_FxScheduler(void *scheduler);
extern void FX_InitTemplates(void);
extern void FxHelper_Init(void *helper);
extern void *imp_g_rendererExists;
int FX_Init(int rendererExists)
{
    int i;
    if (!*(int *)fxInitialized) {
        *(int *)fxInitialized = 1;
        effectActiveCountBolt = 0;
        effectActiveCountNonBolt = 0;
        effectActiveCount = 0;
        effectBlockSightCount = 0;
        effectClusterCount = 0;
    }

    for (i = 0; i < effectActiveCountBolt; i++) {
        void *eff = ((void **)effectListBolt)[i];
        if (eff) {
            typedef void (*Fn)(void *);
            ((Fn)(*(void ***)eff)[1])(eff);
        }
    }
    for (i = 0; i < effectActiveCountNonBolt; i++) {
        void *eff = ((void **)effectListNonBolt)[i];
        if (eff) {
            typedef void (*Fn)(void *);
            ((Fn)(*(void ***)eff)[1])(eff);
        }
    }
    effectActiveCountBolt = 0;
    effectActiveCountNonBolt = 0;
    effectActiveCount = 0;
    effectBlockSightCount = 0;
    effectClusterCount = 0;

    void **schedulerPtr = (void **)imp_theFxScheduler;
    if (*schedulerPtr) {
        FxScheduler_Clean(*schedulerPtr, 1, 0);
        Z_FreeInternal(*schedulerPtr);
        *schedulerPtr = NULL;
        *(void **)imp_fxSchedulers = NULL;
    }

    g_rendererExists = rendererExists;

    void *newScheduler = Z_MallocInternal(0xc);
    FxScheduler_FxScheduler(newScheduler);
    *schedulerPtr = newScheduler;

    FX_InitTemplates();

    *(void **)imp_fxSchedulers = *schedulerPtr;

    FxHelper_Init(theFxHelper);
    return 1;
}

extern void Z_FreeInternal(void *ptr);
extern void FxScheduler_Clean(void *scheduler, int bRemoveTemplates, int arg3);
extern void *imp_fxSchedulers;
void FX_Free(int bRemoveTemplates)
{
    int i;
    byte bRemove = (byte)bRemoveTemplates;

    for (i = 0; i < effectActiveCountBolt; i++) {
        void *eff = ((void **)effectListBolt)[i];
        if (eff) {
            typedef void (*DestroyFn)(void *);
            ((DestroyFn)(*(void ***)eff)[1])(eff);
        }
    }

    for (i = 0; i < effectActiveCountNonBolt; i++) {
        void *eff = ((void **)effectListNonBolt)[i];
        if (eff) {
            typedef void (*DestroyFn)(void *);
            ((DestroyFn)(*(void ***)eff)[1])(eff);
        }
    }
    effectActiveCountBolt = 0;
    effectActiveCountNonBolt = 0;
    effectActiveCount = 0;
    effectBlockSightCount = 0;
    effectClusterCount = 0;

    void **schedulerPtr = (void **)imp_theFxScheduler;
    if (*schedulerPtr) {
        FxScheduler_Clean(*schedulerPtr, bRemove, 0);
        if (bRemove) {
            Z_FreeInternal(*schedulerPtr);
            *schedulerPtr = NULL;
            *(void **)imp_fxSchedulers = NULL;
        }
    }
}
