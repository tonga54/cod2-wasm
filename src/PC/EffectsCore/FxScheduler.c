#include "common_types.h"
#include "imports.h"

extern int irand(int min, int max);
extern refexport_t re;

extern FxScheduler *fxSchedulers[1];
static EffectTemplate *effectTemplateArray[256];
static int effectTemplateArrayCount;

extern void GenericParser2_GenericParser2(GenericParser2 *parser);
extern void ZN14GenericParser2D1Ev(GenericParser2 *parser);
extern EffectTemplate *FX_ParseEffect(GenericParser2 *parser, const char *name);
extern void FX_Print(const char *msg, ...);
extern void FxBoltFrame_Release(const FxBoltFrame *frame);
extern const orientation_t *FxBoltFrame_GetOrientation(const FxBoltFrame *frame);
extern const FxBoltFramePtr FxBoltFrame_Acquire(const FxBoltInfo *bolt);
extern void AxisCopy(const vec_t *src, vec_t *dst);
extern float flrand(float min, float max);
extern void RotatePointAroundVector(vec_t *dst, const vec_t *src, const vec_t *dir, float degrees);
extern void Vec3Cross(const vec_t *a, const vec_t *b, vec_t *out);
extern int FxHelper_GetSeed(const FxHelper *helper);
extern void Rand_Init(int seed);
extern void FxHelper_SetIgnorePrecacheErrors(const FxHelper *helper, int ignore);
extern EffectTemplate *FX_RegisterEffect(const char *fileName);
extern Bool FX_GetBoneOrientation(const FxBoltInfo *bolt, orientation_t *orient);
extern Bool FxHelper_CullSpherePreviousFrame(const FxHelper *helper, const vec_t *worldPos, float radius);
extern float FxRange_GetVal(const FxRange *range);
extern float Vec3DistanceSq(const vec_t *a, const vec_t *b);
extern void FxChannelInstance_Create(const FxChannel *master, FxChannelInstance *createe);
extern void FX_CleanTemplate(EffectTemplate *fx);
extern void CG_ImpactMark(MaterialHandle markMaterial, const vec_t *origin, const vec_t *dir, float orientation, const vec_t *color, float radius);
extern void FxArchive_ReadData(const FxArchive *arch, void *p, int byteCount);
extern void FxArchive_WriteData(const FxArchive *arch, const void *p, int byteCount);
extern void FxArchive_ArchiveEffect(const FxArchive *arch, const EffectTemplate **fx);
extern void FxArchive_ArchiveFxBoltInfo(const FxArchive *arch, FxBoltInfo *bolt);

