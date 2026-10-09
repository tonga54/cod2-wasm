#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>

/* This reconstructed translation unit doesn't include the GL header. Keep
 * the actual OpenGL void-returning signature instead of an implicit int. */
extern void glClear(unsigned int mask);
extern void glClearColor(float red, float green, float blue, float alpha);

float g_scale1 = 1.0f;
float g_scale2 = 16777216.0f;

typedef struct SDL_Window SDL_Window;
extern void SDL_GL_SwapWindowDirect(void);

typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CDirect3DDevice[];
extern void *vtbl_CDirect3DTexture[];
extern void *vtbl_CDirect3DCubeTexture[];
extern void R_CapturePendingScreenshotBeforePresent(void);

extern bool g_ShowShadowCookies;
extern UINT32 g_NumBlurShadowPasses;
extern bool g_RenderToShadowCookie;
extern UINT32 g_showtexid;
extern float g_scale1;
extern float g_scale2;
extern bool g_InhibitCopy;
extern bool g_NoTextureID;

#if defined(_MSC_VER)
/* These C names alias the C++ static-member symbols (defined in the data blob /
 * the reconstructed class). cl has no symbol-rename; bind via /alternatename.
 * x86: a C identifier `__ZN..` emits MSVC symbol `___ZN..` (one extra '_'). */
extern bool CDirect3DDevice_mNeedsVertexShaderValidation;
extern bool CDirect3DDevice_mNeedsTransformationValidation;
extern bool CDirect3DDevice_mNeedsRasterizationValidation;
COD2_ALT("CDirect3DDevice_mNeedsVertexShaderValidation", "__ZN15CDirect3DDevice28mNeedsVertexShaderValidationE")
COD2_ALT("CDirect3DDevice_mNeedsTransformationValidation", "__ZN15CDirect3DDevice30mNeedsTransformationValidationE")
COD2_ALT("CDirect3DDevice_mNeedsRasterizationValidation", "__ZN15CDirect3DDevice29mNeedsRasterizationValidationE")
#else
#ifdef _WIN32
#define COD2_COFF_ASM_PREFIX "_"
#else
#define COD2_COFF_ASM_PREFIX ""
#endif
extern bool __attribute__((visibility("default"))) CDirect3DDevice_mNeedsVertexShaderValidation __asm__(COD2_COFF_ASM_PREFIX "__ZN15CDirect3DDevice28mNeedsVertexShaderValidationE");
extern bool __attribute__((visibility("default"))) CDirect3DDevice_mNeedsTransformationValidation __asm__(COD2_COFF_ASM_PREFIX "__ZN15CDirect3DDevice30mNeedsTransformationValidationE");
extern bool __attribute__((visibility("default"))) CDirect3DDevice_mNeedsRasterizationValidation __asm__(COD2_COFF_ASM_PREFIX "__ZN15CDirect3DDevice29mNeedsRasterizationValidationE");
#undef COD2_COFF_ASM_PREFIX
#endif

static const float GaussianBlurWeights[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
static float sPointScale[3] = { 0.0f, 0.0f, 0.0f };

extern void CDirect3DTexture_CDirect3DTexture(const CDirect3DTexture *_this,
                                              UINT32 Width, UINT32 Height, UINT32 Levels, DWORD Usage, D3DFORMAT Format);
extern void CDirect3DVolumeTexture_CDirect3DVolumeTexture(const CDirect3DVolumeTexture *_this,
                                                          UINT32 Width, UINT32 Height, UINT32 Depth, UINT32 Levels, DWORD Usage, D3DFORMAT Format);
extern void CDirect3DCubeTexture_CDirect3DCubeTexture(const CDirect3DCubeTexture *_this,
                                                      UINT32 EdgeLength, UINT32 Levels, DWORD Usage, D3DFORMAT Format);
extern void CDirect3DVertexBuffer_CDirect3DVertexBuffer(const CDirect3DVertexBuffer *_this,
                                                        UINT32 Length, DWORD Usage, D3DPOOL Pool);
extern void CDirect3DIndexBuffer_CDirect3DIndexBuffer(const CDirect3DIndexBuffer *_this,
                                                      UINT32 Length, D3DFORMAT Format, DWORD Usage, D3DPOOL Pool);
extern int CDirect3DVertexDeclaration_CDirect3DVertexDeclaration(const CDirect3DVertexDeclaration *_this,
                                                                 const D3DVERTEXELEMENT9 *pVertexElements);
extern unsigned char CDirect3DVertexShader_CDirect3DVertexShader(const CDirect3DVertexShader *_this,
                                                                 const char *pSrcData);
extern void CDirect3DSurface_CDirect3DSurface(const CDirect3DSurface *_this,
                                              SurfaceType s, GLenum CubemapID, UINT32 Level, UINT32 Width, UINT32 Height,
                                              D3DFORMAT Format, const void *pSurfaceMemory, COpenGLTexture *pOpenGLTextureInfo);
extern void CDirect3DSurface_UpdateOpenGLSurfaceObject(const CDirect3DSurface *_this, int bRecreateSurface);
extern int CDirect3DSwapChain_CDirect3DSwapChain(const CDirect3DSwapChain *_this);
extern int CDirect3DSwapChain_SetBackBuffer(const CDirect3DSwapChain *_this, const IDirect3DSurface9 *pBackBuffer);

extern UINT32 MacOpenGLUtils_GetImageSizeInBytes(UINT32 Width, UINT32 Height, UINT32 Depth, UINT32 LevelCount, const D3DFORMAT *f);
extern UINT32 MacOpenGLUtils_GetPCPixelShaderVersion(void);
extern int MacDisplay_GetCardType(void);
extern void *SDL_GL_GetProcAddress(const char *proc);

extern unsigned int COpenGL_SetColorMaterialEnable(const COpenGL *_this, int Value);
extern unsigned int COpenGL_SetActiveTexUnit(const COpenGL *_this, UINT32 Unit);
extern unsigned int COpenGL_SetDither(const COpenGL *_this, int Value);
extern unsigned int COpenGL_SetBlendEXT(const COpenGL *_this, int ForceValidation, GLenum SrcFactorRGB, GLenum DstFactorRGB, GLenum SrcFactorAlpha, GLenum DstFactorAlpha);
extern unsigned int COpenGL_SetBlend(const COpenGL *_this, int ForceValidation, GLenum SrcFactorRGB, GLenum DstFactorRGB);
extern unsigned int COpenGL_SetLight(const COpenGL *_this, UINT32 Light, const LightInfoType *Src);
extern unsigned int COpenGL_SetTexMatrix(const COpenGL *_this, UINT32 Unit, const float *m);
extern unsigned int COpenGL_SetTexMatrixIdentity(const COpenGL *_this, UINT32 Unit);
extern unsigned int COpenGL_EnableTexCoordArray(const COpenGL *_this, UINT32 Unit, GLint Size, GLenum Type, GLsizei Stride, const unsigned int *Pointer);
extern unsigned int COpenGL_Shutdown(const COpenGL *_this);
extern unsigned int COpenGL_Init(const COpenGL *_this, UINT32 Buffers, UINT32 MaxTextureUnits, UINT32 MaxTextureImageUnits);

extern void *imp___ZN7COpenGL7sOpenGLE;
extern void *imp___ZN13COpenGLMatrix9sIdentityE;

extern void *CDirect3D_GetDirect3DInterface(void);

extern int MacDisplay_GetSupportsSeparateBlendFunc(void);
extern int MacDisplay_GetSupportsAnisotropicFiltering(void);
extern float MacDisplay_GetMaxSupportedAnisotropy(void);
extern int MacDisplay_GetMaxTextureImageUnits(void);
extern int MacDisplay_GetMaxTextureUnits(void);
extern short unsigned int MacDisplay_ReleaseContext(void *ctx);
extern short unsigned int MacDisplay_SetGammaRamp(const D3DGAMMARAMP *pRamp);
extern void MacDisplay_FadeOut(int val);
extern void MacDisplay_FadeIn(float val);
extern short unsigned int MacDisplay_GetCurrentDimensions(int *w, int *h);
extern void *MacDisplay_CreateScreenContext(int depth, int windowed, int stencil, int multiSample, int fsaa, int *hasAux);
extern void MacDisplay_SwapContext(void *ctx);
extern void MacDisplay_SetMode(int w, int h, int depth, int freq);
extern void MacDisplay_GetVideoMemoryInfo(int *video, int *texture);
extern const char *MacDisplay_GetGLVendor(void);
extern const char *MacDisplay_GetGLRenderer(void);
extern const char *MacDisplay_GetGLExtensions(void);
extern void game_dprintf(const char *fmt, ...);
extern void COpenGLMatrix_SetIdentity(float *m);
extern D3DXMATRIX *D3DXMatrixMultiply(D3DXMATRIX *out, const D3DXMATRIX *a, const D3DXMATRIX *b);
extern int MacOpenGLUtils_ConvertD3DProjectionMatrixToOpenGL(float *m, float ViewportWidth, float ViewportHeight);
extern D3DMATRIX *RB_GetActiveWorldMatrix(void);

extern void COpenGLARBFragmentProgram_COpenGLARBFragmentProgram(
    const COpenGLARBFragmentProgram *_this, const string *Name, const string *Code);

typedef struct {
    void **vtable;
    ULONG refCount;
} PixelShaderStub;

static HRESULT PS_QueryInterface(const void *t, const IID *i, void **p)
{
    (void)t;
    (void)i;
    *p = (void *)t;
    return 0;
}
static ULONG PS_AddRef(const void *t)
{
    ((PixelShaderStub *)t)->refCount++;
    return ((PixelShaderStub *)t)->refCount;
}
static ULONG PS_Release(const void *t)
{
    PixelShaderStub *ps = (PixelShaderStub *)t;
    ULONG rc = --ps->refCount;
    if (!rc)
        free(ps);
    return rc;
}
static HRESULT PS_GetDevice(const void *t, void **d)
{
    (void)t;
    (void)d;
    return 0;
}
static HRESULT PS_GetFunction(const void *t, void *d, UINT *s)
{
    (void)t;
    (void)d;
    (void)s;
    return 0;
}

static void PS_Enable(const void *t)
{
    (void)t;
}
static void PS_Disable(const void *t)
{
    (void)t;
}
static void PS_SetConstants(const void *t, int s, const float *d, int c, int u)
{
    (void)t;
    (void)s;
    (void)d;
    (void)c;
    (void)u;
}
static void PS_D1(void *t)
{
    (void)t;
}
static void PS_D0(void *t)
{
    free(t);
}

static void *ps_stub_vtbl[] = {
    (void *)PS_QueryInterface, (void *)PS_AddRef, (void *)PS_Release,
    (void *)PS_GetDevice, (void *)PS_GetFunction,
    (void *)PS_D1, (void *)PS_D0,
    (void *)PS_Enable, (void *)PS_Disable, (void *)PS_SetConstants
};

static IDirect3DVertexShader9 *g_activeVertexShader = NULL;
static IDirect3DVertexDeclaration9 *g_activeVertexDeclaration = NULL;

typedef struct {
    void **vtable;
    ULONG refCount;
} QueryStub;

static HRESULT Q_QueryInterface(const void *t, const IID *i, void **p)
{
    (void)t;
    (void)i;
    *p = (void *)t;
    return 0;
}
static ULONG Q_AddRef(const void *t)
{
    ((QueryStub *)t)->refCount++;
    return ((QueryStub *)t)->refCount;
}
static ULONG Q_Release(const void *t)
{
    QueryStub *q = (QueryStub *)t;
    ULONG rc = --q->refCount;
    if (!rc)
        free(q);
    return rc;
}
static HRESULT Q_GetDevice(const void *t, void **d)
{
    (void)t;
    (void)d;
    return 0;
}
static int Q_GetType(const void *t)
{
    (void)t;
    return 0;
}
static UINT Q_GetDataSize(const void *t)
{
    (void)t;
    return 0;
}
static HRESULT Q_Issue(const void *t, DWORD f)
{
    (void)t;
    (void)f;
    return 0;
}
static HRESULT Q_GetData(const void *t, void *d, DWORD s, DWORD f)
{
    (void)t;
    (void)d;
    (void)s;
    (void)f;
    return 0;
}
static void Q_D1(void *t)
{
    (void)t;
}
static void Q_D0(void *t)
{
    free(t);
}

static void *query_stub_vtbl[] = {
    (void *)Q_QueryInterface, (void *)Q_AddRef, (void *)Q_Release,
    (void *)Q_GetDevice, (void *)Q_GetType, (void *)Q_GetDataSize,
    (void *)Q_Issue, (void *)Q_GetData,
    (void *)Q_D1, (void *)Q_D0
};

typedef struct {
    void **vtable;
    ULONG refCount;
    void *context;
    UINT32 cardType;
    UINT32 _pad010;
    IDirect3DSurface9 *renderTarget;
    IDirect3DSurface9 *depthStencil;
    IDirect3DSurface9 *backBuffer;
    IDirect3DSurface9 *depthBuffer;
    IDirect3DVertexBuffer9 *streams[16];
    UINT streamOffsets[16];
    IDirect3DIndexBuffer9 *indexBuffer;
    UINT _pad0A8;
    UINT streamStrides[16];
    IDirect3DSwapChain9 *swapChain;
    IDirect3DSurface9 *saveSurface;
    IDirect3DSurface9 *saveSurface2;
    unsigned char _pad0F8[0x394 - 0x0F8];
    DWORD srcBlend;
    DWORD destBlend;
    DWORD srcBlendAlpha;
    DWORD destBlendAlpha;
    DWORD _pad3A4;
    float alphaRef;
    GLenum alphaFunc;
    float worldMatrix[16];
    float viewMatrix[16];
    float worldViewMatrix[16];
    unsigned char isWorldViewDirty;
    unsigned char _pad471[3];
    float projMatrix[16];
    unsigned char isProjDirty;
    unsigned char _pad4B5[3];
    float texMatrix[16];
    UINT32 videoMemory;
    UINT32 textureMemory;
    UINT32 maxTextureSize;
    UINT32 maxTextureUnits;
    UINT32 maxTextureImageUnits;
    DWORD alphaSrcBlend;
    DWORD alphaRefVal;
    DWORD alphaFuncVal;
    unsigned char alphaBlendEnable;
    unsigned char _pad519[3];
    DWORD zEnable;
    DWORD zWriteEnable;
    DWORD fillMode;
    DWORD shadeMode;
    DWORD cullMode;
    DWORD lighting;
    DWORD zFunc;
    DWORD fogEnable;
    DWORD stencilEnable;
    DWORD stencilFunc;
    DWORD colorWriteEnable;
    DWORD stencilRef;
    DWORD stencilMask;
    DWORD stencilWriteMask;
    DWORD _pad554;
    DWORD sRGBWriteEnable;
    DWORD separateAlphaBlendEnable;
    DWORD colorWriteEnable1;
    DWORD depthBias;
    DWORD stencilZFail;
    DWORD stencilFail;
    DWORD stencilPass;
    DWORD fogColor;
    DWORD fogTableMode;
    DWORD fogStart;
    DWORD fogEnd;
    DWORD fogDensity;
    DWORD tweenFactor;
    DWORD blendOp;
    DWORD colorWriteEnable2;
    DWORD _pad594;
    DWORD _pad598;
    DWORD _pad59C;
    DWORD _pad5A0;
    DWORD _pad5A4;
    DWORD _pad5A8;
    unsigned char lights[0x44];
    UINT32 viewportX;
    UINT32 viewportY;
    UINT32 viewportW;
    UINT32 viewportH;
    float viewportMinZ;
    float viewportMaxZ;

    void *texStageData;
    void *texStageEnd;
    void *texStageCap;
    union {
        unsigned char bytes[0x5A0];
        struct {
            D3DLIGHT9 lights[8];
            BOOL enabled[8];
            D3DMATERIAL9 material;
        } state;
    } lightData;
    UINT32 bVal1;
    IDirect3DPixelShader9 *pixelShader;
    UINT32 bVal3;
    unsigned char _padBC0[8];
    DWORD bc8Val;
    unsigned char bccPad[0x100];
} DeviceImpl;

static DWORD g_currentFVF = 0;
static float g_vsConst[256 * 4];
static IDirect3DBaseTexture9 *g_boundTextures[8] = { 0 };
static DWORD g_alphaTestEnable = 0;
static DWORD g_textureFactor = 0xFFFFFFFFu;

static float g_depthBias = 0.0f;
static float g_slopeScaleDepthBias = 0.0f;
static DWORD g_textureStageState[8][33];
static DWORD g_samplerState[16][14];
unsigned int g_prebind_texID = 0;
unsigned int g_prebind_texTarget = 0x0DE1;
static byte *g_colorArrayScratch = NULL;
static UINT g_colorArrayScratchCapacity = 0;
static unsigned short *g_indexArrayScratch = NULL;
static UINT g_indexArrayScratchCapacity = 0;
static float *g_texCoordScratch = NULL;
static UINT g_texCoordScratchCapacity = 0;

typedef struct {
    void **vtable;
    ULONG refCount;
    UINT32 elementCount;
    D3DVERTEXELEMENT9 *elements;
} CDirect3DVertexDeclarationImpl;

enum {
    COLOR_BYTES_RGBA,
    COLOR_BYTES_BGRA,
    COLOR_BYTES_ARGB
};

#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_REPEAT 0x2901
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_MIRRORED_REPEAT 0x8370
#define GL_TEXTURE_ENV 0x2300
#define GL_TEXTURE_ENV_MODE 0x2200
#define GL_REPLACE 0x1E01
#define GL_ADD 0x0104
#define GL_COMBINE 0x8570
#define GL_COMBINE_RGB 0x8571
#define GL_COMBINE_ALPHA 0x8572
#define GL_RGB_SCALE 0x8573
#define GL_ADD_SIGNED 0x8574
#define GL_INTERPOLATE 0x8575
#define GL_CONSTANT 0x8576
#define GL_PRIMARY_COLOR 0x8577
#define GL_PREVIOUS 0x8578
#define GL_SOURCE0_RGB 0x8580
#define GL_SOURCE1_RGB 0x8581
#define GL_SOURCE2_RGB 0x8582
#define GL_SOURCE0_ALPHA 0x8588
#define GL_SOURCE1_ALPHA 0x8589
#define GL_SOURCE2_ALPHA 0x858A
#define GL_OPERAND0_RGB 0x8590
#define GL_OPERAND1_RGB 0x8591
#define GL_OPERAND2_RGB 0x8592
#define GL_OPERAND0_ALPHA 0x8598
#define GL_OPERAND1_ALPHA 0x8599
#define GL_OPERAND2_ALPHA 0x859A
#define GL_SUBTRACT 0x84E7
#define GL_TEXTURE0_ARB 0x84C0
#define GL_TEXTURE1_ARB 0x84C1
#define GL_ALPHA_SCALE 0x0D1C
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_WRAP_R 0x8072

#define D3DTA_SELECTMASK 0x0F
#define D3DTA_COMPLEMENT 0x10
#define D3DTA_ALPHAREPLICATE 0x20

static void CDirect3DDevice_LoadIdentityMatrix(float *m)
{
    memset(m, 0, sizeof(float) * 16);
    m[0] = 1.0f;
    m[5] = 1.0f;
    m[10] = 1.0f;
    m[15] = 1.0f;
}

static const D3DVERTEXELEMENT9 *CDirect3DDevice_FindVertexElement(
    UINT stream, BYTE usage, BYTE usageIndex)
{
    CDirect3DVertexDeclarationImpl *declaration;
    UINT32 i;

    if (!g_activeVertexDeclaration)
        return NULL;

    declaration = (CDirect3DVertexDeclarationImpl *)g_activeVertexDeclaration;
    for (i = 0; i + 1 < declaration->elementCount; ++i) {
        const D3DVERTEXELEMENT9 *element = &declaration->elements[i];
        if (element->Stream == stream &&
            element->Usage == usage &&
            element->UsageIndex == usageIndex) {
            return element;
        }
    }

    return NULL;
}

static const byte *CDirect3DDevice_ConvertColorArray(const byte *vertBase, UINT stride,
                                                     int colorOffset, UINT vertexCount, int byteOrder)
{
    UINT i;
    UINT bytesNeeded;

    if (colorOffset < 0 || vertexCount == 0)
        return NULL;

    /* Already-native bytes can share the interleaved vertex upload. Avoid a
     * color copy and the legacy GL layer's separate-array restriding pass. */
    if (byteOrder == COLOR_BYTES_RGBA)
        return NULL;

    bytesNeeded = vertexCount * 4;
    if (bytesNeeded > g_colorArrayScratchCapacity) {
        byte *newScratch = (byte *)realloc(g_colorArrayScratch, bytesNeeded);
        if (!newScratch)
            return NULL;
        g_colorArrayScratch = newScratch;
        g_colorArrayScratchCapacity = bytesNeeded;
    }

    for (i = 0; i < vertexCount; ++i) {
        const byte *src = vertBase + i * stride + colorOffset;
        byte *dst = g_colorArrayScratch + i * 4;

        switch (byteOrder) {
        case COLOR_BYTES_RGBA:
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
            dst[3] = src[3];
            break;
        case COLOR_BYTES_ARGB:
            dst[0] = src[1];
            dst[1] = src[2];
            dst[2] = src[3];
            dst[3] = src[0];
            break;
        default:
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst[3] = src[3];
            break;
        }
    }

    return g_colorArrayScratch;
}

/* Original XSurface colors and world/tess vertices are RGBA. Cached static
 * vertices pass through StdConverterARGB and therefore need a different order. */
static int CDirect3DDevice_ColorByteOrder(UINT stride, int positionComponents)
{
    if (stride == 0x44 || stride == 0x20 || stride == 0x24)
        return COLOR_BYTES_RGBA;
    if (stride == 0x18 ||
        (stride == 0x40 && positionComponents == 3))
        return COLOR_BYTES_ARGB;
    return COLOR_BYTES_BGRA;
}

/* The engine derives up to eight directional lights from its original light
 * grid. Upload them in view space; normals use the full model-view matrix.
 * Emscripten performs the lighting in the generated WebGL vertex shader. */
static void CDirect3DDevice_ApplyModelLights(DeviceImpl *dev, const float *view)
{
    static const float zero[4] = {0, 0, 0, 0};
    int i;
    glLoadMatrixf(view);
    glLightModelfv(0x0B53, zero);
    glMaterialfv(0x0408, 0x1200, &dev->lightData.state.material.Ambient.r);
    glMaterialfv(0x0408, 0x1201, &dev->lightData.state.material.Diffuse.r);
    glMaterialfv(0x0408, 0x1202, zero);
    for (i = 0; i < 8; ++i) {
        const D3DLIGHT9 *light = &dev->lightData.state.lights[i];
        if (dev->lightData.state.enabled[i] && light->Type == 3) {
            float direction[4] = {-light->Direction.x, -light->Direction.y,
                                  -light->Direction.z, 0};
            glEnable(0x4000 + i);
            glLightfv(0x4000 + i, 0x1200, &light->Ambient.r);
            glLightfv(0x4000 + i, 0x1201, &light->Diffuse.r);
            glLightfv(0x4000 + i, 0x1202, zero);
            glLightfv(0x4000 + i, 0x1203, direction);
        } else {
            glDisable(0x4000 + i);
        }
    }
    glEnable(0x0B50);
}

static const float *CDirect3DDevice_CameraTexCoords(const byte *vertices, UINT stride,
                                                  int positionOffset, UINT count,
                                                  const float *worldView)
{
    UINT i;
    if (count > g_texCoordScratchCapacity) {
        float *coords = (float *)realloc(g_texCoordScratch, (size_t)count * 3 * sizeof(float));
        if (!coords)
            return NULL;
        g_texCoordScratch = coords;
        g_texCoordScratchCapacity = count;
    }
    for (i = 0; i < count; ++i) {
        const float *position = (const float *)(vertices + i * stride + positionOffset);
        int axis;
        for (axis = 0; axis < 3; ++axis)
            g_texCoordScratch[i * 3 + axis] = position[0] * worldView[axis]
                + position[1] * worldView[4 + axis] + position[2] * worldView[8 + axis]
                + worldView[12 + axis];
    }
    return g_texCoordScratch;
}

static GLenum CDirect3DDevice_MapCompareFunc(DWORD func)
{
    switch (func) {
    case 1:
        return 0x0200;
    case 2:
        return 0x0201;
    case 3:
        return 0x0202;
    case 4:
        return 0x0203;
    case 5:
        return 0x0204;
    case 6:
        return 0x0205;
    case 7:
        return 0x0206;
    default:
        return 0x0207;
    }
}

static void CDirect3DDevice_DisableExtraTextureUnits(void)
{
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glDisable(0x0DE1);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glClientActiveTextureARB(GL_TEXTURE1_ARB);
    glDisableClientState(0x8078);
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glClientActiveTextureARB(GL_TEXTURE0_ARB);
}

static GLenum CDirect3DDevice_MapBlendFunc(DWORD blend)
{
    switch (blend) {
    case 1:
        return 0;
    case 2:
        return 1;
    case 3:
        return 0x0300;
    case 4:
        return 0x0301;
    case 5:
        return 0x0302;
    case 6:
        return 0x0303;
    case 7:
        return 0x0304;
    case 8:
        return 0x0305;
    case 9:
        return 0x0306;
    case 10:
        return 0x0307;
    case 11:
        return 0x0308;
    case 12:
        return 0x0302;
    case 13:
        return 0x0303;
    default:
        return 1;
    }
}

static unsigned int CDirect3DDevice_GetTextureGLId(IDirect3DBaseTexture9 *texture)
{
    if (!texture)
        return 0;
    if (*(void ***)texture != vtbl_CDirect3DTexture &&
        *(void ***)texture != vtbl_CDirect3DCubeTexture)
        return 0;
    /* CDirect3DTexture.texIDStorage: 3 ptrs (2 vtbl + mpTexID) + 16 int fields(64B) +
       2 ptrs (surfaces,pixelData) => 5*sizeof(void*)+64. x86=0x54, x64=0x68. */
    return *(unsigned int *)((byte *)texture + 5 * sizeof(void *) + 64);
}

static GLenum CDirect3DDevice_GetTextureTarget(IDirect3DBaseTexture9 *texture)
{
    if (!texture)
        return 0x0DE1;
    if (*(void ***)texture == vtbl_CDirect3DCubeTexture)
        return GL_TEXTURE_CUBE_MAP;
    return 0x0DE1;
}

static GLenum CDirect3DDevice_GetImageTextureTarget(const GfxImage *image)
{
    if (image && image->mapType == 5)
        return GL_TEXTURE_CUBE_MAP;
    return 0x0DE1;
}

static void CDirect3DDevice_BindTextureTarget(GLenum target, unsigned int texID)
{
    if (target == GL_TEXTURE_CUBE_MAP) {
        glDisable(0x0DE1);
        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texID);
    } else {
        glDisable(GL_TEXTURE_CUBE_MAP);
        glEnable(0x0DE1);
        glBindTexture(0x0DE1, texID);
    }
}

