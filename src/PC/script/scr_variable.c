#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

extern scr_classStruct_t g_classMap[5];

extern void Com_Error(errorParm_t code, const char *fmt, ...);

const char *var_typename[24] = { (const char *)&str_0021d23c, (const char *)&str_0021d248, (const char *)&str_0021d250, (const char *)&str_0021d258, (const char *)&str_0021d26c, (const char *)&str_0021d274, (const char *)&str_0021d27c, (const char *)&str_0021d280, (const char *)&str_0021d288, (const char *)&str_0021d294, (const char *)&str_0021d2a0, (const char *)&str_0021d2a8, (const char *)&str_0021d2b4, (const char *)&str_0021d2c8, (const char *)&str_0021d2d8, (const char *)&str_0021d2e4, (const char *)&str_0021d2e4, (const char *)&str_0021d2e4, (const char *)&str_0021d2e4, (const char *)&str_0021d2ec, (const char *)&str_0021d2f4, (const char *)&str_0021d304, (const char *)&str_0021d30c, (const char *)&str_0021d314 };

extern const char *var_typename[24];
extern struct scrVarPub_t scrVarPub;
extern unsigned char scrVarGlob[];

#define SCRVL_SL_MAX_STRING_INDEX 0x10000
#define SCRVL_MAX_VARIABLES 0xfffd
#define SCRVL_VAR_GLOB_SIZE 1048608
#define SCRVL_VAR_HASH 0x40
#define SCRVL_VAR_COLLISION 0x20
#define SCRVL_VAR_ALLOCATED 0x60
#define SCRVL_VAR_MASK 0x1f
#define SCRVL_VAR_UNDEFINED 0
#define SCRVL_VAR_POINTER 1
#define SCRVL_VAR_STRING 2
#define SCRVL_VAR_LOCALIZED_STRING 3
#define SCRVL_VAR_VECTOR 4
#define SCRVL_VAR_FLOAT 5
#define SCRVL_VAR_INTEGER 6
#define SCRVL_VAR_OBJECT 0x13
#define SCRVL_VAR_ENTITY 0x15
#define SCRVL_VAR_ARRAY 0x16

#define VG_ID(id) (*(unsigned short *)((byte *)scrVarGlob + (id) * 16))
#define VG_PREV(id) (*(unsigned short *)((byte *)scrVarGlob + (id) * 16 + 2))
#define VG_U16(id) (*(unsigned short *)((byte *)scrVarGlob + (id) * 16 + 4))
#define VG_U32(id) (*(unsigned int *)((byte *)scrVarGlob + (id) * 16 + 4))
#define VG_STATUS(id) (*(unsigned int *)((byte *)scrVarGlob + (id) * 16 + 8))
#define VG_NEXT(id) (*(unsigned short *)((byte *)scrVarGlob + (id) * 16 + 12))
#define VG_NEXT_SIBLING(id) (*(unsigned short *)((byte *)scrVarGlob + (id) * 16 + 14))
#define VG_SIBLING(id) (*(unsigned short *)((byte *)scrVarGlob + (id) * 16 + 6))
#define VG_NAME(id) (VG_STATUS(id) >> 8)

COD2_ASSERT_FIELD(VariableValueInternal, hash.id,      0x0);
COD2_ASSERT_FIELD(VariableValueInternal, hash.u.prev,  0x2);
COD2_ASSERT_FIELD(VariableValueInternal, u.u,          0x4);
COD2_ASSERT_FIELD(VariableValueInternal, u.o.refCount, 0x4);
COD2_ASSERT_FIELD(VariableValueInternal, u.o.u.entnum, 0x6);
COD2_ASSERT_FIELD(VariableValueInternal, u.o.u.self,   0x6);
COD2_ASSERT_FIELD(VariableValueInternal, w.status,     0x8);
COD2_ASSERT_FIELD(VariableValueInternal, v.index,      0xc);
COD2_ASSERT_FIELD(VariableValueInternal, nextSibling,  0xe);
COD2_ASSERT_SIZE (VariableValueInternal, 0x10);

int dbg_alloc_counter = 0;
static unsigned int s_varNetDbg = 0;
static const char str_dbg_alloc_fmt[] = "DBG AllocValue exhausted after %d allocations\n";
static const char str_dbg_site_classmap1[] = "DBG exceeded at: Scr_SetClassMap site1\n";
static const char str_dbg_site_classmap2[] = "DBG exceeded at: Scr_SetClassMap site2\n";
static const char str_dbg_site_gamvar1[] = "DBG exceeded at: Scr_AllocGameVariable site1\n";
static const char str_dbg_site_gamvar2[] = "DBG exceeded at: Scr_AllocGameVariable site2\n";
static const char str_dbg_getvar_fmt[] = "DBG GetVariable #%d: parentId=%d name=%d freeHead=%d\n";
int dbg_getvar_counter = 0;

extern void *MT_Alloc(int size, int type);
extern void MT_Free(void *ptr, int type);
extern char *va(const char *format, ...);
extern void Scr_Error(const char *msg);
extern void Scr_TerminalError(const char *msg);
extern void Com_Printf(const char *fmt, ...);
extern XAnim *Scr_GetAnims(int index);
extern const char *XAnimGetAnimDebugName(const XAnim *anims, unsigned int animIndex);
extern void SL_AddRefToString(unsigned int stringValue);
extern void SL_RemoveRefToString(unsigned int stringValue);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern int SL_GetStringLen(unsigned int stringValue);
extern unsigned int SL_GetStringOfLen(const char *str, unsigned int user, unsigned int len, int type);
extern unsigned int SL_GetStringForFloat(float f);
extern unsigned int SL_GetStringForInt(int i);
extern unsigned int SL_GetStringForVector(const float *v);
extern unsigned int SL_GetString_(const char *str, unsigned int user, int type);
extern unsigned int SL_GetCanonicalString(const char *str);
extern unsigned int SL_ConvertFromString(const char *str);
extern const char **FS_ListFiles(const char *path, const char *extension, int behavior, int *numfiles, int allocTrackType);
extern void FS_FreeFileList(const char **list, int allocTrackType);
extern void TempMemoryReset(void);
extern char *TempMalloc(int len);
extern void TempMemorySetPos(char *pos);
extern void *Hunk_AllocLowInternal(int size);
extern void Hunk_ConvertTempToPermLowInternal(void);
extern void *Hunk_AllocateTempMemoryHighInternal(int size);
extern void Hunk_ClearTempMemoryHigh(void);
extern int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, fsMode_t mode);
extern int FS_Read(void *buffer, int len, fileHandle_t h);
extern void FS_FCloseFile(fileHandle_t h);
extern void Com_BeginParseSession(const char *name);
extern const char *Com_Parse(const char **data_p);
extern void Com_EndParseSession(void);
extern int ___tolower(int c);
extern int stricmp(const char *s1, const char *s2);
extern Bool SetEntityFieldValue(unsigned int classnum, int entnum, int offset, VariableValue *value);
extern unsigned long long GetEntityFieldValue(unsigned int classnum, int entnum, int offset) __attribute_regparm__(0);
extern void Scr_CancelNotifyList(unsigned int notifyListOwnerId);
extern void VM_CancelNotify(unsigned int notifyListOwnerId, unsigned int startLocalId);

int GetVarType(unsigned int id);
static inline __attribute__((always_inline)) void Var_ResetAll(void);
static inline __attribute__((always_inline)) unsigned int AllocVariable(void);
void FreeVariable(unsigned int id);
static inline __attribute__((always_inline)) unsigned int FindVariableIndexInternal(unsigned int parentId, unsigned int name);
static unsigned int GetVariableIndexInternal(unsigned int parentId, unsigned int name);
static unsigned int GetNewVariableIndexInternal(unsigned int parentId, unsigned int name);
static int ThreadInfoCompare(const void *info1, const void *info2);
static inline __attribute__((always_inline)) VariableValueInternal *ScrVarEntry(unsigned int id);
static inline __attribute__((always_inline)) unsigned int ScrVarEntryIndex(const VariableValueInternal *entry);
unsigned int FindNextSibling(unsigned int id);
void Scr_DumpScriptVariables(void);
void Var_Init(void);
unsigned int Scr_GetNumScriptVars(void);
unsigned int GetVariableKeyObject(unsigned int id);
void AddRefToObject(unsigned int id);
void Scr_SetThreadNotifyName(unsigned int startLocalId, unsigned int stringValue);
short unsigned int Scr_GetThreadNotifyName(unsigned int startLocalId);
void Scr_SetThreadWaitTime(unsigned int startLocalId, unsigned int waitTime);
void Scr_ClearWaitTime(unsigned int startLocalId);
unsigned int Scr_GetThreadWaitTime(unsigned int startLocalId);
unsigned int GetParentLocalId(unsigned int threadId);
unsigned int GetSafeParentLocalId(unsigned int threadId);
unsigned int GetStartLocalId(unsigned int threadId);
unsigned int FindObject(unsigned int id);
VariableUnion *GetVariableValueAddress(unsigned int id);
void RemoveRefToEmptyObject(unsigned int id);
unsigned int Scr_GetSelf(unsigned int threadId);
void RemoveRefToVector(const float *vectorValue);
Bool IsValidArrayIndex(unsigned int unsignedValue);
unsigned int GetInternalVariableIndex(unsigned int unsignedValue);
void SetNewVariableValue(unsigned int id, VariableValue *value);
unsigned int Scr_EvalVariableObject(unsigned int id);
unsigned int GetArraySize(unsigned int id);
unsigned int FindPrevSibling(unsigned int id);
unsigned int GetVariableName(unsigned int id);
Bool IsFieldObject(unsigned int id);
Bool IsVarFree(unsigned int id);
unsigned int Scr_GetEntityIdRef(unsigned int entId);
unsigned int Scr_FindField(const char *name, int *type);
int Scr_GetClassnumForCharId(int charId);
void Scr_RemoveThreadNotifyName(unsigned int startLocalId);
void AddRefToValue(int type, VariableUnion u);
const float *Scr_AllocVector(const float *v);
int Scr_GetOffset(int classnum, const char *name);
unsigned int FindEntityId(int entnum, int classnum);
unsigned int FindArrayVariable(unsigned int parentId, int intValue);
unsigned int FindVariable(unsigned int parentId, unsigned int unsignedValue);
unsigned int FindObjectVariable(unsigned int parentId, unsigned int id);
void Scr_AddFields(const char *path, const char *extension);
unsigned long long Scr_EvalVariable(unsigned int id);
static unsigned int GetNewVariableIndexInternal3(unsigned int parentId, unsigned int name, unsigned int index) __attribute_regparm__(3);
unsigned int GetNewObjectVariableReverse(unsigned int parentId, unsigned int id);
unsigned int GetNewObjectVariable(unsigned int parentId, unsigned int id);
void SetVariableEntityFieldValue(unsigned int entId, unsigned int fieldName, VariableValue *value);
void Scr_AddClassField(int classnum, const char *name, unsigned int offset);
unsigned int GetNewArrayVariable(unsigned int parentId, unsigned int unsignedValue);
unsigned int GetNewVariable(unsigned int parentId, unsigned int unsignedValue);
static float Scr_GetObjectUsage(unsigned int parentId) __attribute_regparm__(1);
void Scr_DumpScriptThreads(void);
unsigned int Scr_GetEntityId(int entnum, int classnum);
unsigned int Scr_EvalArrayIndex(unsigned int parentId, VariableValue *index);
unsigned int Scr_GetVariableField(unsigned int parentId, unsigned int name);
unsigned int GetArrayVariable(unsigned int parentId, unsigned int unsignedValue);
unsigned int GetVariable(unsigned int parentId, unsigned int unsignedValue);
unsigned int GetObjectVariable(unsigned int parentId, unsigned int id);
static void CopyArray(unsigned int parentId, unsigned int newParentId) __attribute_regparm__(2);
void Scr_SetClassMap(int classnum);
unsigned int GetArray(unsigned int id);
unsigned int GetObjectA(unsigned int id);
unsigned int AllocValue(void);
unsigned int AllocObject(void);
static inline __attribute__((always_inline)) unsigned int AllocObject_core(void);
unsigned int Scr_AllocArray(void);
static inline __attribute__((always_inline)) unsigned int Scr_AllocArray_core(void);
unsigned int AllocThread(unsigned int self);
unsigned int AllocChildThread(unsigned int self, unsigned int parentLocalId);
static void MakeVariableExternal(VariableValueInternal *entry, VariableValueInternal *parentValue) __attribute_regparm__(2);
#ifdef __EMSCRIPTEN__
void RemoveRefToObject(unsigned int id);
void FreeValue(unsigned int id);
void Var_Shutdown(void);
#else
void RemoveRefToObject(unsigned int id);
void FreeValue(unsigned int id);
void Var_Shutdown(void);
#endif
void Scr_FreeValue(unsigned int id);
void Scr_EvalArray(VariableValue *value, VariableValue *index);
void Scr_RemoveClassMap(int classnum);
static unsigned long long Scr_EvalVariableEntityField(unsigned int entId, unsigned int name) __attribute_regparm__(2);
unsigned long long Scr_EvalVariableField(unsigned int id);
unsigned long long Scr_FindVariableField(unsigned int parentId, unsigned int name);
void RemoveRefToValue(int type, VariableUnion u);
static inline __attribute__((always_inline)) void RemoveRefToVector_core(const float *vectorValue);
static inline __attribute__((always_inline)) void RemoveRefToValue_core(int type, VariableUnion u);
static inline __attribute__((always_inline)) void FreeChildValue_core(unsigned int id);
static inline __attribute__((always_inline)) void SetVariableValue_core(unsigned int id, VariableValue *value);
void Scr_KillEndonThread(unsigned int threadId);
void Scr_FreeEntityList(void);
void SafeRemoveVariable(unsigned int parentId, unsigned int unsignedValue);
void RemoveNextVariable(unsigned int parentId);
void RemoveVariable(unsigned int parentId, unsigned int unsignedValue);
static int Scr_MakeValuePrimitive(unsigned int parentId) __attribute_regparm__(1);
void Scr_FreeGameVariable(int bComplete);
void Scr_FreeEntityNum(int entnum, int classnum);
void RemoveObjectVariable(unsigned int parentId, unsigned int id);
void ClearVariableField(unsigned int parentId, unsigned int name, VariableValue *value);
void Scr_FreeObjects(void);
void Scr_EvalBoolNot(VariableValue *value);
void SetVariableFieldValue(unsigned int id, VariableValue *value);
void Scr_AllocGameVariable(void);
unsigned int Scr_EvalArrayRef(unsigned int parentId);
void Scr_StopThread(unsigned int threadId);
void ClearObject(unsigned int parentId);
void Scr_KillThread(unsigned int parentId);
unsigned int Scr_EvalFieldObject(unsigned int tempVariable, VariableValue *value);
Bool Scr_CastString(VariableValue *value);
static inline __attribute__((always_inline)) void Scr_CastBool_core(VariableValue *value);
void Scr_CastDebugString(VariableValue *value);
void Scr_UnmatchingTypesError(VariableValue *value1, VariableValue *value2);
void Scr_EvalMod(VariableValue *value1, VariableValue *value2);
void Scr_EvalDivide(VariableValue *value1, VariableValue *value2);
void Scr_EvalPlus(VariableValue *value1, VariableValue *value2);
void Scr_EvalShiftRight(VariableValue *value1, VariableValue *value2);
void Scr_EvalShiftLeft(VariableValue *value1, VariableValue *value2);
void Scr_EvalAnd(VariableValue *value1, VariableValue *value2);
void Scr_EvalExOr(VariableValue *value1, VariableValue *value2);
void Scr_EvalOr(VariableValue *value1, VariableValue *value2);
void Scr_EvalLessEqual(VariableValue *value1, VariableValue *value2);
void Scr_EvalGreaterEqual(VariableValue *value1, VariableValue *value2);
void Scr_EvalEquality(VariableValue *value1, VariableValue *value2);
void Scr_EvalInequality(VariableValue *value1, VariableValue *value2);
void Scr_EvalLess(VariableValue *value1, VariableValue *value2);
void Scr_EvalGreater(VariableValue *value1, VariableValue *value2);
void Scr_EvalMinus(VariableValue *value1, VariableValue *value2);
void Scr_EvalBinaryOperator(int op, VariableValue *value1, VariableValue *value2);
void Scr_EvalMultiply(VariableValue *value1, VariableValue *value2);
void Scr_CastBool(VariableValue *value);
void Scr_EvalBoolComplement(VariableValue *value);
void Scr_EvalSizeValue(VariableValue *value);
void Scr_CastVector(VariableValue *value);
void SetVariableValue(unsigned int id, VariableValue *value);
void ClearVariableValue(unsigned int id);
void FreeChildValue(unsigned int id);
static inline __attribute__((always_inline)) void Scr_MakeObjectChildrenExternal(unsigned int parentId);
static inline __attribute__((always_inline)) void Scr_FreeObjectChildren(unsigned int parentId);
void ClearObjectInternal(unsigned int parentId);
void ClearArray(unsigned int parentId, VariableValue *value);

