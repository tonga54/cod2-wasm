
#include "common_types.h"
#include "imports.h"
extern scrVarPub_t scrVarPub;

extern struct scrCompilePub_t scrCompilePub;
extern void Z_FreeInternal(void *ptr);
extern void *Z_MallocInternal(int size);
extern unsigned int FindVariable(unsigned int parentId, unsigned int value);
extern unsigned long long Scr_EvalVariable(unsigned int id);
extern VariableUnion *GetVariableValueAddress(unsigned int id);
extern void AddRefToValue(int type, VariableUnion u);
extern void Scr_EvalBinaryOperator(int op, VariableValue *value1, VariableValue *value2);
extern int GetVarType(unsigned int id);
extern unsigned int FindNextSibling(unsigned int id);
extern unsigned int FindObject(unsigned int id);
extern unsigned int GetVariable(unsigned int parentId, unsigned int value);
extern unsigned int GetNewVariable(unsigned int parentId, unsigned int value);
extern unsigned int GetVariableName(unsigned int id);
extern unsigned int GetObjectA(unsigned int id);
extern unsigned int GetArray(unsigned int id);
extern void SetVariableValue(unsigned int id, VariableValue *value);
extern void Com_Error(int code, const char *fmt, ...);
extern void CompileError(unsigned int sourcePos, const char *fmt, ...);
extern void CompileError2(const char *codePos, const char *msg, ...);
extern void AddOpcodePos(unsigned int sourcePos, int type);
extern void RemoveOpcodePos(void);
extern void AddThreadStartOpcodePos(unsigned int sourcePos);
extern unsigned int Scr_LoadScript(const char *filename);
extern unsigned int Scr_CreateCanonicalFilename(const char *filename);
extern void Scr_UsingTree(const char *filename, unsigned int sourcePos);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern void SL_RemoveRefToString(unsigned int stringValue);
extern char *TempMalloc(int len);
extern char *TempMallocAlign(int len);
extern char *TempMallocAlignStrict(int len);
extern void *Hunk_AllocateTempMemoryHighInternal(int size);
extern void TempMemorySetPos(char *pos);
extern void Hunk_ClearTempMemoryHigh(void);
extern void SL_AddRefToString(unsigned int stringValue);
extern void SL_TransferRefToUser(unsigned int stringValue, unsigned int user);
extern unsigned int SL_TransferToCanonicalString(unsigned int stringValue);
extern Bool IsValidArrayIndex(unsigned int unsignedValue);
extern unsigned int GetInternalVariableIndex(unsigned int unsignedValue);
extern char *va(const char *format, ...);
extern void qsort(void *base, unsigned int nmemb, unsigned int size, int (*compar)(const void *, const void *));
extern void RemoveRefToVector(const float *vectorValue);
extern void Scr_CastBool(VariableValue *value);
extern const float *Scr_AllocVector(const float *v);
extern int Scr_GetSourceBuffer(const char *codePos);
extern int Scr_GetClassnumForCharId(int charId);
extern Bool IsVarFree(unsigned int id);
extern int atoi(const char *nptr);
extern void SetNewVariableValue(unsigned int id, VariableValue *value);
extern unsigned char scrCompileGlob[];
extern unsigned char scrVarGlob[];
extern void DumpCompiledObject(const char *label, unsigned int compiledObj);
extern intptr_t Scr_GetFunction(const char **pName, int *type);   /* returns a function pointer; int truncated it on x64 */
extern intptr_t Scr_GetMethod(const char **pName, int *type);
extern void Scr_EmitAnimation(char *pos, unsigned int animName, unsigned int sourcePos);
static const char str_dbg_before_lt[] = "before-LinkThread";

#define SCRCP (&scrCompilePub)
#define SCRCG ((scrCompileGlob_t *)imp_scrCompileGlob)
#define SCRAP ((scrAnimPub_t *)imp_scrAnimPub)
#define SCRPP ((scrParserPub_t *)imp_scrParserPub)
#define SCRVP (&scrVarPub)

#define SCRCOMP_SCR_DEV_NO 0
#define SCRCOMP_SCR_DEV_ON 1
#define SCRCOMP_SCR_DEV_IGNORE 2

#define SCRCOMP_NODE_FUNCTION 0x44
#define SCRCOMP_NODE_DEV_BEGIN 0x45
#define SCRCOMP_NODE_DEV_END 0x46
#define SCRCOMP_NODE_USINGTREE 0x47
#define SCRCOMP_NODE_INCLUDE 0x55

#define SCRCOMP_VAR_POINTER 1
#define SCRCOMP_VAR_INTEGER 6
#define SCRCOMP_VAR_CODEPOS 7
#define SCRCOMP_VAR_DEVELOPER_CODEPOS 12
#define SCRCOMP_VAR_INCLUDE_CODEPOS 13
#define SCRCOMP_MAX_VARIABLES 65534

static unsigned int LinkThread(unsigned int threadId, VariableValue *pos, int allowFarCall) __attribute_regparm__(3);
static unsigned int SpecifyThreadPosition(unsigned int threadId, unsigned int name, unsigned int sourcePos, int type) __attribute_regparm__(3);
static void Scr_EvalVariableValue(unsigned int id, VariableValue *value);
int CompareCaseInfo(const unsigned int *elem1, const unsigned int *elem2);
void Scr_CompileShutdown(void);
static void Scr_BlockAddLocalVar(scr_block_t *block, unsigned int name);
static unsigned int Scr_CalcLocalVarsVariableExpressionRef(sval_t expr, scr_block_t *block) __attribute_regparm__(2);
static Bool EvalPrimitiveExpression(sval_t expr, VariableCompileValue *constValue) __attribute_regparm__(2);
static Bool EvalBinaryOperatorExpression(sval_t expr1, sval_t expr2, sval_t opcode, sval_t sourcePos, VariableCompileValue *constValue) __attribute_regparm__(3);
static unsigned int Scr_CalcLocalVarsStatement(uintptr_t val, scr_block_t *block) __attribute_regparm__(2);
static unsigned int EmitValue(VariableCompileValue *constValue) __attribute_regparm__(1);
static unsigned int EmitOpcode(unsigned int op, int offset, int callType) __attribute_regparm__(3);
static unsigned int EmitGetInteger(int value, unsigned int sourcePos) __attribute_regparm__(2);
static int Scr_FindLocalVarIndex(unsigned int name, sval_t sourcePos, Bool create, scr_block_t *block) __attribute_regparm__(3);
static unsigned int EmitContinueStatement(unsigned int sourcePos, scr_block_t *block) __attribute_regparm__(2);
static unsigned int EmitBreakStatement(unsigned int sourcePos, scr_block_t *block) __attribute_regparm__(2);
static unsigned int EmitFunction(uintptr_t func, unsigned int sourcePos) __attribute_regparm__(2);
static unsigned int EmitCall(sval_t func_name, sval_t params, Bool bStatement, scr_block_t *block) __attribute_regparm__(3);
static Bool EmitOrEvalPrimitiveExpression(sval_t expr, VariableCompileValue *constValue, scr_block_t *block) __attribute_regparm__(3);
static Bool EmitOrEvalExpression(sval_t expr, VariableCompileValue *constValue, scr_block_t *block) __attribute_regparm__(3);
static unsigned int EmitPostFunctionCall(sval_t func_name, int param_count, Bool bMethod, scr_block_t *block) __attribute_regparm__(3);
static unsigned int EmitMethod(sval_t expr, sval_t func_name, sval_t params, sval_t methodSourcePos, int bStatement, scr_block_t *block) __attribute_regparm__(3);
static unsigned int EmitPrimitiveExpressionFieldObject(sval_t expr, unsigned int sourcePos, scr_block_t *block) __attribute_regparm__(3);
static unsigned int EmitVariableExpression(sval_t expr, scr_block_t *block) __attribute_regparm__(2);
static unsigned int EmitVariableExpressionRef(sval_t expr, scr_block_t *block) __attribute_regparm__(2);
static unsigned int EmitStatement(sval_t val, int lastStatement, unsigned int endSourcePos, scr_block_t *block) __attribute_regparm__(3);
static unsigned int EmitIfStatement(sval_t expr, sval_t stmt, sval_t sourcePos, int lastStatement, unsigned int endSourcePos, scr_block_t *block, sval_t *ifStatBlock) __attribute_regparm__(3);
static unsigned int EmitIfElseStatement(sval_t expr, sval_t stmt1, sval_t stmt2, sval_t sourcePos, sval_t elseSourcePos, int lastStatement, unsigned int endSourcePos, scr_block_t *block, sval_t *ifStatBlock, sval_t *elseStatBlock) __attribute_regparm__(3);
static unsigned int EmitWhileStatement(sval_t expr, sval_t stmt, sval_t sourcePos, sval_t whileSourcePos, scr_block_t *block, sval_t *whileStatBlock) __attribute_regparm__(3);
static unsigned int EmitForStatement(sval_t stmt1, sval_t expr, sval_t stmt2, sval_t stmt, sval_t sourcePos, sval_t forSourcePos, scr_block_t *block, sval_t *forStatBlock, sval_t *forStatPostBlock) __attribute_regparm__(3);
static unsigned int EmitSwitchStatement(sval_t expr, sval_t stmtlist, sval_t sourcePos, int lastStatement, unsigned int endSourcePos, scr_block_t *block) __attribute_regparm__(3);
static unsigned int EmitThreadInternal(unsigned int threadId, uintptr_t val, unsigned int sourcePos, unsigned int endSourcePos, scr_block_t *block) __attribute_regparm__(3);
void ScriptCompile(sval_t val, unsigned int fileId, unsigned int scriptId);
static unsigned int EmitDeveloperStatementList(sval_t val, unsigned int sourcePos, scr_block_t *block, sval_t *devStatBlock) __attribute_regparm__(3);
static void Scr_MergeChildBlocks(scr_block_t *childBlock, const scr_block_t *parentBlock);
static void Scr_EmitLocalVarsAtEnd(scr_block_t *block);
static void Scr_EmitLocalVarsRemove(scr_block_t *block, int count);
static void Scr_MergeBranchChildBlocks(scr_block_t *block, scr_block_t **childBlocks, int childCount);
static intptr_t *ScriptStatementListFirstExecNode(sval_t val);

static unsigned int __attribute_regparm__(3)
    LinkThread(unsigned int threadId, VariableValue *pos, int allowFarCall)
{
    unsigned int varId;
    int count;
    int i;

    varId = FindVariable(threadId, 0);
    if (!varId)
        return 0;

    count = (int)(unsigned int)Scr_EvalVariable(varId);
    if (count <= 0)
        return 0;

    for (i = 0; i < count; i++) {
        unsigned int valueId = FindVariable(threadId, i + 2);
        int *value = (int *)GetVariableValueAddress(valueId);
        int type = GetVarType(valueId);

#    if defined(__x86_64__) || defined(_M_X64)
        char *codePtr = (char *)SCR_CODEPOS_PTR((unsigned int)*value);
#    else
        char *codePtr = (char *)(intptr_t)*value;
#    endif

        if (pos->type == SCRCOMP_VAR_DEVELOPER_CODEPOS) {
            if (type == SCRCOMP_VAR_CODEPOS) {
                CompileError2(codePtr, "normal script cannot reference a function in a /# ... #/ comment");
                continue;
            }
        } else {
            if (!pos->type) {
#    ifdef LT_TRACE
                fprintf(stderr, "[UNKFN-LINKTHREAD-169 nodef] threadId=%u count=%d i=%d valueId=%u progoff=%d\n",
                        threadId, count, i, valueId, (int)(codePtr - SCRVP->programBuffer));
#    endif
                CompileError2(codePtr, "unknown function");
                continue;
            }

            if (!allowFarCall && *(int *)codePtr == 1) {
#    ifdef LT_TRACE
                fprintf(stderr, "[UNKFN-LINKTHREAD-174 farcall] threadId=%u progoff=%d\n",
                        threadId, (int)(codePtr - SCRVP->programBuffer));
#    endif
                CompileError2(codePtr, "unknown function");
                continue;
            }
        }

        *(int *)codePtr = pos->u.intValue;
    }

    return 0;
}

static unsigned int __attribute_regparm__(3)
    SpecifyThreadPosition(unsigned int threadId, unsigned int name, unsigned int sourcePos, int type)
{
    unsigned int posId;
    VariableValue pos;

    posId = GetVariable(threadId, 1);
    Scr_EvalVariableValue(posId, &pos);

    if (pos.type) {
        if (pos.u.intValue) {
            int sourceBuffer = Scr_GetSourceBuffer(SCR_CODEPOS_GET(pos.u));
            const char *filename = SCRPP->sourceBufferLookup[sourceBuffer].buf;

            CompileError(sourcePos, "function '%s' already defined in '%s'",
                         SL_ConvertToString(name), filename);
        } else {
            CompileError(sourcePos, "function '%s' already defined", SL_ConvertToString(name));
        }

        return 0;
    }

    pos.u.intValue = 0;
    pos.type = type;
    SetNewVariableValue(posId, &pos);
    return posId;
}

int CompareCaseInfo(const unsigned int *elem1, const unsigned int *elem2)
{
    if (*elem1 > *elem2)
        return -1;
    return *elem1 < *elem2;
}

void Scr_CompileShutdown(void)
{
    void *node;
    /* x86 offset 88 = precachescriptListHead; on x64 it's at offsetof (struct grew) */
    void **pHead = (void **)((char *)&scrCompileGlob + __builtin_offsetof(struct scrCompileGlob_t, precachescriptListHead));
    while ((node = *pHead) != 0) {
        *pHead = *(void **)((char *)node + 8);
        Z_FreeInternal(node);
    }
}

static unsigned int __attribute_regparm__(2)
    Scr_CalcLocalVarsVariableExpressionRef(sval_t expr, scr_block_t *block)
{
    intptr_t *node = (intptr_t *)(uintptr_t)expr.node;

    if (!node) {
        return 0;
    }

    if (node[0] == 4) {
        Scr_BlockAddLocalVar(block, (unsigned int)node[1]);
        return 0;
    }

    if (node[0] == 13) {
        sval_t child;
        intptr_t *childNode;

        child.node = node[1];
        childNode = (intptr_t *)(uintptr_t)child.node;
        if (childNode && childNode[0] == 17) {
            sval_t inner;
            inner.node = childNode[1];
            return Scr_CalcLocalVarsVariableExpressionRef(inner, block);
        }
    }

    return 0;
}

static Bool __attribute_regparm__(2)
    EvalPrimitiveExpression(sval_t expr, VariableCompileValue *constValue)
{
    sval_t *node;
    sval_t *listNode;
    sval_t *walk;
    VariableCompileValue constValue2[3];
    float vec[3];
    int count;
    int i;

    node = (sval_t *)(uintptr_t)expr.node;
    if (!node) {
        return 0;
    }

    switch (node[0].type) {
    case 7:
        constValue->value.type = 6;
        constValue->value.u.intValue = node[1].intValue;
        constValue->sourcePos = node[2];
        return 1;

    case 8:
        constValue->value.type = 5;
        constValue->value.u.floatValue = node[1].floatValue;
        constValue->sourcePos = node[2];
        return 1;

    case 9:
        constValue->value.type = 6;
        constValue->value.u.intValue = -node[1].intValue;
        constValue->sourcePos = node[2];
        return 1;

    case 10:
        constValue->value.type = 5;
        constValue->value.u.floatValue = -node[1].floatValue;
        constValue->sourcePos = node[2];
        return 1;

    case 11:
        constValue->value.type = 2;
        constValue->value.u.stringValue = node[1].stringValue;
        constValue->sourcePos = node[2];
        return 1;

    case 12:
        constValue->value.type = 3;
        constValue->value.u.stringValue = node[1].stringValue;
        constValue->sourcePos = node[2];
        return 1;

    case 31:
        constValue->value.type = 0;
        constValue->sourcePos = node[1];
        return 1;

    case 46:
        listNode = (sval_t *)(uintptr_t)((sval_t *)(uintptr_t)node[1].node)[0].node;
        count = 0;
        for (walk = listNode; walk; walk = (sval_t *)(uintptr_t)walk[1].node) {
            ++count;
        }

        if (count == 1) {
            sval_t *child;
            sval_t *exprNode;

            child = (sval_t *)(uintptr_t)listNode[0].node;
            if (!child) {
                return 0;
            }
            exprNode = (sval_t *)(uintptr_t)child[0].node;

            if (exprNode[0].type == 6) {
                return EvalPrimitiveExpression(exprNode[1], constValue);
            }

            if (exprNode[0].type == 49) {
                return EvalBinaryOperatorExpression(exprNode[1], exprNode[2], exprNode[3], exprNode[4], constValue);
            }

            return 0;
        }

        if (count != 3) {
            return 0;
        }

        i = 0;
        for (walk = listNode; walk; walk = (sval_t *)(uintptr_t)walk[1].node) {
            sval_t *child;
            sval_t *exprNode;

            child = (sval_t *)(uintptr_t)walk[0].node;
            if (!child) {
                return 0;
            }
            exprNode = (sval_t *)(uintptr_t)child[0].node;

            if (exprNode[0].type == 6) {
                if (!EvalPrimitiveExpression(exprNode[1], &constValue2[i])) {
                    return 0;
                }
            } else if (exprNode[0].type == 49) {
                if (!EvalBinaryOperatorExpression(exprNode[1], exprNode[2], exprNode[3], exprNode[4], &constValue2[i])) {
                    return 0;
                }
            } else {
                return 0;
            }
            ++i;
        }

        for (i = 0; i < 3; ++i) {
            if (constValue2[i].value.type == 5) {
                vec[i] = constValue2[i].value.u.floatValue;
            } else if (constValue2[i].value.type == 6) {
                vec[i] = (float)constValue2[i].value.u.intValue;
            } else {
                CompileError(constValue2[i].sourcePos.sourcePosValue,
                             "type %s is not a float",
                             ((const char *const *)imp_var_typename)[constValue2[i].value.type]);
            }
        }

        constValue->value.type = 4;
        constValue->value.u.vectorValue = SCR_VEC_ENC(Scr_AllocVector(vec));
        constValue->sourcePos = node[2];
        return 1;

    case 72: /* parser node 0x48: true */
        constValue->value.type = 6;
        constValue->value.u.intValue = 1;
        constValue->sourcePos = node[1];
        return 1;

    case 73: /* parser node 0x49: false */
        constValue->value.type = 6;
        constValue->value.u.intValue = 0;
        constValue->sourcePos = node[1];
        return 1;

    default:
        return 0;
    }
}

#    if 0

static __attribute__((naked))
Bool EvalBinaryOperatorExpression(sval_t opcode, sval_t sourcePos, VariableCompileValue *constValue)
{
    __asm__ __volatile__ (
        ".Lf92eea_00092eea:\n"
        "pushl %ebp\n"
        "movl %esp, %ebp\n"
        "pushl %edi\n"
        "pushl %esi\n"
        "pushl %ebx\n"
        "subl $0x4c, %esp\n"
        "movl %edx, %edi\n"
        "movl %ecx, -0x3c(%ebp)\n"

        "movl (%eax), %edx\n"
        "cmpl $6, %edx\n"
        "je .Lf92eea_00092fcb\n"
        "cmpl $0x31, %edx\n"
        "je .Lf92eea_00092f12\n"
        ".Lf92eea_00092f08:\n"
        "xorl %eax, %eax\n"

        ".Lf92eea_00092f0a:\n"
        "addl $0x4c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"

        ".Lf92eea_00092f12:\n"
        "movl 0x10(%eax), %ebx\n"
        "movl 0xc(%eax), %ecx\n"
        "movl 8(%eax), %edx\n"
        "movl 4(%eax), %esi\n"
        "leal -0x24(%ebp), %eax\n"
        "movl %eax, 4(%esp)\n"
        "movl %ebx, (%esp)\n"
        "movl %esi, %eax\n"
        "calll EvalBinaryOperatorExpression\n"
        ".Lf92eea_00092f2f:\n"
        "testb %al, %al\n"
        "je .Lf92eea_00092f08\n"
        "movl (%edi), %eax\n"
        "cmpl $6, %eax\n"
        "je .Lf92eea_00092fdd\n"
        "cmpl $0x31, %eax\n"
        "jne .Lf92eea_00092f08\n"
        "movl 0x10(%edi), %ebx\n"
        "movl 0xc(%edi), %ecx\n"
        "movl 8(%edi), %edx\n"
        "movl 4(%edi), %esi\n"
        "leal -0x30(%ebp), %eax\n"
        "movl %eax, 4(%esp)\n"
        "movl %ebx, (%esp)\n"
        "movl %esi, %eax\n"
        "calll EvalBinaryOperatorExpression\n"
        ".Lf92eea_00092f60:\n"
        "testb %al, %al\n"
        "je .Lf92eea_00092f08\n"
        "movl -0x24(%ebp), %eax\n"
        "movl %eax, 4(%esp)\n"
        "movl -0x20(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll AddRefToValue\n"
        "movl -0x30(%ebp), %eax\n"
        "movl %eax, 4(%esp)\n"
        "movl -0x2c(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll AddRefToValue\n"
        "leal -0x30(%ebp), %eax\n"
        "movl %eax, 8(%esp)\n"
        "leal -0x24(%ebp), %eax\n"
        "movl %eax, 4(%esp)\n"
        "movl -0x3c(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll Scr_EvalBinaryOperator\n"
        "movl imp_scrVarPub, %eax\n"
        "movl 0x10(%eax), %eax\n"
        "testl %eax, %eax\n"
        "je .Lf92eea_00092fef\n"
        "movl %eax, 8(%esp)\n"
        "movl $"%s", 4(%esp)\n"
        "movl 8(%ebp), %edx\n"
        "movl %edx, (%esp)\n"
        "calll CompileError\n"
        "xorl %eax, %eax\n"
        "jmp .Lf92eea_00092f0a\n"
        ".Lf92eea_00092fcb:\n"
        "movl 4(%eax), %esi\n"
        "leal -0x24(%ebp), %edx\n"
        "movl %esi, %eax\n"
        "calll EvalPrimitiveExpression\n"
        "jmp .Lf92eea_00092f2f\n"
        ".Lf92eea_00092fdd:\n"
        "movl 4(%edi), %esi\n"
        "leal -0x30(%ebp), %edx\n"
        "movl %esi, %eax\n"
        "calll EvalPrimitiveExpression\n"
        "jmp .Lf92eea_00092f60\n"
        ".Lf92eea_00092fef:\n"
        "movl -0x24(%ebp), %eax\n"
        "movl -0x20(%ebp), %edx\n"
        "movl 0xc(%ebp), %ecx\n"
        "movl %eax, (%ecx)\n"
        "movl %edx, 4(%ecx)\n"
        "movl 8(%ebp), %eax\n"
        "movl %eax, 8(%ecx)\n"
        "movl $1, %eax\n"
        "jmp .Lf92eea_00092f0a\n"
    );
}
#    endif

static Bool __attribute_regparm__(3)
    EvalBinaryOperatorExpression(sval_t expr1, sval_t expr2, sval_t opcode, sval_t sourcePos, VariableCompileValue *constValue)
{
    VariableCompileValue constValue1;
    VariableCompileValue constValue2;
    sval_t *node1;
    sval_t *node2;

    node1 = (sval_t *)(uintptr_t)expr1.node;
    if (!node1) {
        return 0;
    }

    if (node1[0].type == 6) {
        if (!EvalPrimitiveExpression(node1[1], &constValue1)) {
            return 0;
        }
    } else if (node1[0].type == 49) {
        if (!EvalBinaryOperatorExpression(node1[1], node1[2], node1[3], node1[4], &constValue1)) {
            return 0;
        }
    } else {
        return 0;
    }

    node2 = (sval_t *)(uintptr_t)expr2.node;
    if (!node2) {
        return 0;
    }

    if (node2[0].type == 6) {
        if (!EvalPrimitiveExpression(node2[1], &constValue2)) {
            return 0;
        }
    } else if (node2[0].type == 49) {
        if (!EvalBinaryOperatorExpression(node2[1], node2[2], node2[3], node2[4], &constValue2)) {
            return 0;
        }
    } else {
        return 0;
    }

    AddRefToValue(constValue1.value.type, constValue1.value.u);
    AddRefToValue(constValue2.value.type, constValue2.value.u);
    Scr_EvalBinaryOperator(opcode.type, &constValue1.value, &constValue2.value);

    if (SCRVP->error_message) {
        CompileError(sourcePos.sourcePosValue, "%s", SCRVP->error_message);
        return 0;
    }

    constValue->value = constValue1.value;
    constValue->sourcePos = sourcePos;
    return 1;
}

static int
Scr_CalcLocalVarsLocalIndex(const scr_block_t *block, unsigned int name)
{
    int i;

    if (!block) {
        return -1;
    }

    for (i = 0; i < block->localVarsCount; ++i) {
        if (block->localVars[i].name == name) {
            return i;
        }
    }

    return -1;
}

static void
Scr_CalcLocalVarsAddName(scr_block_t *block, unsigned int name)
{
    if (!block || block->abortLevel) {
        return;
    }

    if (Scr_CalcLocalVarsLocalIndex(block, name) >= 0) {
        return;
    }

    if (block->localVarsCount > 63) {
        Com_Error(1, "LOCAL_VAR_STACK_SIZE exceeded");
    }

    block->localVars[block->localVarsCount].name = name;
    ++block->localVarsCount;
}

static scr_block_t *
Scr_CalcLocalVarsGetChildBlock(sval_t *slot, const scr_block_t *parent)
{
    scr_block_t *child;

    child = slot->block;
    if (!child) {
        child = (scr_block_t *)Hunk_AllocateTempMemoryHighInternal(sizeof(*child));
        slot->block = child;
    }

    memcpy(child, parent, sizeof(*child));
    child->localVarsPublicCount = 0;
    return child;
}

static void
Scr_CalcLocalVarsAddChildBlock(scr_block_t **childBlocks, int *childCount, scr_block_t *child)
{
    if (!childBlocks || !childCount || !child) {
        return;
    }

    if (*childCount > 0x3ff) {
        Com_Error(1, "MAX_SWITCH_CASES exceeded");
    }

    childBlocks[*childCount] = child;
    ++*childCount;
}

static void
Scr_CalcLocalVarsRegisterBreakChild(scr_block_t *block)
{
    if (block->abortLevel) {
        return;
    }

    if (SCRCG->breakChildBlocks) {
        Scr_CalcLocalVarsAddChildBlock(SCRCG->breakChildBlocks, SCRCG->breakChildCount, block);
    }
}

