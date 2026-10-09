#include <stdio.h>
#include <string.h>
#include "common_types.h"

extern const dvar_t *Dvar_RegisterString(const char *dvarName, const char *value, int flags);
extern const dvar_t *Dvar_RegisterBool(const char *dvarName, int value, int flags);

const dvar_t *Dvar_RegisterString_mac(const char *name, const char *value, int flags)
{
    return Dvar_RegisterString(name, value ? value : "", flags);
}

const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags)
{
    return Dvar_RegisterBool(name, value, flags);
}

int AddMovieResource(void *movie, short refNum, short *resId, const unsigned char *resName)
{
    (void)movie;
    (void)refNum;
    (void)resId;
    (void)resName;
    return 0;
}
int AdvanceKeyboardFocus()
{
    return 0;
}
int AECoerceDesc()
{
    return 0;
}
int AECreateAppleEvent()
{
    return 0;
}
int AECreateDesc()
{
    return 0;
}
int AEDisposeDesc()
{
    return 0;
}
int AEGetDescData()
{
    return 0;
}
int AEGetParamDesc()
{
    return 0;
}
int AEGetParamPtr()
{
    return 0;
}
int AEPutParamDesc()
{
    return 0;
}
int AEPutParamPtr()
{
    return 0;
}
int AESend()
{
    return 0;
}
int aglDescribePixelFormat()
{
    return 0;
}
int aglGetDrawable()
{
    return 0;
}
int AlertSoundPlay()
{
    return 0;
}
int AudioConverterDispose()
{
    return 0;
}
int AudioConverterFillBuffer()
{
    return 0;
}
int AudioConverterFillComplexBuffer()
{
    return 0;
}
int AudioConverterNew()
{
    return 0;
}
int AudioConverterReset()
{
    return 0;
}
int AudioDeviceGetPropertyInfo()
{
    return 0;
}
int AudioUnitAddRenderNotify()
{
    return 0;
}
int AudioUnitInitialize()
{
    return 0;
}
int AudioUnitRemoveRenderNotify()
{
    return 0;
}
int AudioUnitUninitialize()
{
    return 0;
}
int CFBooleanGetValue()
{
    return 0;
}
int CFBundleCopyExecutableURL()
{
    return 0;
}
int CFBundleCopyLocalizedString(void *bundle, void *key, void *value, void *table)
{
    (void)bundle;
    (void)key;
    (void)value;
    (void)table;
    return 0;
}
int CFDataGetBytePtr()
{
    return 0;
}
int CFStringCompare()
{
    return 0;
}
int CFStringCreateWithCharacters()
{
    return 0;
}
int CFStringCreateWithFormat()
{
    return 0;
}
int CFStringGetPascalString()
{
    return 0;
}
int CFURLGetFSRef()
{
    return 0;
}
int CGAcquireDisplayFadeReservation()
{
    return 0;
}
int CGAssociateMouseAndMouseCursorPosition()
{
    return 0;
}
int CGDisplayAvailableModes()
{
    return 0;
}
int CGDisplayBitsPerPixel(void *display)
{
    (void)display;
    return 0;
}
int CGDisplayBounds()
{
    return 0;
}
int CGDisplayCurrentMode()
{
    return 0;
}
int CGDisplayFade()
{
    return 0;
}
int CGDisplayIDToOpenGLDisplayMask()
{
    return 0;
}
int CGDisplayIOServicePort()
{
    return 0;
}
int CGDisplayPixelsHigh()
{
    return 0;
}
int CGDisplayPixelsWide()
{
    return 0;
}
int CGGetActiveDisplayList()
{
    return 0;
}
int CGGetDisplayTransferByTable()
{
    return 0;
}
int CGLChoosePixelFormat()
{
    return 0;
}
int CGLCreateContext()
{
    return 0;
}
int CGLDescribeRenderer()
{
    return 0;
}
int CGLDestroyContext()
{
    return 0;
}
int CGLDestroyPixelFormat()
{
    return 0;
}
int CGLDestroyRendererInfo()
{
    return 0;
}
int CGLGetCurrentContext()
{
    return 0;
}
int CGLQueryRendererInfo()
{
    return 0;
}
int CGLSetCurrentContext()
{
    return 0;
}
int CGRectEqualToRect()
{
    return 0;
}
int CGRectGetHeight()
{
    return 0;
}
int CGRectGetWidth()
{
    return 0;
}
int CGRectIsEmpty()
{
    return 0;
}
int CGRectUnion()
{
    return 0;
}
int CGRectZero()
{
    return 0;
}
int CGReleaseDisplayFadeReservation()
{
    return 0;
}
int CGSetDisplayTransferByTable(void *display, unsigned int sampleCount, void *redTable, void *greenTable, void *blueTable)
{
    (void)display;
    (void)sampleCount;
    (void)redTable;
    (void)greenTable;
    (void)blueTable;
    return 0;
}
int CGWarpMouseCursorPosition()
{
    return 0;
}
int DisposeGWorld()
{
    return 0;
}
int DisposeHandle()
{
    return 0;
}
int DisposeMovie()
{
    return 0;
}
int DisposeNibReference()
{
    return 0;
}
int FSCloseIterator()
{
    return 0;
}
int FSDeleteObject()
{
    return 0;
}
int FSGetCatalogInfo()
{
    return 0;
}
int FSGetCatalogInfoBulk()
{
    return 0;
}
int FSGetVolumeInfo()
{
    return 0;
}
int FSMakeFSSpec()
{
    return 0;
}
int FSOpenIterator()
{
    return 0;
}
int FSPathMakeRef(const unsigned char *path, FSRef *ref, unsigned char *isDirectory)
{
    (void)path;
    if (ref)
        memset(ref, 0, sizeof(*ref));
    if (isDirectory)
        *isDirectory = 0;
    return -4; /* unimpErr: browser files use the engine's virtual filesystem. */
}
int FSpMakeFSRef()
{
    return 0;
}
int FSRefMakePath(void *ref, unsigned char *path, unsigned int maxPathSize)
{
    (void)ref;
    if (path && maxPathSize)
        path[0] = '\0';
    return 0;
}
int FSSetCatalogInfo()
{
    return 0;
}
int GetControlBounds()
{
    return 0;
}
int GetControlByID()
{
    return 0;
}
int GetControlData()
{
    return 0;
}
int GetControlDataSize()
{
    return 0;
}
int GetControlKind()
{
    return 0;
}
int GetControlMaximum()
{
    return 0;
}
int GetControlReference()
{
    return 0;
}
int SND_SetEnvironmentEffects_f()
{
    return 0;
}
int SV_GetBrushModelCount()
{
    return 0;
}