static GLenum CDirect3DDevice_MapTextureAddress(DWORD addressMode)
{
    switch (addressMode) {
    case D3DTADDRESS_CLAMP:
        return GL_CLAMP_TO_EDGE;
    case D3DTADDRESS_MIRROR:
        return GL_MIRRORED_REPEAT;
    default:
        return GL_REPEAT;
    }
}

static GLenum CDirect3DDevice_MapTextureMagFilter(DWORD filter)
{
    switch (filter) {
    case D3DTEXF_POINT:
        return GL_NEAREST;
    case D3DTEXF_LINEAR:
    case D3DTEXF_ANISOTROPIC:
    default:
        return GL_LINEAR;
    }
}

static GLenum CDirect3DDevice_MapTextureMinFilter(DWORD minFilter, DWORD mipFilter)
{
    if (minFilter != D3DTEXF_POINT && minFilter != D3DTEXF_LINEAR && minFilter != D3DTEXF_ANISOTROPIC)
        minFilter = D3DTEXF_LINEAR;

    if (mipFilter == D3DTEXF_NONE) {
        return (minFilter == D3DTEXF_POINT) ? GL_NEAREST : GL_LINEAR;
    }

    if (mipFilter == D3DTEXF_POINT) {
        return (minFilter == D3DTEXF_POINT) ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_NEAREST;
    }

    return (minFilter == D3DTEXF_POINT) ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_LINEAR;
}

static GLenum CDirect3DDevice_MapTextureArgSource(DWORD arg)
{
    switch (arg & D3DTA_SELECTMASK) {
    case 0:
        return GL_PRIMARY_COLOR;
    case 1:
        return GL_PREVIOUS;
    case 2:
        return 0x1702;
    case 3:
    case 6:
        return GL_CONSTANT;
    case 4:
        return GL_PRIMARY_COLOR;
    case 5:
    default:
        return GL_PREVIOUS;
    }
}

static GLenum CDirect3DDevice_MapTextureArgOperandRGB(DWORD arg)
{
    if (arg & D3DTA_ALPHAREPLICATE) {
        return (arg & D3DTA_COMPLEMENT) ? 0x0303  : 0x0302 ;
    }
    return (arg & D3DTA_COMPLEMENT) ? 0x0301  : 0x0300 ;
}

static GLenum CDirect3DDevice_MapTextureArgOperandAlpha(DWORD arg)
{
    return (arg & D3DTA_COMPLEMENT) ? 0x0303  : 0x0302 ;
}

static void __attribute__((noinline)) COD2_FORCE_ALIGN_ARG_POINTER CDirect3DDevice_SetTextureFactorColor(void)
{
    GLfloat factor[4];

    factor[0] = ((g_textureFactor >> 16) & 0xFF) / 255.0f;
    factor[1] = ((g_textureFactor >> 8) & 0xFF) / 255.0f;
    factor[2] = (g_textureFactor & 0xFF) / 255.0f;
    factor[3] = ((g_textureFactor >> 24) & 0xFF) / 255.0f;
    glTexEnvfv(GL_TEXTURE_ENV, 0x2201 , factor);
}

static void CDirect3DDevice_SetTextureCombineArgs(GLenum source0Enum, GLenum operand0Enum,
                                                  GLenum source1Enum, GLenum operand1Enum,
                                                  GLenum source2Enum, GLenum operand2Enum,
                                                  DWORD arg0, DWORD arg1, DWORD arg2,
                                                  int alpha)
{
    GLenum (*mapOperand)(DWORD) = alpha ? CDirect3DDevice_MapTextureArgOperandAlpha
                                        : CDirect3DDevice_MapTextureArgOperandRGB;
    GLenum (*sourceEnumBase)(DWORD) = CDirect3DDevice_MapTextureArgSource;

    glTexEnvi(GL_TEXTURE_ENV, source0Enum, sourceEnumBase(arg0));
    glTexEnvi(GL_TEXTURE_ENV, operand0Enum, mapOperand(arg0));
    glTexEnvi(GL_TEXTURE_ENV, source1Enum, sourceEnumBase(arg1));
    glTexEnvi(GL_TEXTURE_ENV, operand1Enum, mapOperand(arg1));
    glTexEnvi(GL_TEXTURE_ENV, source2Enum, sourceEnumBase(arg2));
    glTexEnvi(GL_TEXTURE_ENV, operand2Enum, mapOperand(arg2));
}

static void CDirect3DDevice_ApplyTextureCombineRGB(DWORD op, DWORD arg0, DWORD arg1, DWORD arg2)
{
    GLenum combine = 0x2100;
    GLint scale = 1;
    DWORD src0 = arg1;
    DWORD src1 = arg2;
    DWORD src2 = arg0;

    switch (op) {
    case D3DTOP_DISABLE:
    case D3DTOP_SELECTARG1:
        combine = GL_REPLACE;
        src0 = arg1;
        break;
    case D3DTOP_SELECTARG2:
        combine = GL_REPLACE;
        src0 = arg2;
        break;
    case D3DTOP_MODULATE:
        combine = 0x2100;
        break;
    case D3DTOP_MODULATE2X:
        combine = 0x2100;
        scale = 2;
        break;
    case D3DTOP_MODULATE4X:
        combine = 0x2100;
        scale = 4;
        break;
    case D3DTOP_ADD:
        combine = GL_ADD;
        break;
    case D3DTOP_ADDSIGNED:
    case D3DTOP_ADDSIGNED2X:
        combine = GL_ADD_SIGNED;
        scale = (op == D3DTOP_ADDSIGNED2X) ? 2 : 1;
        break;
    case D3DTOP_SUBTRACT:
        combine = GL_SUBTRACT;
        break;
    case D3DTOP_BLENDDIFFUSEALPHA:
        combine = GL_INTERPOLATE;
        src2 = 0;
        break;
    case D3DTOP_BLENDTEXTUREALPHA:
        combine = GL_INTERPOLATE;
        src2 = 2;
        break;
    case D3DTOP_BLENDFACTORALPHA:
        combine = GL_INTERPOLATE;
        src2 = 3;
        break;
    case D3DTOP_BLENDCURRENTALPHA:
        combine = GL_INTERPOLATE;
        src2 = 1;
        break;
    case D3DTOP_LERP:
        combine = GL_INTERPOLATE;
        break;
    default:
        combine = 0x2100;
        break;
    }

    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, combine);
    glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, scale);
    CDirect3DDevice_SetTextureCombineArgs(GL_SOURCE0_RGB, GL_OPERAND0_RGB,
                                          GL_SOURCE1_RGB, GL_OPERAND1_RGB,
                                          GL_SOURCE2_RGB, GL_OPERAND2_RGB,
                                          src0, src1, src2, 0);
}

static void CDirect3DDevice_ApplyTextureCombineAlpha(DWORD op, DWORD arg0, DWORD arg1, DWORD arg2)
{
    GLenum combine = GL_REPLACE;
    GLint scale = 1;
    DWORD src0 = arg1;
    DWORD src1 = arg2;
    DWORD src2 = arg0;

    switch (op) {
    case D3DTOP_DISABLE:
    case D3DTOP_SELECTARG1:
        combine = GL_REPLACE;
        src0 = arg1;
        break;
    case D3DTOP_SELECTARG2:
        combine = GL_REPLACE;
        src0 = arg2;
        break;
    case D3DTOP_MODULATE:
        combine = 0x2100;
        break;
    case D3DTOP_MODULATE2X:
        combine = 0x2100;
        scale = 2;
        break;
    case D3DTOP_MODULATE4X:
        combine = 0x2100;
        scale = 4;
        break;
    case D3DTOP_ADD:
        combine = GL_ADD;
        break;
    case D3DTOP_ADDSIGNED:
    case D3DTOP_ADDSIGNED2X:
        combine = GL_ADD_SIGNED;
        scale = (op == D3DTOP_ADDSIGNED2X) ? 2 : 1;
        break;
    case D3DTOP_SUBTRACT:
        combine = GL_SUBTRACT;
        break;
    case D3DTOP_BLENDDIFFUSEALPHA:
        combine = GL_INTERPOLATE;
        src2 = 0;
        break;
    case D3DTOP_BLENDTEXTUREALPHA:
        combine = GL_INTERPOLATE;
        src2 = 2;
        break;
    case D3DTOP_BLENDFACTORALPHA:
        combine = GL_INTERPOLATE;
        src2 = 3;
        break;
    case D3DTOP_BLENDCURRENTALPHA:
        combine = GL_INTERPOLATE;
        src2 = 1;
        break;
    case D3DTOP_LERP:
        combine = GL_INTERPOLATE;
        break;
    default:
        combine = GL_REPLACE;
        break;
    }

    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, combine);
    glTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, scale);
    CDirect3DDevice_SetTextureCombineArgs(GL_SOURCE0_ALPHA, GL_OPERAND0_ALPHA,
                                          GL_SOURCE1_ALPHA, GL_OPERAND1_ALPHA,
                                          GL_SOURCE2_ALPHA, GL_OPERAND2_ALPHA,
                                          src0, src1, src2, 1);
}

