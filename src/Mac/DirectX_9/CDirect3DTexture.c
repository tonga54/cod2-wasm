#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>
typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CDirect3DTexture[];
extern void *vtbl_CDirect3DTexture_secondary[];
extern void *vtbl_CDirect3DSurface[];

void __ZdlPv(void *ptr);
UINT32 MacOpenGLUtils_GetLevelSizeInBytes(UINT32 Width, UINT32 Height, UINT32 Depth, const D3DFORMAT *f);
UINT32 MacOpenGLUtils_GetNumTextureLevels(UINT32 Width, UINT32 Height, UINT32 Depth);
int MacOpenGLUtils_GetOpenGLTextureType(bool *CreateOpenGLResources, GLenum *OpenGLInternalFormat, GLenum *OpenGLFormat, GLenum *OpenGLElementType, D3DFORMAT mFormat);
void CDirect3DSurface_CDirect3DSurface(const CDirect3DSurface *_this, SurfaceType s, GLenum CubemapID, UINT32 Level, UINT32 Width, UINT32 Height, D3DFORMAT Format, const void *pSurfaceMemory, COpenGLTexture *pOpenGLTextureInfo);
void CDirect3DSurface_CreateOpenGLSurfaceObject(const CDirect3DSurface *_this);
ULONG CDirect3DSurface_AddRef(const CDirect3DSurface *_this);
ULONG CDirect3DSurface_Release(const CDirect3DSurface *_this);
void CDirect3DSurface_UpdateOpenGLSurfaceObject(const CDirect3DSurface *_this, int bRecreateSurface);
bool CDirect3DSurface_IsDirty(const CDirect3DSurface *_this);

#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_LINEAR 0x2601
#define GL_REPEAT 0x2901
#define GL_TEXTURE_BINDING_2D 0x8069
#define MAX_MIP_LEVELS 16

typedef struct {
    void **primaryVtable;
    void **secondaryVtable;
    GLuint *mpTexID;
    GLenum mTexWrapS;
    GLenum mTexWrapT;
    GLenum mTexWrapR;
    GLuint mTexBorderColor;
    GLenum mTexMinFilter;
    GLenum mTexMagFilter;
    GLfloat mTexAniso;
    UINT32 reserved;
    GLint glWidth;
    GLint glHeight;
    GLint glDepth;
    ULONG refCount;
    UINT32 width;
    UINT32 height;
    UINT32 levelCount;
    D3DFORMAT format;
    void **surfaces;
    byte *pixelData;
    GLuint texIDStorage;
    UINT32 dirtyMask;
} CDirect3DTextureClean;

ULONG CDirect3DTexture_AddRef(const CDirect3DTexture *_this);
void ZN16CDirect3DTextureD0Ev(void *_this);
void ZN16CDirect3DTextureD1Ev(void *_this);

ULONG CDirect3DTexture_AddRef(const CDirect3DTexture *_this)
{
    CDirect3DTextureClean *tex = (CDirect3DTextureClean *)_this;
    return ++tex->refCount;
}

HRESULT CDirect3DTexture_QueryInterface(const CDirect3DTexture *_this, const IID *iid, void **ppvObj)
{
    (void)iid;
    *ppvObj = (void *)_this;
    CDirect3DTexture_AddRef(_this);
    return 0;
}

ULONG CDirect3DTexture_Release(const CDirect3DTexture *_this)
{
    CDirect3DTextureClean *tex = (CDirect3DTextureClean *)_this;
    ULONG rc = --tex->refCount;
    if (!rc) {
        ZN16CDirect3DTextureD0Ev((void *)_this);
    }
    return rc;
}

HRESULT CDirect3DTexture_GetLevelDesc(const CDirect3DTexture *_this, UINT Level, D3DSURFACE_DESC *pDesc)
{
    CDirect3DTextureClean *tex = (CDirect3DTextureClean *)_this;
    HRESULT CDirect3DSurface_GetDesc(const CDirect3DSurface *_this, D3DSURFACE_DESC *pDesc);
    if (Level >= tex->levelCount)
        return -1;
    return CDirect3DSurface_GetDesc((CDirect3DSurface *)tex->surfaces[Level], pDesc);
}