int AUGraphGetCPULoad(void *graph, float *outLoad)
{
    (void)graph;
    if (outLoad)
        *outLoad = 0.0f;
    return 0;
}
char AUGraphGetNodeInfo[64] __attribute__((aligned(4))) = { 0 };
char AUGraphNewNode[64] __attribute__((aligned(4))) = { 0 };
char AUGraphUpdate[64] __attribute__((aligned(4))) = { 0 };
extern unsigned char bg_itemlist[];
extern void G_RegisterWeapon(int weapIndex);
void *bg_itemlist_ptr __attribute__((aligned(4))) = (void *)bg_itemlist;
void *bg_weaponInfoMem __attribute__((aligned(4))) = (void *)G_RegisterWeapon;
char buf[64] __attribute__((aligned(4))) = { 0 };
extern unsigned char cgArray[];
void *cg __attribute__((aligned(4))) = (void *)cgArray;
char cg_debug_ptr[64] __attribute__((aligned(4))) = { 0 };

extern unsigned char cg_entitiesArray[];
void *cg_entities __attribute__((aligned(4))) = (void *)cg_entitiesArray;

char cg_hud_ptr[64] __attribute__((aligned(4))) = { 0 };
extern unsigned char cg_itemsArray[];
void *cg_items __attribute__((aligned(4))) = (void *)cg_itemsArray;

extern const unsigned char vec3_origin[];
void *cg_pmove_mins __attribute__((aligned(4))) = (void *)vec3_origin;

extern unsigned char cgsArray[];
void *cgs __attribute__((aligned(4))) = (void *)cgsArray;

extern unsigned char legacyHacksArray[];
extern void *legacyHacks[];
void *cg_uiglob __attribute__((aligned(4))) = (void *)legacyHacks;
char cg_weapinfo_ptr[64] __attribute__((aligned(4))) = { 0 };
extern unsigned char cg_weaponsArray[];
void *cg_weapons __attribute__((aligned(4))) = (void *)cg_weaponsArray;
extern unsigned char clients[];
void *cl __attribute__((aligned(4))) = (void *)clients;
extern unsigned char clientConnections[];
void *clc __attribute__((aligned(4))) = (void *)clientConnections;

