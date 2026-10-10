#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <stdio.h>
#include <stdarg.h>

extern int alwaysfails;
extern dvar_t *r_rendererInUse;
extern int g_disableRendering;
extern struct DxGlobals dx;
extern struct r_global_permanent_t rgp;
extern struct r_globals_t rg;
extern refimport_t ri;
extern vidConfig_t vidConfig;
extern const char *DXGetErrorDescription9A(HRESULT hr);
extern void Material_FinishLoading(void);
extern void R_AddCmdTouchAllImages(void);
extern void R_InitImages(void);
extern void Material_Init(void);
extern int R_InitFonts(void);
extern void R_InitLightDefs(void);
extern void R_ClearFogs(void);
extern void R_InitDebug(void);
extern void *R_AllocStaticVertexBuffer(void *outBuf, int size);
extern void *R_AllocStaticIndexBuffer(void *outBuf, int size);
extern void R_FinishStaticVertexBuffer(void *buf);
extern void R_FinishStaticIndexBuffer(void *buf);
extern int rand(void);
extern void R_ShutdownRenderTargets(void);
extern void R_ShutdownStaticModelCache(void);
extern void R_FreeStaticVertexBuffer(void *buf);
extern void R_FreeStaticIndexBuffer(void *buf);
extern void WinSleep(int ms);
extern void R_ShutdownBackendData(void);
extern void R_ShutdownDebug(void);
extern void RB_SaveLightVisHistory(void);
extern void R_ShutdownLightDefs(void);
extern void R_ShutdownWorld(void);
extern void R_ShutdownFonts(void);
extern void R_ShutdownModels(void);
extern void Material_Shutdown(void);
extern void R_ShutdownImages(void);
extern void R_UnlockSkinnedCache(void);
extern void R_FlushStaticModelCache(void);
extern void R_UnregisterCmds(void);
extern void R_UnregisterDvars(void);
extern void RB_ClearAllStreamSources(void);
extern GfxDrawGroupGlueBehavior R_EndDrawGroupLoop(GfxDrawGroupType section, int viewIndex);
extern Bool Sys_IsMainThread(void);
extern void R_SyncRenderThread(void);
extern void RB_SetGammaRamp(const void *gammaTable);
extern double pow(double, double);
extern float floorf(float);
extern void R_EndDrawGroupSection(int section);
extern void R_IssueDrawGroups(void);

static vec2_t cornerTexCoords[4];
static const r_index_t quadIndices[6];

void R_FatalInitError(const char *msg);
const char *R_ErrorDescription(HRESULT hr);
static void R_CreateParticleCloudBuffer(void);
static void R_ReleaseForShutdownOrReset(void);
static Bool R_DisplayModeLess(const _D3DDISPLAYMODE *mode0, const _D3DDISPLAYMODE *mode1);
void R_UpdateGpuSyncType(void);
void R_EndRegistration(void);
static void R_EndView(int viewIndex);
static void R_DoneRenderingViews(void);
static void R_TrackStatistics(trStatistics_t *stats);
refexport_t *GetRefAPI(int apiVersion, refimport_t *rimp);
void R_Error(errorParm_t errorLevel, const char *msg, ...);
void R_GammaCorrect(byte *buffer, int bufSize);
static void R_InitSystems(void);
void R_FatalLockError(HRESULT hr);
static void R_Shutdown(qboolean destroyWindow);
void R_SetColorMappings(void);
static Bool R_CreateForInitOrReset(void);
void R_BeginRegistration(vidConfig_t *vidConfigOut);
Bool R_RecoverLostDevice(void);
typedef unsigned char (*D3DDispModeCompFunc)(const _D3DDISPLAYMODE *, const _D3DDISPLAYMODE *);
void ZSt13__adjust_heapIP15_D3DDISPLAYMODEiS0_PFhRKS0_S3_EEvT_T0_S7_T1_T2_(_D3DDISPLAYMODE *first, int holeIndex, int len, _D3DDISPLAYMODE value, D3DDispModeCompFunc comp);
void ZSt16__insertion_sortIP15_D3DDISPLAYMODEPFhRKS0_S3_EEvT_S6_T0_(_D3DDISPLAYMODE *first, _D3DDISPLAYMODE *last, D3DDispModeCompFunc comp);
void ZSt16__introsort_loopIP15_D3DDISPLAYMODEiPFhRKS0_S3_EEvT_S6_T0_T1_(_D3DDISPLAYMODE *first, _D3DDISPLAYMODE *last, int depth_limit, D3DDispModeCompFunc comp);

void R_FatalInitError(const char *msg)
{
    ((void (*)(int, const char *, ...))ri.Printf)(0, (const char *)"********** DirectX returned an unrecoverable error code during initialization  **********\n");
    ((void (*)(int, const char *, ...))ri.Printf)(0, (const char *)"********** Initialization also happens while playing if DirectX loses a device **********\n");
    ((void (*)(int, const char *, ...))ri.Printf)(0, (const char *)"********** Consult the readme for how to continue from this problem            **********\n");
    ((void (*)(int, const char *, ...))ri.Printf)(0, "\n%s\n", msg);
    ((void (*)(void))ri.Sys_DirectXFatalError)();
}

const char *R_ErrorDescription(HRESULT hr)
{
    return DXGetErrorDescription9A(hr);
}