HRESULT CDirect3DTexture_GetSurfaceLevel(const CDirect3DTexture *_this, UINT Level, IDirect3DSurface9 **ppSurfaceLevel)
{
    CDirect3DTextureClean *tex = (CDirect3DTextureClean *)_this;
    if (Level >= tex->levelCount)
        return -1;
    CDirect3DSurface *surface = (CDirect3DSurface *)tex->surfaces[Level];
    CDirect3DSurface_AddRef(surface);
    *ppSurfaceLevel = (IDirect3DSurface9 *)surface;
    return 0;
}

HRESULT CDirect3DTexture_LockRect(const CDirect3DTexture *_this, UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags)
{
    CDirect3DTextureClean *tex = (CDirect3DTextureClean *)_this;
    HRESULT hr;
    HRESULT CDirect3DSurface_LockRect(const CDirect3DSurface *_this, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags);
    if (Level >= tex->levelCount)
        return -1;
    hr = CDirect3DSurface_LockRect((CDirect3DSurface *)tex->surfaces[Level], pLockedRect, pRect, Flags);
    if (hr == 0)
        tex->dirtyMask |= 1u << Level;
    return hr;
}

HRESULT CDirect3DTexture_UnlockRect(const CDirect3DTexture *_this, UINT Level)
{
    CDirect3DTextureClean *tex = (CDirect3DTextureClean *)_this;
    CDirect3DSurface *surface;

    if (Level >= tex->levelCount)
        return -1;
    surface = (CDirect3DSurface *)tex->surfaces[Level];

    if (CDirect3DSurface_IsDirty(surface)) {
        int prevTex = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
        glBindTexture(GL_TEXTURE_2D, tex->texIDStorage);
        CDirect3DSurface_UpdateOpenGLSurfaceObject(surface, 0);
        glBindTexture(GL_TEXTURE_2D, prevTex);
    }

    tex->dirtyMask &= ~(1u << Level);

    return 0;
}

HRESULT CDirect3DTexture_AddDirtyRect(const CDirect3DTexture *_this, const RECT *pDirtyRect)
{
    (void)_this;
    (void)pDirtyRect;
    return 0;
}

DWORD CDirect3DTexture_SetLOD(const CDirect3DTexture *_this, DWORD LODNew)
{
    (void)_this;
    (void)LODNew;
    return 0;
}
DWORD CDirect3DTexture_GetLOD(const CDirect3DTexture *_this)
{
    (void)_this;
    return 0;
}

DWORD CDirect3DTexture_GetLevelCount(const CDirect3DTexture *_this)
{
    CDirect3DTextureClean *tex = (CDirect3DTextureClean *)_this;
    return tex->levelCount;
}

D3DRESOURCETYPE CDirect3DTexture_GetType(const CDirect3DTexture *_this)
{
    (void)_this;
    return D3DRTYPE_TEXTURE;
}
HRESULT CDirect3DTexture_SetAutoGenFilterType(const CDirect3DTexture *_this, D3DTEXTUREFILTERTYPE FilterType)
{
    (void)_this;
    (void)FilterType;
    return 0;
}
D3DTEXTUREFILTERTYPE CDirect3DTexture_GetAutoGenFilterType(const CDirect3DTexture *_this)
{
    (void)_this;
    return 0;
}
void CDirect3DTexture_GenerateMipSubLevels(const CDirect3DTexture *_this)
{
    (void)_this;
}

void ZN16CDirect3DTextureD1Ev(void *_this)
{
    CDirect3DTextureClean *tex = (CDirect3DTextureClean *)_this;
    UINT32 i;

    tex->primaryVtable = vtbl_CDirect3DTexture;
    tex->secondaryVtable = vtbl_CDirect3DTexture_secondary;

    if (tex->surfaces) {
        for (i = 0; i < tex->levelCount; i++) {
            if (tex->surfaces[i])
                __ZdlPv(tex->surfaces[i]);
        }
        free(tex->surfaces);
        tex->surfaces = NULL;
    }

    if (tex->texIDStorage) {
        glDeleteTextures(1, &tex->texIDStorage);
        tex->texIDStorage = 0;
    }

    free(tex->pixelData);
    tex->pixelData = NULL;
}

