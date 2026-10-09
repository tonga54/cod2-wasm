#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include "downstream/wasm/web_pixels.h"
#endif

extern bool g_NoTextureID;
extern bool g_WarmOff;
typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CDirect3DSurface[];

void __ZdlPv(void *ptr);
bool MacOpenGLUtils_IsCompressed(const D3DFORMAT *f);
UINT32 MacOpenGLUtils_GetFormatSizeInBits(const D3DFORMAT *f);
int MacOpenGLUtils_GetOpenGLTextureType(bool *CreateOpenGLResources, GLenum *OpenGLInternalFormat, GLenum *OpenGLFormat, GLenum *OpenGLElementType, D3DFORMAT mFormat);
UINT32 MacOpenGLUtils_GetLevelSizeInBytes(UINT32 Width, UINT32 Height, UINT32 Depth, const D3DFORMAT *f);

typedef struct {
    void *QueryInterface;
    ULONG (*AddRef)(void *object);
    ULONG (*Release)(void *object);
} IUnknownLikeVTable;

typedef struct {
    void **vtable;
    ULONG refCount;
    SurfaceType surfaceType;
    GLenum cubemapTarget;
    UINT32 level;
    UINT32 width;
    UINT32 height;
    D3DFORMAT format;
    byte *surfaceMemory;
    unsigned char isDirty;
    unsigned char pad0[3];
    void *owner;
    unsigned char releaseOwnerOnDestroy;
    unsigned char pad1[3];
    GLenum openGLInternalFormat;
    GLenum openGLFormat;
    GLenum openGLElementType;
} CDirect3DSurfaceImpl;

/* Typed owner setter: CDirect3DTexture set surface->owner via a hardcoded x86
 * offset (surf+40), which on x64 is surfaceMemory (owner is at 56) -> corrupted
 * the pixel pointer. (x64 port Stage 4.) */
void CDirect3DSurface_SetOwner(void *surf, void *owner)
{
    ((CDirect3DSurfaceImpl *)surf)->owner = owner;
}

ULONG CDirect3DSurface_AddRef(const CDirect3DSurface *_this);
void CDirect3DSurface_UpdateOpenGLSurfaceObject(const CDirect3DSurface *_this, int bRecreateSurface);
void ZN16CDirect3DSurfaceD0Ev(const CDirect3DSurface *_this);
void ZN16CDirect3DSurfaceD1Ev(const CDirect3DSurface *_this);

static IUnknownLikeVTable *CDirect3DSurface_GetIUnknownVTable(void *object)
{
    return *(IUnknownLikeVTable **)object;
}

static void CDirect3DSurface_Destroy(CDirect3DSurfaceImpl *surface)
{
    surface->vtable = vtbl_CDirect3DSurface;
    if (surface->releaseOwnerOnDestroy && surface->owner) {
        CDirect3DSurface_GetIUnknownVTable(surface->owner)->Release(surface->owner);
    }
}

ULONG CDirect3DSurface_AddRef(const CDirect3DSurface *_this)
{
    CDirect3DSurfaceImpl *surface = (CDirect3DSurfaceImpl *)_this;
    return ++surface->refCount;
}

void ZN16CDirect3DSurfaceD1Ev(const CDirect3DSurface *_this)
{
    CDirect3DSurface_Destroy((CDirect3DSurfaceImpl *)_this);
}

void ZN16CDirect3DSurfaceD0Ev(const CDirect3DSurface *_this)
{
    CDirect3DSurface_Destroy((CDirect3DSurfaceImpl *)_this);
    __ZdlPv((void *)_this);
}

HRESULT CDirect3DSurface_QueryInterface(const CDirect3DSurface *_this, const IID *iid, void **ppvObj)
{
    (void)iid;
    *ppvObj = (void *)_this;
    CDirect3DSurface_AddRef(_this);
    return 0;
}

