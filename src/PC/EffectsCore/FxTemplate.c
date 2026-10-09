#include "common_types.h"
#include "imports.h"

extern const FxFlagEntry fxAttributeFlags[27];
extern const FxFlagEntry fxSpawnFlags[13];
extern Bool g_rendererExists;

extern MaterialHandle Material_RegisterHandle(const char *name, int imageTrack, int materialType);
extern void MediaHandles_Shutdown(MediaHandles *handles);
extern void Com_Printf(const char *fmt, ...);
extern Bool Com_ValidXModelName(const char *name);
extern struct XModel *FX_XModelPrecache(const char *name);
extern float flrand(float min, float max);

extern void *Hunk_AllocateTempMemoryInternal(int size);
extern void Hunk_FreeTempMemory(void *buf);
extern const FxCurve *FxCurve_AllocAndCreateWithKeys(const float *keyArray, int dimensionCount, int keyCount);
extern void FxChannel_CreateDefault(FxChannel *createe, int dimensions, float value1, float value2);
extern void FxChannel_CreateViaMigration(const FxChannelBackwardCompatible *source, int dimensions, float lifetime, int forceUnitScale, FxChannel *target);
extern Bool GPValue_IsList(const GPValue *_this);
extern const char *GPValue_GetTopValue(const GPValue *_this);
extern void MediaHandles_AddHandle(const MediaHandles *_this, TMediaElement item);
extern void MediaHandles_AddEffect(const MediaHandles *_this, EffectTemplate *fx);
extern EffectTemplate *FX_RegisterEffect(const char *fileName);
extern int I_stricmp(const char *s1, const char *s2);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int atoi(const char *s);
extern double atof(const char *s);

#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <strings.h>

extern int stricmp(const char *s1, const char *s2);

COD2_ASSERT_FIELD(struct FxRange, mMin, 0x00);
COD2_ASSERT_FIELD(struct FxRange, mMax, 0x04);

COD2_ASSERT_FIELD(struct PrimitiveTemplate, mName,             0x00);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mMaterialImpact,   0x20);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mSpawnDelay,       0x48);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mSpawnCount,       0x50);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mLife,             0x58);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mSpawnRange,       0x60);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mMediaHandles,     0x68);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mImpactFxHandles,  0x70);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mDeathFxHandles,   0x78);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mEmitterFxHandles, 0x80);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mPlayFxHandles,    0x88);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mAttributeFlags,   0x90);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mNonUniformScale,  0x9c);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, useLength,         0x9d);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mMin,              0xa0);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mMax,              0xac);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mOrigin1X,         0xb8);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mOrigin1Y,         0xc0);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mOrigin1Z,         0xc8);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mOrigin2X,         0xd0);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mOrigin2Y,         0xd8);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mOrigin2Z,         0xe0);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mRadius,           0xe8);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mHeight,           0xf0);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mWindModifier,     0xf8);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mFxChannels,       0x100);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mRotation,         0x220);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mAngle1,           0x228);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mAngle2,           0x230);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mAngle3,           0x238);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mAngle1Delta,      0x240);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mAngle2Delta,      0x248);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mAngle3Delta,      0x250);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mGravity,          0x258);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mDensity,          0x260);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mVariance,         0x268);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mTexCoordS,        0x270);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mTexCoordT,        0x278);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mElasticity,       0x280);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mSequenceStartFrameMode,  0x288);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mSequenceFixedFrameValue, 0x28c);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mSequencePlayRateMode,    0x290);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mSequenceFixedFpsValue,   0x294);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mSequenceLoopMode,        0x298);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, mSequenceLoopTimes,       0x29c);
COD2_ASSERT_FIELD(struct PrimitiveTemplate, spawnFrustumCullRadius,   0x2a0);

COD2_ASSERT_FIELD(struct FxChannelBackwardCompatible, start,        0x00);
COD2_ASSERT_FIELD(struct FxChannelBackwardCompatible, end,          0x18);
COD2_ASSERT_FIELD(struct FxChannelBackwardCompatible, parm,         0x30);
COD2_ASSERT_FIELD(struct FxChannelBackwardCompatible, flags,        0x38);
COD2_ASSERT_FIELD(struct FxChannelBackwardCompatible, containsData, 0x3c);
COD2_ASSERT_SIZE (struct FxChannelBackwardCompatible, 0x40);

void FxRange_SetRange(const FxRange *_this, float min, float max);
void PrimitiveTemplate_Shutdown(const PrimitiveTemplate *_this);
float FxRange_GetValPct(const FxRange *_this, float percent);
Bool PrimitiveTemplate_ParseGroupFlags(const PrimitiveTemplate *_this, const char *val, int *groupFlags);
void FX_Print(const char *msg, ...);
MaterialHandle FX_RegisterMaterial(const char *material);
struct XModel *FX_ModelRegister(const char *name);
float FxRange_GetVal(const FxRange *_this);
void PrimitiveTemplate_Init(const PrimitiveTemplate *_this);
void PrimitiveTemplate_ParseChannelCurve(const PrimitiveTemplate *_this, GPValue *pairs, FxChannelId channel);
Bool PrimitiveTemplate_ParseImpactFxStrings(const PrimitiveTemplate *_this, GPValue *grp);
Bool PrimitiveTemplate_ParseDeathFxStrings(const PrimitiveTemplate *_this, GPValue *grp);
Bool PrimitiveTemplate_ParseEmitterFxStrings(const PrimitiveTemplate *_this, GPValue *grp);
Bool PrimitiveTemplate_ParsePlayFxStrings(const PrimitiveTemplate *_this, GPValue *grp);
void PrimitiveTemplate_ParseChannelRgbCurve(const PrimitiveTemplate *_this, GPValue *pairs, FxChannelId channel);
void PrimitiveTemplate_CreateBackCompatibleRotationDeltaCurve(const PrimitiveTemplate *_this, float initialValue, float keyScale, float lifetime, int channelId, float graphScale);
Bool PrimitiveTemplate_ParseMaterials(const PrimitiveTemplate *_this, GPValue *grp);
Bool PrimitiveTemplate_ParseFlags(const PrimitiveTemplate *_this, const char *line, const FxFlagEntry *flagEntries, int flagEntryCount);
Bool PrimitiveTemplate_ParseModels(const PrimitiveTemplate *_this, GPValue *grp);
Bool PrimitiveTemplate_ParseRotationDelta(const PrimitiveTemplate *_this, const char *val);
Bool PrimitiveTemplate_ParseChannel(const PrimitiveTemplate *_this, BackCompatibleParameters *backCompatibleParameters, GPGroup *grp, FxChannelId channelId, const PrimitiveTemplate *_this_4, const char *val, const PrimitiveTemplate *_this_6, const char *val_7, const PrimitiveTemplate *_this_8, const char *val_9, const PrimitiveTemplate *_this_10, const char *val_11);
Bool PrimitiveTemplate_ParseVelocity(const PrimitiveTemplate *_this, const char *val);
Bool PrimitiveTemplate_ParseAcceleration(const PrimitiveTemplate *_this, const char *val);
Bool PrimitiveTemplate_ParsePrimitiveInternal(const PrimitiveTemplate *_this, BackCompatibleParameters *backCompatibleParameters, GPGroup *grp);
Bool PrimitiveTemplate_ParsePrimitive(const PrimitiveTemplate *_this, GPGroup *grp);

