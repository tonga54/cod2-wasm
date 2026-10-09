
#include "common_types.h"
#include "imports.h"
#include <string.h>
#include <stdlib.h>

#ifndef __EMSCRIPTEN__
#    define COM_REGPARM3 __attribute__((regparm(3)))
#else
#    define COM_REGPARM3
#endif

/* Original includes (from N_BINCL debug info):
 *   #include "PC/universal/com_math.h"
 */

extern struct saLoadObjGlob_type saLoadObjGlob;
static char szReference[1024];
/* migrated from the ILP32 data blob to typed C (x64 port Stage 2). */
const char *g_pszSndAliasKeyNames[24] = {
    NULL,
    "name",
    "sequence",
    "file",
    "subtitle",
    "vol_min",
    "vol_max",
    "vol_mod",
    "pitch_min",
    "pitch_max",
    "dist_min",
    "dist_max",
    "channel",
    "type",
    "loop",
    "probability",
    "loadspec",
    "masterslave",
    "secondaryaliasname",
    "volumefalloffcurve",
    "startdelay",
    "speakermap",
    "reverb",
    "lfe percentage",
};

/* g_pszChannelNames: migrated from the ILP32 data blob to typed C (re-lays-out
 * per target; trailing blob bytes were inter-symbol padding). (x64 port Stage 2.) */
const char *g_pszChannelNames[11] = {
    "auto", "auto2d", "menu",
    "weapon", "voice", "item",
    "body", "local", "music",
    "announcer", "shellshock",
};

extern int I_stricmp(const char *s0, const char *s1);
extern int I_strncmp(const char *s0, const char *s1, int n);
extern char *I_strupr(char *s);
extern void Com_Printf(const char *fmt, ...);
extern char *va(const char *format, ...);
extern qboolean FS_TouchFile(const char *name);
extern void *imp_fs_copyfiles;
extern void *imp_snd_touchStreamFilesOnLoad;
extern void SND_LoadSoundFile(SoundFile *soundFile);
extern void *Com_AllocSoundMemory(int size, const char *name, int type);
extern Bool Com_AddAliasList(const char *name, snd_alias_list_t *aliasList);
extern int FS_ReadFile(const char *qpath, void **buffer);
extern void FS_FreeFile(void *buffer);
extern void Com_BeginParseSession(const char *filename);
extern void Com_EndParseSession(void);
extern const char *Com_Parse(const char **data_p);
extern const char *Com_ParseOnLine(const char **data_p);
extern void Com_SkipRestOfLine(const char **data_p);
extern void Com_Error(int code, const char *fmt, ...);
extern int FS_Write(const void *buffer, int len, fileHandle_t h);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern int FS_FOpenFileRead(const char *qpath, fileHandle_t *file, qboolean uniqueFILE);
extern int FS_FOpenFileWrite(const char *filename);
extern void Com_UngetToken(void);
extern int FS_Read(void *buffer, int len, fileHandle_t h);
extern void FS_FCloseFile(fileHandle_t h);
extern int stricmp(const char *s0, const char *s1);
extern char *strlwr(char *s);
extern void *Hunk_AllocateTempMemoryInternal(int size);
extern SndCurve *Com_RegisterSoundAliasVolumeFalloffCurve(const char *filename, const char *sourceFile);
extern int ___maskrune(int c, unsigned long mask);
extern int sprintf(char *str, const char *format, ...);

extern void *imp_fs_basepath;
extern void *imp_fs_gamedir;
extern void *imp_fs_homepath;
extern void FS_BuildOSPath(const char *base, const char *game, const char *qpath, char *ospath);
extern void *FS_FileOpen(const char *path, const char *mode);
extern int FS_FileClose(void *stream);
extern int FS_FileExists(const char *qpath);
extern void FS_CopyFile(const char *fromOSPath, const char *toOSPath);
extern char **FS_ListFiles(const char *path, const char *extension, int behavior, int *numfiles, int wantsubs);
extern void FS_FreeFileList(char **list, int allocTrackType);
extern int Hunk_HideTempMemory(void);
extern void Hunk_ShowTempMemory(int mark);
extern void Hunk_ClearTempMemory(void);
extern int FS_FileSeek(void *stream, int offset, int origin);
extern int FS_FileRead(void *buffer, int size, int count, void *stream);
extern int FS_FileWrite(const void *buffer, int size, int count, void *stream);
extern void FS_Remove(const char *osPath);
extern void *malloc(size_t size);
extern void free(void *ptr);
extern void Com_SetCSV(qboolean csv);
extern void Cmd_AddCommand(const char *cmdName, void (*function)(void));
extern SndCurve *Com_GetDefaultSoundAliasVolumeFalloffCurve(void);
extern void *Com_AllocateTempSoundMemory(int size, const char *name);

static snd_alias_build_t *Com_SortTempSoundAliases_r_impl(snd_alias_build_t *pAliasList, int *piAliasCount, int (*test)(snd_alias_build_t *, snd_alias_build_t *), int isRemovingDups);

void Com_InitSoundAlias(void);
static void Com_RefreshVolumeModGroups_f(void);
static int AliasNameCompare(snd_alias_build_t *pFrontList, snd_alias_build_t *pBackList);
static snd_alias_build_t *COM_REGPARM3 Com_SortTempSoundAliases_r(snd_alias_build_t *pAliasList, int *piAliasCount, int (*test)(snd_alias_build_t *, snd_alias_build_t *), int isRemovingDups);
void Com_MakeSoundAliasesPermanent(snd_alias_list_t *aliasInfo, SoundFileInfo *soundFileInfo);
Bool Com_LoadVolumeFalloffCurve(const char *name, SndCurve *curve);
int Com_LoadSoundAliasSounds(SoundFileInfo *soundFileInfo);
void Com_InitDefaultSoundAliasVolumeFalloffCurve(SndCurve *sndCurve);
static const char *Com_GetSubtitleStringEdReference(const char *pszSubtitle);
static void COM_REGPARM3 Com_WriteStringEdReferenceToFile(const char *pszReference, const char *pszSubtitle, fileHandle_t hOutFile);
static int FileNameTypeCompare(snd_alias_build_t *frontList, snd_alias_build_t *backList);
static void COM_REGPARM3 Com_LoadSoundAliasField(const char *loadspec, const char *loadspecCurGame, const char *sourceFile, const char *token, snd_alias_members_t field, char *bFieldSet, snd_alias_build_t *alias);
void Com_ProcessSoundAliasFileLocalization(const char *sourceFile, const char *loadspecCurGame, const char *stringEdFileName);
void Com_WriteLocalizedSoundAliasFiles(void);
void Com_LoadSoundAliasFile(const char *loadspec, const char *loadspecCurGame, const char *sourceFile);

static inline __attribute__((always_inline)) const char *Com_ImportedDvarString(void *importedDvar)
{
    return (*(dvar_t **)importedDvar)->current.string;
}

static void Com_CopyFinalStringEdFile(const char *stringEdFileName, const char *stringEdExternalFileName)
{
    void *in;
    void *out;
    void *buffer;
    int length;

    in = FS_FileOpen(stringEdFileName, "rb");
    if (in) {
        FS_FileSeek(in, 0, 2);
        length = ftell(in);
        FS_FileSeek(in, 0, 0);

        buffer = malloc(length);
        if (FS_FileRead(buffer, 1, length, in) != length) {
            Com_Error(0, "\x15Short read in COM_WriteFinalStringEdFile()\n");
        }
        FS_FileClose(in);

        out = FS_FileOpen(stringEdExternalFileName, "wb");
        if (out) {
            if (FS_FileWrite(buffer, 1, length, out) != length) {
                Com_Error(0, "\x15Short write in COM_WriteFinalStringEdFile()\n");
            }
            FS_FileClose(out);
        }

        free(buffer);
    }

    FS_Remove(stringEdFileName);
    Com_Printf("done\n");
}

void Com_InitSoundAlias(void)
{
    saLoadObjGlob.tempAliases = NULL;
    saLoadObjGlob.tempAliasCount = 0;
}

static void Com_RefreshVolumeModGroups_f(void)
{
    saLoadObjGlob.volumeModGroupsInitialized = 0;
}

static int AliasNameCompare(snd_alias_build_t *pFrontList, snd_alias_build_t *pBackList)
{
    return I_stricmp(pFrontList->szAliasName, pBackList->szAliasName);
}

/* Recursive merge sort for sound alias linked lists.
 * Splits the list in half, recursively sorts both halves, then merges.
 * When isRemovingDups is set, duplicates (same test() result AND same iSequence)
 * are resolved by I_stricmp on szSourceFile: true duplicates (same file) print
 * an error and both are dropped; otherwise the lesser-sorted-file entry is dropped.
 */
