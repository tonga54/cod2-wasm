#include "common_types.h"
#include "imports.h"

extern char *getenv(const char *name);

extern refimport_t ri;
extern DxGlobals dx;

extern SkinBuffers g_skinBuffers[1];
extern GfxBackEndData *frontEndDataOut;
extern byte g_dummyBuf[];
extern r_global_permanent_t rgp;
extern void R_ShutdownDebugEntry(void *entry);
extern void R_UnlockSkinnedCache(void);
extern void R_UpdateGfxEntityBounds(GfxEntity *ent);
extern void R_SkinGfxEntity(GfxEntity *ent);
extern void R_SkinStaticModelCachedCmd(SkinStaticModelCachedCmd *skinCmd, SkinBuffers *skinBuffers);
extern void R_SkinXModelCmd(SkinXModelCmd *skinCmd, int context);
extern void R_SkinRigidXModelCmd(SkinRigidXModelCmd *skinRigidCmd);
extern void FX_UpdateScheduledEffectsBolt(void);
extern void FX_UpdateScheduledEffectsNonBolt(void);
extern const float AngleNormalize360(const float angle);
extern void R_ConvertColorToBytes(const vec_t *colorFloat, byte *colorBytes);
extern void RB_ExecuteRenderCommands(const void *data);
extern void RB_EndFrame(void);
extern void R_LockSkinnedCache(int lock);
extern void R_InitDebugEntry(DebugGlobals *debugGlobalsEntry);
extern void R_TransferDebugGlobals(DebugGlobals *debugGlobalsEntry);
extern void R_UpdateGpuSyncType(void);
extern void R_UpdateLightsFromDvars(void);
extern void R_SetSunFromDvars(sunflare_t *sun);
extern void RB_CreateDynamicBuffers(void);
extern void RB_AdaptiveGpuSyncTarget(void);
extern void Material_Sort(void);
extern struct r_globals_t rg;

extern unsigned char s_backEndData[];
extern GfxCmdArray *s_cmdList;
extern struct GfxDebugFrameGlob s_debugFrameGlob;
extern int printf(const char *, ...);

void R_ShutdownBackendData(void);
static __attribute_regparm__(3) void R_ProcessFrontendCmdInternal(int type, void *data, int isRenderThread);
void R_AddFrontendCmd(int type, void *data);
void R_SyncRenderThread(void);
GfxViewParms *R_AllocViewParms(void);
void R_BeginDebugFrame(void);
void R_AddCmdTouchAllImages(void);
void R_AbortRenderCommands(void);
void R_BeginFrame(void);
static void R_BeginFrame_impl(void);
void R_UpdateEffectsBolt(void);
void R_UpdateEffectsNonBolt(void);
void R_UpdateXModelBoundsDelayed(GfxEntity *ent);
void R_SkinGfxEntityDelayed(GfxEntity *ent);
void R_InitBackendData(void);
void R_EndFrame(void);
static void R_EndFrame_impl(void);
void R_AddCmdDrawSurfs(GfxDrawSurf *drawSurfs, int drawSurfCount, MaterialTechniqueType techType);
int R_BeginDelayedDrawing(void);
void R_EndDelayedDrawing(int marker);
void R_IssueDelayedDrawing(int marker);
void R_AddCmdDrawStretchPic(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const vec_t *color, MaterialHandle material);
void R_AddCmdDrawStretchPicRotate(float x, float y, float w, float h, float s0, float t0, float s1, float t1, float angle, const vec_t *color, MaterialHandle material);
void R_AddCmdDrawStretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
void R_AddCmdDrawTextWithCursor(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style, int cursorPos, int cursor);
void R_AddCmdDrawTextInSpace(const char *text, FontHandle font, const vec_t *org, const vec_t *xPixelStep, const vec_t *yPixelStep, const vec_t *color);
void R_AddCmdDrawQuadPic(vec2_t *verts, const vec_t *color, MaterialHandle material);
void R_AddCmdDrawSprite(MaterialHandle material, const byte *rgbaColor, const vec_t *pos, float radius, float minScreenRadius, int renderFxFlags);
void R_AddCmdDrawFullScreenColoredQuad(float s0, float t0, float s1, float t1, const vec_t *color, MaterialHandle material);
void R_AddCmdSetMaterialColor(const vec_t *color);
void R_AddCmdLightProperties(int lightIndex, const GfxLight *light);
void R_AddCmdSetRenderTarget(GfxRenderTargetId renderTargetId);
void R_AddCmdDrawSun(int viewIndex);
void R_AddCmdBeginView(int viewCount, const GfxSceneDef *sceneDef, const GfxViewParms *viewParms, const GfxLodParms *lodParms);
void R_AddCmdSetViewport(int x, int y, int width, int height);
void R_EndDebugFrame(void);
void R_AddCmdApplyEarlyPostEffects(void);
void R_AddCmdApplyLatePostEffects(float blurRadius);
void R_AddCmdDrawSunPostEffects(int viewIndex);
void R_AddCmdClearScreen(int whichToClear, const vec_t *color, float depth, int stencil);
void R_AddCmdSaveScreen(void);
void R_AddCmdBlendSavedScreen(int fadeMsec);

