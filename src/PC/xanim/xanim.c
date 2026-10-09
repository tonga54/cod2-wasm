#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <string.h>
#include <stddef.h>

extern const dvar_t *com_developer;

#define XANIM_CALC_ABI COD2_REGPARM(3) COD2_SSEREGPARM

static XAnimInfo g_xAnimInfo[4096];
static unsigned int g_end;
extern void SL_RemoveRefToString(unsigned int stringValue);
extern void SL_AddRefToString(unsigned int stringValue);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern void Scr_AddConstString(unsigned int value);
extern void Scr_NotifyNum(int entnum, int classnum, unsigned int stringValue, unsigned int paramcount);
static XAnimNotify g_notifyList[128];
static int g_notifyListSize;
static Bool g_anim_developer;

extern void *Hunk_AllocAlignInternal(int size, int align);
extern unsigned int SL_GetString_(const char *str, int user, int type);
extern void SL_RemoveRefToStringOfLen(unsigned int stringValue, int len);
extern unsigned int SL_GetStringOfLen(const void *str, int user, unsigned int len, int type);
extern int XModelGetBoneIndex(const XModel *model, unsigned int name);
extern void *Z_MallocInternal(int size);
extern void Z_FreeInternal(void *ptr);
extern qboolean Hunk_DataOnHunk(void *data);
extern void Hunk_AddData(int type, void *data, void *alloc);
extern const char *va(const char *format, ...);
extern void *Hunk_FindDataForFile(int type, const char *name);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern XAnimParts *XAnimLoadFile(const char *name, Alloc_t Alloc);
extern const char *Hunk_SetDataForFile(int type, const char *name, void *data, Alloc_t Alloc);

static inline __attribute__((always_inline)) void XAnimClearInfoNotify(XAnimInfo *info)
{
    if (info->notifyName) {
        SL_RemoveRefToString(info->notifyName);
        info->notifyName = 0;
    }

    info->notifyIndex = -1;
}

static inline __attribute__((always_inline)) void XAnimFreeInfoToList(unsigned int infoIndex)
{
    XAnimInfo *info = &g_xAnimInfo[infoIndex];

    XAnimClearInfoNotify(info);
    info->prev = 0;
    info->next = g_xAnimInfo[0].next;
    g_xAnimInfo[info->next].prev = (unsigned short)infoIndex;
    g_xAnimInfo[0].next = (unsigned short)infoIndex;
}

static inline __attribute__((always_inline)) XAnimInfo *XAnimAllocInfoForTree(XAnimTree_s *tree, unsigned int animIndex)
{
    unsigned int infoIndex = g_xAnimInfo[0].next;

    if (!infoIndex) {
        Com_Error(1, "\x15"
                     "exceeded maximum number of anim info");
        return &g_xAnimInfo[0];
    }

    g_xAnimInfo[0].next = g_xAnimInfo[infoIndex].next;
    g_xAnimInfo[g_xAnimInfo[0].next].prev = 0;
    tree->infoArray[animIndex] = (unsigned short)infoIndex;

    return &g_xAnimInfo[infoIndex];
}

void XAnimInit(void);
void XAnimShutdown(void);
void XAnimAbort(void);
void XAnimFree(XAnimParts *parts);
void XAnimBlend(XAnim *anims, unsigned int animIndex, const char *name, unsigned int children, unsigned int num, unsigned int flags);
XAnim *XAnimCreateAnims(const char *debugName, int size, Alloc_t Alloc);
void XAnimFreeList(XAnim *anims);
XAnim *XAnimGetAnims(const XAnimTree *tree);
float XAnimGetLength(const XAnim *anims, unsigned int animIndex);
float XAnimGetTime(const XAnimTree *tree, unsigned int animIndex);
float XAnimGetWeight(const XAnimTree *tree, unsigned int animIndex);
Bool XAnimHasFinished(const XAnimTree *tree, unsigned int animIndex);
int XAnimGetNumChildren(const XAnim *anims, unsigned int animIndex);
unsigned int XAnimGetChildAt(const XAnim *anims, unsigned int animIndex, unsigned int childIndex);
const char *XAnimGetAnimName(const XAnim *anims, unsigned int animIndex);
const char *XAnimGetAnimTreeDebugName(const XAnim *anims);
unsigned int XAnimGetAnimTreeSize(const XAnim *anims);
int DObjGetClientNotifyList(XAnimNotify **notifyList);
static void *Hunk_AllocXAnimPrecache(int size);
Bool XAnimIsPrimitive(XAnim *anims, unsigned int animIndex);
void XAnimSetTime(XAnimTree *tree, unsigned int animIndex, float time);
void XAnimSetAnimRate(XAnimTree *tree, unsigned int animIndex, float rate);
int XAnimIsLooped(const XAnim *anims, unsigned int animIndex);
Bool XAnimNotetrackExists(const XAnim *anims, unsigned int animIndex, unsigned int name);
static void __attribute_regparm__(3) Z18XAnim_GetTimeIndexIhEvPK9XAnimTimePK19XAnimDynamicIndicesiPiPf(const XAnimTime *animTime, const XAnimDynamicIndices *indices, int tableSize, int *keyFrameIndex, float *keyFrameLerpFrac);
static void __attribute_regparm__(3) Z28XAnim_GetTimeIndexCompressedItEvPK9XAnimTimePKT_iPiPf(const XAnimTime *animTime, const unsigned short *indices, int tableSize, int *keyFrameIndex, float *keyFrameLerpFrac);
const char *XAnimGetAnimDebugName(const XAnim *anims, unsigned int animIndex);
XAnimTree *XAnimCreateTree(XAnim *anims, Alloc_t Alloc);
XAnimParts *XAnimPrecache(const char *name, Alloc_t Alloc);
void XAnimCreate(XAnim *anims, unsigned int animIndex, const char *name);
static unsigned int __attribute_regparm__(2) XAnimGetDescendantWithGreatestWeight(const XAnimTree *tree, unsigned int animIndex);
int XAnimGetLengthMsec(const XAnim *anims, unsigned int anim);
void XAnimClearGoalWeight(XAnimTree *tree, unsigned int animIndex, float blendTime);
static void __attribute_regparm__(3) __attribute_sseregparm__ XAnim_CalcDeltaForTime(const XAnimParts *parts, vec_t *rotDelta, vec_t *posDelta, float time);
void XAnimGetAbsDelta(const XAnim *anims, unsigned int animIndex, vec_t *rot, vec_t *trans, float time);
static void __attribute_regparm__(3) __attribute_sseregparm__ XAnimCalcRelDeltaParts(const XAnimParts *parts, XAnimSimpleRotPos *rotPos, int quatIndex, float weightScale, float time1, float time2);
void XAnimGetRelDelta(const XAnim *anims, unsigned int animIndex, vec_t *rot, vec_t *trans, float time1, float time2);
static void __attribute_regparm__(2) XAnimClearTreeWeights(XAnimTree *tree, unsigned int animIndex);
void __attribute_regparm__(3) __attribute_sseregparm__ XAnimCalcDeltaTree(const XAnimTree *tree, unsigned int animIndex, float weightScale, const Bool bClear, Bool bNormQuat, XAnimSimpleRotPos *rotPos);
void XAnimCalcAbsDelta(XAnimTree *tree, unsigned int animIndex, vec_t *rot, vec_t *trans);
void XAnimCalcDelta(XAnimTree *tree, unsigned int animIndex, vec_t *rot, vec_t *trans, int bUseGoalWeight);
static void __attribute_regparm__(2) XAnimResetTime(XAnimTree *tree, unsigned int animIndex);
static void __attribute_regparm__(3) __attribute_sseregparm__ XAnimUpdateOldTime(XAnimTree *tree, unsigned int animIndex, XAnimState *syncState, Bool parentHadWeight, Bool *infoExistsForParent, Bool *childHadWeightForParent, float dtime);
void DObjInitServerTime(DObj *obj, float dtime);
static int __attribute_regparm__(3) __attribute_sseregparm__
    XAnimSetGoalWeightInternal(XAnimTree *tree, unsigned int animIndex, int bForce,
                               float goalWeight, float goalTime, float rate,
                               unsigned int notifyName, unsigned int notifyType);
void XAnimSetCompleteGoalWeight(XAnimTree *tree, unsigned int animIndex, float goalWeight, float goalTime, float rate, unsigned int notifyName, unsigned int notifyType, int bRestart);
void XAnimCloneAnimTree(const XAnimTree *from, XAnimTree *to);
void XAnimFreeTree(XAnimTree *tree, Free_t Free);
void XAnimClearTree(XAnimTree *tree);
void XANIM_CALC_ABI XAnimCalc(const DObj *obj, unsigned int animIndex, float weightScale, XAnimPart (*rotTransArray)(), int bClear, int bNormQuat, XAnimCalcAnimInfo *info, int rotTransArrayIndex);
void DObjCalcAnim(const DObj *obj, int *partBits);
static float __attribute_regparm__(2) XAnimGetAverageRateFrequency(const XAnimTree *tree, unsigned int animIndex);
void XAnimClearTreeGoalWeights(XAnimTree *tree, unsigned int animIndex, float blendTime);
void XAnimClearTreeGoalWeightsStrict(XAnimTree *tree, unsigned int animIndex, float blendTime);
static void __attribute_regparm__(3) __attribute_sseregparm__
    XAnimProcessServerNotify(const XAnimTree *tree, XAnimInfo *info, const XAnimEntry *anim, float time);
static const XAnimNotifyInfo *XAnimGetNextNotifyInfo(const XAnimParts *parts, float time);
int XAnimSetCompleteGoalWeightKnobAll(XAnimTree *tree, unsigned int animIndex, unsigned int rootIndex, float goalWeight, float goalTime, float rate, unsigned int notifyName, int bRestart);
int XAnimSetGoalWeight(XAnimTree *tree, unsigned int animIndex, float goalWeight, float goalTime, float rate, unsigned int notifyName, unsigned int notifyType, int bRestart);
static void __attribute_regparm__(3) XAnimDisplay(const XAnimTree *tree, unsigned int animIndex, int depth);
void DObjDisplayAnim(DObj *obj);
static void __attribute_regparm__(3) XAnimFillInSyncNodes_r(XAnim *anims, unsigned int animIndex, Bool bLoop);
static void __attribute_regparm__(2) XAnimSetupSyncNodes_r(XAnim *anims, unsigned int animIndex);
void XAnimSetupSyncNodes(XAnim *anims);
static void __attribute_regparm__(2) __attribute_sseregparm__ XAnimProcessClientNotify(XAnimInfo *info, const XAnimEntry *anim, float dtime);
static void __attribute_regparm__(3) __attribute_sseregparm__ XAnimUpdateInfoSyncInternal(const XAnimTree *tree, unsigned int animIndex, Bool bNotify, XAnimState *syncState, float dtime);
static void __attribute_regparm__(3) __attribute_sseregparm__ XAnimUpdateInfoInternal(const XAnimTree *tree, float dtime, unsigned int animIndex, Bool bNotify);
void DObjUpdateClientInfo(DObj *obj, float dtime);
static float __attribute_regparm__(3) __attribute_sseregparm__ XAnimGetNotifyFracServer(const XAnimTree *tree, XAnimInfo *info, const XAnimEntry *anim, const XAnimState *syncState, const XAnimState *nextSyncState, float dtime);
static float __attribute_regparm__(3) __attribute_sseregparm__ XAnimGetServerNotifyFracSyncTotal(const XAnimTree *tree, XAnimInfo *info, const XAnimEntry *anim, const XAnimState *syncState, const XAnimState *nextSyncState, float dtime);
static float __attribute_regparm__(3) __attribute_sseregparm__ XAnimFindServerNoteTrack(const XAnimTree *tree, unsigned int animIndex, float dtime);
int DObjUpdateServerInfo(DObj *obj, float dtime, int bNotify);

void XAnimInit(void)
{
    int i;
    for (i = 0; i < 4096; i++) {
        g_xAnimInfo[i].prev = (unsigned short)((i + 4095) % 4096);
        g_xAnimInfo[i].next = (unsigned short)((i + 1) % 4096);
    }
    g_xAnimInfo[0].s.time = 0.0f;
    g_xAnimInfo[0].s.oldTime = 0.0f;
    g_xAnimInfo[0].s.timeCount = 0;
    g_xAnimInfo[0].s.oldTimeCount = 0;
    g_end = SL_GetString_("end", 0, 3);
    g_anim_developer = com_developer->current.integer != 0;
}

void XAnimShutdown(void)
{
    if (!g_end)
        return;
    SL_RemoveRefToString(g_end);
    g_end = 0;
}

void XAnimAbort(void)
{
    g_end = 0;
}

void XAnimFree(XAnimParts *parts)
{
    unsigned short *boneNames = parts->names;
    int boneCount = parts->boneCount;
    int i;

    if (boneCount > 0) {
        for (i = 0; i < boneCount; i++) {
            SL_RemoveRefToString(boneNames[i]);
        }
    }

    if (parts->notifyCount) {
        XAnimNotifyInfo *notify = parts->notify;
        int notifyCount = parts->notifyCount;
        for (i = 0; i < notifyCount; i++) {
            SL_RemoveRefToString(*(unsigned short *)((byte *)notify + i * 8));
        }
    }
}

void XAnimBlend(XAnim *anims, unsigned int animIndex, const char *name, unsigned int children, unsigned int num, unsigned int flags)
{
    unsigned int j;

    anims->entries[animIndex].numAnims = (unsigned short)num;
    anims->entries[animIndex].u.s.flags = (unsigned short)flags;
    anims->entries[animIndex].u.s.children = (unsigned short)children;

    for (j = 0; j < num; j++) {
        anims->entries[anims->entries[animIndex].u.s.children + j].parent = (unsigned short)animIndex;
    }

    if (anims->debugAnimNames) {
        int len = strlen(name) + 1;
        char *nameCopy = (char *)Z_MallocInternal(len);
        strcpy(nameCopy, name);
        ((char **)anims->debugAnimNames)[animIndex] = nameCopy;
    }
}

XAnim *XAnimCreateAnims(const char *debugName, int size, Alloc_t Alloc)
{
    /* 0xc + size*8 was the x86 layout (header 12, XAnimEntry 8). On x64 the union pointer grows
       both; size by offsetof/sizeof so it is correct on each arch. */
    XAnim *anims = (XAnim *)Alloc((int)(offsetof(XAnim, entries) + (size_t)size * sizeof(XAnimEntry)));
    anims->size = size;

    if (g_anim_developer) {
        int len = strlen(debugName) + 1;
        char *nameCopy = (char *)Z_MallocInternal(len);
        strcpy(nameCopy, debugName);
        anims->debugName = nameCopy;
        /* size*4 assumed 4-byte pointers; on x64 these are 8 bytes. */
        anims->debugAnimNames = (const char **)Z_MallocInternal((int)((size_t)size * sizeof(char *)));
    }

    if (Hunk_DataOnHunk(anims)) {
        Hunk_AddData(6, anims, (void *)Alloc);
    }
    return anims;
}