static int CDirect3DDevice_LightmapScale(void)
{
    static int scale = -1;
    if (scale < 0) {
        const char *e = getenv("LMAP_SCALE");
        scale = e ? atoi(e) : 1;
        if (scale < 1)
            scale = 1;
        if (scale > 4)
            scale = 4;
    }
    return scale;
}

static void CDirect3DDevice_ApplyTextureStageState(UINT stage)
{
    DWORD colorOp = g_textureStageState[stage][D3DTSS_COLOROP];
    DWORD alphaOp = g_textureStageState[stage][D3DTSS_ALPHAOP];
    DWORD colorArg0 = g_textureStageState[stage][D3DTSS_COLORARG0];
    DWORD colorArg1 = g_textureStageState[stage][D3DTSS_COLORARG1];
    DWORD colorArg2 = g_textureStageState[stage][D3DTSS_COLORARG2];
    DWORD alphaArg0 = g_textureStageState[stage][D3DTSS_ALPHAARG0];
    DWORD alphaArg1 = g_textureStageState[stage][D3DTSS_ALPHAARG1];
    DWORD alphaArg2 = g_textureStageState[stage][D3DTSS_ALPHAARG2];

    if (!colorOp)
        colorOp = D3DTOP_MODULATE;
    if (!alphaOp)
        alphaOp = D3DTOP_MODULATE;
    /* D3DTA_DIFFUSE is zero, a valid vertex-color input. Device creation
     * initializes the defaults; replacing zero here loses colors and alpha. */

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    CDirect3DDevice_SetTextureFactorColor();
    CDirect3DDevice_ApplyTextureCombineRGB(colorOp, colorArg0, colorArg1, colorArg2);
    CDirect3DDevice_ApplyTextureCombineAlpha(alphaOp, alphaArg0, alphaArg1, alphaArg2);
}

static void CDirect3DDevice_ApplySamplerState(UINT sampler, GLenum target)
{
    DWORD addressU = g_samplerState[sampler][D3DSAMP_ADDRESSU];
    DWORD addressV = g_samplerState[sampler][D3DSAMP_ADDRESSV];
    DWORD addressW = g_samplerState[sampler][D3DSAMP_ADDRESSW];
    DWORD minFilter = g_samplerState[sampler][D3DSAMP_MINFILTER];
    DWORD magFilter = g_samplerState[sampler][D3DSAMP_MAGFILTER];
    DWORD mipFilter = g_samplerState[sampler][D3DSAMP_MIPFILTER];

    if (!addressU)
        addressU = D3DTADDRESS_WRAP;
    if (!addressV)
        addressV = D3DTADDRESS_WRAP;
    if (!addressW)
        addressW = D3DTADDRESS_WRAP;
    if (!minFilter)
        minFilter = D3DTEXF_LINEAR;
    if (!magFilter)
        magFilter = D3DTEXF_LINEAR;

    glTexParameteri(target, 0x2802 ,
                    CDirect3DDevice_MapTextureAddress(addressU));
    glTexParameteri(target, 0x2803 ,
                    CDirect3DDevice_MapTextureAddress(addressV));
    if (target == GL_TEXTURE_CUBE_MAP) {
        glTexParameteri(target, GL_TEXTURE_WRAP_R,
                        CDirect3DDevice_MapTextureAddress(addressW));
    }
    glTexParameteri(target, 0x2801 ,
                    CDirect3DDevice_MapTextureMinFilter(minFilter, mipFilter));
    glTexParameteri(target, 0x2800 ,
                    CDirect3DDevice_MapTextureMagFilter(magFilter));
}

static void CDirect3DDevice_UpdateTextureIfNeeded(IDirect3DBaseTexture9 *texture)
{
    extern void CDirect3DTexture_UpdateOpenGLSurfaces(const CDirect3DTexture *_this);

    if (!texture)
        return;
    if (*(void ***)texture == vtbl_CDirect3DTexture)
        CDirect3DTexture_UpdateOpenGLSurfaces((const CDirect3DTexture *)texture);
}

static GfxImage *CDirect3DDevice_SelectMaterialColorImage(const Material *material)
{
    GfxImage *fallback;
    int textureIndex;

    if (!material || !material->textures || !material->textureCount)
        return NULL;

    fallback = NULL;
    for (textureIndex = 0; textureIndex < material->textureCount; ++textureIndex) {
        const MaterialTextureDef *texture = &material->textures[textureIndex];
        GfxImage *image;

        if (texture->semantic == 5)
            continue;

        image = texture->u.image;
        if (!image)
            continue;

        if (texture->semantic == 2)
            return image;

        if (!fallback)
            fallback = image;
    }

    return fallback;
}

static unsigned int CDirect3DDevice_GetImageGLId(GfxImage *image)
{
    IDirect3DBaseTexture9 *texture;

    if (!image)
        return 0;

    texture = image->texture.basemap;
    CDirect3DDevice_UpdateTextureIfNeeded(texture);
    return CDirect3DDevice_GetTextureGLId(texture);
}

ULONG CDirect3DDevice_AddRef(const CDirect3DDevice *_this)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    return ++dev->refCount;
}

HRESULT CDirect3DDevice_QueryInterface(const CDirect3DDevice *_this, const IID *iid, LPVOID *ppvObj)
{
    (void)iid;
    *ppvObj = (void *)_this;
    CDirect3DDevice_AddRef(_this);
    return 0;
}

ULONG CDirect3DDevice_Release(const CDirect3DDevice *_this)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    ULONG rc = --dev->refCount;
    if (!rc) {

        void (*dtor)(void *) = (void (*)(void *))dev->vtable[120];
        dtor((void *)_this);
    }
    return rc;
}

HRESULT CDirect3DDevice_CreateTexture(const CDirect3DDevice *_this, UINT Width, UINT Height,
                                      UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                                      IDirect3DTexture9 **ppTexture, HANDLE *pSharedHandle)
{
    void *tex;
    (void)_this;
    (void)Pool;
    (void)pSharedHandle;
    tex = malloc(((0x68) + (sizeof(void*)>4 ? 0x100 : 0)));
    CDirect3DTexture_CDirect3DTexture((const CDirect3DTexture *)tex, Width, Height, Levels, Usage, Format);
    *ppTexture = (IDirect3DTexture9 *)tex;
    return 0;
}

HRESULT CDirect3DDevice_CreateVolumeTexture(const CDirect3DDevice *_this, UINT Width, UINT Height,
                                            UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                                            IDirect3DVolumeTexture9 **ppVolumeTexture, HANDLE *pSharedHandle)
{
    void *tex;
    (void)_this;
    (void)Pool;
    (void)pSharedHandle;
    tex = malloc(((0x6c) + (sizeof(void*)>4 ? 0x100 : 0)));
    CDirect3DVolumeTexture_CDirect3DVolumeTexture((const CDirect3DVolumeTexture *)tex, Width, Height, Depth, Levels, Usage, Format);
    *ppVolumeTexture = (IDirect3DVolumeTexture9 *)tex;
    return 0;
}

HRESULT CDirect3DDevice_CreateCubeTexture(const CDirect3DDevice *_this, UINT EdgeLength,
                                          UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                                          IDirect3DCubeTexture9 **ppCubeTexture, HANDLE *pSharedHandle)
{
    void *tex;
    (void)_this;
    (void)Pool;
    (void)pSharedHandle;
    tex = malloc(((0x7c) + (sizeof(void*)>4 ? 0x100 : 0)));
    CDirect3DCubeTexture_CDirect3DCubeTexture((const CDirect3DCubeTexture *)tex, EdgeLength, Levels, Usage, Format);
    *ppCubeTexture = (IDirect3DCubeTexture9 *)tex;
    return 0;
}

HRESULT CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels,
                            DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                            IDirect3DVolumeTexture9 **ppVolumeTexture, HANDLE *pSharedHandle)
{
    void *tex;
    (void)Pool;
    (void)pSharedHandle;
    tex = malloc(((0x6c) + (sizeof(void*)>4 ? 0x100 : 0)));
    CDirect3DVolumeTexture_CDirect3DVolumeTexture((const CDirect3DVolumeTexture *)tex, Width, Height, Depth, Levels, Usage, Format);
    *ppVolumeTexture = (IDirect3DVolumeTexture9 *)tex;
    return 0;
}

HRESULT CDirect3DDevice_CreateVertexBuffer(const CDirect3DDevice *_this, UINT Length,
                                           DWORD Usage, DWORD FVF, D3DPOOL Pool,
                                           IDirect3DVertexBuffer9 **ppVertexBuffer, HANDLE *pSharedHandle)
{
    void *vb;
    (void)_this;
    (void)FVF;
    (void)pSharedHandle;
    vb = malloc(((0x3c) + (sizeof(void*)>4 ? 0x100 : 0)));
    CDirect3DVertexBuffer_CDirect3DVertexBuffer((const CDirect3DVertexBuffer *)vb, Length, Usage, Pool);
    *ppVertexBuffer = (IDirect3DVertexBuffer9 *)vb;
    return 0;
}

HRESULT CDirect3DDevice_CreateIndexBuffer(const CDirect3DDevice *_this, UINT Length,
                                          DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                                          IDirect3DIndexBuffer9 **ppIndexBuffer, HANDLE *pSharedHandle)
{
    void *ib;
    (void)_this;
    (void)pSharedHandle;
    ib = malloc(((0x34) + (sizeof(void*)>4 ? 0x100 : 0)));
    CDirect3DIndexBuffer_CDirect3DIndexBuffer((const CDirect3DIndexBuffer *)ib, Length, Format, Usage, Pool);
    *ppIndexBuffer = (IDirect3DIndexBuffer9 *)ib;
    return 0;
}

HRESULT CDirect3DDevice_CreateVertexDeclaration(const CDirect3DDevice *_this,
                                                const D3DVERTEXELEMENT9 *pVertexElements, IDirect3DVertexDeclaration9 **ppDecl)
{
    void *decl;
    (void)_this;
    decl = malloc(((0x10) + (sizeof(void*)>4 ? 0x100 : 0)));
    CDirect3DVertexDeclaration_CDirect3DVertexDeclaration((const CDirect3DVertexDeclaration *)decl, pVertexElements);
    *ppDecl = (IDirect3DVertexDeclaration9 *)decl;
    return 0;
}

HRESULT CDirect3DDevice_CreateVertexShader(const CDirect3DDevice *_this,
                                           const DWORD *pFunction, IDirect3DVertexShader9 **ppShader)
{
#ifdef __EMSCRIPTEN__
    /* The browser target selects the real DX7 fixed-function renderer.
       ARB/D3D shader creation must report unsupported, never fake success. */
    *ppShader = NULL;
    return (HRESULT)0x8876086a; /* D3DERR_NOTAVAILABLE */
#else

    void *shader;
    int errorPos;
    (void)_this;
    shader = malloc(((0x19c) + (sizeof(void*)>4 ? 0x100 : 0)));
    memset(shader, 0, 0x19c);
    CDirect3DVertexShader_CDirect3DVertexShader((const CDirect3DVertexShader *)shader, (const char *)pFunction);
    *ppShader = (IDirect3DVertexShader9 *)shader;
    glGetIntegerv(0x864b, &errorPos);
    if (errorPos != -1) {
        return 0x8876086c;
    }
    return 0;
#endif
}

HRESULT CDirect3DDevice_CreatePixelShader(const CDirect3DDevice *_this,
                                          const DWORD *pFunction, IDirect3DPixelShader9 **ppShader)
{
#ifdef __EMSCRIPTEN__
    /* The browser target selects the real DX7 fixed-function renderer.
       ARB/D3D shader creation must report unsupported, never fake success. */
    *ppShader = NULL;
    return (HRESULT)0x8876086a; /* D3DERR_NOTAVAILABLE */
#else

    (void)_this;

    if (pFunction) {
        const char *src = (const char *)pFunction;
        if (src[0] == '!' && src[1] == '!') {

            void *program = calloc(1, 0x80);   /* 0x20 was x86 size; over-allocate for x64 COpenGLARBFragmentProgram */
            const char *codePtr = src;
            COpenGLARBFragmentProgram_COpenGLARBFragmentProgram(
                (const COpenGLARBFragmentProgram *)program, NULL, (const string *)&codePtr);
            *ppShader = (IDirect3DPixelShader9 *)program;
            return 0;
        }
    }

    {
        PixelShaderStub *ps;
        ps = (PixelShaderStub *)calloc(1, sizeof(PixelShaderStub));
        ps->vtable = ps_stub_vtbl;
        ps->refCount = 1;
        *ppShader = (IDirect3DPixelShader9 *)ps;
    }
    return 0;
#endif
}

HRESULT CDirect3DDevice_CreatePixelShaderOpenGL(const CDirect3DDevice *_this,
                                                OpenGLPixelShaderType ShaderType, const long unsigned int *pSrcData,
                                                IDirect3DPixelShader9 **ppShader)
{
#ifdef __EMSCRIPTEN__
    /* The browser target selects the real DX7 fixed-function renderer.
       ARB/D3D shader creation must report unsupported, never fake success. */
    *ppShader = NULL;
    return (HRESULT)0x8876086a; /* D3DERR_NOTAVAILABLE */
#else

    (void)_this;
    (void)ShaderType;

    if (pSrcData) {
        const char *src = (const char *)pSrcData;
        if (src[0] == '!' && src[1] == '!') {
            void *program = calloc(1, 0x80);   /* 0x20 was x86 size; over-allocate for x64 COpenGLARBFragmentProgram */
            const char *codePtr = src;
            COpenGLARBFragmentProgram_COpenGLARBFragmentProgram(
                (const COpenGLARBFragmentProgram *)program, NULL, (const string *)&codePtr);
            *ppShader = (IDirect3DPixelShader9 *)program;
            return 0;
        }
    }

    {
        PixelShaderStub *ps;
        ps = (PixelShaderStub *)calloc(1, sizeof(PixelShaderStub));
        ps->vtable = ps_stub_vtbl;
        ps->refCount = 1;
        *ppShader = (IDirect3DPixelShader9 *)ps;
    }
    return 0;
#endif
}

HRESULT CDirect3DDevice_CreateDepthStencilSurface(const CDirect3DDevice *_this,
                                                  UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
                                                  DWORD MultisampleQuality, BOOL Discard,
                                                  IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle)
{
    void *surfMem = NULL;
    void *surf;
    int currentTexture = 0;
    (void)_this;
    (void)MultiSample;
    (void)MultisampleQuality;
    (void)Discard;
    (void)pSharedHandle;

    if (!g_NoTextureID) {
        UINT allocW = 1, allocH = 1;
        UINT imageSize;
        while (allocH < Height)
            allocH <<= 1;
        while (allocW < Width)
            allocW <<= 1;
        imageSize = MacOpenGLUtils_GetImageSizeInBytes(allocW, allocH, 1, 1, &Format);
        surfMem = malloc(imageSize);
    }

    glGetIntegerv(0x8069, &currentTexture);
    surf = malloc(((0x3c) + (sizeof(void*)>4 ? 0x100 : 0)));
    CDirect3DSurface_CDirect3DSurface((const CDirect3DSurface *)surf, 0, 0, 0, Width, Height, Format, surfMem, 0);
    *ppSurface = (IDirect3DSurface9 *)surf;
    glBindTexture(0xde1, currentTexture);
    return 0;
}

HRESULT CDirect3DDevice_CreateRenderTarget(const CDirect3DDevice *_this,
                                           UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
                                           DWORD MultisampleQuality, BOOL Lockable,
                                           IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle)
{
    void *surfMem = NULL;
    void *surf;
    int currentTexture = 0;
    (void)_this;
    (void)MultiSample;
    (void)MultisampleQuality;
    (void)Lockable;
    (void)pSharedHandle;

    if (!g_NoTextureID) {
        UINT allocW = 1, allocH = 1;
        UINT imageSize;
        while (allocH < Height)
            allocH <<= 1;
        while (allocW < Width)
            allocW <<= 1;
        imageSize = MacOpenGLUtils_GetImageSizeInBytes(allocW, allocH, 1, 1, &Format);
        surfMem = malloc(imageSize);
    }

    glGetIntegerv(0x8069, &currentTexture);
    surf = malloc(((0x3c) + (sizeof(void*)>4 ? 0x100 : 0)));
    CDirect3DSurface_CDirect3DSurface((const CDirect3DSurface *)surf, 0, 0, 0, Width, Height, Format, surfMem, 0);
    *ppSurface = (IDirect3DSurface9 *)surf;
    glBindTexture(0xde1, currentTexture);
    return 0;
}

HRESULT CDirect3DDevice_CreateQuery(const CDirect3DDevice *_this, D3DQUERYTYPE Type, bool (*ppQuery)())
{
    QueryStub *q;
    (void)_this;
    (void)Type;
    if (ppQuery) {
        q = (QueryStub *)calloc(1, sizeof(QueryStub));
        q->vtable = query_stub_vtbl;
        q->refCount = 1;
        *(void **)ppQuery = (void *)q;
    }
    return 0;
}

