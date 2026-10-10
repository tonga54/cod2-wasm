#include "common_types.h"
#include "imports.h"

#include <string.h>
#include <strings.h>
extern int stricmp(const char *s1, const char *s2);

extern void BG_InitWeaponString(int index, const char *name);
extern int FS_FOpenFileByMode(const char *filename, fileHandle_t *f, int mode);
extern void Com_Error(int code, const char *fmt, ...);
extern void FS_FCloseFile(fileHandle_t f);
extern int FS_Read(void *buffer, int len, fileHandle_t f);
extern void Com_BeginParseSession(const char *name);
extern char *Com_Parse(char **data_p);
extern void Com_EndParseSession(void);
extern void *Hunk_AllocLowInternal(int size);
extern void *Hunk_AllocLowAlignInternal(int size, int align);
extern qboolean G_ParseWeaponAccurayGraphs(WeaponDef *weapDef);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern qboolean Info_Validate(const char *s);
extern int ParseConfigStringToStruct(byte *pStruct, cspField_t *fields, int numFields, const char *buffer, int maxFieldTypes, qboolean (*parseSpecificFieldType)(byte *, const char *, int), long int (*setConfigString)(byte *, const char *));
extern char *I_strlwr(char *s);

static WeaponDef bg_defaultWeaponDefs;
static int g_playerAnimTypeNamesCount;
static char *g_playerAnimTypeNames[64];

#include "bg_weapons_load_obj_weaponDefFields_decls.inc"

__attribute__((used)) char *offhandClassNames[3] = {
    (char *)"None",
    (char *)"Frag Grenade",
    (char *)"Smoke Grenade",
};
__attribute__((used)) char *szProjectileExplosionNames[3] = {
    (char *)"grenade",
    (char *)"rocket",
    (char *)"none",
};
__attribute__((used)) char *szWeapStanceNames[3] = {
    (char *)"stand",
    (char *)"duck",
    (char *)"prone",
};
__attribute__((used)) char *szWeapSlotNames[3] = {
    (char *)"none",
    (char *)"primary",
    (char *)"primaryb",
};
__attribute__((used, packed, aligned(4)))
char *szWeapOverlayReticleNames[] = {
    "none",
    "crosshair",
    "FG42",
    "Springfield",
    "binoculars",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};
__attribute__((used)) char *szWeapClassNames[10] = {
    (char *)"rifle",
    (char *)"mg",
    (char *)"smg",
    (char *)"spread",
    (char *)"pistol",
    (char *)"grenade",
    (char *)"rocketlauncher",
    (char *)"turret",
    (char *)"non-player",
    (char *)"item",
};
__attribute__((used, packed, aligned(4)))
char *szWeapTypeNames[] = {
    "bullet",
    "grenade",
    "projectile",
    "binoculars",
    NULL,
    NULL,
};

#if defined(__x86_64__) || defined(_M_X64)

__attribute__((used))
cspField_t weaponDefFields[] = {
#    include "bg_weapons_load_obj_weaponDefFields.inc"
};
#else
__attribute__((used, packed, aligned(4)))
const char *weaponDefFields[] = {
#    include "bg_weapons_load_obj_weaponDefFields.inc"
};
#    define weaponDefFields ((cspField_t *)weaponDefFields)
#endif

int BG_GetWeaponSlotForName(const char *pszSlotName);
const char *BG_GetWeaponSlotNameForIndex(int iSlot);
void BG_LoadWeaponStrings(void);
void BG_LoadPlayerAnimTypes(void);
void SetConfigString(char **ppszConfigString, const char *pszKeyValue);
WeaponDef *BG_LoadDefaultWeaponDef(void);
static qboolean BG_ParseWeaponDefSpecificFieldType(byte *pStruct, const char *pValue, const int iFieldType);
static void SetConfigString2(byte *pMember, const char *pszKeyValue);
WeaponDef *BG_LoadWeaponDefInternal(const char *folder, const char *name);

