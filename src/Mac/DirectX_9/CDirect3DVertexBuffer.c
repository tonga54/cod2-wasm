#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>
typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CDirect3DVertexBuffer[];
extern fnptr_t vtbl_CStaticCacheInfo[];
extern void *imp___ZTV15CCacheInfoBlock;

typedef struct {
    void **vtable;
    ULONG refCount;
    UINT32 lengthBytes;
    byte *data;
    DWORD usage;
#ifdef __EMSCRIPTEN__
    unsigned webBuffer;
    int webDirty;
    UINT webStride, webPhase;
    int webColorOffset, webColorOrder, webLocked;
#endif
} CDirect3DVertexBufferClean;

#ifdef __EMSCRIPTEN__
extern void glGenBuffers(int count, unsigned *buffers);
extern void glDeleteBuffers(int count, const unsigned *buffers);
extern void glBindBuffer(unsigned target, unsigned buffer);
extern void glBufferData(unsigned target, ptrdiff_t size, const void *data, unsigned usage);

/* Color order matches the bridge: RGBA=0, BGRA=1, ARGB=2. Stream phase
 * is part of the key because one buffer may serve different vertex layouts. */
unsigned CDirect3DVertexBuffer_BindStaticWebColors(const CDirect3DVertexBuffer *_this,
    UINT stride, int colorOffset, int colorOrder, UINT phase)
{
    CDirect3DVertexBufferClean *vb = (CDirect3DVertexBufferClean *)_this;
    byte *converted = NULL;
    const byte *upload = vb->data;
    size_t i;
    /* Dynamic tessellation, animated models and HUD keep the client-array path. */
    if ((vb->usage & 0x200) || vb->webLocked || !vb->data || !vb->lengthBytes) return 0;
    if (!colorOrder || colorOffset < 0) {
        stride = phase = 0;
        colorOffset = -1;
        colorOrder = 0;
    } else if (!stride || phase >= stride || (UINT)colorOffset > stride ||
        stride - (UINT)colorOffset < 4 || colorOrder > 2 || colorOrder < 0) {
        return 0;
    }
    if (vb->webStride != stride || vb->webPhase != phase ||
        vb->webColorOffset != colorOffset || vb->webColorOrder != colorOrder)
        vb->webDirty = 1;
    if (vb->webDirty && colorOrder) {
        converted = malloc(vb->lengthBytes);
        if (!converted) return 0;
        memcpy(converted, vb->data, vb->lengthBytes);
        /* Match the client-array RGBA conversion byte for byte. Only colors
         * change; positions, normals, texture coordinates and padding survive. */
        for (i = (size_t)phase + colorOffset; i + 4 <= vb->lengthBytes; i += stride) {
            const byte *src = vb->data + i;
            byte *dst = converted + i;
            if (colorOrder == 2) { /* ARGB */
                dst[0] = src[1]; dst[1] = src[2]; dst[2] = src[3]; dst[3] = src[0];
            } else { /* BGRA */
                dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0]; dst[3] = src[3];
            }
        }
        upload = converted;
    }
    if (!vb->webBuffer) {
        glGenBuffers(1, &vb->webBuffer);
        vb->webDirty = 1;
    }
    if (!vb->webBuffer) { free(converted); return 0; }
    glBindBuffer(0x8892, vb->webBuffer);
    if (vb->webDirty) {
        glBufferData(0x8892, vb->lengthBytes, upload, 0x88E4);
        vb->webDirty = 0;
        vb->webStride = stride;
        vb->webPhase = phase;
        vb->webColorOffset = colorOffset;
        vb->webColorOrder = colorOrder;
    }
    free(converted);
    return vb->webBuffer;
}

unsigned CDirect3DVertexBuffer_BindStaticWeb(const CDirect3DVertexBuffer *_this)
{
    return CDirect3DVertexBuffer_BindStaticWebColors(_this, 0, -1, 0, 0);
}
#endif

ULONG CDirect3DVertexBuffer_AddRef(const CDirect3DVertexBuffer *_this);
void ZN21CDirect3DVertexBufferD0Ev(void *_this);
void ZN21CDirect3DVertexBufferD1Ev(void *_this);
void ZN21CDirect3DVertexBufferD2Ev(void *_this);

ULONG CDirect3DVertexBuffer_AddRef(const CDirect3DVertexBuffer *_this)
{
    CDirect3DVertexBufferClean *vb = (CDirect3DVertexBufferClean *)_this;
    return ++vb->refCount;
}

