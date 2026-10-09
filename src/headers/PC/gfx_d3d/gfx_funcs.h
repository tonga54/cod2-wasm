#ifndef CLEAN_PC_GFX_D3D_GFX_FUNCS_H
#define CLEAN_PC_GFX_D3D_GFX_FUNCS_H

#include "common_types.h"
#include "bytematch.h"

#ifndef GFX_FUNCS_LOCAL_TYPEDEFS
#define GFX_FUNCS_LOCAL_TYPEDEFS
typedef int (*GfxImageCompFunc)(GfxImage *, GfxImage *);
typedef void (*CubemapPixelCallback)(const vec_t *facePos, int userData, byte *pixel);
typedef unsigned char (*D3DDispModeCompFunc)(const _D3DDISPLAYMODE *, const _D3DDISPLAYMODE *);
typedef unsigned char (*MaterialCompFunc)(const Material *, const Material *);
typedef struct GfxSurfaceInternal {
    const Material *material;
    unsigned short lightmapIndex;
    unsigned short sortGroup;
    srfTriangles_t *tris;
} GfxSurfaceInternal;
#endif

#ifndef RB_BACKEND_REGPARM1_ABI
#define RB_BACKEND_REGPARM1_ABI COD2_REGPARM(1)
#endif
#ifndef RB_BACKEND_REGPARM2_ABI
#define RB_BACKEND_REGPARM2_ABI COD2_REGPARM(2)
#endif
#ifndef RB_BACKEND_REGPARM2_SSE_ABI
#define RB_BACKEND_REGPARM2_SSE_ABI COD2_REGPARM(2) COD2_SSEREGPARM
#endif
#ifndef RB_BACKEND_REGPARM3_SSE_ABI
#define RB_BACKEND_REGPARM3_SSE_ABI COD2_REGPARM(3) COD2_SSEREGPARM
#endif
#ifndef R_DPVS_REGPARM3_ABI
#define R_DPVS_REGPARM3_ABI COD2_REGPARM(3) BM_NOINLINE
#endif
#ifndef RB_TESS_REGPARM2_ABI
#define RB_TESS_REGPARM2_ABI COD2_REGPARM(2) BM_NOINLINE
#endif
#ifndef RB_TESS_REGPARM3_SSE_ABI
#define RB_TESS_REGPARM3_SSE_ABI COD2_REGPARM(3) COD2_SSEREGPARM BM_NOINLINE
#endif
#ifndef R_MARKS_REGPARM3_ABI
#define R_MARKS_REGPARM3_ABI COD2_REGPARM(3)
#endif
#ifndef MATERIAL_REGPARM2_ABI
#define MATERIAL_REGPARM2_ABI COD2_REGPARM(2)
#endif
#ifndef MATERIAL_REGPARM3_ABI
#define MATERIAL_REGPARM3_ABI COD2_REGPARM(3)
#endif

void R_ResetSunLightOverride(void);
void R_ReleaseWorld(void);
void R_GetWorldBounds(vec_t *min, vec_t *max);
void R_InterpretSunLightParseParams(SunLightParseParams *sunParse);
void R_SetSunLightOverride(const vec_t *sunColor);
IDirect3DVertexBuffer9 *R_CreateWorldVertexBuffer(GfxWorldVertex *vertices, int vertexCount);
void R_ReloadWorld(void);
void R_ShutdownWorld(void);
void R_UpdateLightsFromDvars(void);
void R_LoadWorld(const char *name, int *checksum);
void R_ResetSunLightParseParams(void);

const char *R_ParseSunLight(SunLightParseParams *params, const char *text);

void *R_AllocStaticVertexBuffer(IDirect3DVertexBuffer9 **vb, int sizeInBytes);
void R_FinishStaticVertexBuffer(IDirect3DVertexBuffer9 *vb);
void R_FreeStaticVertexBuffer(IDirect3DVertexBuffer9 *vb);
void *R_AllocStaticIndexBuffer(IDirect3DIndexBuffer9 **ib, int sizeInBytes);
void R_FinishStaticIndexBuffer(IDirect3DIndexBuffer9 *ib);
void R_FreeStaticIndexBuffer(IDirect3DIndexBuffer9 *ib);

