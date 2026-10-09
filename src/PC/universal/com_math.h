#ifndef PC_UNIVERSAL_COM_MATH_H_H
#define PC_UNIVERSAL_COM_MATH_H_H

#include "Mac/DirectX 9/MacOpenGLUtils.h"
#include "PC/gfx_d3d/rb_backend.h"
#include "PC/universal/com_vector.h"

struct ClientVoicePacket_t;
struct DObjTrace_s;
struct DObj_s;
struct EffectVisInfo;
struct FxCamera;
struct FxHelper;
struct GfxGammaRamp;
struct GfxViewport;
struct MarkPoly;
struct SndCurve;
struct SoundFile;
struct TraceCheckCount;
struct TraceExtents;
struct TraceThreadInfo;
struct XAnimDeltaPart;
struct XAnimDeltaPartQuat;
union XAnimDeltaPartQuatData;
struct XAnimDeltaPartQuatDataFrames;
struct XAnimDeltaPartTrans;
union XAnimDeltaPartTransData;
struct XAnimDeltaPartTransFrames;
union XAnimDynamicIndices;
struct XAnimEntry;
struct XAnimNotifyInfo;
struct XAnimParent;
struct XAnimPart;
struct XAnimPartQuat;
union XAnimPartQuatData;
struct XAnimPartQuatDataFrames;
union XAnimPartQuatFrames;
struct XAnimPartTrans;
union XAnimPartTransData;
struct XAnimPartTransFrames;
struct XAnimParts_s;
struct XAnimTree;
struct XAnimTree_s;
struct XAnim_s;
struct animation_s;
struct cLeaf_t;
struct cameraShake_t;
struct clientInfo_t;
struct compassactor_t;
struct displayContextDef_s;
struct dmodel_t;
struct dnode_t;
struct gitem_s;
struct lerpFrame_t;
struct moveclip_t;
struct objective_t;
struct refexport_t;
struct refimport_t;
struct shellshock_parms_t;
struct sightclip_t;
struct sightpointtrace_t;
struct snd_alias_list_t;
struct snd_alias_t;
struct snd_listener;
struct snd_overlay_info_t;
struct snd_save_2D_sample_t;
struct snd_save_3D_sample_t;
struct snd_save_stream_t;
struct traceWork_t;
struct vidConfig_t;
struct viewState_t;
struct weaponInfo_s;

enum CubemapShot {
    CUBEMAPSHOT_NONE = 0,
    CUBEMAPSHOT_UP = 1,
    CUBEMAPSHOT_DOWN = 2,
    CUBEMAPSHOT_LEFT = 3,
    CUBEMAPSHOT_RIGHT = 4,
    CUBEMAPSHOT_FRONT = 5,
    CUBEMAPSHOT_BACK = 6,
    CUBEMAPSHOT_COUNT = 7
};

enum DemoType {
    DEMO_TYPE_NONE = 0,
    DEMO_TYPE_CLIENT = 1,
    DEMO_TYPE_SERVER = 2
};

enum errorParm_t {
    ERR_FATAL = 0,
    ERR_DROP = 1,
    ERR_SERVERDISCONNECT = 2,
    ERR_DISCONNECT = 3,
    ERR_SCRIPT = 4,
    ERR_SCRIPT_DROP = 5,
    ERR_LOCALIZATION = 6
};

enum itemType_t {
    IT_BAD = 0,
    IT_WEAPON = 1,
    IT_AMMO = 2,
    IT_HEALTH = 3
};

enum snd_alias_type_t {
    SAT_UNKNOWN = 0,
    SAT_LOADED = 1,
    SAT_STREAMED = 2,
    SAT_PRIMED = 3,
    SAT_COUNT = 4
};

enum team_t {
    TEAM_FREE = 0,
    TEAM_AXIS = 1,
    TEAM_ALLIES = 2,
    TEAM_SPECTATOR = 3,
    TEAM_NUM_TEAMS = 4
};

struct ClientVoicePacket_t {
    int dataSize;
    byte data[256];
};

