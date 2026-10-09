#include "common_types.h"
#include "imports.h"

extern int stricmp(const char *s1, const char *s2);

extern void *Hunk_AllocateTempMemoryInternal(int size);
extern void *Hunk_AllocInternal(int size);
extern float GraphGetValueFromFraction(int knotCount, float *knots, float fraction);

extern struct g_sa_type g_sa;

COD2_ASSERT_FIELD(struct g_sa_type, aliasInfo, 4104);
COD2_ASSERT_FIELD(struct g_sa_type, soundFileInfo, 4152);
COD2_ASSERT_FIELD(struct g_sa_type, volumeFalloffCurves, 4244);
COD2_ASSERT_FIELD(struct snd_alias_list_t, aliasName, 0);
COD2_ASSERT_FIELD(struct snd_alias_list_t, head, 4);
COD2_ASSERT_FIELD(struct snd_alias_list_t, count, 8);
COD2_ASSERT_FIELD(struct snd_alias_list_t, pHashNext, 12);
COD2_ASSERT_FIELD(struct SoundFileInfo, count, 0);
COD2_ASSERT_FIELD(struct SoundFileInfo, files, 4);
COD2_ASSERT_SIZE(struct snd_alias_list_t, 16);
COD2_ASSERT_SIZE(struct SoundFileInfo, 8);
COD2_ASSERT_SIZE(struct SndCurve, 0x48);

