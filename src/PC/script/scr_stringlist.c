#include "common_types.h"
#include "imports.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
extern scrMemTreePub_t scrMemTreePub;

extern unsigned char scrStringGlob[];

#define SG_W0(i) (*(unsigned short *)((char *)&scrStringGlob + (unsigned int)(i) * 4))
#define SG_W1(i) (*(unsigned short *)((char *)&scrStringGlob + (unsigned int)(i) * 4 + 2))
#define SG_INIT_FLAG (*(unsigned char *)((char *)&scrStringGlob + 65536))
#define SG_RESTART (*(void **)((char *)&scrStringGlob + 65540))

/* x64 workaround: the hash-chain unlink can leave a freed string's bucket in the
   chain (a reconstruction bug in the bucket-recycling logic). Re-finding such a
   stale bucket and ref-adding it writes a refcount into a now-FREE memory-tree
   node, corrupting the buddy free-list. SL_NODE_DEAD() reports whether a string
   node is currently free per the buddy allocator (MT_IsNodeCovered), so ref-add
   paths can refuse to touch dead nodes. */
extern int MT_IsNodeCovered(int);
/* Guard used ONLY in the find match-and-return paths: when the hash chain still references a
   node the buddy allocator considers FREE (an x64 reconstruction artifact), returning it is
   fine for identity but ref-adding it would corrupt the buddy free-list -> later id collisions
   ("X already defined"). It must NOT gate the direct ref-add functions (SL_AddRefToString /
   Scr_SetString / SL_TransferRefToUser) -- those run for live nodes (e.g. a function name like
   'main' at registration) and skipping them frees a live string -> "could not find main". */
#if defined(_M_X64) || defined(__x86_64__)
#define SL_NODE_DEAD(n) (MT_IsNodeCovered((int)(n)))
#else
#define SL_NODE_DEAD(n) (0)   /* x86 string table is unaffected; keep original behavior */
#endif

extern void MT_Init(void);
extern byte *MT_InitForceAlloc(void);
extern unsigned short MT_AllocIndex(int numBytes, int type);
extern void MT_FreeIndex(unsigned int nodeNum, int numBytes);
extern void MT_ForceAllocIndex(byte *allocBits, unsigned int nodeNum, int numBytes);
extern void MT_FinishForceAlloc(byte *allocBits);
extern void Scr_DumpScriptThreads(void);
extern void Scr_DumpScriptVariables(void);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);

unsigned int SL_ConvertFromString(const char *str);
void SL_Shutdown(void);
const char *SL_ConvertToString(unsigned int stringValue);
void SL_TransferRefToUser(unsigned int stringValue, unsigned int user);
void SL_AddRefToString(unsigned int stringValue);
void SL_RemoveRefToStringOfLen(unsigned int stringValue, unsigned int len);
int SL_GetStringLen(unsigned int stringValue);
unsigned int SL_FindStringOfLen(const char *str, unsigned int len);
unsigned int SL_FindLowercaseString(const char *str);
unsigned int SL_FindString(const char *str);
unsigned int SL_GetStringOfLen(const char *str, unsigned int user, unsigned int len, int type);
unsigned int SL_GetLowercaseString_(const char *str, unsigned int user, int type);
unsigned int SL_GetString_(const char *str, unsigned int user, int type);
void SL_RemoveRefToString(unsigned int stringValue);
void SL_ShutdownSystem(unsigned int user);
void Scr_SetString(scr_string_t *to, unsigned int from);
unsigned int SL_ConvertToLowercase(unsigned int stringValue, unsigned int user, int type);
void Scr_ShutdownGameStrings(void);
void SL_Init(void);
unsigned int Scr_AllocString(const char *s, int sys);
unsigned int SL_GetString(const char *str, unsigned int user);
unsigned int SL_GetStringForFloat(float f);
unsigned int SL_GetStringForInt(int i);
unsigned int SL_GetStringForVector(const float *v);
unsigned int Scr_CreateCanonicalFilename(const char *filename);

unsigned int SL_ConvertFromString(const char *str)
{
    return (int)(((byte *)str - 4) - (byte *)scrMemTreePub.mt_buffer) / 8;
}

void SL_Shutdown(void)
{
    SG_INIT_FLAG = 0;
}

const char *SL_ConvertToString(unsigned int stringValue)
{
    if (!stringValue)
        return 0;
    byte *base = (byte *)scrMemTreePub.mt_buffer;
    const char *result = (const char *)(base + stringValue * 8 + 4);
    return result;
}

static int dbg_transfer_count = 0;
static int dbg_transfer_decr = 0;
static int dbg_transfer_setbit = 0;
void SL_TransferRefToUser(unsigned int stringValue, unsigned int user)
{
    byte *entry = (byte *)scrMemTreePub.mt_buffer + stringValue * 8;
    if (entry[1] & user) {
        *(unsigned short *)(entry + 2) -= 1;
    } else {
        entry[1] |= (byte)user;
    }
}
void DBG_PrintTransferStats(void)
{
}

