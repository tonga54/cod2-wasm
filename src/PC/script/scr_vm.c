#include "common_types.h"
#include "imports.h"
#include "scr_debugger.h"

extern char *getenv(const char *name);

#if COD2_FEATURE_SCRIPT_DEBUGGER

unsigned char *Scr_GetProgramBuffer(void)
{
    return (unsigned char *)((struct scrVarPub_t *)imp_scrVarPub)->programBuffer;
}
unsigned int Scr_GetProgramLen(void)
{
    return (unsigned int)((struct scrCompilePub_t *)imp_scrCompilePub)->programLen;
}
#endif

extern struct scrVmPub_t scrVmPub;
extern char g_EndPos;
extern jmp_buf g_script_error[33];
extern int g_script_error_level;
extern unsigned char scrVmGlob[];

#ifndef _WIN32
extern void longjmp(jmp_buf env, int val);
extern int setjmp(jmp_buf env);
#endif
extern int ___tolower(int c);

COD2_ASSERT_FIELD(scrVmPub_t, top,           0x10);
COD2_ASSERT_FIELD(scrVmPub_t, abort_on_error, 0x15);
COD2_ASSERT_FIELD(scrVarPub_t, pauseArrayId, 0x20);

#define VAR_UNDEFINED 0
#define VAR_POINTER 1
#define VAR_STRING 2
#define VAR_LOCALIZED_STRING 3
#define VAR_VECTOR 4
#define VAR_FLOAT 5
#define VAR_INTEGER 6

typedef enum ScrVmOpcode_e {
    VMOP_End = 0x00,
    VMOP_Return = 0x01,
    VMOP_GetUndefined = 0x02,
    VMOP_GetZero = 0x03,
    VMOP_GetByte = 0x04,
    VMOP_GetNegByte = 0x05,
    VMOP_GetUnsignedShort = 0x06,
    VMOP_GetNegUnsignedShort = 0x07,
    VMOP_GetInteger = 0x08,
    VMOP_GetFloat = 0x09,
    VMOP_GetString = 0x0a,
    VMOP_GetIString = 0x0b,
    VMOP_GetVector = 0x0c,
    VMOP_GetLevelObject = 0x0d,
    VMOP_GetAnimObject = 0x0e,
    VMOP_GetSelf = 0x0f,
    VMOP_GetLevel = 0x10,
    VMOP_GetGame = 0x11,
    VMOP_GetAnim = 0x12,
    VMOP_GetAnimation = 0x13,
    VMOP_GetGameRef = 0x14,
    VMOP_GetFunction = 0x15,
    VMOP_CreateLocalVariable = 0x16,
    VMOP_RemoveLocalVariables = 0x17,
    VMOP_EvalLocalVariableCached0 = 0x18,
    VMOP_EvalLocalVariableCached1 = 0x19,
    VMOP_EvalLocalVariableCached2 = 0x1a,
    VMOP_EvalLocalVariableCached3 = 0x1b,
    VMOP_EvalLocalVariableCached4 = 0x1c,
    VMOP_EvalLocalVariableCached5 = 0x1d,
    VMOP_EvalLocalVariable = 0x1e,
    VMOP_EvalLocalArrayCached = 0x1f,
    VMOP_EvalArray = 0x20,
    VMOP_EvalLocalVariableRefCached = 0x21,
    VMOP_EvalArrayRefCached = 0x22,
    VMOP_EvalArrayRef = 0x23,
    VMOP_ClearArray = 0x24,
    VMOP_EmptyArray = 0x25,
    VMOP_IsDefinedSelfField = 0x26,
    VMOP_IsDefinedLevelField = 0x27,
    VMOP_IsDefinedAnimField = 0x28,
    VMOP_EvalSelfFieldVariable = 0x29,
    VMOP_EvalFieldVariable = 0x2a,
    VMOP_EvalLevelFieldVariable = 0x2b,
    VMOP_EvalAnimFieldVariable = 0x2c,
    VMOP_EvalSelfFieldVariableRef = 0x2d,
    VMOP_EvalFieldVariableRef = 0x2e,
    VMOP_ClearFieldVariable = 0x2f,
    VMOP_SafeCreateLocalVariables = 0x30,
    VMOP_ClearParams = 0x31,
    VMOP_CheckClearParams = 0x32,
    VMOP_EvalLocalVariableRefCached0 = 0x33,
    VMOP_CheckClearParams2 = 0x34,
    VMOP_EndOn = 0x35,
    VMOP_Notify = 0x36,
    VMOP_EvalLocalVariableObject = 0x37,
    VMOP_SetLevelFieldVariableField = 0x38,
    VMOP_EvalLocalArrayRefCached0 = 0x39,
    VMOP_SetAnimFieldVariableField = 0x3a,
    VMOP_SetSelfFieldVariableField = 0x3b,
    VMOP_SetLocalVariableFieldCached = 0x3c,
    VMOP_SetLocalVariableField = 0x3d,
    VMOP_CallBuiltin0 = 0x3e,
    VMOP_CallBuiltin1 = 0x3f,
    VMOP_CallBuiltin2 = 0x40,
    VMOP_CallBuiltin3 = 0x41,
    VMOP_CallBuiltin4 = 0x42,
    VMOP_CallBuiltin5 = 0x43,
    VMOP_CallBuiltin = 0x44,
    VMOP_CallBuiltinMethod0 = 0x45,
    VMOP_CallBuiltinMethod1 = 0x46,
    VMOP_CallBuiltinMethod2 = 0x47,
    VMOP_CallBuiltinMethod3 = 0x48,
    VMOP_CallBuiltinMethod4 = 0x49,
    VMOP_CallBuiltinMethod5 = 0x4a,
    VMOP_CallBuiltinMethod = 0x4b,
    VMOP_Wait = 0x4c,
    VMOP_WaitTillFrameEnd = 0x4d,
    VMOP_PreScriptCall = 0x4e,
    VMOP_ScriptFunctionCallPointer = 0x4f,
    VMOP_ScriptFunctionCall = 0x50,
    VMOP_ScriptFunctionCallExpr = 0x51,
    VMOP_ScriptMethodCall = 0x52,
    VMOP_ScriptMethodCallExpr = 0x53,
    VMOP_ScriptThreadCall = 0x54,
    VMOP_ScriptThreadCallExpr = 0x55,
    VMOP_ScriptMethodThreadCall = 0x56,
    VMOP_ScriptMethodThreadCallExpr = 0x57,
    VMOP_DecTop = 0x58,
    VMOP_EvalFieldObject = 0x59,
    VMOP_EvalLocalVariableObjectCached = 0x5a,
    VMOP_CastBool = 0x5b,
    VMOP_BoolNot = 0x5c,
    VMOP_BoolComplement = 0x5d,
    VMOP_JumpOnFalse = 0x5e,
    VMOP_JumpOnTrue = 0x5f,
    VMOP_JumpOnFalseExpr = 0x60,
    VMOP_JumpOnTrueExpr = 0x61,
    VMOP_Jump = 0x62,
    VMOP_JumpBack = 0x63,
    VMOP_Inc = 0x64,
    VMOP_Dec = 0x65,
    VMOP_Bit_Or = 0x66,
    VMOP_Bit_Xor = 0x67,
    VMOP_Bit_And = 0x68,
    VMOP_Equal = 0x69,
    VMOP_NotEqual = 0x6a,
    VMOP_LessThan = 0x6b,
    VMOP_GreaterThan = 0x6c,
    VMOP_LessEqual = 0x6d,
    VMOP_GreaterEqual = 0x6e,
    VMOP_ShiftLeft = 0x6f,
    VMOP_ShiftRight = 0x70,
    VMOP_Plus = 0x71,
    VMOP_Minus = 0x72,
    VMOP_Multiply = 0x73,
    VMOP_Divide = 0x74,
    VMOP_Mod = 0x75,
    VMOP_SizeOf = 0x76,
    VMOP_WaitTillMatch = 0x77,
    VMOP_WaitTill = 0x78,
    VMOP_NotifyCall = 0x79,
    VMOP_EndOnCallback = 0x7a,
    VMOP_VoidCodepos = 0x7b,
    VMOP_Switch = 0x7c,
    VMOP_EndSwitch = 0x7d,
    VMOP_Vector = 0x7e,
    VMOP_Nop = 0x7f,
    VMOP_Abort = 0x80,
    VMOP_Object = 0x81,
    VMOP_ThreadObject = 0x82,
    VMOP_EvalLevelFieldVariableRef = 0x83,
    VMOP_EvalAnimFieldVariableRef = 0x84,
    VMOP_DevblockBegin = 0x85,
    VMOP_DevblockEnd = 0x86
} ScrVmOpcode;

static const char *VM_OpcodeName(unsigned int opcode)
{
    static const char *const names[0x87] = {
        [0x00] = "End", [0x01] = "Return", [0x02] = "GetUndefined", [0x03] = "GetZero", [0x04] = "GetByte", [0x05] = "GetNegByte", [0x06] = "GetUnsignedShort", [0x07] = "GetNegUnsignedShort", [0x08] = "GetInteger", [0x09] = "GetFloat", [0x0a] = "GetString", [0x0b] = "GetIString", [0x0c] = "GetVector", [0x0d] = "GetLevelObject", [0x0e] = "GetAnimObject", [0x0f] = "GetSelf", [0x10] = "GetLevel", [0x11] = "GetGame", [0x12] = "GetAnim", [0x13] = "GetAnimation", [0x14] = "GetGameRef", [0x15] = "GetFunction", [0x16] = "CreateLocalVariable", [0x17] = "RemoveLocalVariables", [0x18] = "EvalLocalVariableCached0", [0x19] = "EvalLocalVariableCached1", [0x1a] = "EvalLocalVariableCached2", [0x1b] = "EvalLocalVariableCached3", [0x1c] = "EvalLocalVariableCached4", [0x1d] = "EvalLocalVariableCached5", [0x1e] = "EvalLocalVariable", [0x1f] = "EvalLocalArrayCached", [0x20] = "EvalArray", [0x21] = "EvalLocalVariableRefCached", [0x22] = "EvalArrayRefCached", [0x23] = "SetVariableField", [0x24] = "ClearArray", [0x25] = "EmptyArray", [0x26] = "IsDefinedSelfField", [0x27] = "IsDefinedLevelField", [0x28] = "IsDefinedAnimField", [0x29] = "EvalSelfFieldVariable", [0x2a] = "EvalFieldVariable", [0x2b] = "EvalLevelFieldVariable", [0x2c] = "EvalAnimFieldVariable", [0x2d] = "EvalSelfFieldVariableRef", [0x2e] = "EvalFieldVariableRef", [0x2f] = "ClearFieldVariable", [0x30] = "SafeCreateLocalVariables", [0x31] = "ClearParams", [0x32] = "CheckClearParams", [0x33] = "EvalLocalVariableRefCached0", [0x34] = "CheckClearParams2", [0x35] = "EndOn", [0x36] = "Notify", [0x37] = "EvalLocalVariableObject", [0x38] = "SetLevelFieldVariableField", [0x39] = "EvalLocalArrayRefCached0", [0x3a] = "SetAnimFieldVariableField", [0x3b] = "SetSelfFieldVariableField", [0x3c] = "SetLocalVariableFieldCached", [0x3d] = "SetLocalVariableField", [0x3e] = "CallBuiltin0", [0x3f] = "CallBuiltin1", [0x40] = "CallBuiltin2", [0x41] = "CallBuiltin3", [0x42] = "CallBuiltin4", [0x43] = "CallBuiltin5", [0x44] = "CallBuiltin", [0x45] = "CallBuiltinMethod0", [0x46] = "CallBuiltinMethod1", [0x47] = "CallBuiltinMethod2", [0x48] = "CallBuiltinMethod3", [0x49] = "CallBuiltinMethod4", [0x4a] = "CallBuiltinMethod5", [0x4b] = "CallBuiltinMethod", [0x4c] = "Wait", [0x4d] = "WaitTillFrameEnd", [0x4e] = "PreScriptCall", [0x4f] = "ScriptFunctionCallPointer", [0x50] = "ScriptFunctionCall", [0x51] = "ScriptFunctionCallExpr", [0x52] = "ScriptMethodCall", [0x53] = "ScriptMethodCallExpr", [0x54] = "ScriptThreadCall", [0x55] = "ScriptThreadCallExpr", [0x56] = "ScriptMethodThreadCall", [0x57] = "ScriptMethodThreadCallExpr", [0x58] = "DecTop", [0x59] = "EvalFieldObject", [0x5a] = "EvalLocalVariableObjectCached", [0x5b] = "CastBool", [0x5c] = "BoolNot", [0x5d] = "BoolComplement", [0x5e] = "JumpOnFalse", [0x5f] = "JumpOnTrue", [0x60] = "JumpOnFalseExpr", [0x61] = "JumpOnTrueExpr", [0x62] = "Jump", [0x63] = "JumpBack", [0x64] = "Inc", [0x65] = "Dec", [0x66] = "Bit_Or", [0x67] = "Bit_Xor", [0x68] = "Bit_And", [0x69] = "Equal", [0x6a] = "NotEqual", [0x6b] = "LessThan", [0x6c] = "GreaterThan", [0x6d] = "LessEqual", [0x6e] = "GreaterEqual", [0x6f] = "ShiftLeft", [0x70] = "ShiftRight", [0x71] = "Plus", [0x72] = "Minus", [0x73] = "Multiply", [0x74] = "Divide", [0x75] = "Mod", [0x76] = "SizeOf", [0x77] = "WaitTillMatch", [0x78] = "WaitTill", [0x79] = "NotifyCall", [0x7a] = "EndOnCallback", [0x7b] = "VoidCodepos", [0x7c] = "Switch", [0x7d] = "EndSwitch", [0x7e] = "Vector", [0x7f] = "Nop", [0x80] = "Abort", [0x81] = "Object", [0x82] = "ThreadObject", [0x83] = "EvalLevelFieldVariableRef", [0x84] = "EvalAnimFieldVariableRef", [0x85] = "DevblockBegin", [0x86] = "DevblockEnd"
    };

    if (opcode < 0x87 && names[opcode]) {
        return names[opcode];
    }
    return "InvalidOpcode";
}

static unsigned int VM_OpcodeStaticLength(unsigned int opcode)
{
    static const unsigned char lengths[0x87] = {
        [0x00] = 1, [0x01] = 1, [0x02] = 1, [0x03] = 1, [0x04] = 2, [0x05] = 2, [0x06] = 3, [0x07] = 3, [0x08] = 5, [0x09] = 5, [0x0a] = 3, [0x0b] = 3, [0x0c] = 13, [0x0d] = 1, [0x0e] = 1, [0x0f] = 1, [0x10] = 1, [0x11] = 1, [0x12] = 1, [0x13] = 5, [0x14] = 1, [0x15] = 5, [0x16] = 3, [0x17] = 2, [0x18] = 1, [0x19] = 1, [0x1a] = 1, [0x1b] = 1, [0x1c] = 1, [0x1d] = 1, [0x1e] = 2, [0x1f] = 2, [0x20] = 1, [0x21] = 1, [0x22] = 2, [0x23] = 1, [0x24] = 1, [0x25] = 1, [0x26] = 1, [0x27] = 3, [0x28] = 3, [0x29] = 3, [0x2a] = 3, [0x2b] = 3, [0x2c] = 3, [0x2d] = 3, [0x2e] = 3, [0x2f] = 3, [0x30] = 3, [0x31] = 1, [0x32] = 2, [0x33] = 2, [0x34] = 1, [0x35] = 1, [0x36] = 1, [0x37] = 2, [0x38] = 3, [0x39] = 1, [0x3a] = 3, [0x3b] = 3, [0x3c] = 1, [0x3d] = 2, [0x3e] = 3, [0x3f] = 3, [0x40] = 3, [0x41] = 3, [0x42] = 3, [0x43] = 3, [0x44] = 4, [0x45] = 3, [0x46] = 3, [0x47] = 3, [0x48] = 3, [0x49] = 3, [0x4a] = 3, [0x4b] = 4, [0x4c] = 1, [0x4d] = 1, [0x4e] = 1, [0x4f] = 5, [0x50] = 5, [0x51] = 1, [0x52] = 5, [0x53] = 1, [0x54] = 9, [0x55] = 5, [0x56] = 9, [0x57] = 5, [0x58] = 1, [0x59] = 1, [0x5a] = 2, [0x5b] = 1, [0x5c] = 1, [0x5d] = 1, [0x5e] = 3, [0x5f] = 3, [0x60] = 3, [0x61] = 3, [0x62] = 5, [0x63] = 3, [0x64] = 1, [0x65] = 1, [0x66] = 1, [0x67] = 1, [0x68] = 1, [0x69] = 1, [0x6a] = 1, [0x6b] = 1, [0x6c] = 1, [0x6d] = 1, [0x6e] = 1, [0x6f] = 1, [0x70] = 1, [0x71] = 1, [0x72] = 1, [0x73] = 1, [0x74] = 1, [0x75] = 1, [0x76] = 1, [0x77] = 1, [0x78] = 1, [0x79] = 1, [0x7a] = 1, [0x7b] = 1, [0x7c] = 5, [0x7d] = 0, [0x7e] = 1, [0x7f] = 1, [0x80] = 1, [0x81] = 9, [0x82] = 3, [0x83] = 3, [0x84] = 3, [0x85] = 2, [0x86] = 2
    };

    if (opcode < 0x87) {
        return lengths[opcode];
    }
    return 0;
}

extern void FreeValue(unsigned int value);
extern void Var_Shutdown(void);
extern void SL_Shutdown(void);
extern void Scr_RemoveThreadNotifyName(unsigned int startLocalId);
extern void RemoveObjectVariable(unsigned int parentId, unsigned int id);
extern void AddRefToObject(unsigned int id);
extern unsigned int GetArraySize(unsigned int id);
extern void RemoveVariable(unsigned int parentId, unsigned int value);
extern unsigned int FindObject(unsigned int id);
extern unsigned int FindObjectVariable(unsigned int parentId, unsigned int id);
extern unsigned int Scr_GetSelf(unsigned int startLocalId);
extern unsigned int FindVariable(unsigned int parentId, unsigned int value);
extern unsigned int GetNewArrayVariable(unsigned int parentId, unsigned int unsignedValue);
extern unsigned int Scr_GetThreadNotifyName(unsigned int startLocalId);
extern unsigned int Scr_GetEntityId(int entnum, int classnum);
extern unsigned int FindEntityId(int entnum, int classnum);
extern void Com_Error(int code, const char *fmt, ...);
extern char *va(const char *format, ...);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern Bool Scr_CastString(VariableValue *value);
extern void Scr_CastDebugString(VariableValue *value);
extern int GetVarType(unsigned int id);
extern void Com_Printf(const char *fmt, ...);
extern void Scr_PrintPrevCodePos(int type, const char *codePos, unsigned int index);
extern void _exit(int status);
extern qboolean Scr_SetObjectField(unsigned int classnum, int entnum, int offset);
extern unsigned char scrVarGlob[];
extern void SL_Init(void);
extern void Var_Init(void);
extern unsigned int AllocValue(void);
extern unsigned int AllocObject(void);
extern unsigned int Scr_AllocArray(void);
extern unsigned int SL_GetString(const char *str, unsigned int user);
extern void SL_AddRefToString(unsigned int stringValue);
extern void SL_RemoveRefToString(unsigned int stringValue);
extern const float *Scr_AllocVector(const float *v);
extern void Scr_DumpScriptThreads(void);
extern void Scr_DumpScriptVariables(void);
extern void RemoveRefToValue(int type, VariableUnion u);
extern void AddRefToValue(int type, VariableUnion u);
extern void Scr_CastBool(VariableValue *value);
extern Bool IsValidArrayIndex(unsigned int unsignedValue);
extern unsigned int GetInternalVariableIndex(unsigned int unsignedValue);
extern int Scr_IsInOpcodeMemory(const char *pos);
extern void *MT_Alloc(int size, int type);
extern void MT_Free(void *ptr, int size);
extern int MT_Realloc(int oldNumBytes, int newNumbytes);
extern unsigned int Scr_GetThreadWaitTime(unsigned int startLocalId);
extern unsigned int GetSafeParentLocalId(unsigned int threadId);
extern unsigned int GetStartLocalId(unsigned int threadId);
extern void Scr_EvalEquality(VariableValue *value1, VariableValue *value2);
extern void RuntimeError(const char *codePos, unsigned int index, const char *msg, const char *dialogMessage);
extern unsigned int FindNextSibling(unsigned int id);
extern unsigned int FindPrevSibling(unsigned int id);
extern unsigned int GetParentLocalId(unsigned int threadId);
extern unsigned int GetVariableKeyObject(unsigned int id);
extern VariableUnion *GetVariableValueAddress(unsigned int id);
extern void Scr_ClearWaitTime(unsigned int startLocalId);
extern void ClearVariableValue(unsigned int id);
extern unsigned int GetVariable(unsigned int parentId, unsigned int name);
extern unsigned int GetArray(unsigned int id);
extern unsigned int GetNewObjectVariable(unsigned int parentId, unsigned int id);
extern unsigned int GetNewObjectVariableReverse(unsigned int parentId, unsigned int id);
extern unsigned int GetObjectVariable(unsigned int parentId, unsigned int id);
extern unsigned int AllocThread(unsigned int self);
extern unsigned int AllocChildThread(unsigned int self, unsigned int parentLocalId);
extern Bool IsValidArrayIndex(unsigned int unsignedValue);
extern unsigned int GetInternalVariableIndex(unsigned int unsignedValue);
extern unsigned int GetNewVariable(unsigned int parentId, unsigned int name);
extern void SetNewVariableValue(unsigned int id, const void *value);
extern void Scr_KillThread(unsigned int parentId);
extern void Scr_KillEndonThread(unsigned int threadId);
extern void Scr_StopThread(unsigned int threadId);
extern void Scr_SetThreadWaitTime(unsigned int startLocalId, unsigned int waitTime);
extern void Scr_SetThreadNotifyName(unsigned int startLocalId, unsigned int stringValue);
extern void RemoveRefToEmptyObject(unsigned int id);
extern struct XAnim_s *Scr_GetAnims(int index);
extern XAnim *XAnimGetAnims(const struct XAnimTree_s *tree);
extern const char *XAnimGetAnimTreeDebugName(const XAnim *anims);
extern const char *XAnimGetAnimDebugName(const XAnim *anims, unsigned int animIndex);
extern void Scr_CompileShutdown(void);
extern void Scr_FreeEntityList(void);
extern void Scr_FreeGameVariable(int bComplete);
extern void ClearObject(unsigned int parentId);
extern void Scr_FreeObjects(void);
extern unsigned int Scr_GetEntityIdRef(unsigned int entId);

#define VM_STACKBUF_HEADER_SIZE ((unsigned)offsetof(VariableStackBuffer, buf)) /* 0x0b on x86; 0x0f on x64 (pos is 8 bytes) */
#define VM_STACKBUF_VALUE_SIZE 5
#define VM_LOCAL_ARCHIVE_SLOTS 256u
#define VM_LOCAL_ARCHIVE_MAX_LOCALS 64u

typedef struct VM_LocalVarArchive_s {
    unsigned int localId;
    unsigned int count;
    unsigned short vars[VM_LOCAL_ARCHIVE_MAX_LOCALS];
} VM_LocalVarArchive;

static VM_LocalVarArchive vmLocalVarArchives[VM_LOCAL_ARCHIVE_SLOTS];
static unsigned int vmLocalVarArchiveNext;

void Scr_ClearErrorMessage(void);
void Scr_Settings(int developer, int developer_script, int abort_on_error);
void Scr_Shutdown(void);
void Scr_Abort(void);
void Scr_SetLoading(int bLoading);
unsigned int Scr_GetNumScriptThreads(void);
void Scr_ResetTimeout(void);
#ifdef __EMSCRIPTEN__
static void __attribute_regparm__(3) VM_CancelNotifyInternal(unsigned int notifyListOwnerId, unsigned int startLocalId, unsigned int notifyListId, unsigned int notifyNameListId, unsigned int stringValue);
#else
static void __attribute_regparm__(3) VM_CancelNotifyInternal(unsigned int notifyListOwnerId, unsigned int startLocalId, unsigned int notifyListId, unsigned int notifyNameListId, unsigned int stringValue);
#endif
void VM_CancelNotify(unsigned int notifyListOwnerId, unsigned int startLocalId);
static VariableStackBuffer *__attribute_regparm__(3) VM_ArchiveStack(int size, const char *pos, VariableValue *top, unsigned int localVarCount, unsigned int *pLocalId);
static void __attribute_regparm__(3) VM_TerminateStack(unsigned int endLocalId, unsigned int startLocalId, VariableStackBuffer *stackValue);
static void __attribute_regparm__(3) VM_TrimStack(unsigned int startLocalId, VariableStackBuffer *stackValue, Bool fromEndon);
static void __attribute_regparm__(1) Scr_CancelWaittill(unsigned int startLocalId);
void Scr_CancelNotifyList(unsigned int notifyListOwnerId);
void Scr_FreeThread(int handle);
void Scr_InitSystem(void);
void Scr_ShutdownSystem(int sys, int bComplete);
int Scr_IsSystemActive(int sys);
unsigned int Scr_GetNumParam(void);
void Scr_AddArray(void);
void Scr_AddArrayStringIndexed(unsigned int stringValue);
unsigned long long __attribute_regparm__(0) GetEntityFieldValue(unsigned int classnum, int entnum, int offset);
void Scr_SetStructField(unsigned int structId, unsigned int index);
void Scr_Init(void);
void Scr_TraverseScript(const char *pos);
const char *Scr_GetNextCodepos(VariableValue *top, const char *pos, int opcode, int mode, unsigned int *localId);
static void __attribute_regparm__(3) VM_Notify(unsigned int notifyListOwnerId, unsigned int stringValue, VariableValue *top);
void Scr_SetDynamicEntityField(int entnum, int classnum, unsigned int index);
void Scr_MakeArray(void);
void Scr_NotifyNum(int entnum, int classnum, unsigned int stringValue, unsigned int paramcount);
Bool SetEntityFieldValue(unsigned int classnum, int entnum, int offset, VariableValue *value);
void Scr_ObjectError(const char *error);
void Scr_ParamError(unsigned int index, const char *error);
void Scr_TerminalError(const char *error);
int Scr_GetPointerType(unsigned int index);
const char *Scr_GetTypeName(unsigned int index);
int Scr_GetType(unsigned int index);
unsigned int Scr_GetObject(unsigned int index);
struct scr_entref_t Scr_GetEntityRef(unsigned int index);
void Scr_GetVector(unsigned int index, float *vectorValue);
const char *Scr_GetDebugString(unsigned int index);
float Scr_GetFloat(unsigned int index);
int Scr_GetInt(unsigned int index);
static unsigned int VM_Execute(struct function_stack_t fs);
static void VM_Resume(unsigned int timeId);
void Scr_RunCurrentThreads(void);
void Scr_IncTime(void);
void Scr_AddExecThread(scr_func_t handle, unsigned int paramcount);
scr_thread_t Scr_ExecEntThreadNum(int entnum, int classnum, scr_func_t handle, unsigned int paramcount);
scr_thread_t Scr_ExecThread(scr_func_t handle, unsigned int paramcount);
const char *Scr_GetIString(unsigned int index);
scr_anim_t Scr_GetAnim(unsigned int index, struct XAnimTree_s *tree);
unsigned int Scr_GetConstString(unsigned int index);
unsigned int Scr_GetConstStringIncludeNull(unsigned int index);
const char *Scr_GetString(unsigned int index);
unsigned int Scr_GetConstLowercaseString(unsigned int index);
void Scr_Error(const char *error);
void Scr_AddStruct(void);
void Scr_AddEntityNum(int entnum, int classnum);
void Scr_AddBool(int value);
void Scr_AddInt(int value);
void Scr_AddFloat(float value);
void Scr_AddUndefined(void);
void Scr_AddObject(unsigned int id);
void Scr_AddString(const char *value);
void Scr_AddConstString(unsigned int value);
void Scr_AddVector(const float *value);
extern void RemoveRefToObject(unsigned int id);
extern float floorf(float);

void Scr_ClearErrorMessage(void)
{
    struct scrVarPub_t *p = (struct scrVarPub_t *)imp_scrVarPub;
    p->error_message = NULL;
    ((struct scrVmGlob_t *)scrVmGlob)->dialog_error_message = NULL;
    p->error_index = 0;
}

void Scr_Settings(int developer, int developer_script, int abort_on_error)
{
    struct scrVarPub_t *p = (struct scrVarPub_t *)imp_scrVarPub;
    p->developer = developer != 0;
    p->developer_script = developer_script != 0;
    scrVmPub.abort_on_error = abort_on_error != 0;
}

void Scr_Shutdown(void)
{
    struct scrVarPub_t *base = (struct scrVarPub_t *)imp_scrVarPub;
    unsigned int val;

    if (!base->bInited)
        return;

#if COD2_FEATURE_SCRIPT_DEBUGGER
    Scr_ShutdownDebuggerSystem(0);
#endif

    base->bInited = 0;
    val = base->tempVariable;
    if (val) {
        FreeValue(val);
        base->tempVariable = 0;
    }
    Var_Shutdown();
    SL_Shutdown();
}

void Scr_Abort(void)
{
    struct scrVarPub_t *p = (struct scrVarPub_t *)imp_scrVarPub;
    p->timeArrayId = 0;
    p->bInited = 0;
}

void Scr_SetLoading(int bLoading)
{
    ((struct scrVmGlob_t *)scrVmGlob)->loading = bLoading;
}

unsigned int Scr_GetNumScriptThreads(void)
{
    return 0;
}

static inline __attribute__((always_inline)) void Scr_ResetTimeout_core(void)
{
#if defined(__GNUC__) && !defined(__clang__) && defined(__i386__) && !defined(__EMSCRIPTEN__)

    unsigned long long tsc;
    unsigned int tsc_low_raw, tsc_high_raw;
    __asm__ __volatile__("rdtsc" : "=a"(tsc_low_raw), "=d"(tsc_high_raw));
    tsc = (unsigned long long)tsc_low_raw;
    *(unsigned int *)(scrVmGlob + 24) = (unsigned int)(tsc >> 2);
#else
    *(unsigned int *)(scrVmGlob + 24) = 0;
#endif
}

void Scr_ResetTimeout(void)
{
    Scr_ResetTimeout_core();
}

static void __attribute_regparm__(3) VM_CancelNotifyInternal(unsigned int notifyListOwnerId, unsigned int startLocalId, unsigned int notifyListId, unsigned int notifyNameListId, unsigned int stringValue)
{
    Scr_RemoveThreadNotifyName(startLocalId);
    RemoveObjectVariable(notifyNameListId, startLocalId);
    if (GetArraySize(notifyNameListId))
        return;
    RemoveVariable(notifyListId, stringValue);
    if (GetArraySize(notifyListId))
        return;
    RemoveVariable(notifyListOwnerId, 0x1fffe);
}

void VM_CancelNotify(unsigned int notifyListOwnerId, unsigned int startLocalId)
{
    unsigned int notifyListId = FindObject(FindVariable(notifyListOwnerId, 0x1fffe));
    unsigned int stringValue = (unsigned short)Scr_GetThreadNotifyName(startLocalId);
    unsigned int notifyNameListId = FindObject(FindVariable(notifyListId, stringValue));
    VM_CancelNotifyInternal(notifyListOwnerId, startLocalId, notifyListId, notifyNameListId, stringValue);
}

static unsigned int VM_CurrentFrameLocalCacheCount(void)
{
    unsigned int *base = (unsigned int *)(scrVmGlob + 24);
    unsigned int previousFrameLocals = 0;
    unsigned int frameIndex;
    unsigned int *frameBase;

    if (scrVmPub.function_count > 1) {
        for (frameIndex = 1; frameIndex < (unsigned int)scrVmPub.function_count; frameIndex++) {
            previousFrameLocals += scrVmPub.function_frame_start[frameIndex].fs.localVarCount;
        }
    }

    frameBase = base + previousFrameLocals;
    if (scrVmPub.localVars < frameBase)
        return 0;

    return (unsigned int)(scrVmPub.localVars - frameBase);
}

static void VM_ArchiveLocalVars(VM_LocalVarArchive *archive, unsigned int localId,
                                unsigned int localVarCount, unsigned int cacheCount)
{
    unsigned int count = localVarCount;
    unsigned int missingLeading = 0;
    unsigned int i;
    unsigned int varId;

    if (count > VM_LOCAL_ARCHIVE_MAX_LOCALS)
        count = VM_LOCAL_ARCHIVE_MAX_LOCALS;
    if (cacheCount > count)
        cacheCount = count;

    missingLeading = count - cacheCount;
    varId = FindPrevSibling(localId);
    for (i = 0; i < missingLeading && varId; i++) {
        archive->vars[i] = (unsigned short)varId;
        varId = FindPrevSibling(varId);
    }

    while (i < missingLeading) {
        archive->vars[i++] = 0;
    }

    for (i = 0; i < cacheCount; i++) {
        archive->vars[missingLeading + i] =
            (unsigned short)scrVmPub.localVars[(int)i + 1 - (int)cacheCount];
    }

    archive->count = count;
}

static VariableStackBuffer *__attribute_regparm__(3)
    VM_ArchiveStack(int size, const char *pos, VariableValue *top, unsigned int localVarCount, unsigned int *pLocalId)
{
    struct scrVarPub_t *svp = (struct scrVarPub_t *)imp_scrVarPub;
    unsigned int valueBytes = (unsigned int)size * VM_STACKBUF_VALUE_SIZE;
    unsigned int bufLen = VM_STACKBUF_HEADER_SIZE + valueBytes;
    VariableStackBuffer *stackValue = (VariableStackBuffer *)MT_Alloc((int)bufLen, 1);
    unsigned int localId = *pLocalId;
    char *record;
    int i;
    unsigned int cacheLocalVarCount = VM_CurrentFrameLocalCacheCount();

    if (cacheLocalVarCount > localVarCount)
        cacheLocalVarCount = localVarCount;

    stackValue->localId = (unsigned short)localId;
    stackValue->size = (unsigned short)size;
    stackValue->bufLen = (unsigned short)bufLen;
    stackValue->pos = pos;
    stackValue->time = (byte)svp->time;

    {
        VM_LocalVarArchive *archive = NULL;

        for (i = 0; i < (int)VM_LOCAL_ARCHIVE_SLOTS; i++) {
            if (vmLocalVarArchives[i].localId == localId) {
                archive = &vmLocalVarArchives[i];
                break;
            }
        }

        if (!archive) {
            archive = &vmLocalVarArchives[vmLocalVarArchiveNext++ % VM_LOCAL_ARCHIVE_SLOTS];
        }

        archive->localId = localId;
        VM_ArchiveLocalVars(archive, localId, localVarCount, cacheLocalVarCount);
    }

    scrVmPub.localVars -= cacheLocalVarCount;

    record = (char *)stackValue + VM_STACKBUF_HEADER_SIZE + valueBytes;
    for (i = 0; i < size; i++) {
        VariableValue *value = top - i;

        record -= VM_STACKBUF_VALUE_SIZE;
        record[0] = (char)value->type;

        if (value->type == 7) {
            scrVmPub.function_count--;
            scrVmPub.function_frame--;
#if defined(__x86_64__) || defined(_M_X64)

            *(unsigned int *)(record + 1) =
                SCR_CODEPOS_ENC(scrVmPub.function_frame->fs.pos);
#else
            *(const char **)(record + 1) = scrVmPub.function_frame->fs.pos;
#endif
            scrVmPub.localVars -= scrVmPub.function_frame->fs.localVarCount;
            localId = GetParentLocalId(localId);
        } else {
            *(int *)(record + 1) = value->u.intValue;
        }
    }

    scrVmPub.function_count--;
    scrVmPub.function_frame--;

    AddRefToObject(localId);
    *pLocalId = localId;

    return stackValue;
}

