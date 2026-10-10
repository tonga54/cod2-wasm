#include "common_types.h"
#include "imports.h"

extern struct scrMemTreePub_t scrMemTreePub;
extern unsigned char scrMemTreeGlob[];

extern byte *Z_VirtualAllocInternal(int size);
extern void Z_VirtualFreeInternal(void *ptr);
extern void Com_Printf(const char *fmt, ...);
extern void Scr_TerminalError(const char *msg);

unsigned int Scr_GetStringUsage(void);
byte *MT_InitForceAlloc(void);
void MT_Init(void);
void MT_DumpTree(void);
short unsigned int MT_AllocIndex(int numBytes, int type);
unsigned int *MT_Alloc(int numBytes, int type);
void MT_ForceAllocIndex(byte *allocBits, unsigned int nodeNum, int numBytes);
int MT_Realloc(int oldNumBytes, int newNumbytes);
void MT_Free(void *p, int numBytes);
void MT_FreeIndex(unsigned int nodeNum, int numBytes);
void MT_FinishForceAlloc(byte *allocBits);

#define MEMORY_NODE_BITS 16
#define MEMORY_NODE_COUNT 0x10000

#define NODE_PREV(i) (*(unsigned short *)(scrMemTreeGlob + (unsigned)(i) * 8))
#define NODE_NEXT(i) (*(unsigned short *)(scrMemTreeGlob + (unsigned)(i) * 8 + 2))

#define LOG_BITS(i) ((unsigned char)scrMemTreeGlob[0x80200 + (unsigned char)(i)])

#define MEMTREE_GLOB ((struct scrMemTreeGlob_t *)(scrMemTreeGlob))
#define TOTAL_ALLOC (MEMTREE_GLOB->totalAlloc)
#define TOTAL_ALLOC_BUCKETS (MEMTREE_GLOB->totalAllocBuckets)

static unsigned char mt_size[MEMORY_NODE_COUNT];

static int MT_GetSize(int numBytes)
{
    int numBuckets;
    if (numBytes > 0xFFFF) {
        MT_DumpTree();
        Com_Printf("%s: failed memory allocation of %d bytes for script usage\n",
                   "MT_GetSize: max allocation exceeded", numBytes);
        Scr_TerminalError("failed memory allocation for script usage");
        return 0;
    }
    numBuckets = (numBytes + 7) / 8 - 1;
    if (numBuckets < 0)
        numBuckets = 0;
    if (numBuckets > 255)
        return LOG_BITS((unsigned)numBuckets >> 8) + 8;
    return LOG_BITS(numBuckets);
}

static void MT_InitBits(void)
{
    int i, temp;
    unsigned char bits;

    for (i = 0; i < 256; i++) {

        bits = 0;
        for (temp = i; temp; temp >>= 1)
            if (temp & 1)
                bits++;
        scrMemTreeGlob[0x80100 + i] = bits;

        for (bits = 8; i & ((1 << bits) - 1); bits--) {}
        scrMemTreeGlob[0x80000 + i] = bits;

        bits = 0;
        for (temp = i; temp; temp >>= 1)
            bits++;
        scrMemTreeGlob[0x80200 + i] = bits;
    }
}

static void MT_ListAdd(int b, int size)
{
    unsigned short h = (MEMTREE_GLOB->head[(unsigned)(size)]);
    NODE_PREV(b) = 0;
    NODE_NEXT(b) = h;
    if (h)
        NODE_PREV(h) = (unsigned short)b;
    (MEMTREE_GLOB->head[(unsigned)(size)]) = (unsigned short)b;
    mt_size[b] = (unsigned char)(size + 1);
}

static void MT_ListRemove(int b, int size)
{
    unsigned short p = NODE_PREV(b);
    unsigned short n = NODE_NEXT(b);
    if (p)
        NODE_NEXT(p) = n;
    else
        (MEMTREE_GLOB->head[(unsigned)(size)]) = n;
    if (n)
        NODE_PREV(n) = p;
    mt_size[b] = 0;
}

static void MT_AddMemoryNode(int newNode, int size)
{    MT_ListAdd(newNode, size);
}

static void MT_RemoveHeadMemoryNode(int size)
{
    int b = (MEMTREE_GLOB->head[(unsigned)(size)]);    if (b)
        MT_ListRemove(b, size);
}

static int MT_RemoveMemoryNode(int target, int size)
{
    if (target <= 0 || target >= MEMORY_NODE_COUNT)
        return 0;
    if (mt_size[target] != (unsigned char)(size + 1))
        return 0;    MT_ListRemove(target, size);
    return 1;
}

int MT_IsNodeCovered(int nodeNum)
{
    int size;
    if (nodeNum <= 0 || nodeNum >= MEMORY_NODE_COUNT)
        return 0;
    for (size = 0; size <= MEMORY_NODE_BITS; size++) {
        int base = nodeNum & ~((1 << size) - 1);
        if (mt_size[base] == (unsigned char)(size + 1))
            return 1;
    }
    return 0;
}

int MT_SearchTreeAny(int nodeNum)
{
    if (nodeNum <= 0 || nodeNum >= MEMORY_NODE_COUNT)
        return 0;
    return mt_size[nodeNum] != 0;
}

void MT_VerifyNotInTree(int nodeNum)
{
    if (nodeNum > 0 && nodeNum < MEMORY_NODE_COUNT && mt_size[nodeNum])
        Com_Printf("TREE BUG: allocated node %d found free (size %d)!\n",
                   nodeNum, mt_size[nodeNum] - 1);
}

unsigned int Scr_GetStringUsage(void)
{
    return TOTAL_ALLOC_BUCKETS;
}