int R_CheckDxCaps(const D3DCAPS9 *caps);

void R_RegisterCmds(void);
void R_UnregisterCmds(void);

void R_InitDebugEntry(DebugGlobals *debugGlobalsEntry);
void R_ShutdownDebugEntry(DebugGlobals *debugGlobalsEntry);
void R_TransferDebugGlobals(DebugGlobals *debugGlobalsEntry);
void R_ShutdownDebug(void);
void R_LocateDebugStrings(trDebugString_t *strings, int stringCount, int maxStringCount);
void R_LocateDebugLines(trDebugLine_t *lines, int lineCount, int maxLineCount);
void R_AddDebugString(DebugGlobals *debugGlobalsEntry, const vec_t *origin, const vec_t *color, float scale, const char *string);
void R_AddPlume(const vec_t *origin, int score, const vec_t *color, int duration);
void R_AddDebugPolygon(DebugGlobals *debugGlobalsEntry, const vec_t *color, const int pointCount, vec3_t *points);
void R_AddDebugLine(DebugGlobals *debugGlobalsEntry, const vec_t *start, const vec_t *end, const vec_t *color);
void R_InitDebug(void);
void R_AddDebugBox(DebugGlobals *debugGlobalsEntry, const vec_t *mins, const vec_t *maxs, const vec_t *color);
void R_AddScaledDebugString(DebugGlobals *debugGlobalsEntry, const GfxViewParms *viewParms, const vec_t *origin, const vec_t *color, const char *string);

void R_AddVisSurf_diag_count(void);
int R_AddVisSurf_diag_get(void);
void R_DrawModel(int entIndex);
float R_GetFarPlaneDist(void);
void R_ClearDpvsScene(void);
void R_SetCullDist(float dist);
int R_CellForPoint(const vec_t *origin);
void R_FrustumClipPlanes(const D3DMATRIX *viewProjMtx, vec4_t *sidePlanes, int sidePlaneCount, DpvsPlane *frustumPlanes);
void R_AddWorldSurfacesDpvs(const GfxViewParms *viewParms, int cameraCellIndex);

void R_RegisterDvars(void);
void R_UnregisterDvars(void);

void R_ClearFlares(void);

void R_ClearFogs(void);
void R_SetFog(int fogvar, float start, float end, int r, int g, int b, float density);
void R_SwitchFog(int fogvar, int startTime, int transitionTime);
void R_ArchiveFogState(MemoryFile *memFile);

const Glyph *R_GetCharacterGlyph(FontHandle font, unsigned int letter);
FontHandle R_RegisterFont(const char *fontName, int imageTrack);
int R_DuplicateFont(FontHandle fontCopy, const char *name);
int R_InitFonts(void);
void R_ShutdownFonts(void);
float R_NormalizedTextScale(FontHandle font, float scale);
int R_TextHeight(FontHandle font);
void R_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style);
int R_TextWidth(const char *text, int maxChars, FontHandle font);
int R_ConsoleTextWidth(const short int *string, int maxChars, FontHandle font);
void R_DrawConsoleText(const short int *string, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style);

Font *R_LoadFont(const char *fontName, int imageTrack);

refexport_t *GetRefAPI_v60_adapt(refimport_t *rimp_v60);