static int R_TextTraceContains(const char *text, const char *needle)
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

static int R_ShouldTraceTextCmd(const char *text)
{
    static int enabled = -1;
    if (enabled < 0)
        enabled = getenv("TEXTCMD_DBG") ? 1 : 0;
    if (!enabled)
        return 0;
    return R_TextTraceContains(text, "Deathmatch") ||
           R_TextTraceContains(text, "Click to Continue") ||
           R_TextTraceContains(text, "Score Limit") ||
           R_TextTraceContains(text, "Time Limit") ||
           R_TextTraceContains(text, "50") ||
           R_TextTraceContains(text, "30");
}

static int R_CmdBytesContain(const byte *bytes, int len, const char *needle)
{
    int i;
    int j;

    if (!bytes || len <= 0 || !needle || !*needle) {
        return 0;
    }

    for (i = 0; i < len; ++i) {
        for (j = 0; needle[j] && i + j < len && bytes[i + j] == (byte)needle[j]; ++j) {
        }
        if (!needle[j]) {
            return 1;
        }
    }

    return 0;
}

static void R_DumpCommandListForMenu(const GfxCmdArray *cl)
{
    static int dumped;
    int off;
    int limit;
    int count;

    if (!getenv("DBGSPAM") || dumped || !cl || cl->usedTotal <= 0) {
        return;
    }

    if (!R_CmdBytesContain((const byte *)cl->cmds, cl->usedTotal, "Score Limit")) {
        return;
    }

    dumped = 1;
    limit = cl->usedTotal;
    if (getenv("DBGSPAM"))
        printf("[cmddump] used=%d critical=%d\n", cl->usedTotal, cl->usedCritical);

    for (off = 0, count = 0; off >= 0 && off + 4 <= limit && count < 120; ++count) {
        const GfxCmdHeader *h = (const GfxCmdHeader *)((const byte *)cl->cmds + off);
        if (getenv("DBGSPAM"))
            printf("[cmddump] off=%d id=%u bytes=%u", off, h->id, h->byteCount);
        if ((h->id == 1 || h->id == 2) && h->byteCount >= 8) {
            const void *target = *(const void **)((const byte *)h + 4);
            printf(" target=%d", (int)((const byte *)target - (const byte *)cl->cmds));
        } else if (h->id == 21 && h->byteCount >= 48) {
            const GfxCmdDrawText *txt = (const GfxCmdDrawText *)h;
            printf(" text='%.80s' xy=(%.1f,%.1f)", txt->text, txt->x, txt->y);
        }
        printf("\n");

        if (h->id == 0) {
            break;
        }
        if (h->byteCount <= 0) {
            if (getenv("DBGSPAM"))
                printf("[cmddump] stop invalid byteCount at off=%d\n", off);
            break;
        }
        off += h->byteCount;
    }
}

static void R_ResetCmdListState(void)
{
    if (s_cmdList == NULL) {
        return;
    }

    s_cmdList->usedTotal = 0;
    s_cmdList->usedCritical = 0;
    s_cmdList->lastCmd = NULL;
}

static inline __attribute__((always_inline)) Bool R_ClearModifiedDvar(const dvar_t *dvar)
{

    if (!dvar->modified) {
        return 0;
    }

    ri.Dvar_ClearModified(dvar);
    return 1;
}

static inline __attribute__((always_inline)) void R_ResetFrameBackendData(void)
{
    s_cmdList = &frontEndDataOut->commands;
    R_ResetCmdListState();
    frontEndDataOut->drawSurfCount = 0;
    frontEndDataOut->surfPos = 0;
    frontEndDataOut->entityCount = 0;
    frontEndDataOut->polyCount = 0;
    frontEndDataOut->polyVertCount = 0;
    frontEndDataOut->viewParmCount = 0;
    frontEndDataOut->debugGlobals.vertCount = 0;
    frontEndDataOut->debugGlobals.stringCount = 0;
    frontEndDataOut->debugGlobals.lineCount = 0;
}

static inline __attribute__((always_inline)) void R_SelectSkinnedCacheForFrame(DxGlobals *dx)
{
    dx->dynamicBufferFrame = (dx->dynamicBufferFrame + 1) % 2;
    frontEndDataOut->skinnedCacheVb = &dx->skinnedCacheVbPool[dx->dynamicBufferFrame];
    frontEndDataOut->skinnedCacheVb->used = 0;
}