HRESULT CDirect3DDevice_CreateOffscreenPlainSurface(const CDirect3DDevice *_this,
                                                    UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool,
                                                    IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle)
{
    CDirect3DSurface *surface;
    void *surfaceMemory;
    int currentTexture;
    UINT allocWidth;
    UINT allocHeight;
    UINT imageSize;

    (void)_this;
    (void)Pool;

    surfaceMemory = NULL;
    currentTexture = 0;

    glGetIntegerv(0x8069, &currentTexture);

    if (!g_NoTextureID) {
        allocWidth = 1;
        while (allocWidth < Width) {
            allocWidth <<= 1;
        }
        allocHeight = 1;
        while (allocHeight < Height) {
            allocHeight <<= 1;
        }
        imageSize = MacOpenGLUtils_GetImageSizeInBytes(allocWidth, allocHeight, 1, 1, &Format);
        surfaceMemory = malloc(imageSize);
    }

    surface = (CDirect3DSurface *)malloc(0x80);   /* 0x3c was x86 sizeof(CDirect3DSurfaceImpl); x64 is 0x50 */
    CDirect3DSurface_CDirect3DSurface(surface, 0, 0, 0, Width, Height, Format, surfaceMemory, 0);

    if (ppSurface != NULL) {
        *ppSurface = (IDirect3DSurface9 *)surface;
    }
    if (pSharedHandle != NULL) {
        *pSharedHandle = 0;
    }

    glBindTexture(0xde1, currentTexture);
    return 0;
}

HRESULT CDirect3DDevice_TestCooperativeLevel(const CDirect3DDevice *_this)
{
    (void)_this;
    return 0;
}

UINT CDirect3DDevice_GetAvailableTextureMem(const CDirect3DDevice *_this)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    return dev->textureMemory;
}

HRESULT CDirect3DDevice_EvictManagedResources(const CDirect3DDevice *_this)
{
    (void)_this;
    return 0;
}

HRESULT CDirect3DDevice_GetDirect3D(const CDirect3DDevice *_this, IDirect3D9 **ppD3D8)
{
    (void)_this;
    *ppD3D8 = (IDirect3D9 *)CDirect3D_GetDirect3DInterface();
    return 0;
}

HRESULT CDirect3DDevice_GetDeviceCaps(const CDirect3DDevice *_this, D3DCAPS9 *pCaps)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    int maxTexUnits, maxTexImageUnits, maxStages;
    (void)dev;

    memset(pCaps, 0, sizeof(D3DCAPS9));
    pCaps->DeviceType = 1;
    pCaps->AdapterOrdinal = dev->cardType;
    pCaps->Caps = 0;
    pCaps->Caps2 = 0x30020000;
    pCaps->Caps3 = 0x120;
    pCaps->PresentationIntervals = 0x80000001;
    pCaps->CursorCaps = 1;
    pCaps->DevCaps = 0x19bfc0;
    pCaps->PrimitiveMiscCaps = 0xcaf2;

    if (MacDisplay_GetSupportsSeparateBlendFunc()) {
        pCaps->PrimitiveMiscCaps |= 0x20000;
    }

    pCaps->RasterCaps = 0;
    if (MacDisplay_GetSupportsAnisotropicFiltering()) {
        pCaps->RasterCaps |= 0x20000;
    }
    pCaps->RasterCaps |= 0x5602081;
    pCaps->RasterCaps &= 0xfffeffff;
    pCaps->RasterCaps |= 0x2000000;

    pCaps->ZCmpCaps = 0xff;
    pCaps->SrcBlendCaps = 0x7ff;
    pCaps->DestBlendCaps = 0x7ff;
    pCaps->AlphaCmpCaps = 0xff;
    pCaps->ShadeCaps = 0x84208;
    pCaps->TextureCaps = 0x234d07;
    pCaps->TextureFilterCaps = 0x3030300;
    pCaps->CubeTextureFilterCaps = 0x3030300;

    if (MacDisplay_GetSupportsAnisotropicFiltering()) {
        pCaps->TextureFilterCaps |= 0x4000400;
        pCaps->CubeTextureFilterCaps |= 0x4000400;
    }

    pCaps->VolumeTextureFilterCaps = 0;
    pCaps->TextureAddressCaps = 0xd;
    pCaps->VolumeTextureAddressCaps = 0;
    pCaps->LineCaps = 0;
    pCaps->MaxTextureWidth = dev->maxTextureSize;
    pCaps->MaxTextureHeight = dev->maxTextureSize;
    pCaps->MaxVolumeExtent = 0;
    pCaps->MaxTextureRepeat = 0;
    pCaps->MaxTextureAspectRatio = 0x800;

    if (MacDisplay_GetSupportsAnisotropicFiltering()) {
        pCaps->MaxAnisotropy = (DWORD)MacDisplay_GetMaxSupportedAnisotropy();
    }

    pCaps->MaxVertexW = 0.0f;
    pCaps->GuardBandLeft = 0.0f;
    pCaps->GuardBandTop = 0.0f;
    pCaps->GuardBandRight = 0.0f;
    pCaps->GuardBandBottom = 0.0f;
    pCaps->ExtentsAdjust = 0.0f;
    pCaps->StencilCaps = 0xff;
    pCaps->FVFCaps = 0;
    pCaps->TextureOpCaps = 0x3feffff;
    pCaps->VertexProcessingCaps = 0x3b;
    pCaps->MaxActiveLights = 8;
    pCaps->MaxUserClipPlanes = 0;
    pCaps->MaxVertexBlendMatrices = 0;
    pCaps->MaxVertexBlendMatrixIndex = 0;
    pCaps->MaxPointSize = 1.0f;
    pCaps->MaxPrimitiveCount = 0xffff;
    pCaps->MaxVertexIndex = 0xffff;
    pCaps->MaxStreams = 0x10;
    pCaps->MaxStreamStride = 0x400;
    pCaps->VertexShaderVersion = 0xfffe0200;

    pCaps->PixelShaderVersion = MacOpenGLUtils_GetPCPixelShaderVersion();
    pCaps->MaxVertexShaderConst = 0x100;
    pCaps->PixelShader1xMaxValue = 8.0f;

    maxTexImageUnits = MacDisplay_GetMaxTextureImageUnits();
    maxTexUnits = MacDisplay_GetMaxTextureUnits();
    maxStages = (maxTexUnits < maxTexImageUnits) ? maxTexImageUnits : maxTexUnits;
    pCaps->MaxSimultaneousTextures = maxStages;
    pCaps->MaxTextureBlendStages = maxStages;

    pCaps->DevCaps2 = 1;
    pCaps->MaxNpatchTessellationLevel = 0.0f;
    pCaps->DeclTypes = 0;
    pCaps->NumSimultaneousRTs = 1;
    pCaps->StretchRectFilterCaps = 0x200;
    pCaps->VertexTextureFilterCaps = 0;
    pCaps->MaxVShaderInstructionsExecuted = 0;
    pCaps->MaxPShaderInstructionsExecuted = 0;
    pCaps->MaxVertexShader30InstructionSlots = 0;
    return 0;
}

HRESULT CDirect3DDevice_GetDisplayMode(const CDirect3DDevice *_this, UINT iSwapChain, D3DDISPLAYMODE *pMode)
{
    (void)_this;
    (void)iSwapChain;
    (void)pMode;
    return 0;
}

HRESULT CDirect3DDevice_GetCreationParameters(const CDirect3DDevice *_this, D3DDEVICE_CREATION_PARAMETERS *pParameters)
{
    (void)_this;
    (void)pParameters;
    return 0;
}

HRESULT CDirect3DDevice_GetBackBuffer(const CDirect3DDevice *_this, UINT iSwapChain, UINT BackBuffer,
                                      D3DBACKBUFFER_TYPE Type, IDirect3DSurface9 **ppBackBuffer)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    (void)iSwapChain;
    (void)BackBuffer;
    (void)Type;
    *ppBackBuffer = dev->backBuffer;
    if (dev->backBuffer) {
        void **vtbl = *(void ***)dev->backBuffer;
        ((ULONG (*)(const void *))vtbl[1])((const void *)dev->backBuffer);
    }
    return 0;
}

HRESULT CDirect3DDevice_BeginScene(const CDirect3DDevice *_this)
{
    (void)_this;
    return 0;
}

HRESULT CDirect3DDevice_EndScene(const CDirect3DDevice *_this)
{
    (void)_this;
    return 0;
}

HRESULT CDirect3DDevice_Clear(const CDirect3DDevice *_this, DWORD Count, const D3DRECT *pRects,
                              DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    GLbitfield glFlags = 0;
    (void)Count;
    (void)pRects;

    if (Flags & 1) {
        float r = ((Color >> 16) & 0xFF) / 255.0f;
        float g = ((Color >> 8) & 0xFF) / 255.0f;
        float b = (Color & 0xFF) / 255.0f;
        float a = ((Color >> 24) & 0xFF) / 255.0f;
        glClearColor(r, g, b, a);
        glFlags |= 0x4000;
    }
    if (Flags & 2) {
        glClearDepth((double)Z);
        glFlags |= 0x100;
    }
    if (Flags & 4) {
        glClearStencil(Stencil);
        glFlags |= 0x400;
    }
    if (glFlags) {

        if (Flags == 1 && Color == 0x00000000) {
            return 0;
        }
        if (Flags & 1) {
            glColorMask(1, 1, 1, 1);
        }
        if (Flags & 2) {
            glDepthMask(1);
        }
        if (Flags & 4) {
            glStencilMask(0xffffffffu);
        }
        glClear(glFlags);
        if (Flags & 1) {
            glColorMask((dev->colorWriteEnable & 1) != 0,
                        (dev->colorWriteEnable & 2) != 0,
                        (dev->colorWriteEnable & 4) != 0,
                        (dev->colorWriteEnable & 8) != 0);
        }
        if (Flags & 2) {
            glDepthMask(dev->zWriteEnable ? 1 : 0);
        }
        if (Flags & 4) {
            glStencilMask(dev->stencilWriteMask);
        }
    }
    return 0;
}

HRESULT CDirect3DDevice_Present(const CDirect3DDevice *_this, const RECT *pSourceRect,
                                const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion)
{
    extern SDL_Window *sdl_gl_window;
    (void)_this;
    (void)pSourceRect;
    (void)pDestRect;
    (void)hDestWindowOverride;
    (void)pDirtyRegion;
    R_CapturePendingScreenshotBeforePresent();
    if (sdl_gl_window) {
        SDL_GL_SwapWindowDirect();
    }
    return 0;
}

HRESULT CDirect3DDevice_DrawPrimitive(const CDirect3DDevice *_this,
                                      D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    (void)_this;
    (void)PrimitiveType;
    (void)StartVertex;
    (void)PrimitiveCount;
    return 0;
}

COD2_FORCE_ALIGN_ARG_POINTER
/* Static-model cache draws advertise 65,536 vertices even for a tiny mesh.
 * Rebase the actual index range before converting or uploading attributes. */
static const unsigned short *CDirect3DDevice_IndexRange(const unsigned short *indices,
    UINT indexCount, UINT *firstVertex, UINT *vertexCount)
{
    UINT i, lo = 65535, hi = 0;
    if (!indexCount)
        return NULL;
    for (i = 0; i < indexCount; ++i) {
        if (indices[i] < lo) lo = indices[i];
        if (indices[i] > hi) hi = indices[i];
    }
    *firstVertex = lo;
    *vertexCount = hi - lo + 1;
    if (!lo)
        return indices;
    if (indexCount > g_indexArrayScratchCapacity) {
        unsigned short *scratch = realloc(g_indexArrayScratch,
            (size_t)indexCount * sizeof(*scratch));
        if (!scratch)
            return NULL;
        g_indexArrayScratch = scratch;
        g_indexArrayScratchCapacity = indexCount;
    }
    for (i = 0; i < indexCount; ++i)
        g_indexArrayScratch[i] = indices[i] - lo;
    return g_indexArrayScratch;
}

HRESULT CDirect3DDevice_DrawIndexedPrimitive(const CDirect3DDevice *_this,
                                             D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex,
                                             UINT NumVertices, UINT startIndex, UINT primCount)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    byte *vbData, *ibData, *vertBase;
    UINT stride, offset;
    int indexCount;
    const unsigned short *drawIndices;
    UINT firstVertex, drawVertexCount, vbBytes, ibBytes;
    long long firstByte;
    GLenum glMode;
    GLenum stage0Target;
    r_backEndGlobals_t *backEndState;
    int is2D;
    int usesLightmap;
    int positionOffset;
    int positionComponents;
    int colorOffset;
    int colorByteOrder;
    int texOffset;
    int lightmapOffset;
    int normalOffset;
    float textureWorldView[16];
    int textureWorldViewValid = 0;
    const D3DVERTEXELEMENT9 *element;

#ifdef __EMSCRIPTEN__
    static unsigned int webStateChecks;
    int webCheck = webStateChecks++ < 2;
#define WEB_STATE_CHECK(stage) do { if (webCheck) fprintf(stdout, "[web-state] %s error=0x%x\n", stage, glGetError()); } while (0)
#else
#define WEB_STATE_CHECK(stage) do {} while (0)
#endif
    WEB_STATE_CHECK("entry");
    (void)PrimitiveType;

    if (!dev->streams[0] || !dev->indexBuffer)
        return 0;

    vbData = *(byte **)((byte *)dev->streams[0] + sizeof(void *) + 8);   /* VB.data: vtable+refCount+lengthBytes (x86 was +12) */
    if (!vbData)
        return 0;
    offset = dev->streamOffsets[0];
    stride = dev->streamStrides[0];
    backEndState = (r_backEndGlobals_t *)imp_backEnd;
    is2D = backEndState && backEndState->projection2D;
    usesLightmap = !is2D && (stride == 0x44 || stride == 0x20);
    stage0Target = 0x0DE1;

    positionOffset = 0;
    positionComponents = 3;
    colorOffset = stride == 0x44                       ? 0x18
                  : stride == 0x40                     ? 0x1c
                  : stride == 0x24                     ? 0x18
                  : (stride == 0x18 || stride == 0x20) ? 0x0c
                                                       : -1;
    texOffset = stride == 0x44   ? 0x1c
                : stride == 0x40 ? 0x20
                : stride == 0x24 ? 0x1c
                : (stride == 0x20 || stride == 0x18) ? 0x10
                                 : -1;
    lightmapOffset = stride == 0x44 ? 0x24 : stride == 0x20 ? 0x18 : -1;
    normalOffset = stride == 0x44   ? 0x0c
                   : stride == 0x40 ? 0x10
                   : stride == 0x24 ? 0x0c
                                    : -1;

    element = CDirect3DDevice_FindVertexElement(0, 0, 0);
    if (!element)
        element = CDirect3DDevice_FindVertexElement(0, 9, 0);
    if (element) {
        positionOffset = element->Offset;
        positionComponents = element->Type == 3 ? 4 : 3;
    }
    element = CDirect3DDevice_FindVertexElement(0, 10, 0);
    if (element)
        colorOffset = element->Offset;
    element = CDirect3DDevice_FindVertexElement(0, 5, 0);
    if (element)
        texOffset = element->Offset;
    element = CDirect3DDevice_FindVertexElement(0, 5, 1);
    if (element)
        lightmapOffset = element->Offset;
    element = CDirect3DDevice_FindVertexElement(0, 3, 0);
    if (element)
        normalOffset = element->Offset;

    colorByteOrder = CDirect3DDevice_ColorByteOrder(stride, positionComponents);

    ibData = *(byte **)((byte *)dev->indexBuffer + sizeof(void *) + 8);   /* IB.data (x86 was +12) */
    if (!ibData)
        return 0;

    vbBytes = *(UINT *)((byte *)dev->streams[0] + sizeof(void *) + 4);
    ibBytes = *(UINT *)((byte *)dev->indexBuffer + sizeof(void *) + 4);
    if (!stride || !primCount || primCount > 0x7fffffffu / 3 ||
        startIndex > ibBytes / 2 || primCount * 3 > ibBytes / 2 - startIndex)
        return 0;
    indexCount = primCount * 3;
    drawIndices = CDirect3DDevice_IndexRange(
        (const unsigned short *)(ibData + startIndex * 2), indexCount,
        &firstVertex, &drawVertexCount);
    if (!drawIndices)
        return 0;
    firstByte = (long long)offset + ((long long)BaseVertexIndex + firstVertex) * stride;
    if (firstByte < 0 || firstByte > vbBytes ||
        (long long)drawVertexCount * stride > vbBytes - firstByte)
        return 0;
    vertBase = vbData + (size_t)firstByte;

#ifdef __EMSCRIPTEN__
    {
        static unsigned int traceStrides[128];
        if (!is2D && stride < 128 && traceStrides[stride] < 3 && getenv("GTRACE")) {
            const unsigned short *indices = drawIndices;
            const byte *first = vertBase + indices[0] * stride;
            const float *uv = (const float *)(first + texOffset);
            const byte *color = first + colorOffset;
            const materialCommands_t *commands = (const materialCommands_t *)imp_tess;
            ++traceStrides[stride];
            fprintf(stdout, "[world-input] stride=%u fvf=%x material=%s verts=%u base=%d min=%u indices=%u,%u,%u uv=%g,%g color=%u,%u,%u,%u lm=%d\n",
                stride, g_currentFVF, commands->material ? commands->material->info.name : "null",
                NumVertices, BaseVertexIndex, MinVertexIndex, indices[0], indices[1], indices[2],
                uv[0], uv[1], color[0], color[1], color[2], color[3], commands->lmapIndex);
            fprintf(stdout, "[world-stages] op0=%u arg1=%u arg2=%u coord=%u tex=%u op1=%u arg1=%u arg2=%u coord=%u tex=%u\n",
                g_textureStageState[0][1], g_textureStageState[0][2], g_textureStageState[0][3], g_textureStageState[0][11], CDirect3DDevice_GetTextureGLId(g_boundTextures[0]),
                g_textureStageState[1][1], g_textureStageState[1][2], g_textureStageState[1][3], g_textureStageState[1][11], CDirect3DDevice_GetTextureGLId(g_boundTextures[1]));
        }
    }
