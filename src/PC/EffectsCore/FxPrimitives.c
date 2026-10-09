#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <math.h>

#define FXH_TIME(p) (((FxHelper *)(p))->mTime)

#define PART_ANCHOR_X(p) (((Effect *)(p))->origin[0])
#define PART_ANCHOR_Y(p) (((Effect *)(p))->origin[1])
#define PART_ANCHOR_Z(p) (((Effect *)(p))->origin[2])

COD2_ASSERT_FIELD(struct Effect, normTime, 0x3c);
COD2_ASSERT_FIELD(struct Effect, mFlags, 0xa8);
COD2_ASSERT_FIELD(struct Effect, mTimeStart, 0xb8);
COD2_ASSERT_FIELD(struct Effect, mTimeEnd, 0xbc);
COD2_ASSERT_FIELD(struct Effect, mModelPtr, 0xb4);
COD2_ASSERT_FIELD(struct Effect, mBolt, 0xc0);
COD2_ASSERT_FIELD(struct FxHelper, mTime, 0x04);
COD2_ASSERT_FIELD(struct FxHelper, mFrameTime, 0x0c);
COD2_ASSERT_FIELD(struct FxHelper, mCamera, 0x14);
COD2_ASSERT_FIELD(struct FxCamera, numPlanes, 0x6c);
COD2_ASSERT_FIELD(struct FxBoltFrame, next, 0x38);
COD2_ASSERT_FIELD(struct FxBoltFrame, mBolt, 0x3c);
COD2_ASSERT_FIELD(struct FxBoltInfo, dobjHandle, 0x00);
COD2_ASSERT_FIELD(struct FxBoltInfo, boneIndex, 0x04);
COD2_ASSERT_FIELD(struct Particle, gravity, 0xf4);
COD2_ASSERT_FIELD(struct Particle, elasticity, 0x100);
COD2_ASSERT_FIELD(struct Tail, tailLength, 0x258);
COD2_ASSERT_FIELD(struct Tail, lengthBlendFactor, 0x25c);
COD2_ASSERT_FIELD(struct Cloud, randomDirection, 0x24c);
COD2_ASSERT_FIELD(struct Cloud, useLengthFlag, 0x25c);
COD2_ASSERT_FIELD(struct Cloud, randomLengthBlend, 0x260);
COD2_ASSERT_FIELD(struct Cloud, heightChannel, 0x264);
COD2_ASSERT_FIELD(struct Cloud, heightRandChannel, 0x270);
COD2_ASSERT_FIELD(struct Emitter, emitLastTime, 0x270);
COD2_ASSERT_FIELD(struct Emitter, emitStep, 0x274);
COD2_ASSERT_FIELD(struct Emitter, emitFx, 0x290);
COD2_ASSERT_FIELD(Particle, velocityWeightX,  0x12c);
COD2_ASSERT_FIELD(Particle, velocityWeightY,  0x130);
COD2_ASSERT_FIELD(Particle, velocityWeightZ,  0x134);
COD2_ASSERT_FIELD(Particle, velocity2WeightX, 0x138);
COD2_ASSERT_FIELD(Particle, velocity2WeightY, 0x13c);
COD2_ASSERT_FIELD(Particle, velocity2WeightZ, 0x140);
COD2_ASSERT_FIELD(struct Effect, mBolt,       0xc0);

extern FxBoltFrame *FxBoltFrame_g_mFrameList;
extern void *Hunk_AllocInternal(int size);
extern struct XModel *XModelPrecache(const char *name, void *(*alloc1)(int), void *(*alloc2)(int));

void FxBoltFrame_Release(const FxBoltFrame *_this);
const orientation_t *FxBoltFrame_GetOrientation(const FxBoltFrame *_this);
static void *FxModelAlloc(int size);
struct XModel *FX_XModelPrecache(const char *name);
void Effect_Die(const Effect *_this);
Bool Effect_Cull(const Effect *_this);
void Effect_Draw(const Effect *_this);
void Effect_SetTimeStartEnd(const Effect *_this, int start, int end);
unsigned char Effect_TypeID(const Effect *_this);
void Effect_FixupArchiveLoad(const Effect *_this, const PrimitiveTemplate *primTemplate);
void FxChannelInstance_Create(const FxChannel *master, FxChannelInstance *createe);
void Particle_CreateChannelInstances(const Particle *_this, const PrimitiveTemplate *primTemp);
void Particle_SetRandomVelocity2Weights(const Particle *_this, float weight1, float weight2, float weight3);
unsigned char Particle_TypeID(const Particle *_this);
unsigned char OrientedParticle_TypeID(const OrientedParticle *_this);
void Cloud_Die(const Cloud *_this);
void Cloud_CreateChannelInstances(const Cloud *_this, const PrimitiveTemplate *primTemp);
unsigned char Cloud_TypeID(const Cloud *_this);
void Line_Die(const Line *_this);
unsigned char Line_TypeID(const Line *_this);
void Tail_CreateChannelInstances(const Tail *_this, const PrimitiveTemplate *primTemp);
unsigned char Tail_TypeID(const Tail *_this);
unsigned char Cylinder_TypeID(const Cylinder *_this);
Bool Emitter_Cull(const Emitter *_this);
unsigned char Emitter_TypeID(const Emitter *_this);
void Light_CreateChannelInstances(const Light *_this, const PrimitiveTemplate *primTemp);
void Light_Draw(const Light *_this);
unsigned char Light_TypeID(const Light *_this);
void Flash_Init(const Flash *_this);
unsigned char Flash_TypeID(const Flash *_this);
static void GLOBAL__I__ZN11FxBoltFrame12g_mFrameListE(void);
void Particle_AddVisibility(const Particle *_this);
void Emitter_Draw(const Emitter *_this);
void Cylinder_Draw(const Cylinder *_this);
void Tail_Draw(const Tail *_this);
void Line_Draw(const Line *_this);
void Cloud_Draw(const Cloud *_this);
void OrientedParticle_Draw(const OrientedParticle *_this);
void Particle_Draw(const Particle *_this);
Bool Effect_Update(const Effect *_this);
Bool Particle_Cull(const Particle *_this);
Bool OrientedParticle_Cull(const OrientedParticle *_this);
Bool Cloud_Cull(const Cloud *_this);
Bool Line_Cull(const Line *_this);
Bool Tail_Cull(const Tail *_this);
Bool Cylinder_Cull(const Cylinder *_this);
Bool Light_Cull(const Light *_this);
void Particle_SetRandomVelocityWeights(const Particle *_this, float weight1, float weight2, float weight3);
float Particle_GetVisibility(const Particle *_this, const vec_t *start, const vec_t *dir, float halfLen);
void Particle_FixupArchiveLoad(const Particle *_this, const PrimitiveTemplate *primTemplate);
void Light_FixupArchiveLoad(const Light *_this, const PrimitiveTemplate *primTemplate);
void Particle_SetAxis(const Particle *_this, vec3_t *ax);
void Flash_Draw(const Flash *_this);
const FxBoltFramePtr FxBoltFrame_Acquire(const FxBoltInfo *bolt);
void Particle_Die(const Particle *_this);
void Tail_CalcNewEndpoint(const Tail *_this, const orientation_t *or_);
void Cloud_FixupArchiveLoad(const Cloud *_this, const PrimitiveTemplate *primTemplate);
void Tail_FixupArchiveLoad(const Tail *_this, const PrimitiveTemplate *primTemplate);
void Light_UpdateRGB(const Light *_this, const Light *_this_1);
void ZN6EffectD1Ev(void *_this);
void FxBoltFramePtr_Archive(const FxBoltFramePtr *_this, FxArchive *arch);
void ZN6EffectD0Ev(void *_this);
void Effect_SetBoltFrame(const Effect *_this, FxBoltFramePtr *boltFrame);
void Particle_UpdateRGB(const Particle *_this);
Bool Flash_Update(const Flash *_this);
void Particle_IntegrateVelocity(const Particle *_this, float normDuration, vec_t *outVector);
void Particle_IntegrateVelocity2(const Particle *_this, float normDuration, vec_t *outVector);
void Particle_IntegrateTotalVelocity(const Particle *_this, int duration, vec_t *outVector);
void Particle_Particle(const Particle *_this);
void FX_SetPrimitiveVTable(void *effect, PrimType type);
void ZN8ParticleD1Ev(void *_this);
void ZN8ParticleD0Ev(void *_this);
void Light_Light(const Light *_this);
void ZN5LightD1Ev(void *_this);
void ZN5LightD0Ev(void *_this);
void Tail_InitEndPoint(const Tail *_this);
Bool Light_Update(const Light *_this);
void Particle_CalcVelocityValue(const Particle *_this, float normTime, vec_t *outVector, const orientation_t *or_);
void Particle_CalcVelocity2Value(const Particle *_this, float normTime, vec_t *outVector, const orientation_t *or_);
void Particle_GetTotalVelocity(const Particle *_this, float normTime, vec_t *outVector, const orientation_t *or_);
void Emitter_UpdateEmitFx(const Emitter *_this, vec_t *bindVelocity, const orientation_t *or_);
void Particle_GetTotalVelocityAtTime0(const Particle *_this, vec_t *outVector);
void Particle_ApplyImpact(const Particle *_this, const orientation_t *or_, float normTime, const vec_t *velocity, float traceFraction, const vec_t *traceNormal);
Bool Particle_UpdateOrigin(const Particle *_this, const orientation_t *or_);
Bool Emitter_Update(const Emitter *_this);
void Particle_UpdateAlpha(const Particle *_this);
Bool Cylinder_Update(const Cylinder *_this);
Bool Tail_Update(const Tail *_this);
Bool Line_Update(const Line *_this);
Bool Cloud_Update(const Cloud *_this);
Bool OrientedParticle_Update(const OrientedParticle *_this);
Bool Particle_Update(const Particle *_this, const Particle *_this_1, const Cloud *_this_2);
void Effect_Archive(const Effect *_this, FxArchive *arch);
void Light_Archive(const Light *_this, FxArchive *arch);
void Flash_Archive(const Flash *_this, FxArchive *arch);
void Particle_Archive(const Particle *_this, FxArchive *arch);
void OrientedParticle_Archive(const OrientedParticle *_this, FxArchive *arch);
void Cloud_Archive(const Cloud *_this, FxArchive *arch);
void Line_Archive(const Line *_this, FxArchive *arch);
void Tail_Archive(const Tail *_this, FxArchive *arch);
void Cylinder_Archive(const Cylinder *_this, FxArchive *arch);
void Emitter_Archive(const Emitter *_this, FxArchive *arch);
void ZN8CylinderD0Ev(void *_this);
void ZN8CylinderD1Ev(void *_this);
void Cylinder_Cylinder(const Cylinder *_this);
void OrientedParticle_OrientedParticle(const OrientedParticle *_this);
void ZN16OrientedParticleD1Ev(void *_this);
void ZN16OrientedParticleD0Ev(void *_this);
void Cloud_Cloud(Cloud *_this);
void ZN5CloudD1Ev(void *_this);
void ZN5CloudD0Ev(void *_this);
void Line_Line(const Line *_this);
void ZN4LineD1Ev(void *_this);
void ZN4LineD0Ev(void *_this);
void Tail_Tail(const Tail *_this);
void ZN4TailD1Ev(void *_this);
void ZN4TailD0Ev(void *_this);
void Emitter_Emitter(const Emitter *_this);
void ZN7EmitterD1Ev(void *_this);
void ZN7EmitterD0Ev(void *_this);
float Effect_GetVisibility(const Effect *_this, const vec_t *start, const vec_t *dir, float halfLen);
void Effect_AddVisibility(const Effect *_this);
Bool Flash_Cull(const Flash *_this);
void ZN5FlashD0Ev(void *_this);
void ZN5FlashD1Ev(void *_this);

extern void __ZdaPv(void *ptr);
extern void *__Znam(int size);
extern void OrientationDirFromWorldDir(void *orient, vec_t *worldDir, vec_t *localDir);
extern void OrientationPosToWorldPos(void *orient, vec_t *localPos, vec_t *worldPos);
extern void AxisTransformVector(void *curve, float x, float y, float z, vec_t *out);
extern void FxArchive_ReadData(void *arch, void *data, int size);
extern void FxArchive_WriteData(void *arch, void *data, int size);
extern void FxArchive_ArchiveChannelInstance(void *arch, void *channelInst);
extern byte *__ZN11FxBoltFrame12g_mFrameListE;
extern byte __ZTV6Effect[];
static inline __attribute__((always_inline)) void FxBoltFrame_ReleaseHelper(byte *boltFrame)
{
    if (!boltFrame)
        return;
    int refCount = *(int *)boltFrame - 1;
    *(int *)boltFrame = refCount;
    if (refCount != 0)
        return;

    byte **prevNext = &__ZN11FxBoltFrame12g_mFrameListE;
    byte *cur = *prevNext;
    while (cur) {
        if (cur == boltFrame) {
            *prevNext = (*(byte **)&((FxBoltFrame *)(boltFrame))->next);
            break;
        }
        prevNext = &(*(byte **)&((FxBoltFrame *)(cur))->next);
        cur = *prevNext;
    }
    if (boltFrame)
        __ZdaPv(boltFrame);
}

void FxBoltFrame_Release(const FxBoltFrame *_this)
{
    byte *boltFrame = (byte *)_this;
    int refCount = *(int *)boltFrame - 1;
    *(int *)boltFrame = refCount;
    if (refCount != 0)
        return;
    byte **prevNext = &__ZN11FxBoltFrame12g_mFrameListE;
    byte *cur = *prevNext;
    while (cur) {
        if (cur == boltFrame) {
            *prevNext = (*(byte **)&((FxBoltFrame *)(boltFrame))->next);
            break;
        }
        prevNext = &(*(byte **)&((FxBoltFrame *)(cur))->next);
        cur = *prevNext;
    }
    if (boltFrame)
        __ZdaPv(boltFrame);
}

extern int FX_GetBoneOrientation(int *boltInfo, orientation_t *orient);
const orientation_t *FxBoltFrame_GetOrientation(const FxBoltFrame *_this)
{
    byte *p = (byte *)_this;
    clientActive_t *cl_ptr;
    int serverTime;
    orientation_t *orient;

    if ((((FxBoltFrame *)(p))->mBolt.dobjHandle) < 0)
        return (const orientation_t *)0;

    orient = (&((FxBoltFrame *)(p))->orientation);

    cl_ptr = *(clientActive_t **)imp_cl;
    serverTime = cl_ptr->skelTimeStamp;
    if (serverTime != (((FxBoltFrame *)(p))->cachedServerTime)) {

        (((FxBoltFrame *)(p))->cachedServerTime) = serverTime;

        if (!FX_GetBoneOrientation(((void *)&((FxBoltFrame *)(p))->mBolt), orient)) {

            (((FxBoltFrame *)(p))->mBolt.dobjHandle) = -1;
            (((FxBoltFrame *)(p))->mBolt.boneIndex) = -1;
            return (const orientation_t *)0;
        }
    }

    return orient;
}

static void *FxModelAlloc(int size)
{
    return Hunk_AllocInternal(size);
}

struct XModel *FX_XModelPrecache(const char *name)
{
    return XModelPrecache(name, FxModelAlloc, FxModelAlloc);
}

void Effect_Die(const Effect *_this)
{
}

Bool Effect_Cull(const Effect *_this)
{
    return 0;
}

void Effect_Draw(const Effect *_this)
{
}

void Effect_SetTimeStartEnd(const Effect *_this, int start, int end)
{
    ((Effect *)_this)->mTimeStart = start;
    ((Effect *)_this)->mTimeEnd = end;
}

unsigned char Effect_TypeID(const Effect *_this)
{
    return 0;
}

void Effect_FixupArchiveLoad(const Effect *_this, const PrimitiveTemplate *primTemplate)
{
}

extern void FxCurveIterator_Create(void *inst, void *curve);
extern float FxRange_GetVal(void *range);
void FxChannelInstance_Create(const FxChannel *master, FxChannelInstance *createe)
{

    FxCurveIterator_Create(createe, (void *)master->curve);

    createe->scale = FxRange_GetVal((void *)&master->scaleRange);
}

void Particle_CreateChannelInstances(const Particle *_this, const PrimitiveTemplate *primTemp)
{
    byte *t = (byte *)_this;
    byte *p = (byte *)primTemp;
#define CREATE(field, idx)                                                                           \
    FxChannelInstance_Create((const FxChannel *)(p + offsetof(PrimitiveTemplate, mFxChannels[idx])), \
                             (FxChannelInstance *)(t + offsetof(Particle, field)))
    CREATE(colorChannelInstance, 0);
    CREATE(colorRandChannelInstance, 1);
    CREATE(alphaChannelInstance, 2);
    CREATE(alphaRandChannelInstance, 3);
    CREATE(sizeChannelInstance, 4);
    CREATE(sizeRandChannelInstance, 5);
    CREATE(size2ChannelInstance, 6);
    CREATE(size2RandChannelInstance, 7);
    CREATE(rotationDeltaChannelInstance, 10);
    CREATE(rotationDeltaRandChannelInstance, 11);
    CREATE(velocityXChannelInstance, 12);
    CREATE(velocityYChannelInstance, 13);
    CREATE(velocityZChannelInstance, 14);
    CREATE(velocityXRandChannelInstance, 15);
    CREATE(velocityYRandChannelInstance, 16);
    CREATE(velocityZRandChannelInstance, 17);
    CREATE(velocity2XChannelInstance, 18);
    CREATE(velocity2YChannelInstance, 19);
    CREATE(velocity2ZChannelInstance, 20);
    CREATE(velocity2XRandChannelInstance, 21);
    CREATE(velocity2YRandChannelInstance, 22);
    CREATE(velocity2ZRandChannelInstance, 23);
#undef CREATE
}