#define GPV_STRING(v) (*(const char **)((byte *)(v)))
#define GPV_NEXT(v) (*(GPValue **)((byte *)(v) + 4))
#define GPV_LIST(v) (*(GPValue **)((byte *)(v) + 0x10))

#define GPG_PAIRS(g) (*(GPValue **)((byte *)(g) + 0x10))
#define GPG_SUBGROUPS(g) (*(GPValue **)((byte *)(g) + 0x1c))

static int ParseFloatRange(const char *val, float *outMin, float *outMax)
{
    float a, b;
    int n = sscanf(val, "%f %f", &a, &b);
    if (n == 0)
        return 0;
    if (n == 1)
        b = a;
    *outMin = a;
    *outMax = b;
    return 1;
}

Bool PrimitiveTemplate_ParsePrimitive(const PrimitiveTemplate *_this, GPGroup *grp)
{
    BackCompatibleParameters *backCompatibleParameters;
    Bool result;

    backCompatibleParameters = (BackCompatibleParameters *)Hunk_AllocateTempMemoryInternal(0x600);
    result = PrimitiveTemplate_ParsePrimitiveInternal(_this, backCompatibleParameters, grp);
    Hunk_FreeTempMemory(backCompatibleParameters);
    return result;
}

static inline __attribute__((always_inline)) int ParseVec3Range(const char *val, float *minV, float *maxV)
{
    int n = sscanf(val, "%f %f %f   %f %f %f",
                   &minV[0], &minV[1], &minV[2],
                   &maxV[0], &maxV[1], &maxV[2]);
    if (n <= 2 || n == 4 || n == 5)
        return 0;
    if (n == 3) {
        maxV[0] = minV[0];
        maxV[1] = minV[1];
        maxV[2] = minV[2];
    }
    return n;
}

static inline __attribute__((always_inline)) void EnsureMinMax(float *a, float *b)
{
    if (*a > *b) {
        float t = *a;
        *a = *b;
        *b = t;
    }
}

static inline __attribute__((always_inline)) float AbsCeil(float val)
{
    int iv = (int)val;
    if (iv <= -1) {
        iv = -iv;
    }
    return (float)iv;
}

static inline __attribute__((always_inline)) void CreateTwoKeyCurve(byte *_this, int channelOffset, float initialValue, float maxRange)
{
    float keys[4];
    keys[0] = 0.0f;
    if (maxRange != 0.0f) {
        keys[1] = initialValue / maxRange;
    } else {
        keys[1] = 0.0f;
    }
    keys[2] = 1.0f;
    keys[3] = keys[1];

    const FxCurve *curve = FxCurve_AllocAndCreateWithKeys(keys, 1, 2);
    *(const FxCurve **)(_this + channelOffset) = curve;
    *(float *)(_this + channelOffset + 4) = maxRange;
    *(float *)(_this + channelOffset + 8) = maxRange;
}

static inline __attribute__((always_inline)) void CreateAccelCurve(byte *_this, int channelOffset, float initialValue, float maxScale, float maxRange)
{
    float keys[4];
    float scale = maxScale * 0.001f;
    float range = maxRange * scale;
    keys[0] = 0.0f;
    keys[1] = 0.0f;
    keys[2] = 1.0f;
    keys[3] = initialValue * scale;
    if (maxRange != 0.0f) {
        keys[3] = keys[3] / maxRange;
    } else {
        keys[3] = 0.0f;
    }

    const FxCurve *curve = FxCurve_AllocAndCreateWithKeys(keys, 1, 2);
    *(const FxCurve **)(_this + channelOffset) = curve;
    *(float *)(_this + channelOffset + 4) = range;
    *(float *)(_this + channelOffset + 8) = range;
}

static inline __attribute__((always_inline)) void CreateAccelCurveUnit(byte *_this, int channelOffset, float initialValue, float maxScale, float maxRange)
{
    float keys[4];
    float scale = maxScale * 0.001f;
    keys[0] = 0.0f;
    keys[1] = 0.0f;
    keys[2] = 1.0f;
    keys[3] = initialValue * scale;
    if (maxRange != 0.0f) {
        keys[3] = keys[3] / maxRange;
    } else {
        keys[3] = 0.0f;
    }

    const FxCurve *curve = FxCurve_AllocAndCreateWithKeys(keys, 1, 2);
    *(const FxCurve **)(_this + channelOffset) = curve;
    *(float *)(_this + channelOffset + 4) = 1.0f;
    *(float *)(_this + channelOffset + 8) = 1.0f;
}

void FxRange_SetRange(const FxRange *_this, float min, float max)
{
    FxRange *r = (FxRange *)_this;
    r->mMin = min;
    r->mMax = max;
}

void PrimitiveTemplate_Shutdown(const PrimitiveTemplate *_this)
{
    PrimitiveTemplate *pt = (PrimitiveTemplate *)_this;
    MediaHandles_Shutdown(&pt->mMediaHandles);
    MediaHandles_Shutdown(&pt->mImpactFxHandles);
    MediaHandles_Shutdown(&pt->mDeathFxHandles);
    MediaHandles_Shutdown(&pt->mEmitterFxHandles);
    MediaHandles_Shutdown(&pt->mPlayFxHandles);
}

float FxRange_GetValPct(const FxRange *_this, float percent)
{
    float base = _this->mMin;
    float range = _this->mMax - base;
    return base + range * percent;
}

Bool PrimitiveTemplate_ParseGroupFlags(const PrimitiveTemplate *_this, const char *val, int *groupFlags)
{
    char flags_buf[0x80] = { [0x60] = '0' };
    int count;
    int i;
    Bool result = 1;

    (void)_this;
    count = sscanf(val, "%s %s %s %s", flags_buf, flags_buf + 0x20, flags_buf + 0x40, flags_buf + 0x60);

    *groupFlags = 0;

    for (i = 0; i + 1 <= count; i++) {
        const char *tok = &flags_buf[i << 5];

        if (stricmp(tok, "linear") == 0)
            *groupFlags |= 1;
        else if (stricmp(tok, "nonlinear") == 0)
            *groupFlags |= 4;
        else if (stricmp(tok, "wave") == 0)
            *groupFlags |= 8;
        else if (stricmp(tok, "random") == 0)
            *groupFlags |= 2;
        else if (stricmp(tok, "clamp") == 0)
            *groupFlags |= 0xc;
        else
            result = 0;

        if (i + 1 == 4)
            return result;
    }

    return 1;
}

void FX_Print(const char *msg, ...)
{
    char text[1024];
    va_list argptr;

    va_start(argptr, msg);
    vsnprintf(text, 0x400, msg, argptr);
    va_end(argptr);

    Com_Printf(text);
}

MaterialHandle FX_RegisterMaterial(const char *material)
{
    return Material_RegisterHandle(material, 3, 6);
}

struct XModel *FX_ModelRegister(const char *name)
{
    if (!Com_ValidXModelName(name)) {
        return NULL;
    }

    return FX_XModelPrecache(name + 7);
}