int GetVarType(unsigned int id)
{
    return VG_STATUS(id) & 0x1f;
}

static inline __attribute__((always_inline)) unsigned int Scr_VariableHash(unsigned int parentId, unsigned int name)
{
    return ((parentId + name) % SCRVL_MAX_VARIABLES) + 1;
}

static inline __attribute__((always_inline)) void Var_ResetAll(void)
{
    unsigned int id;
    unsigned int prev;

    prev = 0;
    for (id = 1; id <= SCRVL_MAX_VARIABLES; id++) {
        VG_STATUS(id) = 0;
        VG_ID(id) = (unsigned short)id;
        VG_NEXT(id) = (unsigned short)id;
        VG_U16(prev) = (unsigned short)id;
        VG_PREV(id) = (unsigned short)prev;
        VG_NEXT_SIBLING(id) = 0;
        prev = id;
    }
    VG_STATUS(0) = 0;
    VG_ID(0) = 0;
    VG_NEXT(0) = 0;
    VG_U16(prev) = 0;
    VG_PREV(0) = (unsigned short)prev;
}

static inline __attribute__((always_inline)) unsigned int AllocVariable(void)
{
    unsigned int index = VG_U16(0);
    unsigned int id;
    unsigned int next;

    if (!index) {
        Scr_TerminalError("exceeded maximum number of script variables");
        return 0;
    }

    id = VG_ID(index);
    next = VG_U16(id);

    {
        unsigned int linkId = id;

        if (index != id && !(VG_STATUS(index) & SCRVL_VAR_ALLOCATED)) {
            VG_ID(VG_NEXT(index)) = (unsigned short)id;
            VG_ID(index) = (unsigned short)index;
            VG_NEXT(id) = VG_NEXT(index);
            VG_U16(id) = VG_U16(index);
            linkId = index;
        }

        VG_U16(0) = (unsigned short)next;
        VG_PREV(next) = 0;
        VG_NEXT(linkId) = (unsigned short)index;
        VG_NEXT_SIBLING(linkId) = (unsigned short)index;
        VG_PREV(index) = (unsigned short)index;

        id = VG_ID(index);
        s_varNetDbg++;
        if ((s_varNetDbg & 0x7ff) == 0)
            fprintf(stderr, "[varleak] net=%u\n", s_varNetDbg);
        return id;
    }
}

void FreeVariable(unsigned int id)
{
    unsigned int index;
    unsigned int nextSiblingIndex;
    unsigned int previousSiblingId;
    unsigned int freeHead;

    if (!id || id > SCRVL_MAX_VARIABLES)
        return;

    index = VG_NEXT(id);
    nextSiblingIndex = VG_NEXT_SIBLING(id);

    VG_PREV(nextSiblingIndex) = VG_PREV(index);
    previousSiblingId = VG_ID(VG_PREV(nextSiblingIndex));
    VG_NEXT_SIBLING(previousSiblingId) = (unsigned short)nextSiblingIndex;

    ((byte *)scrVarGlob)[id * 16 + 8] = 0;
    freeHead = VG_U16(0);
    VG_U16(id) = (unsigned short)freeHead;
    VG_PREV(index) = 0;
    VG_PREV(freeHead) = (unsigned short)index;
    VG_U16(0) = (unsigned short)index;
    s_varNetDbg--;
}

static inline __attribute__((always_inline)) unsigned int FindVariableIndexInternal(unsigned int parentId, unsigned int name)
{
    unsigned int index = Scr_VariableHash(parentId, name);
    unsigned int start = index;
    unsigned int valueId = VG_ID(index);

    if ((VG_STATUS(valueId) & 0x60) != SCRVL_VAR_HASH)
        return 0;

    for (;;) {
        if (VG_NAME(valueId) == name)
            return index;

        index = VG_NEXT(valueId);
        if (index == start)
            return 0;

        valueId = VG_ID(index);
    }
}

static unsigned int GetNewVariableIndexInternal(unsigned int parentId, unsigned int name)
{
    unsigned int index = Scr_VariableHash(parentId, name);
    return GetNewVariableIndexInternal3(parentId, name, index);
}

static unsigned int GetVariableIndexInternal(unsigned int parentId, unsigned int name)
{
    unsigned int index = FindVariableIndexInternal(parentId, name);

    if (index)
        return index;
    return GetNewVariableIndexInternal(parentId, name);
}

static int ThreadInfoCompare(const void *info1, const void *info2)
{
    const JCOEF *threadInfo1 = (const JCOEF *)info1;
    const JCOEF *threadInfo2 = (const JCOEF *)info2;
    int count1 = *(int *)((byte *)threadInfo1 + 0x80);
    int count2 = *(int *)((byte *)threadInfo2 + 0x80);
    int i;

    if (count1 <= 0 || count2 <= 0)
        return count1 - count2;

    for (i = 0; i < count1 && i < count2; i++) {
        if (threadInfo1[i] != threadInfo2[i])
            return threadInfo1[i] - threadInfo2[i];
    }
    return count1 - count2;
}

unsigned int FindNextSibling(unsigned int id)
{
    unsigned int sibling = VG_ID(VG_NEXT_SIBLING(id));

    if (sibling == id || (VG_STATUS(sibling) & SCRVL_VAR_MASK) > 0xe)
        return 0;
    return sibling;
}

static unsigned int ScrVar_NextSiblingId(unsigned int id)
{
    unsigned int nextSiblingIndex;

    if (!id || id > SCRVL_MAX_VARIABLES)
        return 0;

    nextSiblingIndex = VG_NEXT_SIBLING(id);
    if (!nextSiblingIndex || nextSiblingIndex > SCRVL_MAX_VARIABLES)
        return 0;

    return VG_ID(nextSiblingIndex);
}

void Scr_DumpScriptVariables(void)
{
    extern void Scr_DumpScriptVarSummary(void);
    Scr_DumpScriptVarSummary();
}

void Scr_DumpScriptVarSummary(void)
{
    unsigned int freeChainCount = 0;
    unsigned int freeStatusCount = 0;
    unsigned int statusCounts[4] = { 0, 0, 0, 0 };
    unsigned int typeCounts[32];
    unsigned int freeHead = VG_U16(0);
    unsigned int id;
    unsigned int index;
    unsigned int guard;

    memset(typeCounts, 0, sizeof(typeCounts));

    for (id = 1; id <= SCRVL_MAX_VARIABLES; id++) {
        unsigned int status = VG_STATUS(id);
        unsigned int statusKind = (status & 0x60) >> 5;

        statusCounts[statusKind]++;

        if ((status & 0x60) == 0) {
            freeStatusCount++;
        } else {
            typeCounts[status & SCRVL_VAR_MASK]++;
        }
    }

    index = freeHead;
    guard = 0;
    while (index && index <= SCRVL_MAX_VARIABLES && guard <= SCRVL_MAX_VARIABLES) {
        unsigned int valueId = VG_ID(index);

        freeChainCount++;
        guard++;

        if (!valueId || valueId > SCRVL_MAX_VARIABLES)
            break;

        index = VG_U16(valueId);
    }

    Com_Printf("script vars: freeHead=%u freeChain=%u freeStatus=%u status00=%u status20=%u status40=%u status60=%u\n",
               freeHead, freeChainCount, freeStatusCount,
               statusCounts[0], statusCounts[1], statusCounts[2], statusCounts[3]);
    Com_Printf("script vars types: undef=%u objref=%u str=%u locstr=%u vec=%u float=%u int=%u codepos=%u precodepos=%u function=%u stack=%u anim=%u thread=%u notify=%u time=%u child=%u obj=%u deadent=%u ent=%u array=%u deadthread=%u\n",
               typeCounts[0], typeCounts[1], typeCounts[2], typeCounts[3],
               typeCounts[4], typeCounts[5], typeCounts[6], typeCounts[7],
               typeCounts[8], typeCounts[9], typeCounts[10], typeCounts[11],
               typeCounts[15], typeCounts[16], typeCounts[17], typeCounts[18],
               typeCounts[19], typeCounts[20], typeCounts[21], typeCounts[22],
               typeCounts[23]);

    if (index > SCRVL_MAX_VARIABLES || guard > SCRVL_MAX_VARIABLES) {
        Com_Printf("script vars: free chain stopped at index=%u guard=%u\n", index, guard);
    }
}

void Var_Init(void)
{
    int i;
    Var_ResetAll();
    for (i = 0; i <= 3; i++) {
        g_classMap[i].entArrayId = 0;
        g_classMap[i].id = 0;
    }
}

unsigned int Scr_GetNumScriptVars(void)
{
    return 0;
}

unsigned int GetVariableKeyObject(unsigned int id)
{
    return (VG_STATUS(id) >> 8) - 0x10000;
}

void AddRefToObject(unsigned int id)
{
    VG_U16(id) += 1;
}

void Scr_SetThreadNotifyName(unsigned int startLocalId, unsigned int stringValue)
{
    VariableValueInternal *entry = (VariableValueInternal *)((byte *)scrVarGlob + startLocalId * 16);
    unsigned int val = entry->w.status;
    val = (val & 0xe0) | 0x10;
    entry->w.status = val | (stringValue << 8);
}

short unsigned int Scr_GetThreadNotifyName(unsigned int startLocalId)
{
    return *(unsigned short *)((byte *)scrVarGlob + startLocalId * 16 + 9);
}

void Scr_SetThreadWaitTime(unsigned int startLocalId, unsigned int waitTime)
{
    VariableValueInternal *entry = (VariableValueInternal *)((byte *)&scrVarGlob + startLocalId * 16);
    entry->w.status = (entry->w.status & 0xe0) | 0x11;
    VG_STATUS(startLocalId) |= (waitTime << 8);
}

void Scr_ClearWaitTime(unsigned int startLocalId)
{
    VariableValueInternal *entry = (VariableValueInternal *)((byte *)&scrVarGlob + startLocalId * 16);
    entry->w.status = (entry->w.status & 0xffffffe0) | 0xf;
}

unsigned int Scr_GetThreadWaitTime(unsigned int startLocalId)
{
    return VG_STATUS(startLocalId) >> 8;
}

unsigned int GetParentLocalId(unsigned int threadId)
{
    return VG_STATUS(threadId) >> 8;
}

unsigned int GetSafeParentLocalId(unsigned int threadId)
{
    unsigned int val = VG_STATUS(threadId);
    if ((val & 0x1f) == 0x12)
        return val >> 8;
    return 0;
}

unsigned int GetStartLocalId(unsigned int threadId)
{
    while ((VG_STATUS(threadId) & 0x1f) == 0x12) {
        threadId = VG_STATUS(threadId) >> 8;
    }
    return threadId;
}

unsigned int FindObject(unsigned int id)
{
    return VG_U32(id);
}

VariableUnion *GetVariableValueAddress(unsigned int id)
{
    return &((VariableValueInternal *)((byte *)scrVarGlob + id * 16))->u.u;
}

void RemoveRefToEmptyObject(unsigned int id)
{
    unsigned short cnt = VG_U16(id);
    if (!cnt) {
        FreeVariable(id);
    } else {
        VG_U16(id) = (unsigned short)(cnt - 1);
    }
}

unsigned int Scr_GetSelf(unsigned int threadId)
{
    return VG_SIBLING(threadId);
}

static inline __attribute__((always_inline)) void RemoveRefToVector_core(const float *vectorValue)
{
    unsigned short *refCount;

    if (*((unsigned char *)vectorValue - 1))
        return;

    refCount = (unsigned short *)((byte *)vectorValue - 4);
    if (*refCount == 0) {
        MT_Free(refCount, 0x10);
    } else {
        *refCount -= 1;
    }
}

void RemoveRefToVector(const float *vectorValue)
{
    RemoveRefToVector_core(vectorValue);
}

static inline __attribute__((always_inline)) void RemoveRefToValue_core(int type, VariableUnion u)
{
    switch (type) {
    case SCRVL_VAR_POINTER:
        RemoveRefToObject(u.pointerValue);
        break;
    case SCRVL_VAR_STRING:
    case SCRVL_VAR_LOCALIZED_STRING:
        SL_RemoveRefToString(u.stringValue);
        break;
    case SCRVL_VAR_VECTOR:
        RemoveRefToVector_core(SCR_VEC_PTR(u));
        break;
    default:
        break;
    }
}