void Particle_SetRandomVelocity2Weights(const Particle *_this, float weight1, float weight2, float weight3)
{
    Particle *p = (Particle *)_this;
    p->velocity2WeightX = weight1;
    p->velocity2WeightY = weight2;
    p->velocity2WeightZ = weight3;
}

unsigned char Particle_TypeID(const Particle *_this)
{
    return 1;
}

unsigned char OrientedParticle_TypeID(const OrientedParticle *_this)
{
    return 7;
}

void Cloud_Die(const Cloud *_this)
{
}

void Cloud_CreateChannelInstances(const Cloud *_this, const PrimitiveTemplate *primTemp)
{
    byte *t = (byte *)_this;
    byte *p = (byte *)primTemp;
    Particle_CreateChannelInstances((const Particle *)_this, primTemp);
    FxChannelInstance_Create((const FxChannel *)(p + 0x160), (FxChannelInstance *)&((Cloud *)t)->heightChannel.curveIterator.master);
    FxChannelInstance_Create((const FxChannel *)(p + 0x16c), (FxChannelInstance *)&((Cloud *)t)->heightRandChannel.curveIterator.master);
}

unsigned char Cloud_TypeID(const Cloud *_this)
{
    return 12;
}

void Line_Die(const Line *_this)
{
}

unsigned char Line_TypeID(const Line *_this)
{
    return 2;
}

void Tail_CreateChannelInstances(const Tail *_this, const PrimitiveTemplate *primTemp)
{
    byte *t = (byte *)_this;
    byte *p = (byte *)primTemp;
    Particle_CreateChannelInstances((const Particle *)_this, primTemp);
    FxChannelInstance_Create((const FxChannel *)(p + 0x160), (FxChannelInstance *)&((Tail *)t)->lengthChannelInstance.curveIterator.master);
    FxChannelInstance_Create((const FxChannel *)(p + 0x16c), (FxChannelInstance *)&((Tail *)t)->lengthRandChannelInstance.curveIterator.master);
}

unsigned char Tail_TypeID(const Tail *_this)
{
    return 3;
}

unsigned char Cylinder_TypeID(const Cylinder *_this)
{
    return 4;
}

Bool Emitter_Cull(const Emitter *_this)
{
    return 0;
}

unsigned char Emitter_TypeID(const Emitter *_this)
{
    return 5;
}

void Light_CreateChannelInstances(const Light *_this, const PrimitiveTemplate *primTemp)
{
    byte *t = (byte *)_this;
    byte *p = (byte *)primTemp;
    FxChannelInstance_Create((const FxChannel *)(p + 0x100), (FxChannelInstance *)&((Light *)t)->colorChannelInstance.curveIterator.master);
    FxChannelInstance_Create((const FxChannel *)(p + 0x10c), (FxChannelInstance *)&((Light *)t)->colorRandChannelInstance.curveIterator.master);
    FxChannelInstance_Create((const FxChannel *)(p + 0x130), (FxChannelInstance *)&((Light *)t)->sizeChannelInstance.curveIterator.master);
    FxChannelInstance_Create((const FxChannel *)(p + 0x13c), (FxChannelInstance *)&((Light *)t)->sizeRandChannelInstance.curveIterator.master);
}

extern void FxHelper_AddLightToScene(void *helper, float *origin, float radius, float r, float g, float b);
void Light_Draw(const Light *_this)
{
    byte *p = (byte *)_this;
    FxHelper *helper = *(FxHelper **)imp_theFxHelper;

    FxHelper_AddLightToScene(helper,
                             ((float *)((byte *)(p) + offsetof(Effect, mRefEnt.origin))),
                             (((Effect *)(p))->mRefEnt.radius[0]),
                             (((Effect *)(p))->mRefEnt.dlightColor[0]),
                             (((Effect *)(p))->mRefEnt.dlightColor[1]),
                             (((Effect *)(p))->mRefEnt.dlightColor[2]));
}

unsigned char Light_TypeID(const Light *_this)
{
    return 9;
}

extern float Vec3Normalize(float *v);
void Flash_Init(const Flash *_this)
{
    byte *p = (byte *)_this;
    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    float dif[3];
    float dist, dot, falloff;
    float *camOrigin, *camDir;

    camOrigin = helper->mCamera.vieworg;
    dif[0] = PART_ANCHOR_X(p) - camOrigin[0];
    dif[1] = PART_ANCHOR_Y(p) - camOrigin[1];
    dif[2] = *(float *)(p + 0xc) - camOrigin[2];

    dist = Vec3Normalize(dif);

    camDir = helper->mCamera.frustum[0];
    dot = dif[0] * camDir[0] + dif[1] * camDir[1] + dif[2] * camDir[2];

    if (dist > 600.0f) {
        dot = 0.0f;
    } else if (dot < 0.5f) {
        if (dist > 100.0f)
            dot = 0.0f;
        else if (dist <= 100.0f)
            dot += 1.100000023841858f;
    }

    falloff = dist * dist / (-360000.0f) + 1.0f;
    dot *= falloff;

    *(float *)(p + 0xd4) *= dot;
}

unsigned char Flash_TypeID(const Flash *_this)
{
    return 11;
}

static void GLOBAL__I__ZN11FxBoltFrame12g_mFrameListE(void)
{
}

void Particle_AddVisibility(const Particle *_this)
{
    byte *p = (byte *)_this;
    int *countPtr = (int *)imp_g_effectVisArrayCount;
    byte *visArray = (byte *)imp_g_effectVisArray;
    int idx = *countPtr;
    byte *entry = visArray + idx * 20;
    float radius, alpha;

    *countPtr = idx + 1;

    (*(int *)&((EffectVisInfo *)(entry))->origin[0]) = (*(int *)&((Effect *)(p))->mRefEnt.origin[0]);
    (*(int *)&((EffectVisInfo *)(entry))->origin[1]) = (*(int *)&((Effect *)(p))->mRefEnt.origin[1]);
    (*(int *)&((EffectVisInfo *)(entry))->origin[2]) = (*(int *)&((Effect *)(p))->mRefEnt.origin[2]);

    radius = (((Effect *)(p))->mRefEnt.radius[0]);
    (((EffectVisInfo *)(entry))->distSq) = radius * radius;

    alpha = (float)(((byte *)((byte *)(p) + offsetof(Effect, mRefEnt.materialRGBA)))[3]);
    (((EffectVisInfo *)(entry))->vis) = alpha * (-0.003921568859368563f) + 1.0f;
}

extern void AxisCopy(const vec_t *src, vec_t *dst);
extern void FxHelper_AddFxToScene(void *helper, void *ent, int sortGroup);
static void FX_AddFxToScene_impl(byte *effect, int reType)
{
    byte ent[0x74];
    memset(ent, 0, sizeof(GfxEntity));
    (*(int *)&((GfxEntity *)(ent))->reType) = reType;
    (*(int *)&((GfxEntity *)(ent))->customMaterial) = (*(int *)&((Effect *)(effect))->mRefEnt.customMaterial);
    (*(int *)&((GfxEntity *)(ent))->rotation) = (*(int *)&((Effect *)(effect))->mRefEnt.rotation);
    AxisCopy(((vec_t *)((byte *)(effect) + offsetof(Effect, mRefEnt.axis))), ((vec_t *)((byte *)(ent) + offsetof(GfxEntity, axis))));

    (((GfxEntity *)(ent))->origin[0]) = (((Effect *)(effect))->mRefEnt.origin[0]);
    (((GfxEntity *)(ent))->origin[1]) = (((Effect *)(effect))->mRefEnt.origin[1]);
    (((GfxEntity *)(ent))->origin[2]) = (((Effect *)(effect))->mRefEnt.origin[2]);

    (*(int *)&((GfxEntity *)(ent))->radius) = *(int *)&((Effect *)effect)->mRefEnt.radius[0];
    *(int *)&((GfxEntity *)ent)->radius[1] = *(int *)&((Effect *)effect)->mRefEnt.radius[1];

    ((byte *)((byte *)(ent) + offsetof(GfxEntity, materialRGBA)))
    [0] = ((byte *)((byte *)(effect) + offsetof(Effect, mRefEnt.materialRGBA)))[0];
    ((byte *)((byte *)(ent) + offsetof(GfxEntity, materialRGBA)))
    [1] = ((byte *)((byte *)(effect) + offsetof(Effect, mRefEnt.materialRGBA)))[1];
    ((byte *)((byte *)(ent) + offsetof(GfxEntity, materialRGBA)))
    [2] = ((byte *)((byte *)(effect) + offsetof(Effect, mRefEnt.materialRGBA)))[2];
    ((byte *)((byte *)(ent) + offsetof(GfxEntity, materialRGBA)))
    [3] = ((byte *)((byte *)(effect) + offsetof(Effect, mRefEnt.materialRGBA)))[3];
    (((GfxEntity *)(ent))->materialSubimageIndex) = ((Effect *)effect)->mRefEnt.materialSubimageIndex;
    (*(int *)&((GfxEntity *)(ent))->scale) = *(int *)&((Effect *)effect)->mRefEnt.scale;

    (((GfxEntity *)(ent))->endpos[0]) = ((Effect *)effect)->mRefEnt.endpos[0];
    (((GfxEntity *)(ent))->endpos[1]) = ((Effect *)effect)->mRefEnt.endpos[1];
    (((GfxEntity *)(ent))->endpos[2]) = ((Effect *)effect)->mRefEnt.endpos[2];

    int flags = (((Effect *)(effect))->mFlags);
    if (flags & 1)
        (((GfxEntity *)(ent))->renderFxFlags) |= 8;
    if (flags & 0x4000000)
        (((GfxEntity *)(ent))->renderFxFlags) |= 0x80;
    FxHelper_AddFxToScene(*(void **)imp_theFxHelper, ent,
                          *(int *)&((Effect *)effect)->mModelPtr);
}

void Emitter_Draw(const Emitter *_this)
{
    byte *p = (byte *)_this;

    if (!((*(int *)&((Emitter *)p)->_base[168]) & 0x10))
        return;

    if ((((Effect *)(p))->mRefEnt.scale) == 0.0f)
        return;

    FX_AddFxToScene_impl(p, 1);
}

void Cylinder_Draw(const Cylinder *_this)
{
    FX_AddFxToScene_impl((byte *)_this, 9);
}

void Tail_Draw(const Tail *_this)
{
    FX_AddFxToScene_impl((byte *)_this, 8);
}

void Line_Draw(const Line *_this)
{
    FX_AddFxToScene_impl((byte *)_this, 8);
}

void Cloud_Draw(const Cloud *_this)
{
    FX_AddFxToScene_impl((byte *)_this, 6);
}

void OrientedParticle_Draw(const OrientedParticle *_this)
{
    FX_AddFxToScene_impl((byte *)_this, 7);
}

void Particle_Draw(const Particle *_this)
{
    byte *p = (byte *)_this;

    if ((((Effect *)(p))->mRefEnt.radius[0]) == 0.0f)
        return;
    if ((((Effect *)(p))->mRefEnt.radius[1]) == 0.0f)
        return;
    FX_AddFxToScene_impl(p, 4);
}

Bool Effect_Update(const Effect *_this)
{
    byte *p = (byte *)_this;
    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    int startTime = ((Effect *)p)->mTimeStart;
    int curTime = FXH_TIME(helper);
    int endTime;
    float normDuration;

    if (startTime > curTime)
        return 0;

    endTime = ((Effect *)p)->mTimeEnd;
    normDuration = (float)(curTime - startTime) / (float)(endTime - startTime);
    ((Effect *)p)->normTime = normDuration;

    if (normDuration > 1.0f)
        ((Effect *)p)->normTime = 1.0f;

    if (0.0f > ((Effect *)p)->normTime)
        ((Effect *)p)->normTime = 0.0f;

    return 1;
}

extern unsigned char FxHelper_CullSphere(void *helper, float *origin, float radius, int cullType);
Bool Particle_Cull(const Particle *_this)
{
    byte *p = (byte *)_this;
    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    int cullType = (helper->mCamera.numPlanes);

    if (*(byte *)(p + 0xab) & 2) {
        if (cullType >= 5)
            cullType = 5;
    }
    return (Bool)FxHelper_CullSphere(helper, ((float *)((byte *)(p) + offsetof(Effect, mRefEnt.origin))), (((Effect *)(p))->mRefEnt.radius[0]), cullType);
}

Bool OrientedParticle_Cull(const OrientedParticle *_this)
{
    byte *p = (byte *)_this;
    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    int cullType = (helper->mCamera.numPlanes);
    if ((*(byte *)&((OrientedParticle *)p)->_base[171]) & 2) {
        if (cullType >= 5)
            cullType = 5;
    }
    return (Bool)FxHelper_CullSphere(helper, ((float *)((byte *)(p) + offsetof(Effect, mRefEnt.origin))), (((Effect *)(p))->mRefEnt.radius[0]), cullType);
}

Bool Cloud_Cull(const Cloud *_this)
{
    byte *p = (byte *)_this;
    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    int cullType = (helper->mCamera.numPlanes);
    float halfLen, height, radius, cullRadius;

    if (*(byte *)(p + 0xab) & 2) {
        if (cullType >= 5)
            cullType = 5;
    }

    halfLen = (((Effect *)(p))->mRefEnt.scale);
    height = (((Effect *)(p))->mRefEnt.radius[1]);
    radius = (((Effect *)(p))->mRefEnt.radius[0]);
    cullRadius = (radius - height < 0.0f ? height : radius) + halfLen;

    return (Bool)FxHelper_CullSphere(helper, ((float *)((byte *)(p) + offsetof(Effect, mRefEnt.origin))), cullRadius, cullType);
}

extern unsigned char FxHelper_CullCylinder(void *helper, float *origin1, float *origin2, float radius1, float radius2, int cullType);
Bool Line_Cull(const Line *_this)
{
    byte *p = (byte *)_this;
    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    int cullType = (helper->mCamera.numPlanes);
    float radius;
    if ((*(byte *)&((Line *)p)->_base[171]) & 2) {
        if (cullType >= 5)
            cullType = 5;
    }
    radius = (((Effect *)(p))->mRefEnt.radius[0]);
    return (Bool)FxHelper_CullCylinder(helper, ((float *)((byte *)(p) + offsetof(Effect, mRefEnt.origin))), ((float *)((byte *)(p) + offsetof(Effect, mRefEnt.endpos))), radius, radius, cullType);
}

Bool Tail_Cull(const Tail *_this)
{
    byte *p = (byte *)_this;
    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    int cullType = (helper->mCamera.numPlanes);
    float radius;
    if (*(byte *)(p + 0xab) & 2) {
        if (cullType >= 5)
            cullType = 5;
    }
    radius = (((Effect *)(p))->mRefEnt.radius[0]);
    return (Bool)FxHelper_CullCylinder(helper, ((float *)((byte *)(p) + offsetof(Effect, mRefEnt.origin))), ((float *)((byte *)(p) + offsetof(Effect, mRefEnt.endpos))), radius, radius, cullType);
}

Bool Cylinder_Cull(const Cylinder *_this)
{
    byte *p = (byte *)_this;
    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    int cullType = (helper->mCamera.numPlanes);
    if (*(byte *)(p + 0xab) & 2) {
        if (cullType >= 5)
            cullType = 5;
    }
    return (Bool)FxHelper_CullCylinder(helper, ((float *)((byte *)(p) + offsetof(Effect, mRefEnt.origin))), ((float *)((byte *)(p) + offsetof(Effect, mRefEnt.endpos))),
                                       (((Effect *)(p))->mRefEnt.radius[0]), (((Effect *)(p))->mRefEnt.radius[1]), cullType);
}

Bool Light_Cull(const Light *_this)
{
    byte *p = (byte *)_this;
    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    int cullType = (helper->mCamera.numPlanes);
    if (*(byte *)(p + 0xab) & 2) {
        if (cullType >= 5)
            cullType = 5;
    }
    return (Bool)FxHelper_CullSphere(helper, ((float *)((byte *)(p) + offsetof(Effect, mRefEnt.origin))), (((Effect *)(p))->mRefEnt.radius[0]), cullType);
}

void Particle_SetRandomVelocityWeights(const Particle *_this, float weight1, float weight2, float weight3)
{
    Particle *p = (Particle *)_this;
    p->velocityWeightX = weight1;
    p->velocityWeightY = weight2;
    p->velocityWeightZ = weight3;
}

