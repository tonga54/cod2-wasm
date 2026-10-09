#include "common_types.h"
#include "imports.h"
extern int alwaysfails;
#include "bytematch.h"

extern char *getenv(const char *name);

extern DxGlobals dx;
extern dvar_t *r_rendererInUse;
extern const dvar_t *r_depthPrepassModels;
extern const dvar_t *r_drawDecals;
extern const dvar_t *r_distortion;
extern const dvar_t *r_testFill;
extern const dvar_t *sc_blur;
extern const dvar_t *r_glow;
extern const dvar_t *r_fullbright;
extern const dvar_t *r_showFbColorDebug;
extern const dvar_t *r_blur;
extern const dvar_t *r_glowBloomCutoff;
extern const dvar_t *r_glowBloomDesaturation;
extern const dvar_t *sc_showDebug;
extern r_global_permanent_t rgp;
extern refimport_t ri;

#define RB_BACKEND_REGPARM1_ABI COD2_REGPARM(1)
#define RB_BACKEND_REGPARM2_ABI COD2_REGPARM(2)
#define RB_BACKEND_REGPARM2_SSE_ABI COD2_REGPARM(2) COD2_SSEREGPARM
#define RB_BACKEND_REGPARM3_SSE_ABI COD2_REGPARM(3) COD2_SSEREGPARM

int g_rb_tess_type_counts[8];
#ifdef GFX_REAL_D3D9
int g_rb_dip_calls;
#endif
int g_rb_tess_type_idxzero[8];
int g_rb_last_tess_type = 0;
int g_tess_since_begin = 0;
int g_rb_endsurface_count = 0;
int g_rb_stretchpic_calls = 0;
int g_q_stretchpic = 0;
int g_disp_stretchpic = 0;
int g_rb_endsurface_draw = 0;
int g_rb_endsurface_nomaterial = 0;
int g_rb_endsurface_notechnique = 0;
int g_rb_endsurface_dxstate = 0;
int g_rb_endsurface_idxzero = 0;
int g_rb_endsurface_flag1skip = 0;
int g_rb_endsurface_flag2skip = 0;
int g_dsc_techtype[3] = { 0 };
int g_dsc_surfcount[3] = { 0 };
int g_rdsl_ignore_decal = 0;
int g_rdsl_ignore_techm1 = 0;
int g_rdsl_ignore_technull = 0;
int g_rdsl_noignore = 0;
int g_rdsl_sortchange = 0;
int g_rdsl_bf_entry = 0;
void *g_technull_mat = 0;
int g_technull_type = 0;
int g_technull_saved = 0;
int g_technull_saved2 = 0;

void rdsl_log_null_technique2(void *material, int techType)
{
    (void)material;
    (void)techType;
}

extern int printf(const char *, ...);
int rb_drawsurfscmd_count = 0;
int rb_drawsurfscmd_dxskip = 0;

#define RB_DVAR(imp) (*(const dvar_t **)(imp))

static int RB_TextTraceContains(const char *text, const char *needle)
{
    const char *hay;
    const char *n;

    if (!text || !needle || !*needle) {
        return 0;
    }

    for (hay = text; *hay; ++hay) {
        n = needle;
        while (*n && hay[n - needle] == *n) {
            ++n;
        }
        if (!*n) {
            return 1;
        }
    }

    return 0;
}

static int RB_ShouldTraceTextCmd(const char *text)
{
    static int enabled = -1;
    if (enabled < 0)
        enabled = getenv("TEXTCMD_DBG") ? 1 : 0;
    if (!enabled)
        return 0;
    return RB_TextTraceContains(text, "Deathmatch") ||
           RB_TextTraceContains(text, "Click to Continue") ||
           RB_TextTraceContains(text, "Score Limit") ||
           RB_TextTraceContains(text, "Time Limit") ||
           RB_TextTraceContains(text, "50") ||
           RB_TextTraceContains(text, "30");
}

static int diag_rb_skip_this_frame = 0;
static void diag_rb_cmd(int cmdType)
{
    (void)cmdType;
}

extern struct DxState dxState;
static void diag_rb_frame_start(void)
{
    (void)dxState;
}

extern int rb_rdsl_call_count;
extern int g_dip_gl_draw;
extern int g_rb_endsurface_count;
extern int g_rb_endsurface_draw;
extern int g_rb_endsurface_nomaterial;
extern int g_rb_endsurface_notechnique;
extern int g_rb_endsurface_dxstate;
extern int g_rb_endsurface_idxzero;
extern int g_rb_endsurface_flag1skip;
extern int g_rb_endsurface_flag2skip;
extern int g_dip_drawflag_zero;
extern int g_dip_vs_null;
extern int g_dip_vs_bound;
extern int g_dip_vs_skip;
extern int g_dip_numelems_zero;
extern int g_dip_is_tri;
extern int g_technull_saved;
extern void *g_technull_mat;
extern int g_technull_type;
extern int g_tess_since_begin;
extern int g_rb_tess_type_counts[];
extern int g_rb_last_tess_type;
extern int g_rb_exec_count;
extern int g_dsc_techtype[];
extern int g_dsc_surfcount[];
extern int rb_drawsurfscmd_dxskip;
extern int g_rb_skip_reason;
extern int g_rb_dispatch_count;
extern int g_rb_first_cmd;
static void diag_rb_frame_end(void)
{

    rb_rdsl_call_count = 0;
    g_rb_endsurface_count = 0;
    g_rb_endsurface_draw = 0;
    g_rb_endsurface_nomaterial = 0;
    g_rb_endsurface_notechnique = 0;
    g_rb_endsurface_dxstate = 0;
    g_rb_endsurface_idxzero = 0;
    g_rb_endsurface_flag1skip = 0;
    g_rb_endsurface_flag2skip = 0;
    g_dip_gl_draw = 0;
    g_dip_is_tri = 0;
    g_dip_drawflag_zero = 0;
    g_dip_vs_null = 0;
    g_dip_vs_bound = 0;
    g_dip_vs_skip = 0;
    g_dip_numelems_zero = 0;
    g_rdsl_ignore_technull = 0;
    g_rdsl_ignore_decal = 0;
    g_rdsl_ignore_techm1 = 0;
    g_rdsl_noignore = 0;
    g_rdsl_sortchange = 0;
    g_rdsl_bf_entry = 0;
    rb_drawsurfscmd_count = 0;
    rb_drawsurfscmd_dxskip = 0;
    g_technull_saved = 0;
    memset(g_rb_tess_type_counts, 0, sizeof(int) * 8);
    g_tess_since_begin = 0;
}

extern struct materialCommands_t tess;
extern struct r_backEndGlobals_t backEnd;
extern const GfxBackEndData *backEndData;
extern GLuint g_FenceID;
extern const byte color_table[8][4];
extern const Bool refEntIsInWorldSpace[10];

extern void RB_TessBad(const surfaceType_t *surfType);
extern void RB_TessPoly(const surfaceType_t *surfType);
extern void RB_TessBackEndEntity(const surfaceType_t *surfType);
extern void RB_TessXModelSkinned(const surfaceType_t *surfType);
extern void RB_TessXModelRigid(const surfaceType_t *surfType);
extern void RB_TessStaticModelCached(const surfaceType_t *surfType);
extern void RB_TessTriangles(const surfaceType_t *surfType);

static void (*const rb_tessTable[8])(const surfaceType_t *) = {
    RB_TessBad,
    RB_TessPoly,
    RB_TessBackEndEntity,
    RB_TessXModelSkinned,
    RB_TessXModelRigid,
    RB_TessStaticModelCached,
    RB_TessTriangles,
    RB_TessBad
};

extern FontHandle R_RegisterFont(const char *fontName, int imageTrack);
extern void RB_TouchAllImages(void);
extern int ColorIndex(int c);
extern void RB_EndSurface(void);
extern void RB_BeginSurface(const Material *material, MaterialTechniqueType techType, int lmapIndex);
extern void *Image_GetSurface(void *image);
extern void RB_TessEntity(void *entity);
extern Bool Material_IsDefault(MaterialHandle handle);
extern BOOL QueryPerformanceFrequency(void *lpPerformanceFrequency);
extern void RB_DrawSun(const void *sunData);
extern void RB_SetShadowLookupMatrix(const void *matrix);
extern void RB_UpdateViewportConstants(void);
extern void RB_UpdateViewport(void);
extern void RB_SetRenderTarget(int renderTargetId);
extern void RB_DrawSunPostEffects(const void *sunData);
extern void *RB_GetActiveWorldMatrix(void);
extern void MatrixIdentity44(void *matrix);
extern void MatrixMultiply44(const void *a, const void *b, void *out);
extern Bool RB_GetViewport(void *viewport);
extern int MacOpenGLUtils_ConvertD3DProjectionMatrixToOpenGL(void *proj, float width, float height);
extern void RB_SetViewMatrix(const void *matrix);
extern void RB_SetProjectionMatrix(const void *matrix);
extern void MatrixForViewer(void *out, const void *origin, const void *axis);
extern void InfinitePerspectiveMatrix(void *out, float fovX, float fovY, float zNear);
extern void RB_PushMatrixStack(void);
extern void RB_PopMatrixStack(void);
extern float Vec2Normalize(float *v);
extern float Vec3Normalize(float *v);
extern void R_DecomposeSort(unsigned int sortKey, int *entityIndex, const Material **material, int *lightmap);
extern void RB_SetDepthRange(float near, float far);
extern void RB_SetDepthHackNearClip(float nearClip);
extern void RB_SetWorldMatrixForEntity(const void *entity);
extern int RB_FogOffset(void);
extern Glyph *R_GetCharacterGlyph(FontHandle font, int charCode);
extern void RB_ChangedWorldMatrix(float worldScale);
extern void RB_SetMatricesForView(const void *viewParms);
extern float floorf(float x);
extern float sqrtf(float x);
extern float sinf(float x);
extern float cosf(float x);
extern double pow(double base, double exponent);

static inline void RB_SetVertex2D(char *tessBase, int vertIndex, const dvar_t *renderer,
                                  float px, float py, float s, float t, D3DCOLOR color)
{
    if (renderer->current.integer != 2) {
        char *v = tessBase + vertIndex * 64;
        ((GfxVertexDx7 *)v)->xyz[0] = px;
        ((GfxVertexDx7 *)v)->xyz[1] = py;
        ((GfxVertexDx7 *)v)->xyz[2] = 0;
        ((GfxVertexDx7 *)v)->normal[0] = 1.0f;
        ((GfxVertex *)v)->normal[0] = 0;
        ((GfxVertex *)v)->normal[1] = 0;
        ((GfxVertex *)v)->normal[2] = 1.0f;
        ((GfxVertex *)v)->color.packed = color;
        ((GfxVertex *)v)->texCoord[0] = s;
        ((GfxVertex *)v)->texCoord[1] = t;
        ((GfxVertex *)v)->binormal[0] = 0;
        ((GfxVertex *)v)->binormal[1] = 1.0f;
        ((GfxVertex *)v)->binormal[2] = 0;
        ((GfxVertex *)v)->tangent[0] = 1.0f;
        ((GfxVertex *)v)->tangent[1] = 0;
        ((GfxVertex *)v)->tangent[2] = 0;
    } else {
        char *v = tessBase + vertIndex * 36;
        ((GfxVertexDx7 *)v)->xyz[0] = px;
        ((GfxVertexDx7 *)v)->xyz[1] = py;
        ((GfxVertexDx7 *)v)->xyz[2] = 0;
        ((GfxVertexDx7 *)v)->normal[0] = 0;
        ((GfxVertexDx7 *)v)->normal[1] = 0;
        ((GfxVertexDx7 *)v)->normal[2] = 1.0f;
        ((GfxVertexDx7 *)v)->color.packed = color;
        ((GfxVertexDx7 *)v)->texCoord[0] = s;
        ((GfxVertexDx7 *)v)->texCoord[1] = t;
    }
}

static inline int RB_CheckTessOverflow4(char *t)
{
    int vc = tess.vertexCount;
    int ic = tess.indexCount;
    if (vc + 4 > 0x154a || ic + 6 > 0x100000) {
        int savedDecl = tess.declType;
        RB_EndSurface();
        RB_BeginSurface(tess.material, tess.techType, tess.lmapIndex);
        if (tess.declType != savedDecl) {
            if (tess.indexCount != 0 || tess.optimizedIndexCount != 0)
                RB_EndSurface();
            tess.declType = savedDecl;
        }
        vc = tess.vertexCount;
    }
    return vc;
}

static inline void RB_WriteQuadIndicesAt(char *t, int vc, int ic)
{
    tess.indices[ic + 0] = (r_index_t)(vc + 3);
    tess.indices[ic + 1] = (r_index_t)vc;
    tess.indices[ic + 2] = (r_index_t)(vc + 2);
    tess.indices[ic + 3] = (r_index_t)(vc + 2);
    tess.indices[ic + 4] = (r_index_t)vc;
    tess.indices[ic + 5] = (r_index_t)(vc + 1);
}

static inline void RB_WriteQuadIndices(char *t, int vc)
{
    RB_WriteQuadIndicesAt(t, vc, tess.indexCount);
}

static inline void RB_BeginSurface2D(char *t, const Material *material)
{
    if (material != tess.material || tess.techType != 3) {
        if (tess.indexCount != 0 || tess.optimizedIndexCount != 0)
            RB_EndSurface();
        RB_BeginSurface(material, 3, 0x1f);
    }
}

static inline unsigned int nextPow2(unsigned int v)
{
    unsigned int p = 1;
    if (v <= 1)
        return 1;
    while (p < v)
        p <<= 1;
    return p;
}

extern const char *R_ErrorDescription(HRESULT hr);
extern void R_Error(int level, const char *msg, ...);
extern void R_FlushStaticModelCache(void);
extern void R_SetColorMappings(void);
extern void RB_SetViewport(const void *viewport);
extern BOOL QueryPerformanceCounter(void *lpPerformanceCount);

void RB_SetCodeConstant(int constant, vec_t x, vec_t y, vec_t z, vec_t w);
static void RB_GotoCmd(GfxRenderCommandExecState *execState);
static void RB_ReturnCmd(GfxRenderCommandExecState *execState);
void RB_SetGammaRamp(const GfxGammaRamp *gammaTable);
static void RB_TouchAllImagesCmd(GfxRenderCommandExecState *execState);
qboolean RB_IsGpuFenceFinished(void);
void RB_GpuWaited(int ticks);
static void RB_EndFrame_real(void);
void RB_EndFrame(void)
{
    {
        extern int g_rb_endsurface_count, g_rb_endsurface_nomaterial, g_rb_endsurface_notechnique,
                   g_rb_endsurface_dxstate, g_rb_endsurface_draw, g_rb_endsurface_flag1skip,
                   g_rb_endsurface_flag2skip, g_rb_endsurface_idxzero;
        extern void Com_Printf(const char *, ...);
        extern int g_rb_stretchpic_calls, g_q_stretchpic, g_disp_stretchpic;
        static int fc;
        if (++fc == 120)
            Com_Printf("[rbdiag] q=%d disp=%d stretchpic=%d count=%d draw=%d nomat=%d notech=%d dxstate=%d f1=%d f2=%d idx0=%d\n",
                       g_q_stretchpic, g_disp_stretchpic, g_rb_stretchpic_calls,
                       g_rb_endsurface_count, g_rb_endsurface_draw, g_rb_endsurface_nomaterial,
                       g_rb_endsurface_notechnique, g_rb_endsurface_dxstate, g_rb_endsurface_flag1skip,
                       g_rb_endsurface_flag2skip, g_rb_endsurface_idxzero);
    }
    RB_EndFrame_real();
}
void RB_InitBackendGlobalStructs(void);
void RB_RegisterBackendAssets(void);
void RB_LookupColor(int c, byte *color);
static void RB_CallCmd(GfxRenderCommandExecState *execState);
static void RB_SetClipPlanesCmd(GfxRenderCommandExecState *execState);
static void RB_StretchRawCmd(GfxRenderCommandExecState *execState);
static void RB_DrawSunCmd(GfxRenderCommandExecState *execState);
void RB_ClearScreen(int whichToClear, const vec_t *color, float depth, int stencil);
static void RB_ClearScreenCmd(GfxRenderCommandExecState *execState);
static void RB_BACKEND_REGPARM2_ABI RB_UpdateColorInternal(const vec_t *floatColor, byte *color);
void RB_UpdateColor(const vec_t *color_allies, const vec_t *color_axis);
void RB_AdaptiveGpuSyncWait(void);
void RB_AdaptiveGpuSyncTarget(void);
static void RB_BACKEND_REGPARM1_ABI RB_EndBenchmarkGpu_impl(void *time);
static void RB_BACKEND_REGPARM1_ABI RB_BeginBenchmarkGpu_impl(void *time);
void RB_Set3D(void);
static void RB_SetMaterialColorCmd(GfxRenderCommandExecState *execState);
static void RB_SetLightPropertiesCmd(GfxRenderCommandExecState *execState);
static void RB_SetStencilRefValueCmd(GfxRenderCommandExecState *execState);
static void RB_SetShadowCookieCmd(GfxRenderCommandExecState *execState);
static void RB_BeginViewCmd(GfxRenderCommandExecState *execState);
static void RB_SetViewportCmd(GfxRenderCommandExecState *execState);
static void RB_SetRenderTargetCmd(GfxRenderCommandExecState *execState);
static void RB_RenderDrawSurfList(GfxDrawSurf *drawSurfs, int drawSurfCount,
                                  MaterialTechniqueType techType, GfxDrawSurfOrder order);
static void RB_DrawSurfsCmd(GfxRenderCommandExecState *execState);
static void RB_DrawSunPostEffectsCmd(GfxRenderCommandExecState *execState);
static void RB_Set2D(void);
static void RB_DrawTrianglesCmd(GfxRenderCommandExecState *execState);
static void RB_SaveScreenCmd(GfxRenderCommandExecState *execState);
static void RB_ApplyEarlyPostEffectsCmd(GfxRenderCommandExecState *execState);
static void RB_DrawSpriteCmd(GfxRenderCommandExecState *execState);
void RB_DrawLines2D(int count, int width, const GfxPointVertex *verts);
void RB_DrawTextInSpace(const char *text, FontHandle font, const vec_t *org, const vec_t *xPixelStep, const vec_t *yPixelStep, D3DCOLOR color);
static void RB_DrawTextInSpaceCmd(GfxRenderCommandExecState *execState);
static float RB_BACKEND_REGPARM2_ABI RB_TestFillPass3D_impl(const Material *material, MaterialTechniqueType techType);
static float RB_BACKEND_REGPARM2_ABI RB_TestFillPass3D(const Material *material, MaterialTechniqueType techType);
void RB_DrawStretchPic(const Material *material, float x, float y, float w, float h, float s0, float t0, float s1, float t1, D3DCOLOR color, GfxPrimStatsTarget statsTarget);
static void RB_StretchPicCmd(GfxRenderCommandExecState *execState);
void RB_ExecuteRenderCommands(const void *data);
void RB_DrawFullScreenColoredQuad(const Material *material, float s0, float t0, float s1, float t1, D3DCOLOR color);
static void RB_DrawFullScreenColoredQuadCmd(GfxRenderCommandExecState *execState);
static void RB_BlendSavedScreenCmd(GfxRenderCommandExecState *execState);
static void RB_BlurShadowCookieCmd(GfxRenderCommandExecState *execState);
static void RB_DrawTextCmd(GfxRenderCommandExecState *execState);
static void RB_ApplyLatePostEffectsCmd(GfxRenderCommandExecState *execState);
static void RB_StretchPicRotateCmd(GfxRenderCommandExecState *execState);
static void RB_DrawQuadPicCmd(GfxRenderCommandExecState *execState);
void RB_DrawLines3D(int count, int width, const GfxPointVertex *verts, int depthTest);
static void RB_DrawLinesCmd(GfxRenderCommandExecState *execState);
static void RB_StencilPlanesCmd(GfxRenderCommandExecState *execState);
static void RB_DrawPointsCmd(GfxRenderCommandExecState *execState);

void (*const RB_RenderCommandTable[34])(GfxRenderCommandExecState *execState) = {
    NULL,
    RB_GotoCmd,
    RB_CallCmd,
    RB_ReturnCmd,
    RB_SetMaterialColorCmd,
    RB_SetLightPropertiesCmd,
    NULL,
    NULL,
    NULL,
    NULL,
    RB_SaveScreenCmd,
    RB_ClearScreenCmd,
    RB_BeginViewCmd,
    RB_SetViewportCmd,
    RB_SetRenderTargetCmd,
    RB_StretchPicCmd,
    RB_StretchPicRotateCmd,
    RB_StretchRawCmd,
    RB_DrawQuadPicCmd,
    RB_DrawSpriteCmd,
    RB_DrawFullScreenColoredQuadCmd,
    RB_DrawTextCmd,
    RB_DrawTextInSpaceCmd,
    RB_DrawSurfsCmd,
    RB_DrawSunCmd,
    RB_ApplyEarlyPostEffectsCmd,
    RB_ApplyLatePostEffectsCmd,
    RB_DrawSunPostEffectsCmd,
    NULL,
    RB_BlendSavedScreenCmd,
    NULL,
    NULL,
    NULL,
    RB_TouchAllImagesCmd,
};

void RB_SetCodeConstant(int constant, vec_t x, vec_t y, vec_t z, vec_t w)
{
    vec_t *c = backEnd.codeConsts[constant - 128];
    c[0] = x;
    c[1] = y;
    c[2] = z;
    c[3] = w;
}

static void RB_GotoCmd(GfxRenderCommandExecState *execState)
{
    const byte *base = backEndData ? backEndData->commands.cmds : NULL;
    const byte *cmd = (const byte *)execState->cmd;
    const void *target = ((const GfxCmdCall *)cmd)->subCmd;   /* x86 was cmd+4 */
    static int traceCount;

    if (base && traceCount < 120) {
        int off = (int)(cmd - base);
        int targetOff = (int)((const byte *)target - base);
        if (off < 5000 || targetOff < 5000) {
            if (getenv("DBGSPAM"))
                printf("[cmdflow] goto off=%d target=%d\n", off, targetOff);
            ++traceCount;
        }
    }

    execState->cmd = target;
}

static void RB_ReturnCmd(GfxRenderCommandExecState *execState)
{
    int idx = execState->stackPos - 1;
    execState->stackPos = idx;
    execState->cmd = execState->retCmd[idx];
}

void RB_SetGammaRamp(const GfxGammaRamp *gammaTable)
{
    unsigned short d3dGammaRamp[256 * 3];
    int i;
    byte *dx;
    void *dev;
    void **vt;

    for (i = 0; i < 256; i++) {
        d3dGammaRamp[i] = gammaTable->entries[i];
        d3dGammaRamp[i + 256] = gammaTable->entries[i];
        d3dGammaRamp[i + 512] = gammaTable->entries[i];
    }

    dx = (byte *)imp_dx;
    dev = ((DxGlobals *)dx)->device;
    vt = *(void ***)dev;
    ((void(D3DVTCC *)(void *, int, int, void *))vt[0x54 / 4])(dev, ((DxGlobals *)dx)->targetWindowIndex, 0, d3dGammaRamp);
}

static void RB_TouchAllImagesCmd(GfxRenderCommandExecState *execState)
{
    const GfxCmdHeader *cmd;

    RB_TouchAllImages();
    cmd = (const GfxCmdHeader *)execState->cmd;
    execState->cmd = (const void *)((byte *)cmd + cmd->byteCount);
}

qboolean RB_IsGpuFenceFinished(void)
{
    byte *dx = (byte *)imp_dx;
    qboolean finished;

    if (!((DxGlobals *)dx)->flushGpuQueryIssued)
        return 1;

    finished = glTestFenceAPPLE(g_FenceID) != 0;
    if (finished)
        glDeleteFencesAPPLE(1, &g_FenceID);
    if (finished) {
        ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
        return 1;
    }
    return 0;
}

void RB_GpuWaited(int ticks)
{
    dx.gpuSyncAlreadyWaited = ticks;
}

#ifdef GFX_REAL_D3D9
long long g_perfFreq;
extern unsigned int __attribute__((stdcall)) GetTickCount(void);
long long QueryPerf(void)
{

    g_perfFreq = 1000;
    return (long long)(unsigned long long)GetTickCount();
}
#endif