static void
Scr_CalcLocalVarsRegisterContinueChild(scr_block_t *block)
{
    if (block->abortLevel) {
        return;
    }

    if (SCRCG->continueChildBlocks) {
        Scr_CalcLocalVarsAddChildBlock(SCRCG->continueChildBlocks, SCRCG->continueChildCount, block);
    }
}

static void
Scr_CalcLocalVarsNormalizeChildBlock(scr_block_t *child, const scr_block_t *parent)
{
    int i;

    if (!child || !parent) {
        return;
    }

    child->localVarsPublicCount = parent->localVarsCount;

    for (i = 0; i < parent->localVarsCount; ++i) {
        unsigned int name = parent->localVars[i].name;
        int at = Scr_CalcLocalVarsLocalIndex(child, name);

        if (at < 0) {
            if (child->localVarsCount > 63) {
                Com_Error(1, "LOCAL_VAR_STACK_SIZE exceeded");
            }
            at = child->localVarsCount++;
        }

        while (at > i) {
            child->localVars[at] = child->localVars[at - 1];
            --at;
        }

        child->localVars[i].name = name;
    }
}

static void
Scr_CalcLocalVarsNormalizeChildBlocks(scr_block_t *parent, scr_block_t **childBlocks, int childCount)
{
    int i;

    if (!parent || parent->abortLevel) {
        return;
    }

    for (i = 0; i < childCount; ++i) {
        Scr_CalcLocalVarsNormalizeChildBlock(childBlocks[i], parent);
    }
}

static int
Scr_CalcLocalVarsChildHasName(const scr_block_t *block, unsigned int name)
{
    return Scr_CalcLocalVarsLocalIndex(block, name) >= 0;
}

static void
Scr_CalcLocalVarsMergeBranchChildBlocks(scr_block_t *block, scr_block_t **childBlocks, int childCount)
{
    int i;
    int child;
    scr_block_t *first;

    if (!block || block->abortLevel || childCount <= 0 || !childBlocks) {
        return;
    }

    for (child = 0; child < childCount; ++child) {
        if (childBlocks[child]) {
            childBlocks[child]->abortLevel = 0;
        }
    }

    first = childBlocks[0];
    if (!first) {
        return;
    }

    for (i = 0; i < first->localVarsCount; ++i) {
        unsigned int name = first->localVars[i].name;

        if (Scr_CalcLocalVarsLocalIndex(block, name) >= 0) {
            continue;
        }

        for (child = 1; child < childCount; ++child) {
            if (!Scr_CalcLocalVarsChildHasName(childBlocks[child], name)) {
                break;
            }
        }

        if (child == childCount) {
            Scr_CalcLocalVarsAddName(block, name);
        }
    }
}

static void
Scr_CalcLocalVarsAssignmentRef(sval_t expr, scr_block_t *block)
{
    sval_t *node;

    node = (sval_t *)(uintptr_t)expr.node;
    if (!node) {
        return;
    }

    Scr_CalcLocalVarsVariableExpressionRef(expr, block);
}

static void
Scr_CalcLocalVarsStatementList(sval_t val, scr_block_t *block)
{
    intptr_t *listNode;

    for (listNode = ScriptStatementListFirstExecNode(val);
         listNode;
         listNode = (intptr_t *)(uintptr_t)listNode[1]) {
        if (listNode[0]) {
            Scr_CalcLocalVarsStatement((uintptr_t)listNode[0], block);
        }
    }
}

static void
Scr_CalcLocalVarsStatementListToBlock(sval_t val, scr_block_t *block)
{
    intptr_t *listNode;

    for (listNode = ScriptStatementListFirstExecNode(val);
         listNode;
         listNode = (intptr_t *)(uintptr_t)listNode[1]) {
        if (listNode[0]) {
            Scr_CalcLocalVarsStatement((uintptr_t)listNode[0], block);
        }
    }
}

static void
Scr_CalcLocalVarsSwitchStatement(sval_t stmtlist, scr_block_t *block)
{
    scr_block_t **oldBreakChildBlocks;
    int *oldBreakChildCount;
    scr_block_t **breakChildBlocks;
    int breakChildCount;
    scr_block_t **caseBlocks;
    int caseBlockCount;
    scr_block_t *currentBlock;
    int switchAbortLevel;
    int hasDefault;
    intptr_t *listNode;

    oldBreakChildBlocks = SCRCG->breakChildBlocks;
    oldBreakChildCount = SCRCG->breakChildCount;
    breakChildBlocks = (scr_block_t **)Hunk_AllocateTempMemoryHighInternal(0x1000);
    caseBlocks = (scr_block_t **)Hunk_AllocateTempMemoryHighInternal(0x1000);
    breakChildCount = 0;
    caseBlockCount = 0;
    currentBlock = NULL;
    switchAbortLevel = 3;
    hasDefault = 0;

    SCRCG->breakChildBlocks = breakChildBlocks;
    SCRCG->breakChildCount = &breakChildCount;

    for (listNode = ScriptStatementListFirstExecNode(stmtlist);
         listNode;
         listNode = (intptr_t *)(uintptr_t)listNode[1]) {
        sval_t *stmt = (sval_t *)(uintptr_t)listNode[0];

        if (!stmt) {
            continue;
        }

        if (stmt[0].type == 0x3d || stmt[0].type == 0x3e) {
            if (currentBlock) {
                if (currentBlock->abortLevel == 2) {
                    currentBlock->abortLevel = 0;
                    Scr_CalcLocalVarsAddChildBlock(caseBlocks, &caseBlockCount, currentBlock);
                    switchAbortLevel = 0;
                } else if (currentBlock->abortLevel) {
                    if (currentBlock->abortLevel < switchAbortLevel) {
                        switchAbortLevel = currentBlock->abortLevel;
                    }
                }
                currentBlock = NULL;
            }

            if (stmt[0].type == 0x3d) {
                currentBlock = Scr_CalcLocalVarsGetChildBlock(&stmt[3], block);
            } else {
                currentBlock = Scr_CalcLocalVarsGetChildBlock(&stmt[2], block);
                hasDefault = 1;
            }
            continue;
        }

        if (currentBlock) {
            Scr_CalcLocalVarsStatement((uintptr_t)stmt, currentBlock);
        }
    }

    if (hasDefault) {
        if (currentBlock) {
            if (currentBlock->abortLevel == 2) {

                currentBlock->abortLevel = 0;
                Scr_CalcLocalVarsAddChildBlock(caseBlocks, &caseBlockCount, currentBlock);
                switchAbortLevel = 0;
            } else if (currentBlock->abortLevel) {
                if (currentBlock->abortLevel < switchAbortLevel) {
                    switchAbortLevel = currentBlock->abortLevel;
                }
            } else {

                Scr_CalcLocalVarsRegisterBreakChild(currentBlock);
                Scr_CalcLocalVarsAddChildBlock(caseBlocks, &caseBlockCount, currentBlock);
            }
        }

        if (!block->abortLevel) {
            block->abortLevel = switchAbortLevel;
        }

        Scr_CalcLocalVarsMergeBranchChildBlocks(block, breakChildBlocks, breakChildCount);
        Scr_CalcLocalVarsNormalizeChildBlocks(block, caseBlocks, caseBlockCount);
    }

    SCRCG->breakChildBlocks = oldBreakChildBlocks;
    SCRCG->breakChildCount = oldBreakChildCount;
}

static unsigned int __attribute_regparm__(2)
    Scr_CalcLocalVarsStatement(uintptr_t val, scr_block_t *block)
{
    sval_t *node;

    node = (sval_t *)(uintptr_t)val;
    if (!node || node[0].type > 0x40) {
        return 0;
    }

    switch (node[0].type) {
    case 0x02:
        Scr_CalcLocalVarsAssignmentRef(node[1], block);
        break;

    case 0x1b:
    case 0x1c:
        if (!block->abortLevel) {
            block->abortLevel = 3;
        }
        break;

    case 0x25: {
        scr_block_t *ifBlock = Scr_CalcLocalVarsGetChildBlock(&node[4], block);
        Scr_CalcLocalVarsStatement((uintptr_t)node[2].node, ifBlock);
        Scr_CalcLocalVarsNormalizeChildBlock(ifBlock, block);
        break;
    }

    case 0x26: {
        scr_block_t *ifBlock;
        scr_block_t *elseBlock;
        scr_block_t *childBlocks[2];
        int childCount = 0;
        int abortLevel = 3;

        ifBlock = Scr_CalcLocalVarsGetChildBlock(&node[6], block);
        Scr_CalcLocalVarsStatement((uintptr_t)node[2].node, ifBlock);
        if (!ifBlock->abortLevel) {
            childBlocks[childCount++] = ifBlock;
            abortLevel = 0;
        } else if (ifBlock->abortLevel < abortLevel) {
            abortLevel = ifBlock->abortLevel;
        }

        elseBlock = Scr_CalcLocalVarsGetChildBlock(&node[7], block);
        Scr_CalcLocalVarsStatement((uintptr_t)node[3].node, elseBlock);
        if (!elseBlock->abortLevel) {
            childBlocks[childCount++] = elseBlock;
            abortLevel = 0;
        } else if (elseBlock->abortLevel < abortLevel) {
            abortLevel = elseBlock->abortLevel;
        }

        if (!block->abortLevel) {
            block->abortLevel = abortLevel;
        }

        Scr_CalcLocalVarsMergeBranchChildBlocks(block, childBlocks, childCount);
        Scr_CalcLocalVarsNormalizeChildBlocks(block, childBlocks, childCount);
        break;
    }

    case 0x27: {
        scr_block_t **oldBreakChildBlocks;
        int *oldBreakChildCount;
        scr_block_t **oldContinueChildBlocks;
        int *oldContinueChildCount;
        scr_block_t **breakChildBlocks;
        scr_block_t **continueChildBlocks;
        int breakChildCount = 0;
        int continueChildCount = 0;
        scr_block_t *whileBlock;
        int i;

        oldBreakChildBlocks = SCRCG->breakChildBlocks;
        oldBreakChildCount = SCRCG->breakChildCount;
        oldContinueChildBlocks = SCRCG->continueChildBlocks;
        oldContinueChildCount = SCRCG->continueChildCount;

        breakChildBlocks = (scr_block_t **)Hunk_AllocateTempMemoryHighInternal(0x1000);
        continueChildBlocks = (scr_block_t **)Hunk_AllocateTempMemoryHighInternal(0x1000);
        SCRCG->breakChildBlocks = breakChildBlocks;
        SCRCG->breakChildCount = &breakChildCount;
        SCRCG->continueChildBlocks = continueChildBlocks;
        SCRCG->continueChildCount = &continueChildCount;

        whileBlock = Scr_CalcLocalVarsGetChildBlock(&node[5], block);
        Scr_CalcLocalVarsStatement((uintptr_t)node[2].node, whileBlock);

        if (!whileBlock->abortLevel) {
            Scr_CalcLocalVarsAddChildBlock(continueChildBlocks, &continueChildCount, whileBlock);
        }

        for (i = 0; i < continueChildCount; ++i) {
            Scr_CalcLocalVarsMergeBranchChildBlocks(block, &continueChildBlocks[i], 1);
        }

        Scr_CalcLocalVarsMergeBranchChildBlocks(block, breakChildBlocks, breakChildCount);
        Scr_CalcLocalVarsNormalizeChildBlock(whileBlock, block);

        SCRCG->breakChildBlocks = oldBreakChildBlocks;
        SCRCG->breakChildCount = oldBreakChildCount;
        SCRCG->continueChildBlocks = oldContinueChildBlocks;
        SCRCG->continueChildCount = oldContinueChildCount;
        break;
    }

    case 0x28: {
        scr_block_t **oldBreakChildBlocks;
        int *oldBreakChildCount;
        scr_block_t **oldContinueChildBlocks;
        int *oldContinueChildCount;
        scr_block_t **breakChildBlocks;
        scr_block_t **continueChildBlocks;
        int breakChildCount = 0;
        int continueChildCount = 0;
        scr_block_t *forBlock;
        scr_block_t *postBlock;
        int i;

        Scr_CalcLocalVarsStatement((uintptr_t)node[1].node, block);

        oldBreakChildBlocks = SCRCG->breakChildBlocks;
        oldBreakChildCount = SCRCG->breakChildCount;
        oldContinueChildBlocks = SCRCG->continueChildBlocks;
        oldContinueChildCount = SCRCG->continueChildCount;

        breakChildBlocks = (scr_block_t **)Hunk_AllocateTempMemoryHighInternal(0x1000);
        continueChildBlocks = (scr_block_t **)Hunk_AllocateTempMemoryHighInternal(0x1000);
        SCRCG->breakChildBlocks = breakChildBlocks;
        SCRCG->breakChildCount = &breakChildCount;
        SCRCG->continueChildBlocks = continueChildBlocks;
        SCRCG->continueChildCount = &continueChildCount;

        forBlock = Scr_CalcLocalVarsGetChildBlock(&node[7], block);
        postBlock = Scr_CalcLocalVarsGetChildBlock(&node[8], block);

        Scr_CalcLocalVarsStatement((uintptr_t)node[4].node, forBlock);
        if (!forBlock->abortLevel) {
            Scr_CalcLocalVarsAddChildBlock(continueChildBlocks, &continueChildCount, forBlock);
        }

        for (i = 0; i < continueChildCount; ++i) {
            Scr_CalcLocalVarsMergeBranchChildBlocks(block, &continueChildBlocks[i], 1);
        }

        Scr_CalcLocalVarsStatement((uintptr_t)node[3].node, postBlock);
        Scr_CalcLocalVarsMergeBranchChildBlocks(block, &postBlock, 1);
        Scr_CalcLocalVarsNormalizeChildBlock(postBlock, block);
        Scr_CalcLocalVarsMergeBranchChildBlocks(block, breakChildBlocks, breakChildCount);
        Scr_CalcLocalVarsNormalizeChildBlock(forBlock, block);

        SCRCG->breakChildBlocks = oldBreakChildBlocks;
        SCRCG->breakChildCount = oldBreakChildCount;
        SCRCG->continueChildBlocks = oldContinueChildBlocks;
        SCRCG->continueChildCount = oldContinueChildCount;
        break;
    }

    case 0x29:
    case 0x2a:
    case 0x2b:
        Scr_CalcLocalVarsAssignmentRef(node[1], block);
        break;

    case 0x2c:
        Scr_CalcLocalVarsStatementList(node[1], block);
        break;

    case 0x2d: {
        scr_block_t *devBlock = Scr_CalcLocalVarsGetChildBlock(&node[3], block);
        Scr_CalcLocalVarsStatementListToBlock(node[1], devBlock);
        Scr_CalcLocalVarsNormalizeChildBlock(devBlock, block);
        break;
    }

    case 0x37: {

        intptr_t *listNode = ScriptStatementListFirstExecNode(node[2]);
        listNode = listNode ? (intptr_t *)(uintptr_t)listNode[1] : 0;
        for (; listNode; listNode = (intptr_t *)(uintptr_t)listNode[1]) {
            sval_t *expr = (sval_t *)(uintptr_t)listNode[0];
            if (expr) {
                Scr_CalcLocalVarsAddName(block, expr[0].stringValue);
            }
        }
        break;
    }

    case 0x3c:
        Scr_CalcLocalVarsSwitchStatement(node[2], block);
        break;

    case 0x3f:
        Scr_CalcLocalVarsRegisterBreakChild(block);
        if (!block->abortLevel) {
            block->abortLevel = 2;
        }
        break;

    case 0x40:
        Scr_CalcLocalVarsRegisterContinueChild(block);
        if (!block->abortLevel) {
            block->abortLevel = 1;
        }
        break;

    default:
        break;
    }

    return 0;
}

static unsigned int __attribute_regparm__(1)
    EmitValue(VariableCompileValue *constValue)
{
    unsigned int sourcePos;
    unsigned int stringValue;
    const float *vectorValue;
    int i;

    switch (constValue->value.type) {
    case 0:
        sourcePos = constValue->sourcePos.sourcePosValue;
        EmitOpcode(2, 1, 0);
        AddOpcodePos(sourcePos, 1);
        break;

    case 1:
        break;

    case 2:
        sourcePos = constValue->sourcePos.sourcePosValue;
        stringValue = constValue->value.u.stringValue;
        EmitOpcode(0x0a, 1, 0);
        AddOpcodePos(sourcePos, 1);
        *(short *)TempMallocAlign(2) = (short)stringValue;

        if (SCRCP->developer_statement == SCRCOMP_SCR_DEV_IGNORE) {
            if (!SCRCG->bConstRefCount) {
                SL_RemoveRefToString(stringValue);
            }
        } else {
            if (SCRCG->bConstRefCount) {
                SL_AddRefToString(stringValue);
            }
            SL_TransferRefToUser(stringValue, 1);
        }
        break;

    case 3:
        sourcePos = constValue->sourcePos.sourcePosValue;
        stringValue = constValue->value.u.stringValue;
        EmitOpcode(0x0b, 1, 0);
        AddOpcodePos(sourcePos, 1);
        *(short *)TempMallocAlign(2) = (short)stringValue;

        if (SCRCP->developer_statement == SCRCOMP_SCR_DEV_IGNORE) {
            if (!SCRCG->bConstRefCount) {
                SL_RemoveRefToString(stringValue);
            }
        } else {
            if (SCRCG->bConstRefCount) {
                SL_AddRefToString(stringValue);
            }
            SL_TransferRefToUser(stringValue, 1);
        }
        break;

    case 4:
        sourcePos = constValue->sourcePos.sourcePosValue;
        vectorValue = SCR_VEC_PTR(constValue->value.u);
        EmitOpcode(0x0c, 1, 0);
        AddOpcodePos(sourcePos, 1);
        for (i = 0; i != 3; ++i) {
            *(float *)TempMallocAlignStrict(4) = vectorValue[i];
        }
        RemoveRefToVector(vectorValue);
        break;

    case 5:
        sourcePos = constValue->sourcePos.sourcePosValue;
        EmitOpcode(9, 1, 0);
        AddOpcodePos(sourcePos, 1);
        *(float *)TempMallocAlignStrict(4) = constValue->value.u.floatValue;
        break;

    case 6:
        EmitGetInteger(constValue->value.u.intValue, constValue->sourcePos.sourcePosValue);
        break;

    default:
        break;
    }

    return 0;
}

static unsigned int __attribute_regparm__(3)
    EmitOpcode(unsigned int op, int offset, int callType)
{
    byte *prevOpcode;
    unsigned int foldedIndex;
    int valueIndex;

    if (SCRCP->value_count) {
        valueIndex = SCRCP->value_count;
        SCRCP->value_count = 0;
        if (valueIndex > 0) {
            VariableCompileValue *value = SCRCG->value_start;
            int i;

            for (i = 0; i != valueIndex; ++i, ++value) {
                EmitValue(value);
            }
        }
    }

    SCRCP->allowedBreakpoint = (!SCRCG->cumulOffset || callType == 2 || callType == 3);

    SCRCG->cumulOffset += offset;
    if (SCRCG->cumulOffset > SCRCG->maxOffset) {
        SCRCG->maxOffset = SCRCG->cumulOffset;
    }
    if (callType && SCRCG->cumulOffset > SCRCG->maxCallOffset) {
        SCRCG->maxCallOffset = SCRCG->cumulOffset;
    }

    SCRVP->checksum = SCRVP->checksum * 31 + op;

    prevOpcode = SCRCP->opcodePos;
    if (prevOpcode) {
        SCRCG->codePos = prevOpcode;

        if ((unsigned int)(op - 0x20) <= 0x3e) {
            switch (op) {
            case 0x20:
                if (*prevOpcode == 0x1e) {
                    RemoveOpcodePos();
                    *SCRCP->opcodePos = 0x1f;
                    return 0;
                }

                foldedIndex = *prevOpcode - 0x18;
                if (foldedIndex <= 5) {
                    RemoveOpcodePos();
                    *SCRCP->opcodePos = 0x1f;
                    SCRCG->codePos = (byte *)TempMalloc(1);
                    *SCRCG->codePos = (byte)foldedIndex;
                    return 0;
                }
                break;

            case 0x23:
                if (*prevOpcode == 0x37) {
                    RemoveOpcodePos();
                    *SCRCP->opcodePos = 0x22;
                    return 0;
                }
                if (*prevOpcode == 0x36) {
                    RemoveOpcodePos();
                    *SCRCP->opcodePos = 0x21;
                    return 0;
                }
                break;

            case 0x2a:
                if (*prevOpcode == 0x26) {
                    *prevOpcode = 0x29;
                    return 0;
                }
                if (*prevOpcode == 0x0d) {
                    *prevOpcode = 0x27;
                    return 0;
                }
                if (*prevOpcode == 0x0e) {
                    *prevOpcode = 0x28;
                    return 0;
                }
                break;

            case 0x2e:
                if (*prevOpcode == 0x26) {
                    *prevOpcode = 0x2d;
                    return 0;
                }
                if (*prevOpcode == 0x0d) {
                    *prevOpcode = 0x2b;
                    return 0;
                }
                if (*prevOpcode == 0x0e) {
                    *prevOpcode = 0x2c;
                    return 0;
                }
                break;

            case 0x31:
                if (*prevOpcode == 0x16) {
                    *prevOpcode = 0x30;
                    return 0;
                }
                break;

            case 0x39:
                if (*prevOpcode == 0x37) {
                    RemoveOpcodePos();
                    *SCRCP->opcodePos = 0x3d;
                    return 0;
                }
                if (*prevOpcode == 0x36) {
                    RemoveOpcodePos();
                    *SCRCP->opcodePos = 0x3c;
                    return 0;
                }
                if (*prevOpcode == 0x2d) {
                    RemoveOpcodePos();
                    *SCRCP->opcodePos = 0x3b;
                    return 0;
                }
                if (*prevOpcode == 0x2b) {
                    RemoveOpcodePos();
                    *SCRCP->opcodePos = 0x38;
                    return 0;
                }
                if (*prevOpcode == 0x2c) {
                    RemoveOpcodePos();
                    *SCRCP->opcodePos = 0x3a;
                    return 0;
                }
                break;

            case 0x50:
                if (*prevOpcode == 0x4e) {
                    *prevOpcode = 0x4f;
                    return 0;
                }
                break;

            case 0x52:
                if (*prevOpcode == 0x0f) {
                    RemoveOpcodePos();
                    *SCRCP->opcodePos = 0x50;
                    if (SCRCG->prevOpcodePos && *SCRCG->prevOpcodePos == 0x4e) {
                        TempMemorySetPos((char *)SCRCP->opcodePos);
                        SCRCP->opcodePos = SCRCP->opcodePos - 1;
                        SCRCG->prevOpcodePos = NULL;
                        SCRCG->codePos = SCRCP->opcodePos;
                        *SCRCG->codePos = 0x4f;
                    }
                    return 0;
                }
                break;

            case 0x56:
                if (*prevOpcode == 0x0f) {
                    RemoveOpcodePos();
                    *SCRCP->opcodePos = 0x54;
                    return 0;
                }
                break;

            case 0x59:
                if (*prevOpcode == 0x1e) {
                    *prevOpcode = 0x5a;
                    return 0;
                }

                foldedIndex = *prevOpcode - 0x18;
                if (foldedIndex <= 5) {
                    *prevOpcode = 0x5a;
                    SCRCG->codePos = (byte *)TempMalloc(1);
                    *SCRCG->codePos = (byte)foldedIndex;
                    return 0;
                }
                break;

            case 0x5e:
                if (*prevOpcode == 0x5c) {
                    RemoveOpcodePos();
                    *SCRCP->opcodePos = 0x5f;
                    return 0;
                }
                break;

            default:
                break;
            }
        }
    }

    SCRCG->prevOpcodePos = SCRCP->opcodePos;
    SCRCP->opcodePos = (byte *)TempMalloc(1);
    SCRCG->codePos = SCRCP->opcodePos;
    *SCRCG->codePos = (byte)op;
    return 0;
}

static unsigned int __attribute_regparm__(2)
    EmitGetInteger(int value, unsigned int sourcePos)
{
    if (value == 0) {
        EmitOpcode(3, 1, 0);
        AddOpcodePos(sourcePos, 1);
        return 0;
    }

    if (value > 0) {
        if (value <= 0xff) {
            EmitOpcode(4, 1, 0);
            AddOpcodePos(sourcePos, 1);
            *TempMalloc(1) = (char)value;
        } else if (value <= 0xffff) {
            EmitOpcode(6, 1, 0);
            AddOpcodePos(sourcePos, 1);
            *(short *)TempMallocAlign(2) = (short)value;
        } else {
            EmitOpcode(8, 1, 0);
            AddOpcodePos(sourcePos, 1);
            *(int *)TempMallocAlign(4) = value;
        }
        return 0;
    }

    if (value > -256) {
        EmitOpcode(5, 1, 0);
        AddOpcodePos(sourcePos, 1);
        *TempMalloc(1) = (char)-value;
    } else if (value > -65536) {
        EmitOpcode(7, 1, 0);
        AddOpcodePos(sourcePos, 1);
        *(short *)TempMallocAlign(2) = (short)-value;
    } else {
        EmitOpcode(8, 1, 0);
        AddOpcodePos(sourcePos, 1);
        *(int *)TempMallocAlign(4) = value;
    }

    return 0;
}