#endif

    {
        extern void glBindVertexArray(unsigned int);
        glBindVertexArray(0);
    }
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glClientActiveTextureARB(GL_TEXTURE0_ARB);
    if (!usesLightmap) {
        CDirect3DDevice_DisableExtraTextureUnits();
    }

    WEB_STATE_CHECK("texture units");
    {

#ifndef __EMSCRIPTEN__
        {
            GLuint vsId = (!is2D && g_activeVertexShader)
                              ? *(GLuint *)((char *)g_activeVertexShader + 4)
                              : 0;
            int useProg = vsId && dev->pixelShader && getenv("D3D_PROG");
            if (useProg) {
                glEnable(0x8620 );
                glBindProgramARB(0x8620, vsId);
                {
                    void **vt = *(void ***)dev->pixelShader;
                    ((void (*)(const void *))vt[7])(dev->pixelShader);
                }
            } else {

                glDisable(0x8804);
                glBindProgramARB(0x8804, 0);
                glDisable(0x8620);
                glBindProgramARB(0x8620, 0);
            }
        }
#endif
        if (!is2D && dev->zEnable) {
            glEnable(0x0B71);
            glDepthFunc(CDirect3DDevice_MapCompareFunc(dev->zFunc));
            glDepthMask(dev->zWriteEnable ? 1 : 0);
        } else {
            glDisable(0x0B71);
            glDepthMask(0);
        }
        glDisable(0x0B44);

        glDisable(0x0B60);
        glDisable(0x0B50);
        if (g_alphaTestEnable) {
            glEnable(0x0BC0);
            glAlphaFunc(CDirect3DDevice_MapCompareFunc(dev->alphaFuncVal), dev->alphaRef);
        } else {
            glDisable(0x0BC0);
        }
        glColorMask(
            (dev->colorWriteEnable & 1) != 0,
            (dev->colorWriteEnable & 2) != 0,
            (dev->colorWriteEnable & 4) != 0,
            (dev->colorWriteEnable & 8) != 0);
        if (is2D) {
            glColorMask(1, 1, 1, 1);
        }
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        WEB_STATE_CHECK("fixed state");
        if (!is2D) {

            const GfxViewParms *viewParms = backEndState ? backEndState->viewParms : NULL;
            if (viewParms) {
                float world[16];
                float view[16];
                float modelView[16];
                float proj[16];
                GfxCodeMatrices *activeMatrices = NULL;
                float aspect = (dev->viewportW && dev->viewportH)
                                   ? ((float)dev->viewportW / (float)dev->viewportH)
                                   : (640.0f / 480.0f);

                (void)aspect;
                if (backEndState && backEndState->codeMatrixStackLevel >= 0 &&
                    backEndState->codeMatrixStackLevel < 3) {
                    activeMatrices = &backEndState->codeMatrixStack[backEndState->codeMatrixStackLevel];
                }
                if (activeMatrices) {
                    memcpy(world, &activeMatrices->world.matrix[0], sizeof(world));
                    memcpy(view, &activeMatrices->view.matrix[0], sizeof(view));
                    memcpy(proj, &activeMatrices->projection.matrix[0], sizeof(proj));
                } else {
                    D3DMATRIX *activeWorld = RB_GetActiveWorldMatrix();
                    CDirect3DDevice_LoadIdentityMatrix(world);
                    if (activeWorld)
                        memcpy(world, activeWorld, sizeof(world));
                    memcpy(view, &viewParms->viewMatrix, sizeof(view));
                    memcpy(proj, &viewParms->projectionMatrix, sizeof(proj));
                }

                /* D3D TCI_CAMERASPACEPOSITION is generated before the GL
                 * handedness conversion, then transformed by the material's
                 * texture matrix. Sky cube maps have no authored UV direction. */
                D3DXMatrixMultiply(textureWorldView, world, view);
                textureWorldViewValid = 1;
                view[2] = -view[2];
                view[6] = -view[6];
                view[10] = -view[10];
                view[14] = -view[14];
                D3DXMatrixMultiply(modelView, world, view);

                MacOpenGLUtils_ConvertD3DProjectionMatrixToOpenGL(
                    proj,
                    dev->viewportW ? (float)dev->viewportW : 640.0f,
                    dev->viewportH ? (float)dev->viewportH : 480.0f);

                glMatrixMode(0x1701);
                glLoadMatrixf(proj);
                glMatrixMode(0x1700);
                if (dev->lighting && normalOffset >= 0)
                    CDirect3DDevice_ApplyModelLights(dev, view);
                glLoadMatrixf(modelView);
            } else {
                glMatrixMode(0x1701);
                glLoadIdentity();
                glScalef(1.0f / 4000.0f, 1.0f / 4000.0f, 1.0f / 4000.0f);
                glMatrixMode(0x1700);
                glLoadIdentity();
            }
        } else {

            float orthoW = dev->viewportW ? (float)dev->viewportW : 640.0f;
            float orthoH = dev->viewportH ? (float)dev->viewportH : 480.0f;
            float ortho[16] = {
                2.0f / orthoW, 0, 0, 0,
                0, -2.0f / orthoH, 0, 0,
                0, 0, -1.0f, 0,
                -1.0f, 1.0f, 0, 1.0f
            };
            glMatrixMode(0x1701);
            glLoadMatrixf(ortho);
            glMatrixMode(0x1700);
            glLoadIdentity();
        }
    }

    WEB_STATE_CHECK("matrices");
    if (1)
    {
#ifndef __EMSCRIPTEN__
        extern void glBindTexture(unsigned int, unsigned int);
#endif
        {
#ifndef __EMSCRIPTEN__
            extern void glBindTexture(unsigned int, unsigned int);
#endif
            DWORD colorOp = g_textureStageState[0][D3DTSS_COLOROP];
            unsigned int glTexID;
            GfxImage *materialImage = NULL;

            if (is2D || stride != 0x44) {
                CDirect3DDevice_UpdateTextureIfNeeded(g_boundTextures[0]);
                glTexID = CDirect3DDevice_GetTextureGLId(g_boundTextures[0]);
                stage0Target = CDirect3DDevice_GetTextureTarget(g_boundTextures[0]);
                if (!glTexID) {
                    glTexID = g_prebind_texID;
                    stage0Target = g_prebind_texTarget ? g_prebind_texTarget : 0x0DE1;
                }
            } else {
                glTexID = 0;
            }

            if (!is2D && stride != 0x44) {
                extern void *imp_tess;
                const Material *mmat = ((materialCommands_t *)imp_tess)->material;
                if (mmat) {
                    GfxImage *mimg = CDirect3DDevice_SelectMaterialColorImage(mmat);
                    unsigned int mtex = CDirect3DDevice_GetImageGLId(mimg);
                    if (mtex) {
                        glTexID = mtex;
                        stage0Target = CDirect3DDevice_GetImageTextureTarget(mimg);
                    }
                }
            }
            if (!glTexID && usesLightmap) {
                extern void *imp_tess;
                byte *tessBase = (byte *)imp_tess;
                const Material *mat = ((materialCommands_t *)tessBase)->material;
                materialImage = CDirect3DDevice_SelectMaterialColorImage(mat);
                glTexID = CDirect3DDevice_GetImageGLId(materialImage);
                stage0Target = CDirect3DDevice_GetImageTextureTarget(materialImage);
            }
            if (glTexID && colorOp != D3DTOP_DISABLE) {
                CDirect3DDevice_BindTextureTarget(stage0Target, glTexID);
                CDirect3DDevice_ApplySamplerState(0, stage0Target);
                if (usesLightmap) {

                    CDirect3DDevice_ApplyTextureStageState(0);

                    {
                        extern void *imp_tess;
                        extern GfxWorld s_world;
                        byte *tessBase = (byte *)imp_tess;
                        int lmapIndex = ((materialCommands_t *)tessBase)->lmapIndex;
                        int lmapAllowed = !dev->alphaBlendEnable || dev->destBlend == 6;
                        CDirect3DDevice_DisableExtraTextureUnits();
                        if (lmapAllowed && stage0Target != GL_TEXTURE_CUBE_MAP && lmapIndex >= 0 && lmapIndex < s_world.lightmapCount && s_world.lightmaps) {
                            GfxImage *lmapImg = s_world.lightmaps[lmapIndex][0];
                            if (lmapImg) {
                                unsigned int lmTexID = CDirect3DDevice_GetImageGLId(lmapImg);
                                if (lmTexID) {
                                    glActiveTextureARB(GL_TEXTURE1_ARB);
                                    glDisable(GL_TEXTURE_CUBE_MAP);
                                    glEnable(0x0DE1);
                                    glBindTexture(0x0DE1, lmTexID);

                                    glMatrixMode(0x1702 );
                                    glLoadIdentity();
                                    glMatrixMode(0x1700 );
                                    CDirect3DDevice_ApplySamplerState(1, 0x0DE1);
                                    if (g_textureStageState[1][D3DTSS_COLOROP] != D3DTOP_DISABLE) {
                                        CDirect3DDevice_ApplyTextureStageState(1);
                                    } else {

                                        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
                                        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, 0x2100);
                                        glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, CDirect3DDevice_LightmapScale());
                                        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, 0x1702);
                                        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, 0x0300);
                                        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
                                        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, 0x0300);
                                    }

                                    glTexParameteri(0x0DE1, 0x2801 , 0x2601 );
                                    glTexParameteri(0x0DE1, 0x2800 , 0x2601 );

                                    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, 0x1E01 );
                                    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
                                    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, 0x0302 );
                                    glTexEnvi(GL_TEXTURE_ENV, 0x0D1C , 1);

                                    glActiveTextureARB(GL_TEXTURE0_ARB);
                                }
                            }
                        }
                    }
                } else {

                    CDirect3DDevice_ApplyTextureStageState(0);
                }
            } else {
                glDisable(0x0DE1);
                glDisable(GL_TEXTURE_CUBE_MAP);
            }
        }
    }

    WEB_STATE_CHECK("texture sampling");
    if (dev->alphaBlendEnable) {
        glEnable(0x0BE2);
        glBlendFunc(CDirect3DDevice_MapBlendFunc(dev->srcBlend),
                    CDirect3DDevice_MapBlendFunc(dev->destBlend));
    } else {
        glDisable(0x0BE2);
    }

    WEB_STATE_CHECK("blend");
    glEnableClientState(0x8074);
    glVertexPointer(positionComponents, 0x1406 , stride,
                    vertBase + positionOffset);
    if (!is2D && dev->lighting && normalOffset >= 0) {
        glEnableClientState(0x8075);
        glNormalPointer(0x1406, stride, vertBase + normalOffset);
    }

    {
        if (usesLightmap) {
            const byte *colors = CDirect3DDevice_ConvertColorArray(
                vertBase, stride, colorOffset, drawVertexCount,
                colorByteOrder);

            glEnableClientState(0x8076);
            if (colors)
                glColorPointer(4, 0x1401 , 0, colors);
            else
                glColorPointer(4, 0x1401 , stride,
                               vertBase + colorOffset);

            if (lightmapOffset >= 0) {
                glClientActiveTextureARB(0x84C1);
                glEnableClientState(0x8078);
                glTexCoordPointer(2, 0x1406 , stride,
                                  vertBase + lightmapOffset);
                glClientActiveTextureARB(0x84C0);
            }
        } else if (colorOffset >= 0) {
            const byte *colors = CDirect3DDevice_ConvertColorArray(
                vertBase, stride, colorOffset, drawVertexCount,
                colorByteOrder);

            glEnableClientState(0x8076);
            if (colors)
                glColorPointer(4, 0x1401 , 0, colors);
            else
                glColorPointer(4, 0x1401 , stride, vertBase + colorOffset);
        }
        if (texOffset >= 0) {
            glEnableClientState(0x8078);
            if (stage0Target == GL_TEXTURE_CUBE_MAP) {
                const float *generated = NULL;
                if (textureWorldViewValid &&
                    (g_textureStageState[0][D3DTSS_TEXCOORDINDEX] & 0xffff0000u) == 0x20000u)
                    generated = CDirect3DDevice_CameraTexCoords(vertBase, stride,
                        positionOffset, drawVertexCount, textureWorldView);
                if (generated)
                    glTexCoordPointer(3, 0x1406, 0, generated);
                else if (normalOffset >= 0)
                    glTexCoordPointer(3, 0x1406 , stride,
                                      vertBase + normalOffset);
                else
                    glTexCoordPointer(2, 0x1406 , stride, vertBase + texOffset);
            } else {
                glTexCoordPointer(2, 0x1406 , stride, vertBase + texOffset);
            }
        }
    }

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glMatrixMode(0x1702);
    if (g_textureStageState[0][D3DTSS_TEXTURETRANSFORMFLAGS] != D3DTTFF_DISABLE)
        glLoadMatrixf(dev->texMatrix);
    else
        glLoadIdentity();
    glMatrixMode(0x1700);

    {
        extern void glDrawElements(unsigned int, int, unsigned int, const void *);
        indexCount = primCount * 3;
#ifdef __EMSCRIPTEN__
        static unsigned int webDrawChecks;
        if (webDrawChecks++ < 4) {
            GLint viewport[4];
            const float *position = (const float *)(vertBase + positionOffset);
            glGetIntegerv(0x0BA2, viewport);
            fprintf(stdout, "[web-draw] 2D=%d stride=%u verts=%u tris=%u xy=(%.1f,%.1f) viewport=(%d,%d,%d,%d) error=0x%x\n",
                    is2D, stride, NumVertices, primCount, position[0], position[1],
                    viewport[0], viewport[1], viewport[2], viewport[3], glGetError());
        }
#endif
        WEB_STATE_CHECK("attributes");
        glDrawElements(0x0004 , indexCount,
                       0x1403 ,
                       drawIndices);
    }

    WEB_STATE_CHECK("draw");
    glDisableClientState(0x8074);
    glDisableClientState(0x8075);
    glDisableClientState(0x8076);
    glDisableClientState(0x8078);
    CDirect3DDevice_DisableExtraTextureUnits();

    return 0;
}
#undef WEB_STATE_CHECK

HRESULT CDirect3DDevice_DrawPrimitiveUP(const CDirect3DDevice *_this,
                                        D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount,
                                        const void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    (void)_this;
    (void)PrimitiveType;
    (void)PrimitiveCount;
    (void)pVertexStreamZeroData;
    (void)VertexStreamZeroStride;
    return 0;
}

HRESULT CDirect3DDevice_DrawIndexedPrimitiveUP(const CDirect3DDevice *_this,
                                               D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices,
                                               UINT PrimitiveCount, const long unsigned int *pIndexData,
                                               D3DFORMAT IndexDataFormat, const long unsigned int *pVertexStreamZeroData,
                                               UINT VertexStreamZeroStride)
{
    (void)_this;
    (void)PrimitiveType;
    (void)MinVertexIndex;
    (void)NumVertexIndices;
    (void)PrimitiveCount;
    (void)pIndexData;
    (void)IndexDataFormat;
    (void)pVertexStreamZeroData;
    (void)VertexStreamZeroStride;
    return 0;
}

static void CDirect3DDevice_ApplyDepthBias(void)
{
    if (g_depthBias != 0.0f || g_slopeScaleDepthBias != 0.0f) {
        glEnable(0x8037 );

        ((void (*)(float, float))glPolygonOffset)(g_slopeScaleDepthBias, g_depthBias * 16777216.0f);
    } else {
        glDisable(0x8037 );
    }
}

HRESULT COD2_FORCE_ALIGN_ARG_POINTER CDirect3DDevice_SetRenderState(const CDirect3DDevice *_this, D3DRENDERSTATETYPE State, DWORD Value)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    switch (State) {
    case D3DRS_LIGHTING:
        dev->lighting = Value;
        break;
    case D3DRS_ZENABLE:
        dev->zEnable = Value;
        break;
    case D3DRS_ZWRITEENABLE:
        dev->zWriteEnable = Value;
        break;
    case D3DRS_ZFUNC:
        dev->zFunc = Value;
        break;
    case D3DRS_DEPTHBIAS:
        g_depthBias = *(const float *)&Value;
        CDirect3DDevice_ApplyDepthBias();
        break;
    case D3DRS_SLOPESCALEDEPTHBIAS:
        g_slopeScaleDepthBias = *(const float *)&Value;
        CDirect3DDevice_ApplyDepthBias();
        break;
    case D3DRS_ALPHATESTENABLE:
        g_alphaTestEnable = Value;
        break;
    case D3DRS_ALPHAREF:
        dev->alphaRefVal = Value;
        dev->alphaRef = (float)Value / 255.0f;
        break;
    case D3DRS_ALPHAFUNC:
        dev->alphaFuncVal = Value;
        dev->alphaFunc = CDirect3DDevice_MapCompareFunc(Value);
        break;
    case D3DRS_SRCBLEND:
        dev->srcBlend = Value;
        break;
    case D3DRS_DESTBLEND:
        dev->destBlend = Value;
        break;
    case D3DRS_CULLMODE:
        dev->cullMode = Value;
        break;
    case D3DRS_ALPHABLENDENABLE:
        dev->alphaBlendEnable = (unsigned char)Value;
        break;
    case D3DRS_FOGENABLE:
        dev->fogEnable = Value;
        break;
    case D3DRS_TEXTUREFACTOR:
        g_textureFactor = Value;
        break;
    case 0x22: {
        float fc[4];
        dev->fogColor = Value;
        fc[0] = ((Value >> 16) & 0xFF) / 255.0f;
        fc[1] = ((Value >> 8) & 0xFF) / 255.0f;
        fc[2] = (Value & 0xFF) / 255.0f;
        fc[3] = ((Value >> 24) & 0xFF) / 255.0f;
        glFogfv(0x0B66 , fc);
        break;
    }
    case D3DRS_COLORWRITEENABLE:
        dev->colorWriteEnable = Value;
        break;
    case D3DRS_BLENDOP:
        dev->blendOp = Value;
        break;
    case D3DRS_SEPARATEALPHABLENDENABLE:
        dev->separateAlphaBlendEnable = Value;
        break;
    case D3DRS_SRCBLENDALPHA:
        dev->srcBlendAlpha = Value;
        break;
    case D3DRS_DESTBLENDALPHA:
        dev->destBlendAlpha = Value;
        break;
    case D3DRS_BLENDOPALPHA:
        dev->alphaSrcBlend = Value;
        break;
    default:
        break;
    }
    return 0;
}