static void R_CreateParticleCloudBuffer(void)
{
    float *verts;
    unsigned short *indices;
    int xIter, yIter, zIter, corner;
    int vertexIndex = 0;

    verts = R_AllocStaticVertexBuffer(&dx.particleCloudVertexBuffer, 0x14000);
    indices = R_AllocStaticIndexBuffer(&dx.particleCloudIndexBuffer, 0x3000);

    for (xIter = 0; xIter < 8; xIter++) {
        float xBase = (float)xIter;
        int baseVertForX = xIter * 128;

        for (yIter = 0; yIter < 8; yIter++) {
            float yBase = (float)yIter;

            for (zIter = 0; zIter < 16; zIter++) {

                float px = ((float)rand() * 4.656612873077393e-10f + xBase) * 0.25f - 1.0f;
                float py = ((float)rand() * 4.656612873077393e-10f + yBase) * 0.25f - 1.0f;
                float pz = ((float)rand() * 4.656612873077393e-10f + (float)zIter) * 0.125f - 1.0f;

                for (corner = 0; corner < 4; corner++) {

                    *verts++ = px;
                    *verts++ = py;
                    *verts++ = pz;

                    *verts++ = cornerTexCoords[corner][0];
                    *verts++ = cornerTexCoords[corner][1];
                }

                {
                    int baseVert = (baseVertForX + zIter) * 4;
                    int qi;
                    for (qi = 0; qi < 6; qi++) {
                        *indices++ = (unsigned short)(baseVert + quadIndices[qi]);
                    }
                }
            }
            baseVertForX += 16;
        }
    }

    R_FinishStaticIndexBuffer(dx.particleCloudIndexBuffer);
    R_FinishStaticVertexBuffer(dx.particleCloudVertexBuffer);
}

static inline void R_SafeRelease(void **objPtr)
{
    if (*objPtr) {
        do {
            void *obj = *objPtr;
            ((void (*)(void *))((*(void ***)obj)[2]))(obj);
            *objPtr = NULL;
        } while (*(int *)&alwaysfails);
    }
}

static void R_ReleaseForShutdownOrReset(void)
{
    byte *d = (byte *)&dx;
    int i;

    {
        int scCount = *(int *)(d + 11592);
        for (i = 0; i < scCount; i++) {
            void **psc = (void **)(d + 0x2d50 + i * 16);
            R_SafeRelease(psc);
        }
    }

    R_ShutdownRenderTargets();
    R_ShutdownStaticModelCache();

    R_SafeRelease((void **)(d + 11656));

    R_SafeRelease((void **)(d + 11696));

    for (i = 0; i < 2; i++) {
        R_SafeRelease((void **)(d + 0x2d98 + i * 12));
    }

    if (*(void **)(d + 11728)) {

        ((void (*)(void *))ri.Z_VirtualFreeInternal)(*(void **)(d + 11728));
        *(void **)(d + 11728) = NULL;
        *(int *)(d + 11732) = 0;
    }

    if (*(void **)(d + 11704)) {
        R_FreeStaticVertexBuffer(*(void **)(d + 11704));
        *(void **)(d + 11704) = NULL;
    }
    if (*(void **)(d + 11708)) {
        R_FreeStaticIndexBuffer(*(void **)(d + 11708));
        *(void **)(d + 11708) = NULL;
    }

    R_SafeRelease((void **)(d + 11612));

    {
        byte *sunFlare = (byte *)imp_sunFlareArray;
        int view;
        for (view = 0; view < 4; view++) {
            byte *viewBase = sunFlare + view * 0x30;
            int tex;
            for (tex = 0; tex < 2; tex++) {
                R_SafeRelease((void **)(viewBase + 0x24 + tex * 4));
            }
        }
    }
}

static Bool R_DisplayModeLess(const _D3DDISPLAYMODE *mode0, const _D3DDISPLAYMODE *mode1)
{
    if (mode0->Width > mode1->Width)
        return 0;
    if (mode0->Width < mode1->Width)
        return 1;
    if (mode0->Height > mode1->Height)
        return 0;
    if (mode0->Height < mode1->Height)
        return 1;
    return mode0->RefreshRate < mode1->RefreshRate;
}

static HRESULT R_CreateDevice_impl(HWND hwnd, DWORD behavior, void *d3dpp)
{
    /* Typed field access: the original used (byte*)&dx + 12 (adapterIndex) and
     * + 8 (device-out), which are x86 offsets -- on x64 dx.device is at 16 and
     * adapterIndex at 24 (the leading pointers grew), so the device wrote over
     * dx.d3d9 and dx.device stayed NULL -> texture creation crashed. */
    HRESULT hr;
    int attempt;

    for (;;) {

        ri.Printf(0, "Creating D3D device...\n");

        for (attempt = 0; attempt < 20; attempt++) {

            void *d3d9 = (void *)dx.d3d9;
            void **vtable = *(void ***)d3d9;
            hr = ((HRESULT(__attribute__((stdcall)) *)(void *, int, int, HWND, DWORD, void *, void **))
                      vtable[0x40 / 4])(d3d9, dx.adapterIndex, 1, hwnd, behavior, d3dpp, (void **)&dx.device);

            if (hr >= 0)
                return hr;

#ifdef __EMSCRIPTEN__
            return hr;
#else
            WinSleep(100);
#endif
        }

        if (dx.adapterIndex == 0)
            return hr;

        dx.adapterIndex = 0;
    }
}

static HRESULT R_CreateDevice(HWND hwnd, DWORD behavior, void *d3dpp)
{
    return R_CreateDevice_impl(hwnd, behavior, d3dpp);
}