void ZN16CDirect3DTextureD0Ev(void *_this)
{
    ZN16CDirect3DTextureD1Ev(_this);
    free(_this);
}

void ZN16CDirect3DTextureD2Ev(void *_this)
{
    ZN16CDirect3DTextureD1Ev(_this);
}

void CDirect3DTexture_CDirect3DTexture(const CDirect3DTexture *_this, UINT32 Width, UINT32 Height, UINT32 Levels, DWORD Usage, D3DFORMAT Format)
{
    CDirect3DTextureClean *tex = (CDirect3DTextureClean *)_this;
    UINT32 i, w, h, totalSize, offset;
    bool createGL = 0;
    GLenum internalFmt = 0, glFmt = 0, glType = 0;
    int prevTex = 0;

    (void)Usage;

    memset(tex, 0, sizeof(*tex));

    tex->primaryVtable = vtbl_CDirect3DTexture;
    tex->secondaryVtable = vtbl_CDirect3DTexture_secondary;
    tex->mpTexID = &tex->texIDStorage;
    tex->mTexWrapS = GL_REPEAT;
    tex->mTexWrapT = GL_REPEAT;
    tex->mTexMinFilter = GL_LINEAR;
    tex->mTexMagFilter = GL_LINEAR;
    tex->mTexAniso = 1.0f;
    tex->glWidth = Width;
    tex->glHeight = Height;
    tex->glDepth = 1;
    tex->refCount = 1;
    tex->width = Width;
    tex->height = Height;
    tex->format = Format;

    if (Levels == 0)
        tex->levelCount = MacOpenGLUtils_GetNumTextureLevels(Width, Height, 1);
    else
        tex->levelCount = Levels;

    if (tex->levelCount > MAX_MIP_LEVELS)
        tex->levelCount = MAX_MIP_LEVELS;

    MacOpenGLUtils_GetOpenGLTextureType(&createGL, &internalFmt, &glFmt, &glType, Format);

    glGenTextures(1, &tex->texIDStorage);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glBindTexture(GL_TEXTURE_2D, tex->texIDStorage);
#ifdef __EMSCRIPTEN__
    /* Partial authored mip chains are complete in D3D. WebGL otherwise
     * samples black when a material requests mip filtering beyond that chain.
     * Use the existing levels, without generating textures or extra passes. */
    glTexParameteri(GL_TEXTURE_2D, 0x813D, tex->levelCount - 1);
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    totalSize = 0;
    w = Width;
    h = Height;
    for (i = 0; i < tex->levelCount; i++) {
        totalSize += MacOpenGLUtils_GetLevelSizeInBytes(w ? w : 1, h ? h : 1, 1, &Format);
        w >>= 1;
        h >>= 1;
    }
    tex->pixelData = (byte *)calloc(1, totalSize ? totalSize : 1);

    tex->surfaces = (void **)calloc(tex->levelCount, sizeof(void *));
    offset = 0;
    w = Width;
    h = Height;
    for (i = 0; i < tex->levelCount; i++) {
        UINT32 levelW = w ? w : 1;
        UINT32 levelH = h ? h : 1;
        UINT32 levelSize = MacOpenGLUtils_GetLevelSizeInBytes(levelW, levelH, 1, &Format);
        void *surf = calloc(1, 128);
        CDirect3DSurface_CDirect3DSurface((CDirect3DSurface *)surf, 0, 0, i,
                                          levelW, levelH, Format, tex->pixelData + offset, NULL);

        {   /* set surface->owner = tex via typed setter (was surf+40, an x86
             * offset; on x64 that's surfaceMemory -> corrupted the pixel ptr). */
            extern void CDirect3DSurface_SetOwner(void *surf, void *owner);
            CDirect3DSurface_SetOwner(surf, tex);
        }
        tex->surfaces[i] = surf;

        if (createGL)
            CDirect3DSurface_CreateOpenGLSurfaceObject((CDirect3DSurface *)surf);

        offset += levelSize;
        w >>= 1;
        h >>= 1;
    }

    glBindTexture(GL_TEXTURE_2D, prevTex);
}