struct DObjTrace_s {
    float fraction;
    int surfaceflags;
    vec3_t normal;
    short unsigned int partName;
    short unsigned int partGroup;
};

struct DObj_s {
    XAnimTree *tree;
    DSkel *skel;
    int timeStamp;
    short unsigned int *animToModel;
    short unsigned int duplicateParts;
    volatile int locked;
    unsigned char numModels;
    unsigned char numBones;
    unsigned char ignoreCollision;
    XModel *models[8];
    unsigned char modelParents[8];
    unsigned char matOffset[8];
    vec3_t mins;
    vec3_t maxs;
};

struct EffectVisInfo {
    vec3_t origin;
    float distSq;
    float vis;
};

struct FxCamera {
    vec3_t vieworg;
    vec4_t frustum[6];
    int numPlanes;
};

struct FxHelper {
    int time;
    int mTime;
    int mOldTime;
    int mFrameTime;
    qboolean mTimeFrozen;
    struct FxCamera mCamera;
    FxCamera mPrevCamera;
    int mSeed;
    float adsZoomFactor;
};

struct GfxGammaRamp {
    short unsigned int entries[256];
};

struct GfxViewport {
    int x;
    int y;
    int width;
    int height;
};

struct MarkPoly {
    int prevMark;
    int nextMark;
    int lastFrameDrawn;
    vec3_t origin;
    float radius;
    MaterialHandle mtlHandle;
    short unsigned int lmapIndex;
    byte vertCount;
    GfxWorldVertex verts[9];
};

struct SndCurve {
    const char *filename;
    int knotCount;
    vec2_t knots[8];
};

struct SoundFile {
    const char *soundName;
    struct MssSound *fileMem;
    byte isStreamFound;
    snd_alias_type_t type;
};

struct TraceCheckCount {
    int global;
    int *edges;
    int *verts;
    short unsigned int *partitions;
};

struct TraceExtents {
    vec3_t start;
    vec3_t end;
    vec3_t invDelta;
};

struct TraceThreadInfo {
    struct TraceCheckCount checkcount;
    cbrush_t *box_brush;
    cmodel_t *box_model;
};

struct XAnimDeltaPart {
    XAnimDeltaPartTrans *trans;
    XAnimDeltaPartQuat *quat;
};

struct XAnimDeltaPartQuat {
    short unsigned int size;
    union XAnimDeltaPartQuatData u;
};

union XAnimDeltaPartQuatData {
    struct XAnimDeltaPartQuatDataFrames frames;
    XQuat2 frame0;
};

struct XAnimDeltaPartQuatDataFrames {
    XQuat2 *frames;
    XAnimDynamicIndices indices;
};

struct XAnimDeltaPartTrans {
    short unsigned int size;
    union XAnimDeltaPartTransData u;
};

union XAnimDeltaPartTransData {
    struct XAnimDeltaPartTransFrames frames;
    vec3_t frame0;
};

struct XAnimDeltaPartTransFrames {
    vec3_t *frames;
    XAnimDynamicIndices indices;
};

union XAnimDynamicIndices {
    byte _1[1];
    short unsigned int _2[1];
};

struct XAnimEntry {
    short unsigned int numAnims;
    short unsigned int parent;
    int u;
};

struct XAnimNotifyInfo {
    ScriptString name;
    float time;
};

struct XAnimParent {
    short unsigned int flags;
    short unsigned int children;
};

struct XAnimPart {
    XAnimPartTrans *trans;
    XAnimPartQuat *quat;
};

struct XAnimPartQuat {
    short unsigned int size;
    union XAnimPartQuatData u;
};

union XAnimPartQuatData {
    struct XAnimPartQuatDataFrames frames;
    XQuat frame0;
    XQuat2 frame02;
};

struct XAnimPartQuatDataFrames {
    union XAnimPartQuatFrames u;
    XAnimDynamicIndices indices;
};

union XAnimPartQuatFrames {
    XQuat *frames;
    XQuat2 *frames2;
};

