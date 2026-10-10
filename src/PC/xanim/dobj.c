#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

extern int XModelGetLodForDist(XModel *model, float dist);
extern float XModelGetLodOutDist(const XModel *model);
extern int XModelBad(const XModel *model);
extern const char *XModelGetName(const XModel *model);
extern int XModelGetBoneIndex(const XModel *model, unsigned int name);
extern int XModelGetSurfaces(const XModel *model, struct XSurface_s ***surfaces, int lod, int **partBits);
extern int XModelGetContents(const XModel *model);
extern int XModelTraceLine(const XModel *model, trace_t *results, const DObjAnimMat *boneMtxList, vec_t *localStart, vec_t *localEnd, int contentmask);
extern void XModelGetBounds(const XModel *model, vec_t *mins, vec_t *maxs);
extern unsigned int SL_FindString(const char *str);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern void SL_RemoveRefToStringOfLen(unsigned int stringValue, unsigned int len);
extern unsigned int SL_GetStringOfLen(void *duplicatePartBits, int user, int len, int flag);
extern void Com_Printf(const char *fmt, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern float sinf(float x);
extern float cosf(float x);
extern float sqrtf(float x);
extern void DObjCalcAnim(const DObj *obj, int *partBits);

static unsigned int g_empty;

void DObjInit(void);
void DObjShutdown(void);
void DObjAbort(void);
Bool DObjIgnoreCollision(const DObj *obj, int modelIndex);
int DObjSkelIsBoneUpToDate(DObj *obj, int boneIndex);
int DObjSkelAreBonesUpToDate(const DObj *obj, int *partBits);
void DObjFree(DObj_s *obj);
int DObjGetAllocSkelSize(const DObj *obj);
qboolean DObjSkelExists(const DObj *obj, int timeStamp);
void DObjSkelClear(const DObj *obj);
void DObjCreateSkel(const DObj *obj, char *buf, int timeStamp);
int DObjGetNumModels(const DObj *obj);
XModel *DObjGetModel(const DObj *obj, int modelIndex);
DObjAnimMat *DObjGetRotTransArray(const DObj *obj);
int DObjGetMatOffset(const DObj *obj, int modelIndex);
void DObjGetBoneInfo(const DObj *obj, XBoneInfo **boneInfo);
int DObjGetNumSurfaces(const DObj *obj, char *lods);
struct XSurface_s *DObjGetSurface(const DObj *obj, int modelIndex, int subMatIndex, int lod);
const char *DObjGetSurfaceName(DObj *obj, int modelIndex, int subMatIndex, int lod);
int DObjGetBoneIndex(const DObj *obj, unsigned int boneName);
XAnimTree *DObjGetTree(const DObj *obj);
int DObjBad(const DObj *obj);
int DObjNumBones(const DObj *obj);
int DObjGetLodForDist(const DObj *obj, int modelIndex, float dist);
float DObjGetLodOutDist(const DObj *obj);
int DObjHasContents(DObj *obj, int contentmask);
void DObjSetModel(DObj *obj, const XModel *model);
void DObjGetBounds(const DObj *obj, vec_t *mins, vec_t *maxs);
void DObjGeomTraceline(DObj *obj, vec_t *localStart, vec_t *localEnd, int contentmask, DObjTrace *results);
int DObjGetSurfaces(const DObj *obj, DSurface *surfaces, int *partBits, char *lods);
static void DObjCreateDuplicateParts(const DObj *obj) __attribute_regparm__(1);

static inline __attribute__((always_inline)) int DObjFindBoneIndexForTag(const DObj *obj, unsigned int tagName)
{
    int numModels = obj->numModels;
    int boneIndex = 0;
    int modelIndex;

    for (modelIndex = 0; modelIndex < numModels; ++modelIndex) {
        const XModel *model = obj->models[modelIndex];
        int modelBoneIndex = XModelGetBoneIndex(model, tagName);

        if (modelBoneIndex >= 0) {
            return boneIndex + modelBoneIndex;
        }

        boneIndex += ((const XModelParts *)model->parts)->numBones;
    }

    return -1;
}
void DObjCompleteHierarchyBits(const DObj *obj, int *partBits);
void DObjGetHierarchyBits(DObj *obj, int boneIndex, int *partBits);
void DObjSetLocalTagInternal(const DObj *obj, const vec_t *trans, const vec_t *angles, int boneIndex);
qboolean DObjSetLocalTag(const DObj *obj, int *partBits, unsigned int tagName, const vec_t *trans, const vec_t *angles);
qboolean DObjSetControlTagAngles(const DObj *obj, int *partBits, unsigned int tagName, vec_t *angles);
void DObjDumpInfo(const DObj *obj);
void DObjCreate(DObjModel_s *dobjModels, unsigned int numModels, XAnimTree_s *tree, char *buf, unsigned int entnum);
void DObjCalcSkel(const DObj *obj, int *partBits);
void DObjTraceline(DObj *obj, vec_t *start, vec_t *end, unsigned char *priorityMap, DObjTrace *trace);

void DObjInit(void)
{
    int duplicatePartBits[5];
    int *dp = duplicatePartBits;
    duplicatePartBits[0] = 0;
    duplicatePartBits[1] = 0;
    duplicatePartBits[2] = 0;
    duplicatePartBits[3] = 0;
    duplicatePartBits[4] = 0;
    g_empty = SL_GetStringOfLen(dp, 0, 0x11, 0xc);
}

void DObjShutdown(void)
{
    if (!g_empty)
        return;
    SL_RemoveRefToStringOfLen(g_empty, 0x11);
    g_empty = 0;
}

void DObjAbort(void)
{
    g_empty = 0;
}

Bool DObjIgnoreCollision(const DObj *obj, int modelIndex)
{
    return (obj->ignoreCollision >> modelIndex) & 1;
}

int DObjSkelIsBoneUpToDate(DObj *obj, int boneIndex)
{
    DSkel *skel = obj->skel;
    return (skel->skelPartBits[boneIndex >> 5] >> (boneIndex & 0x1f)) & 1;
}

int DObjSkelAreBonesUpToDate(const DObj *obj, int *partBits)
{
    int i;
    DSkel *skel = obj->skel;
    for (i = 1; i < 5; i++) {
        if (~skel->skelPartBits[i - 1] & partBits[i - 1])
            return 0;
    }
    return 1;
}

void DObjFree(DObj_s *obj)
{
    if (obj->tree) {
        obj->animToModel = NULL;
        obj->tree = NULL;
    }
    if (obj->duplicateParts) {
        if ((unsigned int)obj->duplicateParts != g_empty) {
            const char *str = SL_ConvertToString(obj->duplicateParts);
            int len = strlen(str + 16) + 17;
            SL_RemoveRefToStringOfLen(obj->duplicateParts, len);
        }
        obj->duplicateParts = 0;
    }
}

int DObjGetAllocSkelSize(const DObj *obj)
{
    return (obj->numBones << 5) + 0x30;
}

qboolean DObjSkelExists(const DObj *obj, int timeStamp)
{
    if (obj->timeStamp != timeStamp) {
        ((DObj *)obj)->skel = NULL;
        return 0;
    }
    return obj->skel != NULL;
}

void DObjSkelClear(const DObj *obj)
{
    ((DObj *)obj)->timeStamp = 0;
    ((DObj *)obj)->skel = NULL;
}

void DObjCreateSkel(const DObj *obj, char *buf, int timeStamp)
{
    int i;
    DSkel *skel = (DSkel *)buf;
    ((DObj *)obj)->skel = skel;
    ((DObj *)obj)->timeStamp = timeStamp;
    for (i = 0; i < 4; i++) {
        skel->animPartBits[i] = 0;
        skel->controlPartBits[i] = 0;
        skel->skelPartBits[i] = 0;
    }
}

int DObjGetNumModels(const DObj *obj)
{
    return obj->numModels;
}

XModel *DObjGetModel(const DObj *obj, int modelIndex)
{
    return obj->models[modelIndex];
}

DObjAnimMat *DObjGetRotTransArray(const DObj *obj)
{
    DSkel *skel = obj->skel;
    if (skel)
        return skel->mat;
    return 0;
}

int DObjGetMatOffset(const DObj *obj, int modelIndex)
{
    return obj->matOffset[modelIndex];
}

void DObjGetBoneInfo(const DObj *obj, XBoneInfo **boneInfo)
{
    int j, i;
    for (j = 0; j < obj->numModels; j++) {
        XModel *model = obj->models[j];
        int size = *(short *)(*(int *)model);
        for (i = 0; i < size; i++) {
            *boneInfo++ = &model->boneInfo[i];
        }
    }
}

int DObjGetNumSurfaces(const DObj *obj, char *lods)
{
    int numSurfaces = 0;
    int numModels = obj->numModels;
    int i;
    for (i = numModels - 1; i >= 0; i--) {
        signed char lod = lods[i];
        if (lod >= 0) {
            XModel *model = obj->models[i];
            XModelLodInfo *lodInfo = &model->lodInfo[(unsigned char)lod];
            if (lodInfo->surfs) {
                numSurfaces += lodInfo->numsurfs;
            }
        }
    }
    return numSurfaces;
}

struct XSurface_s *DObjGetSurface(const DObj *obj, int modelIndex, int subMatIndex, int lod)
{
    XModel *model = obj->models[modelIndex];
    XModelSurfs *surfs = model->lodInfo[lod].surfs;
    int *surfList = *(int **)surfs;
    return (struct XSurface_s *)*(int *)(surfList + subMatIndex);
}

const char *DObjGetSurfaceName(DObj *obj, int modelIndex, int subMatIndex, int lod)
{
    XModel *model = obj->models[modelIndex];
    unsigned short *matNames = model->lodInfo[lod].surfNames;
    unsigned short name = matNames[subMatIndex];
    if (!name)
        return (const char *)"DEFAULT";
    return SL_ConvertToString(name);
}

int DObjGetBoneIndex(const DObj *obj, unsigned int boneName)
{
    int j;
    int numModels = obj->numModels;
    int boneIndex = 0;

    for (j = 0; j < numModels; ++j) {
        XModel *model = obj->models[j];
        int localBoneIndex = XModelGetBoneIndex(model, boneName);

        if (localBoneIndex >= 0) {
            return boneIndex + localBoneIndex;
        }

        boneIndex += ((XModelParts *)model->parts)->numBones;
    }

    return -1;
}

XAnimTree *DObjGetTree(const DObj *obj)
{
    return *(XAnimTree **)obj;
}

int DObjBad(const DObj *obj)
{
    int i;

    for (i = obj->numModels - 1; i >= 0; --i) {
        if (XModelBad(obj->models[i])) {
            return 1;
        }
    }

    return 0;
}

int DObjNumBones(const DObj *obj)
{
    return obj->numBones;
}

int DObjGetLodForDist(const DObj *obj, int modelIndex, float dist)
{
    return XModelGetLodForDist(obj->models[modelIndex], dist);
}

float DObjGetLodOutDist(const DObj *obj)
{
    int modelIndex;
    float lodOutDist = 0.0f;

    for (modelIndex = 0; modelIndex < obj->numModels; ++modelIndex) {
        float curDist = XModelGetLodOutDist(obj->models[modelIndex]);

        if (curDist > lodOutDist) {
            lodOutDist = curDist;
        }
    }

    return lodOutDist;
}

int DObjHasContents(DObj *obj, int contentmask)
{
    int i;

    for (i = 0; i < obj->numModels; ++i) {
        if (XModelGetContents(obj->models[i]) & contentmask) {
            return 1;
        }
    }

    return 0;
}

void DObjSetModel(DObj *obj, const XModel *model)
{
    obj->skel = (DSkel *)(*(int *)model + 0x14);
    obj->numBones = (unsigned char)*(unsigned short *)*(int *)model;
    obj->models[0] = (XModel *)model;
}

void DObjGetBounds(const DObj *obj, vec_t *mins, vec_t *maxs)
{
    const vec_t *objMins = obj->mins;
    const vec_t *objMaxs;

    mins[0] = obj->mins[0];
    mins[1] = objMins[1];
    mins[2] = objMins[2];

    objMaxs = obj->maxs;
    maxs[0] = obj->maxs[0];
    maxs[1] = objMaxs[1];
    maxs[2] = objMaxs[2];
}

void DObjGeomTraceline(DObj *obj, vec_t *localStart, vec_t *localEnd, int contentmask, DObjTrace *results)
{
    trace_t trace;

    results->partName = 0;
    results->partGroup = 0;

    trace.fraction = results->fraction;
    trace.surfaceFlags = 0;
    trace.normal[0] = 0.0f;
    trace.normal[1] = 0.0f;
    trace.normal[2] = 0.0f;

    if (obj->skel) {
        DObjAnimMat *boneMtxList = obj->skel->mat;
        int i;

        if (boneMtxList) {
            for (i = 0; i < obj->numModels; ++i) {
                XModel *model = obj->models[i];
                XModelParts *modelParts = (XModelParts *)model->parts;
                unsigned short *names = modelParts->hierarchy->names;
                int hit = XModelTraceLine(model, &trace, boneMtxList, localStart, localEnd, contentmask);

                if (hit >= 0) {
                    results->partName = names[hit];
                }

                boneMtxList += modelParts->numBones;
            }
        }
    }

    results->fraction = trace.fraction;
    results->surfaceflags = trace.surfaceFlags;
    {
        vec_t *outNormal = results->normal;
        outNormal[0] = trace.normal[0];
        outNormal[1] = trace.normal[1];
        outNormal[2] = trace.normal[2];
    }
}

int DObjGetSurfaces(const DObj *obj, DSurface *surfaces, int *partBits, char *lods)
{
    int modelIndex;
    int surfaceCount = 0;

    partBits[0] = 0;
    partBits[1] = 0;
    partBits[2] = 0;
    partBits[3] = 0;

    for (modelIndex = 0; modelIndex < obj->numModels; ++modelIndex) {
        signed char lod = (signed char)lods[modelIndex];
        XModel *model;
        XModelLodInfo *lodInfo;
        XModelSurfs *surfs;
        int numBoneBits;
        int numsurfs;
        int surfIndex;
        int targBoneIndex;
        int targWord;
        int targBit;
        int *surfPartBits;

        if (lod < 0) {
            continue;
        }

        model = obj->models[modelIndex];
        lodInfo = &model->lodInfo[(unsigned char)lod];
        surfs = lodInfo->surfs;
        if (!surfs) {
            continue;
        }

        numBoneBits = (((XModelParts *)model->parts)->numBones - 1) >> 5;
        numsurfs = lodInfo->numsurfs;

        if (surfaceCount + numsurfs > 64) {
            Com_Printf("ERROR: models with more than %i total surfaces\n", 64);
            for (surfIndex = 0; surfIndex < obj->numModels; ++surfIndex) {
                struct XSurface_s **debugSurfs;
                int *debugPartBits;
                int debugLod = (signed char)lods[surfIndex];
                int debugCount = XModelGetSurfaces(obj->models[surfIndex], &debugSurfs, debugLod, &debugPartBits);

                Com_Printf("  model '%s' lod %i has %i surfaces\n",
                           XModelGetName(obj->models[surfIndex]), debugLod, debugCount);
            }
            Com_Error(1, "Max surfs exceeded - see console for details");
        }

        for (surfIndex = 0; surfIndex < numsurfs; ++surfIndex) {
            surfaces[surfaceCount].modelIndex = (short)modelIndex;
            surfaces[surfaceCount].subMatIndex = (short)surfIndex;
            ++surfaceCount;
        }

        targBoneIndex = obj->matOffset[modelIndex];
        targWord = targBoneIndex >> 5;
        targBit = targBoneIndex & 31;
        surfPartBits = surfs->partBits;

        if (targBit) {
            int invTargBit = 32 - targBit;
            int i;

            partBits[targWord] |= surfPartBits[0] << targBit;
            ++targWord;

            for (i = 0; i < numBoneBits; ++i) {
                partBits[targWord + i] |=
                    (surfPartBits[i] >> invTargBit) | (surfPartBits[i + 1] << targBit);
            }

            partBits[targWord + numBoneBits] |= surfPartBits[numBoneBits] >> invTargBit;
        } else if (numBoneBits >= 0) {
            int i;

            for (i = 0; i <= numBoneBits; ++i) {
                partBits[targWord + i] |= surfPartBits[i];
            }
        }
    }

    return surfaceCount;
}

static void __attribute_regparm__(1) DObjCreateDuplicateParts(const DObj *obj)
{
    struct {
        int duplicatePartBits[4];
        unsigned char duplicateParts[1024];
    } data;
    int boneIndex;
    int duplicatePartByteCount;
    int modelIndex;

    data.duplicatePartBits[0] = 0;
    data.duplicatePartBits[1] = 0;
    data.duplicatePartBits[2] = 0;
    data.duplicatePartBits[3] = 0;

    boneIndex = ((XModelParts *)obj->models[0]->parts)->numBones;
    duplicatePartByteCount = 0;

    for (modelIndex = 1; modelIndex < obj->numModels; ++modelIndex) {
        XModel *model = obj->models[modelIndex];
        XModelParts *modelParts = (XModelParts *)model->parts;
        unsigned short *boneNames;
        int numBones;
        int localBoneIndex;
        int rootMelded;

        if (obj->modelParents[modelIndex] != 0xff) {
            boneIndex += modelParts->numBones;
            continue;
        }

        boneNames = modelParts->hierarchy->names;
        numBones = modelParts->numBones;
        rootMelded = 0;

        for (localBoneIndex = 0; localBoneIndex < numBones; ++localBoneIndex) {
            int name = boneNames[localBoneIndex];
            int parentIndex = DObjFindBoneIndexForTag(obj, (unsigned int)name);
            int globalBoneIndex = boneIndex + localBoneIndex;

            if (parentIndex != globalBoneIndex) {
                if (localBoneIndex == 0) {
                    rootMelded = 1;
                }

                data.duplicateParts[duplicatePartByteCount] = (unsigned char)(globalBoneIndex + 1);
                data.duplicatePartBits[globalBoneIndex >> 5] |= 1 << (globalBoneIndex & 31);
                data.duplicateParts[duplicatePartByteCount + 1] = (unsigned char)(parentIndex + 1);
                duplicatePartByteCount += 2;
            }
        }

        if (!rootMelded) {
            Com_Printf("WARNING: Attempting to meld model, but root part '%s' of model '%s' not found in model '%s' or any of its descendants\n",
                       SL_ConvertToString(boneNames[0]),
                       model->name,
                       obj->models[0]->name);
        }

        boneIndex += modelParts->numBones;
    }

    if (duplicatePartByteCount) {
        data.duplicateParts[duplicatePartByteCount] = 0;
        ((DObj *)obj)->duplicateParts = (unsigned short)SL_GetStringOfLen(
            &data,
            0,
            duplicatePartByteCount + 0x11,
            0xc);
    } else {
        ((DObj *)obj)->duplicateParts = (unsigned short)g_empty;
    }
}

static inline __attribute__((always_inline)) int DObjPartBitIsSet(const int *partBits, int boneIndex)
{
    return (partBits[boneIndex >> 5] >> (boneIndex & 31)) & 1;
}

static inline __attribute__((always_inline)) void DObjSetPartBit(int *partBits, int boneIndex)
{
    partBits[boneIndex >> 5] |= 1 << (boneIndex & 31);
}

static inline __attribute__((always_inline)) int DObjGetDuplicatePartTarget(const unsigned char *duplicateParts, int boneIndex)
{
    while (((int)duplicateParts[0] - 1) != boneIndex) {
        duplicateParts += 2;
    }

    return (int)duplicateParts[1] - 1;
}

static inline __attribute__((always_inline)) int DObjGetModelStartIndexes(const DObj *obj, int boneIndex, int *startIndexes)
{
    int modelIndex = 0;
    int nextStartIndex;

    startIndexes[0] = 0;
    nextStartIndex = ((XModelParts *)obj->models[0]->parts)->numBones;

    while (boneIndex >= nextStartIndex) {
        ++modelIndex;
        startIndexes[modelIndex] = nextStartIndex;
        nextStartIndex += ((XModelParts *)obj->models[modelIndex]->parts)->numBones;
    }

    return modelIndex;
}

static inline __attribute__((always_inline)) int DObjFindModelForBoneFromIndex(int boneIndex, int modelIndex, const int *startIndexes)
{
    while (modelIndex > 0 && boneIndex < startIndexes[modelIndex]) {
        --modelIndex;
    }

    return modelIndex;
}

void DObjCompleteHierarchyBits(const DObj *obj, int *partBits)
{
    const char *duplicatePartBitsString;
    const int *duplicatePartBits;
    const unsigned char *duplicateParts;
    int startIndexes[8];
    int boneIndex;
    int modelIndex;

    boneIndex = obj->numBones - 1;

    if (!obj->duplicateParts) {
        DObjCreateDuplicateParts(obj);
    }

    duplicatePartBitsString = SL_ConvertToString(obj->duplicateParts);
    duplicatePartBits = (const int *)duplicatePartBitsString;
    duplicateParts = (const unsigned char *)(duplicatePartBitsString + 0x10);

    modelIndex = DObjGetModelStartIndexes(obj, boneIndex, startIndexes);

    while (modelIndex >= 0) {
        XModel *model = obj->models[modelIndex];
        XModelParts *modelParts = (XModelParts *)model->parts;
        unsigned char *parentList = modelParts->hierarchy->parentList;
        int startIndex = startIndexes[modelIndex];
        int localBoneIndex = boneIndex - startIndex;

        while (localBoneIndex >= 0) {
            if (DObjPartBitIsSet(partBits, boneIndex)) {
                int parentBoneIndex;

                if (DObjPartBitIsSet(duplicatePartBits, boneIndex)) {
                    parentBoneIndex = DObjGetDuplicatePartTarget(duplicateParts, boneIndex);
                } else {
                    int nonRootLocalIndex = localBoneIndex - modelParts->numRootBones;

                    if (nonRootLocalIndex >= 0) {
                        parentBoneIndex = boneIndex - parentList[nonRootLocalIndex];
                    } else {
                        parentBoneIndex = obj->modelParents[modelIndex];
                        if ((unsigned char)(parentBoneIndex + 1) == 0) {
                            --boneIndex;
                            localBoneIndex = boneIndex - startIndex;
                            continue;
                        }
                    }
                }

                DObjSetPartBit(partBits, parentBoneIndex);
            }

            --boneIndex;
            localBoneIndex = boneIndex - startIndex;
        }

        --modelIndex;
    }
}

void DObjGetHierarchyBits(DObj *obj, int boneIndex, int *partBits)
{
    const char *duplicatePartBitsString;
    const int *duplicatePartBits;
    const unsigned char *duplicateParts;
    int startIndexes[8];
    int modelIndex;
    int i;

    for (i = 0; i < 4; ++i) {
        partBits[i] = 0;
    }

    if (!obj->duplicateParts) {
        DObjCreateDuplicateParts(obj);
    }

    duplicatePartBitsString = SL_ConvertToString(obj->duplicateParts);
    duplicatePartBits = (const int *)duplicatePartBitsString;
    duplicateParts = (const unsigned char *)(duplicatePartBitsString + 0x10);

    modelIndex = DObjGetModelStartIndexes(obj, boneIndex, startIndexes);

    if (getenv("DBGSPAM")) Com_Printf("[ckpt] hier-enter obj=%p numModels=%d numBones=%d mp[0]=%d mp[1]=%d matOff[0]=%d\n",
               (void *)obj, obj->numModels, obj->numBones, obj->modelParents[0], obj->modelParents[1], obj->matOffset[0]);
    int dbgN = 0;
    for (;;) {
        XModel *model = obj->models[modelIndex];
        XModelParts *modelParts = (XModelParts *)model->parts;
        int localBoneIndex = boneIndex - startIndexes[modelIndex];

        if (getenv("DBGSPAM") && dbgN < 40) { dbgN++; Com_Printf("[ckpt] hier boneIndex=%d modelIndex=%d localBI=%d numRoot=%d start=%d\n",
                                            boneIndex, modelIndex, localBoneIndex, modelParts ? modelParts->numRootBones : -1, startIndexes[modelIndex]); }

        DObjSetPartBit(partBits, boneIndex);

        if (DObjPartBitIsSet(duplicatePartBits, boneIndex)) {
            boneIndex = DObjGetDuplicatePartTarget(duplicateParts, boneIndex);
        } else {
            localBoneIndex -= modelParts->numRootBones;
            if (localBoneIndex >= 0) {
                boneIndex -= modelParts->hierarchy->parentList[localBoneIndex];
                continue;
            }

            boneIndex = obj->modelParents[modelIndex];
            if ((unsigned char)(boneIndex + 1) == 0) {
                return;
            }
        }

        modelIndex = DObjFindModelForBoneFromIndex(boneIndex, modelIndex, startIndexes);
    }
}

static void DObjAnglesToQuatLocal(const vec_t *angles, float *quat)
{
    if (angles) {
        const float halfDegToRad = 0.008726646259971648f;
        float sy = sinf(angles[1] * halfDegToRad);
        float cy = cosf(angles[1] * halfDegToRad);
        float sp = sinf(angles[0] * halfDegToRad);
        float cp = cosf(angles[0] * halfDegToRad);
        float sr = sinf(angles[2] * halfDegToRad);
        float cr = cosf(angles[2] * halfDegToRad);
        float negSpSy = -sp * sy;
        float spCy = sp * cy;
        float syCp = sy * cp;
        float cyCp = cy * cp;

        quat[0] = sr * cyCp + cr * negSpSy;
        quat[1] = cr * spCy + sr * syCp;
        quat[2] = cr * syCp - sr * spCy;
        quat[3] = cr * cyCp - sr * negSpSy;
    } else {
        quat[0] = 0.0f;
        quat[1] = 0.0f;
        quat[2] = 0.0f;
        quat[3] = 1.0f;
    }
}

void DObjSetLocalTagInternal(const DObj *obj, const vec_t *trans, const vec_t *angles, int boneIndex)
{
    DObjAnimMat *rotTrans;

    if (!obj->skel) {
        return;
    }

    rotTrans = &obj->skel->mat[boneIndex];
    DObjAnglesToQuatLocal(angles, rotTrans->quat);

    rotTrans->transWeight = 0.0f;
    rotTrans->trans[0] = trans[0];
    rotTrans->trans[1] = trans[1];
    rotTrans->trans[2] = trans[2];
}

qboolean DObjSetLocalTag(const DObj *obj, int *partBits, unsigned int tagName, const vec_t *trans, const vec_t *angles)
{
    int boneIndex;
    int boneIndexHigh;
    int boneIndexLow;

    boneIndex = DObjFindBoneIndexForTag(obj, tagName);
    if (boneIndex < 0) {
        return 0;
    }

    boneIndexHigh = boneIndex >> 5;
    boneIndexLow = 1 << (boneIndex & 0x1f);

    if (!(partBits[boneIndexHigh] & boneIndexLow)) {
        return 0;
    }

    if (obj->skel->skelPartBits[boneIndexHigh] & boneIndexLow) {
        return 0;
    }

    obj->skel->animPartBits[boneIndexHigh] |= boneIndexLow;
    DObjSetLocalTagInternal(obj, trans, angles, boneIndex);
    return 1;
}

qboolean DObjSetControlTagAngles(const DObj *obj, int *partBits, unsigned int tagName, vec_t *angles)
{
    int boneIndex;
    int boneIndexHigh;
    int boneIndexLow;

    boneIndex = DObjFindBoneIndexForTag(obj, tagName);
    if (boneIndex < 0) {
        return 0;
    }

    boneIndexHigh = boneIndex >> 5;
    boneIndexLow = 1 << (boneIndex & 0x1f);

    if (!(partBits[boneIndexHigh] & boneIndexLow)) {
        return 0;
    }

    {
        DSkel *skel = obj->skel;

        if ((skel->skelPartBits[boneIndexHigh] | skel->controlPartBits[boneIndexHigh]) & boneIndexLow) {
            return 0;
        }

        /* Controllers add aim/lean to the sampled pose. Replacing it with a
         * zero translation and the controller rotation collapses the spine
         * and erases the walk/run animation, even for a zero-angle control. */
        if (!(skel->animPartBits[boneIndexHigh] & boneIndexLow)) {
            DObjCalcAnim(obj, partBits);
        }
        DObjAnimMat *mat = &skel->mat[boneIndex];
        float control[4];
        float animated[4];
        DObjAnglesToQuatLocal(angles, control);
        memcpy(animated, mat->quat, sizeof(animated));
        mat->quat[0] = control[3] * animated[0] + control[0] * animated[3] + control[1] * animated[2] - control[2] * animated[1];
        mat->quat[1] = control[3] * animated[1] - control[0] * animated[2] + control[1] * animated[3] + control[2] * animated[0];
        mat->quat[2] = control[3] * animated[2] + control[0] * animated[1] - control[1] * animated[0] + control[2] * animated[3];
        mat->quat[3] = control[3] * animated[3] - control[0] * animated[0] - control[1] * animated[1] - control[2] * animated[2];
        skel->controlPartBits[boneIndexHigh] |= boneIndexLow;
        skel->animPartBits[boneIndexHigh] |= boneIndexLow;
    }
    return 1;
}

static inline __attribute__((always_inline))
const char *DObjGetBoneNameString(const DObj *obj, int boneIndex)
{
    int modelIndex;
    int baseBoneIndex = 0;

    for (modelIndex = 0; modelIndex < obj->numModels; ++modelIndex) {
        const XModel *model = obj->models[modelIndex];
        const XModelParts *parts = (const XModelParts *)model->parts;
        int localBoneIndex = boneIndex - baseBoneIndex;

        if (localBoneIndex < parts->numBones) {
            return SL_ConvertToString(parts->hierarchy->names[localBoneIndex]);
        }

        baseBoneIndex += parts->numBones;
    }

    return 0;
}

void DObjDumpInfo(const DObj *obj)
{
    int modelIndex;
    int boneIndex;

    if (!obj) {
        Com_Printf("No Dobj\n");
        return;
    }

    Com_Printf("\nModels:\n");
    boneIndex = 0;
    for (modelIndex = 0; modelIndex < obj->numModels; ++modelIndex) {
        XModel *model = obj->models[modelIndex];
        Com_Printf("%d: '%s'\n", boneIndex, model->name);
        boneIndex += ((const XModelParts *)model->parts)->numBones;
    }

    Com_Printf("\nBones:\n");
    for (boneIndex = 0; boneIndex < obj->numBones; ++boneIndex) {
        Com_Printf("Bone %d: '%s'\n", boneIndex, DObjGetBoneNameString(obj, boneIndex));
    }

    if (obj->duplicateParts) {
        const unsigned char *duplicateParts;

        Com_Printf("\nPart duplicates:\n");
        duplicateParts = (const unsigned char *)SL_ConvertToString(obj->duplicateParts) + 0x10;
        while (duplicateParts[0]) {
            int fromBoneIndex = duplicateParts[0] - 1;
            int toBoneIndex = duplicateParts[1] - 1;

            Com_Printf("%d ('%s') -> %d ('%s')\n",
                       fromBoneIndex,
                       DObjGetBoneNameString(obj, fromBoneIndex),
                       toBoneIndex,
                       DObjGetBoneNameString(obj, toBoneIndex));
            duplicateParts += 2;
        }
    } else {
        Com_Printf("\nNo part duplicates.\n");
    }

    Com_Printf("\n");
}

void DObjCreate(DObjModel_s *dobjModels, unsigned int numModels, XAnimTree_s *tree, char *buf, unsigned int entnum)
{
    DObj *obj = (DObj *)buf;
    unsigned int modelIndex;
    unsigned int newNumModels;
    int boneIndex;
    unsigned char numBones;
    float mins[3];
    float maxs[3];

    obj->skel = 0;
    obj->timeStamp = 0;
    obj->duplicateParts = 0;
    obj->ignoreCollision = 0;

    obj->tree = tree;
    if (tree) {
        int animCount = tree->anims->size;
        int animBytes = 2 * animCount;
        unsigned char *pAge;
        unsigned char nextAge;

        obj->animToModel = (unsigned short *)((char *)tree + 8 + animBytes);
        pAge = (unsigned char *)obj->animToModel + animBytes;
        nextAge = (unsigned char)(pAge[0] + 1);
        if (!nextAge) {
            memset(pAge + 1, 0, animCount);
            nextAge = 1;
        }
        pAge[0] = nextAge;
        tree->entnum = (unsigned short)entnum;
    } else {
        obj->animToModel = 0;
    }

    if (numModels) {
        newNumModels = 0;
        boneIndex = 0;
        numBones = 0;

        do {
            DObjModel_s *dobjModel = &dobjModels[newNumModels];
            XModel *model = dobjModel->model;

            obj->models[newNumModels] = model;
            obj->modelParents[newNumModels] = 0xff;
            numBones = (unsigned char)boneIndex;
            obj->matOffset[newNumModels] = numBones;

            if (dobjModel->ignoreCollision) {
                obj->ignoreCollision |= (unsigned char)(1 << newNumModels);
            }

            if (newNumModels && dobjModel->boneName && dobjModel->boneName[0]) {
                unsigned int name = SL_FindString(dobjModel->boneName);

                if (name) {
                    unsigned int parentModelIndex;

                    for (parentModelIndex = 0; parentModelIndex < newNumModels; ++parentModelIndex) {
                        int parentBoneIndex = XModelGetBoneIndex(obj->models[parentModelIndex], name);

                        if (parentBoneIndex >= 0) {
                            obj->modelParents[newNumModels] = (unsigned char)(parentBoneIndex + obj->matOffset[parentModelIndex]);
                            break;
                        }
                    }

                    if (parentModelIndex != newNumModels) {
                        goto got_model_parent;
                    }
                }

                Com_Printf("WARNING: Part '%s' not found in model '%s' or any of its descendants\n", dobjModel->boneName, obj->models[0]->name);
            }

        got_model_parent:
            if (model) {
                boneIndex += ((XModelParts *)model->parts)->numBones;
                if (boneIndex > 0x7f) {
                    Com_Error(1, "\x15"
                                 "dobj for xmodel '%s' has more than %d bones",
                              obj->models[0]->name, 0x7f);
                    break;
                }
                numBones = (unsigned char)boneIndex;
            }

            ++newNumModels;
        } while (newNumModels != numModels);
    } else {
        newNumModels = 0;
        numBones = 0;
    }

    obj->numModels = (unsigned char)newNumModels;
    obj->numBones = numBones;

    mins[0] = 0.0f;
    mins[1] = 0.0f;
    mins[2] = 0.0f;
    maxs[0] = 0.0f;
    maxs[1] = 0.0f;
    maxs[2] = 0.0f;

    for (modelIndex = 0; modelIndex < obj->numModels; ++modelIndex) {
        XModel *model = obj->models[modelIndex];

        if (model) {
            vec3_t modelMin;
            vec3_t modelMax;

            XModelGetBounds(model, modelMin, modelMax);
            mins[0] += modelMin[0];
            mins[1] += modelMin[1];
            mins[2] += modelMin[2];
            maxs[0] += modelMax[0];
            maxs[1] += modelMax[1];
            maxs[2] += modelMax[2];
        }
    }

    obj->mins[0] = mins[0];
    obj->mins[1] = mins[1];
    obj->mins[2] = mins[2];
    obj->maxs[0] = maxs[0];
    obj->maxs[1] = maxs[1];
    obj->maxs[2] = maxs[2];
}

static inline __attribute__((always_inline)) int DObjPartBitTestLocal(const int *bits, int boneIndex)
{
    return (bits[boneIndex >> 5] >> (boneIndex & 31)) & 1;
}

static inline __attribute__((always_inline)) void DObjPartBitSetLocal(int *bits, int boneIndex)
{
    bits[boneIndex >> 5] |= 1 << (boneIndex & 31);
}

static inline __attribute__((always_inline)) void DObjCopyAnimMatLocal(DObjAnimMat *dst, const DObjAnimMat *src)
{
    dst->quat[0] = src->quat[0];
    dst->quat[1] = src->quat[1];
    dst->quat[2] = src->quat[2];
    dst->quat[3] = src->quat[3];
    dst->trans[0] = src->trans[0];
    dst->trans[1] = src->trans[1];
    dst->trans[2] = src->trans[2];
    dst->transWeight = src->transWeight;
}

static inline __attribute__((always_inline)) void DObjFinalizeQuatWeightLocal(DObjAnimMat *mat)
{
    float lenSq = mat->quat[0] * mat->quat[0] + mat->quat[1] * mat->quat[1] + mat->quat[2] * mat->quat[2] + mat->quat[3] * mat->quat[3];

    if (lenSq == 0.0f) {
        mat->quat[0] = 0.0f;
        mat->quat[1] = 0.0f;
        mat->quat[2] = 0.0f;
        mat->quat[3] = 1.0f;
        mat->transWeight = 2.0f;
    } else {
        mat->transWeight = 2.0f / lenSq;
    }
}

static inline __attribute__((always_inline)) void DObjComposeAnimMatLocal(DObjAnimMat *child, const DObjAnimMat *parent)
{
    DObjAnimMat local = *child;
    const float px = parent->quat[0];
    const float py = parent->quat[1];
    const float pz = parent->quat[2];
    const float pw = parent->quat[3];
    const float cx = local.quat[0];
    const float cy = local.quat[1];
    const float cz = local.quat[2];
    const float cw = local.quat[3];
    const float scale = parent->transWeight;
    const float xx = px * px * scale;
    const float yy = py * py * scale;
    const float zz = pz * pz * scale;
    const float xy = px * py * scale;
    const float xz = px * pz * scale;
    const float yz = py * pz * scale;
    const float wx = pw * px * scale;
    const float wy = pw * py * scale;
    const float wz = pw * pz * scale;
    const float lx = local.trans[0];
    const float ly = local.trans[1];
    const float lz = local.trans[2];

    child->quat[0] = pw * cx + px * cw + py * cz - pz * cy;
    child->quat[1] = pw * cy - px * cz + py * cw + pz * cx;
    child->quat[2] = pw * cz + px * cy - py * cx + pz * cw;
    child->quat[3] = pw * cw - px * cx - py * cy - pz * cz;

    child->trans[0] = parent->trans[0] + (1.0f - yy - zz) * lx + (xy - wz) * ly + (xz + wy) * lz;
    child->trans[1] = parent->trans[1] + (xy + wz) * lx + (1.0f - xx - zz) * ly + (yz - wx) * lz;
    child->trans[2] = parent->trans[2] + (xz - wy) * lx + (yz + wx) * ly + (1.0f - xx - yy) * lz;

    DObjFinalizeQuatWeightLocal(child);
}

static inline __attribute__((always_inline)) void DObjApplyDuplicatePartLocal(DObjAnimMat *mat, const unsigned char *duplicateParts, int boneIndex)
{
    const unsigned char *pos = duplicateParts;

    if (!pos) {
        return;
    }

    while (pos[0]) {
        int dstBone = (int)pos[0] - 1;
        int srcBone = (int)pos[1] - 1;

        if (dstBone == boneIndex) {
            if (srcBone >= 0) {
                DObjCopyAnimMatLocal(&mat[boneIndex], &mat[srcBone]);
            }
            return;
        }
        pos += 2;
    }
}

void DObjCalcSkel(const DObj *obj, int *partBits)
{
    DSkel *skel;
    DObjAnimMat *mat;
    int neededBits[4];
    int anyNeeded = 0;
    const unsigned char *duplicateParts = NULL;
    int modelIndex;
    int globalBoneIndex = 0;

    if (!obj || !partBits || !obj->skel) {
        return;
    }

    skel = obj->skel;
    mat = skel->mat;

    for (modelIndex = 0; modelIndex < 4; ++modelIndex) {
        neededBits[modelIndex] = partBits[modelIndex] & ~skel->skelPartBits[modelIndex];
        if (neededBits[modelIndex]) {
            anyNeeded = 1;
        }
    }

    if (!anyNeeded) {
        return;
    }

    DObjCalcAnim(obj, partBits);

    if (obj->duplicateParts) {
        const char *duplicateString = SL_ConvertToString(obj->duplicateParts);
        duplicateParts = (const unsigned char *)duplicateString + 16;
    }

    for (modelIndex = 0; modelIndex < obj->numModels; ++modelIndex) {
        XModel *model = obj->models[modelIndex];
        XModelParts *parts;
        int numRootBones;
        int numBones;
        int childCount;
        int childIndex;
        DObjAnimMat *modelParentMat = NULL;

        if (!model || !model->parts) {
            continue;
        }

        parts = (XModelParts *)model->parts;
        numRootBones = parts->numRootBones;
        numBones = parts->numBones;
        childCount = numBones - numRootBones;

        if (obj->modelParents[modelIndex] != 0xff) {
            modelParentMat = &mat[obj->modelParents[modelIndex]];
        }

        while (numRootBones-- > 0) {
            if (DObjPartBitTestLocal(neededBits, globalBoneIndex)) {
                if (modelParentMat) {
                    DObjComposeAnimMatLocal(&mat[globalBoneIndex], modelParentMat);
                } else {
                    DObjFinalizeQuatWeightLocal(&mat[globalBoneIndex]);
                }
                DObjApplyDuplicatePartLocal(mat, duplicateParts, globalBoneIndex);
                DObjPartBitSetLocal(skel->skelPartBits, globalBoneIndex);
            }
            ++globalBoneIndex;
        }

        for (childIndex = 0; childIndex < childCount; ++childIndex) {
            if (DObjPartBitTestLocal(neededBits, globalBoneIndex)) {
                int parentDelta = 1;
                int parentBoneIndex;

                if (parts->hierarchy) {
                    parentDelta = parts->hierarchy->parentList[childIndex];
                    if (parentDelta <= 0) {
                        parentDelta = 1;
                    }
                }

                parentBoneIndex = globalBoneIndex - parentDelta;
                if (parentBoneIndex >= 0) {
                    DObjComposeAnimMatLocal(&mat[globalBoneIndex], &mat[parentBoneIndex]);
                } else {
                    DObjFinalizeQuatWeightLocal(&mat[globalBoneIndex]);
                }

                DObjApplyDuplicatePartLocal(mat, duplicateParts, globalBoneIndex);
                DObjPartBitSetLocal(skel->skelPartBits, globalBoneIndex);
            }
            ++globalBoneIndex;
        }
    }
}

static void DObjTraceBuildAxesLocal(const DObjAnimMat *mat, float axis[3][3])
{
    const float x = mat->quat[0];
    const float y = mat->quat[1];
    const float z = mat->quat[2];
    const float w = mat->quat[3];
    const float scale = mat->transWeight;
    const float sx = scale * x;
    const float sy = scale * y;
    const float sz = scale * z;
    const float xx = sx * x;
    const float xy = sx * y;
    const float xz = sx * z;
    const float xw = sx * w;
    const float yy = sy * y;
    const float yz = sy * z;
    const float yw = sy * w;
    const float zz = sz * z;
    const float zw = sz * w;

    axis[0][0] = 1.0f - yy - zz;
    axis[0][1] = xy + zw;
    axis[0][2] = xz - yw;
    axis[1][0] = xy - zw;
    axis[1][1] = 1.0f - xx - zz;
    axis[1][2] = yz + xw;
    axis[2][0] = xz + yw;
    axis[2][1] = yz - xw;
    axis[2][2] = 1.0f - xx - yy;
}

static void DObjTraceTransformPointLocal(const DObjAnimMat *mat, const vec3_t in, vec3_t out)
{
    const float x = mat->quat[0];
    const float y = mat->quat[1];
    const float z = mat->quat[2];
    const float w = mat->quat[3];
    const float scale = mat->transWeight;
    const float sx = scale * x;
    const float sy = scale * y;
    const float sz = scale * z;
    const float xx = sx * x;
    const float xy = sx * y;
    const float xz = sx * z;
    const float xw = sx * w;
    const float yy = sy * y;
    const float yz = sy * z;
    const float yw = sy * w;
    const float zz = sz * z;
    const float zw = sz * w;

    out[0] = mat->trans[0] + (1.0f - yy - zz) * in[0] + (xy - zw) * in[1] + (xz + yw) * in[2];
    out[1] = mat->trans[1] + (xy + zw) * in[0] + (1.0f - xx - zz) * in[1] + (yz - xw) * in[2];
    out[2] = mat->trans[2] + (xz - yw) * in[0] + (yz + xw) * in[1] + (1.0f - xx - yy) * in[2];
}

static void DObjTraceInversePointLocal(const DObjAnimMat *mat, const vec3_t in, vec3_t out)
{
    float v[3];
    float axis[3][3];

    v[0] = in[0] - mat->trans[0];
    v[1] = in[1] - mat->trans[1];
    v[2] = in[2] - mat->trans[2];
    DObjTraceBuildAxesLocal(mat, axis);

    out[0] = axis[0][0] * v[0] + axis[0][1] * v[1] + axis[0][2] * v[2];
    out[1] = axis[1][0] * v[0] + axis[1][1] * v[1] + axis[1][2] * v[2];
    out[2] = axis[2][0] * v[0] + axis[2][1] * v[1] + axis[2][2] * v[2];
}

static int DObjTraceLineAabbLocal(const vec3_t start, const vec3_t end,
                                  const vec3_t mins, const vec3_t maxs,
                                  float limit, float *hitFrac,
                                  int *hitAxis, float *hitSign,
                                  int *startSolid, int *endSolid)
{
    int axis;
    float enter = 0.0f;
    float leave = limit;
    int enterAxis = 0;
    float enterSign = 0.0f;
    int bStartSolid = 1;
    int bEndSolid = 1;

    for (axis = 0; axis < 3; ++axis) {
        const float s = start[axis];
        const float e = end[axis];
        const float d = e - s;

        if (s < mins[axis]) {
            bStartSolid = 0;
            if (e < mins[axis]) {
                return 0;
            }
            if (d != 0.0f) {
                const float f = (mins[axis] - s) / d;
                if (f > enter) {
                    enter = f;
                    enterAxis = axis;
                    enterSign = -1.0f;
                }
            }
        } else if (s > maxs[axis]) {
            bStartSolid = 0;
            if (e > maxs[axis]) {
                return 0;
            }
            if (d != 0.0f) {
                const float f = (maxs[axis] - s) / d;
                if (f > enter) {
                    enter = f;
                    enterAxis = axis;
                    enterSign = 1.0f;
                }
            }
        }

        if (e < mins[axis]) {
            bEndSolid = 0;
            if (d != 0.0f) {
                const float f = (mins[axis] - s) / d;
                if (f < leave) {
                    leave = f;
                }
            }
        } else if (e > maxs[axis]) {
            bEndSolid = 0;
            if (d != 0.0f) {
                const float f = (maxs[axis] - s) / d;
                if (f < leave) {
                    leave = f;
                }
            }
        }

        if (enter >= leave) {
            return 0;
        }
    }

    *hitFrac = enter;
    *hitAxis = enterAxis;
    *hitSign = enterSign;
    *startSolid = bStartSolid;
    *endSolid = bEndSolid;
    return 1;
}

static void DObjTraceSetStartSolidNormalLocal(const vec3_t start, const vec3_t delta, DObjTrace *trace)
{
    trace->normal[0] = 0.0f;
    trace->normal[1] = 0.0f;
    trace->normal[2] = 0.0f;

    if (delta[0] != 0.0f || delta[1] != 0.0f) {
        float lenSq;
        trace->normal[0] = start[0];
        trace->normal[1] = start[1];
        lenSq = trace->normal[0] * trace->normal[0] + trace->normal[1] * trace->normal[1];
        if (lenSq > 0.0f) {
            float invLen = 1.0f / sqrtf(lenSq);
            trace->normal[0] *= invLen;
            trace->normal[1] *= invLen;
        }
    } else {
        trace->normal[2] = (delta[2] < 0.0f) ? 1.0f : -1.0f;
    }
}

void DObjTraceline(DObj *obj, vec_t *start, vec_t *end, unsigned char *priorityMap, DObjTrace *trace)
{
    DObjAnimMat *boneMatrix;
    const unsigned char *duplicateParts;
    float delta[3];
    float lenSq;
    float invLenSq;
    float solidHitFrac;
    int lowestPriority;
    int globalBoneIndex;
    int modelIndex;
    int hitAxis = -1;
    float hitSign = 0.0f;
    DObjAnimMat *hitBoneMatrix = NULL;
    unsigned short boneClassifications[128];

    trace->surfaceflags = 0;
    trace->partName = 0;
    trace->partGroup = 0;
    trace->normal[0] = 0.0f;
    trace->normal[1] = 0.0f;
    trace->normal[2] = 0.0f;

    delta[0] = end[0] - start[0];
    delta[1] = end[1] - start[1];
    delta[2] = end[2] - start[2];
    lenSq = delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2];
    if (lenSq == 0.0f || !obj || !obj->skel || !obj->skel->mat || !obj->numModels) {
        return;
    }

    boneMatrix = obj->skel->mat;
    invLenSq = 1.0f / lenSq;
    duplicateParts = obj->duplicateParts ? (const unsigned char *)SL_ConvertToString(obj->duplicateParts) + 16 : NULL;
    solidHitFrac = trace->fraction;
    lowestPriority = 2;
    globalBoneIndex = 0;

    for (modelIndex = 0; modelIndex < obj->numModels; ++modelIndex) {
        XModel *model = obj->models[modelIndex];
        XModelParts *modelParts;
        XBoneHierarchy *hierarchy;
        unsigned short *names;
        int numBones;
        int localBoneIndex;
        int ignoreCollision;

        if (!model || !model->parts) {
            continue;
        }

        modelParts = (XModelParts *)model->parts;
        hierarchy = modelParts->hierarchy;
        names = hierarchy ? hierarchy->names : NULL;
        numBones = modelParts->numBones;
        ignoreCollision = obj->ignoreCollision & (1 << modelIndex);

        for (localBoneIndex = 0; localBoneIndex < numBones; ++localBoneIndex, ++globalBoneIndex, ++boneMatrix) {
            unsigned short classification = modelParts->partClassification ? modelParts->partClassification[localBoneIndex] : 0;
            int currentPriority = priorityMap[classification];
            XBoneInfo *boneInfo;
            vec3_t center;
            vec3_t startLocal;
            vec3_t endLocal;
            float startToCenter[3];
            float closestT;
            float closestDistSq;
            float radiusDelta;
            float traceLimit;
            float frac;
            int axis;
            float sign;
            int startSolid;
            int endSolid;

            if (duplicateParts && duplicateParts[0] && (int)duplicateParts[0] - 1 == globalBoneIndex) {
                if (currentPriority == 1) {
                    int srcBone = (int)duplicateParts[1] - 1;
                    classification = (srcBone >= 0 && srcBone < 128) ? boneClassifications[srcBone] : 0;
                    currentPriority = priorityMap[classification];
                }
                duplicateParts += 2;
            } else if (currentPriority == 1) {
                int parentBone = -1;

                if (localBoneIndex < modelParts->numRootBones) {
                    if (obj->modelParents[modelIndex] != 0xff) {
                        parentBone = obj->modelParents[modelIndex];
                    }
                } else if (hierarchy) {
                    int parentDelta = hierarchy->parentList[localBoneIndex - modelParts->numRootBones];
                    if (parentDelta <= 0) {
                        parentDelta = 1;
                    }
                    parentBone = globalBoneIndex - parentDelta;
                }

                classification = (parentBone >= 0 && parentBone < 128) ? boneClassifications[parentBone] : 0;
                currentPriority = priorityMap[classification];
            }

            if (globalBoneIndex < 128) {
                boneClassifications[globalBoneIndex] = classification;
            }

            if (ignoreCollision || currentPriority < lowestPriority || !model->boneInfo) {
                continue;
            }

            boneInfo = &model->boneInfo[localBoneIndex];
            if (boneInfo->radiusSquared == 0.0f) {
                continue;
            }

            DObjTraceTransformPointLocal(boneMatrix, boneInfo->offset, center);
            startToCenter[0] = start[0] - center[0];
            startToCenter[1] = start[1] - center[1];
            startToCenter[2] = start[2] - center[2];
            closestT = -(delta[0] * startToCenter[0] + delta[1] * startToCenter[1] + delta[2] * startToCenter[2]) * invLenSq;

            if (closestT >= 1.0f) {
                startToCenter[0] = end[0] - center[0];
                startToCenter[1] = end[1] - center[1];
                startToCenter[2] = end[2] - center[2];
            } else if (closestT > 0.0f) {
                startToCenter[0] += delta[0] * closestT;
                startToCenter[1] += delta[1] * closestT;
                startToCenter[2] += delta[2] * closestT;
            }

            closestDistSq = startToCenter[0] * startToCenter[0] + startToCenter[1] * startToCenter[1] + startToCenter[2] * startToCenter[2];
            radiusDelta = boneInfo->radiusSquared - closestDistSq;
            if (radiusDelta <= 0.0f) {
                continue;
            }

            if (currentPriority == lowestPriority) {
                float sphereFrac = closestT - sqrtf(radiusDelta * invLenSq);
                if (sphereFrac >= trace->fraction) {
                    continue;
                }
                traceLimit = trace->fraction;
            } else {
                traceLimit = solidHitFrac;
            }

            DObjTraceInversePointLocal(boneMatrix, start, startLocal);
            DObjTraceInversePointLocal(boneMatrix, end, endLocal);
            if (!DObjTraceLineAabbLocal(startLocal, endLocal, boneInfo->bounds[0], boneInfo->bounds[1],
                                        traceLimit, &frac, &axis, &sign, &startSolid, &endSolid)) {
                continue;
            }

            if (startSolid) {
                if (!endSolid) {
                    continue;
                }
                if (delta[0] * start[0] + delta[1] * start[1] > 0.0f) {
                    continue;
                }
                trace->fraction = 0.0f;
                trace->partName = names ? names[localBoneIndex] : 0;
                trace->partGroup = classification;
                DObjTraceSetStartSolidNormalLocal(start, delta, trace);
                return;
            }

            if (currentPriority == lowestPriority) {
                if (frac >= trace->fraction) {
                    lowestPriority = currentPriority;
                    continue;
                }
            }

            trace->fraction = frac;
            trace->partName = names ? names[localBoneIndex] : 0;
            trace->partGroup = classification;
            lowestPriority = currentPriority;
            hitAxis = axis;
            hitSign = sign;
            hitBoneMatrix = boneMatrix;
        }
    }

    if (hitBoneMatrix) {
        float axis[3][3];
        DObjTraceBuildAxesLocal(hitBoneMatrix, axis);
        trace->normal[0] = hitSign * axis[hitAxis][0];
        trace->normal[1] = hitSign * axis[hitAxis][1];
        trace->normal[2] = hitSign * axis[hitAxis][2];
    }
}