ULONG CDirect3DSurface_Release(const CDirect3DSurface *_this)
{
    CDirect3DSurfaceImpl *surface = (CDirect3DSurfaceImpl *)_this;
    ULONG rc = --surface->refCount;
    if (!rc) {
        ZN16CDirect3DSurfaceD0Ev(_this);
    }
    return rc;
}

HRESULT CDirect3DSurface_GetDesc(const CDirect3DSurface *_this, D3DSURFACE_DESC *pDesc)
{
    const CDirect3DSurfaceImpl *surface = (const CDirect3DSurfaceImpl *)_this;
    pDesc->Format = surface->format;
    pDesc->Width = surface->width;
    pDesc->Height = surface->height;
    return 0;
}

HRESULT CDirect3DSurface_LockRect(const CDirect3DSurface *_this, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags)
{
    CDirect3DSurfaceImpl *surface = (CDirect3DSurfaceImpl *)_this;
    UINT32 formatBits;

    (void)Flags;

    if (!surface->surfaceMemory || (unsigned int)surface->surfaceMemory >= 0xf0000000) {
        static byte scratch[4 * 1024 * 1024];
        pLockedRect->pBits = scratch;
    } else {
        pLockedRect->pBits = surface->surfaceMemory;
    }

    if (MacOpenGLUtils_IsCompressed(&surface->format)) {
        UINT32 blocksWide = surface->width ? surface->width + 3 : 4;
        UINT32 blockSize = surface->format == D3DFMT_DXT1 ? 8 : 16;
        pLockedRect->Pitch = (INT)(blockSize * (blocksWide >> 2));
    } else {
        formatBits = MacOpenGLUtils_GetFormatSizeInBits(&surface->format);
        pLockedRect->Pitch = (INT)((surface->width * formatBits) >> 3);
    }

    if (pRect) {
        byte *address = (byte *)pLockedRect->pBits;
        formatBits = MacOpenGLUtils_GetFormatSizeInBits(&surface->format);
        UINT32 byteOffset = ((UINT32)pRect->left * formatBits) >> 3;
        byteOffset += (UINT32)pRect->top * (UINT32)pLockedRect->Pitch;
        pLockedRect->pBits = address + byteOffset;
    }

    surface->isDirty = 1;
    return 0;
}

HRESULT CDirect3DSurface_UnlockRect(const CDirect3DSurface *_this)
{
    CDirect3DSurfaceImpl *surface = (CDirect3DSurfaceImpl *)_this;

    if (surface->isDirty && surface->surfaceMemory && surface->owner) {
        unsigned int texID = *(unsigned int *)((byte *)surface->owner + 0x54);
        if (texID) {
            int prevTex = 0;
            glGetIntegerv(0x8069 , &prevTex);
            glBindTexture(0x0DE1 , texID);
            CDirect3DSurface_UpdateOpenGLSurfaceObject(_this, 1);
            glBindTexture(0x0DE1, prevTex);
        }
    }

    return 0;
}

static void CDirect3DSurface_UploadPixels(CDirect3DSurfaceImpl *surface, GLenum target,
                                        UINT32 w, UINT32 h, int recreate)
{
    const void *pixels = surface->surfaceMemory;
    GLenum format = surface->openGLFormat;
#ifdef __EMSCRIPTEN__
    byte *rgba = NULL;
    if (format == 0x80E1 && surface->openGLElementType == 0x1401) {
        rgba = (byte *)malloc((size_t)w * h * 4);
        if (!rgba) abort();
        web_bgra_to_rgba(rgba, surface->surfaceMemory, (size_t)w * h);
        pixels = rgba;
        format = 0x1908;
    }
#endif
    if (recreate)
        glTexImage2D(target, surface->level, surface->openGLInternalFormat,
                     w, h, 0, format, surface->openGLElementType, pixels);
    else
        glTexSubImage2D(target, surface->level, 0, 0, w, h,
                       format, surface->openGLElementType, pixels);
#ifdef __EMSCRIPTEN__
    free(rgba);
#endif
}

