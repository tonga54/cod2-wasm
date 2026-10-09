#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>
typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CDirect3DCubeTexture[];
extern fnptr_t vtbl_CDirect3DCubeTexture_secondary[];

void __ZdlPv(void *ptr);
UINT32 MacOpenGLUtils_GetLevelSizeInBytes(UINT32 Width, UINT32 Height, UINT32 Depth, const D3DFORMAT *f);
UINT32 MacOpenGLUtils_GetNumTextureLevels(UINT32 Width, UINT32 Height, UINT32 Depth);
int MacOpenGLUtils_GetOpenGLTextureType(bool *CreateOpenGLResources, GLenum *OpenGLInternalFormat, GLenum *OpenGLFormat, GLenum *OpenGLElementType, D3DFORMAT mFormat);
void CDirect3DSurface_CDirect3DSurface(const CDirect3DSurface *_this, SurfaceType s, GLenum CubemapID, UINT32 Level, UINT32 Width, UINT32 Height, D3DFORMAT Format, const void *pSurfaceMemory, COpenGLTexture *pOpenGLTextureInfo);
void CDirect3DSurface_CreateOpenGLSurfaceObject(const CDirect3DSurface *_this);
ULONG CDirect3DSurface_AddRef(const CDirect3DSurface *_this);
void CDirect3DSurface_UpdateOpenGLSurfaceObject(const CDirect3DSurface *_this, int bRecreateSurface);
bool CDirect3DSurface_IsDirty(const CDirect3DSurface *_this);

#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_LINEAR 0x2601
#define GL_REPEAT 0x2901
#define GL_TEXTURE_BINDING_CUBE_MAP 0x8514
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
    UINT32 edgeLength;
    UINT32 pad_height;
    UINT32 levelCount;
    D3DFORMAT format;
    void **surfaces;
    byte *pixelData;
    GLuint texIDStorage;
} CDirect3DCubeTextureClean;

ULONG CDirect3DCubeTexture_AddRef(const CDirect3DCubeTexture *_this);
void ZN20CDirect3DCubeTextureD0Ev(void *_this);
void ZN20CDirect3DCubeTextureD1Ev(void *_this);

ULONG CDirect3DCubeTexture_AddRef(const CDirect3DCubeTexture *_this)
{
    CDirect3DCubeTextureClean *tex = (CDirect3DCubeTextureClean *)_this;
    return ++tex->refCount;
}

HRESULT CDirect3DCubeTexture_QueryInterface(const CDirect3DCubeTexture *_this, const IID *iid, void **ppvObj)
{
    (void)iid;
    *ppvObj = (void *)_this;
    CDirect3DCubeTexture_AddRef(_this);
    return 0;
}

ULONG CDirect3DCubeTexture_Release(const CDirect3DCubeTexture *_this)
{
    CDirect3DCubeTextureClean *tex = (CDirect3DCubeTextureClean *)_this;
    ULONG rc = --tex->refCount;
    if (!rc)
        ZN20CDirect3DCubeTextureD0Ev((void *)_this);
    return rc;
}

HRESULT CDirect3DCubeTexture_GetLevelDesc(const CDirect3DCubeTexture *_this, UINT Level, D3DSURFACE_DESC *pDesc)
{
    CDirect3DCubeTextureClean *tex = (CDirect3DCubeTextureClean *)_this;
    HRESULT CDirect3DSurface_GetDesc(const CDirect3DSurface *_this, D3DSURFACE_DESC *pDesc);
    if (Level >= tex->levelCount)
        return -1;
    return CDirect3DSurface_GetDesc((CDirect3DSurface *)tex->surfaces[Level * 6], pDesc);
}

HRESULT CDirect3DCubeTexture_GetCubeMapSurface(const CDirect3DCubeTexture *_this, D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface9 **ppCubeMapSurface)
{
    CDirect3DCubeTextureClean *tex = (CDirect3DCubeTextureClean *)_this;
    UINT32 idx;
    if (Level >= tex->levelCount || FaceType >= 6)
        return -1;
    idx = Level * 6 + FaceType;
    CDirect3DSurface_AddRef((CDirect3DSurface *)tex->surfaces[idx]);
    *ppCubeMapSurface = (IDirect3DSurface9 *)tex->surfaces[idx];
    return 0;
}