void R_GetImageList(ImageList *imageList);
int R_GetMinSpecImageMemory(void);
void R_ResetImageAllocations(void);
void R_FreeImageAllocations(void);
void Image_Create2DTexture(GfxImage *image, int width, int height, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool);
void Image_Create3DTexture(GfxImage *image, int width, int height, int depth, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool);
void Image_CreateCubeTexture(GfxImage *image, int edgeLen, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool);
void Image_PicmipForSemantic(unsigned char semantic, Picmip *picmip);
IDirect3DSurface9 *Image_GetSurface(GfxImage *image);
void R_SetPicmip(void);
water_t *R_LoadWaterSetup(const water_t *water);
void R_DownsampleMipMapBilinear(const byte *src, int srcBufferSize, int srcWidth, int srcHeight, int texelPitch, byte *dst, int dstBufferSize);
void Image_Release(GfxImage *image);
void R_ReloadLostImages(void);
void Image_TrackTexture(GfxImage *image, int imageFlags, D3DFORMAT format, int width, int height, int depth);
void Image_TrackFullscreenTexture(GfxImage *image, int picmip, D3DFORMAT format);
GfxImage *Image_AllocProg(int imageProgType, int category);
GfxImage *Image_Alloc(const char *name, int category, int semantic, int imageTrack);
void R_ImageList_f(void);
GfxImage *Image_Register(const char *imageName, int semantic, int imageTrack);
void R_InitImages(void);
void Image_SetupRenderTarget(GfxImage *image, int width, int height, D3DFORMAT imageFormat);
void Image_SetupSystem(GfxImage *image, int width, int height, D3DFORMAT imageFormat);
void Image_RebuildCosinePowerMap(float shift);
void R_ShutdownImages(void);
void R_ReleaseLostImages(void);
void Image_UpdatePicmip(GfxImage *image);
void Image_Reload(GfxImage *image);
void ZSt13__adjust_heapIPP8GfxImageiS1_PFiS1_S1_EEvT_T0_S6_T1_T2_( GfxImage **first, int holeIndex, int len, GfxImage *value, GfxImageCompFunc comp);
void ZSt16__insertion_sortIPP8GfxImagePFiS1_S1_EEvT_S5_T0_( GfxImage **first, GfxImage **last, GfxImageCompFunc comp);
void ZSt16__introsort_loopIPP8GfxImageiPFiS1_S1_EEvT_S5_T0_T1_( GfxImage **first, GfxImage **last, int depth_limit, GfxImageCompFunc comp);

void Image_GetPicmip(const GfxImage *image, Picmip *picmip);
D3DCUBEMAP_FACES Image_CubemapFace(int face);
int Image_GetCardMemoryAmount(int imageFlags, D3DFORMAT format, int width, int height, int depth);
void Image_Setup(GfxImage *image, int width, int height, int depth, int imageFlags, DWORD usage, D3DFORMAT imageFormat);
void Image_UploadData(GfxImage *image, D3DFORMAT format, int face, int mipLevel, const byte *src);

void Image_Generate2D(GfxImage *image, byte *pixels, int width, int height, int imageFormat);
void Image_Generate3D(GfxImage *image, byte *pixels, int width, int height, int depth, D3DFORMAT imageFormat);
void Image_BuildWaterMap(GfxImage *image);
void Image_LoadFromData(GfxImage *image, GfxImageFileHeader *fileHeader, const byte *srcData);
Bool Image_LoadFromFile(GfxImage *image);
GfxImage *R_CreateWaterMap(char *name, int imageWidth, int imageHeight);
Bool Image_LoadRaw(GfxImage *image, const char *filepath, int imageTrack);
GfxImage *Image_Load(const char *name, int semantic, int imageTrack);

void Wavelet_DecompressLevel(byte *src, byte *dst, WaveletDecode *decode);