static inline __attribute__((always_inline)) int FindNameIndex(const char *pValue, char **names, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        if (!stricmp(pValue, names[i])) {
            return i;
        }
    }
    return -1;
}

static inline __attribute__((always_inline)) void BG_InitWeaponDefStrings(WeaponDef *weapDef)
{
    const int *pType;
    const int *pTypeEnd;

    weapDef->szInternalName = (char *)"";
    pType = &weaponDefFields[0].iFieldType;
    pTypeEnd = &weaponDefFields[366].iFieldType;
    for (; pType != pTypeEnd; pType = (const int *)((const byte *)pType + sizeof(cspField_t))) {
        if (*pType == 0) {
            *(char **)((byte *)weapDef + pType[-1]) = (char *)"";
        }
    }
}

int BG_GetWeaponSlotForName(const char *pszSlotName)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (!stricmp(pszSlotName, szWeapSlotNames[i])) {
            break;
        }
    }
    if (i == 3) {
        i = 0;
    }

    return i;
}

const char *BG_GetWeaponSlotNameForIndex(int iSlot)
{
    return szWeapSlotNames[iSlot];
}

void BG_LoadWeaponStrings(void)
{
    int i;

    if (g_playerAnimTypeNamesCount > 0) {
        for (i = 0; i < g_playerAnimTypeNamesCount; i++) {
            BG_InitWeaponString(i, g_playerAnimTypeNames[i]);
        }
    }
}

void BG_LoadPlayerAnimTypes(void)
{
    fileHandle_t f;
    int iLen;
    char buf[4096];
    char *text_p;
    char *token;

    g_playerAnimTypeNamesCount = 0;

    iLen = FS_FOpenFileByMode("mp/playeranimtypes.txt", &f, 0);
    if (iLen < 0) {
        Com_Error(1, "\x15"
                     "Couldn't load file '%s'",
                  "mp/playeranimtypes.txt");
    }

    if (iLen > 4095) {
        FS_FCloseFile(f);
        Com_Error(1, "\x15'%s' max size exceeded", "mp/playeranimtypes.txt");
    }

    FS_Read(buf, iLen, f);
    buf[iLen] = '\0';
    FS_FCloseFile(f);

    text_p = buf;
    Com_BeginParseSession("BG_AnimParseAnimScript");

    while (1) {
        token = Com_Parse(&text_p);
        if (!token || *token == '\0') {
            break;
        }
        if (g_playerAnimTypeNamesCount > 63) {
            Com_Error(1, "\x15Player anim type array size exceeded");
        }
        g_playerAnimTypeNames[g_playerAnimTypeNamesCount] = Hunk_AllocLowInternal(strlen(token) + 1);
        strcpy(g_playerAnimTypeNames[g_playerAnimTypeNamesCount], token);
        g_playerAnimTypeNamesCount++;
    }

    Com_EndParseSession();
}

static inline __attribute__((always_inline)) void SetConfigString_core(char **ppszConfigString, const char *pszKeyValue)
{
    char *buf;

    if (*pszKeyValue == '\0') {
        *ppszConfigString = (char *)"";
        return;
    }

    buf = Hunk_AllocLowAlignInternal(strlen(pszKeyValue) + 1, 1);
    strcpy(buf, pszKeyValue);
    *ppszConfigString = buf;
}

void SetConfigString(char **ppszConfigString, const char *pszKeyValue)
{
    SetConfigString_core(ppszConfigString, pszKeyValue);
}

static void SetConfigString2(byte *pMember, const char *pszKeyValue)
{
    char *buf;

    if (*pszKeyValue == '\0') {
        *(char **)pMember = (char *)"";
        return;
    }

    buf = Hunk_AllocLowAlignInternal(strlen(pszKeyValue) + 1, 1);
    strcpy(buf, pszKeyValue);
    *(char **)pMember = buf;
}