void CDirect3DSurface_CreateOpenGLSurfaceObject(const CDirect3DSurface *_this)
{
    CDirect3DSurfaceImpl *surface = (CDirect3DSurfaceImpl *)_this;
    GLenum target;
    UINT32 w, h;

    if (!surface->surfaceMemory)
        return;

    if (surface->surfaceType == 1)
        target = surface->cubemapTarget;
    else
        target = 0x0DE1;

    w = surface->width ? surface->width : 1;
    h = surface->height ? surface->height : 1;

    if (MacOpenGLUtils_IsCompressed(&surface->format)) {
        UINT32 dataSize = MacOpenGLUtils_GetLevelSizeInBytes(w, h, 1, &surface->format);
        glCompressedTexImage2DARB(target, surface->level,
                                  surface->openGLInternalFormat, w, h, 0, dataSize, surface->surfaceMemory);
    } else {
        CDirect3DSurface_UploadPixels(surface, target, w, h, 1);
    }
}

void CDirect3DSurface_UpdateOpenGLSurfaceObject(const CDirect3DSurface *_this, int bRecreateSurface)
{
    CDirect3DSurfaceImpl *surface = (CDirect3DSurfaceImpl *)_this;
    GLenum target;
    UINT32 w, h;

    if (!surface->surfaceMemory)
        return;

    if (surface->surfaceType == 1)
        target = surface->cubemapTarget;
    else
        target = 0x0DE1;

    w = surface->width ? surface->width : 1;
    h = surface->height ? surface->height : 1;

    if (bRecreateSurface) {
        CDirect3DSurface_CreateOpenGLSurfaceObject(_this);
    } else if (MacOpenGLUtils_IsCompressed(&surface->format)) {
        UINT32 dataSize = MacOpenGLUtils_GetLevelSizeInBytes(w, h, 1, &surface->format);
        glCompressedTexSubImage2D(target, surface->level, 0, 0,
                                  w, h, surface->openGLInternalFormat, dataSize, surface->surfaceMemory);
    } else {
        CDirect3DSurface_UploadPixels(surface, target, w, h, 0);
    }

    surface->isDirty = 0;
}

bool CDirect3DSurface_IsDirty(const CDirect3DSurface *_this)
{
    CDirect3DSurfaceImpl *surface = (CDirect3DSurfaceImpl *)_this;
    return surface->isDirty;
}

void CDirect3DSurface_RecreateSurface(const CDirect3DSurface *_this)
{
    CDirect3DSurface_CreateOpenGLSurfaceObject(_this);
}

void CDirect3DSurface_CDirect3DSurface(const CDirect3DSurface *_this, SurfaceType s, GLenum CubemapID, UINT32 Level, UINT32 Width, UINT32 Height, D3DFORMAT Format, const void *pSurfaceMemory, COpenGLTexture *pOpenGLTextureInfo)
{
    CDirect3DSurfaceImpl *surface = (CDirect3DSurfaceImpl *)_this;
    bool createGL = 0;

    (void)pOpenGLTextureInfo;

    surface->vtable = vtbl_CDirect3DSurface;
    surface->refCount = 1;
    surface->surfaceType = s;
    surface->cubemapTarget = CubemapID;
    surface->level = Level;
    surface->width = Width;
    surface->height = Height;
    surface->format = Format;
    surface->surfaceMemory = (byte *)pSurfaceMemory;
    surface->isDirty = 0;
    surface->owner = NULL;
    surface->releaseOwnerOnDestroy = 0;
    surface->openGLInternalFormat = 0;
    surface->openGLFormat = 0;
    surface->openGLElementType = 0;

    MacOpenGLUtils_GetOpenGLTextureType(&createGL,
                                        &surface->openGLInternalFormat,
                                        &surface->openGLFormat,
                                        &surface->openGLElementType,
                                        Format);

    (void)createGL;
    (void)g_NoTextureID;
    (void)pSurfaceMemory;
}