static inline __attribute__((always_inline)) void FreeChildValue_core(unsigned int id)
{
    VariableUnion value;
    unsigned int type;
    unsigned int index;
    unsigned int prevSiblingIndex;
    unsigned int nextSiblingIndex;
    unsigned int freeHead;

    type = VG_STATUS(id) & SCRVL_VAR_MASK;
    value.intValue = VG_U32(id);
    RemoveRefToValue_core(type, value);

    index = VG_NEXT(id);
    prevSiblingIndex = VG_PREV(index);
    nextSiblingIndex = VG_NEXT_SIBLING(id);

    VG_PREV(nextSiblingIndex) = (unsigned short)prevSiblingIndex;
    VG_NEXT_SIBLING(VG_ID(prevSiblingIndex)) = (unsigned short)nextSiblingIndex;

    VG_STATUS(id) = 0;
    freeHead = VG_U16(0);
    VG_U16(id) = (unsigned short)freeHead;
    VG_PREV(index) = 0;
    VG_PREV(freeHead) = (unsigned short)index;
    VG_U16(0) = (unsigned short)index;
}

static inline __attribute__((always_inline)) void SetVariableValue_core(unsigned int id, VariableValue *value)
{
    unsigned int status = VG_STATUS(id);
    VariableUnion oldValue;

    oldValue.intValue = VG_U32(id);
    RemoveRefToValue_core(status & SCRVL_VAR_MASK, oldValue);

    VG_STATUS(id) = (VG_STATUS(id) & ~SCRVL_VAR_MASK) | value->type;
    VG_U32(id) = value->u.intValue;
}

Bool IsValidArrayIndex(unsigned int unsignedValue)
{
    return (unsignedValue + 0x7e0002) <= 0xfe0001;
}

static inline __attribute__((always_inline)) unsigned int GetInternalVariableIndex_core(unsigned int unsignedValue)
{
    return (unsignedValue + 0x800000) & 0xffffff;
}

unsigned int GetInternalVariableIndex(unsigned int unsignedValue)
{
    return GetInternalVariableIndex_core(unsignedValue);
}

void SetNewVariableValue(unsigned int id, VariableValue *value)
{
    VariableValueInternal *entry = (VariableValueInternal *)((byte *)&scrVarGlob + id * 16);
    entry->w.status |= value->type;
    entry->u.u.intValue = value->u.intValue;
}

unsigned int Scr_EvalVariableObject(unsigned int id)
{
    unsigned int type = VG_STATUS(id) & 0x1f;
    unsigned int objectId;

    if (type == 1) {
        objectId = VG_U32(id);
        type = VG_STATUS(objectId) & 0x1f;
        if (type <= 0x15)
            return objectId;
    }

    Com_Printf("^3%s is not a field object\n", var_typename[type]);
    return 0;
}

unsigned int GetArraySize(unsigned int id)
{
    return VG_SIBLING(id);
}

unsigned int FindPrevSibling(unsigned int id)
{
    unsigned int next, result;

    next = VG_NEXT_SIBLING(id);
    next = VG_PREV(next);
    next = VG_PREV(next);
    result = VG_ID(next);

    if ((VG_STATUS(result) & 0x1f) >= 0xf)
        return 0;
    return result;
}

unsigned int GetVariableName(unsigned int id)
{
    return VG_STATUS(id) >> 8;
}

Bool IsFieldObject(unsigned int id)
{
    return (VG_STATUS(id) & 0x1f) <= 0x15;
}

Bool IsVarFree(unsigned int id)
{
    return (((VariableValueInternal *)((byte *)scrVarGlob + id * 16))->w.status & 0x60) == 0;
}

unsigned int Scr_GetEntityIdRef(unsigned int entId)
{
    VariableValueInternal *entry = (VariableValueInternal *)((byte *)scrVarGlob + entId * 16);
    unsigned int entnum = entry->u.o.u.entnum;
    unsigned int classnum = *(unsigned short *)((byte *)entry + 9);

    return entnum | (classnum << 16);
}

unsigned int Scr_FindField(const char *name, int *type)
{
    const char *pos;
    int len;
    unsigned int index;

    for (pos = scrVarPub.fieldBuffer; *pos; pos += len + 3) {
        len = strlen(pos) + 1;

        if (!stricmp(name, pos)) {
            pos = &pos[len];
            index = *(const unsigned short *)pos;
            *type = (signed char)pos[2];
            return index;
        }
    }

    return 0;
}

int Scr_GetClassnumForCharId(int charId)
{
    int i;
    for (i = 0; i < 4; i++) {
        if (g_classMap[i].charId == (char)charId)
            return i;
    }
    return -1;
}

void Scr_RemoveThreadNotifyName(unsigned int startLocalId)
{
    byte *entry = (byte *)scrVarGlob + startLocalId * 16;
    unsigned int notifyName;
    notifyName = *(unsigned short *)((byte *)scrVarGlob + startLocalId * 16 + 9);

    SL_RemoveRefToString(notifyName);

    *(unsigned int *)(entry + 8) = (*(unsigned int *)(entry + 8) & 0xffffffe0) | 0xf;
}

static inline __attribute__((always_inline)) void AddRefToValue_core(int type, VariableUnion u)
{
    switch (type) {
    case 1:
        VG_U16(u.intValue) += 1;
        break;
    case 2:
    case 3:
        SL_AddRefToString(u.intValue);
        break;
    case 4: {
        /* vectors are tagged offsets on x64 (SCR_VEC_TAG_PROG|...) -> decode, don't deref raw */
        const byte *vp = (const byte *)SCR_VEC_PTR(u);
        if (vp[-1] == 0)
            *(unsigned short *)(vp - 4) += 1;
        break;
    }
    }
}

void AddRefToValue(int type, VariableUnion u)
{
    AddRefToValue_core(type, u);
}

static inline __attribute__((always_inline)) const float *Scr_AllocVector_core(const float *v)
{
    byte *mem = (byte *)MT_Alloc(0x10, 2);
    float *vec = (float *)(mem + 4);

    *(int *)mem = 0;
    vec[0] = v[0];
    vec[1] = v[1];
    vec[2] = v[2];

    return vec;
}

const float *Scr_AllocVector(const float *v)
{
    return Scr_AllocVector_core(v);
}

int Scr_GetOffset(int classnum, const char *name)
{
    unsigned int fieldName = SL_ConvertFromString(name);
    unsigned int bucket = FindVariableIndexInternal(g_classMap[classnum].id, fieldName);
    unsigned int valueId = VG_ID(bucket);

    if (!valueId)
        return -1;

    return (int)VG_U32(valueId);
}

unsigned int FindEntityId(int entnum, int classnum)
{
    unsigned int name = ((unsigned int)entnum + 0x800000) & 0xffffff;
    unsigned int bucket = FindVariableIndexInternal(g_classMap[classnum].entArrayId, name);
    unsigned int valueId = VG_ID(bucket);

    if (!valueId)
        return 0;

    return VG_U32(valueId);
}

unsigned int FindArrayVariable(unsigned int parentId, int intValue)
{
    unsigned int bucket = FindVariableIndexInternal(parentId,
                                                    ((unsigned int)intValue + 0x800000) & 0xffffff);
    return (unsigned int)VG_ID(bucket);
}

unsigned int FindVariable(unsigned int parentId, unsigned int index)
{
    unsigned int bucket = FindVariableIndexInternal(parentId, index);
    return (unsigned int)VG_ID(bucket);
}

unsigned int FindObjectVariable(unsigned int parentId, unsigned int id)
{
    unsigned int bucket = FindVariableIndexInternal(parentId,
                                                    id + SCRVL_SL_MAX_STRING_INDEX);
    return (unsigned int)VG_ID(bucket);
}

void Scr_AddFields(const char *path, const char *extension)
{
    const char **files;
    int numFiles;
    int i;

    numFiles = 0;
    files = FS_ListFiles(path, extension, 0, &numFiles, 10);

    if (((struct scrCompilePub_t *)imp_scrCompilePub)->script_loading) {
        Hunk_ConvertTempToPermLowInternal();
    }

    TempMemoryReset();
    scrVarPub.fieldBuffer = (const char *)Hunk_AllocLowInternal(0);
    *(char *)scrVarPub.fieldBuffer = '\0';

    for (i = 0; i < numFiles; i++) {
        char filename[64];
        fileHandle_t f;
        int len;
        char *fileBuffer;
        const char *sourcePos;
        const char *token;

        sprintf(filename, "%s/%s", path, files[i]);

        len = FS_FOpenFileByMode(filename, &f, FS_READ);
        if (len < 0) {
            Com_Error(1, va("\x15"
                            "cannot find '%s'",
                            filename));
        }

        fileBuffer = (char *)Hunk_AllocateTempMemoryHighInternal(len + 1);
        FS_Read(fileBuffer, len, f);
        fileBuffer[len] = '\0';
        FS_FCloseFile(f);

        sourcePos = fileBuffer;
        Com_BeginParseSession("Scr_AddFields");

        for (token = Com_Parse(&sourcePos); sourcePos; token = Com_Parse(&sourcePos)) {
            char type;
            char *fieldName;
            int nameLen;
            unsigned int index;
            const char *field;

            if (!strcmp(token, "float")) {
                type = 5;
            } else if (!strcmp(token, "int")) {
                type = 6;
            } else if (!strcmp(token, "string")) {
                type = 2;
            } else if (!strcmp(token, "vector")) {
                type = 4;
            } else {
                Com_Error(1, va("\x15unknown type '%s' in '%s'", token, filename));
                break;
            }

            fieldName = (char *)Com_Parse(&sourcePos);
            if (!sourcePos) {
                Com_Error(1, va("\x15missing field name in '%s'", filename));
            }

            nameLen = strlen(fieldName);
            while (nameLen > 0) {
                nameLen--;
                fieldName[nameLen] = (char)___tolower((signed char)fieldName[nameLen]);
            }
            nameLen = strlen(fieldName);
            index = SL_GetCanonicalString(fieldName);

            for (field = scrVarPub.fieldBuffer; *field; field += strlen(field) + 4) {
                int existingLen = strlen(field) + 1;

                if (!stricmp(fieldName, field)) {
                    if (*(const unsigned short *)(field + existingLen) != 0) {
                        Com_Error(1, "\x15"
                                     "duplicate key '%s' in '%s'",
                                  fieldName, filename);
                    }
                    break;
                }
            }

            {
                int lenWithNul = nameLen + 1;
                int entrySize = lenWithNul + 3;
                char *entry = TempMalloc(entrySize + 1);
                char *meta;

                strcpy(entry, fieldName);
                meta = entry + lenWithNul;
                *(unsigned short *)meta = (unsigned short)index;
                meta[2] = type;
                meta[3] = '\0';
                TempMemorySetPos(entry + entrySize);
            }
        }

        Com_EndParseSession();
        Hunk_ClearTempMemoryHigh();
    }

    if (files) {
        FS_FreeFileList(files, 10);
    }

    *(char *)TempMalloc(1) = '\0';
    Hunk_ConvertTempToPermLowInternal();
}

static inline __attribute__((always_inline)) unsigned long long Scr_EvalVariable_core(unsigned int id)
{
    VariableUnion u;
    unsigned int type;

    u.intValue = VG_U32(id);
    type = VG_STATUS(id) & SCRVL_VAR_MASK;
    AddRefToValue_core(type, u);

    return ((unsigned long long)type << 32) | (unsigned int)u.intValue;
}

unsigned long long Scr_EvalVariable(unsigned int id)
{
    return Scr_EvalVariable_core(id);
}

static inline __attribute__((always_inline)) unsigned int Scr_LinkVariableToParent(unsigned int parentId, unsigned int newIndex)
{
    unsigned int oldNext = VG_NEXT_SIBLING(parentId);
    unsigned int newId = VG_ID(newIndex);

    VG_NEXT_SIBLING(newId) = (unsigned short)oldNext;
    VG_PREV(oldNext) = (unsigned short)newIndex;
    VG_PREV(newIndex) = VG_NEXT(parentId);
    VG_NEXT_SIBLING(parentId) = (unsigned short)newIndex;

    return newId;
}

static inline __attribute__((always_inline)) VariableValueInternal *ScrVarEntry(unsigned int id)
{
    return (VariableValueInternal *)((byte *)scrVarGlob + id * 16);
}

static inline __attribute__((always_inline)) unsigned int ScrVarEntryIndex(const VariableValueInternal *entry)
{
    return (unsigned int)(((const byte *)entry - (const byte *)scrVarGlob) >> 4);
}

static void Scr_AddRefToArrayNameValue(unsigned int name)
{
    VariableUnion value;
    int type;

    if (name <= 0xffff) {
        value.stringValue = name;
        type = SCRVL_VAR_STRING;
    } else if (name <= 0x1fffd) {
        value.pointerValue = name - SCRVL_SL_MAX_STRING_INDEX;
        type = SCRVL_VAR_POINTER;
    } else {
        value.intValue = name - 0x800000;
        type = SCRVL_VAR_INTEGER;
    }

    AddRefToValue(type, value);
}

static void Scr_UnlinkFreeIndex(unsigned int freeIndex, unsigned int nextFreeIndex)
{
    unsigned int prevFreeIndex = VG_PREV(freeIndex);

    VG_U16(VG_ID(prevFreeIndex)) = (unsigned short)nextFreeIndex;
    VG_PREV(nextFreeIndex) = (unsigned short)prevFreeIndex;
}

static unsigned int Scr_PopFreeIndex(VariableValueInternal **freeEntry,
                                     VariableValueInternal **freeValue)
{
    unsigned int index = VG_U16(0);
    unsigned int nextFreeIndex;

    if (!index) {
        Scr_TerminalError("exceeded maximum number of script variables");
        return 0;
    }

    *freeEntry = ScrVarEntry(index);
    *freeValue = ScrVarEntry((*freeEntry)->hash.id);
    nextFreeIndex = (*freeValue)->u.next;
    VG_U16(0) = (unsigned short)nextFreeIndex;
    VG_PREV(nextFreeIndex) = 0;

    return index;
}

static void Scr_UpdateMovedCollisionChain(VariableValueInternal *entryValue,
                                          unsigned int oldIndex,
                                          unsigned int newIndex)
{
    unsigned int previousValueId = VG_ID(entryValue->v.next);
    unsigned int scanIndex = VG_NEXT(previousValueId);

    while (scanIndex != oldIndex) {
        previousValueId = VG_ID(scanIndex);
        scanIndex = VG_NEXT(previousValueId);
    }

    VG_NEXT(previousValueId) = (unsigned short)newIndex;
}

static unsigned int Scr_FinishNewVariableIndex(unsigned int parentId,
                                               unsigned int name,
                                               unsigned int returnIndex,
                                               VariableValueInternal *entryForName)
{
    entryForName->w.status = (entryForName->w.status & 0xff) | (name << 8);

    if ((VG_STATUS(parentId) & SCRVL_VAR_MASK) == SCRVL_VAR_ARRAY) {
        ScrVarEntry(parentId)->u.o.u.size++;
        Scr_AddRefToArrayNameValue(name);
    }

    return returnIndex;
}