#    if 0
static __attribute__((naked))
int Scr_FindLocalVarIndex(unsigned int name, sval_t sourcePos, Bool create, scr_block_t *block)
{
    __asm__ __volatile__ (
        "pushl %ebp\n"
        "movl %esp, %ebp\n"
        "pushl %edi\n"
        "pushl %esi\n"
        "pushl %ebx\n"
        "subl $0x3c, %esp\n"
        "movl %eax, -0x24(%ebp)\n"
        "movl %edx, -0x28(%ebp)\n"
        "movb %cl, -0x29(%ebp)\n"

        "movl 8(%ebp), %esi\n"
        "testl %esi, %esi\n"
        "je .Lf95688_000957d9\n"
        "movl 8(%ebp), %eax\n"
        "movl 0xc(%eax), %ebx\n"
        "testl %ebx, %ebx\n"
        "jle .Lf95688_000957ca\n"
        "movl %eax, %edi\n"
        "xorl %esi, %esi\n"
        "movl 8(%ebp), %eax\n"
        "jmp .Lf95688_000956da\n"
        ".Lf95688_000956bc:\n"
        "movl -0x24(%ebp), %eax\n"
        "cmpl 0x18(%edi), %eax\n"
        "je .Lf95688_00095761\n"
        ".Lf95688_000956c8:\n"
        "addl $1, %esi\n"
        "addl $4, %edi\n"
        "movl 8(%ebp), %eax\n"
        "cmpl 0xc(%eax), %esi\n"
        "jge .Lf95688_000957ca\n"
        ".Lf95688_000956da:\n"
        "cmpl 4(%eax), %esi\n"
        "jne .Lf95688_000956bc\n"
        "movl %eax, %edx\n"
        "leal 1(%esi), %eax\n"
        "movl %eax, 4(%edx)\n"
        "xorl %ecx, %ecx\n"
        "xorl %edx, %edx\n"
        "movl $0x16, %eax\n"
        "calll EmitOpcode\n"
        "movl 0x18(%edi), %eax\n"
        "movl %eax, -0x1c(%ebp)\n"

        "movzbl scrCompileGlob+28, %edx\n"
        "movb %dl, -0x1d(%ebp)\n"
        "movb $1, scrCompileGlob+28\n"
        "movl $2, (%esp)\n"
        "calll TempMallocAlign\n"
        "movl %eax, scrCompileGlob\n"
        "cmpl $2, scrCompilePub+40\n"
        "je .Lf95688_000957f6\n"
        "cmpb $0, scrCompileGlob+28\n"
        "jne .Lf95688_00095813\n"
        ".Lf95688_00095737:\n"
        "movl scrCompileGlob, %ebx\n"
        "movl -0x1c(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll SL_TransferToCanonicalString\n"
        "movw %ax, (%ebx)\n"
        ".Lf95688_0009574b:\n"
        "movzbl -0x1d(%ebp), %edx\n"
        "movb %dl, scrCompileGlob+28\n"

        "movl -0x24(%ebp), %eax\n"
        "cmpl 0x18(%edi), %eax\n"
        "jne .Lf95688_000956c8\n"
        ".Lf95688_00095761:\n"
        "cmpb $0, scrCompileGlob+28\n"
        "je .Lf95688_00095848\n"
        ".Lf95688_0009576e:\n"
        "movl %esi, %ecx\n"
        "andl $7, %ecx\n"
        "movl $1, %eax\n"
        "movl %eax, %edx\n"
        "shlb %cl, %dl\n"
        "movl %esi, %ebx\n"
        "sarl $3, %ebx\n"
        "movl 8(%ebp), %eax\n"
        "movzbl 0x10(%ebx, %eax), %ecx\n"
        "movzbl %dl, %eax\n"
        "testl %ecx, %eax\n"
        "jne .Lf95688_00095839\n"
        "cmpb $0, -0x29(%ebp)\n"
        "jne .Lf95688_00095823\n"
        ".Lf95688_0009579e:\n"
        "movl -0x24(%ebp), %edx\n"
        "movl %edx, (%esp)\n"
        "calll SL_ConvertToString\n"
        "movl %eax, 8(%esp)\n"
        "movl $"uninitialised variable '%s'", 4(%esp)\n"
        "movl -0x28(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll CompileError\n"
        "xorl %eax, %eax\n"

        ".Lf95688_000957c2:\n"
        "addl $0x3c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"

        ".Lf95688_000957ca:\n"
        "cmpb $0, -0x29(%ebp)\n"
        "je .Lf95688_0009579e\n"
        "cmpb $0, scrCompileGlob+80\n"
        "jne .Lf95688_0009579e\n"
        ".Lf95688_000957d9:\n"
        "movl $"unreachable code", 4(%esp)\n"
        "movl -0x28(%ebp), %edx\n"
        "movl %edx, (%esp)\n"
        "calll CompileError\n"
        "xorl %eax, %eax\n"

        "addl $0x3c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"

        ".Lf95688_000957f6:\n"
        "cmpb $0, scrCompileGlob+28\n"
        "jne .Lf95688_0009574b\n"
        "movl -0x1c(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll SL_RemoveRefToString\n"
        "jmp .Lf95688_0009574b\n"

        ".Lf95688_00095813:\n"
        "movl -0x1c(%ebp), %edx\n"
        "movl %edx, (%esp)\n"
        "calll SL_AddRefToString\n"
        "jmp .Lf95688_00095737\n"

        ".Lf95688_00095823:\n"
        "cmpb $0, scrCompileGlob+80\n"
        "jne .Lf95688_0009579e\n"
        "orb %cl, %dl\n"
        "movl 8(%ebp), %eax\n"
        "movb %dl, 0x10(%ebx, %eax)\n"
        ".Lf95688_00095839:\n"
        "movl %esi, %eax\n"
        "notl %eax\n"
        "movl 8(%ebp), %edx\n"
        "addl 4(%edx), %eax\n"
        "jmp .Lf95688_000957c2\n"
        ".Lf95688_00095848:\n"
        "movl %eax, (%esp)\n"
        "calll SL_RemoveRefToString\n"
        "jmp .Lf95688_0009576e\n"
    );
}
#    endif

static void Scr_EmitCanonicalLocalVarName(unsigned int stringValue)
{
    Bool oldConstRefCount = SCRCG->bConstRefCount;

    SCRCG->bConstRefCount = 1;
    SCRCG->codePos = (byte *)TempMallocAlign(2);

    if (SCRCP->developer_statement == SCRCOMP_SCR_DEV_IGNORE) {
        if (!SCRCG->bConstRefCount)
            SL_RemoveRefToString(stringValue);
    } else {
        if (SCRCG->bConstRefCount)
            SL_AddRefToString(stringValue);
        *(unsigned short *)SCRCG->codePos = (unsigned short)SL_TransferToCanonicalString(stringValue);
    }

    SCRCG->bConstRefCount = oldConstRefCount;
}

static int __attribute_regparm__(3)
    Scr_FindLocalVarIndex(unsigned int name, sval_t sourcePos, Bool create, scr_block_t *block)
{
    int i;

    if (!block) {
        CompileError(sourcePos.sourcePosValue, "unreachable code");
        return 0;
    }

    for (i = 0; i < block->localVarsCount; ++i) {
        if (i == block->localVarsCreateCount) {
            block->localVarsCreateCount = i + 1;
            EmitOpcode(0x16, 0, 0);
            Scr_EmitCanonicalLocalVarName(block->localVars[i].name);
        }

        if (block->localVars[i].name == name) {
            byte bit = (byte)(1 << (i & 7));
            int byteIndex = i >> 3;

            if (!SCRCG->bConstRefCount)
                SL_RemoveRefToString(name);

            if (!(block->localVarsInitBits[byteIndex] & bit)) {
                if (!create || SCRCG->forceNotCreate) {
                    CompileError(sourcePos.sourcePosValue, "uninitialised variable '%s'", SL_ConvertToString(name));
                    return 0;
                }
                block->localVarsInitBits[byteIndex] |= bit;
            }

            return block->localVarsCreateCount - i - 1;
        }
    }

    if (!create || SCRCG->forceNotCreate) {
        CompileError(sourcePos.sourcePosValue, "uninitialised variable '%s'", SL_ConvertToString(name));
        return 0;
    }

    CompileError(sourcePos.sourcePosValue, "unreachable code");
    return 0;
}

static unsigned int __attribute_regparm__(2)
    EmitContinueStatement(unsigned int sourcePos, scr_block_t *block)
{
    ContinueStatementInfo *statement;

    if (!SCRCG->bCanContinue || block->abortLevel) {
        CompileError(sourcePos, "illegal continue statement");
        return 0;
    }

    if (SCRCG->continueChildBlocks) {
        if (*SCRCG->continueChildCount > 0x3ff) {
            Com_Error(1, "MAX_SWITCH_CASES exceeded");
        }
        SCRCG->continueChildBlocks[*SCRCG->continueChildCount] = block;
        ++*SCRCG->continueChildCount;
    }

    Scr_EmitLocalVarsRemove(block, block->localVarsPublicCount);
    block->abortLevel = 1;

    EmitOpcode(0x62, 0, 0);
    AddOpcodePos(sourcePos, 1);
    SCRCG->codePos = (byte *)TempMallocAlign(4);
    *(int *)SCRCG->codePos = 0;

    statement = (ContinueStatementInfo *)Hunk_AllocateTempMemoryHighInternal(sizeof(*statement));
    statement->codePos = (const char *)SCRCG->codePos;
    statement->nextCodePos = TempMalloc(0);
    statement->next = SCRCG->currentContinueStatement;
    SCRCG->currentContinueStatement = statement;

    return 0;
}

static unsigned int __attribute_regparm__(2)
    EmitBreakStatement(unsigned int sourcePos, scr_block_t *block)
{
    BreakStatementInfo *statement;

    if (!SCRCG->bCanBreak || block->abortLevel) {
        CompileError(sourcePos, "illegal break statement");
        return 0;
    }

    if (SCRCG->breakChildBlocks) {
        if (*SCRCG->breakChildCount > 0x3ff) {
            Com_Error(1, "MAX_SWITCH_CASES exceeded");
        }
        SCRCG->breakChildBlocks[*SCRCG->breakChildCount] = block;
        ++*SCRCG->breakChildCount;
    }

    Scr_EmitLocalVarsRemove(block, SCRCG->breakBlock->localVarsPublicCount);
    block->abortLevel = 2;

    EmitOpcode(0x62, 0, 0);
    AddOpcodePos(sourcePos, 1);
    SCRCG->codePos = (byte *)TempMallocAlign(4);
    *(int *)SCRCG->codePos = 0;

    statement = (BreakStatementInfo *)Hunk_AllocateTempMemoryHighInternal(sizeof(*statement));
    statement->codePos = (const char *)SCRCG->codePos;
    statement->nextCodePos = TempMalloc(0);
    statement->next = SCRCG->currentBreakStatement;
    SCRCG->currentBreakStatement = statement;

    return 0;
}

static unsigned int __attribute_regparm__(2)
    EmitFunction(uintptr_t func, unsigned int sourcePos)
{
    sval_t *node = (sval_t *)(uintptr_t)func;
    unsigned int scope;
    unsigned int threadId;
    unsigned int name;
    VariableValue value;
    VariableValue count;
    unsigned int countId;
    unsigned int newId;
    PrecacheEntry *entry;

    if (SCRCP->developer_statement == SCRCOMP_SCR_DEV_IGNORE) {
        name = node[1].stringValue;
        if (!SCRCG->bConstRefCount)
            SL_RemoveRefToString(name);

        if (node[0].type == 0x15) {
            name = node[2].stringValue;
            if (!SCRCG->bConstRefCount)
                SL_RemoveRefToString(name);
            --SCRCP->far_function_count;
        }

        return 0;
    }

    if (node[0].type == 0x14) {
        scope = GetVariable(SCRCG->fileId, node[1].stringValue);
        name = node[1].stringValue;

        if (SCRCP->developer_statement == SCRCOMP_SCR_DEV_IGNORE) {
            if (!SCRCG->bConstRefCount)
                SL_RemoveRefToString(name);
        } else {
            if (SCRCG->bConstRefCount)
                SL_AddRefToString(name);
            SL_TransferRefToUser(name, 2);
        }

        threadId = GetObjectA(scope);
        scope = 0;
    } else {
        unsigned int scriptId;
        unsigned int scriptThreadTable;
        VariableValue scriptValue;

        scope = Scr_CreateCanonicalFilename(
            SL_ConvertToString(node[1].stringValue));
        if (!SCRCG->bConstRefCount)
            SL_RemoveRefToString(node[1].stringValue);

        scriptId = FindVariable(SCRCP->loadedscripts, scope);
        Scr_EvalVariableValue(scriptId, &scriptValue);

        SL_AddRefToString(scope);
        if (!SCRCG->bConstRefCount)
            SL_RemoveRefToString(scope);

        entry = SCRCG->precachescriptList;
        entry->filename = (scr_string_t)scope;
        entry->sourcePos = sourcePos;
        entry->include = 0;
        SCRCG->precachescriptList = entry + 1;

        scriptThreadTable = GetObjectA(GetVariable(SCRCP->scripts, scope));
        name = node[2].stringValue;

        if (scriptValue.type) {
            scope = FindVariable(scriptThreadTable, name);
            if (!scope) {
#    ifdef LT_TRACE
                fprintf(stderr, "[UNKFN-1844] file='%s' func='%s' pos=%u\n",
                        SL_ConvertToString((unsigned int)entry->filename), SL_ConvertToString(name), sourcePos);
#    endif
                CompileError(sourcePos, "unknown function");
                return 0;
            }

            if (GetVarType(scope) != SCRCOMP_VAR_POINTER) {
#    ifdef LT_TRACE
                fprintf(stderr, "[UNKFN-1849] file='%s' func='%s' pos=%u\n",
                        SL_ConvertToString((unsigned int)entry->filename), SL_ConvertToString(name), sourcePos);
#    endif
                CompileError(sourcePos, "unknown function");
                return 0;
            }
        } else {
            scope = GetVariable(scriptThreadTable, name);
        }

        if (SCRCP->developer_statement == SCRCOMP_SCR_DEV_IGNORE) {
            if (!SCRCG->bConstRefCount)
                SL_RemoveRefToString(name);
        } else {
            if (SCRCG->bConstRefCount)
                SL_AddRefToString(name);
            SL_TransferRefToUser(name, 2);
        }

        threadId = GetObjectA(scope);
        scope = FindVariable(threadId, 1);
        if (scope) {
            Scr_EvalVariableValue(scope, &value);

            if (value.type == SCRCOMP_VAR_INCLUDE_CODEPOS) {
#    ifdef LT_TRACE
                fprintf(stderr, "[UNKFN-1871] file='%s' func='%s' pos=%u\n",
                        SL_ConvertToString((unsigned int)entry->filename), SL_ConvertToString(name), sourcePos);
#    endif
                CompileError(sourcePos, "unknown function");
                return 0;
            }

            if (value.u.intValue) {
                if (value.type == SCRCOMP_VAR_CODEPOS ||
                    SCRCP->developer_statement != SCRCOMP_SCR_DEV_NO) {
                    SCRCG->codePos = (byte *)TempMallocAlign(4);
                    *(int *)SCRCG->codePos = value.u.intValue;
                    return 0;
                }

                CompileError(sourcePos, "normal script cannot reference a function in a /# ... #/ comment");
                return 0;
            }
        }

        scope = 1;
    }

    SCRCG->codePos = (byte *)TempMallocAlign(4);
    *(int *)SCRCG->codePos = scope;

    countId = GetVariable(threadId, 0);
    Scr_EvalVariableValue(countId, &count);
    if (!count.type) {
        count.u.intValue = 0;
        count.type = SCRCOMP_VAR_INTEGER;
    }

    newId = GetNewVariable(threadId, count.u.intValue + 2);
    SCR_CODEPOS_SET(value.u, (const char *)SCRCG->codePos);
    value.type = SCRCP->developer_statement < SCRCOMP_SCR_DEV_ON
                     ? SCRCOMP_VAR_CODEPOS
                     : SCRCOMP_VAR_DEVELOPER_CODEPOS;
    SetNewVariableValue(newId, &value);

    ++count.u.intValue;
    SetVariableValue(countId, &count);
    AddOpcodePos(sourcePos, 0);

    return 0;
}

static int __attribute_regparm__(3)
    EmitCallArgs(sval_t params, scr_block_t *block, VariableCompileValue *constValue)
{
    int param_count = 0;
    intptr_t *list = (intptr_t *)(uintptr_t)params.node;
    intptr_t *node = list ? (intptr_t *)(uintptr_t)list[0] : 0;

    while (node) {
        intptr_t *entry = (intptr_t *)(uintptr_t)node[0];
        sval_t expr;

        expr.node = entry ? entry[0] : 0;
        if (EmitOrEvalExpression(expr, constValue, block)) {
            EmitValue(constValue);
        }

        ++param_count;
        node = (intptr_t *)(uintptr_t)node[1];
    }

    return param_count;
}

static void __attribute_regparm__(1)
    EmitDeveloperParamOpcodePos(sval_t params)
{
    intptr_t *list;
    intptr_t *node;

    if (!SCRVP->developer) {
        return;
    }

    list = (intptr_t *)(uintptr_t)params.node;
    node = list ? (intptr_t *)(uintptr_t)list[0] : 0;
    while (node) {
        intptr_t *entry = (intptr_t *)(uintptr_t)node[0];
        if (entry) {
            AddOpcodePos((unsigned int)entry[1], 0);
        }
        node = (intptr_t *)(uintptr_t)node[1];
    }
}

static int __attribute_regparm__(1)
    EmitFunctionTableIndex(intptr_t func)
{
    int i;

    /* Dedup by the low 32 bits: a builtin cached in a 4-byte script-variable value comes back
       truncated on x64, but its low bits still uniquely identify the (single-module) function,
       so it matches the full pointer already stored here. */
    for (i = 0; i < scrCompilePub.func_table_size; ++i) {
        if ((unsigned int)scrCompilePub.func_table[i] == (unsigned int)func) {
            return i;
        }
    }

    if (scrCompilePub.func_table_size == 0x400) {
        Com_Error(1, "\x15SCR_FUNC_TABLE_SIZE exceeded");
    }

    i = scrCompilePub.func_table_size;
    scrCompilePub.func_table[i] = func;
    scrCompilePub.func_table_size = i + 1;
    return i;
}

static int __attribute_regparm__(2)
    EmitPrepareDeveloperCall(int *type, char **savedPos)
{
    if (*type != 1) {
        return 0;
    }

    if (scrCompilePub.programLen) {
        *type = 0;
        return 0;
    }

    if (SCRVP->developer_script) {
        scrCompilePub.programLen = 1;
    } else {
        *savedPos = TempMalloc(0);
        scrCompilePub.programLen = 2;
    }

    return 1;
}

static void __attribute_regparm__(2)
    EmitFinishDeveloperCall(int type, char *savedPos)
{
    if (type != 1) {
        return;
    }

    scrCompilePub.programLen = 0;
    if (!SCRVP->developer_script) {
        TempMemorySetPos(savedPos);
    }
}

static void __attribute_regparm__(3)
    EmitBuiltinFunctionOpcode(int param_count, unsigned int sourcePos, intptr_t func)
{
    int opcode;
    int index;

    if (param_count > 0xff) {
        CompileError(sourcePos, "parameter count exceeds 256");
        return;
    }

    if (param_count > 5) {
        char *out;

        opcode = 0x44;
        EmitOpcode(opcode, 1 - param_count, 1);
        AddOpcodePos(sourcePos, 1);
        out = TempMalloc(1);
        SCRCG->codePos = (byte *)out;
        *out = (char)param_count;
    } else {
        opcode = param_count + 0x3e;
        EmitOpcode(opcode, 1 - param_count, 1);
        AddOpcodePos(sourcePos, 1);
    }

    index = EmitFunctionTableIndex(func);
    SCRCG->codePos = (byte *)TempMallocAlign(2);
    *(short *)SCRCG->codePos = (short)index;
}

static void __attribute_regparm__(3)
    EmitBuiltinMethodOpcode(int param_count, unsigned int sourcePos, intptr_t meth)
{
    int opcode;
    int index;

    if (param_count > 0xff) {
        CompileError(sourcePos, "parameter count exceeds 256");
        return;
    }

    if (param_count > 5) {
        char *out;

        opcode = 0x4b;
        EmitOpcode(opcode, -param_count, 1);
        AddOpcodePos(sourcePos, 1);
        out = TempMalloc(1);
        SCRCG->codePos = (byte *)out;
        *out = (char)param_count;
    } else {
        opcode = param_count + 0x45;
        EmitOpcode(opcode, -param_count, 1);
        AddOpcodePos(sourcePos, 1);
    }

    index = EmitFunctionTableIndex(meth);
    SCRCG->codePos = (byte *)TempMallocAlign(2);
    *(short *)SCRCG->codePos = (short)index;
}

static unsigned int __attribute_regparm__(3)
    EmitCall(sval_t func_name, sval_t params, Bool bStatement, scr_block_t *block)
{
    intptr_t *node = (intptr_t *)(uintptr_t)func_name.node;
    VariableCompileValue constValue;
    int param_count;

    if (node && node[0] == 0x1a) {
        intptr_t *target = (intptr_t *)(uintptr_t)node[1];

        if (target && target[0] == 0x12) {
            intptr_t *funcNode = (intptr_t *)(uintptr_t)target[1];

            if (funcNode && funcNode[0] == 0x14 &&
                !FindVariable(SCRCG->fileId, (unsigned int)funcNode[1])) {
                unsigned int name = (unsigned int)funcNode[1];
                const char *pName;
                unsigned int sourcePos;
                VariableValue value;
                unsigned int varId;
                int type;
                intptr_t func;
                char *savedPos = 0;

                if (!name) {
                    goto regular_call;
                }

                pName = SL_ConvertToString(name);
                sourcePos = (unsigned int)node[2];
                varId = FindVariable(scrCompilePub.builtinFunc, name);
                if (varId) {
                    Scr_EvalVariableValue(varId, &value);
                    type = (value.type != SCRCOMP_VAR_CODEPOS);
                    func = value.u.intValue;
                } else {
                    unsigned int newId;

                    type = 0;
                    func = Scr_GetFunction(&pName, &type);
                    newId = GetNewVariable(scrCompilePub.builtinFunc, name);
                    value.type = (type == 1) ? SCRCOMP_VAR_DEVELOPER_CODEPOS : SCRCOMP_VAR_CODEPOS;
                    value.u.intValue = func;
                    SetVariableValue(newId, &value);
                }

                if (!func) {
                    goto regular_call;
                }

                if (EmitPrepareDeveloperCall(&type, &savedPos) && !bStatement) {
                    CompileError(sourcePos, "return value of developer command can not be accessed if not in a /# ... #/ comment");
                    return 0;
                }

                param_count = EmitCallArgs(params, block, &constValue);
                if (!SCRCG->bConstRefCount) {
                    SL_RemoveRefToString(name);
                }

                EmitBuiltinFunctionOpcode(param_count, sourcePos, func);
                EmitDeveloperParamOpcodePos(params);
                if (bStatement) {
                    EmitOpcode(0x58, -1, 0);
                }
                EmitFinishDeveloperCall(type, savedPos);
                return 0;
            }
        }

    regular_call:
        if (node[0] == 0x1a) {
            EmitOpcode(0x4e, 1, 0);
        }
    }

    param_count = EmitCallArgs(params, block, &constValue);
    EmitPostFunctionCall(func_name, param_count, 0, block);
    EmitDeveloperParamOpcodePos(params);
    if (bStatement) {
        EmitOpcode(0x58, -1, 0);
    }
    return 0;
}

static unsigned int __attribute_regparm__(3)
    EmitMethod(sval_t expr, sval_t func_name, sval_t params, sval_t methodSourcePos, int bStatement, scr_block_t *block)
{
    intptr_t *node = (intptr_t *)(uintptr_t)func_name.node;
    VariableCompileValue constValue;
    int param_count;

    if (node && node[0] == 0x1a) {
        intptr_t *target = (intptr_t *)(uintptr_t)node[1];

        if (target && target[0] == 0x12) {
            intptr_t *methNode = (intptr_t *)(uintptr_t)target[1];

            if (methNode && methNode[0] == 0x14 &&
                !FindVariable(SCRCG->fileId, (unsigned int)methNode[1])) {
                unsigned int name = (unsigned int)methNode[1];
                const char *pName;
                unsigned int sourcePos;
                VariableValue value;
                unsigned int varId;
                int type;
                intptr_t meth;
                char *savedPos = 0;

                if (!name) {
                    goto regular_method;
                }

                pName = SL_ConvertToString(name);
                sourcePos = (unsigned int)node[2];
                varId = FindVariable(scrCompilePub.builtinMeth, name);
                if (varId) {
                    Scr_EvalVariableValue(varId, &value);
                    type = (value.type != SCRCOMP_VAR_CODEPOS);
                    meth = value.u.intValue;
                } else {
                    unsigned int newId;

                    type = 0;
                    meth = Scr_GetMethod(&pName, &type);
                    newId = GetNewVariable(scrCompilePub.builtinMeth, name);
                    value.type = (type == 1) ? SCRCOMP_VAR_DEVELOPER_CODEPOS : SCRCOMP_VAR_CODEPOS;
                    value.u.intValue = meth;
                    SetVariableValue(newId, &value);
                }

                if (!meth) {
                    goto regular_method;
                }

                if (EmitPrepareDeveloperCall(&type, &savedPos) && !bStatement) {
                    CompileError(sourcePos, "return value of developer command can not be accessed if not in a /# ... #/ comment");
                    return 0;
                }

                param_count = EmitCallArgs(params, block, &constValue);
                if (EmitOrEvalPrimitiveExpression(expr, &constValue, block)) {
                    EmitValue(&constValue);
                }
                if (!SCRCG->bConstRefCount) {
                    SL_RemoveRefToString(name);
                }

                EmitBuiltinMethodOpcode(param_count, sourcePos, meth);
                AddOpcodePos(methodSourcePos.sourcePosValue, 0);
                EmitDeveloperParamOpcodePos(params);
                if (bStatement) {
                    EmitOpcode(0x58, -1, 0);
                }
                EmitFinishDeveloperCall(type, savedPos);
                return 0;
            }
        }

    regular_method:
        if (node[0] == 0x1a) {
            EmitOpcode(0x4e, 1, 0);
        }
    }

    param_count = EmitCallArgs(params, block, &constValue);
    if (EmitOrEvalPrimitiveExpression(expr, &constValue, block)) {
        EmitValue(&constValue);
    }
    EmitPostFunctionCall(func_name, param_count, 1, block);
    AddOpcodePos(methodSourcePos.sourcePosValue, 0);
    EmitDeveloperParamOpcodePos(params);
    if (bStatement) {
        EmitOpcode(0x58, -1, 0);
    }
    return 0;
}

#    if 0

