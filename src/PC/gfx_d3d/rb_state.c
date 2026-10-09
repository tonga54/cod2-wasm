#include "common_types.h"
extern dvar_t *r_rendererInUse;
extern Bool g_RenderToShadowCookie;
extern Bool g_InhibitCopy;
#include "imports.h"
extern int alwaysfails;
extern vidConfig_t vidConfig;
extern r_global_permanent_t rgp;
extern DxGlobals dx;

extern struct DxState dxState;
extern r_backEndGlobals_t backEnd;
static const DxTextureStageEnums texStageEnums = {
    D3DTSS_ALPHAOP, { D3DTSS_ALPHAARG1, D3DTSS_ALPHAARG2, D3DTSS_ALPHAARG0 }
};
static const DxTextureStageEnums texStageEnums_002f24ac = {
    D3DTSS_COLOROP, { D3DTSS_COLORARG1, D3DTSS_COLORARG2, D3DTSS_COLORARG0 }
};
extern const byte defaultSamplerStateTable[];
extern const DWORD s_blendTable[];
extern const DWORD s_blendOpTable[];
extern const DWORD s_cullTable[];
extern const byte s_filterTable[];
extern const DWORD s_stencilOpTable[];
extern const DWORD s_stencilFuncTable[];
extern const DxStencilDecode s_stencilOpDecode[];
extern const DxStencilDecode s_stencilFuncDecode[];
extern const DxTextureOpDecode s_textureOpTable[];
extern const DWORD s_textureArgTable[];
extern const GfxViewportBehavior s_viewportBehaviorForRenderTarget[];

#define VTABLE(obj) (*(void ***)((void *)(obj)))

typedef HRESULT(D3DVTCC *SetIndicesFn)(void *device, IDirect3DIndexBuffer9 *ib);
typedef HRESULT(D3DVTCC *SetRenderStateFn)(void *device, DWORD state, DWORD value);
typedef HRESULT(D3DVTCC *SetRenderTargetFn)(void *device, DWORD index, IDirect3DSurface9 *surface);
typedef HRESULT(D3DVTCC *SetDepthStencilSurfaceFn)(void *device, IDirect3DSurface9 *surface);
typedef HRESULT(D3DVTCC *SetStreamSourceFn)(void *device, UINT streamIndex, IDirect3DVertexBuffer9 *vb, UINT vertexOffset, UINT vertexStride);
typedef HRESULT(D3DVTCC *SetSamplerStateFn)(void *device, DWORD samplerIndex, DWORD samplerState, DWORD value);
typedef HRESULT(D3DVTCC *SetTextureFn)(void *device, DWORD samplerIndex, IDirect3DBaseTexture9 *texture);
typedef HRESULT(D3DVTCC *SetTextureStageStateFn)(void *device, DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value);
typedef HRESULT(D3DVTCC *SetTransformFn)(void *device, D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix);
typedef HRESULT(D3DVTCC *SetMaterialFn)(void *device, const D3DMATERIAL9 *material);
typedef HRESULT(D3DVTCC *SetViewportFn)(void *device, const D3DVIEWPORT9 *viewport);
typedef HRESULT(D3DVTCC *SetVertexDeclarationFn)(void *device, IDirect3DVertexDeclaration9 *vertexDecl);

typedef struct {
    IDirect3DVertexBuffer9 *vb;
    int offset;
    int stride;
} DxTrackedStreamState;

extern void MatrixInverse44(const float *mat, float *dst);
extern void MatrixIdentity44(float (*out)[4]);
extern void MatrixSet44(float (*out)[4], const vec_t *origin, vec3_t *axis, vec_t scale);
extern int MacOpenGLUtils_GetSubPixelOffset(float *xOffset, float *yOffset);
extern void RB_SetCodeConstant(int constant, vec_t x, vec_t y, vec_t z, vec_t w);

enum {
    RB_STENCIL_OP_DECODE_COUNT = 6,
    RB_STENCIL_FUNC_DECODE_COUNT = 2,
    RB_STATE0_SRC_BLEND_RGB_MASK = 0x0000000f,
    RB_STATE0_DST_BLEND_RGB_MASK = 0x000000f0,
    RB_STATE0_BLEND_OP_RGB_MASK = 0x00000700,
    RB_STATE0_BLEND_OP_MASKS = 0x07000700,
    RB_STATE0_ALPHA_TEST_DISABLE = 0x00000800,
    RB_STATE0_ALPHA_TEST_FUNC_MASK = 0x00003000,
    RB_STATE0_ALPHA_TEST_MASK = 0x00000f00,
    RB_STATE0_CULL_MASK = 0x0000c000,
    RB_STATE0_SRC_BLEND_ALPHA_MASK = 0x000f0000,
    RB_STATE0_DST_BLEND_ALPHA_MASK = 0x00f00000,
    RB_STATE0_BLEND_OP_ALPHA_MASK = 0x07000000,
    RB_STATE0_COLOR_WRITE_RGB = 0x08000000,
    RB_STATE0_COLOR_WRITE_ALPHA = 0x10000000,
    RB_STATE0_FOG_ENABLE = 0x20000000,
    RB_STATE0_NORMALIZE_NORMALS = 0x40000000,
    RB_STATE0_WIREFRAME = 0x80000000u,
    RB_STATE0_RGB_BLEND_BITS = 0x000007ff,
    RB_STATE0_ALPHA_BLEND_BITS = 0x07ff0000,
    RB_STATE1_DEPTH_WRITE = 0x00000001,
    RB_STATE1_DEPTH_TEST_DISABLE = 0x00000002,
    RB_STATE1_DEPTH_FUNC_MASK = 0x0000000c,
    RB_STATE1_POLYGON_OFFSET_MASK = 0x00000030,
    RB_STATE1_STENCIL_ENABLE = 0x00000040,
    RB_STATE1_STENCIL_TWO_SIDED = 0x00000080,
    RB_STATE1_STENCIL_DISABLED_MASK = 0x0000007f,
    RB_STATE1_FRONT_STENCIL_MASK = 0x000fffff,
    RB_STATE1_STENCIL_OP_MASK = 0x1ff1ff00,
    RB_STATE1_STENCIL_FUNC_MASK = 0xe00e0000
};