static unsigned int __attribute_regparm__(3)
    GetNewVariableIndexInternal3(unsigned int parentId, unsigned int name, unsigned int index)
{
    VariableValueInternal *list = (VariableValueInternal *)scrVarGlob;
    VariableValueInternal *entry;
    VariableValueInternal *entryValue;
    VariableValueInternal *newEntry = NULL;
    VariableValueInternal *newEntryValue = NULL;
    VariableValueInternal *parentValue;
    unsigned short newIndex;
    unsigned short next;
    unsigned short prev;
    unsigned short nextSiblingIndex;
    unsigned short prevId;
    unsigned short id;
    unsigned int type;

    entry = &list[index];
    entryValue = &list[entry->hash.id];
    type = entryValue->w.status & SCRVL_VAR_ALLOCATED;

    switch (type) {
    case 0:
        newIndex = entry->v.index;
        next = entryValue->u.next;

        if (newIndex == entry->hash.id ||
            (entry->w.status & SCRVL_VAR_ALLOCATED)) {
            newEntryValue = entryValue;
        } else {
            list[newIndex].hash.id = entry->hash.id;
            entry->hash.id = (unsigned short)index;
            entryValue->v.index = newIndex;
            entryValue->u.next = entry->u.next;
            newEntryValue = entry;
        }

        prev = entry->hash.u.prev;
        list[list[prev].hash.id].u.next = next;
        list[next].hash.u.prev = prev;

        newEntryValue->w.status = SCRVL_VAR_HASH;
        newEntryValue->v.index = (unsigned short)index;
        break;

    case SCRVL_VAR_HASH:
        if (entry->w.status & SCRVL_VAR_ALLOCATED) {
            index = list[0].u.next;
            if (!index) {
                Scr_TerminalError("exceeded maximum number of script variables");
                return 0;
            }

            entry = &list[index];
            newEntryValue = &list[entry->hash.id];
            next = newEntryValue->u.next;
            list[0].u.next = next;
            list[next].hash.u.prev = 0;

            newEntryValue->w.status = SCRVL_VAR_COLLISION;
            newEntryValue->v.next = entryValue->v.next;
            entryValue->v.index = (unsigned short)index;
        } else {
            newIndex = entry->v.index;
            newEntry = &list[newIndex];
            newEntryValue = entry;

            prev = newEntry->hash.u.prev;
            next = entry->u.next;
            list[list[prev].hash.id].u.next = next;
            list[next].hash.u.prevSibling = prev;

            newEntry->hash.id = entry->hash.id;
            entry->hash.id = (unsigned short)index;
            newEntry->hash.u.prev = entry->hash.u.prev;

            list[list[newEntry->hash.u.prev].hash.id].nextSibling = newIndex;
            list[entryValue->nextSibling].hash.u.prevSibling = newIndex;

            entryValue->w.status =
                (entryValue->w.status & ~SCRVL_VAR_ALLOCATED) |
                SCRVL_VAR_COLLISION;
            newEntryValue->w.status = SCRVL_VAR_HASH;
        }
        break;

    default:
        if (entry->w.status & SCRVL_VAR_ALLOCATED) {
            newIndex = list[0].u.next;
            if (!newIndex) {
                Scr_TerminalError("exceeded maximum number of script variables");
                return 0;
            }

            newEntry = &list[newIndex];
            newEntryValue = &list[newEntry->hash.id];
            next = newEntryValue->u.next;
            list[0].u.next = next;
            list[next].hash.u.prev = 0;
        } else {
            newIndex = entry->v.index;
            newEntry = &list[newIndex];
            newEntryValue = entry;

            prev = newEntry->hash.u.prev;
            next = entry->u.next;
            list[list[prev].hash.id].u.next = next;
            list[next].hash.u.prev = prev;
        }

        nextSiblingIndex = entryValue->nextSibling;
        list[list[entry->hash.u.prev].hash.id].nextSibling = newIndex;
        list[nextSiblingIndex].hash.u.prev = newIndex;

        if (type == SCRVL_VAR_COLLISION) {
            nextSiblingIndex = entryValue->v.index;
            prevId = list[nextSiblingIndex].hash.id;

            while (list[prevId].v.index != index)
                prevId = list[list[prevId].v.next].hash.id;

            list[prevId].v.index = newIndex;
        } else {
            entryValue->v.index = newIndex;
        }

        newEntry->hash.u.prev = entry->hash.u.prev;
        id = newEntry->hash.id;
        newEntry->hash.id = entry->hash.id;
        entry->hash.id = id;
        newEntryValue->w.status = SCRVL_VAR_HASH;
        newEntryValue->v.index = (unsigned short)index;
        break;
    }

    newEntryValue->w.status = (unsigned char)newEntryValue->w.status;
    newEntryValue->w.status |= name << 8;

    parentValue = &list[parentId];
    if ((parentValue->w.status & SCRVL_VAR_MASK) == SCRVL_VAR_ARRAY) {
        parentValue->u.o.u.size++;
        Scr_AddRefToArrayNameValue(name);
    }

    return index;
}

unsigned int GetNewObjectVariableReverse(unsigned int parentId, unsigned int id)
{
    unsigned int name = id + 0x10000;
    unsigned int newIndex = GetNewVariableIndexInternal3(parentId, name, Scr_VariableHash(parentId, name));
    unsigned int previousIndex = VG_PREV(VG_NEXT_SIBLING(parentId));
    unsigned int siblingIndex = VG_PREV(previousIndex);
    unsigned int siblingId = VG_ID(siblingIndex);
    unsigned int newId = VG_ID(newIndex);

    VG_NEXT_SIBLING(newId) = VG_NEXT(parentId);
    VG_PREV(previousIndex) = (unsigned short)newIndex;
    VG_PREV(newIndex) = (unsigned short)siblingIndex;
    VG_NEXT_SIBLING(siblingId) = (unsigned short)newIndex;

    return newId;
}

unsigned int GetNewObjectVariable(unsigned int parentId, unsigned int id)
{
    unsigned int name = id + 0x10000;
    unsigned int newIndex = GetNewVariableIndexInternal3(parentId, name, Scr_VariableHash(parentId, name));
    unsigned int parentTailIndex = VG_NEXT_SIBLING(parentId);
    unsigned int newId = VG_ID(newIndex);

    VG_NEXT_SIBLING(newId) = (unsigned short)parentTailIndex;
    VG_PREV(parentTailIndex) = (unsigned short)newIndex;
    VG_PREV(newIndex) = VG_NEXT(parentId);
    VG_NEXT_SIBLING(parentId) = (unsigned short)newIndex;

    return newId;
}

void SetVariableEntityFieldValue(unsigned int entId, unsigned int fieldName, VariableValue *value)
{
    unsigned int classnum = VG_STATUS(entId) >> 8;
    unsigned int internalName = GetInternalVariableIndex_core(fieldName);
    unsigned int fieldIndex = FindVariableIndexInternal(g_classMap[classnum].id, internalName);
    unsigned int fieldId = VG_ID(fieldIndex);

    if (fieldId) {
        if (SetEntityFieldValue(classnum, VG_SIBLING(entId), (int)VG_U32(fieldId), value))
            return;
    }

    fieldIndex = GetNewVariableIndexInternal3(entId, fieldName, Scr_VariableHash(entId, fieldName));
    fieldId = Scr_LinkVariableToParent(entId, fieldIndex);
    SetNewVariableValue(fieldId, value);
}

void Scr_AddClassField(int classnum, const char *name, unsigned int offset)
{
    unsigned int classId = g_classMap[classnum].id;
    unsigned int fieldOffset = offset & 0xffff;
    unsigned int fieldName = GetInternalVariableIndex_core(SL_GetCanonicalString(name));
    unsigned int newIndex = GetNewVariableIndexInternal3(classId, fieldName, Scr_VariableHash(classId, fieldName));
    unsigned int fieldId = Scr_LinkVariableToParent(classId, newIndex);
    unsigned int stringName;

    VG_STATUS(fieldId) = (VG_STATUS(fieldId) & ~SCRVL_VAR_MASK) | SCRVL_VAR_INTEGER;
    VG_U32(fieldId) = fieldOffset;

    stringName = SL_GetString_(name, 0, 0xf);
    newIndex = GetNewVariableIndexInternal3(classId, stringName, Scr_VariableHash(classId, stringName));
    fieldId = Scr_LinkVariableToParent(classId, newIndex);
    SL_RemoveRefToString(stringName);

    VG_STATUS(fieldId) = (VG_STATUS(fieldId) & ~SCRVL_VAR_MASK) | SCRVL_VAR_INTEGER;
    VG_U32(fieldId) = fieldOffset;
}

unsigned int GetNewArrayVariable(unsigned int parentId, unsigned int unsignedValue)
{
    unsigned int name = GetInternalVariableIndex_core(unsignedValue);
    unsigned int newIndex = GetNewVariableIndexInternal3(parentId, name, Scr_VariableHash(parentId, name));

    return Scr_LinkVariableToParent(parentId, newIndex);
}

unsigned int GetNewVariable(unsigned int parentId, unsigned int unsignedValue)
{
    unsigned int name = unsignedValue;
    unsigned int newIndex = GetNewVariableIndexInternal3(parentId, name, Scr_VariableHash(parentId, name));

    return Scr_LinkVariableToParent(parentId, newIndex);
}

static float Scr_GetVariableUsage(unsigned int id)
{
    unsigned int type = VG_STATUS(id) & SCRVL_VAR_MASK;

    if (type == SCRVL_VAR_POINTER) {
        unsigned int parentId = VG_U32(id);

        if ((VG_STATUS(parentId) & SCRVL_VAR_MASK) == SCRVL_VAR_ARRAY) {
            float refCount = (float)VG_U16(parentId) + 1.0f;
            return 1.0f + Scr_GetObjectUsage(parentId) / refCount;
        }
    }

    return 1.0f;
}

static float __attribute_regparm__(1)
    Scr_GetObjectUsage(unsigned int parentId)
{
    float usage = 1.0f;
    unsigned int id = FindNextSibling(parentId);

    while (id) {
        usage += Scr_GetVariableUsage(id);
        id = FindNextSibling(id);
    }

    return usage;
}
void Scr_DumpScriptThreads(void)
{
    unsigned int id;
    int count = 0;

    Com_Printf("Script threads:\n");

    for (id = 1; id <= SCRVL_MAX_VARIABLES; id++) {
        VariableStackBuffer *stackValue;
        float usage;

        if (!(VG_STATUS(id) & SCRVL_VAR_ALLOCATED))
            continue;

        if ((VG_STATUS(id) & SCRVL_VAR_MASK) != 0xa)
            continue;

#if defined(__x86_64__) || defined(_M_X64)
        if (!VG_U32(id))
            continue;
        stackValue = (VariableStackBuffer *)(scrMemTreeGlob + VG_U32(id));
#else
        stackValue = (VariableStackBuffer *)VG_U32(id);
        if (!stackValue)
            continue;
#endif

        usage = Scr_GetObjectUsage(stackValue->localId);
        Com_Printf("  thread %u pos=%p locals=%u stack=%u usage=%.2f\n",
                   id, stackValue->pos, stackValue->localId, stackValue->size, usage);
        count++;
    }

    Com_Printf("%d script threads\n", count);
}
unsigned int Scr_GetEntityId(int entnum, int classnum)
{
    unsigned int entArrayId = g_classMap[classnum].entArrayId;
    unsigned int name = GetInternalVariableIndex_core((unsigned int)entnum);
    unsigned int entryIndex = FindVariableIndexInternal(entArrayId, name);
    unsigned int entryId;
    unsigned int entityId;

    if (!entryIndex) {
        entryIndex = GetNewVariableIndexInternal3(entArrayId, name, Scr_VariableHash(entArrayId, name));
        entryId = Scr_LinkVariableToParent(entArrayId, entryIndex);
    } else {
        entryId = VG_ID(entryIndex);
    }

    if ((VG_STATUS(entryId) & SCRVL_VAR_MASK) != SCRVL_VAR_UNDEFINED)
        return VG_U32(entryId);

    entityId = AllocVariable();
    VG_STATUS(entityId) = (classnum << 8) | SCRVL_VAR_ALLOCATED | SCRVL_VAR_ENTITY;
    VG_U16(entityId) = 0;
    VG_SIBLING(entityId) = (unsigned short)entnum;

    VG_STATUS(entryId) |= SCRVL_VAR_POINTER;
    VG_U32(entryId) = entityId;

    return entityId;
}

unsigned int Scr_EvalArrayIndex(unsigned int parentId, VariableValue *index)
{
    if (index->type == SCRVL_VAR_INTEGER) {
        int intValue = index->u.intValue;

        if (!IsValidArrayIndex((unsigned int)intValue)) {
            Scr_Error(va("array index %d out of range", intValue));
            return 0;
        }

        return GetArrayVariable(parentId, (unsigned int)intValue);
    }

    if (index->type == SCRVL_VAR_STRING) {
        unsigned int name = index->u.stringValue;
        unsigned int entryIndex = FindVariableIndexInternal(parentId, name);
        unsigned int id;

        if (!entryIndex) {
            entryIndex = GetNewVariableIndexInternal3(parentId, name, Scr_VariableHash(parentId, name));
            id = Scr_LinkVariableToParent(parentId, entryIndex);
        } else {
            id = VG_ID(entryIndex);
        }

        SL_RemoveRefToString(name);
        return id;
    }

    Scr_Error(va("%s is not an array index", var_typename[index->type]));
    return 0;
}

unsigned int Scr_GetVariableField(unsigned int parentId, unsigned int name)
{
    int type = (int)(VG_STATUS(parentId) & SCRVL_VAR_MASK);
    unsigned int fieldIndex;

    if (type <= SCRVL_VAR_OBJECT) {
        fieldIndex = FindVariableIndexInternal(parentId, name);
        if (!fieldIndex) {
            fieldIndex = GetNewVariableIndexInternal3(parentId, name, Scr_VariableHash(parentId, name));
            return Scr_LinkVariableToParent(parentId, fieldIndex);
        }

        return VG_ID(fieldIndex);
    }

    if (type == SCRVL_VAR_ENTITY) {
        fieldIndex = FindVariableIndexInternal(parentId, name);
        if (fieldIndex)
            return VG_ID(fieldIndex);

        scrVarPub.entId = parentId;
        scrVarPub.entFieldName = name;
        return 0xfffe;
    }

    Scr_Error(va("cannot set field of %s", var_typename[type]));
    return 0;
}