void XAnimFreeList(XAnim *anims)
{
    unsigned int i;

    if (anims->debugName) {
        Z_FreeInternal((void *)anims->debugName);
        anims->debugName = NULL;
    }

    if (anims->debugAnimNames) {
        for (i = 0; i < anims->size; ++i) {
            if (anims->debugAnimNames[i]) {
                Z_FreeInternal((void *)anims->debugAnimNames[i]);
                anims->debugAnimNames[i] = NULL;
            }
        }

        Z_FreeInternal((void *)anims->debugAnimNames);
        anims->debugAnimNames = NULL;
    }
}

XAnim *XAnimGetAnims(const XAnimTree *tree)
{
    return *(XAnim **)tree;
}

float XAnimGetLength(const XAnim *anims, unsigned int animIndex)
{
    const XAnimParts *parts = anims->entries[animIndex].u.parts;
    return (float)parts->numframes / parts->framerate;
}

float XAnimGetTime(const XAnimTree *tree, unsigned int animIndex)
{
    unsigned short info = ((const XAnimTree_s *)tree)->infoArray[animIndex];
    if (!info)
        return 0.0f;
    return g_xAnimInfo[info].s.time;
}

float XAnimGetWeight(const XAnimTree *tree, unsigned int animIndex)
{
    unsigned short info = ((const XAnimTree_s *)tree)->infoArray[animIndex];
    if (!info)
        return 0.0f;
    return g_xAnimInfo[info].s.weight;
}

Bool XAnimHasFinished(const XAnimTree *tree, unsigned int animIndex)
{
    unsigned short infoIndex;
    XAnimInfo *info;

    infoIndex = ((const XAnimTree_s *)tree)->infoArray[animIndex];
    if (!infoIndex) {
        return 1;
    }

    info = &g_xAnimInfo[infoIndex];
    {
        const XAnimState *s = &info->s;
        if (s->oldTime > s->time) {
            return 1;
        }
        if (s->time == 1.0f) {
            return 1;
        }
        if (s->timeCount > s->oldTimeCount) {
            return 1;
        }
    }

    return 0;
}

int XAnimGetNumChildren(const XAnim *anims, unsigned int animIndex)
{
    return anims->entries[animIndex].numAnims;
}

unsigned int XAnimGetChildAt(const XAnim *anims, unsigned int animIndex, unsigned int childIndex)
{
    return anims->entries[animIndex].u.s.children + childIndex;
}

const char *XAnimGetAnimName(const XAnim *anims, unsigned int animIndex)
{
    const XAnimEntry *entry = &anims->entries[animIndex];
    if (entry->numAnims != 0)
        return (const char *)"";
    return entry->u.parts->name;
}

const char *XAnimGetAnimTreeDebugName(const XAnim *anims)
{
    return *(const char **)anims;
}

unsigned int XAnimGetAnimTreeSize(const XAnim *anims)
{
    return anims->size;
}

int DObjGetClientNotifyList(XAnimNotify **notifyList)
{
    *notifyList = g_notifyList;
    return g_notifyListSize;
}

static void *Hunk_AllocXAnimPrecache(int size)
{
    return Hunk_AllocAlignInternal(size, 4);
}

Bool XAnimIsPrimitive(XAnim *anims, unsigned int animIndex)
{
    return anims->entries[animIndex].numAnims == 0;
}

void XAnimSetTime(XAnimTree *tree, unsigned int animIndex, float time)
{
    unsigned short infoIndex = ((XAnimTree_s *)tree)->infoArray[animIndex];
    XAnimInfo *info;

    if (!infoIndex) {
        return;
    }

    info = &g_xAnimInfo[infoIndex];
    {
        XAnimState *s = &info->s;
        s->time = time;
        s->timeCount = 0;
        s->oldTime = time;
        s->oldTimeCount = 0;
    }
    info->notifyIndex = -1;
}

void XAnimSetAnimRate(XAnimTree *tree, unsigned int animIndex, float rate)
{
    unsigned short index = ((XAnimTree_s *)tree)->infoArray[animIndex];
    g_xAnimInfo[index].s.rate = rate;
}

/* Returns int (not Bool): several callers declare it `extern int` and read the
 * full eax. A char/Bool return only sets al and leaves eax's upper bits dirty
 * (UB the GCC build tolerated, MSVC does not) -> a not-looped anim read as
 * "looped" and spuriously failed CG_RegisterWeapon's ADS check. int forces the
 * compiler to zero-extend into eax; the value logic is unchanged. */
int XAnimIsLooped(const XAnim *anims, unsigned int animIndex)
{
    /* was hardcoded x86 layout (entries at +0xc, 8-byte stride, union at +4); on x64 XAnimEntry
       is 16B with the union at +8 and entries[] at +24 -> use the typed struct (correct on both). */
    const XAnimEntry *e = &anims->entries[animIndex];
    if (e->numAnims != 0)
        return e->u.s.flags & 1;
    return e->u.parts->bLoop;
}

Bool XAnimNotetrackExists(const XAnim *anims, unsigned int animIndex, unsigned int name)
{
    const XAnimParts *parts;
    const XAnimNotifyInfo *notify;
    int i;

    parts = anims->entries[animIndex].u.parts;
    notify = parts->notify;
    if (!notify) {
        return 0;
    }

    for (i = 0; i < parts->notifyCount; i++) {
        if ((unsigned short)notify[i].name == name) {
            return 1;
        }
    }

    return 0;
}

static void __attribute_regparm__(3)
    Z18XAnim_GetTimeIndexIhEvPK9XAnimTimePK19XAnimDynamicIndicesiPiPf(const XAnimTime *animTime, const XAnimDynamicIndices *indices, int tableSize, int *keyFrameIndex, float *keyFrameLerpFrac)
{
    if (animTime->frameCount <= tableSize) {
        *keyFrameLerpFrac = animTime->frameFrac - (float)animTime->frameIndex;
        *keyFrameIndex = animTime->frameIndex;
        return;
    }

    const byte *frames = indices->_1;
    const int frameIndex = animTime->frameIndex;
    int index = (int)(animTime->time * (float)tableSize);

    while (frameIndex < (int)frames[index]) {
        --index;
    }

    while (frameIndex >= (int)frames[index + 1]) {
        ++index;
    }

    *keyFrameLerpFrac = (animTime->frameFrac - (float)frames[index]) / (float)((int)frames[index + 1] - (int)frames[index]);
    *keyFrameIndex = index;
}

static void __attribute_regparm__(3)
    Z28XAnim_GetTimeIndexCompressedItEvPK9XAnimTimePKT_iPiPf(const XAnimTime *animTime, const unsigned short *indices, int tableSize, int *keyFrameIndex, float *keyFrameLerpFrac)
{
    const int frameIndex = animTime->frameIndex;
    int index = (int)(animTime->time * (float)tableSize);

    while (frameIndex < (int)indices[index]) {
        --index;
    }

    while (frameIndex >= (int)indices[index + 1]) {
        ++index;
    }

    *keyFrameLerpFrac = (animTime->frameFrac - (float)indices[index]) / (float)((int)indices[index + 1] - (int)indices[index]);
    *keyFrameIndex = index;
}

const char *XAnimGetAnimDebugName(const XAnim *anims, unsigned int animIndex)
{
    const XAnimEntry *anim;
    const char *debugName;

    anim = &anims->entries[animIndex];
    if (anims->debugAnimNames) {
        debugName = anims->debugAnimNames[animIndex];
        if (anim->numAnims == 0 && anim->u.parts->isDefault) {
            return va((const char *)"^3%s (missing)", debugName);
        }
        return debugName;
    }

    if (anim->numAnims != 0) {
        return va((const char *)"%i", animIndex);
    }

    return anim->u.parts->name;
}

XAnimTree *XAnimCreateTree(XAnim *anims, Alloc_t Alloc)
{
    unsigned int treeSize = anims->size * 5 + 9;
    XAnimTree_s *tree = (XAnimTree_s *)Alloc(treeSize);

    memset(tree, 0, treeSize);
    tree->anims = anims;
    return (XAnimTree *)tree;
}

XAnimParts *XAnimPrecache(const char *name, Alloc_t Alloc)
{
    XAnimParts *parts = (XAnimParts *)Hunk_FindDataForFile(5, name);
    XAnimParts *defaultParts;
    unsigned short notifyInfoIndex;
    int i;

    if (parts) {
        return parts;
    }

    parts = XAnimLoadFile(name, Alloc);
    if (!parts) {
        Com_Printf("^3WARNING: Couldn't find xanim '%s', using default xanim '%s' instead\n", name, "void");

        defaultParts = (XAnimParts *)Hunk_FindDataForFile(5, "void");
        if (!defaultParts) {
            defaultParts = XAnimLoadFile("void", Alloc);
            if (!defaultParts) {
                Com_Error(1, "\x15"
                             "Cannot find xanim '%s'.",
                          "void");
                return parts;
            }

            Hunk_SetDataForFile(5, "void", defaultParts, Alloc);
        }

        parts = (XAnimParts *)Alloc(0x2c);
        *parts = *defaultParts;

        for (i = 0; i < parts->boneCount; ++i) {
            SL_AddRefToString(parts->names[i]);
        }

        if (parts->notifyCount) {
            for (notifyInfoIndex = 0; notifyInfoIndex < parts->notifyCount; ++notifyInfoIndex) {
                SL_AddRefToString(parts->notify[notifyInfoIndex].name);
            }
        }

        parts->isDefault = 1;
    }

    parts->name = Hunk_SetDataForFile(5, name, parts, Alloc);
    return parts;
}

void XAnimCreate(XAnim *anims, unsigned int animIndex, const char *name)
{
    XAnimParts *parts = (XAnimParts *)Hunk_FindDataForFile(5, name);

    if (!parts) {
        Com_Error(1, "\x15"
                     "Cannot find xanim '%s'",
                  name);
        return;
    }

    {
        XAnimEntry *entry = &anims->entries[animIndex];
        entry->numAnims = 0;
        entry->u.parts = parts;
    }

    if (anims->debugAnimNames) {
        char *nameCopy = (char *)Z_MallocInternal(strlen(name) + 1);
        strcpy(nameCopy, name);
        ((char **)anims->debugAnimNames)[animIndex] = nameCopy;
    }
}

static unsigned int __attribute_regparm__(2) XAnimGetDescendantWithGreatestWeight(const XAnimTree *tree, unsigned int animIndex)
{
    const XAnimTree_s *tree_s = (const XAnimTree_s *)tree;
    const XAnimEntry *anim = &tree_s->anims->entries[animIndex];
    unsigned int numAnims = anim->numAnims;
    float bestWeight = 0.0f;
    unsigned int result = 0;
    unsigned int i;

    if (!numAnims) {
        return animIndex;
    }

    for (i = 0; i < numAnims; ++i) {
        unsigned int childIndex = anim->u.s.children + i;
        unsigned int infoIndex = tree_s->infoArray[childIndex];
        float testWeight = g_xAnimInfo[infoIndex].s.weight;

        if (bestWeight < testWeight) {
            unsigned int test = XAnimGetDescendantWithGreatestWeight(tree, childIndex);
            if (test) {
                bestWeight = testWeight;
                result = test;
            }
        }
    }

    return result;
}

int XAnimGetLengthMsec(const XAnim *anims, unsigned int anim)
{
    const XAnimParts *parts = anims->entries[anim].u.parts;
    return (int)((float)parts->numframes / parts->framerate * 1000.0f);
}

void XAnimClearGoalWeight(XAnimTree *tree, unsigned int animIndex, float blendTime)
{
    unsigned short infoIndex = ((XAnimTree_s *)tree)->infoArray[animIndex];
    XAnimInfo *info;

    if (!infoIndex) {
        return;
    }

    info = &g_xAnimInfo[infoIndex];

    {
        XAnimState *s = &info->s;

        if (s->goalWeight != 0.0f || s->goalTime > blendTime) {
            s->goalTime = blendTime;
        }

        s->goalWeight = 0.0f;
        if (blendTime == 0.0f) {
            s->weight = 0.0f;
        }
    }

    if (info->notifyName) {
        SL_RemoveRefToString(info->notifyName);
        info->notifyName = 0;
    }

    info->notifyIndex = -1;
}

static __attribute_regparm__(3) __attribute_sseregparm__
    void XAnim_CalcDeltaForTime(const XAnimParts *parts, vec_t *rotDelta, vec_t *posDelta, float time)
{
    const XAnimDeltaPart *animDelta = parts->deltaPart;
    const XAnimDeltaPartQuat *quat = animDelta->quat;
    const XAnimDeltaPartTrans *trans;
    const int frameCount = parts->numframes;
    XAnimTime animTime;
    int keyFrameIndex;
    float keyFrameLerpFrac;

    if (time == 1.0f || frameCount == 0) {
        if (quat) {
            if (quat->size) {
                const XQuat2 *frame = &quat->u.frames.frames[quat->size];
                rotDelta[0] = (float)(*frame)[0];
                rotDelta[1] = (float)(*frame)[1];
            } else {
                rotDelta[0] = (float)quat->u.frame0[0];
                rotDelta[1] = (float)quat->u.frame0[1];
            }
        } else {
            rotDelta[0] = 0.0f;
            rotDelta[1] = 32767.0f;
        }

        trans = animDelta->trans;
        if (trans) {
            if (trans->size) {
                const vec_t *frame = trans->u.frames.frames[trans->size];
                posDelta[0] = frame[0];
                posDelta[1] = frame[1];
                posDelta[2] = frame[2];
            } else {
                posDelta[0] = trans->u.frame0[0];
                posDelta[1] = trans->u.frame0[1];
                posDelta[2] = trans->u.frame0[2];
            }
        } else {
            posDelta[0] = 0.0f;
            posDelta[1] = 0.0f;
            posDelta[2] = 0.0f;
        }
        return;
    }

    if (quat) {
        if (quat->size) {
            const XQuat2 *frames = quat->u.frames.frames;
            animTime.time = time;
            animTime.frameCount = frameCount;
            animTime.frameFrac = (float)frameCount * time;
            animTime.frameIndex = (int)animTime.frameFrac;

            if (frameCount > 255) {
                const unsigned short *indices = quat->u.frames.indices._2;
                if ((int)quat->size < frameCount) {
                    Z28XAnim_GetTimeIndexCompressedItEvPK9XAnimTimePKT_iPiPf(&animTime, indices, quat->size, &keyFrameIndex, &keyFrameLerpFrac);
                } else {
                    keyFrameLerpFrac = animTime.frameFrac - (float)animTime.frameIndex;
                    keyFrameIndex = animTime.frameIndex;
                }
            } else {
                Z18XAnim_GetTimeIndexIhEvPK9XAnimTimePK19XAnimDynamicIndicesiPiPf(&animTime, &quat->u.frames.indices, quat->size, &keyFrameIndex, &keyFrameLerpFrac);
            }

            rotDelta[0] = (float)frames[keyFrameIndex][0] + ((float)frames[keyFrameIndex + 1][0] - (float)frames[keyFrameIndex][0]) * keyFrameLerpFrac;
            rotDelta[1] = (float)frames[keyFrameIndex][1] + ((float)frames[keyFrameIndex + 1][1] - (float)frames[keyFrameIndex][1]) * keyFrameLerpFrac;
        } else {
            rotDelta[0] = (float)quat->u.frame0[0];
            rotDelta[1] = (float)quat->u.frame0[1];
        }
    } else {
        rotDelta[0] = 0.0f;
        rotDelta[1] = 32767.0f;
    }

    trans = animDelta->trans;
    if (trans) {
        if (trans->size) {
            const vec3_t *frames = trans->u.frames.frames;
            animTime.time = time;
            animTime.frameCount = frameCount;
            animTime.frameFrac = (float)frameCount * time;
            animTime.frameIndex = (int)animTime.frameFrac;

            if (frameCount > 255) {
                const unsigned short *indices = trans->u.frames.indices._2;
                if ((int)trans->size < frameCount) {
                    Z28XAnim_GetTimeIndexCompressedItEvPK9XAnimTimePKT_iPiPf(&animTime, indices, trans->size, &keyFrameIndex, &keyFrameLerpFrac);
                } else {
                    keyFrameLerpFrac = animTime.frameFrac - (float)animTime.frameIndex;
                    keyFrameIndex = animTime.frameIndex;
                }
            } else {
                Z18XAnim_GetTimeIndexIhEvPK9XAnimTimePK19XAnimDynamicIndicesiPiPf(&animTime, &trans->u.frames.indices, trans->size, &keyFrameIndex, &keyFrameLerpFrac);
            }

            posDelta[0] = frames[keyFrameIndex][0] + (frames[keyFrameIndex + 1][0] - frames[keyFrameIndex][0]) * keyFrameLerpFrac;
            posDelta[1] = frames[keyFrameIndex][1] + (frames[keyFrameIndex + 1][1] - frames[keyFrameIndex][1]) * keyFrameLerpFrac;
            posDelta[2] = frames[keyFrameIndex][2] + (frames[keyFrameIndex + 1][2] - frames[keyFrameIndex][2]) * keyFrameLerpFrac;
        } else {
            posDelta[0] = trans->u.frame0[0];
            posDelta[1] = trans->u.frame0[1];
            posDelta[2] = trans->u.frame0[2];
        }
    } else {
        posDelta[0] = 0.0f;
        posDelta[1] = 0.0f;
        posDelta[2] = 0.0f;
    }
}