void SL_AddRefToString(unsigned int stringValue)
{
    byte *base = (byte *)scrMemTreePub.mt_buffer;
    *(unsigned short *)(base + stringValue * 8 + 2) += 1;
}

static inline __attribute__((always_inline)) unsigned int compute_hash_slot(const char *str, unsigned int len)
{
    unsigned int x;
    if (len <= 0xff) {
        if (len == 0)
            return 1;

        {
            unsigned int hash = 0;
            const char *p = str;
            const char *end = str + len;
            do {
                hash = hash * 31 + (signed char)*p;
                p++;
            } while (p != end);
            return (hash % 16383) + 1;
        }
    }
    x = len >> 2;
    return (x % 16383) + 1;
}

static unsigned int get_entry_strlen(byte *entry)
{
    unsigned int prev = (unsigned int)((unsigned char)(entry[0] - 1));
    if (entry[4 + prev] != 0) {
        byte *p = entry + 4 + prev;
        do {
            prev += 0x100;
            p += 0x100;
        } while (*p != 0);
    }
    return prev + 1;
}

void SL_RemoveRefToStringOfLen(unsigned int stringValue, unsigned int len)
{
    byte *base = (byte *)scrMemTreePub.mt_buffer;
    byte *entry = base + stringValue * 8;
    unsigned short ref;
    unsigned int esi;
    unsigned int newEntry_offset;
    unsigned short *edi;
    unsigned int chain_next_idx;
    unsigned short *hash_ptr;
    unsigned short *adr0;
    unsigned int ecx_slot;

    ref = *(unsigned short *)(entry + 2) - 1;
    *(unsigned short *)(entry + 2) = ref;

    if (ref != 0)
        return;

#if defined(_M_X64) || defined(__x86_64__)
    /* x64: do NOT reclaim a string node whose refcount reached zero. The free path (hash-chain
       unlink + buddy MT_FreeIndex) has an x64 reconstruction corruption: freed nodes linger in
       the hash chain and get re-found (colliding ids -> "X already defined" / "unknown
       function"), and live function names ('main') get dropped between compile and runtime
       lookup. Leaking zero-ref nodes for the session sidesteps the whole corruption: the node
       stays valid and correctly findable. The pool is ~64K nodes; a single map load/compile
       stays well within it. (x86 keeps the original reclaiming behavior below.) */
    return;
#endif

    esi = compute_hash_slot((const char *)(entry + 4), len);

    newEntry_offset = esi * 4;
    edi = (unsigned short *)((char *)&scrStringGlob + newEntry_offset);

    MT_FreeIndex(stringValue, len + 4);

#if defined(_M_X64) || defined(__x86_64__)
    /* x64: the original chain unlink (below) leaves freed strings findable in the hash
       chain on x64 (a reconstruction bug in the bucket recycling) -> they get re-found and
       ref-added, corrupting the buddy free-list. Do a robust brute-force removal instead:
       drop every bucket that references the freed string from its chain. */
    {
        unsigned int b;
        for (b = 1; b < 0x4000; b++) {
            if ((SG_W0(b) & 0xc000) == 0)
                continue;                                   /* empty bucket */
            if (SG_W1(b) != (unsigned short)stringValue)
                continue;                                   /* not this string */

            if (SG_W0(b) & 0x8000) {                        /* chain HEAD at slot b */
                unsigned int nxt = SG_W0(b) & 0x3fff;
                if (nxt == b) {                             /* single element -> empty + recycle */
                    unsigned short oh = SG_W0(0);
                    SG_W0(b) = oh; SG_W1(b) = 0;
                    SG_W1((unsigned int)oh) = (unsigned short)b; SG_W0(0) = (unsigned short)b;
                } else {                                    /* pull the next bucket up into the head */
                    unsigned short oh;
                    SG_W1(b) = SG_W1(nxt);
                    SG_W0(b) = (unsigned short)(0x8000 | (SG_W0(nxt) & 0x3fff));
                    oh = SG_W0(0);
                    SG_W0(nxt) = oh; SG_W1(nxt) = 0;
                    SG_W1((unsigned int)oh) = (unsigned short)nxt; SG_W0(0) = (unsigned short)nxt;
                }
            } else {                                        /* chained bucket -> neutralize (find skips SG_W1==0) */
                SG_W1(b) = 0;
            }
        }
        (void)edi; (void)esi; (void)adr0;
        return;
    }
#endif

    chain_next_idx = (unsigned int)(SG_W0(esi) & 0x3fff);
    hash_ptr = (unsigned short *)((char *)&scrStringGlob + chain_next_idx * 4);

    if (edi[1] == (unsigned short)stringValue) {

        if (hash_ptr == edi) {

            ecx_slot = esi;
            goto add_to_freelist;
        } else {

            unsigned short new_w0 = (unsigned short)((hash_ptr[0] & 0x3fff) | 0x8000);
            SG_W0(esi) = new_w0;

            edi[1] = hash_ptr[1];

            SG_RESTART = (void *)edi;

            edi = hash_ptr;
            ecx_slot = chain_next_idx;
            goto add_to_freelist;
        }
    } else {
        unsigned int prev_idx;

        if (hash_ptr[1] == (unsigned short)stringValue) {

            prev_idx = esi;
            goto patch_and_free;
        }

        prev_idx = esi;
        {
            int _rml_iters = 0;
            while (1) {
                if (++_rml_iters > 16384) {
                    edi = hash_ptr;
                    ecx_slot = chain_next_idx;
                    goto add_to_freelist;
                }
                unsigned int old_prev = prev_idx;
                prev_idx = chain_next_idx;
                chain_next_idx = (unsigned int)(hash_ptr[0] & 0x3fff);
                hash_ptr = (unsigned short *)((char *)&scrStringGlob + chain_next_idx * 4);
                if (hash_ptr[1] == (unsigned short)stringValue)
                    break;
            }
        }

    patch_and_free:

        {
            unsigned short pf = SG_W0(prev_idx) & 0xc000;
            unsigned short cn = hash_ptr[0] & 0x3fff;
            SG_W0(prev_idx) = pf | cn;
        }
        edi = hash_ptr;
        ecx_slot = chain_next_idx;
        goto add_to_freelist;
    }

add_to_freelist:

    {
        unsigned short old_head = SG_W0(0);
        edi[0] = old_head;
        edi[1] = 0;

        SG_W1((unsigned int)old_head) = (unsigned short)ecx_slot;

        SG_W0(0) = (unsigned short)ecx_slot;
    }

    return;
}

