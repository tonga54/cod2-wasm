#include "common_types.h"
#include "imports.h"

extern void Com_Printf(const char *fmt, ...);
extern int MT_SearchTreeAny(int nodeNum);

enum {
    SCR_ANIMTREE_NAMES = 0,
    SCR_ANIMTREE_XANIM = 1,
    SCR_VAR_POINTER = 1,
    SCR_VAR_INTEGER = 6,
    SCR_VAR_CODEPOS = 7,
    SCR_SL_MAX_STRING_INDEX = 0x10000,
    SCR_MAX_XANIMTREE_NUM = 128,
    SCR_ANIM_FLAG_LOOPSYNC = 1,
    SCR_ANIM_FLAG_NONLOOPSYNC = 2,
    SCR_ANIM_FLAG_COMPLETE = 8,
};

extern struct scrAnimPub_t scrAnimPub;
extern struct scrVarPub_t scrVarPub;
extern struct scrParserPub_t scrParserPub;
extern struct scrAnimGlob_t scrAnimGlob;
static const char *propertyNames[3] = {
    "loopsync",
    "nonloopsync",
    "complete",
};

extern void *Hunk_AllocAlignInternal(int size, int align);

void SetAnimCheck(int bAnimCheck);
static void *Hunk_AllocXAnimTreePrecache(int size);
static inline __attribute__((always_inline)) void Scr_EmitAnimationInternal(char *pos, unsigned int animName, unsigned int names);
static int Scr_GetAnimTreeSize(unsigned int parentNode);
static void ConnectScriptToAnim(unsigned int names, int index, unsigned int filename, unsigned int name, int treeIndex);
struct XAnim_s *Scr_GetAnims(int index);
static int Scr_CreateAnimationTree(unsigned int parentNode, unsigned int names, struct XAnim_s *anims, unsigned int childIndex, const char *parentName, unsigned int parentIndex, unsigned int filename, int treeIndex);
static void Scr_CheckAnimsDefined(unsigned int names, unsigned int filename);
static void Scr_PrecacheAnimationTree(unsigned int parentNode);
static unsigned int Scr_UsingTreeInternal(const char *filename, int *index, int user);
static void AnimTreeCompileError(const char *msg);
static int GetAnimTreeParseProperties(void);
void Scr_UsingTree(const char *filename, unsigned int sourcePos);
struct scr_animtree_t Scr_FindAnimTree(const char *filename);
void Scr_FindAnim(const char *filename, const char *animName, scr_anim_t *anim, int user);
void Scr_EmitAnimation(char *pos, unsigned int animName, unsigned int sourcePos);
static Bool AnimTreeParseInternal(unsigned int parentNode, unsigned int names, Bool bIncludeParent, int bLoop, int bComplete);
static Bool Scr_LoadAnimTreeInternal(const char *filename, unsigned int parentNode, unsigned int names);
void Scr_LoadAnimTreeAtIndex(int index, Alloc_t Alloc, int user);