static __attribute__((naked))
Bool __attribute_regparm__(3) EmitOrEvalPrimitiveExpression(sval_t expr, VariableCompileValue *constValue, scr_block_t *block)
{
    __asm__ __volatile__ (
        ".Lf962cc_000962cc:\n"
        "pushl %ebp\n"
        "movl %esp, %ebp\n"
        "pushl %edi\n"
        "pushl %esi\n"
        "pushl %ebx\n"
        "subl $0x4c, %esp\n"
        "movl %edx, %edi\n"
        "movl %ecx, %ebx\n"
        "movl (%eax), %edx\n"
        "subl $0x11, %edx\n"
        "cmpl $0x39, %edx\n"
        "ja .Lf962cc_000962ea\n"
        "jmpl *.Ljt_962cc_0(, %edx, 4)\n"
        ".Lf962cc_000962ea:\n"
        "movl %edi, %edx\n"
        "calll EvalPrimitiveExpression\n"
        ".Lf962cc_000962f1:\n"
        "movzbl %al, %eax\n"
        ".Lf962cc_000962f4:\n"
        "addl $0x4c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"
        ".Lf962cc_000962fc:\n"
        "movl 4(%eax), %esi\n"
        "movl %ecx, %edx\n"
        "movl %esi, %eax\n"
        "calll EmitVariableExpression\n"
        "xorl %eax, %eax\n"
        "jmp .Lf962cc_000962f4\n"
        ".Lf962cc_0009630c:\n"
        "movl 8(%eax), %edx\n"
        "movl %edx, -0x48(%ebp)\n"
        "movl 4(%eax), %esi\n"
        "xorl %ecx, %ecx\n"
        "movl $1, %edx\n"
        "movl $0x15, %eax\n"
        "calll EmitOpcode\n"
        "movl $3, 4(%esp)\n"
        "movl -0x48(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll AddOpcodePos\n"
        "movl -0x48(%ebp), %edx\n"
        "movl %esi, %eax\n"
        "calll EmitFunction\n"
        "xorl %eax, %eax\n"
        "jmp .Lf962cc_000962f4\n"
        ".Lf962cc_00096347:\n"
        "movl 4(%eax), %esi\n"
        "movl (%esi), %eax\n"
        "cmpl $0x17, %eax\n"
        "je .Lf962cc_000965e2\n"
        "cmpl $0x18, %eax\n"
        "je .Lf962cc_00096585\n"
        ".Lf962cc_0009635e:\n"
        "xorl %eax, %eax\n"
        "jmp .Lf962cc_000962f4\n"
        ".Lf962cc_00096362:\n"
        "movl 4(%eax), %esi\n"
        "xorl %ecx, %ecx\n"
        "movl $1, %edx\n"
        "movl $0xf, %eax\n"
        ".Lf962cc_00096371:\n"
        "calll EmitOpcode\n"
        "movl $1, 4(%esp)\n"
        "movl %esi, (%esp)\n"
        "calll AddOpcodePos\n"
        "xorl %eax, %eax\n"
        "addl $0x4c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"
        ".Lf962cc_00096390:\n"
        "movl 4(%eax), %esi\n"
        "xorl %ecx, %ecx\n"
        "movl $1, %edx\n"
        "movl $0x10, %eax\n"
        "jmp .Lf962cc_00096371\n"
        ".Lf962cc_000963a1:\n"
        "movl 4(%eax), %esi\n"
        "xorl %ecx, %ecx\n"
        "movl $1, %edx\n"
        "movl $0x11, %eax\n"
        "jmp .Lf962cc_00096371\n"
        ".Lf962cc_000963b2:\n"
        "movl 4(%eax), %esi\n"
        "xorl %ecx, %ecx\n"
        "movl $1, %edx\n"
        "movl $0x12, %eax\n"
        "jmp .Lf962cc_00096371\n"
        ".Lf962cc_000963c3:\n"
        "movl 8(%eax), %edx\n"
        "movl %edx, -0x48(%ebp)\n"
        "movl 4(%eax), %esi\n"

        "movl (%esi), %eax\n"
        "movl %eax, %edx\n"
        "testl %eax, %eax\n"
        "je .Lf962cc_000965bc\n"
        "xorl %ecx, %ecx\n"
        ".Lf962cc_000963da:\n"
        "addl $1, %ecx\n"
        "movl 4(%edx), %edx\n"
        "testl %edx, %edx\n"
        "jne .Lf962cc_000963da\n"

        "cmpl $1, %ecx\n"
        "je .Lf962cc_000965fb\n"
        "cmpl $3, %ecx\n"
        "jne .Lf962cc_000965bc\n"
        "movl %eax, -0x3c(%ebp)\n"
        "testl %eax, %eax\n"
        "je .Lf962cc_00096617\n"
        ".Lf962cc_00096401:\n"
        "movl -0x3c(%ebp), %edx\n"
        "movl (%edx), %eax\n"
        "movl (%eax), %eax\n"
        "movl %ebx, %ecx\n"
        "leal -0x24(%ebp), %edx\n"
        "calll EmitOrEvalExpression\n"
        "movb %al, -0x41(%ebp)\n"
        "testb %al, %al\n"
        "je .Lf962cc_00096455\n"
        "movl scrCompilePub, %eax\n"
        "movl %eax, -0x40(%ebp)\n"
        "cmpl $0x1f, %eax\n"
        "jg .Lf962cc_00096686\n"
        "movl $scrCompileGlob+80, %ecx\n"
        "leal (%eax, %eax, 2), %edx\n"
        "shll $2, %edx\n"
        "movl -0x24(%ebp), %eax\n"
        "movl %eax, 0xc(%edx, %ecx)\n"
        "movl -0x20(%ebp), %eax\n"
        "movl %eax, 0x10(%edx, %ecx)\n"
        "movl -0x1c(%ebp), %eax\n"
        "movl %eax, 0x14(%edx, %ecx)\n"
        "movl -0x40(%ebp), %eax\n"
        "addl $1, %eax\n"
        "movl %eax, scrCompilePub\n"
        ".Lf962cc_00096455:\n"
        "movl -0x3c(%ebp), %eax\n"
        "movl 4(%eax), %eax\n"
        "movl %eax, -0x3c(%ebp)\n"
        "testl %eax, %eax\n"
        "je .Lf962cc_0009660d\n"
        "cmpb $0, -0x41(%ebp)\n"
        "jne .Lf962cc_00096401\n"
        "movl -0x3c(%ebp), %edx\n"
        "movl (%edx), %eax\n"
        "movl (%eax), %eax\n"

        "movl %ebx, %ecx\n"
        "leal -0x30(%ebp), %edx\n"
        "calll EmitOrEvalExpression\n"
        "testb %al, %al\n"
        "je .Lf962cc_00096455\n"
        "leal -0x30(%ebp), %eax\n"
        "calll EmitValue\n"
        "jmp .Lf962cc_00096455\n"

        ".Lf962cc_0009648b:\n"
        "movl 8(%eax), %edx\n"
        "movl %edx, -0x48(%ebp)\n"
        "movl 4(%eax), %esi\n"

        "leal -0x30(%ebp), %edi\n"
        "movl %edi, %edx\n"
        "movl %esi, %eax\n"
        "calll EmitOrEvalPrimitiveExpression\n"
        "testb %al, %al\n"
        "jne .Lf962cc_000965d6\n"

        ".Lf962cc_000964a8:\n"
        "xorl %ecx, %ecx\n"
        "xorl %edx, %edx\n"
        "movl $0x76, %eax\n"
        "calll EmitOpcode\n"
        "movl $0, 4(%esp)\n"
        "movl -0x48(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll AddOpcodePos\n"
        "xorl %eax, %eax\n"
        "jmp .Lf962cc_000962f4\n"
        ".Lf962cc_000964d0:\n"
        "movl 4(%eax), %esi\n"
        "xorl %ecx, %ecx\n"
        "movl $1, %edx\n"
        "movl $0x25, %eax\n"
        "jmp .Lf962cc_00096371\n"
        ".Lf962cc_000964e4:\n"
        "movl 8(%eax), %edx\n"
        "movl %edx, -0x48(%ebp)\n"
        "movl 4(%eax), %esi\n"
        "xorl %ecx, %ecx\n"
        "movl $1, %edx\n"
        "movl $0x13, %eax\n"
        "calll EmitOpcode\n"
        "movl $1, 4(%esp)\n"
        "movl -0x48(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll AddOpcodePos\n"
        "movl $4, (%esp)\n"
        "calll TempMallocAlign\n"
        "movl %eax, scrCompileGlob\n"
        "movl $0xffffffff, (%eax)\n"
        "movl -0x48(%ebp), %edx\n"
        "movl %edx, 8(%esp)\n"
        "movl %esi, 4(%esp)\n"
        "movl scrCompileGlob, %eax\n"
        "movl %eax, (%esp)\n"
        "calll Scr_EmitAnimation\n"
        "cmpb $0, scrCompileGlob+28\n"
        "jne .Lf962cc_0009635e\n"
        "movl %esi, (%esp)\n"
        "calll SL_RemoveRefToString\n"
        "xorl %eax, %eax\n"
        "jmp .Lf962cc_000962f4\n"
        ".Lf962cc_0009655c:\n"
        "movl 4(%eax), %esi\n"
        "movl imp_scrAnimPub, %eax\n"
        "movl 0x414(%eax), %eax\n"
        "testl %eax, %eax\n"
        "jne .Lf962cc_000965ae\n"
        "movl $"#using_animtree was not specified", 4(%esp)\n"
        "movl %esi, (%esp)\n"
        "calll CompileError\n"
        "xorl %eax, %eax\n"
        "jmp .Lf962cc_000962f4\n"
        ".Lf962cc_00096585:\n"
        "movl 0x10(%esi), %eax\n"
        "movl 0xc(%esi), %ecx\n"
        "movl 8(%esi), %edx\n"
        "movl 4(%esi), %esi\n"
        "movl %ebx, 8(%esp)\n"
        "movl $0, 4(%esp)\n"
        "movl %eax, (%esp)\n"
        "movl %esi, %eax\n"
        "calll EmitMethod\n"
        "xorl %eax, %eax\n"
        "jmp .Lf962cc_000962f4\n"
        ".Lf962cc_000965ae:\n"
        "movl %esi, %edx\n"
        "calll EmitGetInteger\n"
        "xorl %eax, %eax\n"
        "jmp .Lf962cc_000962f4\n"

        ".Lf962cc_000965bc:\n"
        "movl $"expression list must have 1 or 3 parameters", 4(%esp)\n"
        "movl -0x48(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll CompileError\n"
        "xorl %eax, %eax\n"
        "jmp .Lf962cc_000962f1\n"

        ".Lf962cc_000965d6:\n"
        "movl %edi, %eax\n"
        "calll EmitValue\n"
        "jmp .Lf962cc_000964a8\n"

        ".Lf962cc_000965e2:\n"
        "movl 8(%esi), %edx\n"
        "movl 4(%esi), %esi\n"
        "movl %ecx, (%esp)\n"
        "xorl %ecx, %ecx\n"
        "movl %esi, %eax\n"
        "calll EmitCall\n"
        "xorl %eax, %eax\n"
        "jmp .Lf962cc_000962f4\n"

        ".Lf962cc_000965fb:\n"
        "movl (%eax), %eax\n"
        "movl (%eax), %eax\n"
        "movl %ebx, %ecx\n"
        "movl %edi, %edx\n"
        "calll EmitOrEvalExpression\n"
        "jmp .Lf962cc_000962f1\n"
        ".Lf962cc_0009660d:\n"
        "cmpb $0, -0x41(%ebp)\n"
        "je .Lf962cc_0009669e\n"
        ".Lf962cc_00096617:\n"
        "movl scrCompilePub, %eax\n"
        "subl $3, %eax\n"
        "movl %eax, scrCompilePub\n"
        "leal (%eax, %eax, 2), %eax\n"
        "leal scrCompileGlob+92(, %eax, 4), %ecx\n"
        "xorl %esi, %esi\n"
        "leal -0x30(%ebp), %ebx\n"

        ".Lf962cc_00096633:\n"
        "movl 4(%ecx), %edx\n"
        "cmpl $5, %edx\n"
        "je .Lf962cc_0009667f\n"
        "cmpl $6, %edx\n"
        "jne .Lf962cc_000966f6\n"
        "cvtsi2ssl (%ecx), %xmm0\n"
        "movss %xmm0, 8(%ebx)\n"
        ".Lf962cc_0009664d:\n"
        "addl $0xc, %esi\n"
        "addl $0xc, %ecx\n"
        "subl $4, %ebx\n"
        "cmpl $0x24, %esi\n"
        "jne .Lf962cc_00096633\n"
        "movl $4, 4(%edi)\n"
        "leal -0x30(%ebp), %edx\n"
        "movl %edx, (%esp)\n"
        "calll Scr_AllocVector\n"
        "movl %eax, (%edi)\n"

        ".Lf962cc_0009666f:\n"
        "movl -0x48(%ebp), %eax\n"
        "movl %eax, 8(%edi)\n"
        "movl $1, %eax\n"
        "jmp .Lf962cc_000962f1\n"

        ".Lf962cc_0009667f:\n"
        "movl (%ecx), %eax\n"
        "movl %eax, 8(%ebx)\n"
        "jmp .Lf962cc_0009664d\n"

        ".Lf962cc_00096686:\n"
        "movl $"VALUE_STACK_SIZE exceeded", 4(%esp)\n"
        "movl -0x1c(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll CompileError\n"
        "jmp .Lf962cc_00096455\n"
        ".Lf962cc_0009669e:\n"
        "xorl %ecx, %ecx\n"
        "movl $0xfffffffe, %edx\n"
        "movl $0x7e, %eax\n"
        "calll EmitOpcode\n"
        "movl $1, 4(%esp)\n"
        "movl -0x48(%ebp), %edx\n"
        "movl %edx, (%esp)\n"
        "calll AddOpcodePos\n"

        "movl imp_scrVarPub, %eax\n"
        "cmpb $0, 0xa(%eax)\n"
        "je .Lf962cc_000966ef\n"
        "movl (%esi), %ebx\n"
        "testl %ebx, %ebx\n"
        "je .Lf962cc_000966ef\n"
        ".Lf962cc_000966d3:\n"
        "movl $0, 4(%esp)\n"
        "movl (%ebx), %eax\n"
        "movl 4(%eax), %eax\n"
        "movl %eax, (%esp)\n"
        "calll AddOpcodePos\n"
        "movl 4(%ebx), %ebx\n"
        "testl %ebx, %ebx\n"
        "jne .Lf962cc_000966d3\n"

        ".Lf962cc_000966ef:\n"
        "xorl %eax, %eax\n"
        "jmp .Lf962cc_000962f1\n"

        ".Lf962cc_000966f6:\n"
        "movl imp_var_typename, %eax\n"
        "movl (%eax, %edx, 4), %eax\n"
        "movl %eax, 8(%esp)\n"
        "movl $"type %s is not a float", 4(%esp)\n"
        "movl 8(%ecx), %eax\n"
        "movl %eax, (%esp)\n"
        "calll CompileError\n"
        "jmp .Lf962cc_0009666f\n"
        ".section .rodata\n"
        ".balign 4\n"
        ".Ljt_962cc_0:\n"
        ".long .Lf962cc_000962fc\n"
        ".long .Lf962cc_0009630c\n"
        ".long .Lf962cc_00096347\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_00096362\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_00096390\n"
        ".long .Lf962cc_000963a1\n"
        ".long .Lf962cc_000963b2\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000963c3\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_0009648b\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000964d0\n"
        ".long .Lf962cc_000964e4\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_000962ea\n"
        ".long .Lf962cc_0009655c\n"
        ".text\n"
    );
}
#    endif

static Bool __attribute_regparm__(3)
    EmitOrEvalPrimitiveExpression(sval_t expr, VariableCompileValue *constValue, scr_block_t *block)
{
    sval_t *node = (sval_t *)(uintptr_t)expr.node;
    int type;

    if (!node) {
        return 0;
    }

    type = node[0].type;
    switch (type) {
    case 17:
        EmitVariableExpression(node[1], block);
        return 0;

    case 18:
        EmitOpcode(0x15, 1, 0);
        AddOpcodePos(node[2].sourcePosValue, 3);
        EmitFunction(node[1].node, node[2].sourcePosValue);
        return 0;

    case 19: {
        sval_t *callNode = (sval_t *)(uintptr_t)node[1].node;

        if (callNode) {
            if (callNode[0].type == 23) {
                EmitCall(callNode[1], callNode[2], 0, block);
                return 0;
            }
            if (callNode[0].type == 24) {
                EmitMethod(callNode[1], callNode[2], callNode[3], callNode[4], 0, block);
                return 0;
            }
        }
        return 0;
    }

    case 32:
        EmitOpcode(0x0f, 1, 0);
        AddOpcodePos(node[1].sourcePosValue, 1);
        return 0;

    case 34:
        EmitOpcode(0x10, 1, 0);
        AddOpcodePos(node[1].sourcePosValue, 1);
        return 0;

    case 35:
        EmitOpcode(0x11, 1, 0);
        AddOpcodePos(node[1].sourcePosValue, 1);
        return 0;

    case 36:
        EmitOpcode(0x12, 1, 0);
        AddOpcodePos(node[1].sourcePosValue, 1);
        return 0;

    case 46: {
        sval_t *listNode = (sval_t *)(uintptr_t)((sval_t *)(uintptr_t)node[1].node)[0].node;
        sval_t *walk;
        int count = 0;

        for (walk = listNode; walk; walk = (sval_t *)(uintptr_t)walk[1].node) {
            ++count;
        }

        if (count == 1) {
            sval_t *child = (sval_t *)(uintptr_t)listNode[0].node;

            if (!child) {
                return 0;
            }

            return EmitOrEvalExpression(child[0], constValue, block);
        }

        if (count != 3) {
            CompileError(node[2].sourcePosValue, "expression list must have 1 or 3 parameters");
            return 0;
        }

        {

            Bool success = 1;
            sval_t *cur;

            for (cur = listNode; cur; cur = (sval_t *)(uintptr_t)cur[1].node) {
                sval_t *child = (sval_t *)(uintptr_t)cur[0].node;
                VariableCompileValue value;

                if (!child) {
                    success = 0;
                    continue;
                }

                if (success) {
                    success = EmitOrEvalExpression(child[0], &value, block);
                    if (success) {
                        if (SCRCP->value_count > 31) {
                            CompileError(value.sourcePos.sourcePosValue, "VALUE_STACK_SIZE exceeded");
                        } else {
                            SCRCG->value_start[SCRCP->value_count] = value;
                            ++SCRCP->value_count;
                        }
                    }
                } else {
                    if (EmitOrEvalExpression(child[0], &value, block)) {
                        EmitValue(&value);
                    }
                }
            }

            if (success) {
                float vec[3];
                int first;
                int i;

                SCRCP->value_count -= 3;
                first = SCRCP->value_count;

                for (i = 0; i < 3; ++i) {
                    VariableCompileValue *value = &SCRCG->value_start[first + i];

                    if (value->value.type == 5) {
                        vec[i] = value->value.u.floatValue;
                    } else if (value->value.type == 6) {
                        vec[i] = (float)value->value.u.intValue;
                    } else {
                        CompileError(value->sourcePos.sourcePosValue,
                                     "type %s is not a float",
                                     ((const char *const *)imp_var_typename)[value->value.type]);
                        vec[i] = 0.0f;
                    }
                }

                constValue->value.type = 4;
                constValue->value.u.vectorValue = SCR_VEC_ENC(Scr_AllocVector(vec));
                constValue->sourcePos = node[2];
                return 1;
            }
        }

        EmitOpcode(0x7e, -2, 0);
        AddOpcodePos(node[2].sourcePosValue, 1);

        if (SCRVP->developer) {
            for (walk = listNode; walk; walk = (sval_t *)(uintptr_t)walk[1].node) {
                sval_t *child = (sval_t *)(uintptr_t)walk[0].node;
                if (child) {
                    AddOpcodePos(child[1].sourcePosValue, 0);
                }
            }
        }
        return 0;
    }

    case 52: {
        VariableCompileValue value;

        if (EmitOrEvalPrimitiveExpression(node[1], &value, block)) {
            EmitValue(&value);
        }
        EmitOpcode(0x76, 0, 0);
        AddOpcodePos(node[2].sourcePosValue, 0);
        return 0;
    }

    case 66:
        EmitOpcode(0x25, 1, 0);
        AddOpcodePos(node[1].sourcePosValue, 1);
        return 0;

    case 67:
        EmitOpcode(0x13, 1, 0);
        AddOpcodePos(node[2].sourcePosValue, 1);
        SCRCG->codePos = (byte *)TempMallocAlign(4);
        *(int *)SCRCG->codePos = -1;
        Scr_EmitAnimation((char *)SCRCG->codePos, node[1].stringValue, node[2].sourcePosValue);
        if (!SCRCG->bConstRefCount) {
            SL_RemoveRefToString(node[1].stringValue);
        }
        return 0;

    case 74:
        if (!SCRAP->animtrees) {
            CompileError(node[1].sourcePosValue, "#using_animtree was not specified");
            return 0;
        }
        EmitGetInteger(SCRAP->animtrees, node[1].sourcePosValue);
        return 0;

    default:
        return EvalPrimitiveExpression(expr, constValue);
    }
}

static Bool __attribute_regparm__(3)
    EmitOrEvalExpression(sval_t expr, VariableCompileValue *constValue, scr_block_t *block)
{
    intptr_t *node = (intptr_t *)(uintptr_t)expr.node;
    sval_t lhs;
    sval_t rhs;
    sval_t opcode;
    sval_t sourcePos;
    VariableCompileValue constValue1;
    VariableCompileValue constValue2;
    char *pos;
    char *nextPos;

    switch (node[0]) {
    case 6:
        lhs.node = node[1];
        return EmitOrEvalPrimitiveExpression(lhs, constValue, block);

    case 47:
        lhs.node = node[1];
        rhs.node = node[2];
        opcode.type = node[3];
        sourcePos.sourcePosValue = (unsigned int)node[4];

        if (EmitOrEvalExpression(lhs, &constValue1, block)) {
            EmitValue(&constValue1);
        }

        EmitOpcode(0x61, -1, 0);
        AddOpcodePos(opcode.sourcePosValue, 0);
        pos = TempMallocAlign(2);
        *(short *)pos = 0;
        nextPos = TempMalloc(0);

        if (EmitOrEvalExpression(rhs, &constValue1, block)) {
            EmitValue(&constValue1);
        }

        EmitOpcode(0x5b, 0, 0);
        AddOpcodePos(sourcePos.sourcePosValue, 0);
        *(short *)pos = (short)(TempMalloc(0) - nextPos);
        return 0;

    case 48:
        lhs.node = node[1];
        rhs.node = node[2];
        opcode.type = node[3];
        sourcePos.sourcePosValue = (unsigned int)node[4];

        if (EmitOrEvalExpression(lhs, &constValue1, block)) {
            EmitValue(&constValue1);
        }

        EmitOpcode(0x60, -1, 0);
        AddOpcodePos(opcode.sourcePosValue, 0);
        pos = TempMallocAlign(2);
        *(short *)pos = 0;
        nextPos = TempMalloc(0);

        if (EmitOrEvalExpression(rhs, &constValue1, block)) {
            EmitValue(&constValue1);
        }

        EmitOpcode(0x5b, 0, 0);
        AddOpcodePos(sourcePos.sourcePosValue, 0);
        *(short *)pos = (short)(TempMalloc(0) - nextPos);
        return 0;

    case 49:
        lhs.node = node[1];
        rhs.node = node[2];
        opcode.type = node[3];
        sourcePos.sourcePosValue = (unsigned int)node[4];

        if (EmitOrEvalExpression(lhs, &constValue1, block)) {
            if (SCRCP->value_count > 31) {
                CompileError(constValue1.sourcePos.sourcePosValue, "VALUE_STACK_SIZE exceeded");
            }

            SCRCG->value_start[SCRCP->value_count] = constValue1;
            ++SCRCP->value_count;

            if (EmitOrEvalExpression(rhs, &constValue2, block)) {
                --SCRCP->value_count;
                Scr_EvalBinaryOperator(opcode.type, &constValue1.value, &constValue2.value);
                if (SCRVP->error_message) {
                    CompileError(sourcePos.sourcePosValue, "%s", SCRVP->error_message);
                    return 0;
                }

                constValue->value = constValue1.value;
                constValue->sourcePos = sourcePos;
                return 1;
            }
        } else if (EmitOrEvalExpression(rhs, &constValue2, block)) {
            EmitValue(&constValue2);
        }

        EmitOpcode((signed char)opcode.type, -1, 0);
        AddOpcodePos(sourcePos.sourcePosValue, 0);
        return 0;

    case 50:
        lhs.node = node[1];
        sourcePos.sourcePosValue = (unsigned int)node[2];

        if (EmitOrEvalExpression(lhs, constValue, block)) {
            EmitValue(constValue);
        }

        EmitOpcode(0x5c, 0, 0);
        AddOpcodePos(sourcePos.sourcePosValue, 0);
        return 0;

    case 51:
        lhs.node = node[1];
        sourcePos.sourcePosValue = (unsigned int)node[2];

        if (EmitOrEvalExpression(lhs, constValue, block)) {
            EmitValue(constValue);
        }

        EmitOpcode(0x5d, 0, 0);
        AddOpcodePos(sourcePos.sourcePosValue, 0);
        return 0;

    default:
        return 0;
    }
}

static unsigned int __attribute_regparm__(3)
    EmitPostFunctionCall(sval_t func_name, int param_count, Bool bMethod, scr_block_t *block)
{
    intptr_t *node = (intptr_t *)(uintptr_t)func_name.node;

    if (node[0] == 0x1a) {
        unsigned int sourcePos = (unsigned int)node[2];
        intptr_t *target = (intptr_t *)(uintptr_t)node[1];

        if (target[0] == 0x12) {
            uintptr_t func = (uintptr_t)target[1];

            if (bMethod)
                EmitOpcode(0x52, ~param_count, 3);
            else
                EmitOpcode(0x50, -param_count, 3);

            AddOpcodePos(sourcePos, 3);
            EmitFunction(func, sourcePos);
            return 0;
        }

        if (target[0] == 0x16) {
            sval_t expr;
            unsigned int exprSourcePos;
            VariableCompileValue constValue;

            expr.node = target[1];
            exprSourcePos = (unsigned int)target[2];
            if (EmitOrEvalExpression(expr, &constValue, block))
                EmitValue(&constValue);

            if (bMethod)
                EmitOpcode(0x53, -param_count - 2, 3);
            else
                EmitOpcode(0x51, ~param_count, 3);

            AddOpcodePos(exprSourcePos, 0);
            AddOpcodePos(sourcePos, 1);
            return 0;
        }

        return 0;
    }

    if (node[0] == 0x1e) {
        unsigned int callSourcePos = (unsigned int)node[2];
        unsigned int funcSourcePos = (unsigned int)node[3];
        intptr_t *target = (intptr_t *)(uintptr_t)node[1];

        if (target[0] == 0x12) {
            uintptr_t func = (uintptr_t)target[1];
            char *out;

            if (bMethod)
                EmitOpcode(0x56, -param_count, 2);
            else
                EmitOpcode(0x54, 1 - param_count, 2);

            AddOpcodePos(funcSourcePos, 3);
            EmitFunction(func, funcSourcePos);
            out = TempMallocAlign(4);
            SCRCG->codePos = (byte *)out;
            *(int *)out = param_count;
            AddOpcodePos(callSourcePos, 0);
            return 0;
        }

        if (target[0] == 0x16) {
            sval_t expr;
            unsigned int exprSourcePos;
            VariableCompileValue constValue;
            char *out;

            expr.node = target[1];
            exprSourcePos = (unsigned int)target[2];
            if (EmitOrEvalExpression(expr, &constValue, block))
                EmitValue(&constValue);

            if (bMethod)
                EmitOpcode(0x57, ~param_count, 2);
            else
                EmitOpcode(0x55, -param_count, 2);

            AddOpcodePos(exprSourcePos, 1);
            out = TempMallocAlign(4);
            SCRCG->codePos = (byte *)out;
            *(int *)out = param_count;
            AddOpcodePos(callSourcePos, 0);
            return 0;
        }

        AddOpcodePos(callSourcePos, 0);
    }

    return 0;
}