int SL_GetStringLen(unsigned int stringValue)
{
    byte *base = (byte *)scrMemTreePub.mt_buffer;
    byte *entry = base + stringValue * 8;
    unsigned int ecx;
    byte *edx_ptr;

    ecx = (unsigned int)((unsigned char)(entry[0] - 1));
    if (entry[4 + ecx] != 0) {
        edx_ptr = entry + 4 + ecx;
        do {
            ecx += 0x100;
            edx_ptr += 0x100;
        } while (*edx_ptr != 0);
    }
    return (int)ecx;
}

unsigned int SL_FindStringOfLen(const char *str, unsigned int len)
{
    unsigned int hash;
    unsigned short *entry;
    unsigned int stringValue;
    unsigned int byteLen;
    byte *base;
    unsigned int newEntry_offset;
    unsigned int newIndex;
    unsigned int prev_idx;

    hash = compute_hash_slot(str, len);

    entry = (unsigned short *)((char *)&scrStringGlob + hash * 4);

    {
        unsigned int flags = (unsigned int)(entry[0]) & 0xc000;
        if (flags != 0x8000) {
            stringValue = 0;
            return stringValue;
        }
    }

    stringValue = (unsigned int)entry[1];
    base = (byte *)scrMemTreePub.mt_buffer;
    byteLen = len & 0xff;

    {
        byte *refEntry = base + stringValue * 8;
        if ((unsigned char)refEntry[0] == (unsigned char)byteLen && stringValue != 0xFFFF) {

            if (memcmp(str, refEntry + 4, len) == 0) {

                return stringValue;
            }
        }
    }

    {
        unsigned int cur_link = (unsigned int)(entry[0]) & 0x3fff;
        unsigned int newEntry_idx = cur_link;
        unsigned short *newEntry_ptr = (unsigned short *)((char *)&scrStringGlob + newEntry_idx * 4);

        if (newEntry_ptr == entry) {
            return 0;
        }

        prev_idx = hash;

        {
            int _find_iters = 0;
            while (1) {
                unsigned int sv2;
                byte *e2;

                if (++_find_iters > 16384)
                    return 0;

                sv2 = (unsigned int)newEntry_ptr[1];
                e2 = base + sv2 * 8;

                if ((unsigned char)e2[0] == (unsigned char)byteLen && sv2 != 0xFFFF) {
                    if (memcmp(str, e2 + 4, len) == 0) {
                        {
                            unsigned short pf = SG_W0(prev_idx) & 0xc000;
                            unsigned short nxt1 = newEntry_ptr[0] & 0x3fff;
                            SG_W0(prev_idx) = pf | nxt1;
                        }
                        {
                            unsigned short cf = newEntry_ptr[0] & 0xc000;
                            unsigned short nxt2 = entry[0] & 0x3fff;
                            newEntry_ptr[0] = cf | nxt2;
                        }
                        {
                            unsigned short ef = entry[0] & 0xc000;
                            entry[0] = ef | (unsigned short)(newEntry_idx & 0x3fff);
                        }
                        {
                            unsigned short tmp = newEntry_ptr[1];
                            newEntry_ptr[1] = entry[1];
                            entry[1] = (unsigned short)sv2;
                        }
                        stringValue = sv2;
                        return stringValue;
                    }
                }

                prev_idx = newEntry_idx;
                newEntry_idx = (unsigned int)(newEntry_ptr[0]) & 0x3fff;
                newEntry_ptr = (unsigned short *)((char *)&scrStringGlob + newEntry_idx * 4);

                if (newEntry_ptr == entry)
                    return 0;
            }
        }
    }
}