void XAnimGetAbsDelta(const XAnim *anims, unsigned int animIndex, vec_t *rot, vec_t *trans, float time)
{
    const XAnimEntry *anim = &anims->entries[animIndex];

    if (anim->numAnims || !anim->u.parts->bDelta) {
        rot[0] = 0.0f;
        rot[1] = 1.0f;
        trans[0] = 0.0f;
        trans[1] = 0.0f;
        trans[2] = 0.0f;
        return;
    }

    vec_t rotDelta[2];
    vec3_t posDelta;
    XAnim_CalcDeltaForTime(anim->u.parts, rotDelta, posDelta, time);

    const float rotScale = 3.0518509447574615e-05f;
    const float rot0 = rotDelta[0] * rotScale;
    const float rot1 = rotDelta[1] * rotScale;

    if (rot0 == 0.0f && rot1 == 0.0f) {
        rot[0] = 0.0f;
        rot[1] = 1.0f;
    } else {
        rot[0] = rot0;
        rot[1] = rot1;
    }

    trans[0] = posDelta[0];
    trans[1] = posDelta[1];
    trans[2] = posDelta[2];
}

void XAnimGetRelDelta(const XAnim *anims, unsigned int animIndex, vec_t *rot, vec_t *trans, float time1, float time2)
{
    const XAnimEntry *ent = &anims->entries[animIndex];
    const XAnimParts *parts;
    XAnimSimpleRotPos rp;

    if (ent->numAnims != 0)
        goto identity;
    parts = ent->u.parts;
    if (*((const char *)parts + 3) == 0)
        goto identity;

    rp.rot[0] = 0.0f;
    rp.rot[1] = 0.0f;
    rp.posWeight = 0.0f;
    rp.pos[0] = 0.0f;
    rp.pos[1] = 0.0f;
    rp.pos[2] = 0.0f;
    XAnimCalcRelDeltaParts(parts, &rp, 0, 1.0f, time1, time2);

    if (rp.rot[0] != 0.0f || rp.rot[1] != 0.0f) {
        rot[0] = rp.rot[0];
        rot[1] = rp.rot[1];
    } else {
        rot[0] = 0.0f;
        rot[1] = 1.0f;
    }
    trans[0] = rp.pos[0];
    trans[1] = rp.pos[1];
    trans[2] = rp.pos[2];
    return;

identity:
    rot[0] = 0.0f;
    rot[1] = 1.0f;
    trans[0] = 0.0f;
    trans[1] = 0.0f;
    trans[2] = 0.0f;
}

__attribute_regparm__(3) __attribute_sseregparm__
    void XAnimCalcRelDeltaParts(const XAnimParts *parts, XAnimSimpleRotPos *rotPos, int quatIndex, float weightScale, float time1, float time2)
{
    vec2_t q1;
    vec2_t q2;
    vec3_t vec1;
    vec3_t vec2;
    float *quat;
    float dx;
    float dy;
    float dz;
    float qx;
    float qy;
    float norm;
    float rotScale;

    XAnim_CalcDeltaForTime(parts, q1, vec1, time1);
    XAnim_CalcDeltaForTime(parts, q2, vec2, time2);

    if (parts->bLoop && time1 > time2) {
        XAnimDeltaPartTrans *trans = parts->deltaPart->trans;
        if (trans && trans->size) {
            vec3_t *frames = trans->u.frames.frames;
            vec2[0] += frames[trans->size][0];
            vec2[1] += frames[trans->size][1];
            vec2[2] += frames[trans->size][2];
            vec2[0] -= frames[0][0];
            vec2[1] -= frames[0][1];
            vec2[2] -= frames[0][2];
        }
    }

    rotScale = weightScale * 9.313794180343393e-10f;
    rotPos->rot[0] += (q2[0] * q1[1] - q2[1] * q1[0]) * rotScale;
    rotPos->rot[1] += (q2[0] * q1[0] + q2[1] * q1[1]) * rotScale;

    dx = vec2[0] - vec1[0];
    dy = vec2[1] - vec1[1];
    dz = vec2[2] - vec1[2];

    quat = (float *)q1 + quatIndex * 2;
    qx = quat[0];
    qy = quat[1];
    norm = qx * qx + qy * qy;
    if (norm != 0.0f) {
        float xx = (qx * qx) * (2.0f / norm);
        float xy = (qx * qy) * (2.0f / norm);
        float oldDx = dx;

        dx = (1.0f - xx) * oldDx + xy * dy;
        dy = dy - (xy * oldDx + xx * dy);
    }

    rotPos->posWeight += weightScale;
    rotPos->pos[0] += dx * weightScale;
    rotPos->pos[1] += dy * weightScale;
    rotPos->pos[2] += dz * weightScale;
}

static void __attribute_regparm__(2) XAnimClearTreeWeights(XAnimTree *tree, unsigned int animIndex)
{
    XAnimTree_s *tree_s = (XAnimTree_s *)tree;
    unsigned int infoIndex = tree_s->infoArray[animIndex];
    const XAnimEntry *anim;
    unsigned int i;

    if (!infoIndex) {
        return;
    }

    anim = &tree_s->anims->entries[animIndex];
    for (i = 0; i < anim->numAnims; ++i) {
        XAnimClearTreeWeights(tree, anim->u.s.children + i);
    }

    XAnimFreeInfoToList(infoIndex);
    tree_s->infoArray[animIndex] = 0;
}

static float XAnimFastInvSqrt(float x)
{
    union {
        float f;
        int i;
    } v;

    v.f = x;
    v.i = 0x5f3759df - (v.i >> 1);
    v.f = v.f * (1.5f - (0.5f * x * v.f * v.f));
    return v.f;
}

static void XAnimSimpleRotPosClear(XAnimSimpleRotPos *rotPos)
{
    rotPos->rot[0] = 0.0f;
    rotPos->rot[1] = 0.0f;
    rotPos->posWeight = 0.0f;
    rotPos->pos[0] = 0.0f;
    rotPos->pos[1] = 0.0f;
    rotPos->pos[2] = 0.0f;
}

static void XAnimSimpleRotPosScalePos(XAnimSimpleRotPos *rotPos, float scale)
{
    rotPos->pos[0] *= scale;
    rotPos->pos[1] *= scale;
    rotPos->pos[2] *= scale;
}

static void XAnimSimpleRotPosAddScaledRot(XAnimSimpleRotPos *dst, const XAnimSimpleRotPos *src, float scale)
{
    dst->rot[0] += src->rot[0] * scale;
    dst->rot[1] += src->rot[1] * scale;
}

static void XAnimSimpleRotPosAddScaledPos(XAnimSimpleRotPos *dst, const XAnimSimpleRotPos *src, float scale)
{
    dst->pos[0] += src->pos[0] * scale;
    dst->pos[1] += src->pos[1] * scale;
    dst->pos[2] += src->pos[2] * scale;
}

static void XAnimProcessClientNotifyArgs(XAnimInfo *info, const XAnimEntry *anim, float dtime)
{
    XAnimProcessClientNotify(info, anim, dtime);
}

__attribute_regparm__(3) __attribute_sseregparm__
    void XAnimCalcDeltaTree(const XAnimTree *tree, unsigned int animIndex, float weightScale, const Bool bClear, Bool bNormQuat, XAnimSimpleRotPos *rotPos)
{
    const XAnimEntry *anim;
    unsigned int numAnims;
    unsigned int firstChildIndex;
    unsigned int firstWeightedChild;
    unsigned int secondWeightedChild;
    unsigned int childIndex;
    unsigned int infoIndex;
    XAnimInfo *info;
    XAnimState *state;
    XAnimParts *parts;
    XAnimSimpleRotPos newRotPos;
    XAnimSimpleRotPos *rotPos2;
    float weight;
    float scale;
    int i;
    const XAnimTree_s *tree_s = (const XAnimTree_s *)tree;

    anim = &tree_s->anims->entries[animIndex];
    numAnims = anim->numAnims;

    if (!numAnims) {
        if (bClear) {
            XAnimSimpleRotPosClear(rotPos);
        }

        parts = anim->u.parts;
        if (!parts->bDelta) {
            return;
        }

        infoIndex = tree_s->infoArray[animIndex];
        if (!infoIndex) {
            return;
        }

        info = &g_xAnimInfo[infoIndex];
        state = &info->s;

        if (tree_s->bAbs) {
            vec2_t quat;
            vec3_t trans;

            XAnim_CalcDeltaForTime(parts, quat, trans, state->time);
            scale = weightScale * 0.000030518509447574615f;
            rotPos->rot[0] += quat[0] * scale;
            rotPos->rot[1] += quat[1] * scale;
            rotPos->posWeight += weightScale;
            rotPos->pos[0] += trans[0] * weightScale;
            rotPos->pos[1] += trans[1] * weightScale;
            rotPos->pos[2] += trans[2] * weightScale;
        } else {
            XAnimCalcRelDeltaParts(parts, rotPos, 1, weightScale, state->oldTime, state->time);
        }
        return;
    }

    firstChildIndex = anim->u.s.children;
    firstWeightedChild = numAnims;
    weight = 0.0f;

    for (i = 0; i < (int)numAnims; ++i) {
        childIndex = firstChildIndex + (unsigned int)i;
        infoIndex = tree_s->infoArray[childIndex];
        if (!infoIndex) {
            continue;
        }

        state = &g_xAnimInfo[infoIndex].s;
        weight = tree_s->bUseGoalWeight ? state->goalWeight : state->weight;
        if (weight != 0.0f) {
            firstWeightedChild = (unsigned int)i;
            break;
        }
    }

    if (firstWeightedChild == numAnims) {
        if (bClear) {
            XAnimSimpleRotPosClear(rotPos);
        }
        return;
    }

    secondWeightedChild = numAnims;
    for (i = (int)firstWeightedChild + 1; i < (int)numAnims; ++i) {
        childIndex = firstChildIndex + (unsigned int)i;
        infoIndex = tree_s->infoArray[childIndex];
        if (!infoIndex) {
            continue;
        }

        state = &g_xAnimInfo[infoIndex].s;
        if (tree_s->bUseGoalWeight) {
            weight = state->goalWeight;
        } else {
            weight = state->weight;
        }

        if (weight != 0.0f) {
            secondWeightedChild = (unsigned int)i;
            break;
        }
    }

    if (secondWeightedChild == numAnims) {
        XAnimCalcDeltaTree(tree, firstChildIndex + firstWeightedChild, weightScale, bClear, bNormQuat, rotPos);
        return;
    }

    if (bClear) {
        rotPos2 = rotPos;
    } else {
        rotPos2 = &newRotPos;
    }

    childIndex = firstChildIndex + firstWeightedChild;
    infoIndex = tree_s->infoArray[childIndex];
    state = &g_xAnimInfo[infoIndex].s;
    weight = tree_s->bUseGoalWeight ? state->goalWeight : state->weight;
    XAnimCalcDeltaTree(tree, childIndex, weight, 1, 1, rotPos2);

    childIndex = firstChildIndex + secondWeightedChild;
    infoIndex = tree_s->infoArray[childIndex];
    state = &g_xAnimInfo[infoIndex].s;
    weight = tree_s->bUseGoalWeight ? state->goalWeight : state->weight;
    XAnimCalcDeltaTree(tree, childIndex, weight, 0, 1, rotPos2);

    for (i = (int)secondWeightedChild + 1; i < (int)numAnims; ++i) {
        childIndex = firstChildIndex + (unsigned int)i;
        infoIndex = tree_s->infoArray[childIndex];
        if (!infoIndex) {
            continue;
        }

        state = &g_xAnimInfo[infoIndex].s;
        weight = tree_s->bUseGoalWeight ? state->goalWeight : state->weight;
        if (weight != 0.0f) {
            XAnimCalcDeltaTree(tree, childIndex, weight, 0, 1, rotPos2);
        }
    }

    if (!bNormQuat) {
        if (rotPos->posWeight != 0.0f) {
            XAnimSimpleRotPosScalePos(rotPos, 1.0f / rotPos->posWeight);
        }
        return;
    }

    if (bClear) {
        scale = rotPos->rot[0] * rotPos->rot[0] + rotPos->rot[1] * rotPos->rot[1];
        if (scale != 0.0f) {
            scale = XAnimFastInvSqrt(scale) * weightScale;
            rotPos->rot[0] *= scale;
            rotPos->rot[1] *= scale;
        }

        if (rotPos->posWeight != 0.0f) {
            XAnimSimpleRotPosScalePos(rotPos, weightScale / rotPos->posWeight);
            rotPos->posWeight = weightScale;
        }
        return;
    }

    scale = rotPos2->rot[0] * rotPos2->rot[0] + rotPos2->rot[1] * rotPos2->rot[1];
    if (scale != 0.0f) {
        XAnimSimpleRotPosAddScaledRot(rotPos, rotPos2, XAnimFastInvSqrt(scale) * weightScale);
    }

    if (rotPos2->posWeight != 0.0f) {
        XAnimSimpleRotPosAddScaledPos(rotPos, rotPos2, weightScale / rotPos2->posWeight);
        rotPos->posWeight += weightScale;
    }
}