extern float Vec3DistanceSq(float *a, float *b);
float Particle_GetVisibility(const Particle *_this, const vec_t *start, const vec_t *dir, float halfLen)
{
    byte *p = (byte *)_this;
    float *origin = ((float *)((byte *)(p) + offsetof(Effect, mRefEnt.origin)));
    float *s = (float *)start;
    float *d = (float *)dir;
    float dot, absDist, projPt[3], distSq, radiusSq;

    dot = (origin[0] - s[0]) * d[0] + (origin[1] - s[1]) * d[1] + (origin[2] - s[2]) * d[2];

    absDist = dot - halfLen;
    if (absDist < 0.0f)
        absDist = -absDist;
    if (absDist > halfLen)
        return 1.0f;

    projPt[0] = s[0] + d[0] * dot;
    projPt[1] = s[1] + d[1] * dot;
    projPt[2] = s[2] + d[2] * dot;

    distSq = Vec3DistanceSq(origin, projPt);

    radiusSq = (((Effect *)(p))->mRefEnt.radius[0]);
    radiusSq *= radiusSq;
    if (radiusSq <= distSq)
        return 1.0f;

    return (float)(((byte *)((byte *)(p) + offsetof(Effect, mRefEnt.materialRGBA)))[3]) * (-0.003921568859368563f) + 1.0f;
}

static inline __attribute__((always_inline)) void Particle_FixupArchiveLoad_core(const Particle *_this, const PrimitiveTemplate *primTemplate)
{
    byte *t = (byte *)_this;
    byte *p = (byte *)primTemplate;

#define FIX(field, idx)                                                      \
    *(int *)&((Particle *)t)->field.curveIterator.master =                   \
        *(int *)&((PrimitiveTemplate *)p)->mFxChannels[idx].curve
    FIX(colorChannelInstance, 0);
    FIX(colorRandChannelInstance, 1);
    FIX(alphaChannelInstance, 2);
    FIX(alphaRandChannelInstance, 3);
    FIX(sizeChannelInstance, 4);
    FIX(sizeRandChannelInstance, 5);
    FIX(size2ChannelInstance, 6);
    FIX(size2RandChannelInstance, 7);
    FIX(rotationDeltaChannelInstance, 10);
    FIX(rotationDeltaRandChannelInstance, 11);
    FIX(velocityXChannelInstance, 12);
    FIX(velocityYChannelInstance, 13);
    FIX(velocityZChannelInstance, 14);
    FIX(velocityXRandChannelInstance, 15);
    FIX(velocityYRandChannelInstance, 16);
    FIX(velocityZRandChannelInstance, 17);
    FIX(velocity2XChannelInstance, 18);
    FIX(velocity2YChannelInstance, 19);
    FIX(velocity2ZChannelInstance, 20);
    FIX(velocity2XRandChannelInstance, 21);
    FIX(velocity2YRandChannelInstance, 22);
    FIX(velocity2ZRandChannelInstance, 23);
#undef FIX
}

void Particle_FixupArchiveLoad(const Particle *_this, const PrimitiveTemplate *primTemplate)
{
    Particle_FixupArchiveLoad_core(_this, primTemplate);
}

void Light_FixupArchiveLoad(const Light *_this, const PrimitiveTemplate *primTemplate)
{
    byte *t = (byte *)_this;
    byte *p = (byte *)primTemplate;
#define FIX(field, idx)                                                   \
    *(int *)&((Light *)t)->field.curveIterator.master =                   \
        *(int *)&((PrimitiveTemplate *)p)->mFxChannels[idx].curve
    FIX(colorChannelInstance, 0);
    FIX(colorRandChannelInstance, 1);
    FIX(sizeChannelInstance, 4);
    FIX(sizeRandChannelInstance, 5);
#undef FIX
}

void Particle_SetAxis(const Particle *_this, vec3_t *ax)
{
    byte *p = (byte *)_this;
    float *src = (float *)ax;

    if (ax) {
        ((Particle *)p)->displayAxis[0][0] = src[0];
        ((Particle *)p)->displayAxis[0][1] = src[1];
        ((Particle *)p)->displayAxis[0][2] = src[2];
    } else {
        ((Particle *)p)->displayAxis[0][0] = 0.0f;
        ((Particle *)p)->displayAxis[0][1] = 0.0f;
        ((Particle *)p)->displayAxis[0][2] = 0.0f;
    }

    if (ax) {
        ((Particle *)p)->displayAxis[1][0] = src[3];
        ((Particle *)p)->displayAxis[1][1] = src[4];
        ((Particle *)p)->displayAxis[1][2] = src[5];
    } else {
        ((Particle *)p)->displayAxis[1][0] = 0.0f;
        ((Particle *)p)->displayAxis[1][1] = 0.0f;
        ((Particle *)p)->displayAxis[1][2] = 0.0f;
    }

    if (ax) {
        ((Particle *)p)->displayAxis[2][0] = src[6];
        ((Particle *)p)->displayAxis[2][1] = src[7];
        ((Particle *)p)->displayAxis[2][2] = src[8];
    } else {
        ((Particle *)p)->displayAxis[2][0] = 0.0f;
        ((Particle *)p)->displayAxis[2][1] = 0.0f;
        ((Particle *)p)->displayAxis[2][2] = 0.0f;
    }
}

extern float floorf(float x);
static inline __attribute__((always_inline)) int FloatToByte(float f)
{
    int v = (int)(f * 255.0f);
    if (v >= 255)
        return 255;
    if (v > 0)
        return v;
    return 0;
}

static inline __attribute__((always_inline)) int FlashColorByte(float f)
{
    int v = (int)floorf(f * 255.0f + 0.5f);
    if (v >= 255)
        return 255;
    if (v > 0)
        return v;
    return 0;
}
void Flash_Draw(const Flash *_this)
{
    byte *self = (byte *)_this;

    float color[4];
    const float *src = (const float *)(self + 0x6c);
    int n;
    for (n = 1; n != 4; n++) {
        float v = *src++;
        if (v < 0.0f)
            v = 0.0f;
        else if (v > 1.0f)
            v = 1.0f;
        color[n - 1] = v;
    }
    color[3] = 1.0f;

    ((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))
    [0] = (byte)FlashColorByte(color[0]);
    ((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))
    [1] = (byte)FlashColorByte(color[1]);
    ((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))
    [2] = (byte)FlashColorByte(color[2]);
    ((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))
    [3] = (byte)FlashColorByte(color[3]);

    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    (((Effect *)(self))->mRefEnt.origin[0]) = (helper->mCamera.vieworg[0]) + (helper->mCamera.frustum[0][0]) * 8.0f;
    (((Effect *)(self))->mRefEnt.origin[1]) = (helper->mCamera.vieworg[1]) + (helper->mCamera.frustum[0][1]) * 8.0f;
    (((Effect *)(self))->mRefEnt.origin[2]) = (helper->mCamera.vieworg[2]) + (helper->mCamera.frustum[0][2]) * 8.0f;

    (((Effect *)(self))->mRefEnt.radius[0]) = 12.0f;
    ((Effect *)self)->mRefEnt.radius[1] = 12.0f;

    FX_AddFxToScene_impl(self, 4);
}

static void FxBoltFrame_Acquire_impl(byte *retPtr, byte *bolt)
{

    byte *frame = __ZN11FxBoltFrame12g_mFrameListE;
    int entity = (((FxBoltInfo *)(bolt))->dobjHandle);
    int bone = (((FxBoltInfo *)(bolt))->boneIndex);
    while (frame) {
        if ((((FxBoltFrame *)(frame))->mBolt.dobjHandle) == entity && (((FxBoltFrame *)(frame))->mBolt.boneIndex) == bone) {
            (((FxBoltFrame *)(frame))->refCount) += 1;
            *(byte **)retPtr = frame;
            return;
        }
        frame = (*(byte **)&((FxBoltFrame *)(frame))->next);
    }

    byte *newFrame = (byte *)__Znam(sizeof(FxBoltFrame));
    if (newFrame)
        memset(newFrame, 0, sizeof(FxBoltFrame));
    (((FxBoltFrame *)(newFrame))->refCount) = 0;
    (((FxBoltFrame *)(newFrame))->cachedServerTime) = 0;
    (((FxBoltFrame *)(newFrame))->mBolt.dobjHandle) = entity;
    (((FxBoltFrame *)(newFrame))->mBolt.boneIndex) = bone;
    (*(byte **)&((FxBoltFrame *)(newFrame))->next) = __ZN11FxBoltFrame12g_mFrameListE;
    __ZN11FxBoltFrame12g_mFrameListE = newFrame;
    (((FxBoltFrame *)(newFrame))->refCount) += 1;
    *(byte **)retPtr = newFrame;
}
const FxBoltFramePtr FxBoltFrame_Acquire(const FxBoltInfo *bolt)
{
    FxBoltFramePtr ret;

    FxBoltFrame_Acquire_impl((byte *)&ret, (byte *)bolt);
    return ret;
}

extern float flrand(float min, float max);
extern void FxScheduler_PlayEffect(void *scheduler, void *fx, float *origin, float *dir, void *bolt);
extern void FX_PlayEffect(EffectTemplate *fx, const vec_t *origin, const vec_t *normal);
void Particle_Die(const Particle *_this)
{
    byte *p = (byte *)_this;
    int flags;
    float x, y, z, lenSq, len, scale;
    float norm[3];

    flags = ((Particle *)p)->base.mFlags;
    if (!(flags & 0x200))
        return;
    if (flags & 0x400)
        return;

    if ((*(void **)&((Particle *)p)->base.emitEffect) == 0)
        return;

    z = flrand(-1.0f, 1.0f);
    y = flrand(-1.0f, 1.0f);
    x = flrand(-1.0f, 1.0f);

    norm[0] = x;
    norm[1] = y;
    norm[2] = z;

    lenSq = x * x + y * y + z * z;
    len = __builtin_sqrtf(lenSq);
    if ((double)len >= 1e-06) {

        scale = 1.0f / len;
        norm[0] *= scale;
        norm[1] *= scale;
        norm[2] *= scale;
    } else {
        norm[0] = 0.0f;
        norm[1] = 0.0f;
        norm[2] = 1.0f;
    }

    FX_PlayEffect((EffectTemplate *)((Particle *)p)->base.emitEffect, (float *)(p + 4), norm);
}

void Tail_CalcNewEndpoint(const Tail *_this, const orientation_t *or_)
{
    byte *self = (byte *)_this;

    float dx = (((Tail *)(self))->endpoint[0]) - PART_ANCHOR_X(self);
    float dy = (((Tail *)(self))->endpoint[1]) - PART_ANCHOR_Y(self);
    float dz = (((Tail *)(self))->endpoint[2]) - PART_ANCHOR_Z(self);

    float lenSq = dx * dx + dy * dy + dz * dz;
    float len = __builtin_sqrtf(lenSq);
    if (len <= 0.0f)
        return;

    float invLen = 1.0f / len;
    dx *= invLen;
    dy *= invLen;
    dz *= invLen;

    float scale = (((Tail *)(self))->tailLength);
    float newX = dx * scale + PART_ANCHOR_X(self);
    float newY = dy * scale + PART_ANCHOR_Y(self);
    float newZ = dz * scale + PART_ANCHOR_Z(self);

    if (or_) {

        vec3_t localPt;
        localPt[0] = newX;
        localPt[1] = newY;
        localPt[2] = newZ;
        OrientationPosToWorldPos((void *)or_, localPt, (vec_t *)&((Tail *)self)->base.base.mRefEnt.endpos[0]);
    } else {
        ((Effect *)self)->mRefEnt.endpos[0] = newX;
        ((Effect *)self)->mRefEnt.endpos[1] = newY;
        ((Effect *)self)->mRefEnt.endpos[2] = newZ;
    }
}

void Cloud_FixupArchiveLoad(const Cloud *_this, const PrimitiveTemplate *primTemplate)
{
    byte *t = (byte *)_this;
    byte *p = (byte *)primTemplate;

    Particle_FixupArchiveLoad_core((const Particle *)_this, primTemplate);

    *(int *)&((Cloud *)t)->heightChannel.curveIterator.master =
        *(int *)&((PrimitiveTemplate *)p)->mFxChannels[8].curve;
    *(int *)&((Cloud *)t)->heightRandChannel.curveIterator.master =
        *(int *)&((PrimitiveTemplate *)p)->mFxChannels[9].curve;
}

void Tail_FixupArchiveLoad(const Tail *_this, const PrimitiveTemplate *primTemplate)
{
    byte *t = (byte *)_this;
    byte *p = (byte *)primTemplate;

    Particle_FixupArchiveLoad_core((const Particle *)_this, primTemplate);

    *(int *)&((Tail *)t)->lengthChannelInstance.curveIterator.master =
        *(int *)&((PrimitiveTemplate *)p)->mFxChannels[8].curve;
    *(int *)&((Tail *)t)->lengthRandChannelInstance.curveIterator.master =
        *(int *)&((PrimitiveTemplate *)p)->mFxChannels[9].curve;
}

#define KEY_TIME(k) (*(float *)(k))
#define KEY_V(k, i) (*(float *)((byte *)(k) + 4 + (i) * 4))

static inline __attribute__((always_inline)) int Curve_FindKey(byte *channelInst, byte *keys, int keyStride, float normTime)
{
    int keyIdx = (((FxChannelInstance *)(channelInst))->curveIterator.currentKeyIndex);
    if (KEY_TIME(keys + keyIdx * keyStride) > normTime)
        keyIdx = 0;
    while (KEY_TIME(keys + (keyIdx + 1) * keyStride) < normTime)
        keyIdx++;
    (((FxChannelInstance *)(channelInst))->curveIterator.currentKeyIndex) = keyIdx;
    return keyIdx;
}

static inline __attribute__((always_inline)) float EvalCurve1(byte *channelInst, float normTime)
{
    byte *curve = *(byte **)channelInst;
    int componentCount = *(int *)curve + 1;
    byte *keys = curve + 8;
    int keyStride = componentCount * 4;

    int keyIdx = Curve_FindKey(channelInst, keys, keyStride, normTime);

    byte *k0 = keys + keyIdx * 8;
    byte *k1 = k0 + 8;
    float t0 = KEY_TIME(k0);
    float v0 = KEY_V(k0, 0);
    float v1 = KEY_V(k1, 0);
    return v0 + (normTime - t0) * (v1 - v0) / (KEY_TIME(k1) - t0);
}

static inline __attribute__((always_inline)) void EvalCurve3(byte *channelInst, float normTime, float *out)
{
    byte *curve = *(byte **)channelInst;
    int componentCount = *(int *)curve + 1;
    byte *keys = curve + 8;
    int keyStride = componentCount * 4;

    int keyIdx = Curve_FindKey(channelInst, keys, keyStride, normTime);

    byte *k0 = keys + keyIdx * 16;
    byte *k1 = k0 + 16;
    float t0 = KEY_TIME(k0);
    float frac = (normTime - t0) / (KEY_TIME(k1) - t0);

    out[0] = KEY_V(k0, 0) + (KEY_V(k1, 0) - KEY_V(k0, 0)) * frac;
    out[1] = KEY_V(k0, 1) + (KEY_V(k1, 1) - KEY_V(k0, 1)) * frac;
    out[2] = KEY_V(k0, 2) + (KEY_V(k1, 2) - KEY_V(k0, 2)) * frac;
}

void Light_UpdateRGB(const Light *_this, const Light *_this_1)
{
    byte *self = (byte *)_this;
    float normTime = ((Effect *)self)->normTime;
    float *rgb = (float *)&((Light *)self)->base.mRefEnt.dlightColor[0];

    if (*(byte *)(self + 0xa9) & 0x20) {

        float blendFactor = ((Light *)self)->colorBlendFactor;
        float val0[3], val1[3];
        EvalCurve3(((char *)self + offsetof(Light, colorChannelInstance.curveIterator.master)), normTime, val0);
        EvalCurve3(((char *)self + offsetof(Light, colorRandChannelInstance.curveIterator.master)), normTime, val1);
        float scale = *(float *)(((char *)self + offsetof(Light, colorChannelInstance.curveIterator.master)) + 8);
        rgb[0] = (val0[0] + (val1[0] - val0[0]) * blendFactor) * scale;
        rgb[1] = (val0[1] + (val1[1] - val0[1]) * blendFactor) * scale;
        rgb[2] = (val0[2] + (val1[2] - val0[2]) * blendFactor) * scale;
    } else {

        float val[3];
        EvalCurve3(((char *)self + offsetof(Light, colorChannelInstance.curveIterator.master)), normTime, val);
        float scale = *(float *)(((char *)self + offsetof(Light, colorChannelInstance.curveIterator.master)) + 8);
        rgb[0] = val[0] * scale;
        rgb[1] = val[1] * scale;
        rgb[2] = val[2] * scale;
    }
}

static inline __attribute__((always_inline)) void ZN6EffectD1Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
}
void ZN6EffectD1Ev(void *_this)
{
    ZN6EffectD1Ev_impl(_this);
}