void R_UpdateGpuSyncType(void)
{
    int v;
    if (!*(byte *)(*(int *)imp_r_multiGpu + 8))
        v = *(int *)(*(int *)imp_r_gpuSync + 8);
    else
        v = 0;
    dx.gpuSync = v;
}

void R_EndRegistration(void)
{
    Material_FinishLoading();
    R_AddCmdTouchAllImages();
}

static void R_EndView(int viewIndex)
{
    R_EndDrawGroupLoop(4, viewIndex);
    R_EndDrawGroupSection(4);
}

static void R_DoneRenderingViews(void)
{
    R_IssueDrawGroups();
}

static void R_TrackStatistics(trStatistics_t *stats)
{
    rg.stats = stats;
}

refexport_t *GetRefAPI(int apiVersion, refimport_t *rimp)
{
    static refexport_t re;
    typedef void (*ri_Printf_fn)(int, const char *, ...);

    if (apiVersion == 60) {
        extern refexport_t *GetRefAPI_v60_adapt(refimport_t *);
        return GetRefAPI_v60_adapt(rimp);
    }

    memcpy(&ri, rimp, sizeof(ri));
    memset(&re, 0, sizeof(re));

    if (apiVersion != 59) {
        ((ri_Printf_fn)(*(void **)&ri))(0, "Mismatched REF_API_VERSION: expected %i, got %i\n", 59, apiVersion);
        return NULL;
    }

    byte *r = (byte *)&re;
/* The offsets below are x86 byte offsets into refexport_t (4-byte pointer slots).
 * On x64 every slot is 8 bytes, so slot N sits at off*2. refexport_t is a uniform
 * array of pointer-sized slots (its lone bool member is slot-padded), so doubling
 * the x86 offset yields the correct x64 offset. Without this, re is built with
 * every function pointer at half its real offset -> garbage (re.BeginRegistration
 * faulted at CL_InitRenderer). (x64 port Stage 4.) */
#if defined(_M_X64) || defined(__x86_64__)
#define RE(off, fn) *(void **)(r + (off) * 2) = (void *)(fn)
#else
#define RE(off, fn) *(void **)(r + (off)) = (void *)(fn)
#endif

    RE(0, R_Shutdown);
    RE(4, R_BeginRegistration);
    RE(8, imp_R_RegisterModel);
    RE(12, imp_R_RegisterInlineModel);
    RE(48, imp_R_GetMinSpecImageMemory);
    RE(16, imp_Material_RegisterHandle);
    RE(20, imp_R_RegisterRawImage);
    RE(24, imp_Material_IsDefault);
    RE(28, imp_R_LoadWorld);
    RE(32, imp_R_GetWorldBounds);
    RE(36, imp_R_FinishLoadingModels);
    RE(40, imp_R_SetIgnorePrecacheErrors);
    RE(44, imp_R_GetIgnorePrecacheErrors);
    RE(68, R_EndRegistration);
    RE(52, imp_R_GetMaterialName);
    RE(56, imp_R_GetMaterialSubimageCount);
    RE(60, imp_R_IsMaterialRefractive);
    RE(64, imp_R_GetFarPlaneDist);
    RE(168, imp_R_BeginFrame);
    RE(172, imp_R_EndFrame);
    RE(176, imp_R_BeginDebugFrame);
    RE(180, imp_R_EndDebugFrame);
    RE(184, R_EndView);
    RE(188, R_DoneRenderingViews);
    RE(192, imp_R_AddCmdSaveScreen);
    RE(196, imp_R_AddCmdBlendSavedScreen);
    RE(200, imp_R_AddCmdClearScreen);
    RE(204, imp_R_AddCmdSetViewport);
    RE(208, imp_R_MarkFragments);
    RE(212, imp_R_ModelBounds);
    RE(72, imp_R_ClearScene);
    RE(76, imp_R_DefaultVertexFrames);
    RE(80, imp_R_AddPolyToScene);
    RE(84, imp_R_AddLightToScene);
    RE(88, imp_R_InterpretSunLightParseParams);
    RE(92, imp_R_ResetSunLightParseParams);
    RE(96, imp_R_SetCullDist);
    RE(100, imp_R_SetFog);
    RE(104, imp_R_SwitchFog);
    RE(108, imp_R_ArchiveFogState);
    RE(112, imp_R_ClearFogs);
    RE(116, imp_R_SetSunLightOverride);
    RE(120, imp_R_ResetSunLightOverride);
    RE(124, imp_R_RenderScene);
    RE(128, imp_R_BeginDelayedDrawing);
    RE(132, imp_R_EndDelayedDrawing);
    RE(136, imp_R_IssueDelayedDrawing);
    RE(140, imp_R_ClearFlares);
    RE(148, imp_R_AddCmdDrawStretchPic);
    RE(152, imp_R_AddCmdDrawStretchPicRotate);
    RE(156, imp_R_AddCmdDrawStretchRaw);
    RE(160, imp_R_AddCmdDrawQuadPic);
    RE(164, imp_R_AddCmdDrawSprite);
    RE(144, imp_R_AddCmdSetMaterialColor);
    RE(224, imp_R_RegisterFont);
    RE(228, imp_R_ResetImageAllocations);
    RE(232, imp_R_FreeImageAllocations);
    RE(236, imp_R_BeginCubemapShot);
    RE(240, imp_R_EndCubemapShot);
    RE(244, imp_R_SaveCubemapShot);
    RE(248, imp_R_LightingFromCubemapShots);
    RE(252, imp_R_LocateDebugStrings);
    RE(256, imp_R_LocateDebugLines);
    RE(260, imp_R_AddPlume);
    RE(264, imp_R_ShutdownDebug);
    RE(216, R_TrackStatistics);
    RE(220, imp_R_PickMaterial);
    RE(268, imp_RB_UpdateColor);
    RE(272, imp_R_NormalizedTextScale);
    RE(276, imp_R_TextWidth);
    RE(280, imp_R_TextHeight);
    RE(284, imp_R_DrawText);
    RE(288, imp_R_AddCmdDrawTextInSpace);
    RE(292, imp_R_ConsoleTextWidth);
    RE(296, imp_R_DrawConsoleText);
    RE(300, imp_R_AddCmdDrawTextWithCursor);
    RE(304, imp_R_DObjGetSurfMaterials);
    RE(308, imp_R_DObjReplaceMaterial);
    RE(312, imp_R_ParseSunLight);
    RE(316, imp_Material_Duplicate);
    RE(320, imp_R_DuplicateFont);
    *(byte *)(r + 324) = 1;
    RE(328, imp_R_SyncRenderThread);
    RE(332, imp_R_AbortRenderCommands);
    RE(336, imp_RB_IsGpuFenceFinished);
    RE(340, imp_RB_AdaptiveGpuSyncWait);
    RE(344, imp_RB_GpuWaited);
    RE(348, imp_R_SetLodOrigin);

#undef RE

    return &re;
}