static inline __attribute__((always_inline)) GfxCmdCall *R_AllocDelayedCall(short id, int *marker)
{
    GfxCmdArray *cl = s_cmdList;
    int used;
    GfxCmdCall *cmd;

    used = cl->usedTotal;
    if (marker) {
        *marker = used;
    }

    if (0x30000 - used <= (int)sizeof(GfxCmdCall) - 1) {
        cl->lastCmd = NULL;
        return NULL;
    }

    cmd = (GfxCmdCall *)((char *)cl + used);
    cl->usedTotal = used + (int)sizeof(GfxCmdCall);      /* x86 was 8; GfxCmdCall is 16 on x64 (header+pad+8B ptr) */
    cl->usedCritical += (int)sizeof(GfxCmdCall);
    cl->lastCmd = &cmd->header;
    cmd->header.id = id;
    cmd->header.byteCount = (unsigned short)sizeof(GfxCmdCall);
    return cmd;
}

static inline __attribute__((always_inline)) void *R_AllocCmd(int byteCount, int criticalByteCount, unsigned short id)
{
    GfxCmdArray *cl = s_cmdList;
    int usedBytes;
    int availBytes;
    GfxCmdHeader *cmd;

    usedBytes = cl->usedTotal;
    availBytes = 0x30000 - usedBytes;
    availBytes += cl->usedCritical;
    availBytes -= 0x2000;
    if (availBytes <= byteCount - 1) {
        cl->lastCmd = NULL;
        return NULL;
    }

    cmd = (GfxCmdHeader *)((byte *)cl + usedBytes);
    cl->usedTotal = usedBytes + byteCount;
    cl->usedCritical += criticalByteCount;
    cl->lastCmd = cmd;
    cmd->id = id;
    cmd->byteCount = byteCount;
    return cmd;
}

static inline __attribute__((always_inline)) void *R_AllocCriticalCmd(int byteCount, unsigned short id)
{
    GfxCmdArray *cl = s_cmdList;
    int usedBytes;
    GfxCmdHeader *cmd;

    usedBytes = cl->usedTotal;
    if (0x30000 - usedBytes <= byteCount - 1) {
        cl->lastCmd = NULL;
        return NULL;
    }

    cmd = (GfxCmdHeader *)((byte *)cl + usedBytes);
    cl->usedTotal = usedBytes + byteCount;
    cl->usedCritical += byteCount;
    cl->lastCmd = cmd;
    cmd->id = id;
    cmd->byteCount = byteCount;
    return cmd;
}

void R_ShutdownBackendData(void)
{
    R_ShutdownDebugEntry(&((GfxBackEndData *)s_backEndData)->debugGlobals);
}

static __attribute_regparm__(3) void R_ProcessFrontendCmdInternal(int type, void *data, int isRenderThread)
{
    switch (type) {
    case 0:
        R_UpdateGfxEntityBounds(*(GfxEntity **)data);
        break;
    case 1:
        R_SkinGfxEntity(*(GfxEntity **)data);
        break;
    case 2:
        R_SkinRigidXModelCmd((SkinRigidXModelCmd *)data);
        break;
    case 3:
        FX_UpdateScheduledEffectsNonBolt();
        break;
    case 4:
        FX_UpdateScheduledEffectsBolt();
        break;
    case 5:
        R_SkinStaticModelCachedCmd((SkinStaticModelCachedCmd *)data, ((SkinBuffers *)g_skinBuffers) + isRenderThread);
        break;
    case 6:
    case 7:
        R_SkinXModelCmd((SkinXModelCmd *)data, isRenderThread);
        break;
    default:
        break;
    }
}

void R_AddFrontendCmd(int type, void *data)
{
    R_ProcessFrontendCmdInternal(type, data, 0);
}

void R_SyncRenderThread(void)
{
}

GfxViewParms *R_AllocViewParms(void)
{
    GfxBackEndData *data = frontEndDataOut;
    int index = data->viewParmCount;
    data->viewParmCount = index + 1;
    return &data->viewParms[index];
}

void R_BeginDebugFrame(void)
{
    if (!rg.registered)
        return;
    s_debugFrameGlob.restoreCmdList = s_cmdList;
    s_debugFrameGlob.restoreFrontEndDataOut = frontEndDataOut;
    if (dx.skinnedCacheLockAddr) {
        s_debugFrameGlob.restoreSkinnedCache = 1;
        R_UnlockSkinnedCache();
    }
    s_cmdList = &s_debugFrameGlob.frontEndDataOut.commands;
    frontEndDataOut = &s_debugFrameGlob.frontEndDataOut;
}