HRESULT CDirect3DVertexBuffer_QueryInterface(const CDirect3DVertexBuffer *_this, const IID *iid, void **ppvObj)
{
    (void)iid;
    *ppvObj = (void *)_this;
    CDirect3DVertexBuffer_AddRef(_this);
    return 0;
}

ULONG CDirect3DVertexBuffer_Release(const CDirect3DVertexBuffer *_this)
{
    CDirect3DVertexBufferClean *vb = (CDirect3DVertexBufferClean *)_this;
    ULONG rc = --vb->refCount;
    if (!rc) {

        ((void (*)(void *))vb->vtable[15])((void *)_this);
    }
    return rc;
}

HRESULT CDirect3DVertexBuffer_Lock(const CDirect3DVertexBuffer *_this, UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags)
{
    CDirect3DVertexBufferClean *vb = (CDirect3DVertexBufferClean *)_this;
    (void)SizeToLock;
    (void)Flags;
#ifdef __EMSCRIPTEN__
    vb->webDirty = 1;
    vb->webLocked = 1;
#endif
    *ppbData = vb->data + OffsetToLock;
    return 0;
}

HRESULT CDirect3DVertexBuffer_Unlock(const CDirect3DVertexBuffer *_this)
{
#ifdef __EMSCRIPTEN__
    ((CDirect3DVertexBufferClean *)_this)->webDirty = 1;
    ((CDirect3DVertexBufferClean *)_this)->webLocked = 0;
#endif
    (void)_this;
    return 0;
}

HRESULT CDirect3DVertexBuffer_GetDesc(const CDirect3DVertexBuffer *_this, D3DVERTEXBUFFER_DESC *pDesc)
{
    CDirect3DVertexBufferClean *vb = (CDirect3DVertexBufferClean *)_this;
    pDesc->Format = 0;
    pDesc->Type = D3DRTYPE_VERTEXBUFFER;
    pDesc->Usage = vb->usage;
    pDesc->Pool = 0;
    pDesc->Size = vb->lengthBytes;
    pDesc->FVF = 0;
    return 0;
}

void ZN21CDirect3DVertexBufferD1Ev(void *_this)
{
    CDirect3DVertexBufferClean *vb = (CDirect3DVertexBufferClean *)_this;
    vb->vtable = vtbl_CDirect3DVertexBuffer;
#ifdef __EMSCRIPTEN__
    if (vb->webBuffer) glDeleteBuffers(1, &vb->webBuffer);
    vb->webBuffer = 0;
#endif
    free(vb->data);
    vb->data = NULL;
}

void ZN21CDirect3DVertexBufferD0Ev(void *_this)
{
    ZN21CDirect3DVertexBufferD1Ev(_this);
    free(_this);
}

void ZN21CDirect3DVertexBufferD2Ev(void *_this)
{
    ZN21CDirect3DVertexBufferD1Ev(_this);
}

void CDirect3DVertexBuffer_CDirect3DVertexBuffer(const CDirect3DVertexBuffer *_this, UINT32 Length, DWORD Usage, D3DPOOL Pool)
{
    CDirect3DVertexBufferClean *vb = (CDirect3DVertexBufferClean *)_this;
    (void)Pool;
    vb->vtable = vtbl_CDirect3DVertexBuffer;
    vb->refCount = 1;
    vb->lengthBytes = Length;
    vb->data = (byte *)calloc(1, Length);
    vb->usage = Usage;
#ifdef __EMSCRIPTEN__
    vb->webBuffer = 0;
    vb->webDirty = 1;
    vb->webStride = vb->webPhase = 0;
    vb->webColorOffset = -1;
    vb->webColorOrder = vb->webLocked = 0;
#endif
}

HRESULT CDirect3DVertexBuffer_GetDevice(const CDirect3DVertexBuffer *_this, IDirect3DDevice9 **ppDevice)
{
    (void)_this;
    (void)ppDevice;
    return 0;
}

HRESULT CDirect3DVertexBuffer_SetPrivateData(const CDirect3DVertexBuffer *_this, const GUID *refguid, const void *pData, DWORD SizeOfData, DWORD Flags)
{
    (void)_this;
    (void)refguid;
    (void)pData;
    (void)SizeOfData;
    (void)Flags;
    return 0;
}