void R_FatalInitError(const char *msg);
const char *R_ErrorDescription(HRESULT hr);
void R_UpdateGpuSyncType(void);
void R_EndRegistration(void);
refexport_t *GetRefAPI(int apiVersion, refimport_t *rimp);
void R_Error(errorParm_t errorLevel, const char *msg, ...);
void R_GammaCorrect(byte *buffer, int bufSize);
void R_FatalLockError(HRESULT hr);
void R_SetColorMappings(void);
void R_BeginRegistration(vidConfig_t *vidConfigOut);
Bool R_RecoverLostDevice(void);
void ZSt13__adjust_heapIP15_D3DDISPLAYMODEiS0_PFhRKS0_S3_EEvT_T0_S7_T1_T2_( _D3DDISPLAYMODE *first, int holeIndex, int len, _D3DDISPLAYMODE value, D3DDispModeCompFunc comp);
void ZSt16__insertion_sortIP15_D3DDISPLAYMODEPFhRKS0_S3_EEvT_S6_T0_( _D3DDISPLAYMODE *first, _D3DDISPLAYMODE *last, D3DDispModeCompFunc comp);
void ZSt16__introsort_loopIP15_D3DDISPLAYMODEiPFhRKS0_S3_EEvT_S6_T0_T1_( _D3DDISPLAYMODE *first, _D3DDISPLAYMODE *last, int depth_limit, D3DDispModeCompFunc comp);

boolean empty_output_buffer(j_compress_ptr cinfo);
void R_SaveJpg(const char *filename, int quality, int image_width, int image_height, unsigned char *image_buffer);
void R_LoadJpg(const char *filepath, byte **file, byte **pic, int *width, int *height, D3DFORMAT *imageFormat);

GfxLightDef *R_RegisterLightDef(const char *name);
void R_InitLightDefs(void);
void R_ShutdownLightDefs(void);
int R_GetPointLightPartitions(const GfxDrawSurf *drawSurfs, int drawSurfCount, PointLightPartition *partitions, int partitionLimit);

GfxLightDef *R_LoadLightDef(const char *name);

GfxWorld *R_LoadWorldInternal(const char *name);

int R_MarkFragments(vec3_t *points, const vec_t *origin, vec3_t *axis, float radius, int maxPoints, GfxWorldVertex *verts, int maxFragments, GfxMarkFragment *fragmentBuffer, MaterialHandle markMaterial);

void *Material_Alloc(int size);
const float *Material_RegisterLiteral(const vec_t *literal);
void Material_SetTechnique(const char *name, MaterialTechnique *technique);
void Material_SetTechniqueSet(const char *name, MaterialTechniqueSet *techniqueSet);
void Material_SetStateMap(const char *name, MaterialStateMap *stateMap);
void Material_SetShader(const char *shaderName, MaterialShaderType shaderType, int shaderVersion, MaterialShader *mtlShader);
Bool Material_IsDefault(const Material *material);
Bool R_IsMaterialRefractive(MaterialHandle handle);
void Material_FinishLoading(void);
void Material_ReleaseAll(void);
void Material_UpdatePicmipAll(void);
int Material_LoadFile(const char *filename, int *file);
const char *R_GetMaterialName(_ValueType handle);
int R_GetMaterialSubimageCount(MaterialHandle handle);
void Material_Sort(void);
const char *Material_RegisterString(const char *string);
MaterialVertexDeclaration *Material_AllocVertexDecl(MaterialStreamRouting *routingData, int streamCount, Bool *existing);
MaterialStateMap *Material_FindStateMap(const char *name);
MaterialTechniqueSet *Material_FindTechniqueSet(const char *name);
MaterialTechnique *Material_FindTechnique(const char *name);
MaterialShader *Material_FindShader(const char *shaderName, MaterialShaderType shaderType, int shaderVersion);
void Material_Shutdown(void);
MaterialHandle R_RegisterRawImage(const char *name, int baseImageFlags, int imageTrack);
void Material_ReloadAll(void);
void Load_BuildVertexDecl(MaterialVertexDeclaration **mtlVertDecl);
void R_Cmd_ReloadMaterialTextures(void);
MaterialHandle Material_Duplicate(MaterialHandle mtlCopy, const char *name);
MaterialHandle Material_Register(const char *name, int imageTrack);
MaterialHandle Material_RegisterHandle(const char *name, int baseImageFlags, int imageTrack);
void Material_Init(void);
void ZSt13__adjust_heapIPP8MaterialiS1_PFhPKS0_S4_EEvT_T0_S8_T1_T2_( Material **first, int holeIndex, int len, Material *value, MaterialCompFunc comp);
void ZSt16__insertion_sortIPP8MaterialPFhPKS0_S4_EEvT_S7_T0_( Material **first, Material **last, MaterialCompFunc comp);
void ZSt16__introsort_loopIPP8MaterialiPFhPKS0_S4_EEvT_S7_T0_T1_( Material **first, Material **last, int depth_limit, MaterialCompFunc comp);