struct XAnimPartTrans {
    short unsigned int size;
    union XAnimPartTransData u;
};

union XAnimPartTransData {
    struct XAnimPartTransFrames frames;
    vec3_t frame0;
};

struct XAnimPartTransFrames {
    vec3_t *frames;
    union XAnimDynamicIndices indices;
};

struct XAnimParts_s {
    short unsigned int numframes;
    Bool bLoop;
    Bool bDelta;
    float framerate;
    float frequency;
    byte notifyCount;
    short int boneCount;
    short unsigned int *names;
    char *simpleQuatBits;
    XAnimPart *parts;
    XAnimNotifyInfo *notify;
    XAnimDeltaPart *deltaPart;
    const char *name;
    Bool isDefault;
};

struct XAnimTree {
    DSkel **reset_marker_reader;
    const XAnimTime read_markers;
    const int read_restart_marker;
    const XAnimParts_s *saw_SOI;
    const XAnimParts_s *saw_SOF;
    XAnimPart next_restart_num;
    float discarded_bytes;
};

struct XAnimTree_s {
    XAnim *anims;
    short unsigned int entnum;
    Bool bAbs;
    Bool bUseGoalWeight;
    short unsigned int infoArray[1];
};

struct XAnim_s {
    const char *debugName;
    unsigned int size;
    const char **debugAnimNames;
    XAnimEntry entries[1];
};

struct animation_s {
    char name[64];
    int initialLerp;
    float moveSpeed;
    int duration;
    int nameHash;
    int flags;
    long long int movetype;
    int noteType;
};

struct cLeaf_t {
    short unsigned int firstCollAabbIndex;
    short unsigned int collAabbCount;
    int brushContents;
    int terrainContents;
    vec3_t mins;
    vec3_t maxs;
    int leafBrushNode;
    short int cluster;
};

struct cameraShake_t {
    int time;
    float scale;
    float length;
    float radius;
    vec3_t src;
    float size;
    float rumbleScale;
};

struct clientInfo_t {
    qboolean infoValid;
    qboolean nextValid;
    int clientNum;
    char name[32];
    team_t team;
    team_t oldteam;
    int score;
    int location;
    int health;
    char model[64];
    char attachModelNames[6][64];
    char attachTagNames[6][64];
    struct lerpFrame_t legs;
    lerpFrame_t torso;
    float lerpMoveDir;
    float lerpLean;
    vec3_t playerAngles;
    qboolean leftHandGun;
    qboolean dobjDirty;
    vec3_t angles[6];
    vec3_t tag_origin_angles;
    vec3_t tag_origin_offset;
    int clientConditions[9][2];
    struct XAnimTree_s *pXAnimTree;
    int iDObjWeapon;
    int stanceTransitionTime;
    int turnAnimEndTime;
    char turnAnimType;
};

struct compassactor_t {
    int iLastUpdate;
    vec2_t vLastPos;
    float fLastYaw;
    int pingTime;
    int beginFadeTime;
    Bool enemy;
};

struct displayContextDef_s {
    float bias;
    int realTime;
    int frameTime;
    int cursorx;
    int cursory;
    qboolean debug;
    int screenWidth;
    int screenHeight;
    float screenAspect;
    float FPS;
    float blurRadiusOut;
    menuDef_t *Menus[128];
    int menuCount;
    menuDef_t *menuStack[16];
    int openMenuCount;
};

struct dmodel_t {
    float mins[3];
    float maxs[3];
    int firstTriangle;
    int numTriangles;
    int firstSurface;
    int numSurfaces;
    int firstBrush;
    int numBrushes;
};

struct dnode_t {
    int planeNum;
    int children[2];
    int mins[3];
    int maxs[3];
};

struct gitem_s {
    char *classname;
    char *pickup_sound;
    char *world_model[2];
    char *icon;
    char *pickup_name;
    int quantity;
    itemType_t giType;
    int giTag;
    int giAmmoIndex;
    int giClipIndex;
};