extern unsigned int FindNextSibling(unsigned int id);
extern unsigned int GetVariableName(unsigned int id);
extern int GetVarType(unsigned int id);
extern unsigned int FindObject(unsigned int varId);
extern unsigned int FindVariable(unsigned int parent, unsigned int name);
extern unsigned int GetNewVariable(unsigned int parent, unsigned int name);
extern unsigned int GetVariable(unsigned int parent, unsigned int name);
extern unsigned int GetObjectA(unsigned int varId);
extern unsigned int GetArray(unsigned int id);
extern unsigned int GetArraySize(unsigned int id);
extern unsigned int GetArrayVariable(unsigned int parentId, unsigned int unsignedValue);
extern unsigned int FindArrayVariable(unsigned int parentId, int intValue);
extern void RemoveVariable(unsigned int parentId, unsigned int unsignedValue);
extern void RemoveRefToObject(unsigned int objId);
extern void *GetVariableValueAddress(unsigned int id);
extern void SetVariableValue(unsigned int id, void *value);
extern void SL_RemoveRefToString(unsigned int stringValue);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern unsigned int SL_GetLowercaseString_(const char *str, unsigned int user, int type);
extern unsigned int SL_GetString_(const char *str, unsigned int user, int type);
extern unsigned int Scr_CreateCanonicalFilename(const char *filename);
extern unsigned long long Scr_EvalVariable(unsigned int varId);
extern int Scr_IsInOpcodeMemory(const char *pos);
extern Bool Scr_IsIdentifier(const char *token);
extern int Scr_AllocArray(void);
extern void *XAnimCreateAnims(const char *debugName, int size, Alloc_t Alloc);
extern void XAnimBlend(void *anims, unsigned int animIndex, const char *name, unsigned int children, unsigned int num, unsigned int flags);
extern XAnimParts *XAnimPrecache(const char *name, Alloc_t Alloc);
extern void XAnimCreate(void *anims, unsigned int animIndex, const char *name);
extern void XAnimSetupSyncNodes(void *anims);
extern byte *Scr_AddSourceBuffer(const char *filename, const char *extFilename, byte *oldFilename, int flag);
extern void Com_BeginParseSession(const char *name);
extern void Com_EndParseSession(void);
extern const char *Com_Parse(const char **buf);
extern const char *Com_ParseOnLine(const char **buf);
extern const char *Com_GetLastTokenPos(void);
extern void Hunk_ClearTempMemoryHigh(void);
extern int I_stricmp(const char *s1, const char *s2);
extern char *va(const char *format, ...);
extern void CompileError(unsigned int sourcePos, const char *fmt, ...);
extern void CompileError2(const char *codePos, const char *fmt, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern int sprintf(char *buf, const char *fmt, ...);

void SetAnimCheck(int bAnimCheck)
{
    scrAnimGlob.bAnimCheck = bAnimCheck;
}

static void *Hunk_AllocXAnimTreePrecache(int size)
{
    return Hunk_AllocAlignInternal(size, 4);
}

#if defined(_M_X64) || defined(__x86_64__)
/* x64: anim refs may live in the program buffer (script %anim, encodable as a 4-byte offset)
   OR in arbitrary C memory (Scr_FindAnim hands in scr_anim_t fields inside static bg_state
   structs at module addresses like 0x7ff6...). The 4-byte chain slot cannot hold a full 8-byte
   C pointer, so out-of-buffer positions are kept in a side table of full pointers, referenced
   by a bit-31-tagged index. (Not reset between levels; 16384 slots covers many map loads.) */
#define ANIM_REF_TAG 0x80000000u
static const char *g_animRefTable[16384];
static unsigned int g_animRefCount = 1;   /* index 0 reserved = end-of-chain */
static unsigned int AnimRef_Enc(const char *pos)
{
    const char *base = SCR_PROGBUF_BASE();
    uintptr_t off = (uintptr_t)(pos - base);
    if (base && off < 0x4000000u)
        return (unsigned int)off;             /* inside the program buffer -> offset */
    if (g_animRefCount < 16384) {
        unsigned int i = g_animRefCount++;
        g_animRefTable[i] = pos;
        return ANIM_REF_TAG | i;
    }
    return 0;
}
static const char *AnimRef_Dec(unsigned int enc)
{
    if (!enc)
        return (const char *)0;
    if (enc & ANIM_REF_TAG)
        return g_animRefTable[enc & ~ANIM_REF_TAG];
    return SCR_PROGBUF_BASE() + enc;
}
#endif

static inline __attribute__((always_inline)) void Scr_EmitAnimationInternal(char *pos, unsigned int animName, unsigned int names)
{
    VariableUnion *value;
    unsigned int animId;
    VariableValue tempValue;

    animId = FindVariable(names, animName);
    if (!animId) {
        animId = GetNewVariable(names, animName);
        /* The anim-ref chain slot is only 4 bytes (scr_anim_t / TempMallocAlign(4)); store the
           link as the 4-byte encoded code position (program-buffer offset or side-table index
           on x64), never an 8-byte raw pointer. */
#if defined(_M_X64) || defined(__x86_64__)
        *(unsigned int *)pos = 0;
        tempValue.type = SCR_VAR_CODEPOS;
        tempValue.u.codePosValue = AnimRef_Enc(pos);
#else
        *(const char **)pos = 0;
        tempValue.type = SCR_VAR_CODEPOS;
        SCR_CODEPOS_SET(tempValue.u, pos);
#endif
        SetVariableValue(animId, &tempValue);
        return;
    }

    value = (VariableUnion *)GetVariableValueAddress(animId);
#if defined(_M_X64) || defined(__x86_64__)
    *(unsigned int *)pos = value->codePosValue;   /* link = previous head (encoded) */
    value->codePosValue = AnimRef_Enc(pos);       /* new head */
#else
    *(const char **)pos = SCR_CODEPOS_GET(*value);
    SCR_CODEPOS_SET(*value, pos);
#endif
}

static int Scr_GetAnimTreeSize(unsigned int parentNode)
{
    unsigned int node;
    int size;

    size = 0;
    for (node = FindNextSibling(parentNode); node; node = FindNextSibling(node)) {
        if (GetVariableName(node) >= SCR_SL_MAX_STRING_INDEX) {
            continue;
        }
        if (GetVarType(node) == SCR_VAR_POINTER) {
            size += Scr_GetAnimTreeSize(FindObject(node));
            continue;
        }
        size++;
    }

    if (size) {
        size++;
    }
    return size;
}

static void ConnectScriptToAnim(unsigned int names, int index, unsigned int filename, unsigned int name, int treeIndex)
{
    VariableUnion *value;
    unsigned int animId;
    const char *codePos;
    const char *nextCodePos;
    unsigned short idx;
    unsigned short tree;

    animId = FindVariable(names, name);
    if (!animId) {
        return;
    }

    value = (VariableUnion *)GetVariableValueAddress(animId);
    if (!value->codePosValue) {
        Com_Error(1, "\x15"
                     "duplicate animation '%s' in 'animtrees/%s.atr'",
                  SL_ConvertToString(name), SL_ConvertToString(filename));
    }

    idx = (unsigned short)index;
    tree = (unsigned short)treeIndex;
#if defined(_M_X64) || defined(__x86_64__)
    codePos = AnimRef_Dec(value->codePosValue);
#else
    codePos = SCR_CODEPOS_GET(*value);
#endif
    for (; codePos; codePos = nextCodePos) {
#if defined(_M_X64) || defined(__x86_64__)
        /* link stored as a 4-byte encoded code position (see Scr_EmitAnimationInternal) */
        nextCodePos = AnimRef_Dec(*(const unsigned int *)codePos);
#else
        nextCodePos = *(const char *const *)codePos;
#endif
        ((scr_anim_t *)codePos)->tree = tree;
        ((scr_anim_t *)codePos)->index = idx;
    }

    value->codePosValue = 0;
}

struct XAnim_s *Scr_GetAnims(int index)
{
    return *(struct XAnim_s **)(0x114e22c + index * 4);
}

static int Scr_CreateAnimationTree(unsigned int parentNode, unsigned int names, struct XAnim_s *anims, unsigned int childIndex, const char *parentName, unsigned int parentIndex, unsigned int filename, int treeIndex)
{
    unsigned int flagsId;
    unsigned int nodeRef;
    unsigned int name;
    unsigned int size;
    unsigned short flags;
    unsigned int checksum;
#define SCRVP (((struct scrVarPub_t *)imp_scrVarPub))

    size = 0;
    for (nodeRef = FindNextSibling(parentNode); nodeRef; nodeRef = FindNextSibling(nodeRef)) {
        if (GetVariableName(nodeRef) < SCR_SL_MAX_STRING_INDEX) {
            size++;
        }
    }

    flags = 0;
    flagsId = FindArrayVariable(parentNode, 0);
    if (flagsId) {
        flags = (unsigned short)((VariableUnion *)GetVariableValueAddress(flagsId))->intValue;
    }

    checksum = SCRVP->checksum;
    checksum = checksum * 31 + parentIndex;
    checksum = checksum * 31 + childIndex;
    checksum = checksum * 31 + size;
    checksum = checksum * 31 + flags;
    SCRVP->checksum = checksum;

    XAnimBlend(anims, parentIndex, parentName, childIndex, size, flags);

    parentIndex = childIndex;
    childIndex += size;

    for (nodeRef = FindNextSibling(parentNode); nodeRef; nodeRef = FindNextSibling(nodeRef)) {
        name = GetVariableName(nodeRef);
        if (name >= SCR_SL_MAX_STRING_INDEX) {
            continue;
        }

        ConnectScriptToAnim(names, parentIndex, filename, name, treeIndex);

        if (GetVarType(nodeRef) == SCR_VAR_POINTER) {
            childIndex = Scr_CreateAnimationTree(
                FindObject(nodeRef), names, anims, childIndex,
                SL_ConvertToString(name), parentIndex, filename, treeIndex);
        } else {
            SCRVP->checksum = SCRVP->checksum * 31 + parentIndex;
            XAnimCreate(anims, parentIndex, SL_ConvertToString(name));
        }

        parentIndex++;
    }

    return (int)childIndex;
#undef SCRVP
}

static void Scr_CheckAnimsDefined(unsigned int names, unsigned int filename)
{
    VariableUnion *value;
    unsigned int animId;
    unsigned int name;
    char *msg;

    for (animId = FindNextSibling(names); animId; animId = FindNextSibling(animId)) {
        name = GetVariableName(animId);
        value = (VariableUnion *)GetVariableValueAddress(animId);
        if (!value->codePosValue) {
            continue;
        }

        msg = va("animation '%s' not defined in anim tree '%s'",
                 SL_ConvertToString(name), SL_ConvertToString(filename));
        if (Scr_IsInOpcodeMemory(SCR_CODEPOS_GET(*value))) {
            CompileError2(SCR_CODEPOS_GET(*value), "%s", msg);
            continue;
        }
        Com_Error(1, "%s", msg);
    }
}

static void Scr_PrecacheAnimationTree(unsigned int parentNode)
{
    unsigned int node;
    unsigned int name;

    for (node = FindNextSibling(parentNode); node; node = FindNextSibling(node)) {
        name = GetVariableName(node);
        if (name >= SCR_SL_MAX_STRING_INDEX) {
            continue;
        }
        if (GetVarType(node) == SCR_VAR_POINTER) {
            Scr_PrecacheAnimationTree(FindObject(node));
            continue;
        }
        XAnimPrecache(SL_ConvertToString(name), Hunk_AllocXAnimTreePrecache);
    }
}

static unsigned int Scr_UsingTreeInternal(const char *filename, int *index, int user)
{
    unsigned int id;
    unsigned int fileId;
    unsigned int names;
    unsigned int name;
    int i;

    name = Scr_CreateCanonicalFilename(filename);
    id = FindVariable(scrAnimPub.animtrees, name);

    if (id) {
        fileId = FindObject(id);
        *index = 0;

        for (i = 1; i <= scrAnimPub.xanim_num[user]; i++) {
            if (scrAnimGlob.using_xanim_lookup[user][i] == id) {
                *index = i;
                break;
            }
        }
    } else {
        id = GetNewVariable(scrAnimPub.animtrees, name);
        fileId = GetObjectA(id);

        scrAnimPub.xanim_num[user]++;
        if (scrAnimPub.xanim_num[user] >= SCR_MAX_XANIMTREE_NUM) {
            Com_Error(1, "too many xanim trees");
            scrAnimPub.xanim_num[user] = SCR_MAX_XANIMTREE_NUM - 1;
        }

        scrAnimGlob.using_xanim_lookup[user][scrAnimPub.xanim_num[user]] =
            (unsigned short)id;
        *index = scrAnimPub.xanim_num[user];
    }

    names = GetArray(GetVariable(fileId, SCR_ANIMTREE_NAMES));
    SL_RemoveRefToString(name);
    return names;
}

static void AnimTreeCompileError(const char *msg)
{
    const char *pos;

    pos = Com_GetLastTokenPos();
    Com_EndParseSession();
    CompileError((unsigned int)(pos - scrAnimGlob.start), "%s", msg);
}

static int GetAnimTreeParseProperties(void)
{
    const char *token;
    int flags;
    int i;

    flags = 0;
    while (1) {
        token = Com_ParseOnLine(&scrAnimGlob.pos);
        if (!token[0]) {
            return flags;
        }

        for (i = 0; i < 3; i++) {
            if (!I_stricmp(token, propertyNames[i])) {
                break;
            }
        }

        switch (i) {
        case 0:
            flags |= SCR_ANIM_FLAG_LOOPSYNC;
            break;
        case 1:
            flags |= SCR_ANIM_FLAG_NONLOOPSYNC;
            break;
        case 2:
            flags |= SCR_ANIM_FLAG_COMPLETE;
            break;
        default:
            AnimTreeCompileError("unknown anim property");
            break;
        }
    }
}

void Scr_UsingTree(const char *filename, unsigned int sourcePos)
{
    if (!Scr_IsIdentifier(filename)) {
        CompileError(sourcePos, "bad anim tree name");
        return;
    }

    scrAnimPub.animTreeNames =
        Scr_UsingTreeInternal(filename, &scrAnimPub.animTreeIndex, 1);
}

struct scr_animtree_t Scr_FindAnimTree(const char *filename)
{
    struct scr_animtree_t result;
    unsigned int filenameId;
    unsigned int fileId;

    filenameId = Scr_CreateCanonicalFilename(filename);
    fileId = FindVariable(scrAnimPub.animtrees, filenameId);
    SL_RemoveRefToString(filenameId);

    result.anims = 0;
    if (!fileId)
        return result;

    GetVariableName(fileId);
    fileId = FindObject(fileId);
    if (!FindVariable(fileId, 1))
        return result;

#if defined(_M_X64) || defined(__x86_64__)
    /* The XAnim* was stored with SCR_CODEPOS_SET (a program-buffer-relative offset) at
       Scr_LoadAnimTreeAtIndex; decode it back to a real pointer. Reading the raw value would
       hand back the offset (e.g. 0xce60) as a pointer -> crash in XAnimGetAnimTreeSize. */
    result.anims = (struct XAnim_s *)(uintptr_t)SCR_CODEPOS_PTR((unsigned int)Scr_EvalVariable(FindVariable(fileId, 1)));
#else
    result.anims = (struct XAnim_s *)(uintptr_t)Scr_EvalVariable(FindVariable(fileId, 1));
#endif
    return result;
}

void *Scr_FindAnimTree_asm(const char *filename)
{
    struct scr_animtree_t result = Scr_FindAnimTree(filename);
    return (void *)result.anims;
}

void Scr_FindAnim(const char *filename, const char *animName, scr_anim_t *anim, int user)
{
    int index;
    unsigned int name;

    name = SL_GetLowercaseString_(animName, 0, 4);
    Scr_EmitAnimationInternal(
        (char *)anim, name, Scr_UsingTreeInternal(filename, &index, user));
    SL_RemoveRefToString(name);
}

void Scr_EmitAnimation(char *pos, unsigned int animName, unsigned int sourcePos)
{
    if (!scrAnimPub.animTreeNames) {
        CompileError(sourcePos, "#using_animtree was not specified");
        return;
    }

    Scr_EmitAnimationInternal(pos, animName, scrAnimPub.animTreeNames);
}

static Bool AnimTreeParseInternal(unsigned int parentNode, unsigned int names, Bool bIncludeParent, int bLoop, int bComplete)
{
    VariableValue tempValue;
    unsigned int index;
    unsigned int currentAnim;
    unsigned int node;
    const char *token;
    int flags;
    Bool bIgnore;

    tempValue.type = SCR_VAR_INTEGER;
    index = 0;
    currentAnim = 0;
    flags = 0;
    bIgnore = 0;

    while (1) {
        token = Com_Parse(&scrAnimGlob.pos);

        if (!scrAnimGlob.pos) {
            if (bIgnore) {
                RemoveVariable(parentNode, index);
            }

            if (bIncludeParent && !GetArraySize(parentNode)) {
                index = SL_GetString_(bLoop ? "void_loop" : "void", 0, 4);
                GetVariable(parentNode, index);
                SL_RemoveRefToString(index);
            }

            return 1;
        }

        if (Scr_IsIdentifier(token)) {
            if (bIgnore) {
                RemoveVariable(parentNode, index);
            }

            index = SL_GetLowercaseString_(token, 2, 4);
            if (FindVariable(parentNode, index)) {
                AnimTreeCompileError("duplicate animation");
            }

            currentAnim = GetVariable(parentNode, index);
            bIgnore = 0;

            if (!bComplete && !FindVariable(names, index)) {
                bIgnore = (scrAnimGlob.bAnimCheck == 0);
            }

            flags = 0;
            token = Com_ParseOnLine(&scrAnimGlob.pos);
            if (!token[0]) {
                continue;
            }

            if (Scr_IsIdentifier(token)) {
                AnimTreeCompileError("FIXME: aliases not yet implemented");
            }
            if (token[0] != ':' || token[1]) {
                AnimTreeCompileError("bad token");
            }

            flags = GetAnimTreeParseProperties();
            token = Com_Parse(&scrAnimGlob.pos);
            if (token[0] != '{' || token[1]) {
                AnimTreeCompileError("properties cannot be applied to primitive animations");
            }
        }

        if (token[0] == '{') {
            if (token[1]) {
                AnimTreeCompileError("bad token");
            }
            if (*Com_ParseOnLine(&scrAnimGlob.pos)) {
                AnimTreeCompileError("token not allowed after '{'");
            }
            if (!currentAnim) {
                AnimTreeCompileError("no animation specified for this block");
            }

            node = GetArray(currentAnim);
            if (bComplete || ((flags & SCR_ANIM_FLAG_COMPLETE) && !bIgnore)) {
                if (AnimTreeParseInternal(node, names, (Bool)!bIgnore,
                                          flags & SCR_ANIM_FLAG_LOOPSYNC, 1)) {
                    AnimTreeCompileError("unexpected end of file");
                }
            } else {
                if (AnimTreeParseInternal(node, names, (Bool)!bIgnore,
                                          flags & SCR_ANIM_FLAG_LOOPSYNC, 0)) {
                    AnimTreeCompileError("unexpected end of file");
                }
            }

            if (GetArraySize(node)) {
                tempValue.u.intValue = flags;
                SetVariableValue(GetArrayVariable(node, 0), &tempValue);
            } else {
                RemoveVariable(parentNode, index);
            }

            currentAnim = 0;
            bIgnore = 0;
            continue;
        }

        if (token[0] != '}') {
            AnimTreeCompileError("bad token");
            continue;
        }

        if (token[1]) {
            AnimTreeCompileError("bad token");
        }
        if (*Com_ParseOnLine(&scrAnimGlob.pos)) {
            AnimTreeCompileError("token not allowed after '}'");
        }

        if (bIgnore) {
            RemoveVariable(parentNode, index);
        }

        if (bIncludeParent && !GetArraySize(parentNode)) {
            index = SL_GetString_(bLoop ? "void_loop" : "void", 0, 4);
            GetVariable(parentNode, index);
            SL_RemoveRefToString(index);
        }

        return 0;
    }
}

static Bool Scr_LoadAnimTreeInternal(const char *filename, unsigned int parentNode, unsigned int names)
{
    const char *oldSourceBuf;
    const char *oldFilename;
    const char *sourceBuffer;
    const char *errorPos;
    char extFilename[96];

    sprintf(extFilename, "animtrees/%s.atr", filename);
    oldSourceBuf = scrParserPub.sourceBuf;
    oldFilename = scrParserPub.scriptfilename;

    sourceBuffer = (const char *)Scr_AddSourceBuffer(0, extFilename, 0, 1);
    if (!sourceBuffer) {
        return 0;
    }

    scrParserPub.scriptfilename = extFilename;
    Com_BeginParseSession("Scr_AnimTreeParse");
    scrAnimGlob.start = sourceBuffer;
    scrAnimGlob.pos = sourceBuffer;

    if (!AnimTreeParseInternal(parentNode, names, 1, 0, 0)) {
        errorPos = Com_GetLastTokenPos();
        Com_EndParseSession();
        CompileError((unsigned int)(errorPos - scrAnimGlob.start), "%s", "bad token");
    } else {
        Com_EndParseSession();
    }

    scrParserPub.scriptfilename = oldFilename;
    scrParserPub.sourceBuf = oldSourceBuf;
    Hunk_ClearTempMemoryHigh();

    return (Bool)(GetArraySize(parentNode) != 0);
}

void Scr_LoadAnimTreeAtIndex(int index, Alloc_t Alloc, int user)
{
    unsigned int id;
    unsigned int fileId;
    unsigned int filenameId;
    unsigned int namesVar;
    unsigned int names;
    unsigned int name;
    int size;
    int size2;
    scr_animtree_t animtree;
    VariableValue tempValue;

    id = scrAnimGlob.using_xanim_lookup[user][index];
    filenameId = GetVariableName(id) & 0xffff;
    fileId = FindObject(id);

    if (FindVariable(fileId, SCR_ANIMTREE_XANIM)) {
        return;
    }

    namesVar = FindVariable(fileId, SCR_ANIMTREE_NAMES);
    if (!namesVar) {
        scrAnimPub.xanim_lookup[user][index].anims = 0;
        return;
    }

    animtree.anims = 0;
    names = FindObject(namesVar);
    scrAnimPub.animtree_node = Scr_AllocArray();

    if (!Scr_LoadAnimTreeInternal(SL_ConvertToString(filenameId),
                                  scrAnimPub.animtree_node, names)) {
        Com_Error(1, "unknown anim tree '%s'", SL_ConvertToString(filenameId));
    }

    size = Scr_GetAnimTreeSize(scrAnimPub.animtree_node);
    animtree.anims =
        (struct XAnim_s *)XAnimCreateAnims(SL_ConvertToString(filenameId), size, Alloc);

    name = SL_GetString_("root", 0, 4);
    ConnectScriptToAnim(names, 0, filenameId, name, index);
    SL_RemoveRefToString(name);

    Scr_PrecacheAnimationTree(scrAnimPub.animtree_node);
    size2 = Scr_CreateAnimationTree(scrAnimPub.animtree_node, names, animtree.anims,
                                    1, "root", 0, filenameId, index);
    (void)size2;

    Scr_CheckAnimsDefined(names, filenameId);

    RemoveVariable(fileId, SCR_ANIMTREE_NAMES);
    RemoveRefToObject(scrAnimPub.animtree_node);
    scrAnimPub.animtree_node = 0;

    SCR_CODEPOS_SET(tempValue.u, (const char *)animtree.anims);
    tempValue.type = SCR_VAR_CODEPOS;
    SetVariableValue(GetVariable(fileId, SCR_ANIMTREE_XANIM), &tempValue);

    XAnimSetupSyncNodes(animtree.anims);
    scrAnimPub.xanim_lookup[user][index] = animtree;
}