HRESULT IncludeClass_Close(const IncludeClass *_this, LPCVOID data);
HRESULT IncludeClass_Open(const IncludeClass *_this, D3DXINCLUDE_TYPE IncludeType, LPCSTR filename, LPCVOID parentData, LPCVOID *data, MaterialTechnique *(*byteCount)[4][34]);
void Material_PreLoadAllShaderText(void);

struct XModel *R_RegisterModel(const char *name);
GfxBrushModel *R_RegisterInlineModel(int modelIndex);
void R_SetIgnorePrecacheErrors(qboolean ignore);
qboolean R_GetIgnorePrecacheErrors(void);
void R_UnlockSkinnedCache(void);
struct DObj_s *R_GetGfxEntityDObj(GfxSceneEntity *sceneEnt, GfxEntity *ent);
void R_DObjReplaceMaterial(struct DObj_s *obj, int lod, int surfaceIndex, MaterialHandle material);
void R_OptimizeAllModels(void);
void R_ReleaseAllModels(void);
void *Model_Alloc(int size);
void R_GetRigidTransform(const DObjSkelMat *bone, const vec_t *origin, vec3_t *axis, float scale, vec3_t *boneAxis);
void R_ModelBounds(GfxBrushModel *bmodel, vec_t *mins, vec_t *maxs);
void R_LockSkinnedCache(GfxLockType lockType);
void R_ShutdownModels(void);
void R_FinishLoadingModels(void);
void R_DObjGetSurfMaterials(struct DObj_s *obj, int lod, MaterialHandle *matHandleArray);
void R_UpdateXModelBounds(GfxSceneEntity *sceneEnt, GfxEntity *ent);
void R_SkinSceneDObj(GfxSceneEntity *sceneEnt, GfxEntity *ent);
void R_SkinXModel(GfxSceneEntity *sceneEnt, GfxEntity *ent, int smodelIndex);
void R_SkinStaticModel(GfxSceneEntity *sceneEnt, GfxEntity *ent, int smodelIndex);
void R_SkinSceneEnt(GfxSceneEntity *sceneEnt, GfxEntity *ent);
void R_SkinXModelCmd(SkinXModelCmd *skinCmd, int context);
void R_SkinRigidXModelCmd(SkinRigidXModelCmd *skinRigidCmd);

trXSkin_t *R_LoadXSkins(struct XModel *model);

void R_RegisterOutdoorImage(GfxWorld *world);
void R_GenerateOutdoorImage(GfxImage *outdoorImage);

void R_ShutdownBackendData(void);
void R_AddFrontendCmd(int type, void *data);
void R_SyncRenderThread(void);
GfxViewParms *R_AllocViewParms(void);
void R_BeginDebugFrame(void);
void R_AddCmdTouchAllImages(void);
void R_AbortRenderCommands(void);
void R_BeginFrame(void);
void R_UpdateEffectsBolt(void);
void R_UpdateEffectsNonBolt(void);
void R_UpdateXModelBoundsDelayed(GfxEntity *ent);
void R_SkinGfxEntityDelayed(GfxEntity *ent);
void R_InitBackendData(void);
void R_EndFrame(void);
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

const char *R_DescribeFormat(int format);
void R_ShutdownRenderTargets(void);
void R_InitRenderTargets(void);