Bool PrimitiveTemplate_ParseMaterials(const PrimitiveTemplate *_this, GPValue *grp)
{
    GPValue *p;
    const char *str;
    TMediaElement media;

    if (!g_rendererExists)
        return 1;

    if (GPValue_IsList(grp)) {
        for (p = GPV_LIST(grp); p; p = GPV_NEXT(p)) {
            media.material = Material_RegisterHandle(GPV_STRING(p), 3, 6);
            MediaHandles_AddHandle((MediaHandles *)((byte *)_this + 0x68), media);
        }
        return 1;
    }

    str = GPValue_GetTopValue(grp);
    if (!str) {
        FX_Print("PrimitiveTemplate::ParseMaterials called with an empty list!\n");
        return 0;
    }
    media.material = Material_RegisterHandle(str, 3, 6);
    MediaHandles_AddHandle((MediaHandles *)((byte *)_this + 0x68), media);
    return 1;
}

Bool PrimitiveTemplate_ParseModels(const PrimitiveTemplate *_this, GPValue *grp)
{
    PrimitiveTemplate *thisPtr = (PrimitiveTemplate *)_this;
    GPValue *p;
    const char *str;
    struct XModel *model;
    TMediaElement media;

    if (GPValue_IsList(grp)) {
        for (p = GPV_LIST(grp); p; p = GPV_NEXT(p)) {
            str = GPV_STRING(p);
            if (!Com_ValidXModelName(str)) {
                FX_Print("PrimitiveTemplate::ParseModels, could not register model '%s'\n", str);
                return 0;
            }
            model = FX_XModelPrecache(str + 7);
            if (!model) {
                FX_Print("PrimitiveTemplate::ParseModels, could not register model '%s'\n", str);
                return 0;
            }
            media.model = model;
            MediaHandles_AddHandle(&thisPtr->mMediaHandles, media);
        }
        return 1;
    }

    str = GPValue_GetTopValue(grp);
    if (!str) {
        FX_Print("PrimitiveTemplate::ParseModels called with an empty list!\n");
        return 0;
    }
    if (!Com_ValidXModelName(str)) {
        FX_Print("PrimitiveTemplate::ParseModels, could not register model '%s'\n", str);
        return 0;
    }
    model = FX_XModelPrecache(str + 7);
    if (!model) {
        FX_Print("PrimitiveTemplate::ParseModels, could not register model '%s'\n", str);
        return 0;
    }
    media.model = model;
    MediaHandles_AddHandle(&thisPtr->mMediaHandles, media);
    return 1;
}

Bool PrimitiveTemplate_ParseImpactFxStrings(const PrimitiveTemplate *_this, GPValue *grp)
{
    PrimitiveTemplate *thisPtr = (PrimitiveTemplate *)_this;
    GPValue *p;
    const char *str;
    EffectTemplate *fx;

    if (GPValue_IsList(grp)) {
        for (p = GPV_LIST(grp); p; p = GPV_NEXT(p)) {
            fx = FX_RegisterEffect(GPV_STRING(p));
            if (!fx) {
                FX_Print("FxTemplate: Impact effect file not found.\n");
                return 0;
            }
            MediaHandles_AddEffect(&thisPtr->mImpactFxHandles, fx);
        }
        thisPtr->mAttributeFlags |= 0x820;
        return 1;
    }

    str = GPValue_GetTopValue(grp);
    if (!str) {
        FX_Print("PrimitiveTemplate::ParseImpactFxStrings called with an empty list!\n");
        return 0;
    }
    fx = FX_RegisterEffect(str);
    if (!fx) {
        FX_Print("FxTemplate: Impact effect file not found.\n");
        return 0;
    }
    MediaHandles_AddEffect(&thisPtr->mImpactFxHandles, fx);
    thisPtr->mAttributeFlags |= 0x820;
    return 1;
}

Bool PrimitiveTemplate_ParseDeathFxStrings(const PrimitiveTemplate *_this, GPValue *grp)
{
    PrimitiveTemplate *thisPtr = (PrimitiveTemplate *)_this;
    GPValue *p;
    const char *str;
    EffectTemplate *fx;

    if (GPValue_IsList(grp)) {
        for (p = GPV_LIST(grp); p; p = GPV_NEXT(p)) {
            fx = FX_RegisterEffect(GPV_STRING(p));
            if (!fx) {
                FX_Print("FxTemplate: Death effect file not found.\n");
                return 0;
            }
            MediaHandles_AddEffect(&thisPtr->mDeathFxHandles, fx);
        }
        thisPtr->mAttributeFlags |= 0x200;
        return 1;
    }

    str = GPValue_GetTopValue(grp);
    if (!str) {
        FX_Print("PrimitiveTemplate::ParseDeathFxStrings called with an empty list!\n");
        return 0;
    }
    fx = FX_RegisterEffect(str);
    if (!fx) {
        FX_Print("FxTemplate: Death effect file not found.\n");
        return 0;
    }
    MediaHandles_AddEffect(&thisPtr->mDeathFxHandles, fx);
    thisPtr->mAttributeFlags |= 0x200;
    return 1;
}

Bool PrimitiveTemplate_ParseEmitterFxStrings(const PrimitiveTemplate *_this, GPValue *grp)
{
    PrimitiveTemplate *thisPtr = (PrimitiveTemplate *)_this;
    GPValue *p;
    const char *str;
    EffectTemplate *fx;

    if (GPValue_IsList(grp)) {
        for (p = GPV_LIST(grp); p; p = GPV_NEXT(p)) {
            fx = FX_RegisterEffect(GPV_STRING(p));
            if (!fx) {
                FX_Print("FxTemplate: Emitter effect file not found.\n");
                return 0;
            }
            MediaHandles_AddEffect(&thisPtr->mEmitterFxHandles, fx);
        }
        thisPtr->mAttributeFlags |= 0x100;
        return 1;
    }

    str = GPValue_GetTopValue(grp);
    if (!str) {
        FX_Print("PrimitiveTemplate::ParseEmitterFxStrings called with an empty list!\n");
        return 0;
    }
    fx = FX_RegisterEffect(str);
    if (!fx) {
        FX_Print("FxTemplate: Emitter effect file not found.\n");
        return 0;
    }
    MediaHandles_AddEffect(&thisPtr->mEmitterFxHandles, fx);
    thisPtr->mAttributeFlags |= 0x100;
    return 1;
}

Bool PrimitiveTemplate_ParsePlayFxStrings(const PrimitiveTemplate *_this, GPValue *grp)
{
    PrimitiveTemplate *thisPtr = (PrimitiveTemplate *)_this;
    GPValue *p;
    const char *str;
    EffectTemplate *fx;

    if (GPValue_IsList(grp)) {
        for (p = GPV_LIST(grp); p; p = GPV_NEXT(p)) {
            fx = FX_RegisterEffect(GPV_STRING(p));
            if (!fx) {
                FX_Print("FxTemplate: Effect file not found.\n");
                return 0;
            }
            MediaHandles_AddEffect(&thisPtr->mPlayFxHandles, fx);
        }
        return 1;
    }

    str = GPValue_GetTopValue(grp);
    if (!str) {
        FX_Print("PrimitiveTemplate::ParsePlayFxStrings called with an empty list!\n");
        return 0;
    }
    fx = FX_RegisterEffect(str);
    if (!fx) {
        FX_Print("FxTemplate: Effect file not found.\n");
        return 0;
    }
    MediaHandles_AddEffect(&thisPtr->mPlayFxHandles, fx);
    return 1;
}