HRESULT CDirect3DDevice_GetRenderState(const CDirect3DDevice *_this, D3DRENDERSTATETYPE State, DWORD *pValue)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    if (!pValue)
        return 0;
    switch (State) {
    case D3DRS_LIGHTING:
        *pValue = dev->lighting;
        break;
    case D3DRS_ZENABLE:
        *pValue = dev->zEnable;
        break;
    case D3DRS_ZWRITEENABLE:
        *pValue = dev->zWriteEnable;
        break;
    case D3DRS_ZFUNC:
        *pValue = dev->zFunc;
        break;
    case D3DRS_ALPHATESTENABLE:
        *pValue = g_alphaTestEnable;
        break;
    case D3DRS_ALPHAREF:
        *pValue = dev->alphaRefVal;
        break;
    case D3DRS_ALPHAFUNC:
        *pValue = dev->alphaFuncVal;
        break;
    case D3DRS_SRCBLEND:
        *pValue = dev->srcBlend;
        break;
    case D3DRS_DESTBLEND:
        *pValue = dev->destBlend;
        break;
    case D3DRS_CULLMODE:
        *pValue = dev->cullMode;
        break;
    case D3DRS_ALPHABLENDENABLE:
        *pValue = dev->alphaBlendEnable;
        break;
    case D3DRS_FOGENABLE:
        *pValue = dev->fogEnable;
        break;
    case D3DRS_TEXTUREFACTOR:
        *pValue = g_textureFactor;
        break;
    case D3DRS_FOGCOLOR:
        *pValue = dev->fogColor;
        break;
    case D3DRS_COLORWRITEENABLE:
        *pValue = dev->colorWriteEnable;
        break;
    case D3DRS_BLENDOP:
        *pValue = dev->blendOp;
        break;
    case D3DRS_SEPARATEALPHABLENDENABLE:
        *pValue = dev->separateAlphaBlendEnable;
        break;
    case D3DRS_SRCBLENDALPHA:
        *pValue = dev->srcBlendAlpha;
        break;
    case D3DRS_DESTBLENDALPHA:
        *pValue = dev->destBlendAlpha;
        break;
    case D3DRS_BLENDOPALPHA:
        *pValue = dev->alphaSrcBlend;
        break;
    default:
        *pValue = 0;
        break;
    }
    return 0;
}

HRESULT CDirect3DDevice_SetTexture(const CDirect3DDevice *_this, DWORD Stage, IDirect3DBaseTexture9 *pTexture)
{
    (void)_this;
    if (Stage < 8) {
        g_boundTextures[Stage] = pTexture;
        if (Stage == 0 && !pTexture) {
            g_prebind_texID = 0;
            g_prebind_texTarget = 0x0DE1;
        }
    }
    return 0;
}

HRESULT CDirect3DDevice_GetTexture(const CDirect3DDevice *_this, DWORD Stage, IDirect3DBaseTexture9 **ppTexture)
{
    (void)_this;
    if (ppTexture)
        *ppTexture = (Stage < 8) ? g_boundTextures[Stage] : NULL;
    return 0;
}

HRESULT CDirect3DDevice_SetTextureStageState(const CDirect3DDevice *_this, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    (void)_this;
    if (Stage < 8 && (unsigned int)Type < 33)
        g_textureStageState[Stage][Type] = Value;
    return 0;
}

HRESULT CDirect3DDevice_GetTextureStageState(const CDirect3DDevice *_this, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD *pValue)
{
    (void)_this;
    if (pValue)
        *pValue = (Stage < 8 && (unsigned int)Type < 33) ? g_textureStageState[Stage][Type] : 0;
    return 0;
}

HRESULT CDirect3DDevice_SetSamplerState(const CDirect3DDevice *_this, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    (void)_this;
    if (Sampler < 16 && (unsigned int)Type < 14)
        g_samplerState[Sampler][Type] = Value;
    return 0;
}

HRESULT CDirect3DDevice_GetSamplerState(const CDirect3DDevice *_this, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD *pValue)
{
    (void)_this;
    if (pValue)
        *pValue = (Sampler < 16 && (unsigned int)Type < 14) ? g_samplerState[Sampler][Type] : 0;
    return 0;
}

HRESULT CDirect3DDevice_SetTransform(const CDirect3DDevice *_this, D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix)
{
    DeviceImpl *dev = (DeviceImpl *)_this;

    if (!pMatrix)
        return 0;

    if (State == 0x100) {
        memcpy(dev->worldMatrix, pMatrix, 64);
        dev->isWorldViewDirty = 1;
    } else if (State == 2) {
        memcpy(dev->viewMatrix, pMatrix, 64);
        dev->isWorldViewDirty = 1;
    } else if (State == 3) {
        memcpy(dev->projMatrix, pMatrix, 64);
        dev->isProjDirty = 1;
    } else if (State >= D3DTS_TEXTURE0 && State < D3DTS_TEXTURE0 + 8) {
        memcpy(dev->texMatrix, pMatrix, 64);
    }
    return 0;
}

HRESULT CDirect3DDevice_GetTransform(const CDirect3DDevice *_this, D3DTRANSFORMSTATETYPE State, D3DMATRIX *pMatrix)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    const float *src = NULL;
    if (State == 0x100) {
        src = dev->worldMatrix;
    } else if (State == 2) {
        src = dev->viewMatrix;
    } else if (State == 3) {
        src = dev->projMatrix;
    } else if (State >= D3DTS_TEXTURE0 && State < D3DTS_TEXTURE0 + 8) {
        src = dev->texMatrix;
    }
    if (src && pMatrix) {
        memcpy(pMatrix, src, 64);
    }
    return 0;
}

HRESULT CDirect3DDevice_SetViewport(const CDirect3DDevice *_this, const D3DVIEWPORT9 *pViewport)
{
    DeviceImpl *dev = (DeviceImpl *)_this;

    dev->viewportX = pViewport->X;
    dev->viewportY = pViewport->Y;
    dev->viewportW = pViewport->Width;
    dev->viewportH = pViewport->Height;
    dev->viewportMinZ = pViewport->MinZ;
    dev->viewportMaxZ = pViewport->MaxZ;

    glViewport(pViewport->X, pViewport->Y, pViewport->Width, pViewport->Height);
    glDepthRange((double)pViewport->MinZ, (double)pViewport->MaxZ);
    return 0;
}

HRESULT CDirect3DDevice_GetViewport(const CDirect3DDevice *_this, IDirect3DIndexBuffer9 *(*pViewport)[10])
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    UINT32 *dst = (UINT32 *)pViewport;
    dst[0] = dev->viewportX;
    dst[1] = dev->viewportY;
    dst[2] = dev->viewportW;
    dst[3] = dev->viewportH;
    *(float *)&dst[4] = dev->viewportMinZ;
    *(float *)&dst[5] = dev->viewportMaxZ;
    return 0;
}

HRESULT CDirect3DDevice_SetMaterial(const CDirect3DDevice *_this, const D3DMATERIAL9 *pMaterial)
{
    if (!pMaterial) return (HRESULT)0x8876086c;
    ((DeviceImpl *)_this)->lightData.state.material = *pMaterial;
    return 0;
}

HRESULT CDirect3DDevice_GetMaterial(const CDirect3DDevice *_this, char (*pMaterial)[4])
{
    (void)_this;
    (void)pMaterial;
    return 0;
}

HRESULT CDirect3DDevice_SetLight(const CDirect3DDevice *_this, DWORD Index, const D3DLIGHT9 *pLight)
{
    if (Index >= 8 || !pLight) return (HRESULT)0x8876086c;
    ((DeviceImpl *)_this)->lightData.state.lights[Index] = *pLight;
    return 0;
}

HRESULT CDirect3DDevice_GetLight(const CDirect3DDevice *_this, DWORD Index)
{
    (void)_this;
    (void)Index;
    return 0;
}

HRESULT CDirect3DDevice_LightEnable(const CDirect3DDevice *_this, DWORD Index, BOOL Enable)
{
    if (Index >= 8) return (HRESULT)0x8876086c;
    ((DeviceImpl *)_this)->lightData.state.enabled[Index] = Enable != 0;
    return 0;
}

HRESULT CDirect3DDevice_GetLightEnable(const CDirect3DDevice *_this, DWORD Index, BOOL *pEnable)
{
    if (Index >= 8 || !pEnable) return (HRESULT)0x8876086c;
    *pEnable = ((DeviceImpl *)_this)->lightData.state.enabled[Index];
    return 0;
}

HRESULT CDirect3DDevice_SetClipPlane(const CDirect3DDevice *_this, DWORD Index, const float *pPlane)
{
    (void)_this;
    (void)Index;
    (void)pPlane;
    return 0;
}

HRESULT CDirect3DDevice_GetClipPlane(const CDirect3DDevice *_this, DWORD Index, float *pPlane)
{
    (void)_this;
    (void)Index;
    (void)pPlane;
    return 0;
}

HRESULT CDirect3DDevice_SetStreamSource(const CDirect3DDevice *_this, UINT StreamNumber,
                                        IDirect3DVertexBuffer9 *pStreamData, UINT OffsetInBytes, UINT Stride)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    dev->streams[StreamNumber] = (IDirect3DVertexBuffer9 *)pStreamData;
    dev->streamOffsets[StreamNumber] = OffsetInBytes;
    dev->streamStrides[StreamNumber] = Stride;
    CDirect3DDevice_mNeedsTransformationValidation = 1;
    CDirect3DDevice_mNeedsRasterizationValidation = 1;
    return 0;
}

HRESULT CDirect3DDevice_GetStreamSource(const CDirect3DDevice *_this, UINT StreamNumber,
                                        IDirect3DVertexBuffer9 **ppStreamData, UINT *pOffsetInBytes, UINT *pStride)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    *ppStreamData = dev->streams[StreamNumber];
    if (dev->streams[StreamNumber]) {
        void **vtbl = *(void ***)dev->streams[StreamNumber];
        ((ULONG (*)(const void *))vtbl[1])((const void *)dev->streams[StreamNumber]);
    }
    *pStride = dev->streamStrides[StreamNumber];
    *pOffsetInBytes = dev->streamOffsets[StreamNumber];
    return 0;
}

HRESULT CDirect3DDevice_SetIndices(const CDirect3DDevice *_this, IDirect3DIndexBuffer9 *pIndexData)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    dev->indexBuffer = pIndexData;
    return 0;
}

HRESULT CDirect3DDevice_GetIndices(const CDirect3DDevice *_this, IDirect3DIndexBuffer9 **ppIndexData)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    *ppIndexData = dev->indexBuffer;
    return 0;
}

HRESULT CDirect3DDevice_SetVertexDeclaration(const CDirect3DDevice *_this, IDirect3DVertexDeclaration9 *pDecl)
{
    (void)_this;
    g_activeVertexDeclaration = pDecl;
    return 0;
}

HRESULT CDirect3DDevice_GetVertexDeclaration(const CDirect3DDevice *_this, IDirect3DVertexDeclaration9 **ppDecl)
{
    (void)_this;
    *ppDecl = g_activeVertexDeclaration;
    return 0;
}

HRESULT CDirect3DDevice_SetFVF(const CDirect3DDevice *_this, DWORD FVF)
{
    (void)_this;
    g_currentFVF = FVF;
    return 0;
}

HRESULT CDirect3DDevice_GetFVF(const CDirect3DDevice *_this, DWORD *pFVF)
{
    (void)_this;
    (void)pFVF;
    return 0;
}

HRESULT CDirect3DDevice_SetVertexShader(const CDirect3DDevice *_this, IDirect3DVertexShader9 *pShader)
{
    (void)_this;
    g_activeVertexShader = pShader;
    CDirect3DDevice_mNeedsVertexShaderValidation = 1;

    return 0;
}

HRESULT CDirect3DDevice_GetVertexShader(const CDirect3DDevice *_this, IDirect3DVertexShader9 **ppShader)
{
    (void)_this;
    (void)ppShader;
    return 0;
}

HRESULT CDirect3DDevice_SetPixelShader(const CDirect3DDevice *_this, IDirect3DPixelShader9 *pShader)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    if (dev->pixelShader != pShader) {
        dev->pixelShader = pShader;
        CDirect3DDevice_mNeedsRasterizationValidation = 1;
    }
    return 0;
}

HRESULT CDirect3DDevice_GetPixelShader(const CDirect3DDevice *_this, IDirect3DPixelShader9 **ppShader)
{
    (void)_this;
    (void)ppShader;
    return 0;
}

HRESULT CDirect3DDevice_SetVertexShaderConstantF(const CDirect3DDevice *_this, UINT StartRegister,
                                                 const float *pConstantData, UINT Vector4fCount)
{
    UINT i;
    const float *pf = pConstantData;
    (void)_this;
    for (i = StartRegister; i < StartRegister + Vector4fCount; i++) {
#ifndef __EMSCRIPTEN__
        glProgramEnvParameter4fvARB(0x8620, i, pf);
#endif
        if (i < 256)
            memcpy(g_vsConst + i * 4, pf, 16);
        pf += 4;
    }
    return 0;
}

HRESULT CDirect3DDevice_GetVertexShaderConstantF(const CDirect3DDevice *_this, UINT StartRegister,
                                                 float *pConstantData, UINT Vector4fCount)
{
    (void)_this;
    (void)StartRegister;
    (void)pConstantData;
    (void)Vector4fCount;
    return 0;
}

HRESULT CDirect3DDevice_SetPixelShaderConstantF(const CDirect3DDevice *_this, UINT StartRegister,
                                                const float *pConstantData, UINT Vector4fCount)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    if (dev->pixelShader) {
        void **vtbl = *(void ***)dev->pixelShader;

        ((void (*)(const void *, int, const float *, int, int))vtbl[9])(
            (const void *)dev->pixelShader, StartRegister, pConstantData, Vector4fCount, 0);
    }
    return 0;
}

HRESULT CDirect3DDevice_GetPixelShaderConstantF(const CDirect3DDevice *_this, UINT StartRegister,
                                                float *pConstantData, UINT Vector4fCount)
{
    (void)_this;
    (void)StartRegister;
    (void)pConstantData;
    (void)Vector4fCount;
    return 0;
}

HRESULT CDirect3DDevice_SetVertexShaderConstantI(const CDirect3DDevice *_this, UINT StartRegister, const int *pConstantData, UINT Vector4iCount)
{
    (void)_this;
    (void)StartRegister;
    (void)pConstantData;
    (void)Vector4iCount;
    return 0;
}
HRESULT CDirect3DDevice_GetVertexShaderConstantI(const CDirect3DDevice *_this, UINT StartRegister, int *pConstantData, UINT Vector4iCount)
{
    (void)_this;
    (void)StartRegister;
    (void)pConstantData;
    (void)Vector4iCount;
    return 0;
}
HRESULT CDirect3DDevice_SetVertexShaderConstantB(const CDirect3DDevice *_this, UINT StartRegister, const BOOL *pConstantData, UINT BoolCount)
{
    (void)_this;
    (void)StartRegister;
    (void)pConstantData;
    (void)BoolCount;
    return 0;
}
HRESULT CDirect3DDevice_GetVertexShaderConstantB(const CDirect3DDevice *_this, UINT StartRegister, BOOL *pConstantData, UINT BoolCount)
{
    (void)_this;
    (void)StartRegister;
    (void)pConstantData;
    (void)BoolCount;
    return 0;
}
HRESULT CDirect3DDevice_SetPixelShaderConstantI(const CDirect3DDevice *_this, UINT StartRegister, const int *pConstantData, UINT Vector4iCount)
{
    (void)_this;
    (void)StartRegister;
    (void)pConstantData;
    (void)Vector4iCount;
    return 0;
}
HRESULT CDirect3DDevice_GetPixelShaderConstantI(const CDirect3DDevice *_this, UINT StartRegister, int *pConstantData, UINT Vector4iCount)
{
    (void)_this;
    (void)StartRegister;
    (void)pConstantData;
    (void)Vector4iCount;
    return 0;
}
HRESULT CDirect3DDevice_SetPixelShaderConstantB(const CDirect3DDevice *_this, UINT StartRegister, const BOOL *pConstantData, UINT BoolCount)
{
    (void)_this;
    (void)StartRegister;
    (void)pConstantData;
    (void)BoolCount;
    return 0;
}
HRESULT CDirect3DDevice_GetPixelShaderConstantB(const CDirect3DDevice *_this, UINT StartRegister, BOOL *pConstantData, UINT BoolCount)
{
    (void)_this;
    (void)StartRegister;
    (void)pConstantData;
    (void)BoolCount;
    return 0;
}

HRESULT CDirect3DDevice_SetRenderTarget(const CDirect3DDevice *_this, DWORD RenderTargetIndex, IDirect3DSurface9 *pRenderTarget)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    (void)RenderTargetIndex;
    dev->renderTarget = pRenderTarget;
    return 0;
}