void R_Error(errorParm_t errorLevel, const char *msg, ...)
{
    char text[1024];
    va_list vargs;

    if (Sys_IsMainThread())
        R_SyncRenderThread();

    if (dx.inScene) {
        void *dev = (void *)dx.device;
        void **vt = *(void ***)dev;
        ((int (*)(void *))vt[0xa8 / 4])(dev);
        dx.inScene = 0;
    }

    va_start(vargs, msg);
    vsnprintf(text, 0x3ff, msg, vargs);
    va_end(vargs);
    text[1023] = '\0';

#ifdef GFX_REAL_D3D9
    fprintf(stderr, "[R_ERROR] level=%d caller=%p msg='%s'\n",
            errorLevel, __builtin_return_address(0), text);
    fflush(stderr);
#endif
    ((void (*)(int, const char *, ...))ri.Error)(errorLevel, "%s", text);
}

void R_GammaCorrect(byte *buffer, int bufSize)
{
    unsigned short gammaTable[256];
    float invGamma;
    int i;

    invGamma = 1.0f / *(float *)(*(int *)imp_r_gamma + 8);

    if (invGamma == 1.0f) {

        for (i = 0; i < 256; i++)
            gammaTable[i] = (unsigned short)(i + (i << 8));
    } else {

        for (i = 0; i < 256; i++)
            gammaTable[i] = (unsigned short)(int)floorf((float)pow((double)(i / 255.0f), (double)invGamma) * 65535.0f + 0.5f);
    }

    for (i = 0; i < bufSize; i++)
        buffer[i] = (byte)(gammaTable[buffer[i]] / 257);
}

static void R_InitSystems(void)
{
    R_InitImages();
    Material_Init();
    R_InitFonts();
    rg.sceneWaterMapSetupsCount = 0;
    R_InitLightDefs();
    R_ClearFogs();
    R_InitDebug();
    *(char *)&rg = 1;
}

void R_FatalLockError(HRESULT hr)
{
    ((void (*)(int, const char *, ...))ri.Printf)(0, (const char *)"********** DirectX failed a call to lock a vertex buffer or an index buffer **********\n");
    ((void (*)(int, const char *, ...))ri.Printf)(0, "********** error information:  %s\n", DXGetErrorDescription9A(hr));
    ((void (*)(void))ri.Sys_DirectXFatalError)();
}

static void R_Shutdown(qboolean destroyWindow)
{
    byte *d = (byte *)&dx;

#ifdef GFX_REAL_D3D9
    if (getenv("REALD3D9_MATDIAG")) {
        fprintf(stderr, "[R_SHUTDOWN] destroyWindow=%d caller0=%p caller1=%p caller2=%p\n",
                destroyWindow, __builtin_return_address(0),
                __builtin_return_address(1), __builtin_return_address(2));
        fflush(stderr);
    }
#endif

    *(byte *)&rg = 0;

    if (dx.device) {
        if (!dx.deviceLost)
            RB_ClearAllStreamSources();
    }

    R_ShutdownBackendData();
    R_ShutdownDebug();
    RB_SaveLightVisHistory();
    R_ShutdownLightDefs();
    R_ShutdownWorld();
    R_ShutdownFonts();
    R_ShutdownModels();
    Material_Shutdown();
    R_ShutdownImages();

    *(int *)((byte *)&rgp + 4252) = 0;
    R_UnlockSkinnedCache();
    R_FlushStaticModelCache();

    if (destroyWindow) {
        R_ReleaseForShutdownOrReset();

        while (*(int *)(d + 11592) != 0) {
            int idx = *(int *)(d + 11592) - 1;
            *(int *)(d + 11592) = idx;
            *(void **)(d + 11584 + idx * 16 + 12) = NULL;
        }

        R_SafeRelease((void **)(d + 8));

        R_SafeRelease((void **)(d + 4));

        R_UnregisterDvars();
    }

    R_UnregisterCmds();
}