void CDirect3DTexture_UpdateOpenGLSurfaces(const CDirect3DTexture *_this)
{
    CDirect3DTextureClean *tex = (CDirect3DTextureClean *)_this;
    UINT32 dirtyMask;
    unsigned int i;
    int prevTex = 0;

    dirtyMask = tex->dirtyMask;

    if (!dirtyMask || !tex->surfaces || !tex->texIDStorage)
        return;

    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glBindTexture(GL_TEXTURE_2D, tex->texIDStorage);

    for (i = 0; i < tex->levelCount; i++) {
        CDirect3DSurface *surf = (CDirect3DSurface *)tex->surfaces[i];
        if (!(dirtyMask & (1u << i)))
            continue;
        if (surf && CDirect3DSurface_IsDirty(surf))
            CDirect3DSurface_UpdateOpenGLSurfaceObject(surf, 0);
        tex->dirtyMask &= ~(1u << i);
    }

    glBindTexture(GL_TEXTURE_2D, prevTex);
}

HRESULT CDirect3DTexture_GetDevice(const CDirect3DTexture *_this, IDirect3DDevice9 **ppDevice)
{
    (void)_this;
    (void)ppDevice;
    return 0;
}
HRESULT CDirect3DTexture_SetPrivateData(const CDirect3DTexture *_this, const GUID *refguid, const void *pData, DWORD SizeOfData, DWORD Flags)
{
    (void)_this;
    (void)refguid;
    (void)pData;
    (void)SizeOfData;
    (void)Flags;
    return 0;
}
HRESULT CDirect3DTexture_GetPrivateData(const CDirect3DTexture *_this, const GUID *refguid, void *pData, DWORD *pSizeOfData)
{
    (void)_this;
    (void)refguid;
    (void)pData;
    (void)pSizeOfData;
    return 0;
}
HRESULT CDirect3DTexture_FreePrivateData(const CDirect3DTexture *_this, const GUID *refguid)
{
    (void)_this;
    (void)refguid;
    return 0;
}
DWORD CDirect3DTexture_SetPriority(const CDirect3DTexture *_this, DWORD PriorityNew)
{
    (void)_this;
    (void)PriorityNew;
    return 0;
}
DWORD CDirect3DTexture_GetPriority(const CDirect3DTexture *_this)
{
    (void)_this;
    return 0;
}
void CDirect3DTexture_PreLoad(const CDirect3DTexture *_this)
{
    (void)_this;
}

fnptr_t vtbl_CDirect3DTexture[] = { (fnptr_t)CDirect3DTexture_QueryInterface, (fnptr_t)CDirect3DTexture_AddRef, (fnptr_t)CDirect3DTexture_Release, (fnptr_t)CDirect3DTexture_GetDevice, (fnptr_t)CDirect3DTexture_SetPrivateData, (fnptr_t)CDirect3DTexture_GetPrivateData, (fnptr_t)CDirect3DTexture_FreePrivateData, (fnptr_t)CDirect3DTexture_SetPriority, (fnptr_t)CDirect3DTexture_GetPriority, (fnptr_t)CDirect3DTexture_PreLoad, (fnptr_t)CDirect3DTexture_GetType, (fnptr_t)CDirect3DTexture_SetLOD, (fnptr_t)CDirect3DTexture_GetLOD, (fnptr_t)CDirect3DTexture_GetLevelCount, (fnptr_t)CDirect3DTexture_SetAutoGenFilterType, (fnptr_t)CDirect3DTexture_GetAutoGenFilterType, (fnptr_t)CDirect3DTexture_GenerateMipSubLevels, (fnptr_t)CDirect3DTexture_GetLevelDesc, (fnptr_t)CDirect3DTexture_GetSurfaceLevel, (fnptr_t)CDirect3DTexture_LockRect, (fnptr_t)CDirect3DTexture_UnlockRect, (fnptr_t)CDirect3DTexture_AddDirtyRect, (fnptr_t)ZN16CDirect3DTextureD1Ev, (fnptr_t)ZN16CDirect3DTextureD0Ev };