unsigned int SL_FindLowercaseString(const char *str)
{
    char newStr[0x2000];
    int slen;
    int i;

    slen = (int)strlen(str) + 1;

    if (slen > 0x2000)
        return 0;

    if (slen > 0) {
        for (i = 0; i < slen; i++) {
            newStr[i] = (char)tolower(str[i]);
        }
    }

    return SL_FindStringOfLen(newStr, (unsigned int)slen);
}

unsigned int SL_FindString(const char *str)
{
    unsigned int len = (unsigned int)strlen(str) + 1;
    return SL_FindStringOfLen(str, len);
}

unsigned int SL_GetStringOfLen(const char *str, unsigned int user, unsigned int len, int type)
{
    unsigned int hash;
    unsigned short *entry_ptr;
    unsigned int byteLen;
    unsigned int stringValue;
    unsigned int esi_sv;
    byte *base;

    hash = compute_hash_slot(str, len);

loop_top: {
    unsigned int ebx_hash = hash;
    unsigned int ebx_offset = ebx_hash * 4;
    entry_ptr = (unsigned short *)((char *)&scrStringGlob + ebx_offset);
    byteLen = len & 0xff;

    {
        unsigned int sw0_val = (unsigned int)SG_W0(hash);
        unsigned int sw1_val = (unsigned int)SG_W1(hash);
        unsigned int flags0 = sw0_val & 0xc000;

        if (flags0 == 0x8000) {

            base = (byte *)scrMemTreePub.mt_buffer;
            {
                unsigned int sv_head = sw1_val;
                byte *refEntry = base + sv_head * 8;

                if ((unsigned char)refEntry[0] == (unsigned char)byteLen && sv_head != 0xFFFF) {

                    if (memcmp(str, refEntry + 4, len) == 0) {
                        /* return the matched id (identity preserved) but never ref-add a
                           node the buddy considers FREE -- that write corrupts the free-list */
                        if (!((unsigned char)refEntry[1] & (unsigned char)user) && !SL_NODE_DEAD(sv_head)) {
                            refEntry[1] |= (unsigned char)user;
                            *(unsigned short *)(refEntry + 2) += 1;

                            sv_head = (unsigned int)entry_ptr[1];
                        }
                        esi_sv = sv_head;
                        return esi_sv;
                    }
                }
            }

            {
                unsigned int cur_sv = sw0_val & 0x3fff;
                unsigned int cur_idx = cur_sv;
                unsigned short *cur_ptr = (unsigned short *)((char *)&scrStringGlob + cur_idx * 4);

                if (cur_ptr == entry_ptr) {

                    goto alloc_mid_chain_occupied;
                }

                {
                    unsigned int prev_sv = hash;
                    int _chain_iters = 0;

                    while (1) {
                        unsigned int node_sv;
                        byte *node_entry;

                        if (++_chain_iters > 16384) {

                            goto alloc_mid_chain_occupied;
                        }

                        node_sv = (unsigned int)cur_ptr[1];
                        node_entry = base + node_sv * 8;

                        if ((unsigned char)node_entry[0] == (unsigned char)byteLen && node_sv != 0xFFFF) {
                            byte *cmp_ptr = node_entry + 4;
                            if (memcmp(str, cmp_ptr, len) == 0) {

                                {
                                    unsigned short pf = SG_W0(prev_sv) & 0xc000;
                                    unsigned short cn = cur_ptr[0] & 0x3fff;
                                    SG_W0(prev_sv) = pf | cn;
                                }
                                {
                                    unsigned short cf = cur_ptr[0] & 0xc000;
                                    unsigned short en = entry_ptr[0] & 0x3fff;
                                    cur_ptr[0] = cf | en;
                                }
                                {
                                    unsigned short ef = entry_ptr[0] & 0xc000;
                                    entry_ptr[0] = ef | (unsigned short)(cur_idx & 0x3fff);
                                }
                                {
                                    unsigned short tmp = cur_ptr[1];
                                    cur_ptr[1] = entry_ptr[1];
                                    entry_ptr[1] = (unsigned short)node_sv;
                                }

                                if (!((unsigned char)node_entry[1] & (unsigned char)user) && !SL_NODE_DEAD(node_sv)) {
                                    node_entry[1] |= (unsigned char)user;
                                    *(unsigned short *)(node_entry + 2) += 1;
                                }
                                esi_sv = node_sv;
                                return esi_sv;
                            }
                        }

                        prev_sv = cur_idx;
                        cur_idx = (unsigned int)(cur_ptr[0]) & 0x3fff;
                        cur_sv = cur_idx;
                        cur_ptr = (unsigned short *)((char *)&scrStringGlob + cur_idx * 4);

                        if (cur_ptr == entry_ptr)
                            goto alloc_mid_chain_occupied;
                    }
                }

            alloc_mid_chain_occupied:;

                {
                    unsigned int newIndex = (unsigned int)SG_W0(0);
                    unsigned short *newNode;

                    if (newIndex == 0) {

                        Scr_DumpScriptThreads();
                        Scr_DumpScriptVariables();
                        Com_Error(1, "\x15"
                                     "exceeded maximum number of script strings\n");
                        newIndex = (unsigned int)SG_W0(0);
                    }

                    esi_sv = (unsigned int)MT_AllocIndex((int)(len + 4), type);

                    newNode = (unsigned short *)((char *)&scrStringGlob + newIndex * 4);

                    {
                        unsigned short freenext = SG_W0(newIndex) & 0x3fff;
                        SG_W0(0) = freenext;
                    }

                    {
                        unsigned short fh = SG_W0(0);
                        SG_W1((unsigned int)fh) = 0;
                    }

                    {
                        unsigned short chain_next2 = entry_ptr[0] & 0x3fff;
                        newNode[0] = chain_next2 | 0x4000;
                    }

                    {
                        unsigned short ef2 = entry_ptr[0] & 0xc000;
                        entry_ptr[0] = ef2 | (unsigned short)(newIndex & 0x3fff);
                    }

                    newNode[1] = entry_ptr[1];

                    goto store_and_return;
                }
            }
        } else if (flags0 == 0) {

            base = (byte *)scrMemTreePub.mt_buffer;

            {
                unsigned int newIndex2 = (unsigned int)SG_W0(0);

                if (newIndex2 == 0) {
                    Scr_DumpScriptThreads();
                    Scr_DumpScriptVariables();
                    Com_Error(1, "\x15"
                                 "exceeded maximum number of script strings\n");
                    newIndex2 = (unsigned int)SG_W0(0);
                }

                esi_sv = (unsigned int)MT_AllocIndex((int)(len + 4), type);

                {
                    unsigned int ecx2 = (unsigned int)entry_ptr[1];
                    unsigned short edx2 = entry_ptr[0] & 0x3fff;
                    unsigned short af2 = SG_W0(ecx2) & 0xc000;
                    SG_W0(ecx2) = af2 | edx2;
                    SG_W1((unsigned int)edx2) = (unsigned short)ecx2;
                }

                entry_ptr[0] = (unsigned short)(hash | 0x8000);

                goto store_and_return;
            }
        } else {

            unsigned int next_idx = sw0_val & 0x3fff;
            unsigned int cur_search = next_idx;

            {
                int _mid_iters = 0;
                while ((SG_W0(cur_search) & 0x3fff) != hash) {
                    cur_search = (unsigned int)(SG_W0(cur_search) & 0x3fff);
                    if (++_mid_iters > 16384)
                        break;
                }
            }

            base = (byte *)scrMemTreePub.mt_buffer;

            {
                unsigned int newIndex3 = (unsigned int)SG_W0(0);
                unsigned short *newNode3;

                if (newIndex3 == 0) {

                    Scr_DumpScriptThreads();
                    Scr_DumpScriptVariables();
                    Com_Error(1, "\x15"
                                 "exceeded maximum number of script strings\n");
                    newIndex3 = (unsigned int)SG_W0(0);
                }

                esi_sv = (unsigned int)MT_AllocIndex((int)(len + 4), type);

                newNode3 = (unsigned short *)((char *)&scrStringGlob + newIndex3 * 4);

                {
                    unsigned short freenext3 = SG_W0(newIndex3) & 0x3fff;
                    SG_W0(0) = freenext3;
                    SG_W1((unsigned int)freenext3) = 0;
                }

                {
                    unsigned short csf = SG_W0(cur_search) & 0xc000;
                    SG_W0(cur_search) = csf | (unsigned short)(newIndex3 & 0x3fff);
                }

                newNode3[0] = (unsigned short)(next_idx | 0x4000);

                newNode3[1] = entry_ptr[1];

                entry_ptr[0] = (unsigned short)(hash | 0x8000);

                goto store_and_return;
            }
        }
    }
}

store_and_return:

    entry_ptr[1] = (unsigned short)esi_sv;

    {
        byte *newMem = (byte *)scrMemTreePub.mt_buffer + esi_sv * 8;
        memcpy(newMem + 4, str, len);
        newMem[1] = (byte)user;
        *(unsigned short *)(newMem + 2) = 1;
        newMem[0] = (byte)byteLen;
    }

    return esi_sv;
}