void R_AddCmdTouchAllImages(void)
{
    GfxCmdArray *cl = s_cmdList;
    int usedBytes = cl->usedTotal;
    int availBytes = (int)sizeof(cl->cmds) - usedBytes;
    availBytes += cl->usedCritical;
    availBytes -= 0x2000;
    if (availBytes <= 3) {
        cl->lastCmd = NULL;
        return;
    }
    GfxCmdHeader *cmdBuf = (GfxCmdHeader *)((char *)cl->cmds + usedBytes);
    cl->usedTotal = usedBytes + 4;
    cl->lastCmd = cmdBuf;
    cmdBuf->id = 0x21;
    cmdBuf->byteCount = 4;
}

void R_AbortRenderCommands(void)
{
    DxGlobals *dx;
    GfxCmdArray *cl;

    if (!rg.registered) {
        return;
    }

    R_UnlockSkinnedCache();
    cl = &frontEndDataOut->commands;
    cl->usedTotal = 0;
    cl->usedCritical = 0;
    cl->lastCmd = NULL;

    dx = (DxGlobals *)imp_dx;
    if (!dx->tempSkinPos) {
        return;
    }

    ri.Z_VirtualDecommitInternal(dx->tempSkinBuf, dx->tempSkinPos);
    dx->tempSkinPos = 0;
}

static void R_BeginFrame_impl(void)
{
    const dvar_t *forceLod;
    int lodIndex;
    Bool lightsChanged;
    Bool gpuSyncChanged;
    r_global_permanent_t *rgp_p;

    if (!rg.registered) {
        return;
    }

    rgp_p = &rgp;
    if (rgp_p->materialLoaded) {
        rgp_p->materialLoaded = 0;
        Material_Sort();
    }

    if (rgp_p->world != NULL) {
        lightsChanged = R_ClearModifiedDvar(*(const dvar_t **)imp_r_lightTweakAmbient);
        lightsChanged |= R_ClearModifiedDvar(*(const dvar_t **)imp_r_lightTweakDiffuseFraction);
        lightsChanged |= R_ClearModifiedDvar(*(const dvar_t **)imp_r_lightTweakSunLight);
        lightsChanged |= R_ClearModifiedDvar(*(const dvar_t **)imp_r_lightTweakAmbientColor);
        lightsChanged |= R_ClearModifiedDvar(*(const dvar_t **)imp_r_lightTweakSunColor);
        lightsChanged |= R_ClearModifiedDvar(*(const dvar_t **)imp_r_lightTweakSunDiffuseColor);
        lightsChanged |= R_ClearModifiedDvar(*(const dvar_t **)imp_r_lightTweakSunDirection);
        if (lightsChanged) {
            R_UpdateLightsFromDvars();
        }
    }

    if ((*(const dvar_t **)imp_r_sun_from_dvars)->current.enabled && rgp.world != NULL) {
        R_SetSunFromDvars(&rgp.world->sun);
    }

    gpuSyncChanged = R_ClearModifiedDvar(*(const dvar_t **)imp_r_gpuSync);
    gpuSyncChanged |= R_ClearModifiedDvar(*(const dvar_t **)imp_r_multiGpu);
    if (gpuSyncChanged) {
        R_UpdateGpuSyncType();
    }

    forceLod = *(const dvar_t **)imp_r_forceLod;
    if (forceLod->current.integer != forceLod->reset.integer) {
        for (lodIndex = 0; lodIndex < 4; ++lodIndex) {
            ((void (*)(int, float))ri.XModelSetTestLods)(lodIndex, ((*(const dvar_t **)imp_r_forceLod)->current.integer == lodIndex) ? 0.0f : 0.001f);
        }
    } else {
        ((void (*)(int, float))ri.XModelSetTestLods)(0, (*(const dvar_t **)imp_r_highLodDist)->current.value);
        ((void (*)(int, float))ri.XModelSetTestLods)(1, (*(const dvar_t **)imp_r_mediumLodDist)->current.value);
        ((void (*)(int, float))ri.XModelSetTestLods)(2, (*(const dvar_t **)imp_r_lowLodDist)->current.value);
        ((void (*)(int, float))ri.XModelSetTestLods)(3, (*(const dvar_t **)imp_r_lowestLodDist)->current.value);
    }

    ((void (*)(qboolean))ri.CL_FlushDebugData)(0);
    if ((*(const dvar_t **)imp_r_skinCache)->current.enabled) {
        R_LockSkinnedCache(0);
    }
}

void R_BeginFrame(void)
{
    R_BeginFrame_impl();
}