static unsigned int __attribute_regparm__(3)
    EmitPrimitiveExpressionFieldObject(sval_t expr, unsigned int sourcePos, scr_block_t *block)
{
    intptr_t *node = (intptr_t *)(uintptr_t)expr.node;

    switch (node[0]) {
    case 17:
        EmitVariableExpression(((sval_t *)node)[1], block);
        EmitOpcode(0x59, -1, 0);
        AddOpcodePos((unsigned int)node[2], 0);
        return 0;

    case 19: {
        intptr_t *callNode = (intptr_t *)(uintptr_t)node[1];

        if (callNode[0] == 0x17) {
            EmitCall(((sval_t *)callNode)[1], ((sval_t *)callNode)[2], 0, block);
            EmitOpcode(0x59, -1, 0);
            AddOpcodePos((unsigned int)callNode[3], 0);
            return 0;
        }

        if (callNode[0] == 0x18) {
            EmitMethod(((sval_t *)callNode)[1], ((sval_t *)callNode)[2],
                       ((sval_t *)callNode)[3], ((sval_t *)callNode)[4],
                       0, block);
            EmitOpcode(0x59, -1, 0);
            AddOpcodePos((unsigned int)callNode[5], 0);
            return 0;
        }
        return 0;
    }

    case 32:
        EmitOpcode(0x26, 0, 0);
        AddOpcodePos((unsigned int)node[1], 1);
        return 0;

    case 34:
        EmitOpcode(0x0d, 0, 0);
        AddOpcodePos((unsigned int)node[1], 1);
        return 0;

    case 36:
        EmitOpcode(0x0e, 0, 0);
        AddOpcodePos((unsigned int)node[1], 1);
        return 0;

    case 46: {
        intptr_t *list = (intptr_t *)(uintptr_t)node[1];
        intptr_t *first = (intptr_t *)(uintptr_t)list[0];
        intptr_t *inner;
        intptr_t *innerExpr;
        sval_t nextExpr;

        if (!first)
            break;

        if (first[1])
            break;

        inner = (intptr_t *)(uintptr_t)first[0];
        sourcePos = (unsigned int)inner[1];
        innerExpr = (intptr_t *)(uintptr_t)inner[0];
        if (innerExpr[0] != 6)
            break;

        nextExpr.node = innerExpr[1];
        sourcePos = (unsigned int)innerExpr[2];
        return EmitPrimitiveExpressionFieldObject(nextExpr, sourcePos, block);
    }
    }

    CompileError(sourcePos, "not an object");
    return 0;
}

static unsigned int __attribute_regparm__(2)
    EmitVariableExpression(sval_t expr, scr_block_t *block)
{
    intptr_t *node = (intptr_t *)(uintptr_t)expr.node;

    switch (node[0]) {
    case 4: {
        sval_t sourcePos;
        int localIndex;
        unsigned int opcode;

        sourcePos.node = node[2];
        localIndex = Scr_FindLocalVarIndex((unsigned int)node[1], sourcePos, 0, block);
        opcode = (localIndex <= 5) ? (unsigned int)(localIndex + 0x18) : 0x1e;
        EmitOpcode(opcode, 1, 0);
        if (opcode == 0x1e) {
            char *out = TempMalloc(1);
            SCRCG->codePos = (byte *)out;
            *out = (char)localIndex;
        }
        AddOpcodePos((unsigned int)sourcePos.node, 1);
        return 0;
    }

    case 13: {
        sval_t primitiveExpr;
        sval_t valueExpr;
        sval_t exprSourcePos;
        unsigned int fieldSourcePos;
        VariableCompileValue constValue;

        primitiveExpr.node = node[1];
        valueExpr.node = node[2];
        fieldSourcePos = (unsigned int)node[3];
        exprSourcePos.node = node[4];

        if (EmitOrEvalExpression(valueExpr, &constValue, block)) {
            EmitValue(&constValue);
        }
        if (EmitOrEvalPrimitiveExpression(primitiveExpr, &constValue, block)) {
            EmitValue(&constValue);
        }
        EmitOpcode(0x20, -1, 0);
        AddOpcodePos((unsigned int)exprSourcePos.node, 0);
        AddOpcodePos(fieldSourcePos, 1);
        return 0;
    }

    case 15: {
        sval_t primitiveExpr;
        unsigned int fieldName;
        unsigned int sourcePos;
        unsigned short *out;

        primitiveExpr.node = node[1];
        fieldName = (unsigned int)node[2];
        sourcePos = (unsigned int)node[3];

        EmitPrimitiveExpressionFieldObject(primitiveExpr, sourcePos, block);
        EmitOpcode(0x2a, 1, 0);
        AddOpcodePos(sourcePos, 0);
        out = (unsigned short *)TempMallocAlign(2);
        SCRCG->codePos = (byte *)out;

        if (SCRCP->developer_statement == SCRCOMP_SCR_DEV_IGNORE) {
            if (!SCRCG->bConstRefCount) {
                SL_RemoveRefToString(fieldName);
            }
        } else {
            if (SCRCG->bConstRefCount) {
                SL_AddRefToString(fieldName);
            }
            *out = (unsigned short)SL_TransferToCanonicalString(fieldName);
        }
        return 0;
    }

    case 0x4f: {
        unsigned int stringValue = (unsigned int)node[1];
        unsigned int sourcePos = (unsigned int)node[2];
        const char *text;
        int classnum;
        int entnum;
        unsigned int type;

        if (SCRCP->script_loading) {
            CompileError(sourcePos, "$ can only be used in the script debugger");
            return 0;
        }

        text = SL_ConvertToString(stringValue);
        if (text[0] == 't') {
            entnum = atoi(text + 1);
            if ((unsigned int)(entnum - 1) > 0xfffc || IsVarFree((unsigned int)(unsigned short)entnum)) {
                CompileError(sourcePos, "bad expression");
                return 0;
            }

            type = (unsigned int)GetVarType((unsigned int)(unsigned short)entnum);
            if (type > 0x17 || !((1u << type) & 0x00878000u)) {
                CompileError(sourcePos, "bad expression");
                return 0;
            }

            EmitOpcode(0x82, 1, 0);
            *(unsigned short *)TempMallocAlign(2) = (unsigned short)entnum;
            return 0;
        }

        classnum = Scr_GetClassnumForCharId((signed char)text[0]);
        if (classnum < 0) {
            CompileError(sourcePos, "bad expression");
            return 0;
        }

        entnum = atoi(text + 1);
        if (!entnum && text[1] != '0') {
            CompileError(sourcePos, "bad expression");
            return 0;
        }

        EmitOpcode(0x81, 1, 0);
        *(int *)TempMallocAlign(4) = classnum;
        *(int *)TempMallocAlign(4) = entnum;
        return 0;
    }

    case 0x35:
        CompileError((unsigned int)node[2], SCRCP->script_loading ? "self field can only be used in the script debugger" : "self field in assignment expression not currently supported");
        return 0;

    default:
        return 0;
    }
}

#    if 0

static __attribute__((naked)) __attribute_regparm__(2)
unsigned int EmitVariableExpression(sval_t expr, scr_block_t *block)
{
    __asm__ __volatile__ (
        "pushl %ebp\n"
        "movl %esp, %ebp\n"
        "pushl %edi\n"
        "pushl %esi\n"
        "pushl %ebx\n"
        "subl $0x3c, %esp\n"
        "movl %eax, %ecx\n"
        "movl %edx, %esi\n"
        "movl (%eax), %eax\n"
        "cmpl $0xf, %eax\n"
        "je .Lf97358_0009740b\n"
        "jle .Lf97358_0009738c\n"
        "cmpl $0x35, %eax\n"
        "je .Lf97358_00097511\n"
        "cmpl $0x4f, %eax\n"
        "je .Lf97358_0009748a\n"
        ".Lf97358_00097384:\n"
        "addl $0x3c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"
        ".Lf97358_0009738c:\n"
        "cmpl $4, %eax\n"
        "je .Lf97358_0009753c\n"
        "cmpl $0xd, %eax\n"
        "jne .Lf97358_00097384\n"
        "movl 0x10(%ecx), %edx\n"
        "movl %edx, -0x2c(%ebp)\n"
        "movl 0xc(%ecx), %edi\n"
        "movl 8(%ecx), %eax\n"
        "movl %eax, -0x30(%ebp)\n"
        "movl 4(%ecx), %ebx\n"

        "movl %esi, %ecx\n"
        "leal -0x24(%ebp), %edx\n"
        "calll EmitOrEvalExpression\n"
        "testb %al, %al\n"
        "jne .Lf97358_000975d1\n"

        ".Lf97358_000973be:\n"
        "movl %esi, %ecx\n"
        "leal -0x24(%ebp), %edx\n"
        "movl %ebx, %eax\n"
        "calll EmitOrEvalPrimitiveExpression\n"
        "testb %al, %al\n"
        "jne .Lf97358_000975c4\n"

        ".Lf97358_000973d2:\n"
        "xorl %ecx, %ecx\n"
        "movl $0xffffffff, %edx\n"
        "movl $0x20, %eax\n"
        "calll EmitOpcode\n"
        "movl $0, 4(%esp)\n"
        "movl -0x2c(%ebp), %edx\n"
        "movl %edx, (%esp)\n"
        "calll AddOpcodePos\n"
        "movl $1, 4(%esp)\n"
        "movl %edi, (%esp)\n"
        "calll AddOpcodePos\n"
        "jmp .Lf97358_00097384\n"
        ".Lf97358_0009740b:\n"
        "movl 0xc(%ecx), %edi\n"
        "movl 8(%ecx), %eax\n"
        "movl %eax, -0x30(%ebp)\n"
        "movl 4(%ecx), %ebx\n"
        "movl %edx, %ecx\n"
        "movl %edi, %edx\n"
        "movl %ebx, %eax\n"
        "calll EmitPrimitiveExpressionFieldObject\n"
        "xorl %ecx, %ecx\n"
        "movl $1, %edx\n"
        "movl $0x2a, %eax\n"
        "calll EmitOpcode\n"
        "movl $0, 4(%esp)\n"
        "movl %edi, (%esp)\n"
        "calll AddOpcodePos\n"
        "movl $2, (%esp)\n"
        "calll TempMallocAlign\n"
        "movl %eax, scrCompileGlob\n"
        "cmpl $2, scrCompilePub+40\n"
        "je .Lf97358_0009758d\n"
        "cmpb $0, scrCompileGlob+28\n"
        "jne .Lf97358_00097501\n"
        ".Lf97358_0009746e:\n"
        "movl scrCompileGlob, %ebx\n"
        "movl -0x30(%ebp), %edx\n"
        "movl %edx, (%esp)\n"
        "calll SL_TransferToCanonicalString\n"
        "movw %ax, (%ebx)\n"
        "addl $0x3c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"
        ".Lf97358_0009748a:\n"
        "movl 8(%ecx), %eax\n"
        "movl %eax, -0x30(%ebp)\n"
        "movl 4(%ecx), %ebx\n"

        "cmpb $0, scrCompilePub+36\n"
        "jne .Lf97358_000975af\n"
        "movl %ebx, (%esp)\n"
        "calll SL_ConvertToString\n"
        "movl %eax, %ebx\n"
        "movzbl (%eax), %eax\n"
        "cmpb $0x74, %al\n"
        "je .Lf97358_00097634\n"
        "movsbl %al, %eax\n"
        "movl %eax, (%esp)\n"
        "calll Scr_GetClassnumForCharId\n"
        "movl %eax, %esi\n"
        "testl %eax, %eax\n"
        "js .Lf97358_000976b1\n"
        "leal 1(%ebx), %eax\n"
        "movl %eax, (%esp)\n"
        "calll atoi\n"
        "movl %eax, %edi\n"
        "testl %eax, %eax\n"
        "jne .Lf97358_000975f8\n"
        "cmpb $0x30, 1(%ebx)\n"
        "je .Lf97358_000975f8\n"
        ".Lf97358_000974e9:\n"
        "movl $"bad expression", 4(%esp)\n"
        "movl -0x30(%ebp), %edx\n"
        "movl %edx, (%esp)\n"
        "calll CompileError\n"
        "jmp .Lf97358_00097384\n"

        ".Lf97358_00097501:\n"
        "movl -0x30(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll SL_AddRefToString\n"
        "jmp .Lf97358_0009746e\n"
        ".Lf97358_00097511:\n"
        "movl $"self field in assignment expression not currently supported", %eax\n"
        "cmpb $0, scrCompilePub+36\n"
        "movl $"self field can only be used in the script debugger", %edx\n"
        "cmovnel %edx, %eax\n"
        "movl %eax, 4(%esp)\n"
        "movl 8(%ecx), %eax\n"
        "movl %eax, (%esp)\n"
        "calll CompileError\n"
        "addl $0x3c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"
        ".Lf97358_0009753c:\n"
        "movl 8(%ecx), %eax\n"
        "movl %eax, -0x30(%ebp)\n"
        "movl 4(%ecx), %ebx\n"

        "movl %edx, (%esp)\n"
        "xorl %ecx, %ecx\n"
        "movl %eax, %edx\n"
        "movl %ebx, %eax\n"
        "calll Scr_FindLocalVarIndex\n"
        "movl %eax, %esi\n"
        "cmpl $5, %eax\n"
        "jle .Lf97358_000975aa\n"
        "movl $0x1e, %ebx\n"
        ".Lf97358_0009755f:\n"
        "xorl %ecx, %ecx\n"
        "movl $1, %edx\n"
        "movl %ebx, %eax\n"
        "calll EmitOpcode\n"
        "cmpl $0x1e, %ebx\n"
        "je .Lf97358_000975de\n"
        ".Lf97358_00097572:\n"
        "movl $1, 4(%esp)\n"
        "movl -0x30(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll AddOpcodePos\n"

        "addl $0x3c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"
        ".Lf97358_0009758d:\n"
        "cmpb $0, scrCompileGlob+28\n"
        "jne .Lf97358_00097384\n"
        "movl -0x30(%ebp), %edx\n"
        "movl %edx, (%esp)\n"
        "calll SL_RemoveRefToString\n"
        "jmp .Lf97358_00097384\n"

        ".Lf97358_000975aa:\n"
        "leal 0x18(%eax), %ebx\n"
        "jmp .Lf97358_0009755f\n"

        ".Lf97358_000975af:\n"
        "movl $"$ can only be used in the script debugger", 4(%esp)\n"
        "movl %eax, (%esp)\n"
        "calll CompileError\n"
        "jmp .Lf97358_00097384\n"

        ".Lf97358_000975c4:\n"
        "leal -0x24(%ebp), %eax\n"
        "calll EmitValue\n"
        "jmp .Lf97358_000973d2\n"

        ".Lf97358_000975d1:\n"
        "leal -0x24(%ebp), %eax\n"
        "calll EmitValue\n"
        "jmp .Lf97358_000973be\n"

        ".Lf97358_000975de:\n"
        "movl $1, (%esp)\n"
        "calll TempMalloc\n"
        "movl %eax, scrCompileGlob\n"
        "movl %esi, %edx\n"
        "movb %dl, (%eax)\n"
        "jmp .Lf97358_00097572\n"

        ".Lf97358_000975f8:\n"
        "xorl %ecx, %ecx\n"
        "movl $1, %edx\n"
        "movl $0x81, %eax\n"
        "calll EmitOpcode\n"

        "movl $4, (%esp)\n"
        "calll TempMallocAlign\n"
        "movl %eax, scrCompileGlob\n"
        "movl %esi, (%eax)\n"

        "movl $4, (%esp)\n"
        "calll TempMallocAlign\n"
        "movl %eax, scrCompileGlob\n"
        "movl %edi, (%eax)\n"
        "jmp .Lf97358_00097384\n"

        ".Lf97358_00097634:\n"
        "leal 1(%ebx), %eax\n"
        "movl %eax, (%esp)\n"
        "calll atoi\n"
        "movl %eax, %ebx\n"
        "leal -1(%eax), %eax\n"
        "cmpl $0xfffc, %eax\n"
        "ja .Lf97358_000974e9\n"
        "movzwl %bx, %esi\n"
        "movl %esi, (%esp)\n"
        "calll IsVarFree\n"
        "testb %al, %al\n"
        "jne .Lf97358_000974e9\n"
        "movl %esi, (%esp)\n"
        "calll GetVarType\n"
        "movl %eax, %ecx\n"
        "cmpl $0x17, %eax\n"
        "ja .Lf97358_000974e9\n"
        "movl $1, %eax\n"
        "shll %cl, %eax\n"
        "testl $0x00878000, %eax\n"
        "je .Lf97358_000974e9\n"
        "xorl %ecx, %ecx\n"
        "movl $1, %edx\n"
        "movl $0x82, %eax\n"
        "calll EmitOpcode\n"
        "movl $2, (%esp)\n"
        "calll TempMallocAlign\n"
        "movl %eax, scrCompileGlob\n"
        "movw %bx, (%eax)\n"
        "jmp .Lf97358_00097384\n"
        ".Lf97358_000976b1:\n"
        "movl $"bad expression", 4(%esp)\n"
        "movl -0x30(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll CompileError\n"
        "jmp .Lf97358_00097384\n"
    );
}

#    endif

static unsigned int __attribute_regparm__(2)
    EmitVariableExpressionRef(sval_t expr, scr_block_t *block)
{
    intptr_t *node = (intptr_t *)(uintptr_t)expr.node;

    switch (node[0]) {
    case 4: {
        sval_t name;
        sval_t sourcePos;
        int localIndex;

        name.node = node[1];
        sourcePos.node = node[2];
        localIndex = Scr_FindLocalVarIndex((unsigned int)name.node, sourcePos, 1, block);
        EmitOpcode(localIndex < 1 ? 0x36 : 0x37, 0, 0);
        if (localIndex) {
            char *op = TempMalloc(1);
            SCRCG->codePos = (byte *)op;
            *op = (char)localIndex;
        }
        AddOpcodePos((unsigned int)sourcePos.node, 1);
        return 0;
    }

    case 13: {
        sval_t index;
        sval_t valueExpr;
        unsigned int assignSourcePos;
        unsigned int fieldSourcePos;
        VariableCompileValue constValue;
        intptr_t *indexNode;

        index.node = node[1];
        valueExpr.node = node[2];
        fieldSourcePos = (unsigned int)node[3];
        assignSourcePos = (unsigned int)node[4];

        if (EmitOrEvalExpression(valueExpr, &constValue, block))
            EmitValue(&constValue);

        indexNode = (intptr_t *)(uintptr_t)index.node;
        if (indexNode[0] == 0x11) {
            sval_t nested;
            nested.node = indexNode[1];
            EmitVariableExpressionRef(nested, block);
        } else if (indexNode[0] == 0x23) {
            EmitOpcode(0x14, 0, 0);
            AddOpcodePos((unsigned int)indexNode[1], 1);
        } else {
            CompileError(fieldSourcePos, "not an lvalue");
        }

        EmitOpcode(0x23, -1, 0);
        AddOpcodePos(assignSourcePos, 0);
        AddOpcodePos(fieldSourcePos, 1);
        return 0;
    }

    case 15: {
        sval_t index;
        unsigned int sourcePos;
        unsigned int fieldName;
        char *out;

        index.node = node[1];
        fieldName = (unsigned int)node[2];
        sourcePos = (unsigned int)node[3];

        EmitPrimitiveExpressionFieldObject(index, sourcePos, block);
        EmitOpcode(0x2e, 0, 0);
        out = TempMallocAlign(2);
        SCRCG->codePos = (byte *)out;

        if (scrCompilePub.developer_statement == SCRCOMP_SCR_DEV_IGNORE) {
            if (!SCRCG->bConstRefCount)
                SL_RemoveRefToString(fieldName);
            return 0;
        }

        if (SCRCG->bConstRefCount)
            SL_AddRefToString(fieldName);

        *(unsigned short *)out = (unsigned short)SL_TransferToCanonicalString(fieldName);
        return 0;
    }

    case 0x35:
    case 0x4f:
        CompileError((unsigned int)node[2],
                     scrCompilePub.script_loading ? "$ and self field can only be used in the script debugger" : "not an lvalue");
        return 0;
    }

    return 0;
}