void RB_ChangeIndices(IDirect3DIndexBuffer9 *ib);
void RB_ChangeStreamSource(int streamIndex, IDirect3DVertexBuffer9 *vb, int vertexOffset, int vertexStride);
void RB_DecideDefaultSamplerState(void);
void RB_SetAnisotropy(void);
void RB_SetAlphaAntiAliasingState(int stateBits0);
static __attribute_regparm__(3) void RB_ChangeTextureStageState(int stageIndex, const DxTextureStageEnums *texStageEnums, int texStageBits, int *activeTexStageBits);
void RB_ChangeAlphaStageState(int stageIndex, int texStageBits);
void RB_ChangeColorStageState(int stageIndex, int texStageBits);
void RB_SetSamplerConstantDx7(unsigned int color);
void RB_ChangeGenTexCoords(int samplerIndex, int genTexCoords);
D3DMATRIX *RB_GetActiveWorldMatrix(void);
void RB_ChangedWorldMatrix(float worldScale);
void RB_SetViewMatrix(const D3DMATRIX *matrix);
void RB_SetViewMatrixForWDx7(float w);
void RB_SetDepthHackNearClip(float nearClip);
void RB_SetShadowLookupMatrix(const D3DMATRIX *matrix);
void RB_SetMatricesForView(const GfxViewParms *viewParms);
void RB_PushMatrixStack(void);
void RB_PopMatrixStack(void);
void RB_InitSceneViewport(void);
Bool RB_GetViewport(GfxViewport *outViewport);
void RB_SetDepthRange(float nearValue, float farValue);
void RB_SetViewport(const GfxViewport *viewport);
void RB_ReleaseVertexDecl(void);
void RB_ChangeState_1(int stateBits1);
void RB_SetSampler(int samplerIndex, int samplerState, GfxImage *image);
void RB_BindDefaultImages(void);
void RB_UnbindAllImages(void);
void RB_UnbindImage(const GfxImage *image);
void RB_UpdateViewportConstants(void);
void RB_InitImages(void);
void RB_SetRenderTarget(GfxRenderTargetId newTargetId);
void RB_ClearAllStreamSources(void);
void RB_ChangeState_0(int stateBits0);
void RB_SetProjectionMatrix(const D3DMATRIX *matrix);
void RB_UpdateViewport(void);
void RB_SetInitialState(void);
void RB_SetWorldMatrixForEntity(const GfxEntity *re);

static void RB_SetTextureStageStateDx7(int samplerIndex, D3DTEXTURESTAGESTATETYPE state, DWORD value)
{
    void *device;

    device = dx.device;
    do {
        ((SetTextureStageStateFn)VTABLE(device)[0x10c / 4])(device, samplerIndex, state, value);
    } while (*(volatile int *)&alwaysfails != 0);
}

static void RB_SetRenderStateDx7(DWORD state, DWORD value)
{
    void *device;

    device = dx.device;
    do {
        ((SetRenderStateFn)VTABLE(device)[0xe4 / 4])(device, state, value);
    } while (*(volatile int *)&alwaysfails != 0);
}

static inline __attribute__((always_inline)) void RB_SetTransformDx7(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix)
{
    void *device;

    device = dx.device;
    do {
        ((SetTransformFn)VTABLE(device)[0xb0 / 4])(device, state, matrix);
    } while (*(volatile int *)&alwaysfails != 0);
}

static inline __attribute__((always_inline)) void RB_SetViewportDx7(const D3DVIEWPORT9 *viewport)
{
    void *device;

    device = dx.device;
    do {
        ((SetViewportFn)VTABLE(device)[0xbc / 4])(device, viewport);
    } while (*(volatile int *)&alwaysfails != 0);
}

static void RB_SetMaterialDx7(const D3DMATERIAL9 *material)
{
    void *device;

    device = dx.device;
    ((SetMaterialFn)VTABLE(device)[0xc4 / 4])(device, material);
}

static void RB_SetRenderTargetSurfaceDx7(IDirect3DSurface9 *surface)
{
    void *device;

    device = dx.device;
    do {
        ((SetRenderTargetFn)VTABLE(device)[0x94 / 4])(device, 0, surface);
    } while (*(volatile int *)&alwaysfails != 0);
}

static void RB_SetDepthStencilSurfaceDx7(IDirect3DSurface9 *surface)
{
    void *device;

    device = dx.device;
    do {
        ((SetDepthStencilSurfaceFn)VTABLE(device)[0x9c / 4])(device, surface);
    } while (*(volatile int *)&alwaysfails != 0);
}

static void RB_SetTextureDx7(int samplerIndex, IDirect3DBaseTexture9 *texture)
{
    void *device;

    device = dx.device;
    do {
        ((SetTextureFn)VTABLE(device)[0x104 / 4])(device, samplerIndex, texture);
    } while (*(volatile int *)&alwaysfails != 0);
}

static void RB_SetSamplerStateDx7(int samplerIndex, DWORD samplerState, DWORD value)
{
    void *device;

    device = dx.device;
    do {
        ((SetSamplerStateFn)VTABLE(device)[0x114 / 4])(device, samplerIndex, samplerState, value);
    } while (*(volatile int *)&alwaysfails != 0);
}

static void RB_SetRenderStateFloatDx7(DWORD state, float value)
{
    union {
        float value;
        DWORD bits;
    } packedValue;

    packedValue.value = value;
    RB_SetRenderStateDx7(state, packedValue.bits);
}

static inline __attribute__((always_inline)) GfxCodeMatrices *RB_GetActiveCodeMatrices(void)
{
    r_backEndGlobals_t *be = &backEnd;
    return &be->codeMatrixStack[be->codeMatrixStackLevel];
}

static Bool RB_UsingDx7Renderer(void);

static inline __attribute__((always_inline)) void RB_InvalidateCodeMatrix(GfxCodeMatrix *matrix)
{
    matrix->valid[0] = 0;
    matrix->valid[1] = 0;
    matrix->valid[2] = 0;
    matrix->valid[3] = 0;
}

static inline __attribute__((always_inline)) void RB_ValidatePrimaryCodeMatrix(GfxCodeMatrix *matrix)
{
    matrix->valid[0] = 1;
    matrix->valid[1] = 0;
    matrix->valid[2] = 0;
    matrix->valid[3] = 0;
}

static inline __attribute__((always_inline)) void RB_SetPrimaryCodeMatrix(GfxCodeMatrix *matrix, const D3DMATRIX *src)
{
    matrix->matrix[0] = *src;
    RB_ValidatePrimaryCodeMatrix(matrix);
}

static inline __attribute__((always_inline)) void RB_FinalizeWorldMatrixChange(GfxCodeMatrices *activeMatrices, float worldScale)
{
    activeMatrices->worldScale = worldScale;
    RB_ValidatePrimaryCodeMatrix(&activeMatrices->world);
    RB_InvalidateCodeMatrix(&activeMatrices->normalizedWorld);
    RB_InvalidateCodeMatrix(&activeMatrices->worldView);
    RB_InvalidateCodeMatrix(&activeMatrices->normalizedWorldView);
    RB_InvalidateCodeMatrix(&activeMatrices->worldViewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->normalizedWorldViewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->shadowLookupMatrix);
    RB_InvalidateCodeMatrix(&activeMatrices->lightGridLookupMatrix);
    RB_InvalidateCodeMatrix(&activeMatrices->worldOutdoorLookup);
    RB_InvalidateCodeMatrix(&activeMatrices->OGLworldViewProjection);

    if (RB_UsingDx7Renderer()) {
        RB_SetTransformDx7(0x100, &activeMatrices->world.matrix[0]);
    }
}

static Bool RB_UsingDx7Renderer(void)
{
    return r_rendererInUse->current.integer == 2;
}

static Bool RB_SupportsSlopeScaleDepthBias(void)
{
    return dx.slopeScaleDepthBias != 0;
}

static Bool RB_SupportsAlphaToCoverage(void)
{
    return dx.hasTransparencyMsaa != 0;
}

static int RB_NextPowerOfTwo(int value)
{
    int powerOfTwo;

    if (value <= 1) {
        return 1;
    }

    powerOfTwo = 1;
    while (powerOfTwo < value) {
        powerOfTwo <<= 1;
    }

    return powerOfTwo;
}

static DWORD RB_GetFilterTableValue(byte filterIndex)
{
    const byte *entry;

    filterIndex &= 3;
    entry = &s_filterTable[filterIndex * 4];
    return (DWORD)entry[0] | ((DWORD)entry[1] << 8) | ((DWORD)entry[2] << 16) | ((DWORD)entry[3] << 24);
}