static void RB_EndFrame_real(void)
{
    char *dx = (char *)imp_dx;
    void *device;
    void **vtable;
    HRESULT hr;
    char *r_gamma_cvar;
    char *r_ignoreHwGamma_cvar;

    device = ((DxGlobals *)dx)->device;
    vtable = *(void ***)device;
#ifdef GFX_REAL_D3D9
    if (getenv("REALD3D9_RDSL")) {
        static int fc = 0;
        if ((fc++ % 30) == 0)
            fprintf(stderr, "[RDSL] bf_entry=%d noignore(drawn)=%d technull=%d decal=%d techm1=%d\n",
                    g_rdsl_bf_entry, g_rdsl_noignore, g_rdsl_ignore_technull,
                    g_rdsl_ignore_decal, g_rdsl_ignore_techm1);
    }
#endif
#ifdef GFX_REAL_D3D9

    if (getenv("REALD3D9_BBREAD")) {
        static int bbr = 0;
        if (bbr < 40) {
            bbr++;
            IDirect3DSurface9 *bbS = ((DxGlobals *)dx)->renderTargets[0].colorSurface;
            IDirect3DSurface9 *sysSurf = NULL;

            ((HRESULT(D3DVTCC *)(void *, UINT, UINT, DWORD, DWORD, void *, void *))vtable[0x90 / 4])(
                device, 640, 480, 22, 2, &sysSurf, NULL);
            if (sysSurf && bbS) {

                HRESULT grd = ((HRESULT(D3DVTCC *)(void *, void *, void *))vtable[0x80 / 4])(device, bbS, sysSurf);
                struct {
                    int Pitch;
                    void *pBits;
                } lr;
                lr.pBits = 0;
                lr.Pitch = 0;
                HRESULT lh = ((HRESULT(D3DVTCC *)(void *, void *, void *, DWORD))((*(void ***)sysSurf)[0x34 / 4]))(sysSurf, &lr, NULL, 0x10);
                if (lh >= 0 && lr.pBits && lr.Pitch > 0) {
                    unsigned *base = (unsigned *)lr.pBits;
                    unsigned acc = 0;
                    int yy, xx, nz = 0;
                    for (yy = 0; yy < 480; yy += 17)
                        for (xx = 0; xx < 640; xx += 17) {
                            unsigned p = base[yy * (lr.Pitch / 4) + xx] & 0xFFFFFF;
                            acc |= p;
                            if (p)
                                nz++;
                        }
                    fprintf(stderr, "[BBREAD] grd=0x%08x BB content: OR=%06x nonblackSamples=%d\n", (unsigned)grd, acc, nz);
                    ((HRESULT(D3DVTCC *)(void *))((*(void ***)sysSurf)[0x38 / 4]))(sysSurf);
                } else {
                    fprintf(stderr, "[BBREAD] grd=0x%08x lock failed 0x%08x\n", (unsigned)grd, (unsigned)lh);
                }
                ((HRESULT(D3DVTCC *)(void *))((*(void ***)sysSurf)[2]))(sysSurf);
            } else {
                fprintf(stderr, "[BBREAD] CreateOffscreenPlainSurface failed sys=%p\n", (void *)sysSurf);
            }
        }
    }
    if (getenv("REALD3D9_FRAMELOG")) {
        extern int g_rb_dip_calls;
        extern int rb_drawsurfscmd_count;
        static int fc2 = 0;
        if ((fc2++ % 30) == 0) {
            int tessSum = 0, k;
            for (k = 0; k < 8; k++)
                tessSum += g_rb_tess_type_counts[k];
            fprintf(stderr, "[FRAMELOG] DrawIndexedPrim=%d tessDispatch=%d drawSurfsCmd=%d\n",
                    g_rb_dip_calls, tessSum, rb_drawsurfscmd_count);
        }
    }

    if (getenv("REALD3D9_CLEARTEST")) {
        extern int g_rb_dip_calls;

        IDirect3DSurface9 *bbSurf = ((DxGlobals *)dx)->renderTargets[0].colorSurface;

        {
            static int once = 0;
            if (!once) {
                once = 1;
                unsigned char desc[64];
                IDirect3DSurface9 *curRT = NULL;

                ((HRESULT(D3DVTCC *)(void *, DWORD, void *))(vtable[0x98 / 4]))(device, 0, &curRT);
                if (curRT) {
                    memset(desc, 0, sizeof desc);
                    ((HRESULT(D3DVTCC *)(void *, void *))((*(void ***)curRT)[0x30 / 4]))(curRT, desc);
                    fprintf(stderr, "[RTDESC] curRT W=%u H=%u fmt=%u\n",
                            *(unsigned *)(desc + 24), *(unsigned *)(desc + 28), *(unsigned *)desc);
                }
                if (bbSurf) {
                    memset(desc, 0, sizeof desc);
                    ((HRESULT(D3DVTCC *)(void *, void *))((*(void ***)bbSurf)[0x30 / 4]))(bbSurf, desc);
                    fprintf(stderr, "[RTDESC] storedBB W=%u H=%u fmt=%u\n",
                            *(unsigned *)(desc + 24), *(unsigned *)(desc + 28), *(unsigned *)desc);
                }
                fflush(stderr);
            }
        }
        if (getenv("REALD3D9_FORCEBB") && bbSurf) {
            ((HRESULT(D3DVTCC *)(void *, DWORD, void *))(vtable[0x94 / 4]))(device, 0, bbSurf);
        }
        ((HRESULT(D3DVTCC *)(void *, DWORD, const void *, DWORD, D3DCOLOR, float, DWORD))(vtable[0xAC / 4]))(device, 0, NULL, 1 , 0xFFFF0000u, 1.0f, 0);
        {
            extern int rb_drawsurfscmd_count;
            int tessSum = 0, k;
            for (k = 0; k < 8; k++)
                tessSum += g_rb_tess_type_counts[k];
            fprintf(stderr, "[REALD3D9] frame: drawSurfsCmd=%d tessDispatch=%d DrawIndexedPrim=%d (cleared red)\n",
                    rb_drawsurfscmd_count, tessSum, g_rb_dip_calls);
            fflush(stderr);
            rb_drawsurfscmd_count = 0;
            for (k = 0; k < 8; k++)
                g_rb_tess_type_counts[k] = 0;
        }
        g_rb_dip_calls = 0;
    }
#endif
#ifdef GFX_REAL_D3D9

    if (getenv("REALD3D9_PERFLOG")) {
        extern long long QueryPerf(void);
        static long long lastFrameEnd;
        static int pc;
        long long t0 = QueryPerf();
        hr = ((HRESULT(D3DVTCC *)(void *, void *, void *, void *, void *))(vtable[0x44 / 4]))(device, NULL, NULL, NULL, NULL);
        long long t1 = QueryPerf();
        extern long long g_perfFreq;
        if (g_perfFreq && (pc++ % 30) == 0) {
            double presentMs = (double)(t1 - t0) * 1000.0 / (double)g_perfFreq;
            double frameMs = lastFrameEnd ? (double)(t1 - lastFrameEnd) * 1000.0 / (double)g_perfFreq : 0;
            extern int g_smc_hit, g_smc_new;
            extern long long g_smc_recache_ms;
            fprintf(stderr, "[PERFLOG] frameMs=%.1f presentMs=%.1f (cpuMs=%.1f) -> %.1f fps | SMC hit=%d recache=%d recacheMs=%.1f\n",
                    frameMs, presentMs, frameMs - presentMs, frameMs > 0 ? 1000.0 / frameMs : 0,
                    g_smc_hit / 30, g_smc_new / 30, (double)g_smc_recache_ms / 30.0);
            g_smc_hit = 0;
            g_smc_new = 0;
            g_smc_recache_ms = 0;
            fflush(stderr);
        }
        lastFrameEnd = t1;
    } else
#endif
        hr = ((HRESULT(D3DVTCC *)(void *, void *, void *, void *, void *))(vtable[0x44 / 4]))(device, NULL, NULL, NULL, NULL);

    if (hr < 0) {
        if (hr != (HRESULT)0x88760868) {
            R_Error(0, "Direct3DDevice9::Present failed: %s\n", R_ErrorDescription(hr));
        }
        ((DxGlobals *)dx)->deviceLost = 1;
        R_FlushStaticModelCache();
    }

    dx = (char *)imp_dx;
    if (((DxGlobals *)dx)->dynamicIndexBuffer)   /* lazily set; NULL if no indexed draw this frame */
        *(int *)((DxGlobals *)dx)->dynamicIndexBuffer = 0;

    backEnd.projection2D = 0;

    r_gamma_cvar = *(char **)imp_r_gamma;

    if (((const dvar_t *)r_gamma_cvar)->modified ||
        ((const dvar_t *)*(char **)imp_r_ignoreHwGamma)->modified) {

        ri.Dvar_ClearModified((const dvar_t *)r_gamma_cvar);

        r_ignoreHwGamma_cvar = *(char **)imp_r_ignoreHwGamma;
        ri.Dvar_ClearModified((const dvar_t *)r_ignoreHwGamma_cvar);

        if (!((const dvar_t *)r_ignoreHwGamma_cvar)->current.enabled) {
            R_SetColorMappings();
            return;
        }
    }
}

void RB_InitSceneViewport(void);

void RB_InitBackendGlobalStructs(void)
{
    memset(&backEnd, 0, 0x36e90);
    *(int *)&backEnd.worldEntity = 3;
    backEnd.resolvedPostSunTarget = 0xe;
    backEnd.resolvedSceneTarget = 0xe;
    RB_InitSceneViewport();
}

void RB_RegisterBackendAssets(void)
{
    backEnd.debugFont = R_RegisterFont("fonts/smalldevfont", 1);
}

void RB_LookupColor(int c, byte *color)
{
    int idx = (unsigned char)ColorIndex((byte)c);
    if ((unsigned)idx <= 7) {
        *(unsigned int *)color = ((const unsigned int *)color_table)[idx];
    } else if ((byte)c == '8') {
        *(unsigned int *)color = *(unsigned int *)&backEnd.color_allies;
    } else if ((byte)c == '9') {
        *(unsigned int *)color = *(unsigned int *)&backEnd.color_axis;
    } else {
        color[2] = 0xff;
        color[1] = 0xff;
        color[0] = 0xff;
        color[3] = 0xff;
    }
}

static void RB_CallCmd(GfxRenderCommandExecState *execState)
{
    const GfxCmdHeader *cmd = (const GfxCmdHeader *)execState->cmd;
    const byte *base = backEndData ? backEndData->commands.cmds : NULL;
    const void *target = ((const GfxCmdCall *)cmd)->subCmd;   /* x86 was cmd+4 */
    static int traceCount;
    int idx = execState->stackPos;

    execState->retCmd[idx] = (const void *)((byte *)cmd + cmd->byteCount);
    execState->stackPos = idx + 1;

    if (base && traceCount < 160) {
        int off = (int)((const byte *)cmd - base);
        int targetOff = (int)((const byte *)target - base);
        int retOff = (int)((const byte *)execState->retCmd[idx] - base);
        if (off < 5000 || targetOff < 5000) {
            if (getenv("DBGSPAM"))
                printf("[cmdflow] call off=%d target=%d ret=%d stack=%d\n",
                       off, targetOff, retOff, idx);
            ++traceCount;
        }
    }

    execState->cmd = target;
}

static void RB_SetClipPlanesCmd(GfxRenderCommandExecState *execState)
{
    DxState *dxs = &dxState;
    byte *cmd = (byte *)execState->cmd;
    int planeCount = ((GfxCmdSetClipPlanes *)cmd)->clipPlaneCount;
    void *device;
    void **vtable;
    int i;

    if (planeCount != dxs->clipPlaneCount) {

        do {
            device = ((DxGlobals *)imp_dx)->device;
            vtable = *(void ***)device;
            ((HRESULT(D3DVTCC *)(void *, DWORD, DWORD))(vtable[0xE4 / 4]))(
                device, 0x98, (DWORD)((1 << planeCount) - 1));
        } while (*(volatile int *)&alwaysfails);

        dxs->clipPlaneCount = planeCount;
    }

    if (planeCount > 0) {
        byte *planeData = (byte *)((GfxCmdSetClipPlanes *)cmd)->clipPlanes;
        for (i = 0; i < planeCount; i++) {

            do {
                device = ((DxGlobals *)imp_dx)->device;
                vtable = *(void ***)device;
                ((HRESULT(D3DVTCC *)(void *, DWORD, const float *))(vtable[0xDC / 4]))(
                    device, (DWORD)i, (const float *)planeData);
            } while (*(volatile int *)&alwaysfails);
            planeData += 16;
        }
    }

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_StretchRawCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd = (byte *)execState->cmd;
    int x = ((GfxCmdStretchRaw *)cmd)->x;
    int y = ((GfxCmdStretchRaw *)cmd)->y;
    int w = ((GfxCmdStretchRaw *)cmd)->w;
    int h = ((GfxCmdStretchRaw *)cmd)->h;
    int cols = ((GfxCmdStretchRaw *)cmd)->cols;
    int rows = ((GfxCmdStretchRaw *)cmd)->rows;
    byte *data = ((GfxCmdStretchRaw *)cmd)->data;
    void *rawTexture = NULL;
    void *device;
    void **devVtable;
    void **texVtable;
    HRESULT hr;
    int lockedRect[2];
    int dstRect[4];
    void *surface = NULL;
    byte *dest;
    int pitch;
    int rowBytes;
    int i;

    device = ((DxGlobals *)imp_dx)->device;
    devVtable = *(void ***)device;
    /* D3DUSAGE_DYNAMIC (0x200): this texture is locked with D3DLOCK_DISCARD below,
     * which real D3D9 only permits on dynamic textures (Wine ignores the rule). */
    hr = ((HRESULT(D3DVTCC *)(void *, UINT, UINT, UINT, DWORD, DWORD, DWORD, void **, void *))(devVtable[0x5C / 4]))(
        device, (UINT)cols, (UINT)rows, 1, 0x200, 0x16, 0, &rawTexture, NULL);

    if (hr >= 0) {

        do {
            texVtable = *(void ***)rawTexture;
            ((HRESULT(D3DVTCC *)(void *, UINT, void *, void *, DWORD))(texVtable[0x4C / 4]))(
                rawTexture, 0, lockedRect, NULL, 0x2000);
        } while (*(volatile int *)&alwaysfails);

        dest = (byte *)(size_t)lockedRect[1];
        pitch = lockedRect[0];

        if (rows > 0) {
            rowBytes = cols * 4;
            for (i = 0; i < rows; i++) {
                memcpy(dest, data, rowBytes);
                data += rowBytes;
                dest += pitch;
            }
        }

        texVtable = *(void ***)rawTexture;
        ((HRESULT(D3DVTCC *)(void *, UINT))(texVtable[0x50 / 4]))(rawTexture, 0);

        dstRect[0] = x;
        dstRect[1] = y;
        dstRect[2] = x + w;
        dstRect[3] = y + h;

        texVtable = *(void ***)rawTexture;
        ((HRESULT(D3DVTCC *)(void *, UINT, void **))(texVtable[0x48 / 4]))(rawTexture, 0, &surface);

        {
            char *dx = (char *)imp_dx;
            void *backBuffer = ((DxGlobals *)dx)->renderTargets[0].colorSurface;
            device = ((DxGlobals *)dx)->device;
            devVtable = *(void ***)device;
            ((HRESULT(D3DVTCC *)(void *, void *, void *, void *, void *, DWORD))(devVtable[0x88 / 4]))(
                device, surface, NULL, backBuffer, dstRect, 2);
        }

        ((ULONG (*)(void *))(*(void ***)surface)[0x08 / 4])(surface);
        ((ULONG (*)(void *))(*(void ***)rawTexture)[0x08 / 4])(rawTexture);
    }

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_DrawSunCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd = (byte *)execState->cmd;
    unsigned int byteCount;
    RB_DrawSun(*(void **)(cmd + 4));
    cmd = (byte *)execState->cmd;
    byteCount = ((const GfxCmdHeader *)cmd)->byteCount;
    execState->cmd = (const void *)(cmd + byteCount);
}

void RB_ClearScreen(int whichToClear, const vec_t *color, float depth, int stencil)
{
    DxState *dxs = &dxState;
    DWORD clearFlags = 0;
    int viewport[4];
    byte r, g, b, a;
    DWORD d3dColor;
    void *device;
    void **vtable;

    if (whichToClear & 2)
        clearFlags |= 2;
    if (whichToClear & 4)
        clearFlags |= 4;
    if (whichToClear & 1)
        clearFlags |= 1;

    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = dxs->renderTargetWidth;
    viewport[3] = dxs->renderTargetHeight;
    RB_SetViewport(viewport);
    backEnd.viewportIsDirty = 1;

    r = (byte)(int)floorf(color[0] * 255.0f + 0.5f);
    g = (byte)(int)floorf(color[1] * 255.0f + 0.5f);
    b = (byte)(int)floorf(color[2] * 255.0f + 0.5f);
    a = (byte)(int)floorf(color[3] * 255.0f + 0.5f);

    d3dColor = ((DWORD)a << 24) | ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;

#ifdef GFX_REAL_D3D9

    if (getenv("REALD3D9_GREENCLEAR") && (clearFlags & 1))
        d3dColor = 0xFF00FF00u;
#endif

    do {
        device = ((DxGlobals *)imp_dx)->device;
        vtable = *(void ***)device;
#ifdef GFX_REAL_D3D9
        if (getenv("REALD3D9_CLEARRT") && (clearFlags & 1)) {
            static int cc = 0;
            if (cc++ < 8) {
                IDirect3DSurface9 *crt = NULL;
                void *bb = dx.renderTargets[0].colorSurface;
                ((HRESULT(D3DVTCC *)(void *, DWORD, void *))(vtable[0x98 / 4]))(device, 0, &crt);
                fprintf(stderr, "[CLEARRT] clear color=%08x RT=%p bb=%p %s rtId=%d\n",
                        (unsigned)d3dColor, (void *)crt, bb, (crt == bb ? "==BB" : "OFFSCREEN"),
                        dxs->renderTargetId);
                if (crt)
                    ((HRESULT(D3DVTCC *)(void *))((*(void ***)crt)[2]))(crt);
            }
        }
#endif
        ((HRESULT(D3DVTCC *)(void *, DWORD, void *, DWORD, DWORD, float, DWORD))(vtable[0xAC / 4]))(
            device, 0, NULL, clearFlags, d3dColor, depth, (DWORD)(byte)stencil);
    } while (*(volatile int *)&alwaysfails);
}

static void RB_ClearScreenCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd = (byte *)execState->cmd;
    unsigned int byteCount;
    RB_ClearScreen(((GfxCmdClearScreen *)cmd)->whichToClear,
                   ((GfxCmdClearScreen *)cmd)->color,
                   ((GfxCmdClearScreen *)cmd)->depth,
                   ((GfxCmdClearScreen *)cmd)->stencil);
    cmd = (byte *)execState->cmd;
    byteCount = ((const GfxCmdHeader *)cmd)->byteCount;
    execState->cmd = (const void *)(cmd + byteCount);
}

static void RB_BACKEND_REGPARM2_ABI RB_UpdateColorInternal(const vec_t *floatColor, byte *color)
{
    color[0] = (byte)(int)floorf(floatColor[0] * 255.0f + 0.5f);
    color[1] = (byte)(int)floorf(floatColor[1] * 255.0f + 0.5f);
    color[2] = (byte)(int)floorf(floatColor[2] * 255.0f + 0.5f);
    color[3] = (byte)(int)floorf(floatColor[3] * 255.0f + 0.5f);
}

void RB_UpdateColor(const vec_t *color_allies, const vec_t *color_axis)
{
    RB_UpdateColorInternal(color_allies, (byte *)&backEnd.color_allies);
    RB_UpdateColorInternal(color_axis, (byte *)&backEnd.color_axis);
}

static inline unsigned int rdtsc_lo(void)
{
    unsigned int lo;
    lo = 0;
    return lo;
}

void RB_AdaptiveGpuSyncWait(void)
{
    byte *dx = (byte *)imp_dx;
    long long startTime;
    long long waitedTime;
    int syncTarget, diff;

    if (((DxGlobals *)dx)->gpuSync != 3) {

        if (((DxGlobals *)dx)->flushGpuQueryIssued) {
            qboolean finished = glTestFenceAPPLE(g_FenceID) != 0;
            if (finished)
                glDeleteFencesAPPLE(1, &g_FenceID);
            if (finished)
                ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
        }
        return;
    }

    startTime = (long long)rdtsc_lo() - (long long)((DxGlobals *)dx)->gpuSyncAlreadyWaited;
    ((DxGlobals *)dx)->gpuSyncAlreadyWaited = 0;
    waitedTime = 0;

    for (;;) {
        dx = (byte *)imp_dx;
        if (((DxGlobals *)dx)->flushGpuQueryIssued) {
            qboolean finished = glTestFenceAPPLE(g_FenceID) != 0;
            if (finished)
                glDeleteFencesAPPLE(1, &g_FenceID);
            if (finished) {
                ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
                dx = (byte *)imp_dx;
                break;
            }

            waitedTime = (long long)rdtsc_lo() - startTime;
            if (waitedTime > 0x7fffffffLL)
                return;
        } else {
            break;
        }
    }

    syncTarget = ((DxGlobals *)dx)->gpuSyncDelay;
    diff = syncTarget - (int)waitedTime;
    syncTarget -= (diff + ((unsigned int)diff >> 31)) >> 1;
    ((DxGlobals *)dx)->gpuSyncDelay = syncTarget;
}

void RB_AdaptiveGpuSyncTarget(void)
{
    byte *dx = (byte *)imp_dx;
    int val;

    if (((DxGlobals *)dx)->flushGpuQueryIssued) {
        qboolean finished = glTestFenceAPPLE(g_FenceID) != 0;
        if (finished)
            glDeleteFencesAPPLE(1, &g_FenceID);
        if (!finished)
            return;
        ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
    }

    val = ((DxGlobals *)dx)->gpuSyncDelay - 0x4e20;
    val &= ~(val >> 31);
    ((DxGlobals *)dx)->gpuSyncDelay = val;
}

static void RB_BACKEND_REGPARM1_ABI RB_EndBenchmarkGpu_impl(void *time)
{
    byte *dx;
    void *device;
    void **vtable;

    do {
        dx = (byte *)imp_dx;
        device = ((DxGlobals *)dx)->device;
        vtable = *(void ***)device;
        ((HRESULT(D3DVTCC *)(void *))(vtable[0xA8 / 4]))(device);
    } while (*(volatile int *)&alwaysfails);

    dx = (byte *)imp_dx;
    ((DxGlobals *)dx)->inScene = 0;

    while (((DxGlobals *)dx)->flushGpuQueryIssued) {
        qboolean finished = glTestFenceAPPLE(g_FenceID) != 0;
        if (finished)
            glDeleteFencesAPPLE(1, &g_FenceID);
        if (finished) {
            ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
            break;
        }
        dx = (byte *)imp_dx;
    }

    glGenFencesAPPLE(1, &g_FenceID);
    glSetFenceAPPLE(g_FenceID);
    dx = (byte *)imp_dx;
    ((DxGlobals *)dx)->flushGpuQueryIssued = 1;

    while (((DxGlobals *)dx)->flushGpuQueryIssued) {
        qboolean finished = glTestFenceAPPLE(g_FenceID) != 0;
        if (finished)
            glDeleteFencesAPPLE(1, &g_FenceID);
        if (finished) {
            ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
            break;
        }
        dx = (byte *)imp_dx;
    }

    QueryPerformanceCounter(time);
}

static void RB_BACKEND_REGPARM1_ABI RB_EndBenchmarkGpu(void *time)
{
    RB_EndBenchmarkGpu_impl(time);
}

static void RB_BACKEND_REGPARM1_ABI RB_BeginBenchmarkGpu_impl(void *time)
{
    byte *dx;
    void *device;
    void **vtable;

    dx = (byte *)imp_dx;
    while (((DxGlobals *)dx)->flushGpuQueryIssued) {
        qboolean finished = glTestFenceAPPLE(g_FenceID) != 0;
        if (finished)
            glDeleteFencesAPPLE(1, &g_FenceID);
        if (finished) {
            ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
            break;
        }
        dx = (byte *)imp_dx;
    }

    glGenFencesAPPLE(1, &g_FenceID);
    glSetFenceAPPLE(g_FenceID);
    dx = (byte *)imp_dx;
    ((DxGlobals *)dx)->flushGpuQueryIssued = 1;

    while (((DxGlobals *)dx)->flushGpuQueryIssued) {
        qboolean finished = glTestFenceAPPLE(g_FenceID) != 0;
        if (finished)
            glDeleteFencesAPPLE(1, &g_FenceID);
        if (finished) {
            ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
            break;
        }
        dx = (byte *)imp_dx;
    }

    QueryPerformanceCounter(time);

    dx = (byte *)imp_dx;
    ((DxGlobals *)dx)->inScene = 1;

    do {
        dx = (byte *)imp_dx;
        device = ((DxGlobals *)dx)->device;
        vtable = *(void ***)device;
        ((HRESULT(D3DVTCC *)(void *))(vtable[0xA4 / 4]))(device);
    } while (*(volatile int *)&alwaysfails);
}

static void RB_BACKEND_REGPARM1_ABI RB_BeginBenchmarkGpu(void *time)
{
    RB_BeginBenchmarkGpu_impl(time);
}

void RB_Set3D(void)
{
    const GfxViewParms *vp;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    backEnd.projection2D = 0;
    MatrixIdentity44(RB_GetActiveWorldMatrix());
    RB_ChangedWorldMatrix(1.0f);
    RB_SetMatricesForView(backEnd.viewParms);

    vp = backEnd.viewParms;

    backEnd.codeConsts[11][0] = vp->origin[0];
    backEnd.codeConsts[11][1] = vp->origin[1];
    backEnd.codeConsts[11][2] = vp->origin[2];
    backEnd.codeConsts[11][3] = 1.0f;

    backEnd.codeConsts[12][0] = vp->axis[0][0];
    backEnd.codeConsts[12][1] = vp->axis[0][1];
    backEnd.codeConsts[12][2] = vp->axis[0][2];
    backEnd.codeConsts[12][3] = 0.0f;

    backEnd.codeConsts[13][0] = vp->axis[1][0];
    backEnd.codeConsts[13][1] = vp->axis[1][1];
    backEnd.codeConsts[13][2] = vp->axis[1][2];
    backEnd.codeConsts[13][3] = 0.0f;

    backEnd.codeConsts[14][0] = vp->axis[2][0];
    backEnd.codeConsts[14][1] = vp->axis[2][1];
    backEnd.codeConsts[14][2] = vp->axis[2][2];
    backEnd.codeConsts[14][3] = 0.0f;
}