unsigned int SL_GetLowercaseString_(const char *str, unsigned int user, int type)
{
    char newStr[0x2000];
    int slen;
    unsigned int i;

    slen = (int)strlen(str) + 1;

    if ((unsigned int)slen > 0x2000) {
        Com_Error(1, "max string length exceeded: \"%s\"", str);
        return 0;
    }

    if (slen > 0) {
        i = 0;
        while ((unsigned int)i < (unsigned int)slen) {
            newStr[i] = (char)tolower(str[i]);
            i++;
        }
    }

    return SL_GetStringOfLen(newStr, user, (unsigned int)slen, type);
}

unsigned int SL_GetString_(const char *str, unsigned int user, int type)
{
    unsigned int len = (unsigned int)strlen(str) + 1;
    return SL_GetStringOfLen(str, user, len, type);
}

void SL_RemoveRefToString(unsigned int stringValue)
{
    byte *base = (byte *)scrMemTreePub.mt_buffer;
    byte *entry = base + stringValue * 8;
    unsigned int ecx_len;

    ecx_len = (unsigned int)((unsigned char)(entry[0] - 1));
    if (entry[4 + ecx_len] != 0) {
        byte *p = entry + 4 + ecx_len;
        do {
            ecx_len += 0x100;
            p += 0x100;
        } while (*p != 0);
    }
    ecx_len += 1;

    SL_RemoveRefToStringOfLen(stringValue, ecx_len);
}