static void __attribute_regparm__(3)
    VM_TerminateStack(unsigned int endLocalId, unsigned int startLocalId, VariableStackBuffer *stackValue)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    unsigned int localId = stackValue->localId;
    unsigned int size = stackValue->size;
    byte *buf = (byte *)stackValue->buf + size * VM_STACKBUF_VALUE_SIZE;
    unsigned int count = 0;

    while (count != size) {
        VariableUnion u;
        int type;

        u.intValue = *(int *)(buf - sizeof(int));
        buf -= VM_STACKBUF_VALUE_SIZE;
        type = *buf;

        if (type == 7) {
            unsigned int parentLocalId = GetParentLocalId(localId);

            Scr_KillThread(localId);
            RemoveRefToObject(localId);

            if (localId == endLocalId) {
                VariableValue tempValue;
                unsigned int timeArray;
                unsigned int timeIndex;
                unsigned int varId;

                *buf = VAR_UNDEFINED;
                Scr_SetThreadWaitTime(startLocalId, varPub->time);

                stackValue->pos = SCR_CODEPOS_GET(u);
                stackValue->localId = (short unsigned int)parentLocalId;
                stackValue->size = (short unsigned int)(size - count);

                tempValue.u.stackValue = SCR_STACK_ENC(stackValue);
                tempValue.type = 10;

                timeArray = GetArray(GetVariable(varPub->timeArrayId, (unsigned int)varPub->time));
                timeIndex = GetNewObjectVariable(timeArray, startLocalId);
                SetNewVariableValue(timeIndex, &tempValue);
                return;
            }

            localId = parentLocalId;
        } else {
            RemoveRefToValue(type, u);
        }

        count++;
    }

    Scr_KillThread(localId);
    RemoveRefToObject(localId);
    MT_Free(stackValue, stackValue->bufLen);
}

static void __attribute_regparm__(3)
    VM_TrimStack(unsigned int startLocalId, VariableStackBuffer *stackValue, Bool fromEndon)
{
    unsigned int localId = stackValue->localId;
    unsigned int size = stackValue->size;
    byte *buf = (byte *)stackValue->buf + size * VM_STACKBUF_VALUE_SIZE;
    unsigned int count = 0;

    while (count != size) {
        VariableUnion u;
        int type;

        u.intValue = *(int *)(buf - sizeof(int));
        buf -= VM_STACKBUF_VALUE_SIZE;
        type = *buf;

        if (type == 7) {
            if (FindObjectVariable(((struct scrVarPub_t *)imp_scrVarPub)->pauseArrayId, localId)) {
                VariableValue tempValue;
                unsigned int varId;

                stackValue->localId = (short unsigned int)localId;
                stackValue->size = (short unsigned int)(size - count);
                Scr_StopThread(localId);

                if (fromEndon) {
                    return;
                }

                Scr_SetThreadNotifyName(startLocalId, 0);
                stackValue->pos = NULL;

                tempValue.u.stackValue = SCR_STACK_ENC(stackValue);
                tempValue.type = 10;
                varId = GetNewVariable(startLocalId, 0x1ffff);
                SetNewVariableValue(varId, &tempValue);
                return;
            }

            {
                unsigned int parentLocalId = GetParentLocalId(localId);

                Scr_KillThread(localId);
                RemoveRefToObject(localId);
                localId = parentLocalId;
            }
        } else {
            RemoveRefToValue(type, u);
        }

        count++;
    }

    if (fromEndon) {
        RemoveVariable(startLocalId, 0x1ffff);
    }

    Scr_KillThread(startLocalId);
    RemoveRefToObject(startLocalId);
    MT_Free(stackValue, stackValue->bufLen);
}

static void __attribute_regparm__(1) Scr_CancelWaittill(unsigned int startLocalId)
{
    unsigned int selfId;
    unsigned int selfNameId;
    unsigned int notifyListOwnerId;

    selfId = Scr_GetSelf(startLocalId);
    selfNameId = FindObject(FindObjectVariable(((struct scrVarPub_t *)imp_scrVarPub)->pauseArrayId, selfId));
    notifyListOwnerId = GetVariableValueAddress(FindObjectVariable(selfNameId, startLocalId))->pointerValue;

    VM_CancelNotify(notifyListOwnerId, startLocalId);
    RemoveObjectVariable(selfNameId, startLocalId);

    if (!GetArraySize(selfNameId)) {
        RemoveObjectVariable(((struct scrVarPub_t *)imp_scrVarPub)->pauseArrayId, selfId);
    }
}

void Scr_CancelNotifyList(unsigned int notifyListOwnerId)
{
    unsigned int notifyNameListId;

    while ((notifyNameListId = FindVariable(notifyListOwnerId, 0x1fffe)) != 0) {
        unsigned int notifyListId;
        unsigned int selfStartLocalId;
        unsigned int startLocalId;

        notifyNameListId = FindObject(notifyNameListId);
        notifyListId = FindNextSibling(notifyNameListId);
        if (!notifyListId) {
            return;
        }

        notifyListId = FindObject(notifyListId);
        selfStartLocalId = FindNextSibling(notifyListId);
        if (!selfStartLocalId) {
            return;
        }

        startLocalId = GetVariableKeyObject(selfStartLocalId);
        if (GetVarType(selfStartLocalId) == 10) {
            VariableStackBuffer *stackValue;

            stackValue = SCR_STACK_PTR(*GetVariableValueAddress(selfStartLocalId));
            Scr_CancelWaittill(startLocalId);
            VM_TrimStack(startLocalId, stackValue, 0);
            continue;
        }

        AddRefToObject(startLocalId);
        Scr_CancelWaittill(startLocalId);

        selfStartLocalId = GetStartLocalId(Scr_GetSelf(startLocalId));
        {
            unsigned int stackId = FindVariable(selfStartLocalId, 0x1ffff);
            if (stackId) {
                VariableStackBuffer *stackValue;

                stackValue = SCR_STACK_PTR(*GetVariableValueAddress(stackId));
                VM_TrimStack(selfStartLocalId, stackValue, 1);
            }
        }

        Scr_KillEndonThread(startLocalId);
        RemoveRefToEmptyObject(startLocalId);
    }
}

void Scr_FreeThread(int handle)
{
    RemoveRefToObject((unsigned short)handle);
}

void Scr_InitSystem(void)
{
    unsigned int timeArrayId = AllocObject();
    struct scrVarPub_t *p = (struct scrVarPub_t *)imp_scrVarPub;

    p->timeArrayId = timeArrayId;
    p->pauseArrayId = Scr_AllocArray();
    p->levelId = AllocObject();
    p->animId = AllocObject();

    p->time = 0;
    g_script_error_level = -1;

#if COD2_FEATURE_SCRIPT_DEBUGGER
    Scr_InitDebuggerSystem();
#endif
}

void Scr_ShutdownSystem(int sys, int bComplete)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    unsigned int id;

    (void)sys;

    Scr_CompileShutdown();
    Scr_FreeEntityList();

    if (!varPub->timeArrayId) {
        return;
    }

    Scr_FreeGameVariable(bComplete);

    id = FindNextSibling(varPub->timeArrayId);
    while (id) {
        unsigned int timeId = FindObject(id);
        unsigned int stackId;

        AddRefToObject(timeId);
        while ((stackId = FindNextSibling(timeId)) != 0) {
            unsigned int startLocalId = GetVariableKeyObject(stackId);
            VariableStackBuffer *stackValue = SCR_STACK_PTR(*GetVariableValueAddress(stackId));

            RemoveObjectVariable(timeId, startLocalId);
            Scr_ClearWaitTime(startLocalId);
            VM_TerminateStack(startLocalId, startLocalId, stackValue);
        }
        RemoveRefToObject(timeId);

        id = FindNextSibling(id);
    }

    while ((id = FindNextSibling(varPub->pauseArrayId)) != 0) {
        unsigned int notifyEntry = FindObject(id);
        unsigned int parentId;

        notifyEntry = FindNextSibling(notifyEntry);
        parentId = GetVariableValueAddress(notifyEntry)->pointerValue;

        AddRefToObject(parentId);
        Scr_CancelNotifyList(parentId);
        RemoveRefToObject(parentId);
    }

    ClearObject(varPub->levelId);
    RemoveRefToEmptyObject(varPub->levelId);
    varPub->levelId = 0;

    ClearObject(varPub->animId);
    RemoveRefToEmptyObject(varPub->animId);
    varPub->animId = 0;

    ClearObject(varPub->timeArrayId);
    RemoveRefToEmptyObject(varPub->timeArrayId);
    varPub->timeArrayId = 0;

    RemoveRefToEmptyObject(varPub->pauseArrayId);
    varPub->pauseArrayId = 0;

    Scr_FreeObjects();
}

int Scr_IsSystemActive(int sys)
{
    return ((struct scrVarPub_t *)imp_scrVarPub)->timeArrayId != 0;
}

unsigned int Scr_GetNumParam(void)
{
    return scrVmPub.outparamcount;
}

static inline __attribute__((always_inline)) VariableValue *IncInParam(void)
{
    while (scrVmPub.outparamcount) {
        RemoveRefToValue(scrVmPub.top->type, scrVmPub.top->u);
        --scrVmPub.top;
        --scrVmPub.outparamcount;
    }

    if (scrVmPub.top == scrVmPub.maxstack) {
        Com_Error(1, "\x15Internal script stack overflow");
    }

    ++scrVmPub.top;
    ++scrVmPub.inparamcount;

    return scrVmPub.top;
}

void Scr_AddArray(void)
{
    unsigned int varId;

    scrVmPub.top--;
    scrVmPub.inparamcount--;

    varId = GetNewArrayVariable(scrVmPub.top->u.pointerValue, GetArraySize(scrVmPub.top->u.pointerValue));
    SetNewVariableValue(varId, scrVmPub.top + 1);
}

void Scr_AddArrayStringIndexed(unsigned int stringValue)
{
    unsigned int parentId;
    unsigned int varId;
    byte *top;

    top = (byte *)scrVmPub.top;
    scrVmPub.top = (void *)(top - 8);
    scrVmPub.inparamcount--;

    parentId = *(unsigned int *)(top - 8);
    varId = GetNewVariable(parentId, stringValue);
    SetNewVariableValue(varId, (byte *)scrVmPub.top + 8);
}

extern void Scr_GetObjectField(unsigned int classnum, int entnum, int offset);
extern unsigned char bg_weapClips[];

unsigned long long __attribute_regparm__(0)
    GetEntityFieldValue(unsigned int classnum, int entnum, int offset)
{

    scrVmPub.top = (VariableValue *)((byte *)scrVmGlob - sizeof(VariableValue));
    *(int *)(scrVmGlob + 4) = 0;
    Scr_GetObjectField(classnum, entnum, offset);
    scrVmPub.inparamcount = 0;

    return *(const unsigned long long *)scrVmGlob;
}

extern unsigned int Scr_GetVariableField(unsigned int structId, unsigned int index);
extern void SetVariableFieldValue(unsigned int id, const void *value);
void Scr_SetStructField(unsigned int structId, unsigned int index)
{
    unsigned int varId;

    varId = Scr_GetVariableField(structId, index);
    scrVmPub.inparamcount = 0;
    SetVariableFieldValue(varId, scrVmPub.top);
    scrVmPub.top = (VariableValue *)((byte *)scrVmPub.top - 8);
}

void Scr_Init(void)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    struct scrCompilePub_t *compilePub;
    struct scrAnimPub_t *animPub;

    if (varPub->bInited)
        return;

    SL_Init();
    Var_Init();

    scrVmPub.maxstack = &scrVmPub.stack[2047];
    scrVmPub.top = scrVmPub.stack;
    scrVmPub.function_count = 0;
    scrVmPub.function_frame = scrVmPub.function_frame_start;
    scrVmPub.localVars = (unsigned int *)(scrVmGlob + 24);
    varPub->evaluate = 0;
    scrVmPub.debugCode = 0;
    varPub->error_message = NULL;
    *(int *)(scrVmGlob + 16) = 0;
    varPub->error_index = 0;
    scrVmPub.terminal_error = 0;
    scrVmPub.outparamcount = 0;
    scrVmPub.inparamcount = 0;

    varPub->tempVariable = AllocValue();
    varPub->timeArrayId = 0;
    varPub->pauseArrayId = 0;
    varPub->levelId = 0;
    varPub->gameId = 0;
    varPub->animId = 0;
    varPub->freeEntList = 0;
    scrVmPub.stack[0].type = 7;
    *(int *)(scrVmGlob + 20) = 0;

    compilePub = (struct scrCompilePub_t *)imp_scrCompilePub;
    compilePub->script_loading = 0;
    animPub = (struct scrAnimPub_t *)imp_scrAnimPub;
    animPub->animtree_loading = 0;
    compilePub->scripts = 0;
    compilePub->loadedscripts = 0;
    animPub->animtrees = 0;
    compilePub->builtinMeth = 0;
    compilePub->builtinFunc = 0;

    varPub->bInited = 1;
}

void Scr_TraverseScript(const char *pos)
{
    const byte *scan = (const byte *)pos;

    for (;;) {
        const byte *next = scan + 1;
        unsigned int opcode = *scan;

        switch (opcode) {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0xd:
        case 0xe:
        case 0xf:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x14:
        case 0x18:
        case 0x19:
        case 0x1a:
        case 0x1b:
        case 0x1c:
        case 0x1d:
        case 0x20:
        case 0x21:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x31:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x39:
        case 0x3c:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x51:
        case 0x53:
        case 0x58:
        case 0x59:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
        case 0x68:
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6e:
        case 0x6f:
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
        case 0x78:
        case 0x79:
        case 0x7a:
        case 0x7b:
        case 0x7e:
            scan = next;
            break;
        case 0x4:
        case 0x5:
        case 0x17:
        case 0x1e:
        case 0x1f:
        case 0x22:
        case 0x32:
        case 0x33:
        case 0x37:
        case 0x3d:
        case 0x5a:
        case 0x85:
        case 0x86:
            scan += 2;
            break;
        case 0x6:
        case 0x7:
        case 0xa:
        case 0xb:
        case 0x16:
        case 0x27:
        case 0x28:
        case 0x29:
        case 0x2a:
        case 0x2b:
        case 0x2c:
        case 0x2d:
        case 0x2e:
        case 0x2f:
        case 0x30:
        case 0x38:
        case 0x3a:
        case 0x3b:
        case 0x3e:
        case 0x3f:
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x45:
        case 0x46:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x5e:
        case 0x5f:
        case 0x60:
        case 0x61:
        case 0x63:
            scan += 3;
            break;
        case 0x44:
        case 0x4b:
            scan += 4;
            break;
        case 0x8:
        case 0x9:
        case 0x13:
        case 0x15:
        case 0x4f:
        case 0x50:
        case 0x52:
        case 0x55:
        case 0x57:
        case 0x62:
        case 0x7c:
            scan += 5;
            break;
        case 0x54:
        case 0x56:
            scan += 9;
            break;
        case 0x0c:
            scan += 13;
            break;
        case 0x7d:
            scan += 3 + 8 * *(const unsigned short *)next;
            break;
        default:
            return;
        }
    }
}

const char *Scr_GetNextCodepos(VariableValue *top, const char *pos, int opcode, int mode, unsigned int *localId)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    const char *next;
    VariableValue value;
    unsigned int caseValue;
    unsigned short caseCount;
    int jumpOffset;
    int i;

    *localId = scrVmPub.function_frame->fs.localId;

    for (;;) {
        next = pos + 1;

        if (mode == 2) {
            switch (opcode) {
            case 0x4f:
            case 0x50:
            case 0x54:
                if (scrVmPub.function_count <= 31) {
                    *localId = 0;
#if defined(__x86_64__) || defined(_M_X64)
                    return SCR_CODEPOS_PTR(*(const unsigned int *)next);
#else
                    return *(const char *const *)next;
#endif
                }
                break;

            case 0x51:
            case 0x55:
                if (top->type == 9 && scrVmPub.function_count <= 31) {
                    *localId = 0;
                    return SCR_CODEPOS_GET(top->u);
                }
                break;

            case 0x52:
            case 0x56:
                if (top->type == 1 && scrVmPub.function_count <= 31) {
                    *localId = 0;
#if defined(__x86_64__) || defined(_M_X64)
                    return SCR_CODEPOS_PTR(*(const unsigned int *)next);
#else
                    return *(const char *const *)next;
#endif
                }
                break;

            case 0x53:
            case 0x57:
                if (top[-1].type == 1 && top->type == 9 && scrVmPub.function_count <= 31) {
                    *localId = 0;
                    return SCR_CODEPOS_GET(top->u);
                }
                break;

            default:
                break;
            }
        }

        switch (opcode) {
        case 0x00:
        case 0x01:
            if (scrVmPub.function_count <= 1)
                return NULL;
            {
                function_frame_t *frame = scrVmPub.function_frame - 1;
                if (frame->fs.pos == &g_EndPos)
                    return NULL;
                *localId = frame->fs.localId;
                return frame->fs.pos;
            }

        case 0x04:
        case 0x05:
        case 0x17:
        case 0x1e:
        case 0x1f:
        case 0x22:
        case 0x32:
        case 0x33:
        case 0x37:
        case 0x3d:
        case 0x5a:
        case 0x77:
        case 0x85:
        case 0x86:
            next = pos + 2;
            goto clear_error_and_return_next;

        case 0x06:
        case 0x07:
        case 0x0a:
        case 0x0b:
        case 0x16:
        case 0x27:
        case 0x28:
        case 0x29:
        case 0x2a:
        case 0x2b:
        case 0x2c:
        case 0x2d:
        case 0x2e:
        case 0x2f:
        case 0x30:
        case 0x38:
        case 0x3a:
        case 0x3b:
        case 0x3e:
        case 0x3f:
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x45:
        case 0x46:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x82:
        case 0x83:
        case 0x84:
            next = pos + 3;
            goto clear_error_and_return_next;

        case 0x44:
        case 0x4b:
            next = pos + 4;
            goto clear_error_and_return_next;

        case 0x08:
        case 0x09:
        case 0x13:
        case 0x15:
        case 0x4f:
        case 0x50:
        case 0x52:
        case 0x55:
        case 0x57:
            next = pos + 5;
            goto clear_error_and_return_next;

        case 0x54:
        case 0x56:
        case 0x81:
            next = pos + 9;
            goto clear_error_and_return_next;

        case 0x0c:
            next = pos + 13;
            goto clear_error_and_return_next;

        case 0x5e:
        case 0x60:
            value = *top;
            AddRefToValue(value.type, value.u);
            Scr_CastBool(&value);
            if (varPub->error_message)
                goto clear_error_and_return_next;
            jumpOffset = *(const unsigned short *)next;
            next = pos + 3;
            if (!value.u.intValue)
                next += jumpOffset;
            return next;

        case 0x5f:
        case 0x61:
            value = *top;
            AddRefToValue(value.type, value.u);
            Scr_CastBool(&value);
            if (varPub->error_message)
                goto clear_error_and_return_next;
            jumpOffset = *(const unsigned short *)next;
            next = pos + 3;
            if (value.u.intValue)
                next += jumpOffset;
            return next;

        case 0x62:
            jumpOffset = *(const int *)next;
            return next + 4 + jumpOffset;

        case 0x63:
            return next + 2 - *(const unsigned short *)next;

        case 0x7c:
            jumpOffset = *(const int *)next;
            next = next + 4 + jumpOffset;
            caseCount = *(const unsigned short *)next;
            next += 2;

            if (top->type == 2) {
                caseValue = top->u.stringValue;
            } else if (top->type == 6) {
                if (!IsValidArrayIndex(top->u.intValue))
                    return next + caseCount * 8;
                caseValue = GetInternalVariableIndex(top->u.intValue);
            } else {
                return next + caseCount * 8;
            }

            for (i = 0; i < caseCount; ++i) {
                unsigned int switchValue = *(const unsigned int *)next;
#if defined(__x86_64__) || defined(_M_X64)
                const char *caseCodePos = SCR_CODEPOS_PTR(*(const unsigned int *)(next + 4));
#else
                const char *caseCodePos = *(const char *const *)(next + 4);
#endif
                next += 8;

                if (caseValue == switchValue || (i == caseCount - 1 && switchValue == 0))
                    return caseCodePos;
            }
            return next;

        case 0x7d:
            return next + 2 + 8 * *(const unsigned short *)next;

        default:
            goto clear_error_and_return_next;
        }

    clear_error_and_return_next:
        varPub->error_message = NULL;
        ((struct scrVmGlob_t *)scrVmGlob)->dialog_error_message = NULL;
        varPub->error_index = 0;

        opcode = *(const unsigned char *)next;
        if (opcode == 0x39) {
            pos = next;
            continue;
        }
        return next;
    }
}

static void VM_NotifyEndLocalThreads(unsigned int selfId)
{
    int function_count = scrVmPub.function_count;
    int firstFrame;
    int lastFrame;
    int scan;

    if (!function_count) {
        return;
    }

    if (scrVmPub.function_frame_start[function_count].fs.localId == selfId) {
        firstFrame = function_count;
        lastFrame = function_count;
    } else {
        firstFrame = function_count;
        lastFrame = function_count;

        for (scan = function_count - 1; scan >= 0; --scan) {
            unsigned int localId = scrVmPub.function_frame_start[scan + 1].fs.localId;

            if (!GetSafeParentLocalId(localId)) {
                firstFrame = scan;
            }

            if (scrVmPub.function_frame_start[scan].fs.localId == selfId) {
                lastFrame = scan;
                break;
            }
        }

        if (scan < 0 || lastFrame > firstFrame) {
            return;
        }
    }

    for (scan = firstFrame; scan >= lastFrame; --scan) {
        scrVmPub.function_frame_start[scan].fs.pos = &g_EndPos;
    }
}

static VariableStackBuffer *VM_NotifyRemoveStackFromWait(unsigned int selfId, unsigned int startLocalId)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    unsigned int waitString = (unsigned short)Scr_GetThreadNotifyName(startLocalId);
    VariableStackBuffer *stackValue;

    if (!waitString) {
        unsigned int stackId = FindVariable(startLocalId, 0x1ffff);
        stackValue = SCR_STACK_PTR(*GetVariableValueAddress(stackId));
        RemoveVariable(startLocalId, 0x1ffff);
        return stackValue;
    }

    {
        unsigned int waitSelfId = Scr_GetSelf(startLocalId);
        unsigned int selfNameId = FindObject(FindObjectVariable(varPub->pauseArrayId, waitSelfId));
        unsigned int notifyListOwnerId = GetVariableValueAddress(FindObjectVariable(selfNameId, startLocalId))->pointerValue;
        unsigned int notifyListId = FindObject(FindVariable(notifyListOwnerId, 0x1fffe));
        unsigned int notifyNameListId = FindObject(FindVariable(notifyListId, waitString));
        unsigned int stackId = FindObjectVariable(notifyNameListId, startLocalId);

        stackValue = SCR_STACK_PTR(*GetVariableValueAddress(stackId));
        VM_CancelNotifyInternal(notifyListOwnerId, startLocalId, notifyListId, notifyNameListId, waitString);
        RemoveObjectVariable(selfNameId, startLocalId);
        if (!GetArraySize(selfNameId)) {
            RemoveObjectVariable(varPub->pauseArrayId, waitSelfId);
        }
    }

    return stackValue;
}

static void VM_NotifyTerminateWaitStack(unsigned int selfId, unsigned int startLocalId)
{
    VariableStackBuffer *stackValue = VM_NotifyRemoveStackFromWait(selfId, startLocalId);
    VM_TerminateStack(selfId, startLocalId, stackValue);
}

static void VM_NotifyTerminatePausedStack(unsigned int selfId, unsigned int startLocalId)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    unsigned int time = Scr_GetThreadWaitTime(startLocalId);
    unsigned int id;
    VariableStackBuffer *stackValue;

    Scr_ClearWaitTime(startLocalId);
    /* Timed waits live in timeArrayId; pauseArrayId indexes notify owners.
     * Reading a time bucket from pauseArrayId interpreted an object id as a
     * stack pointer when an endon cancelled a waiting connection thread. */
    id = FindObject(FindVariable(varPub->timeArrayId, time));
    stackValue = SCR_STACK_PTR(*GetVariableValueAddress(FindObjectVariable(id, startLocalId)));
    RemoveObjectVariable(id, startLocalId);

    if (!GetArraySize(id) && time != (unsigned int)varPub->time) {
        RemoveVariable(varPub->timeArrayId, time);
    }

    VM_TerminateStack(selfId, startLocalId, stackValue);
}

static int VM_NotifyWaittillMatches(VariableStackBuffer *stackValue, VariableValue *top)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    const char *pos = stackValue->pos;
    int needed = (signed char)pos[0];
    int lastIndex = needed - 1;
    int matchIndex = 0;
    byte *buf = (byte *)stackValue->buf + ((int)stackValue->size - needed) * VM_STACKBUF_VALUE_SIZE;
    VariableValue *current = top;

    if (!needed) {
        return 1;
    }

    if (top->type == 8) {
        return 0;
    }

    for (;;) {
        VariableValue savedValue;
        VariableValue currentValue;

        savedValue.type = buf[0];
        if (savedValue.type == 8) {
            return 1;
        }
        savedValue.u.intValue = *(int *)(buf + 1);
        AddRefToValue(savedValue.type, savedValue.u);

        currentValue = *current;
        AddRefToValue(currentValue.type, currentValue.u);
        Scr_EvalEquality(&savedValue, &currentValue);

        if (varPub->error_message) {
            RuntimeError(pos, (unsigned int)(matchIndex + 4), varPub->error_message,
                         ((struct scrVmGlob_t *)scrVmGlob)->dialog_error_message);
            varPub->error_message = NULL;
            ((struct scrVmGlob_t *)scrVmGlob)->dialog_error_message = NULL;
            varPub->error_index = 0;
            return 0;
        }

        if (!savedValue.u.intValue) {
            return 0;
        }

        if (matchIndex == lastIndex) {
            return 1;
        }

        --current;
        if (current->type == 8) {
            return 1;
        }

        buf += VM_STACKBUF_VALUE_SIZE;
        ++matchIndex;
    }
}

static void VM_NotifyAppendStackParams(VariableUnion *stackRef, VariableStackBuffer **pStackValue, VariableValue *top)
{
    VariableStackBuffer *stackValue = *pStackValue;
    unsigned int oldSize = stackValue->size;
    unsigned int newSize = oldSize;
    unsigned int oldBytes = oldSize * VM_STACKBUF_VALUE_SIZE;
    unsigned int newBufLen;
    VariableValue *firstValue = top;
    VariableValue *scan = top;
    byte *dst;
    unsigned int addCount;
    unsigned int i;

    do {
        ++newSize;
        --scan;
    } while (scan->type != 8);

    firstValue = scan + 1;
    addCount = newSize - oldSize;
    newBufLen = VM_STACKBUF_HEADER_SIZE + newSize * VM_STACKBUF_VALUE_SIZE;

    if (!MT_Realloc(stackValue->bufLen, newBufLen)) {
        VariableStackBuffer *newStackValue = (VariableStackBuffer *)MT_Alloc((int)newBufLen, 1);
        newStackValue->bufLen = (unsigned short)newBufLen;
        newStackValue->pos = stackValue->pos;
        newStackValue->localId = stackValue->localId;
        newStackValue->time = stackValue->time;
        memcpy(newStackValue->buf, stackValue->buf, oldBytes);
        MT_Free(stackValue, stackValue->bufLen);
        stackRef->stackValue = SCR_STACK_ENC(newStackValue);
        stackValue = newStackValue;
        *pStackValue = newStackValue;
    }

    stackValue->size = (unsigned short)newSize;
    dst = (byte *)stackValue->buf + oldBytes;

    for (i = 0; i < addCount; ++i) {
        VariableValue *value = firstValue + i;

        AddRefToValue(value->type, value->u);
        dst[0] = (byte)value->type;
        *(int *)(dst + 1) = value->u.intValue;
        dst += VM_STACKBUF_VALUE_SIZE;
    }
}

static void VM_NotifySuspendStack(unsigned int notifyListOwnerId, unsigned int stringValue,
                                  unsigned int notifyListId, unsigned int notifyNameListId,
                                  unsigned int selfNameId, unsigned int selfId,
                                  unsigned int startLocalId, VariableStackBuffer *stackValue,
                                  VariableValue *top, Bool noStack)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    static unsigned int traceCount;
    VariableValue tempValue;
    unsigned int waitArray;
    unsigned int stackId;
    VariableUnion *stackRef;

    tempValue.type = 10;
    tempValue.u.stackValue = SCR_STACK_ENC(stackValue);

    waitArray = GetArray(GetVariable(varPub->timeArrayId, (unsigned int)varPub->time));
    stackId = GetNewObjectVariable(waitArray, startLocalId);
    SetNewVariableValue(stackId, &tempValue);
    stackRef = GetVariableValueAddress(stackId);
    if (traceCount < 32) {
        if (getenv("DBGSPAM"))
            Com_Printf("[team-trace] notify queue local=%u time=%u pauseArray=%u bucket=%u stack=%u pos=%p\n",
                       startLocalId, (unsigned int)varPub->time, varPub->pauseArrayId,
                       waitArray, stackId, (const void *)stackValue->pos);
        ++traceCount;
    }

    VM_CancelNotifyInternal(notifyListOwnerId, startLocalId, notifyListId, notifyNameListId, stringValue);
    RemoveObjectVariable(selfNameId, startLocalId);
    if (!GetArraySize(selfNameId)) {
        RemoveObjectVariable(varPub->pauseArrayId, selfId);
    }

    Scr_SetThreadWaitTime(startLocalId, varPub->time);

    if (!noStack) {
        VM_NotifyAppendStackParams(stackRef, &stackValue, top);
    }
}

static void __attribute_regparm__(3)
    VM_Notify(unsigned int notifyListOwnerId, unsigned int stringValue, VariableValue *top)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    static unsigned int traceCount;
    unsigned int notifyListId;
    unsigned int notifyNameListId;
    unsigned int notifyListVar;
    unsigned int notifyNameListVar;
    unsigned int scanId;

    notifyListVar = FindVariable(notifyListOwnerId, 0x1fffe);
    if (traceCount < 64) {
        if (getenv("DBGSPAM"))
            Com_Printf("[team-trace] notify owner=%u name=%u('%s') listvar=%u topType=%d\n",
                       notifyListOwnerId, stringValue, SL_ConvertToString(stringValue),
                       notifyListVar, top->type);
        ++traceCount;
    }
    if (!notifyListVar) {
        return;
    }

    notifyListId = FindObject(notifyListVar);
    notifyNameListVar = FindVariable(notifyListId, stringValue);
    if (traceCount < 64) {
        if (getenv("DBGSPAM"))
            Com_Printf("[team-trace] notify lookup list=%u namevar=%u\n",
                       notifyListId, notifyNameListVar);
        ++traceCount;
    }
    if (!notifyNameListVar) {
        return;
    }

    notifyNameListId = FindObject(notifyNameListVar);
    AddRefToObject(notifyNameListId);
    varPub->evaluate = 1;

    scanId = notifyNameListId;
    while ((scanId = FindPrevSibling(scanId)) != 0) {
        unsigned int startLocalId = GetVariableKeyObject(scanId);
        unsigned int selfId = Scr_GetSelf(startLocalId);
        unsigned int selfNameId = FindObject(FindObjectVariable(varPub->pauseArrayId, selfId));
        int varType = GetVarType(scanId);

        if (!varType) {
            unsigned int currentStartLocalId;
            int currentType;

            VM_CancelNotifyInternal(notifyListOwnerId, startLocalId, notifyListId, notifyNameListId, stringValue);
            Scr_KillEndonThread(startLocalId);
            RemoveObjectVariable(selfNameId, startLocalId);
            if (!GetArraySize(selfNameId)) {
                RemoveObjectVariable(varPub->pauseArrayId, selfId);
            }

            currentStartLocalId = GetStartLocalId(selfId);
            currentType = GetVarType(currentStartLocalId);

            if (currentType == 0x10) {
                VM_NotifyTerminateWaitStack(selfId, currentStartLocalId);
                scanId = notifyNameListId;
                continue;
            }

            if (currentType == 0x11) {
                VM_NotifyTerminatePausedStack(selfId, currentStartLocalId);
                scanId = notifyNameListId;
                continue;
            }

            if (currentType == 0x0f) {
                VM_NotifyEndLocalThreads(selfId);
                scanId = notifyNameListId;
                continue;
            }

            scanId = notifyNameListId;
            continue;
        }

        {
            VariableUnion *stackRef = GetVariableValueAddress(scanId);
            VariableStackBuffer *stackValue = SCR_STACK_PTR(*stackRef);
            Bool noStack;

            if (stackValue->pos[-1] == 0x77) {
                if (!VM_NotifyWaittillMatches(stackValue, top)) {
                    continue;
                }
                stackValue->pos++;
                noStack = 1;
            } else {
                noStack = (top->type == 8);
            }

            VM_NotifySuspendStack(notifyListOwnerId, stringValue, notifyListId, notifyNameListId,
                                  selfNameId, selfId, startLocalId, stackValue, top, noStack);
            scanId = notifyNameListId;
        }
    }

    RemoveRefToObject(notifyNameListId);
    varPub->evaluate = 0;
}

void Scr_SetDynamicEntityField(int entnum, int classnum, unsigned int index)
{
    unsigned int entId = Scr_GetEntityId(entnum, classnum);
    unsigned int varId = Scr_GetVariableField(entId, index);

    scrVmPub.inparamcount = 0;
    SetVariableFieldValue(varId, scrVmPub.top);
    scrVmPub.top--;
}

void Scr_MakeArray(void)
{
    IncInParam();
    scrVmPub.top->type = VAR_POINTER;
    scrVmPub.top->u.pointerValue = Scr_AllocArray();
}

void Scr_NotifyNum(int entnum, int classnum, unsigned int stringValue, unsigned int paramcount)
{
    VariableValue *startTop;
    unsigned int oldInParamCount;
    unsigned int notifyListOwnerId;
    int oldStartTopType;

    while (scrVmPub.outparamcount) {
        RemoveRefToValue(scrVmPub.top->type, scrVmPub.top->u);
        --scrVmPub.top;
        --scrVmPub.outparamcount;
    }

    startTop = scrVmPub.top - paramcount;
    oldInParamCount = scrVmPub.inparamcount - paramcount;

    notifyListOwnerId = FindEntityId(entnum, classnum);
    if (notifyListOwnerId) {
        oldStartTopType = startTop->type;
        startTop->type = 8;
        scrVmPub.inparamcount = 0;
        VM_Notify(notifyListOwnerId, stringValue, scrVmPub.top);
        startTop->type = oldStartTopType;
    }

    while (scrVmPub.top != startTop) {
        RemoveRefToValue(scrVmPub.top->type, scrVmPub.top->u);
        --scrVmPub.top;
    }

    scrVmPub.inparamcount = oldInParamCount;
}