unsigned int GetArrayVariable(unsigned int parentId, unsigned int unsignedValue)
{
    unsigned int name = GetInternalVariableIndex_core(unsignedValue);
    unsigned int entryIndex = FindVariableIndexInternal(parentId, name);

    if (!entryIndex) {
        entryIndex = GetNewVariableIndexInternal3(parentId, name, Scr_VariableHash(parentId, name));
        return Scr_LinkVariableToParent(parentId, entryIndex);
    }

    return VG_ID(entryIndex);
}

unsigned int GetVariable(unsigned int parentId, unsigned int unsignedValue)
{
    unsigned int name = unsignedValue;
    unsigned int entryIndex = FindVariableIndexInternal(parentId, name);

    if (!entryIndex) {
        entryIndex = GetNewVariableIndexInternal3(parentId, name, Scr_VariableHash(parentId, name));
        return Scr_LinkVariableToParent(parentId, entryIndex);
    }

    return VG_ID(entryIndex);
}

unsigned int GetObjectVariable(unsigned int parentId, unsigned int id)
{
    unsigned int name = id + SCRVL_SL_MAX_STRING_INDEX;
    unsigned int entryIndex = FindVariableIndexInternal(parentId, name);

    if (!entryIndex) {
        entryIndex = GetNewVariableIndexInternal3(parentId, name, Scr_VariableHash(parentId, name));
        return Scr_LinkVariableToParent(parentId, entryIndex);
    }

    return VG_ID(entryIndex);
}

static void __attribute_regparm__(2)
    CopyArray(unsigned int parentId, unsigned int newParentId)
{
    unsigned int id = ScrVar_NextSiblingId(parentId);

    while (id && id != parentId) {
        unsigned int type = VG_STATUS(id) & SCRVL_VAR_MASK;
        unsigned int name = VG_NAME(id);
        unsigned int newIndex = FindVariableIndexInternal(newParentId, name);
        unsigned int newId;

        if (!newIndex) {
            newIndex = GetNewVariableIndexInternal3(newParentId, name, Scr_VariableHash(newParentId, name));
            newId = Scr_LinkVariableToParent(newParentId, newIndex);
        } else {
            newId = VG_ID(newIndex);
        }

        VG_STATUS(newId) |= type;

        if (type == SCRVL_VAR_POINTER) {
            unsigned int objectId = VG_U32(id);

            if ((VG_STATUS(objectId) & SCRVL_VAR_MASK) == SCRVL_VAR_ARRAY) {
                unsigned int newArrayId = Scr_AllocArray();

                VG_U32(newId) = newArrayId;
                CopyArray(objectId, newArrayId);
            } else {
                VG_U32(newId) = objectId;
                AddRefToObject(objectId);
            }
        } else {
            VG_U32(newId) = VG_U32(id);

            if (type == SCRVL_VAR_STRING || type == SCRVL_VAR_LOCALIZED_STRING) {
                SL_AddRefToString(VG_U32(id));
            } else if (type == SCRVL_VAR_VECTOR && !*((const byte *)VG_U32(id) - 1)) {
                (*(unsigned short *)((byte *)VG_U32(id) - 4))++;
            }
        }

        id = ScrVar_NextSiblingId(id);
    }
}
static inline __attribute__((always_inline)) unsigned int Scr_AllocArray_core(void)
{
    unsigned int result = AllocVariable();
    VG_STATUS(result) = 0x60;
    VG_STATUS(result) |= 0x16;
    VG_U16(result) = 0;
    VG_SIBLING(result) = 0;
    return result;
}

void Scr_SetClassMap(int classnum)
{
    g_classMap[classnum].entArrayId = (unsigned short)Scr_AllocArray_core();
    g_classMap[classnum].id = (unsigned short)Scr_AllocArray_core();
}

unsigned int GetArray(unsigned int id)
{
    if ((VG_STATUS(id) & SCRVL_VAR_MASK) == SCRVL_VAR_UNDEFINED) {
        VG_STATUS(id) |= SCRVL_VAR_POINTER;
        VG_U32(id) = Scr_AllocArray_core();
    }

    return VG_U32(id);
}

static inline __attribute__((always_inline)) unsigned int AllocObject_core(void)
{
    unsigned int result = AllocVariable();
    VG_STATUS(result) = 0x60;
    VG_STATUS(result) |= 0x13;
    VG_U16(result) = 0;
    return result;
}

unsigned int GetObjectA(unsigned int id)
{
    unsigned int type = VG_STATUS(id) & SCRVL_VAR_MASK;

    if (type == SCRVL_VAR_UNDEFINED) {

        VG_STATUS(id) = (VG_STATUS(id) & ~SCRVL_VAR_MASK) | SCRVL_VAR_POINTER;
        VG_U32(id) = AllocObject_core();
    }
    return VG_U32(id);
}

unsigned int AllocValue(void)
{
    unsigned int result = AllocVariable();

    VG_STATUS(result) = 0x60;
    return result;
}

unsigned int AllocObject(void)
{
    return AllocObject_core();
}

unsigned int Scr_AllocArray(void)
{
    return Scr_AllocArray_core();
}

unsigned int AllocThread(unsigned int self)
{
    unsigned int result = AllocVariable();
    VG_STATUS(result) = 0x60;
    VG_STATUS(result) |= 0x0F;
    VG_U16(result) = 0;
    VG_SIBLING(result) = (unsigned short)self;
    return result;
}

unsigned int AllocChildThread(unsigned int self, unsigned int parentLocalId)
{
    unsigned int result = AllocVariable();
    VG_STATUS(result) = 0x60;
    VG_STATUS(result) |= 0x12;
    VG_STATUS(result) |= (parentLocalId << 8);
    VG_U16(result) = 0;
    VG_SIBLING(result) = (unsigned short)self;
    return result;
}

static inline __attribute__((always_inline)) void Scr_RemoveRefToArrayNameValue(unsigned int name)
{
    VariableUnion value;
    int type;

    if (name <= 0xffff) {
        value.stringValue = name;
        type = SCRVL_VAR_STRING;
    } else if (name <= 0x1fffd) {
        value.pointerValue = name - SCRVL_SL_MAX_STRING_INDEX;
        type = SCRVL_VAR_POINTER;
    } else {
        value.intValue = name - 0x800000;
        type = SCRVL_VAR_INTEGER;
    }

    RemoveRefToValue(type, value);
}

static void __attribute_regparm__(2)
    MakeVariableExternal(VariableValueInternal *entry, VariableValueInternal *parentValue)
{
    unsigned int entryIndex = ScrVarEntryIndex(entry);
    unsigned int valueId = entry->hash.id;
    VariableValueInternal *entryValue = ScrVarEntry(valueId);
    unsigned int status;

    if ((parentValue->w.status & SCRVL_VAR_MASK) == SCRVL_VAR_ARRAY) {
        parentValue->u.o.u.size--;
        Scr_RemoveRefToArrayNameValue(entryValue->w.status >> 8);
    }

    status = entryValue->w.status;

    if ((status & SCRVL_VAR_ALLOCATED) == SCRVL_VAR_HASH) {
        unsigned int swapIndex = entryValue->v.index;
        VariableValueInternal *swapEntry = ScrVarEntry(swapIndex);
        VariableValueInternal *swapValue = ScrVarEntry(swapEntry->hash.id);

        if (swapEntry != entry) {
            unsigned int entryPrevIndex = entry->hash.u.prev;
            unsigned int entryNextSiblingIndex = entryValue->nextSibling;
            unsigned int swapPrevIndex = swapEntry->hash.u.prev;
            unsigned int swapNextSiblingIndex = swapValue->nextSibling;
            unsigned int entryHash = *(unsigned int *)&entry->hash;

            swapValue->w.status = (swapValue->w.status & ~SCRVL_VAR_ALLOCATED) | SCRVL_VAR_HASH;

            VG_PREV(swapNextSiblingIndex) = (unsigned short)entryIndex;
            VG_NEXT_SIBLING(VG_ID(swapPrevIndex)) = (unsigned short)entryIndex;
            VG_PREV(entryNextSiblingIndex) = (unsigned short)swapIndex;
            VG_NEXT_SIBLING(VG_ID(entryPrevIndex)) = (unsigned short)swapIndex;

            *(unsigned int *)&entry->hash = *(unsigned int *)&swapEntry->hash;
            *(unsigned int *)&swapEntry->hash = entryHash;

            status = entryValue->w.status;
            entryIndex = swapIndex;
        }
    } else {
        VariableValueInternal *prevEntry = entry;
        VariableValueInternal *scanValue = entryValue;

        for (;;) {
            unsigned int nextIndex = scanValue->v.index;
            VariableValueInternal *nextEntry = ScrVarEntry(nextIndex);

            if (nextEntry == entry)
                break;

            prevEntry = nextEntry;
            scanValue = ScrVarEntry(nextEntry->hash.id);
        }

        VG_NEXT(prevEntry->hash.id) = entryValue->v.index;
    }

    entryValue->w.status = (status & ~SCRVL_VAR_ALLOCATED) | SCRVL_VAR_ALLOCATED;
    entryValue->v.index = (unsigned short)entryIndex;
}

void RemoveRefToObject(unsigned int id)
{
    unsigned int status;
    unsigned short refCount = VG_U16(id);

    if (refCount) {
        refCount--;
        VG_U16(id) = refCount;

        if (!refCount) {
            status = VG_STATUS(id);
            if ((status & SCRVL_VAR_MASK) == SCRVL_VAR_ENTITY &&
                VG_ID(VG_NEXT_SIBLING(id)) == id) {
                status = (status & ~SCRVL_VAR_MASK) | 0x14;
                VG_STATUS(id) = status;
                RemoveVariable(g_classMap[status >> 8].entArrayId,
                               GetInternalVariableIndex_core(VG_SIBLING(id)));
            }
        }

        return;
    }

    if (VG_ID(VG_NEXT_SIBLING(id)) != id) {
        AddRefToObject(id);
        Scr_FreeObjectChildren(id);
        RemoveRefToEmptyObject(id);
    }

    FreeVariable(id);
}

void Scr_FreeValue(unsigned int id)
{
    RemoveRefToObject(id);
}

void Scr_EvalArray(VariableValue *value, VariableValue *index)
{
    if (value->type == SCRVL_VAR_STRING) {
        const char *str;
        int len;
        int offset;
        char c[2];

        if (index->type != SCRVL_VAR_INTEGER) {
            Scr_Error(va("%s is not a string index", var_typename[index->type]));
            return;
        }

        offset = index->u.intValue;
        str = SL_ConvertToString(value->u.stringValue);
        len = strlen(str);
        if (offset < 0 || offset >= len) {
            Scr_Error(va("string index %d out of range", offset));
            return;
        }

        c[0] = str[offset];
        c[1] = '\0';
        index->type = SCRVL_VAR_STRING;
        index->u.stringValue = SL_GetStringOfLen(c, 0, 2, 0xe);
        SL_RemoveRefToString(value->u.stringValue);
        return;
    }

    if (value->type == SCRVL_VAR_VECTOR) {
        int offset;

        if (index->type != SCRVL_VAR_INTEGER) {
            Scr_Error(va("%s is not a vector index", var_typename[index->type]));
            return;
        }

        offset = index->u.intValue;
        if ((unsigned int)offset > 2) {
            Scr_Error(va("vector index %d out of range", offset));
            return;
        }

        index->type = SCRVL_VAR_FLOAT;
        index->u.floatValue = SCR_VEC_PTR(value->u)[offset];
        RemoveRefToVector(SCR_VEC_PTR(value->u));
        return;
    }

    if (value->type == SCRVL_VAR_POINTER) {
        unsigned int parentId = value->u.pointerValue;
        unsigned int id = 0;
        unsigned long long result;

        if ((VG_STATUS(parentId) & SCRVL_VAR_MASK) != SCRVL_VAR_ARRAY) {
            scrVarPub.error_index = 1;
            Scr_Error(va("%s is not an array",
                         var_typename[VG_STATUS(parentId) & SCRVL_VAR_MASK]));
            return;
        }

        if (index->type == SCRVL_VAR_INTEGER) {
            int intValue = index->u.intValue;

            if (IsValidArrayIndex((unsigned int)intValue)) {
                id = FindArrayVariable(parentId, intValue);
            } else {
                Scr_Error(va("array index %d out of range", intValue));
                AddRefToObject(parentId);
            }
        } else if (index->type == SCRVL_VAR_STRING) {
            unsigned int name = index->u.stringValue;

            id = FindVariable(parentId, name);
            SL_RemoveRefToString(name);
        } else {
            Scr_Error(va("%s is not an array index", var_typename[index->type]));
            AddRefToObject(parentId);
        }

        result = Scr_EvalVariable(id);
        index->u.intValue = (unsigned int)result;
        index->type = (int)(result >> 32);
        RemoveRefToObject(parentId);
        return;
    }

    scrVarPub.error_index = 1;
    Scr_Error(va("%s is not an array, string, or vector", var_typename[value->type]));
}

void Scr_RemoveClassMap(int classnum)
{
    if (!scrVarPub.bInited) {
        return;
    }

    RemoveRefToObject(g_classMap[classnum].entArrayId);
    g_classMap[classnum].entArrayId = 0;
    RemoveRefToObject(g_classMap[classnum].id);
    g_classMap[classnum].id = 0;
}

static inline __attribute__((always_inline)) unsigned long long Scr_PackValueResult(unsigned int value, unsigned int type)
{
    return ((unsigned long long)type << 32) | value;
}

static unsigned long long __attribute_regparm__(2)
    Scr_EvalVariableEntityField(unsigned int entId, unsigned int name)
{
    unsigned int classnum = VG_STATUS(entId) >> 8;
    unsigned int fieldName = GetInternalVariableIndex_core(name);
    unsigned int fieldIndex = FindVariableIndexInternal(g_classMap[classnum].id, fieldName);
    unsigned int value;
    unsigned int type;
    unsigned long long result;

    if (!fieldIndex)
        return Scr_PackValueResult(0, SCRVL_VAR_UNDEFINED);

    result = GetEntityFieldValue(classnum, VG_SIBLING(entId), (int)VG_U32(VG_ID(fieldIndex)));
    value = (unsigned int)result;
    type = (unsigned int)(result >> 32);

    if (type == SCRVL_VAR_POINTER &&
        (VG_STATUS(value) & SCRVL_VAR_MASK) == SCRVL_VAR_ARRAY &&
        VG_U16(value)) {
        unsigned int newArrayId;

        RemoveRefToObject(value);
        newArrayId = Scr_AllocArray();
        CopyArray(value, newArrayId);
        value = newArrayId;
    }

    return Scr_PackValueResult(value, type);
}

unsigned long long Scr_EvalVariableField(unsigned int id)
{
    if (id == 0xfffe)
        return Scr_EvalVariableEntityField(scrVarPub.entId, scrVarPub.entFieldName);

    return Scr_EvalVariable_core(id);
}