WeaponDef *BG_LoadDefaultWeaponDef(void)
{
    BG_InitWeaponDefStrings(&bg_defaultWeaponDefs);

    bg_defaultWeaponDefs.szInternalName = "none";
    bg_defaultWeaponDefs.accuracyGraphName[0] = "noweapon.accu";
    bg_defaultWeaponDefs.accuracyGraphName[1] = "noweapon.accu";

    G_ParseWeaponAccurayGraphs(&bg_defaultWeaponDefs);

    return &bg_defaultWeaponDefs;
}

static qboolean BG_ParseWeaponDefSpecificFieldType(byte *pStruct, const char *pValue, const int iFieldType)
{
    int arrayIndex;
    int arraySize;
    WeaponDef *wd = (WeaponDef *)pStruct;

    switch (iFieldType - 8) {
    case 0:
        arrayIndex = FindNameIndex(pValue, szWeapTypeNames, 4);
        if (arrayIndex < 0) {
            Com_Error(1, "Unknown weapon type \"%s\" in \"%s\"\n", pValue, wd->szInternalName);
        }
        wd->weapType = arrayIndex;
        return 1;

    case 1:
        arrayIndex = FindNameIndex(pValue, szWeapClassNames, 10);
        if (arrayIndex < 0) {
            Com_Error(1, "Unknown weapon class \"%s\" in \"%s\"\n", pValue, wd->szInternalName);
        }
        wd->weapClass = arrayIndex;
        return 1;

    case 2:
        arrayIndex = FindNameIndex(pValue, szWeapOverlayReticleNames, 5);
        if (arrayIndex < 0) {
            Com_Error(1, "Unknown weapon overlay reticle \"%s\" in \"%s\"\n", pValue, wd->szInternalName);
        }
        wd->overlayReticle = arrayIndex;
        return 1;

    case 3:
        arrayIndex = FindNameIndex(pValue, szWeapSlotNames, 3);
        if (arrayIndex < 0) {
            Com_Error(1, "Unknown weapon slot \"%s\" in \"%s\"\n", pValue, wd->szInternalName);
        }
        wd->weapSlot = arrayIndex;
        return 1;

    case 4:
        arrayIndex = FindNameIndex(pValue, szWeapStanceNames, 3);
        if (arrayIndex < 0) {
            Com_Error(1, "Unknown weapon stance \"%s\" in \"%s\"\n", pValue, wd->szInternalName);
        }
        wd->stance = arrayIndex;
        return 1;

    case 5:
        arrayIndex = FindNameIndex(pValue, szProjectileExplosionNames, 3);
        if (arrayIndex < 0) {
            Com_Error(1, "Unknown projectile explosion \"%s\" in \"%s\"\n", pValue, wd->szInternalName);
        }
        wd->projExplosion = arrayIndex;
        return 1;

    case 6:
        arrayIndex = FindNameIndex(pValue, offhandClassNames, 3);
        if (arrayIndex < 0) {
            Com_Error(1, "Unknown offhand class \"%s\" in \"%s\"\n", pValue, wd->szInternalName);
        }
        wd->offhandClass = arrayIndex;
        return 1;

    case 7:
        arraySize = g_playerAnimTypeNamesCount;
        arrayIndex = FindNameIndex(pValue, g_playerAnimTypeNames, arraySize);
        if (arrayIndex < 0) {
            Com_Error(1, "Unknown playerAnimType \"%s\" in \"%s\"\n", pValue, wd->szInternalName);
        }
        wd->playerAnimType = arrayIndex;
        return 1;

    default:
        Com_Error(1, "Bad field type %i in %s\n", iFieldType, wd->szInternalName);
        return 0;
    }
}