static inline __attribute__((always_inline)) void Scr_SetErrorMessageAndJump(const char *error)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    struct scrCompilePub_t *compilePub = (struct scrCompilePub_t *)imp_scrCompilePub;

    if (!varPub->error_message)
        varPub->error_message = error;

    if (varPub->evaluate || compilePub->script_loading) {
        if (scrVmPub.terminal_error)
            Com_Error(1, "\x15%s", varPub->error_message);
        return;
    }

    if (varPub->developer && *(int *)(scrVmGlob + 20))
        scrVmPub.terminal_error = 1;

    if (scrVmPub.function_count || scrVmPub.debugCode)
        longjmp(g_script_error[g_script_error_level], -1);

    Com_Error(1, "\x15%s", varPub->error_message);
}

static inline __attribute__((always_inline)) VariableValue *Scr_GetParamValue(unsigned int index)
{
    if (index >= scrVmPub.outparamcount) {
        Scr_SetErrorMessageAndJump(va("parameter %d does not exist", index + 1));
        return scrVmPub.top;
    }

    return scrVmPub.top - index;
}

static inline __attribute__((always_inline)) VariableValue *Scr_GetConstStringValue(unsigned int index)
{
    VariableValue *value;

    if (index >= scrVmPub.outparamcount) {
        Scr_SetErrorMessageAndJump(va("parameter %d does not exist", index + 1));
        return NULL;
    }

    value = scrVmPub.top - index;
    if (!Scr_CastString(value)) {
        ((struct scrVarPub_t *)imp_scrVarPub)->error_index = index + 1;
        Scr_SetErrorMessageAndJump(va("parameter %d does not exist", index + 1));
        return NULL;
    }

    return value;
}

static inline __attribute__((always_inline)) void Scr_ParamTypeError(unsigned int index, const char *fmt, int type)
{
    ((struct scrVarPub_t *)imp_scrVarPub)->error_index = index + 1;
    Scr_SetErrorMessageAndJump(va(fmt, ((const char *const *)imp_var_typename)[type]));
}

Bool SetEntityFieldValue(unsigned int classnum, int entnum, int offset, VariableValue *value)
{
    scrVmPub.outparamcount = 1;
    scrVmPub.top = value;

    if (!Scr_SetObjectField(classnum, entnum, offset)) {
        scrVmPub.outparamcount = 0;
        return 0;
    }

    if (scrVmPub.outparamcount) {
        RemoveRefToValue(scrVmPub.top->type, scrVmPub.top->u);
        scrVmPub.top--;
        scrVmPub.outparamcount = 0;
    }

    return 1;
}

void Scr_ObjectError(const char *error)
{
    ((struct scrVarPub_t *)imp_scrVarPub)->error_index = -1;
    Scr_SetErrorMessageAndJump(error);
}

void Scr_ParamError(unsigned int index, const char *error)
{
    ((struct scrVarPub_t *)imp_scrVarPub)->error_index = index + 1;
    Scr_SetErrorMessageAndJump(error);
}

void Scr_TerminalError(const char *error)
{
    Scr_DumpScriptThreads();
    Scr_DumpScriptVariables();
    scrVmPub.terminal_error = 1;
    Scr_SetErrorMessageAndJump(error);
}

int Scr_GetPointerType(unsigned int index)
{
    VariableValue *value;

    if (index >= scrVmPub.outparamcount) {
        Scr_SetErrorMessageAndJump(va("parameter %d does not exist", index + 1));
        return 0;
    }

    value = scrVmPub.top - index;
    if (value->type != VAR_POINTER) {
        Scr_SetErrorMessageAndJump(va("type %s is not a pointer", ((const char *const *)imp_var_typename)[value->type]));
        return 0;
    }

    return GetVarType(value->u.pointerValue);
}

const char *Scr_GetTypeName(unsigned int index)
{
    if (index >= scrVmPub.outparamcount) {
        Scr_SetErrorMessageAndJump(va("parameter %d does not exist", index + 1));
        return NULL;
    }

    return ((const char *const *)imp_var_typename)[(scrVmPub.top - index)->type];
}

int Scr_GetType(unsigned int index)
{
    if (index >= scrVmPub.outparamcount) {
        Scr_SetErrorMessageAndJump(va("parameter %d does not exist", index + 1));
        return 0;
    }

    return (scrVmPub.top - index)->type;
}

unsigned int Scr_GetObject(unsigned int index)
{
    VariableValue *value = Scr_GetParamValue(index);

    if (index >= scrVmPub.outparamcount) {
        return 0;
    }
    if (value->type != VAR_POINTER) {
        Scr_ParamTypeError(index, "type %s is not an object", value->type);
        return 0;
    }

    return value->u.pointerValue;
}

struct scr_entref_t Scr_GetEntityRef(unsigned int index)
{
    struct scr_entref_t ref;
    VariableValue *value;
    unsigned int id;

    ref.entnum = 0;
    ref.classnum = 0;

    if (index >= scrVmPub.outparamcount) {
        Scr_SetErrorMessageAndJump(va("parameter %d does not exist", index + 1));
        return ref;
    }

    value = scrVmPub.top - index;
    if (value->type != VAR_POINTER) {
        Scr_ParamTypeError(index, "type %s is not an entity", value->type);
        return ref;
    }

    id = value->u.pointerValue;
    if (GetVarType(id) != 21) {
        ((struct scrVarPub_t *)imp_scrVarPub)->error_index = index + 1;
        Scr_SetErrorMessageAndJump(
            va("type %s is not an entity", ((const char *const *)imp_var_typename)[GetVarType(id)]));
        return ref;
    }

    id = Scr_GetEntityIdRef(id);
    ref.entnum = (short unsigned int)(id & 0xffff);
    ref.classnum = (short unsigned int)(id >> 16);
    return ref;
}

void Scr_GetVector(unsigned int index, float *vectorValue)
{
    VariableValue *value = Scr_GetParamValue(index);
    const float *from;

    if (index >= scrVmPub.outparamcount) {
        return;
    }
    if (value->type != VAR_VECTOR) {
        Scr_ParamTypeError(index, "type %s is not a vector", value->type);
        return;
    }

    from = SCR_VEC_PTR(value->u);
    vectorValue[0] = from[0];
    vectorValue[1] = from[1];
    vectorValue[2] = from[2];
}

const char *Scr_GetDebugString(unsigned int index)
{
    VariableValue *value = Scr_GetParamValue(index);

    if (index >= scrVmPub.outparamcount) {
        return NULL;
    }

    Scr_CastDebugString(value);
    return SL_ConvertToString(value->u.stringValue);
}

float Scr_GetFloat(unsigned int index)
{
    VariableValue *value = Scr_GetParamValue(index);

    if (index >= scrVmPub.outparamcount) {
        return 0.0f;
    }

    if (value->type == VAR_FLOAT) {
        return value->u.floatValue;
    }
    if (value->type == VAR_INTEGER) {
        return (float)value->u.intValue;
    }

    Scr_ParamTypeError(index, "type %s is not a float", value->type);
    return 0.0f;
}

int Scr_GetInt(unsigned int index)
{
    VariableValue *value = Scr_GetParamValue(index);

    if (index >= scrVmPub.outparamcount) {
        return 0;
    }
    if (value->type != VAR_INTEGER) {
        Scr_ParamTypeError(index, "type %s is not an int", value->type);
        return 0;
    }

    return value->u.intValue;
}

static const char str_dbg_vmexec_top0[] = "DBG ERROR: scrVmPub.top is NULL after VM_Execute!\n";
static const char str_dbg_null_builtin[] = "DBG ERROR: NULL builtin index=%d pos=%p\n";
static const char str_dbg_builtin_oor[] = "DBG FATAL: builtin index %d out of range, pos=%p byte[-3]=%02x byte[-1]=%02x\n";

static unsigned int dbg_op_ring[64];
static int dbg_op_ring_idx;
static const char str_dbg_op_dump[] = "  op[%d]: pos=%p opcode=0x%02x\n";
static const char str_dbg_op_hdr[] = "Last 32 opcodes before fatal:\n";
static const char str_dbg_scr_error[] = "DBG: VM_Execute script error recovery at fs=%p opcode=0x%02x msg=%s\n";
static const char str_dbg_castbool[] = "DBG: op 0x5f CastBool at fs=%p val=0x%x type=%d\n";

static void VM_DebugRecordOpcode(const char *pos, unsigned int opcode)
{
    unsigned int i = (unsigned int)dbg_op_ring_idx & 31u;
    dbg_op_ring[i * 2u] = (unsigned int)(uintptr_t)pos;
    dbg_op_ring[i * 2u + 1u] = opcode;
    dbg_op_ring_idx++;
}