static void RB_GetSamplerFilterModes(byte samplerState, DWORD *minFilter, DWORD *magFilter)
{
    DxGlobals *dx;

    dx = (DxGlobals *)imp_dx;
    *minFilter = RB_GetFilterTableValue(samplerState & 3);
    *magFilter = *minFilter;
    if (*minFilter == D3DTEXF_ANISOTROPIC) {
        if (dx->anisotropy > 0) {
            *magFilter = D3DTEXF_LINEAR;
        } else {
            *minFilter = D3DTEXF_LINEAR;
            *magFilter = D3DTEXF_LINEAR;
        }
    }
}

void RB_ChangeIndices(IDirect3DIndexBuffer9 *ib)
{
    void *device;

    dxState.indexBufferDeselecting = dxState.indexBuffer;
    dxState.indexBuffer = ib;

    device = dx.device;
    do {
        ((SetIndicesFn)VTABLE(device)[0x1a0 / 4])(device, ib);
    } while (*(volatile int *)&alwaysfails != 0);

    dxState.indexBufferDeselecting = NULL;
}

void RB_ChangeStreamSource(int streamIndex, IDirect3DVertexBuffer9 *vb, int vertexOffset, int vertexStride)
{
    DxTrackedStreamState *streamState;
    void *device;

    streamState = (DxTrackedStreamState *)&dxState.streams[streamIndex];
    dxState.vertexBufferDeselecting = streamState->vb;
    streamState->vb = vb;
    streamState->offset = vertexOffset;
    streamState->stride = vertexStride;

    device = dx.device;
    do {
        ((SetStreamSourceFn)VTABLE(device)[0x190 / 4])(device, streamIndex, vb, vertexOffset, vertexStride);
    } while (*(volatile int *)&alwaysfails != 0);

    dxState.vertexBufferDeselecting = NULL;
}

static inline __attribute__((always_inline)) void RB_DecideDefaultSamplerState_core(void)
{
    int idx = (*(const dvar_t **)imp_r_textureMode)->current.integer;
    backEnd.defaultSamplerState = defaultSamplerStateTable[idx];
}

static inline __attribute__((always_inline)) void RB_SetAnisotropy_core(void)
{
    const dvar_t *anisotropyDvar;
    DxGlobals *dx;
    void *device;
    int anisotropy;
    int samplerCount;
    int samplerIndex;

    anisotropyDvar = *(const dvar_t **)imp_r_anisotropy;
    dx = (DxGlobals *)imp_dx;
    dx->anisotropy = anisotropyDvar->current.integer;
    if (dx->anisotropy > dx->maxAnisotropy) {
        dx->anisotropy = dx->maxAnisotropy;
    }
    if (dx->anisotropy <= 1) {
        dx->anisotropy = 0;
    }

    anisotropy = dx->anisotropy;
    if (anisotropy < 1) {
        anisotropy = 1;
    }

    samplerCount = vidConfig.maxTextureMaps;
    if (samplerCount <= 0) {
        return;
    }

    device = dx->device;
    for (samplerIndex = 0; samplerIndex < samplerCount; ++samplerIndex) {
        do {
            ((SetSamplerStateFn)VTABLE(device)[0x114 / 4])(device, samplerIndex, 0xa, anisotropy);
        } while (*(volatile int *)&alwaysfails != 0);
    }
}

void RB_DecideDefaultSamplerState(void)
{
    RB_DecideDefaultSamplerState_core();
}

void RB_SetAnisotropy(void)
{
    RB_SetAnisotropy_core();
}

void RB_SetAlphaAntiAliasingState(int stateBits0)
{
    void *device;
    DWORD aaAlphaFormat;

    if (stateBits0 & 0xf00) {
        aaAlphaFormat = 0;
    } else if ((*(const dvar_t **)imp_r_aaAlpha)->current.integer == 2) {
        aaAlphaFormat = 0x41415353;
    } else {
        aaAlphaFormat = 0x434f5441;
    }

    device = dx.device;
    do {
        ((SetRenderStateFn)VTABLE(device)[0xe4 / 4])(device, 0xb5, aaAlphaFormat);
    } while (*(volatile int *)&alwaysfails != 0);
}

static __attribute_regparm__(3) void RB_ChangeTextureStageState(int stageIndex, const DxTextureStageEnums *texStageEnums, int texStageBits, int *activeTexStageBits)
{
    int *activeTexStageState;
    int changedBits;
    const DxTextureOpDecode *texOp;
    int argIndex;
    int argShift;
    int argMask;

    activeTexStageState = &activeTexStageBits[stageIndex];
    changedBits = texStageBits ^ *activeTexStageState;
    texOp = &s_textureOpTable[texStageBits & 0x1f];

    if (changedBits & 0x1f) {
        RB_SetTextureStageStateDx7(stageIndex, texStageEnums->op, texOp->enumerant);
    }

    argShift = 10;
    argMask = 0x7c00;
    for (argIndex = 0; argIndex < 3; ++argIndex) {
        if (changedBits & argMask) {
            if (texOp->usedArgs & (1 << argIndex)) {
                unsigned int argStateBits;
                DWORD texArg;

                argStateBits = (unsigned int)texStageBits >> argShift;
                texArg = s_textureArgTable[argStateBits & 7];
                if (argStateBits & 8) {
                    texArg |= 0x10;
                }
                if (argStateBits & 0x10) {
                    texArg |= 0x20;
                }

                RB_SetTextureStageStateDx7(stageIndex, texStageEnums->arg[argIndex], texArg);
            } else {
                texStageBits = (texStageBits & ~argMask) | (*activeTexStageState & argMask);
            }
        }

        argShift += 5;
        argMask <<= 5;
    }

    *activeTexStageState = texStageBits;
}

void RB_ChangeAlphaStageState(int stageIndex, int texStageBits)
{
    RB_ChangeTextureStageState(stageIndex, &texStageEnums, texStageBits, dxState.activeAlphaStageBits);
}

void RB_ChangeColorStageState(int stageIndex, int texStageBits)
{

    RB_ChangeTextureStageState(stageIndex, &texStageEnums_002f24ac, texStageBits, dxState.activeColorStageBits);
}

void RB_SetSamplerConstantDx7(unsigned int color)
{
    void *device;

    if (color == dxState.textureFactor) {
        return;
    }

    dxState.textureFactor = color;
    device = dx.device;
    do {
        ((SetRenderStateFn)VTABLE(device)[0xe4 / 4])(device, 0x3c, color);
    } while (*(volatile int *)&alwaysfails != 0);
}