HRESULT CDirect3DDevice_GetRenderTarget(const CDirect3DDevice *_this, DWORD RenderTargetIndex, IDirect3DSurface9 **ppRenderTarget)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    (void)RenderTargetIndex;
    *ppRenderTarget = dev->renderTarget;
    if (dev->renderTarget) {
        void **vtbl = *(void ***)dev->renderTarget;
        ((ULONG (*)(const void *))vtbl[1])((const void *)dev->renderTarget);
    }
    return 0;
}

HRESULT CDirect3DDevice_SetDepthStencilSurface(const CDirect3DDevice *_this, IDirect3DSurface9 *pNewZStencil)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    dev->depthStencil = pNewZStencil;
    return 0;
}

HRESULT CDirect3DDevice_GetDepthStencilSurface(const CDirect3DDevice *_this, IDirect3DSurface9 **ppZStencilSurface)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    *ppZStencilSurface = dev->depthBuffer;
    if (dev->depthBuffer) {
        void **vtbl = *(void ***)dev->depthBuffer;
        ((ULONG (*)(const void *))vtbl[1])((const void *)dev->depthBuffer);
    }
    return 0;
}

HRESULT CDirect3DDevice_SetStreamSourceFreq(const CDirect3DDevice *_this, UINT StreamNumber, UINT Setting)
{
    (void)_this;
    (void)StreamNumber;
    (void)Setting;
    return 0;
}
HRESULT CDirect3DDevice_GetStreamSourceFreq(const CDirect3DDevice *_this, UINT StreamNumber, UINT *pSetting)
{
    (void)_this;
    (void)StreamNumber;
    (void)pSetting;
    return 0;
}

HRESULT CDirect3DDevice_ValidateDevice(const CDirect3DDevice *_this, DWORD *pNumPasses)
{
    (void)_this;
    if (pNumPasses)
        *pNumPasses = 1;
    return 0;
}

HRESULT CDirect3DDevice_Reset(const CDirect3DDevice *_this, D3DPRESENT_PARAMETERS *pPresentationParameters)
{
    (void)_this;
    (void)pPresentationParameters;
    return 0;
}

long unsigned int CDirect3DDevice_SetGammaRamp(const CDirect3DDevice *_this, UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP *pRamp)
{
    (void)_this;
    (void)iSwapChain;
    (void)Flags;
    MacDisplay_SetGammaRamp(pRamp);
    return 0;
}

long unsigned int CDirect3DDevice_GetGammaRamp(const CDirect3DDevice *_this, UINT iSwapChain, D3DGAMMARAMP *pRamp)
{
    (void)_this;
    (void)iSwapChain;
    (void)pRamp;
    return 0;
}

HRESULT CDirect3DDevice_StretchRect(const CDirect3DDevice *_this, IDirect3DSurface9 *pSourceSurface,
                                    const RECT *pSourceRect, IDirect3DSurface9 *pDestSurface, const RECT *pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
    (void)_this;
    (void)pSourceSurface;
    (void)pSourceRect;
    (void)pDestSurface;
    (void)pDestRect;
    (void)Filter;
    return 0;
}

HRESULT CDirect3DDevice_UpdateTexture(const CDirect3DDevice *_this, IDirect3DBaseTexture9 *pSourceTexture, IDirect3DBaseTexture9 *pDestinationTexture)
{
    (void)_this;
    (void)pSourceTexture;
    (void)pDestinationTexture;
    return 0;
}

HRESULT CDirect3DDevice_MultiplyTransform(const CDirect3DDevice *_this)
{
    (void)_this;
    return 0;
}

HRESULT CDirect3DDevice_SetCursorProperties(const CDirect3DDevice *_this, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9 *pCursorBitmap)
{
    (void)_this;
    (void)XHotSpot;
    (void)YHotSpot;
    (void)pCursorBitmap;
    return 0;
}
long unsigned int CDirect3DDevice_SetCursorPosition(const CDirect3DDevice *_this, int XScreenSpace, int YScreenSpace, DWORD Flags)
{
    (void)_this;
    (void)XScreenSpace;
    (void)YScreenSpace;
    (void)Flags;
    return 0;
}
BOOL CDirect3DDevice_ShowCursor(const CDirect3DDevice *_this, BOOL bShow)
{
    (void)_this;
    (void)bShow;
    return 0;
}

HRESULT CDirect3DDevice_CreateAdditionalSwapChain(const CDirect3DDevice *_this, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DSwapChain9 **pSwapChain)
{
    (void)_this;
    (void)pPresentationParameters;
    (void)pSwapChain;
    return 0;
}
HRESULT CDirect3DDevice_GetSwapChain(const CDirect3DDevice *_this, UINT iSwapChain, IDirect3DSwapChain9 **pSwapChain)
{
    (void)_this;
    (void)iSwapChain;
    (void)pSwapChain;
    return 0;
}
UINT CDirect3DDevice_GetNumberOfSwapChains(const CDirect3DDevice *_this)
{
    (void)_this;
    return 1;
}
HRESULT CDirect3DDevice_GetRasterStatus(const CDirect3DDevice *_this, UINT iSwapChain, D3DRASTER_STATUS *pRasterStatus)
{
    (void)_this;
    (void)iSwapChain;
    (void)pRasterStatus;
    return 0;
}
HRESULT CDirect3DDevice_SetDialogBoxMode(const CDirect3DDevice *_this, BOOL bEnableDialogs)
{
    (void)_this;
    (void)bEnableDialogs;
    return 0;
}

HRESULT CDirect3DDevice_UpdateSurface(const CDirect3DDevice *_this, IDirect3DSurface9 *pSourceSurface, const RECT *pSourceRect, IDirect3DSurface9 *pDestinationSurface, const POINT *pDestPoint)
{
    (void)_this;
    (void)pSourceSurface;
    (void)pSourceRect;
    (void)pDestinationSurface;
    (void)pDestPoint;
    return 0;
}
HRESULT CDirect3DDevice_GetRenderTargetData(const CDirect3DDevice *_this, IDirect3DSurface9 *pRenderTarget, IDirect3DSurface9 *pDestSurface)
{
    (void)_this;
    (void)pRenderTarget;
    (void)pDestSurface;
    return 0;
}
HRESULT CDirect3DDevice_GetFrontBufferData(const CDirect3DDevice *_this, UINT iSwapChain, IDirect3DSurface9 *pDestSurface)
{
    (void)_this;
    (void)iSwapChain;
    (void)pDestSurface;
    return 0;
}
HRESULT CDirect3DDevice_ColorFill(const CDirect3DDevice *_this, IDirect3DSurface9 *pSurface, const RECT *pRect, D3DCOLOR color)
{
    (void)_this;
    (void)pSurface;
    (void)pRect;
    (void)color;
    return 0;
}

HRESULT CDirect3DDevice_CreateStateBlock(const CDirect3DDevice *_this, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9 **ppSB)
{
    (void)_this;
    (void)Type;
    (void)ppSB;
    return 0;
}
HRESULT CDirect3DDevice_BeginStateBlock(const CDirect3DDevice *_this)
{
    (void)_this;
    return 0;
}
HRESULT CDirect3DDevice_EndStateBlock(const CDirect3DDevice *_this, IDirect3DStateBlock9 **ppSB)
{
    (void)_this;
    (void)ppSB;
    return 0;
}
HRESULT CDirect3DDevice_SetClipStatus(const CDirect3DDevice *_this, const D3DCLIPSTATUS9 *pClipStatus)
{
    (void)_this;
    (void)pClipStatus;
    return 0;
}
HRESULT CDirect3DDevice_GetClipStatus(const CDirect3DDevice *_this, D3DCLIPSTATUS9 *pClipStatus)
{
    (void)_this;
    (void)pClipStatus;
    return 0;
}
HRESULT CDirect3DDevice_SetPaletteEntries(const CDirect3DDevice *_this, UINT PaletteNumber, const PALETTEENTRY *pEntries)
{
    (void)_this;
    (void)PaletteNumber;
    (void)pEntries;
    return 0;
}
HRESULT CDirect3DDevice_GetPaletteEntries(const CDirect3DDevice *_this, UINT PaletteNumber, _D3DFORMAT (*pEntries)[4][4])
{
    (void)_this;
    (void)PaletteNumber;
    (void)pEntries;
    return 0;
}
HRESULT CDirect3DDevice_SetCurrentTexturePalette(const CDirect3DDevice *_this, UINT PaletteNumber)
{
    (void)_this;
    (void)PaletteNumber;
    return 0;
}
HRESULT CDirect3DDevice_GetCurrentTexturePalette(const CDirect3DDevice *_this, UINT *PaletteNumber)
{
    (void)_this;
    (void)PaletteNumber;
    return 0;
}
HRESULT CDirect3DDevice_SetScissorRect(const CDirect3DDevice *_this, const RECT *pRect)
{
    (void)_this;
    (void)pRect;
    return 0;
}
HRESULT CDirect3DDevice_GetScissorRect(const CDirect3DDevice *_this, RECT *pRect)
{
    (void)_this;
    (void)pRect;
    return 0;
}
HRESULT CDirect3DDevice_SetSoftwareVertexProcessing(const CDirect3DDevice *_this, BOOL bSoftware)
{
    (void)_this;
    (void)bSoftware;
    return 0;
}
BOOL CDirect3DDevice_GetSoftwareVertexProcessing(const CDirect3DDevice *_this)
{
    (void)_this;
    return 0;
}
HRESULT CDirect3DDevice_SetNPatchMode(const CDirect3DDevice *_this, float nSegments)
{
    (void)_this;
    (void)nSegments;
    return 0;
}
float CDirect3DDevice_GetNPatchMode(const CDirect3DDevice *_this)
{
    (void)_this;
    return 0.0f;
}
HRESULT CDirect3DDevice_ProcessVertices(const CDirect3DDevice *_this, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9 *pDestBuffer, IDirect3DVertexDeclaration9 *pVertexDecl, DWORD Flags)
{
    (void)_this;
    (void)SrcStartIndex;
    (void)DestIndex;
    (void)VertexCount;
    (void)pDestBuffer;
    (void)pVertexDecl;
    (void)Flags;
    return 0;
}
HRESULT CDirect3DDevice_DrawRectPatch(const CDirect3DDevice *_this, UINT Handle, const float *pNumSegs, const D3DRECTPATCH_INFO *pRectPatchInfo)
{
    (void)_this;
    (void)Handle;
    (void)pNumSegs;
    (void)pRectPatchInfo;
    return 0;
}
HRESULT CDirect3DDevice_DrawTriPatch(const CDirect3DDevice *_this, UINT Handle, const float *pNumSegs, const D3DTRIPATCH_INFO *pTriPatchInfo)
{
    (void)_this;
    (void)Handle;
    (void)pNumSegs;
    (void)pTriPatchInfo;
    return 0;
}
HRESULT CDirect3DDevice_DeletePatch(const CDirect3DDevice *_this, UINT Handle)
{
    (void)_this;
    (void)Handle;
    return 0;
}

long unsigned int CDirect3DDevice_StartPixelOneToOneState(const CDirect3DDevice *_this)
{
    (void)_this;
    return 0;
}

long unsigned int CDirect3DDevice_EndPixelOneToOneState(const CDirect3DDevice *_this)
{
    (void)_this;
    return 0;
}

long unsigned int CDirect3DDevice_SetAlphaFuncAndRef(const CDirect3DDevice *_this, DWORD AlphaFuncValue, DWORD AlphaRefValue)
{
    (void)_this;
    (void)AlphaFuncValue;
    (void)AlphaRefValue;
    return 0;
}

GLenum CDirect3DDevice_GetStencilOpFunc(const CDirect3DDevice *_this, UINT32 Value)
{
    (void)_this;
    switch (Value) {
    case 1:
        return 0x1e00;
    case 3:
        return 0x1e01;
    case 4:
        return 0x1e02;
    case 5:
        return 0x1e03;
    case 6:
        return 0x150a;
    case 7:
        return 0x8507;
    case 8:
        return 0x8508;
    default:
        return 0;
    }
}

long unsigned int CDirect3DDevice_CopyColorToRenderTargetTexture(const CDirect3DDevice *_this)
{
    (void)_this;
    return 0;
}

bool CDirect3DDevice_CTexStage_GetUsesTexture(const CTexStage *_this, bool *UsesTexFactor)
{
    (void)_this;
    (void)UsesTexFactor;
    return 0;
}

long unsigned int CDirect3DDevice_SetTextureCoordStream(const CDirect3DDevice *_this,
                                                        UINT32 TexUnit, UINT32 TransformFlags, const D3DMATRIX *OriginalTexTransform,
                                                        GLint VSize, UINT32 Tci, UINT32 StartVertex, UINT32 EndVertex,
                                                        const float *pSrcCoords, UINT32 SrcStride, const float *pSrc2Coords, UINT32 Src2Stride)
{
    (void)_this;
    (void)TexUnit;
    (void)TransformFlags;
    (void)OriginalTexTransform;
    (void)VSize;
    (void)Tci;
    (void)StartVertex;
    (void)EndVertex;
    (void)pSrcCoords;
    (void)SrcStride;
    (void)pSrc2Coords;
    (void)Src2Stride;
    return 0;
}

extern unsigned char *COpenGL_TexUnitBase(const COpenGL *_this, UINT32 Unit);

#define GL_TEXTURE_ENV_ 0x2300

#define CMB_COMBINE_RGB 0x8571
#define CMB_RGB_SCALE 0x8573
#define CMB_SRC0_RGB 0x8580
#define CMB_SRC1_RGB 0x8581
#define CMB_SRC2_RGB 0x8582
#define CMB_OP0_RGB 0x8590
#define CMB_OP1_RGB 0x8591
#define CMB_OP2_RGB 0x8592

#define CMB_COMBINE_ALPHA 0x8572
#define CMB_ALPHA_SCALE 0x0D1C
#define CMB_SRC0_ALPHA 0x8588
#define CMB_SRC1_ALPHA 0x8589
#define CMB_SRC2_ALPHA 0x858A
#define CMB_OP0_ALPHA 0x8598
#define CMB_OP1_ALPHA 0x8599
#define CMB_OP2_ALPHA 0x859A

static inline void Cmb_SetI(const COpenGL *gl, UINT32 Unit, unsigned char *u,
                            int off, unsigned int glEnum, int val)
{
    if (*(int *)(u + off) != val) {
        COpenGL_SetActiveTexUnit(gl, Unit);
        glTexEnvi(GL_TEXTURE_ENV_, glEnum, val);
        *(int *)(u + off) = val;
    }
}
static inline void Cmb_SetScale(const COpenGL *gl, UINT32 Unit, unsigned char *u,
                                int off, unsigned int glEnum, float scale)
{
    if (scale != (float)*(int *)(u + off)) {
        COpenGL_SetActiveTexUnit(gl, Unit);
        glTexEnvf(GL_TEXTURE_ENV_, glEnum, scale);
        *(int *)(u + off) = (int)scale;
    }
}

long unsigned int COpenGL_SetTexCombinerRGB1(const COpenGL *_this, UINT32 Unit,
                                             GLenum ColorOp, GLenum Source0, GLenum Operand0, GLfloat Scale)
{
    unsigned char *u = COpenGL_TexUnitBase(_this, Unit);
    Cmb_SetI(_this, Unit, u, 112, CMB_COMBINE_RGB, ColorOp);
    Cmb_SetScale(_this, Unit, u, 168, CMB_RGB_SCALE, Scale);
    Cmb_SetI(_this, Unit, u, 116, CMB_SRC0_RGB, Source0);
    Cmb_SetI(_this, Unit, u, 120, CMB_OP0_RGB, Operand0);
    return 0;
}

long unsigned int COpenGL_SetTexCombinerRGB2(const COpenGL *_this, UINT32 Unit,
                                             GLenum ColorOp, GLenum Source0, GLenum Operand0, GLenum Source1, GLenum Operand1, GLfloat Scale)
{
    unsigned char *u = COpenGL_TexUnitBase(_this, Unit);
    Cmb_SetI(_this, Unit, u, 112, CMB_COMBINE_RGB, ColorOp);
    Cmb_SetScale(_this, Unit, u, 168, CMB_RGB_SCALE, Scale);
    Cmb_SetI(_this, Unit, u, 124, CMB_SRC1_RGB, Source1);
    Cmb_SetI(_this, Unit, u, 128, CMB_OP1_RGB, Operand1);
    Cmb_SetI(_this, Unit, u, 116, CMB_SRC0_RGB, Source0);
    Cmb_SetI(_this, Unit, u, 120, CMB_OP0_RGB, Operand0);
    return 0;
}

long unsigned int COpenGL_SetTexCombinerRGB3(const COpenGL *_this, UINT32 Unit,
                                             GLenum ColorOp, GLenum Source0, GLenum Operand0, GLenum Source1, GLenum Operand1,
                                             GLenum Source2, GLenum Operand2, GLfloat Scale)
{
    unsigned char *u = COpenGL_TexUnitBase(_this, Unit);
    Cmb_SetI(_this, Unit, u, 112, CMB_COMBINE_RGB, ColorOp);
    Cmb_SetScale(_this, Unit, u, 168, CMB_RGB_SCALE, Scale);
    Cmb_SetI(_this, Unit, u, 132, CMB_SRC2_RGB, Source2);
    Cmb_SetI(_this, Unit, u, 136, CMB_OP2_RGB, Operand2);
    Cmb_SetI(_this, Unit, u, 124, CMB_SRC1_RGB, Source1);
    Cmb_SetI(_this, Unit, u, 128, CMB_OP1_RGB, Operand1);
    Cmb_SetI(_this, Unit, u, 116, CMB_SRC0_RGB, Source0);
    Cmb_SetI(_this, Unit, u, 120, CMB_OP0_RGB, Operand0);
    return 0;
}