void CDirect3DSurface_IgnorePixelStorei(GLenum pname, GLint param)
{
    (void)pname;
    (void)param;
}
void CDirect3DSurface_IgnoreTexParameteri(GLenum target, GLenum pname, GLint param)
{
    (void)target;
    (void)pname;
    (void)param;
}

HRESULT CDirect3DSurface_GetDevice(const CDirect3DSurface *_this, IDirect3DDevice9 **ppDevice)
{
    (void)_this;
    (void)ppDevice;
    return 0;
}
HRESULT CDirect3DSurface_SetPrivateData(const CDirect3DSurface *_this, const GUID *refguid, const void *pData, DWORD SizeOfData, DWORD Flags)
{
    (void)_this;
    (void)refguid;
    (void)pData;
    (void)SizeOfData;
    (void)Flags;
    return 0;
}
HRESULT CDirect3DSurface_GetPrivateData(const CDirect3DSurface *_this, const GUID *refguid, void *pData, DWORD *pSizeOfData)
{
    (void)_this;
    (void)refguid;
    (void)pData;
    (void)pSizeOfData;
    return 0;
}
HRESULT CDirect3DSurface_FreePrivateData(const CDirect3DSurface *_this, const GUID *refguid)
{
    (void)_this;
    (void)refguid;
    return 0;
}
DWORD CDirect3DSurface_SetPriority(const CDirect3DSurface *_this, DWORD PriorityNew)
{
    (void)_this;
    (void)PriorityNew;
    return 0;
}
DWORD CDirect3DSurface_GetPriority(const CDirect3DSurface *_this)
{
    (void)_this;
    return 0;
}
void CDirect3DSurface_PreLoad(const CDirect3DSurface *_this)
{
    (void)_this;
}
D3DRESOURCETYPE CDirect3DSurface_GetType(const CDirect3DSurface *_this)
{
    (void)_this;
    return D3DRTYPE_SURFACE;
}
HRESULT CDirect3DSurface_GetContainer(const CDirect3DSurface *_this, const IID *riid, void **ppContainer)
{
    (void)_this;
    (void)riid;
    (void)ppContainer;
    return 0;
}
HRESULT CDirect3DSurface_GetDC(const CDirect3DSurface *_this, HDC *phdc)
{
    (void)_this;
    (void)phdc;
    return 0;
}
HRESULT CDirect3DSurface_ReleaseDC(const CDirect3DSurface *_this, HDC hdc)
{
    (void)_this;
    (void)hdc;
    return 0;
}

fnptr_t vtbl_CDirect3DSurface[] = { (fnptr_t)CDirect3DSurface_QueryInterface, (fnptr_t)CDirect3DSurface_AddRef, (fnptr_t)CDirect3DSurface_Release, (fnptr_t)CDirect3DSurface_GetDevice, (fnptr_t)CDirect3DSurface_SetPrivateData, (fnptr_t)CDirect3DSurface_GetPrivateData, (fnptr_t)CDirect3DSurface_FreePrivateData, (fnptr_t)CDirect3DSurface_SetPriority, (fnptr_t)CDirect3DSurface_GetPriority, (fnptr_t)CDirect3DSurface_PreLoad, (fnptr_t)CDirect3DSurface_GetType, (fnptr_t)CDirect3DSurface_GetContainer, (fnptr_t)CDirect3DSurface_GetDesc, (fnptr_t)CDirect3DSurface_LockRect, (fnptr_t)CDirect3DSurface_UnlockRect, (fnptr_t)CDirect3DSurface_GetDC, (fnptr_t)CDirect3DSurface_ReleaseDC, (fnptr_t)ZN16CDirect3DSurfaceD1Ev, (fnptr_t)ZN16CDirect3DSurfaceD0Ev };