float FxRange_GetVal(const FxRange *_this)
{
    float base = _this->mMin;
    float amplitude = _this->mMax;

    if (base == amplitude) {
        return base;
    }

    return flrand(base, amplitude);
}

void PrimitiveTemplate_Init(const PrimitiveTemplate *_this)
{
    PrimitiveTemplate *t = (PrimitiveTemplate *)_this;

    t->mLife.mMin = 1.0f;
    t->mLife.mMax = 1.0f;
    t->mSpawnCount.mMin = 1.0f;
    t->mSpawnCount.mMax = 1.0f;
    t->mRadius.mMin = 1.0f;
    t->mRadius.mMax = 1.0f;
    t->mHeight.mMin = 1.0f;
    t->mHeight.mMax = 1.0f;

    byte *tb = (byte *)t;
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[0].curve, 3, 1.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[1].curve, 3, 1.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[2].curve, 1, 1.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[3].curve, 1, 1.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[4].curve, 1, 1.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[5].curve, 1, 1.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[6].curve, 1, 1.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[7].curve, 1, 1.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[8].curve, 1, 1.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[9].curve, 1, 1.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[10].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[11].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[12].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[13].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[14].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[15].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[16].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[17].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[18].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[19].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[20].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[21].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[22].curve, 1, 0.0f, 0.0f);
    FxChannel_CreateDefault((FxChannel *)&((PrimitiveTemplate *)tb)->mFxChannels[23].curve, 1, 0.0f, 0.0f);

    t->mTexCoordS.mMin = 1.0f;
    t->mTexCoordS.mMax = 1.0f;
    t->mTexCoordT.mMin = 1.0f;
    t->mTexCoordT.mMax = 1.0f;
    t->mVariance.mMin = 1.0f;
    t->mVariance.mMax = 1.0f;
    t->mDensity.mMin = 10.0f;
    t->mDensity.mMax = 10.0f;

    t->mSequenceStartFrameMode = 0;
    t->mSequenceFixedFrameValue = 1;
    t->mSequencePlayRateMode = 0;
    t->mSequenceFixedFpsValue = 1.0f;
    t->mSequenceLoopMode = 0;
    t->mSequenceLoopTimes = 1;
    t->spawnFrustumCullRadius = 0.0f;
}

void PrimitiveTemplate_ParseChannelCurve(const PrimitiveTemplate *_this, GPValue *pairs, FxChannelId channel)
{
    GPValue *list;
    int keyCount;
    float *keys;
    int dimensions = (channel == FXCHAN_COLOR || channel == FXCHAN_COLOR_RAND) ? 3 : 1;
    int stride = dimensions + 1;

    list = GPV_LIST(pairs);

    keyCount = 0;
    if (list) {
        GPValue *p = list;
        while (p) {
            keyCount++;
            p = GPV_NEXT(p);
        }
    }

    if (!keyCount || channel < 0 || channel >= FXCHAN_COUNT)
        return;
    keys = (float *)Hunk_AllocateTempMemoryInternal(keyCount * stride * sizeof(float));

    if (list) {
        float *cur = keys;
        GPValue *p = list;
        keyCount = 0;
        do {
            int parsed = dimensions == 3
                ? sscanf(GPV_STRING(p), "%f %f %f %f", &cur[0], &cur[1], &cur[2], &cur[3])
                : sscanf(GPV_STRING(p), "%f %f", &cur[0], &cur[1]);
            if (parsed != stride) {
                Hunk_FreeTempMemory(keys);
                return;
            }
            keyCount++;
            p = GPV_NEXT(p);
            cur += stride;
        } while (p);
    } else {
        keyCount = 0;
    }

    ((PrimitiveTemplate *)_this)->mFxChannels[channel].curve =
        FxCurve_AllocAndCreateWithKeys(keys, dimensions, keyCount);

    Hunk_FreeTempMemory(keys);
}

void PrimitiveTemplate_CreateBackCompatibleRotationDeltaCurve(const PrimitiveTemplate *_this, float initialValue, float keyScale, float lifetime, int channelId, float graphScale)
{
    byte *thisPtr = (byte *)_this;
    float keys[40];
    int i;

    float timeStep = lifetime / 20.0f * 0.00065f;
    float oneF = 1.0f;
    float endT = timeStep - oneF;
    float negT = -timeStep;

    for (i = 0; i < 20; i++) {
        keys[i * 2] = (float)i / 19.0f;

        if (i == 0) {
            if (keyScale == 0.0f) {

                keys[1] = initialValue;
            } else {
                keys[1] = initialValue / keyScale;
            }
        } else {

            if (keyScale == 0.0f) {

                keys[i * 2 + 1] = keys[(i - 1) * 2 + 1];
            } else if (negT < 0.0f && endT < 0.0f) {

                float factor = oneF - timeStep;
                keys[i * 2 + 1] = keys[(i - 1) * 2 + 1] * factor;
            } else if (negT < 0.0f) {

                keys[i * 2 + 1] = keys[(i - 1) * 2 + 1] * negT;
            } else if (endT < 0.0f) {

                keys[i * 2 + 1] = keys[(i - 1) * 2 + 1];
            } else {

                keys[i * 2 + 1] = keys[(i - 1) * 2 + 1];
            }
        }
    }

    int chanOff = channelId * 3 * 4;
    *(const FxCurve **)(((char *)thisPtr + offsetof(PrimitiveTemplate, mFxChannels[0].curve)) + chanOff) =
        FxCurve_AllocAndCreateWithKeys(keys, 1, 20);

    *(unsigned int *)(((char *)thisPtr + offsetof(PrimitiveTemplate, mFxChannels[0].curve)) + chanOff + 4) = *(unsigned int *)&graphScale;
    *(unsigned int *)(((char *)thisPtr + offsetof(PrimitiveTemplate, mFxChannels[0].curve)) + chanOff + 8) = *(unsigned int *)&graphScale;
}

Bool PrimitiveTemplate_ParseFlags(const PrimitiveTemplate *_this, const char *line, const FxFlagEntry *flagEntries, int flagEntryCount)
{
    byte *thisPtr = (byte *)_this;
    int lineLen;
    char *flag;
    int parsedLength;
    int flagLength;
    int entryIndex;

    lineLen = strlen(line);
    if (lineLen == 0)
        return 0;
    flag = (char *)Hunk_AllocateTempMemoryInternal(lineLen + 1);
    parsedLength = 0;
    while (parsedLength < lineLen) {
        while (parsedLength < lineLen && (unsigned char)line[parsedLength] <= ' ')
            ++parsedLength;
        if (parsedLength == lineLen)
            break;
        flagLength = 0;
        while (parsedLength < lineLen && (unsigned char)line[parsedLength] > ' ')
            flag[flagLength++] = line[parsedLength++];
        flag[flagLength] = '\0';
        for (entryIndex = 0; entryIndex < flagEntryCount; ++entryIndex) {
            if (!stricmp(flagEntries[entryIndex].flag, flag)) {
                ((PrimitiveTemplate *)thisPtr)->mAttributeFlags |= flagEntries[entryIndex].masks[0];
                ((PrimitiveTemplate *)thisPtr)->mSpawnFlags |= flagEntries[entryIndex].masks[1];
                break;
            }
        }
        if (entryIndex == flagEntryCount) {
            Hunk_FreeTempMemory(flag);
            return 0;
        }
    }

    Hunk_FreeTempMemory(flag);
    return 1;
}