void FxBoltFramePtr_Archive(const FxBoltFramePtr *_this, FxArchive *arch)
{
    byte *self = (byte *)_this;
    byte *a = (byte *)arch;

    if ((((FxArchive *)(a))->isReading)) {

        int tmp;
        FxBoltInfo boltInfo;
        FxArchive_ReadData(arch, &tmp, 4);
        boltInfo.dobjHandle = tmp;
        if (tmp < 0) {

            FxBoltFrame_ReleaseHelper(*(byte **)self);
            *(byte **)self = NULL;
            return;
        }
        FxArchive_ReadData(arch, &tmp, 4);
        boltInfo.boneIndex = tmp;

        FxBoltFramePtr acquiredPtr = FxBoltFrame_Acquire(&boltInfo);
        byte *acquired = (byte *)acquiredPtr.value;

        byte *oldBf = *(byte **)self;
        if (oldBf != acquired) {
            if (oldBf)
                FxBoltFrame_ReleaseHelper(oldBf);
            *(byte **)self = NULL;
            if (acquired) {
                *(int *)acquired += 1;
                *(byte **)self = acquired;
            }
        }

        if (acquired)
            FxBoltFrame_ReleaseHelper(acquired);
    } else {

        int tmp;
        if (!*(byte **)self) {
            tmp = -1;
            FxArchive_WriteData(arch, &tmp, 4);
        } else {
            tmp = ((FxBoltFrame *)*(byte **)self)->mBolt.dobjHandle;
            FxArchive_WriteData(arch, &tmp, 4);
            tmp = ((FxBoltFrame *)*(byte **)self)->mBolt.boneIndex;
            FxArchive_WriteData(arch, &tmp, 4);
        }
    }
}

static inline __attribute__((always_inline)) void ZN6EffectD0Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
    if (self)
        __ZdaPv(self);
}
void ZN6EffectD0Ev(void *_this)
{
    ZN6EffectD0Ev_impl(_this);
}

void Effect_SetBoltFrame(const Effect *_this, FxBoltFramePtr *boltFrame)
{
    byte *self = (byte *)_this;
    byte **bfSlot = (byte **)(((char *)self + offsetof(Effect, mBolt.value)));
    byte *newBf = *(byte **)boltFrame;
    byte *oldBf = *bfSlot;

    if (oldBf == newBf)
        return;

    if (oldBf) {
        int rc = *(int *)oldBf - 1;
        *(int *)oldBf = rc;
        if (rc == 0) {

            byte **prevNext = &__ZN11FxBoltFrame12g_mFrameListE;
            byte *cur = *prevNext;
            while (cur) {
                if (cur == oldBf) {
                    *prevNext = (*(byte **)&((FxBoltFrame *)(oldBf))->next);
                    break;
                }
                prevNext = &(*(byte **)&((FxBoltFrame *)(cur))->next);
                cur = *prevNext;
            }
            if (oldBf)
                __ZdaPv(oldBf);
        }
        *bfSlot = NULL;
    }

    if (newBf) {
        *(int *)newBf += 1;
        *bfSlot = newBf;
    }
}

void Particle_UpdateRGB(const Particle *_this)
{
    byte *self = (byte *)_this;
    float normTime = ((Effect *)self)->normTime;
    float rgb[3];

    if (*(byte *)(self + 0xa9) & 0x20) {

        float blendFactor = (((Particle *)(self))->blendWeight[0]);
        float v0[3], v1[3];
        EvalCurve3(((char *)self + offsetof(Particle, colorChannelInstance.curveIterator.master)), normTime, v0);
        EvalCurve3(((char *)self + offsetof(Particle, colorRandChannelInstance.curveIterator.master)), normTime, v1);
        float scale = *(float *)(((char *)self + offsetof(Particle, colorChannelInstance.curveIterator.master)) + 8);
        rgb[0] = (v0[0] + (v1[0] - v0[0]) * blendFactor) * scale;
        rgb[1] = (v0[1] + (v1[1] - v0[1]) * blendFactor) * scale;
        rgb[2] = (v0[2] + (v1[2] - v0[2]) * blendFactor) * scale;
    } else {
        EvalCurve3(((char *)self + offsetof(Particle, colorChannelInstance.curveIterator.master)), normTime, rgb);
        float scale = *(float *)(((char *)self + offsetof(Particle, colorChannelInstance.curveIterator.master)) + 8);
        rgb[0] *= scale;
        rgb[1] *= scale;
        rgb[2] *= scale;
    }

    ((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))
    [0] = (byte)FloatToByte(rgb[0]);
    ((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))
    [1] = (byte)FloatToByte(rgb[1]);
    ((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))
    [2] = (byte)FloatToByte(rgb[2]);
}

Bool Flash_Update(const Flash *_this)
{
    byte *p = (byte *)_this;
    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    int startTime = *(int *)(p + 0xb8);
    int curTime = FXH_TIME(helper);
    int endTime;
    float normDuration;

    if (startTime > curTime)
        return 0;

    endTime = *(int *)(p + 0xbc);
    normDuration = (float)(curTime - startTime) / (float)(endTime - startTime);
    *(float *)(p + 0x3c) = normDuration;

    if (normDuration > 1.0f)
        *(float *)(p + 0x3c) = 1.0f;

    if (0.0f > *(float *)(p + 0x3c))
        *(float *)(p + 0x3c) = 0.0f;

    Light_UpdateRGB((const Light *)_this, (const Light *)_this);
    return 1;
}

extern float FxCurve_Integrate(void *curve, float normDuration);
static inline __attribute__((always_inline)) float IntegrateChannel(byte *self, int curveOff, int blendCurveOff, float blendFactor, int scaleOff, float normDuration, int useBlend)
{
    float val;
    if (useBlend) {
        float v0 = FxCurve_Integrate(*(void **)(self + curveOff), normDuration);
        float v1 = FxCurve_Integrate(*(void **)(self + blendCurveOff), normDuration);
        val = v0 + (v1 - v0) * blendFactor;
    } else {
        val = FxCurve_Integrate(*(void **)(self + curveOff), normDuration);
    }
    return val * *(float *)(self + scaleOff);
}
void Particle_IntegrateVelocity(const Particle *_this, float normDuration, vec_t *outVector)
{
    byte *self = (byte *)_this;
    int useBlend = (*(byte *)(self + 0xaa) & 8) != 0;
    float vx, vy, vz;

    if (useBlend) {
        vx = IntegrateChannel(self, 0x1bc, 0x1e0, (((Particle *)(self))->velocityWeightX), 0x1bc + 8, normDuration, 1);
        vy = IntegrateChannel(self, 0x1c8, 0x1ec, (((Particle *)(self))->velocityWeightY), 0x1c8 + 8, normDuration, 1);
        vz = IntegrateChannel(self, 0x1d4, 0x1f8, (((Particle *)(self))->velocityWeightZ), 0x1d4 + 8, normDuration, 1);
    } else {
        vx = IntegrateChannel(self, 0x1bc, 0, 0, 0x1bc + 8, normDuration, 0);
        vy = IntegrateChannel(self, 0x1c8, 0, 0, 0x1c8 + 8, normDuration, 0);
        vz = IntegrateChannel(self, 0x1d4, 0, 0, 0x1d4 + 8, normDuration, 0);
    }

    if (!(*(byte *)(self + 0xaa) & 0x20)) {
        AxisTransformVector(((char *)self + offsetof(Particle, displayAxis[0][0])), vx, vy, vz, outVector);
    } else {
        outVector[0] = vx;
        outVector[1] = vy;
        outVector[2] = vz;
    }

    float lifetimeSec = (float)((((Effect *)(self))->mTimeEnd) - (((Effect *)(self))->mTimeStart)) * 0.001f;
    outVector[0] *= lifetimeSec;
    outVector[1] *= lifetimeSec;
    outVector[2] *= lifetimeSec;
}

void Particle_IntegrateVelocity2(const Particle *_this, float normDuration, vec_t *outVector)
{
    byte *self = (byte *)_this;
    int useBlend = (*(byte *)(self + 0xaa) & 8) != 0;
    float vx, vy, vz;

    if (useBlend) {
        vx = IntegrateChannel(self, 0x204, 0x228, (((Particle *)(self))->velocity2WeightX), 0x204 + 8, normDuration, 1);
        vy = IntegrateChannel(self, 0x210, 0x234, (((Particle *)(self))->velocity2WeightY), 0x210 + 8, normDuration, 1);
        vz = IntegrateChannel(self, 0x21c, 0x240, (((Particle *)(self))->velocity2WeightZ), 0x21c + 8, normDuration, 1);
    } else {
        vx = IntegrateChannel(self, 0x204, 0, 0, 0x204 + 8, normDuration, 0);
        vy = IntegrateChannel(self, 0x210, 0, 0, 0x210 + 8, normDuration, 0);
        vz = IntegrateChannel(self, 0x21c, 0, 0, 0x21c + 8, normDuration, 0);
    }

    if (!(*(byte *)(self + 0xaa) & 0x20)) {
        AxisTransformVector(((char *)self + offsetof(Particle, displayAxis[0][0])), vx, vy, vz, outVector);
    } else {
        outVector[0] = vx;
        outVector[1] = vy;
        outVector[2] = vz;
    }

    float lifetimeSec = (float)((((Effect *)(self))->mTimeEnd) - (((Effect *)(self))->mTimeStart)) * 0.001f;
    outVector[0] *= lifetimeSec;
    outVector[1] *= lifetimeSec;
    outVector[2] *= lifetimeSec;
}

void Particle_IntegrateTotalVelocity(const Particle *_this, int duration, vec_t *outVector)
{
    byte *self = (byte *)_this;
    int lifetime = (((Effect *)(self))->mTimeEnd) - (((Effect *)(self))->mTimeStart);
    float durationF = (float)duration;
    float normDuration;
    if (lifetime > duration) {
        normDuration = durationF / (float)lifetime;
    } else {
        normDuration = 1.0f;
    }
    vec3_t velocitySum, velocity2Sum;
    Particle_IntegrateVelocity(_this, normDuration, velocitySum);
    Particle_IntegrateVelocity2(_this, normDuration, velocity2Sum);

    float gravityAccum = durationF * ((Particle *)self)->gravity * 0.001f;
    float scale = durationF * 0.001f;
    float gravX = scale * 0.0f;
    float gravY = scale * 0.0f;
    float gravZ = scale * gravityAccum;

    outVector[0] = velocitySum[0] + velocity2Sum[0] + gravX;
    outVector[1] = velocitySum[1] + velocity2Sum[1] + gravY;
    outVector[2] = velocitySum[2] + velocity2Sum[2] + gravZ;
}

void Particle_Particle(const Particle *_this)
{
    int *p;
    int zero = 0;
    ((struct Effect *)_this)->mBolt.value = 0;
    FX_SetPrimitiveVTable((void *)_this, PT_PARTICLE);
    p = (int *)((byte *)_this + 0xc4);
    p[0] = zero;
    p[1] = zero;
    p[2] = zero;
}

static inline __attribute__((always_inline)) void ZN8ParticleD1Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
}
void ZN8ParticleD1Ev(void *_this)
{
    ZN8ParticleD1Ev_impl(_this);
}

static inline __attribute__((always_inline)) void ZN8ParticleD0Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
    if (self)
        __ZdaPv(self);
}
void ZN8ParticleD0Ev(void *_this)
{
    ZN8ParticleD0Ev_impl(_this);
}

void Light_Light(const Light *_this)
{
    ((struct Effect *)_this)->mBolt.value = 0;
    FX_SetPrimitiveVTable((void *)_this, PT_LIGHT);
}

static inline __attribute__((always_inline)) void ZN5LightD1Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
}
void ZN5LightD1Ev(void *_this)
{
    ZN5LightD1Ev_impl(_this);
}

static inline __attribute__((always_inline)) void ZN5LightD0Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
    if (self)
        __ZdaPv(self);
}
void ZN5LightD0Ev(void *_this)
{
    ZN5LightD0Ev_impl(_this);
}

void Tail_InitEndPoint(const Tail *_this)
{
    byte *self = (byte *)_this;
    float normTime = ((Effect *)self)->normTime;

    float tailLen;
    if (*(byte *)(self + 0xaa) & 2) {
        float blendFactor = ((Tail *)self)->lengthBlendFactor;
        float v0 = EvalCurve1(((char *)self + offsetof(Tail, lengthChannelInstance.curveIterator.master)), normTime);
        float v1 = EvalCurve1(((char *)self + offsetof(Tail, lengthRandChannelInstance.curveIterator.master)), normTime);
        tailLen = (v0 + (v1 - v0) * blendFactor) * *(float *)(((char *)self + offsetof(Tail, lengthChannelInstance.curveIterator.master)) + 8);
    } else {
        tailLen = EvalCurve1(((char *)self + offsetof(Tail, lengthChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Tail, lengthChannelInstance.curveIterator.master)) + 8);
    }
    (((Tail *)(self))->tailLength) = tailLen;

    Tail_CalcNewEndpoint(_this, (const orientation_t *)NULL);
}

Bool Light_Update(const Light *_this)
{
    byte *self = (byte *)_this;
    int startTime = (((Effect *)(self))->mTimeStart);
    int curTime = FXH_TIME(*(FxHelper **)imp_theFxHelper);

    if (startTime > curTime)
        return 0;

    ((Effect *)self)->normTime =
        (float)(curTime - startTime) / (float)((((Effect *)(self))->mTimeEnd) - startTime);
    if (((Effect *)self)->normTime > 1.0f)
        ((Effect *)self)->normTime = 1.0f;
    if (0.0f > ((Effect *)self)->normTime)
        ((Effect *)self)->normTime = 0.0f;

    byte *boltFrame = (*(byte **)&((Effect *)(self))->mBolt.value);
    void *orient = NULL;
    if (boltFrame) {
        int boneIdx = (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle);
        if (boneIdx < 0)
            return 0;
        int clTime = (*(clientActive_t **)imp_cl)->skelTimeStamp;
        if ((((FxBoltFrame *)(boltFrame))->cachedServerTime) != clTime) {
            (((FxBoltFrame *)(boltFrame))->cachedServerTime) = clTime;
            if (!FX_GetBoneOrientation(((void *)&((FxBoltFrame *)(boltFrame))->mBolt), ((void *)&((FxBoltFrame *)(boltFrame))->orientation))) {
                (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) = -1;
                (((FxBoltFrame *)(boltFrame))->mBolt.boneIndex) = -1;
                return 0;
            }
        }
        orient = (&((FxBoltFrame *)(boltFrame))->orientation);
    }

    float normTime = ((Effect *)self)->normTime;
    float radius;
    if ((*(short *)&((Effect *)self)->mFlags) < 0) {
        float bf = ((Light *)self)->sizeBlendFactor;
        float v0 = EvalCurve1(((char *)self + offsetof(Light, sizeChannelInstance.curveIterator.master)), normTime);
        float v1 = EvalCurve1(((char *)self + offsetof(Light, sizeRandChannelInstance.curveIterator.master)), normTime);
        radius = (v0 + (v1 - v0) * bf) * *(float *)(((char *)self + offsetof(Light, sizeChannelInstance.curveIterator.master)) + 8);
    } else {
        radius = EvalCurve1(((char *)self + offsetof(Light, sizeChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Light, sizeChannelInstance.curveIterator.master)) + 8);
    }
    (((Effect *)(self))->mRefEnt.radius[0]) = radius;

    Light_UpdateRGB(_this, NULL);

    if (orient) {
        OrientationPosToWorldPos(orient, (vec_t *)(self + 4), (vec_t *)&((Light *)self)->base.mRefEnt.origin[0]);
    } else {
        (((Effect *)(self))->mRefEnt.origin[0]) = PART_ANCHOR_X(self);
        (((Effect *)(self))->mRefEnt.origin[1]) = PART_ANCHOR_Y(self);
        (((Effect *)(self))->mRefEnt.origin[2]) = PART_ANCHOR_Z(self);
    }
    return 1;
}

void Particle_CalcVelocityValue(const Particle *_this, float normTime, vec_t *outVector, const orientation_t *or_)
{
    byte *self = (byte *)_this;
    float vx, vy, vz;

    if (*(byte *)(self + 0xaa) & 8) {

        float bfx = (((Particle *)(self))->velocityWeightX);
        float v0x = EvalCurve1(((char *)self + offsetof(Particle, velocityXChannelInstance.curveIterator.master)), normTime);
        float v1x = EvalCurve1(((char *)self + offsetof(Particle, velocityXRandChannelInstance.curveIterator.master)), normTime);
        vx = (v0x + (v1x - v0x) * bfx) * *(float *)(((char *)self + offsetof(Particle, velocityXChannelInstance.curveIterator.master)) + 8);

        float bfy = (((Particle *)(self))->velocityWeightY);
        float v0y = EvalCurve1(((char *)self + offsetof(Particle, velocityYChannelInstance.curveIterator.master)), normTime);
        float v1y = EvalCurve1(((char *)self + offsetof(Particle, velocityYRandChannelInstance.curveIterator.master)), normTime);
        vy = (v0y + (v1y - v0y) * bfy) * *(float *)(((char *)self + offsetof(Particle, velocityYChannelInstance.curveIterator.master)) + 8);

        float bfz = (((Particle *)(self))->velocityWeightZ);
        float v0z = EvalCurve1(((char *)self + offsetof(Particle, velocityZChannelInstance.curveIterator.master)), normTime);
        float v1z = EvalCurve1(((char *)self + offsetof(Particle, velocityZRandChannelInstance.curveIterator.master)), normTime);
        vz = (v0z + (v1z - v0z) * bfz) * *(float *)(((char *)self + offsetof(Particle, velocityZChannelInstance.curveIterator.master)) + 8);
    } else {

        vx = EvalCurve1(((char *)self + offsetof(Particle, velocityXChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Particle, velocityXChannelInstance.curveIterator.master)) + 8);
        vy = EvalCurve1(((char *)self + offsetof(Particle, velocityYChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Particle, velocityYChannelInstance.curveIterator.master)) + 8);
        vz = EvalCurve1(((char *)self + offsetof(Particle, velocityZChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Particle, velocityZChannelInstance.curveIterator.master)) + 8);
    }

    if (!(*(byte *)(self + 0xaa) & 0x20)) {
        AxisTransformVector(((char *)self + offsetof(Particle, displayAxis[0][0])), vx, vy, vz, outVector);
    } else {
        outVector[0] = vx;
        outVector[1] = vy;
        outVector[2] = vz;
    }
}