void SL_ShutdownSystem(unsigned int user)
{
    unsigned int esi;
    unsigned char user_notb = (unsigned char)(~(unsigned char)user);
    byte *base;
    unsigned short *entry_ptr;
    unsigned int sv;
    byte *mem;

    for (esi = 1; esi < 0x4000; esi++) {
    retry_slot:
        entry_ptr = (unsigned short *)((char *)&scrStringGlob + esi * 4);

        if ((SG_W0(esi) & 0xc000) == 0)
            continue;

        sv = (unsigned int)entry_ptr[1];
        base = (byte *)scrMemTreePub.mt_buffer;
        mem = base + sv * 8;

        if (!((unsigned char)mem[1] & (unsigned char)user))
            continue;

        mem[1] &= user_notb;
        SG_RESTART = (void *)0;
        SL_RemoveRefToString(sv);

        if (SG_RESTART != (void *)0)
            goto retry_slot;
    }

    return;
}

void Scr_SetString(scr_string_t *to, unsigned int from)
{
    if (from) {
        byte *base = (byte *)scrMemTreePub.mt_buffer;
        *(unsigned short *)(base + from * 8 + 2) += 1;
    }

    if (*to) {
        SL_RemoveRefToString((unsigned int)*to);
    }

    *to = (scr_string_t)from;
    return;
}

unsigned int SL_ConvertToLowercase(unsigned int stringValue, unsigned int user, int type)
{
    char newStr[0x2000];
    byte *base = (byte *)scrMemTreePub.mt_buffer;
    byte *entry = base + stringValue * 8;
    unsigned int ecx_prev;
    unsigned int esi_len;
    const char *str_ptr;
    unsigned int newStringValue;
    unsigned int i;

    ecx_prev = (unsigned int)((unsigned char)(entry[0] - 1));
    if (entry[4 + ecx_prev] != 0) {
        byte *p = entry + 4 + ecx_prev;
        do {
            ecx_prev += 0x100;
            p += 0x100;
        } while (*p != 0);
    }
    esi_len = ecx_prev + 1;

    if (esi_len > 0x2000)
        return stringValue;

    if (stringValue != 0)
        str_ptr = (const char *)(entry + 4);
    else
        str_ptr = (const char *)0;

    if (esi_len > 0) {
        if (str_ptr != (const char *)0) {
            for (i = 0; i < esi_len; i++) {
                newStr[i] = (char)tolower(str_ptr[i]);
            }
        }

    }

    newStringValue = SL_GetStringOfLen(newStr, user, esi_len, type);
    SL_RemoveRefToString(stringValue);
    return newStringValue;
}

void Scr_ShutdownGameStrings(void)
{
    unsigned int ebx_i;
    byte *base;
    unsigned short *entry_ptr;
    unsigned int sv;
    byte *mem;

    for (ebx_i = 1; ebx_i < 0x4000; ebx_i++) {
    retry_game:
        entry_ptr = (unsigned short *)((char *)&scrStringGlob + ebx_i * 4);

        if ((SG_W0(ebx_i) & 0xc000) == 0)
            continue;

        sv = (unsigned int)entry_ptr[1];
        base = (byte *)scrMemTreePub.mt_buffer;
        mem = base + sv * 8;

        if (!(mem[1] & 1))
            continue;

        mem[1] &= 0xfe;
        SG_RESTART = (void *)0;
        SL_RemoveRefToString(sv);

        if (SG_RESTART != (void *)0)
            goto retry_game;
    }

    return;
}