#if !defined(VM_EXECUTE_USE_ASM_REFERENCE)
static unsigned int VM_Execute_CXX_Candidate_Pass66(struct function_stack_t fs);
static unsigned int VM_Execute(struct function_stack_t fs)
{
    return VM_Execute_CXX_Candidate_Pass66(fs);
}
#else
static __attribute__((naked)) unsigned int VM_Execute(struct function_stack_t fs)
{
    __asm__ __volatile__(
        "pushl %ebp\n"
        "movl %esp, %ebp\n"
        "pushl %edi\n"
        "pushl %esi\n"
        "pushl %ebx\n"
        "subl $0x5c, %esp\n"

        "movl g_script_error_level, %eax\n"
        "addl $1, %eax\n"
        "movl %eax, g_script_error_level\n"
        "leal (%eax, %eax, 8), %eax\n"
        "leal g_script_error(, %eax, 8), %eax\n"
        "movl %eax, (%esp)\n"
        "calll setjmp\n"
        "testl %eax, %eax\n"
        "jne .Lf840ae_00086a6e\n"
        "movl $0, -0x48(%ebp)\n"
        "movl $0, -0x44(%ebp)\n"
        "movl $0, -0x40(%ebp)\n"
        "movl $0, -0x38(%ebp)\n"
        "movl $0, -0x34(%ebp)\n"
        "movl $0, -0x30(%ebp)\n"
        "movl $0, -0x2c(%ebp)\n"
        "movl $0, -0x3c(%ebp)\n"
        ".Lf840ae_00084116:\n"
        "movl 8(%ebp), %ecx\n"
        ".Lf840ae_00084119:\n"
        "movzbl (%ecx), %edi\n"

        "pushl %eax\n"
        "movl dbg_op_ring_idx, %eax\n"
        "andl $31, %eax\n"
        "movl %ecx, dbg_op_ring(, %eax, 8)\n"
        "movl %edi, dbg_op_ring+4(, %eax, 8)\n"
        "addl $1, dbg_op_ring_idx\n"
        "popl %eax\n"

        "leal 1(%ecx), %esi\n"
        "movl %esi, 8(%ebp)\n"
        "cmpl $0x86, %edi\n"
        "ja .Lf840ae_00084155\n"
        ".Lf840ae_0008412a:\n"
        "jmpl *.Ljt_840ae_0(, %edi, 4)\n"
        ".Lf840ae_00084131:\n"
        "movl 0x14(%ebp), %eax\n"
        "addl $8, %eax\n"
        "movl %eax, 0x14(%ebp)\n"
        "movl $8, 4(%eax)\n"
        "movl 8(%ebp), %ecx\n"
        "movzbl (%ecx), %edi\n"

        "pushl %eax\n"
        "movl dbg_op_ring_idx, %eax\n"
        "andl $31, %eax\n"
        "movl %ecx, dbg_op_ring(, %eax, 8)\n"
        "movl %edi, dbg_op_ring+4(, %eax, 8)\n"
        "addl $1, dbg_op_ring_idx\n"
        "popl %eax\n"

        "leal 1(%ecx), %esi\n"
        "movl %esi, 8(%ebp)\n"
        "cmpl $0x86, %edi\n"
        "jbe .Lf840ae_0008412a\n"
        ".Lf840ae_00084155:\n"
        "movl 8(%ebp), %eax\n"
        "movl scrVmPub+12, %ecx\n"
        "movl %eax, (%ecx)\n"
        "movl 0x18(%ebp), %edx\n"
        "movl %edx, 0x10(%ecx)\n"
        "jmp .Lf840ae_000849a4\n"
        ".Lf840ae_0008415d:\n"
        "movzbl (%esi), %eax\n"
        "movl %eax, scrVmPub+28\n"
        "addl $1, 8(%ebp)\n"
        ".Lf840ae_00084169:\n"
        "movl 0x14(%ebp), %ecx\n"
        "leal -8(%ecx), %eax\n"
        "movl %eax, scrVmPub+16\n"

        "movl 8(%ebp), %eax\n"
        "movzwl (%eax), %edx\n"
        "addl $2, %eax\n"
        "movl %eax, 8(%ebp)\n"

        "movl %edx, %esi\n"
        "cmpl $1, 4(%ecx)\n"
        "je .Lf840ae_00085c23\n"
        ".Lf840ae_0008418c:\n"
        "movl %ecx, %eax\n"
        ".Lf840ae_0008418e:\n"
        "movl 4(%eax), %ebx\n"
        "movl (%eax), %eax\n"
        "movl %eax, 4(%esp)\n"
        "movl %ebx, (%esp)\n"
        "calll RemoveRefToValue\n"
        "movl imp_scrVarPub, %esi\n"
        "movl $0xffffffff, 0x14(%esi)\n"
        "movl imp_var_typename, %eax\n"
        "movl (%eax, %ebx, 4), %eax\n"
        "movl %eax, 4(%esp)\n"
        "movl $" %
            s is not an entity ", (%esp)\n"
                               "calll va\n"
                               "movl 0x10(%esi), %edi\n"
                               "testl %edi, %edi\n"
                               "je .Lf840ae_000861dd\n"
                               ".Lf840ae_000841cf:\n"
                               "cmpb $0, 0xc(%esi)\n"
                               "jne .Lf840ae_000841e4\n"
                               "movl imp_scrCompilePub, %eax\n"
                               "cmpb $0, 0x24(%eax)\n"
                               "je .Lf840ae_00086184\n"
                               ".Lf840ae_000841e4:\n"
                               "cmpb $0, scrVmPub+22\n"
                               "je .Lf840ae_0008420d\n"
                               ".Lf840ae_000841ed:\n"
                               "movl imp_scrVarPub, %eax\n"
                               "movl 0x10(%eax), %eax\n"
                               "movl %eax, 8(%esp)\n"
                               "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_0008420d:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl 4(%eax), %edx\n"
              "cmpl $5, %edx\n"
              "je .Lf840ae_00086101\n"
              "cmpl $6, %edx\n"
              "jne .Lf840ae_00086087\n"
              "movl (%eax), %eax\n"
              "leal (%eax, %eax, 4), %eax\n"
              "shll $2, %eax\n"
              "movl %eax, -0x40(%ebp)\n"
              ".Lf840ae_00084230:\n"
              "cmpl $0x00FFFFFE, %eax\n"
              "ja .Lf840ae_000843dd\n"
              "movl -0x40(%ebp), %edi\n"
              "testl %edi, %edi\n"
              "je .Lf840ae_0008424f\n"
              "rdtsc\n"
              "xorl %edx, %edx\n"
              "shrdl $2, %edx, %eax\n"
              "movl %eax, scrVmGlob+24\n"
              ".Lf840ae_0008424f:\n"
              "movl imp_scrVarPub, %ebx\n"
              "movl 0x18(%ebx), %eax\n"
              "addl %eax, -0x40(%ebp)\n"
              "andl $0x00FFFFFF, -0x40(%ebp)\n"
              "movl 0x14(%ebp), %ecx\n"
              "subl $8, %ecx\n"
              "movl %ecx, 0x14(%ebp)\n"
              "movl $0xa, -0x24(%ebp)\n"
              "movl %ecx, %eax\n"
              "subl 0x18(%ebp), %eax\n"
              "sarl $3, %eax\n"
              "leal 0xc(%ebp), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl 0x10(%ebp), %edx\n"
              "movl %edx, (%esp)\n"
              "movl 8(%ebp), %edx\n"
              "calll VM_ArchiveStack\n"
              "movl %eax, -0x28(%ebp)\n"
              "movl -0x40(%ebp), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl 0x1c(%ebx), %eax\n"
              "movl %eax, (%esp)\n"
              "calll GetVariable\n"
              "movl %eax, (%esp)\n"
              "calll GetArray\n"
              "movl 0xc(%ebp), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll GetNewObjectVariable\n"
              "leal -0x28(%ebp), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll SetNewVariableValue\n"
              "movl -0x40(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_SetThreadWaitTime\n"
              ".Lf840ae_000842dc:\n"
              "movl 0x18(%ebp), %eax\n"
              "movl $0, 0xc(%eax)\n"
              "movl -0x3c(%ebp), %edi\n"
              "testl %edi, %edi\n"
              "je .Lf840ae_0008451c\n"
              ".Lf840ae_000842f1:\n"
              "subl $1, -0x3c(%ebp)\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll RemoveRefToObject\n"
              "movl scrVmPub+12, %edx\n"
              "movl (%edx), %eax\n"
              "movl %eax, 8(%ebp)\n"
              "movl 4(%edx), %eax\n"
              "movl %eax, 0xc(%ebp)\n"
              "movl 8(%edx), %eax\n"
              "movl %eax, 0x10(%ebp)\n"
              "movl 0xc(%edx), %ecx\n"
              "movl %ecx, 0x14(%ebp)\n"
              "movl 0x10(%edx), %eax\n"
              "movl %eax, 0x18(%ebp)\n"
              "movl 0x14(%edx), %eax\n"
              "movl %eax, 4(%ecx)\n"
              "addl $8, 0x14(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00084335:\n"
              "movzbl (%esi), %eax\n"
              "movl %eax, scrVmPub+28\n"
              "addl $1, 8(%ebp)\n"
              ".Lf840ae_00084341:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, scrVmPub+16\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %ecx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl scrVmPub+12, %edx\n"
              "movl %eax, (%edx)\n"
              "movl imp_scrCompilePub, %eax\n"

              "cmpl $0x4000, %ecx\n"
              "jb .Ldbg_builtin_inrange\n"

              "pushl %ecx\n"
              "pushl $str_dbg_op_hdr\n"
              "calll Com_Printf\n"
              "addl $4, %esp\n"

              "xorl %eax, %eax\n"
              ".Ldbg_dump_loop:\n"
              "pushl %eax\n"
              "movl dbg_op_ring_idx, %edx\n"
              "addl %eax, %edx\n"
              "andl $31, %edx\n"
              "pushl dbg_op_ring+4(, %edx, 8)\n"
              "pushl dbg_op_ring(, %edx, 8)\n"
              "pushl %eax\n"
              "pushl $str_dbg_op_dump\n"
              "calll Com_Printf\n"
              "addl $16, %esp\n"
              "popl %eax\n"
              "addl $1, %eax\n"
              "cmpl $32, %eax\n"
              "jl .Ldbg_dump_loop\n"
              "popl %ecx\n"
              "movl $1, (%esp)\n"
              "calll _exit\n"
              ".Ldbg_builtin_inrange:\n"
              "movl 0x38(%eax, %ecx, 4), %eax\n"
              "testl %eax, %eax\n"
              "jne .Ldbg_builtin_ok\n"

              "pushl %ecx\n"
              "pushl 8(%ebp)\n"
              "pushl %ecx\n"
              "pushl $str_dbg_null_builtin\n"
              "calll Com_Printf\n"
              "addl $12, %esp\n"
              "popl %ecx\n"

              "pushl %ecx\n"
              "pushl $str_dbg_op_hdr\n"
              "calll Com_Printf\n"
              "addl $4, %esp\n"
              "xorl %eax, %eax\n"
              ".Ldbg_dump_loop2:\n"
              "pushl %eax\n"
              "movl dbg_op_ring_idx, %edx\n"
              "addl %eax, %edx\n"
              "andl $31, %edx\n"
              "pushl dbg_op_ring+4(, %edx, 8)\n"
              "pushl dbg_op_ring(, %edx, 8)\n"
              "pushl %eax\n"
              "pushl $str_dbg_op_dump\n"
              "calll Com_Printf\n"
              "addl $16, %esp\n"
              "popl %eax\n"
              "addl $1, %eax\n"
              "cmpl $32, %eax\n"
              "jl .Ldbg_dump_loop2\n"
              "popl %ecx\n"
              "movl $1, (%esp)\n"
              "calll _exit\n"
              ".Ldbg_builtin_ok:\n"
              "calll *%eax\n"
              ".Lf840ae_00084366:\n"
              "movl scrVmPub+16, %edx\n"
              "movl %edx, 0x14(%ebp)\n"
              "movl scrVmPub+12, %eax\n"
              "movl (%eax), %eax\n"
              "movl %eax, 8(%ebp)\n"
              "movl scrVmPub+28, %esi\n"
              "testl %esi, %esi\n"
              "je .Lf840ae_000843bd\n"
              "movl $0, scrVmPub+28\n"
              "leal (, %esi, 8), %eax\n"
              "subl %eax, %edx\n"
              "movl %edx, scrVmPub+16\n"
              "xorl %ebx, %ebx\n"
              ".Lf840ae_0008439e:\n"
              "movl 0x14(%ebp), %eax\n"

              "movl (%eax), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl 4(%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll RemoveRefToValue\n"

              "subl $8, 0x14(%ebp)\n"
              "addl $1, %ebx\n"
              "cmpl %ebx, %esi\n"
              "jne .Lf840ae_0008439e\n"
              ".Lf840ae_000843bd:\n"
              "movl scrVmPub+24, %ebx\n"
              "testl %ebx, %ebx\n"
              "je .Lf840ae_0008452e\n"
              "movl $0, scrVmPub+24\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000843dd:\n"
              "movl imp_scrVarPub, %edx\n"
              "movl $2, 0x14(%edx)\n"
              "movl -0x40(%ebp), %ebx\n"
              "testl %ebx, %ebx\n"
              "js .Lf840ae_00084444\n"
              "movl 0x10(%edx), %ecx\n"
              "testl %ecx, %ecx\n"
              "je .Lf840ae_00086492\n"
              ".Lf840ae_000843fc:\n"
              "cmpb $0, 0xc(%edx)\n"
              "jne .Lf840ae_00084411\n"
              "movl imp_scrCompilePub, %eax\n"
              "cmpb $0, 0x24(%eax)\n"
              "je .Lf840ae_00086263\n"
              ".Lf840ae_00084411:\n"
              "cmpb $0, scrVmPub+22\n"
              "je .Lf840ae_00086167\n"
              ".Lf840ae_0008441e:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              "movl imp_scrVarPub, %edx\n"
              ".Lf840ae_00084444:\n"
              "movl 0x10(%edx), %edi\n"
              "testl %edi, %edi\n"
              "je .Lf840ae_00086178\n"
              ".Lf840ae_0008444f:\n"
              "cmpb $0, 0xc(%edx)\n"
              "jne .Lf840ae_00084464\n"
              "movl imp_scrCompilePub, %eax\n"
              "cmpb $0, 0x24(%eax)\n"
              "je .Lf840ae_000861e5\n"
              ".Lf840ae_00084464:\n"
              "cmpb $0, scrVmPub+22\n"
              "je .Lf840ae_0008448d\n"
              ".Lf840ae_0008446d:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_0008448d:\n"
              "movl $0xa, -0x24(%ebp)\n"
              "movl 0x14(%ebp), %ecx\n"
              "movl %ecx, %eax\n"
              "subl 0x18(%ebp), %eax\n"
              "sarl $3, %eax\n"
              "leal 0xc(%ebp), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl 0x10(%ebp), %edx\n"
              "movl %edx, (%esp)\n"
              "movl 8(%ebp), %edx\n"
              "calll VM_ArchiveStack\n"
              "movl %eax, -0x28(%ebp)\n"
              "movl imp_scrVarPub, %ebx\n"
              "movl 0x18(%ebx), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl 0x1c(%ebx), %eax\n"
              "movl %eax, (%esp)\n"
              "calll GetVariable\n"
              "movl %eax, (%esp)\n"
              "calll GetArray\n"
              "movl 0xc(%ebp), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll GetNewObjectVariableReverse\n"
              "leal -0x28(%ebp), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll SetNewVariableValue\n"
              "movl 0x18(%ebx), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_SetThreadWaitTime\n"
              "movl 0x18(%ebp), %eax\n"
              "movl $0, 0xc(%eax)\n"
              "movl -0x3c(%ebp), %edi\n"
              "testl %edi, %edi\n"
              "jne .Lf840ae_000842f1\n"
              ".Lf840ae_0008451c:\n"

              "movl 0x18(%ebp), %edx\n"
              "movl %edx, scrVmPub+16\n"
              "subl $1, g_script_error_level\n"
              "movl 0xc(%ebp), %eax\n"

              "addl $0x5c, %esp\n"
              "popl %ebx\n"
              "popl %esi\n"
              "popl %edi\n"
              "popl %ebp\n"
              "retl\n"

              ".Lf840ae_0008452e:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $0, 4(%eax)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00084546:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x2c(%eax), %eax\n"
              "movl %eax, -0x44(%ebp)\n"

              ".Lf840ae_00084551:\n"
              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, 4(%esp)\n"
              "movl -0x44(%ebp), %edx\n"
              "movl %edx, (%esp)\n"
              "calll Scr_GetVariableField\n"
              "movl %eax, -0x48(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00084577:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $8, 4(%eax)\n"
              ".Lf840ae_00084587:\n"
              "cmpl $0x1e, scrVmPub+8\n"
              "jle .Lf840ae_000862da\n"
              "movl imp_scrVarPub, %edx\n"
              "movl 0x10(%edx), %ecx\n"
              "testl %ecx, %ecx\n"
              "je .Lf840ae_000866d2\n"
              ".Lf840ae_000845a5:\n"
              "cmpb $0, 0xc(%edx)\n"
              "jne .Lf840ae_000845ba\n"
              "movl imp_scrCompilePub, %eax\n"
              "cmpb $0, 0x24(%eax)\n"
              "je .Lf840ae_000865ce\n"
              ".Lf840ae_000845ba:\n"
              "cmpb $0, scrVmPub+22\n"
              "je .Lf840ae_000845e3\n"
              ".Lf840ae_000845c3:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_000845e3:\n"
              "movl 0x14(%ebp), %eax\n"
              "cmpl $9, 4(%eax)\n"
              "jne .Lf840ae_00084652\n"
              "cmpl $0x1e, scrVmPub+8\n"
              "jle .Lf840ae_000865f7\n"
              "movl imp_scrVarPub, %edx\n"
              "movl $1, 0x14(%edx)\n"
              "movl 0x10(%edx), %eax\n"
              "testl %eax, %eax\n"
              "je .Lf840ae_000869ea\n"
              ".Lf840ae_00084611:\n"
              "cmpb $0, 0xc(%edx)\n"
              "jne .Lf840ae_00084626\n"
              "movl imp_scrCompilePub, %eax\n"
              "cmpb $0, 0x24(%eax)\n"
              "je .Lf840ae_00086774\n"
              ".Lf840ae_00084626:\n"
              "cmpb $0, scrVmPub+22\n"
              "je .Lf840ae_0008464f\n"
              ".Lf840ae_0008462f:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_0008464f:\n"
              "movl 0x14(%ebp), %eax\n"
              ".Lf840ae_00084652:\n"
              "movl 4(%eax), %edx\n"
              "movl imp_var_typename, %eax\n"
              "movl (%eax, %edx, 4), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl $" %
            s is not a function pointer ", (%esp)\n"
                                        "calll va\n"
                                        "movl imp_scrVarPub, %edx\n"
                                        "movl 0x10(%edx), %ecx\n"
                                        "testl %ecx, %ecx\n"
                                        "je .Lf840ae_000864ec\n"
                                        ".Lf840ae_0008467e:\n"
                                        "cmpb $0, 0xc(%edx)\n"
                                        "jne .Lf840ae_00084693\n"
                                        "movl imp_scrCompilePub, %eax\n"
                                        "cmpb $0, 0x24(%eax)\n"
                                        "je .Lf840ae_00086338\n"
                                        ".Lf840ae_00084693:\n"
                                        "cmpb $0, scrVmPub+22\n"
                                        "je .Lf840ae_000846bc\n"
                                        ".Lf840ae_0008469c:\n"
                                        "movl imp_scrVarPub, %eax\n"
                                        "movl 0x10(%eax), %eax\n"
                                        "movl %eax, 8(%esp)\n"
                                        "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_000846bc:\n"
              "movl 0x14(%ebp), %edx\n"
              "cmpl $1, 4(%edx)\n"
              "jne .Lf840ae_00086e60\n"
              "cmpl $0x1e, scrVmPub+8\n"
              "jg .Lf840ae_00086e0e\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl (%edx), %eax\n"
              "movl %eax, (%esp)\n"
              "calll AllocChildThread\n"
              "movl %eax, 0xc(%ebp)\n"
              "subl $8, 0x14(%ebp)\n"
              ".Lf840ae_000846ee:\n"
              "movl 8(%ebp), %edx\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, (%eax)\n"
              "movl scrVmPub+12, %edx\n"

              "movl (%edx), %eax\n"
              "movl (%eax), %ecx\n"
              "addl $4, %eax\n"
              "movl %eax, (%edx)\n"

              "movl %ecx, 8(%ebp)\n"
              ".Lf840ae_0008470a:\n"
              "movl scrVmPub+12, %eax\n"
              "cmpl $0, 0xc(%eax)\n"
              "jne .Lf840ae_frame_stack_saved\n"
              "movl 0x14(%ebp), %edx\n"
              "movl %edx, 0xc(%eax)\n"
              "movl 0x18(%ebp), %ecx\n"
              "movl %ecx, 0x10(%eax)\n"
              "movl 4(%edx), %edx\n"
              "movl %edx, 0x14(%eax)\n"
              ".Lf840ae_frame_stack_saved:\n"
              "movl 0x10(%ebp), %edx\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, 8(%eax)\n"
              "movl $0, 0x10(%ebp)\n"
              "addl $1, scrVmPub+8\n"
              "movl scrVmPub+12, %edx\n"
              "addl $0x18, %edx\n"
              "movl %edx, scrVmPub+12\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, 4(%edx)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00084740:\n"
              "movl (%edx), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl %ecx, (%esp)\n"
              "calll RemoveRefToValue\n"
              "movl 0x14(%ebp), %eax\n"
              "subl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl 0xc(%eax), %edx\n"
              "movl imp_var_typename, %eax\n"
              "movl (%eax, %edx, 4), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl $" %
            s is not a function pointer ", (%esp)\n"
                                        "calll va\n"
                                        "movl imp_scrVarPub, %edx\n"
                                        "movl 0x10(%edx), %ecx\n"
                                        "testl %ecx, %ecx\n"
                                        "je .Lf840ae_00086b0f\n"
                                        ".Lf840ae_00084783:\n"
                                        "cmpb $0, 0xc(%edx)\n"
                                        "jne .Lf840ae_00086306\n"
                                        "movl imp_scrCompilePub, %eax\n"
                                        "cmpb $0, 0x24(%eax)\n"
                                        "jne .Lf840ae_00086306\n"
                                        "cmpb $0, 0xa(%edx)\n"
                                        "je .Lf840ae_000847bf\n"
                                        "movl $1, %eax\n"
                                        "movl scrVmGlob+20, %esi\n"
                                        "testl %esi, %esi\n"
                                        "movzbl scrVmPub+22, %edx\n"
                                        "cmovel %edx, %eax\n"
                                        "movb %al, scrVmPub+22\n"
                                        ".Lf840ae_000847bf:\n"
                                        "movl scrVmPub+8, %ebx\n"
                                        "testl %ebx, %ebx\n"
                                        "jne .Lf840ae_000861be\n"
                                        "cmpb $0, scrVmPub+20\n"
                                        "jne .Lf840ae_000861be\n"
                                        "movl imp_scrVarPub, %eax\n"
                                        "movl 0x10(%eax), %eax\n"
                                        "movl %eax, 8(%esp)\n"
                                        "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_000847fa:\n"
              "cmpl $0x1e, scrVmPub+8\n"
              "jle .Lf840ae_0008649e\n"
              "movl imp_scrVarPub, %edx\n"
              "movl $1, 0x14(%edx)\n"
              "movl 0x10(%edx), %ecx\n"
              "testl %ecx, %ecx\n"
              "je .Lf840ae_000869b7\n"
              ".Lf840ae_0008481f:\n"
              "cmpb $0, 0xc(%edx)\n"
              "jne .Lf840ae_00084834\n"
              "movl imp_scrCompilePub, %eax\n"
              "cmpb $0, 0x24(%eax)\n"
              "je .Lf840ae_000866e6\n"
              ".Lf840ae_00084834:\n"
              "cmpb $0, scrVmPub+22\n"
              "je .Lf840ae_0008485d\n"
              ".Lf840ae_0008483d:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_0008485d:\n"
              "movl 0x14(%ebp), %eax\n"
              "cmpl $9, 4(%eax)\n"
              "jne .Lf840ae_000848cc\n"
              "cmpl $0x1e, scrVmPub+8\n"
              "jle .Lf840ae_00086651\n"
              "movl imp_scrVarPub, %edx\n"
              "movl $1, 0x14(%edx)\n"
              "movl 0x10(%edx), %eax\n"
              "testl %eax, %eax\n"
              "je .Lf840ae_00086a56\n"
              ".Lf840ae_0008488b:\n"
              "cmpb $0, 0xc(%edx)\n"
              "jne .Lf840ae_000848a0\n"
              "movl imp_scrCompilePub, %eax\n"
              "cmpb $0, 0x24(%eax)\n"
              "je .Lf840ae_00086879\n"
              ".Lf840ae_000848a0:\n"
              "cmpb $0, scrVmPub+22\n"
              "je .Lf840ae_000848c9\n"
              ".Lf840ae_000848a9:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_000848c9:\n"
              "movl 0x14(%ebp), %eax\n"
              ".Lf840ae_000848cc:\n"
              "movl 4(%eax), %edx\n"
              "movl imp_var_typename, %eax\n"
              "movl (%eax, %edx, 4), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl $" %
            s is not a function pointer ", (%esp)\n"
                                        "calll va\n"
                                        "movl imp_scrVarPub, %edx\n"
                                        "movl 0x10(%edx), %ecx\n"
                                        "testl %ecx, %ecx\n"
                                        "je .Lf840ae_00086649\n"
                                        ".Lf840ae_000848f8:\n"
                                        "cmpb $0, 0xc(%edx)\n"
                                        "jne .Lf840ae_0008490d\n"
                                        "movl imp_scrCompilePub, %eax\n"
                                        "cmpb $0, 0x24(%eax)\n"
                                        "je .Lf840ae_000864c3\n"
                                        ".Lf840ae_0008490d:\n"
                                        "cmpb $0, scrVmPub+22\n"
                                        "je .Lf840ae_00084936\n"
                                        ".Lf840ae_00084916:\n"
                                        "movl imp_scrVarPub, %eax\n"
                                        "movl 0x10(%eax), %eax\n"
                                        "movl %eax, 8(%esp)\n"
                                        "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_00084936:\n"
              "movl 0x14(%ebp), %eax\n"
              "cmpl $1, 4(%eax)\n"
              "jne .Lf840ae_00086bf4\n"
              "cmpl $0x1e, scrVmPub+8\n"
              "jg .Lf840ae_00086b9b\n"
              "movl (%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll AllocThread\n"
              "movl %eax, 0xc(%ebp)\n"
              "subl $8, 0x14(%ebp)\n"
              ".Lf840ae_00084961:\n"
              "movl 8(%ebp), %edx\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, (%eax)\n"
              "movl 0x18(%ebp), %edx\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, 0x10(%eax)\n"
              "movl scrVmPub+12, %edx\n"

              "movl (%edx), %eax\n"
              "movl (%eax), %ecx\n"
              "addl $4, %eax\n"
              "movl %eax, (%edx)\n"

              "movl %ecx, 8(%ebp)\n"
              "movl 0x14(%ebp), %ecx\n"
              ".Lf840ae_0008498b:\n"
              "movl scrVmPub+12, %ebx\n"

              "movl (%ebx), %eax\n"
              "movl (%eax), %edx\n"
              "addl $4, %eax\n"
              "movl %eax, (%ebx)\n"

              "shll $3, %edx\n"
              "subl %edx, %ecx\n"
              "movl %ecx, 0x18(%ebp)\n"
              "movl %ecx, %edx\n"
              ".Lf840ae_000849a4:\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, 0xc(%eax)\n"
              "movl 4(%edx), %edx\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, 0x14(%eax)\n"
              "movl 0x18(%ebp), %eax\n"
              "movl $8, 4(%eax)\n"
              "addl $1, -0x3c(%ebp)\n"
              "jmp .Lf840ae_0008470a\n"
              ".Lf840ae_000849ca:\n"
              "movzbl (%esi), %eax\n"
              "shll $2, %eax\n"
              "movl scrVmPub, %edx\n"
              "subl %eax, %edx\n"
              "movl (%edx), %edx\n"
              "movl %edx, -0x48(%ebp)\n"
              ".Lf840ae_000849dd:\n"
              "leal 2(%ecx), %eax\n"
              "movl %eax, 8(%ebp)\n"
              "movl %eax, %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000849ea:\n"
              "movl scrVmPub, %ebx\n"
              "leal 4(%ebx), %eax\n"
              "movl %eax, scrVmPub\n"
              "addl $1, 0x10(%ebp)\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, 4(%esp)\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll GetNewVariable\n"
              "movl %eax, 4(%ebx)\n"
              ".Lf840ae_00084a1a:\n"
              "movl 0x14(%ebp), %eax\n"
              "cmpl $8, 4(%eax)\n"
              "je .Lf840ae_00084116\n"
              ".Lf840ae_00084a27:\n"
              "movl %eax, 4(%esp)\n"
              "movl scrVmPub, %eax\n"
              "movl (%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll SetVariableValue\n"
              ".Lf840ae_00084a3a:\n"
              "subl $8, 0x14(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00084a46:\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_GetSelf\n"
              "movl %eax, -0x44(%ebp)\n"
              "movl %eax, (%esp)\n"
              "calll IsFieldObject\n"
              "testb %al, %al\n"
              "je .Lf840ae_00086f27\n"
              ".Lf840ae_00084a64:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, 4(%esp)\n"
              "movl -0x44(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_FindVariableField\n"
              "movl %eax, 8(%ebx)\n"
              "movl %edx, 0xc(%ebx)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00084a96:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl 8(%ebp), %eax\n"
              "movzbl (%eax), %edx\n"
              "shll $2, %edx\n"
              "movl scrVmPub, %eax\n"
              "subl %edx, %eax\n"
              "movl (%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalVariable\n"
              "movl %eax, 8(%ebx)\n"
              "movl %edx, 0xc(%ebx)\n"
              "addl $1, 8(%ebp)\n"
              ".Lf840ae_00084ac3:\n"
              "movl 0x14(%ebp), %edx\n"
              "leal -8(%edx), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl %edx, (%esp)\n"
              "calll Scr_EvalArray\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00084ada:\n"
              "movl imp_var_typename, %eax\n"
              "movl (%eax, %edx, 4), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl $" --must be applied to an int(applied to % s) ", (%esp)\n"
                                                                   "calll va\n"
                                                                   "movl imp_scrVarPub, %edx\n"
                                                                   "movl 0x10(%edx), %ecx\n"
                                                                   "testl %ecx, %ecx\n"
                                                                   "je .Lf840ae_00086a4e\n"
                                                                   ".Lf840ae_00084b03:\n"
                                                                   "cmpb $0, 0xc(%edx)\n"
                                                                   "jne .Lf840ae_00084b18\n"
                                                                   "movl imp_scrCompilePub, %eax\n"
                                                                   "cmpb $0, 0x24(%eax)\n"
                                                                   "je .Lf840ae_00086731\n"
                                                                   ".Lf840ae_00084b18:\n"
                                                                   "cmpb $0, scrVmPub+22\n"
                                                                   "je .Lf840ae_00084b41\n"
                                                                   ".Lf840ae_00084b21:\n"
                                                                   "movl imp_scrVarPub, %eax\n"
                                                                   "movl 0x10(%eax), %eax\n"
                                                                   "movl %eax, 8(%esp)\n"
                                                                   "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_00084b41:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalOr\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00084b58:\n"
              "movl 0x14(%ebp), %eax\n"
              "cmpl $7, 4(%eax)\n"
              "jne .Lf840ae_0008533e\n"
              "movzbl (%esi), %eax\n"
              "shll $2, %eax\n"
              "movl scrVmPub, %edx\n"
              "subl %eax, %edx\n"
              "movl (%edx), %eax\n"
              "movl %eax, (%esp)\n"
              "calll ClearVariableValue\n"
              ".Lf840ae_00084b7d:\n"
              "addl $1, 8(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00084b89:\n"
              "movl imp_var_typename, %eax\n"
              "movl (%eax, %edx, 4), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl $" ++must be applied to an int(applied to % s) ", (%esp)\n"
                                                                   "calll va\n"
                                                                   "movl imp_scrVarPub, %edx\n"
                                                                   "movl 0x10(%edx), %edi\n"
                                                                   "testl %edi, %edi\n"
                                                                   "je .Lf840ae_00086b2b\n"
                                                                   ".Lf840ae_00084bb2:\n"
                                                                   "cmpb $0, 0xc(%edx)\n"
                                                                   "jne .Lf840ae_00084bc7\n"
                                                                   "movl imp_scrCompilePub, %eax\n"
                                                                   "cmpb $0, 0x24(%eax)\n"
                                                                   "je .Lf840ae_00086952\n"
                                                                   ".Lf840ae_00084bc7:\n"
                                                                   "cmpb $0, scrVmPub+22\n"
                                                                   "je .Lf840ae_00084bf0\n"
                                                                   ".Lf840ae_00084bd0:\n"
                                                                   "movl imp_scrVarPub, %eax\n"
                                                                   "movl 0x10(%eax), %eax\n"
                                                                   "movl %eax, 8(%esp)\n"
                                                                   "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_00084bf0:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl -0x48(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalVariableField\n"
              "movl %eax, 8(%ebx)\n"
              "movl %edx, 0xc(%ebx)\n"
              "movl 0x14(%ebp), %eax\n"
              "movl 4(%eax), %edx\n"
              "cmpl $6, %edx\n"
              "jne .Lf840ae_00084ada\n"
              "subl $1, (%eax)\n"
              "addl $1, 8(%ebp)\n"
              ".Lf840ae_00084c20:\n"
              "movl -0x48(%ebp), %edx\n"
              ".Lf840ae_00084c23:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl %edx, (%esp)\n"
              "calll SetVariableFieldValue\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00084c37:\n"
              "movl 0x14(%ebp), %eax\n"
              "cmpl $8, 4(%eax)\n"
              "je .Lf840ae_000866c3\n"
              "movl imp_scrVarPub, %edx\n"
              "movl 0x10(%edx), %eax\n"
              "testl %eax, %eax\n"
              "je .Lf840ae_00086a62\n"
              ".Lf840ae_00084c55:\n"
              "cmpb $0, 0xc(%edx)\n"
              "jne .Lf840ae_00084c6a\n"
              "movl imp_scrCompilePub, %eax\n"
              "cmpb $0, 0x24(%eax)\n"
              "je .Lf840ae_000868bc\n"
              ".Lf840ae_00084c6a:\n"
              "cmpb $0, scrVmPub+22\n"
              "je .Lf840ae_00084c93\n"
              ".Lf840ae_00084c73:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_00084c93:\n"
              "movl scrVmPub, %eax\n"
              "movl (%eax), %eax\n"
              "movl %eax, -0x48(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00084ca5:\n"
              "movl -0x38(%ebp), %ebx\n"
              "testl %ebx, %ebx\n"
              "je .Lf840ae_00084cd0\n"
              "xorl %ebx, %ebx\n"

              ".Lf840ae_00084cae:\n"
              "movl 8(%ebp), %eax\n"
              "movl (%eax), %ecx\n"
              "leal 4(%eax), %edx\n"
              "movl %edx, 8(%ebp)\n"

              "movl %ecx, -0x30(%ebp)\n"

              "movl 4(%eax), %edx\n"
              "addl $8, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, -0x2c(%ebp)\n"
              "addl $1, %ebx\n"
              "cmpl %ebx, -0x38(%ebp)\n"
              "jne .Lf840ae_00084cae\n"
              ".Lf840ae_00084cd0:\n"
              "movl -0x30(%ebp), %esi\n"
              "testl %esi, %esi\n"
              "jne .Lf840ae_00084cdd\n"
              "movl -0x2c(%ebp), %edx\n"
              "movl %edx, 8(%ebp)\n"
              ".Lf840ae_00084cdd:\n"
              "movl $0, -0x38(%ebp)\n"
              ".Lf840ae_00084ce4:\n"
              "movl 0x14(%ebp), %eax\n"

              "movl (%eax), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl 4(%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll RemoveRefToValue\n"
              "jmp .Lf840ae_00084a3a\n"

              ".Lf840ae_00084cfd:\n"
              "movl scrVmGlob+20, %edi\n"
              "testl %edi, %edi\n"
              "jne .Lf840ae_000869f6\n"
              "cmpb $0, scrVmPub+21\n"
              "je .Lf840ae_000867b7\n"
              "calll Scr_DumpScriptThreads\n"
              "calll Scr_DumpScriptVariables\n"
              "movb $1, scrVmPub+22\n"
              "movl imp_scrVarPub, %edx\n"
              "movl 0x10(%edx), %ecx\n"
              "testl %ecx, %ecx\n"
              "je .Lf840ae_00086b8f\n"
              ".Lf840ae_00084d3a:\n"
              "cmpb $0, 0xc(%edx)\n"
              "jne .Lf840ae_00084d4f\n"
              "movl imp_scrCompilePub, %eax\n"
              "cmpb $0, 0x24(%eax)\n"
              "je .Lf840ae_00086b45\n"
              ".Lf840ae_00084d4f:\n"
              "cmpb $0, scrVmPub+22\n"
              "je .Lf840ae_00084d78\n"
              ".Lf840ae_00084d58:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_00084d78:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl -0x48(%ebp), %edx\n"
              "movl %edx, (%esp)\n"
              "calll Scr_EvalVariableField\n"
              "movl %eax, 8(%ebx)\n"
              "movl %edx, 0xc(%ebx)\n"
              "movl 0x14(%ebp), %eax\n"
              "movl 4(%eax), %edx\n"
              "cmpl $6, %edx\n"
              "jne .Lf840ae_00084b89\n"
              "addl $1, (%eax)\n"
              "addl $1, 8(%ebp)\n"
              "movl -0x48(%ebp), %edx\n"
              "jmp .Lf840ae_00084c23\n"
              ".Lf840ae_00084db0:\n"
              "movl -0x48(%ebp), %edx\n"
              ".Lf840ae_00084db3:\n"
              "movl 0x14(%ebp), %ebx\n"
              "movl %edx, (%esp)\n"
              "calll Scr_EvalArrayRef\n"
              "movl %ebx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalArrayIndex\n"
              "movl %eax, -0x48(%ebp)\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00084dd2:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl 8(%ebp), %eax\n"
              "movl (%eax), %ecx\n"
              "leal 4(%eax), %edx\n"
              "movl %edx, 8(%ebp)\n"
              "movl 4(%eax), %edx\n"
              "addl $8, %eax\n"
              "movl %eax, 8(%ebp)\n"
              "movl %ecx, 4(%esp)\n"
              "movl %edx, (%esp)\n"
              "calll FindEntityId\n"
              "movl %eax, 8(%ebx)\n"
              "movl 0x14(%ebp), %eax\n"
              "movl (%eax), %edx\n"
              "testl %edx, %edx\n"
              "jne .Lf840ae_00084e9d\n"
              "movl $0, 4(%eax)\n"
              "movl imp_scrVarPub, %edx\n"
              "movl 0x10(%edx), %edi\n"
              "testl %edi, %edi\n"
              "je .Lf840ae_00086b1f\n"
              ".Lf840ae_00084e23:\n"
              "cmpb $0, 0xc(%edx)\n"
              "jne .Lf840ae_00085fea\n"
              "movl imp_scrCompilePub, %eax\n"
              "cmpb $0, 0x24(%eax)\n"
              "jne .Lf840ae_00085fea\n"
              "cmpb $0, 0xa(%edx)\n"
              "je .Lf840ae_00084e5f\n"
              "movl $1, %eax\n"
              "movl scrVmGlob+20, %esi\n"
              "testl %esi, %esi\n"
              "movzbl scrVmPub+22, %edx\n"
              "cmovel %edx, %eax\n"
              "movb %al, scrVmPub+22\n"
              ".Lf840ae_00084e5f:\n"
              "movl scrVmPub+8, %ebx\n"
              "testl %ebx, %ebx\n"
              "jne .Lf840ae_000861be\n"
              "cmpb $0, scrVmPub+20\n"
              "jne .Lf840ae_000861be\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_00084e9a:\n"
              "movl 0x14(%ebp), %eax\n"
              ".Lf840ae_00084e9d:\n"
              "movl $1, 4(%eax)\n"
              "jmp .Lf840ae_00085162\n"
              ".Lf840ae_00084ea9:\n"
              "movl 0x14(%ebp), %ecx\n"
              "leal 8(%ecx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, 8(%ecx)\n"
              "jmp .Lf840ae_00084e9a\n"
              ".Lf840ae_00084ec3:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, 4(%esp)\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll FindVariable\n"
              ".Lf840ae_00084ee7:\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalVariable\n"
              "movl %eax, 8(%ebx)\n"
              "movl %edx, 0xc(%ebx)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"

              ".Lf840ae_00084efd:\n"
              "movzwl (%esi), %eax\n"
              "leal 3(%ecx), %edx\n"
              "movl %edx, 8(%ebp)\n"

              "movl %eax, 4(%esp)\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll FindVariable\n"
              "movl %eax, -0x48(%ebp)\n"
              "testl %eax, %eax\n"
              "jne .Lf840ae_00084116\n"
              "movl imp_scrVarPub, %edx\n"
              "movl 0x10(%edx), %ecx\n"
              "testl %ecx, %ecx\n"
              "je .Lf840ae_00086b03\n"
              ".Lf840ae_00084f31:\n"
              "cmpb $0, 0xc(%edx)\n"
              "jne .Lf840ae_0008604e\n"
              "movl imp_scrCompilePub, %eax\n"
              "cmpb $0, 0x24(%eax)\n"
              "jne .Lf840ae_0008604e\n"
              "cmpb $0, 0xa(%edx)\n"
              "je .Lf840ae_00084f6d\n"
              "movl $1, %eax\n"
              "movl scrVmGlob+20, %edx\n"
              "testl %edx, %edx\n"
              "movzbl scrVmPub+22, %edx\n"
              "cmovel %edx, %eax\n"
              "movb %al, scrVmPub+22\n"
              ".Lf840ae_00084f6d:\n"
              "movl scrVmPub+8, %eax\n"
              "testl %eax, %eax\n"
              "jne .Lf840ae_000861be\n"
              "cmpb $0, scrVmPub+20\n"
              "jne .Lf840ae_000861be\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              "movl $0, -0x48(%ebp)\n"
              ".Lf840ae_00084fae:\n"
              "addl $1, 8(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00084fba:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $6, 4(%eax)\n"
              "movl 8(%ebp), %eax\n"
              "movzbl (%eax), %edx\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %edx, (%eax)\n"
              "addl $1, 8(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00084fe1:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $6, 4(%eax)\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl 0x14(%ebp), %eax\n"
              "movl %edx, (%eax)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_0008500a:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $6, 4(%eax)\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "negl %edx\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %edx, (%eax)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085035:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $6, 4(%eax)\n"

              ".Lf840ae_00085045:\n"
              "movl 8(%ebp), %eax\n"
              "movl (%eax), %edx\n"
              "addl $4, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl 0x14(%ebp), %eax\n"
              "movl %edx, (%eax)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_0008505d:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $5, 4(%eax)\n"

              "movl 8(%ebp), %eax\n"
              "movl (%eax), %edx\n"
              "addl $4, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl 0x14(%ebp), %eax\n"
              "movl %edx, (%eax)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085085:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $6, 4(%eax)\n"
              "movl 8(%ebp), %eax\n"
              "movzbl (%eax), %eax\n"
              "negl %eax\n"
              "movl 0x14(%ebp), %edx\n"
              "movl %eax, (%edx)\n"
              "addl $1, 8(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000850ae:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $2, 4(%eax)\n"

              ".Lf840ae_000850be:\n"
              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl 0x14(%ebp), %eax\n"
              "movl %edx, (%eax)\n"
              "movl 0x14(%ebp), %eax\n"
              "movl (%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll SL_AddRefToString\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000850e4:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $3, 4(%eax)\n"
              "jmp .Lf840ae_000850be\n"
              ".Lf840ae_000850f6:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $4, 4(%eax)\n"
              "movl 8(%ebp), %edx\n"
              "leal 0xc(%edx), %eax\n"
              "movl %eax, 8(%ebp)\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %edx, (%eax)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_0008511c:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x24(%eax), %eax\n"
              "movl %eax, -0x44(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_0008512f:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x2c(%eax), %eax\n"
              "movl %eax, -0x44(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085142:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $1, 4(%eax)\n"
              "movl 0x14(%ebp), %ebx\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_GetSelf\n"
              "movl %eax, (%ebx)\n"
              ".Lf840ae_00085162:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl (%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll AddRefToObject\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085177:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $1, 4(%eax)\n"
              "movl imp_scrVarPub, %ecx\n"
              "movl 0x24(%ecx), %edx\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %edx, (%eax)\n"
              "movl 0x24(%ecx), %eax\n"
              "movl %eax, (%esp)\n"
              "calll AddRefToObject\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000851a8:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x28(%eax), %eax\n"
              "jmp .Lf840ae_00084ee7\n"
              ".Lf840ae_000851be:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $1, 4(%eax)\n"
              "movl imp_scrVarPub, %ecx\n"
              "movl 0x2c(%ecx), %edx\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %edx, (%eax)\n"
              "movl 0x2c(%ecx), %eax\n"
              "movl %eax, (%esp)\n"
              "calll AddRefToObject\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000851ef:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $0xb, 4(%eax)\n"
              "jmp .Lf840ae_00085045\n"
              ".Lf840ae_00085204:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x28(%eax), %eax\n"
              "movl %eax, -0x48(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085217:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $9, 4(%eax)\n"
              "jmp .Lf840ae_00085045\n"
              ".Lf840ae_0008522c:\n"
              "movl scrVmPub, %ebx\n"
              "leal 4(%ebx), %eax\n"
              "movl %eax, scrVmPub\n"
              "addl $1, 0x10(%ebp)\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, 4(%esp)\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll GetNewVariable\n"
              "movl %eax, 4(%ebx)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085264:\n"
              "movzbl (%esi), %esi\n"
              "leal 2(%ecx), %eax\n"
              "movl %eax, 8(%ebp)\n"
              "movl %esi, %eax\n"
              "movzbl %al, %edx\n"
              "leal (, %edx, 4), %eax\n"
              "subl %eax, scrVmPub\n"
              "subl %edx, 0x10(%ebp)\n"
              "movl %esi, %edx\n"
              "testb %dl, %dl\n"
              "je .Lf840ae_00084116\n"
              "xorl %ebx, %ebx\n"
              ".Lf840ae_0008528e:\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll RemoveNextVariable\n"
              "subb $1, %bl\n"
              "movl %esi, %eax\n"
              "addb %bl, %al\n"
              "jne .Lf840ae_0008528e\n"
              "jmp .Lf840ae_00084116\n"
              ".Lf840ae_000852a7:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl scrVmPub, %eax\n"
              "movl (%eax), %eax\n"
              "jmp .Lf840ae_00084ee7\n"
              ".Lf840ae_000852bc:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl scrVmPub, %eax\n"
              "movl -4(%eax), %eax\n"
              "jmp .Lf840ae_00084ee7\n"
              ".Lf840ae_000852d2:\n"
              "movl 0x14(%ebp), %ebx\n"

              "movzwl (%esi), %eax\n"
              "leal 3(%ecx), %edx\n"
              "movl %edx, 8(%ebp)\n"

              "movl %eax, 4(%esp)\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x24(%eax), %eax\n"
              ".Lf840ae_000852ea:\n"
              "movl %eax, (%esp)\n"
              "calll GetVariable\n"
              "movl %ebx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll SetVariableValue\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085303:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalAnd\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_0008531a:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalEquality\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085331:\n"
              "movl 0x14(%ebp), %eax\n"
              "cmpl $8, 4(%eax)\n"
              "je .Lf840ae_000849dd\n"
              ".Lf840ae_0008533e:\n"
              "movl %eax, 4(%esp)\n"
              "movzbl (%esi), %eax\n"
              "shll $2, %eax\n"
              "movl scrVmPub, %edx\n"
              "subl %eax, %edx\n"
              "movl (%edx), %eax\n"
              "movl %eax, (%esp)\n"
              "calll SetVariableValue\n"
              "addl $1, 8(%ebp)\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085363:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl 4(%eax), %edx\n"
              "cmpl $7, %edx\n"
              "je .Lf840ae_00084116\n"
              ".Lf840ae_00085372:\n"
              "movl (%eax), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl %edx, (%esp)\n"
              "calll RemoveRefToValue\n"
              "movl 0x14(%ebp), %eax\n"
              "subl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl 4(%eax), %edx\n"
              "cmpl $7, %edx\n"
              "jne .Lf840ae_00085372\n"
              "jmp .Lf840ae_00084116\n"
              ".Lf840ae_00085396:\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_GetSelf\n"
              "movl %eax, -0x44(%ebp)\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, 4(%esp)\n"
              "movl -0x44(%ebp), %edx\n"
              "movl %edx, (%esp)\n"
              "calll Scr_GetVariableField\n"
              "movl %eax, -0x48(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000853ca:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x24(%eax), %eax\n"
              "movl %eax, -0x44(%ebp)\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, 4(%esp)\n"
              "movl -0x44(%ebp), %edx\n"
              "movl %edx, (%esp)\n"
              "calll Scr_GetVariableField\n"
              "movl %eax, -0x48(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000853fb:\n"
              "movl scrVmPub, %eax\n"
              "movl (%eax), %eax\n"
              "movl %eax, -0x48(%ebp)\n"
              "movl %eax, %edx\n"
              "jmp .Lf840ae_00084db3\n"
              ".Lf840ae_0008540c:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl 8(%ebp), %eax\n"
              "movzbl (%eax), %edx\n"
              "shll $2, %edx\n"
              "movl scrVmPub, %eax\n"
              "subl %edx, %eax\n"
              "movl (%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalVariable\n"
              "movl %eax, 8(%ebx)\n"
              "movl %edx, 0xc(%ebx)\n"
              "addl $1, 8(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085441:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl scrVmPub, %eax\n"
              "movl -8(%eax), %eax\n"
              "jmp .Lf840ae_00084ee7\n"
              ".Lf840ae_00085457:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl scrVmPub, %eax\n"
              "movl -0xc(%eax), %eax\n"
              "jmp .Lf840ae_00084ee7\n"
              ".Lf840ae_0008546d:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl scrVmPub, %eax\n"
              "movl -0x10(%eax), %eax\n"
              "jmp .Lf840ae_00084ee7\n"
              ".Lf840ae_00085483:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl scrVmPub, %eax\n"
              "movl -0x14(%eax), %eax\n"
              "jmp .Lf840ae_00084ee7\n"
              ".Lf840ae_00085499:\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_GetSelf\n"
              "movl %eax, -0x44(%ebp)\n"
              "movl %eax, (%esp)\n"
              "calll IsFieldObject\n"
              "testb %al, %al\n"
              "jne .Lf840ae_00084116\n"
              "jmp .Lf840ae_00086f2f\n"
              ".Lf840ae_000854bc:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x24(%eax), %eax\n"
              "movl %eax, -0x44(%ebp)\n"
              ".Lf840ae_000854c7:\n"
              "movl 0x14(%ebp), %ebx\n"
              "leal 8(%ebx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, 4(%esp)\n"
              "movl -0x44(%ebp), %edx\n"
              "movl %edx, (%esp)\n"
              "calll FindVariable\n"
              "jmp .Lf840ae_00084ee7\n"
              ".Lf840ae_000854f0:\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x2c(%eax), %eax\n"
              "movl %eax, -0x44(%ebp)\n"
              "jmp .Lf840ae_000854c7\n"
              ".Lf840ae_000854fd:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl -0x48(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll ClearArray\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085514:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $1, 4(%eax)\n"
              "movl 0x14(%ebp), %ebx\n"
              "calll Scr_AllocArray\n"
              "movl %eax, (%ebx)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085536:\n"
              "movzbl (%esi), %eax\n"
              "shll $2, %eax\n"
              "movl scrVmPub, %edx\n"
              "subl %eax, %edx\n"
              "movl (%edx), %edx\n"
              "movl %edx, -0x48(%ebp)\n"
              "leal 2(%ecx), %eax\n"
              "movl %eax, 8(%ebp)\n"
              "jmp .Lf840ae_00084db3\n"
              ".Lf840ae_00085554:\n"
              "movl 0x14(%ebp), %eax\n"
              "addl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl $6, 4(%eax)\n"
              "movl 0x14(%ebp), %eax\n"
              "movl $0, (%eax)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"

              ".Lf840ae_00085575:\n"
              "movzwl (%esi), %eax\n"
              "leal 3(%ecx), %edx\n"
              "movl %edx, 8(%ebp)\n"

              "movl 0x14(%ebp), %edx\n"
              "movl %edx, 8(%esp)\n"
              "movl %eax, 4(%esp)\n"
              "movl -0x44(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll ClearVariableField\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_0008559c:\n"
              "movl (%esi), %eax\n"
              "leal 5(%ecx, %eax), %eax\n"
              "movl %eax, 8(%ebp)\n"
              "movl %eax, %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000855ac:\n"
              "rdtsc\n"
              "shrl $2, %eax\n"
              "subl scrVmGlob+24, %eax\n"
              "cmpl $0xff000000, %eax\n"
              "jl .Lf840ae_00084cfd\n"
              "leal 3(%ecx), %eax\n"
              "movzwl (%esi), %edx\n"
              "subl %edx, %eax\n"
              "movl %eax, 8(%ebp)\n"
              "movl %eax, %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000855d4:\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll GetSafeParentLocalId\n"
              "movl %eax, %ebx\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_KillThread\n"
              "movl 0x10(%ebp), %eax\n"
              "shll $2, %eax\n"
              "subl %eax, scrVmPub\n"
              "movl 0x14(%ebp), %ecx\n"
              "movl (%ecx), %eax\n"
              "movl 4(%ecx), %edx\n"
              "movl %eax, -0x20(%ebp)\n"
              "movl %edx, -0x1c(%ebp)\n"
              "leal -8(%ecx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl 4(%eax), %edx\n"
              "cmpl $7, %edx\n"
              "je .Lf840ae_00085633\n"
              ".Lf840ae_00085614:\n"
              "movl (%eax), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl %edx, (%esp)\n"
              "calll RemoveRefToValue\n"
              "movl 0x14(%ebp), %eax\n"
              "subl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl 4(%eax), %edx\n"
              "cmpl $7, %edx\n"
              "jne .Lf840ae_00085614\n"
              ".Lf840ae_00085633:\n"
              "subl $1, scrVmPub+8\n"
              "subl $0x18, scrVmPub+12\n"
              "testl %ebx, %ebx\n"
              "jne .Lf840ae_0008637a\n"
              "movl -0x20(%ebp), %eax\n"
              "movl -0x1c(%ebp), %edx\n"
              "movl 0x14(%ebp), %ecx\n"
              "movl %eax, 8(%ecx)\n"
              "movl %edx, 0xc(%ecx)\n"
              "movl -0x3c(%ebp), %edi\n"
              "testl %edi, %edi\n"
              "jne .Lf840ae_000842f1\n"
              "jmp .Lf840ae_0008451c\n"
              ".Lf840ae_00085668:\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll GetSafeParentLocalId\n"
              "movl %eax, %ebx\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_KillThread\n"
              "movl 0x10(%ebp), %eax\n"
              "shll $2, %eax\n"
              "subl %eax, scrVmPub\n"
              "movl 0x14(%ebp), %eax\n"
              "movl 4(%eax), %edx\n"
              "cmpl $7, %edx\n"
              "je .Lf840ae_000856b6\n"
              ".Lf840ae_00085697:\n"
              "movl (%eax), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl %edx, (%esp)\n"
              "calll RemoveRefToValue\n"
              "movl 0x14(%ebp), %eax\n"
              "subl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl 4(%eax), %edx\n"
              "cmpl $7, %edx\n"
              "jne .Lf840ae_00085697\n"
              ".Lf840ae_000856b6:\n"
              "subl $1, scrVmPub+8\n"
              "subl $0x18, scrVmPub+12\n"
              "testl %ebx, %ebx\n"
              "je .Lf840ae_000842dc\n"
              "movl 0x14(%ebp), %eax\n"
              "movl $0, 4(%eax)\n"
              ".Lf840ae_000856d6:\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll RemoveRefToObject\n"
              "movl scrVmPub+12, %edx\n"
              "movl (%edx), %eax\n"
              "movl %eax, 8(%ebp)\n"
              "movl 8(%edx), %eax\n"
              "movl %eax, 0x10(%ebp)\n"
              "movl %ebx, 0xc(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000856fd:\n"
              "movzbl (%esi), %eax\n"
              "shll $2, %eax\n"
              "movl scrVmPub, %edx\n"
              "subl %eax, %edx\n"
              "movl (%edx), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalVariableObject\n"
              "movl %eax, -0x44(%ebp)\n"
              "addl $1, 8(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085724:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_CastBool\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085737:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalBoolNot\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_0008574a:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalBoolComplement\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_0008575d:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_CastBool\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "leal 2(%eax), %ecx\n"
              "movl %ecx, 8(%ebp)\n"

              "movl 0x14(%ebp), %eax\n"
              "movl (%eax), %eax\n"
              "testl %eax, %eax\n"
              "jne .Lf840ae_00084a3a\n"
              ".Lf840ae_00085781:\n"
              "leal (%ecx, %edx), %eax\n"
              "movl %eax, 8(%ebp)\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_0008578c:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_CastBool\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "leal 2(%eax), %ecx\n"
              "movl %ecx, 8(%ebp)\n"

              "movl 0x14(%ebp), %eax\n"
              "movl (%eax), %eax\n"
              "testl %eax, %eax\n"
              "je .Lf840ae_00084a3a\n"
              "jmp .Lf840ae_00085781\n"
              ".Lf840ae_000857b2:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_CastBool\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "leal 2(%eax), %ecx\n"
              "movl %ecx, 8(%ebp)\n"

              "movl 0x14(%ebp), %eax\n"
              "movl (%eax), %eax\n"
              "testl %eax, %eax\n"
              "jne .Lf840ae_00084a3a\n"
              ".Lf840ae_000857d6:\n"
              "leal (%ecx, %edx), %eax\n"
              "movl %eax, 8(%ebp)\n"
              "movl %eax, %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000857e3:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_CastBool\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "leal 2(%eax), %ecx\n"
              "movl %ecx, 8(%ebp)\n"

              "movl 0x14(%ebp), %eax\n"
              "movl (%eax), %eax\n"
              "testl %eax, %eax\n"
              "je .Lf840ae_00084a3a\n"
              "jmp .Lf840ae_000857d6\n"
              ".Lf840ae_00085809:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalInequality\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085820:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalLess\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085837:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalGreater\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_0008584e:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalLessEqual\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085865:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalGreaterEqual\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_0008587c:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalShiftLeft\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085893:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalShiftRight\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_000858aa:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalPlus\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_000858c1:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalMinus\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_000858d8:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalMultiply\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_000858ef:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalDivide\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085906:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalMod\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_0008591d:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalSizeValue\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085930:\n"
              "movl 0x14(%ebp), %eax\n"
              "cmpl $1, 4(%eax)\n"
              "jne .Lf840ae_00086bf4\n"
              "movl (%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll IsFieldObject\n"
              "testb %al, %al\n"
              "je .Lf840ae_00086dfa\n"
              "movl 0x14(%ebp), %edx\n"
              "movl (%edx), %eax\n"
              "movl %eax, -0x20(%ebp)\n"
              "leal -8(%edx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "cmpl $2, 4(%eax)\n"
              "jne .Lf840ae_00086f41\n"
              "movl -8(%edx), %esi\n"
              "leal -0x10(%edx), %ecx\n"
              "movl %ecx, 0x14(%ebp)\n"
              "movl $0xa, -0x24(%ebp)\n"
              "movl %ecx, %eax\n"
              "subl 0x18(%ebp), %eax\n"
              "sarl $3, %eax\n"
              "leal 0xc(%ebp), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl 0x10(%ebp), %edx\n"
              "movl %edx, (%esp)\n"
              "movl 8(%ebp), %edx\n"
              "calll VM_ArchiveStack\n"
              "movl %eax, -0x28(%ebp)\n"
              "movl $0x1fffe, 4(%esp)\n"
              "movl -0x20(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll GetVariable\n"
              "movl %eax, (%esp)\n"
              "calll GetArray\n"
              "movl %esi, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll GetVariable\n"
              "movl %eax, (%esp)\n"
              "calll GetArray\n"
              "movl 0xc(%ebp), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll GetNewObjectVariable\n"
              "leal -0x28(%ebp), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll SetNewVariableValue\n"
              "movl $1, -0x1c(%ebp)\n"
              "movl 0xc(%ebp), %ebx\n"
              "movl %ebx, (%esp)\n"
              "calll Scr_GetSelf\n"
              "movl %eax, 4(%esp)\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x20(%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll GetObjectVariable\n"
              "movl %eax, (%esp)\n"
              "calll GetArray\n"
              "movl %ebx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll GetNewObjectVariable\n"
              "leal -0x20(%ebp), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll SetNewVariableValue\n"
              "movl %esi, 4(%esp)\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_SetThreadNotifyName\n"
              "jmp .Lf840ae_000842dc\n"
              ".Lf840ae_00085a41:\n"
              "movl 0x14(%ebp), %eax\n"
              "cmpl $1, 4(%eax)\n"
              "jne .Lf840ae_00086bf4\n"
              "movl (%eax), %esi\n"
              "movl %esi, (%esp)\n"
              "calll IsFieldObject\n"
              "testb %al, %al\n"
              "je .Lf840ae_00086dfa\n"
              "movl 0x14(%ebp), %edx\n"
              "leal -8(%edx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "cmpl $2, 4(%eax)\n"
              "jne .Lf840ae_00086d9b\n"
              "movl -8(%edx), %ebx\n"
              "leal -0x10(%edx), %ecx\n"
              "movl %ecx, 0x14(%ebp)\n"
              "movl 8(%ebp), %edx\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, (%eax)\n"
              "movl %ebx, %edx\n"
              "movl %esi, %eax\n"
              "calll VM_Notify\n"
              "movl scrVmPub+12, %eax\n"
              "movl (%eax), %eax\n"
              "movl %eax, 8(%ebp)\n"
              "movl %esi, (%esp)\n"
              "calll RemoveRefToObject\n"
              "movl %ebx, (%esp)\n"
              "calll SL_RemoveRefToString\n"
              "movl 0x14(%ebp), %eax\n"
              "movl 4(%eax), %edx\n"
              "cmpl $8, %edx\n"
              "je .Lf840ae_00084a3a\n"
              ".Lf840ae_00085ab8:\n"
              "movl (%eax), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl %edx, (%esp)\n"
              "calll RemoveRefToValue\n"
              "movl 0x14(%ebp), %eax\n"
              "subl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl 4(%eax), %edx\n"
              "cmpl $8, %edx\n"
              "jne .Lf840ae_00085ab8\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085adc:\n"
              "movl 0x14(%ebp), %eax\n"
              "cmpl $1, 4(%eax)\n"
              "jne .Lf840ae_00086e60\n"
              "movl (%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll IsFieldObject\n"
              "testb %al, %al\n"
              "je .Lf840ae_00086ff8\n"
              "movl 0x14(%ebp), %eax\n"
              "cmpl $2, -4(%eax)\n"
              "jne .Lf840ae_00086fa6\n"
              "movl -8(%eax), %esi\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll AddRefToObject\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll AllocThread\n"
              "movl %eax, %ebx\n"
              "movl $0x1fffe, 4(%esp)\n"
              "movl 0x14(%ebp), %eax\n"
              "movl (%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll GetVariable\n"
              "movl %eax, (%esp)\n"
              "calll GetArray\n"
              "movl %esi, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll GetVariable\n"
              "movl %eax, (%esp)\n"
              "calll GetArray\n"
              "movl %ebx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll GetObjectVariable\n"
              "movl %ebx, (%esp)\n"
              "calll RemoveRefToObject\n"
              "movl $1, -0x1c(%ebp)\n"
              "movl 0x14(%ebp), %eax\n"
              "movl (%eax), %eax\n"
              "movl %eax, -0x20(%ebp)\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x20(%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll GetObjectVariable\n"
              "movl %eax, (%esp)\n"
              "calll GetArray\n"
              "movl %ebx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll GetNewObjectVariable\n"
              "leal -0x20(%ebp), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll SetNewVariableValue\n"
              "movl %esi, 4(%esp)\n"
              "movl %ebx, (%esp)\n"
              "calll Scr_SetThreadNotifyName\n"
              "subl $0x10, 0x14(%ebp)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085bc9:\n"
              "movl $1, scrVmPub+28\n"
              "jmp .Lf840ae_00084341\n"
              ".Lf840ae_00085bd8:\n"
              "movl $4, scrVmPub+28\n"
              "jmp .Lf840ae_00084341\n"
              ".Lf840ae_00085be7:\n"
              "movl $5, scrVmPub+28\n"
              "jmp .Lf840ae_00084341\n"
              ".Lf840ae_00085bf6:\n"
              "movl $1, scrVmPub+28\n"
              "movl 0x14(%ebp), %ecx\n"
              "leal -8(%ecx), %eax\n"
              "movl %eax, scrVmPub+16\n"

              "movl 8(%ebp), %eax\n"
              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, %esi\n"
              "cmpl $1, 4(%ecx)\n"
              "jne .Lf840ae_0008418c\n"
              ".Lf840ae_00085c23:\n"
              "movl (%ecx), %ecx\n"
              "movl %ecx, -0x44(%ebp)\n"
              "movl %ecx, (%esp)\n"
              "calll GetVarType\n"
              "cmpl $0x15, %eax\n"
              "je .Lf840ae_000862a5\n"
              "movl -0x44(%ebp), %edx\n"
              "movl %edx, (%esp)\n"
              "calll GetVarType\n"
              "movl %eax, %ebx\n"
              "movl -0x44(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll RemoveRefToObject\n"
              "movl imp_scrVarPub, %esi\n"
              "movl $0xffffffff, 0x14(%esi)\n"
              "movl imp_var_typename, %eax\n"
              "movl (%eax, %ebx, 4), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl $" %
            s is not an entity ", (%esp)\n"
                               "calll va\n"
                               "movl 0x10(%esi), %ecx\n"
                               "testl %ecx, %ecx\n"
                               "je .Lf840ae_000866de\n"
                               ".Lf840ae_00085c81:\n"
                               "cmpb $0, 0xc(%esi)\n"
                               "jne .Lf840ae_00085c96\n"
                               "movl imp_scrCompilePub, %eax\n"
                               "cmpb $0, 0x24(%eax)\n"
                               "je .Lf840ae_000865a5\n"
                               ".Lf840ae_00085c96:\n"
                               "cmpb $0, scrVmPub+22\n"
                               "je .Lf840ae_00085cbf\n"
                               ".Lf840ae_00085c9f:\n"
                               "movl imp_scrVarPub, %eax\n"
                               "movl 0x10(%eax), %eax\n"
                               "movl %eax, 8(%esp)\n"
                               "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_00085cbf:\n"
              "movl 0x14(%ebp), %eax\n"
              "jmp .Lf840ae_0008418e\n"
              ".Lf840ae_00085cc7:\n"
              "movl $2, scrVmPub+28\n"
              "jmp .Lf840ae_00084169\n"
              ".Lf840ae_00085cd6:\n"
              "movl $3, scrVmPub+28\n"
              "jmp .Lf840ae_00084169\n"
              ".Lf840ae_00085ce5:\n"
              "movl $4, scrVmPub+28\n"
              "jmp .Lf840ae_00084169\n"
              ".Lf840ae_00085cf4:\n"
              "movl $2, scrVmPub+28\n"
              "jmp .Lf840ae_00084341\n"
              ".Lf840ae_00085d03:\n"
              "movl $3, scrVmPub+28\n"
              "jmp .Lf840ae_00084341\n"
              ".Lf840ae_00085d12:\n"
              "movl 0x14(%ebp), %ebx\n"

              "movzwl (%esi), %eax\n"
              "leal 3(%ecx), %edx\n"
              "movl %edx, 8(%ebp)\n"

              "movl %eax, 4(%esp)\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x2c(%eax), %eax\n"
              "jmp .Lf840ae_000852ea\n"

              ".Lf840ae_00085d2f:\n"
              "movzwl (%esi), %ebx\n"
              "leal 3(%ecx), %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_GetSelf\n"
              "movl %eax, -0x44(%ebp)\n"
              "movl %ebx, 4(%esp)\n"
              "movl %eax, (%esp)\n"
              "calll Scr_GetVariableField\n"
              "movl %eax, -0x48(%ebp)\n"
              "movl %eax, %edx\n"
              "jmp .Lf840ae_00084c23\n"
              ".Lf840ae_00085d5c:\n"
              "movl 0x14(%ebp), %eax\n"
              "jmp .Lf840ae_00084a27\n"
              ".Lf840ae_00085d64:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movzbl (%esi), %edx\n"
              "shll $2, %edx\n"
              "movl scrVmPub, %eax\n"
              "subl %edx, %eax\n"
              "movl (%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll SetVariableValue\n"
              "addl $1, 8(%ebp)\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085d8b:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "subl $8, %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalExOr\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085da2:\n"
              "movl (%esi), %eax\n"
              "leal 5(%ecx, %eax), %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movzwl (%eax), %edx\n"
              "addl $2, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, %ebx\n"
              "movzwl %dx, %edx\n"
              "movl %edx, -0x38(%ebp)\n"
              "movl 0x14(%ebp), %eax\n"
              "movl 4(%eax), %edx\n"
              "cmpl $2, %edx\n"
              "je .Lf840ae_00086593\n"
              "cmpl $6, %edx\n"
              "je .Lf840ae_000869c3\n"
              "movl imp_var_typename, %eax\n"
              "movl (%eax, %edx, 4), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl $" cannot switch on %
            s ", (%esp)\n"
              "calll va\n"
              "movl imp_scrVarPub, %edx\n"
              "movl 0x10(%edx), %edi\n"
              "testl %edi, %edi\n"
              "je .Lf840ae_00086afb\n"
              ".Lf840ae_00085dfd:\n"
              "cmpb $0, 0xc(%edx)\n"
              "jne .Lf840ae_0008601c\n"
              "movl imp_scrCompilePub, %eax\n"
              "cmpb $0, 0x24(%eax)\n"
              "jne .Lf840ae_0008601c\n"
              "cmpb $0, 0xa(%edx)\n"
              "je .Lf840ae_00085e39\n"
              "movl $1, %eax\n"
              "movl scrVmGlob+20, %esi\n"
              "testl %esi, %esi\n"
              "movzbl scrVmPub+22, %edx\n"
              "cmovel %edx, %eax\n"
              "movb %al, scrVmPub+22\n"
              ".Lf840ae_00085e39:\n"
              "movl scrVmPub+8, %ecx\n"
              "testl %ecx, %ecx\n"
              "jne .Lf840ae_000861be\n"
              "cmpb $0, scrVmPub+20\n"
              "jne .Lf840ae_000861be\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_00085e74:\n"
              "testw %bx, %bx\n"
              "je .Lf840ae_00084a3a\n"
              "xorl %ebx, %ebx\n"
              ".Lf840ae_00085e7f:\n"
              "movl -0x38(%ebp), %esi\n"
              "subl %ebx, %esi\n"

              "movl 8(%ebp), %eax\n"
              "movl (%eax), %ecx\n"
              "leal 4(%eax), %edx\n"
              "movl %edx, 8(%ebp)\n"

              "movl %ecx, -0x30(%ebp)\n"

              "movl 4(%eax), %edx\n"
              "addl $8, %eax\n"
              "movl %eax, 8(%ebp)\n"

              "movl %edx, -0x2c(%ebp)\n"
              "cmpl %ecx, -0x34(%ebp)\n"
              "je .Lf840ae_00086a40\n"
              "addl $1, %ebx\n"
              "cmpl %ebx, -0x38(%ebp)\n"
              "jne .Lf840ae_00085e7f\n"
              "testl %ecx, %ecx\n"
              "je .Lf840ae_00086943\n"
              "movl $0, -0x38(%ebp)\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085ec3:\n"
              "movzwl (%esi), %esi\n"
              "movl %esi, -0x38(%ebp)\n"
              "leal 3(%ecx, %esi, 8), %eax\n"
              "movl %eax, 8(%ebp)\n"
              "movl %eax, %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085ed7:\n"
              "movl 0x14(%ebp), %eax\n"
              "subl $0x10, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl %eax, (%esp)\n"
              "calll Scr_CastVector\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_00085ef0:\n"
              "subl $1, g_script_error_level\n"
              "xorl %eax, %eax\n"

              "addl $0x5c, %esp\n"
              "popl %ebx\n"
              "popl %esi\n"
              "popl %edi\n"
              "popl %ebp\n"
              "retl\n"

              ".Lf840ae_00085f01:\n"
              "movl $5, scrVmPub+28\n"
              "jmp .Lf840ae_00084169\n"
              ".Lf840ae_00085f10:\n"
              "movl 0x14(%ebp), %edx\n"
              "movl 4(%edx), %ecx\n"
              "cmpl $9, %ecx\n"
              "jne .Lf840ae_00084740\n"
              "movl (%edx), %ebx\n"
              "leal -8(%edx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "cmpl $1, 4(%eax)\n"
              "jne .Lf840ae_00086bf4\n"
              "cmpl $0x1e, scrVmPub+8\n"
              "jg .Lf840ae_00086b9b\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl -8(%edx), %eax\n"
              "movl %eax, (%esp)\n"
              "calll AllocChildThread\n"
              "movl %eax, 0xc(%ebp)\n"
              "subl $8, 0x14(%ebp)\n"
              "movl 8(%ebp), %edx\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, (%eax)\n"
              "movl %ebx, 8(%ebp)\n"
              "jmp .Lf840ae_0008470a\n"
              ".Lf840ae_00085f69:\n"
              "movl 0x14(%ebp), %edx\n"
              "movl 4(%edx), %ecx\n"
              "cmpl $9, %ecx\n"
              "jne .Lf840ae_00086395\n"
              "movl (%edx), %ebx\n"
              "leal -8(%edx), %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "cmpl $1, 4(%eax)\n"
              "jne .Lf840ae_00086bf4\n"
              "cmpl $0x1e, scrVmPub+8\n"
              "jg .Lf840ae_00086b9b\n"
              "movl -8(%edx), %eax\n"
              "movl %eax, (%esp)\n"
              "calll AllocThread\n"
              "movl %eax, 0xc(%ebp)\n"
              "movl 0x14(%ebp), %ecx\n"
              "subl $8, %ecx\n"
              "movl %ecx, 0x14(%ebp)\n"
              "movl 8(%ebp), %edx\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, (%eax)\n"
              "movl 0x18(%ebp), %edx\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, 0x10(%eax)\n"
              "movl %ebx, 8(%ebp)\n"
              "jmp .Lf840ae_0008498b\n"
              ".Lf840ae_00085fcb:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x34(%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_EvalFieldObject\n"
              "movl %eax, -0x44(%ebp)\n"
              "jmp .Lf840ae_00084a3a\n"
              ".Lf840ae_00085fea:\n"
              "cmpb $0, scrVmPub+22\n"
              "je .Lf840ae_00084e9a\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              "jmp .Lf840ae_00084e9a\n"
              ".Lf840ae_0008601c:\n"
              "cmpb $0, scrVmPub+22\n"
              "je .Lf840ae_00085e74\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              "jmp .Lf840ae_00085e74\n"
              ".Lf840ae_0008604e:\n"
              "cmpb $0, scrVmPub+22\n"
              "je .Lf840ae_00084b7d\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              "movl $0, -0x48(%ebp)\n"
              "jmp .Lf840ae_00084fae\n"
              ".Lf840ae_00086087:\n"
              "movl imp_scrVarPub, %ebx\n"
              "movl $2, 0x14(%ebx)\n"
              "movl 4(%eax), %eax\n"
              "movl imp_var_typename, %edx\n"
              "movl (%edx, %eax, 4), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl $" type %
            s is not a float ", (%esp)\n"
                             "calll va\n"
                             "movl 0x10(%ebx), %ecx\n"
                             "testl %ecx, %ecx\n"
                             "je .Lf840ae_0008638d\n"
                             ".Lf840ae_000860bb:\n"
                             "cmpb $0, 0xc(%ebx)\n"
                             "jne .Lf840ae_000860d0\n"
                             "movl imp_scrCompilePub, %eax\n"
                             "cmpb $0, 0x24(%eax)\n"
                             "je .Lf840ae_00086221\n"
                             ".Lf840ae_000860d0:\n"
                             "cmpb $0, scrVmPub+22\n"
                             "je .Lf840ae_000860f9\n"
                             ".Lf840ae_000860d9:\n"
                             "movl imp_scrVarPub, %eax\n"
                             "movl 0x10(%eax), %eax\n"
                             "movl %eax, 8(%esp)\n"
                             "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_000860f9:\n"
              "movl -0x40(%ebp), %eax\n"
              "jmp .Lf840ae_00084230\n"
              ".Lf840ae_00086101:\n"
              "movss (%eax), %xmm0\n"
              "movss %xmm0, -0x4c(%ebp)\n"
              "ucomiss lit4_002ed5e8, %xmm0\n"
              "jp .Lf840ae_00086115\n"
              "jb .Lf840ae_00086167\n"
              ".Lf840ae_00086115:\n"
              "mulss lit4_002ed694, %xmm0\n"
              "addss lit4_002ed5d8, %xmm0\n"
              "movss %xmm0, (%esp)\n"
              "calll floorf\n"
              "fstps -0x50(%ebp)\n"
              "cvttss2si -0x50(%ebp), %eax\n"
              "movl %eax, -0x40(%ebp)\n"
              "testl %eax, %eax\n"
              "jne .Lf840ae_00084230\n"
              "movss -0x4c(%ebp), %xmm0\n"
              "ucomiss lit4_002ed5e8, %xmm0\n"
              "jp .Lf840ae_00086156\n"
              "je .Lf840ae_0008663b\n"
              ".Lf840ae_00086156:\n"
              "movl $1, -0x40(%ebp)\n"
              "movl $1, %eax\n"
              "jmp .Lf840ae_00084230\n"
              ".Lf840ae_00086167:\n"
              "movl imp_scrVarPub, %edx\n"
              "movl 0x10(%edx), %edi\n"
              "testl %edi, %edi\n"
              "jne .Lf840ae_0008444f\n"
              ".Lf840ae_00086178:\n"
              "movl $" negative wait is not allowed ", 0x10(%edx)\n"
                                                    "jmp .Lf840ae_0008444f\n"
                                                    ".Lf840ae_00086184:\n"
                                                    "cmpb $0, 0xa(%esi)\n"
                                                    "je .Lf840ae_000861a7\n"
                                                    "movl $1, %eax\n"
                                                    "movl scrVmGlob+20, %esi\n"
                                                    "testl %esi, %esi\n"
                                                    "movzbl scrVmPub+22, %edx\n"
                                                    "cmovel %edx, %eax\n"
                                                    "movb %al, scrVmPub+22\n"
                                                    ".Lf840ae_000861a7:\n"
                                                    "movl scrVmPub+8, %ebx\n"
                                                    "testl %ebx, %ebx\n"
                                                    "jne .Lf840ae_000861be\n"
                                                    "cmpb $0, scrVmPub+20\n"
                                                    "je .Lf840ae_000841ed\n"
                                                    ".Lf840ae_000861be:\n"
                                                    "movl $0xffffffff, 4(%esp)\n"
                                                    "movl g_script_error_level, %eax\n"
                                                    "leal (%eax, %eax, 8), %eax\n"
                                                    "leal g_script_error(, %eax, 8), %eax\n"
                                                    "movl %eax, (%esp)\n"
                                                    "calll longjmp\n"
                                                    ".Lf840ae_000861dd:\n"
                                                    "movl %eax, 0x10(%esi)\n"
                                                    "jmp .Lf840ae_000841cf\n"
                                                    ".Lf840ae_000861e5:\n"
                                                    "cmpb $0, 0xa(%edx)\n"
                                                    "je .Lf840ae_00086208\n"
                                                    "movl $1, %eax\n"
                                                    "movl scrVmGlob+20, %esi\n"
                                                    "testl %esi, %esi\n"
                                                    "movzbl scrVmPub+22, %edx\n"
                                                    "cmovel %edx, %eax\n"
                                                    "movb %al, scrVmPub+22\n"
                                                    ".Lf840ae_00086208:\n"
                                                    "movl scrVmPub+8, %ebx\n"
                                                    "testl %ebx, %ebx\n"
                                                    "jne .Lf840ae_000861be\n"
                                                    "cmpb $0, scrVmPub+20\n"
                                                    "je .Lf840ae_0008446d\n"
                                                    "jmp .Lf840ae_000861be\n"
                                                    ".Lf840ae_00086221:\n"
                                                    "cmpb $0, 0xa(%ebx)\n"
                                                    "je .Lf840ae_00086244\n"
                                                    "movl $1, %eax\n"
                                                    "movl scrVmGlob+20, %edx\n"
                                                    "testl %edx, %edx\n"
                                                    "movzbl scrVmPub+22, %edx\n"
                                                    "cmovel %edx, %eax\n"
                                                    "movb %al, scrVmPub+22\n"
                                                    ".Lf840ae_00086244:\n"
                                                    "movl scrVmPub+8, %eax\n"
                                                    "testl %eax, %eax\n"
                                                    "jne .Lf840ae_000861be\n"
                                                    "cmpb $0, scrVmPub+20\n"
                                                    "je .Lf840ae_000860d9\n"
                                                    "jmp .Lf840ae_000861be\n"
                                                    ".Lf840ae_00086263:\n"
                                                    "cmpb $0, 0xa(%edx)\n"
                                                    "je .Lf840ae_00086286\n"
                                                    "movl $1, %eax\n"
                                                    "movl scrVmGlob+20, %edx\n"
                                                    "testl %edx, %edx\n"
                                                    "movzbl scrVmPub+22, %edx\n"
                                                    "cmovel %edx, %eax\n"
                                                    "movb %al, scrVmPub+22\n"
                                                    ".Lf840ae_00086286:\n"
                                                    "movl scrVmPub+8, %eax\n"
                                                    "testl %eax, %eax\n"
                                                    "jne .Lf840ae_000861be\n"
                                                    "cmpb $0, scrVmPub+20\n"
                                                    "je .Lf840ae_0008441e\n"
                                                    "jmp .Lf840ae_000861be\n"
                                                    ".Lf840ae_000862a5:\n"
                                                    "movl -0x44(%ebp), %edx\n"
                                                    "movl %edx, (%esp)\n"
                                                    "calll Scr_GetEntityIdRef\n"
                                                    "movl %eax, %ebx\n"
                                                    "movl -0x44(%ebp), %eax\n"
                                                    "movl %eax, (%esp)\n"
                                                    "calll RemoveRefToObject\n"
                                                    "movl 8(%ebp), %edx\n"
                                                    "movl scrVmPub+12, %eax\n"
                                                    "movl %edx, (%eax)\n"
                                                    "movl imp_scrCompilePub, %eax\n"
                                                    "movl 0x38(%eax, %esi, 4), %eax\n"
                                                    "movl %ebx, (%esp)\n"
                                                    "calll *%eax\n"
                                                    "jmp .Lf840ae_00084366\n"
                                                    ".Lf840ae_000862da:\n"
                                                    "movl 0xc(%ebp), %eax\n"
                                                    "movl %eax, (%esp)\n"
                                                    "calll Scr_GetSelf\n"
                                                    "movl %eax, %ebx\n"
                                                    "movl %eax, (%esp)\n"
                                                    "calll AddRefToObject\n"
                                                    "movl 0xc(%ebp), %eax\n"
                                                    "movl %eax, 4(%esp)\n"
                                                    "movl %ebx, (%esp)\n"
                                                    "calll AllocChildThread\n"
                                                    "movl %eax, 0xc(%ebp)\n"
                                                    "jmp .Lf840ae_000846ee\n"
                                                    ".Lf840ae_00086306:\n"
                                                    "cmpb $0, scrVmPub+22\n"
                                                    "je .Lf840ae_000847fa\n"
                                                    "movl imp_scrVarPub, %eax\n"
                                                    "movl 0x10(%eax), %eax\n"
                                                    "movl %eax, 8(%esp)\n"
                                                    "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              "jmp .Lf840ae_000847fa\n"
              ".Lf840ae_00086338:\n"
              "cmpb $0, 0xa(%edx)\n"
              "je .Lf840ae_0008635b\n"
              "movl $1, %eax\n"
              "movl scrVmGlob+20, %edx\n"
              "testl %edx, %edx\n"
              "movzbl scrVmPub+22, %edx\n"
              "cmovel %edx, %eax\n"
              "movb %al, scrVmPub+22\n"
              ".Lf840ae_0008635b:\n"
              "movl scrVmPub+8, %eax\n"
              "testl %eax, %eax\n"
              "jne .Lf840ae_000861be\n"
              "cmpb $0, scrVmPub+20\n"
              "je .Lf840ae_0008469c\n"
              "jmp .Lf840ae_000861be\n"
              ".Lf840ae_0008637a:\n"
              "movl -0x20(%ebp), %eax\n"
              "movl -0x1c(%ebp), %edx\n"
              "movl 0x14(%ebp), %ecx\n"
              "movl %eax, (%ecx)\n"
              "movl %edx, 4(%ecx)\n"
              "jmp .Lf840ae_000856d6\n"
              ".Lf840ae_0008638d:\n"
              "movl %eax, 0x10(%ebx)\n"
              "jmp .Lf840ae_000860bb\n"
              ".Lf840ae_00086395:\n"
              "movl (%edx), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl %ecx, (%esp)\n"
              "calll RemoveRefToValue\n"
              "movl 0x14(%ebp), %eax\n"
              "subl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl 0xc(%eax), %edx\n"
              "movl imp_var_typename, %eax\n"
              "movl (%eax, %edx, 4), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl $" %
            s is not a function pointer ", (%esp)\n"
                                        "calll va\n"
                                        "movl imp_scrVarPub, %edx\n"
                                        "movl 0x10(%edx), %ecx\n"
                                        "testl %ecx, %ecx\n"
                                        "je .Lf840ae_00086b17\n"
                                        ".Lf840ae_000863d8:\n"
                                        "cmpb $0, 0xc(%edx)\n"
                                        "jne .Lf840ae_00086463\n"
                                        "movl imp_scrCompilePub, %eax\n"
                                        "cmpb $0, 0x24(%eax)\n"
                                        "jne .Lf840ae_00086463\n"
                                        "cmpb $0, 0xa(%edx)\n"
                                        "je .Lf840ae_00086410\n"
                                        "movl $1, %eax\n"
                                        "movl scrVmGlob+20, %edx\n"
                                        "testl %edx, %edx\n"
                                        "movzbl scrVmPub+22, %edx\n"
                                        "cmovel %edx, %eax\n"
                                        "movb %al, scrVmPub+22\n"
                                        ".Lf840ae_00086410:\n"
                                        "movl scrVmPub+8, %eax\n"
                                        "testl %eax, %eax\n"
                                        "jne .Lf840ae_000861be\n"
                                        "cmpb $0, scrVmPub+20\n"
                                        "jne .Lf840ae_000861be\n"
                                        "movl imp_scrVarPub, %eax\n"
                                        "movl 0x10(%eax), %eax\n"
                                        "movl %eax, 8(%esp)\n"
                                        "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_0008644a:\n"
              "movl 0x14(%ebp), %eax\n"

              "movl (%eax), %edx\n"
              "movl %edx, 4(%esp)\n"
              "movl 4(%eax), %eax\n"
              "movl %eax, (%esp)\n"
              "calll RemoveRefToValue\n"
              "jmp .Lf840ae_00084a3a\n"

              ".Lf840ae_00086463:\n"
              "cmpb $0, scrVmPub+22\n"
              "je .Lf840ae_00084ce4\n"
              "movl imp_scrVarPub, %eax\n"
              "movl 0x10(%eax), %eax\n"
              "movl %eax, 8(%esp)\n"
              "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              "jmp .Lf840ae_0008644a\n"
              ".Lf840ae_00086492:\n"
              "movl $" wait is too long ", 0x10(%edx)\n"
                                        "jmp .Lf840ae_000843fc\n"
                                        ".Lf840ae_0008649e:\n"
                                        "movl 0xc(%ebp), %eax\n"
                                        "movl %eax, (%esp)\n"
                                        "calll Scr_GetSelf\n"
                                        "movl %eax, %ebx\n"
                                        "movl %eax, (%esp)\n"
                                        "calll AddRefToObject\n"
                                        "movl %ebx, (%esp)\n"
                                        "calll AllocThread\n"
                                        "movl %eax, 0xc(%ebp)\n"
                                        "jmp .Lf840ae_00084961\n"
                                        ".Lf840ae_000864c3:\n"
                                        "cmpb $0, 0xa(%edx)\n"
                                        "jne .Lf840ae_0008670f\n"
                                        ".Lf840ae_000864cd:\n"
                                        "movl scrVmPub+8, %eax\n"
                                        "testl %eax, %eax\n"
                                        "jne .Lf840ae_000861be\n"
                                        "cmpb $0, scrVmPub+20\n"
                                        "je .Lf840ae_00084916\n"
                                        "jmp .Lf840ae_000861be\n"
                                        ".Lf840ae_000864ec:\n"
                                        "movl %eax, 0x10(%edx)\n"
                                        "jmp .Lf840ae_0008467e\n"
                                        ".Lf840ae_000864f4:\n"
                                        "movl 0x14(%ebp), %eax\n"
                                        "movl (%eax), %eax\n"
                                        "movl %eax, 4(%esp)\n"
                                        "movl $" switch index %
            d out of range ", (%esp)\n"
                           "calll va\n"
                           "movl imp_scrVarPub, %edx\n"
                           "movl 0x10(%edx), %ecx\n"
                           "testl %ecx, %ecx\n"
                           "je .Lf840ae_00086b87\n"
                           ".Lf840ae_0008651a:\n"
                           "cmpb $0, 0xc(%edx)\n"
                           "jne .Lf840ae_00086b33\n"
                           "movl imp_scrCompilePub, %eax\n"
                           "cmpb $0, 0x24(%eax)\n"
                           "jne .Lf840ae_00086b33\n"
                           "cmpb $0, 0xa(%edx)\n"
                           "je .Lf840ae_00086556\n"
                           "movl $1, %eax\n"
                           "movl scrVmGlob+20, %edx\n"
                           "testl %edx, %edx\n"
                           "movzbl scrVmPub+22, %edx\n"
                           "cmovel %edx, %eax\n"
                           "movb %al, scrVmPub+22\n"
                           ".Lf840ae_00086556:\n"
                           "movl scrVmPub+8, %eax\n"
                           "testl %eax, %eax\n"
                           "jne .Lf840ae_000861be\n"
                           "cmpb $0, scrVmPub+20\n"
                           "jne .Lf840ae_000861be\n"
                           ".Lf840ae_00086570:\n"
                           "movl imp_scrVarPub, %eax\n"
                           "movl 0x10(%eax), %eax\n"
                           "movl %eax, 8(%esp)\n"
                           "movl $"\x15 %
            s ", 4(%esp)\n"
              "movl $1, (%esp)\n"
              "calll Com_Error\n"
              ".Lf840ae_00086590:\n"
              "movl 0x14(%ebp), %eax\n"
              ".Lf840ae_00086593:\n"
              "movl (%eax), %eax\n"
              "movl %eax, -0x34(%ebp)\n"
              "movl %eax, (%esp)\n"
              "calll SL_RemoveRefToString\n"
              "jmp .Lf840ae_00085e74\n"
              ".Lf840ae_000865a5:\n"
              "cmpb $0, 0xa(%esi)\n"
              "jne .Lf840ae_00086921\n"
              ".Lf840ae_000865af:\n"
              "movl scrVmPub+8, %eax\n"
              "testl %eax, %eax\n"
              "jne .Lf840ae_000861be\n"
              "cmpb $0, scrVmPub+20\n"
              "je .Lf840ae_00085c9f\n"
              "jmp .Lf840ae_000861be\n"
              ".Lf840ae_000865ce:\n"
              "cmpb $0, 0xa(%edx)\n"
              "jne .Lf840ae_000868ff\n"
              ".Lf840ae_000865d8:\n"
              "movl scrVmPub+8, %eax\n"
              "testl %eax, %eax\n"
              "jne .Lf840ae_000861be\n"
              "cmpb $0, scrVmPub+20\n"
              "je .Lf840ae_000845c3\n"
              "jmp .Lf840ae_000861be\n"
              ".Lf840ae_000865f7:\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_GetSelf\n"
              "movl %eax, %ebx\n"
              "movl %eax, (%esp)\n"
              "calll AddRefToObject\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl %ebx, (%esp)\n"
              "calll AllocChildThread\n"
              "movl %eax, 0xc(%ebp)\n"
              "movl 8(%ebp), %edx\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, (%eax)\n"
              "movl 0x14(%ebp), %eax\n"
              "movl (%eax), %edx\n"
              "movl %edx, 8(%ebp)\n"
              "subl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "jmp .Lf840ae_0008470a\n"
              ".Lf840ae_0008663b:\n"
              "movl $0, -0x40(%ebp)\n"
              "xorl %eax, %eax\n"
              "jmp .Lf840ae_00084230\n"
              ".Lf840ae_00086649:\n"
              "movl %eax, 0x10(%edx)\n"
              "jmp .Lf840ae_000848f8\n"
              ".Lf840ae_00086651:\n"
              "movl (%eax), %esi\n"
              "subl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl 0xc(%ebp), %eax\n"
              "movl %eax, (%esp)\n"
              "calll Scr_GetSelf\n"
              "movl %eax, %ebx\n"
              "movl %eax, (%esp)\n"
              "calll AddRefToObject\n"
              "movl %ebx, (%esp)\n"
              "calll AllocThread\n"
              "movl %eax, 0xc(%ebp)\n"
              "movl 8(%ebp), %edx\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, (%eax)\n"
              "movl 0x18(%ebp), %edx\n"
              "movl scrVmPub+12, %eax\n"
              "movl %edx, 0x10(%eax)\n"
              "movl %esi, 8(%ebp)\n"
              "movl 0x14(%ebp), %ecx\n"
              "jmp .Lf840ae_0008498b\n"
              ".Lf840ae_00086699:\n"
              "movl 0x14(%ebp), %eax\n"
              "movl 4(%eax), %edx\n"
              "cmpl $8, %edx\n"
              "je .Lf840ae_000866c3\n"
              ".Lf840ae_000866a4:\n"
              "movl (%eax), %eax\n"
              "movl %eax, 4(%esp)\n"
              "movl %edx, (%esp)\n"
              "calll RemoveRefToValue\n"
              "movl 0x14(%ebp), %eax\n"
              "subl $8, %eax\n"
              "movl %eax, 0x14(%ebp)\n"
              "movl 4(%eax), %edx\n"
              "cmpl $8, %edx\n"
              "jne .Lf840ae_000866a4\n"
              ".Lf840ae_000866c3:\n"
              "movl $7, 4(%eax)\n"
              "movl 8(%ebp), %ecx\n"
              "jmp .Lf840ae_00084119\n"
              ".Lf840ae_000866d2:\n"
              "movl $" script stack overflow(too many embedded function calls) ", 0x10(%edx)\n"
                                                                               "jmp .Lf840ae_000845a5\n"
                                                                               ".Lf840ae_000866de:\n"
                                                                               "movl %eax, 0x10(%esi)\n"
                                                                               "jmp .Lf840ae_00085c81\n"
                                                                               ".Lf840ae_000866e6:\n"
                                                                               "cmpb $0, 0xa(%edx)\n"
                                                                               "jne .Lf840ae_00086995\n"
                                                                               ".Lf840ae_000866f0:\n"
                                                                               "movl scrVmPub+8, %eax\n"
                                                                               "testl %eax, %eax\n"
                                                                               "jne .Lf840ae_000861be\n"
                                                                               "cmpb $0, scrVmPub+20\n"
                                                                               "je .Lf840ae_0008483d\n"
                                                                               "jmp .Lf840ae_000861be\n"
                                                                               ".Lf840ae_0008670f:\n"
                                                                               "movl $1, %eax\n"
                                                                               "movl scrVmGlob+20, %edx\n"
                                                                               "testl %edx, %edx\n"
                                                                               "movzbl scrVmPub+22, %edx\n"
                                                                               "cmovel %edx, %eax\n"
                                                                               "movb %al, scrVmPub+22\n"
                                                                               "jmp .Lf840ae_000864cd\n"
                                                                               ".Lf840ae_00086731:\n"
                                                                               "cmpb $0, 0xa(%edx)\n"
                                                                               "je .Lf840ae_00086754\n"
                                                                               "movl $1, %eax\n"
                                                                               "movl scrVmGlob+20, %edi\n"
                                                                               "testl %edi, %edi\n"
                                                                               "movzbl scrVmPub+22, %edx\n"
                                                                               "cmovel %edx, %eax\n"
                                                                               "movb %al, scrVmPub+22\n"
                                                                               ".Lf840ae_00086754:\n"
                                                                               "movl scrVmPub+8, %esi\n"
                                                                               "testl %esi, %esi\n"
                                                                               "jne .Lf840ae_000861be\n"
                                                                               "cmpb $0, scrVmPub+20\n"
                                                                               "je .Lf840ae_00084b21\n"
                                                                               "jmp .Lf840ae_000861be\n"
                                                                               ".Lf840ae_00086774:\n"
                                                                               "cmpb $0, 0xa(%edx)\n"
                                                                               "je .Lf840ae_00086797\n"
                                                                               "movl $1, %eax\n"
                                                                               "movl scrVmGlob+20, %esi\n"
                                                                               "testl %esi, %esi\n"
                                                                               "movzbl scrVmPub+22, %edx\n"
                                                                               "cmovel %edx, %eax\n"
                                                                               "movb %al, scrVmPub+22\n"
                                                                               ".Lf840ae_00086797:\n"
                                                                               "movl scrVmPub+8, %ebx\n"
                                                                               "testl %ebx, %ebx\n"
                                                                               "jne .Lf840ae_000861be\n"
                                                                               "cmpb $0, scrVmPub+20\n"
                                                                               "je .Lf840ae_0008462f\n"
                                                                               "jmp .Lf840ae_000861be\n"
                                                                               ".Lf840ae_000867b7:\n"
                                                                               "movl $" script runtime error : potential infinite loop in script - killing thread.\n ", (%esp)\n"
                                                                                                                                                                     "calll Com_Printf\n"
                                                                                                                                                                     "movl $0, 8(%esp)\n"
                                                                                                                                                                     "movl 8(%ebp), %eax\n"
                                                                                                                                                                     "movl %eax, 4(%esp)\n"
                                                                                                                                                                     "movl $0, (%esp)\n"
                                                                                                                                                                     "calll Scr_PrintPrevCodePos\n"
                                                                                                                                                                     "rdtsc\n"
                                                                                                                                                                     "xorl %edx, %edx\n"
                                                                                                                                                                     "shrdl $2, %edx, %eax\n"
                                                                                                                                                                     "movl %eax, scrVmGlob+24\n"
                                                                                                                                                                     "movl 0xc(%ebp), %eax\n"
                                                                                                                                                                     ".Lf840ae_000867ee:\n"
                                                                                                                                                                     "movl %eax, (%esp)\n"
                                                                                                                                                                     "calll GetSafeParentLocalId\n"
                                                                                                                                                                     "movl %eax, %ebx\n"
                                                                                                                                                                     "movl 0xc(%ebp), %eax\n"
                                                                                                                                                                     "movl %eax, (%esp)\n"
                                                                                                                                                                     "calll Scr_KillThread\n"
                                                                                                                                                                     "movl 0x10(%ebp), %eax\n"
                                                                                                                                                                     "shll $2, %eax\n"
                                                                                                                                                                     "subl %eax, scrVmPub\n"
                                                                                                                                                                     "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                     "movl 4(%eax), %edx\n"
                                                                                                                                                                     "cmpl $7, %edx\n"
                                                                                                                                                                     "je .Lf840ae_00086839\n"
                                                                                                                                                                     ".Lf840ae_0008681a:\n"
                                                                                                                                                                     "movl (%eax), %eax\n"
                                                                                                                                                                     "movl %eax, 4(%esp)\n"
                                                                                                                                                                     "movl %edx, (%esp)\n"
                                                                                                                                                                     "calll RemoveRefToValue\n"
                                                                                                                                                                     "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                     "subl $8, %eax\n"
                                                                                                                                                                     "movl %eax, 0x14(%ebp)\n"
                                                                                                                                                                     "movl 4(%eax), %edx\n"
                                                                                                                                                                     "cmpl $7, %edx\n"
                                                                                                                                                                     "jne .Lf840ae_0008681a\n"
                                                                                                                                                                     ".Lf840ae_00086839:\n"
                                                                                                                                                                     "subl $1, scrVmPub+8\n"
                                                                                                                                                                     "subl $0x18, scrVmPub+12\n"
                                                                                                                                                                     "testl %ebx, %ebx\n"
                                                                                                                                                                     "je .Lf840ae_000842dc\n"
                                                                                                                                                                     "movl 0xc(%ebp), %eax\n"
                                                                                                                                                                     "movl %eax, (%esp)\n"
                                                                                                                                                                     "calll RemoveRefToObject\n"
                                                                                                                                                                     "movl scrVmPub+12, %edx\n"
                                                                                                                                                                     "movl (%edx), %eax\n"
                                                                                                                                                                     "movl %eax, 8(%ebp)\n"
                                                                                                                                                                     "movl 8(%edx), %eax\n"
                                                                                                                                                                     "movl %eax, 0x10(%ebp)\n"
                                                                                                                                                                     "movl %ebx, 0xc(%ebp)\n"
                                                                                                                                                                     "subl $8, 0x14(%ebp)\n"
                                                                                                                                                                     "movl %ebx, %eax\n"
                                                                                                                                                                     "jmp .Lf840ae_000867ee\n"
                                                                                                                                                                     ".Lf840ae_00086879:\n"
                                                                                                                                                                     "cmpb $0, 0xa(%edx)\n"
                                                                                                                                                                     "je .Lf840ae_0008689c\n"
                                                                                                                                                                     "movl $1, %eax\n"
                                                                                                                                                                     "movl scrVmGlob+20, %esi\n"
                                                                                                                                                                     "testl %esi, %esi\n"
                                                                                                                                                                     "movzbl scrVmPub+22, %edx\n"
                                                                                                                                                                     "cmovel %edx, %eax\n"
                                                                                                                                                                     "movb %al, scrVmPub+22\n"
                                                                                                                                                                     ".Lf840ae_0008689c:\n"
                                                                                                                                                                     "movl scrVmPub+8, %ebx\n"
                                                                                                                                                                     "testl %ebx, %ebx\n"
                                                                                                                                                                     "jne .Lf840ae_000861be\n"
                                                                                                                                                                     "cmpb $0, scrVmPub+20\n"
                                                                                                                                                                     "je .Lf840ae_000848a9\n"
                                                                                                                                                                     "jmp .Lf840ae_000861be\n"
                                                                                                                                                                     ".Lf840ae_000868bc:\n"
                                                                                                                                                                     "cmpb $0, 0xa(%edx)\n"
                                                                                                                                                                     "je .Lf840ae_000868df\n"
                                                                                                                                                                     "movl $1, %eax\n"
                                                                                                                                                                     "movl scrVmGlob+20, %edi\n"
                                                                                                                                                                     "testl %edi, %edi\n"
                                                                                                                                                                     "movzbl scrVmPub+22, %edx\n"
                                                                                                                                                                     "cmovel %edx, %eax\n"
                                                                                                                                                                     "movb %al, scrVmPub+22\n"
                                                                                                                                                                     ".Lf840ae_000868df:\n"
                                                                                                                                                                     "movl scrVmPub+8, %esi\n"
                                                                                                                                                                     "testl %esi, %esi\n"
                                                                                                                                                                     "jne .Lf840ae_000861be\n"
                                                                                                                                                                     "cmpb $0, scrVmPub+20\n"
                                                                                                                                                                     "je .Lf840ae_00084c73\n"
                                                                                                                                                                     "jmp .Lf840ae_000861be\n"
                                                                                                                                                                     ".Lf840ae_000868ff:\n"
                                                                                                                                                                     "movl $1, %eax\n"
                                                                                                                                                                     "movl scrVmGlob+20, %edx\n"
                                                                                                                                                                     "testl %edx, %edx\n"
                                                                                                                                                                     "movzbl scrVmPub+22, %edx\n"
                                                                                                                                                                     "cmovel %edx, %eax\n"
                                                                                                                                                                     "movb %al, scrVmPub+22\n"
                                                                                                                                                                     "jmp .Lf840ae_000865d8\n"
                                                                                                                                                                     ".Lf840ae_00086921:\n"
                                                                                                                                                                     "movl $1, %eax\n"
                                                                                                                                                                     "movl scrVmGlob+20, %edx\n"
                                                                                                                                                                     "testl %edx, %edx\n"
                                                                                                                                                                     "movzbl scrVmPub+22, %edx\n"
                                                                                                                                                                     "cmovel %edx, %eax\n"
                                                                                                                                                                     "movb %al, scrVmPub+22\n"
                                                                                                                                                                     "jmp .Lf840ae_000865af\n"
                                                                                                                                                                     ".Lf840ae_00086943:\n"
                                                                                                                                                                     "movl %edx, 8(%ebp)\n"
                                                                                                                                                                     "movl $0, -0x38(%ebp)\n"
                                                                                                                                                                     "jmp .Lf840ae_00084a3a\n"
                                                                                                                                                                     ".Lf840ae_00086952:\n"
                                                                                                                                                                     "cmpb $0, 0xa(%edx)\n"
                                                                                                                                                                     "je .Lf840ae_00086975\n"
                                                                                                                                                                     "movl $1, %eax\n"
                                                                                                                                                                     "movl scrVmGlob+20, %esi\n"
                                                                                                                                                                     "testl %esi, %esi\n"
                                                                                                                                                                     "movzbl scrVmPub+22, %edx\n"
                                                                                                                                                                     "cmovel %edx, %eax\n"
                                                                                                                                                                     "movb %al, scrVmPub+22\n"
                                                                                                                                                                     ".Lf840ae_00086975:\n"
                                                                                                                                                                     "movl scrVmPub+8, %ebx\n"
                                                                                                                                                                     "testl %ebx, %ebx\n"
                                                                                                                                                                     "jne .Lf840ae_000861be\n"
                                                                                                                                                                     "cmpb $0, scrVmPub+20\n"
                                                                                                                                                                     "je .Lf840ae_00084bd0\n"
                                                                                                                                                                     "jmp .Lf840ae_000861be\n"
                                                                                                                                                                     ".Lf840ae_00086995:\n"
                                                                                                                                                                     "movl $1, %eax\n"
                                                                                                                                                                     "movl scrVmGlob+20, %edx\n"
                                                                                                                                                                     "testl %edx, %edx\n"
                                                                                                                                                                     "movzbl scrVmPub+22, %edx\n"
                                                                                                                                                                     "cmovel %edx, %eax\n"
                                                                                                                                                                     "movb %al, scrVmPub+22\n"
                                                                                                                                                                     "jmp .Lf840ae_000866f0\n"
                                                                                                                                                                     ".Lf840ae_000869b7:\n"
                                                                                                                                                                     "movl $" script stack overflow(too many embedded function calls) ", 0x10(%edx)\n"
                                                                                                                                                                                                                                      "jmp .Lf840ae_0008481f\n"
                                                                                                                                                                                                                                      ".Lf840ae_000869c3:\n"
                                                                                                                                                                                                                                      "movl (%eax), %eax\n"
                                                                                                                                                                                                                                      "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                      "calll IsValidArrayIndex\n"
                                                                                                                                                                                                                                      "testb %al, %al\n"
                                                                                                                                                                                                                                      "je .Lf840ae_000864f4\n"
                                                                                                                                                                                                                                      "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                                                                                      "movl (%eax), %eax\n"
                                                                                                                                                                                                                                      "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                      "calll GetInternalVariableIndex\n"
                                                                                                                                                                                                                                      "movl %eax, -0x34(%ebp)\n"
                                                                                                                                                                                                                                      "jmp .Lf840ae_00085e74\n"
                                                                                                                                                                                                                                      ".Lf840ae_000869ea:\n"
                                                                                                                                                                                                                                      "movl $" script stack overflow(too many embedded function calls) ", 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                       "jmp .Lf840ae_00084611\n"
                                                                                                                                                                                                                                                                                                       ".Lf840ae_000869f6:\n"
                                                                                                                                                                                                                                                                                                       "movl $" script runtime warning : potential infinite loop in script.\n ", (%esp)\n"
                                                                                                                                                                                                                                                                                                                                                                              "calll Com_Printf\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl $0, 8(%esp)\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl 8(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl %eax, 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl $0, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                                                              "calll Scr_PrintPrevCodePos\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl 8(%ebp), %edx\n"
                                                                                                                                                                                                                                                                                                                                                                              "leal 2(%edx), %eax\n"
                                                                                                                                                                                                                                                                                                                                                                              "movzwl (%edx), %edx\n"
                                                                                                                                                                                                                                                                                                                                                                              "subl %edx, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl %eax, 8(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                                                              "rdtsc\n"
                                                                                                                                                                                                                                                                                                                                                                              "xorl %edx, %edx\n"
                                                                                                                                                                                                                                                                                                                                                                              "shrdl $2, %edx, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl %eax, scrVmGlob+24\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl 8(%ebp), %ecx\n"
                                                                                                                                                                                                                                                                                                                                                                              "jmp .Lf840ae_00084119\n"
                                                                                                                                                                                                                                                                                                                                                                              ".Lf840ae_00086a40:\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl -0x2c(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl %eax, 8(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl %esi, -0x38(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                                                              "jmp .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                                                              ".Lf840ae_00086a4e:\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl %eax, 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                              "jmp .Lf840ae_00084b03\n"
                                                                                                                                                                                                                                                                                                                                                                              ".Lf840ae_00086a56:\n"
                                                                                                                                                                                                                                                                                                                                                                              "movl $" script stack overflow(too many embedded function calls) ", 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_0008488b\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086a62:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $" function called with too many parameters ", 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00084c55\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086a6e:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $0, -0x48(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $0, -0x44(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $0, -0x40(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "xorl %edi, %edi\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $0, -0x38(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $0, -0x34(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $0, -0x30(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $0, -0x2c(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $0, -0x3c(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086aa8:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl scrVmGlob+16, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl %eax, 0xc(%esp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %ebx\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl 0x10(%ebx), %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl %eax, 8(%esp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebx), %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl %eax, 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl 8(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "calll RuntimeError\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $0, 0x10(%ebx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $0, scrVmGlob+16\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $0, 0x14(%ebx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "leal -0x1f(%edi), %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "cmpl $0x65, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "ja .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "jmpl *.Ljt_840ae_1(, %eax, 4)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086afb:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl %eax, 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00085dfd\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086b03:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                               "movl $" cannot create a new local variable in the debugger ", 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "jmp .Lf840ae_00084f31\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           ".Lf840ae_00086b0f:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl %eax, 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "jmp .Lf840ae_00084783\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           ".Lf840ae_00086b17:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl %eax, 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "jmp .Lf840ae_000863d8\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           ".Lf840ae_00086b1f:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl $" unknown object ", 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "jmp .Lf840ae_00084e23\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           ".Lf840ae_00086b2b:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl %eax, 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "jmp .Lf840ae_00084bb2\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           ".Lf840ae_00086b33:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "cmpb $0, scrVmPub+22\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "jne .Lf840ae_00086570\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "jmp .Lf840ae_00086590\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           ".Lf840ae_00086b45:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "cmpb $0, 0xa(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "je .Lf840ae_00086b68\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl $1, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl scrVmGlob+20, %edx\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "testl %edx, %edx\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movzbl scrVmPub+22, %edx\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "cmovel %edx, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movb %al, scrVmPub+22\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           ".Lf840ae_00086b68:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl scrVmPub+8, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "testl %eax, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "jne .Lf840ae_000861be\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "cmpb $0, scrVmPub+20\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "je .Lf840ae_00084d58\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "jmp .Lf840ae_000861be\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           ".Lf840ae_00086b87:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl %eax, 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "jmp .Lf840ae_0008651a\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           ".Lf840ae_00086b8f:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl $" potential infinite loop in script ", 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "jmp .Lf840ae_00084d3a\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           ".Lf840ae_00086b9b:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl imp_scrVarPub, %edx\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl $1, 0x14(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl 0x10(%edx), %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "testl %eax, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "jne .Lf840ae_00086bb6\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           "movl $" script stack overflow(too many embedded function calls) ", 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            ".Lf840ae_00086bb6:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            "cmpb $0, 0xc(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            "jne .Lf840ae_00086bcb\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            "movl imp_scrCompilePub, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            "cmpb $0, 0x24(%eax)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            "je .Lf840ae_0008700c\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            ".Lf840ae_00086bcb:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            "cmpb $0, scrVmPub+22\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            "je .Lf840ae_00086bf4\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            ".Lf840ae_00086bd4:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            "movl imp_scrVarPub, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            "movl 0x10(%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            "movl %eax, 8(%esp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            "movl $"\x15 %
                                                                                                                                                                                                                                                                                                                                             s ", 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll Com_Error\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086bf4:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl 4(%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086bfa:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl $2, 0x14(%eax)\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086c06:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_var_typename, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl (%eax, %edx, 4), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl $" %
                                                                                                                                                                                                                                                                                                                                             s is not an object ", (%esp)\n"
                                                                                                                                                                                                                                                                                                                                                                "calll va\n"
                                                                                                                                                                                                                                                                                                                                                                "movl imp_scrVarPub, %edx\n"
                                                                                                                                                                                                                                                                                                                                                                "movl 0x10(%edx), %esi\n"
                                                                                                                                                                                                                                                                                                                                                                "testl %esi, %esi\n"
                                                                                                                                                                                                                                                                                                                                                                "je .Lf840ae_00086c78\n"
                                                                                                                                                                                                                                                                                                                                                                ".Lf840ae_00086c2b:\n"
                                                                                                                                                                                                                                                                                                                                                                "cmpb $0, 0xc(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                "jne .Lf840ae_00086c3c\n"
                                                                                                                                                                                                                                                                                                                                                                "movl imp_scrCompilePub, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                "cmpb $0, 0x24(%eax)\n"
                                                                                                                                                                                                                                                                                                                                                                "je .Lf840ae_00086c7d\n"
                                                                                                                                                                                                                                                                                                                                                                ".Lf840ae_00086c3c:\n"
                                                                                                                                                                                                                                                                                                                                                                "cmpb $0, scrVmPub+22\n"
                                                                                                                                                                                                                                                                                                                                                                "je .Lf840ae_00086c65\n"
                                                                                                                                                                                                                                                                                                                                                                ".Lf840ae_00086c45:\n"
                                                                                                                                                                                                                                                                                                                                                                "movl imp_scrVarPub, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                "movl 0x10(%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                                                "movl %eax, 8(%esp)\n"
                                                                                                                                                                                                                                                                                                                                                                "movl $"\x15 %
                                                                                                                                                                                                                                                                                                                                             s ", 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll Com_Error\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086c65:\n"
                                                                                                                                                                                                                                                                                                                                               "leal -0x21(%edi), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "cmpl $0x63, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "ja .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               "jmpl *.Ljt_840ae_2(, %eax, 4)\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086c78:\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086c2b\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086c7d:\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, 0xa(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_00086ca0\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmGlob+20, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "testl %ebx, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "movzbl scrVmPub+22, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "cmovel %edx, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movb %al, scrVmPub+22\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086ca0:\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+8, %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "testl %ecx, %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000861be\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, scrVmPub+20\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_00086c45\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_000861be\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086cbc:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ecx), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "testl %edx, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "jle .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+28, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "subl %edx, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "addl $1, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 0x14(%ecx)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086cdf:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ecx), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "cmpl $0, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "jle .Lf840ae_00086d2c\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+28, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "subl %edx, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "addl $2, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 0x14(%ecx)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086cff:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl $0, 0x14(%eax)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086d10:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "testl %edx, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "jns .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, 0x14(%eax)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086d2c:\n"
                                                                                                                                                                                                                                                                                                                                               "jge .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, 0x14(%ecx)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086d3e:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x34(%ebx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll ClearVariableValue\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x34(%ebx), %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "movl %ebx, -0x48(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00084ce4\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086d5a:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %edx\n"

                                                                                                                                                                                                                                                                                                                                               "movl (%edx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 4(%edx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll RemoveRefToValue\n"

                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "leal -8(%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "movl %edx, 0x14(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl -8(%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 4(%edx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll RemoveRefToValue\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086d89:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086d8c:\n"
                                                                                                                                                                                                                                                                                                                                               "movl $0, 4(%eax)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 8(%ebp), %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00084119\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086d9b:\n"
                                                                                                                                                                                                                                                                                                                                               "addl $8, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 0x14(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, 0x14(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x10(%edx), %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "testl %ecx, %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_00086dbc\n"
                                                                                                                                                                                                                                                                                                                                               "movl $" first parameter of notify must evaluate to a string ", 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086dbc:\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, 0xc(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_00086dd1\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrCompilePub, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, 0x24(%eax)\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_00086e77\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086dd1:\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, scrVmPub+22\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_00086dfa\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086dda:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x10(%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 8(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl $"\x15 %
                                                                                                                                                                                                                                                                                                                                             s ", 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll Com_Error\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086dfa:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl (%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll GetVarType\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086bfa\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086e0e:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x10(%edx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "testl %eax, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_00086e22\n"
                                                                                                                                                                                                                                                                                                                                               "movl $" script stack overflow(too many embedded function calls) ", 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                ".Lf840ae_00086e22:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                "cmpb $0, 0xc(%edx)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                "jne .Lf840ae_00086e37\n"
                                                                                                                                                                                                                                                                                                                                                                                                                "movl imp_scrCompilePub, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                "cmpb $0, 0x24(%eax)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                "je .Lf840ae_00086eb9\n"
                                                                                                                                                                                                                                                                                                                                                                                                                ".Lf840ae_00086e37:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                "cmpb $0, scrVmPub+22\n"
                                                                                                                                                                                                                                                                                                                                                                                                                "je .Lf840ae_00086e60\n"
                                                                                                                                                                                                                                                                                                                                                                                                                ".Lf840ae_00086e40:\n"
                                                                                                                                                                                                                                                                                                                                                                                                                "movl imp_scrVarPub, %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                "movl 0x10(%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                                                                                                "movl %eax, 8(%esp)\n"
                                                                                                                                                                                                                                                                                                                                                                                                                "movl $"\x15 %
                                                                                                                                                                                                                                                                                                                                             s ", 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll Com_Error\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086e60:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl 4(%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086e66:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, 0x14(%eax)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086c06\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086e77:\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, 0xa(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_00086e9a\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmGlob+20, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "testl %edx, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "movzbl scrVmPub+22, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "cmovel %edx, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movb %al, scrVmPub+22\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086e9a:\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+8, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "testl %eax, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000861be\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, scrVmPub+20\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000861be\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086dda\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086eb9:\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, 0xa(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_00086edc\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmGlob+20, %esi\n"
                                                                                                                                                                                                                                                                                                                                               "testl %esi, %esi\n"
                                                                                                                                                                                                                                                                                                                                               "movzbl scrVmPub+22, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "cmovel %edx, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movb %al, scrVmPub+22\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086edc:\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+8, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "testl %ebx, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000861be\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, scrVmPub+20\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000861be\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086e40\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086efc:\n"
                                                                                                                                                                                                                                                                                                                                               "addl $1, 8(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086f00:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x34(%ebx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll ClearVariableValue\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x34(%ebx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll GetObjectA\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, -0x44(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 8(%ebp), %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00084119\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086f27:\n"
                                                                                                                                                                                                                                                                                                                                               "addl $8, 0x14(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "addl $2, 8(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086f2f:\n"
                                                                                                                                                                                                                                                                                                                                               "movl -0x44(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll GetVarType\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086c06\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086f41:\n"
                                                                                                                                                                                                                                                                                                                                               "movl %edx, 0x14(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "movl $3, 0x14(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x10(%edx), %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "testl %ebx, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_00086dbc\n"
                                                                                                                                                                                                                                                                                                                                               "movl $" first parameter of waittill must evaluate to a string ", 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086dbc\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086f68:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x34(%ebx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll ClearVariableValue\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x34(%ebx), %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "movl %ebx, -0x48(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 8(%ebp), %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00084119\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086f87:\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+28, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "testl %eax, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               "movl $0, scrVmPub+28\n"
                                                                                                                                                                                                                                                                                                                                               "movl 8(%ebp), %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00084119\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086fa6:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x10(%edx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "testl %eax, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_00086fba\n"
                                                                                                                                                                                                                                                                                                                                               "movl $" first parameter of endon must evaluate to a string ", 0x10(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086fba:\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, 0xc(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_00086fcf\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrCompilePub, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, 0x24(%eax)\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_0008704f\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086fcf:\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, scrVmPub+22\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_00086ff8\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086fd8:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x10(%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 8(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl $"\x15 %
                                                                                                                                                                                                                                                                                                                                             s ", 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll Com_Error\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00086ff8:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl (%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll GetVarType\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086e66\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_0008700c:\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, 0xa(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_0008702f\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmGlob+20, %esi\n"
                                                                                                                                                                                                                                                                                                                                               "testl %esi, %esi\n"
                                                                                                                                                                                                                                                                                                                                               "movzbl scrVmPub+22, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "cmovel %edx, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movb %al, scrVmPub+22\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_0008702f:\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+8, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "testl %ebx, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000861be\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, scrVmPub+20\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000861be\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086bd4\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_0008704f:\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, 0xa(%edx)\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_00087072\n"
                                                                                                                                                                                                                                                                                                                                               "movl $1, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmGlob+20, %esi\n"
                                                                                                                                                                                                                                                                                                                                               "testl %esi, %esi\n"
                                                                                                                                                                                                                                                                                                                                               "movzbl scrVmPub+22, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "cmovel %edx, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movb %al, scrVmPub+22\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00087072:\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+8, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "testl %ebx, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000861be\n"
                                                                                                                                                                                                                                                                                                                                               "cmpb $0, scrVmPub+20\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000861be\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086fd8\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00087092:\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+28, %edi\n"
                                                                                                                                                                                                                                                                                                                                               "testl %edi, %edi\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_000870a0:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"

                                                                                                                                                                                                                                                                                                                                               "movl (%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "movl %edx, 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 4(%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll RemoveRefToValue\n"

                                                                                                                                                                                                                                                                                                                                               "movl $0, scrVmPub+28\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_000870c3:\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+28, %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "testl %ecx, %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_00087114\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_000870cd:\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+16, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "addl $8, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 0x14(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086d8c\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_000870dd:\n"
                                                                                                                                                                                                                                                                                                                                               "addl $4, 8(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_000870e1:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl 4(%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "cmpl $8, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_00086d8c\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_000870f0:\n"
                                                                                                                                                                                                                                                                                                                                               "movl (%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl %edx, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll RemoveRefToValue\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "subl $8, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 0x14(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 4(%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "cmpl $8, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000870f0\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086d8c\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00087114:\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+16, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "movl (%edx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 4(%edx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll RemoveRefToValue\n"
                                                                                                                                                                                                                                                                                                                                               "subl $8, scrVmPub+16\n"
                                                                                                                                                                                                                                                                                                                                               "movl scrVmPub+28, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "subl $1, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, scrVmPub+28\n"
                                                                                                                                                                                                                                                                                                                                               "testl %eax, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_00087114\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_000870cd\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00087145:\n"
                                                                                                                                                                                                                                                                                                                                               "addl $4, 8(%ebp)\n"

                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00087149:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 8(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl (%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "addl $4, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 8(%ebp)\n"

                                                                                                                                                                                                                                                                                                                                               "movl %edx, %esi\n"
                                                                                                                                                                                                                                                                                                                                               "testl %edx, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000871ed\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_0008715e:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "addl $8, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 0x14(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00086d8c\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_0008716c:\n"
                                                                                                                                                                                                                                                                                                                                               "movl imp_scrVarPub, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x34(%ebx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll ClearVariableValue\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x34(%ebx), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll GetObjectA\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, -0x44(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00087190:\n"
                                                                                                                                                                                                                                                                                                                                               "addl $1, 8(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00087194:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"

                                                                                                                                                                                                                                                                                                                                               "movl (%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "movl %edx, 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 4(%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll RemoveRefToValue\n"

                                                                                                                                                                                                                                                                                                                                               "subl $8, 0x14(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00084ce4\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_000871b1:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl 4(%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "cmpl $8, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "je .Lf840ae_00084ce4\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_000871c0:\n"
                                                                                                                                                                                                                                                                                                                                               "movl (%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl %edx, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll RemoveRefToValue\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "subl $8, %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, 0x14(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 4(%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "cmpl $8, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000871c0\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00084ce4\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_000871e4:\n"
                                                                                                                                                                                                                                                                                                                                               "addl $2, 8(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_000871ed:\n"
                                                                                                                                                                                                                                                                                                                                               "xorl %ebx, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_000871ef:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 0x14(%ebp), %eax\n"

                                                                                                                                                                                                                                                                                                                                               "movl (%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "movl %edx, 4(%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl 4(%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "movl %eax, (%esp)\n"
                                                                                                                                                                                                                                                                                                                                               "calll RemoveRefToValue\n"

                                                                                                                                                                                                                                                                                                                                               "subl $8, 0x14(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "addl $1, %ebx\n"
                                                                                                                                                                                                                                                                                                                                               "cmpl %ebx, %esi\n"
                                                                                                                                                                                                                                                                                                                                               "jne .Lf840ae_000871ef\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_0008715e\n"
                                                                                                                                                                                                                                                                                                                                               ".Lf840ae_00087213:\n"
                                                                                                                                                                                                                                                                                                                                               "movl 8(%ebp), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "leal 2(%eax), %edx\n"
                                                                                                                                                                                                                                                                                                                                               "movzwl (%eax), %eax\n"
                                                                                                                                                                                                                                                                                                                                               "subl %eax, %edx\n"
                                                                                                                                                                                                                                                                                                                                               "movl %edx, 8(%ebp)\n"
                                                                                                                                                                                                                                                                                                                                               "movl %edx, %ecx\n"
                                                                                                                                                                                                                                                                                                                                               "jmp .Lf840ae_00084119\n"
                                                                                                                                                                                                                                                                                                                                               ".section .rodata\n"
                                                                                                                                                                                                                                                                                                                                               ".balign 4\n"
                                                                                                                                                                                                                                                                                                                                               ".Ljt_840ae_0:\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085668\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000855d4\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008452e\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085554\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084fba\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085085\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084fe1\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008500a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085035\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008505d\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000850ae\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000850e4\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000850f6\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008511c\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008512f\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085142\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085177\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000851a8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000851be\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000851ef\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085204\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085217\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008522c\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085264\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000852a7\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000852bc\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085441\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085457\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008546d\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085483\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008540c\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a96\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084ac3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000853fb\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085536\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084db0\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000854fd\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085514\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085499\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000854bc\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000854f0\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a46\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a64\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000853ca\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084546\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085396\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084551\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085575\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000849ea\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a1a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085331\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084b58\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085363\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084c37\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084c93\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000849ca\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000852d2\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084c20\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085d12\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085d2f\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085d5c\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085d64\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084341\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085bc9\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085cf4\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085d03\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085bd8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085be7\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084335\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084169\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085bf6\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085cc7\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085cd6\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085ce5\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085f01\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008415d\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008420d\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008448d\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084131\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084577\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084587\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000845e3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000846bc\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085f10\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000847fa\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008485d\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084936\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085f69\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084ce4\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085fcb\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000856fd\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085724\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085737\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008574a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008575d\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008578c\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000857b2\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000857e3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008559c\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000855ac\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084d78\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084bf0\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084b41\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085d8b\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085303\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008531a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085809\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085820\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085837\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008584e\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085865\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008587c\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085893\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000858aa\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000858c1\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000858d8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000858ef\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085906\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008591d\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085930\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085930\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085a41\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085adc\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084131\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085da2\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085ec3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085ed7\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00085ef0\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084dd2\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084ea9\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084ec3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084efd\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084b7d\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000849dd\n"
                                                                                                                                                                                                                                                                                                                                               ".text\n"
                                                                                                                                                                                                                                                                                                                                               ".section .rodata\n"
                                                                                                                                                                                                                                                                                                                                               ".balign 4\n"
                                                                                                                                                                                                                                                                                                                                               ".Ljt_840ae_1:\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d5a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d5a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d3e\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d3e\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d3e\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084ce4\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086f00\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d89\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d89\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086f68\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086f68\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086f87\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086699\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00087092\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870a0\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870c3\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084ce4\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870dd\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870dd\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870e1\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870dd\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000870e1\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00087145\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00087149\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00087145\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00087149\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_0008716c\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086efc\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000871e4\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000871e4\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000871e4\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000871e4\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00087213\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084a3a\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00087190\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00087194\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_000871b1\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00087194\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084ca5\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00084116\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d3e\n"
                                                                                                                                                                                                                                                                                                                                               ".text\n"
                                                                                                                                                                                                                                                                                                                                               ".section .rodata\n"
                                                                                                                                                                                                                                                                                                                                               ".balign 4\n"
                                                                                                                                                                                                                                                                                                                                               ".Ljt_840ae_2:\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d10\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d10\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d10\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d10\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cff\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cff\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cff\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cff\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cff\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cbc\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cbc\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cbc\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cbc\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cbc\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cbc\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cbc\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cdf\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cdf\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cdf\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cdf\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cdf\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cdf\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cdf\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cff\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086cff\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086aa8\n"
                                                                                                                                                                                                                                                                                                                                               ".long .Lf840ae_00086d10\n"
                                                                                                                                                                                                                                                                                                                                               ".text\n");
}
#endif

#if !defined(VM_EXECUTE_USE_ASM_REFERENCE)

extern unsigned long long Scr_EvalVariable(unsigned int id);
extern unsigned long long Scr_FindVariableField(unsigned int parentId, unsigned int name);
extern unsigned long long Scr_EvalVariableField(unsigned int id);
extern void Scr_EvalArray(VariableValue *value, VariableValue *index);
extern unsigned int Scr_EvalArrayRef(unsigned int parentId);
extern unsigned int Scr_EvalArrayIndex(unsigned int parentId, VariableValue *index);
extern void SetVariableValue(unsigned int id, VariableValue *value);
extern void ClearVariableValue(unsigned int id);
extern void ClearArray(unsigned int parentId, VariableValue *value);
extern void ClearVariableField(unsigned int parentId, unsigned int name, VariableValue *value);
extern void RemoveNextVariable(unsigned int parentId);
extern Bool IsFieldObject(unsigned int id);
extern unsigned int Scr_EvalFieldObject(unsigned int tempVariable, VariableValue *value);
extern unsigned int Scr_EvalVariableObject(unsigned int id);
extern unsigned int FindEntityId(int entnum, int classnum);
extern void Scr_EvalBoolNot(VariableValue *value);
extern void Scr_EvalBoolComplement(VariableValue *value);
extern void Scr_EvalOr(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalExOr(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalAnd(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalInequality(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalLess(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalGreater(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalLessEqual(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalGreaterEqual(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalShiftLeft(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalShiftRight(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalPlus(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalMinus(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalMultiply(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalDivide(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalMod(VariableValue *value1, VariableValue *value2);
extern void Scr_EvalSizeValue(VariableValue *value);
extern void Scr_CastVector(VariableValue *value);
extern unsigned int Scr_GetEntityIdRef(unsigned int id);
extern int GetVarType(unsigned int id);

static unsigned short VM_ReadU16(const char **pos)
{
    unsigned short v = *(const unsigned short *)*pos;
    *pos += 2;
    return v;
}

static int VM_ReadI32(const char **pos)
{
    int v = *(const int *)*pos;
    *pos += 4;
    return v;
}

static float VM_ReadF32(const char **pos)
{
    float v = *(const float *)*pos;
    *pos += 4;
    return v;
}

static VariableValue *VM_PushValue(VariableValue **top)
{
    ++*top;
    return *top;
}

static void VM_PushInt(VariableValue **top, int value)
{
    VariableValue *out = VM_PushValue(top);
    out->u.intValue = value;
    out->type = VAR_INTEGER;
}

static void VM_PushObject(VariableValue **top, unsigned int objectId)
{
    VariableValue *out = VM_PushValue(top);
    out->u.pointerValue = objectId;
    out->type = VAR_POINTER;
    AddRefToObject(objectId);
}

static VariableValue VM_ValueFromPacked(unsigned long long packed)
{
    VariableValue value;
    value.u.intValue = (int)(unsigned int)packed;
    value.type = (int)(packed >> 32);
    return value;
}

static void VM_PushPackedValue(VariableValue **top, unsigned long long packed)
{
    VariableValue *out = VM_PushValue(top);
    *out = VM_ValueFromPacked(packed);
}

static unsigned int VM_LocalVarCacheId(unsigned int slot)
{
    return scrVmPub.localVars[-(int)slot];
}

static void VM_EvalArrayRefAndPop(unsigned int *fieldValueId, VariableValue **top)
{
    unsigned int arrayId = Scr_EvalArrayRef(*fieldValueId);
    *fieldValueId = Scr_EvalArrayIndex(arrayId, *top);
    --*top;
}

static void VM_EvalBinaryAndPop(VariableValue **top, void (*eval)(VariableValue *, VariableValue *))
{
    eval(*top - 1, *top);
    --*top;
}

static void VM_CandidateFieldObjectCheck(unsigned int objectId)
{
    if (!IsFieldObject(objectId)) {
        Com_Error(1, "VM_Execute C candidate expected field object");
    }
}

static void VM_CandidatePushEntityObject(VariableValue **top, int classnum, int entnum)
{
    VariableValue *out = VM_PushValue(top);
    out->u.pointerValue = FindEntityId(entnum, classnum);
    if (out->u.pointerValue) {
        out->type = VAR_POINTER;
        AddRefToObject(out->u.pointerValue);
    } else {
        out->type = VAR_UNDEFINED;
        Com_Error(1, "VM_Execute C candidate could not resolve entity object");
    }
}

static void VM_CandidateIncDecField(unsigned int fieldValueId, VariableValue **top, int delta)
{
    VM_PushPackedValue(top, Scr_EvalVariableField(fieldValueId));
    if ((*top)->type != VAR_INTEGER) {
        Com_Error(1, "VM_Execute C candidate expected integer field for inc/dec");
    }
    (*top)->u.intValue += delta;
    SetVariableFieldValue(fieldValueId, *top);
    --*top;
}

static void VM_CandidateDumpOpcodeRing(void)
{
    int i;

    Com_Printf(str_dbg_op_hdr);
    for (i = 0; i < 32; ++i) {
        unsigned int slot = (unsigned int)(dbg_op_ring_idx + i) & 31u;
        Com_Printf(str_dbg_op_dump, i, (void *)(uintptr_t)dbg_op_ring[slot * 2u],
                   dbg_op_ring[slot * 2u + 1u]);
    }
}

static void VM_CandidateAbortBadBuiltinIndex(unsigned int builtinIndex)
{
    (void)builtinIndex;
    VM_CandidateDumpOpcodeRing();
    _exit(1);
}

static void VM_CandidateAbortNullBuiltin(unsigned int builtinIndex, const char *pos)
{
    Com_Printf(str_dbg_null_builtin, builtinIndex, pos);
    VM_CandidateDumpOpcodeRing();
    _exit(1);
}

static void VM_CandidateCompleteCall(const char **pos, VariableValue **top)
{
    unsigned int paramCount;

    *top = scrVmPub.top;
    if (scrVmPub.function_frame) {
        *pos = scrVmPub.function_frame->fs.pos;
    }

    paramCount = scrVmPub.outparamcount;
    if (paramCount) {
        scrVmPub.outparamcount = 0;
        scrVmPub.top -= paramCount;
        while (paramCount--) {
            RemoveRefToValue((*top)->type, (*top)->u);
            --*top;
        }
    }

    if (scrVmPub.inparamcount) {
        scrVmPub.inparamcount = 0;
    } else {
        VM_PushValue(top)->type = VAR_UNDEFINED;
    }
}

static void VM_CandidateCallBuiltin(const char **pos, VariableValue **top, unsigned int paramCount)
{
    struct scrCompilePub_t *compilePub = (struct scrCompilePub_t *)imp_scrCompilePub;
    unsigned int builtinIndex = VM_ReadU16(pos);
    BuiltinFunction func;

    scrVmPub.outparamcount = paramCount;
    scrVmPub.top = *top;
    if (scrVmPub.function_frame) {
        scrVmPub.function_frame->fs.pos = *pos;
    }

    if (builtinIndex >= 0x4000) {
        VM_CandidateAbortBadBuiltinIndex(builtinIndex);
    }
    if (!compilePub->func_table[builtinIndex]) {
        VM_CandidateAbortNullBuiltin(builtinIndex, *pos);
    }

    func = (BuiltinFunction)(uintptr_t)compilePub->func_table[builtinIndex];
    { extern void dbg_check439_pre(unsigned int); dbg_check439_pre(builtinIndex); }
    { extern unsigned int g_dbg_lastBuiltin; extern void *g_dbg_lastBuiltinFn;
      g_dbg_lastBuiltin = builtinIndex; g_dbg_lastBuiltinFn = (void *)func; }
    func();
    { extern void dbg_check439(void); dbg_check439(); }
    VM_CandidateCompleteCall(pos, top);
}

static void VM_CandidateCallBuiltinMethod(const char **pos, VariableValue **top, unsigned int paramCount)
{
    struct scrCompilePub_t *compilePub = (struct scrCompilePub_t *)imp_scrCompilePub;
    unsigned int builtinIndex = VM_ReadU16(pos);
    unsigned int objectId;
    unsigned int entRefValue;
    scr_entref_t entref;
    BuiltinMethod method;

    scrVmPub.outparamcount = paramCount;
    scrVmPub.top = *top - 1;

    if ((*top)->type != VAR_POINTER) {
        RemoveRefToValue((*top)->type, (*top)->u);
        ((struct scrVarPub_t *)imp_scrVarPub)->error_index = (unsigned int)-1;
        Com_Error(1, "VM_Execute C candidate method target is not an entity");
    }

    objectId = (*top)->u.pointerValue;
    if (GetVarType(objectId) != 21) {
        int objectType = GetVarType(objectId);
        RemoveRefToObject(objectId);
        ((struct scrVarPub_t *)imp_scrVarPub)->error_index = (unsigned int)-1;
        Com_Error(1, "VM_Execute C candidate method target type %d is not an entity", objectType);
    }

    entRefValue = Scr_GetEntityIdRef(objectId);
    entref.entnum = (unsigned short)(entRefValue & 0xffff);
    entref.classnum = (unsigned short)(entRefValue >> 16);
    RemoveRefToObject(objectId);

    if (scrVmPub.function_frame) {
        scrVmPub.function_frame->fs.pos = *pos;
    }

    method = (BuiltinMethod)(uintptr_t)compilePub->func_table[builtinIndex];
    { extern unsigned int g_dbg_lastBuiltin; g_dbg_lastBuiltin = builtinIndex | 0x10000; }
    ((void (*)(scr_entref_t))method)(entref);
    { extern void dbg_check439(void); dbg_check439(); }
    VM_CandidateCompleteCall(pos, top);
}

static unsigned int VM_CandidateFinishOutermost(VariableValue *startTop, unsigned int localId)
{
    scrVmPub.top = startTop;
    g_script_error_level--;
    return localId;
}

static void VM_CandidateRestoreCaller(unsigned int parentLocalId, const char **pos,
                                      unsigned int *localId, unsigned int *localVarCount)
{
    function_frame_t *frame = scrVmPub.function_frame;

    RemoveRefToObject(*localId);
    *pos = frame->fs.pos;
    *localVarCount = frame->fs.localVarCount;

    *localId = parentLocalId ? parentLocalId : frame->fs.localId;
}

static void VM_CandidatePopToFrameSentinel(VariableValue **top)
{
    while ((*top)->type != 7) {
        RemoveRefToValue((*top)->type, (*top)->u);
        --*top;
    }
}

static int VM_CandidateThreadSuspendReturn(const char **pos, unsigned int *localId,
                                           unsigned int *localVarCount, VariableValue **top,
                                           VariableValue **startTop, unsigned int *resultLocalId,
                                           unsigned int *threadCount);

static int VM_CandidateHandleEnd(const char **pos, unsigned int *localId,
                                 unsigned int *localVarCount, VariableValue **top,
                                 VariableValue **startTop, unsigned int *resultLocalId,
                                 unsigned int *threadCount)
{
    unsigned int parentLocalId = GetSafeParentLocalId(*localId);
    extern void dbg_end_probe(int);

    dbg_end_probe(0);
    Scr_KillThread(*localId);
    dbg_end_probe(1);
    scrVmPub.localVars -= *localVarCount;
    VM_CandidatePopToFrameSentinel(top);
    dbg_end_probe(2);

    scrVmPub.function_count--;
    scrVmPub.function_frame--;

    if (parentLocalId == 0) {

        int r = VM_CandidateThreadSuspendReturn(pos, localId, localVarCount, top,
                                               startTop, resultLocalId, threadCount);
        dbg_end_probe(3);
        return r;
    }

    (*top)->type = VAR_UNDEFINED;
    VM_CandidateRestoreCaller(parentLocalId, pos, localId, localVarCount);
    return 0;
}

static int VM_CandidateHandleReturn(const char **pos, unsigned int *localId,
                                    unsigned int *localVarCount, VariableValue **top,
                                    VariableValue **startTop, unsigned int *resultLocalId,
                                    unsigned int *threadCount)
{
    unsigned int parentLocalId = GetSafeParentLocalId(*localId);
    VariableValue returnValue = **top;

    Scr_KillThread(*localId);
    scrVmPub.localVars -= *localVarCount;
    --*top;
    VM_CandidatePopToFrameSentinel(top);

    scrVmPub.function_count--;
    scrVmPub.function_frame--;

    if (parentLocalId == 0) {

        return VM_CandidateThreadSuspendReturn(pos, localId, localVarCount, top,
                                               startTop, resultLocalId, threadCount);
    }

    **top = returnValue;
    VM_CandidateRestoreCaller(parentLocalId, pos, localId, localVarCount);
    return 0;
}

static unsigned int VM_CandidateSuspendCurrentStack(const char *archivePos,
                                                    unsigned int localVarCount,
                                                    VariableValue *top,
                                                    VariableValue *startTop,
                                                    unsigned int *localId)
{
    int stackSize = (int)(top - startTop);
    return (unsigned int)(uintptr_t)VM_ArchiveStack(stackSize, archivePos, top, localVarCount, localId);
}

static unsigned int VM_CandidateFinishSuspend(VariableValue *startTop, unsigned int localId)
{
    scrVmPub.top = startTop;
    g_script_error_level--;
    return localId;
}

static int VM_CandidateThreadSuspendReturn(const char **pos, unsigned int *localId,
                                           unsigned int *localVarCount, VariableValue **top,
                                           VariableValue **startTop, unsigned int *resultLocalId,
                                           unsigned int *threadCount)
{
    ((*startTop) + 1)->type = VAR_UNDEFINED;
    if (*threadCount == 0) {
        *resultLocalId = VM_CandidateFinishSuspend(*startTop, *localId);
        return 1;
    }
    --*threadCount;
    RemoveRefToObject(*localId);
    {
        function_frame_t *ff = scrVmPub.function_frame;
        *pos = ff->fs.pos;
        *localId = ff->fs.localId;
        *localVarCount = ff->fs.localVarCount;
        *top = ff->fs.top;
        *startTop = ff->fs.startTop;
        (*top)->type = ff->topType;
        *top = *top + 1;
    }
    return 0;
}

static unsigned int VM_CandidateWaitTimeFromValue(VariableValue *value)
{
    if (value->type == VAR_FLOAT) {
        float seconds = value->u.floatValue;
        int ticks;

        if (seconds < 0.0f) {
            Scr_SetErrorMessageAndJump("negative wait is not allowed");
            return 0;
        }

        ticks = (int)floorf(seconds * 20.0f + 0.5f);
        if (!ticks && seconds != 0.0f) {
            ticks = 1;
        }
        return (unsigned int)ticks;
    }

    if (value->type == VAR_INTEGER) {
        return (unsigned int)(value->u.intValue * 20);
    }

    ((struct scrVarPub_t *)imp_scrVarPub)->error_index = 2;
    Scr_SetErrorMessageAndJump(va("type %s is not a float", ((const char *const *)imp_var_typename)[value->type]));
    return 0;
}

static int VM_CandidateHandleWait(const char **pos, unsigned int *localVarCount,
                                  VariableValue **top, VariableValue **startTop,
                                  unsigned int *localId, unsigned int *resultLocalId,
                                  unsigned int *threadCount)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    unsigned int waitTime = VM_CandidateWaitTimeFromValue(*top);
    VariableStackBuffer *stackValue;
    VariableValue tempValue;
    unsigned int waitArray;
    unsigned int stackId;
    const char *archivePos = *pos;

    if (waitTime > 0x00fffffeu) {
        ((struct scrVarPub_t *)imp_scrVarPub)->error_index = 2;
        Scr_SetErrorMessageAndJump("wait time too long");
    }

    if (waitTime) {
        Scr_ResetTimeout();
    }

    waitTime = (waitTime + (unsigned int)varPub->time) & 0x00ffffffu;
    --*top;

    stackValue = (VariableStackBuffer *)(uintptr_t)
        VM_CandidateSuspendCurrentStack(archivePos, *localVarCount, *top, *startTop, localId);
    tempValue.u.stackValue = SCR_STACK_ENC(stackValue);
    tempValue.type = 10;

    waitArray = GetArray(GetVariable(varPub->timeArrayId, waitTime));
    stackId = GetNewObjectVariable(waitArray, *localId);
    SetNewVariableValue(stackId, &tempValue);
    Scr_SetThreadWaitTime(*localId, waitTime);

    return VM_CandidateThreadSuspendReturn(pos, localId, localVarCount, top,
                                           startTop, resultLocalId, threadCount);
}

static int VM_CandidateHandleWaitTillFrameEnd(const char **pos, unsigned int *localVarCount,
                                              VariableValue **top, VariableValue **startTop,
                                              unsigned int *localId, unsigned int *resultLocalId,
                                              unsigned int *threadCount)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    VariableStackBuffer *stackValue;
    VariableValue tempValue;
    unsigned int waitArray;
    unsigned int stackId;
    const char *archivePos = *pos;

    stackValue = (VariableStackBuffer *)(uintptr_t)
        VM_CandidateSuspendCurrentStack(archivePos, *localVarCount, *top, *startTop, localId);
    tempValue.u.stackValue = SCR_STACK_ENC(stackValue);
    tempValue.type = 10;

    waitArray = GetArray(GetVariable(varPub->timeArrayId, (unsigned int)varPub->time));
    stackId = GetNewObjectVariableReverse(waitArray, *localId);
    SetNewVariableValue(stackId, &tempValue);
    Scr_SetThreadWaitTime(*localId, (unsigned int)varPub->time);

    return VM_CandidateThreadSuspendReturn(pos, localId, localVarCount, top,
                                           startTop, resultLocalId, threadCount);
}

static int VM_CandidateHandleWaitTill(const char **pos, unsigned int *localVarCount,
                                      VariableValue **top, VariableValue **startTop,
                                      unsigned int *localId, unsigned int *resultLocalId,
                                      unsigned int *threadCount)
{
    const char *archivePos = *pos;
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    static unsigned int traceCount;
    unsigned int notifyListOwnerId;
    unsigned int stringValue;
    VariableStackBuffer *stackValue;
    VariableValue tempValue;
    unsigned int notifyListId;
    unsigned int notifyNameListId;
    unsigned int stackId;
    unsigned int selfId;
    unsigned int selfNameId;
    unsigned int selfVarId;

    if ((*top)->type != VAR_POINTER || !IsFieldObject((*top)->u.pointerValue)) {
        Scr_SetErrorMessageAndJump("first waittill argument is not a field object");
    }
    notifyListOwnerId = (*top)->u.pointerValue;
    --*top;

    if ((*top)->type != VAR_STRING) {
        Scr_SetErrorMessageAndJump("second waittill argument is not a string");
    }
    stringValue = (*top)->u.stringValue;
    --*top;

    stackValue = (VariableStackBuffer *)(uintptr_t)
        VM_CandidateSuspendCurrentStack(archivePos, *localVarCount, *top, *startTop, localId);
    tempValue.u.stackValue = SCR_STACK_ENC(stackValue);
    tempValue.type = 10;

    notifyListId = GetArray(GetVariable(notifyListOwnerId, 0x1fffe));
    notifyNameListId = GetArray(GetVariable(notifyListId, stringValue));
    stackId = GetNewObjectVariable(notifyNameListId, *localId);
    SetNewVariableValue(stackId, &tempValue);

    tempValue.u.pointerValue = notifyListOwnerId;
    tempValue.type = VAR_POINTER;
    selfId = Scr_GetSelf(*localId);
    selfNameId = GetArray(GetObjectVariable(varPub->pauseArrayId, selfId));
    selfVarId = GetNewObjectVariable(selfNameId, *localId);
    SetNewVariableValue(selfVarId, &tempValue);
    Scr_SetThreadNotifyName(*localId, stringValue);
    if (traceCount < 64) {
        if (getenv("DBGSPAM"))
            Com_Printf("[team-trace] waittill owner=%u name=%u('%s') local=%u self=%u list=%u nameList=%u stack=%u pos=%p\n",
                       notifyListOwnerId, stringValue, SL_ConvertToString(stringValue), *localId, selfId,
                       notifyListId, notifyNameListId, stackId, (const void *)archivePos);
        if (getenv("DBGSPAM"))
            Com_Printf("[team-trace] code %02x | %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                       (unsigned char)archivePos[-1],
                       (unsigned char)archivePos[0], (unsigned char)archivePos[1],
                       (unsigned char)archivePos[2], (unsigned char)archivePos[3],
                       (unsigned char)archivePos[4], (unsigned char)archivePos[5],
                       (unsigned char)archivePos[6], (unsigned char)archivePos[7],
                       (unsigned char)archivePos[8], (unsigned char)archivePos[9],
                       (unsigned char)archivePos[10], (unsigned char)archivePos[11],
                       (unsigned char)archivePos[12], (unsigned char)archivePos[13],
                       (unsigned char)archivePos[14], (unsigned char)archivePos[15]);
        if (getenv("DBGSPAM"))
            Scr_PrintPrevCodePos(0, archivePos, 0);
        ++traceCount;
    }

    return VM_CandidateThreadSuspendReturn(pos, localId, localVarCount, top,
                                           startTop, resultLocalId, threadCount);
}

static void VM_CandidateHandleNotifyCall(VariableValue **top)
{
    unsigned int notifyListOwnerId;
    unsigned int stringValue;

    if ((*top)->type != VAR_POINTER || !IsFieldObject((*top)->u.pointerValue)) {
        Scr_SetErrorMessageAndJump("notify object is not a field object");
    }
    notifyListOwnerId = (*top)->u.pointerValue;
    --*top;

    if ((*top)->type != VAR_STRING) {
        Scr_SetErrorMessageAndJump("notify name is not a string");
    }
    stringValue = (*top)->u.stringValue;
    --*top;

    VM_Notify(notifyListOwnerId, stringValue, *top);
    RemoveRefToObject(notifyListOwnerId);
    SL_RemoveRefToString(stringValue);

    while ((*top)->type != 8) {
        RemoveRefToValue((*top)->type, (*top)->u);
        --*top;
    }
    --*top;
}

static void VM_CandidateHandleEndOnCallback(unsigned int localId, VariableValue **top)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    unsigned int notifyListOwnerId;
    unsigned int stringValue;
    unsigned int threadId;
    unsigned int notifyListId;
    unsigned int notifyNameListId;
    unsigned int stackId;
    unsigned int selfNameId;
    unsigned int selfVarId;
    VariableValue tempValue;

    if ((*top)->type != VAR_POINTER || !IsFieldObject((*top)->u.pointerValue)) {
        Scr_SetErrorMessageAndJump("endon object is not a field object");
    }
    notifyListOwnerId = (*top)->u.pointerValue;

    if ((*top - 1)->type != VAR_STRING) {
        Scr_SetErrorMessageAndJump("endon notify name is not a string");
    }
    stringValue = (*top - 1)->u.stringValue;

    AddRefToObject(localId);
    threadId = AllocThread(localId);

    notifyListId = GetArray(GetVariable(notifyListOwnerId, 0x1fffe));
    notifyNameListId = GetArray(GetVariable(notifyListId, stringValue));
    stackId = GetObjectVariable(notifyNameListId, threadId);
    (void)stackId;
    RemoveRefToObject(threadId);

    tempValue.u.pointerValue = notifyListOwnerId;
    tempValue.type = VAR_POINTER;
    selfNameId = GetArray(GetObjectVariable(varPub->pauseArrayId, localId));
    selfVarId = GetNewObjectVariable(selfNameId, threadId);
    SetNewVariableValue(selfVarId, &tempValue);
    Scr_SetThreadNotifyName(threadId, stringValue);

    *top -= 2;
}

static void VM_CandidateHandleSwitch(const char **pos, VariableValue **top)
{
    const char *opcodeStart = *pos - 1;
    const char *tablePos = opcodeStart + 5 + *(const int *)*pos;
    unsigned int caseCount;
    unsigned int caseValue;
    unsigned int i;
    const char *defaultPos = NULL;

    *pos = tablePos;
    caseCount = VM_ReadU16(pos);

    if ((*top)->type == VAR_STRING) {
        caseValue = (*top)->u.stringValue;
        SL_RemoveRefToString(caseValue);
    } else if ((*top)->type == VAR_INTEGER) {
        if (!IsValidArrayIndex((unsigned int)(*top)->u.intValue)) {
            Scr_SetErrorMessageAndJump("switch index out of range");
        }
        caseValue = GetInternalVariableIndex((unsigned int)(*top)->u.intValue);
    } else {
        Scr_SetErrorMessageAndJump(va("cannot switch on %s", ((const char *const *)imp_var_typename)[(*top)->type]));
        caseValue = 0;
    }

    for (i = 0; i < caseCount; i++) {
        unsigned int currentCaseValue = *(const unsigned int *)*pos;
#    if defined(__x86_64__) || defined(_M_X64)
        const char *currentCodePos = SCR_CODEPOS_PTR(*(const unsigned int *)(*pos + 4));
#    else
        const char *currentCodePos = *(const char *const *)(*pos + 4);
#    endif
        *pos += 8;

        if (currentCaseValue == caseValue) {
            *pos = currentCodePos;
            --*top;
            return;
        }
        if (!currentCaseValue) {
            defaultPos = currentCodePos;
        }
    }

    if (defaultPos) {
        *pos = defaultPos;
    }
    --*top;
}

static void VM_CandidateSaveCallerFrame(const char *returnPos, unsigned int localVarCount,
                                        VariableValue *top, VariableValue *startTop)
{
    function_frame_t *frame = scrVmPub.function_frame;

    if (!frame) {
        return;
    }

    frame->fs.pos = returnPos;
    if (!frame->fs.top) {
        frame->fs.top = top;
        frame->fs.startTop = startTop;
        frame->topType = top->type;
    }
    frame->fs.localVarCount = localVarCount;
}

static void VM_CandidateEnterScriptFrame(const char **pos, unsigned int *localId,
                                         unsigned int *localVarCount, VariableValue **top,
                                         VariableValue **startTop, const char *targetPos,
                                         unsigned int newLocalId, const char *returnPos)
{
    VM_CandidateSaveCallerFrame(returnPos, *localVarCount, *top, *startTop);
    *localVarCount = 0;
    *localId = newLocalId;
    *pos = targetPos;

    scrVmPub.function_count++;
    scrVmPub.function_frame++;
    scrVmPub.function_frame->fs.pos = NULL;
    scrVmPub.function_frame->fs.localId = newLocalId;
    scrVmPub.function_frame->fs.localVarCount = 0;
    scrVmPub.function_frame->fs.top = NULL;
    scrVmPub.function_frame->fs.startTop = NULL;
    scrVmPub.function_frame->topType = 0;
}

static void VM_CandidateCheckFrameDepth(void)
{
    if (scrVmPub.function_count > 30) {
        ((struct scrVarPub_t *)imp_scrVarPub)->error_index = 1;
        Scr_SetErrorMessageAndJump("script stack overflow (too many embedded function calls)");
    }
}

static const char *VM_CandidateReadScriptCallTarget(const char **pos)
{
#    if defined(__x86_64__) || defined(_M_X64)
    const char *target = SCR_CODEPOS_PTR(*(const unsigned int *)*pos);
#    else
    const char *target = *(const char *const *)*pos;
#    endif
    *pos += 4;
    return target;
}

static const char *VM_CandidatePopCodepos(VariableValue **top)
{
    const char *target;

    if ((*top)->type != 9) {
        unsigned int badType = (*top)->type;
        RemoveRefToValue((*top)->type, (*top)->u);
        ((struct scrVarPub_t *)imp_scrVarPub)->error_index = 1;
        Scr_SetErrorMessageAndJump(va("%s is not a function pointer", ((const char *const *)imp_var_typename)[badType]));
    }

    target = SCR_CODEPOS_GET((*top)->u);
    --*top;
    return target;
}

static unsigned int VM_CandidatePopObjectForCall(VariableValue **top, unsigned int errorIndex)
{
    unsigned int objectId;

    if ((*top)->type != VAR_POINTER) {
        unsigned int badType = (*top)->type;
        RemoveRefToValue((*top)->type, (*top)->u);
        ((struct scrVarPub_t *)imp_scrVarPub)->error_index = errorIndex;
        Scr_SetErrorMessageAndJump(va("%s is not an object", ((const char *const *)imp_var_typename)[badType]));
    }

    objectId = (*top)->u.pointerValue;
    --*top;
    return objectId;
}

static void VM_CandidateEnterFunctionCall(const char **pos, unsigned int *localId,
                                          unsigned int *localVarCount, VariableValue **top,
                                          VariableValue **startTop, const char *targetPos,
                                          const char *returnPos)
{
    unsigned int selfId;
    unsigned int newLocalId;

    VM_CandidateCheckFrameDepth();
    selfId = Scr_GetSelf(*localId);
    AddRefToObject(selfId);
    newLocalId = AllocChildThread(selfId, *localId);
    VM_CandidateEnterScriptFrame(pos, localId, localVarCount, top, startTop, targetPos, newLocalId, returnPos);
}

static void VM_CandidateEnterMethodCall(const char **pos, unsigned int *localId,
                                        unsigned int *localVarCount, VariableValue **top,
                                        VariableValue **startTop, const char *targetPos,
                                        const char *returnPos)
{
    unsigned int objectId;
    unsigned int newLocalId;

    VM_CandidateCheckFrameDepth();
    objectId = VM_CandidatePopObjectForCall(top, 1);
    newLocalId = AllocChildThread(objectId, *localId);
    VM_CandidateEnterScriptFrame(pos, localId, localVarCount, top, startTop, targetPos, newLocalId, returnPos);
}

static void VM_CandidatePrepareThreadCallerFrame(const char *returnPos,
                                                 unsigned int localVarCount,
                                                 VariableValue *top,
                                                 VariableValue *callerStartTop,
                                                 unsigned int paramCount)
{
    function_frame_t *frame = scrVmPub.function_frame;
    VariableValue *threadStartTop = top - paramCount;

    frame->fs.pos = returnPos;
    frame->fs.localVarCount = localVarCount;
    frame->fs.top = threadStartTop;
    frame->fs.startTop = callerStartTop;
    frame->topType = threadStartTop->type;
}

static void VM_CandidateEnterFunctionThreadCall(const char **pos, unsigned int *localId,
                                                unsigned int *localVarCount, VariableValue **top,
                                                VariableValue **startTop, const char *targetPos,
                                                const char *returnPos, unsigned int paramCount)
{
    unsigned int selfId;
    unsigned int newLocalId;
    VariableValue *callerStartTop = *startTop;

    VM_CandidateCheckFrameDepth();
    selfId = Scr_GetSelf(*localId);
    AddRefToObject(selfId);
    newLocalId = AllocThread(selfId);
    VM_CandidatePrepareThreadCallerFrame(returnPos, *localVarCount, *top,
                                         callerStartTop, paramCount);
    *startTop = *top - paramCount;
    (*startTop)->type = 8;
    VM_CandidateEnterScriptFrame(pos, localId, localVarCount, top, startTop, targetPos, newLocalId, returnPos);
}

static void VM_CandidateEnterMethodThreadCall(const char **pos, unsigned int *localId,
                                              unsigned int *localVarCount, VariableValue **top,
                                              VariableValue **startTop, const char *targetPos,
                                              const char *returnPos, unsigned int paramCount)
{
    unsigned int objectId;
    unsigned int newLocalId;
    VariableValue *callerStartTop = *startTop;

    VM_CandidateCheckFrameDepth();
    objectId = VM_CandidatePopObjectForCall(top, 1);
    newLocalId = AllocThread(objectId);
    VM_CandidatePrepareThreadCallerFrame(returnPos, *localVarCount, *top,
                                         callerStartTop, paramCount);
    *startTop = *top - paramCount;
    (*startTop)->type = 8;
    VM_CandidateEnterScriptFrame(pos, localId, localVarCount, top, startTop, targetPos, newLocalId, returnPos);
}

static unsigned int VM_Execute_CXX_Candidate_Pass66(struct function_stack_t fs)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    unsigned int fieldValueId = 0;
    unsigned int objectId = 0;
    unsigned int waitTime = 0;
    unsigned int caseCount = 0;
    unsigned int caseValue = 0;
    unsigned int currentCaseValue = 0;
    const char *currentCodePos = NULL;
    unsigned int thread_count = 0;
    const char *pos = fs.pos;
    unsigned int localId = fs.localId;
    unsigned int localVarCount = fs.localVarCount;
    VariableValue *top = fs.top;
    VariableValue *startTop = fs.startTop;
    unsigned int resultLocalId = 0;

    (void)waitTime;
    (void)caseCount;
    (void)caseValue;
    (void)currentCaseValue;
    (void)currentCodePos;
    (void)thread_count;

    g_script_error_level++;
    if (setjmp(g_script_error[g_script_error_level])) {
        RuntimeError(pos, varPub->error_index, varPub->error_message,
                     ((struct scrVmGlob_t *)scrVmGlob)->dialog_error_message);
        varPub->error_message = NULL;
        ((struct scrVmGlob_t *)scrVmGlob)->dialog_error_message = NULL;
        varPub->error_index = 0;
    }

    for (;;) {
        { extern void dbg_check439_op(unsigned int); extern unsigned int g_lastop_dbg; dbg_check439_op(g_lastop_dbg); }
        const char *opcodePos = pos;
        unsigned int opcode = *(const unsigned char *)pos++;
        { extern unsigned int g_lastop_dbg; g_lastop_dbg = opcode; }

        VM_DebugRecordOpcode(opcodePos, opcode);

#    if COD2_FEATURE_SCRIPT_DEBUGGER

        if (opcode >= 0x87 && opcode <= 0x89) {
            extern int Scr_Debugger_CodePosToLine(const char *codePos, const char **filenameOut);
            unsigned int bpLine = (unsigned int)Scr_Debugger_CodePosToLine(opcodePos, 0);

            if (opcode == 0x88)
                opcode = (unsigned int)(unsigned char)
                    Scr_HitAssignmentBreakpoint((void *)top, pos, bpLine, 0);
            else
                opcode = (unsigned int)(unsigned char)
                    Scr_HitBreakpoint((void *)top, pos, bpLine, 0);
        }
#    endif

        switch ((ScrVmOpcode)opcode) {
        case VMOP_End:
            if (VM_CandidateHandleEnd(&pos, &localId, &localVarCount, &top, &startTop, &resultLocalId, &thread_count)) {
                return resultLocalId;
            }
            break;

        case VMOP_Return:
            if (VM_CandidateHandleReturn(&pos, &localId, &localVarCount, &top, &startTop, &resultLocalId, &thread_count)) {
                return resultLocalId;
            }
            break;

        case VMOP_GetUndefined: {
            VariableValue *out = VM_PushValue(&top);
            out->type = VAR_UNDEFINED;
            break;
        }

        case VMOP_GetZero:
            VM_PushInt(&top, 0);
            break;

        case VMOP_GetByte:
            VM_PushInt(&top, *(const unsigned char *)pos++);
            break;

        case VMOP_GetNegByte:
            VM_PushInt(&top, -(int)*(const unsigned char *)pos++);
            break;

        case VMOP_GetUnsignedShort:
            VM_PushInt(&top, VM_ReadU16(&pos));
            break;

        case VMOP_GetNegUnsignedShort:
            VM_PushInt(&top, -(int)VM_ReadU16(&pos));
            break;

        case VMOP_GetInteger:
            VM_PushInt(&top, VM_ReadI32(&pos));
            break;

        case VMOP_GetFloat: {
            VariableValue *out = VM_PushValue(&top);
            out->u.floatValue = VM_ReadF32(&pos);
            out->type = VAR_FLOAT;
            break;
        }

        case VMOP_GetString:
        case VMOP_GetIString: {
            VariableValue *out = VM_PushValue(&top);
            out->u.stringValue = VM_ReadU16(&pos);
            out->type = (opcode == VMOP_GetString) ? VAR_STRING : VAR_LOCALIZED_STRING;
            SL_AddRefToString(out->u.stringValue);
            break;
        }

        case VMOP_GetVector: {
            VariableValue *out = VM_PushValue(&top);
            out->u.vectorValue = SCR_VEC_ENC((const float *)pos);
            out->type = VAR_VECTOR;
            pos += 12;
            break;
        }

        case VMOP_GetLevelObject:
            objectId = varPub->levelId;
            break;

        case VMOP_GetAnimObject:
            objectId = varPub->animId;
            break;

        case VMOP_GetSelf:
            VM_PushObject(&top, Scr_GetSelf(localId));
            break;

        case VMOP_GetLevel:
            VM_PushObject(&top, varPub->levelId);
            break;

        case VMOP_GetGame:
            VM_PushPackedValue(&top, Scr_EvalVariable(varPub->gameId));
            break;

        case VMOP_GetAnim:
            VM_PushObject(&top, varPub->animId);
            break;

        case VMOP_GetAnimation: {
            VariableValue *out = VM_PushValue(&top);
            out->u.intValue = VM_ReadI32(&pos);
            out->type = 11;
            break;
        }

        case VMOP_GetGameRef:
            fieldValueId = varPub->gameId;
            break;

        case VMOP_GetFunction: {
            VariableValue *out = VM_PushValue(&top);
#    if defined(__x86_64__) || defined(_M_X64)

            out->u.codePosValue = *(const unsigned int *)pos;
#    else
            out->u.codePosValue = *(const char *const *)pos;
#    endif
            out->type = 9;
            pos += 4;
            break;
        }

        case VMOP_CreateLocalVariable:
            scrVmPub.localVars++;
            localVarCount++;
            scrVmPub.localVars[0] = GetNewVariable(localId, VM_ReadU16(&pos));
            break;

        case VMOP_RemoveLocalVariables: {
            unsigned int count = *(const unsigned char *)pos++;
            unsigned int i;

            scrVmPub.localVars -= count;
            localVarCount -= count;
            for (i = 0; i < count; i++) {
                RemoveNextVariable(localId);
            }
            break;
        }

        case VMOP_EvalLocalVariableCached0:
        case VMOP_EvalLocalVariableCached1:
        case VMOP_EvalLocalVariableCached2:
        case VMOP_EvalLocalVariableCached3:
        case VMOP_EvalLocalVariableCached4:
        case VMOP_EvalLocalVariableCached5:
            VM_PushPackedValue(&top,
                               Scr_EvalVariable(VM_LocalVarCacheId(opcode - VMOP_EvalLocalVariableCached0)));
            break;

        case VMOP_EvalLocalVariable:
            VM_PushPackedValue(&top, Scr_EvalVariable(VM_LocalVarCacheId(*(const unsigned char *)pos++)));
            break;

        case VMOP_EvalLocalArrayCached:
            VM_PushPackedValue(&top, Scr_EvalVariable(VM_LocalVarCacheId(*(const unsigned char *)pos++)));
            Scr_EvalArray(top, top - 1);
            top--;
            break;

        case VMOP_EvalArray:
            Scr_EvalArray(top, top - 1);
            top--;
            break;

        case VMOP_EvalLocalVariableRefCached:
            fieldValueId = VM_LocalVarCacheId(0);
            VM_EvalArrayRefAndPop(&fieldValueId, &top);
            break;

        case VMOP_EvalArrayRefCached:
            fieldValueId = VM_LocalVarCacheId(*(const unsigned char *)pos++);
            VM_EvalArrayRefAndPop(&fieldValueId, &top);
            break;

        case VMOP_EvalArrayRef:

            VM_EvalArrayRefAndPop(&fieldValueId, &top);
            break;

        case VMOP_ClearArray:
            ClearArray(fieldValueId, top);
            top--;
            break;

        case VMOP_EmptyArray: {
            VariableValue *out = VM_PushValue(&top);
            out->u.pointerValue = Scr_AllocArray();
            out->type = VAR_POINTER;
            break;
        }

        case VMOP_IsDefinedSelfField:
            objectId = Scr_GetSelf(localId);
            VM_CandidateFieldObjectCheck(objectId);
            break;

        case VMOP_IsDefinedLevelField:
            objectId = varPub->levelId;
            VM_PushPackedValue(&top, Scr_EvalVariable(FindVariable(objectId, VM_ReadU16(&pos))));
            break;

        case VMOP_IsDefinedAnimField:
            objectId = varPub->animId;
            VM_PushPackedValue(&top, Scr_EvalVariable(FindVariable(objectId, VM_ReadU16(&pos))));
            break;

        case VMOP_EvalSelfFieldVariable:
            objectId = Scr_GetSelf(localId);
            VM_CandidateFieldObjectCheck(objectId);
            VM_PushPackedValue(&top, Scr_FindVariableField(objectId, VM_ReadU16(&pos)));
            break;

        case VMOP_EvalFieldVariable:
            VM_PushPackedValue(&top, Scr_FindVariableField(objectId, VM_ReadU16(&pos)));
            break;

        case VMOP_EvalLevelFieldVariable:
            objectId = varPub->levelId;
            fieldValueId = Scr_GetVariableField(objectId, VM_ReadU16(&pos));
            break;

        case VMOP_EvalAnimFieldVariable:
            objectId = varPub->animId;
            fieldValueId = Scr_GetVariableField(objectId, VM_ReadU16(&pos));
            break;

        case VMOP_EvalSelfFieldVariableRef:
            objectId = Scr_GetSelf(localId);
            fieldValueId = Scr_GetVariableField(objectId, VM_ReadU16(&pos));
            break;

        case VMOP_EvalFieldVariableRef:
            fieldValueId = Scr_GetVariableField(objectId, VM_ReadU16(&pos));
            break;

        case VMOP_ClearFieldVariable:
            ClearVariableField(objectId, VM_ReadU16(&pos), top);
            break;

        case VMOP_SafeCreateLocalVariables:
            scrVmPub.localVars++;
            localVarCount++;
            scrVmPub.localVars[0] = GetNewVariable(localId, VM_ReadU16(&pos));

            if (top->type != 8) {
                SetVariableValue(scrVmPub.localVars[0], top);
                top--;
            }
            break;

        case VMOP_ClearParams:
            if (top->type != 8) {
                SetVariableValue(VM_LocalVarCacheId(0), top);
                top--;
            }
            break;

        case VMOP_CheckClearParams: {
            unsigned int slot = *(const unsigned char *)pos++;
            if (top->type != 8) {
                SetVariableValue(VM_LocalVarCacheId(slot), top);
                top--;
            }
            break;
        }

        case VMOP_EvalLocalVariableRefCached0: {
            unsigned int slot = *(const unsigned char *)pos++;
            unsigned int varId = VM_LocalVarCacheId(slot);

            if (top->type == 7) {
                ClearVariableValue(varId);
            } else {
                SetVariableValue(varId, top);
                top--;
            }
            break;
        }

        case VMOP_CheckClearParams2:
            while (top->type != 7) {
                RemoveRefToValue(top->type, top->u);
                top--;
            }
            break;

        case VMOP_EndOn:
            if (top->type != 8) {
                Com_Error(1, "VM_Execute C candidate expected void codepos for EndOn");
            }
            top->type = 7;
            break;

        case VMOP_VoidCodepos:
            VM_PushValue(&top)->type = 8;
            break;

        case VMOP_Nop:
            break;

        case VMOP_DevblockBegin:
        case VMOP_DevblockEnd:
            pos++;
            break;

        case VMOP_Notify:
            fieldValueId = VM_LocalVarCacheId(0);
            break;

        case VMOP_EvalLocalVariableObject:
            fieldValueId = VM_LocalVarCacheId(*(const unsigned char *)pos++);
            break;

        case VMOP_SetLevelFieldVariableField: {
            unsigned int fieldName = VM_ReadU16(&pos);
            unsigned int varId = GetVariable(varPub->levelId, fieldName);
            SetVariableValue(varId, top);
            top--;
            break;
        }

        case VMOP_SetAnimFieldVariableField: {
            unsigned int fieldName = VM_ReadU16(&pos);
            unsigned int varId = GetVariable(varPub->animId, fieldName);
            SetVariableValue(varId, top);
            top--;
            break;
        }

        case VMOP_SetSelfFieldVariableField: {
            unsigned int fieldName = VM_ReadU16(&pos);
            objectId = Scr_GetSelf(localId);
            fieldValueId = Scr_GetVariableField(objectId, fieldName);
            SetVariableFieldValue(fieldValueId, top);
            top--;
            break;
        }

        case VMOP_EvalLocalArrayRefCached0:
            SetVariableFieldValue(fieldValueId, top);
            top--;
            break;

        case VMOP_SetLocalVariableFieldCached:
            SetVariableValue(VM_LocalVarCacheId(0), top);
            top--;
            break;

        case VMOP_SetLocalVariableField:
            SetVariableValue(VM_LocalVarCacheId(*(const unsigned char *)pos++), top);
            top--;
            break;

        case VMOP_ScriptFunctionCallPointer: {
            VariableValue *out = VM_PushValue(&top);
            const char *targetPos;

            out->type = 8;
            targetPos = VM_CandidateReadScriptCallTarget(&pos);
            VM_CandidateEnterFunctionCall(&pos, &localId, &localVarCount, &top, &startTop,
                                          targetPos, pos);
            break;
        }

        case VMOP_ScriptFunctionCall: {
            const char *targetPos = VM_CandidateReadScriptCallTarget(&pos);
            VM_CandidateEnterFunctionCall(&pos, &localId, &localVarCount, &top, &startTop,
                                          targetPos, pos);
            break;
        }

        case VMOP_ScriptFunctionCallExpr: {
            const char *returnPos = pos;
            const char *targetPos = VM_CandidatePopCodepos(&top);
            VM_CandidateEnterFunctionCall(&pos, &localId, &localVarCount, &top, &startTop,
                                          targetPos, returnPos);
            break;
        }

        case VMOP_ScriptMethodCall: {
            const char *targetPos = VM_CandidateReadScriptCallTarget(&pos);
            VM_CandidateEnterMethodCall(&pos, &localId, &localVarCount, &top, &startTop,
                                        targetPos, pos);
            break;
        }

        case VMOP_ScriptMethodCallExpr: {
            const char *returnPos = pos;
            const char *targetPos = VM_CandidatePopCodepos(&top);
            VM_CandidateEnterMethodCall(&pos, &localId, &localVarCount, &top, &startTop,
                                        targetPos, returnPos);
            break;
        }

        case VMOP_ScriptThreadCall: {
            const char *targetPos = VM_CandidateReadScriptCallTarget(&pos);
            unsigned int paramCount = VM_ReadI32(&pos);
            VM_CandidateEnterFunctionThreadCall(&pos, &localId, &localVarCount, &top, &startTop,
                                                targetPos, pos, paramCount);
            thread_count++;
            break;
        }

        case VMOP_ScriptThreadCallExpr: {
            const char *targetPos = VM_CandidatePopCodepos(&top);
            unsigned int paramCount = VM_ReadI32(&pos);
            VM_CandidateEnterFunctionThreadCall(&pos, &localId, &localVarCount, &top, &startTop,
                                                targetPos, pos, paramCount);
            thread_count++;
            break;
        }

        case VMOP_ScriptMethodThreadCall: {
            const char *targetPos = VM_CandidateReadScriptCallTarget(&pos);
            unsigned int paramCount = VM_ReadI32(&pos);
            VM_CandidateEnterMethodThreadCall(&pos, &localId, &localVarCount, &top, &startTop,
                                              targetPos, pos, paramCount);
            thread_count++;
            break;
        }

        case VMOP_ScriptMethodThreadCallExpr: {
            const char *targetPos = VM_CandidatePopCodepos(&top);
            unsigned int paramCount = VM_ReadI32(&pos);
            VM_CandidateEnterMethodThreadCall(&pos, &localId, &localVarCount, &top, &startTop,
                                              targetPos, pos, paramCount);
            thread_count++;
            break;
        }

        case VMOP_DecTop:
            RemoveRefToValue(top->type, top->u);
            top--;
            break;

        case VMOP_EvalFieldObject:
            objectId = Scr_EvalFieldObject(varPub->tempVariable, top);
            top--;
            break;

        case VMOP_EvalLocalVariableObjectCached:
            objectId = Scr_EvalVariableObject(VM_LocalVarCacheId(*(const unsigned char *)pos++));
            break;

        case VMOP_Wait:
            if (VM_CandidateHandleWait(&pos, &localVarCount, &top, &startTop, &localId, &resultLocalId, &thread_count))
                return resultLocalId;
            break;

        case VMOP_WaitTillFrameEnd:
            if (VM_CandidateHandleWaitTillFrameEnd(&pos, &localVarCount, &top, &startTop, &localId, &resultLocalId, &thread_count))
                return resultLocalId;
            break;

        case VMOP_PreScriptCall:
            VM_PushValue(&top)->type = 8;
            break;

        case VMOP_CallBuiltin0:
        case VMOP_CallBuiltin1:
        case VMOP_CallBuiltin2:
        case VMOP_CallBuiltin3:
        case VMOP_CallBuiltin4:
        case VMOP_CallBuiltin5:
            VM_CandidateCallBuiltin(&pos, &top, opcode - VMOP_CallBuiltin0);
            break;

        case VMOP_CallBuiltin: {
            unsigned int paramCount = *(const unsigned char *)pos++;
            VM_CandidateCallBuiltin(&pos, &top, paramCount);
            break;
        }

        case VMOP_CallBuiltinMethod0:
        case VMOP_CallBuiltinMethod1:
        case VMOP_CallBuiltinMethod2:
        case VMOP_CallBuiltinMethod3:
        case VMOP_CallBuiltinMethod4:
        case VMOP_CallBuiltinMethod5:
            VM_CandidateCallBuiltinMethod(&pos, &top, opcode - VMOP_CallBuiltinMethod0);
            break;

        case VMOP_CallBuiltinMethod: {
            unsigned int paramCount = *(const unsigned char *)pos++;
            VM_CandidateCallBuiltinMethod(&pos, &top, paramCount);
            break;
        }

        case VMOP_CastBool:
            Scr_CastBool(top);
            break;

        case VMOP_BoolNot:
            Scr_EvalBoolNot(top);
            break;

        case VMOP_BoolComplement:
            Scr_EvalBoolComplement(top);
            break;

        case VMOP_JumpOnFalse: {
            unsigned int offset;
            Scr_CastBool(top);
            offset = VM_ReadU16(&pos);
            if (!top->u.intValue) {
                pos += offset;
            }
            top--;
            break;
        }

        case VMOP_JumpOnTrue: {
            unsigned int offset;
            Scr_CastBool(top);
            offset = VM_ReadU16(&pos);
            if (top->u.intValue) {
                pos += offset;
            }
            top--;
            break;
        }

        case VMOP_JumpOnFalseExpr: {
            unsigned int offset;
            Scr_CastBool(top);
            offset = VM_ReadU16(&pos);
            if (!top->u.intValue) {
                pos += offset;
            } else {
                top--;
            }
            break;
        }

        case VMOP_JumpOnTrueExpr: {
            unsigned int offset;
            Scr_CastBool(top);
            offset = VM_ReadU16(&pos);
            if (top->u.intValue) {
                pos += offset;
            } else {
                top--;
            }
            break;
        }

        case VMOP_Jump: {
            int offset = VM_ReadI32(&pos);
            pos += offset;
            break;
        }

        case VMOP_JumpBack: {
            const char *opcodeStart = pos - 1;
            unsigned int offset = VM_ReadU16(&pos);

            pos = opcodeStart + 3 - offset;
            break;
        }

        case VMOP_Inc:
            VM_CandidateIncDecField(fieldValueId, &top, 1);
            pos++;
            break;

        case VMOP_Dec:
            VM_CandidateIncDecField(fieldValueId, &top, -1);
            pos++;
            break;

        case VMOP_Bit_Or:
            VM_EvalBinaryAndPop(&top, Scr_EvalOr);
            break;

        case VMOP_Bit_Xor:
            VM_EvalBinaryAndPop(&top, Scr_EvalExOr);
            break;

        case VMOP_Bit_And:
            VM_EvalBinaryAndPop(&top, Scr_EvalAnd);
            break;

        case VMOP_Equal:
            VM_EvalBinaryAndPop(&top, Scr_EvalEquality);
            break;

        case VMOP_NotEqual:
            VM_EvalBinaryAndPop(&top, Scr_EvalInequality);
            break;

        case VMOP_LessThan:
            VM_EvalBinaryAndPop(&top, Scr_EvalLess);
            break;

        case VMOP_GreaterThan:
            VM_EvalBinaryAndPop(&top, Scr_EvalGreater);
            break;

        case VMOP_LessEqual:
            VM_EvalBinaryAndPop(&top, Scr_EvalLessEqual);
            break;

        case VMOP_GreaterEqual:
            VM_EvalBinaryAndPop(&top, Scr_EvalGreaterEqual);
            break;

        case VMOP_ShiftLeft:
            VM_EvalBinaryAndPop(&top, Scr_EvalShiftLeft);
            break;

        case VMOP_ShiftRight:
            VM_EvalBinaryAndPop(&top, Scr_EvalShiftRight);
            break;

        case VMOP_Plus:
            VM_EvalBinaryAndPop(&top, Scr_EvalPlus);
            break;

        case VMOP_Minus:
            VM_EvalBinaryAndPop(&top, Scr_EvalMinus);
            break;

        case VMOP_Multiply:
            VM_EvalBinaryAndPop(&top, Scr_EvalMultiply);
            break;

        case VMOP_Divide:
            VM_EvalBinaryAndPop(&top, Scr_EvalDivide);
            break;

        case VMOP_Mod:
            VM_EvalBinaryAndPop(&top, Scr_EvalMod);
            break;

        case VMOP_SizeOf:
            Scr_EvalSizeValue(top);
            break;

        case VMOP_WaitTillMatch:
        case VMOP_WaitTill:
            if (VM_CandidateHandleWaitTill(&pos, &localVarCount, &top, &startTop, &localId, &resultLocalId, &thread_count))
                return resultLocalId;
            break;

        case VMOP_NotifyCall:
            VM_CandidateHandleNotifyCall(&top);
            break;

        case VMOP_EndOnCallback:
            VM_CandidateHandleEndOnCallback(localId, &top);
            break;

        case VMOP_Switch:
            VM_CandidateHandleSwitch(&pos, &top);
            break;

        case VMOP_EndSwitch:
            caseCount = VM_ReadU16(&pos);
            pos += caseCount * 8;
            caseCount = 0;
            break;

        case VMOP_Object: {
            int entnum = VM_ReadI32(&pos);
            int classnum = VM_ReadI32(&pos);
            VM_CandidatePushEntityObject(&top, classnum, entnum);
            break;
        }

        case VMOP_ThreadObject:
            VM_PushObject(&top, VM_ReadU16(&pos));
            break;

        case VMOP_EvalLevelFieldVariableRef:
            VM_PushPackedValue(&top, Scr_EvalVariable(FindVariable(localId, VM_ReadU16(&pos))));
            break;

        case VMOP_EvalAnimFieldVariableRef:
            fieldValueId = FindVariable(localId, VM_ReadU16(&pos));
            if (!fieldValueId) {
                Com_Error(1, "VM_Execute C candidate missing field variable ref");
            }
            break;

        case VMOP_Vector:
            top -= 2;
            Scr_CastVector(top);
            break;

        case VMOP_Abort:
            g_script_error_level--;
            return 0;

        default:
            scrVmPub.top = top;
            Com_Error(1, "VM_Execute C candidate pass66 hit unconverted opcode 0x%02x (%s)",
                      opcode, VM_OpcodeName(opcode));
            g_script_error_level--;
            return localId;
        }
    }

    (void)objectId;
    (void)startTop;
}
#endif

static unsigned int VM_RestoreLocalVarsFromSibling(unsigned int localId)
{
    unsigned int localVarCount = 0;
    unsigned int varId;
    unsigned int *localVarLimit = (unsigned int *)(scrVmGlob + 24) + 2047;

    if (localId <= 2)
        return 0;

    varId = FindPrevSibling(localId);

    while (varId && scrVmPub.localVars < localVarLimit) {
        ++scrVmPub.localVars;
        *scrVmPub.localVars = varId;
        localVarCount++;
        varId = FindPrevSibling(varId);
    }

    return localVarCount;
}

static unsigned int VM_RestoreArchivedLocalVars(VariableStackBuffer *stackValue)
{
    unsigned int i;

    for (i = 0; i < VM_LOCAL_ARCHIVE_SLOTS; i++) {
        VM_LocalVarArchive *archive = &vmLocalVarArchives[i];

        if (archive->localId == stackValue->localId) {
            unsigned int count = archive->count;
            unsigned int varIndex;

            for (varIndex = 0; varIndex < count; varIndex++) {
                ++scrVmPub.localVars;
                *scrVmPub.localVars = archive->vars[varIndex];
            }

            archive->localId = 0;
            archive->count = 0;
            return count;
        }
    }

    return VM_RestoreLocalVarsFromSibling(stackValue->localId);
}

static void VM_Resume(unsigned int timeId)
{
    struct scrVarPub_t *svp = (struct scrVarPub_t *)imp_scrVarPub;
    static unsigned int traceCount;
    unsigned int stackId;

    Scr_ResetTimeout();
    AddRefToObject(timeId);

    while ((stackId = FindNextSibling(timeId)) != 0) {
        unsigned int startLocalId;
        VariableStackBuffer *stackValue;
        unsigned int size;
        byte *record;
        VariableValue *top;
        const char *pos;
        unsigned int localId;
        unsigned int localVarCount;
        unsigned int i;
        struct function_stack_t fs;
        unsigned int endLocalId;

        startLocalId = GetVariableKeyObject(stackId);
        stackValue = SCR_STACK_PTR(*GetVariableValueAddress(stackId));
        if (traceCount < 64) {
            if (getenv("DBGSPAM"))
                Com_Printf("[team-trace] resume bucket=%u local=%u time=%u archivedTime=%u pos=%p size=%u\n",
                           timeId, startLocalId, (unsigned int)svp->time,
                           (unsigned int)stackValue->time, (const void *)stackValue->pos,
                           (unsigned int)stackValue->size);
            ++traceCount;
        }
        RemoveObjectVariable(timeId, startLocalId);

        scrVmPub.function_frame->fs.pos = stackValue->pos;
        scrVmPub.function_count++;
        scrVmPub.function_frame++;

        size = stackValue->size;
        record = (byte *)stackValue + VM_STACKBUF_HEADER_SIZE;

        if (!size) {
            top = scrVmPub.stack;
        } else {
            top = scrVmPub.stack;

            for (i = 0; i < size; i++) {
                unsigned int type = record[0];

                top++;
                top->type = type;

                if (type == 7) {
#if defined(__x86_64__) || defined(_M_X64)
                    scrVmPub.function_frame->fs.pos =
                        SCR_CODEPOS_PTR(*(unsigned int *)(record + 1));
#else
                    scrVmPub.function_frame->fs.pos = *(const char **)(record + 1);
#endif
                    scrVmPub.function_count++;
                    scrVmPub.function_frame++;
                } else {
                    top->u.intValue = *(int *)(record + 1);
                }

                record += VM_STACKBUF_VALUE_SIZE;
            }
        }

        pos = stackValue->pos;
        localId = stackValue->localId;
        Scr_ClearWaitTime(startLocalId);

        if (scrVmPub.function_count > 0) {
            unsigned int frameIndex;
            unsigned int frameLocalId = localId;

            for (frameIndex = (unsigned int)scrVmPub.function_count; frameIndex > 0; frameIndex--) {
                /* Frame zero is the VM sentinel. Live frames are 1..count,
                 * matching VM_Execute and VM_CandidateEnterScriptFrame. */
                function_frame_t *frame = &scrVmPub.function_frame_start[frameIndex];
                frame->fs.localId = frameLocalId;
                frame->fs.top = NULL;
                frame->fs.startTop = NULL;
                frame->topType = 0;
                frameLocalId = GetParentLocalId(frameLocalId);
            }

            for (i = 1; i < (unsigned int)scrVmPub.function_count; i++) {
                function_frame_t *frame = &scrVmPub.function_frame_start[i];
                frame->fs.localVarCount = VM_RestoreLocalVarsFromSibling(frame->fs.localId);
            }

            localVarCount = VM_RestoreArchivedLocalVars(stackValue);
            scrVmPub.function_frame->fs.localVarCount = localVarCount;
        } else {
            localVarCount = VM_RestoreArchivedLocalVars(stackValue);
        }

        if ((byte)svp->time != stackValue->time) {
            Scr_ResetTimeout();
        }

        MT_Free(stackValue, stackValue->bufLen);

        fs.pos = pos;
        fs.localId = localId;
        fs.localVarCount = localVarCount;
        fs.top = top;
        fs.startTop = scrVmPub.stack;

        endLocalId = VM_Execute(fs);
        RemoveRefToObject(endLocalId);
        RemoveRefToValue(scrVmPub.stack[1].type, scrVmPub.stack[1].u);
    }

    RemoveRefToObject(timeId);
    ClearVariableValue(svp->tempVariable);
    scrVmPub.top = scrVmPub.stack;
}

extern unsigned int FindVariable(unsigned int parentId, unsigned int name);
extern unsigned int FindObject(unsigned int id);
extern void SafeRemoveVariable(unsigned int parentId, unsigned int name);
extern void Scr_FreeEntityList(void);

static inline __attribute__((always_inline)) void Scr_RunPendingThreads(void)
{
    struct scrVarPub_t *svp = (struct scrVarPub_t *)imp_scrVarPub;
    static unsigned int traceCount;
    unsigned int notifyId = svp->timeArrayId;
    unsigned int varId;

    if (!notifyId)
        return;

    varId = FindVariable(notifyId, (unsigned int)svp->time);
    if (!varId)
        return;

    if (traceCount < 64) {
        if (getenv("DBGSPAM"))
            Com_Printf("[team-trace] scheduler time=%u timeArray=%u var=%u bucket=%u\n",
                       (unsigned int)svp->time, notifyId, varId, FindObject(varId));
        ++traceCount;
    }
    VM_Resume(FindObject(varId));
    SafeRemoveVariable(svp->timeArrayId, (unsigned int)svp->time);
}

void Scr_RunCurrentThreads(void)
{
#if COD2_FEATURE_SCRIPT_DEBUGGER
    Scr_UpdateDebugger();
#endif
    Scr_RunPendingThreads();
}

void Scr_IncTime(void)
{
    struct scrVarPub_t *svp;

    Scr_RunPendingThreads();
    Scr_FreeEntityList();

    svp = (struct scrVarPub_t *)imp_scrVarPub;
    svp->time = (svp->time + 1) & 0x00FFFFFF;
}

#if defined(_MSC_VER)
/* VM_ExecuteExtCall is the C++-mangled VM_Execute; cl can't rename, so bind the
 * call site to the mangled symbol via /alternatename (regparm no-ops on MSVC). */
extern unsigned int VM_ExecuteExtCall(unsigned int threadId, const char *pos, unsigned int paramcount);
COD2_ALT("VM_ExecuteExtCall", "_Z10VM_ExecutejPKcj")
#else
extern unsigned int __attribute_regparm__(3)
    VM_ExecuteExtCall(unsigned int threadId, const char *pos, unsigned int paramcount) __asm__("_Z10VM_ExecutejPKcj");
#endif

__attribute__((noinline)) unsigned int __attribute_regparm__(3)
    VM_ExecuteExtCall(unsigned int threadId, const char *pos, unsigned int paramcount)
{
    struct scrVarPub_t *varPub = (struct scrVarPub_t *)imp_scrVarPub;
    struct function_stack_t fs;
    VariableValue *top;
    VariableValue *startTop;
    unsigned int oldInParamCount;
    unsigned int result;

    while (scrVmPub.outparamcount) {
        RemoveRefToValue(scrVmPub.top->type, scrVmPub.top->u);
        --scrVmPub.top;
        --scrVmPub.outparamcount;
    }

    top = scrVmPub.top;
    startTop = top - paramcount;
    oldInParamCount = scrVmPub.inparamcount - paramcount;

    if (scrVmPub.function_count > 29) {
        Scr_KillThread(threadId);
        scrVmPub.inparamcount = oldInParamCount + 1;
        RuntimeError(pos, 0, "script stack overflow (too many embedded function calls)", NULL);
        return threadId;
    }

    if (scrVmPub.function_count) {
        ++scrVmPub.function_count;
        ++scrVmPub.function_frame;
        scrVmPub.function_frame->fs.localId = 0;
    }

    scrVmPub.function_frame->fs.pos = pos;
    ++scrVmPub.function_count;
    ++scrVmPub.function_frame;
    scrVmPub.function_frame->fs.localId = threadId;

    startTop->type = 8;
    scrVmPub.inparamcount = 0;

    fs.pos = pos;
    fs.localId = threadId;
    fs.localVarCount = 0;
    fs.top = top;
    fs.startTop = startTop;

    result = VM_Execute(fs);

    scrVmPub.top = startTop + 1;
    scrVmPub.inparamcount = oldInParamCount + 1;
    ClearVariableValue(varPub->tempVariable);

    if (scrVmPub.function_count) {
        --scrVmPub.function_count;
        --scrVmPub.function_frame;
    }

    return result;
}

void Scr_AddExecThread(scr_func_t handle, unsigned int paramcount)
{
    extern unsigned int AllocThread(unsigned int self);

    struct scrVarPub_t *pub = (struct scrVarPub_t *)imp_scrVarPub;
    const char *pos = pub->programBuffer + handle;
    unsigned int threadId;
    unsigned int result;

    if (!scrVmPub.function_count)
        Scr_ResetTimeout_core();

    AddRefToObject(pub->levelId);
    threadId = AllocThread(pub->levelId);
    result = VM_ExecuteExtCall(threadId, pos, paramcount);
    RemoveRefToObject(result);
    scrVmPub.outparamcount++;
    scrVmPub.inparamcount--;
}

scr_thread_t Scr_ExecEntThreadNum(int entnum, int classnum, scr_func_t handle, unsigned int paramcount)
{
    extern unsigned int AllocThread(unsigned int self);
    extern unsigned int Scr_GetEntityId(int entnum, int classnum);
    extern unsigned int FindEntityId(int entnum, int classnum);

    struct scrVarPub_t *pub = (struct scrVarPub_t *)imp_scrVarPub;
    const char *pos = pub->programBuffer + handle;
    unsigned int objId;
    unsigned int threadId;
    unsigned int result;
    VariableValue *value;

    if (!scrVmPub.function_count)
        Scr_ResetTimeout_core();

    objId = Scr_GetEntityId(entnum, classnum);
    AddRefToObject(objId);
    threadId = AllocThread(objId);
    result = VM_ExecuteExtCall(threadId, pos, paramcount);

    value = scrVmPub.top;
    RemoveRefToValue(value->type, value->u);
    scrVmPub.top->type = 0;
    scrVmPub.top = scrVmPub.top - 1;
    scrVmPub.inparamcount--;

    return (unsigned short)result;
}

scr_thread_t Scr_ExecThread(scr_func_t handle, unsigned int numArgs)
{
    extern unsigned int AllocThread(unsigned int self);

    struct scrVarPub_t *pub = (struct scrVarPub_t *)imp_scrVarPub;
    const char *pos = pub->programBuffer + handle;
    unsigned int threadId;
    unsigned int result;
    VariableValue *value;

    if (!scrVmPub.function_count)
        Scr_ResetTimeout_core();

    Scr_IsInOpcodeMemory(pos);
    AddRefToObject(pub->levelId);
    threadId = AllocThread(pub->levelId);

    result = VM_ExecuteExtCall(threadId, pos, numArgs);

    value = scrVmPub.top;
    RemoveRefToValue(value->type, value->u);
    scrVmPub.top->type = 0;
    scrVmPub.top = scrVmPub.top - 1;
    scrVmPub.inparamcount--;

    return (unsigned short)result;
}

const char *Scr_GetIString(unsigned int index)
{
    VariableValue *value = Scr_GetParamValue(index);

    if (index >= scrVmPub.outparamcount) {
        return SL_ConvertToString(0);
    }
    if (value->type != VAR_LOCALIZED_STRING) {
        Scr_ParamTypeError(index, "type %s is not a localized string", value->type);
        return SL_ConvertToString(0);
    }

    return SL_ConvertToString(value->u.stringValue);
}

scr_anim_t Scr_GetAnim(unsigned int index, struct XAnimTree_s *tree)
{
    VariableValue *value;
    scr_anim_t anim;

    anim.index = 0;
    anim.tree = 0;

    if (index >= scrVmPub.outparamcount) {
        Scr_SetErrorMessageAndJump(va("parameter %d does not exist", index + 1));
        return anim;
    }

    value = scrVmPub.top - index;
    if (value->type != 11) {
        ((struct scrVarPub_t *)imp_scrVarPub)->error_message =
            va("type %s is not an anim", ((const char *const *)imp_var_typename)[value->type]);
        RemoveRefToValue(value->type, value->u);
        value->type = 0;
        Scr_ParamError(index, ((struct scrVarPub_t *)imp_scrVarPub)->error_message);
        return anim;
    }

    anim.index = (short unsigned int)(value->u.intValue & 0xffff);
    anim.tree = (short unsigned int)((unsigned int)value->u.intValue >> 16);

    if (tree) {
        XAnim *scriptAnims = Scr_GetAnims(anim.tree);
        XAnim *entityAnims = XAnimGetAnims(tree);

        if (scriptAnims != entityAnims) {
            const char *entityTreeName = XAnimGetAnimTreeDebugName(entityAnims);
            const char *scriptTreeName = XAnimGetAnimTreeDebugName(scriptAnims);
            const char *animName = XAnimGetAnimDebugName(scriptAnims, anim.index);

            ((struct scrVarPub_t *)imp_scrVarPub)->error_message =
                va("anim '%s' in animtree '%s' does not belong to the entity's animtree '%s'",
                   animName, scriptTreeName, entityTreeName);
            RemoveRefToValue(value->type, value->u);
            value->type = 0;
            Scr_ParamError(index, ((struct scrVarPub_t *)imp_scrVarPub)->error_message);
            anim.index = 0;
            anim.tree = 0;
        }
    }

    return anim;
}

unsigned int Scr_GetConstString(unsigned int index)
{
    VariableValue *value = Scr_GetConstStringValue(index);
    return value ? value->u.stringValue : 0;
}

unsigned int Scr_GetConstStringIncludeNull(unsigned int index)
{
    if (index < scrVmPub.outparamcount && (scrVmPub.top - index)->type == VAR_UNDEFINED) {
        return 0;
    }

    return Scr_GetConstString(index);
}

const char *SL_ConvertToString(unsigned int stringValue);

const char *Scr_GetString(unsigned int index)
{
    unsigned int strIdx = Scr_GetConstString(index);
    return SL_ConvertToString(strIdx);
}

unsigned int Scr_GetConstLowercaseString(unsigned int index)
{
    VariableValue *value = Scr_GetConstStringValue(index);
    unsigned int oldStringValue;
    char newStr[8192];
    const char *str;
    int i;

    if (!value) {
        return 0;
    }

    oldStringValue = value->u.stringValue;
    str = SL_ConvertToString(oldStringValue);

    for (i = 0;; ++i) {
        newStr[i] = (char)___tolower((signed char)str[i]);
        if (!str[i]) {
            break;
        }
    }

    value->u.stringValue = SL_GetString(newStr, 0);
    SL_RemoveRefToString(oldStringValue);
    return value->u.stringValue;
}

void Scr_Error(const char *error)
{
    Scr_SetErrorMessageAndJump(error);
}

static inline __attribute__((always_inline)) void Scr_AddObject_core(unsigned int id)
{
    IncInParam();
    scrVmPub.top->type = VAR_POINTER;
    scrVmPub.top->u.pointerValue = id;
    AddRefToObject(id);
}

void Scr_AddStruct(void)
{
    unsigned int id = AllocObject();
    Scr_AddObject_core(id);
    RemoveRefToObject(id);
}

void Scr_AddEntityNum(int entnum, int classnum)
{
    unsigned int id = Scr_GetEntityId(entnum, classnum);
    Scr_AddObject_core(id);
}

void Scr_AddBool(int value)
{
    IncInParam();
    scrVmPub.top->type = VAR_INTEGER;
    scrVmPub.top->u.intValue = value;
}

void Scr_AddInt(int value)
{
    IncInParam();
    scrVmPub.top->type = VAR_INTEGER;
    scrVmPub.top->u.intValue = value;
}

void Scr_AddFloat(float value)
{
    IncInParam();
    scrVmPub.top->type = VAR_FLOAT;
    scrVmPub.top->u.floatValue = value;
}

void Scr_AddUndefined(void)
{
    IncInParam();
    scrVmPub.top->type = VAR_UNDEFINED;
}

void Scr_AddObject(unsigned int id)
{
    Scr_AddObject_core(id);
}

void Scr_AddString(const char *value)
{
    IncInParam();
    scrVmPub.top->type = VAR_STRING;
    scrVmPub.top->u.stringValue = SL_GetString(value, 0);
}

void Scr_AddConstString(unsigned int value)
{
    IncInParam();
    scrVmPub.top->type = VAR_STRING;
    scrVmPub.top->u.stringValue = value;
    SL_AddRefToString(value);
}

void Scr_AddVector(const float *value)
{
    IncInParam();
    scrVmPub.top->type = VAR_VECTOR;
    scrVmPub.top->u.vectorValue = SCR_VEC_ENC(Scr_AllocVector(value));
}