void R_SetColorMappings(void)
{
    unsigned short gammaRamp[256];
    float invGamma;
    int i;

    if (!vidConfig.deviceSupportsGamma)
        return;

    invGamma = 1.0f / *(float *)(*(int *)imp_r_gamma + 8);

    if (invGamma == 1.0f) {
        for (i = 0; i < 256; i++)
            gammaRamp[i] = (unsigned short)(i + (i << 8));
    } else {
        for (i = 0; i < 256; i++)
            gammaRamp[i] = (unsigned short)(int)floorf((float)pow((double)(i / 255.0f), (double)invGamma) * 65535.0f + 0.5f);
    }

    RB_SetGammaRamp(gammaRamp);
}

extern void R_InitRenderTargets(void);
extern void R_InitStaticModelCache(void);
extern void RB_SetInitialState(void);
extern const char *va(const char *fmt, ...);

static void R_DxFatalError(const char *fmt, int size, HRESULT hr)
{
    typedef void (*ri_fn)(int, const char *, ...);
    typedef void (*ri_err_fn)(void);
    ri_fn Printf = (ri_fn)ri.Printf;
    ri_err_fn Error = (ri_err_fn)ri.Sys_DirectXFatalError;
    const char *msg = va(fmt, size, DXGetErrorDescription9A(hr));
    Printf(0, "------- Initializing Renderer -------");
    Printf(0, "------- Renderer Initialization -------");
    Printf(0, "------- Server Initialization -------");
    Printf(0, "\n%s\n", msg);
    Error();
}

static Bool R_CreateForInitOrReset(void)
{
    typedef void (*ri_fn)(int, const char *, ...);
    byte *dxp = (byte *)(void *)&dx;
    int i;

    ((ri_fn)ri.Printf)(0, "R_InitRenderTargets");
    R_InitRenderTargets();
    ((ri_fn)ri.Printf)(0, "R_InitStaticModelCache");
    R_InitStaticModelCache();
    { extern void Com_Printf(const char *, ...); Com_Printf("[dr] after staticmodelcache=%d\n", g_disableRendering); }
    ((ri_fn)ri.Printf)(0, "Dynamic buffers");

    /* Dynamic buffer pools: were hardcoded x86 DxGlobals byte offsets (dxp+116xx);
     * r_ib_state_t/r_vb_state_t are 16B on x64 (8B buffer ptr) so use typed fields. */
    int isDx7 = (r_rendererInUse->current.integer == 2);
    int vbSize = isDx7 ? 0x120000 : 0x200000;
    void *d3dDevice;
    void **vtable;
    HRESULT hr;
    typedef HRESULT(D3DVTCC * CreateVB_fn)(void *, int, int, int, int, void **, void *);
    typedef HRESULT(D3DVTCC * CreateIB_fn)(void *, int, int, int, int, void **, void *);

    dx.dynamicVertexBufferPool[0].used = 0;
    dx.dynamicVertexBufferPool[0].total = vbSize;
    d3dDevice = (void *)dx.device;
    vtable = *(void ***)d3dDevice;
    hr = ((CreateVB_fn)vtable[26])(d3dDevice, vbSize, 0x208, 0, 0, (void **)&dx.dynamicVertexBufferPool[0].buffer, NULL);
    if (hr < 0)
        R_DxFatalError("Couldn't create a %i-byte dynamic vertex buffer: %s", vbSize, hr);
    dx.dynamicVertexBuffer = &dx.dynamicVertexBufferPool[0];

    for (i = 0; i < 2; i++) {
        int loopVbSize = (r_rendererInUse->current.integer == 2) ? 0x480000 : 0x800000;
        dx.skinnedCacheVbPool[i].used = 0;
        dx.skinnedCacheVbPool[i].total = loopVbSize;
        d3dDevice = (void *)dx.device;
        vtable = *(void ***)d3dDevice;
        hr = ((CreateVB_fn)vtable[26])(d3dDevice, loopVbSize, 0x208, 0, 0, (void **)&dx.skinnedCacheVbPool[i].buffer, NULL);
        if (hr < 0)
            R_DxFatalError("Couldn't create a %i-byte dynamic vertex buffer: %s", loopVbSize, hr);
    }

    dx.dynamicIndexBufferPool[0].used = 0;
    dx.dynamicIndexBufferPool[0].total = 0x200000;
    d3dDevice = (void *)dx.device;
    vtable = *(void ***)d3dDevice;
    hr = ((CreateIB_fn)vtable[27])(d3dDevice, 0x200000, 0x208, 0x65, 0, (void **)&dx.dynamicIndexBufferPool[0].buffer, NULL);
    if (hr < 0)
        R_DxFatalError("Couldn't create a %i-byte dynamic index buffer: %s", 0x200000, hr);
    dx.dynamicIndexBuffer = &dx.dynamicIndexBufferPool[0];

    dx.tempSkinBuf = (byte *)((void *(*)(int))ri.Z_VirtualReserveInternal)(0xa00000);

    ((ri_fn)ri.Printf)(0, "Particle cloud");
    R_CreateParticleCloudBuffer();
    ((ri_fn)ri.Printf)(0, "State");

    dx.tempSkinPos = 0;
    dx.dynamicBufferFrame = 0;

    byte *sunFlares = (byte *)imp_sunFlareArray;
    for (i = 0; i < 4; i++) {
        sunFlares[i * 0x30 + 0x2c] = 0;
        sunFlares[i * 0x30 + 0x2d] = 0;
    }

    { extern void Com_Printf(const char *, ...); Com_Printf("[dr] after stateblock=%d\n", g_disableRendering); }
    ((ri_fn)ri.Printf)(0, "Initial state");
    RB_SetInitialState();
    { extern void Com_Printf(const char *, ...); Com_Printf("[dr] after setinitialstate=%d\n", g_disableRendering); }
    return 1;
}