void XAnimCalcAbsDelta(XAnimTree *tree, unsigned int animIndex, vec_t *rot, vec_t *trans)
{
    XAnimSimpleRotPos rotPos;

    ((XAnimTree_s *)tree)->bAbs = 1;
    ((XAnimTree_s *)tree)->bUseGoalWeight = 1;

    XAnimCalcDeltaTree(tree, animIndex, 1.0f, 1, 0, &rotPos);

    if (rotPos.rot[0] != 0.0f || rotPos.rot[1] != 0.0f) {
        rot[0] = rotPos.rot[0];
        rot[1] = rotPos.rot[1];
    } else {
        rot[0] = 0.0f;
        rot[1] = 1.0f;
    }

    trans[0] = rotPos.pos[0];
    trans[1] = rotPos.pos[1];
    trans[2] = rotPos.pos[2];
}

void XAnimCalcDelta(XAnimTree *tree, unsigned int animIndex, vec_t *rot, vec_t *trans, int bUseGoalWeight)
{
    XAnimSimpleRotPos rotPos;

    ((XAnimTree_s *)tree)->bAbs = 0;
    ((XAnimTree_s *)tree)->bUseGoalWeight = (Bool)bUseGoalWeight;

    XAnimCalcDeltaTree(tree, animIndex, 1.0f, 1, 0, &rotPos);

    if (rotPos.rot[0] == 0.0f || rotPos.rot[1] == 0.0f) {
        rot[0] = 0.0f;
        rot[1] = 1.0f;
    } else {
        rot[0] = rotPos.rot[0];
        rot[1] = rotPos.rot[1];
    }

    trans[0] = rotPos.pos[0];
    trans[1] = rotPos.pos[1];
    trans[2] = rotPos.pos[2];
}

static void __attribute_regparm__(2) XAnimResetTime(XAnimTree *tree, unsigned int animIndex)
{
    XAnimTree_s *tree_s = (XAnimTree_s *)tree;
    unsigned int infoIndex = tree_s->infoArray[animIndex];
    XAnimInfo *info;
    const XAnimEntry *anim;
    unsigned int i;

    if (!infoIndex) {
        return;
    }

    info = &g_xAnimInfo[infoIndex];
    if (info->s.time != 0.0f || info->s.timeCount != 0) {
        info->s.time = 0.0f;
        info->s.oldTime = 0.0f;
        info->s.timeCount = 0;
        info->s.oldTimeCount = 0;
        info->notifyIndex = -1;
    }

    anim = &tree_s->anims->entries[animIndex];
    for (i = 0; i < anim->numAnims; ++i) {
        XAnimResetTime(tree, anim->u.s.children + i);
    }
}

static inline __attribute__((always_inline)) void XAnimResetTimeIfNeeded(XAnimInfo *info)
{
    if (info->s.time != 0.0f || info->s.timeCount != 0) {
        info->s.time = 0.0f;
        info->s.oldTime = 0.0f;
        info->s.timeCount = 0;
        info->s.oldTimeCount = 0;
        info->notifyIndex = -1;
    }
}

static inline __attribute__((always_inline)) void XAnimRestart(XAnimTree *tree, unsigned int animIndex)
{
    XAnimTree_s *tree_s = (XAnimTree_s *)tree;
    XAnim *anims = tree_s->anims;
    unsigned int scanIndex;
    const XAnimEntry *anim;
    unsigned int i;

    if (animIndex) {
        scanIndex = animIndex;
        while (scanIndex) {
            anim = &anims->entries[scanIndex];
            if (anim->numAnims && (anim->u.s.flags & 4)) {
                XAnimResetTimeIfNeeded(&g_xAnimInfo[tree_s->infoArray[scanIndex]]);

                for (i = 0; i < anim->numAnims; ++i) {
                    unsigned int childIndex = anim->u.s.children + i;
                    if (tree_s->infoArray[childIndex]) {
                        XAnimResetTime(tree, childIndex);
                    }
                }
                return;
            }

            scanIndex = anim->parent;
        }
    }

    anim = &anims->entries[animIndex];
    if (!anim->numAnims) {
        XAnimResetTimeIfNeeded(&g_xAnimInfo[tree_s->infoArray[animIndex]]);
    }
}

static void __attribute_regparm__(3) __attribute_sseregparm__
    XAnimUpdateOldTime(XAnimTree *tree, unsigned int animIndex, XAnimState *syncState,
                       Bool parentHadWeight, Bool *infoExistsForParent,
                       Bool *childHadWeightForParent, float dtime)
{
    XAnimTree_s *tree_s = (XAnimTree_s *)tree;
    unsigned int infoIndex = tree_s->infoArray[animIndex];
    XAnimInfo *info;
    XAnimState *state;
    const XAnimEntry *anim;
    unsigned int numAnims;
    Bool hadWeight;
    Bool infoExists;
    Bool childHadWeight;
    XAnimState *childSyncState;
    unsigned int i;

    if (!infoIndex) {
        return;
    }

    info = &g_xAnimInfo[infoIndex];
    state = &info->s;

    hadWeight = (parentHadWeight && state->weight != 0.0f);

    if (parentHadWeight && dtime < state->goalTime) {
        state->weight += ((state->goalWeight - state->weight) * dtime) / state->goalTime;
        if (state->weight < 1.0000001111620804e-06f) {
            state->weight = state->goalWeight * 0.0010000000474974513f;
        }
        state->goalTime -= dtime;
    } else {
        state->weight = state->goalWeight;
        state->goalTime = 0.0f;
    }

    infoExists = hadWeight || state->goalWeight != 0.0f;

    anim = &tree_s->anims->entries[animIndex];
    numAnims = anim->numAnims;

    if (!numAnims) {
        childHadWeight = (anim->u.parts->frequency != 0.0f);
    } else {
        childHadWeight = 0;
        childSyncState = (anim->u.s.flags & 4) ? state : syncState;

        for (i = 0; i < numAnims; ++i) {
            Bool childInfoExists = infoExists;
            unsigned int childIndex = anim->u.s.children + i;

            XAnimUpdateOldTime(tree, childIndex, childSyncState, hadWeight,
                               &childInfoExists, &childHadWeight, dtime);
            infoExists = childInfoExists;
        }
    }

    if (!infoExists) {
        XAnimFreeInfoToList(infoIndex);
        tree_s->infoArray[animIndex] = 0;
        return;
    }

    if (hadWeight && childHadWeight) {
        *childHadWeightForParent = 1;
    } else if (numAnims && (anim->u.s.flags & 4)) {
        if (state->time != 0.0f || state->timeCount != 0) {
            state->time = 0.0f;
            state->timeCount = 0;
            state->oldTime = 0.0f;
            state->oldTimeCount = 0;
            info->notifyIndex = -1;

            for (i = 0; i < numAnims; ++i) {
                unsigned int childIndex = anim->u.s.children + i;
                if (tree_s->infoArray[childIndex]) {
                    XAnimResetTime(tree, childIndex);
                }
            }
        }
    } else if (state->time != syncState->time || state->timeCount != syncState->timeCount) {
        state->time = syncState->time;
        state->timeCount = syncState->timeCount;
        info->notifyIndex = -1;
    }

    state->oldTime = state->time;
    state->oldTimeCount = state->timeCount;
    *infoExistsForParent = 1;
}

void DObjInitServerTime(DObj *obj, float dtime)
{
    if (obj->tree) {
        XAnimState syncState;
        Bool infoExists;
        Bool childHadWeight;

        syncState.time = 0.0f;
        syncState.timeCount = 0;
        XAnimUpdateOldTime(obj->tree, 0, &syncState, 1, &infoExists, &childHadWeight, dtime);
    }
}

static int __attribute_regparm__(3) __attribute_sseregparm__
    XAnimSetGoalWeightInternal(XAnimTree *tree, unsigned int animIndex, int bForce,
                               float goalWeight, float goalTime, float rate,
                               unsigned int notifyName, unsigned int notifyType)
{
    XAnimTree_s *tree_s = (XAnimTree_s *)tree;
    unsigned int infoIndex = tree_s->infoArray[animIndex];
    XAnimInfo *info;
    XAnimState *state;

    if (!infoIndex) {
        if (goalWeight == 0.0f && !bForce) {
            return 0;
        }

        info = XAnimAllocInfoForTree(tree_s, animIndex);
        memset(&info->s, 0, sizeof(info->s));
        info->notifyName = 0;
        info->notifyIndex = -1;
        info->notifyChild = 0;
        info->notifyType = 0;
    } else {
        info = &g_xAnimInfo[infoIndex];
        XAnimClearInfoNotify(info);
    }

    state = &info->s;

    if (!animIndex) {
        goalWeight = 1.0f;
        goalTime = 0.0f;
        rate = 1.0f;
    }

    if (goalTime == 0.0f) {
        state->weight = goalWeight;
    } else if (state->weight == 0.0f) {
        state->weight = goalWeight * 0.0010000000474974513f;
    }

    if (goalWeight != 0.0f) {
        if (goalWeight < state->weight) {
            state->goalTime = goalTime * ((state->weight - goalWeight) / state->weight);
        } else {
            state->goalTime = goalTime * ((goalWeight - state->weight) / goalWeight);
        }
    } else if (state->goalWeight != 0.0f || state->goalTime > goalTime) {
        state->goalTime = goalTime;
    }

    state->goalWeight = goalWeight;
    state->rate = rate;

    info->notifyName = (unsigned short)notifyName;
    if (notifyName) {
        SL_AddRefToString(notifyName);
    }

    if (notifyName) {
        const XAnimEntry *anim = &tree_s->anims->entries[animIndex];

        if (anim->numAnims && (anim->u.s.flags & 3)) {
            unsigned int i;
            unsigned int bestAnim = 0;
            float bestWeight = 0.0f;

            for (i = 0; i < anim->numAnims; ++i) {
                unsigned int childIndex = anim->u.s.children + i;
                unsigned int childInfoIndex = tree_s->infoArray[childIndex];
                float childGoalWeight = g_xAnimInfo[childInfoIndex].s.goalWeight;

                if (bestWeight < childGoalWeight) {
                    unsigned int descendant = XAnimGetDescendantWithGreatestWeight(tree, childIndex);
                    if (descendant) {
                        bestWeight = childGoalWeight;
                        bestAnim = descendant;
                    }
                }
            }

            info->notifyChild = (unsigned short)bestAnim;
            if (!bestAnim) {
                return 2;
            }
        } else {
            info->notifyChild = 0;
        }
    } else {
        info->notifyChild = 0;
    }

    info->notifyType = (unsigned short)notifyType;
    return 0;
}

void XAnimSetCompleteGoalWeight(XAnimTree *tree, unsigned int animIndex, float goalWeight, float goalTime, float rate, unsigned int notifyName, unsigned int notifyType, int bRestart)
{
    XAnimTree_s *tree_s = (XAnimTree_s *)tree;
    unsigned int parentAnimIndex;

    if (goalWeight < 0.0010000000474974513f) {
        goalWeight = 0.0f;
    }

    XAnimSetGoalWeightInternal(tree, animIndex, 0, goalWeight, goalTime, rate, notifyName, notifyType);

    parentAnimIndex = animIndex;
    while (parentAnimIndex) {
        unsigned int infoIndex;

        parentAnimIndex = tree_s->anims->entries[parentAnimIndex].parent;
        infoIndex = tree_s->infoArray[parentAnimIndex];

        if (!infoIndex || g_xAnimInfo[infoIndex].s.weight == 0.0f) {
            XAnimSetGoalWeightInternal(tree, parentAnimIndex, 0, 1.0f, goalTime, 1.0f, 0, 0);
        }
    }

    if (bRestart) {
        XAnimRestart(tree, animIndex);
    }
}

void XAnimCloneAnimTree(const XAnimTree *from, XAnimTree *to)
{
    const XAnimTree_s *fromTree = (const XAnimTree_s *)from;
    XAnimTree_s *toTree = (XAnimTree_s *)to;
    int size = fromTree->anims->size;
    int i;

    for (i = 0; i < size; ++i) {
        unsigned int fromInfoIndex = fromTree->infoArray[i];
        unsigned int toInfoIndex = toTree->infoArray[i];
        XAnimInfo *toInfo;

        if (!fromInfoIndex) {
            if (toInfoIndex) {
                XAnimFreeInfoToList(toInfoIndex);
                toTree->infoArray[i] = 0;
            }
            continue;
        }

        if (toInfoIndex) {
            toInfo = &g_xAnimInfo[toInfoIndex];
            XAnimClearInfoNotify(toInfo);
        } else {
            toInfo = XAnimAllocInfoForTree(toTree, i);
        }

        *toInfo = g_xAnimInfo[fromInfoIndex];
        if (toInfo->notifyName) {
            SL_AddRefToString(toInfo->notifyName);
        }
    }
}

void XAnimFreeTree(XAnimTree *tree, Free_t Free)
{
    XAnimTree_s *tree_s = (XAnimTree_s *)tree;
    unsigned int treeSize = tree_s->anims->size * 5 + 9;

    XAnimClearTree(tree);

    if (Free) {
        Free(tree, treeSize);
    }
}

void XAnimClearTree(XAnimTree *tree)
{
    XAnimTree_s *tree_s;
    unsigned int infoIndex;

    if (!tree)
        return;

    tree_s = (XAnimTree_s *)tree;
    infoIndex = tree_s->infoArray[0];
    if (infoIndex) {
        XAnimEntry *anim = &tree_s->anims->entries[0];
        int numAnims = anim->numAnims;
        int child;

        for (child = 0; child < numAnims; ++child) {
            XAnimClearTreeWeights(tree, anim->u.s.children + child);
        }

        XAnimInfo *info = &g_xAnimInfo[infoIndex];
        if (info->notifyName) {
            SL_RemoveRefToString(info->notifyName);
            info->notifyName = 0;
        }

        info->notifyIndex = -1;
        info->prev = 0;
        info->next = g_xAnimInfo[0].next;
        g_xAnimInfo[info->next].prev = infoIndex;
        g_xAnimInfo[0].next = infoIndex;
        tree_s->infoArray[0] = 0;
    }

    {
        int size = tree_s->anims->size;
        unsigned short *stringValue = &tree_s->infoArray[size];
        int j;

        for (j = 0; j < size; ++j) {
            if (stringValue[j]) {
                SL_RemoveRefToStringOfLen(stringValue[j], tree_s->anims->entries[j].u.parts->boneCount + 16);
                stringValue[j] = 0;
            }
        }
    }
}

static int XAnimCalcBitTestLocal(const int *bits, int boneIndex)
{
    return (bits[boneIndex >> 5] >> (boneIndex & 31)) & 1;
}

static void XAnimCalcBitSetLocal(int *bits, int boneIndex)
{
    bits[boneIndex >> 5] |= 1 << (boneIndex & 31);
}

static void XAnimCalcClearRotTransLocal(DObjAnimMat *mat)
{
    mat->quat[0] = 0.0f;
    mat->quat[1] = 0.0f;
    mat->quat[2] = 0.0f;
    mat->quat[3] = 0.0f;
    mat->trans[0] = 0.0f;
    mat->trans[1] = 0.0f;
    mat->trans[2] = 0.0f;
    mat->transWeight = 0.0f;
}