HRESULT CDirect3DCubeTexture_LockRect(const CDirect3DCubeTexture *_this, D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags)
{
    CDirect3DCubeTextureClean *tex = (CDirect3DCubeTextureClean *)_this;
    HRESULT CDirect3DSurface_LockRect(const CDirect3DSurface *_this, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags);
    UINT32 idx;
    if (Level >= tex->levelCount || FaceType >= 6)
        return -1;
    idx = Level * 6 + FaceType;
    return CDirect3DSurface_LockRect((CDirect3DSurface *)tex->surfaces[idx], pLockedRect, pRect, Flags);
}

HRESULT CDirect3DCubeTexture_UnlockRect(const CDirect3DCubeTexture *_this, D3DCUBEMAP_FACES FaceType, UINT Level)
{
    CDirect3DCubeTextureClean *tex = (CDirect3DCubeTextureClean *)_this;
    UINT32 idx;
    CDirect3DSurface *surface;

    if (Level >= tex->levelCount || FaceType >= 6)
        return -1;
    idx = Level * 6 + FaceType;
    surface = (CDirect3DSurface *)tex->surfaces[idx];

    if (CDirect3DSurface_IsDirty(surface)) {
        int prevTex = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &prevTex);
        glBindTexture(GL_TEXTURE_CUBE_MAP, tex->texIDStorage);
        CDirect3DSurface_UpdateOpenGLSurfaceObject(surface, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prevTex);
    }
    return 0;
}

HRESULT CDirect3DCubeTexture_AddDirtyRect(const CDirect3DCubeTexture *_this, D3DCUBEMAP_FACES FaceType, const RECT *pDirtyRect)
{
    (void)_this;
    (void)FaceType;
    (void)pDirtyRect;
    return 0;
}

DWORD CDirect3DCubeTexture_SetLOD(const CDirect3DCubeTexture *_this, DWORD LODNew)
{
    (void)_this;
    (void)LODNew;
    return 0;
}
DWORD CDirect3DCubeTexture_GetLOD(const CDirect3DCubeTexture *_this)
{
    (void)_this;
    return 0;
}
DWORD CDirect3DCubeTexture_GetLevelCount(const CDirect3DCubeTexture *_this)
{
    return ((CDirect3DCubeTextureClean *)_this)->levelCount;
}
D3DRESOURCETYPE CDirect3DCubeTexture_GetType(const CDirect3DCubeTexture *_this)
{
    (void)_this;
    return D3DRTYPE_CUBETEXTURE;
}
HRESULT CDirect3DCubeTexture_SetAutoGenFilterType(const CDirect3DCubeTexture *_this, D3DTEXTUREFILTERTYPE f)
{
    (void)_this;
    (void)f;
    return 0;
}
D3DTEXTUREFILTERTYPE CDirect3DCubeTexture_GetAutoGenFilterType(const CDirect3DCubeTexture *_this)
{
    (void)_this;
    return 0;
}
void CDirect3DCubeTexture_GenerateMipSubLevels(const CDirect3DCubeTexture *_this)
{
    (void)_this;
}

void COpenGLTexture_UpdateOpenGLSurfaces(const COpenGLTexture *_this)
{

    byte *texBase = (byte *)_this - 4;
    unsigned int texID = *(unsigned int *)(texBase + 0x54);
    {
        static int uos = 0;
        if (uos++ < 10)
            fprintf(stderr, "[UOS] texID=%u texBase=%p\n", texID, texBase);
    }
    extern void CDirect3DTexture_UpdateOpenGLSurfaces(const void *);
    CDirect3DTexture_UpdateOpenGLSurfaces(texBase);
}

