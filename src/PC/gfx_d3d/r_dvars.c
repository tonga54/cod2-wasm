#include "common_types.h"
#include "imports.h"

extern const dvar_t *r_ignore;
extern const dvar_t *r_overbrightBits;
extern const dvar_t *r_gamma;
extern const dvar_t *r_ignoreHwGamma;
extern const dvar_t *r_textureMode;
extern const dvar_t *r_anisotropy;
extern const dvar_t *r_fullbright;
extern const dvar_t *r_debugShader;
extern const dvar_t *r_debugEntCounts;
extern const dvar_t *r_optimize;
extern const dvar_t *r_optimizeLightmaps;
extern const dvar_t *r_optimizeXModels;
extern const dvar_t *r_gpuSync;
extern const dvar_t *r_multiGpu;
extern const dvar_t *r_skinCache;
extern const dvar_t *r_fullscreen;
extern const dvar_t *r_monitor;
extern const dvar_t *r_mode;
extern const dvar_t *r_displayRefresh;
extern const dvar_t *r_rendererPreference;
extern const dvar_t *r_rendererInUse;
extern const dvar_t *r_aspectRatio;
extern const dvar_t *r_smc_enable;
extern const dvar_t *r_lodScale;
extern const dvar_t *r_lodBias;
extern const dvar_t *r_znear;
extern const dvar_t *r_znear_depthhack;
extern const dvar_t *r_zfar;
extern const dvar_t *r_fog;
extern const dvar_t *r_polygonOffsetScale;
extern const dvar_t *r_polygonOffsetBias;
extern const dvar_t *r_picmip_manual;
extern const dvar_t *r_picmip;
extern const dvar_t *r_picmip_bump;
extern const dvar_t *r_picmip_spec;
extern const dvar_t *r_lightMap;
extern const dvar_t *r_colorMap;
extern const dvar_t *r_normalMap;
extern const dvar_t *r_specularMap;
extern const dvar_t *r_specularColorScale;
extern const dvar_t *r_cosinePowerMapShift;
extern const dvar_t *r_showTris;
extern const dvar_t *r_showTriCounts;
extern const dvar_t *r_showSurfCounts;
extern const dvar_t *r_showVertCounts;
extern const dvar_t *r_xdebug;
extern const dvar_t *r_railCoreWidth;
extern const dvar_t *r_vc_makelog;
extern const dvar_t *r_vc_showlog;
extern const dvar_t *r_showLightGrid;
extern const dvar_t *r_showMissingLightGrid;
extern const dvar_t *r_lightTweakAmbient;
extern const dvar_t *r_lightTweakDiffuseFraction;
extern const dvar_t *r_lightTweakSunLight;
extern const dvar_t *r_lightTweakAmbientColor;
extern const dvar_t *r_lightTweakSunColor;
extern const dvar_t *r_lightTweakSunDiffuseColor;
extern const dvar_t *r_lightTweakSunDirection;
extern const dvar_t *r_objectiveColorDx7Min;
extern const dvar_t *r_objectiveColorDx7Max;
extern const dvar_t *r_logFile;
extern const dvar_t *r_skipBackEnd;
extern const dvar_t *r_norefresh;
extern const dvar_t *r_swapInterval;
extern const dvar_t *r_aaAlpha;
extern const dvar_t *r_aaSamples;
extern const dvar_t *r_clearColor;
extern const dvar_t *r_clearColor2;
extern const dvar_t *r_drawSun;
extern const dvar_t *r_drawWorld;
extern const dvar_t *r_drawDecals;
extern const dvar_t *r_drawEntities;
extern const dvar_t *r_drawBModels;
extern const dvar_t *r_drawSModels;
extern const dvar_t *r_drawXModels;
extern const dvar_t *r_dlightLimit;
extern const dvar_t *r_drawPrimCap;
extern const dvar_t *r_drawPrimFloor;
extern const dvar_t *r_drawWater;
extern const dvar_t *r_depthPrepassModels;
extern const dvar_t *r_lockPvs;
extern const dvar_t *r_skipPvs;
extern const dvar_t *r_pvsStats;
extern const dvar_t *r_portalFineCull;
extern const dvar_t *r_portalBevels;
extern const dvar_t *r_portalBevelsOnly;
extern const dvar_t *r_singleCell;
extern const dvar_t *r_portalWalkLimit;
extern const dvar_t *r_portalMinClipArea;
extern const dvar_t *r_showPortals;
extern const dvar_t *r_showSModelNames;
extern const dvar_t *r_showFbColorDebug;
extern const dvar_t *r_showFloatZDebug;
extern const dvar_t *r_showGroundLit;
extern const dvar_t *r_highLodDist;
extern const dvar_t *r_mediumLodDist;
extern const dvar_t *r_lowLodDist;
extern const dvar_t *r_lowestLodDist;
extern const dvar_t *r_forceLod;
extern const dvar_t *sc_enable;
extern const dvar_t *sc_blur;
extern const dvar_t *sc_count;
extern const dvar_t *sc_debugCasterCount;
extern const dvar_t *sc_debugReceiverCount;
extern const dvar_t *sc_showOverlay;
extern const dvar_t *sc_showDebug;
extern const dvar_t *sc_wantCount;
extern const dvar_t *sc_wantCountMargin;
extern const dvar_t *sc_fadeRange;
extern const dvar_t *sc_shadowInRate;
extern const dvar_t *sc_shadowOutRate;
extern const dvar_t *sc_length;
extern const dvar_t *sc_offscreenCasterLodBias;
extern const dvar_t *sc_offscreenCasterLodScale;
extern const dvar_t *r_blur;
extern const dvar_t *r_distortion;
extern const dvar_t *r_glow;
extern const dvar_t *r_glowRadius[2];
extern const dvar_t *r_glowSkyBleedIntensity[2];
extern const dvar_t *r_glowBloomIntensity[2];
extern const dvar_t *r_glowBloomCutoff;
extern const dvar_t *r_glowBloomDesaturation;
extern const dvar_t *r_outdoorAwayBias;
extern const dvar_t *r_outdoorDownBias;
extern const dvar_t *r_outdoorFeather;
extern const dvar_t *r_sun_from_dvars;
extern const dvar_t *r_testTransform;
extern const dvar_t *r_testFill;
extern const dvar_t *r_testFillEnable;
extern const dvar_t *vid_xpos;
extern const dvar_t *vid_ypos;
extern const dvar_t *developer;
extern const dvar_t *sys_SSE;
extern const dvar_t *r_sse_skinning;