static float XAnimCalcInvSqrtLocal(float value)
{
    union {
        float f;
        unsigned int i;
    } u;

    if (value == 0.0f) {
        return 0.0f;
    }

    u.f = value;
    u.i = 0x5f3759dfU - (u.i >> 1);
    u.f = u.f * (1.5f - (value * 0.5f * u.f * u.f));
    return u.f;
}

static void XAnimCalcNormalizeRotTransLocal(DObjAnimMat *mat, float weightScale)
{
    const float quatLenSq = mat->quat[0] * mat->quat[0] + mat->quat[1] * mat->quat[1] + mat->quat[2] * mat->quat[2] + mat->quat[3] * mat->quat[3];

    if (quatLenSq != 0.0f) {
        const float scale = weightScale * XAnimCalcInvSqrtLocal(quatLenSq);
        mat->quat[0] *= scale;
        mat->quat[1] *= scale;
        mat->quat[2] *= scale;
        mat->quat[3] *= scale;
    }

    if (mat->transWeight != 0.0f) {
        const float scale = weightScale / mat->transWeight;
        mat->trans[0] *= scale;
        mat->trans[1] *= scale;
        mat->trans[2] *= scale;
        mat->transWeight = weightScale;
    }
}

static void XAnimCalcMakeTimeLocal(const XAnimParts *parts, float time, XAnimTime *animTime)
{
    animTime->time = time;
    animTime->frameCount = parts->numframes;
    animTime->frameFrac = (float)parts->numframes * time;
    animTime->frameIndex = (int)animTime->frameFrac;

    if (animTime->frameIndex >= animTime->frameCount) {
        animTime->frameIndex = animTime->frameCount - 1;
        animTime->frameFrac = (float)animTime->frameIndex;
    }
    if (animTime->frameIndex < 0) {
        animTime->frameIndex = 0;
        animTime->frameFrac = 0.0f;
    }
}

static void XAnimCalcGetFrameIndexLocal(const XAnimTime *animTime, const XAnimDynamicIndices *indices,
                                        int tableSize, int *keyFrameIndex, float *keyFrameLerpFrac)
{
    if (tableSize <= 0) {
        *keyFrameIndex = 0;
        *keyFrameLerpFrac = 0.0f;
        return;
    }

    if (animTime->frameCount > 255 && tableSize < animTime->frameCount) {
        Z28XAnim_GetTimeIndexCompressedItEvPK9XAnimTimePKT_iPiPf(animTime, indices->_2, tableSize,
                                                                 keyFrameIndex, keyFrameLerpFrac);
    } else {
        Z18XAnim_GetTimeIndexIhEvPK9XAnimTimePK19XAnimDynamicIndicesiPiPf(animTime, indices, tableSize,
                                                                          keyFrameIndex, keyFrameLerpFrac);
    }
}

static unsigned short XAnimCalcBuildAnimToModelLocal(const DObj *obj, const XAnimParts *parts)
{
    XAnimToXModel animToModel;
    const int animBoneCount = parts->boneCount;
    const int storedBoneCount = animBoneCount > 128 ? 128 : animBoneCount;
    int globalBoneIndex = 0;
    int modelIndex;

    memset(&animToModel, 0, sizeof(animToModel));
    memset(animToModel.boneIndex, 0x7f, sizeof(animToModel.boneIndex));

    for (modelIndex = 0; modelIndex < obj->numModels; ++modelIndex) {
        const XModel *model = obj->models[modelIndex];
        const XModelParts *modelParts;
        const unsigned short *boneNames;
        int localBoneIndex;
        int numBones;

        if (!model || !model->parts) {
            continue;
        }

        modelParts = (const XModelParts *)model->parts;
        numBones = modelParts->numBones;
        boneNames = modelParts->hierarchy ? modelParts->hierarchy->names : NULL;

        for (localBoneIndex = 0; localBoneIndex < numBones; ++localBoneIndex, ++globalBoneIndex) {
            unsigned int name;
            int animPartIndex;

            if (!boneNames) {
                continue;
            }

            name = boneNames[localBoneIndex];
            for (animPartIndex = storedBoneCount - 1; animPartIndex >= 0; --animPartIndex) {
                if (parts->names[animPartIndex] == name && animToModel.boneIndex[animPartIndex] == 0x7f) {
                    animToModel.boneIndex[animPartIndex] = (unsigned char)globalBoneIndex;
                    XAnimCalcBitSetLocal(animToModel.partBits, globalBoneIndex);
                    break;
                }
            }
        }
    }

    return (unsigned short)SL_GetStringOfLen(&animToModel, 0, (unsigned int)(storedBoneCount + 16), 11);
}

static const unsigned char *XAnimCalcGetAnimToModelLocal(const DObj *obj, const XAnimTree_s *tree,
                                                         unsigned int animIndex, const XAnimParts *parts)
{
    unsigned short *stringTable = obj->animToModel;
    unsigned char *ageTable;
    unsigned short stringValue;
    int len = parts->boneCount + 16;

    if (!stringTable) {
        stringValue = XAnimCalcBuildAnimToModelLocal(obj, parts);
        return (const unsigned char *)SL_ConvertToString(stringValue);
    }

    ageTable = (unsigned char *)stringTable + tree->anims->size * sizeof(unsigned short);
    stringValue = stringTable[animIndex];

    if (!stringValue) {
        ageTable[animIndex + 1] = ageTable[0];
        stringValue = XAnimCalcBuildAnimToModelLocal(obj, parts);
        stringTable[animIndex] = stringValue;
    } else if (ageTable[animIndex + 1] != ageTable[0]) {
        ageTable[animIndex + 1] = ageTable[0];
        SL_RemoveRefToStringOfLen(stringValue, len);
        stringValue = XAnimCalcBuildAnimToModelLocal(obj, parts);
        stringTable[animIndex] = stringValue;
    }

    return (const unsigned char *)SL_ConvertToString(stringValue);
}

static void XAnimCalcAccumulateFullQuatLocal(DObjAnimMat *mat, const XAnimPartQuat *quat,
                                             const XAnimTime *animTime, float scale)
{
    if (!quat) {
        mat->quat[3] += 32767.0f * scale;
        return;
    }

    if (!quat->size) {
        mat->quat[0] += (float)quat->u.frame0[0] * scale;
        mat->quat[1] += (float)quat->u.frame0[1] * scale;
        mat->quat[2] += (float)quat->u.frame0[2] * scale;
        mat->quat[3] += (float)quat->u.frame0[3] * scale;
    } else {
        int frameIndex;
        float lerpFrac;
        const XQuat *frames = (const XQuat *)quat->u.frames.u.frames;

        XAnimCalcGetFrameIndexLocal(animTime, &quat->u.frames.indices, quat->size, &frameIndex, &lerpFrac);
        mat->quat[0] += ((float)frames[frameIndex][0] + ((float)frames[frameIndex + 1][0] - (float)frames[frameIndex][0]) * lerpFrac) * scale;
        mat->quat[1] += ((float)frames[frameIndex][1] + ((float)frames[frameIndex + 1][1] - (float)frames[frameIndex][1]) * lerpFrac) * scale;
        mat->quat[2] += ((float)frames[frameIndex][2] + ((float)frames[frameIndex + 1][2] - (float)frames[frameIndex][2]) * lerpFrac) * scale;
        mat->quat[3] += ((float)frames[frameIndex][3] + ((float)frames[frameIndex + 1][3] - (float)frames[frameIndex][3]) * lerpFrac) * scale;
    }
}

static void XAnimCalcAccumulateSimpleQuatLocal(DObjAnimMat *mat, const XAnimPartQuat *quat,
                                               const XAnimTime *animTime, float scale)
{
    if (!quat) {
        mat->quat[3] += 32767.0f * scale;
        return;
    }

    if (!quat->size) {
        mat->quat[2] += (float)quat->u.frame02[0] * scale;
        mat->quat[3] += (float)quat->u.frame02[1] * scale;
    } else {
        int frameIndex;
        float lerpFrac;
        const XQuat2 *frames = (const XQuat2 *)quat->u.frames.u.frames2;

        XAnimCalcGetFrameIndexLocal(animTime, &quat->u.frames.indices, quat->size, &frameIndex, &lerpFrac);
        mat->quat[2] += ((float)frames[frameIndex][0] + ((float)frames[frameIndex + 1][0] - (float)frames[frameIndex][0]) * lerpFrac) * scale;
        mat->quat[3] += ((float)frames[frameIndex][1] + ((float)frames[frameIndex + 1][1] - (float)frames[frameIndex][1]) * lerpFrac) * scale;
    }
}

static void XAnimCalcAccumulateTransLocal(DObjAnimMat *mat, const XAnimPartTrans *trans,
                                          const XAnimTime *animTime, float weightScale)
{
    if (trans) {
        if (!trans->size) {
            mat->trans[0] += trans->u.frame0[0] * weightScale;
            mat->trans[1] += trans->u.frame0[1] * weightScale;
            mat->trans[2] += trans->u.frame0[2] * weightScale;
        } else {
            int frameIndex;
            float lerpFrac;
            const vec3_t *frames = (const vec3_t *)trans->u.frames.frames;

            XAnimCalcGetFrameIndexLocal(animTime, &trans->u.frames.indices, trans->size, &frameIndex, &lerpFrac);
            mat->trans[0] += (frames[frameIndex][0] + (frames[frameIndex + 1][0] - frames[frameIndex][0]) * lerpFrac) * weightScale;
            mat->trans[1] += (frames[frameIndex][1] + (frames[frameIndex + 1][1] - frames[frameIndex][1]) * lerpFrac) * weightScale;
            mat->trans[2] += (frames[frameIndex][2] + (frames[frameIndex + 1][2] - frames[frameIndex][2]) * lerpFrac) * weightScale;
        }
    }

    mat->transWeight += weightScale;
}

static void XAnimCalcAccumulateLeafLocal(const DObj *obj, const XAnimTree_s *tree, unsigned int animIndex,
                                         const XAnimEntry *anim, float weightScale, DObjAnimMat *rotTransArray,
                                         XAnimCalcAnimInfo *info)
{
    const XAnimParts *parts = anim->u.parts;
    const unsigned char *animToModel;
    const unsigned char *boneIndexMap;
    const int *animPartBits;
    unsigned short infoIndex;
    float time;
    XAnimTime animTime;
    float quatScale;
    int animPartIndex;
    int partCount;

    if (!parts || !info) {
        return;
    }

    animToModel = XAnimCalcGetAnimToModelLocal(obj, tree, animIndex, parts);
    if (!animToModel) {
        return;
    }

    animPartBits = (const int *)animToModel;
    boneIndexMap = animToModel + 16;

    for (animPartIndex = 0; animPartIndex < 4; ++animPartIndex) {
        info->animPartBits[animPartIndex] |= animPartBits[animPartIndex] & ~info->ignorePartBits[animPartIndex];
    }

    infoIndex = tree->infoArray[animIndex];
    time = g_xAnimInfo[infoIndex].s.time;
    XAnimCalcMakeTimeLocal(parts, time, &animTime);
    quatScale = weightScale * 3.0518509447574615e-05f;
    partCount = parts->boneCount;

    for (animPartIndex = 0; animPartIndex < partCount; ++animPartIndex) {
        int modelBoneIndex = boneIndexMap[animPartIndex];
        DObjAnimMat *mat;
        const XAnimPart *part;

        if (modelBoneIndex == 0x7f) {
            continue;
        }
        if (XAnimCalcBitTestLocal(info->ignorePartBits, modelBoneIndex)) {
            continue;
        }

        mat = &rotTransArray[modelBoneIndex];
        part = &parts->parts[animPartIndex];

        if (parts->simpleQuatBits && ((parts->simpleQuatBits[animPartIndex >> 3] >> (animPartIndex & 7)) & 1)) {
            XAnimCalcAccumulateSimpleQuatLocal(mat, part->quat, &animTime, quatScale);
        } else {
            XAnimCalcAccumulateFullQuatLocal(mat, part->quat, &animTime, quatScale);
        }

        XAnimCalcAccumulateTransLocal(mat, part->trans, &animTime, weightScale);
    }
}

static void XAnimCalcNormalizeBonesLocal(const DObj *obj, DObjAnimMat *rotTransArray, float weightScale,
                                         const XAnimCalcAnimInfo *info)
{
    int boneIndex;

    for (boneIndex = 0; boneIndex < obj->numBones; ++boneIndex) {
        if (!XAnimCalcBitTestLocal(info->ignorePartBits, boneIndex)) {
            XAnimCalcNormalizeRotTransLocal(&rotTransArray[boneIndex], weightScale);
        }
    }
}

void XANIM_CALC_ABI XAnimCalc(const DObj *obj, unsigned int animIndex, float weightScale, XAnimPart (*rotTransArray)(), int bClear, int bNormQuat, XAnimCalcAnimInfo *info, int rotTransArrayIndex)
{
    XAnimTree_s *tree;
    XAnim *anims;
    const XAnimEntry *anim;
    DObjAnimMat *matArray = (DObjAnimMat *)rotTransArray;
    unsigned int childCount;
    unsigned int childBase;
    unsigned int childIndex;
    int usedChild = 0;
    int boneIndex;

    (void)rotTransArrayIndex;

    if (!obj || !obj->tree || !matArray || !info) {
        return;
    }

    tree = (XAnimTree_s *)obj->tree;
    anims = tree->anims;
    anim = &anims->entries[animIndex];
    childCount = anim->numAnims;

    if (bClear) {
        for (boneIndex = 0; boneIndex < obj->numBones; ++boneIndex) {
            if (!XAnimCalcBitTestLocal(info->ignorePartBits, boneIndex)) {
                XAnimCalcClearRotTransLocal(&matArray[boneIndex]);
            }
        }
    }

    if (!childCount) {
        XAnimCalcAccumulateLeafLocal(obj, tree, animIndex, anim, weightScale, matArray, info);
        return;
    }

    childBase = anim->u.s.children;
    for (childIndex = 0; childIndex < childCount; ++childIndex) {
        unsigned int subAnimIndex = childBase + childIndex;
        unsigned short infoIndex = tree->infoArray[subAnimIndex];
        float childWeight;

        if (!infoIndex) {
            continue;
        }

        childWeight = g_xAnimInfo[infoIndex].s.weight;
        if (childWeight == 0.0f) {
            continue;
        }

        XAnimCalc(obj, subAnimIndex, weightScale * childWeight, (XAnimPart (*)())matArray,
                  bClear && !usedChild, bNormQuat, info, rotTransArrayIndex);
        usedChild = 1;
    }

    if (!usedChild) {
        return;
    }

    if (bNormQuat) {
        XAnimCalcNormalizeBonesLocal(obj, matArray, weightScale, info);
    }
}

