#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>
typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CDirect3DIndexBuffer[];

#ifdef __EMSCRIPTEN__
#define WEB_INDEX_RANGE_SLOTS 256
typedef struct {
    UINT start, count, firstVertex, vertexCount;
} WebIndexRange;
#endif

typedef struct {
    void **vtable;
    ULONG refCount;
    UINT32 lengthBytes;
    byte *data;
    UINT32 indexSizeBytes;
    DWORD usage;
    byte *lockPtr;
    UINT32 lockSize;
    unsigned char isLocked;
#ifdef __EMSCRIPTEN__
    unsigned webBuffer;
    int webDirty;
    WebIndexRange *webRanges;
#endif
} CDirect3DIndexBufferClean;

#ifdef __EMSCRIPTEN__
extern void glGenBuffers(int count, unsigned *buffers);
extern void glDeleteBuffers(int count, const unsigned *buffers);
extern void glBindBuffer(unsigned target, unsigned buffer);
extern void glBufferData(unsigned target, ptrdiff_t size, const void *data, unsigned usage);

/* Only unlocked, non-dynamic 16-bit data can persist across frames. Range
 * metadata is bounded and invalidated before every write, including partial
 * locks and address reuse after destruction. Indices themselves never change. */
int CDirect3DIndexBuffer_StaticWebRange(const CDirect3DIndexBuffer *_this,
    UINT start, UINT count, UINT *firstVertex, UINT *vertexCount)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    WebIndexRange *range;
    const unsigned short *indices;
    UINT i, lo = 65535, hi = 0;
    unsigned hash;
    if ((ib->usage & 0x200) || ib->isLocked || ib->indexSizeBytes != 2 ||
        !ib->data || !count || start > ib->lengthBytes / 2 ||
        count > ib->lengthBytes / 2 - start)
        return 0;
    if (!ib->webRanges)
        ib->webRanges = calloc(WEB_INDEX_RANGE_SLOTS, sizeof(*ib->webRanges));
    if (!ib->webRanges)
        return 0;
    hash = start * 2654435761u ^ count * 2246822519u;
    hash ^= hash >> 16;
    range = &ib->webRanges[hash & (WEB_INDEX_RANGE_SLOTS - 1)];
    if (range->count != count || range->start != start) {
        indices = (const unsigned short *)ib->data + start;
        for (i = 0; i < count; ++i) {
            if (indices[i] < lo) lo = indices[i];
            if (indices[i] > hi) hi = indices[i];
        }
        range->start = start;
        range->count = count;
        range->firstVertex = lo;
        range->vertexCount = hi - lo + 1;
    }
    *firstVertex = range->firstVertex;
    *vertexCount = range->vertexCount;
    return 1;
}

unsigned CDirect3DIndexBuffer_BindStaticWeb(const CDirect3DIndexBuffer *_this)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    if ((ib->usage & 0x200) || ib->isLocked || ib->indexSizeBytes != 2 ||
        !ib->data || !ib->lengthBytes)
        return 0;
    if (!ib->webBuffer) {
        glGenBuffers(1, &ib->webBuffer);
        ib->webDirty = 1;
    }
    if (!ib->webBuffer)
        return 0;
    glBindBuffer(0x8893, ib->webBuffer);
    if (ib->webDirty) {
        glBufferData(0x8893, ib->lengthBytes, ib->data, 0x88E4);
        ib->webDirty = 0;
    }
    return ib->webBuffer;
}
#endif

ULONG CDirect3DIndexBuffer_AddRef(const CDirect3DIndexBuffer *_this);
void ZN20CDirect3DIndexBufferD0Ev(const CDirect3DIndexBuffer *_this);
void ZN20CDirect3DIndexBufferD1Ev(const CDirect3DIndexBuffer *_this);

ULONG CDirect3DIndexBuffer_AddRef(const CDirect3DIndexBuffer *_this)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    return ++ib->refCount;
}

HRESULT CDirect3DIndexBuffer_QueryInterface(const CDirect3DIndexBuffer *_this, const IID *iid, LPVOID *ppvObj)
{
    (void)iid;
    *ppvObj = (LPVOID)_this;
    CDirect3DIndexBuffer_AddRef(_this);
    return 0;
}

ULONG CDirect3DIndexBuffer_Release(const CDirect3DIndexBuffer *_this)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    ULONG rc = --ib->refCount;
    if (!rc) {
        ZN20CDirect3DIndexBufferD0Ev(_this);
    }
    return rc;
}

void ZN20CDirect3DIndexBufferD1Ev(const CDirect3DIndexBuffer *_this)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    ib->vtable = vtbl_CDirect3DIndexBuffer;
#ifdef __EMSCRIPTEN__
    if (ib->webBuffer) glDeleteBuffers(1, &ib->webBuffer);
    free(ib->webRanges);
    ib->webRanges = NULL;
    ib->webBuffer = 0;
#endif
    free(ib->data);
    ib->data = NULL;
}

void ZN20CDirect3DIndexBufferD0Ev(const CDirect3DIndexBuffer *_this)
{
    ZN20CDirect3DIndexBufferD1Ev(_this);
    free((void *)_this);
}