unsigned long long Scr_FindVariableField(unsigned int parentId, unsigned int name)
{
    unsigned int entryIndex = FindVariableIndexInternal(parentId, name);

    if (entryIndex)
        return Scr_EvalVariable_core(VG_ID(entryIndex));

    if ((VG_STATUS(parentId) & SCRVL_VAR_MASK) == SCRVL_VAR_ENTITY)
        return Scr_EvalVariableEntityField(parentId, name);

    return Scr_PackValueResult(0, SCRVL_VAR_UNDEFINED);
}
void RemoveRefToValue(int type, VariableUnion u)
{
    RemoveRefToValue_core(type, u);
}

void Scr_KillEndonThread(unsigned int threadId)
{
    VariableValueInternal *parentValue = (VariableValueInternal *)((byte *)scrVarGlob + threadId * 16);

    RemoveRefToObject(parentValue->u.o.u.self);
    parentValue->w.status = (parentValue->w.status & ~0x1f) | 0x17;
}

static inline __attribute__((always_inline)) void Scr_MakeObjectChildrenExternal(unsigned int parentId)
{
    VariableValueInternal *parentValue = ScrVarEntry(parentId);
    unsigned int index = parentValue->nextSibling;
    unsigned int id = VG_ID(index);

    while (id != parentId) {
        MakeVariableExternal(ScrVarEntry(index), parentValue);
        index = VG_NEXT_SIBLING(id);
        id = VG_ID(index);
    }
}

static inline __attribute__((always_inline)) void Scr_FreeObjectChildren(unsigned int parentId)
{
    unsigned int childId;

    Scr_MakeObjectChildrenExternal(parentId);

    while ((childId = VG_ID(VG_NEXT_SIBLING(parentId))) != parentId)
        FreeChildValue_core(childId);
}

void Scr_FreeEntityList(void)
{
    while (scrVarPub.freeEntList) {
        unsigned int id = scrVarPub.freeEntList;

        scrVarPub.freeEntList = VG_SIBLING(id);
        VG_SIBLING(id) = 0;

        Scr_CancelNotifyList(id);
        if (VG_ID(VG_NEXT_SIBLING(id)) != id)
            Scr_FreeObjectChildren(id);
        RemoveRefToObject(id);
    }
}

static inline __attribute__((always_inline)) void SafeRemoveVariable_core(unsigned int parentId, unsigned int unsignedValue)
{
    unsigned int index = FindVariableIndexInternal(parentId, unsignedValue);
    unsigned int id;

    if (!index)
        return;

    id = VG_ID(index);
    MakeVariableExternal(ScrVarEntry(index), ScrVarEntry(parentId));
    FreeChildValue_core(id);
}

void SafeRemoveVariable(unsigned int parentId, unsigned int unsignedValue)
{
    SafeRemoveVariable_core(parentId, unsignedValue);
}

void RemoveNextVariable(unsigned int parentId)
{
    unsigned int index = VG_NEXT_SIBLING(parentId);
    unsigned int id = VG_ID(index);

    MakeVariableExternal(ScrVarEntry(index), ScrVarEntry(parentId));
    FreeChildValue_core(id);
}

void RemoveVariable(unsigned int parentId, unsigned int unsignedValue)
{
    unsigned int index = FindVariableIndexInternal(parentId, unsignedValue);
    unsigned int id;

    /* Slot zero anchors the free list. Never unlink it when a key is absent. */
    if (!index)
        return;

    id = VG_ID(index);
    MakeVariableExternal(ScrVarEntry(index), ScrVarEntry(parentId));
    FreeChildValue_core(id);
}

static int __attribute_regparm__(1)
    Scr_MakeValuePrimitive(unsigned int parentId)
{
    unsigned int id;

    if ((VG_STATUS(parentId) & SCRVL_VAR_MASK) != SCRVL_VAR_ARRAY)
        return 0;

    id = FindNextSibling(parentId);
    while (id) {
        unsigned int type = VG_STATUS(id) & SCRVL_VAR_MASK;
        unsigned int name = VG_NAME(id);

        if (type == SCRVL_VAR_POINTER) {
            if (!Scr_MakeValuePrimitive(VG_U32(id))) {
                RemoveVariable(parentId, name);
                id = FindNextSibling(parentId);
                continue;
            }
        } else if (type >= 7 && type <= 11) {
            RemoveVariable(parentId, name);
            id = FindNextSibling(parentId);
            continue;
        }

        id = FindNextSibling(id);
    }

    return 1;
}

void Scr_FreeGameVariable(int bComplete)
{
    if (bComplete) {
        FreeChildValue_core(scrVarPub.gameId);
        scrVarPub.gameId = 0;
        return;
    }

    Scr_MakeValuePrimitive(VG_U32(scrVarPub.gameId));
}

void Scr_FreeEntityNum(int entnum, int classnum)
{
    unsigned int entArrayId;
    unsigned int name;
    unsigned int entryIndex;
    unsigned int entryId;
    unsigned int entId;

    if (!scrVarPub.bInited)
        return;

    entArrayId = g_classMap[classnum].entArrayId;
    name = GetInternalVariableIndex_core((unsigned int)entnum);
    entryIndex = FindVariableIndexInternal(entArrayId, name);

    entryId = VG_ID(entryIndex);
    if (!entryId)
        return;

    entId = VG_U32(entryId);
    VG_STATUS(entId) = (VG_STATUS(entId) & ~SCRVL_VAR_MASK) | 0x14;
    VG_U16(entId)
    ++;
    VG_SIBLING(entId) = (unsigned short)scrVarPub.freeEntList;
    scrVarPub.freeEntList = entId;

    RemoveVariable(entArrayId, name);
}

void RemoveObjectVariable(unsigned int parentId, unsigned int id)
{
    RemoveVariable(parentId, id + 0x10000);
}

void ClearVariableField(unsigned int parentId, unsigned int name, VariableValue *value)
{
    unsigned int fieldIndex = FindVariableIndexInternal(parentId, name);

    if (fieldIndex) {
        RemoveVariable(parentId, name);
        return;
    }

    if ((VG_STATUS(parentId) & SCRVL_VAR_MASK) == SCRVL_VAR_ENTITY) {
        unsigned int classnum = VG_STATUS(parentId) >> 8;
        unsigned int internalName = GetInternalVariableIndex_core(name);

        fieldIndex = FindVariableIndexInternal(g_classMap[classnum].id, internalName);
        if (fieldIndex) {
            unsigned int fieldId = VG_ID(fieldIndex);
            VariableValue *clearValue = value + 1;

            clearValue->type = SCRVL_VAR_UNDEFINED;
            SetEntityFieldValue(classnum, VG_SIBLING(parentId), (int)VG_U32(fieldId), clearValue);
        }
    }
}

void Var_Shutdown(void)
{
    if (scrVarPub.gameId) {
        FreeChildValue_core(scrVarPub.gameId);
        scrVarPub.gameId = 0;
    }
}

void Scr_FreeObjects(void)
{
    unsigned int id;

    for (id = 1; id <= SCRVL_MAX_VARIABLES; id++) {
        unsigned int status = VG_STATUS(id);
        unsigned int type;

        if (!(status & SCRVL_VAR_ALLOCATED))
            continue;

        type = status & SCRVL_VAR_MASK;
        if (type != 0x13 && type != 0x14)
            continue;

        Scr_CancelNotifyList(id);
        AddRefToObject(id);
        Scr_FreeObjectChildren(id);
        RemoveRefToEmptyObject(id);
    }
}

static inline __attribute__((always_inline)) void Scr_CastBool_core(VariableValue *value)
{
    int type = value->type;

    if (type == SCRVL_VAR_INTEGER) {
        value->u.intValue = value->u.intValue != 0;
        return;
    }

    if (type == SCRVL_VAR_FLOAT) {
        value->u.intValue = value->u.floatValue != 0.0f;
        value->type = SCRVL_VAR_INTEGER;
        return;
    }

    RemoveRefToValue_core(type, value->u);
    value->type = SCRVL_VAR_UNDEFINED;
    Scr_Error(va("cannot cast %s to Bool", var_typename[type]));
}

void Scr_EvalBoolNot(VariableValue *value)
{
    Scr_CastBool_core(value);

    if (value->type == SCRVL_VAR_INTEGER)
        value->u.intValue = value->u.intValue == 0;
}

void SetVariableFieldValue(unsigned int id, VariableValue *value)
{
    if (id == 0xfffe) {
        SetVariableEntityFieldValue(scrVarPub.entId, scrVarPub.entFieldName, value);
        return;
    }

    SetVariableValue_core(id, value);
}

void Scr_AllocGameVariable(void)
{
    struct scrVarPub_t *p = (struct scrVarPub_t *)imp_scrVarPub;
    VariableValue tempValue;

    if (p->gameId)
        return;

    p->gameId = AllocValue();
    tempValue.u.pointerValue = Scr_AllocArray();
    tempValue.type = SCRVL_VAR_POINTER;
    SetVariableValue(p->gameId, &tempValue);
}

static unsigned int Scr_GetOrCreateLocalVariable(unsigned int parentId, unsigned int name)
{
    unsigned int entryIndex = FindVariableIndexInternal(parentId, name);

    if (!entryIndex) {
        entryIndex = GetNewVariableIndexInternal3(parentId, name,
                                                  Scr_VariableHash(parentId, name));
        return Scr_LinkVariableToParent(parentId, entryIndex);
    }

    return VG_ID(entryIndex);
}

static unsigned int Scr_CopyArrayForWrite(unsigned int fieldId, unsigned int arrayId)
{
    unsigned int newArrayId;

    if (!VG_U16(arrayId))
        return arrayId;

    RemoveRefToObject(arrayId);
    newArrayId = Scr_AllocArray();
    CopyArray(arrayId, newArrayId);

    if (fieldId)
        VG_U32(fieldId) = newArrayId;

    return newArrayId;
}

unsigned int Scr_EvalArrayRef(unsigned int parentId)
{
    unsigned int type;
    unsigned int arrayId;

    if (parentId == 0xfffe) {
        unsigned int entId = scrVarPub.entId;
        unsigned int fieldName = scrVarPub.entFieldName;
        unsigned int classnum = VG_STATUS(entId) >> 8;
        unsigned int internalName = GetInternalVariableIndex_core(fieldName);
        unsigned int fieldIndex = FindVariableIndexInternal(g_classMap[classnum].id, internalName);

        if (fieldIndex) {
            unsigned int offset = VG_U32(VG_ID(fieldIndex));
            unsigned long long result = GetEntityFieldValue(classnum, VG_SIBLING(entId), (int)offset);
            unsigned int value = (unsigned int)result;
            unsigned int valueType = (unsigned int)(result >> 32);

            if (valueType == SCRVL_VAR_UNDEFINED) {
                parentId = Scr_GetOrCreateLocalVariable(entId, fieldName);
            } else if (valueType == SCRVL_VAR_POINTER) {
                VariableValue newValue;

                if (!VG_U16(value)) {
                    RemoveRefToObject(value);
                    scrVarPub.error_index = 1;
                    Scr_Error("cannot set field of removed entity");
                    return 0;
                }

                RemoveRefToObject(value);
                if ((VG_STATUS(value) & SCRVL_VAR_MASK) != SCRVL_VAR_ARRAY) {
                    scrVarPub.error_index = 1;
                    Scr_Error(va("%s is not an array",
                                 var_typename[VG_STATUS(value) & SCRVL_VAR_MASK]));
                    return 0;
                }

                arrayId = Scr_CopyArrayForWrite(0, value);
                if (arrayId != value) {
                    newValue.u.pointerValue = arrayId;
                    newValue.type = SCRVL_VAR_POINTER;
                    SetEntityFieldValue(classnum, VG_SIBLING(entId), (int)offset, &newValue);
                }

                return arrayId;
            } else {
                scrVarPub.error_index = 1;
                Scr_Error(va("%s is not an array", var_typename[valueType]));
                return 0;
            }
        } else {
            parentId = Scr_GetOrCreateLocalVariable(entId, fieldName);
        }
    }

    type = VG_STATUS(parentId) & SCRVL_VAR_MASK;
    if (type == SCRVL_VAR_UNDEFINED) {
        VG_STATUS(parentId) |= SCRVL_VAR_POINTER;
        arrayId = Scr_AllocArray_core();
        VG_U32(parentId) = arrayId;
        return arrayId;
    }

    if (type != SCRVL_VAR_POINTER) {
        scrVarPub.error_index = 1;
        if (type == SCRVL_VAR_STRING)
            Scr_Error("string is not an array");
        else if (type == SCRVL_VAR_VECTOR)
            Scr_Error("vector is not an array");
        else
            Scr_Error(va("%s is not an array", var_typename[type]));
        return 0;
    }

    arrayId = VG_U32(parentId);
    if ((VG_STATUS(arrayId) & SCRVL_VAR_MASK) != SCRVL_VAR_ARRAY) {
        scrVarPub.error_index = 1;
        Scr_Error(va("%s is not an array",
                     var_typename[VG_STATUS(arrayId) & SCRVL_VAR_MASK]));
        return 0;
    }

    return Scr_CopyArrayForWrite(parentId, arrayId);
}

void Scr_StopThread(unsigned int threadId)
{
    Scr_FreeObjectChildren(threadId);
    RemoveRefToObject(VG_SIBLING(threadId));
    VG_SIBLING(threadId) = (unsigned short)scrVarPub.levelId;
    AddRefToObject(scrVarPub.levelId);
}

void ClearObjectInternal(unsigned int parentId)
{
    unsigned int guard = 0;

    for (;;) {
        unsigned int childId = ScrVar_NextSiblingId(parentId);
        unsigned int name;

        if (!childId || childId == parentId)
            return;

        name = GetVariableName(childId);
        RemoveVariable(parentId, name);

        if (++guard > SCRVL_MAX_VARIABLES) {
            Scr_TerminalError("exceeded maximum number of script variables");
            return;
        }
    }
}

void ClearObject(unsigned int id)
{
    AddRefToObject(id);
    Scr_FreeObjectChildren(id);
    RemoveRefToEmptyObject(id);
}