void DObjCalcAnim(const DObj *obj, int *partBits)
{
    DSkel *skel = obj->skel;
    int ignorePartBits[4];
    int allIgnored = 1;
    DObjAnimMat *mat;
    XAnimCalcAnimInfo info;
    int j;
    int boneIndex;

    for (j = 0; j < 4; ++j) {
        ignorePartBits[j] = ~partBits[j] | skel->animPartBits[j];
        if (ignorePartBits[j] != -1) {
            allIgnored = 0;
        }
    }

    if (allIgnored) {
        return;
    }

    for (j = 0; j < 4; ++j) {
        skel->animPartBits[j] |= partBits[j];
        info.ignorePartBits[j] = ignorePartBits[j];
        info.animPartBits[j] = 0;
    }
    info.ignorePartBits[3] |= 0x80000000;

    mat = skel->mat;

    if (obj->tree) {
        XAnimCalc(obj, 0, 1.0f, (XAnimPart (*)())mat, 1, 0, &info, 0);
        /* A bone can be present only in a low-weight branch (for example,
         * the legs retain weight 0.01 while the torso animation is active).
         * Convert accumulated translations back to a pose before composing
         * the skeleton; otherwise those bone lengths shrink with the weight. */
        XAnimCalcNormalizeBonesLocal(obj, mat, 1.0f, &info);
    }

    if (!obj->numModels) {
        return;
    }

    boneIndex = 0;

    for (j = 0; j < obj->numModels; ++j) {
        const XModelParts *parts = (const XModelParts *)obj->models[j]->parts;
        int numRootBones = parts->numRootBones;
        int numBones = parts->numBones;
        int childBoneCount;
        int endRootBone;
        int i;

        endRootBone = boneIndex + numRootBones;
        while (boneIndex < endRootBone) {
            if (!((ignorePartBits[boneIndex >> 5] >> (boneIndex & 31)) & 1) &&
                !((info.animPartBits[boneIndex >> 5] >> (boneIndex & 31)) & 1)) {
                mat->quat[0] = 0.0f;
                mat->quat[1] = 0.0f;
                mat->quat[2] = 0.0f;
                mat->quat[3] = 1.0f;
                mat->trans[0] = 0.0f;
                mat->trans[1] = 0.0f;
                mat->trans[2] = 0.0f;
            }

            ++mat;
            ++boneIndex;
        }

        childBoneCount = numBones - numRootBones;
        if (childBoneCount) {
            const short *quat = parts->quats;
            const float *trans = parts->trans;
            for (i = 0; i < childBoneCount; ++i) {
                if (!((ignorePartBits[boneIndex >> 5] >> (boneIndex & 31)) & 1) &&
                    !((info.animPartBits[boneIndex >> 5] >> (boneIndex & 31)) & 1)) {
                    mat->quat[0] = (float)quat[0] * 3.0518509447574615e-05f;
                    mat->quat[1] = (float)quat[1] * 3.0518509447574615e-05f;
                    mat->quat[2] = (float)quat[2] * 3.0518509447574615e-05f;
                    mat->quat[3] = (float)quat[3] * 3.0518509447574615e-05f;
                    mat->trans[0] = trans ? trans[0] : 0.0f;
                    mat->trans[1] = trans ? trans[1] : 0.0f;
                    mat->trans[2] = trans ? trans[2] : 0.0f;
                } else if (!XAnimCalcBitTestLocal(ignorePartBits, boneIndex) && trans) {
                    /* XAnim translations are offsets from the model's local
                     * bind translation, including zero for an omitted track. */
                    mat->trans[0] += trans[0];
                    mat->trans[1] += trans[1];
                    mat->trans[2] += trans[2];
                }

                ++mat;
                ++boneIndex;
                quat += 4;
                if (trans)
                    trans += 3;
            }
        }
    }
}

static float __attribute_regparm__(2) XAnimGetAverageRateFrequency(const XAnimTree *tree, unsigned int animIndex)
{
    const XAnimTree_s *tree_s = (const XAnimTree_s *)tree;
    const XAnimEntry *anim = &tree_s->anims->entries[animIndex];
    unsigned int numAnims = anim->numAnims;
    float totalWeight;
    float totalDtime;
    unsigned int i;

    if (!numAnims) {
        return anim->u.parts->frequency;
    }

    totalWeight = 0.0f;
    totalDtime = 0.0f;

    for (i = 0; i < numAnims; ++i) {
        unsigned int childIndex = anim->u.s.children + i;
        unsigned int infoIndex = tree_s->infoArray[childIndex];
        XAnimInfo *info;
        float weight;
        float frequency;

        if (!infoIndex) {
            continue;
        }

        info = &g_xAnimInfo[infoIndex];
        weight = info->s.weight;
        if (weight == 0.0f) {
            continue;
        }

        frequency = XAnimGetAverageRateFrequency(tree, childIndex);
        if (frequency == 0.0f) {
            continue;
        }

        totalWeight += weight;
        totalDtime += weight * frequency * info->s.rate;
    }

    if (totalWeight == 0.0f) {
        return 0.0f;
    }

    return totalDtime / totalWeight;
}

static Bool XAnimClearTreeGoalWeightInfo(XAnimTree *tree, unsigned int animIndex, float blendTime)
{
    unsigned short infoIndex = ((XAnimTree_s *)tree)->infoArray[animIndex];
    XAnimInfo *info;
    XAnimState *state;

    if (!infoIndex) {
        return 0;
    }

    info = &g_xAnimInfo[infoIndex];
    state = &info->s;

    if (state->goalWeight != 0.0f || state->goalTime > blendTime) {
        state->goalTime = blendTime;
    }

    state->goalWeight = 0.0f;

    if (blendTime == 0.0f) {
        state->weight = 0.0f;
    }

    XAnimClearInfoNotify(info);
    return 1;
}

void XAnimClearTreeGoalWeights(XAnimTree *tree, unsigned int animIndex, float blendTime)
{
    const XAnimEntry *anim;
    unsigned int i;

    if (!XAnimClearTreeGoalWeightInfo(tree, animIndex, blendTime)) {
        return;
    }

    anim = &((XAnimTree_s *)tree)->anims->entries[animIndex];
    for (i = 0; i < anim->numAnims; ++i) {
        XAnimClearTreeGoalWeights(tree, anim->u.s.children + i, blendTime);
    }
}

void XAnimClearTreeGoalWeightsStrict(XAnimTree *tree, unsigned int animIndex, float blendTime)
{
    const XAnimEntry *anim = &((XAnimTree_s *)tree)->anims->entries[animIndex];
    unsigned int i;

    for (i = 0; i < anim->numAnims; ++i) {
        unsigned int childIndex = anim->u.s.children + i;
        const XAnimEntry *childAnim;
        unsigned int j;

        if (!XAnimClearTreeGoalWeightInfo(tree, childIndex, blendTime)) {
            continue;
        }

        childAnim = &((XAnimTree_s *)tree)->anims->entries[childIndex];
        for (j = 0; j < childAnim->numAnims; ++j) {
            XAnimClearTreeGoalWeights(tree, childAnim->u.s.children + j, blendTime);
        }
    }
}

static void __attribute_regparm__(3) __attribute_sseregparm__
    XAnimProcessServerNotify(const XAnimTree *tree, XAnimInfo *info, const XAnimEntry *anim, float time)
{
    const XAnimTree_s *tree_s = (const XAnimTree_s *)tree;
    const XAnimEntry *notifyAnim;
    const XAnimParts *parts;
    const XAnimNotifyInfo *notifyInfo;
    float oldTime;
    int notifyIndex;

    if (!tree_s->entnum || !info->notifyName) {
        return;
    }

    oldTime = info->s.time;
    if (oldTime == 1.0f) {
        Scr_AddConstString(g_end);
        Scr_NotifyNum(tree_s->entnum - 1, 0, info->notifyName, 1);
        return;
    }

    notifyAnim = anim;
    if (info->notifyIndex < 0) {
        if (anim->numAnims) {
            if (!info->notifyChild) {
                if (oldTime > time || time == 1.0f) {
                    Scr_AddConstString(g_end);
                    Scr_NotifyNum(tree_s->entnum - 1, 0, info->notifyName, 1);
                }
                return;
            }
            notifyAnim = &tree_s->anims->entries[info->notifyChild];
        }

        parts = notifyAnim->u.parts;
        notifyInfo = XAnimGetNextNotifyInfo(parts, oldTime);
        info->notifyIndex = notifyInfo ? (short)(notifyInfo - parts->notify) : -1;
        if (info->notifyIndex < 0) {
            if (oldTime > time || time == 1.0f) {
                Scr_AddConstString(g_end);
                Scr_NotifyNum(tree_s->entnum - 1, 0, info->notifyName, 1);
            }
            return;
        }
    }

    if (anim->numAnims) {
        notifyAnim = &tree_s->anims->entries[info->notifyChild];
    }

    parts = notifyAnim->u.parts;
    notifyIndex = info->notifyIndex;
    notifyInfo = &parts->notify[notifyIndex];

    if (oldTime > time) {
        while (notifyIndex < parts->notifyCount && time > notifyInfo->time) {
            Scr_AddConstString(notifyInfo->name);
            Scr_NotifyNum(tree_s->entnum - 1, 0, info->notifyName, 1);
            ++notifyIndex;
            ++notifyInfo;
        }

        notifyInfo = &parts->notify[info->notifyIndex];
        notifyIndex = info->notifyIndex;
        if (oldTime <= notifyInfo->time) {
            while (notifyIndex < parts->notifyCount) {
                Scr_AddConstString(notifyInfo->name);
                Scr_NotifyNum(tree_s->entnum - 1, 0, info->notifyName, 1);
                ++notifyIndex;
                ++notifyInfo;
            }
        }
        return;
    }

    if (time == 1.0f) {
        if (oldTime <= notifyInfo->time) {
            while (notifyIndex < parts->notifyCount) {
                Scr_AddConstString(notifyInfo->name);
                Scr_NotifyNum(tree_s->entnum - 1, 0, info->notifyName, 1);
                ++notifyIndex;
                ++notifyInfo;
            }
        }
        return;
    }

    if (notifyInfo->time >= time || oldTime > notifyInfo->time) {
        return;
    }

    while (notifyIndex < parts->notifyCount && time > notifyInfo->time) {
        Scr_AddConstString(notifyInfo->name);
        Scr_NotifyNum(tree_s->entnum - 1, 0, info->notifyName, 1);
        ++notifyIndex;
        ++notifyInfo;
    }
}

static float XAnimSiblingGoalBlendTime(const XAnimTree_s *tree, unsigned int animIndex, float goalWeight, float goalTime)
{
    const XAnimEntry *parentAnim;
    unsigned int parentIndex;
    unsigned int numAnims;
    unsigned int firstChild;
    float bestWeight;
    unsigned int i;

    if (!animIndex) {
        return goalWeight;
    }

    parentIndex = tree->anims->entries[animIndex].parent;
    parentAnim = &tree->anims->entries[parentIndex];
    numAnims = parentAnim->numAnims;
    firstChild = parentAnim->u.s.children;
    bestWeight = 0.0f;

    for (i = 0; i < numAnims; ++i) {
        unsigned int childIndex = firstChild + i;
        unsigned int infoIndex = tree->infoArray[childIndex];
        float weight = infoIndex ? g_xAnimInfo[infoIndex].s.goalWeight : 0.0f;

        if (childIndex == animIndex) {
            weight = goalWeight - weight;
            if (weight < 0.0f) {
                weight = -weight;
            }
        }

        if (bestWeight < weight) {
            bestWeight = weight;
        }
    }

    return goalTime * bestWeight;
}

static void XAnimClearGoalWeightForBlend(XAnimInfo *info, float blendTime)
{
    XAnimState *state = &info->s;

    if (state->goalWeight != 0.0f) {
        state->goalTime = blendTime;
    } else if (state->goalTime > blendTime) {
        state->goalTime = blendTime;
    }

    state->goalWeight = 0.0f;
    if (blendTime == 0.0f) {
        state->rate = 0.0f;
    }

    XAnimClearInfoNotify(info);
}

static void XAnimClearSiblingGoalWeights(XAnimTree *tree, unsigned int keepAnimIndex, float blendTime)
{
    XAnimTree_s *tree_s = (XAnimTree_s *)tree;
    const XAnimEntry *keepAnim = &tree_s->anims->entries[keepAnimIndex];
    const XAnimEntry *parentAnim = &tree_s->anims->entries[keepAnim->parent];
    unsigned int firstChild = parentAnim->u.s.children;
    unsigned int i;

    for (i = 0; i < parentAnim->numAnims; ++i) {
        unsigned int childIndex = firstChild + i;
        unsigned int infoIndex;

        if (childIndex == keepAnimIndex) {
            continue;
        }

        infoIndex = tree_s->infoArray[childIndex];
        if (infoIndex) {
            XAnimClearGoalWeightForBlend(&g_xAnimInfo[infoIndex], blendTime);
        }
    }
}

int XAnimSetCompleteGoalWeightKnobAll(XAnimTree *tree, unsigned int animIndex, unsigned int rootIndex, float goalWeight, float goalTime, float rate, unsigned int notifyName, int bRestart)
{
    XAnimTree_s *tree_s = (XAnimTree_s *)tree;
    unsigned int currentIndex;
    int error;

    if (goalWeight < 0.0010000000474974513f) {
        goalWeight = 0.0f;
    }

    if (animIndex) {
        float blendTime = XAnimSiblingGoalBlendTime(tree_s, animIndex, goalWeight, goalTime);
        XAnimClearSiblingGoalWeights(tree, animIndex, blendTime);
    }

    error = XAnimSetGoalWeightInternal(tree, animIndex, 0, goalWeight, goalTime, rate, notifyName, 0);

    if (animIndex) {
        unsigned int parentIndex = tree_s->anims->entries[animIndex].parent;

        while (!tree_s->infoArray[parentIndex]) {
            XAnimSetGoalWeightInternal(tree, parentIndex, 1, 0.0f, goalTime, 1.0f, 0, 0);
            if (!parentIndex) {
                break;
            }
            parentIndex = tree_s->anims->entries[parentIndex].parent;
        }
    }

    if (bRestart) {
        XAnimRestart(tree, animIndex);
    }

    currentIndex = animIndex;
    while (currentIndex) {
        unsigned int parentIndex = tree_s->anims->entries[currentIndex].parent;
        float blendTime;

        if (parentIndex == rootIndex) {
            return error;
        }
        if (!parentIndex) {
            return 1;
        }

        blendTime = XAnimSiblingGoalBlendTime(tree_s, currentIndex, 1.0f, goalTime);
        XAnimClearSiblingGoalWeights(tree, currentIndex, blendTime);
        XAnimSetGoalWeightInternal(tree, parentIndex, 0, 1.0f, goalTime, 1.0f, 0, 0);

        if (bRestart) {
            XAnimRestart(tree, parentIndex);
        }

        currentIndex = parentIndex;
    }

    return 1;
}