static snd_alias_build_t *Com_SortTempSoundAliases_r_impl(snd_alias_build_t *pAliasList, int *piAliasCount, int (*test)(snd_alias_build_t *, snd_alias_build_t *), int isRemovingDups)
{
    snd_alias_build_t *pSplitPoint;
    snd_alias_build_t *pFrontList;
    snd_alias_build_t *pBackList;
    snd_alias_build_t *result;
    snd_alias_build_t **ppListPos;
    int iFrontCount;
    int iBackCount;
    int count;
    int cmp;
    int i;

    count = *piAliasCount;

    if (count == 1) {
        pAliasList->pNext = 0;
        return pAliasList;
    }

    iFrontCount = count / 2;
    iBackCount = count - iFrontCount;

    pSplitPoint = pAliasList;
    for (i = 0; i < iFrontCount; i++) {
        pSplitPoint = (snd_alias_build_t *)(uintptr_t)pSplitPoint->pNext;
    }

    pFrontList = Com_SortTempSoundAliases_r_impl(pAliasList, &iFrontCount, test, isRemovingDups);
    pBackList = Com_SortTempSoundAliases_r_impl(pSplitPoint, &iBackCount, test, isRemovingDups);

    *piAliasCount = 0;
    result = NULL;
    ppListPos = &result;

    for (;;) {
        if (isRemovingDups) {

            if (iFrontCount <= 0)
                goto front_exhausted;
            if (iBackCount <= 0)
                goto back_exhausted;

            cmp = test(pFrontList, pBackList);

            if (cmp == 0) {

                cmp = pFrontList->iSequence - pBackList->iSequence;

                if (cmp == 0) {

                    cmp = I_stricmp(pFrontList->szSourceFile, pBackList->szSourceFile);

                    if (cmp == 0) {

                        Com_Printf("^1ERROR: sound alias file %s: duplicate alias '%s'\n", pFrontList->szSourceFile, pFrontList->szAliasName);
                        pFrontList = (snd_alias_build_t *)(uintptr_t)pFrontList->pNext;
                        iFrontCount--;
                        pBackList = (snd_alias_build_t *)(uintptr_t)pBackList->pNext;
                        iBackCount--;
                        continue;
                    } else if (cmp < 0) {

                        pFrontList = (snd_alias_build_t *)(uintptr_t)pFrontList->pNext;
                        iFrontCount--;
                        continue;
                    } else {

                        pBackList = (snd_alias_build_t *)(uintptr_t)pBackList->pNext;
                        iBackCount--;
                        continue;
                    }
                }
            }

            if (cmp > 0) {
                *ppListPos = pBackList;
                pBackList = (snd_alias_build_t *)(uintptr_t)pBackList->pNext;
                iBackCount--;
            } else {
                *ppListPos = pFrontList;
                pFrontList = (snd_alias_build_t *)(uintptr_t)pFrontList->pNext;
                iFrontCount--;
            }
        } else {

            if (iFrontCount <= 0)
                goto front_exhausted;
            if (iBackCount <= 0)
                goto back_exhausted;

            cmp = test(pFrontList, pBackList);

            if (cmp > 0) {
                *ppListPos = pBackList;
                pBackList = (snd_alias_build_t *)(uintptr_t)pBackList->pNext;
                iBackCount--;
            } else {
                *ppListPos = pFrontList;
                pFrontList = (snd_alias_build_t *)(uintptr_t)pFrontList->pNext;
                iFrontCount--;
            }
        }

        (*piAliasCount)++;
        ppListPos = (snd_alias_build_t **)&((*ppListPos)->pNext);
    }

back_exhausted:

    *ppListPos = pFrontList;
    *piAliasCount += iFrontCount;
    return result;

front_exhausted:

    *ppListPos = pBackList;
    *piAliasCount += iBackCount;
    return result;
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
static snd_alias_build_t *COM_REGPARM3 Com_SortTempSoundAliases_r(snd_alias_build_t *pAliasList, int *piAliasCount, int (*test)(snd_alias_build_t *, snd_alias_build_t *), int isRemovingDups)
{
    return Com_SortTempSoundAliases_r_impl(pAliasList, piAliasCount, test, isRemovingDups);
}

void Com_MakeSoundAliasesPermanent(snd_alias_list_t *aliasInfo, SoundFileInfo *soundFileInfo)
{
    snd_alias_build_t *alias;
    snd_alias_build_t *other;
    snd_alias_build_t *sameSoundFile;
    snd_alias_list_t *aliasList;
    snd_alias_t *permAlias;
    SoundFile *currentSound;
    const char *currentName;
    const char *typeName;
    const char *otherTypeName;
    char *strings;
    char *aliasName;
    char *nextStrings;
    char *subtitle;
    char *stringsAfterSubtitle;
    int stringBytesCount;
    int soundCount;
    int sameType;
    int len;
    int type;
    int flags;

#    define SA_ALIAS_NAME(a) ((a)->szAliasName)
#    define SA_SECONDARY_NAME(a) ((a)->szSecondaryAliasName)
#    define SA_SUBTITLE(a) ((a)->subtitleText)
#    define SA_SEQUENCE(a) ((a)->iSequence)
#    define SA_SOUND_FILE(a) ((a)->szSoundFile)
#    define SA_PERM_SOUND(a) ((a)->permSoundFile)
#    define SA_VOL_MIN(a) ((a)->fVolMin)
#    define SA_VOL_MAX(a) ((a)->fVolMax)
#    define SA_PITCH_MIN(a) ((a)->fPitchMin)
#    define SA_PITCH_MAX(a) ((a)->fPitchMax)
#    define SA_DIST_MIN(a) ((a)->fDistMin)
#    define SA_DIST_MAX(a) ((a)->fDistMax)
#    define SA_CHANNEL(a) ((a)->iChannel)
#    define SA_TYPE(a) ((int)(a)->eType)
#    define SA_CURVE(a) ((a)->volumeFalloffCurve)
#    define SA_SLAVE_PERCENT(a) ((a)->fSlavePercentage)
#    define SA_PROBABILITY(a) ((a)->fProbability)
#    define SA_LFE_PERCENT(a) ((a)->fLfePercentage)
#    define SA_START_DELAY(a) ((a)->startDelay)
#    define SA_LOOPING(a) ((a)->bLooping)
#    define SA_MASTER(a) ((a)->bMaster)
#    define SA_SLAVE(a) ((a)->bSlave)
#    define SA_FULL_DRY(a) ((a)->bFullDryLevel)
#    define SA_NO_WET(a) ((a)->bNoWetLevel)
#    define SA_SAME_SOUND(a) (*(snd_alias_build_t **)&(a)->pSameSoundFile)
#    define SA_NEXT(a) (*(snd_alias_build_t **)&(a)->pNext)

    soundFileInfo->count = 0;
    aliasInfo->count = 0;

    if (!saLoadObjGlob.tempAliasCount) {
        return;
    }

    saLoadObjGlob.tempAliases = Com_SortTempSoundAliases_r(saLoadObjGlob.tempAliases, &saLoadObjGlob.tempAliasCount, AliasNameCompare, 1);
    if (!saLoadObjGlob.tempAliases) {
        return;
    }

    saLoadObjGlob.tempAliases = Com_SortTempSoundAliases_r(saLoadObjGlob.tempAliases, &saLoadObjGlob.tempAliasCount, FileNameTypeCompare, 0);
    if (!saLoadObjGlob.tempAliases) {
        return;
    }

    stringBytesCount = 0;
    other = NULL;
    currentName = NULL;
    soundCount = 0;

    for (alias = saLoadObjGlob.tempAliases; alias; alias = SA_NEXT(alias)) {
        len = (int)strlen(SA_SOUND_FILE(alias)) + 1;

        sameType = 0;
        if (other && SA_TYPE(alias) == SA_TYPE(other)) {
            sameType = 1;
        }

        if (currentName && !I_stricmp(currentName, SA_SOUND_FILE(alias))) {
            if (sameType) {
                SA_SAME_SOUND(alias) = other;
                continue;
            }

            type = SA_TYPE(alias);
            if (type == 2) {
                typeName = "streamed";
            } else if (type == 3) {
                typeName = "primed";
            } else {
                typeName = "loaded";
            }

            type = SA_TYPE(other);
            if (type == 2) {
                otherTypeName = "streamed";
            } else if (type == 3) {
                otherTypeName = "primed";
            } else {
                otherTypeName = "loaded";
            }

            Com_Printf("WARNING: sound file '%s' used as %s in alias '%s' and %s in alias '%s'\n", SA_SOUND_FILE(alias), typeName, SA_ALIAS_NAME(alias), otherTypeName, SA_ALIAS_NAME(other));
        }

        SA_SAME_SOUND(alias) = NULL;
        stringBytesCount += len;
        ++soundCount;
        other = alias;
        currentName = SA_SOUND_FILE(alias);
    }

    saLoadObjGlob.tempAliases = Com_SortTempSoundAliases_r(saLoadObjGlob.tempAliases, &saLoadObjGlob.tempAliasCount, AliasNameCompare, 1);
    if (!saLoadObjGlob.tempAliases) {
        return;
    }

    currentName = NULL;
    for (alias = saLoadObjGlob.tempAliases; alias; alias = SA_NEXT(alias)) {
        len = (int)strlen(SA_ALIAS_NAME(alias)) + 1;
        if (!currentName || I_stricmp(currentName, SA_ALIAS_NAME(alias))) {
            stringBytesCount += len;
            currentName = SA_ALIAS_NAME(alias);
        }

        if (SA_SUBTITLE(alias)) {
            stringBytesCount += (int)strlen(SA_SUBTITLE(alias)) + 1;
        }
    }

#    if defined(__x86_64__) || defined(_M_X64)

    aliasInfo->head = (snd_alias_t *)Com_AllocSoundMemory(saLoadObjGlob.tempAliasCount * (int)sizeof(snd_alias_t), "Com_MakeSoundAliasesPermanent:aliases", 0xe);
    soundFileInfo->files = (SoundFile *)Com_AllocSoundMemory(soundCount * (int)sizeof(SoundFile), "Com_MakeSoundAliasesPermanent:soundFiles", 0xe);
#    else
    aliasInfo->head = (snd_alias_t *)Com_AllocSoundMemory(saLoadObjGlob.tempAliasCount * 68, "Com_MakeSoundAliasesPermanent:aliases", 0xe);
    soundFileInfo->files = (SoundFile *)Com_AllocSoundMemory(soundCount * 16, "Com_MakeSoundAliasesPermanent:soundFiles", 0xe);
#    endif
    strings = (char *)Com_AllocSoundMemory(stringBytesCount, "Com_MakeSoundAliasesPermanent:strings", 0xe);

    alias = saLoadObjGlob.tempAliases;
    if (!alias) {
        return;
    }

    aliasList = NULL;
    aliasName = strings;

    for (;;) {
        strcpy(aliasName, SA_ALIAS_NAME(alias));
        nextStrings = aliasName + strlen(aliasName) + 1;

        for (;;) {
            if (SA_SUBTITLE(alias)) {
                strcpy(nextStrings, SA_SUBTITLE(alias));
                stringsAfterSubtitle = nextStrings + strlen(nextStrings) + 1;
                subtitle = nextStrings;
            } else {
                stringsAfterSubtitle = nextStrings;
                subtitle = NULL;
            }

            permAlias = &aliasInfo->head[aliasInfo->count];

            if (!aliasList || I_stricmp(aliasName, aliasList->head->pszAliasName)) {
#    if defined(__x86_64__) || defined(_M_X64)

                aliasList = (snd_alias_list_t *)Com_AllocSoundMemory((int)sizeof(snd_alias_list_t), "Com_MakeSoundAliasesPermanent:aliasList", 0xe);
#    else
                aliasList = (snd_alias_list_t *)Com_AllocSoundMemory(0x10, "Com_MakeSoundAliasesPermanent:aliasList", 0xe);
#    endif
                if (!Com_AddAliasList(aliasName, aliasList)) {
                    Com_Printf("^1ERROR: alias '%s' already added - ignoring\n", aliasName);
                    nextStrings = stringsAfterSubtitle;
                    aliasList = NULL;
                    goto next_alias;
                }

                aliasList->aliasName = aliasName;
                aliasList->head = permAlias;
            }

            sameSoundFile = SA_SAME_SOUND(alias);
            if (sameSoundFile) {
                currentSound = SA_PERM_SOUND(sameSoundFile);
                nextStrings = stringsAfterSubtitle;
            } else {
                strcpy(stringsAfterSubtitle, SA_SOUND_FILE(alias));
                nextStrings = stringsAfterSubtitle + strlen(stringsAfterSubtitle) + 1;

                currentSound = &soundFileInfo->files[soundFileInfo->count];
                currentSound->soundName = stringsAfterSubtitle;
                currentSound->fileMem = NULL;
                currentSound->type = (snd_alias_type_t)SA_TYPE(alias);
                ++soundFileInfo->count;
            }

            SA_PERM_SOUND(alias) = currentSound;

            permAlias->pszAliasName = aliasName;
            if (SA_SECONDARY_NAME(alias)[0]) {
                permAlias->pszSecondaryAliasName = (const char *)Com_AllocSoundMemory((int)strlen(SA_SECONDARY_NAME(alias)) + 1, "Com_AddSoundAlias", 0xe);
                strcpy((char *)permAlias->pszSecondaryAliasName, SA_SECONDARY_NAME(alias));
            } else {
                permAlias->pszSecondaryAliasName = NULL;
            }

            permAlias->soundFile = currentSound;
            permAlias->pszSubtitle = subtitle;
            permAlias->iSequence = SA_SEQUENCE(alias);
            permAlias->fVolMin = SA_VOL_MIN(alias);
            permAlias->fVolMax = SA_VOL_MAX(alias);
            permAlias->fPitchMin = SA_PITCH_MIN(alias);
            permAlias->fPitchMax = SA_PITCH_MAX(alias);
            permAlias->fDistMin = SA_DIST_MIN(alias);
            permAlias->fDistMax = SA_DIST_MAX(alias);

            flags = permAlias->flags;
            flags &= 0xfffff87f;
            flags |= SA_CHANNEL(alias) << 7;
            permAlias->flags = flags;

            flags &= 0xffffff9f;
            flags |= SA_TYPE(alias) << 5;
            permAlias->flags = flags;

            permAlias->volumeFalloffCurve = SA_CURVE(alias);

            if (SA_LOOPING(alias)) {
                flags |= 1;
            } else {
                flags &= ~1;
            }
            permAlias->flags = flags;

            if (SA_MASTER(alias)) {
                flags |= 2;
            } else {
                flags &= ~2;
            }
            permAlias->flags = flags;

            if (SA_SLAVE(alias)) {
                flags |= 4;
            } else {
                flags &= ~4;
            }
            permAlias->flags = flags;

            if (SA_FULL_DRY(alias)) {
                flags |= 8;
            } else {
                flags &= ~8;
            }
            permAlias->flags = flags;

            if (SA_NO_WET(alias)) {
                flags |= 0x10;
            } else {
                flags &= ~0x10;
            }
            permAlias->flags = flags;

            permAlias->fSlavePercentage = SA_SLAVE_PERCENT(alias);
            permAlias->fProbability = SA_PROBABILITY(alias);
            permAlias->fLfePercentage = SA_LFE_PERCENT(alias);
            permAlias->startDelay = SA_START_DELAY(alias);

            ++aliasInfo->count;
            ++aliasList->count;

        next_alias:
            alias = SA_NEXT(alias);
            if (!alias) {
                return;
            }

            if (aliasName && !I_stricmp(aliasName, SA_ALIAS_NAME(alias))) {
                continue;
            }

            aliasName = nextStrings;
            break;
        }
    }

#    undef SA_ALIAS_NAME
#    undef SA_SECONDARY_NAME
#    undef SA_SUBTITLE
#    undef SA_SEQUENCE
#    undef SA_SOUND_FILE
#    undef SA_PERM_SOUND
#    undef SA_VOL_MIN
#    undef SA_VOL_MAX
#    undef SA_PITCH_MIN
#    undef SA_PITCH_MAX
#    undef SA_DIST_MIN
#    undef SA_DIST_MAX
#    undef SA_CHANNEL
#    undef SA_TYPE
#    undef SA_CURVE
#    undef SA_SLAVE_PERCENT
#    undef SA_PROBABILITY
#    undef SA_LFE_PERCENT
#    undef SA_START_DELAY
#    undef SA_LOOPING
#    undef SA_MASTER
#    undef SA_SLAVE
#    undef SA_FULL_DRY
#    undef SA_NO_WET
#    undef SA_SAME_SOUND
#    undef SA_NEXT
}

Bool Com_LoadVolumeFalloffCurve(const char *name, SndCurve *curve)
{
    char fileName[64];
    char buffer[8192];
    const char *parse;
    fileHandle_t fileHandle;
    int fileLength;
    int payloadLength;
    int knotIndex;

    Com_sprintf(fileName, sizeof(fileName), "soundaliases/%s.vfcurve", name);

    fileLength = FS_FOpenFileRead(fileName, &fileHandle, 1);
    if (fileLength < 0) {
        Com_Printf("^1ERROR: Could not load sndcurve file '%s'\n", fileName);
        return 0;
    }

    if (fileLength == 0) {
        FS_FCloseFile(fileHandle);
        Com_Printf("^1ERROR: sndcurve file '%s' is empty\n", fileName);
        return 0;
    }

    FS_Read(buffer, 8, fileHandle);
    buffer[8] = '\0';

    if (memcmp(buffer, "SNDCURVE", 8)) {
        FS_FCloseFile(fileHandle);
        Com_Printf("^1ERROR: \"%s\" does not appear to be a sndcurve file\n", fileName);
        return 0;
    }

    payloadLength = fileLength - 8;
    if (payloadLength > 0x1fff) {
        FS_FCloseFile(fileHandle);
        Com_Printf("^1ERROR: \"%s\" Is too long of a sndcurve file to parse\n", fileName);
        return 0;
    }

    memset(buffer, 0, sizeof(buffer));
    FS_Read(buffer, payloadLength, fileHandle);
    buffer[payloadLength] = '\0';
    FS_FCloseFile(fileHandle);

    parse = buffer;
    Com_BeginParseSession(fileName);

    curve->knotCount = atoi(Com_Parse(&parse));
    if (curve->knotCount <= 1) {
        Com_EndParseSession();
        Com_Printf("^1ERROR: sndcurve parse failure on file \"%s\": knot count (%d) is less than 2\n", fileName, curve->knotCount);
        return 0;
    }

    if (curve->knotCount > 8) {
        Com_EndParseSession();
        Com_Printf("^1ERROR: sndcurve parse failure on file \"%s\": knot count (%d) is greater than maxKnots (%d)\n", fileName, curve->knotCount, 8);
        return 0;
    }

    knotIndex = 0;
    for (;;) {
        const char *token = Com_Parse(&parse);

        if (!*token || *token == '}') {
            break;
        }

        if (knotIndex == 8) {
            Com_EndParseSession();
            Com_Printf("^1ERROR: sndcurve parse failure on file \"%s\": knots parsed (%d) is greater than or equal to maxKnots (%d)\n", fileName, 8, 8);
            return 0;
        }

        curve->knots[knotIndex][0] = (float)atof(token);
        if (curve->knots[knotIndex][0] < 0.0f || curve->knots[knotIndex][0] > 1.0f) {
            Com_EndParseSession();
            Com_Printf("^1ERROR: sndcurve parse failure on file \"%s\": knot x-coord '%f' is not in the range 0-1.\n", fileName, (double)curve->knots[knotIndex][0]);
            return 0;
        }

        token = Com_Parse(&parse);
        if (!*token || *token == '}') {
            break;
        }

        curve->knots[knotIndex][1] = (float)atof(token);
        if (curve->knots[knotIndex][1] < 0.0f || curve->knots[knotIndex][1] > 1.0f) {
            Com_EndParseSession();
            Com_Printf("^1ERROR: sndcurve parse failure on file \"%s\": knot x-coord '%f' is not in the range 0-1.\n", fileName, (double)curve->knots[knotIndex][1]);
            return 0;
        }

        ++knotIndex;
    }

    Com_EndParseSession();

    if (curve->knotCount != knotIndex) {
        Com_Printf("^1ERROR: sndcurve parse failure on file \"%s\": knot count (%d) does not match knots parsed (%d).\n", fileName, curve->knotCount, knotIndex);
        return 0;
    }

    if (curve->knots[0][0] != 0.0f || curve->knots[0][1] != 1.0f ||
        curve->knots[curve->knotCount - 1][0] != 1.0f || curve->knots[curve->knotCount - 1][1] != 0.0f) {
        curve->knots[0][0] = 0.0f;
        curve->knots[0][1] = 1.0f;
        curve->knots[curve->knotCount - 1][0] = 1.0f;
        curve->knots[curve->knotCount - 1][1] = 0.0f;
        Com_Printf("^3WARNING^7: sndcurve parse on file \"%s\": the first point must be '0.0000 1.0000' and the last point must be '1.0000 0.0000'.\nadjusting sndcurve endpoints.\n", fileName);
    }

    curve->filename = name;
    return 1;
}

int Com_LoadSoundAliasSounds(SoundFileInfo *soundFileInfo)
{
    int soundIndex;
    int numMissing;

    if (soundFileInfo->count <= 0) {
        return 0;
    }

    numMissing = 0;
    for (soundIndex = 0; soundIndex < soundFileInfo->count; ++soundIndex) {
        SoundFile *soundFile = &soundFileInfo->files[soundIndex];

        if (soundFile->type == 1) {
            SND_LoadSoundFile(soundFile);
            if (!soundFile->fileMem) {
                ++numMissing;
            }
            continue;
        }

        if ((*(dvar_t **)imp_snd_touchStreamFilesOnLoad)->current.enabled ||
            (*(dvar_t **)imp_fs_copyfiles)->current.enabled) {
            soundFile->isStreamFound = FS_TouchFile(va("sound/%s", soundFile->soundName));
        } else {
            soundFile->isStreamFound = 1;
        }

        if (!soundFile->isStreamFound) {
            Com_Printf("^1ERROR: Streamed sound file '%s' not found\n", soundFile->soundName);
        }
    }

    return numMissing;
}

void Com_InitDefaultSoundAliasVolumeFalloffCurve(SndCurve *sndCurve)
{
    sndCurve->filename = "";
    sndCurve->knots[0][0] = 0.0f;
    sndCurve->knots[0][1] = 1.0f;
    sndCurve->knots[1][0] = 1.0f;
    sndCurve->knots[1][1] = 0.0f;
    sndCurve->knotCount = 2;
}

static const char *Com_GetSubtitleStringEdReference(const char *pszSubtitle)
{
    char *file;
    const char *ptr;
    const char *token;

    if (FS_ReadFile("soundaliases/subtitle.st", (void **)&file) < 0) {
        Com_Printf("WARNING: Could not read local copy of StringEd file %s\n", "soundaliases/subtitle.st");
        return NULL;
    }

    Com_BeginParseSession("soundaliases/subtitle.st");
    ptr = file;

    for (token = Com_Parse(&ptr); ptr; token = Com_Parse(&ptr)) {
        if (!strncmp(token, "REFERENCE", 10)) {
            strcpy(szReference, Com_ParseOnLine(&ptr));
            Com_SkipRestOfLine(&ptr);

            while (1) {
                token = Com_Parse(&ptr);
                if (!ptr)
                    Com_Error(1, "\x15StringEd file %s has bad syntax", "soundaliases/subtitle.st");

                if (!strncmp(token, "LANG_ENGLISH", 13)) {
                    if (!I_stricmp(pszSubtitle, Com_ParseOnLine(&ptr))) {
                        Com_EndParseSession();
                        FS_FreeFile(file);
                        return szReference;
                    }
                }
            }
        }

        Com_SkipRestOfLine(&ptr);
    }

    Com_EndParseSession();
    FS_FreeFile(file);
    return NULL;
}

static void COM_REGPARM3 Com_WriteStringEdReferenceToFile(const char *pszReference, const char *pszSubtitle, fileHandle_t hOutFile)
{
    FS_Write("REFERENCE           ", strlen("REFERENCE           "), hOutFile);
    FS_Write(pszReference, strlen(pszReference), hOutFile);
    FS_Write("\r\nLANG_ENGLISH        \"", strlen("\r\nLANG_ENGLISH        \""), hOutFile);
    FS_Write(pszSubtitle, strlen(pszSubtitle), hOutFile);
    FS_Write("\"\r\n\r\n", strlen("\"\r\n\r\n"), hOutFile);
}

static int FileNameTypeCompare(snd_alias_build_t *frontList, snd_alias_build_t *backList)
{
    int cmp;

    cmp = I_stricmp(frontList->szSoundFile, backList->szSoundFile);
    if (cmp)
        return cmp;

    cmp = (int)frontList->eType - (int)backList->eType;
    if (cmp)
        return cmp;

    return I_stricmp(frontList->szAliasName, backList->szAliasName);
}

static int Com_SoundAliasCharIsIdent(int c)
{
    signed char sc;

    sc = (signed char)c;
    if (sc <= 31)
        return 0;

    if (sc == '_')
        return 1;

    return ___maskrune(sc, 0x500) != 0;
}

static const char *Com_SoundAliasFindInvalidIdentifierChar(const char *text)
{
    const char *scan;

    for (scan = text; *scan; ++scan) {
        if (!Com_SoundAliasCharIsIdent((unsigned char)*scan))
            return scan;
    }

    return NULL;
}

static int Com_SoundAliasLoadspecKeep(const char *loadspec, const char *loadspecCurGame, const char *token, const char *sourceFile)
{
    char loadlist[0x4000];
    char *list;
    char *match;
    int len;
    int keep;

    len = strlen(loadspec);
    loadlist[0x3fff] = 0;
    strncpy(loadlist, token, sizeof(loadlist));
    if (loadlist[0x3fff]) {
        Com_Printf("^1ERROR: Sound alias file %s: loadspec is > %i characters\n", sourceFile, 0x3fff);
        return 0;
    }

    strlwr(loadlist);

    if (loadlist[0] == '!') {
        keep = !strcmp(loadspec, "menu");
    } else {
        keep = 1;
    }

    list = loadlist;
    for (;;) {
        match = strstr(list, loadspec);
        if (!match)
            break;

        if ((match == loadlist || match[-1] <= '!') && match[len] <= ' ')
            return keep;

        list = match + 1;
    }

    if (strcmp(loadspec, "menu") && !strcmp(list, loadspecCurGame))
        return keep;

    return keep ^ 1;
}

static void COM_REGPARM3 Com_LoadSoundAliasField(const char *loadspec, const char *loadspecCurGame, const char *sourceFile, const char *token, snd_alias_members_t field, char *bFieldSet, snd_alias_build_t *alias)
{
    const char *badChar;
    float value;
    int i;
    int len;
    char loadlist[0x4000];

    if (!field)
        return;

    if (bFieldSet[field]) {
        Com_Printf("^1ERROR: Sound alias file %s: Duplicate entries for the '%s' column\n", sourceFile, g_pszSndAliasKeyNames[field]);
        alias->error = 1;
        return;
    }

    bFieldSet[field] = 1;

    switch (field) {
    case SA_NAME:
        if (strlen(token) > 0x3e) {
            Com_Printf("^1ERROR: Sound alias file %s: Alias name '%s' is longer than %i characters\n", sourceFile, token, 0x3f);
            alias->error = 1;
            return;
        }

        badChar = Com_SoundAliasFindInvalidIdentifierChar(token);
        if (badChar) {
            Com_Printf("^1ERROR: Sound alias file %s: Alias name '%s' is invalid\n", sourceFile, badChar);
            alias->error = 1;
            return;
        }

        strcpy(alias->szAliasName, token);
        return;

    case SA_SEQUENCE:
        alias->iSequence = atoi(token);
        return;

    case SA_FILE:
        if (strlen(token) > 0x3e) {
            Com_Printf("^1ERROR: Sound alias file %s: Sound file '%s' is longer than %i characters\n", sourceFile, token, 0x3f);
            alias->error = 1;
            return;
        }

        strcpy(alias->szSoundFile, token);
        return;

    case SA_SUBTITLE:
        len = strlen(token);
        for (i = 0; i < len; ++i) {
            if ((signed char)token[i] < 0) {
                Com_Printf("^1ERROR: Sound alias file %s: Subtitle '%s' has invalid character '%c' ascii %i\n", sourceFile, token, (int)(signed char)token[i], (unsigned char)token[i]);
                alias->error = 1;
                return;
            }
        }

        alias->subtitleText = (char *)Hunk_AllocateTempMemoryInternal(len + 1);
        memcpy(alias->subtitleText, token, len);
        alias->subtitleText[len] = 0;
        return;

    case SA_VOL_MIN:
        value = (float)atof(token);
        alias->fVolMin = value;
        if (value < 0.0f || value > 1.0f) {
            Com_Printf("^1ERROR: Sound alias file %s: MinVolume '%f' is not within the range of '%f'-'%f'.\n", sourceFile, value, 0.0, 1.0);
            alias->error = 1;
            return;
        }

        if (!bFieldSet[SA_VOL_MAX])
            alias->fVolMax = value;
        return;

    case SA_VOL_MAX:
        value = (float)atof(token);
        alias->fVolMax = value;
        if (value < 0.0f || value > 1.0f) {
            Com_Printf("^1ERROR: Sound alias file %s: MaxVolume '%f' is not within the range of '%f'-'%f'.\n", sourceFile, value, 0.0, 1.0);
            alias->error = 1;
        }
        return;

    case SA_VOL_MOD:
        for (i = 0; i < 32; ++i) {
            if (!stricmp(token, saLoadObjGlob.volumeModGroups[i].name)) {
                alias->fVolMod = saLoadObjGlob.volumeModGroups[i].value;
                return;
            }
        }

        Com_Error(1, "\x15Sound alias file %s: Volume Mod Group '%s' not found.", sourceFile, token);
        alias->fVolMod = 0.0f;
        return;

    case SA_PITCH_MIN:
        value = (float)atof(token);
        alias->fPitchMin = value;
        if (!bFieldSet[SA_PITCH_MAX])
            alias->fPitchMax = value;
        return;

    case SA_PITCH_MAX:
        alias->fPitchMax = (float)atof(token);
        return;

    case SA_DIST_MIN:
        alias->fDistMin = (float)atof(token);
        return;

    case SA_DIST_MAX:
        alias->fDistMax = (float)atof(token);
        return;

    case SA_CHANNEL:
        for (i = 0; i < 11; ++i) {
            if (!I_stricmp(token, g_pszChannelNames[i])) {
                alias->iChannel = i;
                return;
            }
        }

        len = 0;
        for (i = 0; i < 11; ++i) {
            len += sprintf(loadlist + len, "%s", g_pszChannelNames[i]);
            if (i <= 8) {
                loadlist[len++] = ',';
                loadlist[len++] = ' ';
                loadlist[len] = 0;
            } else if (i == 9) {
                loadlist[len++] = ' ';
                loadlist[len++] = 'o';
                loadlist[len++] = 'r';
                loadlist[len++] = ' ';
                loadlist[len] = 0;
            }
        }

        Com_Printf("^1ERROR: Sound alias file %s: Unknown sound channel '%s'; should be %s\n", sourceFile, token, loadlist);
        alias->error = 1;
        return;

    case SA_TYPE:
        if (!I_stricmp(token, "streamed")) {
            alias->eType = (snd_alias_type_t)2;
            return;
        }

        if (!I_stricmp(token, "primed")) {
            alias->eType = (snd_alias_type_t)2;
            return;
        }

        if (!I_stricmp(token, "loaded")) {
            alias->eType = (snd_alias_type_t)1;
            return;
        }

        Com_Printf("^1ERROR: Sound alias file %s: Unknown sound type '%s'; should be primed, streamed or loaded\n", sourceFile, token);
        alias->error = 1;
        return;

    case SA_LOOP:
        if (!I_stricmp(token, "looping")) {
            alias->bLooping = 1;
            return;
        }

        if (!I_stricmp(token, "nonlooping")) {
            alias->bLooping = 0;
            return;
        }

        Com_Printf("^1ERROR: Sound alias file %s: Unknown sound looping type '%s'; should be looping or nonlooping\n", sourceFile, token);
        alias->error = 1;
        return;

    case SA_PROBABILITY:
        alias->fProbability = (float)atof(token);
        return;

    case SA_LOADSPEC:
        alias->keep = Com_SoundAliasLoadspecKeep(loadspec, loadspecCurGame, token, sourceFile);
        return;

    case SA_MASTERSLAVE:
        if (!stricmp(token, "master")) {
            alias->bMaster = 1;
            alias->bSlave = 0;
            return;
        }

        alias->bMaster = 0;
        alias->bSlave = 1;
        value = (float)atof(token);
        alias->fSlavePercentage = value;
        if (value < 0.0f || value > 1.0f) {
            Com_Printf("^1ERROR: Sound alias file %s: SlavePercentage'%f' is not within the range of '%f'-'%f'.\n", sourceFile, value, 0.0, 1.0);
            alias->error = 1;
        }
        return;

    case SA_SECONDARYALIASNAME:
        if (strlen(token) > 0x3e) {
            Com_Printf("^1ERROR: Sound alias file %s: Secondary Alias name '%s' is longer than %i characters\n", sourceFile, token, 0x3f);
            alias->error = 1;
            return;
        }

        badChar = Com_SoundAliasFindInvalidIdentifierChar(token);
        if (badChar) {
            Com_Printf("^1ERROR: Sound alias file %s: Secondary Alias name '%s' is invalid\n", sourceFile, badChar);
            alias->error = 1;
            return;
        }

        strcpy(alias->szSecondaryAliasName, token);
        return;

    case SA_VOLUMEFALLOFFCURVE:
        alias->volumeFalloffCurve = Com_RegisterSoundAliasVolumeFalloffCurve(token, sourceFile);
        return;

    case SA_STARTDELAY:
        alias->startDelay = atoi(token);
        return;

    case SA_SPEAKERMAP:
        return;

    case SA_REVERB:
        if (strstr(token, "fulldrylevel"))
            alias->bFullDryLevel = 1;
        if (strstr(token, "nowetlevel"))
            alias->bNoWetLevel = 1;
        return;

    case SA_LFEPERCENTAGE:
        value = (float)atof(token);
        if (value > 1.0f)
            value = 1.0f;
        if (value < 0.0f)
            value = 0.0f;
        alias->fLfePercentage = value;
        return;

    default:
        return;
    }
}

static int Com_SoundAliasNeedsCsvQuotes(const char *token)
{
    return strchr(token, ',') || strchr(token, ' ') || strchr(token, '\n') || strchr(token, '\r');
}

static void Com_SoundAliasWriteCsvToken(fileHandle_t h, const char *token, qboolean lastColumn)
{
    const char *text;

    if (Com_SoundAliasNeedsCsvQuotes(token)) {
        text = va(lastColumn ? "\"%s\"" : "\"%s\",", token);
    } else {
        text = va(lastColumn ? "%s" : "%s,", token);
    }

    FS_Write(text, strlen(text), h);
}

static qboolean Com_SoundAliasSubtitleIsReference(const char *subtitle)
{
    const char *scan;

    if (!*subtitle) {
        return 0;
    }

    for (scan = subtitle; *scan; ++scan) {
        if ((*scan >= 'A' && *scan <= 'Z') || (*scan >= '0' && *scan <= '9') || *scan == '_') {
            continue;
        }
        return 0;
    }

    return I_strncmp(subtitle, "SUBTITLE_", 9) == 0;
}

static qboolean Com_SoundAliasSubtitleReferenceExists(const char *subtitleReference)
{
    char *file;
    const char *ptr;
    const char *token;
    qboolean found;

    if (FS_ReadFile("soundaliases/subtitle.st", (void **)&file) < 0) {
        Com_Printf("WARNING: Could not read local copy of StringEd file %s\n", "soundaliases/subtitle.st");
        return 0;
    }

    Com_BeginParseSession("soundaliases/subtitle.st");
    ptr = file;
    found = 0;

    while (ptr) {
        token = Com_Parse(&ptr);
        if (!ptr) {
            break;
        }

        if (!strncmp(token, "REFERENCE", 10)) {
            if (!I_stricmp(subtitleReference + 9, Com_ParseOnLine(&ptr))) {
                found = 1;
                break;
            }
        }

        Com_SkipRestOfLine(&ptr);
    }

    Com_EndParseSession();
    FS_FreeFile(file);
    return found;
}

static void Com_SoundAliasInitBuildDefaults(snd_alias_build_t *alias, const char *sourceFile)
{
    memset(alias, 0, sizeof(*alias));
    strcpy(alias->szSourceFile, sourceFile);
    alias->fVolMin = 1.0f;
    alias->fVolMax = 1.0f;
    alias->fVolMod = 1.0f;
    alias->fPitchMin = 1.0f;
    alias->fPitchMax = 1.0f;
    alias->fDistMin = 120.0f;
    alias->eType = (snd_alias_type_t)1;
    alias->fSlavePercentage = 1.0f;
    alias->fProbability = 1.0f;
    alias->volumeFalloffCurve = Com_GetDefaultSoundAliasVolumeFalloffCurve();
}

static void Com_SoundAliasWriteStringEdFile(const char *subtitle, const char *reference)
{
    char *file;
    const char *ptr;
    const char *start;
    const char *token;
    fileHandle_t hOutFile;
    qboolean referenceAdded;
    char szFromFile[256];
    char szToFile[256];

    hOutFile = FS_FOpenFileWrite("soundaliases/temp.st");
    if (!hOutFile) {
        Com_Printf("WARNING: Could not open output file %s for writing\n", "soundaliases/temp.st");
        return;
    }

    if (FS_ReadFile("soundaliases/subtitle.st", (void **)&file) < 0) {
        Com_Printf("WARNING: Could not read local copy of StringEd file %s\n", "soundaliases/subtitle.st");
        FS_FCloseFile(hOutFile);
        return;
    }

    Com_BeginParseSession("soundaliases/subtitle.st");
    ptr = file;
    start = ptr;
    referenceAdded = 0;

    while (ptr) {
        token = Com_Parse(&ptr);
        if (!ptr) {
            referenceAdded = 1;
            start = NULL;
            break;
        }

        if (!strncmp(token, "ENDMARKER", 10)) {
            if (start < ptr) {
                FS_Write(start, (int)(ptr - start) - 11, hOutFile);
            }
            break;
        }

        if (strncmp(token, "REFERENCE", 10)) {
            Com_SkipRestOfLine(&ptr);
            continue;
        }

        if (strcmp(Com_ParseOnLine(&ptr), reference + 9)) {
            Com_SkipRestOfLine(&ptr);
            continue;
        }

        if (start < ptr) {
            FS_Write(start, (int)(ptr - start), hOutFile);
        }

        Com_WriteStringEdReferenceToFile(reference + 9, subtitle, hOutFile);

        for (;;) {
            start = ptr;
            token = Com_Parse(&ptr);
            if (!ptr) {
                referenceAdded = 1;
                start = NULL;
                break;
            }
            if (!strncmp(token, "REFERENCE", 10) || !strncmp(token, "ENDMARKER", 10)) {
                Com_UngetToken();
                referenceAdded = 1;
                break;
            }
        }

        if (referenceAdded) {
            continue;
        }
    }

    if (!referenceAdded) {
        Com_WriteStringEdReferenceToFile(reference + 9, subtitle, hOutFile);
    }

    Com_EndParseSession();
    FS_FreeFile(file);
    FS_Write("\r\nENDMARKER\r\n\r\n\r\n", 17, hOutFile);
    FS_FCloseFile(hOutFile);

    FS_BuildOSPath(Com_ImportedDvarString(imp_fs_basepath), (const char *)imp_fs_gamedir, "soundaliases/temp.st", szFromFile);
    FS_BuildOSPath(Com_ImportedDvarString(imp_fs_basepath), (const char *)imp_fs_gamedir, "soundaliases/subtitle.st", szToFile);
    FS_CopyFile(szFromFile, szToFile);
    FS_Remove(szFromFile);
}

void Com_ProcessSoundAliasFileLocalization(const char *sourceFile, const char *loadspecCurGame, const char *stringEdFileName)
{
    (void)stringEdFileName;
    char soundAliasFile[256];
    char szFullPath[256];
    char szFromFile[256];
    char szToFile[256];
    char szNewReference[1024];
    char szAliasTokens[SA_NUMFIELDS][1024];
    snd_alias_members_t columnFields[256];
    char bFieldSet[SA_NUMFIELDS];
    snd_alias_build_t alias;
    char *file;
    const char *ptr;
    const char *lineStart;
    const char *token;
    fileHandle_t hAliasOutFile;
    int iColCount;
    int iNumSubtitlesLocalized;
    int col;
    qboolean bHasName;
    qboolean bHasFile;
    void *checkFile;

    Com_sprintf(soundAliasFile, sizeof(soundAliasFile), "soundaliases/%s", sourceFile);
    FS_BuildOSPath(Com_ImportedDvarString(imp_fs_basepath), (const char *)imp_fs_gamedir, soundAliasFile, szFullPath);
    Com_Printf("Processing sound alias file %s..\n", szFullPath);

    checkFile = FS_FileOpen(szFullPath, "r+");
    if (!checkFile) {
        Com_Printf("WARNING: Can not write to sound alias file %s\n", szFullPath);
        return;
    }
    FS_FileClose(checkFile);

    if (FS_ReadFile(soundAliasFile, (void **)&file) < 0) {
        Com_Printf("WARNING: Could not read sound alias file %s\n", soundAliasFile);
        return;
    }

    hAliasOutFile = FS_FOpenFileWrite("soundaliases/temp.csv");
    if (!hAliasOutFile) {
        Com_Printf("WARNING: Could not open output file %s for writing\n", "soundaliases/temp.csv");
        return;
    }

    Com_BeginParseSession(soundAliasFile);
    Com_SetCSV(1);

    ptr = file;
    iColCount = 0;
    iNumSubtitlesLocalized = 0;

    for (;;) {
        lineStart = ptr;
        if (!ptr) {
            break;
        }

        while (*ptr == '\r') {
            ptr++;
            lineStart = ptr;
        }

        if (*ptr == '\n') {
            ptr++;
            FS_Write("\r\n", 2, hAliasOutFile);
            continue;
        }

        token = Com_Parse(&ptr);
        if (!ptr) {
            break;
        }

        (void)I_stricmp(token, "#Chateau");

        if (!*token || *token == '#') {
            Com_SkipRestOfLine(&ptr);
            if (*lineStart == '\n') {
                FS_Write("\r", 1, hAliasOutFile);
            }
            FS_Write(lineStart, (int)(ptr - lineStart), hAliasOutFile);
            continue;
        }

        if (!iColCount) {
            bHasName = 0;
            bHasFile = 0;

            for (col = 0; col < 256; ++col) {
                int field;

                columnFields[col] = SA_INVALID;
                for (field = SA_NAME; field < SA_NUMFIELDS; ++field) {
                    if (!I_stricmp(g_pszSndAliasKeyNames[field], token)) {
                        columnFields[col] = (snd_alias_members_t)field;
                        if (field == SA_NAME) {
                            bHasName = 1;
                        } else if (field == SA_FILE) {
                            bHasFile = 1;
                        }
                        break;
                    }
                }

                iColCount = col + 1;
                if (iColCount == 256 || !ptr || *ptr == '\n') {
                    break;
                }

                token = Com_ParseOnLine(&ptr);
            }

            if (!bHasName || !bHasFile) {
                Com_Error(1, "\x15Sound alias file %s: missing 'name' and/or 'file' columns\n", sourceFile);
            }

            Com_SkipRestOfLine(&ptr);
            if (*lineStart == '\n') {
                FS_Write("\r", 1, hAliasOutFile);
            }
            FS_Write(lineStart, (int)(ptr - lineStart), hAliasOutFile);
            continue;
        }

        memset(bFieldSet, 0, sizeof(bFieldSet));
        Com_SoundAliasInitBuildDefaults(&alias, sourceFile);

        for (col = 0; col < iColCount; ++col) {
            snd_alias_members_t field = columnFields[col];

            strcpy(szAliasTokens[field], token);
            if (*token) {
                Com_LoadSoundAliasField("menu", loadspecCurGame, sourceFile, token, field, bFieldSet, &alias);
            }

            if (col + 1 == iColCount) {
                break;
            }

            token = Com_ParseOnLine(&ptr);
        }

        if (!bFieldSet[SA_NAME] || !bFieldSet[SA_FILE]) {
            Com_Error(1, "\x15Sound alias file %s: alias entry missing name and/or file\n", sourceFile);
        }

        if (!bFieldSet[SA_SUBTITLE]) {
            Com_SkipRestOfLine(&ptr);
            FS_Write(lineStart, (int)(ptr - lineStart), hAliasOutFile);
            continue;
        }

        if (Com_SoundAliasSubtitleIsReference(szAliasTokens[SA_SUBTITLE]) &&
            Com_SoundAliasSubtitleReferenceExists(szAliasTokens[SA_SUBTITLE])) {
            Com_SkipRestOfLine(&ptr);
            FS_Write(lineStart, (int)(ptr - lineStart), hAliasOutFile);
            continue;
        }

        {
            const char *stringEdReference = Com_GetSubtitleStringEdReference(szAliasTokens[SA_SUBTITLE]);

            if (stringEdReference) {
                Com_sprintf(szNewReference, sizeof(szNewReference), "%s%s", "SUBTITLE_", stringEdReference);
            } else if (bFieldSet[SA_SEQUENCE]) {
                Com_sprintf(szNewReference, sizeof(szNewReference), "%s%s_%s",
                            "SUBTITLE_", szAliasTokens[SA_NAME], szAliasTokens[SA_SEQUENCE]);
            } else {
                Com_sprintf(szNewReference, sizeof(szNewReference), "%s%s",
                            "SUBTITLE_", szAliasTokens[SA_NAME]);
            }
        }

        I_strupr(szNewReference);
        Com_SoundAliasWriteStringEdFile(szAliasTokens[SA_SUBTITLE], szNewReference);
        ++iNumSubtitlesLocalized;

        for (col = 0; col < iColCount; ++col) {
            snd_alias_members_t field = columnFields[col];
            qboolean lastColumn = (col == iColCount - 1);

            if (!field) {
                if (!lastColumn) {
                    FS_Write(",", 1, hAliasOutFile);
                }
                continue;
            }

            if (field == SA_SUBTITLE) {
                FS_Write(szNewReference, strlen(szNewReference), hAliasOutFile);
            } else {
                Com_SoundAliasWriteCsvToken(hAliasOutFile, szAliasTokens[field], lastColumn);
            }
        }

        FS_Write("\r\n", 2, hAliasOutFile);
        Com_SkipRestOfLine(&ptr);
    }

    Com_EndParseSession();
    FS_FCloseFile(hAliasOutFile);

    FS_BuildOSPath(Com_ImportedDvarString(imp_fs_basepath), (const char *)imp_fs_gamedir, "soundaliases/temp.csv", szFromFile);
    FS_BuildOSPath(Com_ImportedDvarString(imp_fs_basepath), (const char *)imp_fs_gamedir, soundAliasFile, szToFile);
    if (iNumSubtitlesLocalized) {
        FS_CopyFile(szFromFile, szToFile);
    }
    FS_Remove(szFromFile);

    Com_Printf("Localized %i sound alias subtitles\n", iNumSubtitlesLocalized);
}

#    if 0

__attribute__((naked))
void Com_WriteLocalizedSoundAliasFiles(void)
{
    __asm__ __volatile__ (
        "pushl %ebp\n"
        "movl %esp, %ebp\n"
        "pushl %edi\n"
        "pushl %esi\n"
        "pushl %ebx\n"
        "subl $0x24c, %esp\n"

        "leal -0x11c(%ebp), %ebx\n"
        "movl %ebx, 0xc(%esp)\n"
        "movl $"", 8(%esp)\n"
        "movl $"../source_data/string_resources/subtitle.st", 4(%esp)\n"
        "movl imp_fs_homepath, %eax\n"
        "movl (%eax), %eax\n"
        "movl 8(%eax), %eax\n"
        "movl %eax, (%esp)\n"
        "calll FS_BuildOSPath\n"
        "cld\n"
        "movl $0xffffffff, %ecx\n"
        "xorl %eax, %eax\n"
        "movl %ebx, %edi\n"
        "repne scasb %es:(%edi), %al\n"
        "notl %ecx\n"
        "movb $0, -0x11e(%ecx, %ebp)\n"
        "movl $"r+", 4(%esp)\n"
        "movl %ebx, (%esp)\n"
        "calll FS_FileOpen\n"
        "testl %eax, %eax\n"
        "je .Lfb9a60_000b9d2f\n"
        "movl %eax, (%esp)\n"
        "calll FS_FileClose\n"
        "leal -0x21c(%ebp), %eax\n"
        "movl %eax, 0xc(%esp)\n"
        "movl $"soundaliases/subtitle.st", 8(%esp)\n"
        "movl imp_fs_gamedir, %eax\n"
        "movl %eax, 4(%esp)\n"
        "movl imp_fs_basepath, %eax\n"
        "movl (%eax), %eax\n"
        "movl 8(%eax), %eax\n"
        "movl %eax, (%esp)\n"
        "calll FS_BuildOSPath\n"
        "leal -0x21c(%ebp), %eax\n"
        "movl %eax, 4(%esp)\n"
        "movl %ebx, (%esp)\n"
        "calll FS_CopyFile\n"
        "movl $"soundaliases/subtitle.st", (%esp)\n"
        "calll FS_FileExists\n"
        "testl %eax, %eax\n"
        "je .Lfb9a60_000b9b84\n"
        "movl $"Localizing sound alias subtitle text...\n", (%esp)\n"
        "calll Com_Printf\n"
        "movl %ebx, 4(%esp)\n"
        "movl $"Writing to StringEd file %s\n", (%esp)\n" /* "Writing to StringEd file %s
" */
        "calll Com_Printf\n"
        "movl $0xa, 0x10(%esp)\n"
        "leal -0x1c(%ebp), %eax\n"
        "movl %eax, 0xc(%esp)\n"
        "movl $0, 8(%esp)\n"
        "movl $"csv", 4(%esp)\n"
        "movl $"soundaliases", (%esp)\n"
        "calll FS_ListFiles\n"
        "movl %eax, %esi\n"
        "movl -0x1c(%ebp), %edi\n"
        "testl %edi, %edi\n"
        "jne .Lfb9a60_000b9ba3\n"
        "movl $"WARNING: can't find any sound alias files (soundaliases/*.csv)\n", (%esp)\n"
        "calll Com_Printf\n"

        "addl $0x24c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"

        ".Lfb9a60_000b9b84:\n"
        "movl $"soundaliases/subtitle.st", 4(%esp)\n"
        "movl $"WARNING: Could not make local copy of StringEd file %s\n", (%esp)\n" /* "WARNING: Could not make local copy of StringEd file %s
" */
        "calll Com_Printf\n"

        "addl $0x24c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"

        ".Lfb9a60_000b9ba3:\n"
        "calll Hunk_HideTempMemory\n"
        "movl %eax, -0x22c(%ebp)\n"
        "movl -0x1c(%ebp), %ecx\n"
        "testl %ecx, %ecx\n"
        "jg .Lfb9a60_000b9cf8\n"
        ".Lfb9a60_000b9bb9:\n"
        "movl %eax, (%esp)\n"
        "calll Hunk_ShowTempMemory\n"
        "movl $0xa, 4(%esp)\n"
        "movl %esi, (%esp)\n"
        "calll FS_FreeFileList\n"

        "movl $"rb", 4(%esp)\n"
        "leal -0x21c(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll FS_FileOpen\n"
        "movl %eax, %edi\n"
        "testl %eax, %eax\n"
        "je .Lfb9a60_000b9cd3\n"
        "movl $2, 8(%esp)\n"
        "movl $0, 4(%esp)\n"
        "movl %eax, (%esp)\n"
        "calll FS_FileSeek\n"
        "movl %edi, (%esp)\n"
        "calll ftell\n"
        "movl %eax, %esi\n"
        "movl $0, 8(%esp)\n"
        "movl $0, 4(%esp)\n"
        "movl %edi, (%esp)\n"
        "calll FS_FileSeek\n"
        "movl %esi, (%esp)\n"
        "calll malloc\n"
        "movl %eax, -0x230(%ebp)\n"
        "movl %edi, 0xc(%esp)\n"
        "movl %esi, 8(%esp)\n"
        "movl $1, 4(%esp)\n"
        "movl %eax, (%esp)\n"
        "calll FS_FileRead\n"
        "cmpl %eax, %esi\n"
        "je .Lfb9a60_000b9c69\n"
        "movl $"\x15Short read in COM_WriteFinalStringEdFile()\n", 4(%esp)\n"
        "movl $0, (%esp)\n"
        "calll Com_Error\n"
        ".Lfb9a60_000b9c69:\n"
        "movl %edi, (%esp)\n"
        "calll FS_FileClose\n"
        "movl $"wb", 4(%esp)\n"
        "movl %ebx, (%esp)\n"
        "calll FS_FileOpen\n"
        "movl %eax, %ebx\n"
        "testl %eax, %eax\n"
        "je .Lfb9a60_000b9cc5\n"
        "movl %eax, 0xc(%esp)\n"
        "movl %esi, 8(%esp)\n"
        "movl $1, 4(%esp)\n"
        "movl -0x230(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll FS_FileWrite\n"
        "cmpl %eax, %esi\n"
        "je .Lfb9a60_000b9cbd\n"
        "movl $"\x15Short write in COM_WriteFinalStringEdFile()\n", 4(%esp)\n"
        "movl $0, (%esp)\n"
        "calll Com_Error\n"
        ".Lfb9a60_000b9cbd:\n"
        "movl %ebx, (%esp)\n"
        "calll FS_FileClose\n"
        ".Lfb9a60_000b9cc5:\n"
        "movl -0x230(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll free\n"

        ".Lfb9a60_000b9cd3:\n"
        "leal -0x21c(%ebp), %eax\n"
        "movl %eax, (%esp)\n"
        "calll FS_Remove\n"
        "movl $"done\n", (%esp)\n"
        "calll Com_Printf\n"

        "addl $0x24c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"

        ".Lfb9a60_000b9cf8:\n"
        "xorl %edi, %edi\n"
        ".Lfb9a60_000b9cfa:\n"
        "leal -0x21c(%ebp), %eax\n"
        "movl %eax, 8(%esp)\n"
        "movl $"all_mp", 4(%esp)\n"
        "movl (%esi, %edi, 4), %eax\n"
        "movl %eax, (%esp)\n"
        "calll Com_ProcessSoundAliasFileLocalization\n"
        "calll Hunk_ClearTempMemory\n"
        "addl $1, %edi\n"
        "cmpl -0x1c(%ebp), %edi\n"
        "jl .Lfb9a60_000b9cfa\n"
        "movl -0x22c(%ebp), %eax\n"
        "jmp .Lfb9a60_000b9bb9\n"
        ".Lfb9a60_000b9d2f:\n"
        "movl %ebx, 4(%esp)\n"
        "movl $"WARNING: Can not write to StringEd file %s\n", (%esp)\n" /* "WARNING: Can not write to StringEd file %s
" */
        "calll Com_Printf\n"

        "addl $0x24c, %esp\n"
        "popl %ebx\n"
        "popl %esi\n"
        "popl %edi\n"
        "popl %ebp\n"
        "retl\n"
    );
}
#    endif

void Com_WriteLocalizedSoundAliasFiles(void)
{
    char stringEdExternalFileName[256];
    char stringEdFileName[256];
    char **fileNames;
    int fileCount;
    int mark;
    int i;
    void *file;

    FS_BuildOSPath(Com_ImportedDvarString(imp_fs_homepath), "../source_data/string_resources/subtitle.st", "", stringEdExternalFileName);
    stringEdExternalFileName[strlen(stringEdExternalFileName) - 1] = '\0';

    file = FS_FileOpen(stringEdExternalFileName, "r+");
    if (!file) {
        Com_Printf("WARNING: Can not write to StringEd file %s\n", stringEdExternalFileName);
        return;
    }

    FS_FileClose(file);

    FS_BuildOSPath(Com_ImportedDvarString(imp_fs_basepath), (const char *)imp_fs_gamedir, "soundaliases/subtitle.st", stringEdFileName);
    FS_CopyFile(stringEdExternalFileName, stringEdFileName);

    if (!FS_FileExists("soundaliases/subtitle.st")) {
        Com_Printf("WARNING: Could not make local copy of StringEd file %s\n", "soundaliases/subtitle.st");
        return;
    }

    Com_Printf("Localizing sound alias subtitle text...\n");
    Com_Printf("Writing to StringEd file %s\n", stringEdExternalFileName);

    fileNames = FS_ListFiles("soundaliases", "csv", 0, &fileCount, 10);
    if (!fileCount) {
        Com_Printf("WARNING: can't find any sound alias files (soundaliases/*.csv)\n");
        return;
    }

    mark = Hunk_HideTempMemory();
    for (i = 0; i < fileCount; ++i) {
        Com_ProcessSoundAliasFileLocalization(fileNames[i], "all_mp", stringEdFileName);
        Hunk_ClearTempMemory();
    }

    Hunk_ShowTempMemory(mark);
    FS_FreeFileList(fileNames, 10);
    Com_CopyFinalStringEdFile(stringEdFileName, stringEdExternalFileName);
}

void Com_LoadSoundAliasFile(const char *loadspec, const char *loadspecCurGame, const char *sourceFile)
{
    char filename[64];
    void *file;
    const char *ptr;
    const char *token;
    snd_alias_members_t columnTypes[256];
    int iColCount;
    char bFieldSet[SA_NUMFIELDS];
    snd_alias_build_t alias;
    snd_alias_build_t *aliasCopy;
    int bHasName;
    int bHasFile;
    int i;
    int field;
    float temp;

    Com_sprintf(filename, sizeof(filename), "soundaliases/%s", sourceFile);

    if (FS_ReadFile(filename, &file) < 0) {
        return;
    }

    if (!saLoadObjGlob.volumeModGroupsInitialized) {
        char volumeModFileName[64];
        char buffer[0x2000];
        fileHandle_t fileHandle;
        int fileLength;
        int volumeModGroupIndex;
        const char *parseBuffer;
        char *groupName;

        if (!saLoadObjGlob.refreshVolumeModGroupsCommandInitialized) {
            Cmd_AddCommand("snd_refreshVolumeModGroups", Com_RefreshVolumeModGroups_f);
            saLoadObjGlob.refreshVolumeModGroupsCommandInitialized = 1;
        }

        strcpy(volumeModFileName, "soundaliases/volumemodgroups.def");

        fileLength = FS_FOpenFileRead(volumeModFileName, &fileHandle, 1);
        if (fileLength < 0) {
            Com_Error(1, "ERROR: Could not find '%s'\n", volumeModFileName);
        } else if (fileLength == 0) {
            FS_FCloseFile(fileHandle);
            Com_Error(1, "ERROR: '%s' is empty\n", volumeModFileName);
        } else {
            FS_Read(buffer, 15, fileHandle);
            buffer[15] = 0;
            if (strcmp(buffer, "VOLUMEMODGROUPS")) {
                FS_FCloseFile(fileHandle);
                Com_Error(1, "ERROR: \"%s\" does not appear to be a volumemodgroups file\n", volumeModFileName);
            }

            fileLength -= 15;
            if (fileLength > 0x1fff) {
                FS_FCloseFile(fileHandle);
                Com_Error(1, "ERROR: \"%s\" Is too long of a volumemodgroups file to parse\n", volumeModFileName);
            }

            memset(buffer, 0, sizeof(buffer));
            FS_Read(buffer, fileLength, fileHandle);
            buffer[fileLength] = 0;
            FS_FCloseFile(fileHandle);

            Com_BeginParseSession(volumeModFileName);
            parseBuffer = buffer;
            volumeModGroupIndex = 0;
            for (;;) {
                token = Com_Parse(&parseBuffer);
                if (!token[0] || token[0] == '}') {
                    Com_EndParseSession();
                    break;
                }

                if (volumeModGroupIndex == 32) {
                    Com_EndParseSession();
                    Com_Error(1, "ERROR: volumemodgroups parse failure on file \"%s\": groups parsed (%d) is greater than or equal to maxGroups(%d)\n", volumeModFileName, 32, 32);
                    break;
                }

                groupName = saLoadObjGlob.volumeModGroups[volumeModGroupIndex].name;
                strcpy(groupName, token);

                token = Com_Parse(&parseBuffer);
                if (!token[0] || token[0] == '}') {
                    Com_EndParseSession();
                    Com_Error(1, "ERROR: volumemodgroups parse failure on file \"%s\": groupname '%s' missing a matching value\n", volumeModFileName, groupName);
                    break;
                }

                saLoadObjGlob.volumeModGroups[volumeModGroupIndex].value = (float)atof(token);
                ++volumeModGroupIndex;
            }
        }

        saLoadObjGlob.volumeModGroupsInitialized = 1;
    }

    Com_BeginParseSession(filename);
    Com_SetCSV(1);

    ptr = (const char *)file;
    iColCount = 0;

    for (;;) {
        token = Com_Parse(&ptr);
        if (!ptr) {
            Com_EndParseSession();
            return;
        }

        if (!token[0] || token[0] == '#') {
            Com_SkipRestOfLine(&ptr);
            continue;
        }

        if (!iColCount) {
            bHasName = 0;
            bHasFile = 0;
            i = 1;
            for (;;) {
                columnTypes[i - 1] = SA_INVALID;

                for (field = 1; field < SA_NUMFIELDS; ++field) {
                    if (!I_stricmp(g_pszSndAliasKeyNames[field], token)) {
                        columnTypes[i - 1] = (snd_alias_members_t)field;
                        if (field == SA_NAME) {
                            bHasName = 1;
                        } else if (field == SA_FILE) {
                            bHasFile = 1;
                        }
                        break;
                    }
                }

                iColCount = i;
                if (i == 256 || !ptr || *ptr == '\n') {
                    break;
                }

                ++i;
                token = Com_ParseOnLine(&ptr);
            }

            if (!bHasName || !bHasFile) {
                Com_Printf("^1ERROR: Sound alias file %s: missing 'name' and/or 'file' columns\n", sourceFile);
                Com_EndParseSession();
                return;
            }

            Com_SkipRestOfLine(&ptr);
            continue;
        }

        memset(bFieldSet, 0, sizeof(bFieldSet));

        strcpy(alias.szSourceFile, sourceFile);
        alias.szAliasName[0] = 0;
        alias.szSecondaryAliasName[0] = 0;
        alias.subtitleText = NULL;
        alias.iSequence = 0;
        alias.szSoundFile[0] = 0;
        alias.permSoundFile = NULL;
        alias.fVolMin = 1.0f;
        alias.fVolMax = 1.0f;
        alias.fVolMod = 1.0f;
        alias.fPitchMin = 1.0f;
        alias.fPitchMax = 1.0f;
        alias.fDistMin = 120.0f;
        alias.fDistMax = 0.0f;
        alias.iChannel = 0;
        alias.eType = SAT_LOADED;
        alias.volumeFalloffCurve = Com_GetDefaultSoundAliasVolumeFalloffCurve();
        alias.fSlavePercentage = 1.0f;
        alias.fProbability = 1.0f;
        alias.fLfePercentage = 0.0f;
        alias.startDelay = 0;
        alias.bLooping = 0;
        alias.bMaster = 0;
        alias.bSlave = 0;
        alias.bFullDryLevel = 0;
        alias.bNoWetLevel = 0;
        alias.error = 0;
        alias.keep = strcmp(loadspec, "menu") != 0;
        alias.pSameSoundFile = 0;
        alias.pNext = 0;

        for (i = 0; i < iColCount; ++i) {
            if (token[0]) {
                Com_LoadSoundAliasField(loadspec, loadspecCurGame, sourceFile, token, columnTypes[i], bFieldSet, &alias);
            }

            if (i + 1 == iColCount) {
                break;
            }

            token = Com_ParseOnLine(&ptr);
        }

        if (!bFieldSet[SA_NAME] || !bFieldSet[SA_FILE]) {
            Com_Printf("^1ERROR: Sound alias file %s: alias entry missing name and/or file\n", sourceFile);
            Com_EndParseSession();
            return;
        }

        if (alias.keep && !alias.error) {
            if (alias.fPitchMin > alias.fPitchMax) {
                temp = alias.fPitchMin;
                alias.fPitchMin = alias.fPitchMax;
                alias.fPitchMax = temp;
            }

            if (alias.fPitchMin <= 0.0f) {
                Com_Printf("^1ERROR: sound alias '%s' has pitch_min %g <= 0\n", alias.szAliasName, alias.fPitchMin);
            } else {
                if (alias.fVolMin > alias.fVolMax) {
                    temp = alias.fVolMin;
                    alias.fVolMin = alias.fVolMax;
                    alias.fVolMax = temp;
                }

                if (alias.fVolMin < 0.0f) {
                    Com_Printf("^1ERROR: sound alias '%s' has vol_min %g < 0\n", alias.szAliasName, alias.fVolMin);
                } else {
                    if (alias.fDistMax == 0.0f) {
                        alias.fDistMax = alias.fDistMin * 5.0f;
                    }

                    if (alias.fDistMin > alias.fDistMax) {
                        Com_Printf("^1ERROR: sound alias '%s' has dist_min %g <= dist_max %g\n", alias.szAliasName, alias.fDistMin, alias.fDistMax);
                    } else if (alias.fDistMin <= 0.0f) {
                        Com_Printf("^1ERROR: sound alias '%s' has dist_min %g <= 0\n", alias.szAliasName, alias.fDistMin);
                    } else {
                        if (alias.fVolMod != 1.0f) {
                            alias.fVolMin *= alias.fVolMod;
                            if (alias.fVolMin < 0.0f) {
                                alias.fVolMin = 0.0f;
                            } else if (alias.fVolMin > 1.0f) {
                                alias.fVolMin = 1.0f;
                            }

                            alias.fVolMax *= alias.fVolMod;
                            if (alias.fVolMax < 0.0f) {
                                alias.fVolMax = 0.0f;
                            } else if (alias.fVolMax > 1.0f) {
                                alias.fVolMax = 1.0f;
                            }
                        }

                        aliasCopy = (snd_alias_build_t *)Com_AllocateTempSoundMemory(sizeof(alias), "Com_AddBuildSoundAlias");
                        memcpy(aliasCopy, &alias, sizeof(alias));
#    if defined(__x86_64__) || defined(_M_X64)
                        aliasCopy->pNext = (intptr_t)(uintptr_t)saLoadObjGlob.tempAliases;
#    else
                        aliasCopy->pNext = (int)(uintptr_t)saLoadObjGlob.tempAliases;
#    endif
                        saLoadObjGlob.tempAliases = aliasCopy;
                        ++saLoadObjGlob.tempAliasCount;
                    }
                }
            }
        }

        Com_SkipRestOfLine(&ptr);
    }
}

#else
static snd_alias_build_t *Com_SortTempSoundAliases_r(snd_alias_build_t *pAliasList, int *piAliasCount, int (*test)(snd_alias_build_t *, snd_alias_build_t *), int isRemovingDups)
{
    return Com_SortTempSoundAliases_r_impl(pAliasList, piAliasCount, test, isRemovingDups);
}

void Com_InitDefaultSoundAliasVolumeFalloffCurve(SndCurve *sndCurve)
{
    sndCurve->filename = "";
    sndCurve->knots[0][0] = 0.0f;
    sndCurve->knots[0][1] = 1.0f;
    sndCurve->knots[1][0] = 1.0f;
    sndCurve->knots[1][1] = 0.0f;
    sndCurve->knotCount = 2;
}

int Com_LoadSoundAliasSounds(SoundFileInfo *soundFileInfo)
{
    int soundIndex;
    int numMissing;

    if (soundFileInfo->count <= 0)
        return 0;

    numMissing = 0;
    for (soundIndex = 0; soundIndex < soundFileInfo->count; soundIndex++) {
        SoundFile *soundFile = &soundFileInfo->files[soundIndex];

        if (soundFile->type == 1) {
            SND_LoadSoundFile(soundFile);
            if (!soundFile->fileMem)
                numMissing++;
            continue;
        }

        if ((*(dvar_t **)imp_snd_touchStreamFilesOnLoad)->current.enabled ||
            (*(dvar_t **)imp_fs_copyfiles)->current.enabled) {
            soundFile->isStreamFound = FS_TouchFile(va("sound/%s", soundFile->soundName));
        } else {
            soundFile->isStreamFound = 1;
        }

        if (!soundFile->isStreamFound)
            Com_Printf("^1ERROR: Streamed sound file '%s' not found\n", soundFile->soundName);
    }

    return numMissing;
}
#endif
