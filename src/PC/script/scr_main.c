#include "common_types.h"
#include "imports.h"

extern byte *__DefaultRuneLocale;
extern int ___maskrune(int ch, unsigned int mask);

extern void SL_TransferRefToUser(unsigned int stringValue, unsigned int user);
extern void SL_RemoveRefToString(unsigned int stringValue);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern unsigned int SL_FindString(const char *str);
extern unsigned int SL_GetString_(const char *str, int user, int flags);
extern unsigned int SL_FindLowercaseString(const char *str);
extern void SL_ShutdownSystem(int user);

extern unsigned int Scr_CreateCanonicalFilename(const char *filename);
extern byte *Scr_AddSourceBuffer(const char *filename, const char *extFilename, byte *oldFilename, int flag);
extern int Scr_AllocArray(void);
extern void Scr_LoadAnimTreeAtIndex(int index, Alloc_t Alloc, int user);
extern void Scr_InitOpcodeLookup(void);
extern void Scr_ShutdownOpcodeLookup(void);
extern void Scr_ClearErrorMessage(void);
extern unsigned long long Scr_EvalVariable(unsigned int varId);

extern void ScriptParse(sval_t *parseData, int flag);
extern void ScriptCompile(sval_t parseData, unsigned int compiledObj, unsigned int scriptId);

extern unsigned int FindVariable(unsigned int parent, unsigned int name);
extern unsigned int GetNewVariable(unsigned int parent, unsigned int name);
extern unsigned int GetVariable(unsigned int parent, unsigned int name);
extern unsigned int FindObject(unsigned int varId);
extern unsigned int GetObjectA(unsigned int varId);
extern unsigned int GetArray(unsigned int id);
extern unsigned int GetVarType(unsigned int varId);
extern void ClearObject(unsigned int objId);
extern void RemoveRefToObject(unsigned int objId);

extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern unsigned int FindNextSibling(unsigned int id);
extern unsigned int GetVariableName(unsigned int id);
extern unsigned char scrVarGlob[];

#define SCR_MAIN_MAX_SCRIPT_VARIABLES 65534