struct lerpFrame_t {
    float yawAngle;
    qboolean yawing;
    float pitchAngle;
    qboolean pitching;
    int animationNumber;
    animation_t *animation;
    int animationTime;
    vec3_t oldFramePos;
    float animSpeedScale;
    int oldFrameSnapshotTime;
};

struct moveclip_t {
    vec3_t mins;
    vec3_t maxs;
    vec3_t outerSize;
    TraceExtents extents;
    int passEntityNum;
    int passOwnerNum;
    int contentmask;
};

struct objective_t {
    objectiveState_t state;
    vec3_t origin;
    int entNum;
    int teamNum;
    int icon;
};

struct refexport_t {
    void (*Shutdown)(void);
    void (*BeginRegistration)(void *vidConfigOut);
    struct XModel *(*RegisterModel)(const char *name);
    GfxBrushModel *(*RegisterInlineModel)(int modelIndex);
    MaterialHandle (*RegisterMaterial)(const char *name, int baseImageFlags, int imageTrack);
    MaterialHandle (*RegisterRawImage)(const char *name, int baseImageFlags, int imageTrack);
    Bool (*Material_IsDefault)(MaterialHandle handle);
    void (*LoadWorld)(const char *name, int *checksum);
    void (*GetWorldBounds)(vec_t *min, vec_t *max);
    void (*FinishLoadingModels)(void);
    void (*SetIgnorePrecacheErrors)(qboolean ignore);
    qboolean (*GetIgnorePrecacheErrors)(void);
    int (*GetMinSpecImageMemory)(void);
    const char *(*GetMaterialName)(MaterialHandle handle);
    int (*GetMaterialSubimageCount)(MaterialHandle handle);
    Bool (*IsMaterialRefractive)(MaterialHandle handle);
    float (*GetFarPlaneDist)(void);
    void (*EndRegistration)(void);
    void (*ClearScene)(void);
    void (*DefaultVertexFrames)(int vertCount, GfxWorldVertex *verts);
    void (*AddPolyToScene)(MaterialHandle materialHandle, int lmapIndex, int vertCount, const GfxWorldVertex *verts);
    void (*AddLightToScene)(const vec_t *org, float radius, float r, float g, float b);
    void (*InterpretSunLightParseParams)();
    void (*ResetSunLightParseParams)(void);
    void (*SetCullDist)(float dist);
    void (*SetFog)(int fogvar, float start, float end, int r, int g, int b, float density);
    void (*SwitchFog)(int fogvar, int startTime, int transitionTime);
    void (*ArchiveFogState)(void *memFile);
    void (*ClearFogs)(void);
    void (*SetSunLightOverride)(const vec_t *sunColor);
    void (*ResetSunLightOverride)(void);
    void (*RenderScene)(const refdef_t *refdef);
    int (*BeginDelayedDrawing)(void);
    void (*EndDelayedDrawing)(int marker);
    void (*IssueDelayedDrawing)(int marker);
    void (*ClearFlares)(void);
    void (*SetMaterialColor)(const vec_t *color);
    void (*DrawStretchPic)(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const vec_t *color, MaterialHandle material);
    void (*DrawStretchPicRotate)(float x, float y, float w, float h, float s0, float t0, float s1, float t1, float angle, const vec_t *color, MaterialHandle material);
    void (*DrawStretchRaw)();
    void (*DrawQuadPic)(void *verts, const vec_t *color, MaterialHandle material);
    void (*DrawSprite)(MaterialHandle material, const byte *rgbaColor, const vec_t *pos, float radius, float minScreenRadius, int renderFxFlags);
    void (*BeginFrame)(void);
    void (*EndFrame)(void);
    void (*BeginDebugFrame)(void);
    void (*EndDebugFrame)(void);
    void (*EndView)(int viewIndex);
    void (*DoneRenderingViews)(void);
    void (*SaveScreen)(void);
    void (*BlendSavedScreen)(int fadeMsec);
    void (*ClearScreen)(int whichToClear, const vec_t *color, float depth, int stencil);
    void (*SetViewport)(int x, int y, int width, int height);
    int (*MarkFragments)(void *points, const vec_t *origin, void *axis, float radius, int maxPoints, GfxWorldVertex *verts, int maxFragments, void *fragmentBuffer, MaterialHandle markMaterial);
    void (*ModelBounds)(GfxBrushModel *bmodel, vec_t *mins, vec_t *maxs);
    void (*TrackStatistics)(void *stats);
    qboolean (*PickMaterial)(const vec_t *org, const vec_t *dir, char *name, char *surfaceFlags, char *contents, int charLimit);
    FontHandle (*RegisterFont)(const char *fontName, int imageTrack);
    void (*ResetImageAllocations)(void);
    void (*FreeImageAllocations)(void);
    void (*BeginCubemapShot)(int pixelWidthHeight, int pixelBorder);
    void (*EndCubemapShot)(int shotIndex);
    void (*SaveCubemapShot)(const char *filename, int shotIndex, float n0, float n1);
    void (*LightingFromCubemapShots)(const vec_t *baseColor);
    void (*LocateDebugStrings)(void *strings, int stringCount, int maxStringCount);
    void (*LocateDebugLines)(void *lines, int lineCount, int maxLineCount);
    void (*AddPlume)(const vec_t *origin, int score, const vec_t *color, int duration);
    void (*ShutdownDebug)(void);
    void (*UpdateColor)(const vec_t *color_allies, const vec_t *color_axis);
    float (*NormalizedTextScale)(FontHandle font, float scale);
    int (*TextWidth)(const char *text, int maxChars, FontHandle font);
    int (*TextHeight)(FontHandle font);
    void (*DrawText)(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style);
    void (*DrawTextInSpace)(const char *text, FontHandle font, const vec_t *org, const vec_t *xPixelStep, const vec_t *yPixelStep, const vec_t *color);
    int (*ConsoleTextWidth)(const short int *string, int maxChars, FontHandle font);
    void (*DrawConsoleText)(const short int *string, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style);
    void (*DrawTextWithCursor)(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style, int cursorPos, int cursor);
    void (*DObjGetSurfMaterials)(struct DObj_s *obj, int lod, MaterialHandle *matHandleArray);
    void (*DObjReplaceMaterial)(struct DObj_s *obj, int lod, int surfaceIndex, MaterialHandle material);
    const char *(*ParseSunLight)(void *params, const char *text);
    MaterialHandle (*Material_Duplicate)(MaterialHandle mtlCopy, const char *name);
    int (*DuplicateFont)(FontHandle fontCopy, const char *name);
    bool XModelAllowReadSurface;
    void (*SyncRenderThread)(void);
    void (*AbortRenderCommands)(void);
    qboolean (*IsGpuFenceFinished)(void);
    void (*SyncGpu)(void);
    void (*GpuWaited)(void);
    void (*SetLodOrigin)(const refdef_t *refdef);
};