Bool PrimitiveTemplate_ParseRotationDelta(const PrimitiveTemplate *_this, const char *val)
{
    PrimitiveTemplate *thisPtr = (PrimitiveTemplate *)_this;
    float minVal, maxVal;

    int n = sscanf(val, "%f %f", &minVal, &maxVal);
    if (n == 0)
        return 0;
    if (n == 1)
        maxVal = minVal;

    EnsureMinMax(&minVal, &maxVal);

    float absMin = AbsCeil(minVal);
    float keyScaleVal = absMin;
    if (keyScaleVal < maxVal)
        keyScaleVal = maxVal;
    keyScaleVal = keyScaleVal + keyScaleVal;

    float lifetime = thisPtr->mLife.mMin;
    PrimitiveTemplate_CreateBackCompatibleRotationDeltaCurve(_this, minVal, keyScaleVal, lifetime, 0xa, keyScaleVal);

    if (minVal != maxVal) {
        thisPtr->mAttributeFlags |= 0x40000;
        float lifetime2 = thisPtr->mLife.mMax;
        PrimitiveTemplate_CreateBackCompatibleRotationDeltaCurve(_this, maxVal, keyScaleVal, lifetime2, 0xb, 1.0f);
    }

    return 1;
}

Bool PrimitiveTemplate_ParseVelocity(const PrimitiveTemplate *_this, const char *val)
{
    byte *thisPtr = (byte *)_this;
    float minV[3], maxV[3];

    int n = ParseVec3Range(val, minV, maxV);
    if (!n)
        return 0;

    EnsureMinMax(&minV[0], &maxV[0]);
    EnsureMinMax(&minV[1], &maxV[1]);
    EnsureMinMax(&minV[2], &maxV[2]);

    float maxX = AbsCeil(minV[0]);
    if (maxX < maxV[0])
        maxX = maxV[0];
    float maxY = AbsCeil(minV[1]);
    if (maxY < maxV[1])
        maxY = maxV[1];
    float maxZ = AbsCeil(minV[2]);
    if (maxZ < maxV[2])
        maxZ = maxV[2];
    maxX *= 2.0f;
    maxY *= 2.0f;
    maxZ *= 2.0f;

    float totalMax;

    if (maxX - maxY >= 0.0f)
        totalMax = maxX;
    else
        totalMax = maxY;

    if (totalMax - maxZ >= 0.0f)
        ;
    else
        totalMax = maxZ;

    CreateTwoKeyCurve(thisPtr, 0x190, minV[0], totalMax);

    CreateTwoKeyCurve(thisPtr, 0x19c, minV[1], totalMax);

    CreateTwoKeyCurve(thisPtr, 0x1a8, minV[2], totalMax);

    int allEqual = 1;
    if (minV[0] != maxV[0] || minV[1] != maxV[1] || minV[2] != maxV[2])
        allEqual = 0;

    if (!allEqual) {

        ((PrimitiveTemplate *)thisPtr)->mAttributeFlags |= 0x80000;

        CreateTwoKeyCurve(thisPtr, 0x1b4, maxV[0], 1.0f);

        CreateTwoKeyCurve(thisPtr, 0x1c0, maxV[1], 1.0f);

        CreateTwoKeyCurve(thisPtr, 0x1cc, maxV[2], 1.0f);
    }

    return 1;
}

Bool PrimitiveTemplate_ParseAcceleration(const PrimitiveTemplate *_this, const char *val)
{
    byte *thisPtr = (byte *)_this;
    float minV[3], maxV[3];
    float maxX, maxY, maxZ, totalMax;
    float lifeMax;
    int n;
    int allEqual;

    n = ParseVec3Range(val, minV, maxV);
    if (!n)
        return 0;

    EnsureMinMax(&minV[0], &maxV[0]);
    EnsureMinMax(&minV[1], &maxV[1]);
    EnsureMinMax(&minV[2], &maxV[2]);

    maxX = AbsCeil(minV[0]);
    if (maxX < maxV[0])
        maxX = maxV[0];
    maxY = AbsCeil(minV[1]);
    if (maxY < maxV[1])
        maxY = maxV[1];
    maxZ = AbsCeil(minV[2]);
    if (maxZ < maxV[2])
        maxZ = maxV[2];
    maxX *= 2.0f;
    maxY *= 2.0f;
    maxZ *= 2.0f;

    totalMax = maxX;
    if (totalMax < maxY)
        totalMax = maxY;
    if (totalMax < maxZ)
        totalMax = maxZ;

    lifeMax = ((PrimitiveTemplate *)thisPtr)->mLife.mMax;

    CreateAccelCurve(thisPtr, 0x1d8, minV[0], lifeMax, totalMax);
    CreateAccelCurve(thisPtr, 0x1e4, minV[1], lifeMax, totalMax);
    CreateAccelCurve(thisPtr, 0x1f0, minV[2], lifeMax, totalMax);

    allEqual = (minV[0] == maxV[0] && minV[1] == maxV[1] && minV[2] == maxV[2]);
    if (!allEqual) {
        ((PrimitiveTemplate *)thisPtr)->mAttributeFlags |= 0x100000;
        CreateAccelCurveUnit(thisPtr, 0x1fc, maxV[0], lifeMax, totalMax);
        CreateAccelCurveUnit(thisPtr, 0x208, maxV[1], lifeMax, totalMax);
        CreateAccelCurveUnit(thisPtr, 0x214, maxV[2], lifeMax, totalMax);
    }

    return 1;
}

static int PrimitiveTemplate_ChannelDimensions(FxChannelId channelId)
{
    if (channelId == FXCHAN_COLOR || channelId == FXCHAN_COLOR_RAND)
        return 3;
    return 1;
}

static Bool PrimitiveTemplate_ParseChannelRanges(const char *val, int dimensions, FxRange *ranges)
{
    float minV[3], maxV[3];
    float minVal, maxVal;
    int i;

    if (!val)
        return 0;

    if (dimensions == 3) {
        if (!ParseVec3Range(val, minV, maxV))
            return 0;
        for (i = 0; i < 3; i++) {
            EnsureMinMax(&minV[i], &maxV[i]);
            ranges[i].mMin = minV[i];
            ranges[i].mMax = maxV[i];
        }
        return 1;
    }

    if (!ParseFloatRange(val, &minVal, &maxVal))
        return 0;
    EnsureMinMax(&minVal, &maxVal);
    ranges[0].mMin = minVal;
    ranges[0].mMax = maxVal;
    return 1;
}

static void PrimitiveTemplate_CopyChannelRanges(FxRange *dst, const FxRange *src, int dimensions)
{
    int i;

    for (i = 0; i < dimensions; i++)
        dst[i] = src[i];
}