extern void Swap_Init(void);
extern void R_RegisterDvars(void);
extern void R_RegisterCmds(void);
extern void R_InitBackendData(void);
extern void R_InitDrawGroups(void);
extern void R_InitSystems(void);
extern void FFT_Init(void *sinTable, void *workspace);
extern void *Direct3DCreate9(int sdkVersion);  /* returns a pointer; `int` truncated it on x64 */
extern double sin(double);
static void R_BeginRegistration_impl(vidConfig_t *vidConfigOut)
{
    typedef void (*PrintFunc)(int, const char *, ...);
    PrintFunc ri_printf = (PrintFunc)ri.Printf;

#ifdef GFX_REAL_D3D9
    if (getenv("REALD3D9_MATDIAG")) {
        extern struct DxGlobals dx;
        fprintf(stderr, "[R_BEGINREG] called, dx.device=%p caller=%p\n",
                (void *)dx.device, __builtin_return_address(0));
        fflush(stderr);
    }
#endif

    if (!ri_printf)
        return;
    ri_printf(0, "----- R_Init -----\n");
    Swap_Init();
    R_RegisterDvars();
#ifdef __EMSCRIPTEN__
    /* The browser bridge implements the engine's fixed-function DX7 path.
     * Select it before sampler, image and material initialization. */
    ri.Dvar_SetInt(r_rendererInUse, 2);
#endif
    R_RegisterCmds();
    memset(&rg, 0, sizeof(r_globals_t));
    memset(&rgp, 0, sizeof(r_global_permanent_t));
    {
        extern void RB_InitBackendGlobalStructs(void);
        extern void RB_DecideDefaultSamplerState(void);
        RB_InitBackendGlobalStructs();
        RB_DecideDefaultSamplerState();
    }
    R_InitBackendData();
    R_InitDrawGroups();
    { extern void Com_Printf(const char *, ...); Com_Printf("[disrend] after R_InitDrawGroups = %d\n", g_disableRendering); }

    if (dx.device) {

        R_InitSystems();
    } else {
#ifdef GFX_REAL_D3D9

        {
            extern void *RealD3D9_Direct3DCreate9(void);
            extern void *RealD3D9_CreateGameWindow(int, int);
            extern void RealD3D9_FillPresentParams(void *, void *, int, int);
            int width = 640, height = 480;
            void *hwnd;
            unsigned char pp[56];
            HRESULT hr;

            ri_printf(0, "Initializing REAL Direct3D9 (GFX_REAL_D3D9)\n");
            if (!dx.d3d9)
                dx.d3d9 = (IDirect3D9 *)RealD3D9_Direct3DCreate9();
            hwnd = RealD3D9_CreateGameWindow(width, height);
            RealD3D9_FillPresentParams(pp, hwnd, width, height);
            hr = R_CreateDevice_impl((HWND)hwnd, 0x40, pp);
            ri_printf(0, "REAL CreateDevice hr=0x%x device=%p\n", (unsigned)hr, (void *)dx.device);

            vidConfig.width = width;
            vidConfig.height = height;
            vidConfig.displayFrequency = 60;
            vidConfig.isFullscreen = 0;
            vidConfig.aspectRatioWindow = (float)width / (float)height;
            vidConfig.aspectRatioPixel = (float)height * vidConfig.aspectRatioWindow / (float)width;
        }
#else

        if (!dx.d3d9) {
            ri_printf(0, "Initializing Direct3D\n");
            dx.d3d9 = (IDirect3D9 *)Direct3DCreate9(0x20);
            if (!dx.d3d9) {
                ri_printf(0, "Direct3DCreate9 failed\n");
            }
        }

        {
            int d3dpp[14];
            int width = 640, height = 480;
#ifdef __EMSCRIPTEN__
            // Honor the browser's selected mode throughout window creation,
            // projection, HUD and render-target allocation.
            extern const dvar_t *r_mode;
            static const int modes[][2] = {
                {640,480}, {800,600}, {1024,768},
                {1280,720}, {1600,900}, {1920,1080}
            };
            int mode = r_mode->current.integer;
            if (mode >= 0 && mode < (int)(sizeof(modes) / sizeof(modes[0]))) {
                width = modes[mode][0]; height = modes[mode][1];
            }
#endif

            memset(d3dpp, 0, sizeof(d3dpp));
            d3dpp[0] = width;
            d3dpp[1] = height;
            d3dpp[2] = 0x15;
            d3dpp[3] = 1;
            d3dpp[6] = 1;
            d3dpp[8] = 0x4b;
            d3dpp[11] = 1;

            HRESULT hr = R_CreateDevice_impl(0, 0x40, d3dpp);
            if (hr < 0) {
                R_FatalInitError("Could not create the browser graphics context");
                return;
            }

            vidConfig.width = width;
            vidConfig.height = height;
            vidConfig.displayFrequency = 60;
            vidConfig.isFullscreen = 0;
            vidConfig.aspectRatioWindow = (float)width / (float)height;
            vidConfig.aspectRatioPixel = (float)height * vidConfig.aspectRatioWindow / (float)width;

#ifdef __EMSCRIPTEN__
            extern float CDirect3DDevice_WebMaxAnisotropy(void);
            dx.maxAnisotropy = (int)CDirect3DDevice_WebMaxAnisotropy();
            // The browser DX7 bridge implements color plus lightmap sampling.
            vidConfig.maxTextureMaps = 2;
#endif
        }
#endif

        R_InitSystems();
    }

    if (!R_CreateForInitOrReset()) {
        ri_printf(0, "R_CreateForInitOrReset failed\n");
    }

    {
        extern void RB_RegisterBackendAssets(void);
        RB_RegisterBackendAssets();
    }

    {
        int i;
        float *sinTable = rg.sinTable;
        for (i = 0; i < 1024; i++) {
            double angle = (double)i * 0.3515625 * 0.017453292519943295;
            sinTable[i] = (float)sin(angle);
        }
    }

    FFT_Init(rg.fftBitswap, rg.fftTrigTable);

    {
        int args[3] = { 0, 0, 0 };
        ((void (*)(void *, int, void *, void *, int))ri.DObjCreate)(
            args, 1, NULL, rg.modelDObjBuf, 0);
        rg.modelDObj = (struct DObj_s *)rg.modelDObjBuf;
    }

    {
        extern int RB_CalcSunSpriteSamples(void);
        dx.sunSpriteSamples = RB_CalcSunSpriteSamples();
    }

    memcpy(vidConfigOut, imp_vidConfig, 44);
    { extern void Com_Printf(const char *, ...); Com_Printf("[disrend] R_Init end = %d\n", g_disableRendering); }
}