long unsigned int COpenGL_SetTexCombinerAlpha1(const COpenGL *_this, UINT32 Unit,
                                               GLenum AlphaOp, GLenum Source0, GLenum Operand0, float Scale)
{
    unsigned char *u = COpenGL_TexUnitBase(_this, Unit);
    Cmb_SetI(_this, Unit, u, 140, CMB_COMBINE_ALPHA, AlphaOp);
    Cmb_SetScale(_this, Unit, u, 172, CMB_ALPHA_SCALE, Scale);
    Cmb_SetI(_this, Unit, u, 144, CMB_SRC0_ALPHA, Source0);
    Cmb_SetI(_this, Unit, u, 148, CMB_OP0_ALPHA, Operand0);
    return 0;
}

long unsigned int COpenGL_SetTexCombinerAlpha2(const COpenGL *_this, UINT32 Unit,
                                               GLenum AlphaOp, GLenum Source0, GLenum Operand0, GLenum Source1, GLenum Operand1, float Scale)
{
    unsigned char *u = COpenGL_TexUnitBase(_this, Unit);
    Cmb_SetI(_this, Unit, u, 140, CMB_COMBINE_ALPHA, AlphaOp);
    Cmb_SetScale(_this, Unit, u, 172, CMB_ALPHA_SCALE, Scale);
    Cmb_SetI(_this, Unit, u, 152, CMB_SRC1_ALPHA, Source1);
    Cmb_SetI(_this, Unit, u, 156, CMB_OP1_ALPHA, Operand1);
    Cmb_SetI(_this, Unit, u, 144, CMB_SRC0_ALPHA, Source0);
    Cmb_SetI(_this, Unit, u, 148, CMB_OP0_ALPHA, Operand0);
    return 0;
}

long unsigned int COpenGL_SetTexCombinerAlpha3(const COpenGL *_this, UINT32 Unit,
                                               GLenum AlphaOp, GLenum Source0, GLenum Operand0, GLenum Source1, GLenum Operand1,
                                               GLenum Source2, GLenum Operand2, float Scale)
{
    unsigned char *u = COpenGL_TexUnitBase(_this, Unit);
    Cmb_SetI(_this, Unit, u, 140, CMB_COMBINE_ALPHA, AlphaOp);
    Cmb_SetScale(_this, Unit, u, 172, CMB_ALPHA_SCALE, Scale);
    Cmb_SetI(_this, Unit, u, 160, CMB_SRC2_ALPHA, Source2);
    Cmb_SetI(_this, Unit, u, 164, CMB_OP2_ALPHA, Operand2);
    Cmb_SetI(_this, Unit, u, 152, CMB_SRC1_ALPHA, Source1);
    Cmb_SetI(_this, Unit, u, 156, CMB_OP1_ALPHA, Operand1);
    Cmb_SetI(_this, Unit, u, 144, CMB_SRC0_ALPHA, Source0);
    Cmb_SetI(_this, Unit, u, 148, CMB_OP0_ALPHA, Operand0);
    return 0;
}

long unsigned int CDirect3DDevice_CTexStage_Init(const CTexStage *_this, UINT32 Stage, int IsProgrammableStage)
{
    (void)_this;
    (void)Stage;
    (void)IsProgrammableStage;
    return 0;
}

void ZNSt11char_traitsIcE2eqERKcS2_(void) {}

void ZNSt6vectorIN15CDirect3DDevice9CTexStageESaIS1_EE5eraseEN9__gnu_cxx17__normal_iteratorIPS1_S3_EES7_(void) {}

void ZNSt6vectorIN15CDirect3DDevice9CTexStageESaIS1_EE13_M_insert_auxEN9__gnu_cxx17__normal_iteratorIPS1_S3_EERKS1_(void) {}

long unsigned int CDirect3DDevice_CDirect3DDevice(const CDirect3DDevice *_this,
                                                  UINT Adapter, D3DDEVTYPE DeviceType, ContextRef Context,
                                                  D3DPRESENT_PARAMETERS *pPresentationParameters)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    (void)Adapter;
    (void)DeviceType;
    (void)Context;
    (void)pPresentationParameters;

    dev->vtable = vtbl_CDirect3DDevice;
    dev->refCount = 1;

    return 0;
}

void CDirect3DDevice_Init(void *device)
{
    unsigned int stage;
    int videoMemory = 0;
    int textureMemory = 0;
    DeviceImpl *dev = (DeviceImpl *)device;
    dev->vtable = vtbl_CDirect3DDevice;
    dev->refCount = 1;
    CDirect3DDevice_LoadIdentityMatrix(dev->worldMatrix);
    CDirect3DDevice_LoadIdentityMatrix(dev->viewMatrix);
    CDirect3DDevice_LoadIdentityMatrix(dev->worldViewMatrix);
    CDirect3DDevice_LoadIdentityMatrix(dev->projMatrix);
    CDirect3DDevice_LoadIdentityMatrix(dev->texMatrix);
    MacDisplay_GetVideoMemoryInfo(&videoMemory, &textureMemory);
    dev->videoMemory = (UINT32)videoMemory;
    dev->textureMemory = (UINT32)textureMemory;
    dev->srcBlend = 5;
    dev->destBlend = 6;
    dev->srcBlendAlpha = 5;
    dev->destBlendAlpha = 6;
    dev->alphaRef = 0.0f;
    dev->alphaFunc = 0x0207;
    dev->alphaRefVal = 0;
    dev->alphaFuncVal = 8;
    dev->viewportX = 0;
    dev->viewportY = 0;
    dev->viewportW = 640;
    dev->viewportH = 480;
    dev->viewportMinZ = 0.0f;
    dev->viewportMaxZ = 1.0f;
    glViewport(0, 0, 640, 480);
    glDepthRange(0.0, 1.0);
    dev->zEnable = 1;
    dev->zWriteEnable = 1;
    dev->cullMode = 1;
    dev->zFunc = 4;
    dev->colorWriteEnable = 0xF;
    dev->fogColor = 0xFF000000u;
    dev->blendOp = 1;
    g_alphaTestEnable = 0;
    g_textureFactor = 0xFFFFFFFFu;
    g_prebind_texTarget = 0x0DE1;
    memset(g_textureStageState, 0, sizeof(g_textureStageState));
    memset(g_samplerState, 0, sizeof(g_samplerState));
    for (stage = 0; stage < 8; ++stage) {
        g_textureStageState[stage][D3DTSS_COLOROP] = (stage == 0) ? D3DTOP_MODULATE : D3DTOP_DISABLE;
        g_textureStageState[stage][D3DTSS_COLORARG1] = 2;
        g_textureStageState[stage][D3DTSS_COLORARG2] = 1;
        g_textureStageState[stage][D3DTSS_ALPHAOP] = (stage == 0) ? D3DTOP_MODULATE : D3DTOP_DISABLE;
        g_textureStageState[stage][D3DTSS_ALPHAARG1] = 2;
        g_textureStageState[stage][D3DTSS_ALPHAARG2] = 1;
        g_textureStageState[stage][D3DTSS_TEXCOORDINDEX] = stage;
        g_textureStageState[stage][D3DTSS_TEXTURETRANSFORMFLAGS] = D3DTTFF_DISABLE;
        g_samplerState[stage][D3DSAMP_ADDRESSU] = D3DTADDRESS_WRAP;
        g_samplerState[stage][D3DSAMP_ADDRESSV] = D3DTADDRESS_WRAP;
        g_samplerState[stage][D3DSAMP_ADDRESSW] = D3DTADDRESS_WRAP;
        g_samplerState[stage][D3DSAMP_MINFILTER] = D3DTEXF_LINEAR;
        g_samplerState[stage][D3DSAMP_MAGFILTER] = D3DTEXF_LINEAR;
        g_samplerState[stage][D3DSAMP_MIPFILTER] = D3DTEXF_LINEAR;
        g_samplerState[stage][D3DSAMP_MAXANISOTROPY] = 1;
    }
}

void ZN15CDirect3DDeviceD1Ev(void *_this)
{
    DeviceImpl *dev = (DeviceImpl *)_this;
    dev->vtable = vtbl_CDirect3DDevice;

    if (dev->context) {
        MacDisplay_ReleaseContext(&dev->context);
    }
}

void ZN15CDirect3DDeviceD0Ev(void *_this)
{
    ZN15CDirect3DDeviceD1Ev(_this);

}

void ZN15CDirect3DDeviceD2Ev(void *_this)
{
    ZN15CDirect3DDeviceD1Ev(_this);
}

fnptr_t vtbl_CDirect3DDevice[] = { (fnptr_t)CDirect3DDevice_QueryInterface, (fnptr_t)CDirect3DDevice_AddRef, (fnptr_t)CDirect3DDevice_Release, (fnptr_t)CDirect3DDevice_TestCooperativeLevel, (fnptr_t)CDirect3DDevice_GetAvailableTextureMem, (fnptr_t)CDirect3DDevice_EvictManagedResources, (fnptr_t)CDirect3DDevice_GetDirect3D, (fnptr_t)CDirect3DDevice_GetDeviceCaps, (fnptr_t)CDirect3DDevice_GetDisplayMode, (fnptr_t)CDirect3DDevice_GetCreationParameters, (fnptr_t)CDirect3DDevice_SetCursorProperties, (fnptr_t)CDirect3DDevice_SetCursorPosition, (fnptr_t)CDirect3DDevice_ShowCursor, (fnptr_t)CDirect3DDevice_CreateAdditionalSwapChain, (fnptr_t)CDirect3DDevice_GetSwapChain, (fnptr_t)CDirect3DDevice_GetNumberOfSwapChains, (fnptr_t)CDirect3DDevice_Reset, (fnptr_t)CDirect3DDevice_Present, (fnptr_t)CDirect3DDevice_GetBackBuffer, (fnptr_t)CDirect3DDevice_GetRasterStatus, (fnptr_t)CDirect3DDevice_SetDialogBoxMode, (fnptr_t)CDirect3DDevice_SetGammaRamp, (fnptr_t)CDirect3DDevice_GetGammaRamp, (fnptr_t)CDirect3DDevice_CreateTexture, (fnptr_t)CDirect3DDevice_CreateVolumeTexture, (fnptr_t)CDirect3DDevice_CreateCubeTexture, (fnptr_t)CDirect3DDevice_CreateVertexBuffer, (fnptr_t)CDirect3DDevice_CreateIndexBuffer, (fnptr_t)CDirect3DDevice_CreateRenderTarget, (fnptr_t)CDirect3DDevice_CreateDepthStencilSurface, (fnptr_t)CDirect3DDevice_UpdateSurface, (fnptr_t)CDirect3DDevice_UpdateTexture, (fnptr_t)CDirect3DDevice_GetRenderTargetData, (fnptr_t)CDirect3DDevice_GetFrontBufferData, (fnptr_t)CDirect3DDevice_StretchRect, (fnptr_t)CDirect3DDevice_ColorFill, (fnptr_t)CDirect3DDevice_CreateOffscreenPlainSurface, (fnptr_t)CDirect3DDevice_SetRenderTarget, (fnptr_t)CDirect3DDevice_GetRenderTarget, (fnptr_t)CDirect3DDevice_SetDepthStencilSurface, (fnptr_t)CDirect3DDevice_GetDepthStencilSurface, (fnptr_t)CDirect3DDevice_BeginScene, (fnptr_t)CDirect3DDevice_EndScene, (fnptr_t)CDirect3DDevice_Clear, (fnptr_t)CDirect3DDevice_SetTransform, (fnptr_t)CDirect3DDevice_GetTransform, (fnptr_t)CDirect3DDevice_MultiplyTransform, (fnptr_t)CDirect3DDevice_SetViewport, (fnptr_t)CDirect3DDevice_GetViewport, (fnptr_t)CDirect3DDevice_SetMaterial, (fnptr_t)CDirect3DDevice_GetMaterial, (fnptr_t)CDirect3DDevice_SetLight, (fnptr_t)CDirect3DDevice_GetLight, (fnptr_t)CDirect3DDevice_LightEnable, (fnptr_t)CDirect3DDevice_GetLightEnable, (fnptr_t)CDirect3DDevice_SetClipPlane, (fnptr_t)CDirect3DDevice_GetClipPlane, (fnptr_t)CDirect3DDevice_SetRenderState, (fnptr_t)CDirect3DDevice_GetRenderState, (fnptr_t)CDirect3DDevice_CreateStateBlock, (fnptr_t)CDirect3DDevice_BeginStateBlock, (fnptr_t)CDirect3DDevice_EndStateBlock, (fnptr_t)CDirect3DDevice_SetClipStatus, (fnptr_t)CDirect3DDevice_GetClipStatus, (fnptr_t)CDirect3DDevice_GetTexture, (fnptr_t)CDirect3DDevice_SetTexture, (fnptr_t)CDirect3DDevice_GetTextureStageState, (fnptr_t)CDirect3DDevice_SetTextureStageState, (fnptr_t)CDirect3DDevice_GetSamplerState, (fnptr_t)CDirect3DDevice_SetSamplerState, (fnptr_t)CDirect3DDevice_ValidateDevice, (fnptr_t)CDirect3DDevice_SetPaletteEntries, (fnptr_t)CDirect3DDevice_GetPaletteEntries, (fnptr_t)CDirect3DDevice_SetCurrentTexturePalette, (fnptr_t)CDirect3DDevice_GetCurrentTexturePalette, (fnptr_t)CDirect3DDevice_SetScissorRect, (fnptr_t)CDirect3DDevice_GetScissorRect, (fnptr_t)CDirect3DDevice_SetSoftwareVertexProcessing, (fnptr_t)CDirect3DDevice_GetSoftwareVertexProcessing, (fnptr_t)CDirect3DDevice_SetNPatchMode, (fnptr_t)CDirect3DDevice_GetNPatchMode, (fnptr_t)CDirect3DDevice_DrawPrimitive, (fnptr_t)CDirect3DDevice_DrawIndexedPrimitive, (fnptr_t)CDirect3DDevice_DrawPrimitiveUP, (fnptr_t)CDirect3DDevice_DrawIndexedPrimitiveUP, (fnptr_t)CDirect3DDevice_ProcessVertices, (fnptr_t)CDirect3DDevice_CreateVertexDeclaration, (fnptr_t)CDirect3DDevice_SetVertexDeclaration, (fnptr_t)CDirect3DDevice_GetVertexDeclaration, (fnptr_t)CDirect3DDevice_SetFVF, (fnptr_t)CDirect3DDevice_GetFVF, (fnptr_t)CDirect3DDevice_CreateVertexShader, (fnptr_t)CDirect3DDevice_SetVertexShader, (fnptr_t)CDirect3DDevice_GetVertexShader, (fnptr_t)CDirect3DDevice_SetVertexShaderConstantF, (fnptr_t)CDirect3DDevice_GetVertexShaderConstantF, (fnptr_t)CDirect3DDevice_SetVertexShaderConstantI, (fnptr_t)CDirect3DDevice_GetVertexShaderConstantI, (fnptr_t)CDirect3DDevice_SetVertexShaderConstantB, (fnptr_t)CDirect3DDevice_GetVertexShaderConstantB, (fnptr_t)CDirect3DDevice_SetStreamSource, (fnptr_t)CDirect3DDevice_GetStreamSource, (fnptr_t)CDirect3DDevice_SetStreamSourceFreq, (fnptr_t)CDirect3DDevice_GetStreamSourceFreq, (fnptr_t)CDirect3DDevice_SetIndices, (fnptr_t)CDirect3DDevice_GetIndices, (fnptr_t)CDirect3DDevice_CreatePixelShader, (fnptr_t)CDirect3DDevice_SetPixelShader, (fnptr_t)CDirect3DDevice_GetPixelShader, (fnptr_t)CDirect3DDevice_SetPixelShaderConstantF, (fnptr_t)CDirect3DDevice_GetPixelShaderConstantF, (fnptr_t)CDirect3DDevice_SetPixelShaderConstantI, (fnptr_t)CDirect3DDevice_GetPixelShaderConstantI, (fnptr_t)CDirect3DDevice_SetPixelShaderConstantB, (fnptr_t)CDirect3DDevice_GetPixelShaderConstantB, (fnptr_t)CDirect3DDevice_DrawRectPatch, (fnptr_t)CDirect3DDevice_DrawTriPatch, (fnptr_t)CDirect3DDevice_DeletePatch, (fnptr_t)CDirect3DDevice_CreateQuery, (fnptr_t)ZN15CDirect3DDeviceD1Ev, (fnptr_t)ZN15CDirect3DDeviceD0Ev, (fnptr_t)0 };

const unsigned char __ZZN15CDirect3DDeviceC4Ej11_D3DDEVTYPEP16OpaqueContextRefP23_D3DPRESENT_PARAMETERS_E5C_232[4320] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x77, 0x00, 0x02, 0x00, 0xea, 0x00, 0x02, 0x00, 0xf4, 0x00, 0x02, 0x00, 0x01, 0x01, 0x02, 0x00,
    0x0e, 0x01, 0x02, 0x00, 0x1b, 0x01, 0x02, 0x00, 0x28, 0x01, 0x02, 0x00, 0x35, 0x01, 0x02, 0x00,
    0x94, 0x00, 0x02, 0x00, 0x42, 0x01, 0x02, 0x00, 0x4c, 0x01, 0x02, 0x00, 0x53, 0x01, 0x02, 0x00,
    0x5d, 0x01, 0x02, 0x00, 0x67, 0x01, 0x02, 0x00, 0x73, 0x01, 0x02, 0x00, 0x8f, 0x00, 0x02, 0x00,
    0x8f, 0x00, 0x02, 0x00, 0x94, 0x00, 0x02, 0x00, 0x7d, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