static void PrimitiveTemplate_ParseChannelFlags(FxChannelBackwardCompatible *channel, const char *val)
{
    char token[64];
    int offset;

    if (!val)
        return;

    offset = 0;
    while (sscanf(val + offset, "%63s", token) == 1) {
        int len = strlen(token);

        if (stricmp(token, "random") == 0 || stricmp(token, "rand") == 0)
            channel->flags |= 2;
        else if (stricmp(token, "delay") == 0 || stricmp(token, "fadein") == 0)
            channel->flags = (channel->flags & ~0xc) | 4;
        else if (stricmp(token, "wave") == 0 || stricmp(token, "cos") == 0 || stricmp(token, "cosine") == 0)
            channel->flags = (channel->flags & ~0xc) | 8;
        else if (stricmp(token, "fadeout") == 0)
            channel->flags = (channel->flags & ~0xc) | 0xc;

        offset += len;
        while (val[offset] == ' ' || val[offset] == '\t')
            offset++;
        if (!val[offset])
            break;
    }
}

Bool PrimitiveTemplate_ParseChannel(const PrimitiveTemplate *_this, BackCompatibleParameters *backCompatibleParameters, GPGroup *grp, FxChannelId channelId, const PrimitiveTemplate *_this_4, const char *val, const PrimitiveTemplate *_this_6, const char *val_7, const PrimitiveTemplate *_this_8, const char *val_9, const PrimitiveTemplate *_this_10, const char *val_11)
{
    FxChannelBackwardCompatible *channel;
    GPValue *first;
    GPValue *pair;
    int dimensions;
    Bool parsed;

    (void)_this_4;
    (void)val;
    (void)_this_6;
    (void)val_7;
    (void)_this_8;
    (void)val_9;
    (void)_this_10;
    (void)val_11;

    if (!backCompatibleParameters || !grp || channelId < 0 || channelId >= FXCHAN_COUNT)
        return 0;

    channel = &backCompatibleParameters->fxChannels[channelId];
    dimensions = PrimitiveTemplate_ChannelDimensions(channelId);
    first = GPG_PAIRS(grp);
    if (!first)
        return 0;

    if (!GPV_LIST(first)) {
        if (!PrimitiveTemplate_ParseChannelRanges(GPV_STRING(first), dimensions, channel->start))
            return 0;
        PrimitiveTemplate_CopyChannelRanges(channel->end, channel->start, dimensions);
        channel->flags |= 1;
        channel->containsData = 1;
        return 1;
    }

    parsed = 0;
    for (pair = first; pair; pair = GPV_NEXT(pair)) {
        const char *key = GPV_STRING(pair);
        const char *topValue = GPValue_GetTopValue(pair);

        if (!key)
            continue;

        if (stricmp(key, "start") == 0 || stricmp(key, "begin") == 0) {
            if (!PrimitiveTemplate_ParseChannelRanges(topValue, dimensions, channel->start))
                return 0;
            channel->flags |= 1;
            channel->containsData = 1;
            parsed = 1;
        } else if (stricmp(key, "end") == 0) {
            if (!PrimitiveTemplate_ParseChannelRanges(topValue, dimensions, channel->end))
                return 0;
            channel->flags |= 1;
            channel->containsData = 1;
            parsed = 1;
        } else if (stricmp(key, "value") == 0) {
            if (!PrimitiveTemplate_ParseChannelRanges(topValue, dimensions, channel->start))
                return 0;
            PrimitiveTemplate_CopyChannelRanges(channel->end, channel->start, dimensions);
            channel->flags |= 1;
            channel->containsData = 1;
            parsed = 1;
        } else if (stricmp(key, "parm") == 0 || stricmp(key, "param") == 0 || stricmp(key, "delay") == 0) {
            if (!PrimitiveTemplate_ParseChannelRanges(topValue, 1, &channel->parm))
                return 0;
            parsed = 1;
        } else if (stricmp(key, "flags") == 0 || stricmp(key, "flag") == 0) {
            PrimitiveTemplate_ParseChannelFlags(channel, topValue);
            parsed = 1;
        } else if (stricmp(key, "curve") == 0 || stricmp(key, "graph") == 0) {
            PrimitiveTemplate_ParseChannelCurve(_this, pair, channelId);
            parsed = 1;
        } else if (stricmp(key, "scale") == 0) {
            if (!PrimitiveTemplate_ParseChannelRanges(topValue, 1,
                    &((PrimitiveTemplate *)_this)->mFxChannels[channelId].scaleRange))
                return 0;
            parsed = 1;
        }
    }

    return parsed;
}