void R_BeginRegistration(vidConfig_t *vidConfigOut)
{
    R_BeginRegistration_impl(vidConfigOut);

    *(char *)&rg = 1;
}

extern void R_ReleaseAllModels(void);
extern void R_ReleaseLostImages(void);
extern void Material_ReleaseAll(void);
extern void R_ReleaseWorld(void);
extern void R_ReleaseForShutdownOrReset(void);
extern void R_ReloadWorld(void);
extern void Material_ReloadAll(void);
extern void R_ReloadLostImages(void);
extern void R_OptimizeAllModels(void);
extern int RB_CalcSunSpriteSamples(void);
extern r_global_permanent_t rgp;

Bool R_RecoverLostDevice(void)
{
    byte *dxp = (byte *)(void *)&dx;
    typedef void (*ri_fn)(int, const char *, ...);
    typedef void (*ri_err_fn)(void);
    ri_fn Printf = (ri_fn)ri.Printf;
    ri_err_fn Error = (ri_err_fn)ri.Sys_DirectXFatalError;

    void *d3dDevice = (void *)dx.device;
    void **vtable = *(void ***)d3dDevice;
    typedef HRESULT(D3DVTCC * TestCoopLevel_fn)(void *);
    HRESULT hr = ((TestCoopLevel_fn)vtable[3])(d3dDevice);

    if (hr == (HRESULT)0x88760868)
        return 0;

    Printf(0, "Recovering lost device");

    R_ReleaseAllModels();
    R_ReleaseLostImages();
    Material_ReleaseAll();

    if (rgp.world)
        R_ReleaseWorld();

    int displayMode = *(int *)(dxp + 11596);
    int backBufWidth = *(int *)(dxp + 11604);
    int backBufHeight = *(int *)(dxp + 11608);
    int refreshRate = vidConfig.displayFrequency;
    int isFullscreen = vidConfig.isFullscreen != 0;
    int aaSamples = *(int *)(*(int *)imp_r_aaSamples + 8);

    if (aaSamples > 1) {
        void *d3d = (void *)dx.d3d9;
        void **d3dVtable = *(void ***)d3d;
        typedef HRESULT(D3DVTCC * CheckMultiSample_fn)(void *, int, int, int, int, int, void *);
        int qualityLevels;
        int testSamples = aaSamples;
        while (testSamples > 1) {
            dx.multiSampleType = testSamples;
            hr = ((CheckMultiSample_fn)d3dVtable[11])(d3d, 0, 1, 0x15, !isFullscreen, testSamples, &qualityLevels);
            if (hr >= 0) {
                dx.multiSampleQuality = qualityLevels - 1;
                goto aa_done;
            }
            testSamples--;
        }
        dx.multiSampleType = 0;
        dx.multiSampleQuality = 0;
    }

aa_done:;

    int d3dpp[14];
    memset(d3dpp, 0, sizeof(d3dpp));
    d3dpp[0] = backBufWidth;
    d3dpp[1] = backBufHeight;
    d3dpp[2] = 0x15;
    d3dpp[3] = 1;
    d3dpp[4] = dx.multiSampleType;
    d3dpp[5] = dx.multiSampleQuality;
    d3dpp[6] = 1;
    d3dpp[8] = 0x4b;

    int swapInterval = *(byte *)(*(int *)imp_r_swapInterval + 8);
    d3dpp[11] = swapInterval ? 1 : 0x80000000;
    d3dpp[10] = displayMode;
    d3dpp[9] = 0;
    if (isFullscreen) {
        d3dpp[7] = 0;
        d3dpp[12] = refreshRate;
    } else {
        d3dpp[7] = 1;
        d3dpp[12] = 0;
    }

    R_ReleaseForShutdownOrReset();

    d3dDevice = (void *)dx.device;
    vtable = *(void ***)d3dDevice;
    typedef HRESULT(D3DVTCC * Reset_fn)(void *, void *);
    hr = ((Reset_fn)vtable[16])(d3dDevice, d3dpp);

    if (hr < 0) {
        const char *msg = va("Couldn't reset a lost Direct3D device - IDirect3DDevice9::Reset returned %s", DXGetErrorDescription9A(hr));
        Printf(0, "------- Initializing Renderer -------");
        Printf(0, "------- Renderer Initialization -------");
        Printf(0, "------- Server Initialization -------");
        Printf(0, "\n%s\n", msg);
        Error();
    }

    dx.deviceLost = 0;

    if (!R_CreateForInitOrReset()) {
        Printf(0, "------- Initializing Renderer -------");
        Printf(0, "------- Renderer Initialization -------");
        Printf(0, "------- Server Initialization -------");
        Printf(0, "\nFailed to recreate resources after device reset\n");
        Error();
    }

    if (rgp.world)
        R_ReloadWorld();

    Material_ReloadAll();
    R_ReloadLostImages();
    R_OptimizeAllModels();
    dx.sunSpriteSamples = RB_CalcSunSpriteSamples();

    Printf(0, "Device recovered successfully");
    return 1;
}