int XAnimSetGoalWeight(XAnimTree *tree, unsigned int animIndex, float goalWeight, float goalTime, float rate, unsigned int notifyName, unsigned int notifyType, int bRestart)
{
    XAnimTree_s *tree_s = (XAnimTree_s *)tree;
    unsigned int parentAnimIndex;
    int error;

    if (goalWeight < 0.0010000000474974513f) {
        goalWeight = 0.0f;
    }

    error = XAnimSetGoalWeightInternal(tree, animIndex, 0, goalWeight, goalTime, rate, notifyName, notifyType);

    parentAnimIndex = animIndex;
    while (parentAnimIndex) {
        parentAnimIndex = tree_s->anims->entries[parentAnimIndex].parent;
        if (tree_s->infoArray[parentAnimIndex]) {
            break;
        }

        XAnimSetGoalWeightInternal(tree, parentAnimIndex, 1, 0.0f, goalTime, 1.0f, 0, 0);
    }

    if (bRestart) {
        XAnimRestart(tree, animIndex);
    }

    return error;
}

static Bool XAnimDisplayShowsTime(const XAnimTree *tree, unsigned int animIndex)
{
    const XAnimTree_s *tree_s = (const XAnimTree_s *)tree;
    const XAnimEntry *anim = &tree_s->anims->entries[animIndex];

    return anim->numAnims == 0 || (anim->u.s.flags & 3) != 0;
}

static void __attribute_regparm__(3)
    XAnimDisplay(const XAnimTree *tree, unsigned int animIndex, int depth)
{
    const XAnimTree_s *tree_s = (const XAnimTree_s *)tree;
    unsigned short infoIndex = tree_s->infoArray[animIndex];
    const XAnim *anims;
    const XAnimEntry *anim;
    const XAnimInfo *info;
    const XAnimState *state;
    const char *debugName;
    const char *color;
    int numAnims;

    if (!infoIndex) {
        return;
    }

    anims = tree_s->anims;
    anim = &anims->entries[animIndex];
    numAnims = anim->numAnims;
    info = &g_xAnimInfo[infoIndex];
    state = &info->s;

    for (int i = 0; i < depth; ++i) {
        Com_Printf((const char *)" ");
    }

    debugName = XAnimGetAnimDebugName(anims, animIndex);

    if (state->goalWeight > state->weight) {
        color = (const char *)"^4";
    } else if (state->goalWeight < state->weight) {
        color = (const char *)"^1";
    } else {
        color = (const char *)"";
    }

    if (!numAnims) {
        float realTimeDelta = state->time - state->oldTime;
        float frequency = anim->u.parts->frequency;

        if (realTimeDelta < 0.0f) {
            realTimeDelta += 1.0f;
        }

        if (frequency != 0.0f) {
            realTimeDelta /= frequency;
        } else {
            realTimeDelta = 0.0f;
        }

        if (info->notifyName) {
            Com_Printf((const char *)"%s%s: (weight) %.2f -> %.2f, (time) %.2f -> %.2f, (realtimedelta) %.2f, '%s'\n",
                       color,
                       debugName,
                       state->goalWeight,
                       state->weight,
                       state->oldTime,
                       state->time,
                       realTimeDelta,
                       SL_ConvertToString(info->notifyName));
        } else {
            Com_Printf((const char *)"%s%s: (weight) %.2f -> %.2f, (time) %.2f -> %.2f, (realtimedelta) %.2f\n",
                       color,
                       debugName,
                       state->goalWeight,
                       state->weight,
                       state->oldTime,
                       state->time,
                       realTimeDelta);
        }
        return;
    }

    if (info->notifyName) {
        if (XAnimDisplayShowsTime(tree, animIndex)) {
            Com_Printf((const char *)"%s%s: (weight) %.2f -> %.2f, (time) %.2f -> %.2f, '%s'\n",
                       color,
                       debugName,
                       state->goalWeight,
                       state->weight,
                       state->oldTime,
                       state->time,
                       SL_ConvertToString(info->notifyName));
        } else {
            Com_Printf((const char *)"%s%s: (weight) %.2f -> %.2f, '%s'\n",
                       color,
                       debugName,
                       state->goalWeight,
                       state->weight,
                       SL_ConvertToString(info->notifyName));
        }
    } else if (XAnimDisplayShowsTime(tree, animIndex)) {
        Com_Printf((const char *)"%s%s: (weight) %.2f -> %.2f, (time) %.2f -> %.2f\n",
                   color,
                   debugName,
                   state->goalWeight,
                   state->weight,
                   state->oldTime,
                   state->time);
    } else {
        Com_Printf((const char *)"%s%s: (weight) %.2f -> %.2f\n",
                   color,
                   debugName,
                   state->goalWeight,
                   state->weight);
    }

    for (int i = 0; i < numAnims; ++i) {
        XAnimDisplay(tree, anim->u.s.children + i, depth + 1);
    }
}

void DObjDisplayAnim(DObj *obj)
{
    if (!obj->tree) {
        Com_Printf("NO TREE\n");
        return;
    }

    XAnimDisplay(obj->tree, 0, 0);
    Com_Printf("\n");
}

static void __attribute_regparm__(3)
    XAnimFillInSyncNodes_r(XAnim *anims, unsigned int animIndex, Bool bLoop)
{
    XAnimEntry *anim = &anims->entries[animIndex];
    int numAnims = anim->numAnims;

    if (!numAnims) {
        XAnimParts *parts = anim->u.parts;

        if (parts->bLoop == bLoop) {
            return;
        }

        if (parts->isDefault) {
            XAnimPrecache((const char *)"void_loop", Hunk_AllocXAnimPrecache);
            anim->u.parts = (XAnimParts *)Hunk_FindDataForFile(5, (const char *)"void_loop");
            if (!anim->u.parts) {
                Com_Error(1, (const char *)"\x15"
                                           "Cannot find 'xanim/%s'.\nThis is a default xanim file that you should have.\n",
                          (const char *)"void_loop");
            }
            return;
        }

        if (bLoop) {
            Com_Error(1, (const char *)"\x15"
                                       "animation '%s' in '%s' cannot be sync looping and nonlooping",
                      XAnimGetAnimDebugName(anims, animIndex), anims->debugName);
        } else {
            Com_Error(1, (const char *)"\x15"
                                       "animation '%s' in '%s' cannot be sync nonlooping and looping",
                      XAnimGetAnimDebugName(anims, animIndex), anims->debugName);
        }
        return;
    }

    if (anim->u.s.flags & 3) {
        int count = 0;
        XAnimEntry *walk = anim;

        do {
            ++count;
            walk = &anims->entries[walk->u.s.children];
        } while (walk->numAnims != 0);

        Com_Error(1, (const char *)"\x15"
                                   "duplicate specification of animation sync in '%s', %d nodes above '%s'",
                  anims->debugName, count, XAnimGetAnimDebugName(anims, animIndex));
    }

    anim->u.s.flags |= bLoop ? 1 : 2;

    for (int i = 0; i < numAnims; ++i) {
        XAnimFillInSyncNodes_r(anims, anim->u.s.children + i, bLoop);
    }
}

static void __attribute_regparm__(2)
    XAnimSetupSyncNodes_r(XAnim *anims, unsigned int animIndex)
{
    XAnimEntry *anim = &anims->entries[animIndex];
    int numAnims = anim->numAnims;
    unsigned int flag;

    if (!numAnims) {
        return;
    }

    flag = anim->u.s.flags & 3;
    if (flag) {
        Bool bLoop;

        if (flag == 3) {
            Com_Error(1, (const char *)"\x15"
                                       "animation cannot be sync looping and sync nonlooping");
        }

        anim->u.s.flags |= 4;
        bLoop = (flag == 1);

        for (int i = 0; i < numAnims; ++i) {
            XAnimFillInSyncNodes_r(anims, anim->u.s.children + i, bLoop);
        }
    } else {
        for (int i = 0; i < numAnims; ++i) {
            XAnimSetupSyncNodes_r(anims, anim->u.s.children + i);
        }
    }
}

void XAnimSetupSyncNodes(XAnim *anims)
{
    XAnimEntry *anim = &anims->entries[0];
    int numAnims = anim->numAnims;

    if (!numAnims) {
        return;
    }

    unsigned int flag = anim->u.s.flags & 3;

    if (flag) {
        Bool bLoop;
        int i;

        if (flag == 3) {
            Com_Error(1, "\x15"
                         "animation cannot be sync looping and sync nonlooping");
        }

        anim->u.s.flags |= 4;

        bLoop = (flag == 1);
        for (i = 0; i < numAnims; ++i) {
            XAnimFillInSyncNodes_r(anims, anim->u.s.children + i, bLoop);
        }
    } else {
        int i;

        for (i = 0; i < numAnims; ++i) {
            XAnimSetupSyncNodes_r(anims, anim->u.s.children + i);
        }
    }
}

static void XAnimInsertClientNotify(unsigned int notetrackName, unsigned int notifyType, float timeFrac)
{
    int insertIndex = g_notifyListSize - 1;

    while (insertIndex >= 0 && timeFrac < g_notifyList[insertIndex].timeFrac) {
        g_notifyList[insertIndex + 1] = g_notifyList[insertIndex];
        --insertIndex;
    }

    ++insertIndex;
    g_notifyList[insertIndex].name = SL_ConvertToString(notetrackName);
    g_notifyList[insertIndex].timeFrac = timeFrac;
    g_notifyList[insertIndex].type = notifyType;
    ++g_notifyListSize;
}

static float XAnimClientNotifyFrac(const XAnimState *state, float notetrackTime, float dtime)
{
    float oldTime = state->oldTime;
    float time = state->time;

    if (oldTime == 1.0f) {
        return 1.0f;
    }

    if (oldTime > time) {
        if (time > notetrackTime) {
            return (notetrackTime - oldTime + 1.0f) / dtime;
        }
        if (notetrackTime >= oldTime) {
            return (notetrackTime - oldTime) / dtime;
        }
        return 1.0f;
    }

    if ((time > notetrackTime || time == 1.0f) && notetrackTime >= oldTime) {
        return (notetrackTime - oldTime) / dtime;
    }

    return 1.0f;
}

static void __attribute_regparm__(2) __attribute_sseregparm__
    XAnimProcessClientNotify(XAnimInfo *info, const XAnimEntry *anim, float dtime)
{
    XAnimState *state = &info->s;
    unsigned int notifyType = info->notifyType;
    const XAnimParts *parts;
    const XAnimNotifyInfo *notify;
    int notifyCount;
    int i;

    if (!notifyType) {
        return;
    }

    if (state->oldTime == 1.0f) {
        XAnimInsertClientNotify(g_end, notifyType, 1.0f);
        return;
    }

    if (anim->numAnims) {
        if (state->oldTime > state->time || state->time == 1.0f) {
            XAnimInsertClientNotify(g_end, notifyType, XAnimClientNotifyFrac(state, 1.0f, dtime));
        }
        return;
    }

    parts = anim->u.parts;
    notify = parts->notify;
    notifyCount = parts->notifyCount;
    if (notifyCount <= 0) {
        return;
    }

    if (state->oldTime > state->time) {
        for (i = 0; i < notifyCount; ++i) {
            if (notify[i].time >= state->oldTime) {
                XAnimInsertClientNotify(notify[i].name, notifyType, XAnimClientNotifyFrac(state, notify[i].time, dtime));
            }
        }

        for (i = 0; i < notifyCount; ++i) {
            if (notify[i].time >= state->time) {
                break;
            }
            XAnimInsertClientNotify(notify[i].name, notifyType, XAnimClientNotifyFrac(state, notify[i].time, dtime));
        }
        return;
    }

    for (i = 0; i < notifyCount; ++i) {
        if (notify[i].time < state->oldTime) {
            continue;
        }
        if (state->time != 1.0f && notify[i].time >= state->time) {
            break;
        }
        XAnimInsertClientNotify(notify[i].name, notifyType, XAnimClientNotifyFrac(state, notify[i].time, dtime));
    }
}

static __attribute_regparm__(3) __attribute_sseregparm__
    void XAnimUpdateInfoSyncInternal(const XAnimTree *tree, unsigned int animIndex, Bool bNotify, XAnimState *syncState, float dtime)
{
    const XAnimTree_s *tree_s;
    unsigned int infoIndex;
    XAnimInfo *info;
    XAnimState *state;
    const XAnimEntry *anim;
    unsigned int numAnims;
    unsigned int i;

    tree_s = (const XAnimTree_s *)tree;
    infoIndex = tree_s->infoArray[animIndex];
    if (!infoIndex) {
        return;
    }

    info = &g_xAnimInfo[infoIndex];
    state = &info->s;

    if (state->weight == 0.0f) {
        return;
    }

    if (state->goalWeight == 0.0f) {
        bNotify = 0;
    }

    anim = &tree_s->anims->entries[animIndex];

    if (syncState->oldTime != state->oldTime || syncState->oldTimeCount != state->oldTimeCount) {
        state->time = syncState->oldTime;
        state->timeCount = syncState->oldTimeCount;
        state->oldTime = syncState->oldTime;
        state->oldTimeCount = syncState->oldTimeCount;
        info->notifyIndex = -1;
    }

    if (bNotify) {
        XAnimProcessServerNotify(tree, info, anim, syncState->time);
    }

    state->time = syncState->time;
    state->timeCount = syncState->timeCount;
    info->notifyIndex = -1;

    if (bNotify) {
        XAnimProcessClientNotifyArgs(info, anim, dtime);
    }

    numAnims = anim->numAnims;
    for (i = 0; i < numAnims; ++i) {
        XAnimUpdateInfoSyncInternal(tree, anim->u.s.children + i, bNotify, state, dtime);
    }
}