void R_UpdateGfxEntityBounds(GfxEntity *ent);
void R_SkinGfxEntity(GfxEntity *ent);
void R_DecomposeSort(unsigned int sortValue, int *entIndex, const Material **material, int *lmapIndex);
void R_ClearScene(void);
void R_AddLightToScene(const vec_t *org, float radius, float r, float g, float b);
void R_DefaultVertexFrames(int vertCount, GfxWorldVertex *verts);
void qsortDrawSurfs(GfxDrawSurf *drawSurfs, int drawSurfCount);
void R_AddXModelSurfaces(int entIndex);
void R_AddBModelSurfaces(GfxSceneEntity *sceneEnt, int entIndex);
void R_AddPolyToScene(MaterialHandle materialHandle, int lmapIndex, int vertCount, const GfxWorldVertex *verts);
void R_dpvs_diag_print(int cameraCellIndex, int drawWorld, int cellPtr);
void R_AddDrawSurfForSurface(GfxSurface *surf, int entIndex);
void R_SetLodOrigin(const refdef_t *refdef);
void R_RenderScene(const refdef_t *refdef);
int R_AddStaticModelToScene(int smodelIndex);
GfxEntity *R_AddRefEntityToScene(const GfxEntity *refEnt, GfxModel sceneModel, const struct centity_s *cent);

void R_BeginCubemapShot(const int pixelWidthHeight, const int pixelBorder);
void R_EndCubemapShot(const CubemapShot shotIndex);
void R_LevelShot(void);
void R_SaveCubemapShot(const char *filename, const CubemapShot shotIndex, const float n0, const float n1);
void R_LightingFromCubemapShots(const vec_t *baseColor);
void R_CapturePendingScreenshotBeforePresent(void);
int R_HasPendingScreenshot(void);
void R_ScreenshotCommand(GfxScreenshotType type);

void R_ResetShadowCookies(void);

int R_GetSundvarsSize(void);
void R_RegisterSunDvars(void);
void R_Cmd_LoadSun(void);
void R_Cmd_SaveSun(void);
void R_FlushSun(void);
void R_SetSunFromDvars(sunflare_t *sun);

void R_LoadSunThroughDvars(const char *sunName, sunflare_t *sun);
void R_LoadSun(const char *name, sunflare_t *sun);

void Image_BuildSpecularityMap(float shift, byte *pic);

void R_InitDrawGroups(void);
GfxDrawGroupGlueBehavior R_BeginDrawGroupSection(GfxDrawGroupType group);
void R_BeginDrawGroupLoop(GfxDrawGroupType group, int viewIndex);
GfxDrawGroupGlueBehavior R_EndDrawGroupLoop(GfxDrawGroupType group, int viewIndex);
void R_EndDrawGroupSection(GfxDrawGroupType group);
void R_IssueDrawGroups(void);

void R_InitStaticModelDynamicData(int smodelIndex);

int R_ScaleStaticModelLighting(float directLightScale, float indirectLightScale, float *sunVisibility, vec4_t *colorForDir);
void R_GetStaticModelLightingFromGrid(const GfxWorld *world, GfxStaticModelInstance *smodelInst, float *sunVisibility, vec4_t *colorForDir);
int R_PrepareStaticModelLightingCache(GfxWorld *world, int smodelCount);
Bool R_ValidateStaticModel(struct XModel *model);
int R_FinishStaticModelLightingCache(GfxWorld *world);
int R_GetStaticModelLightingFromGround(const vec_t *groundLight, float *sunVisibility, vec4_t *colorForDir);
void R_CreateStaticModel(GfxWorld *world, struct XModel *model, const vec_t *origin, const vec_t *angles, vec_t scale, GfxStaticModelInstance *smodelInst);
void R_CacheStaticModelLighting(const GfxWorld *world, GfxStaticModelInstance *smodelInst, float sunVisibility, vec4_t *colorForDir);
int R_SortGfxAabbTree(GfxWorld *world, GfxAabbTree *tree);
int R_AllocStaticModels(GfxAabbTree *tree);