void R_UpdateEffectsBolt(void)
{
    R_ProcessFrontendCmdInternal(4, g_dummyBuf, 0);
}

void R_UpdateEffectsNonBolt(void)
{
    R_ProcessFrontendCmdInternal(3, g_dummyBuf, 0);
}

void R_UpdateXModelBoundsDelayed(GfxEntity *ent)
{
    R_ProcessFrontendCmdInternal(0, &ent, 0);
}

void R_SkinGfxEntityDelayed(GfxEntity *ent)
{
    R_ProcessFrontendCmdInternal(1, &ent, 0);
}

void R_InitBackendData(void)
{
    DxGlobals *dx;
    GfxBackEndData *data;

    R_InitDebugEntry(&((GfxBackEndData *)s_backEndData)->debugGlobals);
    RB_CreateDynamicBuffers();

    if (frontEndDataOut != NULL) {
        GfxCmdArray *cl = &frontEndDataOut->commands;
        cl->usedTotal = 0;
        cl->usedCritical = 0;
        cl->lastCmd = NULL;
        return;
    }

    data = (GfxBackEndData *)s_backEndData;
    frontEndDataOut = data;
    data->frameCount = ++rg.frameCountInternal;

    R_UnlockSkinnedCache();

    dx = (DxGlobals *)imp_dx;
    R_SelectSkinnedCacheForFrame(dx);
    R_ResetFrameBackendData();
}

static void R_EndFrame_impl(void)
{
    DxGlobals *dx;
    GfxBackEndData *data;
    GfxCmdArray *cl;

    if (!rg.registered) {
        return;
    }

    RB_AdaptiveGpuSyncTarget();
    R_UnlockSkinnedCache();

    ri.CL_UpdateDebugData();
    R_TransferDebugGlobals(&frontEndDataOut->debugGlobals);

    R_AllocCriticalCmd(4, 0);

    cl = &frontEndDataOut->commands;
    R_DumpCommandListForMenu(cl);
    cl->usedTotal = 0;
    cl->usedCritical = 0;
    cl->lastCmd = NULL;

    RB_ExecuteRenderCommands(frontEndDataOut);
    RB_EndFrame();

    data = (GfxBackEndData *)s_backEndData;
    frontEndDataOut = data;
    data->frameCount = ++rg.frameCountInternal;

    R_UnlockSkinnedCache();
    dx = (DxGlobals *)imp_dx;
    R_SelectSkinnedCacheForFrame(dx);
    R_ResetFrameBackendData();

    if (dx->tempSkinPos != 0) {
        ri.Z_VirtualDecommitInternal(dx->tempSkinBuf, dx->tempSkinPos);
        dx->tempSkinPos = 0;
    }
}

static const char rb_diag_cmdlist_fmt[] = "";
static int g_endframe_count = 0;
void R_EndFrame(void)
{
    R_EndFrame_impl();
}

void R_AddCmdDrawSurfs(GfxDrawSurf *drawSurfs, int drawSurfCount, MaterialTechniqueType techType)
{
    GfxCmdDrawSurfs *cmd;

    cmd = (GfxCmdDrawSurfs *)R_AllocCmd((int)sizeof(GfxCmdDrawSurfs), 0, 0x17);
    if (cmd == NULL) {
        return;
    }

    cmd->order = GFX_DRAW_SURFS_ITER_FORWARD;
    cmd->drawSurfs = drawSurfs;
    cmd->drawSurfCount = drawSurfCount;
    cmd->techType = techType;
}

int R_BeginDelayedDrawing(void)
{
    int marker;
    GfxCmdCall *cmd;

    cmd = R_AllocDelayedCall(1, &marker);
    cmd->subCmd = NULL;
    return marker;
}

void R_EndDelayedDrawing(int marker)
{
    GfxCmdArray *cl = s_cmdList;
    int used = cl->usedTotal;
    if ((int)sizeof(cl->cmds) - used > 3) {
        GfxCmdHeader *cmd = (GfxCmdHeader *)((char *)cl->cmds + used);
        cl->usedTotal = used + 4;
        cl->usedCritical += 4;
        cl->lastCmd = cmd;
        cmd->id = 3;
        cmd->byteCount = 4;
        used = cl->usedTotal;
    } else {
        cl->lastCmd = NULL;
    }
    /* x86 wrote a 4-byte subCmd at marker+4; on x64 subCmd is an 8-byte ptr at offsetof(GfxCmdCall,subCmd) */
    *(const void **)((char *)cl + marker + __builtin_offsetof(GfxCmdCall, subCmd)) = (const void *)((char *)cl + used);
}