HRESULT CDirect3DVertexBuffer_GetPrivateData(const CDirect3DVertexBuffer *_this, const GUID *refguid, void *pData, DWORD *pSizeOfData)
{
    (void)_this;
    (void)refguid;
    (void)pData;
    (void)pSizeOfData;
    return 0;
}

HRESULT CDirect3DVertexBuffer_FreePrivateData(const CDirect3DVertexBuffer *_this, const GUID *refguid)
{
    (void)_this;
    (void)refguid;
    return 0;
}

DWORD CDirect3DVertexBuffer_SetPriority(const CDirect3DVertexBuffer *_this, DWORD PriorityNew)
{
    (void)_this;
    (void)PriorityNew;
    return 0;
}

DWORD CDirect3DVertexBuffer_GetPriority(const CDirect3DVertexBuffer *_this)
{
    (void)_this;
    return 0;
}

void CDirect3DVertexBuffer_PreLoad(const CDirect3DVertexBuffer *_this)
{
    (void)_this;
}

D3DRESOURCETYPE CDirect3DVertexBuffer_GetType(const CDirect3DVertexBuffer *_this)
{
    (void)_this;
    return D3DRTYPE_VERTEXBUFFER;
}

void CDirect3DVertexBuffer_WaitFence(const CDirect3DVertexBuffer *_this, const void *pStart, UINT32 SizeInBytes)
{
    (void)_this;
    (void)pStart;
    (void)SizeInBytes;
}

void CDirect3DVertexBuffer_SetFence(const CDirect3DVertexBuffer *_this, const void *pStart, UINT32 SizeInBytes, const CDirect3DVertexBuffer *_this_3)
{
    (void)_this;
    (void)pStart;
    (void)SizeInBytes;
    (void)_this_3;
}

void ZN16CStaticCacheInfoD0Ev(void *_this)
{
    free(_this);
}

void ZN16CStaticCacheInfoD1Ev(void *_this)
{
    (void)_this;
}

void ZN16CStaticCacheInfoD2Ev(void *_this)
{
    (void)_this;
}

void CStaticCacheInfo_Flush(const CStaticCacheInfo *_this, void (*pStart)(), void (*pEnd)())
{
    (void)_this;
    (void)pStart;
    (void)pEnd;
}

void ZN15CCacheInfoBlockD1Ev(void *_this)
{
    (void)_this;
}

void ZN15CCacheInfoBlockD0Ev(void *_this)
{
    free(_this);
}

void *ZNSt4listIP6CFenceSaIS1_EE5eraseESt14_List_iteratorIS1_E(void *list, void *pos)
{
    (void)list;
    return pos;
}

void ZNSt6vectorI15CCacheInfoBlockSaIS0_EEC1Em(void *vec, unsigned long n)
{
    (void)vec;
    (void)n;
}

void ZNSt6vectorImSaImEE13_M_insert_auxEN9__gnu_cxx17__normal_iteratorIPmS1_EERKm(void *vec, void *pos, const unsigned long *val)
{
    (void)vec;
    (void)pos;
    (void)val;
}

fnptr_t vtbl_CStaticCacheInfo[] = { (fnptr_t)ZN16CStaticCacheInfoD1Ev, (fnptr_t)ZN16CStaticCacheInfoD0Ev };
fnptr_t vtbl_CDirect3DVertexBuffer[] = { (fnptr_t)CDirect3DVertexBuffer_QueryInterface, (fnptr_t)CDirect3DVertexBuffer_AddRef, (fnptr_t)CDirect3DVertexBuffer_Release, (fnptr_t)CDirect3DVertexBuffer_GetDevice, (fnptr_t)CDirect3DVertexBuffer_SetPrivateData, (fnptr_t)CDirect3DVertexBuffer_GetPrivateData, (fnptr_t)CDirect3DVertexBuffer_FreePrivateData, (fnptr_t)CDirect3DVertexBuffer_SetPriority, (fnptr_t)CDirect3DVertexBuffer_GetPriority, (fnptr_t)CDirect3DVertexBuffer_PreLoad, (fnptr_t)CDirect3DVertexBuffer_GetType, (fnptr_t)CDirect3DVertexBuffer_Lock, (fnptr_t)CDirect3DVertexBuffer_Unlock, (fnptr_t)CDirect3DVertexBuffer_GetDesc, (fnptr_t)ZN21CDirect3DVertexBufferD1Ev, (fnptr_t)ZN21CDirect3DVertexBufferD0Ev };