struct refimport_t {
    void (*Printf)(int level, const char *format, ...);
    void (*Error)(int code, const char *format, ...);
    int (*Milliseconds)();
    void *(*Hunk_AllocInternal)();
    void *(*Hunk_AllocateTempMemoryInternal)();
    void *(*Z_MallocInternal)();
    void (*Z_FreeInternal)();
    void *(*Hunk_AllocAlignInternal)();
    void *(*Z_VirtualReserveInternal)();
    void (*Z_VirtualCommitInternal)();
    void (*Z_VirtualDecommitInternal)();
    void (*Z_VirtualFreeInternal)();
    void (*Hunk_FreeTempMemory)();
    void (*Hunk_ClearTempMemory)();
    int (*Hunk_HideTempMemory)();
    void (*Hunk_ShowTempMemory)();
    void *(*Hunk_AllocateTempMemoryHighInternal)();
    void (*Hunk_ClearTempMemoryHigh)();
    void (*Sys_DirectXFatalError)();
    void (*Sys_ShowSplashWindow)();
    void (*Sys_HideSplashWindow)();
    void (*Sys_LoadingKeepAlive)();
    const dvar_t *(*Dvar_RegisterBool)(const char *dvarName, int value, int flags);
    const dvar_t *(*Dvar_RegisterInt)(const char *dvarName, int value, int min, int max, int flags);
    const dvar_t *(*Dvar_RegisterFloat)(const char *dvarName, float value, float min, float max, int flags);
    const dvar_t *(*Dvar_RegisterString)(const char *dvarName, const char *value, int flags);
    const dvar_t *(*Dvar_RegisterEnum)(const char *dvarName, const char **valueList, int defaultIndex, int flags);
    const dvar_t *(*Dvar_RegisterColor)(const char *dvarName, float r, float g, float b, float a, int flags);
    const dvar_t *(*Dvar_RegisterVec2)(const char *dvarName, float x, float y, float min, float max, int flags);
    const dvar_t *(*Dvar_RegisterVec3)(const char *dvarName, float x, float y, float z, float min, float max, int flags);
    const dvar_t *(*Dvar_RegisterVec4)(const char *dvarName, float x, float y, float z, float w, float min, float max, int flags);
    void (*Dvar_UnregisterSystem)(int sysFlag);
    void (*Dvar_ChangeResetValue)();
    Bool (*Dvar_IsAtDefaultValue)(const dvar_t *dvar);
    void (*Dvar_ClearModified)(const dvar_t *dvar);
    void (*Dvar_SetModified)(const dvar_t *dvar);
    void (*Dvar_UpdateEnumDomain)(const dvar_t *dvar, const char **stringTable);
    void (*Dvar_SetBool)(const dvar_t *dvar, int value);
    void (*Dvar_SetInt)(const dvar_t *dvar, int value);
    void (*Dvar_SetFloat)(const dvar_t *dvar, float value);
    void (*Dvar_SetString)(const dvar_t *dvar, const char *value);
    void (*Dvar_SetColor)(const dvar_t *dvar, float r, float g, float b, float a);
    void (*Dvar_SetVec2)(const dvar_t *dvar, float x, float y);
    void (*Dvar_SetVec3)(const dvar_t *dvar, float x, float y, float z);
    void (*Dvar_SetVec4)(const dvar_t *dvar, float x, float y, float z, float w);
    void (*Dvar_SetFromString)();
    void (*Dvar_SetBoolByName)();
    void (*Dvar_SetIntByName)();
    void (*Dvar_SetFloatByName)();
    void (*Dvar_SetStringByName)();
    void (*Dvar_SetColorByName)();
    void (*Dvar_SetVec2ByName)();
    void (*Dvar_SetVec3ByName)();
    void (*Dvar_SetVec4ByName)();
    void (*Dvar_SetFromStringByName)();
    Bool (*Dvar_GetBool)();
    int (*Dvar_GetInt)();
    float (*Dvar_GetFloat)();
    const char *(*Dvar_GetString)();
    const char *(*Dvar_GetVariantString)();
    const char *(*Dvar_EnumToString)();
    void (*Dvar_Reset)();
    void (*Cmd_AddCommand)();
    void (*Cmd_RemoveCommand)();
    int (*Cmd_Argc)();
    char *(*Cmd_Argv)();
    void (*Cbuf_ExecuteText)();
    qboolean (*Com_SaveDvarsToBuffer)();
    qboolean (*Com_LoadDvarsFromBuffer)();
    const dheader_s *(*Com_GetBsp)();
    unsigned int (*SEH_ReadCharFromString)();
    void (*CL_UpdateDebugData)();
    void (*CL_FlushDebugData)();
    void (*StatMon_Warning)();
    int (*FS_ReadFile)();
    void (*FS_FreeFile)();
    int (*FS_FOpenFileRead)();
    const char **(*FS_ListFiles)();
    void (*FS_FreeFileList)();
    qboolean (*FS_FileExists)();
    qboolean (*FS_WriteFile)();
    int (*FS_FOpenFileByMode)();
    void (*FS_FCloseFile)();
    int (*FS_Read)();
    int (*FS_Write)();
    void (*CM_SaveLump)();
    void (*CM_BoxTrace)();
    int (*CM_BoxSightTrace)();
    Bool (*CM_RayTriangleIntersect)();
    struct XModel *(*XModelPrecache)();
    int (*XModelGetSurfaces)();
    int (*XModelBad)();
    void (*Hunk_OverrideDataForFile)();
    int (*XModelGetNumLods)();
    void (*XModelSetTestLods)();
    int (*XModelGetLodForDist)();
    float (*XModelGetLodOutDist)();
    const char *(*XModelGetSurfaceName)();
    const char *(*XModelGetName)();
    unsigned char (*XModelGetFlags)();
    int (*XModelNumBones)();
    const trXSkin_t *(*XModelGetSkins)();
    int (*XModelGetMemUsage)();
    const char *(*XModelGetLodName)();
    const DObjAnimMat *(*XModelGetBasePose)();
    const DObjAnimMat *(*XModelGetBasePoseBone)();
    int (*DObjBad)();
    void (*DB_EnumXAssets)();
    void (*DObjGetBounds)();
    XSurface *(*DObjGetSurface)();
    int (*DObjGetNumModels)();
    int (*DObjGetNumSurfaces)();
    int (*DObjGetSurfaces)();
    int *(*DObjGetPartBits)();
    DObjAnimMat *(*DObjGetRotTransArray)();
    int (*DObjSkelAreBonesUpToDate)();
    int (*DObjGetMatOffset)();
    struct XModel *(*DObjGetModel)();
    const char *(*DObjGetSurfaceName)();
    void (*DObjCreate)();
    int (*DObjGetAllocSkelSize)();
    void (*DObjCreateSkel)();
    void (*DObjCalcAnim)();
    void (*DObjCalcSkel)();
    int (*DObjNumBones)();
    void (*DObjGetBoneInfo)();
    int (*DObjGetLodForDist)();
    float (*DObjGetLodOutDist)();
    void (*DObjCompleteHierarchyBits)();
    void (*DObjSetModel)();
    void (*CIN_UploadCinematic)();
    int (*CIN_PlayCinematic)();
    e_status (*CIN_RunCinematic)();
    void (*CG_DObjCalcPose)();
    const char *(*CL_GetHudMsgIconMaterialName)();
    void *(*CM_GetPlaneNum)();
};