void SL_Init(void)
{

    if (SG_INIT_FLAG != 0) {

        unsigned int idx;
        for (idx = 1; idx < 0x4000; idx++) {
            unsigned int sv2;
            byte *base2;
            byte *mem2;

        reinit_retry:
            if ((SG_W0(idx) & 0xc000) == 0)
                continue;

            SG_RESTART = (void *)0;
            sv2 = (unsigned int)SG_W1(idx);
            base2 = (byte *)scrMemTreePub.mt_buffer;
            mem2 = base2 + sv2 * 8;

            if (mem2[1] & 4) {

                *(unsigned short *)(mem2 + 2) = 1;
                mem2[1] = 4;

                if (SG_RESTART != (void *)0)
                    goto reinit_retry;
                continue;
            }

            *(unsigned short *)(mem2 + 2) = 0;
            mem2[1] = 0;

            {
                unsigned int ecx3 = (unsigned int)((unsigned char)(mem2[0] - 1));
                if (mem2[4 + ecx3] != 0) {
                    byte *pp3 = mem2 + 4 + ecx3;
                    do {
                        ecx3 += 0x100;
                        pp3 += 0x100;
                    } while (*pp3 != 0);
                }
                ecx3 += 1;

                {
                    unsigned int len3 = ecx3;
                    byte *str_ptr3 = mem2 + 4;
                    unsigned int hash3;
                    unsigned int newEntry_idx3;
                    unsigned short *edi3;
                    unsigned int esi3_chain;
                    unsigned short *ecx3_hash;

                    if (len3 > 0xff)
                        hash3 = ((len3 >> 2) % 16383) + 1;
                    else if (len3 == 0)
                        hash3 = 1;
                    else
                        hash3 = compute_hash_slot((const char *)str_ptr3, len3);

                    newEntry_idx3 = hash3;
                    edi3 = (unsigned short *)((char *)&scrStringGlob + newEntry_idx3 * 4);

                    MT_FreeIndex(sv2, len3 + 4);

                    esi3_chain = (unsigned int)(SG_W0(hash3) & 0x3fff);
                    ecx3_hash = (unsigned short *)((char *)&scrStringGlob + esi3_chain * 4);

                    if (edi3[1] == (unsigned short)sv2) {

                        if (ecx3_hash == edi3) {

                            unsigned short old_h3 = SG_W0(0);
                            edi3[0] = old_h3;
                            edi3[1] = 0;
                            SG_W1((unsigned int)old_h3) = (unsigned short)hash3;
                            SG_W0(0) = (unsigned short)hash3;
                        } else {

                            unsigned short new_w03 = (unsigned short)((ecx3_hash[0] & 0x3fff) | 0x8000);
                            SG_W0(hash3) = new_w03;
                            edi3[1] = ecx3_hash[1];
                            SG_RESTART = (void *)edi3;
                            {
                                unsigned short old_h4 = SG_W0(0);
                                ecx3_hash[0] = old_h4;
                                ecx3_hash[1] = 0;
                                SG_W1((unsigned int)old_h4) = (unsigned short)esi3_chain;
                                SG_W0(0) = (unsigned short)esi3_chain;
                            }
                        }
                    } else {
                        unsigned int esi3_prev;
                        if (ecx3_hash[1] == (unsigned short)sv2) {

                            esi3_prev = hash3;
                            {
                                unsigned short pf4 = SG_W0(esi3_prev) & 0xc000;
                                unsigned short cn4 = ecx3_hash[0] & 0x3fff;
                                SG_W0(esi3_prev) = pf4 | cn4;
                                {
                                    unsigned short old_h5 = SG_W0(0);
                                    ecx3_hash[0] = old_h5;
                                    ecx3_hash[1] = 0;
                                    SG_W1((unsigned int)old_h5) = (unsigned short)esi3_chain;
                                    SG_W0(0) = (unsigned short)esi3_chain;
                                }
                            }
                        } else {
                            unsigned int ebx3w = (unsigned int)(ecx3_hash[0] & 0x3fff);
                            unsigned short *ecx3w = (unsigned short *)((char *)&scrStringGlob + ebx3w * 4);
                            int sl_guard = 0x4000;   /* chain <= table size; bound it to avoid an infinite spin on a corrupt/cyclic chain (map-load restart-rehash hang) */
                            esi3_prev = esi3_chain;
                            while (ecx3w[1] != (unsigned short)sv2) {
                                if (--sl_guard < 0) break;
                                esi3_prev = ebx3w;
                                ebx3w = (unsigned int)(ecx3w[0] & 0x3fff);
                                ecx3w = (unsigned short *)((char *)&scrStringGlob + ebx3w * 4);
                            }
                            {
                                unsigned short pf5 = SG_W0(esi3_prev) & 0xc000;
                                unsigned short cn5 = ecx3w[0] & 0x3fff;
                                SG_W0(esi3_prev) = pf5 | cn5;
                                {
                                    unsigned short old_h6 = SG_W0(0);
                                    ecx3w[0] = old_h6;
                                    ecx3w[1] = 0;
                                    SG_W1((unsigned int)old_h6) = (unsigned short)ebx3w;
                                    SG_W0(0) = (unsigned short)ebx3w;
                                }
                            }
                        }
                    }
                }
            }

            if (SG_RESTART != (void *)0)
                goto reinit_retry;
        }

        {
            byte *allocBits = MT_InitForceAlloc();
            unsigned int i2;
            for (i2 = 1; i2 < 0x4000; i2++) {
                if ((SG_W0(i2) & 0xc000) == 0)
                    continue;

                {
                    unsigned int sv3 = (unsigned int)SG_W1(i2);
                    byte *base3 = (byte *)scrMemTreePub.mt_buffer;
                    byte *mem3 = base3 + sv3 * 8;

                    if (!(mem3[1] & 4))
                        continue;

                    {
                        const char *s3;
                        unsigned int sz3;

                        if (sv3 != 0)
                            s3 = (const char *)(mem3 + 4);
                        else
                            s3 = (const char *)0;

                        sz3 = (unsigned int)strlen(s3) + 1 + 4;

                        MT_ForceAllocIndex(allocBits, sv3, (int)sz3);
                    }
                }
            }
            MT_FinishForceAlloc(allocBits);
        }

        return;
    }

    MT_Init();

    SG_W0(0) = 0;

    {
        unsigned int edx_prev = 0;
        unsigned int ecx_cur = 1;
        unsigned int ebx_off = 4;
        unsigned int eax_next;

    init_loop:

        *(unsigned short *)((char *)&scrStringGlob + ebx_off) = 0;

        SG_W0(edx_prev) |= (unsigned short)ecx_cur;

        *(unsigned short *)((char *)&scrStringGlob + ebx_off + 2) = (unsigned short)edx_prev;

        eax_next = ecx_cur + 1;
        if (eax_next == 0x4000)
            goto init_done;

        edx_prev = ecx_cur;
        ecx_cur = eax_next;
        ebx_off += 4;
        goto init_loop;

    init_done:

        *(unsigned short *)((char *)&scrStringGlob + 2) = (unsigned short)ecx_cur;
    }

    SG_INIT_FLAG = 1;

    return;
}