static unsigned int __attribute_regparm__(3)
    EmitStatement(sval_t val, int lastStatement, unsigned int endSourcePos, scr_block_t *block)
{
    sval_t *node = (sval_t *)(uintptr_t)val.node;
    VariableCompileValue constValue;

    if (!node || node[0].type > 0x4d)
        return 0;

    switch (node[0].type) {
    case 0x02: {
        sval_t lhs = node[1];
        sval_t rhs = node[2];
        unsigned int sourcePos = node[3].sourcePosValue;
        unsigned int assignPos = node[4].sourcePosValue;
        sval_t *rhsNode = (sval_t *)(uintptr_t)rhs.node;
        sval_t *lhsNode = (sval_t *)(uintptr_t)lhs.node;

        if (rhsNode && rhsNode[0].type == 6) {
            sval_t *rhsInner = (sval_t *)(uintptr_t)rhsNode[1].node;

            if (rhsInner && rhsInner[0].type == 0x1f && lhsNode) {
                if (lhsNode[0].type == 0x0d) {
                    sval_t index = lhsNode[1];
                    sval_t value = lhsNode[2];
                    unsigned int fieldSourcePos = lhsNode[3].sourcePosValue;
                    unsigned int valueSourcePos = lhsNode[4].sourcePosValue;
                    sval_t *indexNode;

                    if (EmitOrEvalExpression(value, &constValue, block))
                        EmitValue(&constValue);

                    indexNode = (sval_t *)(uintptr_t)index.node;
                    if (indexNode && indexNode[0].type == 0x11) {
                        EmitVariableExpressionRef(indexNode[1], block);
                    } else if (indexNode && indexNode[0].type == 0x23) {
                        EmitOpcode(0x14, 0, 0);
                        AddOpcodePos(indexNode[1].sourcePosValue, 1);
                    } else {
                        CompileError(fieldSourcePos, "not an lvalue");
                    }

                    EmitOpcode(0x24, -1, 0);
                    AddOpcodePos(valueSourcePos, 0);
                    AddOpcodePos(fieldSourcePos, 0);
                    return 0;
                }

                if (lhsNode[0].type == 0x0f) {
                    sval_t fieldObject = lhsNode[1];
                    unsigned int fieldName = lhsNode[2].stringValue;
                    unsigned int fieldSourcePos = lhsNode[3].sourcePosValue;
                    unsigned short *out;

                    EmitPrimitiveExpressionFieldObject(fieldObject, fieldSourcePos, block);
                    EmitOpcode(0x2f, 0, 0);
                    AddOpcodePos(assignPos, 0);
                    out = (unsigned short *)TempMallocAlign(2);
                    SCRCG->codePos = (byte *)out;

                    if (SCRCP->developer_statement == SCRCOMP_SCR_DEV_IGNORE) {
                        if (!SCRCG->bConstRefCount)
                            SL_RemoveRefToString(fieldName);
                    } else {
                        if (SCRCG->bConstRefCount)
                            SL_AddRefToString(fieldName);
                        *out = (unsigned short)SL_TransferToCanonicalString(fieldName);
                    }
                    return 0;
                }

                if (lhsNode[0].type == 0x35 || lhsNode[0].type == 0x4f) {
                    CompileError(lhsNode[2].sourcePosValue,
                                 SCRCP->script_loading ? "$ and self field can only be used in the script debugger" : "not an lvalue");
                    return 0;
                }
            }
        }

        if (EmitOrEvalExpression(rhs, &constValue, block))
            EmitValue(&constValue);
        EmitVariableExpressionRef(lhs, block);
        EmitOpcode(0x39, -1, 0);
        AddOpcodePos(sourcePos, 0);
        return 0;
    }

    case 0x19: {
        sval_t *call = (sval_t *)(uintptr_t)node[1].node;
        if (!call)
            return 0;
        if (call[0].type == 0x17) {
            EmitCall(call[1], call[2], 1, block);
        } else if (call[0].type == 0x18) {
            EmitMethod(call[1], call[2], call[3], call[4], 1, block);
        }
        return 0;
    }

    case 0x1b:
        if (!block->abortLevel)
            block->abortLevel = 3;
        if (EmitOrEvalExpression(node[1], &constValue, block))
            EmitValue(&constValue);
        EmitOpcode(1, -1, 0);
        AddOpcodePos(node[2].sourcePosValue, 0);
        return 0;

    case 0x1c:
        if (!block->abortLevel)
            block->abortLevel = 3;
        EmitOpcode(0, 0, 0);
        AddOpcodePos(node[1].sourcePosValue, 1);
        return 0;

    case 0x1d:
        if (EmitOrEvalExpression(node[1], &constValue, block))
            EmitValue(&constValue);
        EmitOpcode(0x4c, -1, 0);
        AddOpcodePos(node[3].sourcePosValue, 0);
        AddOpcodePos(node[3].sourcePosValue, 0);
        AddOpcodePos(node[2].sourcePosValue, 0);
        return 0;

    case 0x25:
        return EmitIfStatement(node[1], node[2], node[3], lastStatement, endSourcePos, block, &node[4]);

    case 0x26:
        return EmitIfElseStatement(node[1], node[2], node[3], node[4], node[5],
                                   lastStatement, endSourcePos, block, &node[6], &node[7]);

    case 0x27:
        return EmitWhileStatement(node[1], node[2], node[3], node[4], block, &node[5]);

    case 0x28:
        return EmitForStatement(node[1], node[2], node[3], node[4], node[5], node[6],
                                block, &node[7], &node[8]);

    case 0x29:
        SCRCG->forceNotCreate = 1;
        EmitVariableExpressionRef(node[1], block);
        SCRCG->forceNotCreate = 0;
        EmitOpcode(0x64, 1, 0);
        AddOpcodePos(node[2].sourcePosValue, 0);
        EmitOpcode(0x39, -1, 0);
        AddOpcodePos(node[2].sourcePosValue, 0);
        return 0;

    case 0x2a:
        SCRCG->forceNotCreate = 1;
        EmitVariableExpressionRef(node[1], block);
        SCRCG->forceNotCreate = 0;
        EmitOpcode(0x65, 1, 0);
        AddOpcodePos(node[2].sourcePosValue, 0);
        EmitOpcode(0x39, -1, 0);
        AddOpcodePos(node[2].sourcePosValue, 0);
        return 0;

    case 0x2b: {
        Bool oldConstRefCount;
        unsigned int opcode = (unsigned int)(signed char)node[3].type;

        oldConstRefCount = SCRCG->bConstRefCount;
        SCRCG->bConstRefCount = 1;
        EmitVariableExpression(node[1], block);
        SCRCG->bConstRefCount = oldConstRefCount;

        if (EmitOrEvalExpression(node[2], &constValue, block))
            EmitValue(&constValue);
        EmitOpcode(opcode, -1, 0);
        AddOpcodePos(node[4].sourcePosValue, 0);
        EmitVariableExpressionRef(node[1], block);
        EmitOpcode(0x39, -1, 0);
        AddOpcodePos(node[4].sourcePosValue, 0);
        return 0;
    }

    case 0x2c: {
        intptr_t *listNode = ScriptStatementListFirstExecNode(node[1]);

        while (listNode) {
            intptr_t *nextNode = (intptr_t *)(uintptr_t)listNode[1];
            int emitLastStatement = 0;
            sval_t stmt;

            if (lastStatement) {
                if (!nextNode) {
                    emitLastStatement = 1;
                } else if (!SCRVP->developer_script) {
                    intptr_t *scan = nextNode;
                    emitLastStatement = 1;
                    while (scan) {
                        sval_t *scanStmt = (sval_t *)(uintptr_t)scan[0];
                        if (!scanStmt || scanStmt[0].type != 0x2d) {
                            emitLastStatement = 0;
                            break;
                        }
                        scan = (intptr_t *)(uintptr_t)scan[1];
                    }
                }
            }

            stmt.node = listNode[0];
            EmitStatement(stmt, emitLastStatement, endSourcePos, block);
            listNode = nextNode;
        }
        return 0;
    }

    case 0x2d:
        return EmitDeveloperStatementList(node[1], node[2].sourcePosValue, block, &node[3]);

    case 0x37: {

        intptr_t *firstNode = ScriptStatementListFirstExecNode(node[2]);

        if (firstNode) {
            sval_t *firstExprNode = (sval_t *)(uintptr_t)firstNode[0];
            intptr_t *listNode;

            if (EmitOrEvalExpression(firstExprNode[0], &constValue, block))
                EmitValue(&constValue);
            if (EmitOrEvalPrimitiveExpression(node[1], &constValue, block))
                EmitValue(&constValue);
            EmitOpcode(0x78, -2, 0);
            AddOpcodePos(node[4].sourcePosValue, 0);
            AddOpcodePos(node[4].sourcePosValue, 0);
            AddOpcodePos(node[3].sourcePosValue, 0);
            AddOpcodePos(firstExprNode[1].sourcePosValue, 0);
            for (listNode = (intptr_t *)(uintptr_t)firstNode[1]; listNode; listNode = (intptr_t *)(uintptr_t)listNode[1]) {
                sval_t *vnode = (sval_t *)(uintptr_t)listNode[0];
                int idx = Scr_FindLocalVarIndex(vnode[0].stringValue, vnode[1], 1, block);
                EmitOpcode(0x33, 0, 0);
                *(char *)TempMalloc(1) = (char)idx;
                AddOpcodePos(vnode[1].sourcePosValue, 0);
            }
        }
        EmitOpcode(0x34, 0, 0);
        return 0;
    }

    case 0x38: {
        intptr_t *firstNode = ScriptStatementListFirstExecNode(node[2]);
        intptr_t *listNode = firstNode ? (intptr_t *)(uintptr_t)firstNode[1] : NULL;
        int exprCount = 0;

        EmitOpcode(0x7b, 1, 0);
        AddOpcodePos(node[3].sourcePosValue, 1);

        for (; listNode; listNode = (intptr_t *)(uintptr_t)listNode[1]) {
            sval_t *exprNode = (sval_t *)(uintptr_t)listNode[0];
            if (exprNode && EmitOrEvalExpression(exprNode[0], &constValue, block))
                EmitValue(&constValue);
            ++exprCount;
        }

        if (firstNode) {
            sval_t *firstExprNode = (sval_t *)(uintptr_t)firstNode[0];
            if (firstExprNode && EmitOrEvalExpression(firstExprNode[0], &constValue, block))
                EmitValue(&constValue);
            ++exprCount;
            if (EmitOrEvalPrimitiveExpression(node[1], &constValue, block))
                EmitValue(&constValue);
            EmitOpcode(0x79, -2 - exprCount, 0);
            AddOpcodePos(node[4].sourcePosValue, 0);
            if (firstExprNode)
                AddOpcodePos(firstExprNode[1].sourcePosValue, 0);
        }
        AddOpcodePos(node[3].sourcePosValue, 0);
        return 0;
    }

    case 0x39:
        EmitOpcode(0x4d, 0, 0);
        AddOpcodePos(node[1].sourcePosValue, 1);
        AddOpcodePos(node[1].sourcePosValue, 0);
        return 0;

    case 0x3a: {
        intptr_t *listNode;
        int exprCount = 0;
        int signedCount = 0;
        intptr_t *firstNode;

        EmitOpcode(0x7b, 1, 0);
        AddOpcodePos(node[3].sourcePosValue, 1);

        for (listNode = ScriptStatementListFirstExecNode(node[2]); listNode; listNode = (intptr_t *)(uintptr_t)listNode[1]) {
            sval_t *exprNode = (sval_t *)(uintptr_t)listNode[0];
            if (exprNode && EmitOrEvalExpression(exprNode[0], &constValue, block))
                EmitValue(&constValue);
            ++exprCount;
        }
        if (exprCount)
            signedCount = -exprCount;

        if (EmitOrEvalPrimitiveExpression(node[1], &constValue, block))
            EmitValue(&constValue);
        EmitOpcode(0x79, signedCount - 2, 0);
        AddOpcodePos(node[4].sourcePosValue, 0);
        firstNode = ScriptStatementListFirstExecNode(node[2]);
        if (firstNode) {
            sval_t *firstExprNode = (sval_t *)(uintptr_t)firstNode[0];
            if (firstExprNode)
                AddOpcodePos(firstExprNode[1].sourcePosValue, 0);
        }
        AddOpcodePos(node[3].sourcePosValue, 0);
        return 0;
    }

    case 0x3b:
        if (EmitOrEvalExpression(node[2], &constValue, block))
            EmitValue(&constValue);
        if (EmitOrEvalPrimitiveExpression(node[1], &constValue, block))
            EmitValue(&constValue);
        EmitOpcode(0x7a, -2, 0);
        AddOpcodePos(node[4].sourcePosValue, 0);
        AddOpcodePos(node[3].sourcePosValue, 0);
        return 0;

    case 0x3c:
        return EmitSwitchStatement(node[1], node[2], node[3], lastStatement, endSourcePos, block);

    case 0x3d:
        CompileError(node[2].sourcePosValue, "illegal case statement");
        return 0;

    case 0x3e:
        CompileError(node[1].sourcePosValue, "illegal default statement");
        return 0;

    case 0x3f:
        return EmitBreakStatement(node[1].sourcePosValue, block);

    case 0x40:
        return EmitContinueStatement(node[1].sourcePosValue, block);

    case 0x4c: {
        unsigned int stringValue = node[1].stringValue;
        if (SCRVP->developer_script) {
            if (!SCRCG->bConstRefCount)
                SL_RemoveRefToString(stringValue);
            EmitOpcode(0x85, 0, 0);
            *(char *)TempMalloc(1) = 0;
        } else if (!SCRCG->bConstRefCount) {
            SL_RemoveRefToString(stringValue);
        }
        return 0;
    }

    case 0x4d: {
        unsigned int stringValue = node[1].stringValue;
        if (SCRVP->developer_script) {
            if (!SCRCG->bConstRefCount)
                SL_RemoveRefToString(stringValue);
            EmitOpcode(0x86, 0, 0);
            *(char *)TempMalloc(1) = 0;
        } else if (!SCRCG->bConstRefCount) {
            SL_RemoveRefToString(stringValue);
        }
        return 0;
    }

    default:
        return 0;
    }
}

static unsigned int __attribute_regparm__(3)
    EmitIfStatement(sval_t expr, sval_t stmt, sval_t sourcePos, int lastStatement, unsigned int endSourcePos, scr_block_t *block, sval_t *ifStatBlock)
{
    VariableCompileValue constValue;
    scr_block_t *ifBlock;
    unsigned short *jumpOffset;
    char *nextPos;
    unsigned int savedChecksum;

    if (EmitOrEvalExpression(expr, &constValue, block)) {
        EmitValue(&constValue);
    }

    EmitOpcode(0x5e, -1, 0);
    AddOpcodePos(sourcePos.sourcePosValue, 0);

    jumpOffset = (unsigned short *)TempMallocAlign(2);
    SCRCG->codePos = (byte *)jumpOffset;
    *jumpOffset = 0;
    nextPos = TempMalloc(0);

    ifBlock = ifStatBlock->block;

    Scr_MergeChildBlocks(ifBlock, block);
    EmitStatement(stmt, lastStatement, endSourcePos, ifBlock);

    savedChecksum = SCRVP->checksum;
    if (lastStatement) {
        EmitOpcode(0, 0, 0);
        AddOpcodePos(endSourcePos, 1);
    } else {
        Scr_EmitLocalVarsAtEnd(ifBlock);
    }
    SCRVP->checksum = savedChecksum + 1;

    *jumpOffset = (unsigned short)(TempMalloc(0) - nextPos);
    return 0;
}

#    if 0

static __attribute__((naked))
unsigned int EmitIfStatement(sval_t stmt, int lastStatement, unsigned int endSourcePos, scr_block_t *block, sval_t *ifStatBlock)
{
    __asm__ __volatile__ (
        "pushl %ebp\n"
        "movl %esp, %ebp\n"
        "pushl %edi\n"
        "pushl %esi\n"
        "pushl %ebx\n"
        "subl $0x4c, %esp\n"
        "movl %edx, -0x44(%ebp)\n"
        "movl %ecx, %edi\n"
        "movzbl 8(%ebp), %edx\n"
        "movb %dl, -0x45(%ebp)\n"

        "leal -0x24(%ebp), %esi\n"
        "movl 0x10(%ebp), %ecx\n"
        "movl %esi, %edx\n"
        "calll EmitOrEvalExpression\n"
        "testb %al, %al\n"
        "jne .Lf984f6_0009873a\n"

        ".Lf984f6_00098520:\n"
        "xorl %ecx, %ecx\n"
        "movl $0xffffffff, %edx\n"
        "movl $0x5e, %eax\n"
        "calll EmitOpcode\n"
        "movl $0, 4(%esp)\n"
        "movl %edi, (%esp)\n"
        "calll AddOpcodePos\n"
        "movl $2, (%esp)\n"
        "calll TempMallocAlign\n"
        "movl %eax, scrCompileGlob\n"
        "movw $0, (%eax)\n"
        "movl scrCompileGlob, %ebx\n"
        "movl %ebx, -0x40(%ebp)\n"
        "movl $0, (%esp)\n"
        "calll TempMalloc\n"
        "movl %eax, -0x3c(%ebp)\n"
        "movl 0x14(%ebp), %eax\n"
        "movl (%eax), %edi\n"
        "testl %edi, %edi\n"
        "jne .Lf984f6_if_block_ready\n"
        "movl 0x10(%ebp), %edi\n"
        "movl %edi, (%eax)\n"
        ".Lf984f6_if_block_ready:\n"
        "movl %edi, -0x30(%ebp)\n"
        "movl 0x10(%ebp), %edx\n"
        "addl $0x18, %edx\n"
        "movl %edx, -0x2c(%ebp)\n"
        "movl $0, -0x34(%ebp)\n"

        ".Lf984f6_00098587:\n"
        "movl -0x34(%ebp), %eax\n"
        "cmpl 8(%edi), %eax\n"
        "jl .Lf984f6_0009859e\n"
        "movl 0x10(%ebp), %edx\n"
        "movl 4(%edx), %eax\n"
        "cmpl %eax, -0x34(%ebp)\n"
        "jge .Lf984f6_00098686\n"

        ".Lf984f6_0009859e:\n"
        "movl -0x2c(%ebp), %ebx\n"
        "movl (%ebx), %ebx\n"
        "movl %ebx, -0x38(%ebp)\n"

        "movl 0xc(%edi), %esi\n"
        "movl %esi, %ecx\n"
        "cmpl %esi, -0x34(%ebp)\n"
        "jge .Lf984f6_00098653\n"
        "movl -0x30(%ebp), %eax\n"
        "cmpl %ebx, 0x18(%eax)\n"
        "je .Lf984f6_0009867e\n"
        "movl -0x34(%ebp), %ebx\n"
        "leal 0x18(%edi, %ebx, 4), %edx\n"
        ".Lf984f6_000985c7:\n"
        "addl $1, %ebx\n"
        "cmpl %esi, %ebx\n"
        "je .Lf984f6_00098653\n"
        "movl 4(%edx), %eax\n"
        "addl $4, %edx\n"
        "cmpl %eax, -0x38(%ebp)\n"
        "jne .Lf984f6_000985c7\n"

        ".Lf984f6_000985dd:\n"
        "testl %ebx, %ebx\n"
        "js .Lf984f6_00098653\n"
        ".Lf984f6_000985e1:\n"
        "movl 8(%edi), %eax\n"
        "cmpl %eax, %ebx\n"
        "jl .Lf984f6_000985ee\n"
        "addl $1, %eax\n"
        "movl %eax, 8(%edi)\n"
        ".Lf984f6_000985ee:\n"
        "cmpl %ebx, -0x34(%ebp)\n"
        "jge .Lf984f6_00098614\n"
        "leal (, %ebx, 4), %eax\n"
        "leal 0x14(%eax, %edi), %ecx\n"
        "leal 0x18(%eax, %edi), %edx\n"
        ".Lf984f6_00098602:\n"
        "subl $1, %ebx\n"
        "movl (%ecx), %eax\n"
        "movl %eax, (%edx)\n"
        "subl $4, %ecx\n"
        "subl $4, %edx\n"
        "cmpl %ebx, -0x34(%ebp)\n"
        "jl .Lf984f6_00098602\n"
        ".Lf984f6_00098614:\n"
        "movl -0x38(%ebp), %edx\n"
        "movl -0x30(%ebp), %eax\n"
        "movl %edx, 0x18(%eax)\n"
        "movl -0x34(%ebp), %edx\n"
        "sarl $3, %edx\n"
        "movl -0x34(%ebp), %ecx\n"
        "andl $7, %ecx\n"
        "movl 0x10(%ebp), %ebx\n"
        "movzbl 0x10(%edx, %ebx), %eax\n"
        "sarl %cl, %eax\n"
        "testb $1, %al\n"
        "je .Lf984f6_00098642\n"
        "movl $1, %eax\n"
        "shll %cl, %eax\n"
        "orb %al, 0x10(%edx, %edi)\n"
        ".Lf984f6_00098642:\n"
        "addl $1, -0x34(%ebp)\n"
        "addl $4, -0x30(%ebp)\n"
        "addl $4, -0x2c(%ebp)\n"
        "jmp .Lf984f6_00098587\n"
        ".Lf984f6_00098653:\n"
        "cmpl $0x3f, %esi\n"
        "jg .Lf984f6_00098665\n"
        ".Lf984f6_00098658:\n"
        "leal 1(%ecx), %eax\n"
        "movl %eax, 0xc(%edi)\n"
        "movl %esi, %ebx\n"
        "jmp .Lf984f6_000985e1\n"
        ".Lf984f6_00098665:\n"
        "movl $"LOCAL_VAR_STACK_SIZE exceeded", 4(%esp)\n"
        "movl $1, (%esp)\n"
        "calll Com_Error\n"
        "movl 0xc(%edi), %ecx\n"
        "jmp .Lf984f6_00098658\n"

        ".Lf984f6_0009867e:\n"
        "movl -0x34(%ebp), %ebx\n"
        "jmp .Lf984f6_000985dd\n"

        ".Lf984f6_00098686:\n"
        "movl %eax, 4(%edi)\n"
        "movl $0, (%edi)\n"

        "movl 0x14(%ebp), %ebx\n"
        "movl (%ebx), %eax\n"
        "movzbl -0x45(%ebp), %ebx\n"
        "movl %eax, (%esp)\n"
        "movl 0xc(%ebp), %ecx\n"
        "movl %ebx, %edx\n"
        "movl -0x44(%ebp), %eax\n"
        "calll EmitStatement\n"
        "movl 0x14(%ebp), %eax\n"
        "movl (%eax), %esi\n"

        "movl imp_scrVarPub, %edx\n"
        "movl 0x3c(%edx), %edi\n"
        "testl %ebx, %ebx\n"
        "jne .Lf984f6_0009871a\n"

        "movl (%esi), %eax\n"
        "testl %eax, %eax\n"
        "jne .Lf984f6_000986f1\n"
        "movl 4(%esi), %eax\n"
        "movl %eax, %ebx\n"
        "subl 8(%esi), %ebx\n"
        "je .Lf984f6_000986f1\n"
        "xorl %ecx, %ecx\n"
        "xorl %edx, %edx\n"
        "movl $0x17, %eax\n"
        "calll EmitOpcode\n"
        "movl $1, (%esp)\n"
        "calll TempMalloc\n"
        "movl %eax, scrCompileGlob\n"
        "movb %bl, (%eax)\n"
        "movl 8(%esi), %eax\n"
        "movl %eax, 4(%esi)\n"

        ".Lf984f6_000986f1:\n"
        "leal 1(%edi), %eax\n"
        "movl imp_scrVarPub, %edx\n"
        "movl %eax, 0x3c(%edx)\n"

        "movl $0, (%esp)\n"
        "calll TempMalloc\n"
        "subl -0x3c(%ebp), %eax\n"
        "movl -0x40(%ebp), %ebx\n"
        "movw %ax, (%ebx)\n"

        "addl $0x4c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"

        ".Lf984f6_0009871a:\n"
        "xorl %ecx, %ecx\n"
        "xorl %edx, %edx\n"
        "xorl %eax, %eax\n"
        "calll EmitOpcode\n"
        "movl $1, 4(%esp)\n"
        "movl 0xc(%ebp), %ebx\n"
        "movl %ebx, (%esp)\n"
        "calll AddOpcodePos\n"
        "jmp .Lf984f6_000986f1\n"

        ".Lf984f6_0009873a:\n"
        "movl %esi, %eax\n"
        "calll EmitValue\n"
        "jmp .Lf984f6_00098520\n"
    );
}

#    endif

static unsigned int __attribute_regparm__(3)
    EmitIfElseStatement(sval_t expr, sval_t stmt1, sval_t stmt2, sval_t sourcePos, sval_t elseSourcePos, int lastStatement, unsigned int endSourcePos, scr_block_t *block, sval_t *ifStatBlock, sval_t *elseStatBlock)
{
    VariableCompileValue constValue;
    scr_block_t *ifBlock;
    scr_block_t *elseBlock;
    scr_block_t *childBlocks[3];
    int childCount;
    unsigned short *pos1;
    int *pos2;
    char *nextPos1;
    char *nextPos2;
    unsigned int savedChecksum;

    if (EmitOrEvalExpression(expr, &constValue, block)) {
        EmitValue(&constValue);
    }

    EmitOpcode(0x5e, -1, 0);
    AddOpcodePos(sourcePos.sourcePosValue, 0);

    pos1 = (unsigned short *)TempMallocAlign(2);
    SCRCG->codePos = (byte *)pos1;
    *pos1 = 0;
    nextPos1 = TempMalloc(0);

    ifBlock = ifStatBlock->block;
    if (!ifBlock) {
        ifBlock = block;
        ifStatBlock->block = block;
    }

    Scr_MergeChildBlocks(ifBlock, block);
    EmitStatement(stmt1, lastStatement, endSourcePos, ifBlock);
    Scr_EmitLocalVarsAtEnd(ifBlock);

    childCount = 0;
    if (!ifBlock->abortLevel) {
        childBlocks[childCount++] = ifBlock;
    }

    savedChecksum = SCRVP->checksum;
    if (lastStatement) {
        EmitOpcode(0, 0, 0);
        pos2 = (int *)TempMallocAlign(4);
        SCRCG->codePos = (byte *)pos2;
        *pos2 = 0;
        AddOpcodePos(endSourcePos, 1);
        pos2 = 0;
        nextPos2 = 0;
    } else {
        EmitOpcode(0x62, 0, 0);
        AddOpcodePos(elseSourcePos.sourcePosValue, 1);
        pos2 = (int *)TempMallocAlign(4);
        SCRCG->codePos = (byte *)pos2;
        *pos2 = 0;
        nextPos2 = TempMalloc(0);
    }
    SCRVP->checksum = savedChecksum + 1;

    *pos1 = (unsigned short)(TempMalloc(0) - nextPos1);

    elseBlock = elseStatBlock->block;
    if (!elseBlock) {
        elseBlock = block;
        elseStatBlock->block = block;
    }

    Scr_MergeChildBlocks(elseBlock, block);
    EmitStatement(stmt2, lastStatement, endSourcePos, elseBlock);

    savedChecksum = SCRVP->checksum;
    if (lastStatement) {
        EmitOpcode(0, 0, 0);
        AddOpcodePos(endSourcePos, 1);
    } else {
        Scr_EmitLocalVarsAtEnd(elseBlock);
    }
    SCRVP->checksum = savedChecksum + 1;

    if (!elseBlock->abortLevel) {
        childBlocks[childCount++] = elseBlock;
    }

    if (!lastStatement) {
        *pos2 = (int)(TempMalloc(0) - nextPos2);
    }

    Scr_MergeBranchChildBlocks(block, childBlocks, childCount);
    return 0;
}

static void Scr_EmitLocalVarsCreate(scr_block_t *block)
{
    int i;

    if (block->localVarsCreateCount < block->localVarsPublicCount) {
        for (i = block->localVarsCreateCount; i < block->localVarsPublicCount; ++i) {
            EmitOpcode(0x16, 0, 0);
            Scr_EmitCanonicalLocalVarName(block->localVars[i].name);
        }
    }

    block->localVarsCreateCount = block->localVarsPublicCount;
}

static unsigned int __attribute_regparm__(3)
    EmitWhileStatement(sval_t expr, sval_t stmt, sval_t sourcePos, sval_t whileSourcePos, scr_block_t *block, sval_t *whileStatBlock)
{
    VariableCompileValue constValue;
    scr_block_t *whileBlock;
    scr_block_t **oldBreakChildBlocks;
    int *oldBreakChildCount;
    scr_block_t *oldBreakBlock;
    scr_block_t **oldContinueChildBlocks;
    int *oldContinueChildCount;
    Bool bOldCanBreak;
    Bool bOldCanIgnoreBreak;
    BreakStatementInfo *oldBreakStatement;
    Bool bOldCanContinue;
    Bool bOldCanIgnoreContinue;
    ContinueStatementInfo *oldContinueStatement;
    Bool constConditional;
    scr_block_t **breakChildBlocks;
    int breakChildCount;
    unsigned short *pos2;
    char *pos1;
    char *nextPos2;
    char *nextCodePos;
    ContinueStatementInfo *continueStatement;
    BreakStatementInfo *breakStatement;
    sval_t *stmtNode;

    bOldCanBreak = SCRCG->bCanBreak;
    bOldCanIgnoreBreak = SCRCG->bCanIgnoreBreak;
    oldBreakStatement = SCRCG->currentBreakStatement;
    SCRCG->bCanBreak = 0;
    SCRCG->bCanIgnoreBreak = 0;

    bOldCanContinue = SCRCG->bCanContinue;
    bOldCanIgnoreContinue = SCRCG->bCanIgnoreContinue;
    oldContinueStatement = SCRCG->currentContinueStatement;
    SCRCG->bCanContinue = 0;
    SCRCG->bCanIgnoreContinue = 0;

    whileBlock = whileStatBlock->block;
    Scr_MergeChildBlocks(whileBlock, block);
    Scr_EmitLocalVarsCreate(whileBlock);
    block->localVarsCreateCount = whileBlock->localVarsCreateCount;

    pos1 = TempMalloc(0);
    if (EmitOrEvalExpression(expr, &constValue, block)) {
        if (constValue.value.type == 5 || constValue.value.type == 6) {
            Scr_CastBool(&constValue.value);
            if (!constValue.value.u.intValue) {
                CompileError(sourcePos.sourcePosValue, "conditional expression cannot be always false");
            }
            constConditional = 1;
        } else {
            EmitValue(&constValue);
            constConditional = 0;
        }
    } else {
        constConditional = 0;
    }

    oldBreakChildBlocks = SCRCG->breakChildBlocks;
    oldBreakChildCount = SCRCG->breakChildCount;
    oldBreakBlock = SCRCG->breakBlock;
    oldContinueChildBlocks = SCRCG->continueChildBlocks;
    oldContinueChildCount = SCRCG->continueChildCount;

    breakChildCount = 0;
    SCRCG->continueChildBlocks = NULL;
    SCRCG->breakBlock = whileBlock;

    if (constConditional) {
        breakChildBlocks = (scr_block_t **)Hunk_AllocateTempMemoryHighInternal(0x1000);
        SCRCG->breakChildCount = &breakChildCount;
        pos2 = NULL;
        nextPos2 = NULL;
    } else {
        EmitOpcode(0x5e, -1, 0);
        AddOpcodePos(sourcePos.sourcePosValue, 0);
        pos2 = (unsigned short *)TempMallocAlign(2);
        SCRCG->codePos = (byte *)pos2;
        *pos2 = 0;
        nextPos2 = TempMalloc(0);
        breakChildBlocks = NULL;
    }

    SCRCG->breakChildBlocks = breakChildBlocks;
    SCRCG->bCanBreak = 1;
    SCRCG->bCanIgnoreBreak = SCRCP->developer_statement != 0;
    SCRCG->currentBreakStatement = NULL;
    SCRCG->bCanContinue = 1;
    SCRCG->bCanIgnoreContinue = SCRCP->developer_statement != 0;
    SCRCG->currentContinueStatement = NULL;

    EmitStatement(stmt, 0, 0, whileBlock);
    if (whileBlock->abortLevel != 3) {
        whileBlock->abortLevel = 0;
    }

    SCRCG->bCanBreak = 0;
    SCRCG->bCanIgnoreBreak = 0;
    SCRCG->bCanContinue = 0;
    SCRCG->bCanIgnoreContinue = 0;

    nextCodePos = TempMalloc(0);
    for (continueStatement = SCRCG->currentContinueStatement;
         continueStatement;
         continueStatement = continueStatement->next) {
        *(int *)continueStatement->codePos = (int)(nextCodePos - continueStatement->nextCodePos);
    }

    EmitOpcode(0x63, 0, 0);
    AddOpcodePos(whileSourcePos.sourcePosValue, 0);
    stmtNode = (sval_t *)(uintptr_t)stmt.node;
    if (stmtNode[0].type == 0x2c) {
        AddOpcodePos(stmtNode[3].sourcePosValue, 1);
    }

    SCRCG->codePos = (byte *)TempMallocAlign(2);
    *(unsigned short *)SCRCG->codePos = (unsigned short)(TempMalloc(0) - pos1);

    if (pos2) {
        *pos2 = (unsigned short)(TempMalloc(0) - nextPos2);
    }

    nextCodePos = TempMalloc(0);
    for (breakStatement = SCRCG->currentBreakStatement;
         breakStatement;
         breakStatement = breakStatement->next) {
        *(int *)breakStatement->codePos = (int)(nextCodePos - breakStatement->nextCodePos);
    }

    SCRCG->bCanBreak = bOldCanBreak;
    SCRCG->bCanIgnoreBreak = bOldCanIgnoreBreak;
    SCRCG->currentBreakStatement = oldBreakStatement;
    SCRCG->bCanContinue = bOldCanContinue;
    SCRCG->bCanIgnoreContinue = bOldCanIgnoreContinue;
    SCRCG->currentContinueStatement = oldContinueStatement;

    if (constConditional && breakChildCount) {
        Scr_MergeBranchChildBlocks(block, breakChildBlocks, breakChildCount);
    }

    SCRCG->breakChildBlocks = oldBreakChildBlocks;
    SCRCG->breakChildCount = oldBreakChildCount;
    SCRCG->breakBlock = oldBreakBlock;
    SCRCG->continueChildBlocks = oldContinueChildBlocks;
    SCRCG->continueChildCount = oldContinueChildCount;

    return 0;
}