void ZSt13__adjust_heapIP15_D3DDISPLAYMODEiS0_PFhRKS0_S3_EEvT_T0_S7_T1_T2_(
    _D3DDISPLAYMODE *first, int holeIndex, int len, _D3DDISPLAYMODE value, D3DDispModeCompFunc comp)
{
    int topIndex = holeIndex;
    int secondChild = 2 * holeIndex + 2;
    while (secondChild < len) {
        if (comp(&first[secondChild], &first[secondChild - 1]))
            secondChild--;
        first[holeIndex] = first[secondChild];
        holeIndex = secondChild;
        secondChild = 2 * secondChild + 2;
    }
    if (secondChild == len) {
        first[holeIndex] = first[len - 1];
        holeIndex = len - 1;
    }
    while (holeIndex > topIndex) {
        int parent = (holeIndex - 1) / 2;
        if (!comp(&first[parent], &value))
            break;
        first[holeIndex] = first[parent];
        holeIndex = parent;
    }
    first[holeIndex] = value;
}

void ZSt16__insertion_sortIP15_D3DDISPLAYMODEPFhRKS0_S3_EEvT_S6_T0_(
    _D3DDISPLAYMODE *first, _D3DDISPLAYMODE *last, D3DDispModeCompFunc comp)
{
    _D3DDISPLAYMODE *i;
    if (first == last)
        return;
    for (i = first + 1; i != last; i++) {
        _D3DDISPLAYMODE val = *i;
        if (comp(&val, first)) {
            int n = (int)(i - first);
            int k;
            for (k = 0; k < n; k++)
                i[-k] = i[-k - 1];
            *first = val;
        } else {
            _D3DDISPLAYMODE *prev = i - 1;
            _D3DDISPLAYMODE *hole = i;
            while (comp(&val, prev)) {
                *hole = *prev;
                hole = prev;
                prev--;
            }
            *hole = val;
        }
    }
}

void ZSt16__introsort_loopIP15_D3DDISPLAYMODEiPFhRKS0_S3_EEvT_S6_T0_T1_(
    _D3DDISPLAYMODE *first, _D3DDISPLAYMODE *last, int depth_limit, D3DDispModeCompFunc comp)
{
    while (last - first > 16) {
        if (depth_limit == 0) {
            int n = (int)(last - first);
            int half = (n - 2) / 2;
            int i;
            _D3DDISPLAYMODE *end;
            for (i = half; i >= 0; i--)
                ZSt13__adjust_heapIP15_D3DDISPLAYMODEiS0_PFhRKS0_S3_EEvT_T0_S7_T1_T2_(
                    first, i, n, first[i], comp);
            for (end = last - 1; end - first > 0; end--) {
                _D3DDISPLAYMODE value = *end;
                *end = *first;
                ZSt13__adjust_heapIP15_D3DDISPLAYMODEiS0_PFhRKS0_S3_EEvT_T0_S7_T1_T2_(
                    first, 0, (int)(end - first), value, comp);
            }
            return;
        }
        depth_limit--;
        {
            int n = (int)(last - first);
            _D3DDISPLAYMODE *midPtr = first + n / 2;
            _D3DDISPLAYMODE *pivotPtr;
            _D3DDISPLAYMODE pivot;
            _D3DDISPLAYMODE *lo, *hi;
            if (comp(first, midPtr)) {
                if (comp(midPtr, last - 1))
                    pivotPtr = midPtr;
                else if (comp(first, last - 1))
                    pivotPtr = last - 1;
                else
                    pivotPtr = first;
            } else {
                if (comp(first, last - 1))
                    pivotPtr = first;
                else if (comp(last - 1, midPtr))
                    pivotPtr = midPtr;
                else
                    pivotPtr = last - 1;
            }
            pivot = *pivotPtr;
            lo = first;
            hi = last;
            for (;;) {
                while (!comp(&pivot, lo))
                    lo++;
                hi--;
                while (!comp(hi, &pivot))
                    hi--;
                if (lo >= hi)
                    break;
                {
                    _D3DDISPLAYMODE tmp = *lo;
                    *lo = *hi;
                    *hi = tmp;
                }
                lo++;
            }
            ZSt16__introsort_loopIP15_D3DDISPLAYMODEiPFhRKS0_S3_EEvT_S6_T0_T1_(
                lo, last, depth_limit, comp);
            last = lo;
        }
    }
}