void RB_ChangeGenTexCoords(int samplerIndex, int genTexCoords)
{
    D3DMATRIX transform;

    switch ((byte)genTexCoords) {
    case 0:
        RB_SetTextureStageStateDx7(samplerIndex, D3DTSS_TEXCOORDINDEX, samplerIndex);
        RB_SetTextureStageStateDx7(samplerIndex, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        break;
    case 1:
        MatrixInverse44((const float *)&backEnd.viewParms->viewMatrix, (float *)&transform);
        transform._41 = 0.0f;
        transform._42 = 0.0f;
        transform._43 = 0.0f;
        RB_SetTextureStageStateDx7(samplerIndex, D3DTSS_TEXCOORDINDEX, 0x20000);
        RB_SetTextureStageStateDx7(samplerIndex, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
        RB_SetTransformDx7(D3DTS_TEXTURE0 + samplerIndex, &transform);
        break;
    case 2:
        MatrixIdentity44(transform.m);
        transform._32 = -backEnd.viewParms->depthHackNearClip;
        RB_SetTextureStageStateDx7(samplerIndex, D3DTSS_TEXCOORDINDEX, samplerIndex);
        RB_SetTextureStageStateDx7(samplerIndex, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
        RB_SetTransformDx7(D3DTS_TEXTURE0 + samplerIndex, &transform);
        break;
    default:
        break;
    }

    dxState.genTexCoords[samplerIndex] = (byte)genTexCoords;
}

D3DMATRIX *RB_GetActiveWorldMatrix(void)
{
    return &RB_GetActiveCodeMatrices()->world.matrix[0];
}

void RB_ChangedWorldMatrix(float worldScale)
{
    GfxCodeMatrices *activeMatrices;

    activeMatrices = RB_GetActiveCodeMatrices();
    RB_FinalizeWorldMatrixChange(activeMatrices, worldScale);
}

void RB_SetViewMatrix(const D3DMATRIX *matrix)
{
    GfxCodeMatrices *activeMatrices;

    activeMatrices = RB_GetActiveCodeMatrices();
    RB_SetPrimaryCodeMatrix(&activeMatrices->view, matrix);
    RB_InvalidateCodeMatrix(&activeMatrices->worldView);
    RB_InvalidateCodeMatrix(&activeMatrices->normalizedWorldView);
    RB_InvalidateCodeMatrix(&activeMatrices->viewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->worldViewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->normalizedWorldViewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->OGLworldViewProjection);

    if (RB_UsingDx7Renderer()) {
        RB_SetTransformDx7(D3DTS_VIEW, matrix);
    }
}

void RB_SetViewMatrixForWDx7(float w)
{
    D3DMATRIX transform;

    transform = backEnd.viewParms->viewMatrix;
    transform._44 = w;
    RB_SetTransformDx7(D3DTS_VIEW, &transform);
}

void RB_SetDepthHackNearClip(float nearClip)
{
    GfxCodeMatrices *activeMatrices;

    activeMatrices = RB_GetActiveCodeMatrices();
    activeMatrices->projection.matrix[0]._43 = -nearClip;
    RB_ValidatePrimaryCodeMatrix(&activeMatrices->projection);
    RB_InvalidateCodeMatrix(&activeMatrices->viewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->worldViewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->normalizedWorldViewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->OGLworldViewProjection);

    if (RB_UsingDx7Renderer()) {
        RB_SetTransformDx7(D3DTS_PROJECTION, &activeMatrices->projection.matrix[0]);
    }
}

void RB_SetShadowLookupMatrix(const D3DMATRIX *matrix)
{
    r_backEndGlobals_t *be = &backEnd;
    be->shadowLookupMatrix = *matrix;
    RB_InvalidateCodeMatrix(&be->codeMatrixStack[0].shadowLookupMatrix);
}

void RB_SetMatricesForView(const GfxViewParms *viewParms)
{
    GfxCodeMatrices *activeMatrices;

    activeMatrices = RB_GetActiveCodeMatrices();
    RB_SetPrimaryCodeMatrix(&activeMatrices->view, &viewParms->viewMatrix);
    RB_SetPrimaryCodeMatrix(&activeMatrices->projection, &viewParms->projectionMatrix);
    activeMatrices->viewProjection.matrix[0] = viewParms->viewProjectionMatrix;
    activeMatrices->viewProjection.matrix[1] = viewParms->inverseViewProjectionMatrix;
    activeMatrices->viewProjection.valid[0] = 1;
    activeMatrices->viewProjection.valid[1] = 1;
    activeMatrices->viewProjection.valid[2] = 0;
    activeMatrices->viewProjection.valid[3] = 0;
    RB_InvalidateCodeMatrix(&activeMatrices->worldView);
    RB_InvalidateCodeMatrix(&activeMatrices->worldViewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->normalizedWorldViewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->OGLworldViewProjection);

    if (RB_UsingDx7Renderer()) {
        RB_SetTransformDx7(D3DTS_VIEW, &viewParms->viewMatrix);
        RB_SetTransformDx7(D3DTS_PROJECTION, &viewParms->projectionMatrix);
    }
}

void RB_PushMatrixStack(void)
{
    r_backEndGlobals_t *be = &backEnd;

    be->codeMatrixStack[be->codeMatrixStackLevel + 1] = *RB_GetActiveCodeMatrices();
    be->codeMatrixStackLevel += 1;
}

void RB_PopMatrixStack(void)
{
    backEnd.codeMatrixStackLevel -= 1;
    if (!RB_UsingDx7Renderer()) {
        return;
    }

    RB_SetTransformDx7(0x100, &RB_GetActiveCodeMatrices()->world.matrix[0]);
    RB_SetTransformDx7(D3DTS_VIEW, &RB_GetActiveCodeMatrices()->view.matrix[0]);
    RB_SetTransformDx7(D3DTS_PROJECTION, &RB_GetActiveCodeMatrices()->projection.matrix[0]);
}

void RB_InitSceneViewport(void)
{
    r_backEndGlobals_t *be = &backEnd;
    vidConfig_t *vc = (vidConfig_t *)imp_vidConfig;
    be->sceneViewport.width = vc->width;
    be->sceneViewport.height = vc->height;
}

Bool RB_GetViewport(GfxViewport *outViewport)
{
    if (dxState.viewportBehavior == GFX_USE_VIEWPORT_FULL) {
        outViewport->x = 0;
        outViewport->y = 0;
        outViewport->width = dxState.renderTargetWidth;
        outViewport->height = dxState.renderTargetHeight;
    } else {
        r_backEndGlobals_t *be = &backEnd;
        outViewport->x = be->sceneViewport.x;
        outViewport->y = be->sceneViewport.y;
        outViewport->width = be->sceneViewport.width;
        outViewport->height = be->sceneViewport.height;
    }

    return 1;
}

void RB_SetDepthRange(float nearValue, float farValue)
{
    if (nearValue == dxState.viewport.MinZ && farValue == dxState.viewport.MaxZ) {
        return;
    }

    dxState.viewport.MinZ = nearValue;
    dxState.viewport.MaxZ = farValue;
    RB_SetViewportDx7(&dxState.viewport);
}

void RB_SetViewport(const GfxViewport *viewport)
{
    if ((DWORD)viewport->x == dxState.viewport.X &&
        (DWORD)viewport->y == dxState.viewport.Y &&
        (DWORD)viewport->width == dxState.viewport.Width &&
        (DWORD)viewport->height == dxState.viewport.Height) {
        return;
    }

    dxState.viewport.X = viewport->x;
    dxState.viewport.Y = viewport->y;
    dxState.viewport.Width = viewport->width;
    dxState.viewport.Height = viewport->height;
    RB_SetViewportDx7(&dxState.viewport);
}

void RB_ReleaseVertexDecl(void)
{
    void *device;

    if (dxState.vertexDecl == NULL && dxState.fvf == 0) {
        return;
    }

    device = dx.device;
    do {
        ((SetVertexDeclarationFn)VTABLE(device)[0x164 / 4])(device, NULL);
    } while (*(volatile int *)&alwaysfails != 0);

    dxState.fvf = 0;
    dxState.vertexDecl = NULL;
}

void RB_ChangeState_1(int stateBits1)
{
    int changedBits;
    int decodeIndex;

    changedBits = stateBits1 ^ dxState.activeStateBits[1];
    if (!changedBits) {
        return;
    }

    if (changedBits & RB_STATE1_DEPTH_WRITE) {
        RB_SetRenderStateDx7(D3DRS_ZWRITEENABLE, stateBits1 & RB_STATE1_DEPTH_WRITE);
    }

    if (changedBits & RB_STATE1_DEPTH_TEST_DISABLE) {
        RB_SetRenderStateDx7(D3DRS_ZENABLE, (((unsigned int)stateBits1 >> 1) & 1) ^ 1);
    }

    if (stateBits1 & RB_STATE1_DEPTH_TEST_DISABLE) {
        stateBits1 |= dxState.activeStateBits[1] & RB_STATE1_DEPTH_FUNC_MASK;
        changedBits &= ~RB_STATE1_DEPTH_FUNC_MASK;
    }

    if (changedBits & RB_STATE1_DEPTH_FUNC_MASK) {
        DWORD depthFunc;

        switch (stateBits1 & RB_STATE1_DEPTH_FUNC_MASK) {
        case 0x4:
            depthFunc = 4;
            break;
        case 0x8:
            depthFunc = 3;
            break;
        default:
            depthFunc = 8;
            break;
        }

        RB_SetRenderStateDx7(D3DRS_ZFUNC, depthFunc);
    }

    if (changedBits & RB_STATE1_POLYGON_OFFSET_MASK) {
        const dvar_t *polygonOffsetBias;
        float offsetUnits;
        float depthBias;

        polygonOffsetBias = *(const dvar_t **)imp_r_polygonOffsetBias;
        offsetUnits = (float)((stateBits1 & RB_STATE1_POLYGON_OFFSET_MASK) >> 4);
        depthBias = offsetUnits * polygonOffsetBias->current.value * 1.52587890625e-05f;
        if (RB_SupportsSlopeScaleDepthBias()) {
            const dvar_t *polygonOffsetScale;

            polygonOffsetScale = *(const dvar_t **)imp_r_polygonOffsetScale;
            RB_SetRenderStateFloatDx7(D3DRS_SLOPESCALEDEPTHBIAS, offsetUnits * polygonOffsetScale->current.value);
        } else {
            depthBias *= 2.0f;
        }

        RB_SetRenderStateFloatDx7(D3DRS_DEPTHBIAS, depthBias);
    }

    if (stateBits1 & RB_STATE1_STENCIL_ENABLE) {
        if (changedBits & RB_STATE1_STENCIL_ENABLE) {
            RB_SetRenderStateDx7(D3DRS_STENCILENABLE, 1);
        }
    } else {
        if (changedBits & RB_STATE1_STENCIL_ENABLE) {
            RB_SetRenderStateDx7(D3DRS_STENCILENABLE, 0);
        }

        stateBits1 |= dxState.activeStateBits[1] & ~RB_STATE1_STENCIL_DISABLED_MASK;
        changedBits &= RB_STATE1_STENCIL_DISABLED_MASK;
    }

    if (stateBits1 & RB_STATE1_STENCIL_TWO_SIDED) {
        if (changedBits & RB_STATE1_STENCIL_TWO_SIDED) {
            RB_SetRenderStateDx7(D3DRS_TWOSIDEDSTENCILMODE, 1);
        }
    } else {
        if (changedBits & RB_STATE1_STENCIL_TWO_SIDED) {
            RB_SetRenderStateDx7(D3DRS_TWOSIDEDSTENCILMODE, 0);
        }

        stateBits1 |= dxState.activeStateBits[1] & ~RB_STATE1_FRONT_STENCIL_MASK;
        changedBits &= RB_STATE1_FRONT_STENCIL_MASK;
    }

    if (changedBits & RB_STATE1_STENCIL_OP_MASK) {
        for (decodeIndex = 0; decodeIndex < RB_STENCIL_OP_DECODE_COUNT; ++decodeIndex) {
            const DxStencilDecode *decode;

            decode = &s_stencilOpDecode[decodeIndex];
            if (((changedBits >> decode->shift) & 7) != 0) {
                RB_SetRenderStateDx7(decode->renderState, s_stencilOpTable[(stateBits1 >> decode->shift) & 7]);
            }
        }
    }

    if (changedBits & RB_STATE1_STENCIL_FUNC_MASK) {
        for (decodeIndex = 0; decodeIndex < RB_STENCIL_FUNC_DECODE_COUNT; ++decodeIndex) {
            const DxStencilDecode *decode;

            decode = &s_stencilFuncDecode[decodeIndex];
            if (((changedBits >> decode->shift) & 7) != 0) {
                RB_SetRenderStateDx7(decode->renderState, s_stencilFuncTable[(stateBits1 >> decode->shift) & 7]);
            }
        }
    }

    dxState.activeStateBits[1] = stateBits1;
}

void RB_SetSampler(int samplerIndex, int samplerState, GfxImage *image)
{
    byte desiredSamplerState;
    byte currentSamplerState;
    byte diffSamplerState;
    DWORD minFilter;
    DWORD magFilter;
    DWORD prevMinFilter;
    DWORD prevMagFilter;

    desiredSamplerState = (byte)samplerState;
    if (dxState.samplerImage[samplerIndex] != image) {
        RB_SetTextureDx7(samplerIndex, image ? image->texture.basemap : NULL);
        dxState.samplerImage[samplerIndex] = image;
    }

    if (image == NULL) {
        return;
    }

    if ((desiredSamplerState & 0xf) == 0) {
        desiredSamplerState |= (byte)backEnd.defaultSamplerState;
    }

    currentSamplerState = dxState.samplerState[samplerIndex];
    diffSamplerState = desiredSamplerState ^ currentSamplerState;
    if (!diffSamplerState) {
        return;
    }

    if (diffSamplerState & 3) {
        RB_GetSamplerFilterModes(desiredSamplerState, &minFilter, &magFilter);
        RB_GetSamplerFilterModes(currentSamplerState, &prevMinFilter, &prevMagFilter);
        if (minFilter != prevMinFilter) {
            RB_SetSamplerStateDx7(samplerIndex, D3DSAMP_MINFILTER, minFilter);
        }
        if (magFilter != prevMagFilter) {
            RB_SetSamplerStateDx7(samplerIndex, D3DSAMP_MAGFILTER, magFilter);
        }
    }

    if (diffSamplerState & 0xc) {
        RB_SetSamplerStateDx7(samplerIndex, D3DSAMP_MIPFILTER, RB_GetFilterTableValue((desiredSamplerState >> 2) & 3));
    }

    if (diffSamplerState & 0x10) {
        RB_SetSamplerStateDx7(samplerIndex, D3DSAMP_ADDRESSU, (desiredSamplerState & 0x10) ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP);
    }

    if (diffSamplerState & 0x20) {
        RB_SetSamplerStateDx7(samplerIndex, D3DSAMP_ADDRESSV, (desiredSamplerState & 0x20) ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP);
    }

    if (diffSamplerState & 0x40) {
        RB_SetSamplerStateDx7(samplerIndex, D3DSAMP_ADDRESSW, (desiredSamplerState & 0x40) ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP);
    }

    dxState.samplerState[samplerIndex] = desiredSamplerState;
}

void RB_BindDefaultImages(void)
{
    void *defaultImage = rgp.whiteImage;

    for (int i = 0; i < 16; i++) {
        RB_SetSampler(i, dxState.samplerState[i], defaultImage);
    }
}

void RB_UnbindAllImages(void)
{
    if (dx.deviceLost)
        return;

    int count = vidConfig.maxTextureMaps;
    for (int i = 0; i < count; i++) {
        RB_SetSampler(i, 0, NULL);
    }
}

void RB_UnbindImage(const GfxImage *image)
{
    int count = vidConfig.maxTextureMaps;

    for (int i = 0; i < count; i++) {
        if (dxState.samplerImage[i] == image) {
            RB_SetSampler(i, 0, NULL);
        }
    }
}

void RB_UpdateViewportConstants(void)
{
    GfxViewport viewport;
    float invRenderTargetWidth;
    float invRenderTargetHeight;
    float halfViewportWidth;
    float halfViewportHeight;
    float viewportCenterX;
    float viewportCenterY;
    float horizontalScale;
    float verticalScale;
    float xOffset;
    float yOffset;
    int potWidth;
    int potHeight;

    backEnd.projection2D = 0;
    backEnd.viewportIsDirty = 1;

    invRenderTargetWidth = 1.0f / dxState.renderTargetWidth;
    invRenderTargetHeight = 1.0f / dxState.renderTargetHeight;
    if (dxState.viewportBehavior == GFX_USE_VIEWPORT_FULL) {
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = dxState.renderTargetWidth;
        viewport.height = dxState.renderTargetHeight;
    } else {
        viewport = backEnd.sceneViewport;
    }

    halfViewportWidth = 0.5f * viewport.width * invRenderTargetWidth;
    halfViewportHeight = 0.5f * viewport.height * invRenderTargetHeight;
    viewportCenterX = viewport.x * invRenderTargetWidth + halfViewportWidth;
    viewportCenterY = viewport.y * invRenderTargetHeight + halfViewportHeight;

    MacOpenGLUtils_GetSubPixelOffset(&xOffset, &yOffset);
    viewportCenterX += invRenderTargetWidth * xOffset;
    viewportCenterY += invRenderTargetHeight * yOffset;

    potWidth = RB_NextPowerOfTwo(viewport.width);
    potHeight = RB_NextPowerOfTwo(viewport.height);
    horizontalScale = (float)viewport.width / potWidth;
    verticalScale = (float)viewport.height / potHeight;

    RB_SetCodeConstant(0xab, ((float)viewport.width - 0.5f) / potWidth, ((float)viewport.height - 0.5f) / potHeight, invRenderTargetWidth, invRenderTargetHeight);
    RB_SetCodeConstant(0xae, halfViewportWidth * horizontalScale, halfViewportHeight * verticalScale, 0.0f, 1.0f);
    RB_SetCodeConstant(0xaf, viewportCenterX * horizontalScale, viewportCenterY * verticalScale, 0.0f, 0.0f);
}

void RB_InitImages(void)
{
    int samplerIndex;

    for (samplerIndex = 0; samplerIndex < 16; ++samplerIndex) {
        RB_SetSampler(samplerIndex, dxState.samplerState[samplerIndex], rgp.whiteImage);
    }
}

void RB_SetRenderTarget(GfxRenderTargetId newTargetId)
{
    DxGlobals *dx;
    const vidConfig_t *vidConfig;
    GfxRenderTarget *renderTarget;
    int samplerIndex;

    if (dxState.renderTargetId == newTargetId) {
        return;
    }

    g_InhibitCopy = (newTargetId == R_RENDERTARGET_SHADOWCOOKIE && dxState.renderTargetId == R_RENDERTARGET_DYNAMICSHADOWS);
    dxState.renderTargetId = newTargetId;

    dx = (DxGlobals *)imp_dx;
    renderTarget = &dx->renderTargets[newTargetId];
    if (renderTarget->image != NULL) {
        vidConfig = (const vidConfig_t *)imp_vidConfig;
        for (samplerIndex = 0; samplerIndex < vidConfig->maxTextureMaps; ++samplerIndex) {
            if (dxState.samplerImage[samplerIndex] == renderTarget->image) {
                RB_SetSampler(samplerIndex, 0, NULL);
            }
        }
    }

    RB_SetRenderTargetSurfaceDx7(renderTarget->colorSurface);
    dxState.renderTargetSurface = renderTarget->colorSurface;

    switch (newTargetId) {
    case R_RENDERTARGET_FRAME_BUFFER:
        glDrawBuffer(0x405);
        glReadBuffer(0x405);
        break;
    case R_RENDERTARGET_DYNAMICSHADOWS:
        glDrawBuffer(0x40A);
        glReadBuffer(0x40A);
        break;
    case R_RENDERTARGET_SHADOWCOOKIE:
    case R_RENDERTARGET_BLURRED_SCREEN:
    case R_RENDERTARGET_GLOW_0:
    case R_RENDERTARGET_PINGPONG_0:
    case R_RENDERTARGET_PINGPONG_1:
        glDrawBuffer(0x409);
        glReadBuffer(0x409);
        break;
    default:
        break;
    }

    g_RenderToShadowCookie = (newTargetId == R_RENDERTARGET_SHADOWCOOKIE);
    dxState.viewportBehavior = s_viewportBehaviorForRenderTarget[newTargetId];
    dxState.renderTargetWidth = renderTarget->width;
    dxState.renderTargetHeight = renderTarget->height;
    dxState.viewport.X = 0;
    dxState.viewport.Y = 0;
    dxState.viewport.Width = renderTarget->width;
    dxState.viewport.Height = renderTarget->height;
    dxState.viewport.MinZ = 0.0f;
    dxState.viewport.MaxZ = 1.0f;

    if (dxState.depthStencilSurface != renderTarget->depthStencilSurface) {
        RB_SetDepthStencilSurfaceDx7(renderTarget->depthStencilSurface);
        dxState.depthStencilSurface = renderTarget->depthStencilSurface;
    }

    RB_UpdateViewportConstants();
}

void RB_ClearAllStreamSources(void)
{
    void *device;

    if (dxState.streams[0].vb == NULL && dxState.streams[0].offset == 0 && dxState.streams[0].stride == 0) {
        return;
    }

    dxState.vertexBufferDeselecting = dxState.streams[0].vb;
    dxState.streams[0].vb = NULL;
    dxState.streams[0].offset = 0;
    dxState.streams[0].stride = 0;

    device = dx.device;
    do {
        ((SetStreamSourceFn)VTABLE(device)[0x190 / 4])(device, 0, NULL, 0, 0);
    } while (*(volatile int *)&alwaysfails != 0);

    dxState.vertexBufferDeselecting = NULL;
}

void RB_ChangeState_0(int stateBits0)
{
    unsigned int changedBits;

    changedBits = stateBits0 ^ dxState.activeStateBits[0];
    if (!changedBits && (((unsigned int)(stateBits0 ^ dxState.refStateBits[0]) & RB_STATE0_BLEND_OP_MASKS) == 0)) {
        return;
    }

    if (changedBits & RB_STATE0_ALPHA_TEST_DISABLE) {
        RB_SetRenderStateDx7(D3DRS_ALPHATESTENABLE, (((unsigned int)stateBits0 >> 11) & 1) ^ 1);
    }

    if (stateBits0 & RB_STATE0_ALPHA_TEST_DISABLE) {
        stateBits0 = (stateBits0 & ~RB_STATE0_ALPHA_TEST_FUNC_MASK) | (dxState.activeStateBits[0] & RB_STATE0_ALPHA_TEST_FUNC_MASK);
        changedBits &= ~RB_STATE0_ALPHA_TEST_FUNC_MASK;
    } else {
        DWORD alphaFunc;
        byte alphaRef;

        switch (stateBits0 & RB_STATE0_ALPHA_TEST_FUNC_MASK) {
        case 0x1000:
            alphaFunc = 5;
            alphaRef = 0;
            break;
        case 0x2000:
            alphaFunc = 2;
            alphaRef = 0x80;
            break;
        default:
            alphaFunc = 7;
            alphaRef = 0x80;
            break;
        }

        if (changedBits & RB_STATE0_ALPHA_TEST_FUNC_MASK)
            RB_SetRenderStateDx7(D3DRS_ALPHAFUNC, alphaFunc);

        dxState.alphaRef = alphaRef;
        RB_SetRenderStateDx7(D3DRS_ALPHAREF, alphaRef);
    }

    if (changedBits & (RB_STATE0_COLOR_WRITE_RGB | RB_STATE0_COLOR_WRITE_ALPHA)) {
        DWORD colorWriteMask;

        colorWriteMask = (stateBits0 & RB_STATE0_COLOR_WRITE_RGB) ? 7 : 0;
        if (stateBits0 & RB_STATE0_COLOR_WRITE_ALPHA) {
            colorWriteMask |= 8;
        }

        RB_SetRenderStateDx7(D3DRS_COLORWRITEENABLE, colorWriteMask);
    }

    if (changedBits & RB_STATE0_FOG_ENABLE) {
        RB_SetRenderStateDx7(D3DRS_FOGENABLE, ((unsigned int)stateBits0 >> 29) & 1);
    }

    if (changedBits & RB_STATE0_CULL_MASK) {
        DWORD cullVal = s_cullTable[((unsigned int)stateBits0 >> 14) & 3];
#ifdef GFX_REAL_D3D9

        if (getenv("REALD3D9_NOCULL"))
            cullVal = 1;
        else if (getenv("REALD3D9_FLIPCULL")) {
            if (cullVal == 2)
                cullVal = 3;
            else if (cullVal == 3)
                cullVal = 2;
        }
#endif
        RB_SetRenderStateDx7(D3DRS_CULLMODE, cullVal);
    }

    if (changedBits & RB_STATE0_WIREFRAME) {
        RB_SetRenderStateDx7(D3DRS_FILLMODE, (stateBits0 & RB_STATE0_WIREFRAME) ? 2 : 3);
    }

    if ((stateBits0 & RB_STATE0_BLEND_OP_RGB_MASK) == 0) {
        if ((dxState.refStateBits[0] & RB_STATE0_BLEND_OP_RGB_MASK) != 0) {
            RB_SetRenderStateDx7(D3DRS_ALPHABLENDENABLE, 0);
        }

        stateBits0 = (stateBits0 & ~RB_STATE0_RGB_BLEND_BITS) | (dxState.activeStateBits[0] & RB_STATE0_RGB_BLEND_BITS);
        changedBits &= ~RB_STATE0_RGB_BLEND_BITS;
    } else {
        if ((dxState.refStateBits[0] & RB_STATE0_BLEND_OP_RGB_MASK) == 0) {
            RB_SetRenderStateDx7(D3DRS_ALPHABLENDENABLE, 1);
        }

        if (changedBits & RB_STATE0_BLEND_OP_RGB_MASK) {
            RB_SetRenderStateDx7(D3DRS_BLENDOP, s_blendOpTable[((unsigned int)stateBits0 >> 8) & 7]);
        }
    }

    if (changedBits & RB_STATE0_SRC_BLEND_RGB_MASK) {
        RB_SetRenderStateDx7(D3DRS_SRCBLEND, s_blendTable[stateBits0 & RB_STATE0_SRC_BLEND_RGB_MASK]);
    }

    if (changedBits & RB_STATE0_DST_BLEND_RGB_MASK) {
        RB_SetRenderStateDx7(D3DRS_DESTBLEND, s_blendTable[((unsigned int)stateBits0 >> 4) & 0xf]);
    }

    if ((stateBits0 & RB_STATE0_BLEND_OP_ALPHA_MASK) == 0) {
        if ((dxState.refStateBits[0] & RB_STATE0_BLEND_OP_ALPHA_MASK) != 0) {
            RB_SetRenderStateDx7(D3DRS_SEPARATEALPHABLENDENABLE, 0);
        }

        stateBits0 = (stateBits0 & ~RB_STATE0_ALPHA_BLEND_BITS) | (dxState.activeStateBits[0] & RB_STATE0_ALPHA_BLEND_BITS);
        changedBits &= ~RB_STATE0_ALPHA_BLEND_BITS;
    } else {
        if ((dxState.refStateBits[0] & RB_STATE0_BLEND_OP_ALPHA_MASK) == 0) {
            RB_SetRenderStateDx7(D3DRS_SEPARATEALPHABLENDENABLE, 1);
        }

        if (changedBits & RB_STATE0_BLEND_OP_ALPHA_MASK) {
            RB_SetRenderStateDx7(D3DRS_BLENDOPALPHA, s_blendOpTable[((unsigned int)stateBits0 >> 24) & 7]);
        }
    }

    if (changedBits & RB_STATE0_SRC_BLEND_ALPHA_MASK) {
        RB_SetRenderStateDx7(D3DRS_SRCBLENDALPHA, s_blendTable[((unsigned int)stateBits0 >> 16) & 0xf]);
    }

    if (changedBits & RB_STATE0_DST_BLEND_ALPHA_MASK) {
        RB_SetRenderStateDx7(D3DRS_DESTBLENDALPHA, s_blendTable[((unsigned int)stateBits0 >> 20) & 0xf]);
    }

    if (changedBits & RB_STATE0_NORMALIZE_NORMALS) {
        RB_SetRenderStateDx7(D3DRS_NORMALIZENORMALS, ((unsigned int)stateBits0 >> 30) & 1);
    }

    if (RB_SupportsAlphaToCoverage()) {
        const dvar_t *aaAlpha;

        aaAlpha = *(const dvar_t **)imp_r_aaAlpha;
        if (aaAlpha->current.integer != 0 && (changedBits & RB_STATE0_ALPHA_TEST_MASK)) {
            RB_SetAlphaAntiAliasingState(stateBits0);
        }
    }

    dxState.activeStateBits[0] = stateBits0;
}

void RB_SetProjectionMatrix(const D3DMATRIX *matrix)
{
    GfxCodeMatrices *activeMatrices;

    activeMatrices = RB_GetActiveCodeMatrices();
    RB_SetPrimaryCodeMatrix(&activeMatrices->projection, matrix);
    RB_InvalidateCodeMatrix(&activeMatrices->viewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->worldViewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->normalizedWorldViewProjection);
    RB_InvalidateCodeMatrix(&activeMatrices->OGLworldViewProjection);

    if (RB_UsingDx7Renderer()) {
        RB_SetTransformDx7(D3DTS_PROJECTION, &activeMatrices->projection.matrix[0]);
    }
}

void RB_UpdateViewport(void)
{
    r_backEndGlobals_t *be = &backEnd;
    GfxViewport viewport;

    be->viewportIsDirty = 0;
    if (dxState.viewportBehavior == GFX_USE_VIEWPORT_FULL) {
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = dxState.renderTargetWidth;
        viewport.height = dxState.renderTargetHeight;
    } else {
        viewport = be->sceneViewport;
    }

    dxState.viewportIsNull = 0;

    if ((DWORD)viewport.x == dxState.viewport.X &&
        (DWORD)viewport.y == dxState.viewport.Y &&
        (DWORD)viewport.width == dxState.viewport.Width &&
        (DWORD)viewport.height == dxState.viewport.Height) {
        return;
    }

    dxState.viewport.X = viewport.x;
    dxState.viewport.Y = viewport.y;
    dxState.viewport.Width = viewport.width;
    dxState.viewport.Height = viewport.height;
    RB_SetViewportDx7(&dxState.viewport);
}

void COD2_FORCE_ALIGN_ARG_POINTER RB_SetInitialState(void)
{
    const vidConfig_t *vidConfig;
    D3DMATERIAL9 material;
    int stageIndex;

    { extern void Com_Printf(const char *, ...); extern int g_disableRendering; Com_Printf("[dr] SIS entry=%d sizeof(dxState)=%d\n", g_disableRendering, (int)sizeof(dxState)); }
    memset(&dxState, 0, sizeof(dxState));
    { extern void Com_Printf(const char *, ...); extern int g_disableRendering; Com_Printf("[dr] SIS after memset=%d\n", g_disableRendering); }
    RB_DecideDefaultSamplerState_core();
    RB_SetAnisotropy_core();
    { extern void Com_Printf(const char *, ...); extern int g_disableRendering; Com_Printf("[dr] SIS after sampler=%d\n", g_disableRendering); }

    vidConfig = (const vidConfig_t *)imp_vidConfig;
    dxState.refStateBits[0] = 0x18128812;
    dxState.refStateBits[1] = 0xe00e0007;
    dxState.activeStateBits[0] = 0x19128912;
    dxState.activeStateBits[1] = 0xe00e0007;
    dxState.stencilRefValue = 0;

    backEnd.sceneViewport.x = 0;
    backEnd.sceneViewport.y = 0;
    backEnd.sceneViewport.width = vidConfig->width;
    backEnd.sceneViewport.height = vidConfig->height;

    dxState.viewport.X = 0;
    dxState.viewport.Y = 0;
    dxState.viewport.Width = vidConfig->width;
    dxState.viewport.Height = vidConfig->height;
    dxState.viewport.MinZ = 0.0f;
    dxState.viewport.MaxZ = 1.0f;

    dxState.renderTargetId = R_RENDERTARGET_NONE;
    RB_SetRenderTarget(R_RENDERTARGET_FRAME_BUFFER);
    { extern void Com_Printf(const char *, ...); extern int g_disableRendering; Com_Printf("[dr] SIS after setrendertarget=%d\n", g_disableRendering); }
    dxState.viewportIsNull = 0;

    dxState.fog.end = 1.0f;
    dxState.fog.density = 1.0f;

    for (stageIndex = 0; stageIndex < 8; ++stageIndex) {
        dxState.refColorStageBits[stageIndex] = 0;
        dxState.refAlphaStageBits[stageIndex] = 0;
        dxState.activeColorStageBits[stageIndex] = 0;
        dxState.activeAlphaStageBits[stageIndex] = 0;
    }

    RB_SetRenderStateDx7(0x89, 0);
    RB_SetRenderStateDx7(0x93, 1);

    material.Diffuse.r = 1.0f;
    material.Diffuse.g = 1.0f;
    material.Diffuse.b = 1.0f;
    material.Diffuse.a = 1.0f;
    material.Ambient = material.Diffuse;
    material.Specular = material.Diffuse;
    material.Emissive.r = 0.0f;
    material.Emissive.g = 0.0f;
    material.Emissive.b = 0.0f;
    material.Emissive.a = 0.0f;
    material.Power = 32.0f;
    RB_SetMaterialDx7(&material);

    dxState.gridLighting = 0;
    dxState.genTexCoords[0] = 0;
    dxState.genTexCoords[1] = 0;

    for (stageIndex = 0; stageIndex < 16; ++stageIndex) {
        dxState.samplerImage[stageIndex] = NULL;
        dxState.samplerState[stageIndex] = 1;
    }

    dxState.textureColorArg = 1;
    dxState.textureFactor = 0xFFFFFFFF;
    memset(dxState.vertexShaderConsts, 0xFF, sizeof(dxState.vertexShaderConsts));
    memset(dxState.pixelShaderConsts, 0xFF, sizeof(dxState.pixelShaderConsts));
}

void RB_SetWorldMatrixForEntity(const GfxEntity *re)
{
    GfxCodeMatrices *activeMatrices;

    activeMatrices = RB_GetActiveCodeMatrices();
    MatrixSet44(activeMatrices->world.matrix[0].m, re->origin, re->axis, re->scale);
    RB_FinalizeWorldMatrixChange(activeMatrices, re->scale);
}

const GfxViewportBehavior s_viewportBehaviorForRenderTarget[12] = { 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 };

const byte s_filterTable[16] = { 0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0 };

const byte defaultSamplerStateTable[32] = { 1, 2, 6, 10, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const DxTextureOpDecode s_textureOpTable[23] = { { 1, 0 }, { 2, 1 }, { 3, 2 }, { 4, 3 }, { 5, 3 }, { 6, 3 }, { 7, 3 }, { 8, 3 }, { 9, 3 }, { 10, 3 }, { 11, 3 }, { 12, 3 }, { 13, 3 }, { 14, 3 }, { 15, 3 }, { 16, 3 }, { 18, 3 }, { 19, 3 }, { 20, 3 }, { 21, 3 }, { 24, 3 }, { 25, 7 }, { 26, 7 } };

const DWORD s_textureArgTable[10] = { 0x00000006, 0x00000001, 0x00000000, 0x00000004, 0x00000005, 0x00000002, 0x00000003, 0x00000000, 0x00000000, 0x00000000 };

const DWORD s_stencilFuncTable[8] = { 0x00000001, 0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000006, 0x00000007, 0x00000008 };

const DxStencilDecode s_stencilFuncDecode[4] = { { 17, 56 }, { 29, 189 }, { 0, 0 }, { 0, 0 } };

const DWORD s_stencilOpTable[8] = { 0x00000001, 0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000006, 0x00000007, 0x00000008 };

const DxStencilDecode s_stencilOpDecode[8] = { { 8, 55 }, { 11, 53 }, { 14, 54 }, { 20, 188 }, { 23, 186 }, { 26, 187 }, { 0, 0 }, { 0, 0 } };

const DWORD s_blendTable[13] = { 0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000006, 0x00000007, 0x00000008, 0x00000009, 0x0000000A, 0x0000000E, 0x0000000F };

const DWORD s_blendOpTable[6] = { 0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000004, 0x00000005 };

const DWORD s_cullTable[4] = { 0x00000000, 0x00000001, 0x00000003, 0x00000002 };