extern int strnicmp(const char *s1, const char *s2, size_t n);
extern char *strcpy(char *dest, const char *src);
extern char *strlwr(char *s);
extern void Com_StripExtension(const char *in, char *out);
extern char **FS_ListFiles(const char *path, const char *extension, int behavior, int *numfiles, int allocTrackType);
extern void FS_FreeFileList(char **list, int behavior);
extern void *Hunk_HideTempMemory(void);
extern void Hunk_ShowTempMemory(void *mark);
extern void Hunk_ClearTempMemory(void);
extern void Com_InitSoundAlias(void);
extern void Com_MakeSoundAliasesPermanent(void *aliases, void *counts);
extern void Com_LoadSoundAliasFile(const char *trimspec, const char *loadspecCurGame, const char *fileName);
extern int Com_LoadSoundAliasSounds(void *counts);
extern void Com_InitDefaultSoundAliasVolumeFalloffCurve(void *curve);
extern Bool Com_LoadVolumeFalloffCurve(const char *name, void *curve);
extern void Cmd_AddCommand(const char *cmdName, void (*function)(void));
extern void Cmd_RemoveCommand(const char *cmdName);
extern void SND_StopSounds(int which);
extern int SND_GetSoundFileSize(const void *pSoundFile);
extern void Com_Printf(const char *fmt, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int I_stricmp(const char *s0, const char *s1);
extern int ___tolower(int c);

float Com_GetVolumeFalloffCurveValue(SndCurve *volumeFalloffCurve, float fraction);
const char *Com_GetSoundFileName(const snd_alias_t *pAlias);
void *Com_GetSoundFileMem(const snd_alias_t *pAlias);
void Com_LoadSoundAliases(const char *loadspec, const char *loadspecCurGame, snd_alias_system_t system);
SndCurve *Com_RegisterSoundAliasVolumeFalloffCurve(const char *filename, const char *sourceFile);
SndCurve *Com_GetDefaultSoundAliasVolumeFalloffCurve(void);
void *Com_AllocateTempSoundMemory(int size, const char *name);
void *Com_AllocSoundMemory(int size, const char *name, int type);
void Com_UnloadSoundAliasSounds(snd_alias_system_t system);
void Com_UnloadSoundAliases(snd_alias_system_t system);
static void Com_StreamedSoundList(snd_alias_system_t system);
static void Com_LoadedSoundList(snd_alias_system_t system);
static void Com_SoundList_f(void);
snd_alias_t *Com_PickSoundAliasFromList(snd_alias_list_t *aliasList);
snd_alias_list_t *Com_FindSoundAlias(const char *name);
Bool Com_AddAliasList(const char *name, snd_alias_list_t *aliasList);
void Com_DuplicateSoundAlias(snd_alias_list_t *aliasCopy, const char *name);
int SND_GetAliasOffset(const snd_alias_t *alias);
snd_alias_t *SND_GetAliasWithOffset(const char *name, int offset);
snd_alias_t *Com_PickSoundAlias(const char *aliasname);

static inline __attribute__((always_inline)) unsigned int Com_SoundAliasHash(const char *name)
{
    unsigned int hash = 0;

    while (*name) {
        hash = hash * 31337u + (unsigned int)___tolower((signed char)*name);
        name++;
    }

    return hash & 0x3ffu;
}

static inline __attribute__((always_inline)) int Com_SoundAliasRandom(void)
{
    int seed = g_sa.randSeed * 214013 + 2531011;

    g_sa.randSeed = seed;

    return (seed >> 16) & 0x7fff;
}

static inline __attribute__((always_inline)) Bool Com_SoundAliasShouldPick(float probability, float totalProbability)
{
    return probability * 32768.0f > (float)Com_SoundAliasRandom() * totalProbability;
}

static inline __attribute__((always_inline)) snd_alias_type_t Com_SoundAliasType(const snd_alias_t *alias)
{
    return (snd_alias_type_t)((alias->flags & 0x60) >> 5);
}

static inline __attribute__((always_inline)) Bool Com_SoundAliasFileWasSeen(const snd_alias_t *aliases, int count, const snd_alias_t *alias, snd_alias_type_t type)
{
    int index;

    for (index = 0; index < count; index++) {
        const snd_alias_t *prevAlias = &aliases[index];

        if (Com_SoundAliasType(prevAlias) == type && prevAlias->soundFile->soundName == alias->soundFile->soundName) {
            return 1;
        }
    }

    return 0;
}

float Com_GetVolumeFalloffCurveValue(SndCurve *volumeFalloffCurve, float fraction)
{
    return GraphGetValueFromFraction(volumeFalloffCurve->knotCount, (float *)volumeFalloffCurve->knots, fraction);
}

const char *Com_GetSoundFileName(const snd_alias_t *pAlias)
{
    return pAlias->soundFile->soundName;
}

void *Com_GetSoundFileMem(const snd_alias_t *pAlias)
{
    return pAlias->soundFile->fileMem;
}

void Com_LoadSoundAliases(const char *loadspec, const char *loadspecCurGame, snd_alias_system_t system)
{
    char trimspec[0x44];
    int fileCount;
    char **fileNames;
    void *mark;
    int i;
    char **vfcurveFiles;
    int vfcurveCount;

    if (!g_sa.curvesInitialized) {

        memset(g_sa.volumeFalloffCurves, 0, sizeof(g_sa.volumeFalloffCurves));

        Com_InitDefaultSoundAliasVolumeFalloffCurve(g_sa.volumeFalloffCurves);

        vfcurveFiles = FS_ListFiles("soundaliases", "vfcurve", 0, &vfcurveCount, 0xa);

        if (vfcurveCount > 15) {
            Com_Error(1, "Snd_Alias Curve initialization: '.vfcurve' file count (%d) ", vfcurveCount, 15);
        }

        {
            int ci = 0;
            void *curvePtr = g_sa.volumeFalloffCurves;
            char *namePtr = g_sa.volumeFalloffCurveNames[0];
            char **fptr = vfcurveFiles;

            while (ci < vfcurveCount) {

                ci++;
                namePtr += 0x40;
                fptr++;
#if defined(__x86_64__) || defined(_M_X64)

                curvePtr = (void *)((byte *)curvePtr + sizeof(SndCurve));
#else
                curvePtr = (void *)((byte *)curvePtr + 0x48);
#endif

                {
                    const char *fn = *(fptr - 1);
                    int fnLen = strlen(fn) - 8;
                    if (fnLen < 0)
                        fnLen = strlen(fn);
                    if (fnLen >= 0x40)
                        fnLen = 0x3f;
                    I_strncpyz(namePtr, fn, fnLen + 1);
                }

                if (!Com_LoadVolumeFalloffCurve(namePtr, curvePtr)) {

                    Com_Error(0, "Failed to load sndcurve file '%s'", *(fptr - 1));
                }
            }
        }

        FS_FreeFileList(vfcurveFiles, 0xa);

        g_sa.curvesInitialized = 1;
    }

    if (strnicmp(loadspec, "maps/mp/", 8) == 0) {

        Com_StripExtension(loadspec + 8, trimspec);
    } else if (strnicmp(loadspec, "maps/", 5) == 0) {

        Com_StripExtension(loadspec + 5, trimspec);
    } else {

        strcpy(trimspec, loadspec);
    }

    strlwr(trimspec);

    if (system == SASYS_UI && g_sa.initialized[SASYS_UI]) {
        memset(g_sa.pHash, 0, sizeof(g_sa.pHash));
        memset(&g_sa.aliasInfo[SASYS_UI], 0, sizeof(g_sa.aliasInfo[SASYS_UI]));
        memset(&g_sa.soundFileInfo[SASYS_UI], 0, sizeof(g_sa.soundFileInfo[SASYS_UI]));
        g_sa.initialized[SASYS_UI] = 0;
    }

    if (system == 1) {
        const dvar_t *sv_running = *(const dvar_t **)imp_com_sv_running;
        if (sv_running->current.enabled != 0) {

            g_sa.aliasInfo[1].aliasName = g_sa.aliasInfo[2].aliasName;
            g_sa.aliasInfo[1].head = g_sa.aliasInfo[2].head;
            g_sa.aliasInfo[1].count = g_sa.aliasInfo[2].count;
            g_sa.aliasInfo[1].pHashNext = g_sa.aliasInfo[2].pHashNext;

            g_sa.soundFileInfo[1].count = g_sa.soundFileInfo[2].count;
            g_sa.soundFileInfo[1].files = g_sa.soundFileInfo[2].files;

            goto after_load;
        }

        if (g_sa.initialized[SASYS_UI]) {
            memset(g_sa.pHash, 0, sizeof(g_sa.pHash));
            memset(&g_sa.aliasInfo[SASYS_UI], 0, sizeof(g_sa.aliasInfo[SASYS_UI]));
            memset(&g_sa.soundFileInfo[SASYS_UI], 0, sizeof(g_sa.soundFileInfo[SASYS_UI]));
            g_sa.initialized[SASYS_UI] = 0;
        }
    }

    fileNames = FS_ListFiles("soundaliases", "csv", 0, &fileCount, 0xa);

    if (fileCount == 0) {

        Com_Printf((const char *)"WARNING: can't find any sound alias files (soundaliases/*.csv)\n");
        goto done;
    }

    mark = Hunk_HideTempMemory();

    Com_InitSoundAlias();

    for (i = 0; i < fileCount; i++) {

        Com_LoadSoundAliasFile(trimspec, loadspecCurGame, fileNames[i]);
    }

    Com_MakeSoundAliasesPermanent(
#if defined(__x86_64__) || defined(_M_X64)
        (void *)&g_sa.aliasInfo[system],
        (void *)&g_sa.soundFileInfo[system]);
#else
        (void *)((byte *)&g_sa + 4104 + (int)system * 16),
        (void *)((byte *)&g_sa + 4152 + (int)system * 8));
#endif

    Hunk_ClearTempMemory();

    Hunk_ShowTempMemory(mark);

    FS_FreeFileList(fileNames, 0xa);

after_load:

    if ((int)system <= 1) {
#if defined(__x86_64__) || defined(_M_X64)
        if (g_sa.initialized[0] == 0 && g_sa.initialized[1] == 0) {
#else
        if (*(short *)((byte *)&g_sa) == 0) {
#endif

            Cmd_AddCommand("snd_list", (void (*)(void))Com_SoundList_f);
        }
    }

    g_sa.initialized[system] = 1;

    if ((int)system <= 1) {

#if defined(__x86_64__) || defined(_M_X64)
        int missCount = Com_LoadSoundAliasSounds((void *)&g_sa.soundFileInfo[system]);
#else
        int missCount = Com_LoadSoundAliasSounds((void *)((byte *)&g_sa + 4152 + (int)system * 8));
#endif

        if (missCount != 0) {
            const dvar_t *snd_errorOnMissing_dvar = *(const dvar_t **)imp_snd_errorOnMissing;
            if (snd_errorOnMissing_dvar->current.enabled != 0) {

                int errCode = (system != 0) ? 1 : 0;
                {
                    extern const char *va(const char *fmt, ...);
                    Com_Error(errCode, "%s", va("%i sound file(s) are missing or in a bad format\n", missCount));
                }
            }
        }
    }

done:
    return;
}

SndCurve *Com_RegisterSoundAliasVolumeFalloffCurve(const char *filename, const char *sourceFile)
{
    int i;

    for (i = 0; i < 16; i++) {
        if (g_sa.volumeFalloffCurves[i].filename && stricmp(filename, g_sa.volumeFalloffCurves[i].filename) == 0)
            return &g_sa.volumeFalloffCurves[i];
    }

    Com_Error(1, "\x15Sound alias file %s: Volume Falloff Curve '%s' not found.", sourceFile, filename);
    return NULL;
}

SndCurve *Com_GetDefaultSoundAliasVolumeFalloffCurve(void)
{
    return &g_sa.volumeFalloffCurves[0];
}

void *Com_AllocateTempSoundMemory(int size, const char *name)
{
    (void)name;
    return Hunk_AllocateTempMemoryInternal(size);
}

void *Com_AllocSoundMemory(int size, const char *name, int type)
{
    return (void *)Hunk_AllocInternal(size);
}

void Com_UnloadSoundAliasSounds(snd_alias_system_t system)
{
#ifdef __EMSCRIPTEN__
    extern void WebAudio_ClearCache(void);
    WebAudio_ClearCache();
#endif
    snd_alias_t *head;
    int count;
    int index;

    SND_StopSounds(0);

    head = g_sa.aliasInfo[system].head;
    count = g_sa.aliasInfo[system].count;

    for (index = 0; index < count; index++) {
        snd_alias_t *alias = &head[index];

        if (Com_SoundAliasType(alias) != SAT_LOADED) {
            continue;
        }

        if (index != 0) {
            const char *soundName = alias->soundFile->soundName;
            int j;

            for (j = 0; j < index; j++) {
                snd_alias_t *prev = &head[j];

                if (prev->soundFile->soundName == soundName && Com_SoundAliasType(prev) == SAT_LOADED) {
                    break;
                }
            }
        }

        alias->soundFile->fileMem = NULL;
    }
}

void Com_UnloadSoundAliases(snd_alias_system_t system)
{
    if (!g_sa.initialized[system]) {
        return;
    }

    if (system != SASYS_GAME) {
        Com_UnloadSoundAliasSounds(system);
    }

    if (g_sa.aliasInfo[system].head) {
        g_sa.aliasInfo[system].head = NULL;
        g_sa.aliasInfo[system].count = 0;
        memset(g_sa.pHash, 0, sizeof(g_sa.pHash));
    }

    g_sa.initialized[system] = 0;

    if (system <= SASYS_CGAME && !g_sa.initialized[SASYS_UI] && !g_sa.initialized[SASYS_CGAME]) {
        Cmd_RemoveCommand("snd_list");
    }
}

static void Com_StreamedSoundList(snd_alias_system_t system)
{
    snd_alias_t *head;
    snd_alias_t *alias;
    int index;

    if (!g_sa.initialized[system]) {
        return;
    }

    head = g_sa.aliasInfo[system].head;

    if (g_sa.aliasInfo[system].count <= 0) {
        return;
    }

    index = 0;
    alias = head;
    do {
        if (Com_SoundAliasType(alias) == SAT_STREAMED && !Com_SoundAliasFileWasSeen(head, index, alias, SAT_STREAMED)) {
            if (alias->soundFile->isStreamFound) {
                Com_Printf("%-64s\n", alias->soundFile->soundName);
            } else {
                Com_Printf("%-64s FILE NOT FOUND\n", alias->soundFile->soundName);
            }
        }

        index++;
        alias++;
    } while (index < g_sa.aliasInfo[system].count);
}

static void Com_LoadedSoundList(snd_alias_system_t system)
{
    snd_alias_t *head;
    snd_alias_t *alias;
    int totalMem = 0;
    int index;

    if (!g_sa.initialized[system]) {
        return;
    }

    head = g_sa.aliasInfo[system].head;

    if (g_sa.aliasInfo[system].count > 0) {
        index = 0;
        alias = head;
        do {
            if (Com_SoundAliasType(alias) == SAT_LOADED && !Com_SoundAliasFileWasSeen(head, index, alias, SAT_LOADED)) {
                SoundFile *soundFile = alias->soundFile;

                if (soundFile->fileMem) {
                    int soundFileSize = SND_GetSoundFileSize(soundFile->fileMem);

                    totalMem += soundFileSize;
                    Com_Printf("%-64s %7.1f KB\n", alias->soundFile->soundName,
                               (float)soundFileSize * 0.0009765625f);
                } else {
                    Com_Printf("%-64s FAILED TO LOAD\n", soundFile->soundName);
                }
            }

            index++;
            alias++;
        } while (index < g_sa.aliasInfo[system].count);
    }

    Com_Printf("\ntotal usage %7.3f MB\n", (float)totalMem * 0.00000095367431640625f);
}

static void Com_SoundList_f(void)
{
    Com_Printf("\n________________________________________\ncurrently streamed menu sounds:\n");
    Com_StreamedSoundList(SASYS_UI);
    Com_Printf("\n________________________________________\ncurrently streamed in-game sounds:\n");
    Com_StreamedSoundList(SASYS_CGAME);
    Com_Printf("________________________________________\ncurrently loaded menu sounds:\n");
    Com_LoadedSoundList(SASYS_UI);
    Com_Printf("\n________________________________________\ncurrently loaded in-game sounds:\n");
    Com_LoadedSoundList(SASYS_CGAME);
    Com_Printf("\n");
}

static inline __attribute__((always_inline)) snd_alias_t *Com_PickSoundAliasFromList_core(snd_alias_list_t *aliasList)
{
    snd_alias_t *firstAlias;
    snd_alias_t *bestAlias;
    snd_alias_t *alias;
    float totalProbability;
    int maxSequence;
    int index;

    if (!aliasList) {
        return NULL;
    }

    firstAlias = aliasList->head;
    bestAlias = firstAlias;
    totalProbability = firstAlias->fProbability;
    maxSequence = firstAlias->iSequence;

    for (index = 1; index < aliasList->count; index++) {
        alias = &firstAlias[index];
        totalProbability += alias->fProbability;

        if (Com_SoundAliasShouldPick(alias->fProbability, totalProbability)) {
            bestAlias = alias;
        }

        if (alias->iSequence > maxSequence) {
            maxSequence = alias->iSequence;
        }
    }

    if (aliasList->count > 2 && bestAlias->iSequence == maxSequence) {
        int skipSequence = bestAlias->iSequence;

        totalProbability = 0.0f;
        for (index = 0; index < aliasList->count; index++) {
            alias = &firstAlias[index];

            if (alias->iSequence == skipSequence) {
                continue;
            }

            totalProbability += alias->fProbability;
            if (Com_SoundAliasShouldPick(alias->fProbability, totalProbability)) {
                bestAlias = alias;
            }
        }
    }

    bestAlias->iSequence = maxSequence + 1;
    return bestAlias;
}

snd_alias_t *Com_PickSoundAliasFromList(snd_alias_list_t *aliasList)
{
    return Com_PickSoundAliasFromList_core(aliasList);
}

static inline __attribute__((always_inline)) snd_alias_list_t *Com_FindSoundAlias_core(const char *name)
{
    snd_alias_list_t *aliasList;

    if (!name) {
        return NULL;
    }

    for (aliasList = g_sa.pHash[Com_SoundAliasHash(name)]; aliasList;
         aliasList = (snd_alias_list_t *)(uintptr_t)aliasList->pHashNext) {
        if (I_stricmp(name, aliasList->aliasName) == 0) {
            return aliasList;
        }
    }

    return NULL;
}

snd_alias_list_t *Com_FindSoundAlias(const char *name)
{
    return Com_FindSoundAlias_core(name);
}

Bool Com_AddAliasList(const char *name, snd_alias_list_t *aliasList)
{
    unsigned int hash = Com_SoundAliasHash(name);
    snd_alias_list_t *aliasListIter;

    for (aliasListIter = g_sa.pHash[hash]; aliasListIter;
         aliasListIter = (snd_alias_list_t *)(uintptr_t)aliasListIter->pHashNext) {
        if (I_stricmp(name, aliasListIter->aliasName) == 0) {
            return 0;
        }
    }

#if defined(__x86_64__) || defined(_M_X64)
    aliasList->pHashNext = (intptr_t)(uintptr_t)g_sa.pHash[hash];
#else
    aliasList->pHashNext = (int)(uintptr_t)g_sa.pHash[hash];
#endif
    g_sa.pHash[hash] = aliasList;

    return 1;
}

void Com_DuplicateSoundAlias(snd_alias_list_t *aliasCopy, const char *name)
{
    unsigned int hash = Com_SoundAliasHash(name);
    snd_alias_list_t *aliasList;

    for (aliasList = g_sa.pHash[hash]; aliasList;
         aliasList = (snd_alias_list_t *)(uintptr_t)aliasList->pHashNext) {
        if (I_stricmp(name, aliasList->aliasName) == 0) {
            const char *aliasName = aliasList->aliasName;

            *aliasList = *aliasCopy;
            aliasList->aliasName = aliasName;
            return;
        }
    }

    aliasList = Hunk_AllocInternal(sizeof(*aliasList));
    *aliasList = *aliasCopy;
    {
        char *newName = Hunk_AllocInternal(strlen(name) + 1);
        strcpy(newName, name);
        aliasList->aliasName = newName;
    }
#if defined(__x86_64__) || defined(_M_X64)
    aliasList->pHashNext = (intptr_t)(uintptr_t)g_sa.pHash[hash];
#else
    aliasList->pHashNext = (int)(uintptr_t)g_sa.pHash[hash];
#endif
    g_sa.pHash[hash] = aliasList;
}

int SND_GetAliasOffset(const snd_alias_t *alias)
{
    snd_alias_list_t *aliasList = Com_FindSoundAlias_core(alias->pszAliasName);
    snd_alias_t *aliasIter = aliasList->head;
    int count = aliasList->count;
    int index;

    if (count <= 0) {
        return 0;
    }

    if (alias == aliasIter) {
        return 0;
    }

    index = 0;
    do {
        aliasIter++;
        index++;
        if (index == count) {
            return 0;
        }
    } while (alias != aliasIter);

    return index;
}

snd_alias_t *SND_GetAliasWithOffset(const char *name, int offset)
{
    snd_alias_list_t *aliasList = Com_FindSoundAlias_core(name);
    snd_alias_t *alias = aliasList->head;
    int count = aliasList->count;
    int index;

    if (count <= 0) {
        if (count != 0) {
            return alias;
        }

        Com_Error(1, "SND_GetAliasWithOffset: could not find sound alias '%s' with offset %d", name, offset);
        return NULL;
    }

    if (offset != 0) {
        snd_alias_t *p = alias;

        index = 0;
        do {
            p++;
            index++;
            if (index == count) {
                return alias;
            }
        } while (index != offset);

        alias = p;
    }

    return alias;
}

snd_alias_t *Com_PickSoundAlias(const char *aliasname)
{
    return Com_PickSoundAliasFromList_core(Com_FindSoundAlias_core(aliasname));
}