void R_IssueDelayedDrawing(int marker)
{
    GfxCmdCall *cmd;

    cmd = R_AllocDelayedCall(2, NULL);
    cmd->subCmd = (char *)s_cmdList + marker + (int)sizeof(GfxCmdCall);   /* x86 was +8 (goto cmd size) */
}

void R_AddCmdDrawStretchPic(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const vec_t *color, MaterialHandle material)
{
    GfxCmdStretchPic *cmd;

    { extern int g_q_stretchpic; extern void Com_Printf(const char *, ...); g_q_stretchpic++; if (g_q_stretchpic <= 4) Com_Printf("[qsp] #%d material=%p\n", g_q_stretchpic, (void *)material); }
    cmd = (GfxCmdStretchPic *)R_AllocCmd((int)sizeof(GfxCmdStretchPic), 0, 0xf);
    if (cmd == NULL) {
        return;
    }

    cmd->material = material;
    cmd->x = x;
    cmd->y = y;
    cmd->w = w;
    cmd->h = h;
    cmd->s0 = s0;
    cmd->t0 = t0;
    cmd->s1 = s1;
    cmd->t1 = t1;
    R_ConvertColorToBytes(color, cmd->color.array);
}

void R_AddCmdDrawStretchPicRotate(float x, float y, float w, float h, float s0, float t0, float s1, float t1, float angle, const vec_t *color, MaterialHandle material)
{
    GfxCmdStretchPicRotate *cmd;

    cmd = (GfxCmdStretchPicRotate *)R_AllocCmd((int)sizeof(GfxCmdStretchPicRotate), 0, 0x10);
    if (cmd == NULL) {
        return;
    }

    cmd->material = material;
    cmd->x = x;
    cmd->y = y;
    cmd->w = w;
    cmd->h = h;
    cmd->s0 = s0;
    cmd->t0 = t0;
    cmd->s1 = s1;
    cmd->t1 = t1;
    R_ConvertColorToBytes(color, cmd->color.array);
    cmd->rotation = AngleNormalize360(angle);
}

void R_AddCmdDrawStretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty)
{
    GfxCmdStretchRaw *cmd;

    cmd = (GfxCmdStretchRaw *)R_AllocCmd((int)sizeof(GfxCmdStretchRaw), 0, 0x11);
    if (cmd == NULL) {
        return;
    }

    cmd->x = x;
    cmd->y = y;
    cmd->w = w;
    cmd->h = h;
    cmd->cols = cols;
    cmd->rows = rows;
    cmd->data = data;
    cmd->client = client;
    cmd->dirty = dirty;
}

void R_AddCmdDrawTextWithCursor(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style, int cursorPos, int cursor)
{
    int byteCount;
    int len;
    int textIndex;
    char *cmdText;
    GfxCmdDrawText *cmd;

    if (*text == '\0' && cursorPos < 0) {
        return;
    }

    len = strlen(text);

    byteCount = (int)((__builtin_offsetof(GfxCmdDrawText, text) + (unsigned)len + 1 + 3) & ~3u);   /* 0x31 was x86 (font ptr) */
    cmd = (GfxCmdDrawText *)R_AllocCmd(byteCount, 0, 0x15);
    if (cmd == NULL) {
        return;
    }

    cmd->x = x;
    cmd->y = y;
    cmd->font = font;
    cmd->xScale = xScale;
    cmd->yScale = yScale;
    R_ConvertColorToBytes(color, cmd->color.array);
    cmd->style = style;
    cmd->cursorPos = cursorPos;
    cmd->cursor = (char)cursor;
    cmd->maxChars = maxChars;

    cmdText = cmd->text;
    memcpy(cmdText, text, len);
    cmdText[len] = '\0';

    if (R_ShouldTraceTextCmd(cmdText)) {
        printf("[textcmd-front] used=%d bytes=%d text='%.96s' xy=(%.1f,%.1f) scale=(%.3f,%.3f) style=%d max=%d cursor=%d/%d colorBytes=(%u,%u,%u,%u) font=%p\n",
               s_cmdList ? s_cmdList->usedTotal : -1,
               byteCount,
               cmdText,
               x, y, xScale, yScale,
               style, maxChars, cursorPos, cursor,
               cmd->color.array[0], cmd->color.array[1], cmd->color.array[2], cmd->color.array[3],
               font);
    }
}