extern void FX_AddParticle(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
extern void FX_AddLine(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
extern void FX_AddTail(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
extern void FX_AddCylinder(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
extern void FX_AddEmitter(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
extern void FX_AddDecal(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
extern void FX_AddOrientedParticle(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
extern void FX_AddFxRunner(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
extern void FX_AddLight(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
extern void FX_AddCameraShake(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
extern void FX_AddFlash(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);
extern void FX_AddCloud(EffectPrimitive *prim, vec3_t *ax, const vec_t *origin, const int lateTime, const int indexInBatch);

extern void *__Znam(unsigned int size);
extern void __ZdaPv(void *ptr);

void *MediaHandles_GetHandle(const MediaHandles *_this);
void FxScheduler_FxScheduler(const FxScheduler *_this);
float FxScheduler_GetEffectLength(const FxScheduler *_this, EffectTemplate *fx);
void FX_InitTemplates(void);
EffectTemplate *FX_TryRegisterEffect(const char *name);
static void GLOBAL__I__ZN17PrimitiveTemplate8gmMemMgrE(void);
Bool FX_GetBoltingFrame(const PrimitiveTemplate *primTemp, const FxBoltInfo *bolt, FxBoltFramePtr *boltFrame);
void FxScheduler_CreateEffect(const FxScheduler *_this, const EffectTemplate *fx, const PrimitiveTemplate *primTemp, const FxBoltInfo *bolt, const vec_t *origin, MediaHandles *(*axis)[4], int lateTime, int indexInBatch);
void FxScheduler_PlayEffect(const FxScheduler *_this, const EffectTemplate *fx, const vec_t *origin, MediaHandles *(*axis)[4], const FxBoltInfo *bolt);
void FxScheduler_Clean(const FxScheduler *_this, unsigned char bRemoveTemplates, EffectTemplate *fxToPreserve);
void FxScheduler_GetDecalColor(const FxScheduler *_this, const PrimitiveTemplate *primTemp, vec_t *rgba);
float FxScheduler_GetDecalAlpha(const FxScheduler *_this, const PrimitiveTemplate *primTemp);
float FxScheduler_GetDecalSize(const FxScheduler *_this, const PrimitiveTemplate *primTemp);
EffectTemplate *MediaHandles_GetEffect(const MediaHandles *_this);
void FxScheduler_CreateDecalEffect(const FxScheduler *_this, const PrimitiveTemplate *primTemp, vec_t *org, MediaHandles *(*ax)[4]);
void ScheduledEffect_Archive(const ScheduledEffect *_this, FxArchive *arch);
void FxScheduler_Archive(const FxScheduler *_this, FxArchive *arch);

void *MediaHandles_GetHandle(const MediaHandles *_this)
{
    unsigned short count = _this->mMediaList.size;
    TMediaElement *elements;

    if (!count) {
        return NULL;
    }

    elements = _this->mMediaList.elements;
    return elements[irand(0, count)].data;
}

void FxScheduler_FxScheduler(const FxScheduler *_this)
{
    FxScheduler *sched = (FxScheduler *)_this;
    sched->mScheduledHead = NULL;
    sched->mScheduledCount = 0;
}

float FxScheduler_GetEffectLength(const FxScheduler *_this, EffectTemplate *fx)
{
    float maxLen = 0.0f;
    int i;

    for (i = 0; i < fx->mPrimitiveCount; i++) {
        PrimitiveTemplate *prim = fx->mPrimitives[i];
        float len = prim->mSpawnDelay.mMax + prim->mLife.mMax;
        if (len > maxLen)
            maxLen = len;
    }

    return maxLen;
}

void FX_InitTemplates(void)
{
    effectTemplateArrayCount = 0;
}

EffectTemplate *FX_TryRegisterEffect(const char *name)
{
    GenericParser2 parser;
    int effectIndex;
    int count;
    EffectTemplate *registeredTemplate;
    EffectTemplate *fx;

    GenericParser2_GenericParser2(&parser);

    count = effectTemplateArrayCount;
    for (effectIndex = 0; effectIndex < count; effectIndex++) {
        registeredTemplate = effectTemplateArray[effectIndex];
        if (strcmp(*(const char **)registeredTemplate, name) == 0) {

            ZN14GenericParser2D1Ev(&parser);
            return registeredTemplate;
        }
    }

    fx = FX_ParseEffect(&parser, name);
    if (!fx) {
        ZN14GenericParser2D1Ev(&parser);
        return NULL;
    }

    if (effectTemplateArrayCount == 256) {
        FX_Print("^1Max effect templates of '%i' exceeded\n", 256);
        ZN14GenericParser2D1Ev(&parser);
        return NULL;
    }

    effectTemplateArray[effectTemplateArrayCount] = fx;
    effectTemplateArrayCount++;
    registeredTemplate = fx;

    ZN14GenericParser2D1Ev(&parser);
    return registeredTemplate;
}

static void GLOBAL__I__ZN17PrimitiveTemplate8gmMemMgrE(void)
{
}

Bool FX_GetBoltingFrame(const PrimitiveTemplate *primTemp, const FxBoltInfo *bolt, FxBoltFramePtr *boltFrame)
{
    FxBoltFramePtr acquired;
    FxBoltFrame *oldFrame;
    FxBoltFrame *newFrame;
    FxBoltFrame *curFrame;

    if (!(primTemp->mAttributeFlags & 2) || !bolt || bolt->dobjHandle < 0) {
        return 1;
    }

    acquired = FxBoltFrame_Acquire(bolt);
    newFrame = (FxBoltFrame *)(void *)(size_t)acquired._placeholder;
    oldFrame = (FxBoltFrame *)(void *)(size_t)boltFrame->_placeholder;

    if (oldFrame != newFrame) {

        if (oldFrame) {
            FxBoltFrame_Release(oldFrame);
            boltFrame->_placeholder = 0;
        }
        if (newFrame) {

            newFrame->refCount += 1;
            boltFrame->_placeholder = (int)(size_t)(void *)newFrame;
        }
    }

    if (newFrame) {
        FxBoltFrame_Release(newFrame);
    }

    curFrame = (FxBoltFrame *)(void *)(size_t)boltFrame->_placeholder;
    if (!curFrame) {
        return 0;
    }
    if (!FxBoltFrame_GetOrientation(curFrame)) {
        return 0;
    }

    return 1;
}

void FxScheduler_CreateEffect(const FxScheduler *_this, const EffectTemplate *fx, const PrimitiveTemplate *primTemp, const FxBoltInfo *bolt, const vec_t *origin, MediaHandles *(*axis)[4], int lateTime, int indexInBatch)
{
    FxBoltFramePtr boltFrame;
    vec3_t ax[3];
    EffectPrimitive prim;
    int primType;

    boltFrame._placeholder = 0;

    AxisCopy((const vec_t *)axis, (vec_t *)ax);

    if (primTemp->mSpawnFlags & 0x100) {
        vec3_t rotated;
        float angle = flrand(0.0f, 360.0f);
        RotatePointAroundVector(rotated, (const vec_t *)ax, (const vec_t *)axis + 3, angle);
        Vec3Cross((const vec_t *)ax, rotated, ax[1]);
    }

    if (!FX_GetBoltingFrame(primTemp, bolt, &boltFrame)) {
        goto cleanup;
    }

    memset(&prim, 0, sizeof(prim));
    prim.fx = fx;
    prim.primTemp = primTemp;
    prim.boltFrame = boltFrame;

    primType = primTemp->mType;
    if (primType > 12) {
        goto cleanup;
    }

    switch (primType) {
    case 0:
        break;
    case 1:
        FX_AddParticle(&prim, ax, origin, lateTime, indexInBatch);
        break;
    case 2:
        FX_AddLine(&prim, ax, origin, lateTime, indexInBatch);
        break;
    case 3:
        FX_AddTail(&prim, ax, origin, lateTime, indexInBatch);
        break;
    case 4:
        FX_AddCylinder(&prim, ax, origin, lateTime, indexInBatch);
        break;
    case 5:
        FX_AddEmitter(&prim, ax, origin, lateTime, indexInBatch);
        break;
    case 6:
        FX_AddDecal(&prim, ax, origin, lateTime, indexInBatch);
        break;
    case 7:
        FX_AddOrientedParticle(&prim, ax, origin, lateTime, indexInBatch);
        break;
    case 8:
        FX_AddFxRunner(&prim, ax, origin, lateTime, indexInBatch);
        break;
    case 9:
        FX_AddLight(&prim, ax, origin, lateTime, indexInBatch);
        break;
    case 10:
        FX_AddCameraShake(&prim, ax, origin, lateTime, indexInBatch);
        break;
    case 11:
        FX_AddFlash(&prim, ax, origin, lateTime, indexInBatch);
        break;
    case 12:
        FX_AddCloud(&prim, ax, origin, lateTime, indexInBatch);
        break;
    }

cleanup:
    if (boltFrame._placeholder) {
        FxBoltFrame_Release((FxBoltFrame *)(void *)(size_t)boltFrame._placeholder);
    }
}

void FxScheduler_PlayEffect(const FxScheduler *_this, const EffectTemplate *fx, const vec_t *origin, MediaHandles *(*axis)[4], const FxBoltInfo *bolt)
{
    FxHelper *helper;
    int seed;
    orientation_t or_;
    vec3_t ax[3];
    int i;
    float factor;
    int numAdded;
    int seedOffset;
    PrimitiveTemplate *prim;
    float distSq;
    int rangeCheck;
    float culldist;

    helper = *(FxHelper **)imp_theFxHelper;

    seed = FxHelper_GetSeed(helper);
    ((FxScheduler *)_this)->mSeed = seed;
    Rand_Init(seed);

    if (!fx) {
        FxHelper_SetIgnorePrecacheErrors(helper, 1);
        fx = FX_RegisterEffect("fx/error.efx");
        FxHelper_SetIgnorePrecacheErrors(helper, 0);
        if (!fx) {
            FX_Print("FxScheduler::PlayEffect called with invalid effect\n");
            return;
        }
    }

    {
        dvar_t *freezeDvar = *(dvar_t **)imp_fx_freeze;
        dvar_t *enableDvar = *(dvar_t **)imp_fx_enable;
        if (freezeDvar->current.enabled != 0)
            return;
        if (enableDvar->current.enabled == 0)
            return;
    }

    if (bolt) {

        if (bolt->dobjHandle < 0)
            return;
        if (!FX_GetBoneOrientation(bolt, &or_))
            return;
        AxisCopy((const vec_t *)or_.axis, (vec_t *)ax);
    } else {

        if (origin) {
            or_.origin[0] = origin[0];
            or_.origin[1] = origin[1];
            or_.origin[2] = origin[2];
        } else {
            or_.origin[0] = 0.0f;
            or_.origin[1] = 0.0f;
            or_.origin[2] = 0.0f;
        }
        if (axis)
            AxisCopy((const vec_t *)axis, (vec_t *)ax);
        else {
            memset(ax, 0, sizeof(ax));
            ax[0][0] = ax[1][1] = ax[2][2] = 1.0f;
        }
    }

    numAdded = 0;
    seedOffset = 0;

    for (i = 0; i < fx->mPrimitiveCount; i++) {
        int count;
        int t;
        float spawnDistSq;
        float cullDistSq;
        int flags;

        prim = fx->mPrimitives[i];
        factor = 0.0f;

        if (prim->mSpawnRange.mMin != 0.0f) {

            distSq = Vec3DistanceSq(or_.origin, helper->mCamera.vieworg);
            culldist = prim->mSpawnRange.mMin * helper->adsZoomFactor;
            if (distSq < culldist * culldist) {
                continue;
            }
            rangeCheck = 1;
        } else {
            rangeCheck = 0;
        }

        if (prim->mSpawnRange.mMax != 0.0f) {
            if (!rangeCheck) {
                distSq = Vec3DistanceSq(or_.origin, helper->mCamera.vieworg);
            }
            culldist = prim->mSpawnRange.mMax * helper->adsZoomFactor;
            if (distSq > culldist * culldist) {
                continue;
            }
        }

        if (prim->mSpawnFlags & 0x400) {
            if (FxHelper_CullSpherePreviousFrame(helper, or_.origin, prim->spawnFrustumCullRadius)) {
                continue;
            }
        }

        {
            float fcount = FxRange_GetVal(&prim->mSpawnCount);
            count = (int)(fcount + 0.5f);
        }
        if (count == 0)
            continue;

        flags = prim->mSpawnFlags;
        if (flags & 0x200) {
            float start = prim->mSpawnDelay.mMin;
            float end = prim->mSpawnDelay.mMax;
            factor = end - start;
            if (factor < 0.0f)
                factor = -factor;
            factor = factor / (float)count;
        }

        numAdded += count;

        for (t = 0; t < count; t++) {
            int delay;

            flags = prim->mSpawnFlags;
            if (flags & 0x200) {

                delay = (int)((float)t * factor);
            } else {

                float fdelay = FxRange_GetVal(&prim->mSpawnDelay);
                delay = (int)fdelay;
            }

            if (delay > 0) {

                ScheduledEffect *sfx = (ScheduledEffect *)__Znam(0x50);
                if (!sfx)
                    continue;
                memset(sfx, 0, sizeof(ScheduledEffect));

                sfx->mStartTime = helper->mTime + delay;

                sfx->mFx = fx;

                sfx->mPrimIndex = i;

                sfx->mIndexInBatch = t;

                sfx->mSeed = FxHelper_GetSeed(helper) + seedOffset;

                if (bolt) {
                    sfx->mBolt.dobjHandle = bolt->dobjHandle;
                    sfx->mBolt.boneIndex = bolt->boneIndex;
                } else {
                    sfx->mBolt.dobjHandle = -1;
                    sfx->mBolt.boneIndex = -1;
                }

                sfx->mOrigin[0] = or_.origin[0];
                sfx->mOrigin[1] = or_.origin[1];
                sfx->mOrigin[2] = or_.origin[2];

                AxisCopy((const vec_t *)ax, (vec_t *)sfx->mAxis);

                sfx->mScheduledNext = (int)(uintptr_t)((FxScheduler *)_this)->mScheduledHead;
                ((FxScheduler *)_this)->mScheduledHead = sfx;
                ((FxScheduler *)_this)->mScheduledCount += 1;
            } else {

                FxScheduler_CreateEffect(_this, fx, prim, bolt, (const vec_t *)or_.origin, (MediaHandles * (*)[4]) ax, -delay, t);
            }
        }

        seedOffset += 0x369d035;
    }

    if (numAdded) {
        dvar_t *countDvar = *(dvar_t **)imp_fx_count;
        if (countDvar->current.enabled != 0) {
            re.AddPlume(or_.origin, numAdded, *(const vec_t **)&imp_colorYellow, 3000);
        }
    }
}

void FxScheduler_Clean(const FxScheduler *_this, unsigned char bRemoveTemplates, EffectTemplate *fxToPreserve)
{
    byte *self = (byte *)_this;
    ScheduledEffect *sfx;
    int effectIndex;
    int foundTemplateToPreserve;

    while ((sfx = ((FxScheduler *)_this)->mScheduledHead) != NULL) {
        ((FxScheduler *)_this)->mScheduledHead = (ScheduledEffect *)(size_t)sfx->mScheduledNext;
        __ZdaPv(sfx);
    }
    ((FxScheduler *)_this)->mScheduledCount = 0;

    if (!bRemoveTemplates)
        return;

    foundTemplateToPreserve = 0;
    for (effectIndex = 0; effectIndex < effectTemplateArrayCount; effectIndex++) {
        if (effectTemplateArray[effectIndex] == fxToPreserve) {
            foundTemplateToPreserve = 1;
        } else {
            FX_CleanTemplate(effectTemplateArray[effectIndex]);
            effectTemplateArray[effectIndex] = NULL;
        }
    }

    effectTemplateArrayCount = 0;
    if (foundTemplateToPreserve) {
        effectTemplateArray[0] = fxToPreserve;
        effectTemplateArrayCount = 1;
    }
}

static inline __attribute__((always_inline)) void FxCurveIterator_FindKey(const FxCurve *curve, int *currentKeyIndex, float t)
{
    int keyStride = curve->dimensionCount + 1;
    const float *keys = curve->keys;
    const float *lastKeyTime;

    lastKeyTime = &keys[(*currentKeyIndex) * keyStride];

    if (*lastKeyTime > 0.0f) {
        *currentKeyIndex = 0;
        lastKeyTime = &keys[0];
    }

    {
        const float *endCheck = lastKeyTime + keyStride;
        while (t > *endCheck) {
            (*currentKeyIndex)++;
            endCheck += keyStride;
        }
    }
}

static inline __attribute__((always_inline)) void FxCurve_Interp3(const FxCurve *curve, int keyIndex, float t, float *out)
{
    int stride = curve->dimensionCount + 1;
    const float *keyBase = &curve->keys[keyIndex * stride];
    float keyTime = keyBase[0];
    float nextKeyTime = keyBase[stride];
    float frac;

    frac = (t - keyTime) / (nextKeyTime - keyTime);

    out[0] = keyBase[1] + (keyBase[stride + 1] - keyBase[1]) * frac;
    out[1] = keyBase[2] + (keyBase[stride + 2] - keyBase[2]) * frac;
    out[2] = keyBase[3] + (keyBase[stride + 3] - keyBase[3]) * frac;
}

static inline __attribute__((always_inline)) float FxCurve_Interp1(const FxCurve *curve, int keyIndex, float t)
{

    const float *keyBase = &curve->keys[keyIndex * 2];
    float keyTime = keyBase[0];
    float keyVal = keyBase[1];
    float nextKeyTime = keyBase[2];
    float nextKeyVal = keyBase[3];

    return keyVal + (t - keyTime) * (nextKeyVal - keyVal) / (nextKeyTime - keyTime);
}

void FxScheduler_GetDecalColor(const FxScheduler *_this, const PrimitiveTemplate *primTemp, vec_t *rgba)
{
    FxChannelInstance colorChannelInstance;
    const FxCurve *curve;
    int keyIndex;
    float scale;
    float t;

    FxChannelInstance_Create(&primTemp->mFxChannels[0], &colorChannelInstance);

    if (primTemp->mAttributeFlags & 0x2000) {

        float randomWeight = flrand(0.0f, 1.0f);
        FxChannelInstance colorRandChannelInstance;
        FxChannelInstance_Create(&primTemp->mFxChannels[1], &colorRandChannelInstance);

        curve = colorChannelInstance.curveIterator.master;
        keyIndex = colorChannelInstance.curveIterator.currentKeyIndex;
        t = 0.0f;
        FxCurveIterator_FindKey(curve, &keyIndex, t);
        colorChannelInstance.curveIterator.currentKeyIndex = keyIndex;

        {
            float baseColor[3];
            FxCurve_Interp3(curve, keyIndex, t, baseColor);
            rgba[0] = baseColor[0];
            rgba[1] = baseColor[1];
            rgba[2] = baseColor[2];
        }

        {
            const FxCurve *randCurve = colorRandChannelInstance.curveIterator.master;
            int randKeyIndex = colorRandChannelInstance.curveIterator.currentKeyIndex;
            float randColor[3];

            FxCurveIterator_FindKey(randCurve, &randKeyIndex, t);
            colorRandChannelInstance.curveIterator.currentKeyIndex = randKeyIndex;

            {
                int stride = randCurve->dimensionCount + 1;
                const float *keyBase = &randCurve->keys[randKeyIndex * stride];
                float keyTime = keyBase[0];
                float nextKeyTime = keyBase[stride];
                float frac = (t - keyTime) / (nextKeyTime - keyTime);

                randColor[0] = keyBase[1] + (keyBase[stride + 1] - keyBase[1]) * frac;
                randColor[1] = keyBase[2] + (keyBase[stride + 2] - keyBase[2]) * frac;
                randColor[2] = keyBase[3] + (keyBase[stride + 3] - keyBase[3]) * frac;
            }

            rgba[0] = rgba[0] + (randColor[0] - rgba[0]) * randomWeight;
            rgba[1] = rgba[1] + (randColor[1] - rgba[1]) * randomWeight;
            rgba[2] = rgba[2] + (randColor[2] - rgba[2]) * randomWeight;
        }

        scale = colorChannelInstance.scale;
        rgba[0] *= scale;
        rgba[1] *= scale;
        rgba[2] *= scale;
    } else {

        curve = colorChannelInstance.curveIterator.master;
        keyIndex = colorChannelInstance.curveIterator.currentKeyIndex;
        t = 0.0f;
        FxCurveIterator_FindKey(curve, &keyIndex, t);
        colorChannelInstance.curveIterator.currentKeyIndex = keyIndex;

        {
            float color[3];
            FxCurve_Interp3(curve, keyIndex, t, color);
            rgba[0] = color[0];
            rgba[1] = color[1];
            rgba[2] = color[2];
        }

        scale = colorChannelInstance.scale;
        rgba[0] *= scale;
        rgba[1] *= scale;
        rgba[2] *= scale;
    }
}

float FxScheduler_GetDecalAlpha(const FxScheduler *_this, const PrimitiveTemplate *primTemp)
{
    FxChannelInstance alphaChannelInstance;
    const FxCurve *curve;
    int keyIndex;
    float t;
    float result;

    FxChannelInstance_Create(&primTemp->mFxChannels[2], &alphaChannelInstance);

    if (primTemp->mAttributeFlags & 0x4000) {

        float randomWeight = flrand(0.0f, 1.0f);
        FxChannelInstance alphaRandChannelInstance;
        float baseVal, randVal;

        FxChannelInstance_Create(&primTemp->mFxChannels[3], &alphaRandChannelInstance);

        curve = alphaChannelInstance.curveIterator.master;
        keyIndex = alphaChannelInstance.curveIterator.currentKeyIndex;
        t = 0.0f;
        FxCurveIterator_FindKey(curve, &keyIndex, t);
        alphaChannelInstance.curveIterator.currentKeyIndex = keyIndex;
        baseVal = FxCurve_Interp1(curve, keyIndex, t);

        {
            const FxCurve *randCurve = alphaRandChannelInstance.curveIterator.master;
            int randKeyIndex = alphaRandChannelInstance.curveIterator.currentKeyIndex;

            FxCurveIterator_FindKey(randCurve, &randKeyIndex, t);
            alphaRandChannelInstance.curveIterator.currentKeyIndex = randKeyIndex;
            randVal = FxCurve_Interp1(randCurve, randKeyIndex, t);
        }

        result = (baseVal + (randVal - baseVal) * randomWeight) * alphaChannelInstance.scale;
    } else {

        curve = alphaChannelInstance.curveIterator.master;
        keyIndex = alphaChannelInstance.curveIterator.currentKeyIndex;
        t = 0.0f;
        FxCurveIterator_FindKey(curve, &keyIndex, t);
        alphaChannelInstance.curveIterator.currentKeyIndex = keyIndex;
        result = FxCurve_Interp1(curve, keyIndex, t) * alphaChannelInstance.scale;
    }

    if (result < 0.0f)
        result = 0.0f;
    if (result > 1.0f)
        result = 1.0f;

    return result;
}

float FxScheduler_GetDecalSize(const FxScheduler *_this, const PrimitiveTemplate *primTemp)
{
    FxChannelInstance sizeChannelInstance;
    const FxCurve *curve;
    int keyIndex;
    float t;
    float result;

    FxChannelInstance_Create(&primTemp->mFxChannels[4], &sizeChannelInstance);

    if ((short)primTemp->mAttributeFlags < 0) {

        float randomWeight = flrand(0.0f, 1.0f);
        FxChannelInstance sizeRandChannelInstance;
        float baseVal, randVal;

        FxChannelInstance_Create(&primTemp->mFxChannels[5], &sizeRandChannelInstance);

        curve = sizeChannelInstance.curveIterator.master;
        keyIndex = sizeChannelInstance.curveIterator.currentKeyIndex;
        t = 0.0f;
        FxCurveIterator_FindKey(curve, &keyIndex, t);
        sizeChannelInstance.curveIterator.currentKeyIndex = keyIndex;
        baseVal = FxCurve_Interp1(curve, keyIndex, t);

        {
            const FxCurve *randCurve = sizeRandChannelInstance.curveIterator.master;
            int randKeyIndex = sizeRandChannelInstance.curveIterator.currentKeyIndex;

            FxCurveIterator_FindKey(randCurve, &randKeyIndex, t);
            sizeRandChannelInstance.curveIterator.currentKeyIndex = randKeyIndex;
            randVal = FxCurve_Interp1(randCurve, randKeyIndex, t);
        }

        result = (baseVal + (randVal - baseVal) * randomWeight) * sizeChannelInstance.scale;
    } else {

        curve = sizeChannelInstance.curveIterator.master;
        keyIndex = sizeChannelInstance.curveIterator.currentKeyIndex;
        t = 0.0f;
        FxCurveIterator_FindKey(curve, &keyIndex, t);
        sizeChannelInstance.curveIterator.currentKeyIndex = keyIndex;
        result = FxCurve_Interp1(curve, keyIndex, t) * sizeChannelInstance.scale;
    }

    return result;
}

EffectTemplate *MediaHandles_GetEffect(const MediaHandles *_this)
{
    unsigned short count = _this->mMediaList.size;
    TMediaElement *elements;

    if (!count) {
        return NULL;
    }

    elements = *(TMediaElement **)_this;
    return (EffectTemplate *)elements[irand(0, count)].data;
}

void FxScheduler_CreateDecalEffect(const FxScheduler *_this, const PrimitiveTemplate *primTemp, vec_t *org, MediaHandles *(*ax)[4])
{
    float rotation;
    float rgba[4];
    float size;
    MaterialHandle markMaterial;
    unsigned short count;
    TMediaElement *elements;

    rotation = FxRange_GetVal(&((PrimitiveTemplate *)primTemp)->mRotation);

    FxScheduler_GetDecalColor(_this, primTemp, rgba);

    rgba[3] = FxScheduler_GetDecalAlpha(_this, primTemp);

    size = FxScheduler_GetDecalSize(_this, primTemp);

    count = primTemp->mMediaHandles.mMediaList.size;
    if (!count) {
        markMaterial = NULL;
    } else {
        elements = *(TMediaElement **)&primTemp->mMediaHandles;
        markMaterial = (MaterialHandle)elements[irand(0, count)].data;
    }

    CG_ImpactMark(markMaterial, org, (const vec_t *)ax, rotation, rgba, size);
}

static inline __attribute__((always_inline)) void FxArchive_ArchiveInt(const FxArchive *arch, int *field)
{
    int tmp;
    if (arch->isReading) {

        FxArchive_ReadData(arch, &tmp, 4);
        *field = tmp;
    } else {

        tmp = *field;
        FxArchive_WriteData(arch, &tmp, 4);
    }
}

static inline __attribute__((always_inline)) void FxArchive_ArchiveVec3(const FxArchive *arch, vec_t *v)
{
    int tmp;
    if (arch->isReading) {

        FxArchive_ReadData(arch, v, 0xc);
    } else {

        tmp = *(int *)&v[0];
        FxArchive_WriteData(arch, &tmp, 4);
        tmp = *(int *)&v[1];
        FxArchive_WriteData(arch, &tmp, 4);
        tmp = *(int *)&v[2];
        FxArchive_WriteData(arch, &tmp, 4);
    }
}

void ScheduledEffect_Archive(const ScheduledEffect *_this, FxArchive *arch)
{
    ScheduledEffect *sfxPtr = (ScheduledEffect *)_this;

    FxArchive_ArchiveEffect(arch, &sfxPtr->mFx);

    FxArchive_ArchiveInt(arch, &sfxPtr->mPrimIndex);

    FxArchive_ArchiveInt(arch, &sfxPtr->mStartTime);

    FxArchive_ArchiveFxBoltInfo(arch, &sfxPtr->mBolt);

    FxArchive_ArchiveVec3(arch, sfxPtr->mOrigin);

    FxArchive_ArchiveVec3(arch, sfxPtr->mAxis[0]);

    FxArchive_ArchiveVec3(arch, sfxPtr->mAxis[1]);

    FxArchive_ArchiveVec3(arch, sfxPtr->mAxis[2]);

    FxArchive_ArchiveInt(arch, &sfxPtr->mSeed);
}

void FxScheduler_Archive(const FxScheduler *_this, FxArchive *arch)
{
    byte *self = (byte *)_this;
    int pendingCount;
    int i;
    ScheduledEffect *sfx;

    FxScheduler *sched_self = (FxScheduler *)_this;
    if (arch->isReading) {

        sched_self->mScheduledHead = NULL;
        sched_self->mScheduledCount = 0;

        FxArchive_ReadData(arch, &pendingCount, 4);

        for (i = 0; i < pendingCount; i++) {
            ScheduledEffect *newSfx = (ScheduledEffect *)__Znam(0x50);
            if (newSfx) {
                memset(newSfx, 0, sizeof(ScheduledEffect));
            }

            ScheduledEffect_Archive(newSfx, arch);

            {
                const EffectTemplate *fx = newSfx->mFx;
                int primIndex = newSfx->mPrimIndex;
                if (fx && primIndex >= 0 && primIndex < fx->mPrimitiveCount &&
                    fx->mPrimitives[primIndex] != NULL) {

                    newSfx->mScheduledNext = (int)(size_t)sched_self->mScheduledHead;
                    sched_self->mScheduledHead = newSfx;
                    sched_self->mScheduledCount += 1;
                } else {

                    if (newSfx)
                        __ZdaPv(newSfx);
                }
            }
        }
    } else {

        int tmp = sched_self->mScheduledCount;
        FxArchive_WriteData(arch, &tmp, 4);

        {
            FxScheduler *sched = *(FxScheduler **)imp_theFxScheduler;
            sfx = sched->mScheduledHead;
            while (sfx) {
                ScheduledEffect_Archive(sfx, arch);
                sfx = (ScheduledEffect *)(size_t)sfx->mScheduledNext;
            }
        }
    }
}