char CloseComponent[64] __attribute__((aligned(4))) = { 0 };
char cl_packetdelay[64] __attribute__((aligned(4))) = { 0 };

extern unsigned char cls[];
void *cls_ptr_195ecac = (void *)cls;
char cm_phys_ptr[64] __attribute__((aligned(4))) = { 0 };
char com_checksumFeed_dvar[64] __attribute__((aligned(4))) = { 0 };

char com_errorEntered_ptr[64] __attribute__((aligned(4))) = { 0 };
char commandsList[64] __attribute__((aligned(4))) = { 0 };

char com_statmon_ptr[64] __attribute__((aligned(4))) = { 0 };
char CreateEvent[64] __attribute__((aligned(4))) = { 0 };
char CreateNibReferenceWithCFBundle[64] __attribute__((aligned(4))) = { 0 };
char CreateObjSpecifier[64] __attribute__((aligned(4))) = { 0 };
char CreateStandardAlert[64] __attribute__((aligned(4))) = { 0 };
char CreateWindowFromNib[64] __attribute__((aligned(4))) = { 0 };
char d3d_context[64] __attribute__((aligned(4))) = { 0 };

char DisableControl[64] __attribute__((aligned(4))) = { 0 };
void *download_ui_ptr __attribute__((aligned(4))) = (void *)legacyHacks;

char dvar_ptr_195ee78[64] __attribute__((aligned(4))) = { 0 };

char dxIter[64] __attribute__((aligned(4))) = { 0 };
char __dyld_func_lookup[64] __attribute__((aligned(4))) = { 0 };
int EnableControl(void *control)
{
    (void)control;
    return 0;
}
char encode_vol_ptr[64] __attribute__((aligned(4))) = { 0 };
char entityHandlers_ptr[64] __attribute__((aligned(4))) = { 0 };
int FindNextComponent()
{
    return 0;
}

struct {
    const char *flag;
    unsigned int masks[2];
} fxAttributeFlags[27] __attribute__((aligned(4))) = {
    { "org2fromTrace", { 0, 0x008 } },
    { "traceImpactFx", { 0, 0x010 } },
    { "org2isOffset", { 0, 0x020 } },
    { "cheapOrgCalc", { 0, 0x040 } },
    { "cheapOrg2Calc", { 0, 0x080 } },
    { "absoluteVel", { 0x200000, 0 } },
    { "useModel", { 0x000010, 0 } },
    { "useBBox", { 0x000060, 0 } },
    { "usePhysics", { 0x000020, 0 } },
    { "impactKills", { 0x000400, 0 } },
    { "useAlpha", { 0x000080, 0 } },
    { "useRandomColors", { 0x002000, 0 } },
    { "useRandomAlpha", { 0x004000, 0 } },
    { "useRandomSize", { 0x008000, 0 } },
    { "useRandomSize2", { 0x010000, 0 } },
    { "useRandomLength", { 0x020000, 0 } },
    { "useRandomRotationDelta", { 0x040000, 0 } },
    { "useRandomVelocity", { 0x080000, 0 } },
    { "useRandomVelocity2", { 0x100000, 0 } },
    { "absoluteVel2", { 0x400000, 0 } },
    { "affectedByWind", { 0x800000, 0 } },
    { "emitFx", { 0x000100, 0 } },
    { "relative", { 0x000002, 0 } },
    { "blocksSight", { 0x001000, 0 } },
    { "disableFarPlaneCulling", { 0x2000000, 0 } },
    { "impactFx", { 0x000820, 0 } },
    { "linear", { 0x4000000, 0 } },
};

char fx_sort_ptr[64] __attribute__((aligned(4))) = { 0 };