static void RB_SetMaterialColorCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd = (byte *)execState->cmd;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    *(int *)&backEnd.codeConsts[27][0] = *(int *)&((GfxCmdSetMaterialColor *)cmd)->color[0];
    *(int *)&backEnd.codeConsts[27][1] = *(int *)&((GfxCmdSetMaterialColor *)cmd)->color[1];
    *(int *)&backEnd.codeConsts[27][2] = *(int *)&((GfxCmdSetMaterialColor *)cmd)->color[2];
    *(int *)&backEnd.codeConsts[27][3] = *(int *)&((GfxCmdSetMaterialColor *)cmd)->color[3];

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_SetLightPropertiesCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd = (byte *)execState->cmd;
    int idx;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    idx = ((GfxCmdSetLightProperties *)cmd)->lightIndex;

    *(int *)&backEnd.light[idx].ambient[0] = *(int *)&((GfxCmdSetLightProperties *)cmd)->ambient[0];
    *(int *)&backEnd.light[idx].ambient[1] = *(int *)&((GfxCmdSetLightProperties *)cmd)->ambient[1];
    *(int *)&backEnd.light[idx].ambient[2] = *(int *)&((GfxCmdSetLightProperties *)cmd)->ambient[2];
    *(int *)&backEnd.light[idx].ambient[3] = *(int *)&((GfxCmdSetLightProperties *)cmd)->ambient[3];

    *(int *)&backEnd.light[idx].color[0] = *(int *)&((GfxCmdSetLightProperties *)cmd)->color[0];
    *(int *)&backEnd.light[idx].color[1] = *(int *)&((GfxCmdSetLightProperties *)cmd)->color[1];
    *(int *)&backEnd.light[idx].color[2] = *(int *)&((GfxCmdSetLightProperties *)cmd)->color[2];
    *(int *)&backEnd.light[idx].color[3] = *(int *)&((GfxCmdSetLightProperties *)cmd)->color[3];

    *(int *)&backEnd.light[idx].specular[0] = *(int *)&((GfxCmdSetLightProperties *)cmd)->specular[0];
    *(int *)&backEnd.light[idx].specular[1] = *(int *)&((GfxCmdSetLightProperties *)cmd)->specular[1];
    *(int *)&backEnd.light[idx].specular[2] = *(int *)&((GfxCmdSetLightProperties *)cmd)->specular[2];
    *(int *)&backEnd.light[idx].specular[3] = *(int *)&((GfxCmdSetLightProperties *)cmd)->specular[3];

    *(int *)&backEnd.light[idx].def = ((GfxCmdSetLightProperties *)cmd)->lightDef;

    *(int *)&backEnd.light[idx].position[0] = *(int *)&((GfxCmdSetLightProperties *)cmd)->position[0];
    *(int *)&backEnd.light[idx].position[1] = *(int *)&((GfxCmdSetLightProperties *)cmd)->position[1];
    *(int *)&backEnd.light[idx].position[2] = *(int *)&((GfxCmdSetLightProperties *)cmd)->position[2];
    *(int *)&backEnd.light[idx].position[3] = *(int *)&((GfxCmdSetLightProperties *)cmd)->position[3];

    memcpy(&backEnd.codeConsts[3 + idx], &backEnd.light[idx].position, 16);

    memcpy(&backEnd.codeConsts[7 + idx], &backEnd.light[idx].ambient, 16);

    memcpy(&backEnd.codeConsts[5 + idx], &backEnd.light[idx].color, 16);

    memcpy(&backEnd.codeConsts[9 + idx], &backEnd.light[idx].specular, 16);

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_SetStencilRefValueCmd(GfxRenderCommandExecState *execState)
{
    DxState *dxs = &dxState;
    byte *cmd = (byte *)execState->cmd;
    int stencilRef;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    stencilRef = ((GfxCmdSetStencilRefValue *)cmd)->refValue;

    if (dxs->stencilRefValue != stencilRef) {

        do {
            void *dev = ((DxGlobals *)imp_dx)->device;
            void **vt = *(void ***)dev;
            ((int(D3DVTCC *)(void *, int, int))vt[0xe4 / 4])(dev, 0x39, stencilRef);
        } while (*(int *)&alwaysfails);
        dxs->stencilRefValue = stencilRef;
    }

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_SetShadowCookieCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd = (byte *)execState->cmd;
    float shadowMapSize, invSize;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    RB_SetShadowLookupMatrix(cmd + 4);

    shadowMapSize = (float)dx.renderTargets[R_RENDERTARGET_SHADOWCOOKIE].width;
    invSize = 1.0f / shadowMapSize;

    backEnd.codeConsts[44][0] = shadowMapSize;
    backEnd.codeConsts[44][1] = shadowMapSize;
    backEnd.codeConsts[44][2] = invSize;
    backEnd.codeConsts[44][3] = invSize;

    backEnd.codeConsts[45][0] = 0.0f;
    backEnd.codeConsts[45][1] = 0.0f;
    backEnd.codeConsts[45][2] = 0.0f;
    *(int *)&backEnd.codeConsts[45][3] = *(int *)&((GfxCmdSetShadowCookie *)cmd)->fade;

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_BeginViewCmd(GfxRenderCommandExecState *execState)
{
    DxState *dxs = &dxState;
    const GfxCmdBeginView *cmd;
    const GfxViewParms *vp;
    const float *invVPMatrix;
    float invProjDist, twoInvProjDist;
    float pow2W, pow2H;
    float pixelScaleW, pixelScaleH;
    float scale;

    cmd = (const GfxCmdBeginView *)execState->cmd;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    backEnd.viewCount = ((GfxCmdBeginView *)cmd)->viewCount;
    backEnd.sceneDef = cmd->sceneDef;
    vp = cmd->viewParms;
    backEnd.viewParms = vp;
    backEnd.lodParms = cmd->lodParms;

    backEnd.sceneViewport.x = vp->viewport.X;
    backEnd.sceneViewport.y = vp->viewport.Y;
    backEnd.sceneViewport.width = vp->viewport.Width;
    backEnd.sceneViewport.height = vp->viewport.Height;

    backEnd.viewProjectionMatrix = vp->viewProjectionMatrix;

    backEnd.tileIndex = 0;
    backEnd.tileCount = 1;

    backEnd.width = ((const vidConfig_t *)imp_vidConfig)->width;
    backEnd.height = ((const vidConfig_t *)imp_vidConfig)->height;

    if (!dxs->viewportBehavior)
        RB_UpdateViewportConstants();

    vp = backEnd.viewParms;
    invVPMatrix = (const float *)&vp->inverseViewProjectionMatrix;

    invProjDist = 1.0f / invVPMatrix[15];
    {
        float cz = invProjDist * invVPMatrix[14] - vp->origin[2];
        float cy = invProjDist * invVPMatrix[13] - vp->origin[1];
        float cx = invProjDist * invVPMatrix[12] - vp->origin[0];
        backEnd.codeConsts[15][0] = cx;
        backEnd.codeConsts[15][1] = cy;
        backEnd.codeConsts[15][2] = cz;
    }
    backEnd.codeConsts[15][3] = 0.0f;

    twoInvProjDist = invProjDist * 2.0f;

    pow2W = (float)nextPow2((unsigned int)backEnd.width);
    pixelScaleW = 1.0f / ((float)backEnd.width / pow2W);
    scale = pixelScaleW * twoInvProjDist;
    backEnd.codeConsts[16][0] = scale * invVPMatrix[0];
    backEnd.codeConsts[16][1] = scale * invVPMatrix[1];
    backEnd.codeConsts[16][2] = scale * invVPMatrix[2];
    backEnd.codeConsts[16][3] = 0.0f;

    pow2H = (float)nextPow2((unsigned int)backEnd.height);
    pixelScaleH = 1.0f / ((float)backEnd.height / pow2H);
    scale = -(pixelScaleH * twoInvProjDist);
    backEnd.codeConsts[17][0] = scale * invVPMatrix[4];
    backEnd.codeConsts[17][1] = scale * invVPMatrix[5];
    backEnd.codeConsts[17][2] = scale * invVPMatrix[6];
    backEnd.codeConsts[17][3] = 0.0f;

    RB_Set3D();

    cmd = (const GfxCmdBeginView *)execState->cmd;
    execState->cmd = (const void *)((byte *)cmd + cmd->header.byteCount);
}

static void RB_SetViewportCmd(GfxRenderCommandExecState *execState)
{
    const GfxCmdSetViewport *cmd = (const GfxCmdSetViewport *)execState->cmd;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    backEnd.sceneViewport = cmd->viewport;
    backEnd.viewportIsDirty = 1;
    RB_UpdateViewportConstants();

    cmd = (const GfxCmdSetViewport *)execState->cmd;
    execState->cmd = (const void *)((byte *)cmd + cmd->header.byteCount);
}

static void RB_SetRenderTargetCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd = (byte *)execState->cmd;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    RB_SetRenderTarget(((GfxCmdSetRenderTarget *)cmd)->renderTargetId);

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

int rb_rdsl_diag = 0;
int rb_rdsl_call_count = 0;
static void RB_RenderDrawSurfList(GfxDrawSurf *drawSurfs, int drawSurfCount,
                                              MaterialTechniqueType techType, GfxDrawSurfOrder order)
{
    GfxDrawSurf *drawSurf;
    int byteStep, iteration;
    unsigned int prevSort, sortKey;
    const Material *materialPrev, *material;
    int lightmapPrev, lightmap;
    MaterialTechniqueType actualTechType, actualTechTypePrev;
    byte ignoreSurfs, isWorldSpaceEnt, isWorldSpaceEntPrev, entityMergable;
    int entityIndexPrev, entityIndex;
    int depthRange, depthRangePrev;
    float materialTime, materialTimePrev;
    GfxEntity *entities;

    rb_rdsl_call_count++;

    backEnd.currentEntity = &backEnd.worldEntity;
    backEnd.currentEntityLighting = NULL;

    if (order != 0) {
        drawSurf = (GfxDrawSurf *)((char *)drawSurfs + (drawSurfCount - 1) * 8);
        byteStep = -8;
    } else {
        drawSurf = drawSurfs;
        byteStep = 8;
    }

    if (drawSurfCount <= 0) {
        isWorldSpaceEntPrev = 1;
        depthRange = 0;
        goto cleanup;
    }

    actualTechTypePrev = techType;
    materialPrev = NULL;
    lightmapPrev = 0x1f;
    ignoreSurfs = 0;
    isWorldSpaceEnt = 1;
    entityIndexPrev = 0x7fe;
    depthRangePrev = 0;
    prevSort = 0xffffffff;
    materialTime = 0.0f;
    materialTimePrev = 3.4028234663852886e+38f;
    iteration = 0;
    depthRange = 0;
    isWorldSpaceEntPrev = 1;

    sortKey = drawSurf->sort;
    if (sortKey == prevSort)
        goto same_sort;
    goto sort_changed;

advance:
    iteration++;
    drawSurf = (GfxDrawSurf *)((char *)drawSurf + byteStep);
    if (iteration == drawSurfCount)
        goto cleanup;
    sortKey = drawSurf->sort;
    if (sortKey == prevSort)
        goto same_sort;

sort_changed:
    g_rdsl_sortchange++;
    prevSort = sortKey;
    R_DecomposeSort(sortKey, &entityIndex, &material, &lightmap);

    if (!material) {
        g_rdsl_ignore_technull++;
        goto ignore_surf;
    }
#ifdef GFX_REAL_D3D9
    if (getenv("REALD3D9_MATDIAG")) {
        static int md;
        if (md < 30) {
            const char *nm = material ? *(const char **)material : "(nomat)";
            const unsigned *w = (const unsigned *)material;
            fprintf(stderr, "[MATDIAG] surf#%d mat=%p name='%.24s' tt=%d | +30..+4c: %08x %08x %08x %08x %08x %08x %08x %08x\n",
                    md, (void *)material, (nm && (unsigned)nm > 0x10000) ? nm : "(badptr)", techType,
                    w[0x30 / 4], w[0x34 / 4], w[0x38 / 4], w[0x3c / 4], w[0x40 / 4], w[0x44 / 4], w[0x48 / 4], w[0x4c / 4]);
            fflush(stderr);
            md++;
        }
    }
#endif

    actualTechType = techType;
    if (techType == 6 && lightmap == 0x1f) {

        entities = backEnd.sceneDef.entities;
        if (entityIndex == 0x7fe) {
            actualTechType = 9;
        } else if (entityIndex == 0x7ff) {
            actualTechType = 15;
        } else {
            int reType = entities[entityIndex].reType;
            if (reType == 2)
                actualTechType = 12;
            else
                actualTechType = 9;
        }
        actualTechType += RB_FogOffset();
        goto have_tech;
    }
    if (techType != 6)
        goto non_lightmap;

non_lightmap:

    if (!r_depthPrepassModels->current.enabled) {
        if (techType == 0 || techType == 2) {

            entities = backEnd.sceneDef.entities;
            if (entityIndex == 0x7ff) {
                actualTechType = -1;
                goto have_tech;
            }
            if (entityIndex <= 0x7fd) {
                GfxEntity *ent = &entities[entityIndex];
                if (ent->reType <= 2 && !(*(byte *)((char *)ent + 5) & 1)) {
                    actualTechType = -1;
                    goto have_tech;
                }
            }
        }
    }

    if ((unsigned int)(techType - 3) <= 0x17) {
        actualTechType = techType + RB_FogOffset();
    } else {
        actualTechType = techType;
    }

have_tech:
    g_rdsl_bf_entry++;
    material = material;

    if (!r_drawDecals->current.enabled) {
        if (*(byte *)&material->stateBits[1] & 0x30) {
            g_rdsl_ignore_decal++;
            goto ignore_surf;
        }
    }

    if (actualTechType == (MaterialTechniqueType)-1) {
        g_rdsl_ignore_techm1++;
        goto ignore_surf;
    }

    {
        MaterialTechniqueSet *techSet = material->techniqueSet;
        MaterialTechnique *technique;
        if (!techSet) {
            g_rdsl_ignore_technull++;
#    ifdef GFX_REAL_D3D9
            if (getenv("REALD3D9_RDSL") && g_technull_saved < 8) {
                g_technull_saved++;
                fprintf(stderr, "[TECHNULL] techSet==NULL mat='%s' type=%d\n",
                        material->info.name ? material->info.name : "?", actualTechType);
            }
#    endif
            goto ignore_surf;
        }
        technique = techSet->techniques[actualTechType];
#ifdef GFX_REAL_D3D9
        if (!technique && getenv("REALD3D9_RDSL") && g_technull_saved2 < 12) {
            g_technull_saved2++;
            fprintf(stderr, "[TECHNULL] tech[%d]==NULL mat='%s' techset='%s'\n",
                    actualTechType, material->info.name ? material->info.name : "?",
                    techSet->name ? techSet->name : "?");
        }
#endif
        if (!technique) {
            g_rdsl_ignore_technull++;
            if (actualTechType == 6 && g_technull_saved < 30) {
                g_technull_mat = (void *)material;
                g_technull_type = actualTechType;
                g_technull_saved++;
            }
            goto ignore_surf;
        }
    }

    g_rdsl_noignore++;
    ignoreSurfs = 0;
    entities = backEnd.sceneDef.entities;

    if (entityIndex != entityIndexPrev) {

        if ((unsigned int)(entityIndex - 0x7fe) <= 1) {
            isWorldSpaceEnt = 1;
        } else {
            int reType = entities[entityIndex].reType;
            if (refEntIsInWorldSpace[reType])
                isWorldSpaceEnt = 1;
            else
                isWorldSpaceEnt = 0;
        }

        if (!isWorldSpaceEnt) {
            entityMergable = 0;
        } else if (isWorldSpaceEntPrev) {
            entityMergable = 1;
        } else {
            entityMergable = 0;
        }
    } else {
        entityMergable = 1;
        goto check_material;
    }

    if (entityIndex == 0x7fe) {
        materialTime = 0.0f;
    } else {
        materialTime = entities[entityIndex].materialTime;
    }

check_material:

    if (materialPrev == material) {
        if (lightmapPrev == lightmap &&
            actualTechTypePrev == actualTechType &&
            materialTime == materialTimePrev &&
            !(materialTime != materialTimePrev))
        {
            if (entityMergable)
                goto do_entity_setup;
        }
    }

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    if (materialTime != materialTimePrev || materialTime != materialTime) {
        float w = backEnd.sceneDef.floatTime - materialTime;
        float frac = w - floorf(w);
        int isDx7 = r_rendererInUse->current.integer == 2;
        if (isDx7) {
            backEnd.texScrollAmountDx7 = frac;
        } else {
            float radians = (float)((double)frac * 6.283185307179586);
            backEnd.codeConsts[2][1] = cosf(radians);
            backEnd.codeConsts[2][0] = sinf(radians);
            backEnd.codeConsts[2][2] = frac;
            backEnd.codeConsts[2][3] = w;
        }
    }

    RB_BeginSurface(material, actualTechType, lightmap);
    materialPrev = material;
    lightmapPrev = lightmap;
    materialTimePrev = materialTime;
    actualTechTypePrev = actualTechType;

do_entity_setup:

    if (entityIndexPrev == entityIndex)
        goto dispatch;

    if (entityIndex == 0x7fe || entityIndex == 0x7ff) {

        backEnd.currentEntity = &backEnd.worldEntity;
        backEnd.currentEntityLighting = NULL;
    } else {

        GfxEntity *ent = &entities[entityIndex];
        backEnd.currentEntity = ent;
        if (ent->reType <= 1) {

            backEnd.currentEntityLighting = &backEnd.entityLighting[entityIndex];
        } else {
            backEnd.currentEntityLighting = NULL;
        }
    }

    if (isWorldSpaceEnt) {

        if (!isWorldSpaceEntPrev)
            RB_PopMatrixStack();
        if (depthRangePrev) {
            RB_SetDepthRange(0.0f, 1.0f);
            depthRangePrev = 0;
        }
    } else {

        if (isWorldSpaceEntPrev)
            RB_PushMatrixStack();
        RB_SetWorldMatrixForEntity(backEnd.currentEntity);

        depthRange = backEnd.currentEntity->renderFxFlags & 0x18;

        if (depthRange != depthRangePrev) {

            if (tess.indexCount || tess.optimizedIndexCount) {
                int savedDecl = tess.declType;
                RB_EndSurface();
                RB_BeginSurface(tess.material, tess.techType, tess.lmapIndex);
                if (tess.declType != savedDecl) {
                    if (tess.indexCount || tess.optimizedIndexCount)
                        RB_EndSurface();
                    tess.declType = savedDecl;
                }
            }

            if (depthRange == 8) {

                RB_SetDepthHackNearClip(backEnd.viewParms->depthHackNearClip);
                RB_SetDepthRange(0.0f, 0.2f);
            } else if (depthRange == 0x10 || depthRange == 0x18) {
                RB_SetDepthRange(0.0f, 0.5f);
            } else if (depthRange == 0) {
                RB_SetDepthRange(0.0f, 1.0f);
            }

            if (depthRangePrev == 8) {
                float nc = ((const float *)&backEnd.viewParms->projectionMatrix)[14];

                *(int *)&nc ^= 0x80000000;
                RB_SetDepthHackNearClip(nc);
            }
            depthRangePrev = depthRange;
        }
    }

    entityIndexPrev = entityIndex;
    isWorldSpaceEntPrev = isWorldSpaceEnt;

dispatch:

    {
        const surfaceType_t *surfType = drawSurf->surface;
        int type = *(const int *)surfType;
        g_rb_tess_type_counts[type]++;
        g_rb_last_tess_type = type;
        g_tess_since_begin++;
        rb_tessTable[type](surfType);
    }
    goto advance;

ignore_surf:
    ignoreSurfs = 1;
    goto advance;

same_sort:
    if (ignoreSurfs)
        goto advance;
    goto dispatch;

cleanup:

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    if (depthRange) {
        RB_SetDepthRange(0.0f, 1.0f);
        if (depthRange == 8) {
            float nc = ((const float *)&backEnd.viewParms->projectionMatrix)[14];
            *(int *)&nc ^= 0x80000000;
            RB_SetDepthHackNearClip(nc);
        }
    }

    if (!isWorldSpaceEntPrev)
        RB_PopMatrixStack();

    backEnd.currentEntity = NULL;
    backEnd.currentEntityLighting = NULL;
}

static void RB_DrawSurfsCmd(GfxRenderCommandExecState *execState)
{
    DxState *dxs = &dxState;
    byte *cmd;

    cmd = (byte *)execState->cmd;

    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    if (backEnd.projection2D)
        RB_Set3D();

    if (backEnd.viewportIsDirty)
        RB_UpdateViewport();

    if (dxs->viewportIsNull)
        return;

    RB_RenderDrawSurfList(
        ((GfxCmdDrawSurfs *)cmd)->drawSurfs,
        ((GfxCmdDrawSurfs *)cmd)->drawSurfCount,
        ((GfxCmdDrawSurfs *)cmd)->techType,
        ((GfxCmdDrawSurfs *)cmd)->order);
}

static void RB_DrawSunPostEffectsCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd = (byte *)execState->cmd;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    RB_DrawSunPostEffects((const void *)(uintptr_t)((GfxCmdDrawSunPostEffects *)cmd)->viewIndex);

    cmd = (byte *)execState->cmd;
    {
        unsigned int byteCount = ((const GfxCmdHeader *)cmd)->byteCount;
        execState->cmd = (const void *)(cmd + byteCount);
    }
}

static void RB_Set2D(void)
{
    char *be = (char *)&backEnd;
    int viewport[4];
    float transform[16];
    float identity[16];
    float invW, invH;
    int stackIdx;
    char *am;
    int isDx7;
    int i;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    backEnd.projection2D = 1;

    if (!RB_GetViewport(viewport))
        return;

    invW = 1.0f / (float)viewport[2];
    invH = 1.0f / (float)viewport[3];
    for (i = 0; i < 16; i++)
        transform[i] = 0.0f;
    transform[0] = 2.0f * invW;
    transform[5] = -2.0f * invH;
    transform[10] = 1.0f;
    transform[12] = -1.0f;
    transform[13] = 1.0f;
    transform[15] = 1.0f;

    MatrixIdentity44(identity);

    stackIdx = backEnd.codeMatrixStackLevel;
    am = (char *)&backEnd.codeMatrixStack[stackIdx];

    {
        GfxCodeMatrices *cm = (GfxCodeMatrices *)am;
        for (i = 0; i < 16; i++)
            *(int *)&((float *)&cm->projection.matrix[0])[i] = *(int *)&transform[i];
        cm->projection.valid[0] = 1;

        cm->projection.valid[1] = 0;
        cm->projection.valid[2] = 0;
        cm->projection.valid[3] = 0;

        for (i = 0; i < 4; i++) {
            memcpy(&cm->world.matrix[i], identity, 64);
            cm->world.valid[i] = 1;
        }
    }

    memcpy(((char *)am + offsetof(GfxCodeMatrices, view)), ((char *)am + offsetof(GfxCodeMatrices, world)), 0x110);
    memcpy(((char *)am + offsetof(GfxCodeMatrices, worldView)), ((char *)am + offsetof(GfxCodeMatrices, world)), 0x110);
    memcpy(((char *)am + offsetof(GfxCodeMatrices, viewProjection)), ((char *)am + offsetof(GfxCodeMatrices, projection)), 0x110);
    memcpy(((char *)am + offsetof(GfxCodeMatrices, worldViewProjection)), ((char *)am + offsetof(GfxCodeMatrices, projection)), 0x110);

    {
        float OGLView[16], OGLProjection[16], OGLWorldView[16];

        memcpy(OGLView, ((char *)am + offsetof(GfxCodeMatrices, view)), 64);

        memcpy(OGLProjection, ((char *)am + offsetof(GfxCodeMatrices, projection)), 64);

        OGLView[2] = -OGLView[2];
        OGLView[6] = -OGLView[6];
        OGLView[10] = -OGLView[10];
        OGLView[14] = -OGLView[14];

        MacOpenGLUtils_ConvertD3DProjectionMatrixToOpenGL(
            OGLProjection, (float)viewport[2], (float)viewport[3]);

        MatrixMultiply44(((char *)am + offsetof(GfxCodeMatrices, world)), OGLView, OGLWorldView);

        MatrixMultiply44(OGLWorldView, OGLProjection, ((char *)am + offsetof(GfxCodeMatrices, OGLworldViewProjection)));

        memcpy(((char *)am + offsetof(GfxCodeMatrices, normalizedWorld)), ((char *)am + offsetof(GfxCodeMatrices, world)), 0x110);
        memcpy(((char *)am + offsetof(GfxCodeMatrices, normalizedWorldView)), ((char *)am + offsetof(GfxCodeMatrices, worldView)), 0x110);
        memcpy(am + 0x890, ((char *)am + offsetof(GfxCodeMatrices, worldView)), 0x110);
    }

    isDx7 = r_rendererInUse->current.integer == 2;
    if (isDx7) {
        char *dx = (char *)imp_dx;

        do {
            void *device = ((DxGlobals *)dx)->device;
            void **vtable = *(void ***)device;
            ((int(__attribute__((stdcall)) *)(void *, int, const void *))vtable[0xb0 / 4])(
                device, 0x100, identity);
        } while (*(int *)&alwaysfails);
        do {
            void *device = ((DxGlobals *)dx)->device;
            void **vtable = *(void ***)device;
            ((int(__attribute__((stdcall)) *)(void *, int, const void *))vtable[0xb0 / 4])(
                device, 2, identity);
        } while (*(int *)&alwaysfails);
        do {
            void *device = ((DxGlobals *)dx)->device;
            void **vtable = *(void ***)device;
            ((int(__attribute__((stdcall)) *)(void *, int, const void *))vtable[0xb0 / 4])(
                device, 3, transform);
        } while (*(int *)&alwaysfails);
    }

    backEnd.codeConsts[11][0] = 0.0f;
    backEnd.codeConsts[11][1] = 0.0f;
    backEnd.codeConsts[11][2] = 1.0f;
    backEnd.codeConsts[11][3] = 1.0f;

    backEnd.codeConsts[12][0] = 0.0f;
    backEnd.codeConsts[12][1] = 0.0f;
    backEnd.codeConsts[12][2] = 1.0f;
    backEnd.codeConsts[12][3] = 0.0f;

    backEnd.codeConsts[13][0] = 1.0f;
    backEnd.codeConsts[13][1] = 0.0f;
    backEnd.codeConsts[13][2] = 0.0f;
    backEnd.codeConsts[13][3] = 0.0f;

    backEnd.codeConsts[14][0] = 0.0f;
    backEnd.codeConsts[14][1] = 1.0f;
    backEnd.codeConsts[14][2] = 0.0f;
    backEnd.codeConsts[14][3] = 0.0f;
}