struct shellshock_parms_t {
    int view;
    int screenBlend;
    int sound;
    int mouse;
};

struct sightclip_t {
    vec3_t mins;
    vec3_t maxs;
    vec3_t outerSize;
    vec3_t start;
    vec3_t end;
    int passEntityNum[2];
    int contentmask;
};

struct sightpointtrace_t {
    vec3_t start;
    vec3_t end;
    int passEntityNum[2];
    int contentmask;
    qboolean locational;
};

struct snd_alias_list_t {
    const char *aliasName;
    snd_alias_t *head;
    int count;
#if defined(__x86_64__) || defined(_M_X64)

    intptr_t pHashNext;
#else
    int pHashNext;
#endif
};

struct snd_alias_t {
    const char *pszAliasName;
    const char *pszSubtitle;
    const char *pszSecondaryAliasName;
    SoundFile *soundFile;
    int iSequence;
    float fVolMin;
    float fVolMax;
    float fPitchMin;
    float fPitchMax;
    float fDistMin;
    float fDistMax;
    int flags;
    float fSlavePercentage;
    float fProbability;
    float fLfePercentage;
    int startDelay;
    SndCurve *volumeFalloffCurve;
};

struct snd_listener {
    struct orientation_t orient;
    int entnum;
    Bool active;
};