void R_AddCmdDrawTextInSpace(const char *text, FontHandle font, const vec_t *org, const vec_t *xPixelStep, const vec_t *yPixelStep, const vec_t *color)
{
    int byteCount;
    int len;
    int textIndex;
    char *cmdText;
    GfxCmdDrawTextInSpace *cmd;

    if (*text == '\0') {
        return;
    }

    len = strlen(text);

    byteCount = (int)((__builtin_offsetof(GfxCmdDrawTextInSpace, text) + (unsigned)len + 1 + 3) & ~3u);   /* 0x34 was x86 (font ptr) */
    cmd = (GfxCmdDrawTextInSpace *)R_AllocCmd(byteCount, 0, 0x16);
    if (cmd == NULL) {
        return;
    }

    cmd->org[0] = org[0];
    cmd->org[1] = org[1];
    cmd->org[2] = org[2];
    cmd->font = font;
    cmd->xPixelStep[0] = xPixelStep[0];
    cmd->xPixelStep[1] = xPixelStep[1];
    cmd->xPixelStep[2] = xPixelStep[2];
    cmd->yPixelStep[0] = yPixelStep[0];
    cmd->yPixelStep[1] = yPixelStep[1];
    cmd->yPixelStep[2] = yPixelStep[2];
    R_ConvertColorToBytes(color, cmd->color.array);

    cmdText = cmd->text;
    memcpy(cmdText, text, len);
    cmdText[len] = '\0';
}

void R_AddCmdDrawQuadPic(vec2_t *verts, const vec_t *color, MaterialHandle material)
{
    int cornerIndex;
    GfxCmdDrawQuadPic *cmd;

    cmd = (GfxCmdDrawQuadPic *)R_AllocCmd((int)sizeof(GfxCmdDrawQuadPic), 0, 0x12);
    if (cmd == NULL) {
        return;
    }

    cmd->material = material;
    for (cornerIndex = 0; cornerIndex < 4; ++cornerIndex) {
        cmd->verts[cornerIndex][0] = verts[cornerIndex][0];
        cmd->verts[cornerIndex][1] = verts[cornerIndex][1];
    }
    R_ConvertColorToBytes(color, cmd->color.array);
}

void R_AddCmdDrawSprite(MaterialHandle material, const byte *rgbaColor, const vec_t *pos, float radius, float minScreenRadius, int renderFxFlags)
{
    GfxCmdDrawSprite *cmd;

    cmd = (GfxCmdDrawSprite *)R_AllocCmd((int)sizeof(GfxCmdDrawSprite), 0, 0x13);
    if (cmd == NULL) {
        return;
    }

    cmd->material = material;
    cmd->rgbaColor.packed = *(const unsigned int *)rgbaColor;
    cmd->pos[0] = pos[0];
    cmd->pos[1] = pos[1];
    cmd->pos[2] = pos[2];
    cmd->radius = radius;
    cmd->minScreenRadius = minScreenRadius;
    cmd->renderFxFlags = renderFxFlags;
}

void R_AddCmdDrawFullScreenColoredQuad(float s0, float t0, float s1, float t1, const vec_t *color, MaterialHandle material)
{
    GfxCmdDrawFullScreenColoredQuad *cmd;

    cmd = (GfxCmdDrawFullScreenColoredQuad *)R_AllocCmd((int)sizeof(GfxCmdDrawFullScreenColoredQuad), 0, 0x14);
    if (cmd == NULL) {
        return;
    }

    cmd->material = material;
    cmd->s0 = s0;
    cmd->t0 = t0;
    cmd->s1 = s1;
    cmd->t1 = t1;
    R_ConvertColorToBytes(color, cmd->color.array);
}

void R_AddCmdSetMaterialColor(const vec_t *color)
{
    GfxCmdSetMaterialColor *cmd;

    cmd = (GfxCmdSetMaterialColor *)R_AllocCriticalCmd(0x14, 4);

    if (color != NULL) {
        cmd->color[0] = color[0];
        cmd->color[1] = color[1];
        cmd->color[2] = color[2];
        cmd->color[3] = color[3];
    } else {
        cmd->color[0] = 0.0f;
        cmd->color[1] = 0.0f;
        cmd->color[2] = 0.0f;
        cmd->color[3] = 0.0f;
    }
}

void R_AddCmdLightProperties(int lightIndex, const GfxLight *light)
{
    float scale;
    GfxCmdSetLightProperties *cmd;

    cmd = (GfxCmdSetLightProperties *)R_AllocCriticalCmd(0x4c, 5);

    cmd->lightIndex = lightIndex;
    cmd->position[0] = light->position[0];
    cmd->position[1] = light->position[1];
    cmd->position[2] = light->position[2];
    cmd->position[3] = light->position[3];

    cmd->ambient[0] = light->u.dir.ambientColor[0];
    cmd->ambient[1] = light->u.dir.ambientColor[1];
    cmd->ambient[2] = light->u.dir.ambientColor[2];
    cmd->ambient[3] = 1.0f;

    cmd->color[0] = light->color[0];
    cmd->color[1] = light->color[1];
    cmd->color[2] = light->color[2];
    cmd->color[3] = 1.0f;

    scale = (*(const dvar_t **)imp_r_specularColorScale)->current.value;
    cmd->specular[0] = cmd->color[0] * scale;
    cmd->specular[1] = cmd->color[1] * scale;
    cmd->specular[2] = cmd->color[2] * scale;
    cmd->specular[3] = scale;
    cmd->lightDef = light->def;
}