struct {
    const char *flag;
    unsigned int masks[2];
} fxSpawnFlags[13] __attribute__((aligned(4))) = {
    { "evenDistribution", { 0, 0x200 } },
    { "frustumCull", { 0, 0x400 } },
    { "orgOnSphere", { 0, 0x001 } },
    { "absoluteAccel", { 0x400000, 0 } },
    { "depthHack", { 0x001, 0 } },
    { "setShaderTime", { 0x004, 0 } },
    { "useModel", { 0x010, 0 } },
    { "orgOnCylinder", { 0, 0x004 } },
    { "axisFromSphere", { 0, 0x002 } },
    { "randrotaroundfwd", { 0, 0x100 } },
    { "impactFx", { 0x820, 0 } },
    { "deathFx", { 0x200, 0 } },
    { "emitFx", { 0x100, 0 } },
};

char fx_time_dst1[64] __attribute__((aligned(4))) = { 0 };
char fx_time_dst2[64] __attribute__((aligned(4))) = { 0 };
char fx_time_src1[64] __attribute__((aligned(4))) = { 0 };
char fx_time_src2[64] __attribute__((aligned(4))) = { 0 };

char g_cheats_dvar[64] __attribute__((aligned(4))) = { 0 };
char g_clients_ptr[64] __attribute__((aligned(4))) = { 0 };
char g_creatingTexture[64] __attribute__((aligned(4))) = { 0 };
char g_deadChat_ptr[64] __attribute__((aligned(4))) = { 0 };

char g_enemylookDist[64] __attribute__((aligned(4))) = { 0 };

char GetComponentVersion[64] __attribute__((aligned(4))) = { 0 };
#if !defined(_M_X64)  /* x64: real kernel32 GetCurrentProcess (Mac PSN stub collides by name) */
int GetCurrentProcess(void *psn)
{
    (void)psn;
    return 0;
}
#endif
char GetCursor[64] __attribute__((aligned(4))) = { 0 };
char GetGlobalMouse[64] __attribute__((aligned(4))) = { 0 };
char GetGWorldPixMap[64] __attribute__((aligned(4))) = { 0 };
char GetHandleSize[64] __attribute__((aligned(4))) = { 0 };
char GetMainEventLoop[64] __attribute__((aligned(4))) = { 0 };
char GetMainEventQueue[64] __attribute__((aligned(4))) = { 0 };
char GetMediaHandler[64] __attribute__((aligned(4))) = { 0 };
char GetMediaSampleDescription[64] __attribute__((aligned(4))) = { 0 };
char GetMovieDuration[64] __attribute__((aligned(4))) = { 0 };
char GetMovieIndTrackType[64] __attribute__((aligned(4))) = { 0 };
char GetMoviePreferredRate[64] __attribute__((aligned(4))) = { 0 };
char GetMovieTime[64] __attribute__((aligned(4))) = { 0 };
char GetMovieTimeScale[64] __attribute__((aligned(4))) = { 0 };
char GetNextProcess[64] __attribute__((aligned(4))) = { 0 };
char GetPixRowBytes[64] __attribute__((aligned(4))) = { 0 };
char GetPort[64] __attribute__((aligned(4))) = { 0 };
int GetProcessBundleLocation(void *psn, FSRef *location)
{
    (void)psn;
    if (location) {
        memset(location, 0, sizeof(*location));
    }
    return 0;
}
char GetProcessInformation[64] __attribute__((aligned(4))) = { 0 };
char GetQDGlobalsArrow[64] __attribute__((aligned(4))) = { 0 };
char GetStandardAlertDefaultParams[64] __attribute__((aligned(4))) = { 0 };
char GetTrackMedia[64] __attribute__((aligned(4))) = { 0 };
char GetWindowResizeLimits[64] __attribute__((aligned(4))) = { 0 };
char GetWRefCon[64] __attribute__((aligned(4))) = { 0 };
char g_friendlylookDist[64] __attribute__((aligned(4))) = { 0 };

char gfxBuf[64] __attribute__((aligned(4))) = { 0 };

char GoToBeginningOfMovie[64] __attribute__((aligned(4))) = { 0 };

char g_phys_world[64] __attribute__((aligned(4))) = { 0 };

char g_ri[64] __attribute__((aligned(4))) = { 0 };

void GScr_LoadAnimScripts(void) {}
void GScr_PostResetTimeout(void) {}
char g_sNextDmgTableId[64] __attribute__((aligned(4))) = { 0 };
void G_SpawnTriggerHurt(int numBrushModels)
{
    (void)numBrushModels;
}
char g_sv_running_ptr[64] __attribute__((aligned(4))) = { 0 };