void R_InitStaticModelIndexCache(void);
void R_StaticModelCacheStats_f(void);
void R_UsedCachedStaticModelSurface(GfxStaticModelSurfaceCached *surf);
void R_InitStaticModelCache(void);
void R_StaticModelCacheFlush_f(void);
void R_ShutdownStaticModelCache(void);
GfxStaticModelSurfaceCached *R_CacheStaticModelSurface(GfxStaticSurface *staticSurf, const XSurface *xsurf, int smodelIndex, const Material *material);
void R_FlushStaticModelCache(void);
void R_SkinStaticModelCachedCmd(SkinStaticModelCachedCmd *skinCmd, SkinBuffers *skinBuffers);

const vec_t *R_BoundsForSurf_Triangles(const GfxDrawSurf *drawSurf, int entIndex);
const vec_t *R_BoundsForSurf_ModelInst(const GfxDrawSurf *drawSurf, int entIndex);
const vec_t *R_BoundsForSurf_StaticModelCached(const GfxDrawSurf *drawSurf, int entIndex);
const vec_t *R_BoundsForDrawSurf(const GfxDrawSurf *surf);

unsigned int R_AvailableTextureMemory(void);

int R_HashAssetName(const char *name);
int R_HashString(const char *string);
int R_CullPointAndRadius(const vec_t *pt, float radius, const DpvsPlane *clipPlanes, int clipPlaneCount);
void R_ConvertColorToBytes(const vec_t *colorFloat, byte *colorBytes);
qboolean R_PickMaterial(const vec_t *org, const vec_t *dir, char *name, char *surfaceFlags, char *contents, int charLimit);
Bool R_ValidXModelName(const char *name);
float FresnelTerm(float n0, float n1, float cosIncidentAngle);

void RB_UploadWaterTexture(GfxImage *image, water_t *water);

qboolean R_WatersEquivalent(const water_t *w0, const water_t *w1);
void R_CreateWaterSetup(const water_t *source, int waterMapSetupIndex, water_t *destination);

void *R_LoadXModelSurfsSurface(void *surfsCtx, void *partBitsArg, int **streamCursor, void *(*alloc)(int size));
void R_XModelSurfs_SetRefImport(void *rimp_v60);
void *R_RegisterXModelSurfs(void *xmodel);

int XSurfaceGetNumVerts(const XSurface *surface);
int XSurfaceGetNumTris(const XSurface *surface);
void XSurfaceGetTris(const XSurface *surface, r_index_t *dstIndices, int offset);
XVertexInfo *XSurfaceGetVertexInfoArray(const XSurface *surf);
int XSurfaceGetBoneOffset(const XSurface *surf);
void XSurfaceTransferDx7(const XVertexBuffer *surfVerts, GfxVertexDx7 *verts, int vertCount);
void XSurfaceTransfer(const XVertexBuffer *surfVerts, GfxVertex *verts, int vertCount);
long unsigned int XSurfaceGetVerts(const XSurface *surf, DObjSkelMat *boneMatrix, float *pVert, float *pTexCoord, float *pNormal);

XSurface *XModelReadSurface(XModel *model, int *partBits, const byte **pos, Alloc_t Alloc);

void XSurfaceOptimizeRigid(XModel *model, XSurface *surface, XVertexBuffer *surfVerts);
void XModelOptimize(XModel *model);
void XModelUnoptimize(XModel *model);