__attribute__((used)) const char *s_technologyNames[4] = {
    "none",
    "dx9",
    "dx7",
    0,
};
__attribute__((used)) const char *xdebugNames[5] = {
    "",
    "boxes",
    "axes",
    "both",
    0,
};
__attribute__((used, packed, aligned(4)))
const char *textureModeNames[] = {
    "nearest",
    "linear",
    "bilinear",
    "trilinear",
    "anisotropic",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static const char *s_aspectRatioNames[5] = {
    "auto",
    "standard",
    "wide 16:10",
    "wide 16:9",
    0,
};
static const char *s_displayModeNames[] = {
    "640x480",
    "800x600",
    "1024x768",
    0,
};
static const char *s_displayRefreshNames[] = {
    "60",
    0,
};
static const char *r_forceLodNames[6] = {
    "high",
    "medium",
    "low",
    "lowest",
    "none",
    0,
};
static const char *fbColorDebugNames[4] = {
    "None",
    "Screen",
    "Feedback",
    0,
};
static const char *s_aaAlphaNames[4] = {
    "off",
    "dither (fast)",
    "supersample (nice)",
    0,
};
static const char *normalMapNames[3] = {
    "Unchanged",
    "Flat",
    0,
};
static const char *colorMapNames[4] = {
    "Unchanged",
    "White",
    "Black",
    0,
};
static const char *gpuSyncNames[5] = {
    "off",
    "beginning",
    "beforePresent",
    "adaptive",
    0,
};
static const char *debugShaderNames[6] = {
    "none",
    "normal",
    "basisTangent",
    "basisBinormal",
    "basisNormal",
    0,
};

extern void R_RegisterSunDvars(void);
extern refimport_t ri;


void R_RegisterDvars(void);
void R_UnregisterDvars(void);

void R_RegisterDvars(void)
{

    R_RegisterSunDvars();

    r_ignore = ri.Dvar_RegisterInt("r_ignore", 0, (int)0x80000000, 0x7fffffff, 0x2000);

    r_overbrightBits = ri.Dvar_RegisterInt("r_overbrightBits", 1, 0, 1, 0x2021);

    r_gamma = ri.Dvar_RegisterFloat("r_gamma", 1.0f, 0.5f, 3.0f, 0x2001);

    r_ignoreHwGamma = ri.Dvar_RegisterBool("r_ignorehwgamma", 0, 0x2021);

    r_textureMode = ri.Dvar_RegisterEnum("r_textureMode", textureModeNames, 2, 0x2001);

    r_anisotropy = ri.Dvar_RegisterInt("r_anisotropy", 8, 2, 16, 0x2001);

    r_fullbright = ri.Dvar_RegisterBool("r_fullbright", 0, 0x2080);

    r_debugShader = ri.Dvar_RegisterEnum("r_debugShader", debugShaderNames, 0, 0x2080);

    r_debugEntCounts = ri.Dvar_RegisterInt("r_debugEntCounts", 0, 0, 0x7fe, 0x2000);

    r_optimize = ri.Dvar_RegisterBool("r_optimize", 1, 0x2020);

    r_optimizeLightmaps = ri.Dvar_RegisterBool("r_optimizeLightmaps", 1, 0x2020);

    r_optimizeXModels = ri.Dvar_RegisterInt("r_optimizeXModels", 100, 0, 1000, 0x2020);

    r_gpuSync = ri.Dvar_RegisterEnum("r_gpuSync", gpuSyncNames, 3, 0x2000);

    r_multiGpu = ri.Dvar_RegisterBool("r_multiGpu", 0, 0x2000);

    r_skinCache = ri.Dvar_RegisterBool("r_skinCache", 1, 0x2000);

    r_smc_enable = ri.Dvar_RegisterBool("r_smc_enable", 1, 0x2000);

    r_lodScale = ri.Dvar_RegisterFloat("r_lodScale", 1.0f, 1.0f, 4.0f, 0x2001);

    r_lodBias = ri.Dvar_RegisterFloat("r_lodBias", 0.0f, -1000.0f, 0.0f, 0x2001);

    r_znear = ri.Dvar_RegisterFloat("r_znear", 4.0f, 0.001f, 16.0f, 0x2080);

    r_znear_depthhack = ri.Dvar_RegisterFloat("r_znear_depthhack", 0.1f, 0.001f, 16.0f, 0x2080);

    r_zfar = ri.Dvar_RegisterFloat("r_zfar", 0.0f, 0.0f, 3.4028235e+38f, 0x2080);

    r_fog = ri.Dvar_RegisterBool("r_fog", 1, 0x2080);

    r_polygonOffsetScale = ri.Dvar_RegisterFloat("r_polygonOffsetScale", -1.0f, -4.0f, 4.0f, 0x2001);

    r_polygonOffsetBias = ri.Dvar_RegisterFloat("r_polygonOffsetBias", -1.0f, -16.0f, 16.0f, 0x2001);

    r_picmip_manual = ri.Dvar_RegisterBool("r_picmip_manual", 0, 0x2001);

    r_picmip = ri.Dvar_RegisterInt("r_picmip", 0, 0, 3, 0x2001);

    r_picmip_bump = ri.Dvar_RegisterInt("r_picmip_bump", 0, 0, 3, 0x2001);

    r_picmip_spec = ri.Dvar_RegisterInt("r_picmip_spec", 0, 0, 3, 0x2001);

    r_lightMap = ri.Dvar_RegisterEnum("r_lightMap", colorMapNames, 0, 0x2080);

    r_colorMap = ri.Dvar_RegisterEnum("r_colorMap", colorMapNames, 0, 0x2080);

    r_normalMap = ri.Dvar_RegisterEnum("r_normalMap", normalMapNames, 0, 0x2080);

    r_specularMap = ri.Dvar_RegisterEnum("r_specularMap", colorMapNames, 0, 0x2080);

    r_specularColorScale = ri.Dvar_RegisterFloat("r_specularColorScale", 1.0f, 0.01f, 100.0f, 0x2080);

    r_cosinePowerMapShift = ri.Dvar_RegisterFloat("r_cosinePowerMapShift", 0.0f, -1.0f, 1.0f, 0x2080);

    r_showTris = ri.Dvar_RegisterInt("r_showTris", 0, 0, 2, 0x2080);

    r_showTriCounts = ri.Dvar_RegisterBool("r_showTriCounts", 0, 0x2080);

    r_showSurfCounts = ri.Dvar_RegisterBool("r_showSurfCounts", 0, 0x2080);

    r_showVertCounts = ri.Dvar_RegisterBool("r_showVertCounts", 0, 0x2080);

    r_xdebug = ri.Dvar_RegisterEnum("r_xdebug", xdebugNames, 0, 0x2080);

    r_railCoreWidth = ri.Dvar_RegisterFloat("r_railCoreWidth", 1.0f, 0.0f, 16.0f, 0x2001);

    r_vc_makelog = ri.Dvar_RegisterInt("r_vc_makelog", 0, 0, 2, 0x2020);

    r_vc_showlog = ri.Dvar_RegisterInt("r_vc_showlog", 0, 0, 1024, 0x2000);

    r_showLightGrid = ri.Dvar_RegisterBool("r_showLightGrid", 0, 0x2080);

    r_showMissingLightGrid = ri.Dvar_RegisterBool("r_showMissingLightGrid", 0, 0x2080);

    r_lightTweakAmbient = ri.Dvar_RegisterFloat("r_lightTweakAmbient", 0.1f, 0.0f, 4.0f, 0x2200);

    r_lightTweakDiffuseFraction = ri.Dvar_RegisterFloat("r_lightTweakDiffuseFraction", 0.5f, 0.0f, 1.0f, 0x2200);

    r_lightTweakSunLight = ri.Dvar_RegisterFloat("r_lightTweakSunLight", 1.0f, 0.0f, 4.0f, 0x2200);

    r_lightTweakAmbientColor = ri.Dvar_RegisterColor("r_lightTweakAmbientColor", 1.0f, 0.0f, 0.0f, 1.0f, 0x2200);

    r_lightTweakSunColor = ri.Dvar_RegisterColor("r_lightTweakSunColor", 0.0f, 1.0f, 0.0f, 1.0f, 0x2200);

    r_lightTweakSunDiffuseColor = ri.Dvar_RegisterColor("r_lightTweakSunDiffuseColor", 0.0f, 0.0f, 1.0f, 1.0f, 0x2200);

    r_lightTweakSunDirection = ri.Dvar_RegisterVec3("r_lightTweakSunDirection", 0.0f, 0.0f, 0.0f, -360.0f, 360.0f, 0x2200);

    r_objectiveColorDx7Min = ri.Dvar_RegisterColor("r_objectiveColorDx7Min", 0.55f, 0.38f, 0.0f, 1.0f, 0x2000);

    r_objectiveColorDx7Max = ri.Dvar_RegisterColor("r_objectiveColorDx7Max", 1.0f, 0.7f, 0.2f, 1.0f, 0x2000);

    r_logFile = ri.Dvar_RegisterInt("r_logFile", 0, 0, 0x7fffffff, 0x2000);

    r_skipBackEnd = ri.Dvar_RegisterBool("r_skipBackEnd", 0, 0x2080);

    r_norefresh = ri.Dvar_RegisterBool("r_norefresh", 0, 0x2080);

    r_swapInterval = ri.Dvar_RegisterBool("r_swapInterval", 0, 0x2021);

    r_aaAlpha = ri.Dvar_RegisterEnum("r_aaAlpha", s_aaAlphaNames, 1, 0x2001);

    r_aaSamples = ri.Dvar_RegisterInt("r_aaSamples", 1, 1, 4, 0x2021);

    r_clearColor = ri.Dvar_RegisterColor("r_clearColor", 0.0f, 0.0f, 0.0f, 0.0f, 0x2000);

    r_clearColor2 = ri.Dvar_RegisterColor("r_clearColor2", 0.0f, 0.0f, 0.0f, 0.0f, 0x2000);

    r_drawSun = ri.Dvar_RegisterBool("r_drawSun", 1, 0x2001);

    r_drawWorld = ri.Dvar_RegisterBool("r_drawWorld", 1, 0x2080);

    r_drawDecals = ri.Dvar_RegisterBool("r_drawDecals", 1, 0x2080);

    r_drawEntities = ri.Dvar_RegisterBool("r_drawEntities", 1, 0x2080);

    r_drawBModels = ri.Dvar_RegisterBool("r_drawBModels", 1, 0x2080);

    r_drawSModels = ri.Dvar_RegisterBool("r_drawSModels", 1, 0x2080);

    r_drawXModels = ri.Dvar_RegisterBool("r_drawXModels", 1, 0x2080);

    r_dlightLimit = ri.Dvar_RegisterInt("r_dlightLimit", 4, 0, 32, 0x2000);

    r_drawPrimCap = ri.Dvar_RegisterInt("r_drawPrimCap", 0, -1, 10000, 0x2080);

    r_drawPrimFloor = ri.Dvar_RegisterInt("r_drawPrimFloor", 0, 0, 10000, 0x2080);

    r_drawWater = ri.Dvar_RegisterBool("r_drawWater", 1, 0x2080);

    r_depthPrepassModels = ri.Dvar_RegisterBool("r_depthPrepassModels", 0, 0x2000);

    r_lockPvs = ri.Dvar_RegisterBool("r_lockPvs", 0, 0x2080);

    r_skipPvs = ri.Dvar_RegisterBool("r_skipPvs", 0, 0x2080);

    r_pvsStats = ri.Dvar_RegisterBool("r_pvsStats", 0, 0x2000);

    r_portalFineCull = ri.Dvar_RegisterBool("r_portalFineCull", 1, 0x2080);

    r_portalBevels = ri.Dvar_RegisterFloat("r_portalBevels", 0.7f, 0.0f, 1.0f, 0x2001);

    r_portalBevelsOnly = ri.Dvar_RegisterBool("r_portalBevelsOnly", 0, 0x2000);

    r_singleCell = ri.Dvar_RegisterBool("r_singleCell", 0, 0x2080);

    r_portalWalkLimit = ri.Dvar_RegisterInt("r_portalWalkLimit", 0, 0, 100, 0x2080);

    r_portalMinClipArea = ri.Dvar_RegisterFloat("r_portalMinClipArea", 0.02f, 0.0f, 1.0f, 0x2000);

    r_showPortals = ri.Dvar_RegisterInt("r_showPortals", 0, 0, 3, 0x2080);

    r_showSModelNames = ri.Dvar_RegisterBool("r_showSModelNames", 0, 0x2080);

    r_showFbColorDebug = ri.Dvar_RegisterEnum("r_showFbColorDebug", fbColorDebugNames, 0, 0x2080);

    r_showFloatZDebug = ri.Dvar_RegisterBool("r_showFloatZDebug", 0, 0x2080);

    r_showGroundLit = ri.Dvar_RegisterBool("r_showGroundLit", 0, 0x20a0);

    r_highLodDist = ri.Dvar_RegisterFloat("r_highLodDist", -1.0f, -1.0f, 3.4028235e+38f, 0x2080);

    r_mediumLodDist = ri.Dvar_RegisterFloat("r_mediumLodDist", -1.0f, -1.0f, 3.4028235e+38f, 0x2080);

    r_lowLodDist = ri.Dvar_RegisterFloat("r_lowLodDist", -1.0f, -1.0f, 3.4028235e+38f, 0x2080);

    r_lowestLodDist = ri.Dvar_RegisterFloat("r_lowestLodDist", -1.0f, -1.0f, 3.4028235e+38f, 0x2080);

    r_forceLod = ri.Dvar_RegisterEnum("r_forceLod", r_forceLodNames, 4, 0x2080);

    sc_enable = ri.Dvar_RegisterBool("sc_enable", 1, 0x2000);

    sc_blur = ri.Dvar_RegisterInt("sc_blur", 2, 0, 4, 0x2080);

    sc_count = ri.Dvar_RegisterInt("sc_count", 24, 0, 24, 0x2080);

    sc_debugCasterCount = ri.Dvar_RegisterInt("sc_debugCasterCount", 24, 0, 24, 0x2080);

    sc_debugReceiverCount = ri.Dvar_RegisterInt("sc_debugReceiverCount", 24, 0, 24, 0x2080);

    sc_showOverlay = ri.Dvar_RegisterBool("sc_showOverlay", 0, 0x2080);

    sc_showDebug = ri.Dvar_RegisterBool("sc_showDebug", 0, 0x2080);

    sc_wantCount = ri.Dvar_RegisterInt("sc_wantCount", 12, 0, 24, 0x2080);

    sc_wantCountMargin = ri.Dvar_RegisterInt("sc_wantCountMargin", 1, 0, 24, 0x2080);

    sc_fadeRange = ri.Dvar_RegisterFloat("sc_fadeRange", 0.25f, 0.0f, 1.0f, 0x2080);

    sc_shadowInRate = ri.Dvar_RegisterFloat("sc_shadowInRate", 2.0f, 0.0f, 20.0f, 0x2080);

    sc_shadowOutRate = ri.Dvar_RegisterFloat("sc_shadowOutRate", 5.0f, 0.0f, 20.0f, 0x2080);

    sc_length = ri.Dvar_RegisterFloat("sc_length", 400.0f, 1.0f, 2000.0f, 0x2080);

    sc_offscreenCasterLodBias = ri.Dvar_RegisterFloat("sc_offscreenCasterLodBias", 0.0f, -3.4028235e+38f, 3.4028235e+38f, 0x2080);

    sc_offscreenCasterLodScale = ri.Dvar_RegisterFloat("sc_offscreenCasterLodScale", 20.0f, 0.0f, 3.4028235e+38f, 0x2080);

    r_blur = ri.Dvar_RegisterFloat("r_blur", 0.0f, 0.0f, 32.0f, 0x2080);

    r_distortion = ri.Dvar_RegisterBool("r_distortion", 1, 0x2001);

    r_glow = ri.Dvar_RegisterBool("r_glow", 1, 0x2001);

    r_glowRadius[0] = ri.Dvar_RegisterFloat("r_glowRadius0", 5.0f, 0.0f, 32.0f, 0x2000);

    r_glowRadius[1] = ri.Dvar_RegisterFloat("r_glowRadius1", 12.0f, 0.0f, 32.0f, 0x2000);

    r_glowSkyBleedIntensity[0] = ri.Dvar_RegisterFloat("r_glowSkyBleedIntensity0", 0.5f, 0.0f, 1.0f, 0x2000);

    r_glowSkyBleedIntensity[1] = ri.Dvar_RegisterFloat("r_glowSkyBleedIntensity1", 0.0f, 0.0f, 1.0f, 0x2000);

    r_glowBloomIntensity[0] = ri.Dvar_RegisterFloat("r_glowBloomIntensity0", 1.0f, 0.0f, 8.0f, 0x2000);

    r_glowBloomIntensity[1] = ri.Dvar_RegisterFloat("r_glowBloomIntensity1", 1.0f, 0.0f, 8.0f, 0x2000);

    r_glowBloomCutoff = ri.Dvar_RegisterFloat("r_glowBloomCutoff", 0.9f, 0.0f, 1.0f, 0x2000);

    r_glowBloomDesaturation = ri.Dvar_RegisterFloat("r_glowBloomDesaturation", 0.75f, 0.0f, 1.0f, 0x2000);

    r_outdoorAwayBias = ri.Dvar_RegisterFloat("r_outdoorAwayBias", 32.0f, -3.4028235e+38f, 3.4028235e+38f, 0x2000);

    r_outdoorDownBias = ri.Dvar_RegisterFloat("r_outdoorDownBias", 0.0f, -3.4028235e+38f, 3.4028235e+38f, 0x2000);

    r_outdoorFeather = ri.Dvar_RegisterFloat("r_outdoorFeather", 8.0f, -3.4028235e+38f, 3.4028235e+38f, 0x2000);

    ri.Dvar_SetModified(r_outdoorFeather);

    r_sun_from_dvars = ri.Dvar_RegisterBool("r_sun_from_dvars", 0, 0x2080);

    r_testTransform = ri.Dvar_RegisterInt("r_testTransform", 0, 0, 1, 0x2000);

    r_testFill = ri.Dvar_RegisterInt("r_testFill", 0, 0, 500, 0x2000);

    r_testFillEnable = ri.Dvar_RegisterBool("r_testFillEnable", 0, 0x2020);

    *(const dvar_t **)imp_fx_sort = ri.Dvar_RegisterBool("fx_sort", 1, 0x2000);

    vid_xpos = ri.Dvar_RegisterInt("vid_xpos", 3, (int)0xfffff000, 4096, 0x2001);

    vid_ypos = ri.Dvar_RegisterInt("vid_ypos", 22, (int)0xfffff000, 4096, 0x2001);

    developer = ri.Dvar_RegisterInt("developer", 0, 0, 2, 0x2000);

    *(const dvar_t **)imp_sv_cheats = ri.Dvar_RegisterBool("sv_cheats", 0, 0x2048);

    *(const dvar_t **)imp_com_statmon = ri.Dvar_RegisterBool("com_statmon", 0, 0x2000);

    sys_SSE = ri.Dvar_RegisterBool("sys_SSE", 0, 0x2000);

    r_sse_skinning = ri.Dvar_RegisterBool("r_sse_skinning", 1, 0x2000);

    r_fullscreen = ri.Dvar_RegisterBool("r_fullscreen", 1, 0x2040);

    r_monitor = ri.Dvar_RegisterInt("r_monitor", 0, 0, 8, 0x2021);

    r_mode = ri.Dvar_RegisterEnum("r_mode", s_displayModeNames, 0, 0x2021);

    r_displayRefresh = ri.Dvar_RegisterEnum("r_displayRefresh", s_displayRefreshNames, 0, 0x2021);

    r_rendererPreference = ri.Dvar_RegisterEnum("r_rendererPreference", s_technologyNames, 1, 0x2021);

    r_rendererInUse = ri.Dvar_RegisterEnum("r_rendererInUse", s_technologyNames, 0, 0x2040);

    r_aspectRatio = ri.Dvar_RegisterEnum("r_aspectRatio", s_aspectRatioNames, 0, 0x2021);
}

void R_UnregisterDvars(void)
{
    ri.Dvar_UnregisterSystem(0x2000);
}