void *g_trace_zero_ptr __attribute__((aligned(4))) = (void *)vec3_origin;
char g_unknown_195f22c[64] __attribute__((aligned(4))) = { 0 };
char g_unknown_195f230[64] __attribute__((aligned(4))) = { 0 };
char g_useActivateHoldTime[64] __attribute__((aligned(4))) = { 0 };
char g_useActivateReuseTime[64] __attribute__((aligned(4))) = { 0 };
char g_vidConfig[64] __attribute__((aligned(4))) = { 0 };

char g_voiceChatsAllowed_ptr[64] __attribute__((aligned(4))) = { 0 };
char g_voiceChatTalkingDuration_ptr[64] __attribute__((aligned(4))) = { 0 };
char HandleControlKey[64] __attribute__((aligned(4))) = { 0 };
char HideControl[64] __attribute__((aligned(4))) = { 0 };
char HITextViewGetTXNObject[64] __attribute__((aligned(4))) = { 0 };
char HIViewGetRoot[64] __attribute__((aligned(4))) = { 0 };
char HIViewGetViewForMouseEvent[64] __attribute__((aligned(4))) = { 0 };
char InitCursor[64] __attribute__((aligned(4))) = { 0 };
char InstallEventLoopTimer[64] __attribute__((aligned(4))) = { 0 };
char IOBSDNameMatching[64] __attribute__((aligned(4))) = { 0 };
char IOObjectGetClass[64] __attribute__((aligned(4))) = { 0 };
char IOObjectRetain[64] __attribute__((aligned(4))) = { 0 };
char IORegistryEntryCreateCFProperty[64] __attribute__((aligned(4))) = { 0 };
char IORegistryEntryCreateIterator[64] __attribute__((aligned(4))) = { 0 };
char IsMovieDone[64] __attribute__((aligned(4))) = { 0 };
#if !defined(COD2_DOWNSTREAM_WASM)
char jpeg_memory_src[64] __attribute__((aligned(4))) = { 0 };
#endif
char kCFAllocatorDefault[64] __attribute__((aligned(4))) = { 0 };

char loadingMessage[64] __attribute__((aligned(4))) = { 0 };
char LockPixels[64] __attribute__((aligned(4))) = { 0 };
char LSCopyItemInfoForRef[64] __attribute__((aligned(4))) = { 0 };
char MediaSetSoundBalance[64] __attribute__((aligned(4))) = { 0 };

char MoviesTask[64] __attribute__((aligned(4))) = { 0 };
char name[64] __attribute__((aligned(4))) = { 0 };

char NewControlEditTextValidationUPP[64] __attribute__((aligned(4))) = { 0 };
char NewControlKeyFilterUPP[64] __attribute__((aligned(4))) = { 0 };
char NewControlUserPaneDrawUPP[64] __attribute__((aligned(4))) = { 0 };
char NewControlUserPaneHitTestUPP[64] __attribute__((aligned(4))) = { 0 };
char NewControlUserPaneTrackingUPP[64] __attribute__((aligned(4))) = { 0 };
char NewGWorld[64] __attribute__((aligned(4))) = { 0 };
char NewHandle[64] __attribute__((aligned(4))) = { 0 };
char NewMovieFromFile[64] __attribute__((aligned(4))) = { 0 };

char OpenAComponent[64] __attribute__((aligned(4))) = { 0 };
char OpenComponent[64] __attribute__((aligned(4))) = { 0 };
char OpenMovieFile[64] __attribute__((aligned(4))) = { 0 };
char PBGetCatInfoSync[64] __attribute__((aligned(4))) = { 0 };
char PBHGetVolParmsSync[64] __attribute__((aligned(4))) = { 0 };

char PostEventToQueue[64] __attribute__((aligned(4))) = { 0 };
extern unsigned char bulletPriorityMap[];
extern unsigned char riflePriorityMap[];
void *pPriorityMap __attribute__((aligned(4))) = (void *)bulletPriorityMap;
void *pPriorityMapAlt __attribute__((aligned(4))) = (void *)riflePriorityMap;
char PrerollMovie[64] __attribute__((aligned(4))) = { 0 };