struct snd_overlay_info_t {
    const char *pszSampleName;
    float fBaseVolume;
    float fCurVolume;
    int dist;
    float fPitch;
};

struct snd_save_2D_sample_t {
    float fraction;
    float pitch;
    float volume;
    float pan;
};

struct snd_save_3D_sample_t {
    float fraction;
    float pitch;
    float volume;
    vec3_t org;
};

struct snd_save_stream_t {
    float fraction;
    int rate;
    float basevolume;
    float volume;
    float pan;
    vec3_t org;
};

struct traceWork_t {
    struct TraceExtents extents;
    vec3_t delta;
    vec_t deltaLen;
    vec_t deltaLenSq;
    vec3_t midpoint;
    vec3_t halfDelta;
    vec3_t halfDeltaAbs;
    vec3_t size;
    vec3_t bounds[2];
    int contents;
    qboolean isPoint;
    qboolean axialCullOnly;
    float radius;
    float offsetZ;
    vec3_t radiusOffset;
    struct TraceThreadInfo threadInfo;
};

struct vidConfig_t {
    int width;
    int height;
    int displayFrequency;
    qboolean isFullscreen;
    float aspectRatioWindow;
    float aspectRatioPixel;
    int maxTextureSize;
    int maxTextureMaps;
    int maxTextureCoords;
    int maxActiveLights;
    Bool deviceSupportsGamma;
};