static __attribute_regparm__(3) __attribute_sseregparm__
    void XAnimUpdateInfoInternal(const XAnimTree *tree, float dtime, unsigned int animIndex, Bool bNotify)
{
    unsigned int infoIndex;
    XAnimInfo *info;
    XAnimState *state;
    const XAnimEntry *anim;
    unsigned int numAnims;
    float totalDtime;
    float newTime;
    int timeCount;
    int oldTimeCount;
    int i;
    const XAnimTree_s *tree_s = (const XAnimTree_s *)tree;

    infoIndex = tree_s->infoArray[animIndex];
    if (!infoIndex) {
        return;
    }

    info = &g_xAnimInfo[infoIndex];
    state = &info->s;
    if (state->weight == 0.0f) {
        return;
    }

    if (state->goalWeight == 0.0f) {
        bNotify = 0;
    }

    anim = &tree_s->anims->entries[animIndex];
    numAnims = anim->numAnims;

    if (!numAnims) {
        XAnimParts *parts = anim->u.parts;

        totalDtime = dtime * state->rate * parts->frequency;
        if (totalDtime == 0.0f) {
            return;
        }

        newTime = state->oldTime + totalDtime;
        timeCount = state->oldTimeCount;
        if (newTime >= 1.0f) {
            if (parts->bLoop) {
                do {
                    newTime -= 1.0f;
                    ++timeCount;
                } while (newTime >= 1.0f);
            } else {
                newTime = 1.0f;
            }
        }

        if (state->time - newTime > (float)(timeCount - state->oldTimeCount)) {
            return;
        }

        if (bNotify) {
            XAnimProcessServerNotify(tree, info, anim, newTime);
        }

        state->time = newTime;
        state->timeCount = (short)timeCount;
        info->notifyIndex = -1;

        if (bNotify) {
            XAnimProcessClientNotifyArgs(info, anim, totalDtime);
        }
        return;
    }

    if (!(anim->u.s.flags & 3)) {
        totalDtime = dtime * state->rate;
        if (totalDtime == 0.0f) {
            return;
        }

        for (i = 0; i < (int)numAnims; ++i) {
            XAnimUpdateInfoInternal(tree, totalDtime, anim->u.s.children + (unsigned int)i, bNotify);
        }
        return;
    }

    {
        float totalWeight = 0.0f;
        float weightedFrequency = 0.0f;

        for (i = 0; i < (int)numAnims; ++i) {
            unsigned int childAnimIndex = anim->u.s.children + (unsigned int)i;
            unsigned int childInfoIndex = tree_s->infoArray[childAnimIndex];
            XAnimInfo *childInfo;
            float childWeight;
            float averageRateFrequency;

            if (!childInfoIndex) {
                continue;
            }

            childInfo = &g_xAnimInfo[childInfoIndex];
            childWeight = childInfo->s.weight;
            if (childWeight == 0.0f) {
                continue;
            }

            averageRateFrequency = XAnimGetAverageRateFrequency(tree, childAnimIndex);
            if (averageRateFrequency != 0.0f) {
                totalWeight += childWeight;
                weightedFrequency += childWeight * averageRateFrequency * childInfo->s.rate;
            }
        }

        if (totalWeight != 0.0f) {
            totalDtime = weightedFrequency / totalWeight;
        } else {
            totalDtime = 0.0f;
        }
    }

    totalDtime *= state->rate;
    totalDtime *= dtime;
    if (totalDtime == 0.0f) {
        return;
    }

    newTime = state->oldTime + totalDtime;
    timeCount = state->oldTimeCount;
    if (newTime >= 1.0f) {
        if (anim->u.s.flags & 2) {
            newTime = 1.0f;
        } else {
            do {
                newTime -= 1.0f;
                ++timeCount;
            } while (newTime >= 1.0f);
        }
    }

    oldTimeCount = state->oldTimeCount;
    if (state->time - newTime > (float)(timeCount - oldTimeCount)) {
        return;
    }

    if (bNotify) {
        XAnimProcessServerNotify(tree, info, anim, newTime);
    }

    state->time = newTime;
    state->timeCount = (short)timeCount;
    info->notifyIndex = -1;

    if (bNotify) {
        XAnimProcessClientNotifyArgs(info, anim, totalDtime);
    }

    for (i = 0; i < (int)numAnims; ++i) {
        unsigned int childAnimIndex = anim->u.s.children + (unsigned int)i;
        unsigned int childInfoIndex = tree_s->infoArray[childAnimIndex];
        XAnimInfo *childInfo;
        XAnimState *childState;
        const XAnimEntry *childAnim;
        Bool childNotify;
        int childCount;

        if (!childInfoIndex) {
            continue;
        }

        childInfo = &g_xAnimInfo[childInfoIndex];
        childState = &childInfo->s;
        if (childState->weight == 0.0f) {
            continue;
        }

        childNotify = 0;
        if (childState->goalWeight != 0.0f) {
            childNotify = bNotify;
        }

        childAnim = &tree_s->anims->entries[childAnimIndex];

        if (state->oldTime != childState->oldTime || state->oldTimeCount != childState->oldTimeCount) {
            childState->time = state->oldTime;
            childState->timeCount = state->oldTimeCount;
            childState->oldTime = state->oldTime;
            childState->oldTimeCount = state->oldTimeCount;
            childInfo->notifyIndex = -1;
        }

        if (childNotify) {
            XAnimProcessServerNotify(tree, childInfo, childAnim, childState->time);
        }

        childState->time = state->time;
        childState->timeCount = state->timeCount;
        childInfo->notifyIndex = -1;

        if (childNotify) {
            XAnimProcessClientNotifyArgs(childInfo, childAnim, totalDtime);
        }

        childCount = childAnim->numAnims;
        {
            int j;
            for (j = 0; j < childCount; ++j) {
                XAnimUpdateInfoSyncInternal(tree, childAnim->u.s.children + (unsigned int)j, childNotify, childState, totalDtime);
            }
        }
    }
}

void DObjUpdateClientInfo(DObj *obj, float dtime)
{
    g_notifyListSize = 0;

    if (obj->tree) {
        XAnimState syncState;
        Bool infoExists;
        Bool childHadWeight;

        syncState.time = 0.0f;
        syncState.timeCount = 0;
        XAnimUpdateOldTime(obj->tree, 0, &syncState, 1, &infoExists, &childHadWeight, dtime);
        XAnimUpdateInfoInternal(obj->tree, dtime, 0, 1);
    }
}
static inline __attribute__((always_inline)) float XAnimGetNotifyFrac(const XAnimState *syncState, const XAnimState *nextSyncState, float notifyTime, float dtime)
{
    float oldTime = nextSyncState->oldTime;
    float time;
    int oldTimeCountDelta;

    if (oldTime == 1.0f) {
        return 1.0f;
    }

    time = nextSyncState->time;
    oldTimeCountDelta = (int)nextSyncState->oldTimeCount - (int)syncState->oldTimeCount;

    if (oldTime > time) {
        if (time > notifyTime) {
            return ((float)(oldTimeCountDelta + 1) + (notifyTime - syncState->oldTime)) / dtime;
        }

        if (notifyTime >= oldTime) {
            return ((float)oldTimeCountDelta + (notifyTime - syncState->oldTime)) / dtime;
        }

        return 1.0f;
    }

    if ((time > notifyTime || time == 1.0f) && notifyTime >= oldTime) {
        return ((float)oldTimeCountDelta + (notifyTime - syncState->oldTime)) / dtime;
    }

    return 1.0f;
}

static inline __attribute__((always_inline)) const XAnimNotifyInfo *XAnimGetNextNotifyInfo(const XAnimParts *parts, float time)
{
    const XAnimNotifyInfo *bestNotifyInfo = NULL;
    float bestTime = 2.0f;
    int notifyInfoIndex;

    for (notifyInfoIndex = 0; notifyInfoIndex < parts->notifyCount; ++notifyInfoIndex) {
        const XAnimNotifyInfo *notifyInfo = &parts->notify[notifyInfoIndex];
        float notifyTime = notifyInfo->time;

        if (time <= notifyTime && bestTime > notifyTime) {
            bestNotifyInfo = notifyInfo;
            bestTime = notifyTime;
        }
    }

    return bestNotifyInfo;
}

static float __attribute_regparm__(3) __attribute_sseregparm__
    XAnimGetNotifyFracServer(const XAnimTree *tree, XAnimInfo *info, const XAnimEntry *anim,
                             const XAnimState *syncState, const XAnimState *nextSyncState,
                             float dtime)
{
    const XAnimTree_s *tree_s = (const XAnimTree_s *)tree;
    const XAnimEntry *notifyAnim;
    const XAnimParts *parts;
    float notifyTime;

    if (!tree_s->entnum || !info->notifyName) {
        return 1.0f;
    }

    notifyAnim = anim;
    if (anim->numAnims) {
        if (!info->notifyChild) {
            return XAnimGetNotifyFrac(syncState, nextSyncState, 1.0f, dtime);
        }
        notifyAnim = &tree_s->anims->entries[info->notifyChild];
    }

    parts = notifyAnim->u.parts;

    if (info->notifyIndex < 0) {
        const XAnimNotifyInfo *bestNotifyInfo;

        if (info->s.time == 1.0f) {
            return XAnimGetNotifyFrac(syncState, nextSyncState, 1.0f, dtime);
        }

        if (anim->numAnims) {
            if (!info->notifyChild) {
                return XAnimGetNotifyFrac(syncState, nextSyncState, 1.0f, dtime);
            }
            parts = tree_s->anims->entries[info->notifyChild].u.parts;
        }

        bestNotifyInfo = XAnimGetNextNotifyInfo(parts, info->s.time);
        info->notifyIndex = bestNotifyInfo ? (short)(bestNotifyInfo - parts->notify) : -1;
        if (info->notifyIndex < 0) {
            return XAnimGetNotifyFrac(syncState, nextSyncState, 1.0f, dtime);
        }
    }

    notifyTime = parts->notify[info->notifyIndex].time;
    return XAnimGetNotifyFrac(syncState, nextSyncState, notifyTime, dtime);
}

static float __attribute_regparm__(3) __attribute_sseregparm__
    XAnimGetServerNotifyFracSyncTotal(const XAnimTree *tree, XAnimInfo *info, const XAnimEntry *anim,
                                      const XAnimState *syncState, const XAnimState *nextSyncState,
                                      float dtime)
{
    const XAnimTree_s *tree_s = (const XAnimTree_s *)tree;
    float minFrac = XAnimGetNotifyFracServer(tree, info, anim, syncState, nextSyncState, dtime);
    unsigned int i;

    for (i = 0; i < anim->numAnims; ++i) {
        unsigned int childIndex = anim->u.s.children + i;
        unsigned int infoIndex = tree_s->infoArray[childIndex];
        XAnimInfo *childInfo;
        const XAnimEntry *childAnim;
        float testFrac;

        if (!infoIndex) {
            continue;
        }

        childInfo = &g_xAnimInfo[infoIndex];
        if (childInfo->s.weight == 0.0f || childInfo->s.goalWeight == 0.0f) {
            continue;
        }

        childAnim = &tree_s->anims->entries[childIndex];
        testFrac = XAnimGetServerNotifyFracSyncTotal(tree, childInfo, childAnim, syncState, nextSyncState, dtime);
        if (!(minFrac <= testFrac)) {
            minFrac = testFrac;
        }
    }

    return minFrac;
}
static float XAnimUpdateServerNotifyFrac(const XAnimTree *tree, XAnimInfo *info,
                                         const XAnimEntry *anim, XAnimState *state,
                                         float totalDtime, float nextTime,
                                         unsigned short nextTimeCount)
{
    const XAnimTree_s *tree_s = (const XAnimTree_s *)tree;
    XAnimState nextState;
    float frac;
    unsigned int i;

    nextState.time = nextTime;
    nextState.oldTime = state->time;
    nextState.timeCount = (short)nextTimeCount;
    nextState.oldTimeCount = state->timeCount;
    nextState.goalTime = 0.0f;
    nextState.goalWeight = 0.0f;
    nextState.weight = 0.0f;
    nextState.rate = 0.0f;

    frac = XAnimGetNotifyFracServer(tree, info, anim, state, &nextState, totalDtime);

    for (i = 0; i < anim->numAnims; ++i) {
        unsigned int childIndex = anim->u.s.children + i;
        unsigned int infoIndex = tree_s->infoArray[childIndex];
        XAnimInfo *childInfo;
        const XAnimEntry *childAnim;
        float childFrac;

        if (!infoIndex) {
            continue;
        }

        childInfo = &g_xAnimInfo[infoIndex];
        if (childInfo->s.weight == 0.0f && childInfo->s.goalWeight == 0.0f) {
            continue;
        }

        childAnim = &tree_s->anims->entries[childIndex];
        childFrac = XAnimGetServerNotifyFracSyncTotal(tree, childInfo, childAnim, state, &nextState, totalDtime);
        if (!(frac <= childFrac)) {
            frac = childFrac;
        }
    }

    return frac;
}

static float XAnimFindServerNoteTrackInternal(const XAnimTree *tree, unsigned int animIndex, float dtime)
{
    const XAnimTree_s *tree_s = (const XAnimTree_s *)tree;
    unsigned int infoIndex = tree_s->infoArray[animIndex];
    XAnimInfo *info;
    XAnimState *state;
    const XAnimEntry *anim;
    unsigned int numAnims;
    float totalDtime;
    float nextTime;
    unsigned short nextTimeCount;

    if (!infoIndex) {
        return 1.0f;
    }

    info = &g_xAnimInfo[infoIndex];
    state = &info->s;

    if (state->weight == 0.0f || state->goalWeight == 0.0f) {
        return 1.0f;
    }

    anim = &tree_s->anims->entries[animIndex];
    numAnims = anim->numAnims;

    if (!numAnims) {
        const XAnimParts *parts = anim->u.parts;

        totalDtime = state->rate * parts->frequency * dtime;
        if (totalDtime == 0.0f) {
            return 1.0f;
        }

        nextTime = state->oldTime + totalDtime;
        nextTimeCount = state->oldTimeCount;

        if (parts->bLoop) {
            while (nextTime >= 1.0f) {
                nextTime -= 1.0f;
                ++nextTimeCount;
            }
        } else if (nextTime >= 1.0f) {
            nextTime = 1.0f;
        }

        if (state->time - nextTime > (float)((int)nextTimeCount - (int)state->timeCount)) {
            return 1.0f;
        }

        return XAnimGetNotifyFracServer(tree, info, anim, state,
                                        &(XAnimState){ nextTime, state->time, (short)nextTimeCount, state->timeCount, 0.0f, 0.0f, 0.0f, 0.0f },
                                        totalDtime);
    }

    if (anim->u.s.flags & 3) {
        totalDtime = XAnimGetAverageRateFrequency(tree, animIndex) * state->rate * dtime;
        if (totalDtime == 0.0f) {
            return 1.0f;
        }

        nextTime = state->oldTime + totalDtime;
        nextTimeCount = state->oldTimeCount;

        if (anim->u.s.flags & 2) {
            if (nextTime >= 1.0f) {
                nextTime = 1.0f;
            }
        } else {
            while (nextTime >= 1.0f) {
                nextTime -= 1.0f;
                ++nextTimeCount;
            }
        }

        if (state->time - nextTime > (float)((int)nextTimeCount - (int)state->timeCount)) {
            return 1.0f;
        }

        return XAnimUpdateServerNotifyFrac(tree, info, anim, state, totalDtime, nextTime, nextTimeCount);
    }

    totalDtime = dtime * state->rate;
    if (totalDtime == 0.0f) {
        return 1.0f;
    }

    if (numAnims > 0) {
        float frac = 1.0f;
        unsigned int i;

        for (i = 0; i < numAnims; ++i) {
            unsigned int childIndex = anim->u.s.children + i;
            float childFrac = XAnimFindServerNoteTrackInternal(tree, childIndex, totalDtime);
            if (!(childFrac >= frac)) {
                frac = childFrac;
            }
        }

        return frac;
    }

    return 1.0f;
}

static float __attribute_regparm__(3) __attribute_sseregparm__
    XAnimFindServerNoteTrack(const XAnimTree *tree, unsigned int animIndex, float dtime)
{
    return XAnimFindServerNoteTrackInternal(tree, animIndex, dtime);
}

int DObjUpdateServerInfo(DObj *obj, float dtime, int bNotify)
{
    XAnimTree *tree = obj->tree;

    if (!tree) {
        return 0;
    }

    if (!bNotify) {
        XAnimUpdateInfoInternal(tree, dtime, 0, 0);
        return 0;
    }

    float notifyFrac = XAnimFindServerNoteTrack(tree, 0, dtime);
    if (notifyFrac != 1.0f) {
        float notifyDtime = notifyFrac * dtime + 0.0010000000474974513f;
        if (dtime >= notifyDtime) {
            XAnimUpdateInfoInternal(tree, notifyDtime, 0, 1);
            return 1;
        }
    }

    XAnimUpdateInfoInternal(tree, dtime, 0, 1);
    return 0;
}