static unsigned int Scr_FindNextChildVariableForDump(unsigned int parentId, unsigned int afterId)
{
    unsigned int id;

    for (id = afterId + 1; id <= SCR_MAIN_MAX_SCRIPT_VARIABLES; id++) {
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

void DumpCompiledObject(const char *label, unsigned int compiledObj)
{
    (void)label;
    (void)compiledObj;
}
extern byte *TempMalloc(int size);
extern void TempMemoryReset(void);
extern void *Hunk_AllocLowInternal(int size);
extern void *Hunk_AllocInternal(int size);
extern void Hunk_ClearToMark(int mark);
extern int Hunk_SetMark(void);
extern void Hunk_ConvertTempToPermLowInternal(void);

int Scr_IsInOpcodeMemory(const char *pos);
Bool Scr_IsIdentifier(const char *token);
unsigned int SL_TransferToCanonicalString(unsigned int stringValue);
void Scr_BeginLoadAnimTrees(int user);
int Scr_ScanFile(char *buf, int max_size);
unsigned int Scr_LoadScript(const char *filename);
void Scr_PostCompileScripts(void);
void Scr_EndLoadScripts(void);
void Scr_PrecacheAnimTrees(Alloc_t Alloc, int user);
void Scr_EndLoadAnimTrees(void);
void Scr_FreeScripts(int sys);
scr_func_t Scr_GetFunctionHandle(const char *filename, const char *name);
void Scr_BeginLoadScripts(void);
unsigned int SL_GetCanonicalString(const char *str);

int Scr_IsInOpcodeMemory(const char *pos)
{
    struct scrVarPub_t *scrVarPub = (struct scrVarPub_t *)imp_scrVarPub;
    struct scrCompilePub_t *scrCompPub = (struct scrCompilePub_t *)imp_scrCompilePub;

    /* was (unsigned int)(pos - programBuffer): the 64-bit ptr diff truncated to 32 bits on
       x64, giving false positives for out-of-range pos. Use uintptr_t. */
    return (uintptr_t)(pos - scrVarPub->programBuffer) < (uintptr_t)(unsigned int)scrCompPub->programLen;
}

Bool Scr_IsIdentifier(const char *token)
{
    byte *runeLocale = *(byte **)&__DefaultRuneLocale;
    const char *s = token;
    char ch = *s;

    while (ch) {
        int isAlnum;
        int sch = (signed char)ch;

        if (sch & ~0x7f) {
            isAlnum = ___maskrune(sch, 0x500) != 0;
        } else {
            isAlnum = (*(int *)(runeLocale + 0x34 + sch * 4) & 0x500) != 0;
        }

        if (!isAlnum && ch != '_')
            return 0;

        s++;
        ch = *s;
    }

    return 1;
}

unsigned int SL_TransferToCanonicalString(unsigned int stringValue)
{
    struct scrCompilePub_t *scrCompPub = (struct scrCompilePub_t *)imp_scrCompilePub;
    struct scrVarPub_t *scrVarPub;
    unsigned short *entry;
    unsigned short val;

    SL_TransferRefToUser(stringValue, 2);

    entry = scrCompPub->canonicalStrings + stringValue;
    val = *entry;

    if (val != 0)
        return val;

    scrVarPub = (struct scrVarPub_t *)imp_scrVarPub;
    val = scrVarPub->canonicalStrCount + 1;
    scrVarPub->canonicalStrCount = val;
    *entry = val;
    return scrVarPub->canonicalStrCount;
}

static inline __attribute__((always_inline)) void Scr_BeginLoadAnimTrees_(int user)
{
    struct scrAnimPub_t *scrAnimPub = (struct scrAnimPub_t *)imp_scrAnimPub;
    struct scrCompilePub_t *scrCompPub;

    scrAnimPub->animtree_loading = 1;
    scrAnimPub->xanim_num[user] = 0;
    scrAnimPub->xanim_lookup[user][0].anims = NULL;

    scrAnimPub->animtrees = Scr_AllocArray();
    scrAnimPub->animtree_node = 0;

    scrCompPub = (struct scrCompilePub_t *)imp_scrCompilePub;
    scrCompPub->developer_statement = 0;
}

void Scr_BeginLoadAnimTrees(int user)
{
    Scr_BeginLoadAnimTrees_(user);
}

int Scr_ScanFile(char *buf, int max_size)
{
    struct scrCompilePub_t *g = (struct scrCompilePub_t *)imp_scrCompilePub;
    const char *src;
    char ch;
    int count;

    if (max_size <= 0)
        return 0;

    src = g->in_ptr;
    ch = *src;
    src++;
    g->in_ptr = src;

    if (ch == '\0') {
        count = 0;
        goto source_exhausted;
    }

    if (ch == '\n') {
        buf[0] = '\n';
        return 1;
    }

    count = 0;
    for (;;) {
        buf[count] = ch;
        count++;

        if (count == max_size) {
            if (ch == '\n') {
                buf[count] = '\n';
                return count + 1;
            }
            if (ch != '\0')
                return count;
            goto source_exhausted;
        }

        src = g->in_ptr;
        ch = *src;
        src++;
        g->in_ptr = src;

        if (ch == '\0')
            goto source_exhausted;

        if (ch == '\n') {
            buf[count] = '\n';
            return count + 1;
        }
    }

source_exhausted:
    if (g->parseBuf != NULL) {
        g->in_ptr = g->parseBuf;
        g->parseBuf = NULL;
    } else {
        g->in_ptr -= 1;
    }
    return count;
}

unsigned int Scr_LoadScript(const char *filename)
{
    struct scrCompilePub_t *scrCompPub = (struct scrCompilePub_t *)imp_scrCompilePub;
    unsigned int fileId;
    unsigned int scriptId;
    char extFilename[64];
    sval_t parseData;
    struct scrParserPub_t *parserPub;
    const char *oldSourceBuf;
    byte *oldFilename;
    byte *sourceBuf;
    const char *savedParserFilename;
    unsigned int compiledObj;
    unsigned int result;

    if (!filename) {
        return 0;
    }
    fileId = Scr_CreateCanonicalFilename(filename);

    result = FindVariable(scrCompPub->loadedscripts, fileId);
    if (result != 0) {

        SL_RemoveRefToString(fileId);
        result = FindVariable(scrCompPub->scripts, fileId);
        if (result == 0)
            return 0;
        return FindObject(result);
    }

    scriptId = GetNewVariable(scrCompPub->loadedscripts, fileId);
    SL_RemoveRefToString(fileId);

    Com_sprintf(extFilename, 0x40, "%s.gsc", SL_ConvertToString(fileId));

    parserPub = (struct scrParserPub_t *)imp_scrParserPub;
    oldSourceBuf = parserPub->sourceBuf;

    oldFilename = TempMalloc(0);
    sourceBuf = Scr_AddSourceBuffer(SL_ConvertToString(fileId), extFilename, oldFilename, 1);
    if (sourceBuf == NULL)
        return 0;

    ((struct scrAnimPub_t *)imp_scrAnimPub)->animTreeNames = 0;
    scrCompPub->far_function_count = 0;

    parserPub = (struct scrParserPub_t *)imp_scrParserPub;
    savedParserFilename = parserPub->scriptfilename;
    parserPub->scriptfilename = extFilename;

    scrCompPub->in_ptr = "+";
    scrCompPub->parseBuf = (const char *)sourceBuf;

    ScriptParse(&parseData, 0);

    compiledObj = GetArray(GetVariable(scrCompPub->scripts, fileId));
    DumpCompiledObject(extFilename, compiledObj);
    ScriptCompile(parseData, compiledObj, scriptId);
    DumpCompiledObject(extFilename, compiledObj);

    parserPub = (struct scrParserPub_t *)imp_scrParserPub;
    parserPub->scriptfilename = savedParserFilename;
    parserPub->sourceBuf = oldSourceBuf;

    return compiledObj;
}

void Scr_PostCompileScripts(void)
{
    Hunk_ConvertTempToPermLowInternal();
}

void Scr_EndLoadScripts(void)
{
    struct scrCompilePub_t *scrCompPub = (struct scrCompilePub_t *)imp_scrCompilePub;
    scrCompPub->canonicalStrings = NULL;
    {
        struct scrVarPub_t *scrVarPub = (struct scrVarPub_t *)imp_scrVarPub;
        Hunk_ClearToMark(scrVarPub->mark);
    }
    SL_ShutdownSystem(2);

    scrCompPub->script_loading = 0;

    ClearObject(scrCompPub->loadedscripts);
    RemoveRefToObject(scrCompPub->loadedscripts);
    scrCompPub->loadedscripts = 0;

    ClearObject(scrCompPub->scripts);
    RemoveRefToObject(scrCompPub->scripts);
    scrCompPub->scripts = 0;

    ClearObject(scrCompPub->builtinFunc);
    RemoveRefToObject(scrCompPub->builtinFunc);
    scrCompPub->builtinFunc = 0;

    ClearObject(scrCompPub->builtinMeth);
    RemoveRefToObject(scrCompPub->builtinMeth);
    scrCompPub->builtinMeth = 0;
}

void Scr_PrecacheAnimTrees(Alloc_t Alloc, int user)
{
    struct scrAnimPub_t *scrAnimPub = (struct scrAnimPub_t *)imp_scrAnimPub;
    int i;

    if (scrAnimPub->xanim_num[user] > 0) {
        for (i = 1; i <= scrAnimPub->xanim_num[user]; i++) {
            Scr_LoadAnimTreeAtIndex(i, Alloc, user);
        }
    }
}

void Scr_EndLoadAnimTrees(void)
{
    struct scrAnimPub_t *scrAnimPub = (struct scrAnimPub_t *)imp_scrAnimPub;
    struct scrVarPub_t *scrVarPub;

    ClearObject(scrAnimPub->animtrees);
    RemoveRefToObject(scrAnimPub->animtrees);
    scrAnimPub->animtrees = 0;

    if (scrAnimPub->animtree_node != 0)
        RemoveRefToObject(scrAnimPub->animtree_node);

    SL_ShutdownSystem(2);

    scrVarPub = (struct scrVarPub_t *)imp_scrVarPub;
    scrVarPub->endScriptBuffer = (const char *)Hunk_AllocLowInternal(0);
    scrAnimPub->animtree_loading = 0;
}

void Scr_FreeScripts(int sys)
{
    struct scrCompilePub_t *scrCompPub = (struct scrCompilePub_t *)imp_scrCompilePub;
    struct scrAnimPub_t *scrAnimPub;
    struct scrVarPub_t *scrVarPub;

    if (scrCompPub->script_loading != 0) {
        scrCompPub->script_loading = 0;
        Scr_EndLoadScripts();
    }

    scrAnimPub = (struct scrAnimPub_t *)imp_scrAnimPub;
    if (scrAnimPub->animtree_loading != 0) {
        scrAnimPub->animtree_loading = 0;
        Scr_EndLoadAnimTrees();
    }

    if (sys == 0) {
        SL_ShutdownSystem(sys);
        Scr_ShutdownOpcodeLookup();
    }

    if (sys == 0) {
        scrVarPub = (struct scrVarPub_t *)imp_scrVarPub;
        scrVarPub->programBuffer = NULL;
        scrCompPub->programLen = 0;
        scrVarPub->endScriptBuffer = NULL;
        scrVarPub->checksum = 0;
    }
}

scr_func_t Scr_GetFunctionHandle(const char *filename, const char *name)
{
    struct scrCompilePub_t *scrCompPub = (struct scrCompilePub_t *)imp_scrCompilePub;
    struct scrVarPub_t *scrVarPub;
    unsigned int nameStr;
    unsigned int fileId;
    unsigned int fileObj;
    unsigned int varId;
    unsigned int obj;
    unsigned int codeVar;
    unsigned int codePos;

    nameStr = Scr_CreateCanonicalFilename(filename);
    fileId = FindVariable(scrCompPub->scripts, nameStr);
    SL_RemoveRefToString(nameStr);

    if (fileId == 0)
        return 0;

    fileObj = FindObject(fileId);
    nameStr = SL_FindLowercaseString(name);
    if (nameStr == 0)
        return 0;

    varId = FindVariable(fileObj, nameStr);
    if (varId == 0)
        return 0;

    if (GetVarType(varId) != 1)
        return 0;

    obj = FindObject(varId);
    codeVar = FindVariable(obj, 1);
    codePos = (unsigned int)Scr_EvalVariable(codeVar);

    scrVarPub = (struct scrVarPub_t *)imp_scrVarPub;
#if !defined(_M_X64) && !defined(__x86_64__)
    /* x86: code positions are stored as absolute pointers -> convert to a program-buffer
       offset. On x64 they are already stored as offsets (SCR_CODEPOS_ENC), so subtracting
       the buffer base would underflow and wrongly fail every lookup. */
    codePos -= (unsigned int)(size_t)scrVarPub->programBuffer;
#else
    (void)scrVarPub;
#endif
    if (codePos >= (unsigned int)scrCompPub->programLen)
        return 0;

    return (scr_func_t)codePos;
}

void Scr_BeginLoadScripts(void)
{
    struct scrCompilePub_t *scrCompPub = (struct scrCompilePub_t *)imp_scrCompilePub;
    struct scrVarPub_t *scrVarPub;

    scrCompPub->script_loading = 1;
    Scr_InitOpcodeLookup();

    scrCompPub->loadedscripts = Scr_AllocArray();
    scrCompPub->scripts = Scr_AllocArray();
    scrCompPub->builtinFunc = Scr_AllocArray();
    scrCompPub->builtinMeth = Scr_AllocArray();

    scrVarPub = (struct scrVarPub_t *)imp_scrVarPub;
    scrVarPub->programBuffer = (const char *)Hunk_AllocLowInternal(0);
    scrCompPub->programLen = 0;
    scrVarPub->endScriptBuffer = NULL;

    scrVarPub->mark = Hunk_SetMark();
    scrCompPub->canonicalStrings = (unsigned short *)Hunk_AllocInternal(0x20000);
    scrVarPub->canonicalStrCount = 0;

    scrVarPub->fieldBuffer = NULL;
    scrCompPub->value_count = 0;

    Scr_ClearErrorMessage();
    scrCompPub->func_table_size = 0;

    Scr_BeginLoadAnimTrees_(1);
    TempMemoryReset();
}

unsigned int SL_GetCanonicalString(const char *str)
{
    struct scrCompilePub_t *scrCompPub = (struct scrCompilePub_t *)imp_scrCompilePub;
    struct scrVarPub_t *scrVarPub;
    unsigned short *table = scrCompPub->canonicalStrings;
    unsigned short *entry;
    unsigned int stringValue;
    unsigned short val;

    stringValue = SL_FindString(str);
    val = table[stringValue];
    if (val != 0)
        return val;

    stringValue = SL_GetString_(str, 0, 0xf);
    SL_TransferRefToUser(stringValue, 2);

    entry = scrCompPub->canonicalStrings + stringValue;
    val = *entry;
    if (val != 0)
        return val;

    scrVarPub = (struct scrVarPub_t *)imp_scrVarPub;
    val = scrVarPub->canonicalStrCount + 1;
    scrVarPub->canonicalStrCount = val;
    *entry = val;
    return scrVarPub->canonicalStrCount;
}