void Scr_KillThread(unsigned int parentId)
{
    unsigned int pauseEntry;

    Scr_FreeObjectChildren(parentId);

    RemoveRefToObject(VG_SIBLING(parentId));

    pauseEntry = FindObjectVariable(scrVarPub.pauseArrayId, parentId);
    if (pauseEntry) {
        unsigned int notifyListOwnerId = VG_U32(pauseEntry);
        unsigned int childId;

        while ((childId = FindNextSibling(notifyListOwnerId)) != 0) {
            unsigned int threadId = VG_NAME(childId) & 0xffff;
            unsigned int notifyEntry = FindObjectVariable(notifyListOwnerId, threadId);

            VM_CancelNotify(VG_U32(notifyEntry), threadId);
            Scr_KillEndonThread(threadId);
            RemoveObjectVariable(notifyListOwnerId, threadId);
        }

        RemoveObjectVariable(scrVarPub.pauseArrayId, parentId);
    }

    VG_STATUS(parentId) = (VG_STATUS(parentId) & ~SCRVL_VAR_MASK) | 0x17;
}
unsigned int Scr_EvalFieldObject(unsigned int tempVariable, VariableValue *value)
{
    int type = value->type;

    if (type == SCRVL_VAR_POINTER) {
        unsigned int objectId = value->u.pointerValue;
        int objectType = VG_STATUS(objectId) & SCRVL_VAR_MASK;

        if (objectType <= SCRVL_VAR_ENTITY) {
            VariableValue tempValue;

            tempValue.u.pointerValue = objectId;
            tempValue.type = SCRVL_VAR_POINTER;
            SetVariableValue(tempVariable, &tempValue);
            return objectId;
        }

        RemoveRefToObject(objectId);
        Com_Printf(va("%s is not a field object", var_typename[objectType]));
        return 0;
    }

    RemoveRefToValue(type, value->u);
    Com_Printf(va("%s is not a field object", var_typename[type]));
    return 0;
}

Bool Scr_CastString(VariableValue *value)
{
    int type = value->type;

    switch (type) {
    case SCRVL_VAR_STRING:
        return 1;
    case SCRVL_VAR_VECTOR: {
        const float *vectorValue = SCR_VEC_PTR(value->u);

        value->type = SCRVL_VAR_STRING;
        value->u.stringValue = SL_GetStringForVector(vectorValue);
        RemoveRefToVector_core(vectorValue);
        return 1;
    }
    case SCRVL_VAR_FLOAT:
        value->type = SCRVL_VAR_STRING;
        value->u.stringValue = SL_GetStringForFloat(value->u.floatValue);
        return 1;
    case SCRVL_VAR_INTEGER:
        value->type = SCRVL_VAR_STRING;
        value->u.stringValue = SL_GetStringForInt(value->u.intValue);
        return 1;
    default:
        scrVarPub.error_message = va("cannot cast %s to string", var_typename[type]);
        RemoveRefToValue_core(type, value->u);
        value->type = SCRVL_VAR_UNDEFINED;
        return 0;
    }
}

void Scr_CastDebugString(VariableValue *value)
{
    const char *debugString;
    VariableUnion oldValue;
    int oldType;
    unsigned int stringValue;

    switch (value->type) {
    case SCRVL_VAR_STRING:
    case SCRVL_VAR_VECTOR:
    case SCRVL_VAR_FLOAT:
    case SCRVL_VAR_INTEGER:
        Scr_CastString(value);
        return;
    case SCRVL_VAR_LOCALIZED_STRING:
        value->type = SCRVL_VAR_STRING;
        return;
    case SCRVL_VAR_POINTER:
        debugString = var_typename[VG_STATUS(value->u.pointerValue) & SCRVL_VAR_MASK];
        break;
    case 11:
        debugString = XAnimGetAnimDebugName(Scr_GetAnims(value->u.intValue >> 16), value->u.intValue & 0xffff);
        break;
    default:
        debugString = var_typename[value->type];
        break;
    }

    oldValue = value->u;
    oldType = value->type;
    stringValue = SL_GetString_(debugString, 0, 0xe);
    RemoveRefToValue(oldType, oldValue);
    value->type = SCRVL_VAR_STRING;
    value->u.stringValue = stringValue;
}

void Scr_UnmatchingTypesError(VariableValue *value1, VariableValue *value2)
{
    const char *error_message = 0;

    if (!scrVarPub.error_message) {
        int type1 = value1->type;
        int type2 = value2->type;

        Scr_CastDebugString(value1);
        Scr_CastDebugString(value2);
        error_message = va("pair '%s' and '%s' has unmatching types '%s' and '%s'",
                           SL_ConvertToString(value1->u.stringValue),
                           SL_ConvertToString(value2->u.stringValue),
                           var_typename[type1],
                           var_typename[type2]);
    }

    RemoveRefToValue(value1->type, value1->u);
    value1->type = SCRVL_VAR_UNDEFINED;
    RemoveRefToValue(value2->type, value2->u);
    value2->type = SCRVL_VAR_UNDEFINED;
    Scr_Error(error_message);
}

void Scr_EvalMod(VariableValue *value1, VariableValue *value2)
{
    int divisor;

    if (value1->type != SCRVL_VAR_INTEGER || value2->type != SCRVL_VAR_INTEGER) {
        Scr_UnmatchingTypesError(value1, value2);
        return;
    }

    divisor = value2->u.intValue;
    if (divisor == 0) {
        value1->u.intValue = 0;
        Scr_Error("divide by 0");
        return;
    }

    value1->u.intValue %= divisor;
}

void Scr_EvalDivide(VariableValue *value1, VariableValue *value2)
{
    if (value1->type == SCRVL_VAR_FLOAT && value2->type == SCRVL_VAR_INTEGER) {
        value2->type = SCRVL_VAR_FLOAT;
        value2->u.floatValue = (float)value2->u.intValue;
    } else if (value1->type == SCRVL_VAR_INTEGER && value2->type == SCRVL_VAR_FLOAT) {
        value1->type = SCRVL_VAR_FLOAT;
        value1->u.floatValue = (float)value1->u.intValue;
    } else if (value1->type != value2->type) {
        Scr_UnmatchingTypesError(value1, value2);
        return;
    }

    if (value1->type == SCRVL_VAR_FLOAT) {
        if (value2->u.floatValue == 0.0f) {
            value1->u.intValue = 0;
            Scr_Error("divide by 0");
            return;
        }

        value1->u.floatValue /= value2->u.floatValue;
        return;
    }

    if (value1->type == SCRVL_VAR_INTEGER) {
        int divisor = value2->u.intValue;

        value1->type = SCRVL_VAR_FLOAT;
        if (divisor == 0) {
            value1->u.intValue = 0;
            Scr_Error("divide by 0");
            return;
        }

        value1->u.floatValue = (float)value1->u.intValue / (float)divisor;
        return;
    }

    Scr_UnmatchingTypesError(value1, value2);
}

static qboolean Scr_CastPlusOperandToString(VariableValue *value)
{
    switch (value->type) {
    case SCRVL_VAR_STRING:
        return 1;
    case SCRVL_VAR_VECTOR: {
        const float *vectorValue = SCR_VEC_PTR(value->u);

        value->type = SCRVL_VAR_STRING;
        value->u.stringValue = SL_GetStringForVector(vectorValue);
        RemoveRefToVector(vectorValue);
        return 1;
    }
    case SCRVL_VAR_FLOAT:
        value->type = SCRVL_VAR_STRING;
        value->u.stringValue = SL_GetStringForFloat(value->u.floatValue);
        return 1;
    case SCRVL_VAR_INTEGER:
        value->type = SCRVL_VAR_STRING;
        value->u.stringValue = SL_GetStringForInt(value->u.intValue);
        return 1;
    default:
        return 0;
    }
}

void Scr_EvalPlus(VariableValue *value1, VariableValue *value2)
{
    if (value1->type != value2->type) {
        if (value1->type == SCRVL_VAR_STRING) {
            if (!Scr_CastPlusOperandToString(value2)) {
                Scr_UnmatchingTypesError(value1, value2);
                return;
            }
        } else if (value2->type == SCRVL_VAR_STRING) {
            if (!Scr_CastPlusOperandToString(value1)) {
                Scr_UnmatchingTypesError(value1, value2);
                return;
            }
        } else if (value1->type == SCRVL_VAR_FLOAT && value2->type == SCRVL_VAR_INTEGER) {
            value2->type = SCRVL_VAR_FLOAT;
            value2->u.floatValue = (float)value2->u.intValue;
        } else if (value1->type == SCRVL_VAR_INTEGER && value2->type == SCRVL_VAR_FLOAT) {
            value1->type = SCRVL_VAR_FLOAT;
            value1->u.floatValue = (float)value1->u.intValue;
        } else {
            Scr_UnmatchingTypesError(value1, value2);
            return;
        }
    }

    switch (value1->type) {
    case SCRVL_VAR_STRING: {
        unsigned int lhsString = value1->u.stringValue;
        unsigned int rhsString = value2->u.stringValue;
        const char *lhs = SL_ConvertToString(lhsString);
        const char *rhs = SL_ConvertToString(rhsString);
        int lhsLen = SL_GetStringLen(lhsString);
        int rhsLen = SL_GetStringLen(rhsString);
        int totalLen = lhsLen + rhsLen + 1;
        char tempString[0x2001];

        if (totalLen > 0x2000) {
            SL_RemoveRefToString(lhsString);
            SL_RemoveRefToString(rhsString);
            value1->type = SCRVL_VAR_UNDEFINED;
            value2->type = SCRVL_VAR_UNDEFINED;
            Scr_Error(va("cannot concat \"%s\" and \"%s\" - max string length exceeded", lhs, rhs));
            return;
        }

        strcpy(tempString, lhs);
        strcpy(&tempString[lhsLen], rhs);

        value1->u.stringValue = SL_GetStringOfLen(tempString, 0, (unsigned int)totalLen, 0xe);
        SL_RemoveRefToString(lhsString);
        SL_RemoveRefToString(rhsString);
        return;
    }
    case SCRVL_VAR_VECTOR: {
        float tempVector[3];
        const float *lhs = SCR_VEC_PTR(value1->u);
        const float *rhs = SCR_VEC_PTR(value2->u);
        const float *result;

        tempVector[0] = lhs[0] + rhs[0];
        tempVector[1] = lhs[1] + rhs[1];
        tempVector[2] = lhs[2] + rhs[2];

        result = Scr_AllocVector(tempVector);
        RemoveRefToVector(lhs);
        RemoveRefToVector(rhs);
        value1->u.vectorValue = SCR_VEC_ENC(result);
        return;
    }
    case SCRVL_VAR_FLOAT:
        value1->u.floatValue += value2->u.floatValue;
        return;
    case SCRVL_VAR_INTEGER:
        value1->u.intValue += value2->u.intValue;
        return;
    default:
        Scr_UnmatchingTypesError(value1, value2);
        return;
    }
}

void Scr_EvalShiftRight(VariableValue *value1, VariableValue *value2)
{
    if (value1->type != 6 || value2->type != 6) {
        Scr_UnmatchingTypesError(value1, value2);
        return;
    }

    value1->u.intValue >>= value2->u.intValue;
}

void Scr_EvalShiftLeft(VariableValue *value1, VariableValue *value2)
{
    if (value1->type != 6 || value2->type != 6) {
        Scr_UnmatchingTypesError(value1, value2);
        return;
    }

    value1->u.intValue = (int)((unsigned int)value1->u.intValue << value2->u.intValue);
}

void Scr_EvalAnd(VariableValue *value1, VariableValue *value2)
{
    if (value1->type != 6 || value2->type != 6) {
        Scr_UnmatchingTypesError(value1, value2);
        return;
    }

    value1->u.intValue &= value2->u.intValue;
}

void Scr_EvalExOr(VariableValue *value1, VariableValue *value2)
{
    if (value1->type != 6 || value2->type != 6) {
        Scr_UnmatchingTypesError(value1, value2);
        return;
    }

    value1->u.intValue ^= value2->u.intValue;
}

void Scr_EvalOr(VariableValue *value1, VariableValue *value2)
{
    if (value1->type != 6 || value2->type != 6) {
        Scr_UnmatchingTypesError(value1, value2);
        return;
    }

    value1->u.intValue |= value2->u.intValue;
}

static inline __attribute__((always_inline)) qboolean Scr_CastNumericPair(VariableValue *value1, VariableValue *value2)
{
    if (value1->type == value2->type)
        return 1;

    if (value1->type == SCRVL_VAR_FLOAT && value2->type == SCRVL_VAR_INTEGER) {
        value2->type = SCRVL_VAR_FLOAT;
        value2->u.floatValue = (float)value2->u.intValue;
        return 1;
    }

    if (value1->type == SCRVL_VAR_INTEGER && value2->type == SCRVL_VAR_FLOAT) {
        value1->type = SCRVL_VAR_FLOAT;
        value1->u.floatValue = (float)value1->u.intValue;
        return 1;
    }

    Scr_UnmatchingTypesError(value1, value2);
    return 0;
}

void Scr_EvalLessEqual(VariableValue *value1, VariableValue *value2)
{
    if (value1->type != value2->type) {
        if (value1->type == SCRVL_VAR_FLOAT && value2->type == SCRVL_VAR_INTEGER) {
            value2->type = SCRVL_VAR_FLOAT;
            value2->u.floatValue = (float)value2->u.intValue;
        } else if (value1->type == SCRVL_VAR_INTEGER && value2->type == SCRVL_VAR_FLOAT) {
            value1->type = SCRVL_VAR_FLOAT;
            value1->u.floatValue = (float)value1->u.intValue;
        } else {
            Scr_UnmatchingTypesError(value1, value2);
        }
    }

    if (value1->type == SCRVL_VAR_FLOAT) {
        float lhs = value1->u.floatValue;
        value1->type = SCRVL_VAR_INTEGER;
        value1->u.intValue = lhs > value2->u.floatValue;
    } else if (value1->type == SCRVL_VAR_INTEGER) {
        value1->u.intValue = value1->u.intValue > value2->u.intValue;
    } else {
        Scr_UnmatchingTypesError(value1, value2);
    }

    value1->u.intValue = !value1->u.intValue;
}

void Scr_EvalGreaterEqual(VariableValue *value1, VariableValue *value2)
{
    if (value1->type != value2->type) {
        if (value1->type == SCRVL_VAR_FLOAT && value2->type == SCRVL_VAR_INTEGER) {
            value2->type = SCRVL_VAR_FLOAT;
            value2->u.floatValue = (float)value2->u.intValue;
        } else if (value1->type == SCRVL_VAR_INTEGER && value2->type == SCRVL_VAR_FLOAT) {
            value1->type = SCRVL_VAR_FLOAT;
            value1->u.floatValue = (float)value1->u.intValue;
        } else {
            Scr_UnmatchingTypesError(value1, value2);
        }
    }

    if (value1->type == SCRVL_VAR_FLOAT) {
        float rhs = value2->u.floatValue;
        value1->type = SCRVL_VAR_INTEGER;
        value1->u.intValue = rhs > value1->u.floatValue;
    } else if (value1->type == SCRVL_VAR_INTEGER) {
        value1->u.intValue = value1->u.intValue < value2->u.intValue;
    } else {
        Scr_UnmatchingTypesError(value1, value2);
    }

    value1->u.intValue = !value1->u.intValue;
}