void rdsl_log_null_technique2(void *material, int techType);
void RB_EndFrame(void);
void RB_SetCodeConstant(int constant, vec_t x, vec_t y, vec_t z, vec_t w);
void RB_SetGammaRamp(const GfxGammaRamp *gammaTable);
qboolean RB_IsGpuFenceFinished(void);
void RB_GpuWaited(int ticks);
long long QueryPerf(void);
void RB_InitBackendGlobalStructs(void);
void RB_RegisterBackendAssets(void);
void RB_LookupColor(int c, byte *color);
void RB_ClearScreen(int whichToClear, const vec_t *color, float depth, int stencil);
void RB_UpdateColor(const vec_t *color_allies, const vec_t *color_axis);
void RB_AdaptiveGpuSyncWait(void);
void RB_AdaptiveGpuSyncTarget(void);
void RB_Set3D(void);
void RB_DrawLines2D(int count, int width, const GfxPointVertex *verts);
void RB_DrawTextInSpace(const char *text, FontHandle font, const vec_t *org, const vec_t *xPixelStep, const vec_t *yPixelStep, D3DCOLOR color);
void RB_DrawStretchPic(const Material *material, float x, float y, float w, float h, float s0, float t0, float s1, float t1, D3DCOLOR color, GfxPrimStatsTarget statsTarget);
void RB_ExecuteRenderCommands(const void *data);
void RB_DrawFullScreenColoredQuad(const Material *material, float s0, float t0, float s1, float t1, D3DCOLOR color);
void RB_DrawLines3D(int count, int width, const GfxPointVertex *verts, int depthTest);

void RB_DrawDebug(const GfxViewParms *viewParms);

GfxFogOffset RB_FogOffset(void);
void RB_UpdateFogColor(FogColorSrcEnum fogColorSrc);
void RB_SetIteratorFog(void);

void RB_GlowFilterImage(float *radius);
void RB_GaussianFilterImage(float radius, GfxRenderTargetId renderTargetId);

void RB_TouchAllImages(void);

void RB_SaveLightVisHistory(void);
int RB_DeriveEntityLights(vec4_t *colorForDir, float sunVisibility, const Material *material, D3DLIGHT9 *lights, int maxLights);
void RB_ShowLightVisCachePoints(const vec_t *viewOrigin, const DpvsPlane *clipPlanes, int clipPlaneCount);
float RB_GetLightingAtPoint(const GfxLightGrid *lightGrid, const vec_t *samplePos, vec4_t *colorForDir);
void RB_SetupEntityLighting(const GfxEntity *ent, GfxEntityLighting *lighting);
void RB_InitLightVisHistory(const char *bspName);

void RB_BeginSurface(const Material *material, MaterialTechniqueType techType, int lmapIndex);
int RB_SetIndexData(const r_index_t *indices, int indexCount);
void RB_DrawIndexedPrim(const GfxDrawPrimArgs *args, int primCount);
void RB_CreateDynamicBuffers(void);
void RB_SetVertexData(unsigned int streamIndex, const void *data, int vertexCount, int stride);
void RB_DrawTechnique(MaterialVertexDeclType vertDeclType, const GfxDrawPrimArgs *args);
void diag_idxzero(void *tess_base);
void diag_endsurface_entry(void *tess_base);
void RB_EndSurface(void);

int RB_CalcSunSpriteSamples(void);
void RB_DrawSunPostEffects(int viewIndex);
void RB_DrawSun(int viewIndex);

void RB_ChangeIndices(IDirect3DIndexBuffer9 *ib);
void RB_ChangeStreamSource(int streamIndex, IDirect3DVertexBuffer9 *vb, int vertexOffset, int vertexStride);
void RB_DecideDefaultSamplerState(void);
void RB_SetAnisotropy(void);
void RB_SetAlphaAntiAliasingState(int stateBits0);
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
void COD2_FORCE_ALIGN_ARG_POINTER RB_SetInitialState(void);
void RB_SetWorldMatrixForEntity(const GfxEntity *re);

void RB_TessBad(const surfaceType_t *surfType);
void RB_TessParticleCloud(const GfxEntity *re);
void RB_TessXModelRigid(const surfaceType_t *surfType);
void RB_TessBackEndEntity(const surfaceType_t *surfType);
void RB_TessPoly(const surfaceType_t *surfType);
void RB_TessStaticModelCached(const surfaceType_t *surfType);
void RB_TessXModelSkinned(const surfaceType_t *surfType);
void RB_TessTriangles(const surfaceType_t *surfType);
void RB_TessEntity(const GfxEntity *re);

#endif