HRESULT CDirect3DIndexBuffer_Lock(const CDirect3DIndexBuffer *_this, UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    (void)Flags;
#ifdef __EMSCRIPTEN__
    ib->webDirty = 1;
    if (ib->webRanges)
        memset(ib->webRanges, 0, WEB_INDEX_RANGE_SLOTS * sizeof(*ib->webRanges));
#endif
    ib->isLocked = 1;
    ib->lockSize = SizeToLock ? SizeToLock : ib->lengthBytes;
    ib->lockPtr = ib->data + OffsetToLock;
    *ppbData = ib->lockPtr;
    return 0;
}

HRESULT CDirect3DIndexBuffer_Unlock(const CDirect3DIndexBuffer *_this)
{
    ((CDirect3DIndexBufferClean *)_this)->isLocked = 0;
    return 0;
}

HRESULT CDirect3DIndexBuffer_GetDesc(const CDirect3DIndexBuffer *_this, D3DINDEXBUFFER_DESC *pDesc)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    pDesc->Format = ib->indexSizeBytes == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32;
    pDesc->Type = D3DRTYPE_INDEXBUFFER;
    pDesc->Usage = ib->usage;
    pDesc->Pool = 0;
    pDesc->Size = ib->lengthBytes;
    return 0;
}

void CDirect3DIndexBuffer_CDirect3DIndexBuffer(const CDirect3DIndexBuffer *_this, UINT32 Length, D3DFORMAT Format, DWORD Usage, D3DPOOL Pool)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    (void)Pool;
    ib->vtable = vtbl_CDirect3DIndexBuffer;
    ib->refCount = 1;
    ib->lengthBytes = (Length + 3) & ~3U;
    ib->data = (byte *)calloc(1, ib->lengthBytes);

    ib->indexSizeBytes = (Format == D3DFMT_INDEX16) ? 2 : 4;
    ib->usage = Usage;
    ib->lockPtr = NULL;
    ib->lockSize = 0;
    ib->isLocked = 0;
#ifdef __EMSCRIPTEN__
    ib->webBuffer = 0;
    ib->webDirty = 1;
    ib->webRanges = NULL;
#endif
}

HRESULT CDirect3DIndexBuffer_GetDevice(const CDirect3DIndexBuffer *_this, IDirect3DDevice9 **ppDevice)
{
    (void)_this;
    (void)ppDevice;
    return 0;
}

HRESULT CDirect3DIndexBuffer_SetPrivateData(const CDirect3DIndexBuffer *_this, const GUID *refguid, const void *pData, DWORD SizeOfData, DWORD Flags)
{
    (void)_this;
    (void)refguid;
    (void)pData;
    (void)SizeOfData;
    (void)Flags;
    return 0;
}

HRESULT CDirect3DIndexBuffer_GetPrivateData(const CDirect3DIndexBuffer *_this, const GUID *refguid, void *pData, DWORD *pSizeOfData)
{
    (void)_this;
    (void)refguid;
    (void)pData;
    (void)pSizeOfData;
    return 0;
}

HRESULT CDirect3DIndexBuffer_FreePrivateData(const CDirect3DIndexBuffer *_this, const GUID *refguid)
{
    (void)_this;
    (void)refguid;
    return 0;
}

DWORD CDirect3DIndexBuffer_SetPriority(const CDirect3DIndexBuffer *_this, DWORD PriorityNew)
{
    (void)_this;
    (void)PriorityNew;
    return 0;
}

DWORD CDirect3DIndexBuffer_GetPriority(const CDirect3DIndexBuffer *_this)
{
    (void)_this;
    return 0;
}

void CDirect3DIndexBuffer_PreLoad(const CDirect3DIndexBuffer *_this)
{
    (void)_this;
}

D3DRESOURCETYPE CDirect3DIndexBuffer_GetType(const CDirect3DIndexBuffer *_this)
{
    (void)_this;
    return D3DRTYPE_INDEXBUFFER;
}

fnptr_t vtbl_CDirect3DIndexBuffer[] = { (fnptr_t)CDirect3DIndexBuffer_QueryInterface, (fnptr_t)CDirect3DIndexBuffer_AddRef, (fnptr_t)CDirect3DIndexBuffer_Release, (fnptr_t)CDirect3DIndexBuffer_GetDevice, (fnptr_t)CDirect3DIndexBuffer_SetPrivateData, (fnptr_t)CDirect3DIndexBuffer_GetPrivateData, (fnptr_t)CDirect3DIndexBuffer_FreePrivateData, (fnptr_t)CDirect3DIndexBuffer_SetPriority, (fnptr_t)CDirect3DIndexBuffer_GetPriority, (fnptr_t)CDirect3DIndexBuffer_PreLoad, (fnptr_t)CDirect3DIndexBuffer_GetType, (fnptr_t)CDirect3DIndexBuffer_Lock, (fnptr_t)CDirect3DIndexBuffer_Unlock, (fnptr_t)CDirect3DIndexBuffer_GetDesc, (fnptr_t)ZN20CDirect3DIndexBufferD1Ev, (fnptr_t)ZN20CDirect3DIndexBufferD0Ev };