Bool PrimitiveTemplate_ParsePrimitiveInternal(const PrimitiveTemplate *_this, BackCompatibleParameters *backCompatibleParameters, GPGroup *grp)
{
    byte *thisPtr = (byte *)_this;
    PrimitiveTemplate *pt = (PrimitiveTemplate *)_this;
    GPValue *pairs;
    const char *key;
    const char *val;
    float minVal, maxVal;
    float minV[3], maxV[3];
    int n;

    pairs = GPV_LIST(grp);

    memset(backCompatibleParameters, 0, 0x600);
    {
        int channelId;
        byte *bcp = (byte *)backCompatibleParameters;
        for (channelId = 0; channelId < 24; channelId++) {
            byte *chan = bcp + channelId * 0x40;
            int d;
            for (d = 0; d < 3; d++) {

                *(float *)(chan + d * 8 + 0x00) = 1.0f;
                *(float *)(chan + d * 8 + 0x04) = 1.0f;

                *(float *)(chan + 0x18 + d * 8 + 0x00) = 1.0f;
                *(float *)(chan + 0x18 + d * 8 + 0x04) = 1.0f;

                *(float *)(chan + 0x30) = 1.0f;
                *(float *)(chan + 0x34) = 1.0f;
            }
        }
    }

    while (pairs) {
        key = GPV_STRING(pairs);
        val = GPValue_GetTopValue(pairs);

        if (stricmp(key, "count") == 0) {
            if (!ParseFloatRange(val, &minVal, &maxVal))
                goto error_key;
            pt->mSpawnCount.mMin = minVal;
            pt->mSpawnCount.mMax = maxVal;
        }

        else if (stricmp(key, "shaders") == 0 || stricmp(key, "shader") == 0) {
            if (!PrimitiveTemplate_ParseMaterials(_this, pairs))
                goto error_key;
        }

        else if (stricmp(key, "models") == 0 || stricmp(key, "model") == 0) {
            if (!PrimitiveTemplate_ParseModels(_this, pairs))
                goto error_key;
        }

        else if (stricmp(key, "impactfx") == 0) {
            if (!PrimitiveTemplate_ParseImpactFxStrings(_this, pairs))
                goto error_key;
        }

        else if (stricmp(key, "deathfx") == 0) {
            if (!PrimitiveTemplate_ParseDeathFxStrings(_this, pairs))
                goto error_key;
        }

        else if (stricmp(key, "emitfx") == 0) {
            if (!PrimitiveTemplate_ParseEmitterFxStrings(_this, pairs))
                goto error_key;
        }

        else if (stricmp(key, "playfx") == 0) {
            if (!PrimitiveTemplate_ParsePlayFxStrings(_this, pairs))
                goto error_key;
        }

        else if (stricmp(key, "life") == 0) {
            if (!ParseFloatRange(val, &minVal, &maxVal))
                goto error_key;

            pt->mLife.mMin = maxVal;
            pt->mLife.mMax = minVal;
        }

        else if (stricmp(key, "cullrange") == 0) {
            pt->mSpawnRange.mMax = (float)atof(val);
        }

        else if (stricmp(key, "spawnRange") == 0) {
            if (!ParseFloatRange(val, &minVal, &maxVal))
                goto error_key;
            if (minVal > maxVal)
                goto error_key;
            pt->mSpawnRange.mMin = minVal;
            pt->mSpawnRange.mMax = maxVal;
        }

        else if (stricmp(key, "delay") == 0) {
            if (!ParseFloatRange(val, &minVal, &maxVal))
                goto error_key;

            pt->mSpawnDelay.mMin = maxVal;
            pt->mSpawnDelay.mMax = minVal;
        }

        else if (stricmp(key, "bounce") == 0 || stricmp(key, "intensity") == 0) {
            if (!ParseFloatRange(val, &minVal, &maxVal))
                goto error_key;
            if (minVal > maxVal)
                goto error_key;
            if (minVal < 0.0f)
                goto error_key;
            if (minVal > 1.0f)
                goto error_key;
            if (0.0f > maxVal)
                goto error_key;
            if (maxVal > 1.0f)
                goto error_key;
            pt->mElasticity.mMin = minVal;
            pt->mElasticity.mMax = maxVal;
            pt->mAttributeFlags |= 0x20;
        }

        else if (stricmp(key, "min") == 0) {
            n = sscanf(val, "%f %f %f   %f %f %f",
                       &minV[0], &minV[1], &minV[2],
                       &maxV[0], &maxV[1], &maxV[2]);
            if (n <= 2 || n == 4 || n == 5)
                goto error_key;
            pt->mMin[0] = minV[0];
            pt->mMin[1] = minV[1];
            pt->mMin[2] = minV[2];
            pt->mAttributeFlags |= 0x60;
        }

        else if (stricmp(key, "max") == 0) {
            n = sscanf(val, "%f %f %f   %f %f %f",
                       &maxV[0], &maxV[1], &maxV[2],
                       &minV[0], &minV[1], &minV[2]);
            if (n <= 2 || n == 4 || n == 5)
                goto error_key;
            pt->mMax[0] = maxV[0];
            pt->mMax[1] = maxV[1];
            pt->mMax[2] = maxV[2];
            pt->mAttributeFlags |= 0x60;
        }

        else if (stricmp(key, "angle") == 0 || stricmp(key, "angles") == 0) {
            n = ParseVec3Range(val, minV, maxV);
            if (!n)
                goto error_key;

            pt->mAngle1.mMin = maxV[0];
            pt->mAngle1.mMax = minV[0];
            pt->mAngle2.mMin = maxV[1];
            pt->mAngle2.mMax = minV[1];
            pt->mAngle3.mMin = maxV[2];
            pt->mAngle3.mMax = minV[2];
        }

        else if (stricmp(key, "angleDelta") == 0) {
            n = ParseVec3Range(val, minV, maxV);
            if (!n)
                goto error_key;
            pt->mAngle1Delta.mMin = minV[0];
            pt->mAngle1Delta.mMax = maxV[0];
            pt->mAngle2Delta.mMin = minV[1];
            pt->mAngle2Delta.mMax = maxV[1];
            pt->mAngle3Delta.mMin = minV[2];
            pt->mAngle3Delta.mMax = maxV[2];
        }

        else if (stricmp(key, "velocity") == 0 || stricmp(key, "vel") == 0) {
            if (!PrimitiveTemplate_ParseVelocity(_this, val))
                goto error_key;
        }

        else if (stricmp(key, "acceleration") == 0 || stricmp(key, "accel") == 0) {
            if (!PrimitiveTemplate_ParseAcceleration(_this, val))
                goto error_key;
        }

        else if (stricmp(key, "gravity") == 0) {
            if (!ParseFloatRange(val, &minVal, &maxVal))
                goto error_key;
            pt->mGravity.mMin = minVal;
            pt->mGravity.mMax = maxVal;
        }

        else if (stricmp(key, "density") == 0) {
            if (!ParseFloatRange(val, &minVal, &maxVal))
                goto error_key;

            pt->mDensity.mMin = maxVal;
            pt->mDensity.mMax = minVal;
        }

        else if (stricmp(key, "variance") == 0) {
            if (!ParseFloatRange(val, &minVal, &maxVal))
                goto error_key;

            pt->mVariance.mMin = maxVal;
            pt->mVariance.mMax = minVal;
        }

        else if (stricmp(key, "origin") == 0) {
            n = ParseVec3Range(val, minV, maxV);
            if (!n)
                goto error_key;
            pt->mOrigin1X.mMin = minV[0];
            pt->mOrigin1X.mMax = maxV[0];
            pt->mOrigin1Y.mMin = minV[1];
            pt->mOrigin1Y.mMax = maxV[1];
            pt->mOrigin1Z.mMin = minV[2];
            pt->mOrigin1Z.mMax = maxV[2];
        }

        else if (stricmp(key, "origin2") == 0) {
            n = ParseVec3Range(val, minV, maxV);
            if (!n)
                goto error_key;

            pt->mOrigin2X.mMin = maxV[0];
            pt->mOrigin2X.mMax = minV[0];
            pt->mOrigin2Y.mMin = maxV[1];
            pt->mOrigin2Y.mMax = minV[1];
            pt->mOrigin2Z.mMin = maxV[2];
            pt->mOrigin2Z.mMax = minV[2];
        }

        else if (stricmp(key, "radius") == 0) {
            if (!ParseFloatRange(val, &minVal, &maxVal))
                goto error_key;
            pt->mRadius.mMin = minVal;
            pt->mRadius.mMax = maxVal;
        }

        else if (stricmp(key, "height") == 0) {
            if (!ParseFloatRange(val, &minVal, &maxVal))
                goto error_key;

            pt->mHeight.mMin = maxVal;
            pt->mHeight.mMax = minVal;
        }

        else if (stricmp(key, "wind") == 0) {
            if (!ParseFloatRange(val, &minVal, &maxVal))
                goto error_key;
            pt->mWindModifier.mMin = minVal;
            pt->mWindModifier.mMax = maxVal;
        }

        else if (stricmp(key, "rotation") == 0) {
            if (!ParseFloatRange(val, &minVal, &maxVal))
                goto error_key;
            pt->mRotation.mMin = minVal;
            pt->mRotation.mMax = maxVal;
        }

        else if (I_stricmp(key, "rotationDelta") == 0) {
            if (!PrimitiveTemplate_ParseRotationDelta(_this, val))
                goto error_key;
        }

        else if (stricmp(key, "flags") == 0 || stricmp(key, "flag") == 0) {
            if (!PrimitiveTemplate_ParseFlags(_this, val, fxAttributeFlags, 27))
                goto error_key;
        }

        else if (stricmp(key, "spawnFlags") == 0 || stricmp(key, "spawnFlag") == 0) {
            if (!PrimitiveTemplate_ParseFlags(_this, val, fxSpawnFlags, 13))
                goto error_key;
        }

        else if (stricmp(key, "nonUniformScale") == 0) {
            pt->mNonUniformScale = (atoi(val) != 0);
        }

        else if (stricmp(key, "useLength") == 0) {
            pt->useLength = (atoi(val) != 0);
        }

        else if (stricmp(key, "name") == 0) {
            if (!val)
                goto error_key;
            I_strncpyz(pt->mName, val, 0x20);
        }

        else if (stricmp(key, "shaderImpact") == 0) {
            I_strncpyz(pt->mMaterialImpact, val, 0x20);
        }

        else if (stricmp(key, "sequenceStartFrameMode") == 0) {
            n = atoi(val);
            if ((unsigned int)n > 2)
                goto error_key;
            pt->mSequenceStartFrameMode = n;
        }

        else if (stricmp(key, "sequenceFixedFrameValue") == 0) {
            n = atoi(val);
            pt->mSequenceFixedFrameValue = n;
            if (n <= 0)
                goto error_key;
        }

        else if (stricmp(key, "sequencePlayRateMode") == 0) {
            n = atoi(val);
            if ((unsigned int)n > 1)
                goto error_key;
            pt->mSequencePlayRateMode = n;
        }

        else if (stricmp(key, "sequenceFixedFpsValue") == 0) {
            float fpsVal = (float)atof(val);
            pt->mSequenceFixedFpsValue = fpsVal;
            if (fpsVal < 0.0f)
                goto error_key;
        }

        else if (stricmp(key, "sequenceLoopMode") == 0) {
            n = atoi(val);
            if ((unsigned int)n > 1)
                goto error_key;
            pt->mSequenceLoopMode = n;
        }

        else if (stricmp(key, "sequenceLoopTimes") == 0) {
            n = atoi(val);
            pt->mSequenceLoopTimes = n;
            if (n < 0)
                goto error_key;
        }

        else if (stricmp(key, "spawnFrustumCullRadius") == 0) {
            float radius = (float)atof(val);
            pt->spawnFrustumCullRadius = radius;
            if (radius < 0.0f)
                goto error_key;
        }

        else if (stricmp(key, "rgb") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 0, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "rgb2") == 0 || stricmp(key, "rgbRand") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 1, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "alpha") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 2, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "alphaRand") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 3, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "size") == 0 || stricmp(key, "width") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 4, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "sizeRand") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 5, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "size2") == 0 || stricmp(key, "width2") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 6, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "size2Rand") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 7, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "length") == 0 || stricmp(key, "height") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 8, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "lengthRand") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 9, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "rotationDelta") == 0) {

            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 10, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "rotationDeltaRand") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 11, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "velocityX") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 12, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "velocityY") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 13, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "velocityZ") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 14, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "velocityXRand") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 15, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "velocityYRand") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 16, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "velocityZRand") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 17, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "velocity2X") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 18, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "velocity2Y") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 19, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "velocity2Z") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 20, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "velocity2XRand") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 21, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "velocity2YRand") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 22, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else if (stricmp(key, "velocity2ZRand") == 0) {
            if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)pairs, 23, 0, 0, 0, 0, 0, 0, 0, 0))
                goto check_key_error;
        } else {
            FX_Print("Unknown key parsing an effect primitive: %s\n", key);
            return 0;
        }

        pairs = GPV_NEXT(pairs);
        continue;

    check_key_error:
        if (key) {
            FX_Print("^1FX Error while parsing key '%s'\n", key);
            return 0;
        }

        goto post_parse;
    error_key:
        if (key) {
            FX_Print("^1FX Error while parsing key '%s'\n", key);
            return 0;
        }
        goto post_parse;
    }