char ptr_195ecb4[64] __attribute__((aligned(4))) = { 0 };
char ptr_195ecbc[64] __attribute__((aligned(4))) = { 0 };
char ptr_195eea4[64] __attribute__((aligned(4))) = { 0 };
char ptr_195f58c[64] __attribute__((aligned(4))) = { 0 };
char ptr_195f5e0[64] __attribute__((aligned(4))) = { 0 };
char QDRegisterNamedPixMapCursor[64] __attribute__((aligned(4))) = { 0 };
char QDSetNamedPixMapCursor[64] __attribute__((aligned(4))) = { 0 };
char QuitAppModalLoopForWindow[64] __attribute__((aligned(4))) = { 0 };
char rcon_password_dvar[64] __attribute__((aligned(4))) = { 0 };

char record_callback_ptr[64] __attribute__((aligned(4))) = { 0 };
char ReleaseEvent[64] __attribute__((aligned(4))) = { 0 };
char re_ptr_195eca8[64] __attribute__((aligned(4))) = { 0 };
void RestoreBody(void) {}

char r_gammaSetting[64] __attribute__((aligned(4))) = { 0 };

extern unsigned char rgp[];

char R_LoadSun_f[64] __attribute__((aligned(4))) = { 0 };
char r_occlusionQuery[64] __attribute__((aligned(4))) = { 0 };
char R_ReloadMaterialTextures_f[64] __attribute__((aligned(4))) = { 0 };

char R_SaveSun_f[64] __attribute__((aligned(4))) = { 0 };
char R_SmcFlush_f[64] __attribute__((aligned(4))) = { 0 };
char R_SmcStats_f[64] __attribute__((aligned(4))) = { 0 };

char RunAppModalLoopForWindow[64] __attribute__((aligned(4))) = { 0 };
char RunStandardAlert[64] __attribute__((aligned(4))) = { 0 };

void Scr_BeginLoadAnimScripts(void) {}

void Scr_EndLoadAnimScripts(void) {}

char scrPlace[64] __attribute__((aligned(4))) = { 0 };
char SetControlData[64] __attribute__((aligned(4))) = { 0 };
char SetControlFontStyle[64] __attribute__((aligned(4))) = { 0 };
char SetControlMaximum[64] __attribute__((aligned(4))) = { 0 };
char SetControlReference[64] __attribute__((aligned(4))) = { 0 };
char SetCursor[64] __attribute__((aligned(4))) = { 0 };
char SetEventParameter[64] __attribute__((aligned(4))) = { 0 };
int SetKeyboardFocus(void *window, void *control, int part)
{
    (void)window;
    (void)control;
    (void)part;
    return 0;
}
char SetMovieRate[64] __attribute__((aligned(4))) = { 0 };
char SetMovieTimeValue[64] __attribute__((aligned(4))) = { 0 };
char SetPort[64] __attribute__((aligned(4))) = { 0 };
char SetThemeCursor[64] __attribute__((aligned(4))) = { 0 };
char SetTrackVolume[64] __attribute__((aligned(4))) = { 0 };
char SetWindowResizeLimits[64] __attribute__((aligned(4))) = { 0 };
char SetWRefCon[64] __attribute__((aligned(4))) = { 0 };
char ShowControl[64] __attribute__((aligned(4))) = { 0 };

char _snd_local_listener[64] __attribute__((aligned(4))) = { 0 };
char speex_nb_mode_ptr[64] __attribute__((aligned(4))) = { 0 };
char speex_quality_ptr[64] __attribute__((aligned(4))) = { 0 };
char speex_uwb_mode_ptr[64] __attribute__((aligned(4))) = { 0 };
char speex_wb_mode_ptr[64] __attribute__((aligned(4))) = { 0 };
char StartMovie[64] __attribute__((aligned(4))) = { 0 };
char StopMovie[64] __attribute__((aligned(4))) = { 0 };

char sv_cheats_ptr[64] __attribute__((aligned(4))) = { 0 };
char s_vc_logCount[64] __attribute__((aligned(4))) = { 0 };
char sv_com_dvarDump_ptr[64] __attribute__((aligned(4))) = { 0 };

char sv_dedicated_dvar2[64] __attribute__((aligned(4))) = { 0 };

char sv_privatePassword_dvar[64] __attribute__((aligned(4))) = { 0 };

char sv_showcommands_dvar[64] __attribute__((aligned(4))) = { 0 };

char tr[64] __attribute__((aligned(4))) = { 0 };