static unsigned int __attribute_regparm__(3)
    EmitForStatement(sval_t stmt1, sval_t expr, sval_t stmt2, sval_t stmt, sval_t sourcePos, sval_t forSourcePos, scr_block_t *block, sval_t *forStatBlock, sval_t *forStatPostBlock)
{
    VariableCompileValue constValue;
    sval_t *exprNode;
    sval_t *stmtNode;
    scr_block_t *forBlock;
    scr_block_t *postBlock;
    scr_block_t **oldBreakChildBlocks;
    int *oldBreakChildCount;
    scr_block_t *oldBreakBlock;
    scr_block_t **oldContinueChildBlocks;
    int *oldContinueChildCount;
    Bool bOldCanBreak;
    Bool bOldCanIgnoreBreak;
    BreakStatementInfo *oldBreakStatement;
    Bool bOldCanContinue;
    Bool bOldCanIgnoreContinue;
    ContinueStatementInfo *oldContinueStatement;
    Bool constConditional;
    scr_block_t **breakChildBlocks;
    scr_block_t **continueChildBlocks;
    int breakChildCount;
    int continueChildCount;
    unsigned short *pos2;
    char *pos1;
    char *nextPos2;
    char *nextCodePos;
    ContinueStatementInfo *continueStatement;
    BreakStatementInfo *breakStatement;

    bOldCanBreak = SCRCG->bCanBreak;
    bOldCanIgnoreBreak = SCRCG->bCanIgnoreBreak;
    oldBreakStatement = SCRCG->currentBreakStatement;
    SCRCG->bCanBreak = 0;
    SCRCG->bCanIgnoreBreak = 0;

    bOldCanContinue = SCRCG->bCanContinue;
    bOldCanIgnoreContinue = SCRCG->bCanIgnoreContinue;
    oldContinueStatement = SCRCG->currentContinueStatement;
    SCRCG->bCanContinue = 0;
    SCRCG->bCanIgnoreContinue = 0;

    EmitStatement(stmt1, 0, 0, block);

    forBlock = forStatBlock->block;
    Scr_MergeChildBlocks(forBlock, block);
    Scr_EmitLocalVarsCreate(forBlock);
    block->localVarsCreateCount = forBlock->localVarsCreateCount;

    postBlock = forStatPostBlock->block;
    Scr_MergeChildBlocks(postBlock, block);

    pos1 = TempMalloc(0);
    exprNode = (sval_t *)(uintptr_t)expr.node;
    if (exprNode[0].type == 0x41) {
        if (EmitOrEvalExpression(exprNode[1], &constValue, block)) {
            if (constValue.value.type == 5 || constValue.value.type == 6) {
                Scr_CastBool(&constValue.value);
                if (!constValue.value.u.intValue) {
                    CompileError(sourcePos.sourcePosValue, "conditional expression cannot be always false");
                }
                constConditional = 1;
            } else {
                EmitValue(&constValue);
                constConditional = 0;
            }
        } else {
            constConditional = 0;
        }
    } else {
        constConditional = 1;
    }

    oldBreakChildBlocks = SCRCG->breakChildBlocks;
    oldBreakChildCount = SCRCG->breakChildCount;
    oldBreakBlock = SCRCG->breakBlock;
    oldContinueChildBlocks = SCRCG->continueChildBlocks;
    oldContinueChildCount = SCRCG->continueChildCount;

    breakChildCount = 0;
    continueChildCount = 0;
    continueChildBlocks = (scr_block_t **)Hunk_AllocateTempMemoryHighInternal(0x1000);
    SCRCG->continueChildBlocks = continueChildBlocks;
    SCRCG->continueChildCount = &continueChildCount;
    SCRCG->breakBlock = forBlock;

    if (constConditional) {
        breakChildBlocks = (scr_block_t **)Hunk_AllocateTempMemoryHighInternal(0x1000);
        SCRCG->breakChildCount = &breakChildCount;
        pos2 = NULL;
        nextPos2 = NULL;
    } else {
        EmitOpcode(0x5e, -1, 0);
        AddOpcodePos(sourcePos.sourcePosValue, 0);
        pos2 = (unsigned short *)TempMallocAlign(2);
        SCRCG->codePos = (byte *)pos2;
        *pos2 = 0;
        nextPos2 = TempMalloc(0);
        breakChildBlocks = NULL;
    }

    SCRCG->breakChildBlocks = breakChildBlocks;
    SCRCG->bCanBreak = 1;
    SCRCG->bCanIgnoreBreak = SCRCP->developer_statement != 0;
    SCRCG->currentBreakStatement = NULL;
    SCRCG->bCanContinue = 1;
    SCRCG->bCanIgnoreContinue = SCRCP->developer_statement != 0;
    SCRCG->currentContinueStatement = NULL;

    EmitStatement(stmt, 0, 0, forBlock);
    if (!forBlock->abortLevel && SCRCG->continueChildBlocks) {
        if (*SCRCG->continueChildCount > 0x3ff) {
            Com_Error(1, "MAX_SWITCH_CASES exceeded");
        }
        SCRCG->continueChildBlocks[*SCRCG->continueChildCount] = forBlock;
        ++*SCRCG->continueChildCount;
    }

    SCRCG->bCanBreak = 0;
    SCRCG->bCanIgnoreBreak = 0;
    SCRCG->bCanContinue = 0;
    SCRCG->bCanIgnoreContinue = 0;

    nextCodePos = TempMalloc(0);
    for (continueStatement = SCRCG->currentContinueStatement;
         continueStatement;
         continueStatement = continueStatement->next) {
        *(int *)continueStatement->codePos = (int)(nextCodePos - continueStatement->nextCodePos);
    }

    if (continueChildCount) {
        Scr_MergeBranchChildBlocks(postBlock, continueChildBlocks, continueChildCount);
    }

    EmitStatement(stmt2, 0, 0, postBlock);

    EmitOpcode(0x63, 0, 0);
    AddOpcodePos(forSourcePos.sourcePosValue, 0);
    stmtNode = (sval_t *)(uintptr_t)stmt.node;
    if (stmtNode[0].type == 0x2c) {
        AddOpcodePos(stmtNode[3].sourcePosValue, 1);
    }

    SCRCG->codePos = (byte *)TempMallocAlign(2);
    *(unsigned short *)SCRCG->codePos = (unsigned short)(TempMalloc(0) - pos1);

    if (pos2) {
        *pos2 = (unsigned short)(TempMalloc(0) - nextPos2);
    }

    nextCodePos = TempMalloc(0);
    for (breakStatement = SCRCG->currentBreakStatement;
         breakStatement;
         breakStatement = breakStatement->next) {
        *(int *)breakStatement->codePos = (int)(nextCodePos - breakStatement->nextCodePos);
    }

    SCRCG->bCanBreak = bOldCanBreak;
    SCRCG->bCanIgnoreBreak = bOldCanIgnoreBreak;
    SCRCG->currentBreakStatement = oldBreakStatement;
    SCRCG->bCanContinue = bOldCanContinue;
    SCRCG->bCanIgnoreContinue = bOldCanIgnoreContinue;
    SCRCG->currentContinueStatement = oldContinueStatement;

    if (constConditional && breakChildCount) {
        Scr_MergeBranchChildBlocks(block, breakChildBlocks, breakChildCount);
    }

    SCRCG->breakChildBlocks = oldBreakChildBlocks;
    SCRCG->breakChildCount = oldBreakChildCount;
    SCRCG->breakBlock = oldBreakBlock;
    SCRCG->continueChildBlocks = oldContinueChildBlocks;
    SCRCG->continueChildCount = oldContinueChildCount;

    return 0;
}

static unsigned int __attribute_regparm__(3)
    EmitSwitchStatement(sval_t expr, sval_t stmtlist, sval_t sourcePos, int lastStatement, unsigned int endSourcePos, scr_block_t *block)
{
    VariableCompileValue constValue;
    Bool bOldCanIgnoreCase;
    CaseStatementInfo *oldCaseStatement;
    Bool bOldCanBreak;
    Bool bOldCanIgnoreBreak;
    BreakStatementInfo *oldBreakStatement;
    scr_block_t **oldBreakChildBlocks;
    int *oldBreakChildCount;
    scr_block_t *oldBreakBlock;
    scr_block_t **breakChildBlocks;
    int breakChildCount;
    Bool hasDefault;
    intptr_t *listNode;
    intptr_t *nextNode;
    int *pos1;
    unsigned short *pos2;
    char *nextPos1;
    unsigned int (*caseTable)[2];
    int numCases;
    CaseStatementInfo *caseStatement;
    BreakStatementInfo *breakStatement;
    char *nextCodePos;
    int i;

    bOldCanIgnoreCase = SCRCG->bCanIgnoreCase;
    oldCaseStatement = SCRCG->currentCaseStatement;
    SCRCG->bCanIgnoreCase = 0;

    bOldCanBreak = SCRCG->bCanBreak;
    bOldCanIgnoreBreak = SCRCG->bCanIgnoreBreak;
    oldBreakStatement = SCRCG->currentBreakStatement;
    SCRCG->bCanBreak = 0;
    SCRCG->bCanIgnoreBreak = 0;

    if (EmitOrEvalExpression(expr, &constValue, block)) {
        EmitValue(&constValue);
    }

    EmitOpcode(0x7c, -1, 0);
    pos1 = (int *)TempMallocAlign(4);
    SCRCG->codePos = (byte *)pos1;
    *pos1 = 0;
    nextPos1 = TempMalloc(0);

    SCRCG->bCanIgnoreCase = SCRCP->developer_statement != 0;
    SCRCG->currentCaseStatement = NULL;
    SCRCG->bCanIgnoreBreak = SCRCP->developer_statement != 0;
    SCRCG->currentBreakStatement = NULL;

    oldBreakChildBlocks = SCRCG->breakChildBlocks;
    oldBreakChildCount = SCRCG->breakChildCount;
    oldBreakBlock = SCRCG->breakBlock;
    breakChildCount = 0;
    breakChildBlocks = (scr_block_t **)Hunk_AllocateTempMemoryHighInternal(0x1000);
    SCRCG->breakChildBlocks = breakChildBlocks;
    SCRCG->breakChildCount = &breakChildCount;
    SCRCG->breakBlock = NULL;

    hasDefault = 0;
    listNode = ScriptStatementListFirstExecNode(stmtlist);
    while (listNode) {
        sval_t *stmt = (sval_t *)(uintptr_t)listNode[0];
        nextNode = (intptr_t *)(uintptr_t)listNode[1];

        if (stmt[0].type == 0x3d || stmt[0].type == 0x3e) {
            if (SCRCG->breakBlock) {
                SCRCG->bCanBreak = 0;
                Scr_EmitLocalVarsAtEnd(SCRCG->breakBlock);
            }

            if (stmt[0].type == 0x3d) {
                sval_t *caseExpr;
                unsigned int value;
                unsigned int caseSourcePos;

                SCRCG->breakBlock = stmt[3].block;
                caseExpr = (sval_t *)(uintptr_t)stmt[1].node;
                caseSourcePos = stmt[2].sourcePosValue;

                if (caseExpr[0].type == 7) {
                    value = caseExpr[1].intValue;
                    if (!IsValidArrayIndex(value)) {
                        CompileError(caseSourcePos, va("case index %d out of range", value));
                    } else {
                        value = GetInternalVariableIndex(value);
                    }
                } else if (caseExpr[0].type == 0xb) {
                    value = caseExpr[1].stringValue;
                    if (SCRCP->developer_statement == SCRCOMP_SCR_DEV_IGNORE) {
                        if (!SCRCG->bConstRefCount) {
                            SL_RemoveRefToString(value);
                        }
                    } else {
                        if (SCRCG->bConstRefCount) {
                            SL_AddRefToString(value);
                        }
                        SL_TransferRefToUser(value, 1);
                    }
                } else {
                    CompileError(caseSourcePos, "case expression must be an int or string");
                    value = 0;
                }

                if (SCRCP->developer_statement != SCRCOMP_SCR_DEV_IGNORE) {
                    caseStatement = (CaseStatementInfo *)Hunk_AllocateTempMemoryHighInternal(sizeof(*caseStatement));
                    caseStatement->name = value;
                    caseStatement->codePos = TempMalloc(0);
                    caseStatement->sourcePos = caseSourcePos;
                    caseStatement->next = SCRCG->currentCaseStatement;
                    SCRCG->currentCaseStatement = caseStatement;
                }
            } else {
                SCRCG->breakBlock = stmt[2].block;
                if (SCRCP->developer_statement != SCRCOMP_SCR_DEV_IGNORE) {
                    caseStatement = (CaseStatementInfo *)Hunk_AllocateTempMemoryHighInternal(sizeof(*caseStatement));
                    caseStatement->name = 0;
                    caseStatement->codePos = TempMalloc(0);
                    caseStatement->sourcePos = stmt[1].sourcePosValue;
                    caseStatement->next = SCRCG->currentCaseStatement;
                    SCRCG->currentCaseStatement = caseStatement;
                }
                hasDefault = 1;
            }

            Scr_MergeChildBlocks(SCRCG->breakBlock, block);
            SCRCG->bCanBreak = 1;
            listNode = nextNode;
            continue;
        }

        if (!SCRCG->breakBlock) {
            CompileError(endSourcePos, "missing case statement");
            goto emit_switch_table;
        }

        {
            int emitLastStatement = 0;
            if (lastStatement) {
                emitLastStatement = 1;
                if (nextNode && !SCRVP->developer_script) {
                    intptr_t *scanNode = nextNode;
                    while (scanNode) {
                        sval_t *scanStmt = (sval_t *)(uintptr_t)scanNode[0];
                        if (scanStmt[0].type != 0x2d) {
                            emitLastStatement = 0;
                            break;
                        }
                        scanNode = (intptr_t *)(uintptr_t)scanNode[1];
                    }
                } else if (nextNode) {
                    emitLastStatement = 0;
                }
            }

            {
                sval_t stmtVal;
                stmtVal.node = (intptr_t)stmt;
                EmitStatement(stmtVal, emitLastStatement, endSourcePos, SCRCG->breakBlock);
            }
        }

        if (SCRCG->breakBlock && SCRCG->breakBlock->abortLevel) {
            SCRCG->breakBlock = NULL;
            SCRCG->bCanBreak = 0;
        }

        listNode = nextNode;
    }

    if (SCRCG->breakBlock) {
        SCRCG->bCanBreak = 0;
        Scr_EmitLocalVarsAtEnd(SCRCG->breakBlock);
    }

    if (hasDefault) {
        if (SCRCG->breakBlock && !SCRCG->breakBlock->abortLevel && SCRCG->breakChildBlocks) {
            if (*SCRCG->breakChildCount > 0x3ff) {
                Com_Error(1, "MAX_SWITCH_CASES exceeded");
            }
            SCRCG->breakChildBlocks[*SCRCG->breakChildCount] = SCRCG->breakBlock;
            ++*SCRCG->breakChildCount;
        }
        Scr_MergeBranchChildBlocks(block, breakChildBlocks, breakChildCount);
    }

    SCRCG->breakChildBlocks = oldBreakChildBlocks;
    SCRCG->breakChildCount = oldBreakChildCount;
    SCRCG->breakBlock = oldBreakBlock;

emit_switch_table:
    SCRCG->bCanIgnoreCase = 0;
    SCRCG->bCanIgnoreBreak = 0;

    EmitOpcode(0x7d, 0, 0);
    AddOpcodePos(sourcePos.sourcePosValue, 0);

    pos2 = (unsigned short *)TempMallocAlign(2);
    SCRCG->codePos = (byte *)pos2;
    *pos2 = 0;
    *pos1 = (int)((char *)pos2 - nextPos1);

    caseTable = (unsigned int (*)[2])TempMallocAlignStrict(0);
    numCases = 0;
    for (caseStatement = SCRCG->currentCaseStatement;
         caseStatement;
         caseStatement = caseStatement->next) {
        *(unsigned int *)TempMallocAlign(4) = caseStatement->name;
#if defined(_M_X64) || defined(__x86_64__)
        /* store a program-buffer offset: the runtime (VM_CandidateHandleSwitch) decodes the
           case codepos with SCR_CODEPOS_PTR. A truncated raw pointer would jump to garbage. */
        *(unsigned int *)TempMallocAlign(4) = SCR_CODEPOS_ENC(caseStatement->codePos);
#else
        *(unsigned int *)TempMallocAlign(4) = (unsigned int)(uintptr_t)caseStatement->codePos;
#endif
        ++numCases;
    }

    *pos2 = (unsigned short)numCases;
    qsort(caseTable, numCases, 8, (int (*)(const void *, const void *))CompareCaseInfo);

    if (numCases > 1) {
        for (i = 0; i < numCases - 1; ++i) {
            unsigned int value = caseTable[i][0];
            if (value == caseTable[i + 1][0] && value != 0) {
                for (caseStatement = SCRCG->currentCaseStatement;
                     caseStatement;
                     caseStatement = caseStatement->next) {
                    if (caseStatement->name == value) {
                        CompileError(caseStatement->sourcePos, "duplicate case expression");
                        return 0;
                    }
                }
            }
        }
    }

    nextCodePos = TempMalloc(0);
    for (breakStatement = SCRCG->currentBreakStatement;
         breakStatement;
         breakStatement = breakStatement->next) {
        *(int *)breakStatement->codePos = (int)(nextCodePos - breakStatement->nextCodePos);
    }

    SCRCG->bCanIgnoreCase = bOldCanIgnoreCase;
    SCRCG->currentCaseStatement = oldCaseStatement;
    SCRCG->bCanBreak = bOldCanBreak;
    SCRCG->bCanIgnoreBreak = bOldCanIgnoreBreak;
    SCRCG->currentBreakStatement = oldBreakStatement;

    return 0;
}

static unsigned int __attribute_regparm__(3)
    EmitThreadInternal(unsigned int threadId, uintptr_t val, unsigned int sourcePos, unsigned int endSourcePos, scr_block_t *block)
{
    intptr_t *funcNode;
    unsigned int name;
    intptr_t *paramList;
    intptr_t *paramNode;
    intptr_t *stmtList;
    intptr_t *stmtNode;
    int stackUse;

    funcNode = (intptr_t *)(uintptr_t)val;
    SCRCG->threadId = threadId;

    AddThreadStartOpcodePos(sourcePos);

    SCRCG->cumulOffset = 0;
    SCRCG->maxOffset = 0;
    SCRCG->maxCallOffset = 0;

    name = (unsigned int)funcNode[1];
    if (SCRCP->developer_statement == SCRCOMP_SCR_DEV_IGNORE) {
        if (!SCRCG->bConstRefCount) {
            SL_RemoveRefToString(name);
        }
    } else {
        if (SCRCG->bConstRefCount) {
            SL_AddRefToString(name);
        }
        SL_TransferRefToUser(name, 2);
    }

    paramList = (intptr_t *)funcNode[2];
    paramNode = (intptr_t *)((intptr_t *)paramList[0])[1];
    while (paramNode) {
        intptr_t *param;
        unsigned int paramName;
        unsigned int paramSourcePos;
        sval_t sourcePosVal;
        int localIndex;

        param = (intptr_t *)paramNode[0];
        paramName = (unsigned int)param[0];
        paramSourcePos = (unsigned int)param[1];
        sourcePosVal.sourcePosValue = paramSourcePos;

        localIndex = Scr_FindLocalVarIndex(paramName, sourcePosVal, 1, block);
        EmitOpcode(localIndex < 1 ? 0x31 : 0x32, 0, 0);
        if (localIndex) {
            *TempMalloc(1) = (char)localIndex;
        }
        AddOpcodePos(paramSourcePos, 0);

        paramNode = (intptr_t *)paramNode[1];
    }

    EmitOpcode(0x35, 0, 0);
    AddOpcodePos(sourcePos, 0);

    stmtList = (intptr_t *)funcNode[3];
    stmtNode = (intptr_t *)((intptr_t *)stmtList[0])[1];
    while (stmtNode) {
        intptr_t *nextNode;
        int lastStatement;
        sval_t stmtVal;

        nextNode = (intptr_t *)stmtNode[1];
        lastStatement = 0;

        if (!nextNode) {
            lastStatement = 1;
        } else if (!SCRVP->developer_script) {
            intptr_t *scanNode;

            scanNode = nextNode;
            while (scanNode) {
                intptr_t *scanStmt;

                scanStmt = (intptr_t *)scanNode[0];
                if (scanStmt[0] != 0x2d) {
                    break;
                }
                scanNode = (intptr_t *)scanNode[1];
            }

            if (!scanNode) {
                lastStatement = 1;
            }
        }

        stmtVal.node = stmtNode[0];
        EmitStatement(stmtVal, lastStatement, endSourcePos, block);

        if (!nextNode) {
            break;
        }
        stmtNode = nextNode;
    }

    EmitOpcode(0, 0, 0);
    AddOpcodePos(endSourcePos, 1);
    AddOpcodePos(0xfffffffe, 0);

    stackUse = (SCRCG->maxCallOffset << 5) + SCRCG->maxOffset;
    if (stackUse > 0x7ff) {
        CompileError(sourcePos, "function exceeds operand stack size");
    }

    return stackUse;
}

static intptr_t *ScriptListFirstNode(sval_t listVal)
{

    intptr_t *list = (intptr_t *)listVal.node;
    intptr_t *node;
    intptr_t *value;

    if (!list)
        return NULL;

    node = (intptr_t *)list[0];
    if (!node)
        return NULL;

    value = (intptr_t *)node[0];
    if (value && value[0] == 0)
        node = (intptr_t *)node[1];

    return node;
}

static void Scr_EvalVariableValue(unsigned int id, VariableValue *value)
{
    unsigned int type = (*(unsigned int *)(scrVarGlob + id * 16 + 8)) & 0x1f;

    value->u = *(VariableUnion *)(scrVarGlob + id * 16 + 4);
    value->type = (int)type;

    if (type == 1) {
        *(unsigned short *)(scrVarGlob + value->u.pointerValue * 16 + 4) += 1;
    } else if (type == 2 || type == 3) {
        SL_AddRefToString(value->u.stringValue);
    } else if (type == 4 && !*((const byte *)SCR_VEC_PTR(value->u) - 1)) {
        *(unsigned short *)((byte *)SCR_VEC_PTR(value->u) - 4) += 1;
    }
}

static unsigned int Scr_FindNextChildVariable(unsigned int parentId, unsigned int afterId)
{
    unsigned int id;

    for (id = afterId + 1; id <= SCRCOMP_MAX_VARIABLES; id++) {
        unsigned int status = *(unsigned int *)(scrVarGlob + id * 16 + 8);
        unsigned int type = status & 0x1f;
        unsigned int name = status >> 8;

        if (!type || type > 0xe || !name)
            continue;

        if (FindVariable(parentId, name) == id)
            return id;
    }

    return 0;
}

static void Scr_BlockAddLocalVar(scr_block_t *block, unsigned int name)
{
    int i;

    if (block->abortLevel)
        return;

    for (i = 0; i < block->localVarsCount; i++) {
        if (block->localVars[i].name == name)
            return;
    }

    if (block->localVarsCount > 63)
        Com_Error(1, "LOCAL_VAR_STACK_SIZE exceeded");

    block->localVars[block->localVarsCount].name = name;
    block->localVarsCount++;
}

static void Scr_InitThreadBlock(intptr_t *funcNode, scr_block_t *block)
{
    sval_t paramListVal;
    intptr_t *listNode;

    memset(block, 0, sizeof(*block));
    funcNode[6] = (intptr_t)block;

    paramListVal.node = funcNode[2];
    for (listNode = ScriptListFirstNode(paramListVal);
         listNode;
         listNode = (intptr_t *)listNode[1]) {
        intptr_t *paramNode = (intptr_t *)listNode[0];

        if (paramNode)
            Scr_BlockAddLocalVar(block, (unsigned int)paramNode[0]);
    }
}

static void Scr_CalcThreadLocalVars(intptr_t *funcNode, scr_block_t *block)
{
    sval_t stmtListVal;
    intptr_t *listNode;

    stmtListVal.node = funcNode[3];
    for (listNode = ScriptListFirstNode(stmtListVal);
         listNode;
         listNode = (intptr_t *)listNode[1]) {
        uintptr_t stmt = (uintptr_t)listNode[0];

        if (stmt)
            Scr_CalcLocalVarsStatement(stmt, block);
    }
}