static void RB_DrawTrianglesCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd;
    char *t = (char *)&tess;
    const Material *triMaterial;
    MaterialTechniqueType techType;
    int vertexCount, indexCount;
    const float *xyzwData, *normalData, *stData;
    const int *colorData;
    const unsigned short *indexData;
    int isDx7;
    int vc, ic;
    r_index_t *indices;
    int savedDecl;
    int i;

    cmd = (byte *)execState->cmd;

    triMaterial = ((GfxCmdDrawTriangles *)cmd)->material;
    techType = ((GfxCmdDrawTriangles *)cmd)->techType;
    indexCount = ((GfxCmdDrawTriangles *)cmd)->indexCount;
    vertexCount = ((GfxCmdDrawTriangles *)cmd)->vertexCount;

    {
        int normalOfs = 0x10 + vertexCount * 16;
        int colorOfs = normalOfs + vertexCount * 12;
        int stOfs = colorOfs + vertexCount * 4;
        int indexOfs = stOfs + vertexCount * 8;
        xyzwData = (const float *)(cmd + 0x10);
        normalData = (const float *)(cmd + normalOfs);
        colorData = (const int *)(cmd + colorOfs);
        stData = (const float *)(cmd + stOfs);
        indexData = (const unsigned short *)(cmd + indexOfs);
    }

    if (tess.indexCount != 0 || tess.optimizedIndexCount != 0)
        RB_EndSurface();
    if (!backEnd.projection2D)
        RB_Set3D();

    if (triMaterial != tess.material || techType != tess.techType) {
        if (tess.indexCount != 0 || tess.optimizedIndexCount != 0)
            RB_EndSurface();
        RB_BeginSurface(triMaterial, techType, 0x1f);
    }

    vc = tess.vertexCount;
    if (vc + vertexCount > 0x154a ||
        tess.indexCount + indexCount > 0x100000) {
        savedDecl = tess.declType;
        RB_EndSurface();
        RB_BeginSurface(tess.material, tess.techType, tess.lmapIndex);
        if (tess.declType != savedDecl) {
            if (tess.indexCount != 0 || tess.optimizedIndexCount != 0)
                RB_EndSurface();
            tess.declType = savedDecl;
        }
        vc = tess.vertexCount;
    }

    ic = tess.indexCount;
    indices = tess.indices;
    for (i = 0; i < indexCount; i++)
        indices[ic + i] = (r_index_t)(vc + indexData[i]);

    isDx7 = r_rendererInUse->current.integer == 2;

    for (i = 0; i < vertexCount; i++) {
        float px = xyzwData[i * 4 + 0];
        float py = xyzwData[i * 4 + 1];
        float pz = xyzwData[i * 4 + 2];
        float pw = xyzwData[i * 4 + 3];
        int nx = *(const int *)&normalData[i * 3 + 0];
        int ny = *(const int *)&normalData[i * 3 + 1];
        float nzf = normalData[i * 3 + 2];
        int color = colorData[i];
        float s = stData[i * 2 + 0];
        float tt = stData[i * 2 + 1];
        int vi = vc + i;

        if (isDx7) {

            char *v = t + vi * 36;
            ((GfxVertexDx7 *)v)->xyz[0] = px / pw;
            ((GfxVertexDx7 *)v)->xyz[1] = py / pw;
            ((GfxVertexDx7 *)v)->xyz[2] = pz / pw;
            ((GfxVertexDx7 *)v)->normal[0] = nx;
            ((GfxVertexDx7 *)v)->normal[1] = ny;
            ((GfxVertexDx7 *)v)->normal[2] = nzf;
            ((GfxVertexDx7 *)v)->color.packed = color;
            ((GfxVertexDx7 *)v)->texCoord[0] = s;
            ((GfxVertexDx7 *)v)->texCoord[1] = tt;
        } else {

            char *v = t + vi * 64;
            ((GfxVertexDx7 *)v)->xyz[0] = px;
            ((GfxVertexDx7 *)v)->xyz[1] = py;
            ((GfxVertexDx7 *)v)->xyz[2] = pz;
            ((GfxVertex *)v)->xyzw[3] = pw;
            ((GfxVertex *)v)->normal[0] = nx;
            ((GfxVertex *)v)->normal[1] = ny;
            ((GfxVertex *)v)->normal[2] = nzf;
            ((GfxVertex *)v)->color.packed = color;
            ((GfxVertex *)v)->texCoord[0] = s;
            ((GfxVertex *)v)->texCoord[1] = tt;

            ((GfxVertex *)v)->tangent[0] = 1.0f;
            ((GfxVertex *)v)->tangent[1] = 0;
            ((GfxVertex *)v)->tangent[2] = 0;

            ((GfxVertex *)v)->binormal[0] = 0;
            ((GfxVertex *)v)->binormal[1] = 1.0f;
            ((GfxVertex *)v)->binormal[2] = 0;
        }
    }

    tess.indexCount += indexCount;
    tess.vertexCount = vc + vertexCount;

    RB_EndSurface();

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_SaveScreenCmd(GfxRenderCommandExecState *execState)
{
    DxState *dxs = &dxState;
    char *dx;
    void *imageSurface;
    byte *cmd;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    dx = (char *)imp_dx;
    imageSurface = Image_GetSurface(((DxGlobals *)dx)->renderTargets[7].image);

    do {
        void *device = ((DxGlobals *)dx)->device;
        void **vtable = *(void ***)device;
        void *backBuffer = dxs->renderTargetSurface;
        ((int(__attribute__((stdcall)) *)(void *, void *, void *, void *, void *, int))vtable[0x88 / 4])(
            device, backBuffer, NULL, imageSurface, NULL, 2);
    } while (*(int *)&alwaysfails);

    do {
        ((int(__attribute__((stdcall)) *)(void *))((*(void ***)imageSurface)[2]))(imageSurface);
    } while (*(int *)&alwaysfails);

    rgp.savedScreenTime = backEnd.sceneDef.time;

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_ApplyEarlyPostEffectsCmd(GfxRenderCommandExecState *execState)
{
    DxGlobals *dxg = (DxGlobals *)imp_dx;
    DxState *dxs = &dxState;
    int needCopy;
    byte *cmd;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    backEnd.resolvedPostSunTarget = 0xe;

    needCopy = r_distortion->current.enabled;

    if (!needCopy) {

        int isDx7 = r_rendererInUse->current.integer == 2;
        if (!isDx7 && dxg->hasSeparateAlphaBlend) {
            if (RB_DVAR(imp_r_glow)->current.enabled) {
                needCopy = !RB_DVAR(imp_r_fullbright)->current.enabled;
            }
        }
    }

    if (needCopy) {
        char *dx = (char *)imp_dx;
        void *imageSurface = Image_GetSurface(((DxGlobals *)dx)->renderTargets[1].image);

        do {
            void *device = ((DxGlobals *)dx)->device;
            void **vtable = *(void ***)device;
            void *backBuffer = dxs->renderTargetSurface;
            ((int(__attribute__((stdcall)) *)(void *, void *, void *, void *, void *, int))vtable[0x88 / 4])(
                device, backBuffer, NULL, imageSurface, NULL, 2);
        } while (*(int *)&alwaysfails);

        do {
            ((int(__attribute__((stdcall)) *)(void *))((*(void ***)imageSurface)[2]))(imageSurface);
        } while (*(int *)&alwaysfails);

        backEnd.resolvedPostSunTarget = 1;
    }

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_DrawSpriteCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd;
    const Material *spriteMaterial;
    char *t = (char *)&tess;
    byte entity[0x74];

    cmd = (byte *)execState->cmd;

    if (!backEnd.projection2D)
        RB_Set3D();

    spriteMaterial = (const Material *)((GfxCmdDrawSprite *)cmd)->material;
    if (!spriteMaterial) {
        execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
        return;
    }

    if (spriteMaterial != tess.material || tess.techType != 3) {
        if (tess.indexCount != 0 || tess.optimizedIndexCount != 0)
            RB_EndSurface();
        RB_BeginSurface(spriteMaterial, 3, 0x1f);
    }

    memset(entity, 0, sizeof(GfxEntity));

    ((GfxEntity *)entity)->origin[0] = ((GfxCmdDrawSprite *)cmd)->pos[0];
    ((GfxEntity *)entity)->origin[1] = ((GfxCmdDrawSprite *)cmd)->pos[1];
    ((GfxEntity *)entity)->origin[2] = ((GfxCmdDrawSprite *)cmd)->pos[2];

    ((GfxEntity *)entity)->customMaterial = (int)(uintptr_t)((GfxCmdDrawSprite *)cmd)->material;

    ((GfxEntity *)entity)->reType = 4;

    ((GfxEntity *)entity)->renderFxFlags = ((GfxCmdDrawSprite *)cmd)->renderFxFlags;

    ((GfxEntity *)entity)->radius[0] = ((GfxCmdDrawSprite *)cmd)->radius;
    ((GfxEntity *)entity)->radius[1] = ((GfxCmdDrawSprite *)cmd)->radius;

    ((GfxEntity *)entity)->minScreenRadius = ((GfxCmdDrawSprite *)cmd)->minScreenRadius;

    *(unsigned int *)((GfxEntity *)entity)->materialRGBA = ((GfxCmdDrawSprite *)cmd)->rgbaColor.packed;

    RB_TessEntity(entity);

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static inline void RB_SetLineVertex(char *tessBase, int vertIndex, int isDx7,
                                    float px, float py, int zBits, float s, float t, D3DCOLOR color)
{
    if (isDx7) {
        char *v = tessBase + vertIndex * 36;
        ((GfxVertexDx7 *)v)->xyz[0] = px;
        ((GfxVertexDx7 *)v)->xyz[1] = py;
        ((GfxVertexDx7 *)v)->xyz[2] = zBits;
        ((GfxVertexDx7 *)v)->normal[0] = 0;
        ((GfxVertexDx7 *)v)->normal[1] = 0;
        ((GfxVertexDx7 *)v)->normal[2] = 1.0f;
        ((GfxVertexDx7 *)v)->color.packed = color;
        ((GfxVertexDx7 *)v)->texCoord[0] = s;
        ((GfxVertexDx7 *)v)->texCoord[1] = t;
    } else {
        char *v = tessBase + vertIndex * 64;
        ((GfxVertexDx7 *)v)->xyz[0] = px;
        ((GfxVertexDx7 *)v)->xyz[1] = py;
        ((GfxVertexDx7 *)v)->xyz[2] = zBits;
        ((GfxVertexDx7 *)v)->normal[0] = 1.0f;
        ((GfxVertex *)v)->normal[0] = 0;
        ((GfxVertex *)v)->normal[1] = 0;
        ((GfxVertex *)v)->normal[2] = 1.0f;
        ((GfxVertex *)v)->color.packed = color;
        ((GfxVertex *)v)->texCoord[0] = s;
        ((GfxVertex *)v)->texCoord[1] = t;
        ((GfxVertex *)v)->binormal[0] = 0;
        ((GfxVertex *)v)->binormal[1] = 1.0f;
        ((GfxVertex *)v)->binormal[2] = 0;
        ((GfxVertex *)v)->tangent[0] = 1.0f;
        ((GfxVertex *)v)->tangent[1] = 0;
        ((GfxVertex *)v)->tangent[2] = 0;
    }
}

static inline void RB_WriteLineQuadIndices(char *t, int vc)
{
    int ic = tess.indexCount;
    r_index_t *indices = tess.indices;
    indices[ic + 0] = (r_index_t)(vc + 1);
    indices[ic + 1] = (r_index_t)vc;
    indices[ic + 2] = (r_index_t)(vc + 2);
    indices[ic + 3] = (r_index_t)(vc + 2);
    indices[ic + 4] = (r_index_t)vc;
    indices[ic + 5] = (r_index_t)(vc + 3);
}

void RB_DrawLines2D(int count, int width, const GfxPointVertex *verts)
{
    char *t = (char *)&tess;
    const Material *lineMaterial;
    int lineIdx;
    int isDx7;

    (void)width;

    lineMaterial = rgp.whiteMaterial;
    RB_BeginSurface2D(t, lineMaterial);

    if (count <= 0)
        return;

    isDx7 = r_rendererInUse->current.integer == 2;

    for (lineIdx = 0; lineIdx < count; lineIdx++) {
        const GfxPointVertex *p0 = &verts[lineIdx * 2];
        const GfxPointVertex *p1 = &verts[lineIdx * 2 + 1];
        float delta[2];
        float x0, y0, x1, y1;
        int z0, z1;
        D3DCOLOR c0, c1;
        int vc;

        delta[0] = p1->xyz[1] - p0->xyz[1];
        delta[1] = p0->xyz[0] - p1->xyz[0];
        Vec2Normalize(delta);
        delta[0] *= 0.5f;
        delta[1] *= 0.5f;

        vc = RB_CheckTessOverflow4(t);

        RB_WriteLineQuadIndices(t, vc);
        tess.indexCount += 6;

        x0 = p0->xyz[0];
        y0 = p0->xyz[1];
        x1 = p1->xyz[0];
        y1 = p1->xyz[1];
        z0 = *(int *)&p0->xyz[2];
        z1 = *(int *)&p1->xyz[2];
        c0 = *(D3DCOLOR *)p0->color;
        c1 = *(D3DCOLOR *)p1->color;

        RB_SetLineVertex(t, vc + 0, isDx7, x0 - delta[0], y0 - delta[1], z0, 0.0f, 0.0f, c0);
        RB_SetLineVertex(t, vc + 1, isDx7, x1 - delta[0], y1 - delta[1], z1, 0.0f, 1.0f, c1);
        RB_SetLineVertex(t, vc + 2, isDx7, x1 + delta[0], y1 + delta[1], z1, 1.0f, 1.0f, c1);
        RB_SetLineVertex(t, vc + 3, isDx7, x0 + delta[0], y0 + delta[1], z0, 1.0f, 0.0f, c0);

        tess.vertexCount += 4;
    }
}

static inline void RB_SetVertex3DWorld(char *tessBase, int vertIndex, int isDx7,
                                       float px, float py, float pz,
                                       float s, float t, D3DCOLOR color)
{
    if (isDx7) {
        char *v = tessBase + vertIndex * 36;
        ((GfxVertexDx7 *)v)->xyz[0] = px;
        ((GfxVertexDx7 *)v)->xyz[1] = py;
        ((GfxVertexDx7 *)v)->xyz[2] = pz;
        ((GfxVertexDx7 *)v)->normal[0] = 0;
        ((GfxVertexDx7 *)v)->normal[1] = 0;
        ((GfxVertexDx7 *)v)->normal[2] = 1.0f;
        ((GfxVertexDx7 *)v)->color.packed = color;
        ((GfxVertexDx7 *)v)->texCoord[0] = s;
        ((GfxVertexDx7 *)v)->texCoord[1] = t;
    } else {
        char *v = tessBase + vertIndex * 64;
        ((GfxVertexDx7 *)v)->xyz[0] = px;
        ((GfxVertexDx7 *)v)->xyz[1] = py;
        ((GfxVertexDx7 *)v)->xyz[2] = pz;
        ((GfxVertexDx7 *)v)->normal[0] = 1.0f;
        ((GfxVertex *)v)->normal[0] = 0;
        ((GfxVertex *)v)->normal[1] = 0;
        ((GfxVertex *)v)->normal[2] = 1.0f;
        ((GfxVertex *)v)->color.packed = color;
        ((GfxVertex *)v)->texCoord[0] = s;
        ((GfxVertex *)v)->texCoord[1] = t;
        ((GfxVertex *)v)->binormal[0] = 0;
        ((GfxVertex *)v)->binormal[1] = 1.0f;
        ((GfxVertex *)v)->binormal[2] = 0;
        ((GfxVertex *)v)->tangent[0] = 1.0f;
        ((GfxVertex *)v)->tangent[1] = 0;
        ((GfxVertex *)v)->tangent[2] = 0;
    }
}

void RB_DrawTextInSpace(const char *text, FontHandle font, const vec_t *org, const vec_t *xPixelStep, const vec_t *yPixelStep, D3DCOLOR color)
{
    char *t = (char *)&tess;
    const Material *material;
    float startX, startY, startZ;
    int isDx7;
    int (*Q_ReadToken)(const char **, int);

    material = (const Material *)font->material;

    RB_SetMatricesForView(backEnd.viewParms);

    startX = org[0] - 0.5f * xPixelStep[0] - 0.5f * yPixelStep[0];
    startY = org[1] - 0.5f * xPixelStep[1] - 0.5f * yPixelStep[1];
    startZ = org[2] - 0.5f * xPixelStep[2] - 0.5f * yPixelStep[2];

    Q_ReadToken = (int (*)(const char **, int))ri.SEH_ReadCharFromString;

    isDx7 = r_rendererInUse->current.integer == 2;

    while (*text) {
        int charCode;
        Glyph *glyph;
        float posX, posY, posZ;
        float wdX, wdY, wdZ;
        float htX, htY, htZ;
        int vc;

        charCode = Q_ReadToken(&text, 0);
        glyph = R_GetCharacterGlyph(font, charCode);

        {
            float x0 = (float)(signed char)glyph->x0;
            float y0 = (float)(signed char)glyph->y0;
            posX = startX + x0 * xPixelStep[0] + y0 * yPixelStep[0];
            posY = startY + x0 * xPixelStep[1] + y0 * yPixelStep[1];
            posZ = startZ + x0 * xPixelStep[2] + y0 * yPixelStep[2];
        }

        {
            float pw = (float)glyph->pixelWidth;
            float ph = (float)glyph->pixelHeight;
            wdX = pw * xPixelStep[0];
            wdY = pw * xPixelStep[1];
            wdZ = pw * xPixelStep[2];
            htX = ph * yPixelStep[0];
            htY = ph * yPixelStep[1];
            htZ = ph * yPixelStep[2];
        }

        if (material != tess.material || tess.techType != 3) {
            if (tess.indexCount != 0 || tess.optimizedIndexCount != 0)
                RB_EndSurface();
            RB_BeginSurface(material, 3, 0x1f);
        }

        vc = RB_CheckTessOverflow4(t);
        tess.vertexCount = vc + 4;
        tess.indexCount += 6;
        RB_WriteQuadIndices(t, vc);

        RB_SetVertex3DWorld(t, vc + 0, isDx7,
                            posX, posY, posZ,
                            glyph->s0, glyph->t0, color);
        RB_SetVertex3DWorld(t, vc + 1, isDx7,
                            posX + wdX, posY + wdY, posZ + wdZ,
                            glyph->s1, glyph->t0, color);
        RB_SetVertex3DWorld(t, vc + 2, isDx7,
                            posX + wdX + htX, posY + wdY + htY, posZ + wdZ + htZ,
                            glyph->s1, glyph->t1, color);
        RB_SetVertex3DWorld(t, vc + 3, isDx7,
                            posX + htX, posY + htY, posZ + htZ,
                            glyph->s0, glyph->t1, color);

        {
            float dx = (float)glyph->dx;
            startX += dx * xPixelStep[0];
            startY += dx * xPixelStep[1];
            startZ += dx * xPixelStep[2];
        }
    }
}

static void RB_DrawTextInSpaceCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd = (byte *)execState->cmd;
    unsigned int byteCount;

    RB_DrawTextInSpace(
        ((GfxCmdDrawTextInSpace *)cmd)->text,
        ((GfxCmdDrawTextInSpace *)cmd)->font,
        ((GfxCmdDrawTextInSpace *)cmd)->org,
        ((GfxCmdDrawTextInSpace *)cmd)->xPixelStep,
        ((GfxCmdDrawTextInSpace *)cmd)->yPixelStep,
        ((GfxCmdDrawTextInSpace *)cmd)->color.packed);

    cmd = (byte *)execState->cmd;
    byteCount = ((const GfxCmdHeader *)cmd)->byteCount;
    execState->cmd = (const void *)(cmd + byteCount);
}

static float RB_BACKEND_REGPARM2_ABI RB_TestFillPass3D_impl(const Material *material, MaterialTechniqueType techType)
{
    char *t = (char *)&tess;
    float origin[3] = { 0.0f, 0.0f, 0.0f };
    float axis[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    float view[16];
    float projection[16];
    long long beginTime, endTime, frequency;
    int isDx7, iterationCount, i;
    D3DCOLOR white = 0xffffffff;
    float x0, y0, z0, x, y, z;

    MatrixForViewer(view, origin, axis);
    InfinitePerspectiveMatrix(projection, 90.0f, 90.0f, 0.9f);
    RB_Set3D();
    RB_PushMatrixStack();
    MatrixIdentity44(RB_GetActiveWorldMatrix());
    RB_ChangedWorldMatrix(1.0f);
    RB_SetViewMatrix(view);
    RB_SetProjectionMatrix(projection);

    backEnd.currentEntity = &backEnd.worldEntity;
    backEnd.currentEntityLighting = NULL;

    backEnd.codeConsts[5][0] = 1.0f;
    backEnd.codeConsts[5][1] = 1.0f;
    backEnd.codeConsts[5][2] = 1.0f;
    backEnd.codeConsts[5][3] = 1.0f;

    backEnd.codeConsts[3][0] = 0.0f;
    backEnd.codeConsts[3][1] = 0.0f;
    backEnd.codeConsts[3][2] = 0.0f;
    backEnd.codeConsts[3][3] = 1.0f;

    backEnd.codeConsts[11][0] = 0.0f;
    backEnd.codeConsts[11][1] = 0.0f;
    backEnd.codeConsts[11][2] = 0.0f;
    backEnd.codeConsts[11][3] = 1.0f;

    RB_BeginSurface(material, techType, 0);

    origin[0] += axis[0];
    origin[1] += axis[1];
    origin[2] += axis[2];

    isDx7 = r_rendererInUse->current.integer == 2;
    x0 = origin[0];
    y0 = origin[1];
    z0 = origin[2];

    x = x0 + axis[3] + axis[6];
    y = y0 + axis[4] + axis[7];
    z = z0 + axis[5] + axis[8];
    if (isDx7) {
        ((GfxVertexDx7 *)t)->xyz[0] = x;
        ((GfxVertexDx7 *)t)->xyz[1] = y;
        ((GfxVertexDx7 *)t)->xyz[2] = z;
        ((GfxVertexDx7 *)t)->normal[0] = 0;
        ((GfxVertexDx7 *)t)->normal[1] = 0;
        ((GfxVertexDx7 *)t)->normal[2] = 1.0f;
        ((GfxVertexDx7 *)t)->color.packed = white;
        ((GfxVertexDx7 *)t)->texCoord[0] = 0;
        ((GfxVertexDx7 *)t)->texCoord[1] = 0;
    } else {
        ((GfxVertex *)t)->xyzw[0] = x;
        ((GfxVertex *)t)->xyzw[1] = y;
        ((GfxVertex *)t)->xyzw[2] = z;
        ((GfxVertex *)t)->xyzw[3] = 1.0f;
        ((GfxVertex *)t)->normal[0] = 0;
        ((GfxVertex *)t)->normal[1] = 0;
        ((GfxVertex *)t)->normal[2] = 1.0f;
        ((GfxVertex *)t)->color.packed = white;
        ((GfxVertex *)t)->texCoord[0] = 0;
        ((GfxVertex *)t)->texCoord[1] = 0;
        ((GfxVertex *)t)->tangent[0] = 1.0f;
        ((GfxVertex *)t)->tangent[1] = 0;
        ((GfxVertex *)t)->tangent[2] = 0;
        ((GfxVertex *)t)->binormal[0] = 0;
        ((GfxVertex *)t)->binormal[1] = 1.0f;
        ((GfxVertex *)t)->binormal[2] = 0;
    }

    x = x0 - axis[3] + axis[6];
    y = y0 - axis[4] + axis[7];
    z = z0 - axis[5] + axis[8];
    if (isDx7) {
        ((GfxVertexDx7 *)t)->xyz[0] = x;
        ((GfxVertexDx7 *)t)->xyz[1] = y;
        ((GfxVertexDx7 *)t)->xyz[2] = z;
        ((GfxVertexDx7 *)t)->normal[0] = 0;
        ((GfxVertexDx7 *)t)->normal[1] = 0;
        ((GfxVertexDx7 *)t)->normal[2] = 1.0f;
        ((GfxVertexDx7 *)t)->color.packed = white;
        ((GfxVertexDx7 *)t)->texCoord[0] = 1.0f;
        ((GfxVertexDx7 *)t)->texCoord[1] = 0;
    } else {
        ((GfxVertex *)t)->xyzw[0] = x;
        ((GfxVertex *)t)->xyzw[1] = y;
        ((GfxVertex *)t)->xyzw[2] = z;
        ((GfxVertex *)t)->xyzw[3] = 1.0f;
        ((GfxVertex *)t)->normal[0] = 0;
        ((GfxVertex *)t)->normal[1] = 0;
        ((GfxVertex *)t)->normal[2] = 1.0f;
        ((GfxVertex *)t)->color.packed = white;
        ((GfxVertex *)t)->texCoord[0] = 1.0f;
        ((GfxVertex *)t)->texCoord[1] = 0;
    }

    x = x0 - axis[3] - axis[6];
    y = y0 - axis[4] - axis[7];
    z = z0 - axis[5] - axis[8];
    if (isDx7) {
        ((GfxVertexDx7 *)t)->xyz[0] = x;
        ((GfxVertexDx7 *)t)->xyz[1] = y;
        ((GfxVertexDx7 *)t)->xyz[2] = z;
        ((GfxVertexDx7 *)t)->normal[0] = 0;
        ((GfxVertexDx7 *)t)->normal[1] = 0;
        ((GfxVertexDx7 *)t)->normal[2] = 1.0f;
        ((GfxVertexDx7 *)t)->color.packed = white;
        ((GfxVertexDx7 *)t)->texCoord[0] = 1.0f;
        ((GfxVertexDx7 *)t)->texCoord[1] = 1.0f;
    } else {
        ((GfxVertex *)t)->xyzw[0] = x;
        ((GfxVertex *)t)->xyzw[1] = y;
        ((GfxVertex *)t)->xyzw[2] = z;
        ((GfxVertex *)t)->xyzw[3] = 1.0f;
        ((GfxVertex *)t)->normal[0] = 0;
        ((GfxVertex *)t)->normal[1] = 0;
        ((GfxVertex *)t)->normal[2] = 1.0f;
        ((GfxVertex *)t)->color.packed = white;
        ((GfxVertex *)t)->texCoord[0] = 1.0f;
        ((GfxVertex *)t)->texCoord[1] = 1.0f;
        ((GfxVertex *)t)->tangent[0] = 1.0f;
        ((GfxVertex *)t)->tangent[1] = 0;
        ((GfxVertex *)t)->tangent[2] = 0;
        ((GfxVertex *)t)->binormal[0] = 0;
        ((GfxVertex *)t)->binormal[1] = 1.0f;
        ((GfxVertex *)t)->binormal[2] = 0;
    }

    x = x0 + axis[3] - axis[6];
    y = y0 + axis[4] - axis[7];
    z = z0 + axis[5] - axis[8];
    if (isDx7) {
        ((GfxVertexDx7 *)t)->xyz[0] = x;
        ((GfxVertexDx7 *)t)->xyz[1] = y;
        ((GfxVertexDx7 *)t)->xyz[2] = z;
        ((GfxVertexDx7 *)t)->normal[0] = 0;
        ((GfxVertexDx7 *)t)->normal[1] = 0;
        ((GfxVertexDx7 *)t)->normal[2] = 1.0f;
        ((GfxVertexDx7 *)t)->color.packed = white;
        ((GfxVertexDx7 *)t)->texCoord[0] = 0;
        ((GfxVertexDx7 *)t)->texCoord[1] = 1.0f;
    } else {
        ((GfxVertex *)t)->xyzw[0] = x;
        ((GfxVertex *)t)->xyzw[1] = y;
        ((GfxVertex *)t)->xyzw[2] = z;
        ((GfxVertex *)t)->xyzw[3] = 1.0f;
        ((GfxVertex *)t)->normal[0] = 0;
        ((GfxVertex *)t)->normal[1] = 0;
        ((GfxVertex *)t)->normal[2] = 1.0f;
        ((GfxVertex *)t)->color.packed = white;
        ((GfxVertex *)t)->texCoord[0] = 0;
        ((GfxVertex *)t)->texCoord[1] = 1.0f;
    }

    tess.vertexCount = 4;

    iterationCount = r_testFill->current.integer;
    for (i = 0; i < iterationCount; i++) {
        int ic = tess.indexCount;
        r_index_t *indices = tess.indices;
        indices[ic + 0] = 3;
        indices[ic + 1] = 0;
        indices[ic + 2] = 2;
        indices[ic + 3] = 2;
        indices[ic + 4] = 0;
        indices[ic + 5] = 1;
        tess.indexCount = ic + 6;
    }

    RB_BeginBenchmarkGpu_impl(&beginTime);
    RB_EndSurface();
    RB_EndBenchmarkGpu_impl(&endTime);
    RB_PopMatrixStack();

    QueryPerformanceFrequency(&frequency);
    {
        double elapsed = (double)(endTime - beginTime) / (double)frequency;
        return (float)((double)iterationCount / elapsed / 60.0);
    }
}

static float RB_BACKEND_REGPARM2_ABI RB_TestFillPass3D(const Material *material, MaterialTechniqueType techType)
{
    return RB_TestFillPass3D_impl(material, techType);
}

void RB_DrawStretchPic(const Material *material, float x, float y, float w, float h, float s0, float t0, float s1, float t1, D3DCOLOR color, GfxPrimStatsTarget statsTarget)
{
    char *t = (char *)&tess;
    r_index_t *indices;
    int vc, ic;
    int isDx7;
    int savedDecl;

    (void)statsTarget;

    {
        extern int g_rb_stretchpic_calls;
        g_rb_stretchpic_calls++;
    }
    if (!material)
        return;

    if (x != x)
        x = 0.0f;
    if (y != y)
        y = 0.0f;
    if (w != w || h != h)
        return;

    if (!backEnd.projection2D)
        RB_Set2D();

    if (material != tess.material || tess.techType != 3) {
        if (tess.indexCount != 0 || tess.optimizedIndexCount != 0)
            RB_EndSurface();
        RB_BeginSurface(material, 3, 0x1f);
    }

    vc = tess.vertexCount;
    ic = tess.indexCount;
    if (vc + 4 > 0x154a || ic + 6 > 0x100000) {

        savedDecl = tess.declType;
        RB_EndSurface();
        RB_BeginSurface(tess.material, tess.techType, tess.lmapIndex);
        if (tess.declType != savedDecl) {
            if (tess.indexCount != 0 || tess.optimizedIndexCount != 0)
                RB_EndSurface();
            tess.declType = savedDecl;
        }
        vc = tess.vertexCount;
        ic = tess.indexCount;
    }

    indices = tess.indices;
    indices[ic + 0] = (r_index_t)(vc + 3);
    indices[ic + 1] = (r_index_t)vc;
    indices[ic + 2] = (r_index_t)(vc + 2);
    indices[ic + 3] = (r_index_t)(vc + 2);
    indices[ic + 4] = (r_index_t)vc;
    indices[ic + 5] = (r_index_t)(vc + 1);

    isDx7 = r_rendererInUse->current.integer == 2;

    if (isDx7) {

        char *v0 = t + vc * 36;
        char *v1 = v0 + 36;
        char *v2 = v0 + 72;
        char *v3 = v0 + 108;

        ((GfxVertexDx7 *)v0)->xyz[0] = x;
        ((GfxVertexDx7 *)v0)->xyz[1] = y;
        ((GfxVertexDx7 *)v0)->xyz[2] = 0.0f;
        ((GfxVertexDx7 *)v0)->normal[0] = 0.0f;
        ((GfxVertexDx7 *)v0)->normal[1] = 0.0f;
        ((GfxVertexDx7 *)v0)->normal[2] = 1.0f;
        ((GfxVertexDx7 *)v0)->color.packed = color;
        ((GfxVertexDx7 *)v0)->texCoord[0] = s0;
        ((GfxVertexDx7 *)v0)->texCoord[1] = t0;

        ((GfxVertexDx7 *)v1)->xyz[0] = x + w;
        ((GfxVertexDx7 *)v1)->xyz[1] = y;
        ((GfxVertexDx7 *)v1)->xyz[2] = 0.0f;
        ((GfxVertexDx7 *)v1)->normal[0] = 0.0f;
        ((GfxVertexDx7 *)v1)->normal[1] = 0.0f;
        ((GfxVertexDx7 *)v1)->normal[2] = 1.0f;
        ((GfxVertexDx7 *)v1)->color.packed = color;
        ((GfxVertexDx7 *)v1)->texCoord[0] = s1;
        ((GfxVertexDx7 *)v1)->texCoord[1] = t0;

        ((GfxVertexDx7 *)v2)->xyz[0] = x + w;
        ((GfxVertexDx7 *)v2)->xyz[1] = y + h;
        ((GfxVertexDx7 *)v2)->xyz[2] = 0.0f;
        ((GfxVertexDx7 *)v2)->normal[0] = 0.0f;
        ((GfxVertexDx7 *)v2)->normal[1] = 0.0f;
        ((GfxVertexDx7 *)v2)->normal[2] = 1.0f;
        ((GfxVertexDx7 *)v2)->color.packed = color;
        ((GfxVertexDx7 *)v2)->texCoord[0] = s1;
        ((GfxVertexDx7 *)v2)->texCoord[1] = t1;

        ((GfxVertexDx7 *)v3)->xyz[0] = x;
        ((GfxVertexDx7 *)v3)->xyz[1] = y + h;
        ((GfxVertexDx7 *)v3)->xyz[2] = 0.0f;
        ((GfxVertexDx7 *)v3)->normal[0] = 0.0f;
        ((GfxVertexDx7 *)v3)->normal[1] = 0.0f;
        ((GfxVertexDx7 *)v3)->normal[2] = 1.0f;
        ((GfxVertexDx7 *)v3)->color.packed = color;
        ((GfxVertexDx7 *)v3)->texCoord[0] = s0;
        ((GfxVertexDx7 *)v3)->texCoord[1] = t1;
    } else {

        char *v0 = t + vc * 64;
        char *v1 = v0 + 64;
        char *v2 = v0 + 128;
        char *v3 = v0 + 192;

        ((GfxVertex *)v0)->xyzw[0] = x;
        ((GfxVertex *)v0)->xyzw[1] = y;
        ((GfxVertex *)v0)->xyzw[2] = 0.0f;
        ((GfxVertex *)v0)->xyzw[3] = 1.0f;
        ((GfxVertex *)v0)->normal[0] = 0.0f;
        ((GfxVertex *)v0)->normal[1] = 0.0f;
        ((GfxVertex *)v0)->normal[2] = 1.0f;
        ((GfxVertex *)v0)->color.packed = color;
        ((GfxVertex *)v0)->texCoord[0] = s0;
        ((GfxVertex *)v0)->texCoord[1] = t0;
        ((GfxVertex *)v0)->binormal[0] = 0.0f;
        ((GfxVertex *)v0)->binormal[1] = 1.0f;
        ((GfxVertex *)v0)->binormal[2] = 0.0f;
        ((GfxVertex *)v0)->tangent[0] = 1.0f;
        ((GfxVertex *)v0)->tangent[1] = 0.0f;
        ((GfxVertex *)v0)->tangent[2] = 0.0f;

        ((GfxVertex *)v1)->xyzw[0] = x + w;
        ((GfxVertex *)v1)->xyzw[1] = y;
        ((GfxVertex *)v1)->xyzw[2] = 0.0f;
        ((GfxVertex *)v1)->xyzw[3] = 1.0f;
        ((GfxVertex *)v1)->normal[0] = 0.0f;
        ((GfxVertex *)v1)->normal[1] = 0.0f;
        ((GfxVertex *)v1)->normal[2] = 1.0f;
        ((GfxVertex *)v1)->color.packed = color;
        ((GfxVertex *)v1)->texCoord[0] = s1;
        ((GfxVertex *)v1)->texCoord[1] = t0;
        ((GfxVertex *)v1)->binormal[0] = 0.0f;
        ((GfxVertex *)v1)->binormal[1] = 1.0f;
        ((GfxVertex *)v1)->binormal[2] = 0.0f;
        ((GfxVertex *)v1)->tangent[0] = 1.0f;
        ((GfxVertex *)v1)->tangent[1] = 0.0f;
        ((GfxVertex *)v1)->tangent[2] = 0.0f;

        ((GfxVertex *)v2)->xyzw[0] = x + w;
        ((GfxVertex *)v2)->xyzw[1] = y + h;
        ((GfxVertex *)v2)->xyzw[2] = 0.0f;
        ((GfxVertex *)v2)->xyzw[3] = 1.0f;
        ((GfxVertex *)v2)->normal[0] = 0.0f;
        ((GfxVertex *)v2)->normal[1] = 0.0f;
        ((GfxVertex *)v2)->normal[2] = 1.0f;
        ((GfxVertex *)v2)->color.packed = color;
        ((GfxVertex *)v2)->texCoord[0] = s1;
        ((GfxVertex *)v2)->texCoord[1] = t1;
        ((GfxVertex *)v2)->binormal[0] = 0.0f;
        ((GfxVertex *)v2)->binormal[1] = 1.0f;
        ((GfxVertex *)v2)->binormal[2] = 0.0f;
        ((GfxVertex *)v2)->tangent[0] = 1.0f;
        ((GfxVertex *)v2)->tangent[1] = 0.0f;
        ((GfxVertex *)v2)->tangent[2] = 0.0f;

        ((GfxVertex *)v3)->xyzw[0] = x;
        ((GfxVertex *)v3)->xyzw[1] = y + h;
        ((GfxVertex *)v3)->xyzw[2] = 0.0f;
        ((GfxVertex *)v3)->xyzw[3] = 1.0f;
        ((GfxVertex *)v3)->normal[0] = 0.0f;
        ((GfxVertex *)v3)->normal[1] = 0.0f;
        ((GfxVertex *)v3)->normal[2] = 1.0f;
        ((GfxVertex *)v3)->color.packed = color;
        ((GfxVertex *)v3)->texCoord[0] = s0;
        ((GfxVertex *)v3)->texCoord[1] = t1;
        ((GfxVertex *)v3)->binormal[0] = 0.0f;
        ((GfxVertex *)v3)->binormal[1] = 1.0f;
        ((GfxVertex *)v3)->binormal[2] = 0.0f;
        ((GfxVertex *)v3)->tangent[0] = 1.0f;
        ((GfxVertex *)v3)->tangent[1] = 0.0f;
        ((GfxVertex *)v3)->tangent[2] = 0.0f;
    }

    tess.vertexCount += 4;
    tess.indexCount += 6;
}

static void RB_StretchPicCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd = (byte *)execState->cmd;
    unsigned int byteCount;

    g_disp_stretchpic++;
    RB_DrawStretchPic(
        ((GfxCmdStretchPic *)cmd)->material,
        ((GfxCmdStretchPic *)cmd)->x,
        ((GfxCmdStretchPic *)cmd)->y,
        ((GfxCmdStretchPic *)cmd)->w,
        ((GfxCmdStretchPic *)cmd)->h,
        ((GfxCmdStretchPic *)cmd)->s0,
        ((GfxCmdStretchPic *)cmd)->t0,
        ((GfxCmdStretchPic *)cmd)->s1,
        ((GfxCmdStretchPic *)cmd)->t1,
        ((GfxCmdStretchPic *)cmd)->color.packed,
        8);

    cmd = (byte *)execState->cmd;
    byteCount = ((const GfxCmdHeader *)cmd)->byteCount;
    execState->cmd = (const void *)(cmd + byteCount);
}

static float RB_BACKEND_REGPARM2_SSE_ABI RB_BenchmarkRepeatedCalls_impl(const Material *material, int iterationCount, float width, float height)
{
    long long beginTime, endTime, frequency;
    int i;

    if (Material_IsDefault((MaterialHandle)material))
        return 0.0f;

    RB_BeginBenchmarkGpu_impl(&beginTime);

    for (i = 0; i < iterationCount; i++) {
        RB_DrawStretchPic(material, 0.0f, 0.0f, width, height,
                          0.0f, 0.0f, 1.0f, 1.0f, 0xffffffff, 10);
    }

    RB_EndSurface();
    RB_EndBenchmarkGpu_impl(&endTime);

    QueryPerformanceFrequency(&frequency);

    {
        double elapsed = (double)(endTime - beginTime) / (double)frequency;
        return (float)((double)iterationCount / elapsed);
    }
}

static float RB_BACKEND_REGPARM2_SSE_ABI RB_BenchmarkRepeatedCalls(const Material *material, int iterationCount, float width, float height)
{
    return RB_BenchmarkRepeatedCalls_impl(material, iterationCount, width, height);
}

int g_rb_exec_count = 0;
static const char rb_diag_fmt[] = "";
static const char rb_diag_dispatch_fmt[] = "";
static const char rb_endframe_fmt[] = "";
static const char rb_diag_post_fmt[] = "";
int g_rb_dispatch_count = 0;
int g_rb_first_cmd = -1;
int g_rb_skip_reason = 0;

extern void RB_DecideDefaultSamplerState(void);
extern void RB_SetAnisotropy(void);
extern void RB_SetAlphaAntiAliasingState(int state);
extern qboolean R_RecoverLostDevice(void);
extern void RB_DrawDebug(const void *viewParms);
extern void RB_ChangeIndices(int value);
extern void RB_ClearAllStreamSources(void);
extern void Image_RebuildCosinePowerMap(float shift);
void RB_ExecuteRenderCommands(const void *data)
{
    char *dx;
    byte needToTouchImages;
    int deviceState;
    GfxRenderCommandExecState execState;

    backEndData = 0;

    {
        extern void Com_Printf(const char *, ...);
        extern int g_disableRendering;
        static int once;
        if (!once) { once = 1;
            Com_Printf("[rbexec] *imp=%d &direct=%p direct=%d imp=%p\n",
                       *(int *)imp_g_disableRendering, (void *)&g_disableRendering,
                       g_disableRendering, imp_g_disableRendering);
        }
    }

    if (*(int *)imp_g_disableRendering)
        goto done;

    dx = (char *)imp_dx;

    if (!((DxGlobals *)dx)->deviceLost) {
        void *device = ((DxGlobals *)dx)->device;
        void **vtable = *(void ***)device;
        HRESULT hr = ((HRESULT(__attribute__((stdcall)) *)(void *))(vtable[0x0c / 4]))(device);
        if ((unsigned int)(hr + 0x7789f798u) <= 1)
            ((DxGlobals *)dx)->deviceLost = 1;
    }

    needToTouchImages = ((DxGlobals *)dx)->deviceLost;
    if (needToTouchImages) {
        if (!R_RecoverLostDevice())
            goto done;
    }

    backEndData = (const GfxBackEndData *)data;

    {
        char *dvar;
        dvar = *(char **)imp_r_textureMode;
        if (((const dvar_t *)dvar)->modified) {
            ri.Dvar_ClearModified((const dvar_t *)dvar);
            RB_DecideDefaultSamplerState();
        }
        dvar = *(char **)imp_r_anisotropy;
        if (((const dvar_t *)dvar)->modified) {
            ri.Dvar_ClearModified((const dvar_t *)dvar);
            RB_SetAnisotropy();
        }
        dvar = *(char **)imp_r_cosinePowerMapShift;
        if (((const dvar_t *)dvar)->modified) {
            ri.Dvar_ClearModified((const dvar_t *)dvar);
            Image_RebuildCosinePowerMap(((const dvar_t *)dvar)->current.value);
        }
        dvar = *(char **)imp_r_outdoorFeather;
        if (((const dvar_t *)dvar)->modified) {
            ri.Dvar_ClearModified((const dvar_t *)dvar);
            {
                float val = ((const dvar_t *)dvar)->current.value;

                backEnd.codeConsts[50][0] = val;
                backEnd.codeConsts[50][1] = val;
                backEnd.codeConsts[50][2] = val;
                backEnd.codeConsts[50][3] = val;
            }
        }
        dvar = *(char **)imp_r_aaAlpha;
        if (((const dvar_t *)dvar)->modified) {
            ri.Dvar_ClearModified((const dvar_t *)dvar);
            dx = (char *)imp_dx;
            if (((DxGlobals *)dx)->hasTransparencyMsaa) {
                RB_SetAlphaAntiAliasingState(dxState.activeStateBits[0]);
            }
        }
    }

    dx = (char *)imp_dx;
    deviceState = ((DxGlobals *)dx)->gpuSync;
    if (deviceState == 3) {

        while (((DxGlobals *)dx)->flushGpuQueryIssued) {
            qboolean finished = glTestFenceAPPLE(g_FenceID) != 0;
            if (finished)
                glDeleteFencesAPPLE(1, &g_FenceID);
            if (finished) {
                ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
                break;
            }
            dx = (char *)imp_dx;
        }
    } else if (deviceState == 1) {

        while (((DxGlobals *)dx)->flushGpuQueryIssued) {
            qboolean finished = glTestFenceAPPLE(g_FenceID) != 0;
            if (finished)
                glDeleteFencesAPPLE(1, &g_FenceID);
            if (finished) {
                ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
                break;
            }
            dx = (char *)imp_dx;
        }

        glGenFencesAPPLE(1, &g_FenceID);
        glSetFenceAPPLE(g_FenceID);
        dx = (char *)imp_dx;
        ((DxGlobals *)dx)->flushGpuQueryIssued = 1;
    }

    dx = (char *)imp_dx;
    ((DxGlobals *)dx)->inScene = 1;
    do {
        void *device = ((DxGlobals *)dx)->device;
        void **vtable = *(void ***)device;
        ((HRESULT(__attribute__((stdcall)) *)(void *))(vtable[0xa4 / 4]))(device);
    } while (*(volatile int *)&alwaysfails);

    backEnd.frameCount += 1;

    if (needToTouchImages)
        RB_TouchAllImages();

    if (RB_DVAR(imp_r_skipBackEnd)->current.enabled)
        goto post_render;

    {
        const byte *cmdBuf = backEndData->commands.cmds;
        unsigned short cmd;
        static int execTraceCount;
        execState.cmd = cmdBuf;
        execState.stackPos = 0;

        cmd = *(unsigned short *)cmdBuf;

        if (execTraceCount < 1) {
            extern void Com_Printf(const char *, ...);
            Com_Printf("[cmdexec] backEndData=%p cmds=%p first_id=%u\n",
                       (void *)backEndData, (void *)cmdBuf, cmd);
        }

        while (cmd != 0) {
            if (execTraceCount < 40) {
                extern void Com_Printf(const char *, ...);
                const GfxCmdHeader *h = (const GfxCmdHeader *)execState.cmd;
                Com_Printf("[cmdexec] #%d id=%u bytes=%u\n", execTraceCount, h->id, h->byteCount);
                ++execTraceCount;
            }
            RB_RenderCommandTable[cmd](&execState);
            cmd = *(unsigned short *)execState.cmd;
        }
    }

post_render:

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    {
        const GfxViewParms *vp = backEnd.viewParms;
        if (vp) {
            int dev = RB_DVAR(imp_developer)->current.integer;
            if (dev)
                RB_DrawDebug(vp);
        }
    }

    if (backEnd.viewportIsDirty) {
        RB_UpdateViewport();
    }

#ifdef GFX_REAL_D3D9

    if (0)
#endif
        do {
            dx = (char *)imp_dx;
            void *device = ((DxGlobals *)dx)->device;
            void **vtable = *(void ***)device;
            ((HRESULT(D3DVTCC *)(void *, DWORD, void *, DWORD, DWORD, float, DWORD))(vtable[0xac / 4]))(device, 0, NULL, 1, 0x00000000u, 0.0f, 0);
        } while (*(volatile int *)&alwaysfails);

    {
        if (dxState.indexBuffer)
            RB_ChangeIndices(0);
    }
    RB_ClearAllStreamSources();

    dx = (char *)imp_dx;
    do {
        void *device = ((DxGlobals *)dx)->device;
        void **vtable = *(void ***)device;
        ((HRESULT(__attribute__((stdcall)) *)(void *))(vtable[0xa8 / 4]))(device);
    } while (*(volatile int *)&alwaysfails);
    ((DxGlobals *)dx)->inScene = 0;

    {
        int testFillCount = RB_DVAR(imp_r_testFill)->current.integer;
        if (testFillCount > 0) {
            if (!RB_DVAR(imp_r_testFillEnable)->current.enabled) {
                typedef void (*PrintFunc)(int, const char *, ...);
                PrintFunc ri_printf = *(PrintFunc *)&ri;
                ri_printf(0, "Fill testing uses extra textures and materials, so it is usu");
                ri_printf(0, "ally only useful if r_testFillEnable is set before starting the game.\n");
            } else {

                typedef void (*PrintFunc)(int, const char *, ...);
                PrintFunc ri_printf = *(PrintFunc *)&ri;
                char *rgp = (char *)imp_rgp;
                float screenW = (float)dxState.renderTargetWidth;
                float screenH = (float)dxState.renderTargetHeight;
                float result;
                ri_printf(0, "-----------------------------------------------\n");

#define FILL_TEST_2D(mat_off, fmt)                                                                                         \
    result = RB_BenchmarkRepeatedCalls_impl(*(const Material **)(rgp + mat_off), testFillCount, screenW, screenH) / 60.0f; \
    ri_printf(0, fmt, (double)result)
                FILL_TEST_2D(0x105c, "normal fill                 %4.1f overdraw @ 60Hz\n");
                FILL_TEST_2D(0x1060, "alpha blend fill            %4.1f overdraw @ 60Hz\n");
                FILL_TEST_2D(0x1064, "alpha test pass fill        %4.1f overdraw @ 60Hz\n");
                FILL_TEST_2D(0x1068, "alpha test fail fill        %4.1f overdraw @ 60Hz\n");
                FILL_TEST_2D(0x106c, "additive fill               %4.1f overdraw @ 60Hz\n");
                FILL_TEST_2D(0x1070, "depth only fill             %4.1f overdraw @ 60Hz\n");
                FILL_TEST_2D(0x1074, "1-sided stencil keep fill   %4.1f overdraw @ 60Hz\n");
                FILL_TEST_2D(0x1078, "1-sided stencil reject fill %4.1f overdraw @ 60Hz\n");
#undef FILL_TEST_2D

                if (((DxGlobals *)dx)->stencilTwoSided) {
#define FILL_TEST_2D_S(mat_off, fmt)                                                                                       \
    result = RB_BenchmarkRepeatedCalls_impl(*(const Material **)(rgp + mat_off), testFillCount, screenW, screenH) / 60.0f; \
    ri_printf(0, fmt, (double)result)
                    FILL_TEST_2D_S(0x107c, "2-sided stencil keep fill   %4.1f overdraw @ 60Hz\n");
                    FILL_TEST_2D_S(0x1080, "2-sided stencil reject fill %4.1f overdraw @ 60Hz\n");
#undef FILL_TEST_2D_S
                }

#define FILL_TEST_3D(mat_off, tech, fmt)                                        \
    result = RB_TestFillPass3D_impl(*(const Material **)(rgp + mat_off), tech); \
    ri_printf(0, fmt, (double)result)
                FILL_TEST_3D(0x1084, 0x12, "phong point bump fill       %4.1f overdraw @ 60Hz\n");
                FILL_TEST_3D(0x1088, 0x12, "phong point bump+spec fill  %4.1f overdraw @ 60Hz\n");
                FILL_TEST_3D(0x1084, 9, "phong dir bump fill         %4.1f overdraw @ 60Hz\n");
                FILL_TEST_3D(0x1088, 9, "phong dir bump+spec fill    %4.1f overdraw @ 60Hz\n");
#undef FILL_TEST_3D
                ri_printf(0, "-----------------------------------------------\n");
            }

            ri.Dvar_SetInt(*(const dvar_t **)imp_r_testFill, 0);
        }
    }

    {
        int testTransformCount = RB_DVAR(imp_r_testTransform)->current.integer;
        if (testTransformCount > 0) {
            typedef void (*PrintFunc)(int, const char *, ...);
            PrintFunc ri_printf = *(PrintFunc *)&ri;
            char *rgp = (char *)imp_rgp;
            float dynRate;

            dynRate = RB_BenchmarkRepeatedCalls_impl(
                          ((r_global_permanent_t *)rgp)->whiteMaterial, testTransformCount, 0.0f, 0.0f) /
                      60.0f;

            ri_printf(0, "-----------------------------------------------\n");
            ri_printf(0, "static vertex data    %8.0f verts/sec @ 60Hz\n", 0.0);
            ri_printf(0, "static vertex data    %8.0f tris/sec @ 60Hz\n", 0.0);
            ri_printf(0, "skinned vertex data   %8.0f verts/sec @ 60Hz\n", 0.0);
            ri_printf(0, "skinned vertex data   %8.0f tris/sec @ 60Hz\n", 0.0);
            ri_printf(0, "dynamic vertex data   %8.0f verts/sec @ 60Hz\n", (double)(dynRate * 4.0f));
            ri_printf(0, "dynamic vertex data   %8.0f tris/sec @ 60Hz\n", (double)(dynRate * 2.0f));
            ri_printf(0, "-----------------------------------------------\n");
            ri.Dvar_SetInt(*(const dvar_t **)imp_r_testTransform, 0);
        }
    }

    dx = (char *)imp_dx;
    deviceState = ((DxGlobals *)dx)->gpuSync;
#ifdef __EMSCRIPTEN__
    /* WebGL schedules GPU work asynchronously. With synchronization disabled,
     * creating a new query every frame leaks the previous query handle. */
    if (deviceState == 0) {
        if (((DxGlobals *)dx)->flushGpuQueryIssued) {
            glDeleteFencesAPPLE(1, &g_FenceID);
            ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
        }
        goto done;
    }
#endif
    if (deviceState == 3) {
        glGenFencesAPPLE(1, &g_FenceID);
        glSetFenceAPPLE(g_FenceID);
        ((DxGlobals *)dx)->flushGpuQueryIssued = 1;
        goto done;
    }
    if (deviceState == 2) {

        while (((DxGlobals *)dx)->flushGpuQueryIssued) {
            qboolean finished = glTestFenceAPPLE(g_FenceID) != 0;
            if (finished)
                glDeleteFencesAPPLE(1, &g_FenceID);
            if (finished) {
                ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
                break;
            }
            dx = (char *)imp_dx;
        }
        {
            unsigned int startTsc;
            startTsc = 0;
            dx = (char *)imp_dx;
            while (((DxGlobals *)dx)->flushGpuQueryIssued) {
                qboolean finished = glTestFenceAPPLE(g_FenceID) != 0;
                if (finished)
                    glDeleteFencesAPPLE(1, &g_FenceID);
                if (finished) {
                    ((DxGlobals *)dx)->flushGpuQueryIssued = 0;
                    break;
                }
                {
                    unsigned int now;
                    now = 0;
                    if ((int)(now - startTsc) > ((DxGlobals *)dx)->gpuSyncDelay)
                        break;
                }
                dx = (char *)imp_dx;
            }
            {
                unsigned int endTsc;
                endTsc = 0;
                int elapsed = (int)(endTsc - startTsc);
                int scaled = (elapsed * 3 + 3) / 4;
                if (elapsed <= -1)
                    scaled = (elapsed * 3 + 3) / 4;
                ((DxGlobals *)dx)->gpuSyncDelay += scaled;
            }
        }
    }

    glGenFencesAPPLE(1, &g_FenceID);
    glSetFenceAPPLE(g_FenceID);
    dx = (char *)imp_dx;
    ((DxGlobals *)dx)->flushGpuQueryIssued = 1;

done:
    return;
}

void RB_DrawFullScreenColoredQuad(const Material *material, float s0, float t0, float s1, float t1, D3DCOLOR color)
{
    float w = (float)dxState.renderTargetWidth;
    float h = (float)dxState.renderTargetHeight;

    RB_DrawStretchPic(material, 0.0f, 0.0f, w, h, s0, t0, s1, t1, color, 0xa);
}

static void RB_DrawFullScreenColoredQuadCmd(GfxRenderCommandExecState *execState)
{
    DxState *dxs = &dxState;
    byte *cmd = (byte *)execState->cmd;
    float w = (float)dxs->renderTargetWidth;
    float h = (float)dxs->renderTargetHeight;

    RB_DrawStretchPic(
        ((GfxCmdDrawFullScreenColoredQuad *)cmd)->material,
        0.0f, 0.0f, w, h,
        ((GfxCmdDrawFullScreenColoredQuad *)cmd)->s0,
        ((GfxCmdDrawFullScreenColoredQuad *)cmd)->t0,
        ((GfxCmdDrawFullScreenColoredQuad *)cmd)->s1,
        ((GfxCmdDrawFullScreenColoredQuad *)cmd)->t1,
        ((GfxCmdDrawFullScreenColoredQuad *)cmd)->color.packed, 0xa);

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_BlendSavedScreenCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd;
    char *rgp;
    int elapsed, fadeFrames;
    float alpha, alphaScaled;
    int alphaByte;
    D3DCOLOR blendColor;
    const Material *blendMaterial;
    float screenWidth, screenHeight;
    float pow2Width, pow2Height;

    cmd = (byte *)execState->cmd;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    if (!backEnd.projection2D)
        RB_Set2D();

    rgp = (char *)imp_rgp;

    elapsed = backEnd.sceneDef.time - ((r_global_permanent_t *)rgp)->savedScreenTime;
    fadeFrames = ((GfxCmdBlendSavedScreen *)cmd)->fadeMsec;

    if (elapsed < 0 || elapsed >= fadeFrames)
        goto advance;

    alpha = (float)pow(0.01, (double)((float)elapsed / (float)fadeFrames));

    if (alpha > 0.99f)
        alphaScaled = 252.45f;
    else
        alphaScaled = alpha * 255.0f;

    alphaByte = (int)floorf(alphaScaled + 0.5f);

    {
        byte colorBytes[4];
        colorBytes[0] = (byte)alphaByte;
        colorBytes[1] = 0xFF;
        colorBytes[2] = 0xFF;
        colorBytes[3] = 0xFF;
        blendColor = *(D3DCOLOR *)colorBytes;
    }

    backEnd.currentFeedbackImage = dx.renderTargets[R_RENDERTARGET_SAVED_SCREEN].image;

    blendMaterial = ((r_global_permanent_t *)rgp)->shellShockMaterial;

    {
        unsigned int sw = (unsigned int)dxState.renderTargetWidth;
        unsigned int sh = (unsigned int)dxState.renderTargetHeight;
        screenWidth = (float)sw;
        screenHeight = (float)sh;
        pow2Width = (float)nextPow2(sw);
        pow2Height = (float)nextPow2(sh);
    }

    RB_DrawStretchPic(blendMaterial,
                      0.0f, 0.0f, screenWidth, screenHeight,
                      0.0f, screenHeight / pow2Height,
                      screenWidth / pow2Width, 0.0f,
                      blendColor, 10);

advance:
    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_CopyBackBufferToSurface(void *image)
{
    DxState *dxs = &dxState;
    char *dx = (char *)imp_dx;
    void *imageSurface = Image_GetSurface(image);

    do {
        void *device = ((DxGlobals *)dx)->device;
        void **vtable = *(void ***)device;
        void *backBuffer = dxs->renderTargetSurface;
        ((int(__attribute__((stdcall)) *)(void *, void *, void *, void *, void *, int))vtable[0x88 / 4])(
            device, backBuffer, NULL, imageSurface, NULL, 2);
    } while (*(int *)&alwaysfails);

    do {
        ((int(__attribute__((stdcall)) *)(void *))((*(void ***)imageSurface)[2]))(imageSurface);
    } while (*(int *)&alwaysfails);
}

static void RB_BlurShadowCookieCmd(GfxRenderCommandExecState *execState)
{
    int blurCount;
    int blurIter;
    byte *cmd;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    blurCount = sc_blur->current.integer;
    if (blurCount <= 0)
        goto advance;

    for (blurIter = 0; blurIter < blurCount; blurIter++) {
        char *dx = (char *)imp_dx;
        float screenWidth = (float)dxState.renderTargetWidth;
        float screenHeight = (float)dxState.renderTargetHeight;
        void *shadowImage = ((DxGlobals *)dx)->renderTargets[5].image;
        const Material *blurMaterial = rgp.shadowCookieBlurMaterial;

        RB_CopyBackBufferToSurface(shadowImage);
        backEnd.currentFeedbackImage = (GfxImage *)shadowImage;

        RB_DrawStretchPic(blurMaterial,
                          1.0f, 1.0f, screenWidth - 2.0f, screenHeight - 2.0f,
                          0.01171875f, 0.01171875f, 0.99609375f, 0.99609375f,
                          0xffffffff, 10);
        RB_EndSurface();

        screenWidth = (float)dxState.renderTargetWidth;
        screenHeight = (float)dxState.renderTargetHeight;
        shadowImage = ((DxGlobals *)dx)->renderTargets[5].image;

        RB_CopyBackBufferToSurface(shadowImage);
        backEnd.currentFeedbackImage = (GfxImage *)shadowImage;

        RB_DrawStretchPic(blurMaterial,
                          1.0f, 1.0f, screenWidth - 2.0f, screenHeight - 2.0f,
                          0.00390625f, 0.00390625f, 0.98828125f, 0.98828125f,
                          0xffffffff, 10);
        RB_EndSurface();
    }

advance:
    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_BACKEND_REGPARM3_SSE_ABI RB_DrawTextWithCursor_impl(const char *text, int maxChars, FontHandle font,
                                                                   float x, float y, float xScale, float yScale,
                                                                   GfxColor color, int style, int cursorPos, int cursor)
{
    int (*Q_ReadToken)(const char **, int) = (int (*)(const char **, int))ri.SEH_ReadCharFromString;
    int (*Sys_Milliseconds)(void) = (int (*)(void))ri.Milliseconds;
    const Material *material = (const Material *)font->material;
    int traceText = RB_ShouldTraceTextCmd(text);
    GfxColor newColor = color;
    GfxColor newBlack;
    byte savedAlpha;
    newBlack.packed = color.packed & 0xFF;
    savedAlpha = color.array[0];
    float xOrig, yPos, xPos, xAdj, glyphX, glyphY, savedGlyphX2;
    int count, ch;
    Glyph *glyph;

    x += xScale * -0.5f;
    y += yScale * -0.5f;
    xOrig = x;
    yPos = y;
    xPos = x;
    xAdj = 0.0f;
    count = 0;

    if (traceText) {
        printf("[textcmd-back-impl] text='%.96s' font=%p fontName=%s material=%p matName=%s xy=(%.1f,%.1f) scale=(%.3f,%.3f) color=0x%08x bytes=(%u,%u,%u,%u) style=%d max=%d cursor=%d/%d\n",
               text,
               font,
               font && font->name ? font->name : "<null>",
               material,
               material && material->info.name ? material->info.name : "<null>",
               x, y, xScale, yScale,
               color.packed,
               color.array[0], color.array[1], color.array[2], color.array[3],
               style, maxChars, cursorPos, cursor);
    }

    while (*text && maxChars) {
        ch = Q_ReadToken(&text, 0);

        if (ch == '^') {
            const char *next = text;
            if (next && *next != '^' && *next > '/' && *next <= '9') {
                int colorIdx = ColorIndex((int)(unsigned char)*next);
                int defaultIdx = ColorIndex(0x37);
                if ((byte)colorIdx == (byte)defaultIdx) {
                    newColor = color;
                } else {
                    byte rgb[4];
                    RB_LookupColor((int)(unsigned char)*next, rgb);
                    newColor.packed = savedAlpha | ((unsigned int)rgb[0] << 8) |
                                      ((unsigned int)rgb[1] << 16) | ((unsigned int)rgb[2] << 24);
                }
                text++;
                count += 2;
                continue;
            }

        }

        if (ch == '\n') {
            yPos += (float)font->pixelHeight * yScale;
            xPos = xOrig;
            continue;
        }

        if (ch == '\r') {
            xPos = xOrig;
            continue;
        }

        glyph = R_GetCharacterGlyph(font, ch);
        xAdj = (float)(signed char)glyph->x0 * xScale;
        {
            float yAdj = (float)(signed char)glyph->y0 * yScale;
            float w, h, shadowOffset;

            if (style == 3) {
                shadowOffset = 1.0f;
            } else if (style == 6) {
                shadowOffset = 2.0f;
            } else {
                shadowOffset = 0.0f;
            }

            if (shadowOffset > 0.0f) {

                h = (float)glyph->pixelHeight * yScale;
                w = (float)glyph->pixelWidth * xScale;
                glyphY = yPos + yAdj;
                glyphX = xPos + xAdj;
                {
                    float shadowY = glyphY + shadowOffset;
                    float shadowX = glyphX + shadowOffset;
                    RB_DrawStretchPic(material, shadowX, shadowY, w, h,
                                      glyph->s0, glyph->t0, glyph->s1, glyph->t1, newBlack.packed, 8);
                    RB_DrawStretchPic(material, shadowX + 0.25f, shadowY + 0.25f, w, h,
                                      glyph->s0, glyph->t0, glyph->s1, glyph->t1, newBlack.packed, 8);
                }

            } else {
                glyphY = yPos + yAdj;
                glyphX = xPos + xAdj;
            }

            h = (float)glyph->pixelHeight * yScale;
            w = (float)glyph->pixelWidth * xScale;
            if (traceText && count == 0) {
                printf("[textcmd-glyph0] ch=%d glyph=(%d,%d %dx%d) draw=(%.1f,%.1f %.1fx%.1f) st=(%.3f,%.3f %.3f,%.3f) color=0x%08x mat=%s\n",
                       ch,
                       (int)(signed char)glyph->x0,
                       (int)(signed char)glyph->y0,
                       glyph->pixelWidth,
                       glyph->pixelHeight,
                       glyphX, glyphY, w, h,
                       glyph->s0, glyph->t0, glyph->s1, glyph->t1,
                       newColor.packed,
                       material && material->info.name ? material->info.name : "<null>");
            }
            RB_DrawStretchPic(material, glyphX, glyphY, w, h,
                              glyph->s0, glyph->t0, glyph->s1, glyph->t1, newColor.packed, 8);
            savedGlyphX2 = glyphX + 0.25f;
            RB_DrawStretchPic(material, savedGlyphX2, glyphY + 0.25f, w, h,
                              glyph->s0, glyph->t0, glyph->s1, glyph->t1, newColor.packed, 8);
        }

        if (count == cursorPos) {
            int time = Sys_Milliseconds();
            if (time <= -1)
                time += 255;
            if (!((time >> 8) & 1)) {

                Glyph *cg = R_GetCharacterGlyph(font, cursor);
                float ch_ = (float)cg->pixelHeight * yScale;
                float cw = (float)cg->pixelWidth * xScale;
                float cy = yPos + (float)(signed char)cg->y0 * yScale;
                RB_DrawStretchPic(material, glyphX, cy, cw, ch_,
                                  cg->s0, cg->t0, cg->s1, cg->t1, newColor.packed, 8);
                RB_DrawStretchPic(material, savedGlyphX2, cy + 0.25f, cw, ch_,
                                  cg->s0, cg->t0, cg->s1, cg->t1, newColor.packed, 8);
            }
        }

        xPos += (float)glyph->dx * xScale;
        count++;
        maxChars--;
    }

    if (count == cursorPos) {
        float cursorX = xPos + xAdj;
        int time = Sys_Milliseconds();
        if (time <= -1)
            time += 255;
        if (!((time >> 8) & 1)) {
            Glyph *cg = R_GetCharacterGlyph(font, cursor);
            float ch_ = (float)cg->pixelHeight * yScale;
            float cw = (float)cg->pixelWidth * xScale;
            float cy = yPos + (float)(signed char)cg->y0 * yScale;
            RB_DrawStretchPic(material, cursorX, cy, cw, ch_,
                              cg->s0, cg->t0, cg->s1, cg->t1, newColor.packed, 8);
            RB_DrawStretchPic(material, cursorX + 0.25f, cy + 0.25f, cw, ch_,
                              cg->s0, cg->t0, cg->s1, cg->t1, newColor.packed, 8);
        }
    }
}

static void RB_BACKEND_REGPARM3_SSE_ABI RB_DrawTextWithCursor(const char *text, int maxChars, FontHandle font,
                                                              float x, float y, float xScale, float yScale,
                                                              GfxColor color, int style, int cursorPos, int cursor)
{
    RB_DrawTextWithCursor_impl(text, maxChars, font, x, y, xScale, yScale, color, style, cursorPos, cursor);
}

static void RB_DrawTextCmd(GfxRenderCommandExecState *execState)
{
    const GfxCmdDrawText *cmd = (const GfxCmdDrawText *)execState->cmd;

    const char *text = cmd->text;
    int maxChars = cmd->maxChars;
    FontHandle font = cmd->font;   /* was int -> truncated the 8-byte FontHandle on x64 */
    float x = cmd->x;
    float y = cmd->y;
    float xScale = cmd->xScale;
    float yScale = cmd->yScale;
    int style = *(int *)&cmd->color;
    int color = cmd->style;
    int cursorPos = cmd->cursorPos;
    int cursor = (signed char)cmd->cursor;

    if (RB_ShouldTraceTextCmd(text)) {
        printf("[textcmd-back] cmd=%p bytes=%u text='%.96s' xy=(%.1f,%.1f) scale=(%.3f,%.3f) color=0x%08x bytes=(%u,%u,%u,%u) style=%d max=%d cursor=%d/%d font=%p\n",
               cmd,
               cmd->header.byteCount,
               text,
               x, y, xScale, yScale,
               cmd->color.packed,
               cmd->color.array[0], cmd->color.array[1], cmd->color.array[2], cmd->color.array[3],
               cmd->style, maxChars, cursorPos, cursor,
               cmd->font);
    }

    RB_DrawTextWithCursor_impl(text, maxChars, font, x, y, xScale, yScale, *(GfxColor *)&style, color, cursorPos, cursor);

    const byte *cmdBytes = (const byte *)execState->cmd;
    execState->cmd = (const void *)(cmdBytes + *(unsigned short *)(cmdBytes + 2));
}

extern void RB_GaussianFilterImage(float radius, GfxRenderTargetId renderTargetId);
extern void RB_GlowFilterImage(const int *radii);
extern float GetVirtualWidthFromRealWidth(float width);
static void RB_ApplyLatePostEffectsCmd(GfxRenderCommandExecState *execState)
{
    DxGlobals *dxg = (DxGlobals *)imp_dx;
    DxState *dxs = &dxState;
    char *t = (char *)&tess;
    const byte *cmd = (const byte *)execState->cmd;
    int frameBufferTarget;
    float blurRadius;
    int isDx7, hasGlowSupport, needCopy;

    if (tess.indexCount || tess.optimizedIndexCount)
        RB_EndSurface();

    frameBufferTarget = dxs->renderTargetId;
    backEnd.resolvedSceneTarget = 0xe;
    blurRadius = ((GfxCmdApplyLatePostEffects *)cmd)->blurRadius;

    isDx7 = r_rendererInUse->current.integer == 2;
    hasGlowSupport = !isDx7 && dxg->hasSeparateAlphaBlend;

    needCopy = 0;
    if (hasGlowSupport) {
        if (r_glow->current.enabled && !r_fullbright->current.enabled)
            needCopy = 1;
    }

    if (r_showFbColorDebug->current.integer == 2)
        needCopy = 1;
    else if (!isDx7) {
        float blurVal = r_blur->current.value;
        if (blurVal > 0.0f || blurRadius > 0.0f)
            needCopy = 1;
    }

    if (needCopy) {
        char *dx = (char *)imp_dx;
        void *offscreenImage = ((DxGlobals *)dx)->renderTargets[2].image;
        void *imageSurface = Image_GetSurface(offscreenImage);

        do {
            void *device = ((DxGlobals *)dx)->device;
            void **vtable = *(void ***)device;
            void *backBuffer = dxs->renderTargetSurface;
            ((HRESULT(__attribute__((stdcall)) *)(void *, void *, void *, void *, void *, int))
                 vtable[0x88 / 4])(device, backBuffer, NULL, imageSurface, NULL, 2);
        } while (*(volatile int *)&alwaysfails);

        do {
            ((HRESULT(__attribute__((stdcall)) *)(void *))((*(void ***)imageSurface)[2]))(imageSurface);
        } while (*(volatile int *)&alwaysfails);

        backEnd.resolvedSceneTarget = 2;
    }

    if (hasGlowSupport && r_glow->current.enabled &&
        !r_fullbright->current.enabled) {
        char *rgp = (char *)imp_rgp;

        float bloomCutoff = r_glowBloomCutoff->current.value;
        int bloomDesat = r_glowBloomDesaturation->current.integer;
        backEnd.codeConsts[32][0] = bloomCutoff;
        backEnd.codeConsts[32][1] = 1.0f / (1.0f - bloomCutoff);
        backEnd.codeConsts[32][2] = 0.0f;
        *(int *)&backEnd.codeConsts[32][3] = bloomDesat;

        {
            int glowRadii[2] = { 0, 0 };
            int p;
            for (p = 0; p < 2; p++) {
                float intensity = *(float *)(*(char **)imp_r_glowBloomIntensity + 8 + p * 4);
                if (intensity > 0.0f)
                    glowRadii[p] = *(int *)(*(char **)imp_r_glowRadius + 8 + p * 4);
            }

            if (glowRadii[1] != 0 && glowRadii[0] == 0) {
                glowRadii[0] = glowRadii[1];
                glowRadii[1] = 0;
            }

            int glowAxisCount = (glowRadii[1] != 0) ? 2 : (glowRadii[0] != 0) ? 1
                                                                              : 0;
            backEnd.glowIndexFirst = glowAxisCount;
            *(int *)imp_g_TotalFilterPasses = 0;
            RB_GlowFilterImage(glowRadii);
        }

        RB_SetRenderTarget(0);

        {
            int glowCount = backEnd.glowCount;
            int glowIndex = backEnd.glowIndexFirst;
            int pass;
            float sw = (float)dxState.renderTargetWidth;
            float sh = (float)dxState.renderTargetHeight;

            for (pass = 0; pass < glowCount; pass++) {
                float skyBleed = *(float *)(*(char **)imp_r_glowSkyBleedIntensity + 8 + glowIndex * 4);
                int bloomIntensity = *(int *)(*(char **)imp_r_glowBloomIntensity + 8 + glowIndex * 4);
                const Material *glowMaterial;

                if (skyBleed > 0.0f)
                    glowMaterial = ((r_global_permanent_t *)rgp)->glowApplySkyBleedMaterial;
                else
                    glowMaterial = ((r_global_permanent_t *)rgp)->glowApplyBloomMaterial;

                backEnd.codeConsts[33][0] = skyBleed;
                backEnd.codeConsts[33][1] = 0.0f;
                backEnd.codeConsts[33][2] = 0.0f;
                *(int *)&backEnd.codeConsts[33][3] = bloomIntensity;

                backEnd.currentFeedbackImage = backEnd.glowImage[glowIndex];

                {
                    float pw = (float)nextPow2((unsigned int)dxState.renderTargetWidth);
                    float ph = (float)nextPow2((unsigned int)dxState.renderTargetHeight);
                    RB_DrawStretchPic(glowMaterial, 0, 0, sw, sh,
                                      0, sh / ph, sw / pw, 0, 0xffffffff, 10);
                    RB_EndSurface();
                }

                glowIndex = 1 - glowIndex;
            }
            backEnd.glowCount = 0;
        }
    }

    if (!isDx7) {
        float blurVal = r_blur->current.value;
        float totalBlur;

        if (blurVal > 0.0f || blurRadius > 0.0f) {
            if (blurVal <= 0.0f)
                totalBlur = blurRadius;
            else {
                float sqrtf_approx;
                totalBlur = blurVal * blurVal + blurRadius * blurRadius;
                sqrtf_approx = sqrtf(totalBlur);
                totalBlur = sqrtf_approx;
            }

            {
                float virtualWidth = GetVirtualWidthFromRealWidth(3.0f);
                D3DCOLOR blurColor = 0xffffffff;

                if (totalBlur > virtualWidth) {
                    int alpha = (int)floorf(totalBlur / virtualWidth * 255.0f + 0.5f);
                    blurColor = (blurColor & 0xffffff00) | (alpha & 0xff);
                }

                RB_GaussianFilterImage(totalBlur, R_RENDERTARGET_BLURRED_SCREEN);
                {
                    RB_SetRenderTarget(0);

                    backEnd.currentFeedbackImage = dxg->renderTargets[R_RENDERTARGET_BLURRED_SCREEN].image;
                    {
                        char *rgp = (char *)imp_rgp;
                        const Material *blurMaterial = ((r_global_permanent_t *)rgp)->feedbackBlendMaterial;
                        float sw = (float)dxState.renderTargetWidth;
                        float sh = (float)dxState.renderTargetHeight;
                        float pw = (float)nextPow2((unsigned int)dxState.renderTargetWidth);
                        float ph = (float)nextPow2((unsigned int)dxState.renderTargetHeight);

                        RB_DrawStretchPic(blurMaterial, 0, 0, sw, sh,
                                          0, sh / ph, sw / pw, 0, blurColor, 10);
                        RB_EndSurface();
                    }
                }
            }
        }
    }

    RB_SetRenderTarget(frameBufferTarget);

    if (r_showFbColorDebug->current.integer == 1) {
        float halfW = (float)dxState.renderTargetWidth * 0.5f;
        float halfH = (float)dxState.renderTargetHeight * 0.5f;
        const Material *colorMtl = rgp.frameColorDebugMaterial;

        RB_DrawStretchPic(colorMtl, halfW * 0.5f, 0.5f * halfH, halfW * 0.5f, 0.5f * halfH,
                          0, 0, 1.0f, 1.0f, 0xff000000, 10);
        RB_DrawStretchPic(colorMtl, halfW, 0.5f * halfH, halfW * 0.5f, 0.5f * halfH,
                          0, 0, 1.0f, 1.0f, 0xff000000, 10);
        RB_DrawStretchPic(colorMtl, halfW * 0.5f, halfH, halfW * 0.5f, 0.5f * halfH,
                          0, 0, 1.0f, 1.0f, 0xff0000ff, 10);
        RB_DrawStretchPic(rgp.frameAlphaDebugMaterial,
                          halfW, halfH, halfW * 0.5f, 0.5f * halfH,
                          0, 0, 1.0f, 1.0f, 0xffffffff, 10);
        RB_EndSurface();

        {
            const byte *c = (const byte *)execState->cmd;
            execState->cmd = c + *(unsigned short *)(c + 2);
        }
        return;
    }

    if (isDx7) {
        const byte *c = (const byte *)execState->cmd;
        execState->cmd = c + *(unsigned short *)(c + 2);
        return;
    }

    if (r_showFbColorDebug->current.integer == 2) {
        float halfW = 0.5f * (float)dxState.renderTargetWidth;
        float halfH = 0.5f * (float)dxState.renderTargetHeight;
        const Material *mixMtl = rgp.colorChannelMixerMaterial;
        float save0 = backEnd.codeConsts[27][0];
        float save1 = backEnd.codeConsts[27][1];
        float save2 = backEnd.codeConsts[27][2];
        float save3 = backEnd.codeConsts[27][3];

        backEnd.currentFeedbackImage = dxg->renderTargets[3].image;
        RB_SetRenderTarget(0);

        backEnd.codeConsts[27][0] = 1.0f;
        backEnd.codeConsts[27][1] = 0.0f;
        backEnd.codeConsts[27][2] = 0.0f;
        backEnd.codeConsts[27][3] = 0.0f;
        RB_DrawStretchPic(mixMtl, 0.5f * halfW, 0.5f * halfH, 0.5f * halfW, 0.5f * halfH,
                          0.25f, 0.25f, 0.5f, 0.5f, 0xffffffff, 10);
        RB_EndSurface();

        backEnd.codeConsts[27][0] = 0.0f;
        backEnd.codeConsts[27][1] = 1.0f;
        backEnd.codeConsts[27][2] = 0.0f;
        backEnd.codeConsts[27][3] = 0.0f;
        RB_DrawStretchPic(mixMtl, halfW, 0.5f * halfH, 0.5f * halfW, 0.5f * halfH,
                          0.5f, 0.25f, 0.75f, 0.5f, 0xffffffff, 10);
        RB_EndSurface();

        backEnd.codeConsts[27][0] = 0.0f;
        backEnd.codeConsts[27][1] = 0.0f;
        backEnd.codeConsts[27][2] = 1.0f;
        backEnd.codeConsts[27][3] = 0.0f;
        RB_DrawStretchPic(mixMtl, 0.5f * halfW, halfH, 0.5f * halfW, 0.5f * halfH,
                          0.25f, 0.5f, 0.5f, 0.75f, 0xffffffff, 10);
        RB_EndSurface();

        backEnd.codeConsts[27][0] = 0.0f;
        backEnd.codeConsts[27][1] = 0.0f;
        backEnd.codeConsts[27][2] = 0.0f;
        backEnd.codeConsts[27][3] = 1.0f;
        RB_DrawStretchPic(mixMtl, halfW, halfH, 0.5f * halfW, 0.5f * halfH,
                          0.5f, 0.5f, 0.75f, 0.75f, 0xffffffff, 10);
        RB_EndSurface();

        backEnd.codeConsts[27][0] = save0;
        backEnd.codeConsts[27][1] = save1;
        backEnd.codeConsts[27][2] = save2;
        backEnd.codeConsts[27][3] = save3;

        {
            const byte *c = (const byte *)execState->cmd;
            execState->cmd = c + *(unsigned short *)(c + 2);
        }
        return;
    }

    if (sc_showDebug->current.enabled) {
        int halfH = (int)((float)dxState.renderTargetHeight * 0.5f);
        int halfW = (int)((float)dxState.renderTargetWidth * 0.5f);
        const Material *mixMtl = rgp.colorChannelMixerMaterial;
        float save3 = backEnd.codeConsts[27][3];

        RB_SetRenderTarget(0);
        backEnd.currentFeedbackImage = dxg->renderTargets[3].image;

        backEnd.codeConsts[27][0] = 0.333000004f;
        backEnd.codeConsts[27][1] = 0.333999991f;
        backEnd.codeConsts[27][2] = 0.333000004f;
        backEnd.codeConsts[27][3] = 0.0f;
        RB_DrawStretchPic(mixMtl, halfW * 0.5f, halfH * 0.5f, (float)halfW, (float)halfH,
                          0.25f, 0.25f, 0.75f, 0.75f, 0xffffffff, 10);
        RB_EndSurface();

        backEnd.codeConsts[27][3] = save3;

        {
            const byte *c = (const byte *)execState->cmd;
            execState->cmd = c + *(unsigned short *)(c + 2);
        }
        return;
    }

    {
        const byte *c = (const byte *)execState->cmd;
        execState->cmd = c + *(unsigned short *)(c + 2);
    }
}

static void RB_StretchPicRotateCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd;
    char *t = (char *)&tess;
    const Material *material;
    float halfW, halfH, midX, midY;
    float radians, sinR, cosR;
    float cx, sx, sy, cy;
    float v0x, v0y, v1x, v1y, v2x, v2y, v3x, v3y;
    int vc;
    const dvar_t *renderer;
    D3DCOLOR color;

    cmd = (byte *)execState->cmd;

    if (!backEnd.projection2D)
        RB_Set2D();

    material = ((GfxCmdStretchPicRotate *)cmd)->material;
    RB_BeginSurface2D(t, material);
    vc = RB_CheckTessOverflow4(t);

    {
        int ic = tess.indexCount;
        tess.vertexCount = vc + 4;
        tess.indexCount = ic + 6;
        RB_WriteQuadIndicesAt(t, vc, ic);
    }

    halfW = ((GfxCmdStretchPicRotate *)cmd)->w * 0.5f;
    halfH = ((GfxCmdStretchPicRotate *)cmd)->h * 0.5f;
    midX = ((GfxCmdStretchPicRotate *)cmd)->x + halfW;
    midY = ((GfxCmdStretchPicRotate *)cmd)->y + halfH;
    radians = ((GfxCmdStretchPicRotate *)cmd)->rotation * (float)(3.14159265358979323846 / 180.0);
    sinR = sinf(radians);
    cosR = cosf(radians);

    cx = halfW * cosR;
    sx = halfW * sinR;
    sy = -halfH * sinR;
    cy = halfH * cosR;

    v0x = midX - cx - sy;
    v0y = midY - sx - cy;
    v1x = midX + cx - sy;
    v1y = midY + sx - cy;
    v2x = midX + cx + sy;
    v2y = midY + sx + cy;
    v3x = midX - cx + sy;
    v3y = midY - sx + cy;

    color = ((GfxCmdStretchPicRotate *)cmd)->color.packed;
    renderer = r_rendererInUse;

    RB_SetVertex2D(t, vc + 0, renderer, v0x, v0y, ((GfxCmdStretchPicRotate *)cmd)->s0, ((GfxCmdStretchPicRotate *)cmd)->t0, color);
    RB_SetVertex2D(t, vc + 1, renderer, v1x, v1y, ((GfxCmdStretchPicRotate *)cmd)->s1, ((GfxCmdStretchPicRotate *)cmd)->t0, color);
    RB_SetVertex2D(t, vc + 2, renderer, v2x, v2y, ((GfxCmdStretchPicRotate *)cmd)->s1, ((GfxCmdStretchPicRotate *)cmd)->t1, color);
    RB_SetVertex2D(t, vc + 3, renderer, v3x, v3y, ((GfxCmdStretchPicRotate *)cmd)->s0, ((GfxCmdStretchPicRotate *)cmd)->t1, color);

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_DrawQuadPicCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd;
    char *t = (char *)&tess;
    const Material *material;
    int vc;
    const dvar_t *renderer;
    D3DCOLOR color;

    cmd = (byte *)execState->cmd;

    if (!backEnd.projection2D)
        RB_Set2D();

    material = ((GfxCmdDrawQuadPic *)cmd)->material;
    RB_BeginSurface2D(t, material);
    vc = RB_CheckTessOverflow4(t);

    {
        int ic = tess.indexCount;
        tess.vertexCount = vc + 4;
        tess.indexCount = ic + 6;
        RB_WriteQuadIndicesAt(t, vc, ic);
    }

    color = ((GfxCmdDrawQuadPic *)cmd)->color.packed;
    renderer = r_rendererInUse;

    RB_SetVertex2D(t, vc + 0, renderer, ((GfxCmdDrawQuadPic *)cmd)->verts[0][0], ((GfxCmdDrawQuadPic *)cmd)->verts[0][1], 0.0f, 0.0f, color);
    RB_SetVertex2D(t, vc + 1, renderer, ((GfxCmdDrawQuadPic *)cmd)->verts[1][0], ((GfxCmdDrawQuadPic *)cmd)->verts[1][1], 1.0f, 0.0f, color);
    RB_SetVertex2D(t, vc + 2, renderer, ((GfxCmdDrawQuadPic *)cmd)->verts[2][0], ((GfxCmdDrawQuadPic *)cmd)->verts[2][1], 1.0f, 1.0f, color);
    RB_SetVertex2D(t, vc + 3, renderer, ((GfxCmdDrawQuadPic *)cmd)->verts[3][0], ((GfxCmdDrawQuadPic *)cmd)->verts[3][1], 0.0f, 1.0f, color);

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

void RB_DrawLines3D(int count, int width, const GfxPointVertex *verts, int depthTest)
{
    char *t = (char *)&tess;
    const Material *debugMtl = rgp.lineMaterial;
    int isDx7, lineIndex;
    float identity[16];
    float invWidth, invHeight;
    const float *row0, *row1, *row2, *row3;
    char *vp;

    if (debugMtl != tess.material ||
        tess.techType != 3) {
        if (tess.indexCount || tess.optimizedIndexCount)
            RB_EndSurface();
        RB_BeginSurface(debugMtl, 3, 0x1f);
    } else if (tess.indexCount || tess.optimizedIndexCount) {

    }

    if (!depthTest) {
        Material *mtl = rgp.lineMaterial;
        int bits = mtl->stateBits[1];
        mtl->stateBits[1] = (bits & 0xfffffff1) | 2;
    }

    MatrixIdentity44(identity);
    RB_SetProjectionMatrix(identity);
    RB_SetViewMatrix(identity);

    vp = (char *)backEnd.viewParms;
    row0 = (const float *)&backEnd.viewParms->viewProjectionMatrix._11;
    row1 = (const float *)&backEnd.viewParms->viewProjectionMatrix._21;
    row2 = (const float *)&backEnd.viewParms->viewProjectionMatrix._31;
    row3 = (const float *)&backEnd.viewParms->viewProjectionMatrix._41;

    {
        float fWidth = (float)width;
        float screenW = (float)dxState.renderTargetWidth;
        float screenH = (float)dxState.renderTargetHeight;
        invWidth = fWidth / screenW;
        invHeight = fWidth / screenH;
    }

    isDx7 = r_rendererInUse->current.integer == 2;

    for (lineIndex = 0; lineIndex < count; lineIndex++) {
        const float *fromXyz = (const float *)&verts[lineIndex * 2];
        const float *toXyz = (const float *)&verts[lineIndex * 2 + 1];
        const byte *fromColor = (const byte *)&verts[lineIndex * 2] + 12;
        const byte *toColor = (const byte *)&verts[lineIndex * 2 + 1] + 12;
        float posA_x, posA_y, posA_z, posA_w;
        float posB_x, posB_y, posB_z, posB_w;
        float delta[2], offAx, offAy, offBx, offBy;
        int vc, ic;
        r_index_t *indices;

        posA_x = fromXyz[0] * row0[0] + fromXyz[1] * row1[0] + fromXyz[2] * row2[0] + row3[0];
        posA_y = fromXyz[0] * row0[1] + fromXyz[1] * row1[1] + fromXyz[2] * row2[1] + row3[1];
        posA_z = fromXyz[0] * row0[2] + fromXyz[1] * row1[2] + fromXyz[2] * row2[2] + row3[2];
        posA_w = fromXyz[0] * row0[3] + fromXyz[1] * row1[3] + fromXyz[2] * row2[3] + row3[3];

        posB_x = toXyz[0] * row0[0] + toXyz[1] * row1[0] + toXyz[2] * row2[0] + row3[0];
        posB_y = toXyz[0] * row0[1] + toXyz[1] * row1[1] + toXyz[2] * row2[1] + row3[1];
        posB_z = toXyz[0] * row0[2] + toXyz[1] * row1[2] + toXyz[2] * row2[2] + row3[2];
        posB_w = toXyz[0] * row0[3] + toXyz[1] * row1[3] + toXyz[2] * row2[3] + row3[3];

        delta[0] = posA_w * posB_y - posA_y * posB_w;
        delta[1] = posA_x * posB_w - posA_w * posB_x;
        Vec2Normalize(delta);
        delta[0] *= invWidth;
        delta[1] *= invHeight;

        offAx = posA_w * delta[0];
        offAy = posA_w * delta[1];
        offBx = posB_w * delta[0];
        offBy = posB_w * delta[1];

        vc = tess.vertexCount;
        ic = tess.indexCount;
        if (vc + 4 > 0x154a || ic + 6 > 0x100000) {
            int savedDecl = tess.declType;
            RB_EndSurface();
            RB_BeginSurface(tess.material,
                            tess.techType,
                            tess.lmapIndex);
            if (tess.declType != savedDecl) {
                if (tess.indexCount || tess.optimizedIndexCount)
                    RB_EndSurface();
                tess.declType = savedDecl;
            }
            vc = tess.vertexCount;
            ic = tess.indexCount;
        }

        indices = tess.indices;
        indices[ic + 0] = (r_index_t)(vc + 3);
        indices[ic + 1] = (r_index_t)vc;
        indices[ic + 2] = (r_index_t)(vc + 2);
        indices[ic + 3] = (r_index_t)(vc + 2);
        indices[ic + 4] = (r_index_t)vc;
        indices[ic + 5] = (r_index_t)(vc + 1);
        tess.indexCount += 6;

        {
            D3DCOLOR colA = *(D3DCOLOR *)fromColor;
            D3DCOLOR colB = *(D3DCOLOR *)toColor;
            float v0x = posA_x - offAx, v0y = posA_y - offAy;
            float v1x = posB_x - offBx, v1y = posB_y - offBy;
            float v2x = posB_x + offBx, v2y = posB_y + offBy;
            float v3x = posA_x + offAx, v3y = posA_y + offAy;

            if (isDx7) {

                char *v;

                char *vp_;
                vp_ = t + (vc + 0) * 36;
                ((GfxVertexDx7 *)vp_)->xyz[0] = v0x / posA_w;
                ((GfxVertexDx7 *)vp_)->xyz[1] = v0y / posA_w;
                ((GfxVertexDx7 *)vp_)->xyz[2] = posA_z / posA_w;
                ((GfxVertexDx7 *)vp_)->normal[0] = 0;
                ((GfxVertexDx7 *)vp_)->normal[1] = 0;
                ((GfxVertexDx7 *)vp_)->normal[2] = 1.0f;
                ((GfxVertexDx7 *)vp_)->color.packed = colA;
                ((GfxVertexDx7 *)vp_)->texCoord[0] = 0;
                ((GfxVertexDx7 *)vp_)->texCoord[1] = 0;

                vp_ = t + (vc + 1) * 36;
                ((GfxVertexDx7 *)vp_)->xyz[0] = v1x / posB_w;
                ((GfxVertexDx7 *)vp_)->xyz[1] = v1y / posB_w;
                ((GfxVertexDx7 *)vp_)->xyz[2] = posB_z / posB_w;
                ((GfxVertexDx7 *)vp_)->normal[0] = 0;
                ((GfxVertexDx7 *)vp_)->normal[1] = 0;
                ((GfxVertexDx7 *)vp_)->normal[2] = 1.0f;
                ((GfxVertexDx7 *)vp_)->color.packed = colB;
                ((GfxVertexDx7 *)vp_)->texCoord[0] = 0;
                ((GfxVertexDx7 *)vp_)->texCoord[1] = 1.0f;

                vp_ = t + (vc + 2) * 36;
                ((GfxVertexDx7 *)vp_)->xyz[0] = v2x / posB_w;
                ((GfxVertexDx7 *)vp_)->xyz[1] = v2y / posB_w;
                ((GfxVertexDx7 *)vp_)->xyz[2] = posB_z / posB_w;
                ((GfxVertexDx7 *)vp_)->normal[0] = 0;
                ((GfxVertexDx7 *)vp_)->normal[1] = 0;
                ((GfxVertexDx7 *)vp_)->normal[2] = 1.0f;
                ((GfxVertexDx7 *)vp_)->color.packed = colB;
                ((GfxVertexDx7 *)vp_)->texCoord[0] = 1.0f;
                ((GfxVertexDx7 *)vp_)->texCoord[1] = 1.0f;

                vp_ = t + (vc + 3) * 36;
                ((GfxVertexDx7 *)vp_)->xyz[0] = v3x / posA_w;
                ((GfxVertexDx7 *)vp_)->xyz[1] = v3y / posA_w;
                ((GfxVertexDx7 *)vp_)->xyz[2] = posA_z / posA_w;
                ((GfxVertexDx7 *)vp_)->normal[0] = 0;
                ((GfxVertexDx7 *)vp_)->normal[1] = 0;
                ((GfxVertexDx7 *)vp_)->normal[2] = 1.0f;
                ((GfxVertexDx7 *)vp_)->color.packed = colA;
                ((GfxVertexDx7 *)vp_)->texCoord[0] = 1.0f;
                ((GfxVertexDx7 *)vp_)->texCoord[1] = 0;
            } else {

                char *vp_;
                vp_ = t + (vc + 0) * 64;
                ((GfxVertexDx7 *)vp_)->xyz[0] = v0x;
                ((GfxVertexDx7 *)vp_)->xyz[1] = v0y;
                ((GfxVertexDx7 *)vp_)->xyz[2] = posA_z;
                ((GfxVertexDx7 *)vp_)->normal[0] = posA_w;
                ((GfxVertexDx7 *)vp_)->normal[1] = 0;
                ((GfxVertexDx7 *)vp_)->normal[2] = 0;
                ((GfxVertexDx7 *)vp_)->color.packed = 1.0f;
                ((GfxVertexDx7 *)vp_)->texCoord[0] = colA;
                ((GfxVertex *)vp_)->texCoord[0] = 0;
                ((GfxVertex *)vp_)->texCoord[1] = 0;
                ((GfxVertex *)vp_)->binormal[0] = 0;
                ((GfxVertex *)vp_)->binormal[1] = 0.0f;
                ((GfxVertex *)vp_)->binormal[2] = 0;
                ((GfxVertex *)vp_)->tangent[0] = 1.0f;
                ((GfxVertex *)vp_)->tangent[1] = 0;
                ((GfxVertex *)vp_)->tangent[2] = 0;

                vp_ = t + (vc + 1) * 64;
                ((GfxVertex *)vp_)->xyzw[0] = v1x;
                ((GfxVertex *)vp_)->xyzw[1] = v1y;
                ((GfxVertex *)vp_)->xyzw[2] = posB_z;
                ((GfxVertex *)vp_)->xyzw[3] = posB_w;
                ((GfxVertex *)vp_)->normal[0] = 0;
                ((GfxVertex *)vp_)->normal[1] = 0;
                ((GfxVertex *)vp_)->normal[2] = 1.0f;
                ((GfxVertex *)vp_)->color.packed = colB;
                ((GfxVertex *)vp_)->texCoord[0] = 0;
                ((GfxVertex *)vp_)->texCoord[1] = 1.0f;
                ((GfxVertex *)vp_)->binormal[0] = 0;
                ((GfxVertex *)vp_)->binormal[1] = 1.0f;
                ((GfxVertex *)vp_)->binormal[2] = 0;
                ((GfxVertex *)vp_)->tangent[0] = 1.0f;
                ((GfxVertex *)vp_)->tangent[1] = 0;
                ((GfxVertex *)vp_)->tangent[2] = 0;

                vp_ = t + (vc + 2) * 64;
                ((GfxVertex *)vp_)->xyzw[0] = v2x;
                ((GfxVertex *)vp_)->xyzw[1] = v2y;
                ((GfxVertex *)vp_)->xyzw[2] = posB_z;
                ((GfxVertex *)vp_)->xyzw[3] = posB_w;
                ((GfxVertex *)vp_)->normal[0] = 0;
                ((GfxVertex *)vp_)->normal[1] = 0;
                ((GfxVertex *)vp_)->normal[2] = 1.0f;
                ((GfxVertex *)vp_)->color.packed = colB;
                ((GfxVertex *)vp_)->texCoord[0] = 1.0f;
                ((GfxVertex *)vp_)->texCoord[1] = 1.0f;
                ((GfxVertex *)vp_)->binormal[0] = 1.0f;
                ((GfxVertex *)vp_)->binormal[1] = 1.0f;
                ((GfxVertex *)vp_)->binormal[2] = 0;
                ((GfxVertex *)vp_)->tangent[0] = 1.0f;
                ((GfxVertex *)vp_)->tangent[1] = 0;
                ((GfxVertex *)vp_)->tangent[2] = 0;

                vp_ = t + (vc + 3) * 64;
                ((GfxVertex *)vp_)->xyzw[0] = v3x;
                ((GfxVertex *)vp_)->xyzw[1] = v3y;
                ((GfxVertex *)vp_)->xyzw[2] = posA_z;
                ((GfxVertex *)vp_)->xyzw[3] = posA_w;
                ((GfxVertex *)vp_)->normal[0] = 0;
                ((GfxVertex *)vp_)->normal[1] = 0;
                ((GfxVertex *)vp_)->normal[2] = 1.0f;
                ((GfxVertex *)vp_)->color.packed = colA;
                ((GfxVertex *)vp_)->texCoord[0] = 1.0f;
                ((GfxVertex *)vp_)->texCoord[1] = 0;
                ((GfxVertex *)vp_)->binormal[0] = 1.0f;
                ((GfxVertex *)vp_)->binormal[1] = 0;
                ((GfxVertex *)vp_)->binormal[2] = 0;
                ((GfxVertex *)vp_)->tangent[0] = 1.0f;
                ((GfxVertex *)vp_)->tangent[1] = 0;
                ((GfxVertex *)vp_)->tangent[2] = 0;
            }
        }
        tess.vertexCount += 4;
    }

    RB_EndSurface();
    RB_SetMatricesForView(backEnd.viewParms);

    if (!depthTest) {
        Material *mtl = rgp.lineMaterial;
        int bits = mtl->stateBits[1];
        mtl->stateBits[1] = (bits & 0xfffffff1) | 4;
    }
}

static void RB_DrawLinesCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd = (byte *)execState->cmd;
    int count = *(short *)(cmd + 4);
    int width = *(short *)(cmd + 6);
    const GfxPointVertex *verts = (const GfxPointVertex *)(cmd + 8);

    if (backEnd.projection2D) {

        RB_DrawLines2D(count, width, verts);
    } else {

        RB_DrawLines3D(count, width, verts, 1);
    }

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static inline void RB_SetClipSpaceVertex(char *tessBase, int vertIndex, int isDx7,
                                         float px, float py, float pz, float pw, D3DCOLOR color)
{
    if (isDx7) {
        char *v = tessBase + vertIndex * 36;
        ((GfxVertexDx7 *)v)->xyz[0] = px / pw;
        ((GfxVertexDx7 *)v)->xyz[1] = py / pw;
        ((GfxVertexDx7 *)v)->xyz[2] = pz / pw;
        ((GfxVertexDx7 *)v)->normal[0] = 0;
        ((GfxVertexDx7 *)v)->normal[1] = 0;
        ((GfxVertexDx7 *)v)->normal[2] = 1.0f;
        ((GfxVertexDx7 *)v)->color.packed = color;
        ((GfxVertexDx7 *)v)->texCoord[0] = 0;
        ((GfxVertexDx7 *)v)->texCoord[1] = 0;
    } else {
        char *v = tessBase + vertIndex * 64;
        ((GfxVertexDx7 *)v)->xyz[0] = px;
        ((GfxVertexDx7 *)v)->xyz[1] = py;
        ((GfxVertexDx7 *)v)->xyz[2] = pz;
        ((GfxVertexDx7 *)v)->normal[0] = pw;
        ((GfxVertexDx7 *)v)->normal[1] = 0;
        ((GfxVertexDx7 *)v)->normal[2] = 0;
        ((GfxVertexDx7 *)v)->color.packed = 1.0f;
        ((GfxVertex *)v)->color.packed = color;
        ((GfxVertex *)v)->texCoord[0] = 0;
        ((GfxVertex *)v)->texCoord[1] = 0;
        ((GfxVertex *)v)->binormal[0] = 0;
        ((GfxVertex *)v)->binormal[1] = 1.0f;
        ((GfxVertex *)v)->binormal[2] = 0;
        ((GfxVertex *)v)->tangent[0] = 1.0f;
        ((GfxVertex *)v)->tangent[1] = 0;
        ((GfxVertex *)v)->tangent[2] = 0;
    }
}

static void RB_StencilPlanesCmd(GfxRenderCommandExecState *execState)
{
    byte *cmd;
    char *t = (char *)&tess;
    const Material *stencilMaterial;
    int planeCount, planeIdx;
    float zOffset;
    int isDx7;
    D3DCOLOR white = 0xffffffff;

    cmd = (byte *)execState->cmd;

    stencilMaterial = rgp.stencilPlaneMaterial;
    RB_BeginSurface2D(t, stencilMaterial);

    planeCount = ((GfxCmdStencilPlanes *)cmd)->planeCount;
    zOffset = ((GfxCmdStencilPlanes *)cmd)->nearDist;

    if (planeCount <= 0)
        goto done;

    isDx7 = r_rendererInUse->current.integer == 2;

    for (planeIdx = 0; planeIdx < planeCount; planeIdx++) {
        float d = ((GfxCmdStencilPlanes *)cmd)->planeDists[planeIdx];
        float negD = -d;
        float z = d - zOffset;
        int vc;

        vc = RB_CheckTessOverflow4(t);

        {
            int ic = tess.indexCount;
            r_index_t *indices = tess.indices;
            indices[ic + 0] = (r_index_t)vc;
            indices[ic + 1] = (r_index_t)(vc + 1);
            indices[ic + 2] = (r_index_t)(vc + 2);
            indices[ic + 3] = (r_index_t)(vc + 2);
            indices[ic + 4] = (r_index_t)(vc + 3);
            indices[ic + 5] = (r_index_t)vc;
        }

        RB_SetClipSpaceVertex(t, vc + 0, isDx7, negD, negD, z, d, white);
        RB_SetClipSpaceVertex(t, vc + 1, isDx7, negD, d, z, d, white);
        RB_SetClipSpaceVertex(t, vc + 2, isDx7, d, d, z, d, white);
        RB_SetClipSpaceVertex(t, vc + 3, isDx7, d, negD, z, d, white);

        tess.vertexCount = vc + 4;
        tess.indexCount += 6;
    }

done:
    RB_EndSurface();

    cmd = (byte *)execState->cmd;
    execState->cmd = (const void *)(cmd + ((const GfxCmdHeader *)cmd)->byteCount);
}

static void RB_DrawPointsCmd(GfxRenderCommandExecState *execState)
{
    char *t = (char *)&tess;
    const byte *cmd = (const byte *)execState->cmd;
    const Material *debugMtl = rgp.whiteMaterial;
    short pointCount = *(short *)(cmd + 4);
    float size = (float)(short)*(short *)(cmd + 6) * 0.5f;
    const byte *verts = cmd + 8;
    int isDx7, pointIndex;

    if (debugMtl != tess.material ||
        tess.techType != 3) {
        if (tess.indexCount || tess.optimizedIndexCount)
            RB_EndSurface();
        RB_BeginSurface(debugMtl, 3, 0x1f);
    }

    isDx7 = r_rendererInUse->current.integer == 2;

    if (backEnd.projection2D) {

        float identity[16];
        float invWidth, invHeight;
        const float *row0, *row1, *row2, *row3;
        const GfxViewParms *vpParms = backEnd.viewParms;

        MatrixIdentity44(identity);
        RB_SetProjectionMatrix(identity);
        RB_SetViewMatrix(identity);

        row0 = (const float *)&vpParms->viewProjectionMatrix._11;
        row1 = (const float *)&vpParms->viewProjectionMatrix._21;
        row2 = (const float *)&vpParms->viewProjectionMatrix._31;
        row3 = (const float *)&vpParms->viewProjectionMatrix._41;
        {
            float fSize = (float)(short)*(short *)(cmd + 6);
            invWidth = fSize / (float)dxState.renderTargetWidth;
            invHeight = fSize / (float)dxState.renderTargetHeight;
        }

        for (pointIndex = 0; pointIndex < pointCount; pointIndex++) {
            const float *xyz = (const float *)(verts + pointIndex * 16);
            D3DCOLOR color = *(D3DCOLOR *)(verts + pointIndex * 16 + 12);
            float cx, cy, cz, cw, ox, oy;
            int vc, ic;
            r_index_t *indices;

            cx = xyz[0] * row0[0] + xyz[1] * row1[0] + xyz[2] * row2[0] + row3[0];
            cy = xyz[0] * row0[1] + xyz[1] * row1[1] + xyz[2] * row2[1] + row3[1];
            cz = xyz[0] * row0[2] + xyz[1] * row1[2] + xyz[2] * row2[2] + row3[2];
            cw = xyz[0] * row0[3] + xyz[1] * row1[3] + xyz[2] * row2[3] + row3[3];
            ox = cw * invWidth;
            oy = cw * invHeight;

            vc = RB_CheckTessOverflow4(t);
            ic = tess.indexCount;

            indices = tess.indices;
            indices[ic + 0] = (r_index_t)(vc + 1);
            indices[ic + 1] = (r_index_t)vc;
            indices[ic + 2] = (r_index_t)(vc + 2);
            indices[ic + 3] = (r_index_t)(vc + 2);
            indices[ic + 4] = (r_index_t)vc;
            indices[ic + 5] = (r_index_t)(vc + 3);
            tess.indexCount += 6;

            if (isDx7) {
                char *v;
                v = t + (vc + 0) * 36;
                ((GfxVertexDx7 *)v)->xyz[0] = (cx - ox) / cw;
                ((GfxVertexDx7 *)v)->xyz[1] = (cy - oy) / cw;
                ((GfxVertexDx7 *)v)->xyz[2] = cz / cw;
                ((GfxVertexDx7 *)v)->normal[0] = 0;
                ((GfxVertexDx7 *)v)->normal[1] = 0;
                ((GfxVertexDx7 *)v)->normal[2] = 1.0f;
                ((GfxVertexDx7 *)v)->color.packed = color;
                ((GfxVertexDx7 *)v)->texCoord[0] = 0;
                ((GfxVertexDx7 *)v)->texCoord[1] = 0;
                v = t + (vc + 1) * 36;
                ((GfxVertexDx7 *)v)->xyz[0] = (cx - ox) / cw;
                ((GfxVertexDx7 *)v)->xyz[1] = (cy + oy) / cw;
                ((GfxVertexDx7 *)v)->xyz[2] = cz / cw;
                ((GfxVertexDx7 *)v)->normal[0] = 0;
                ((GfxVertexDx7 *)v)->normal[1] = 0;
                ((GfxVertexDx7 *)v)->normal[2] = 1.0f;
                ((GfxVertexDx7 *)v)->color.packed = color;
                ((GfxVertexDx7 *)v)->texCoord[0] = 0;
                ((GfxVertexDx7 *)v)->texCoord[1] = 1.0f;
                v = t + (vc + 2) * 36;
                ((GfxVertexDx7 *)v)->xyz[0] = (cx + ox) / cw;
                ((GfxVertexDx7 *)v)->xyz[1] = (cy + oy) / cw;
                ((GfxVertexDx7 *)v)->xyz[2] = cz / cw;
                ((GfxVertexDx7 *)v)->normal[0] = 0;
                ((GfxVertexDx7 *)v)->normal[1] = 0;
                ((GfxVertexDx7 *)v)->normal[2] = 1.0f;
                ((GfxVertexDx7 *)v)->color.packed = color;
                ((GfxVertexDx7 *)v)->texCoord[0] = 1.0f;
                ((GfxVertexDx7 *)v)->texCoord[1] = 1.0f;
                v = t + (vc + 3) * 36;
                ((GfxVertexDx7 *)v)->xyz[0] = (cx + ox) / cw;
                ((GfxVertexDx7 *)v)->xyz[1] = (cy - oy) / cw;
                ((GfxVertexDx7 *)v)->xyz[2] = cz / cw;
                ((GfxVertexDx7 *)v)->normal[0] = 0;
                ((GfxVertexDx7 *)v)->normal[1] = 0;
                ((GfxVertexDx7 *)v)->normal[2] = 1.0f;
                ((GfxVertexDx7 *)v)->color.packed = color;
                ((GfxVertexDx7 *)v)->texCoord[0] = 1.0f;
                ((GfxVertexDx7 *)v)->texCoord[1] = 0;
            } else {
                char *v;
                v = t + (vc + 0) * 64;
                ((GfxVertex *)v)->xyzw[0] = cx - ox;
                ((GfxVertex *)v)->xyzw[1] = cy - oy;
                ((GfxVertex *)v)->xyzw[2] = cz;
                ((GfxVertex *)v)->xyzw[3] = cw;
                ((GfxVertex *)v)->normal[0] = 0;
                ((GfxVertex *)v)->normal[1] = 0;
                ((GfxVertex *)v)->normal[2] = 1.0f;
                ((GfxVertex *)v)->color.packed = color;
                ((GfxVertex *)v)->texCoord[0] = 0;
                ((GfxVertex *)v)->texCoord[1] = 0;
                ((GfxVertex *)v)->binormal[0] = 0;
                ((GfxVertex *)v)->binormal[1] = 0.0f;
                ((GfxVertex *)v)->binormal[2] = 0;
                ((GfxVertex *)v)->tangent[0] = 1.0f;
                ((GfxVertex *)v)->tangent[1] = 0;
                ((GfxVertex *)v)->tangent[2] = 0;
                v = t + (vc + 1) * 64;
                ((GfxVertex *)v)->xyzw[0] = cx - ox;
                ((GfxVertex *)v)->xyzw[1] = cy + oy;
                ((GfxVertex *)v)->xyzw[2] = cz;
                ((GfxVertex *)v)->xyzw[3] = cw;
                ((GfxVertex *)v)->normal[0] = 0;
                ((GfxVertex *)v)->normal[1] = 0;
                ((GfxVertex *)v)->normal[2] = 1.0f;
                ((GfxVertex *)v)->color.packed = color;
                ((GfxVertex *)v)->texCoord[0] = 0;
                ((GfxVertex *)v)->texCoord[1] = 1.0f;
                ((GfxVertex *)v)->binormal[0] = 0;
                ((GfxVertex *)v)->binormal[1] = 1.0f;
                ((GfxVertex *)v)->binormal[2] = 0;
                ((GfxVertex *)v)->tangent[0] = 1.0f;
                ((GfxVertex *)v)->tangent[1] = 0;
                ((GfxVertex *)v)->tangent[2] = 0;
                v = t + (vc + 2) * 64;
                ((GfxVertex *)v)->xyzw[0] = cx + ox;
                ((GfxVertex *)v)->xyzw[1] = cy + oy;
                ((GfxVertex *)v)->xyzw[2] = cz;
                ((GfxVertex *)v)->xyzw[3] = cw;
                ((GfxVertex *)v)->normal[0] = 0;
                ((GfxVertex *)v)->normal[1] = 0;
                ((GfxVertex *)v)->normal[2] = 1.0f;
                ((GfxVertex *)v)->color.packed = color;
                ((GfxVertex *)v)->texCoord[0] = 1.0f;
                ((GfxVertex *)v)->texCoord[1] = 1.0f;
                ((GfxVertex *)v)->binormal[0] = 1.0f;
                ((GfxVertex *)v)->binormal[1] = 1.0f;
                ((GfxVertex *)v)->binormal[2] = 0;
                ((GfxVertex *)v)->tangent[0] = 1.0f;
                ((GfxVertex *)v)->tangent[1] = 0;
                ((GfxVertex *)v)->tangent[2] = 0;
                v = t + (vc + 3) * 64;
                ((GfxVertex *)v)->xyzw[0] = cx + ox;
                ((GfxVertex *)v)->xyzw[1] = cy - oy;
                ((GfxVertex *)v)->xyzw[2] = cz;
                ((GfxVertex *)v)->xyzw[3] = cw;
                ((GfxVertex *)v)->normal[0] = 0;
                ((GfxVertex *)v)->normal[1] = 0;
                ((GfxVertex *)v)->normal[2] = 1.0f;
                ((GfxVertex *)v)->color.packed = color;
                ((GfxVertex *)v)->texCoord[0] = 1.0f;
                ((GfxVertex *)v)->texCoord[1] = 0;
                ((GfxVertex *)v)->binormal[0] = 1.0f;
                ((GfxVertex *)v)->binormal[1] = 0;
                ((GfxVertex *)v)->binormal[2] = 0;
                ((GfxVertex *)v)->tangent[0] = 1.0f;
                ((GfxVertex *)v)->tangent[1] = 0;
                ((GfxVertex *)v)->tangent[2] = 0;
            }
            tess.vertexCount += 4;
        }

        RB_EndSurface();
        RB_SetMatricesForView(backEnd.viewParms);
    } else {

        for (pointIndex = 0; pointIndex < pointCount; pointIndex++) {
            const float *xyz = (const float *)(verts + pointIndex * 16);
            D3DCOLOR color = *(D3DCOLOR *)(verts + pointIndex * 16 + 12);
            float px = xyz[0], py = xyz[1];
            int pz_i = *(int *)(xyz + 2);
            int vc, ic;
            r_index_t *indices;

            vc = RB_CheckTessOverflow4(t);
            ic = tess.indexCount;

            indices = tess.indices;
            indices[ic + 0] = (r_index_t)(vc + 1);
            indices[ic + 1] = (r_index_t)vc;
            indices[ic + 2] = (r_index_t)(vc + 2);
            indices[ic + 3] = (r_index_t)(vc + 2);
            indices[ic + 4] = (r_index_t)vc;
            indices[ic + 5] = (r_index_t)(vc + 3);
            tess.indexCount += 6;

            if (isDx7) {
                char *v;
                v = t + (vc + 0) * 36;
                ((GfxVertexDx7 *)v)->xyz[0] = px - size;
                ((GfxVertexDx7 *)v)->xyz[1] = py - size;
                ((GfxVertexDx7 *)v)->xyz[2] = pz_i;
                ((GfxVertexDx7 *)v)->normal[0] = 0;
                ((GfxVertexDx7 *)v)->normal[1] = 0;
                ((GfxVertexDx7 *)v)->normal[2] = 1.0f;
                ((GfxVertexDx7 *)v)->color.packed = color;
                ((GfxVertexDx7 *)v)->texCoord[0] = 0;
                ((GfxVertexDx7 *)v)->texCoord[1] = 0;
                v = t + (vc + 1) * 36;
                ((GfxVertexDx7 *)v)->xyz[0] = px - size;
                ((GfxVertexDx7 *)v)->xyz[1] = py + size;
                ((GfxVertexDx7 *)v)->xyz[2] = pz_i;
                ((GfxVertexDx7 *)v)->normal[0] = 0;
                ((GfxVertexDx7 *)v)->normal[1] = 0;
                ((GfxVertexDx7 *)v)->normal[2] = 1.0f;
                ((GfxVertexDx7 *)v)->color.packed = color;
                ((GfxVertexDx7 *)v)->texCoord[0] = 0;
                ((GfxVertexDx7 *)v)->texCoord[1] = 1.0f;
                v = t + (vc + 2) * 36;
                ((GfxVertexDx7 *)v)->xyz[0] = px + size;
                ((GfxVertexDx7 *)v)->xyz[1] = py + size;
                ((GfxVertexDx7 *)v)->xyz[2] = pz_i;
                ((GfxVertexDx7 *)v)->normal[0] = 0;
                ((GfxVertexDx7 *)v)->normal[1] = 0;
                ((GfxVertexDx7 *)v)->normal[2] = 1.0f;
                ((GfxVertexDx7 *)v)->color.packed = color;
                ((GfxVertexDx7 *)v)->texCoord[0] = 1.0f;
                ((GfxVertexDx7 *)v)->texCoord[1] = 1.0f;
                v = t + (vc + 3) * 36;
                ((GfxVertexDx7 *)v)->xyz[0] = px + size;
                ((GfxVertexDx7 *)v)->xyz[1] = py - size;
                ((GfxVertexDx7 *)v)->xyz[2] = pz_i;
                ((GfxVertexDx7 *)v)->normal[0] = 0;
                ((GfxVertexDx7 *)v)->normal[1] = 0;
                ((GfxVertexDx7 *)v)->normal[2] = 1.0f;
                ((GfxVertexDx7 *)v)->color.packed = color;
                ((GfxVertexDx7 *)v)->texCoord[0] = 1.0f;
                ((GfxVertexDx7 *)v)->texCoord[1] = 0;
            } else {
                char *v;
                v = t + (vc + 0) * 64;
                ((GfxVertex *)v)->xyzw[0] = px - size;
                ((GfxVertex *)v)->xyzw[1] = py - size;
                ((GfxVertex *)v)->xyzw[2] = pz_i;
                ((GfxVertex *)v)->xyzw[3] = 1.0f;
                ((GfxVertex *)v)->normal[0] = 0;
                ((GfxVertex *)v)->normal[1] = 0;
                ((GfxVertex *)v)->normal[2] = 1.0f;
                ((GfxVertex *)v)->color.packed = color;
                ((GfxVertex *)v)->texCoord[0] = 0;
                ((GfxVertex *)v)->texCoord[1] = 0;
                ((GfxVertex *)v)->binormal[0] = 0;
                ((GfxVertex *)v)->binormal[1] = 0.0f;
                ((GfxVertex *)v)->binormal[2] = 0;
                ((GfxVertex *)v)->tangent[0] = 1.0f;
                ((GfxVertex *)v)->tangent[1] = 0;
                ((GfxVertex *)v)->tangent[2] = 0;
                v = t + (vc + 1) * 64;
                ((GfxVertex *)v)->xyzw[0] = px - size;
                ((GfxVertex *)v)->xyzw[1] = py + size;
                ((GfxVertex *)v)->xyzw[2] = pz_i;
                ((GfxVertex *)v)->xyzw[3] = 1.0f;
                ((GfxVertex *)v)->normal[0] = 0;
                ((GfxVertex *)v)->normal[1] = 0;
                ((GfxVertex *)v)->normal[2] = 1.0f;
                ((GfxVertex *)v)->color.packed = color;
                ((GfxVertex *)v)->texCoord[0] = 0;
                ((GfxVertex *)v)->texCoord[1] = 1.0f;
                ((GfxVertex *)v)->binormal[0] = 0;
                ((GfxVertex *)v)->binormal[1] = 1.0f;
                ((GfxVertex *)v)->binormal[2] = 0;
                ((GfxVertex *)v)->tangent[0] = 1.0f;
                ((GfxVertex *)v)->tangent[1] = 0;
                ((GfxVertex *)v)->tangent[2] = 0;
                v = t + (vc + 2) * 64;
                ((GfxVertex *)v)->xyzw[0] = px + size;
                ((GfxVertex *)v)->xyzw[1] = py + size;
                ((GfxVertex *)v)->xyzw[2] = pz_i;
                ((GfxVertex *)v)->xyzw[3] = 1.0f;
                ((GfxVertex *)v)->normal[0] = 0;
                ((GfxVertex *)v)->normal[1] = 0;
                ((GfxVertex *)v)->normal[2] = 1.0f;
                ((GfxVertex *)v)->color.packed = color;
                ((GfxVertex *)v)->texCoord[0] = 1.0f;
                ((GfxVertex *)v)->texCoord[1] = 1.0f;
                ((GfxVertex *)v)->binormal[0] = 1.0f;
                ((GfxVertex *)v)->binormal[1] = 1.0f;
                ((GfxVertex *)v)->binormal[2] = 0;
                ((GfxVertex *)v)->tangent[0] = 1.0f;
                ((GfxVertex *)v)->tangent[1] = 0;
                ((GfxVertex *)v)->tangent[2] = 0;
                v = t + (vc + 3) * 64;
                ((GfxVertex *)v)->xyzw[0] = px + size;
                ((GfxVertex *)v)->xyzw[1] = py - size;
                ((GfxVertex *)v)->xyzw[2] = pz_i;
                ((GfxVertex *)v)->xyzw[3] = 1.0f;
                ((GfxVertex *)v)->normal[0] = 0;
                ((GfxVertex *)v)->normal[1] = 0;
                ((GfxVertex *)v)->normal[2] = 1.0f;
                ((GfxVertex *)v)->color.packed = color;
                ((GfxVertex *)v)->texCoord[0] = 1.0f;
                ((GfxVertex *)v)->texCoord[1] = 0;
                ((GfxVertex *)v)->binormal[0] = 1.0f;
                ((GfxVertex *)v)->binormal[1] = 0;
                ((GfxVertex *)v)->binormal[2] = 0;
                ((GfxVertex *)v)->tangent[0] = 1.0f;
                ((GfxVertex *)v)->tangent[1] = 0;
                ((GfxVertex *)v)->tangent[2] = 0;
            }
            tess.vertexCount += 4;
        }

        RB_EndSurface();
    }

    {
        const byte *c = (const byte *)execState->cmd;
        execState->cmd = c + *(unsigned short *)(c + 2);
    }
}

const byte color_table[8][4] = { { 0, 0, 0, 255 }, { 255, 0, 0, 255 }, { 0, 255, 0, 255 }, { 255, 255, 0, 255 }, { 0, 0, 255, 255 }, { 0, 255, 255, 255 }, { 255, 0, 255, 255 }, { 255, 255, 255, 255 } }; /* truncated 48->32 (blob over-capture) */

const Bool refEntIsInWorldSpace[10] = { 0, 0, 0, 0, 1, 1, 0, 1, 1, 1 }; /* truncated 112->10 (blob over-capture) */