WeaponDef *BG_LoadWeaponDefInternal(const char *folder, const char *name)
{
    WeaponDef *weapDef;
    fileHandle_t hFile;
    int iFileLength;
    char szFileName[64];
    char szBuffer[8192];

    weapDef = Hunk_AllocLowInternal(sizeof(WeaponDef));

    BG_InitWeaponDefStrings(weapDef);

    Com_sprintf(szFileName, 64, "weapons/%s/%s", folder, name);

    iFileLength = FS_FOpenFileByMode(szFileName, &hFile, 0);
    if (iFileLength < 0) {
        Com_Printf("^3WARNING: Could not load weapon file '%s'\n", szFileName);
        return 0;
    }

    FS_Read(szBuffer, 10, hFile);
    szBuffer[10] = '\0';
    if (memcmp(szBuffer, "WEAPONFILE", 10) != 0) {
        Com_Printf("^3WARNING: \"%s\" does not appear to be a weapon file\n", szFileName);
        FS_FCloseFile(hFile);
        return 0;
    }

    iFileLength -= 10;
    if (iFileLength > 8191) {
        Com_Printf("^3WARNING: \"%s\" Is too long of a weapon file to parse\n", szFileName);
        FS_FCloseFile(hFile);
        return 0;
    }

    memset(szBuffer, 0, 8192);
    FS_Read(szBuffer, iFileLength, hFile);
    szBuffer[iFileLength] = '\0';
    FS_FCloseFile(hFile);

    if (!Info_Validate(szBuffer)) {
        Com_Printf("^3WARNING: \"%s\" is not a valid weapon file\n", szFileName);
        return 0;
    }

    SetConfigString_core((char **)&weapDef->szInternalName, name);

    if (!ParseConfigStringToStruct((byte *)weapDef, weaponDefFields, 366, szBuffer, 16,
                                   BG_ParseWeaponDefSpecificFieldType,
                                   (long int (*)(byte *, const char *))SetConfigString2)) {
        weapDef = 0;
        return weapDef;
    }

    /* These reciprocals advance the ADS pose, not the firing/bolt cycle. */
    if (weapDef->iAdsTransInTime > 0) {
        weapDef->fOOPosAnimLength[0] = 1.0f / (float)weapDef->iAdsTransInTime;
    } else {
        weapDef->fOOPosAnimLength[0] = 1.0f / 300.0f;
    }

    if (weapDef->iAdsTransOutTime > 0) {
        weapDef->fOOPosAnimLength[1] = 1.0f / (float)weapDef->iAdsTransOutTime;
    } else {
        weapDef->fOOPosAnimLength[1] = 1.0f / 500.0f;
    }

    if (weapDef->destabilizationBaseTime <= 0.0f) {
        weapDef->destabilizationBaseTime = 0x1.e847ep+19f;
    }

    if (weapDef->destabilizationTimeReductionRatio <= 0.0f) {
        weapDef->destabilizationTimeReductionRatio = 0x1.e847e4p+19f;
    }

    if (weapDef->enemyCrosshairRange > 15000.0f) {
        Com_Error(1, "Enemy crosshair ranges should be less than %f ", 15000.0);
    }

    if (weapDef->weapType == 2) {
        if ((float)weapDef->iProjectileSpeed <= 0.0f) {
            Com_Error(1, "Projectile speed for WeapType %s must be greater than 0.0", weapDef->szDisplayName);
        }

        if (weapDef->destabilizationAngleMax >= 45.0f || weapDef->destabilizationAngleMax < 0.0f) {
            Com_Error(1, "Destabilization angle for for WeapType %s must be between 0 and 45 degrees", weapDef->szDisplayName);
        }

        if (weapDef->destabilizationBaseTime <= 0.0f) {
            Com_Error(1, "Destabilization base time for for WeapType %s must be positive", weapDef->szDisplayName);
        }

        if (weapDef->destabilizationTimeReductionRatio <= 0.0f) {
            Com_Error(1, "Destabilization reduction ratio for for WeapType %s must be positive", weapDef->szDisplayName);
        }
    }

    if (!G_ParseWeaponAccurayGraphs(weapDef)) {
        weapDef = 0;
        return weapDef;
    }

    I_strlwr(weapDef->szAmmoName);
    I_strlwr(weapDef->szClipName);

    return weapDef;
}