static void Scr_ResetThreadCompileState(void)
{
    SCRCG->bCanIgnoreCase = 0;
    SCRCG->currentCaseStatement = NULL;
    SCRCG->bCanBreak = 0;
    SCRCG->bCanIgnoreBreak = 0;
    SCRCG->currentBreakStatement = NULL;
    SCRCG->bCanContinue = 0;
    SCRCG->bCanIgnoreContinue = 0;
    SCRCG->currentContinueStatement = NULL;
    SCRCG->breakChildBlocks = NULL;
    SCRCG->continueChildBlocks = NULL;
}

static void EmitFirstThreadMarker(int developerThread)
{
    if (!SCRCG->firstThread[developerThread])
        return;

    SCRCG->firstThread[developerThread] = 0;
    EmitOpcode(0, 0, 0);
    AddOpcodePos(0, 0);
    AddOpcodePos((unsigned int)-2, 0);
}

static void SetThreadBeginPosition(unsigned int threadId)
{
    unsigned int posId;
    VariableUnion *pos;

    posId = FindVariable(threadId, 1);
    if (!posId)
        return;

    pos = GetVariableValueAddress(posId);
    SCR_CODEPOS_SET(*pos, TempMalloc(0));
}

static void SpecifyThreadListPositions(sval_t threadListVal)
{
    intptr_t *listNode;

    SCRCG->in_developer_thread = 0;

    for (listNode = ScriptListFirstNode(threadListVal);
         listNode;
         listNode = (intptr_t *)listNode[1]) {
        intptr_t *node = (intptr_t *)listNode[0];
        unsigned int name;
        unsigned int threadId;
        int type;

        if (!node)
            continue;

        if (node[0] == SCRCOMP_NODE_DEV_BEGIN) {
            if (SCRCG->in_developer_thread) {
                CompileError((unsigned int)node[1], "cannot recurse /#");
                continue;
            }

            SCRCG->in_developer_thread = 1;
            SCRCG->developer_thread_sourcePos = (unsigned int)node[1];
            continue;
        }

        if (node[0] == SCRCOMP_NODE_DEV_END) {
            if (!SCRCG->in_developer_thread) {
                CompileError((unsigned int)node[1], "#/ has no matching /#");
                continue;
            }

            SCRCG->in_developer_thread = 0;
            continue;
        }

        if (node[0] != SCRCOMP_NODE_FUNCTION)
            continue;

        if (SCRCG->in_developer_thread) {
            if (!SCRVP->developer_script)
                continue;
            type = SCRCOMP_VAR_DEVELOPER_CODEPOS;
        } else {
            type = SCRCOMP_VAR_CODEPOS;
        }

        name = (unsigned int)node[1];
        threadId = GetObjectA(GetVariable(SCRCG->fileId, name));
        SpecifyThreadPosition(threadId, name, (unsigned int)node[4], type);
    }

    if (SCRCG->in_developer_thread)
        CompileError(SCRCG->developer_thread_sourcePos, "/# has no matching #/");
}

static void EmitIncludeList_new(sval_t includeListVal)
{
    intptr_t *listNode;

    for (listNode = ScriptListFirstNode(includeListVal);
         listNode;
         listNode = (intptr_t *)listNode[1]) {
        intptr_t *includeNode = (intptr_t *)listNode[0];
        PrecacheEntry *entry;
        unsigned int filename;

        if (!includeNode || includeNode[0] != SCRCOMP_NODE_INCLUDE)
            continue;

        filename = Scr_CreateCanonicalFilename(
            SL_ConvertToString((unsigned int)includeNode[1]));
        if (!SCRCG->bConstRefCount)
            SL_RemoveRefToString((unsigned int)includeNode[1]);

        SL_AddRefToString(filename);
        if (!SCRCG->bConstRefCount)
            SL_RemoveRefToString(filename);

        entry = SCRCG->precachescriptList;
        entry->filename = (scr_string_t)filename;
        entry->include = 1;
        entry->sourcePos = (unsigned int)includeNode[2];
        SCRCG->precachescriptList = entry + 1;

        GetObjectA(GetVariable(SCRCP->scripts, filename));
    }
}

static void EmitThreadList_new(sval_t threadListVal)
{
    intptr_t *listNode;

    SCRCG->in_developer_thread = 0;
    SCRCG->firstThread[0] = 1;
    SCRCG->firstThread[1] = 1;

    for (listNode = ScriptListFirstNode(threadListVal);
         listNode;
         listNode = (intptr_t *)listNode[1]) {
        intptr_t *node = (intptr_t *)listNode[0];
        unsigned int name;
        unsigned int sourcePos;
        unsigned int endSourcePos;
        unsigned int threadId;
        unsigned int varId;
        char *savedPos;
        unsigned int savedChecksum;
        scr_block_t block;

        if (!node)
            continue;

        if (node[0] == SCRCOMP_NODE_DEV_BEGIN) {
            SCRCG->in_developer_thread = 1;
            continue;
        }

        if (node[0] == SCRCOMP_NODE_DEV_END) {
            SCRCG->in_developer_thread = 0;
            continue;
        }

        if (node[0] == SCRCOMP_NODE_USINGTREE) {
            if (SCRCG->in_developer_thread) {
                CompileError((unsigned int)node[2], "cannot put #using_animtree inside /# ... #/ comment");
                continue;
            }

            Scr_UsingTree(SL_ConvertToString((unsigned int)node[1]),
                          (unsigned int)node[3]);
            if (!SCRCG->bConstRefCount)
                SL_RemoveRefToString((unsigned int)node[1]);
            continue;
        }

        if (node[0] != SCRCOMP_NODE_FUNCTION)
            continue;

        name = (unsigned int)node[1];
        sourcePos = (unsigned int)node[4];
        endSourcePos = (unsigned int)node[5];

        SCRCG->forceNotCreate = 0;
        Scr_InitThreadBlock(node, &block);
        Scr_CalcThreadLocalVars(node, &block);

        if (SCRCG->in_developer_thread && !SCRVP->developer_script) {
            savedPos = TempMalloc(0);
            savedChecksum = SCRVP->checksum;
            SCRCP->developer_statement = SCRCOMP_SCR_DEV_IGNORE;
            Scr_ResetThreadCompileState();
            EmitFirstThreadMarker(1);
            EmitThreadInternal(0, (uintptr_t)node, sourcePos, endSourcePos, &block);
            TempMemorySetPos(savedPos);
            SCRVP->checksum = savedChecksum;
            SCRCP->developer_statement = SCRCOMP_SCR_DEV_NO;
            continue;
        }

        SCRCP->developer_statement = SCRCG->in_developer_thread
                                         ? SCRCOMP_SCR_DEV_ON
                                         : SCRCOMP_SCR_DEV_NO;

        Scr_ResetThreadCompileState();
        EmitFirstThreadMarker(SCRCG->in_developer_thread ? 1 : 0);

        threadId = FindObject(FindVariable(SCRCG->fileId, name));

        SetThreadBeginPosition(threadId);
        EmitThreadInternal(threadId, (uintptr_t)node, sourcePos, endSourcePos, &block);
        SCRCP->developer_statement = SCRCOMP_SCR_DEV_NO;
    }
}

static void LinkFile_new(unsigned int fileId)
{
    unsigned int varId;
    VariableValue emptyValue;

    emptyValue.u.intValue = 0;
    emptyValue.type = 0;

    for (varId = FindNextSibling(fileId);
         varId;
         varId = FindNextSibling(varId)) {
        unsigned int threadId;
        unsigned int posId;
        VariableValue pos;

        threadId = FindObject(varId);
        posId = FindVariable(threadId, 1);
        if (!posId) {
#    ifdef LT_TRACE
            fprintf(stderr, "[LINKFILE noposid] fileId=%u varId=%u name='%s' threadId=%u\n",
                    fileId, varId, SL_ConvertToString(GetVariableName(varId)), threadId);
#    endif
            LinkThread(threadId, &emptyValue, 1);
            continue;
        }

        Scr_EvalVariableValue(posId, &pos);
        if (pos.type == SCRCOMP_VAR_INCLUDE_CODEPOS)
            SetVariableValue(varId, &emptyValue);
        else {
#    ifdef LT_TRACE
            if (!pos.type)
                fprintf(stderr, "[LINKFILE nodef] fileId=%u varId=%u name='%s' threadId=%u posId=%u pos.type=%d\n",
                        fileId, varId, SL_ConvertToString(GetVariableName(varId)), threadId, posId, pos.type);
#    endif
            LinkThread(threadId, &pos, 1);
        }
    }
}

void ScriptCompile(sval_t val, unsigned int fileId, unsigned int scriptId)
{
    PrecacheEntry *precachescript, *precachescript2, *precachescriptList;
    unsigned int filename, name;
    unsigned int posId, includePosId, includeFilePosId, includeThreadPosId, toThreadCountId;
    VariableValue includePos, value;
    VariableUnion *pos;

    sval_t *top = (sval_t *)val.node;
    sval_t include_list_val, thread_list_val;
    int i, j, func_count;

    SCRCG->fileId = fileId;
    SCRCG->bConstRefCount = 0;

    SCRAP->animTreeIndex = 0;
    SCRCP->developer_statement = SCRCOMP_SCR_DEV_NO;

    func_count = SCRCP->far_function_count;
    if (func_count > 0) {
        precachescriptList = (PrecacheEntry *)Z_MallocInternal(
            sizeof(*precachescriptList) * func_count);
    } else {
        precachescriptList = NULL;
    }

    SCRCG->precachescriptList = precachescriptList;

    if (precachescriptList) {
        precachescriptList->next = SCRCG->precachescriptListHead;
        SCRCG->precachescriptListHead = precachescriptList;
    }

    include_list_val = top[0];
    thread_list_val = top[1];
    EmitIncludeList_new(include_list_val);
    SpecifyThreadListPositions(thread_list_val);
    EmitThreadList_new(thread_list_val);

    SCRCP->programLen = (char *)TempMalloc(0) - SCRVP->programBuffer;
    Hunk_ClearTempMemoryHigh();

    func_count = SCRCP->far_function_count;

    for (i = 0; i < func_count; i++) {
        precachescript = &precachescriptList[i];
        filename = precachescript->filename;

        includeFilePosId = Scr_LoadScript(SL_ConvertToString(filename));

        if (!includeFilePosId) {
            CompileError(precachescript->sourcePos,
                         "Could not find script '%s'",
                         SL_ConvertToString(filename));
            return;
        }

        SL_RemoveRefToString(filename);

        if (!precachescript->include)
            continue;

        for (j = i + 1; j < func_count; j++) {
            precachescript2 = &precachescriptList[j];
            if (!precachescript2->include)
                break;
            if (precachescript2->filename == filename) {
                CompileError(precachescript2->sourcePos, "Duplicate #include");
                return;
            }
        }

        precachescript->include = 0;

        for (includePosId = FindNextSibling(includeFilePosId);
             includePosId;
             includePosId = FindNextSibling(includePosId)) {
            if (GetVarType(includePosId) != SCRCOMP_VAR_POINTER)
                continue;

            includeThreadPosId = FindVariable(FindObject(includePosId), 1);
            if (!includeThreadPosId)
                continue;

            Scr_EvalVariableValue(includeThreadPosId, &includePos);

            if (includePos.type == SCRCOMP_VAR_INCLUDE_CODEPOS)
                continue;

            name = GetVariableName(includePosId);
            toThreadCountId = GetObjectA(GetVariable(fileId, name));

            posId = SpecifyThreadPosition(toThreadCountId, name,
                                          precachescript->sourcePos,
                                          SCRCOMP_VAR_INCLUDE_CODEPOS);
            pos = GetVariableValueAddress(posId);
            *pos = *GetVariableValueAddress(includeThreadPosId);

            LinkThread(toThreadCountId, &includePos, 0);
        }
    }

    if (precachescriptList) {
        SCRCG->precachescriptListHead = precachescriptList->next;
        Z_FreeInternal(precachescriptList);
    }

    LinkFile_new(fileId);

    value.type = SCRCOMP_VAR_INTEGER;
    value.u.intValue = 1;
    SetVariableValue(scriptId, &value);
}

static void Scr_MergeChildBlocks(scr_block_t *childBlock, const scr_block_t *parentBlock)
{
    int j;

    for (j = 0; j < childBlock->localVarsPublicCount || j < parentBlock->localVarsCreateCount; ++j) {
        unsigned int name = parentBlock->localVars[j].name;
        int insert = j;

        while (insert < childBlock->localVarsCount && childBlock->localVars[insert].name != name) {
            ++insert;
        }

        if (insert == childBlock->localVarsCount) {
            if (childBlock->localVarsCount > 63) {
                Com_Error(1, "LOCAL_VAR_STACK_SIZE exceeded");
            }
            ++childBlock->localVarsCount;
        }

        if (insert >= childBlock->localVarsPublicCount) {
            ++childBlock->localVarsPublicCount;
        }

        while (insert > j) {
            childBlock->localVars[insert] = childBlock->localVars[insert - 1];
            --insert;
        }

        childBlock->localVars[j].name = name;
        if (parentBlock->localVarsInitBits[j >> 3] & (1 << (j & 7))) {
            childBlock->localVarsInitBits[j >> 3] |= (byte)(1 << (j & 7));
        }
    }

    childBlock->localVarsCreateCount = parentBlock->localVarsCreateCount;
    childBlock->abortLevel = 0;
}

static void Scr_EmitLocalVarsAtEnd(scr_block_t *block)
{
    if (!block->abortLevel) {
        int removeCount = block->localVarsCreateCount - block->localVarsPublicCount;
        if (removeCount) {
            char *out;

            EmitOpcode(0x17, 0, 0);
            out = TempMalloc(1);
            SCRCG->codePos = (byte *)out;
            *out = (char)removeCount;
            block->localVarsCreateCount = block->localVarsPublicCount;
        }
    }
}

static void Scr_EmitLocalVarsRemove(scr_block_t *block, int count)
{
    if (!block->abortLevel) {
        int removeCount = block->localVarsCreateCount - count;
        if (removeCount) {
            char *out;

            EmitOpcode(0x17, 0, 0);
            out = TempMalloc(1);
            SCRCG->codePos = (byte *)out;
            *out = (char)removeCount;
            block->localVarsCreateCount = block->localVarsPublicCount;
        }
    }
}

static void Scr_MergeBranchChildBlocks(scr_block_t *block, scr_block_t **childBlocks, int childCount)
{
    int localVarsCreateCount;
    int i;
    int child;

    if (!childCount) {
        return;
    }

    localVarsCreateCount = childBlocks[0]->localVarsCreateCount;
    for (child = 1; child < childCount; ++child) {
        if (localVarsCreateCount > childBlocks[child]->localVarsCreateCount) {
            localVarsCreateCount = childBlocks[child]->localVarsCreateCount;
        }
    }

    block->localVarsCreateCount = localVarsCreateCount;

    for (i = 0; i < localVarsCreateCount; ++i) {
        int byteIndex = i >> 3;
        byte bit = (byte)(1 << (i & 7));

        if (block->localVarsInitBits[byteIndex] & bit) {
            continue;
        }

        for (child = 0; child < childCount; ++child) {
            if (!(childBlocks[child]->localVarsInitBits[byteIndex] & bit)) {
                break;
            }
        }

        if (child == childCount) {
            block->localVarsInitBits[byteIndex] |= bit;
        }
    }
}

static intptr_t *ScriptStatementListFirstExecNode(sval_t val)
{
    intptr_t *list = (intptr_t *)(uintptr_t)val.node;
    intptr_t *head;

    if (!list) {
        return NULL;
    }

    head = (intptr_t *)(uintptr_t)list[0];
    if (!head) {
        return NULL;
    }

    return (intptr_t *)(uintptr_t)head[1];
}

static unsigned int __attribute_regparm__(3)
    EmitDeveloperStatementList(sval_t val, unsigned int sourcePos, scr_block_t *block, sval_t *devStatBlock)
{
    scr_block_t *devBlock;
    unsigned int savedChecksum;
    intptr_t *listNode;

    if (SCRCP->developer_statement) {
        CompileError(sourcePos, "cannot recurse /#");
        return 0;
    }

    savedChecksum = SCRVP->checksum;
    devBlock = devStatBlock->block;
    Scr_MergeChildBlocks(devBlock, block);

    if (SCRVP->developer_script) {
        SCRCP->developer_statement = SCRCOMP_SCR_DEV_ON;
        for (listNode = ScriptStatementListFirstExecNode(val); listNode; listNode = (intptr_t *)(uintptr_t)listNode[1]) {
            sval_t stmt;
            stmt.node = listNode[0];
            EmitStatement(stmt, 0, 0, devBlock);
        }
        Scr_EmitLocalVarsAtEnd(devBlock);
    } else {
        char *savedPos = TempMalloc(0);

        SCRCP->developer_statement = SCRCOMP_SCR_DEV_IGNORE;
        for (listNode = ScriptStatementListFirstExecNode(val); listNode; listNode = (intptr_t *)(uintptr_t)listNode[1]) {
            sval_t stmt;
            stmt.node = listNode[0];
            EmitStatement(stmt, 0, 0, devBlock);
        }
        TempMemorySetPos(savedPos);
    }

    SCRCP->developer_statement = SCRCOMP_SCR_DEV_NO;
    SCRVP->checksum = savedChecksum;
    return 0;
}

#    if 0

static __attribute__((naked))
unsigned int EmitDeveloperStatementList(sval_t val, scr_block_t *block, sval_t *devStatBlock)
{
    __asm__ __volatile__ (
        "pushl %ebp\n"
        "movl %esp, %ebp\n"
        "pushl %edi\n"
        "pushl %esi\n"
        "pushl %ebx\n"
        "subl $0x3c, %esp\n"
        "movl %eax, -0x2c(%ebp)\n"
        "movl %ecx, -0x30(%ebp)\n"

        "movl scrCompilePub+40, %ebx\n"
        "testl %ebx, %ebx\n"
        "jne .Lf9b20c_0009b40e\n"
        "movl %ecx, %edx\n"
        "movl imp_scrVarPub, %eax\n"
        "movl 0x3c(%eax), %eax\n"
        "movl %eax, -0x28(%ebp)\n"
        "movl 8(%ebp), %eax\n"
        "movl (%eax), %esi\n"
        "movl %esi, -0x20(%ebp)\n"
        "addl $0x18, %edx\n"
        "movl %edx, -0x1c(%ebp)\n"
        "xorl %edi, %edi\n"

        ".Lf9b20c_0009b246:\n"
        "cmpl 8(%esi), %edi\n"
        "jl .Lf9b20c_0009b259\n"
        "movl -0x30(%ebp), %edx\n"
        "movl 4(%edx), %eax\n"
        "cmpl %eax, %edi\n"
        "jge .Lf9b20c_0009b33c\n"

        ".Lf9b20c_0009b259:\n"
        "movl -0x1c(%ebp), %ebx\n"
        "movl (%ebx), %ebx\n"
        "movl %ebx, -0x24(%ebp)\n"

        "movl 0xc(%esi), %ebx\n"
        "movl %ebx, -0x34(%ebp)\n"
        "cmpl %ebx, %edi\n"
        "jge .Lf9b20c_0009b309\n"
        "movl -0x24(%ebp), %edx\n"
        "movl -0x20(%ebp), %eax\n"
        "cmpl %edx, 0x18(%eax)\n"
        "je .Lf9b20c_0009b335\n"
        "leal 0x18(%esi, %edi, 4), %edx\n"
        "movl %edi, %ecx\n"
        ".Lf9b20c_0009b284:\n"
        "addl $1, %ecx\n"
        "cmpl %ecx, %ebx\n"
        "je .Lf9b20c_0009b309\n"
        "movl 4(%edx), %eax\n"
        "addl $4, %edx\n"
        "cmpl -0x24(%ebp), %eax\n"
        "jne .Lf9b20c_0009b284\n"
        ".Lf9b20c_0009b296:\n"
        "testl %ecx, %ecx\n"
        "js .Lf9b20c_0009b309\n"
        "movl %ecx, %ebx\n"
        ".Lf9b20c_0009b29c:\n"
        "movl 8(%esi), %eax\n"
        "cmpl %eax, %ebx\n"
        "jl .Lf9b20c_0009b2a9\n"
        "addl $1, %eax\n"
        "movl %eax, 8(%esi)\n"
        ".Lf9b20c_0009b2a9:\n"
        "cmpl %ebx, %edi\n"
        "jge .Lf9b20c_0009b2cd\n"
        "leal (, %ebx, 4), %eax\n"
        "leal 0x14(%eax, %esi), %ecx\n"
        "leal 0x18(%eax, %esi), %edx\n"
        ".Lf9b20c_0009b2bc:\n"
        "subl $1, %ebx\n"
        "movl (%ecx), %eax\n"
        "movl %eax, (%edx)\n"
        "subl $4, %ecx\n"
        "subl $4, %edx\n"
        "cmpl %ebx, %edi\n"
        "jl .Lf9b20c_0009b2bc\n"
        ".Lf9b20c_0009b2cd:\n"
        "movl -0x24(%ebp), %ebx\n"
        "movl -0x20(%ebp), %edx\n"
        "movl %ebx, 0x18(%edx)\n"
        "movl %edi, %edx\n"
        "sarl $3, %edx\n"
        "movl %edi, %ecx\n"
        "andl $7, %ecx\n"
        "movl -0x30(%ebp), %ebx\n"
        "movzbl 0x10(%edx, %ebx), %eax\n"
        "sarl %cl, %eax\n"
        "testb $1, %al\n"
        "je .Lf9b20c_0009b2f9\n"
        "movl $1, %eax\n"
        "shll %cl, %eax\n"
        "orb %al, 0x10(%edx, %esi)\n"
        ".Lf9b20c_0009b2f9:\n"
        "addl $1, %edi\n"
        "addl $4, -0x20(%ebp)\n"
        "addl $4, -0x1c(%ebp)\n"
        "jmp .Lf9b20c_0009b246\n"
        ".Lf9b20c_0009b309:\n"
        "cmpl $0x3f, %ebx\n"
        "jg .Lf9b20c_0009b319\n"
        "movl -0x34(%ebp), %eax\n"
        ".Lf9b20c_0009b311:\n"
        "addl $1, %eax\n"
        "movl %eax, 0xc(%esi)\n"
        "jmp .Lf9b20c_0009b29c\n"
        ".Lf9b20c_0009b319:\n"
        "movl $"LOCAL_VAR_STACK_SIZE exceeded", 4(%esp)\n"
        "movl $1, (%esp)\n"
        "calll Com_Error\n"
        "movl 0xc(%esi), %eax\n"
        "movl %eax, -0x34(%ebp)\n"
        "jmp .Lf9b20c_0009b311\n"
        ".Lf9b20c_0009b335:\n"
        "movl %edi, %ecx\n"
        "jmp .Lf9b20c_0009b296\n"
        ".Lf9b20c_0009b33c:\n"
        "movl %eax, 4(%esi)\n"
        "movl $0, (%esi)\n"

        "movl imp_scrVarPub, %eax\n"
        "cmpb $0, 0xb(%eax)\n"
        "je .Lf9b20c_0009b3ba\n"
        "movl $1, scrCompilePub+40\n"
        "movl 8(%ebp), %ebx\n"
        "movl (%ebx), %esi\n"

        "movl -0x2c(%ebp), %edx\n"
        "movl (%edx), %eax\n"
        "movl 4(%eax), %eax\n"
        "testl %eax, %eax\n"
        "jne .Lf9b20c_0009b36f\n"
        "jmp .Lf9b20c_0009b389\n"
        ".Lf9b20c_0009b36d:\n"
        "movl %ebx, %eax\n"
        ".Lf9b20c_0009b36f:\n"
        "movl 4(%eax), %ebx\n"
        "movl (%eax), %eax\n"
        "movl %esi, (%esp)\n"
        "xorl %ecx, %ecx\n"
        "xorl %edx, %edx\n"
        "calll EmitStatement\n"
        "testl %ebx, %ebx\n"
        "jne .Lf9b20c_0009b36d\n"
        "movl 8(%ebp), %ebx\n"
        "movl (%ebx), %esi\n"

        ".Lf9b20c_0009b389:\n"
        "movl (%esi), %eax\n"
        "testl %eax, %eax\n"
        "jne .Lf9b20c_0009b39d\n"
        "movl 4(%esi), %eax\n"
        "movl %eax, %ebx\n"
        "subl 8(%esi), %ebx\n"
        "jne .Lf9b20c_0009b426\n"

        ".Lf9b20c_0009b39d:\n"
        "movl $0, scrCompilePub+40\n"
        "movl imp_scrVarPub, %eax\n"
        "movl -0x28(%ebp), %edx\n"
        "movl %edx, 0x3c(%eax)\n"

        "addl $0x3c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"

        ".Lf9b20c_0009b3ba:\n"
        "movl $0, (%esp)\n"
        "calll TempMalloc\n"
        "movl %eax, %edi\n"
        "movl $2, scrCompilePub+40\n"
        "movl 8(%ebp), %ebx\n"
        "movl (%ebx), %esi\n"

        "movl -0x2c(%ebp), %edx\n"
        "movl (%edx), %eax\n"
        "movl 4(%eax), %eax\n"
        "testl %eax, %eax\n"
        "jne .Lf9b20c_0009b3ef\n"

        "movl %edi, (%esp)\n"
        "calll TempMemorySetPos\n"
        "jmp .Lf9b20c_0009b39d\n"

        ".Lf9b20c_0009b3ed:\n"
        "movl %ebx, %eax\n"
        ".Lf9b20c_0009b3ef:\n"
        "movl 4(%eax), %ebx\n"
        "movl (%eax), %eax\n"
        "movl %esi, (%esp)\n"
        "xorl %ecx, %ecx\n"
        "xorl %edx, %edx\n"
        "calll EmitStatement\n"
        "testl %ebx, %ebx\n"
        "jne .Lf9b20c_0009b3ed\n"

        "movl %edi, (%esp)\n"
        "calll TempMemorySetPos\n"
        "jmp .Lf9b20c_0009b39d\n"
        ".Lf9b20c_0009b40e:\n"
        "movl $"cannot recurse /#", 4(%esp)\n"
        "movl %edx, (%esp)\n"
        "calll CompileError\n"

        "addl $0x3c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"

        ".Lf9b20c_0009b426:\n"
        "xorl %ecx, %ecx\n"
        "xorl %edx, %edx\n"
        "movl $0x17, %eax\n"
        "calll EmitOpcode\n"
        "movl $1, (%esp)\n"
        "calll TempMalloc\n"
        "movl %eax, scrCompileGlob\n"
        "movb %bl, (%eax)\n"
        "movl 8(%esi), %eax\n"
        "movl %eax, 4(%esi)\n"
        "jmp .Lf9b20c_0009b39d\n"
    );
}
#    endif