post_parse:

    {
        GPValue *sub = GPG_SUBGROUPS(grp);
        while (sub) {
            const char *subKey = GPV_STRING(sub);

            if (stricmp(subKey, "rgb") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 0, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "rgb2") == 0 || stricmp(subKey, "rgbRand") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 1, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "alpha") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 2, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "alphaRand") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 3, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "size") == 0 || stricmp(subKey, "width") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 4, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "sizeRand") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 5, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "size2") == 0 || stricmp(subKey, "width2") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 6, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "size2Rand") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 7, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "length") == 0 || stricmp(subKey, "height") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 8, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "lengthRand") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 9, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "rotationDelta") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 10, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "rotationDeltaRand") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 11, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "velocityX") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 12, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "velocityY") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 13, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "velocityZ") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 14, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "velocityXRand") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 15, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "velocityYRand") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 16, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "velocityZRand") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 17, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "velocity2X") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 18, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "velocity2Y") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 19, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "velocity2Z") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 20, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "velocity2XRand") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 21, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "velocity2YRand") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 22, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else if (stricmp(subKey, "velocity2ZRand") == 0) {
                if (!PrimitiveTemplate_ParseChannel(_this, backCompatibleParameters, (GPGroup *)sub, 23, 0, 0, 0, 0, 0, 0, 0, 0)) {
                    if (subKey) {
                        FX_Print("^1FX Error while parsing key '%s'\n", subKey);
                        return 0;
                    }
                    goto do_migration;
                }
            } else {
                FX_Print("Unknown group key parsing a particle: %s\n", subKey);
                return 0;
            }

            sub = GPV_NEXT(sub);
        }
    }

do_migration:

    {
        int channelId;

        byte *bcp = (byte *)backCompatibleParameters;
        FxChannel *channels = (FxChannel *)&((PrimitiveTemplate *)thisPtr)->mFxChannels[0].curve;
        float *lifePtr = (float *)&((PrimitiveTemplate *)thisPtr)->mLife.mMin;

        for (channelId = 0; channelId < 24; channelId++) {
            FxChannelBackwardCompatible *bc = (FxChannelBackwardCompatible *)(bcp + channelId * 0x40);
            if (!bc->containsData)
                continue;

            int dimensions;
            int forceUnitScale;
            if (channelId == 2) {

                dimensions = 1;
                forceUnitScale = 1;
            } else if (channelId == 0) {

                dimensions = 3;
                forceUnitScale = 0;
            } else if (channelId == 1) {

                dimensions = 1;
                forceUnitScale = 0;
            } else {
                dimensions = 1;
                forceUnitScale = 0;
            }

            float lifetime = lifePtr[0];
            float lifeMax = lifePtr[1];
            float lifeMid = lifetime + (lifeMax - lifetime) * 0.5f;

            FxChannel_CreateViaMigration(bc, dimensions, lifeMid, forceUnitScale, &channels[channelId]);
        }
    }

    {
        float minX = pt->mMin[0];
        float maxX = pt->mMax[0];
        if (minX > maxX) {
            FX_Print("^1FX bounding box mins / maxs invalid for effect '%s'\n", _this);
            return 0;
        }
        float minY = pt->mMin[1];
        float maxY = pt->mMax[1];
        if (minY > maxY) {
            FX_Print("^1FX bounding box mins / maxs invalid for effect '%s'\n", _this);
            return 0;
        }
        float minZ = pt->mMin[2];
        float maxZ = pt->mMax[2];
        if (minZ > maxZ) {
            FX_Print("^1FX bounding box mins / maxs invalid for effect '%s'\n", _this);
            return 0;
        }

        float rangeZ = maxZ - minZ;
        float rangeX = maxX - minX;
        float rangeY = maxY - minY;
        if (rangeX > rangeZ || rangeY > rangeZ) {
            FX_Print("^1FX bounding box width or depth is larger than height for effect '%s'\n", _this);
            return 0;
        }
    }

    return 1;
}