void R_AddCmdSetRenderTarget(GfxRenderTargetId renderTargetId)
{
    GfxCmdSetRenderTarget *cmd;

    cmd = (GfxCmdSetRenderTarget *)R_AllocCriticalCmd(8, 0xe);
    cmd->renderTargetId = renderTargetId;
}

void R_AddCmdDrawSun(int viewIndex)
{
    GfxCmdDrawSunPostEffects *cmd;

    cmd = (GfxCmdDrawSunPostEffects *)R_AllocCmd(8, 0, 0x18);
    cmd->viewIndex = viewIndex;
}

void R_AddCmdBeginView(int viewCount, const GfxSceneDef *sceneDef, const GfxViewParms *viewParms, const GfxLodParms *lodParms)
{
    GfxCmdBeginView *cmd;

    cmd = (GfxCmdBeginView *)R_AllocCriticalCmd(0x30, 0xc);

    cmd->viewCount = viewCount;
    cmd->sceneDef = *sceneDef;
    cmd->viewParms = viewParms;
    cmd->lodParms = *lodParms;
}

void R_AddCmdSetViewport(int x, int y, int width, int height)
{
    GfxCmdSetViewport *cmd;

    cmd = (GfxCmdSetViewport *)R_AllocCriticalCmd(0x14, 0xd);
    cmd->viewport.x = x;
    cmd->viewport.y = y;
    cmd->viewport.width = width;
    cmd->viewport.height = height;
}

void R_EndDebugFrame(void)
{
    if (!rg.registered) {
        return;
    }

    GfxCmdArray *cl;

    R_AllocCriticalCmd(4, 0);

    cl = &frontEndDataOut->commands;
    cl->usedTotal = 0;
    cl->usedCritical = 0;
    cl->lastCmd = NULL;

    RB_ExecuteRenderCommands(frontEndDataOut);
    RB_EndFrame();

    s_cmdList = s_debugFrameGlob.restoreCmdList;
    frontEndDataOut = s_debugFrameGlob.restoreFrontEndDataOut;
    if (s_debugFrameGlob.restoreSkinnedCache) {
        s_debugFrameGlob.restoreSkinnedCache = 0;
        R_LockSkinnedCache(1);
    }

    s_debugFrameGlob.restoreCmdList = NULL;
    s_debugFrameGlob.restoreFrontEndDataOut = NULL;
}

void R_AddCmdApplyEarlyPostEffects(void)
{
    if (R_AllocCmd(4, 0, 0x19) == NULL) {
        return;
    }
}

void R_AddCmdApplyLatePostEffects(float blurRadius)
{
    GfxCmdApplyLatePostEffects *cmd;

    cmd = (GfxCmdApplyLatePostEffects *)R_AllocCmd(8, 0, 0x1a);
    if (cmd == NULL) {
        return;
    }

    cmd->blurRadius = blurRadius;
}

void R_AddCmdDrawSunPostEffects(int viewIndex)
{
    GfxCmdDrawSunPostEffects *cmd;

    cmd = (GfxCmdDrawSunPostEffects *)R_AllocCmd(8, 0, 0x1b);
    if (cmd == NULL) {
        return;
    }

    cmd->viewIndex = viewIndex;
}

void R_AddCmdClearScreen(int whichToClear, const vec_t *color, float depth, int stencil)
{
    GfxCmdClearScreen *cmd;

    cmd = (GfxCmdClearScreen *)R_AllocCriticalCmd(0x1c, 0xb);

    cmd->whichToClear = (byte)whichToClear;
    cmd->stencil = (byte)stencil;
    cmd->depth = depth;
    cmd->color[0] = color[0];
    cmd->color[1] = color[1];
    cmd->color[2] = color[2];
    cmd->color[3] = color[3];
}

void R_AddCmdSaveScreen(void)
{
    if (R_AllocCriticalCmd(4, 0xa) == NULL) {
        return;
    }
}

void R_AddCmdBlendSavedScreen(int fadeMsec)
{
    GfxCmdBlendSavedScreen *cmd;

    if (fadeMsec <= 0) {
        return;
    }

    cmd = (GfxCmdBlendSavedScreen *)R_AllocCmd((int)sizeof(GfxCmdBlendSavedScreen), 0, 0x1d);
    if (cmd == NULL) {
        return;
    }

    cmd->fadeMsec = fadeMsec;
}