unsigned int Scr_AllocString(const char *s, int sys)
{
    unsigned int len = (unsigned int)strlen(s) + 1;
    return SL_GetStringOfLen(s, 1, len, 6);
}

unsigned int SL_GetString(const char *str, unsigned int user)
{
    unsigned int len = (unsigned int)strlen(str) + 1;
    return SL_GetStringOfLen(str, user, len, 6);
}

unsigned int SL_GetStringForFloat(float f)
{
    char tempString[0x80];
    unsigned int len;

    sprintf(tempString, "%g", (double)f);
    len = (unsigned int)strlen(tempString) + 1;
    return SL_GetStringOfLen(tempString, 0, len, 0xe);
}

unsigned int SL_GetStringForInt(int i)
{
    char tempString[0x80];
    unsigned int len;

    sprintf(tempString, "%i", i);
    len = (unsigned int)strlen(tempString) + 1;
    return SL_GetStringOfLen(tempString, 0, len, 0xe);
}

unsigned int SL_GetStringForVector(const float *v)
{
    char tempString[0x80];
    unsigned int len;

    sprintf(tempString, "(%g, %g, %g)", (double)v[0], (double)v[1], (double)v[2]);
    len = (unsigned int)strlen(tempString) + 1;
    return SL_GetStringOfLen(tempString, 0, len, 0xe);
}

unsigned int Scr_CreateCanonicalFilename(const char *filename)
{
    char newFilename[0x400];
    char *dst = newFilename;
    unsigned int remaining = 0x400;
    const char *src = filename;
    int c;

skip_slashes:
    c = (int)(unsigned char)*src++;
    if (c == '\\' || c == '/')
        goto skip_slashes;

check_c:
    if ((unsigned int)c <= 0x1f) {
        if (c == 0) {
            *dst = '\0';
            goto get_len_and_return;
        }

        goto skip_slashes;
    }

    if (c == '\\')
        c = '/';

tolower_store:
    *dst = (char)tolower(c);
    dst++;
    remaining--;
    if (remaining == 0) {
        Com_Error(1, "\x15"
                     "Filename '%s' exceeds maximum length of %d",
                  filename, 0);

    }

    if (c == '/')
        goto skip_slashes;

inner_loop:
    c = (int)(unsigned char)*src++;
    if (c == '\\') {
        c = '/';
        goto tolower_store;
    }
    goto check_c;

get_len_and_return: {
    unsigned int len = (unsigned int)strlen(newFilename) + 1;
    return SL_GetStringOfLen(newFilename, 0, len, 7);
}
}