byte *MT_InitForceAlloc(void)
{
    TOTAL_ALLOC = 0;
    TOTAL_ALLOC_BUCKETS = 0;
    /* Rebuild the free lists around retained strings. Existing free nodes must
     * not participate in this pass or buddies can merge across live strings. */
    for (int i = 0; i <= MEMORY_NODE_BITS; i++)
        MEMTREE_GLOB->head[i] = 0;
    for (int i = 0; i < MEMORY_NODE_COUNT; i++)
        mt_size[i] = 0;
    return Z_VirtualAllocInternal(0x2000);
}

void MT_Init(void)
{
    int i;

    *(char **)&scrMemTreePub = (char *)scrMemTreeGlob;

    MT_InitBits();

    for (i = 0; i <= MEMORY_NODE_BITS; i++)
        (MEMTREE_GLOB->head[(unsigned)(i)]) = 0;
    for (i = 0; i < MEMORY_NODE_COUNT; i++)
        mt_size[i] = 0;

    NODE_PREV(0) = 0;
    NODE_NEXT(0) = 0;

    for (i = 0; i < MEMORY_NODE_BITS; i++)
        MT_AddMemoryNode(1 << i, i);

    TOTAL_ALLOC = 0;
    TOTAL_ALLOC_BUCKETS = 0;
}

void MT_DumpTree(void)
{
    int size, count;
    unsigned short n;

    Com_Printf("********************************\n");
    for (size = 0; size <= MEMORY_NODE_BITS; size++) {
        count = 0;
        for (n = (MEMTREE_GLOB->head[(unsigned)(size)]); n; n = NODE_NEXT(n))
            count++;
        Com_Printf("%d list has %d * %d = %d free buckets\n",
                   size, count, 1 << size, count << size);
    }
    Com_Printf("********************************\n");
    Com_Printf("total memory alloc buckets: %d (%d instances)\n",
               TOTAL_ALLOC_BUCKETS, TOTAL_ALLOC);
    Com_Printf("total memory free buckets: %d\n",
               MEMORY_NODE_COUNT - 1 - TOTAL_ALLOC_BUCKETS);
    Com_Printf("********************************\n");
}

short unsigned int MT_AllocIndex(int numBytes, int type)
{
    int size, newSize, nodeNum;

    (void)type;

    size = MT_GetSize(numBytes);
    if (size > MEMORY_NODE_BITS)
        return 0;

    for (newSize = size;; newSize++) {
        if (newSize > MEMORY_NODE_BITS) {
            MT_DumpTree();
            Com_Printf("%s: failed memory allocation of %d bytes for script usage\n",
                       "MT_AllocIndex", numBytes);
            Scr_TerminalError("failed memory allocation for script usage");
            return 0;
        }
        nodeNum = (MEMTREE_GLOB->head[(unsigned)(newSize)]);
        if (nodeNum)
            break;
    }

    MT_RemoveHeadMemoryNode(newSize);

    while (newSize != size) {
        newSize--;
        MT_AddMemoryNode(nodeNum + (1 << newSize), newSize);
    }

    TOTAL_ALLOC++;
    TOTAL_ALLOC_BUCKETS += 1 << size;

    return (unsigned short)nodeNum;
}

unsigned int *MT_Alloc(int numBytes, int type)
{
    unsigned short idx = MT_AllocIndex(numBytes, type);
    return (unsigned int *)(scrMemTreeGlob + (unsigned)idx * 8);
}

void MT_FreeIndex(unsigned int nodeNum, int numBytes)
{
    int size, lowBit;

    size = MT_GetSize(numBytes);
    TOTAL_ALLOC--;
    TOTAL_ALLOC_BUCKETS -= 1 << size;

    while (1) {
        lowBit = 1 << size;
        if (size == MEMORY_NODE_BITS || !MT_RemoveMemoryNode((int)(nodeNum ^ lowBit), size))
            break;
        nodeNum &= ~(unsigned)lowBit;
        size++;
    }

    MT_AddMemoryNode((int)nodeNum, size);
}

void MT_Free(void *p, int numBytes)
{
    unsigned int nodeIdx = ((unsigned char *)p - scrMemTreeGlob) / 8;
    MT_FreeIndex(nodeIdx, numBytes);
}

int MT_Realloc(int oldNumBytes, int newNumbytes)
{
    return MT_GetSize(oldNumBytes) >= MT_GetSize(newNumbytes);
}

void MT_ForceAllocIndex(byte *allocBits, unsigned int nodeNum, int numBytes)
{
    int size, count;

    size = MT_GetSize(numBytes);
    TOTAL_ALLOC++;
    count = 1 << size;
    TOTAL_ALLOC_BUCKETS += count;

    while (count) {
        allocBits[nodeNum >> 3] |= (1 << (nodeNum & 7));
        nodeNum++;
        count--;
    }
}

static void MT_SafeFreeIndex(unsigned int nodeNum)
{
    int size = 0, lowBit;

    while (1) {
        lowBit = 1 << size;
        if (size == MEMORY_NODE_BITS || !MT_RemoveMemoryNode((int)(nodeNum ^ lowBit), size))
            break;
        nodeNum &= ~(unsigned)lowBit;
        size++;
    }

    MT_AddMemoryNode((int)nodeNum, size);
}

void MT_FinishForceAlloc(byte *allocBits)
{
    unsigned int nodeNum;

    for (nodeNum = 1; nodeNum < MEMORY_NODE_COUNT; nodeNum++) {
        if (!(allocBits[nodeNum >> 3] >> (nodeNum & 7) & 1))
            MT_SafeFreeIndex(nodeNum);
    }

    Z_VirtualFreeInternal(allocBits);
}