void Scr_EvalEquality(VariableValue *value1, VariableValue *value2)
{
    if (!Scr_CastNumericPair(value1, value2))
        return;

    switch (value1->type) {
    case SCRVL_VAR_UNDEFINED:
        value1->type = SCRVL_VAR_INTEGER;
        value1->u.intValue = 1;
        return;
    case SCRVL_VAR_POINTER: {
        unsigned int lhs = value1->u.pointerValue;
        unsigned int rhs = value2->u.pointerValue;
        int lhsIsArray = (VG_STATUS(lhs) & SCRVL_VAR_MASK) == SCRVL_VAR_ARRAY;
        int rhsIsArray = (VG_STATUS(rhs) & SCRVL_VAR_MASK) == SCRVL_VAR_ARRAY;
        int result;

        if ((lhsIsArray || rhsIsArray) && !scrVarPub.developer) {
            Scr_UnmatchingTypesError(value1, value2);
            return;
        }

        result = lhs == rhs;
        value1->type = SCRVL_VAR_INTEGER;
        RemoveRefToObject(lhs);
        RemoveRefToObject(rhs);
        value1->u.intValue = result;
        return;
    }
    case SCRVL_VAR_STRING:
    case SCRVL_VAR_LOCALIZED_STRING: {
        unsigned int lhs = value1->u.stringValue;
        unsigned int rhs = value2->u.stringValue;
        int result = lhs == rhs;

        value1->type = SCRVL_VAR_INTEGER;
        SL_RemoveRefToString(lhs);
        SL_RemoveRefToString(rhs);
        value1->u.intValue = result;
        return;
    }
    case SCRVL_VAR_VECTOR: {
        const float *lhs = SCR_VEC_PTR(value1->u);
        const float *rhs = SCR_VEC_PTR(value2->u);
        int result = lhs[0] == rhs[0] && lhs[1] == rhs[1] && lhs[2] == rhs[2];

        value1->type = SCRVL_VAR_INTEGER;
        RemoveRefToVector(lhs);
        RemoveRefToVector(rhs);
        value1->u.intValue = result;
        return;
    }
    case SCRVL_VAR_FLOAT: {
        float diff = value1->u.floatValue - value2->u.floatValue;

        if (diff < 0.0f)
            diff = -diff;

        value1->type = SCRVL_VAR_INTEGER;
        value1->u.intValue = 9.999999974752427e-07f > diff;
        return;
    }
    case SCRVL_VAR_INTEGER:
        value1->u.intValue = value1->u.intValue == value2->u.intValue;
        return;
    case 9:
    case 11:
        value1->type = SCRVL_VAR_INTEGER;
        value1->u.intValue = value1->u.intValue == value2->u.intValue;
        return;
    default:
        Scr_UnmatchingTypesError(value1, value2);
        return;
    }
}

void Scr_EvalInequality(VariableValue *value1, VariableValue *value2)
{
    Scr_EvalEquality(value1, value2);
    value1->u.intValue = (value1->u.intValue == 0);
}

void Scr_EvalLess(VariableValue *value1, VariableValue *value2)
{
    if (!Scr_CastNumericPair(value1, value2))
        return;

    if (value1->type == SCRVL_VAR_FLOAT) {
        float lhs = value1->u.floatValue;
        float rhs = value2->u.floatValue;
        value1->type = SCRVL_VAR_INTEGER;
        value1->u.intValue = lhs < rhs;
        return;
    }

    if (value1->type == SCRVL_VAR_INTEGER) {
        value1->u.intValue = value1->u.intValue < value2->u.intValue;
        return;
    }

    Scr_UnmatchingTypesError(value1, value2);
}

void Scr_EvalGreater(VariableValue *value1, VariableValue *value2)
{
    if (!Scr_CastNumericPair(value1, value2))
        return;

    if (value1->type == SCRVL_VAR_FLOAT) {
        float lhs = value1->u.floatValue;
        float rhs = value2->u.floatValue;
        value1->type = SCRVL_VAR_INTEGER;
        value1->u.intValue = lhs > rhs;
        return;
    }

    if (value1->type == SCRVL_VAR_INTEGER) {
        value1->u.intValue = value1->u.intValue > value2->u.intValue;
        return;
    }

    Scr_UnmatchingTypesError(value1, value2);
}

void Scr_EvalMinus(VariableValue *value1, VariableValue *value2)
{
    if (!Scr_CastNumericPair(value1, value2))
        return;

    if (value1->type == SCRVL_VAR_FLOAT) {
        value1->u.floatValue -= value2->u.floatValue;
        return;
    }

    if (value1->type == SCRVL_VAR_INTEGER) {
        value1->u.intValue -= value2->u.intValue;
        return;
    }

    if (value1->type == SCRVL_VAR_VECTOR) {
        float tempVector[3];
        const float *lhs = SCR_VEC_PTR(value1->u);
        const float *rhs = SCR_VEC_PTR(value2->u);
        const float *result;

        tempVector[0] = lhs[0] - rhs[0];
        tempVector[1] = lhs[1] - rhs[1];
        tempVector[2] = lhs[2] - rhs[2];

        result = Scr_AllocVector(tempVector);
        RemoveRefToVector(lhs);
        RemoveRefToVector(rhs);
        value1->u.vectorValue = SCR_VEC_ENC(result);
        return;
    }

    Scr_UnmatchingTypesError(value1, value2);
}

void Scr_EvalBinaryOperator(int op, VariableValue *value1, VariableValue *value2)
{
    switch (op) {
    case 0x66:
        Scr_EvalOr(value1, value2);
        break;
    case 0x67:
        Scr_EvalExOr(value1, value2);
        break;
    case 0x68:
        Scr_EvalAnd(value1, value2);
        break;
    case 0x69:
        Scr_EvalEquality(value1, value2);
        break;
    case 0x6a:
        Scr_EvalInequality(value1, value2);
        break;
    case 0x6b:
        Scr_EvalLess(value1, value2);
        break;
    case 0x6c:
        Scr_EvalGreater(value1, value2);
        break;
    case 0x6d:
        Scr_EvalLessEqual(value1, value2);
        break;
    case 0x6e:
        Scr_EvalGreaterEqual(value1, value2);
        break;
    case 0x6f:
        Scr_EvalShiftLeft(value1, value2);
        break;
    case 0x70:
        Scr_EvalShiftRight(value1, value2);
        break;
    case 0x71:
        Scr_EvalPlus(value1, value2);
        break;
    case 0x72:
        Scr_EvalMinus(value1, value2);
        break;
    case 0x73:
        Scr_EvalMultiply(value1, value2);
        break;
    case 0x74:
        Scr_EvalDivide(value1, value2);
        break;
    case 0x75:
        Scr_EvalMod(value1, value2);
        break;
    default:
        break;
    }
}

void Scr_EvalMultiply(VariableValue *value1, VariableValue *value2)
{
    if (!Scr_CastNumericPair(value1, value2))
        return;

    if (value1->type == SCRVL_VAR_FLOAT) {
        value1->u.floatValue *= value2->u.floatValue;
        return;
    }

    if (value1->type == SCRVL_VAR_INTEGER) {
        value1->u.intValue *= value2->u.intValue;
        return;
    }

    Scr_UnmatchingTypesError(value1, value2);
}

void Scr_CastBool(VariableValue *value)
{
    Scr_CastBool_core(value);
}

void Scr_EvalBoolComplement(VariableValue *value)
{
    int type = value->type;

    if (type == SCRVL_VAR_INTEGER) {
        value->u.intValue = ~value->u.intValue;
        return;
    }

    RemoveRefToValue(type, value->u);
    value->type = SCRVL_VAR_UNDEFINED;
    Scr_Error(va("~ cannot be applied to \"%s\"", var_typename[type]));
}

void Scr_EvalSizeValue(VariableValue *value)
{
    int type = value->type;

    if (type == SCRVL_VAR_POINTER) {
        unsigned int objectId = value->u.pointerValue;
        unsigned int size = 1;

        if ((VG_STATUS(objectId) & SCRVL_VAR_MASK) == SCRVL_VAR_ARRAY)
            size = VG_SIBLING(objectId);

        value->type = SCRVL_VAR_INTEGER;
        value->u.intValue = size;
        RemoveRefToObject(objectId);
        return;
    }

    if (type == SCRVL_VAR_STRING) {
        unsigned int stringValue = value->u.stringValue;

        value->type = SCRVL_VAR_INTEGER;
        value->u.intValue = strlen(SL_ConvertToString(stringValue));
        SL_RemoveRefToString(stringValue);
        return;
    }

    {
        const char *errorMessage = va("size cannot be applied to %s", var_typename[type]);

        RemoveRefToValue(type, value->u);
        value->type = SCRVL_VAR_UNDEFINED;
        Scr_Error(errorMessage);
    }
}

void Scr_CastVector(VariableValue *value)
{
    float vec[3];
    VariableValue *errStart = (VariableValue *)((byte *)value + 16);
    int *typePtr = (int *)((byte *)value + 0x14);
    float *dst = vec;
    int i = 0;
    int remaining = 3;
    int type;

    for (;;) {
        type = *typePtr;
        if (type == SCRVL_VAR_FLOAT) {
            *dst = *(float *)((byte *)typePtr - 4);
        } else if (type == SCRVL_VAR_INTEGER) {
            *dst = (float)*(int *)((byte *)typePtr - 4);
        } else {
            break;
        }
        ++i;
        ++dst;
        --remaining;
        typePtr = (int *)((byte *)typePtr - 8);
        if (i == 3) {
            value->type = SCRVL_VAR_VECTOR;
            value->u.vectorValue = SCR_VEC_ENC(Scr_AllocVector_core(&vec[0]));
            return;
        }
    }

    scrVarPub.error_index = remaining;
    {
        VariableValue *cur = errStart;
        int n = 3;
        do {
            RemoveRefToValue_core(cur->type, cur->u);
            cur = (VariableValue *)((byte *)cur - 8);
        } while (--n);
    }
    value->type = SCRVL_VAR_UNDEFINED;
    Scr_Error(va("type %s is not a float", var_typename[type]));
}

void FreeChildValue(unsigned int id)
{
    FreeChildValue_core(id);
}

void FreeValue(unsigned int id)
{
    VariableUnion value;
    unsigned int type;
    unsigned int index;
    unsigned int prevSiblingIndex;
    unsigned int nextSiblingIndex;
    unsigned int freeHead;

    type = VG_STATUS(id) & SCRVL_VAR_MASK;
    value.intValue = VG_U32(id);
    RemoveRefToValue_core(type, value);

    index = VG_NEXT(id);
    prevSiblingIndex = VG_PREV(index);
    nextSiblingIndex = VG_NEXT_SIBLING(id);

    VG_PREV(nextSiblingIndex) = (unsigned short)prevSiblingIndex;
    VG_NEXT_SIBLING(VG_ID(prevSiblingIndex)) = (unsigned short)nextSiblingIndex;

    VG_STATUS(id) = 0;
    freeHead = VG_U16(0);
    VG_U16(id) = (unsigned short)freeHead;
    VG_PREV(index) = 0;
    VG_PREV(freeHead) = (unsigned short)index;
    VG_U16(0) = (unsigned short)index;
}

void SetVariableValue(unsigned int id, VariableValue *value)
{
    SetVariableValue_core(id, value);
}

void ClearVariableValue(unsigned int id)
{
    unsigned int status = VG_STATUS(id);
    VariableUnion value;

    value.intValue = VG_U32(id);
    RemoveRefToValue(status & SCRVL_VAR_MASK, value);
    VG_STATUS(id) &= ~SCRVL_VAR_MASK;
}

void ClearArray(unsigned int parentId, VariableValue *value)
{
    unsigned int arrayId;
    unsigned int name;
    unsigned int type;

    if (parentId == 0xfffe) {
        arrayId = Scr_EvalArrayRef(parentId);
    } else {
        type = VG_STATUS(parentId) & SCRVL_VAR_MASK;
        if (type != SCRVL_VAR_POINTER) {
            scrVarPub.error_index = 1;
            Scr_Error(va("%s is not an array", var_typename[type]));
            return;
        }

        arrayId = VG_U32(parentId);
        if ((VG_STATUS(arrayId) & SCRVL_VAR_MASK) != SCRVL_VAR_ARRAY) {
            scrVarPub.error_index = 1;
            Scr_Error(va("%s is not an array",
                         var_typename[VG_STATUS(arrayId) & SCRVL_VAR_MASK]));
            return;
        }

        arrayId = Scr_CopyArrayForWrite(parentId, arrayId);
    }

    if (!arrayId)
        return;

    /* GSC assignment to undefined also clears absent array keys. */
    if (value->type == SCRVL_VAR_INTEGER) {
        if (!IsValidArrayIndex((unsigned int)value->u.intValue)) {
            Scr_Error(va("array index %d out of range", value->u.intValue));
            return;
        }

        name = GetInternalVariableIndex_core((unsigned int)value->u.intValue);
        SafeRemoveVariable(arrayId, name);
        return;
    }

    if (value->type == SCRVL_VAR_STRING) {
        name = value->u.stringValue;
        SafeRemoveVariable(arrayId, name);
        SL_RemoveRefToString(name);
        return;
    }

    Scr_Error(va("%s is not an array index", var_typename[value->type]));
}

scr_classStruct_t g_classMap[5] = {
    { 0x0, 0x0, 0x65, (const char *)&str_0021d304 },
    { 0x0, 0x0, 0x68, (const char *)&str_0021d324 },
    { 0x0, 0x0, 0x70, (const char *)&str_0021d32c },
    { 0x0, 0x0, 0x76, (const char *)&str_0021d338 },
    { 0x0, 0x0, 0x0, 0 }
};

const char str_0021d23c[] = "undefined";
const char str_0021d248[] = "object";
const char str_0021d250[] = "string";
const char str_0021d258[] = "localized string";
const char str_0021d26c[] = "vector";
const char str_0021d274[] = "float";
const char str_0021d27c[] = "int";
const char str_0021d280[] = "codepos";
const char str_0021d288[] = "precodepos";
const char str_0021d294[] = "function";
const char str_0021d2a0[] = "stack";
const char str_0021d2a8[] = "animation";
const char str_0021d2b4[] = "developer codepos";
const char str_0021d2c8[] = "include codepos";
const char str_0021d2d8[] = "thread list";
const char str_0021d2e4[] = "thread";
const char str_0021d2ec[] = "struct";
const char str_0021d2f4[] = "removed entity";
const char str_0021d304[] = "entity";
const char str_0021d30c[] = "array";
const char str_0021d314[] = "removed thread";
const char str_0021d324[] = "hudelem";
const char str_0021d32c[] = "pathnode";
const char str_0021d338[] = "vehiclenode";