void Particle_CalcVelocity2Value(const Particle *_this, float normTime, vec_t *outVector, const orientation_t *or_)
{
    (void)or_;
    byte *self = (byte *)_this;
    float vx, vy, vz;

    if (*(byte *)(self + 0xaa) & 8) {
        float bfx = (((Particle *)(self))->velocity2WeightX);
        float v0x = EvalCurve1(((char *)self + offsetof(Particle, velocity2XChannelInstance.curveIterator.master)), normTime);
        float v1x = EvalCurve1(((char *)self + offsetof(Particle, velocity2XRandChannelInstance.curveIterator.master)), normTime);
        vx = (v0x + (v1x - v0x) * bfx) * *(float *)(((char *)self + offsetof(Particle, velocity2XChannelInstance.curveIterator.master)) + 8);

        float bfy = (((Particle *)(self))->velocity2WeightY);
        float v0y = EvalCurve1(((char *)self + offsetof(Particle, velocity2YChannelInstance.curveIterator.master)), normTime);
        float v1y = EvalCurve1(((char *)self + offsetof(Particle, velocity2YRandChannelInstance.curveIterator.master)), normTime);
        vy = (v0y + (v1y - v0y) * bfy) * *(float *)(((char *)self + offsetof(Particle, velocity2YChannelInstance.curveIterator.master)) + 8);

        float bfz = (((Particle *)(self))->velocity2WeightZ);
        float v0z = EvalCurve1(((char *)self + offsetof(Particle, velocity2ZChannelInstance.curveIterator.master)), normTime);
        float v1z = EvalCurve1(((char *)self + offsetof(Particle, velocity2ZRandChannelInstance.curveIterator.master)), normTime);
        vz = (v0z + (v1z - v0z) * bfz) * *(float *)(((char *)self + offsetof(Particle, velocity2ZChannelInstance.curveIterator.master)) + 8);
    } else {
        vx = EvalCurve1(((char *)self + offsetof(Particle, velocity2XChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Particle, velocity2XChannelInstance.curveIterator.master)) + 8);
        vy = EvalCurve1(((char *)self + offsetof(Particle, velocity2YChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Particle, velocity2YChannelInstance.curveIterator.master)) + 8);
        vz = EvalCurve1(((char *)self + offsetof(Particle, velocity2ZChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Particle, velocity2ZChannelInstance.curveIterator.master)) + 8);
    }

    if (!(*(byte *)(self + 0xaa) & 0x20)) {
        AxisTransformVector(((char *)self + offsetof(Particle, displayAxis[0][0])), vx, vy, vz, outVector);
    } else {
        outVector[0] = vx;
        outVector[1] = vy;
        outVector[2] = vz;
    }
}

void Particle_GetTotalVelocity(const Particle *_this, float normTime, vec_t *outVector, const orientation_t *or_)
{
    byte *self = (byte *)_this;
    vec3_t velocityValue, velocity2Value, gravityValue;

    Particle_CalcVelocityValue(_this, normTime, velocityValue, or_);
    Particle_CalcVelocity2Value(_this, normTime, velocity2Value, or_);

    int lifetime = (((Effect *)(self))->mTimeEnd) - (((Effect *)(self))->mTimeStart);
    int duration = (int)((float)lifetime * normTime);
    float gravZ = (float)duration * ((Particle *)self)->gravity * 0.001f;
    vec3_t worldGrav;
    worldGrav[0] = 0.0f;
    worldGrav[1] = 0.0f;
    worldGrav[2] = gravZ;

    if (or_) {

        OrientationDirFromWorldDir((void *)or_, worldGrav, gravityValue);
    } else {
        gravityValue[0] = 0.0f;
        gravityValue[1] = 0.0f;
        gravityValue[2] = gravZ;
    }

    outVector[0] = velocityValue[0] + velocity2Value[0] + gravityValue[0];
    outVector[1] = velocityValue[1] + velocity2Value[1] + gravityValue[1];
    outVector[2] = velocityValue[2] + velocity2Value[2] + gravityValue[2];
    outVector[0] += (*(float *)&((Particle *)self)->subclassField0);
    outVector[1] += (*(float *)&((Particle *)self)->subclassField1);
    outVector[2] += (*(float *)&((Particle *)self)->subclassField2);
}

void Emitter_UpdateEmitFx(const Emitter *_this, vec_t *bindVelocity, const orientation_t *or_)
{
    Emitter *emitter = (Emitter *)_this;
    Effect *effect = (Effect *)_this;
    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    vec3_t start, delta;
    float distance, step, emittedDistance;
    int i;

    (void)bindVelocity;
    (void)or_;
    if (!(effect->mFlags & 0x100) || !emitter->emitFx ||
        helper->mFrameTime <= 0 || helper->mTime <= emitter->emitLastTime)
        return;
    emitter->emitLastTime = helper->mTime;

    /* Particle_UpdateOrigin already advanced the emitter for this frame.
       Sample that world-space path, retaining the un-emitted distance between
       frames. Reintegrating from the birth time here repeatedly added elapsed
       time to emitPos and sent impact dust trails far beyond the particle. */
    step = fabsf(emitter->emitStep);
    if (!isfinite(step) || step <= 0.0f)
        return;
    for (i = 0; i < 3; ++i) {
        start[i] = emitter->emitPos[i];
        delta[i] = effect->mRefEnt.origin[i] - start[i];
    }
    distance = sqrtf(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]);
    if (!isfinite(distance) || distance < step)
        return;

    for (emittedDistance = step; emittedDistance <= distance;) {
        vec3_t spawnPos;
        float fraction = emittedDistance / distance;
        float nextDistance;
        for (i = 0; i < 3; ++i)
            spawnPos[i] = start[i] + delta[i] * fraction;
        FxScheduler_PlayEffect(*(void **)imp_theFxScheduler, emitter->emitFx, spawnPos, NULL, NULL);
        for (i = 0; i < 3; ++i)
            emitter->emitPos[i] = spawnPos[i];

        nextDistance = emittedDistance + step;
        if (nextDistance <= emittedDistance)
            break;
        emittedDistance = nextDistance;
    }
}

extern void *imp_cl;
void Particle_GetTotalVelocityAtTime0(const Particle *_this, vec_t *outVector)
{
    byte *self = (byte *)_this;
    byte *boltFrame = (*(byte **)&((Effect *)(self))->mBolt.value);
    void *orient = NULL;

    if (boltFrame) {
        int boneIdx = (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle);
        if (boneIdx < 0) {
            outVector[0] = 0.0f;
            outVector[1] = 0.0f;
            outVector[2] = 0.0f;
            return;
        }
        int curTime = (*(clientActive_t **)imp_cl)->skelTimeStamp;
        if (curTime != (((FxBoltFrame *)(boltFrame))->cachedServerTime)) {
            (((FxBoltFrame *)(boltFrame))->cachedServerTime) = curTime;
            Bool ok = FX_GetBoneOrientation(((void *)&((FxBoltFrame *)(boltFrame))->mBolt), ((void *)&((FxBoltFrame *)(boltFrame))->orientation));
            if (!ok) {
                (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) = -1;
                (((FxBoltFrame *)(boltFrame))->mBolt.boneIndex) = -1;
                outVector[0] = 0.0f;
                outVector[1] = 0.0f;
                outVector[2] = 0.0f;
                return;
            }
        }
        orient = (&((FxBoltFrame *)(boltFrame))->orientation);
        if (!orient) {
            outVector[0] = 0.0f;
            outVector[1] = 0.0f;
            outVector[2] = 0.0f;
            return;
        }
    }
    Particle_GetTotalVelocity(_this, 0.0f, outVector, (const orientation_t *)orient);
}

void Particle_ApplyImpact(const Particle *_this, const orientation_t *or_, float normTime, const vec_t *velocity, float traceFraction, const vec_t *traceNormal)
{
    byte *self = (byte *)_this;

    if (traceNormal[2] > 0.0f) {
        float vLenSq = velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2];
        if (vLenSq >= 16.0f)
            goto do_reflect;
    }

    if (traceFraction == 0.0f) {
        int flags = ((Effect *)self)->mFlags;
        flags &= ~0x0800;
        flags &= ~0x0020;
        flags |= 0x01000000;
        ((Effect *)self)->mFlags = flags;
        return;
    }

do_reflect:;

    vec3_t normal;
    if (or_) {
        OrientationDirFromWorldDir((void *)or_, (vec_t *)traceNormal, normal);
    } else {
        normal[0] = traceNormal[0];
        normal[1] = traceNormal[1];
        normal[2] = traceNormal[2];
    }

    vec3_t reflectVelocity;
    Particle_GetTotalVelocity(_this, normTime, reflectVelocity, or_);

    float oldVx = reflectVelocity[0] - (*(float *)&((Particle *)self)->subclassField0);
    float oldVy = reflectVelocity[1] - (*(float *)&((Particle *)self)->subclassField1);
    float oldVz = reflectVelocity[2] - (*(float *)&((Particle *)self)->subclassField2);

    float bounceScale = (((Particle *)(self))->elasticity);
    reflectVelocity[0] *= bounceScale;
    reflectVelocity[1] *= bounceScale;
    reflectVelocity[2] *= bounceScale;

    (*(float *)&((Particle *)self)->subclassField0) = reflectVelocity[0] - oldVx;
    (*(float *)&((Particle *)self)->subclassField1) = reflectVelocity[1] - oldVy;
    (*(float *)&((Particle *)self)->subclassField2) = reflectVelocity[2] - oldVz;

    float dot = reflectVelocity[0] * normal[0] + reflectVelocity[1] * normal[1] + reflectVelocity[2] * normal[2];
    float reflScale = -2.0f * dot;
    (*(float *)&((Particle *)self)->subclassField0) += reflScale * normal[0];
    (*(float *)&((Particle *)self)->subclassField1) += reflScale * normal[1];
    (*(float *)&((Particle *)self)->subclassField2) += reflScale * normal[2];
}

extern void FxHelper_Trace(void *helper, void *trace, vec_t *start, vec_t *mins, vec_t *maxs, vec_t *end, int contents, int mask);
extern void *imp_vec3_origin;
Bool Particle_UpdateOrigin(const Particle *_this, const orientation_t *or_)
{
    byte *self = (byte *)_this;

    if (*(byte *)(self + 0xab) & 1)
        return 1;

    FxHelper *helper = *(FxHelper **)imp_theFxHelper;
    int frameTimeMs = helper->mFrameTime;
    if (frameTimeMs == 0)
        return 1;

    float ftime = (float)frameTimeMs * 0.001f;
    int startTime = (((Effect *)(self))->mTimeStart);
    float age = (float)(FXH_TIME(helper) - startTime) * 0.001f;
    float lifeTime = (float)((((Effect *)(self))->mTimeEnd) - startTime) * 0.001f;
    float normTime = age / lifeTime;
    if (normTime > 1.0f)
        normTime = 1.0f;

    vec3_t velocity;
    Particle_GetTotalVelocity(_this, normTime, velocity, or_);

    float *origin = (float *)(self + 4);
    vec3_t new_origin;
    new_origin[0] = origin[0] + velocity[0] * ftime;
    new_origin[1] = origin[1] + velocity[1] * ftime;
    new_origin[2] = origin[2] + velocity[2] * ftime;

    int flags = ((Effect *)self)->mFlags;
    if (flags & 0x20) {
        vec3_t start_pt, end_pt;

        if (or_) {
            OrientationPosToWorldPos((void *)or_, origin, start_pt);
            OrientationPosToWorldPos((void *)or_, new_origin, end_pt);
        } else {
            start_pt[0] = origin[0];
            start_pt[1] = origin[1];
            start_pt[2] = origin[2];
            end_pt[0] = new_origin[0];
            end_pt[1] = new_origin[1];
            end_pt[2] = new_origin[2];
        }

        byte trace[0x44];
        vec3_t *mins, *maxs;
        if (flags & 0x40) {
            mins = (vec3_t *)&((Particle *)self)->base.axis[0][0];
            maxs = (vec3_t *)&((Particle *)self)->base.axis[1][0];
        } else {
            mins = (vec3_t *)imp_vec3_origin;
            maxs = mins;
        }
        FxHelper_Trace(helper, trace, start_pt, (vec_t *)mins, (vec_t *)maxs, end_pt, -1, 1);

        float fraction = *(float *)trace;
        if (trace[0x23] == 0 && trace[0x22] == 0 && fraction != 1.0f) {

            if (flags & 0x800) {

                vec3_t endpos;
                endpos[0] = start_pt[0] + (end_pt[0] - start_pt[0]) * fraction;
                endpos[1] = start_pt[1] + (end_pt[1] - start_pt[1]) * fraction;
                endpos[2] = start_pt[2] + (end_pt[2] - start_pt[2]) * fraction;
                FX_PlayEffect((EffectTemplate *)((Effect *)self)->deathEffect, endpos, (vec_t *)(trace + 0x04));
            }

            if (flags & 0x400) {

                return 0;
            }

            float newFtime = ftime * fraction;
            float newAge = age + newFtime;
            float newNormTime = newAge / lifeTime;
            if (newNormTime > 1.0f)
                newNormTime = 1.0f;
            Particle_ApplyImpact(_this, or_, newNormTime, velocity, fraction, (vec_t *)(trace + 0x04));

            origin[0] = origin[0] + (new_origin[0] - origin[0]) * fraction;
            origin[1] = origin[1] + (new_origin[1] - origin[1]) * fraction;
            origin[2] = origin[2] + (new_origin[2] - origin[2]) * fraction;
            return 1;
        }
    }

    origin[0] = new_origin[0];
    origin[1] = new_origin[1];
    origin[2] = new_origin[2];
    return 1;
}