void ZN20CDirect3DCubeTextureD1Ev(void *_this)
{
    CDirect3DCubeTextureClean *tex = (CDirect3DCubeTextureClean *)_this;
    UINT32 i, total;

    tex->primaryVtable = vtbl_CDirect3DCubeTexture;
    tex->secondaryVtable = vtbl_CDirect3DCubeTexture_secondary;

    total = tex->levelCount * 6;
    if (tex->surfaces) {
        for (i = 0; i < total; i++) {
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

void ZN20CDirect3DCubeTextureD0Ev(void *_this)
{
    ZN20CDirect3DCubeTextureD1Ev(_this);
    free(_this);
}

void ZN20CDirect3DCubeTextureD2Ev(void *_this)
{
    ZN20CDirect3DCubeTextureD1Ev(_this);
}

void CDirect3DCubeTexture_CDirect3DCubeTexture(const CDirect3DCubeTexture *_this, UINT32 EdgeLength, UINT32 Levels, DWORD Usage, D3DFORMAT Format)
{
    CDirect3DCubeTextureClean *tex = (CDirect3DCubeTextureClean *)_this;
    UINT32 i, face, w, totalSize, offset;
    bool createGL = 0;
    int prevTex = 0;

    (void)Usage;
    memset(tex, 0, sizeof(*tex));

    tex->primaryVtable = vtbl_CDirect3DCubeTexture;
    tex->secondaryVtable = vtbl_CDirect3DCubeTexture_secondary;
    tex->mpTexID = &tex->texIDStorage;
    tex->mTexWrapS = GL_REPEAT;
    tex->mTexWrapT = GL_REPEAT;
    tex->mTexMinFilter = GL_LINEAR;
    tex->mTexMagFilter = GL_LINEAR;
    tex->mTexAniso = 1.0f;
    tex->glWidth = EdgeLength;
    tex->glHeight = EdgeLength;
    tex->glDepth = 1;
    tex->refCount = 1;
    tex->edgeLength = EdgeLength;
    tex->format = Format;

    if (Levels == 0)
        tex->levelCount = MacOpenGLUtils_GetNumTextureLevels(EdgeLength, EdgeLength, 1);
    else
        tex->levelCount = Levels;
    if (tex->levelCount > MAX_MIP_LEVELS)
        tex->levelCount = MAX_MIP_LEVELS;

    MacOpenGLUtils_GetOpenGLTextureType(&createGL, NULL, NULL, NULL, Format);

    glGenTextures(1, &tex->texIDStorage);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &prevTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex->texIDStorage);
#ifdef __EMSCRIPTEN__
    /* D3D can allocate a single cube mip while a material still requests
     * trilinear filtering. Limit the WebGL chain to the allocated levels. */
    glTexParameteri(GL_TEXTURE_CUBE_MAP, 0x813D, tex->levelCount - 1);
#endif
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);

    totalSize = 0;
    w = EdgeLength;
    for (i = 0; i < tex->levelCount; i++) {
        totalSize += MacOpenGLUtils_GetLevelSizeInBytes(w ? w : 1, w ? w : 1, 1, &Format) * 6;
        w >>= 1;
    }
    tex->pixelData = (byte *)calloc(1, totalSize ? totalSize : 1);

    tex->surfaces = (void **)calloc(tex->levelCount * 6, sizeof(void *));
    offset = 0;
    w = EdgeLength;
    for (i = 0; i < tex->levelCount; i++) {
        UINT32 levelW = w ? w : 1;
        UINT32 levelSize = MacOpenGLUtils_GetLevelSizeInBytes(levelW, levelW, 1, &Format);
        for (face = 0; face < 6; face++) {
            GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
            void *surf = calloc(1, 128);
            CDirect3DSurface_CDirect3DSurface((CDirect3DSurface *)surf, 1, target, i,
                                              levelW, levelW, Format, tex->pixelData + offset, NULL);
            tex->surfaces[i * 6 + face] = surf;
            if (createGL)
                CDirect3DSurface_CreateOpenGLSurfaceObject((CDirect3DSurface *)surf);
            offset += levelSize;
        }
        w >>= 1;
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, prevTex);
}

void CDirect3DCubeTexture_UpdateOpenGLSurfaces(const CDirect3DCubeTexture *_this)
{
    (void)_this;
}

HRESULT CDirect3DCubeTexture_GetDevice(const CDirect3DCubeTexture *_this, void (*ppDevice)())
{
    (void)_this;
    (void)ppDevice;
    return 0;
}
HRESULT CDirect3DCubeTexture_SetPrivateData(const CDirect3DCubeTexture *_this, const GUID *refguid, const void *pData, DWORD SizeOfData, DWORD Flags)
{
    (void)_this;
    (void)refguid;
    (void)pData;
    (void)SizeOfData;
    (void)Flags;
    return 0;
}
HRESULT CDirect3DCubeTexture_GetPrivateData(const CDirect3DCubeTexture *_this, const GUID *refguid, void *pData, DWORD *pSizeOfData)
{
    (void)_this;
    (void)refguid;
    (void)pData;
    (void)pSizeOfData;
    return 0;
}
HRESULT CDirect3DCubeTexture_FreePrivateData(const CDirect3DCubeTexture *_this, const GUID *refguid)
{
    (void)_this;
    (void)refguid;
    return 0;
}
DWORD CDirect3DCubeTexture_SetPriority(const CDirect3DCubeTexture *_this, DWORD PriorityNew)
{
    (void)_this;
    (void)PriorityNew;
    return 0;
}
DWORD CDirect3DCubeTexture_GetPriority(const CDirect3DCubeTexture *_this)
{
    (void)_this;
    return 0;
}
void CDirect3DCubeTexture_PreLoad(const CDirect3DCubeTexture *_this)
{
    (void)_this;
}

extern void ZN16CDirect3DTextureD0Ev(void *);
extern void ZN16CDirect3DTextureD1Ev(void *);
void ZThn4_N16CDirect3DTextureD0Ev(void *p) { ZN16CDirect3DTextureD0Ev((char *)p - 4); }
void ZThn4_N16CDirect3DTextureD1Ev(void *p) { ZN16CDirect3DTextureD1Ev((char *)p - 4); }
void ZThn4_N20CDirect3DCubeTextureD0Ev(void *p) { ZN20CDirect3DCubeTextureD0Ev((char *)p - 4); }
void ZThn4_N20CDirect3DCubeTextureD1Ev(void *p) { ZN20CDirect3DCubeTextureD1Ev((char *)p - 4); }
fnptr_t vtbl_CDirect3DCubeTexture[] = { (fnptr_t)CDirect3DCubeTexture_QueryInterface, (fnptr_t)CDirect3DCubeTexture_AddRef, (fnptr_t)CDirect3DCubeTexture_Release, (fnptr_t)CDirect3DCubeTexture_GetDevice, (fnptr_t)CDirect3DCubeTexture_SetPrivateData, (fnptr_t)CDirect3DCubeTexture_GetPrivateData, (fnptr_t)CDirect3DCubeTexture_FreePrivateData, (fnptr_t)CDirect3DCubeTexture_SetPriority, (fnptr_t)CDirect3DCubeTexture_GetPriority, (fnptr_t)CDirect3DCubeTexture_PreLoad, (fnptr_t)CDirect3DCubeTexture_GetType, (fnptr_t)CDirect3DCubeTexture_SetLOD, (fnptr_t)CDirect3DCubeTexture_GetLOD, (fnptr_t)CDirect3DCubeTexture_GetLevelCount, (fnptr_t)CDirect3DCubeTexture_SetAutoGenFilterType, (fnptr_t)CDirect3DCubeTexture_GetAutoGenFilterType, (fnptr_t)CDirect3DCubeTexture_GenerateMipSubLevels, (fnptr_t)CDirect3DCubeTexture_GetLevelDesc, (fnptr_t)CDirect3DCubeTexture_GetCubeMapSurface, (fnptr_t)CDirect3DCubeTexture_LockRect, (fnptr_t)CDirect3DCubeTexture_UnlockRect, (fnptr_t)CDirect3DCubeTexture_AddDirtyRect, (fnptr_t)ZN20CDirect3DCubeTextureD1Ev, (fnptr_t)ZN20CDirect3DCubeTextureD0Ev };
fnptr_t vtbl_CDirect3DCubeTexture_secondary[] = { (fnptr_t)ZThn4_N20CDirect3DCubeTextureD1Ev, (fnptr_t)ZThn4_N20CDirect3DCubeTextureD0Ev, (fnptr_t)COpenGLTexture_UpdateOpenGLSurfaces };
fnptr_t vtbl_CDirect3DTexture_secondary[] = { (fnptr_t)ZThn4_N16CDirect3DTextureD1Ev, (fnptr_t)ZThn4_N16CDirect3DTextureD0Ev, (fnptr_t)COpenGLTexture_UpdateOpenGLSurfaces };