struct viewState_t {
    playerState_t *ps;
    int damageTime;
    int time;
    float v_dmg_pitch;
    float v_dmg_roll;
    float xyspeed;
    float frametime;
    float fLastIdleFactor;
    int *weapIdleTime;
};

struct weaponInfo_s {
    struct DObj_s *viewModelDObj;
    float viewModelAnimRates[23];
    char handModel[64];
    int iPrevAnim;
    struct XAnimTree_s *tree;
    qboolean registered;
    const gitem_t *item;
    const char *pszTranslatedDisplayName;
    const char *pszTranslatedModename;
    const char *pszTranslatedAIOverlayDescription;
    struct XModel *worldSurfModel;
    struct EffectTemplate *viewFlashEffect;
    struct EffectTemplate *worldFlashEffect;
    vec3_t weaponMidpoint;
    snd_alias_list_t *missileSound;
    snd_alias_list_t *pullbackSound;
    snd_alias_list_t *flashSound;
    snd_alias_list_t *flashSoundPlayer;
    snd_alias_list_t *lastShotSound;
    snd_alias_list_t *lastShotSoundPlayer;
    snd_alias_list_t *meleeSwipeSound;
    snd_alias_list_t *rechamberSound;
    snd_alias_list_t *rechamberSoundPlayer;
    snd_alias_list_t *reloadSound;
    snd_alias_list_t *reloadSoundPlayer;
    snd_alias_list_t *reloadEmptySound;
    snd_alias_list_t *reloadEmptySoundPlayer;
    snd_alias_list_t *reloadStartSound;
    snd_alias_list_t *reloadStartSoundPlayer;
    snd_alias_list_t *reloadEndSound;
    snd_alias_list_t *reloadEndSoundPlayer;
    snd_alias_list_t *raiseSound;
    snd_alias_list_t *altSwitchSound;
    snd_alias_list_t *putawaySound;
    snd_alias_list_t *noteTrackSound[4];
    MaterialHandle ammoIcon;
    MaterialHandle hHudIcon;
    MaterialHandle hModeIcon;
    struct XModel *missileSurfModel;
    void (*missileTrailFunc)();
    float missileDlight;
    vec3_t missileDlightColor;
    int missileRenderfx;
    struct EffectTemplate *shellEjectEffect;
    struct EffectTemplate *lastShotEjectEffect;
    struct EffectTemplate *projExplosionEffect;
    snd_alias_list_t *projExplosionSound;
    struct EffectTemplate *projTrailEffect;
    float trailRadius;
    float wiTrailTime;
    MaterialHandle hReticleCenter;
    MaterialHandle hReticleSide;
    MaterialHandle hADSOverlay;
    vec3_t vPositionOrg[2];
    vec3_t vPositionAng[2];
};

#endif