char TXNSetTypeAttributes[64] __attribute__((aligned(4))) = { 0 };
extern unsigned char __ZN12UI_Component1gE[];
#ifdef __EMSCRIPTEN__
/* WebAssembly has no cross-object ELF-style aliases.  The reconstructed C UI
 * module provides UI_Component_g directly; the generated BSS blob retains the
 * separately named C++ storage symbol for address-table compatibility. */
#elif defined(_MSC_VER)
/* alias UI_Component_g -> the C++ member symbol. The C identifier
 * __ZN12UI_Component1gE emits MSVC symbol ___ZN12UI_Component1gE (one extra
 * leading underscore on x86). */
COD2_ALT("UI_Component_g", "__ZN12UI_Component1gE")
#else
__asm__(".globl UI_Component_g\n.set UI_Component_g, __ZN12UI_Component1gE");
#endif
void *vec3_origin_ptr __attribute__((aligned(4))) = (void *)vec3_origin;
char version[64] __attribute__((aligned(4))) = { 0 };
char voice_freq_ptr[64] __attribute__((aligned(4))) = { 0 };
char voice_maxframe_ptr[64] __attribute__((aligned(4))) = { 0 };
char voice_scale_ptr[64] __attribute__((aligned(4))) = { 0 };
char WaitNextEvent[64] __attribute__((aligned(4))) = { 0 };

int GetHighAndLowIndices(void)
{
    return 0;
}
extern void *vtbl_CD3DXConstantTable[];
int D3DXGetShaderConstantTable(const void *function, void **constantTable)
{
#ifdef GFX_REAL_D3D9

    {
        typedef int(__stdcall * PFN_GetCT)(const void *, void **);
        extern void *__stdcall LoadLibraryA(const char *);
        extern void *__stdcall GetProcAddress(void *, const char *);
        static PFN_GetCT s_real;
        static int s_tried;
        if (!s_tried) {
            const char *libs[] = {
                "d3dx9_43.dll", "d3dx9_42.dll", "d3dx9_41.dll", "d3dx9_40.dll", "d3dx9_39.dll",
                "d3dx9_38.dll", "d3dx9_37.dll", "d3dx9_36.dll", "d3dx9_35.dll", "d3dx9_34.dll",
                "d3dx9_33.dll", "d3dx9_32.dll", "d3dx9_31.dll", "d3dx9_30.dll", "d3dx9_29.dll",
                "d3dx9_28.dll", "d3dx9_27.dll", "d3dx9_26.dll", "d3dx9_25.dll", "d3dx9_24.dll",
                "d3dx9.dll", 0 };
            int li;
            s_tried = 1;
            for (li = 0; libs[li]; li++) {
                void *m = LoadLibraryA(libs[li]);
                if (m) {
                    s_real = (PFN_GetCT)GetProcAddress(m, "D3DXGetShaderConstantTable");
                    if (s_real)
                        break;
                }
            }
        }
        if (s_real)
            return s_real(function, constantTable);
    }
#endif
    (void)function;
    if (constantTable) {

        void **ct = (void **)calloc(1, 4 * sizeof(void *));   /* 16 was x86 (4*4); x64 needs 4*8 */
        ct[0] = vtbl_CD3DXConstantTable;
        ct[1] = (void *)1;
        ct[2] = calloc(1, 32);
        ct[3] = (void *)32;
        *constantTable = ct;
    }
    return 0;
}

int g_dip_vs_null = 0;
int g_dip_vs_bound = 0;
int g_dip_vs_skip = 0;
int g_draw_count = 0;

void *CDirect3D_GetDirect3DInterface(void)
{
    return (void *)0;
}

#define VAO_X16 \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;    \
    vsink++;
void CDirect3DDevice_CreateAndSetFixedFunctionVAO(void)
{
    static volatile int vsink;
    VAO_X16 VAO_X16 VAO_X16 VAO_X16 VAO_X16 VAO_X16 VAO_X16 VAO_X16
}
#undef VAO_X16

extern void *__cxa_allocate_exception(unsigned int thrown_size);
extern void __cxa_throw(void *thrown, void *tinfo, void (*dest)(void *));
void *cxa_allocate_exception(unsigned int thrown_size)
{
    return __cxa_allocate_exception(thrown_size);
}
void cxa_throw(void *thrown, void *tinfo, void (*dest)(void *))
{
    __cxa_throw(thrown, tinfo, dest);
}