Bool Emitter_Update(const Emitter *_this)
{
    byte *self = (byte *)_this;
    int startTime = (((Effect *)(self))->mTimeStart);
    int curTime = FXH_TIME(*(FxHelper **)imp_theFxHelper);

    if (startTime > curTime)
        return 0;

    float normTime = (float)(curTime - startTime) / (float)((((Effect *)(self))->mTimeEnd) - startTime);
    ((Effect *)self)->normTime = normTime;
    if (normTime > 1.0f)
        ((Effect *)self)->normTime = 1.0f;
    if (0.0f > ((Effect *)self)->normTime)
        return 0;
    normTime = ((Effect *)self)->normTime;

    byte *boltFrame = (*(byte **)&((Effect *)(self))->mBolt.value);
    void *orient = NULL;
    if (boltFrame) {
        int boneIdx = (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle);
        if (boneIdx >= 0) {
            int clTime = (*(clientActive_t **)imp_cl)->skelTimeStamp;
            if ((((FxBoltFrame *)(boltFrame))->cachedServerTime) != clTime) {
                (((FxBoltFrame *)(boltFrame))->cachedServerTime) = clTime;
                if (!FX_GetBoneOrientation(((void *)&((FxBoltFrame *)(boltFrame))->mBolt), ((void *)&((FxBoltFrame *)(boltFrame))->orientation))) {
                    (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) = -1;
                    (((FxBoltFrame *)(boltFrame))->mBolt.boneIndex) = -1;
                }
            }
            if ((((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) >= 0)
                orient = (&((FxBoltFrame *)(boltFrame))->orientation);
        }
    }

    if (!Particle_UpdateOrigin((const Particle *)_this, (const orientation_t *)orient))
        return 0;

    vec3_t bindVelocity = { 0, 0, 0 };
    if (orient) {

        FxHelper *helper = *(FxHelper **)imp_theFxHelper;
        int frameTime = helper->mFrameTime;
        if (frameTime > 0) {
            float invFt = 1.0f / ((float)frameTime * 0.001f);
            vec3_t worldPos;
            OrientationPosToWorldPos(orient, (vec_t *)(self + 4), worldPos);
            bindVelocity[0] = (worldPos[0] - (((Effect *)(self))->mRefEnt.origin[0])) * invFt;
            bindVelocity[1] = (worldPos[1] - (((Effect *)(self))->mRefEnt.origin[1])) * invFt;
            bindVelocity[2] = (worldPos[2] - (((Effect *)(self))->mRefEnt.origin[2])) * invFt;
            (((Effect *)(self))->mRefEnt.origin[0]) = worldPos[0];
            (((Effect *)(self))->mRefEnt.origin[1]) = worldPos[1];
            (((Effect *)(self))->mRefEnt.origin[2]) = worldPos[2];
        }
    } else {
        (((Effect *)(self))->mRefEnt.origin[0]) = PART_ANCHOR_X(self);
        (((Effect *)(self))->mRefEnt.origin[1]) = PART_ANCHOR_Y(self);
        (((Effect *)(self))->mRefEnt.origin[2]) = PART_ANCHOR_Z(self);
    }

    Emitter_UpdateEmitFx(_this, bindVelocity, (const orientation_t *)orient);

    float radius;
    if ((*(short *)&((Effect *)self)->mFlags) < 0) {
        float bf = (((Particle *)(self))->blendWeight[2]);
        float v0 = EvalCurve1(((char *)self + offsetof(Emitter, _base[372])), normTime);
        float v1 = EvalCurve1(((char *)self + offsetof(Emitter, _base[384])), normTime);
        radius = (v0 + (v1 - v0) * bf) * *(float *)(((char *)self + offsetof(Emitter, _base[372])) + 8);
    } else {
        radius = EvalCurve1(((char *)self + offsetof(Emitter, _base[372])), normTime) * *(float *)(((char *)self + offsetof(Emitter, _base[372])) + 8);
    }
    (((Effect *)(self))->mRefEnt.radius[0]) = radius;

    if (radius == 0.0f) {
        ((Effect *)self)->mFlags |= 0x01000000;
        return 1;
    }

    Particle_UpdateRGB((const Particle *)_this);
    Particle_UpdateAlpha((const Particle *)_this);

    return 1;
}

void Particle_UpdateAlpha(const Particle *_this)
{
    byte *self = (byte *)_this;
    float normTime = ((Effect *)self)->normTime;
    float alpha;

    if (*(byte *)(self + 0xa9) & 0x40) {

        float blendFactor = (((Particle *)(self))->blendWeight[1]);
        float v0 = EvalCurve1(((char *)self + offsetof(Particle, alphaChannelInstance.curveIterator.master)), normTime);
        float v1 = EvalCurve1(((char *)self + offsetof(Particle, alphaRandChannelInstance.curveIterator.master)), normTime);
        alpha = v0 + (v1 - v0) * blendFactor;
    } else {
        alpha = EvalCurve1(((char *)self + offsetof(Particle, alphaChannelInstance.curveIterator.master)), normTime);
    }
    alpha *= *(float *)(((char *)self + offsetof(Particle, alphaChannelInstance.curveIterator.master)) + 8);

    if (alpha > 1.0f)
        alpha = 1.0f;
    if (alpha < 0.0f)
        alpha = 0.0f;

    if ((*(signed char *)&((Effect *)self)->mFlags) < 0) {

        ((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))
        [3] = (byte)(int)(alpha * 255.0f);
    } else {

        ((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))
        [0] = (byte)(int)((float)((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))[0] * alpha);
        ((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))
        [1] = (byte)(int)((float)((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))[1] * alpha);
        ((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))
        [2] = (byte)(int)((float)((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))[2] * alpha);
        ((byte *)((byte *)(self) + offsetof(Effect, mRefEnt.materialRGBA)))
        [3] = 0xff;
    }
}

Bool Cylinder_Update(const Cylinder *_this)
{

    byte *self = (byte *)_this;
    int startTime = (((Effect *)(self))->mTimeStart);
    int curTime = FXH_TIME(*(FxHelper **)imp_theFxHelper);

    if (startTime > curTime)
        return 0;

    float normTime = (float)(curTime - startTime) / (float)((((Effect *)(self))->mTimeEnd) - startTime);
    ((Effect *)self)->normTime = normTime;
    if (normTime > 1.0f)
        ((Effect *)self)->normTime = 1.0f;
    if (0.0f > ((Effect *)self)->normTime)
        return 0;
    normTime = ((Effect *)self)->normTime;

    byte *boltFrame = (*(byte **)&((Effect *)(self))->mBolt.value);
    void *orient = NULL;
    if (boltFrame) {
        int boneIdx = (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle);
        if (boneIdx >= 0) {
            int clTime = (*(clientActive_t **)imp_cl)->skelTimeStamp;
            if ((((FxBoltFrame *)(boltFrame))->cachedServerTime) != clTime) {
                (((FxBoltFrame *)(boltFrame))->cachedServerTime) = clTime;
                if (!FX_GetBoneOrientation(((void *)&((FxBoltFrame *)(boltFrame))->mBolt), ((void *)&((FxBoltFrame *)(boltFrame))->orientation))) {
                    (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) = -1;
                    (((FxBoltFrame *)(boltFrame))->mBolt.boneIndex) = -1;
                }
            }
            if ((((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) >= 0)
                orient = (&((FxBoltFrame *)(boltFrame))->orientation);
        }
    }

    if (!Particle_UpdateOrigin((const Particle *)_this, (const orientation_t *)orient))
        return 0;

    if (orient)
        OrientationPosToWorldPos(orient, (vec_t *)(self + 4), (vec_t *)(self + 0x7c));
    else {
        (((Effect *)(self))->mRefEnt.origin[0]) = PART_ANCHOR_X(self);
        (((Effect *)(self))->mRefEnt.origin[1]) = PART_ANCHOR_Y(self);
        (((Effect *)(self))->mRefEnt.origin[2]) = PART_ANCHOR_Z(self);
    }

    float tailLen;
    if (*(byte *)(self + 0xaa) & 2) {
        float bf = *(float *)(self + 0x25c);
        tailLen = (EvalCurve1(self + 0x260, normTime) + (EvalCurve1(self + 0x26c, normTime) - EvalCurve1(self + 0x260, normTime)) * bf) * *(float *)(self + 0x260 + 8);
    } else {
        tailLen = EvalCurve1(self + 0x260, normTime) * *(float *)(self + 0x260 + 8);
    }
    (((Tail *)(self))->tailLength) = tailLen;

    Tail_CalcNewEndpoint((const Tail *)_this, (const orientation_t *)orient);
    if (orient)
        OrientationPosToWorldPos(orient, (vec_t *)(self + 0x9c), (vec_t *)(self + 0x9c));

    float radius1, radius2;
    if ((*(short *)&((Effect *)self)->mFlags) < 0) {
        float bf = (((Particle *)(self))->blendWeight[2]);
        radius1 = (EvalCurve1(self + 0x174, normTime) + (EvalCurve1(self + 0x180, normTime) - EvalCurve1(self + 0x174, normTime)) * bf) * *(float *)(self + 0x174 + 8);

        if (*(byte *)(self + 0xaa) & 4) {
            float bf2 = (((Particle *)(self))->blendWeight[3]);
            radius2 = (EvalCurve1(self + 0x198, normTime) + (EvalCurve1(self + 0x1a4, normTime) - EvalCurve1(self + 0x198, normTime)) * bf2) * *(float *)(self + 0x198 + 8);
        } else {
            radius2 = EvalCurve1(self + 0x198, normTime) * *(float *)(self + 0x198 + 8);
        }
    } else {
        radius1 = EvalCurve1(self + 0x174, normTime) * *(float *)(self + 0x174 + 8);
        if (*(byte *)(self + 0xaa) & 4) {
            float bf2 = (((Particle *)(self))->blendWeight[3]);
            radius2 = (EvalCurve1(self + 0x198, normTime) + (EvalCurve1(self + 0x1a4, normTime) - EvalCurve1(self + 0x198, normTime)) * bf2) * *(float *)(self + 0x198 + 8);
        } else {
            radius2 = EvalCurve1(self + 0x198, normTime) * *(float *)(self + 0x198 + 8);
        }
    }
    (((Effect *)(self))->mRefEnt.radius[0]) = radius1;
    ((Effect *)self)->mRefEnt.radius[1] = radius2;

    if (radius1 == 0.0f && radius2 == 0.0f) {
        ((Effect *)self)->mFlags |= 0x01000000;
        return 1;
    }

    Particle_UpdateRGB((const Particle *)_this);
    Particle_UpdateAlpha((const Particle *)_this);
    return 1;
}

Bool Tail_Update(const Tail *_this)
{
    byte *self = (byte *)_this;
    int startTime = (((Effect *)(self))->mTimeStart);
    int curTime = FXH_TIME(*(FxHelper **)imp_theFxHelper);

    if (startTime > curTime)
        return 0;

    float normTime = (float)(curTime - startTime) / (float)((((Effect *)(self))->mTimeEnd) - startTime);
    ((Effect *)self)->normTime = normTime;
    if (normTime > 1.0f)
        ((Effect *)self)->normTime = 1.0f;
    if (0.0f > ((Effect *)self)->normTime)
        return 0;
    normTime = ((Effect *)self)->normTime;

    byte *boltFrame = (*(byte **)&((Effect *)(self))->mBolt.value);
    void *orient = NULL;
    if (boltFrame) {
        int boneIdx = (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle);
        if (boneIdx >= 0) {
            int clTime = (*(clientActive_t **)imp_cl)->skelTimeStamp;
            if ((((FxBoltFrame *)(boltFrame))->cachedServerTime) != clTime) {
                (((FxBoltFrame *)(boltFrame))->cachedServerTime) = clTime;
                if (!FX_GetBoneOrientation(((void *)&((FxBoltFrame *)(boltFrame))->mBolt), ((void *)&((FxBoltFrame *)(boltFrame))->orientation))) {
                    (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) = -1;
                    (((FxBoltFrame *)(boltFrame))->mBolt.boneIndex) = -1;
                }
            }
            if ((((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) >= 0)
                orient = (&((FxBoltFrame *)(boltFrame))->orientation);
        }
    }

    if (!Particle_UpdateOrigin((const Particle *)_this, (const orientation_t *)orient))
        return 0;

    if (orient)
        OrientationPosToWorldPos(orient, (vec_t *)(self + 4), (vec_t *)&((Tail *)self)->base.base.mRefEnt.origin[0]);
    else {
        (((Effect *)(self))->mRefEnt.origin[0]) = PART_ANCHOR_X(self);
        (((Effect *)(self))->mRefEnt.origin[1]) = PART_ANCHOR_Y(self);
        (((Effect *)(self))->mRefEnt.origin[2]) = PART_ANCHOR_Z(self);
    }

    float tailLen;
    if (*(byte *)(self + 0xaa) & 2) {
        float bf = ((Tail *)self)->lengthBlendFactor;
        float v0 = EvalCurve1(((char *)self + offsetof(Tail, lengthChannelInstance.curveIterator.master)), normTime);
        float v1 = EvalCurve1(((char *)self + offsetof(Tail, lengthRandChannelInstance.curveIterator.master)), normTime);
        tailLen = (v0 + (v1 - v0) * bf) * *(float *)(((char *)self + offsetof(Tail, lengthChannelInstance.curveIterator.master)) + 8);
    } else {
        tailLen = EvalCurve1(((char *)self + offsetof(Tail, lengthChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Tail, lengthChannelInstance.curveIterator.master)) + 8);
    }
    (((Tail *)(self))->tailLength) = tailLen;

    Tail_CalcNewEndpoint(_this, (const orientation_t *)orient);

    if (orient)
        OrientationPosToWorldPos(orient, (vec_t *)&((Tail *)self)->base.base.mRefEnt.endpos[0], (vec_t *)&((Tail *)self)->base.base.mRefEnt.endpos[0]);

    float radius;
    if ((*(short *)&((Effect *)self)->mFlags) < 0) {
        float bf = (((Particle *)(self))->blendWeight[2]);
        radius = (EvalCurve1(((char *)self + offsetof(Tail, base.sizeChannelInstance.curveIterator.master)), normTime) + (EvalCurve1(((char *)self + offsetof(Tail, base.sizeRandChannelInstance.curveIterator.master)), normTime) - EvalCurve1(((char *)self + offsetof(Tail, base.sizeChannelInstance.curveIterator.master)), normTime)) * bf) * *(float *)(((char *)self + offsetof(Tail, base.sizeChannelInstance.curveIterator.master)) + 8);
    } else {
        radius = EvalCurve1(((char *)self + offsetof(Tail, base.sizeChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Tail, base.sizeChannelInstance.curveIterator.master)) + 8);
    }
    (((Effect *)(self))->mRefEnt.radius[0]) = radius;
    ((Effect *)self)->mRefEnt.radius[1] = radius;

    if (radius == 0.0f) {
        ((Effect *)self)->mFlags |= 0x01000000;
        return 1;
    }

    Particle_UpdateRGB((const Particle *)_this);
    Particle_UpdateAlpha((const Particle *)_this);
    return 1;
}

extern void OrientationPosToWorldPos(void *orient, vec_t *localPos, vec_t *worldPos);
Bool Line_Update(const Line *_this)
{
    byte *self = (byte *)_this;
    int startTime = (((Effect *)(self))->mTimeStart);
    int curTime = FXH_TIME(*(FxHelper **)imp_theFxHelper);

    if (startTime > curTime)
        return 0;

    float normTime = (float)(curTime - startTime) / (float)((((Effect *)(self))->mTimeEnd) - startTime);
    ((Effect *)self)->normTime = normTime;
    if (normTime > 1.0f)
        ((Effect *)self)->normTime = 1.0f;
    if (0.0f > ((Effect *)self)->normTime)
        return 0;
    normTime = ((Effect *)self)->normTime;

    byte *boltFrame = (*(byte **)&((Effect *)(self))->mBolt.value);
    void *orient = NULL;
    if (boltFrame) {
        int boneIdx = (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle);
        if (boneIdx >= 0) {
            int clTime = (*(clientActive_t **)imp_cl)->skelTimeStamp;
            if ((((FxBoltFrame *)(boltFrame))->cachedServerTime) != clTime) {
                (((FxBoltFrame *)(boltFrame))->cachedServerTime) = clTime;
                if (!FX_GetBoneOrientation(((void *)&((FxBoltFrame *)(boltFrame))->mBolt), ((void *)&((FxBoltFrame *)(boltFrame))->orientation))) {
                    (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) = -1;
                    (((FxBoltFrame *)(boltFrame))->mBolt.boneIndex) = -1;
                }
            }
            if ((((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) >= 0)
                orient = (&((FxBoltFrame *)(boltFrame))->orientation);
        }
    }

    if (!Particle_UpdateOrigin((const Particle *)_this, (const orientation_t *)orient))
        return 0;

    if (orient)
        OrientationPosToWorldPos(orient, (vec_t *)(self + 4), (vec_t *)&((Line *)self)->_base[124]);
    else {
        (((Effect *)(self))->mRefEnt.origin[0]) = PART_ANCHOR_X(self);
        (((Effect *)(self))->mRefEnt.origin[1]) = PART_ANCHOR_Y(self);
        (((Effect *)(self))->mRefEnt.origin[2]) = PART_ANCHOR_Z(self);
    }

    if (orient)
        OrientationPosToWorldPos(orient, (vec_t *)(self + 0x24c), (vec_t *)&((Line *)self)->_base[156]);
    else {
        ((Effect *)self)->mRefEnt.endpos[0] = (((Tail *)(self))->endpoint[0]);
        ((Effect *)self)->mRefEnt.endpos[1] = (((Tail *)(self))->endpoint[1]);
        ((Effect *)self)->mRefEnt.endpos[2] = (((Tail *)(self))->endpoint[2]);
    }

    float radius;
    if ((*(short *)&((Effect *)self)->mFlags) < 0) {
        float bf = (((Particle *)(self))->blendWeight[2]);
        radius = (EvalCurve1(((char *)self + offsetof(Line, _base[372])), normTime) + (EvalCurve1(((char *)self + offsetof(Line, _base[384])), normTime) - EvalCurve1(((char *)self + offsetof(Line, _base[372])), normTime)) * bf) * *(float *)(((char *)self + offsetof(Line, _base[372])) + 8);
    } else {
        radius = EvalCurve1(((char *)self + offsetof(Line, _base[372])), normTime) * *(float *)(((char *)self + offsetof(Line, _base[372])) + 8);
    }
    (((Effect *)(self))->mRefEnt.radius[0]) = radius;
    ((Effect *)self)->mRefEnt.radius[1] = radius;

    if (radius == 0.0f) {
        ((Effect *)self)->mFlags |= 0x01000000;
        return 1;
    }

    Particle_UpdateRGB((const Particle *)_this);
    Particle_UpdateAlpha((const Particle *)_this);

    return 1;
}

Bool Cloud_Update(const Cloud *_this)
{

    byte *self = (byte *)_this;
    int startTime = (((Effect *)(self))->mTimeStart);
    int curTime = FXH_TIME(*(FxHelper **)imp_theFxHelper);

    if (startTime > curTime)
        return 0;

    float normTime = (float)(curTime - startTime) / (float)((((Effect *)(self))->mTimeEnd) - startTime);
    ((Effect *)self)->normTime = normTime;
    if (normTime > 1.0f)
        ((Effect *)self)->normTime = 1.0f;
    if (0.0f > ((Effect *)self)->normTime)
        return 0;
    normTime = ((Effect *)self)->normTime;

    byte *boltFrame = (*(byte **)&((Effect *)(self))->mBolt.value);
    void *orient = NULL;
    if (boltFrame) {
        int boneIdx = (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle);
        if (boneIdx >= 0) {
            int clTime = (*(clientActive_t **)imp_cl)->skelTimeStamp;
            if ((((FxBoltFrame *)(boltFrame))->cachedServerTime) != clTime) {
                (((FxBoltFrame *)(boltFrame))->cachedServerTime) = clTime;
                if (!FX_GetBoneOrientation(((void *)&((FxBoltFrame *)(boltFrame))->mBolt), ((void *)&((FxBoltFrame *)(boltFrame))->orientation))) {
                    (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) = -1;
                    (((FxBoltFrame *)(boltFrame))->mBolt.boneIndex) = -1;
                }
            }
            if ((((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) >= 0)
                orient = (&((FxBoltFrame *)(boltFrame))->orientation);
        }
    }

    if (!Particle_UpdateOrigin((const Particle *)_this, (const orientation_t *)orient))
        return 0;

    if (orient)
        OrientationPosToWorldPos(orient, (vec_t *)(self + 4), (vec_t *)&((Cloud *)self)->base.base.mRefEnt.origin[0]);
    else {
        (((Effect *)(self))->mRefEnt.origin[0]) = PART_ANCHOR_X(self);
        (((Effect *)(self))->mRefEnt.origin[1]) = PART_ANCHOR_Y(self);
        (((Effect *)(self))->mRefEnt.origin[2]) = PART_ANCHOR_Z(self);
    }

    float radius;
    if ((*(short *)&((Effect *)self)->mFlags) < 0) {
        float bf = (((Particle *)(self))->blendWeight[2]);
        radius = (EvalCurve1(((char *)self + offsetof(Cloud, base.sizeChannelInstance.curveIterator.master)), normTime) + (EvalCurve1(((char *)self + offsetof(Cloud, base.sizeRandChannelInstance.curveIterator.master)), normTime) - EvalCurve1(((char *)self + offsetof(Cloud, base.sizeChannelInstance.curveIterator.master)), normTime)) * bf) * *(float *)(((char *)self + offsetof(Cloud, base.sizeChannelInstance.curveIterator.master)) + 8);
    } else {
        radius = EvalCurve1(((char *)self + offsetof(Cloud, base.sizeChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Cloud, base.sizeChannelInstance.curveIterator.master)) + 8);
    }
    (((Effect *)(self))->mRefEnt.radius[0]) = radius;
    if (radius == 0.0f) {
        ((Effect *)self)->mFlags |= 0x01000000;
        return 1;
    }

    float height;
    if (*(byte *)(self + 0xaa) & 2) {
        float bf = (*(float *)&((Cloud *)self)->useLengthFlag);
        height = (EvalCurve1(((char *)self + offsetof(Cloud, heightChannel.curveIterator.master)), normTime) + (EvalCurve1(((char *)self + offsetof(Cloud, heightRandChannel.curveIterator.master)), normTime) - EvalCurve1(((char *)self + offsetof(Cloud, heightChannel.curveIterator.master)), normTime)) * bf) * *(float *)(((char *)self + offsetof(Cloud, heightChannel.curveIterator.master)) + 8);
    } else {
        height = EvalCurve1(((char *)self + offsetof(Cloud, heightChannel.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Cloud, heightChannel.curveIterator.master)) + 8);
    }
    ((Effect *)self)->mRefEnt.radius[1] = height;
    if (height == 0.0f) {
        ((Effect *)self)->mFlags |= 0x01000000;
        return 1;
    }

    if (*(byte *)(self + 0xaa) & 1) {
        float rot;
        if (*(byte *)(self + 0xaa) & 0x10) {
            float bf = (((Particle *)(self))->blendWeight[4]);
            rot = (EvalCurve1(((char *)self + offsetof(Cloud, base.size2ChannelInstance.curveIterator.master)), normTime) + (EvalCurve1(((char *)self + offsetof(Cloud, base.size2RandChannelInstance.curveIterator.master)), normTime) - EvalCurve1(((char *)self + offsetof(Cloud, base.size2ChannelInstance.curveIterator.master)), normTime)) * bf) * *(float *)(((char *)self + offsetof(Cloud, base.size2ChannelInstance.curveIterator.master)) + 8);
        } else {
            rot = EvalCurve1(((char *)self + offsetof(Cloud, base.size2ChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Cloud, base.size2ChannelInstance.curveIterator.master)) + 8);
        }
        (*(float *)&((Effect *)(self))->mRefEnt.materialSubimageIndex) = rot;
    }

    Particle_UpdateRGB((const Particle *)_this);
    Particle_UpdateAlpha((const Particle *)_this);
    return 1;
}

Bool OrientedParticle_Update(const OrientedParticle *_this)
{

    byte *self = (byte *)_this;
    int startTime = (((Effect *)(self))->mTimeStart);
    int curTime = FXH_TIME(*(FxHelper **)imp_theFxHelper);

    if (startTime > curTime)
        return 0;

    float normTime = (float)(curTime - startTime) / (float)((((Effect *)(self))->mTimeEnd) - startTime);
    ((Effect *)self)->normTime = normTime;
    if (normTime > 1.0f)
        ((Effect *)self)->normTime = 1.0f;
    if (0.0f > ((Effect *)self)->normTime)
        return 0;
    normTime = ((Effect *)self)->normTime;

    byte *boltFrame = (*(byte **)&((Effect *)(self))->mBolt.value);
    void *orient = NULL;
    if (boltFrame) {
        int boneIdx = (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle);
        if (boneIdx >= 0) {
            int clTime = (*(clientActive_t **)imp_cl)->skelTimeStamp;
            if ((((FxBoltFrame *)(boltFrame))->cachedServerTime) != clTime) {
                (((FxBoltFrame *)(boltFrame))->cachedServerTime) = clTime;
                if (!FX_GetBoneOrientation(((void *)&((FxBoltFrame *)(boltFrame))->mBolt), ((void *)&((FxBoltFrame *)(boltFrame))->orientation))) {
                    (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) = -1;
                    (((FxBoltFrame *)(boltFrame))->mBolt.boneIndex) = -1;
                }
            }
            if ((((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) >= 0)
                orient = (&((FxBoltFrame *)(boltFrame))->orientation);
        }
    }

    if (!Particle_UpdateOrigin((const Particle *)_this, (const orientation_t *)orient))
        return 0;

    if (orient) {
        OrientationPosToWorldPos(orient, (vec_t *)(self + 4), (vec_t *)&((OrientedParticle *)self)->_base[124]);

        OrientationDirFromWorldDir(orient, (vec_t *)(self + 0x24c), (vec_t *)&((OrientedParticle *)self)->_base[156]);
    } else {
        (((Effect *)(self))->mRefEnt.origin[0]) = PART_ANCHOR_X(self);
        (((Effect *)(self))->mRefEnt.origin[1]) = PART_ANCHOR_Y(self);
        (((Effect *)(self))->mRefEnt.origin[2]) = PART_ANCHOR_Z(self);
        ((Effect *)self)->mRefEnt.endpos[0] = (((Tail *)(self))->endpoint[0]);
        ((Effect *)self)->mRefEnt.endpos[1] = (((Tail *)(self))->endpoint[1]);
        ((Effect *)self)->mRefEnt.endpos[2] = (((Tail *)(self))->endpoint[2]);
    }

    float radius;
    if ((*(short *)&((Effect *)self)->mFlags) < 0) {
        float bf = (((Particle *)(self))->blendWeight[2]);
        radius = (EvalCurve1(((char *)self + offsetof(OrientedParticle, _base[372])), normTime) + (EvalCurve1(((char *)self + offsetof(OrientedParticle, _base[384])), normTime) - EvalCurve1(((char *)self + offsetof(OrientedParticle, _base[372])), normTime)) * bf) * *(float *)(((char *)self + offsetof(OrientedParticle, _base[372])) + 8);
    } else {
        radius = EvalCurve1(((char *)self + offsetof(OrientedParticle, _base[372])), normTime) * *(float *)(((char *)self + offsetof(OrientedParticle, _base[372])) + 8);
    }
    (((Effect *)(self))->mRefEnt.radius[0]) = radius;
    if (radius == 0.0f) {
        ((Effect *)self)->mFlags |= 0x01000000;
        return 1;
    }

    if ((*(byte *)&((OrientedParticle *)self)->_base[170]) & 1) {
        float rot;
        if ((*(byte *)&((OrientedParticle *)self)->_base[170]) & 0x10) {
            float bf = (((Particle *)(self))->blendWeight[4]);
            rot = (EvalCurve1(((char *)self + offsetof(OrientedParticle, _base[396])), normTime) + (EvalCurve1(((char *)self + offsetof(OrientedParticle, _base[408])), normTime) - EvalCurve1(((char *)self + offsetof(OrientedParticle, _base[396])), normTime)) * bf) * *(float *)(((char *)self + offsetof(OrientedParticle, _base[396])) + 8);
        } else {
            rot = EvalCurve1(((char *)self + offsetof(OrientedParticle, _base[396])), normTime) * *(float *)(((char *)self + offsetof(OrientedParticle, _base[396])) + 8);
        }
        ((Effect *)self)->mRefEnt.radius[1] = rot;
    }

    Particle_UpdateRGB((const Particle *)_this);
    Particle_UpdateAlpha((const Particle *)_this);
    return 1;
}

extern void OrientationPosToWorldPos(void *orient, vec_t *localPos, vec_t *worldPos);
Bool Particle_Update(const Particle *_this, const Particle *_this_1, const Cloud *_this_2)
{
    (void)_this_2;
    byte *self = (byte *)_this;
    int startTime = (((Effect *)(self))->mTimeStart);
    int curTime = FXH_TIME(*(FxHelper **)imp_theFxHelper);

    if (startTime > curTime)
        return 0;

    float normTime = (float)(curTime - startTime) / (float)((((Effect *)(self))->mTimeEnd) - startTime);
    ((Effect *)self)->normTime = normTime;
    if (normTime > 1.0f)
        ((Effect *)self)->normTime = 1.0f;
    if (0.0f > ((Effect *)self)->normTime)
        return 0;
    normTime = ((Effect *)self)->normTime;

    byte *boltFrame = (*(byte **)&((Effect *)(self))->mBolt.value);
    void *orient = NULL;
    if (boltFrame) {

        int boneIdx = (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle);
        if (boneIdx >= 0) {
            int cachedTime = (((FxBoltFrame *)(boltFrame))->cachedServerTime);
            int clTime = (*(clientActive_t **)imp_cl)->skelTimeStamp;
            if (cachedTime != clTime) {
                (((FxBoltFrame *)(boltFrame))->cachedServerTime) = clTime;
                Bool ok = FX_GetBoneOrientation(((void *)&((FxBoltFrame *)(boltFrame))->mBolt), ((void *)&((FxBoltFrame *)(boltFrame))->orientation));
                if (!ok) {
                    (((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) = -1;
                    (((FxBoltFrame *)(boltFrame))->mBolt.boneIndex) = -1;
                }
            }
            if ((((FxBoltFrame *)(boltFrame))->mBolt.dobjHandle) >= 0)
                orient = (&((FxBoltFrame *)(boltFrame))->orientation);
        }
    }

    if (!Particle_UpdateOrigin(_this, (const orientation_t *)orient))
        return 0;

    if (orient) {
        vec3_t worldPos;
        OrientationPosToWorldPos(orient, (vec_t *)(self + 4), worldPos);
        (((Effect *)(self))->mRefEnt.origin[0]) = worldPos[0];
        (((Effect *)(self))->mRefEnt.origin[1]) = worldPos[1];
        (((Effect *)(self))->mRefEnt.origin[2]) = worldPos[2];
    } else {
        (((Effect *)(self))->mRefEnt.origin[0]) = PART_ANCHOR_X(self);
        (((Effect *)(self))->mRefEnt.origin[1]) = PART_ANCHOR_Y(self);
        (((Effect *)(self))->mRefEnt.origin[2]) = PART_ANCHOR_Z(self);
    }

    float radius;
    if ((*(short *)&((Effect *)self)->mFlags) < 0) {

        float blendFactor = (((Particle *)(self))->blendWeight[2]);
        float v0 = EvalCurve1(((char *)self + offsetof(Particle, sizeChannelInstance.curveIterator.master)), normTime);
        float v1 = EvalCurve1(((char *)self + offsetof(Particle, sizeRandChannelInstance.curveIterator.master)), normTime);
        radius = (v0 + (v1 - v0) * blendFactor) * *(float *)(((char *)self + offsetof(Particle, sizeChannelInstance.curveIterator.master)) + 8);
    } else {
        radius = EvalCurve1(((char *)self + offsetof(Particle, sizeChannelInstance.curveIterator.master)), normTime) * *(float *)(((char *)self + offsetof(Particle, sizeChannelInstance.curveIterator.master)) + 8);
    }
    (((Effect *)(self))->mRefEnt.radius[0]) = radius;

    if (radius == 0.0f) {
        ((Effect *)self)->mFlags |= 0x01000000;
        return 1;
    }

    float secondRadius = radius;
    if (((Particle *)self)->nonUniformScale) {
        float v = EvalCurve1((char *)self + offsetof(Particle, size2ChannelInstance.curveIterator.master), normTime);
        if (((Effect *)self)->mFlags & 0x10000) {
            float alternate = EvalCurve1((char *)self + offsetof(Particle, size2RandChannelInstance.curveIterator.master), normTime);
            v += (alternate - v) * ((Particle *)self)->blendWeight[3];
        }
        secondRadius = v * ((Particle *)self)->size2ChannelInstance.scale;
    }
    ((Effect *)self)->mRefEnt.radius[1] = secondRadius;

    Particle_UpdateRGB(_this);
    Particle_UpdateAlpha(_this);

    return 1;
}

extern void FxArchive_ArchiveEffect(void *arch, void *effectPtr);
extern void FxArchive_ArchiveMaterial(void *arch, void *materialPtr);
extern void FxArchive_ArchiveModel(void *arch, void *modelPtr);
extern void FxArchive_ArchiveFxGfxEntity(void *arch, void *entityPtr);
extern void FX_SetSortGroup(void *effect);
extern int FX_GetCluster(const vec_t *origin);

static inline __attribute__((always_inline)) void ArchiveInt(void *arch, byte *self, int offset)
{
    byte *a = (byte *)arch;
    if ((((FxArchive *)(a))->isReading)) {
        int v;
        FxArchive_ReadData(arch, &v, 4);
        *(int *)(self + offset) = v;
    } else {
        int v = *(int *)(self + offset);
        FxArchive_WriteData(arch, &v, 4);
    }
}
static inline __attribute__((always_inline)) void ArchiveByte(void *arch, byte *self, int offset)
{
    byte *a = (byte *)arch;
    if ((((FxArchive *)(a))->isReading)) {
        byte v;
        FxArchive_ReadData(arch, &v, 1);
        *(byte *)(self + offset) = v;
    } else {
        byte v = *(byte *)(self + offset);
        FxArchive_WriteData(arch, &v, 1);
    }
}
static inline __attribute__((always_inline)) void ArchiveVec3(void *arch, byte *self, int offset)
{
    byte *a = (byte *)arch;
    if ((((FxArchive *)(a))->isReading)) {
        FxArchive_ReadData(arch, self + offset, 0xc);
    } else {
        float f;
        f = *(float *)(self + offset);
        FxArchive_WriteData(arch, &f, 4);
        f = *(float *)(self + offset + 4);
        FxArchive_WriteData(arch, &f, 4);
        f = *(float *)(self + offset + 8);
        FxArchive_WriteData(arch, &f, 4);
    }
}

void Effect_Archive(const Effect *_this, FxArchive *arch)
{
    byte *self = (byte *)_this;
    byte *a = (byte *)arch;

    ArchiveVec3(arch, self, 0x04);
    ArchiveInt(arch, self, 0xb8);
    ArchiveInt(arch, self, 0xbc);
    ArchiveInt(arch, self, 0xa8);
    ArchiveInt(arch, self, 0x10);
    ArchiveVec3(arch, self, 0x14);
    ArchiveVec3(arch, self, 0x20);
    FxArchive_ArchiveEffect(arch, ((char *)self + offsetof(Effect, deathEffect)));
    FxArchive_ArchiveEffect(arch, ((char *)self + offsetof(Effect, emitEffect)));
    FxArchive_ArchiveFxGfxEntity(arch, ((char *)self + offsetof(Effect, mRefEnt.customMaterial)));
    FxArchive_ArchiveEffect(arch, ((char *)self + offsetof(Effect, impactEffect)));
    ArchiveInt(arch, self, 0x38);
    FxArchive_ArchiveMaterial(arch, ((char *)self + offsetof(Effect, mRefEnt.customMaterial)));
    FxArchive_ArchiveModel(arch, ((char *)self + offsetof(Effect, mModelPtr)));
    FxBoltFramePtr_Archive((const FxBoltFramePtr *)&((Effect *)self)->mBolt.value, arch);

    if ((((FxArchive *)(a))->isReading)) {
        ((Effect *)self)->mClusterId = FX_GetCluster((vec_t *)&((Effect *)self)->mRefEnt.origin[0]);
        FX_SetSortGroup(self);
    }
}

void Light_Archive(const Light *_this, FxArchive *arch)
{
    byte *self = (byte *)_this;
    byte *a = (byte *)arch;
    Effect_Archive((const Effect *)_this, arch);
    FxArchive_ArchiveChannelInstance(arch, ((char *)self + offsetof(Light, colorChannelInstance.curveIterator.master)));
    FxArchive_ArchiveChannelInstance(arch, ((char *)self + offsetof(Light, colorRandChannelInstance.curveIterator.master)));
    FxArchive_ArchiveChannelInstance(arch, ((char *)self + offsetof(Light, sizeChannelInstance.curveIterator.master)));
    FxArchive_ArchiveChannelInstance(arch, ((char *)self + offsetof(Light, sizeRandChannelInstance.curveIterator.master)));

    if ((((FxArchive *)(a))->isReading)) {
        float f;
        FxArchive_ReadData(arch, &f, 4);
        ((Light *)self)->colorBlendFactor = f;
    } else {
        float f = ((Light *)self)->colorBlendFactor;
        FxArchive_WriteData(arch, &f, 4);
    }

    if ((((FxArchive *)(a))->isReading)) {
        float f;
        FxArchive_ReadData(arch, &f, 4);
        ((Light *)self)->sizeBlendFactor = f;
    } else {
        float f = ((Light *)self)->sizeBlendFactor;
        FxArchive_WriteData(arch, &f, 4);
    }
}

void Flash_Archive(const Flash *_this, FxArchive *arch)
{
    Light_Archive((const Light *)_this, arch);
}

void Particle_Archive(const Particle *_this, FxArchive *arch)
{
    byte *self = (byte *)_this;
    int i;
    Effect_Archive((const Effect *)_this, arch);

    ArchiveVec3(arch, self, 0xc4);
    ArchiveVec3(arch, self, 0xd0);
    ArchiveVec3(arch, self, 0xdc);
    ArchiveVec3(arch, self, 0xe8);

    ArchiveInt(arch, self, 0x100);
    ArchiveByte(arch, self, 0x104);

    for (i = 0; i < 22; i++)
        FxArchive_ArchiveChannelInstance(arch, ((char *)self + offsetof(Particle, colorChannelInstance.curveIterator.master)) + i * 0xc);

    for (i = 0; i < 9; i++)
        ArchiveInt(arch, self, 0x108 + i * 4);

    ArchiveVec3(arch, self, 0x12c);
    ArchiveVec3(arch, self, 0x138);
}

extern void FxArchive_ReadData(void *arch, void *data, int size);
extern void FxArchive_WriteData(void *arch, void *data, int size);
void OrientedParticle_Archive(const OrientedParticle *_this, FxArchive *arch)
{
    byte *self = (byte *)_this;
    byte *a = (byte *)arch;
    float *end_xyz;
    Particle_Archive((const Particle *)_this, arch);
    end_xyz = (float *)(self + 0x24c);
    if ((((FxArchive *)(a))->isReading)) {
        FxArchive_ReadData(arch, self + 0x24c, 0xc);
    } else {
        float f;
        f = end_xyz[0];
        FxArchive_WriteData(arch, &f, 4);
        f = end_xyz[1];
        FxArchive_WriteData(arch, &f, 4);
        f = end_xyz[2];
        FxArchive_WriteData(arch, &f, 4);
    }
}

void Cloud_Archive(const Cloud *_this, FxArchive *arch)
{
    byte *self = (byte *)_this;
    byte *a = (byte *)arch;
    Particle_Archive((const Particle *)_this, arch);

    if ((((FxArchive *)(a))->isReading)) {
        FxArchive_ReadData(arch, self + offsetof(Cloud, randomDirection), 0xc);
    } else {
        float f;
        f = (((Cloud *)(self))->randomDirection[0]);
        FxArchive_WriteData(arch, &f, 4);
        f = (((Cloud *)(self))->randomDirection[1]);
        FxArchive_WriteData(arch, &f, 4);
        f = (((Cloud *)(self))->randomDirection[2]);
        FxArchive_WriteData(arch, &f, 4);
    }

    FxArchive_ArchiveChannelInstance(arch, self + offsetof(Cloud, heightChannel));
    FxArchive_ArchiveChannelInstance(arch, self + offsetof(Cloud, heightRandChannel));

    if ((((FxArchive *)(a))->isReading)) {
        float f;
        FxArchive_ReadData(arch, &f, 4);
        (((Cloud *)(self))->randomLengthBlend) = f;
    } else {
        float f = (((Cloud *)(self))->randomLengthBlend);
        FxArchive_WriteData(arch, &f, 4);
    }

    if ((((FxArchive *)(a))->isReading)) {
        byte b;
        FxArchive_ReadData(arch, &b, 1);
        (((Cloud *)(self))->useLengthFlag) = b;
    } else {
        byte b = (((Cloud *)(self))->useLengthFlag);
        FxArchive_WriteData(arch, &b, 1);
    }
}

void Line_Archive(const Line *_this, FxArchive *arch)
{
    byte *self = (byte *)_this;
    byte *a = (byte *)arch;
    float *end_xyz;
    Particle_Archive((const Particle *)_this, arch);
    end_xyz = (float *)(self + 0x24c);
    if ((((FxArchive *)(a))->isReading)) {
        FxArchive_ReadData(arch, self + 0x24c, 0xc);
    } else {
        float f;
        f = end_xyz[0];
        FxArchive_WriteData(arch, &f, 4);
        f = end_xyz[1];
        FxArchive_WriteData(arch, &f, 4);
        f = end_xyz[2];
        FxArchive_WriteData(arch, &f, 4);
    }
}

extern void FxArchive_ArchiveChannelInstance(void *arch, void *channelInst);
void Tail_Archive(const Tail *_this, FxArchive *arch)
{
    byte *self = (byte *)_this;
    byte *a = (byte *)arch;
    Particle_Archive((const Particle *)_this, arch);

    if ((((FxArchive *)(a))->isReading)) {
        FxArchive_ReadData(arch, ((char *)self + offsetof(Tail, endpoint[0])), 0xc);
    } else {
        float f;
        f = (((Tail *)(self))->endpoint[0]);
        FxArchive_WriteData(arch, &f, 4);
        f = (((Tail *)(self))->endpoint[1]);
        FxArchive_WriteData(arch, &f, 4);
        f = (((Tail *)(self))->endpoint[2]);
        FxArchive_WriteData(arch, &f, 4);
    }

    FxArchive_ArchiveChannelInstance(arch, ((char *)self + offsetof(Tail, lengthChannelInstance.curveIterator.master)));
    FxArchive_ArchiveChannelInstance(arch, ((char *)self + offsetof(Tail, lengthRandChannelInstance.curveIterator.master)));

    if ((((FxArchive *)(a))->isReading)) {
        float f;
        FxArchive_ReadData(arch, &f, 4);
        ((Tail *)self)->lengthBlendFactor = f;
    } else {
        float f = ((Tail *)self)->lengthBlendFactor;
        FxArchive_WriteData(arch, &f, 4);
    }
}

void Cylinder_Archive(const Cylinder *_this, FxArchive *arch)
{
    Tail_Archive((const Tail *)_this, arch);
}

void Emitter_Archive(const Emitter *_this, FxArchive *arch)
{
    byte *self = (byte *)_this;
    Particle_Archive((const Particle *)_this, arch);
    ArchiveVec3(arch, self, 0x24c);
    ArchiveVec3(arch, self, 0x258);
    ArchiveVec3(arch, self, 0x264);
    ArchiveInt(arch, self, 0x270);
    ArchiveInt(arch, self, 0x274);
    ArchiveVec3(arch, self, 0x278);
    ArchiveVec3(arch, self, 0x284);
    FxArchive_ArchiveEffect(arch, ((char *)self + offsetof(Emitter, emitFx)));
    ArchiveInt(arch, self, 0x294);
    ArchiveInt(arch, self, 0x298);
    if ((*(int *)&((Effect *)self)->mModelPtr) == 0)
        ((Effect *)self)->mFlags &= ~0x10;
}

static inline __attribute__((always_inline)) void ZN8CylinderD0Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
    if (self)
        __ZdaPv(self);
}
void ZN8CylinderD0Ev(void *_this)
{
    ZN8CylinderD0Ev_impl(_this);
}

static inline __attribute__((always_inline)) void ZN8CylinderD1Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
}
void ZN8CylinderD1Ev(void *_this)
{
    ZN8CylinderD1Ev_impl(_this);
}

void Cylinder_Cylinder(const Cylinder *_this)
{
    extern void *__ZTV8Cylinder;
    Particle_Particle((const Particle *)_this);
    FX_SetPrimitiveVTable((void *)_this, PT_CYLINDER);
}

void OrientedParticle_OrientedParticle(const OrientedParticle *_this)
{
    extern void *__ZTV16OrientedParticle;
    Particle_Particle((const Particle *)_this);
    FX_SetPrimitiveVTable((void *)_this, PT_ORIENTEDPARTICLE);
}

static inline __attribute__((always_inline)) void ZN16OrientedParticleD1Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
}
void ZN16OrientedParticleD1Ev(void *_this)
{
    ZN16OrientedParticleD1Ev_impl(_this);
}

static inline __attribute__((always_inline)) void ZN16OrientedParticleD0Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
    if (self)
        __ZdaPv(self);
}
void ZN16OrientedParticleD0Ev(void *_this)
{
    ZN16OrientedParticleD0Ev_impl(_this);
}

extern float Vec3Normalize(float *v);
void Cloud_Cloud(Cloud *_this)
{
    extern void *__ZTV5Cloud;
    extern float flrand(float min, float max);
    byte *p = (byte *)_this;
    float *dir = (float *)&((Cloud *)p)->randomDirection[0];
    int attempts = 4;
    Particle_Particle((const Particle *)_this);
    FX_SetPrimitiveVTable((void *)p, PT_CLOUD);

    do {
        byte *q = p;
        int n;
        for (n = 3; n != 0; n--) {
            *(float *)(q + 0x24c) = flrand(-1.0f, 1.0f);
            q += 4;
        }
        float lenSq = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
        if (lenSq >= 0.01f && 1.0f >= lenSq) {
            Vec3Normalize(dir);
            return;
        }
    } while (--attempts != 0);

    dir[0] = 1.0f;
    dir[1] = 0.0f;
    dir[2] = 0.0f;
}

static inline __attribute__((always_inline)) void ZN5CloudD1Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
}
void ZN5CloudD1Ev(void *_this)
{
    ZN5CloudD1Ev_impl(_this);
}

static inline __attribute__((always_inline)) void ZN5CloudD0Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
    if (self)
        __ZdaPv(self);
}
void ZN5CloudD0Ev(void *_this)
{
    ZN5CloudD0Ev_impl(_this);
}

void Line_Line(const Line *_this)
{
    extern void *__ZTV4Line;
    Particle_Particle((const Particle *)_this);
    FX_SetPrimitiveVTable((void *)_this, PT_LINE);
}

static inline __attribute__((always_inline)) void ZN4LineD1Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
}
void ZN4LineD1Ev(void *_this)
{
    ZN4LineD1Ev_impl(_this);
}

static inline __attribute__((always_inline)) void ZN4LineD0Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
    if (self)
        __ZdaPv(self);
}
void ZN4LineD0Ev(void *_this)
{
    ZN4LineD0Ev_impl(_this);
}

void Tail_Tail(const Tail *_this)
{
    extern void *__ZTV4Tail;
    Particle_Particle((const Particle *)_this);
    FX_SetPrimitiveVTable((void *)_this, PT_TAIL);
}

static inline __attribute__((always_inline)) void ZN4TailD1Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
}
void ZN4TailD1Ev(void *_this)
{
    ZN4TailD1Ev_impl(_this);
}

static inline __attribute__((always_inline)) void ZN4TailD0Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
    if (self)
        __ZdaPv(self);
}
void ZN4TailD0Ev(void *_this)
{
    ZN4TailD0Ev_impl(_this);
}

void Emitter_Emitter(const Emitter *_this)
{
    extern void *__ZTV7Emitter;
    Particle_Particle((const Particle *)_this);
    FX_SetPrimitiveVTable((void *)_this, PT_EMITTER);
}

static inline __attribute__((always_inline)) void ZN7EmitterD1Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
}
void ZN7EmitterD1Ev(void *_this)
{
    ZN7EmitterD1Ev_impl(_this);
}

static inline __attribute__((always_inline)) void ZN7EmitterD0Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
    if (self)
        __ZdaPv(self);
}
void ZN7EmitterD0Ev(void *_this)
{
    ZN7EmitterD0Ev_impl(_this);
}

float Effect_GetVisibility(const Effect *_this, const vec_t *start, const vec_t *dir, float halfLen)
{
    return 1.0f;
}

void Effect_AddVisibility(const Effect *_this)
{
}

Bool Flash_Cull(const Flash *_this)
{
    return 0;
}

static inline __attribute__((always_inline)) void ZN5FlashD0Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
    if (self)
        __ZdaPv(self);
}
void ZN5FlashD0Ev(void *_this)
{
    ZN5FlashD0Ev_impl(_this);
}

static inline __attribute__((always_inline)) void ZN5FlashD1Ev_impl(void *_this)
{
    byte *self = (byte *)_this;
    *(void **)self = __ZTV6Effect + 8;
    FxBoltFrame_ReleaseHelper((*(byte **)&((Effect *)(self))->mBolt.value));
}
void ZN5FlashD1Ev(void *_this)
{
    ZN5FlashD1Ev_impl(_this);
}

// Reconstructed virtual dispatch must use linked functions, never retail
// executable addresses or the placeholder vtable blobs.
static Bool FX_ParticleUpdateVirtual(const Particle *particle)
{
    return Particle_Update(particle, particle, NULL);
}

void FX_SetPrimitiveVTable(void *effect, PrimType type)
{
#define FXVT(destruct, die, update, cull, draw, visibility, addvis, channels, id, archive, fixup) \
    { (void *)destruct, (void *)destruct, (void *)die, (void *)update, (void *)cull, (void *)draw, \
      (void *)visibility, (void *)addvis, (void *)channels, (void *)id, (void *)archive, (void *)fixup }
    static void *const particle[] = FXVT(ZN8ParticleD0Ev, Particle_Die, FX_ParticleUpdateVirtual, Particle_Cull, Particle_Draw, Particle_GetVisibility, Particle_AddVisibility, Particle_CreateChannelInstances, Particle_TypeID, Particle_Archive, Particle_FixupArchiveLoad);
    static void *const line[] = FXVT(ZN4LineD0Ev, Line_Die, Line_Update, Line_Cull, Line_Draw, Particle_GetVisibility, Particle_AddVisibility, Particle_CreateChannelInstances, Line_TypeID, Line_Archive, Particle_FixupArchiveLoad);
    static void *const tail[] = FXVT(ZN4TailD0Ev, Particle_Die, Tail_Update, Tail_Cull, Tail_Draw, Particle_GetVisibility, Particle_AddVisibility, Tail_CreateChannelInstances, Tail_TypeID, Tail_Archive, Tail_FixupArchiveLoad);
    static void *const cylinder[] = FXVT(ZN8CylinderD0Ev, Particle_Die, Cylinder_Update, Cylinder_Cull, Cylinder_Draw, Particle_GetVisibility, Particle_AddVisibility, Tail_CreateChannelInstances, Cylinder_TypeID, Cylinder_Archive, Particle_FixupArchiveLoad);
    static void *const emitter[] = FXVT(ZN7EmitterD0Ev, Particle_Die, Emitter_Update, Emitter_Cull, Emitter_Draw, Particle_GetVisibility, Particle_AddVisibility, Particle_CreateChannelInstances, Emitter_TypeID, Emitter_Archive, Particle_FixupArchiveLoad);
    static void *const oriented[] = FXVT(ZN16OrientedParticleD0Ev, Particle_Die, OrientedParticle_Update, OrientedParticle_Cull, OrientedParticle_Draw, Particle_GetVisibility, Particle_AddVisibility, Particle_CreateChannelInstances, OrientedParticle_TypeID, OrientedParticle_Archive, Particle_FixupArchiveLoad);
    static void *const light[] = FXVT(ZN5LightD0Ev, Effect_Die, Light_Update, Light_Cull, Light_Draw, Effect_GetVisibility, Effect_AddVisibility, Light_CreateChannelInstances, Light_TypeID, Light_Archive, Light_FixupArchiveLoad);
    static void *const flash[] = FXVT(ZN5FlashD0Ev, Effect_Die, Flash_Update, Flash_Cull, Flash_Draw, Effect_GetVisibility, Effect_AddVisibility, Light_CreateChannelInstances, Flash_TypeID, Flash_Archive, Light_FixupArchiveLoad);
    static void *const cloud[] = FXVT(ZN5CloudD0Ev, Particle_Die, Cloud_Update, Cloud_Cull, Cloud_Draw, Particle_GetVisibility, Particle_AddVisibility, Cloud_CreateChannelInstances, Cloud_TypeID, Cloud_Archive, Cloud_FixupArchiveLoad);
    static void *const *const tables[13] = { NULL, particle, line, tail, cylinder, emitter, NULL, oriented, NULL, light, NULL, flash, cloud };
    *(void *const **)effect = tables[type];
#undef FXVT
}
