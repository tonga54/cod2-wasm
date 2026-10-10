#include "common_types.h"
#include "imports.h"
#include <stdlib.h>

extern int FS_GetFileList(const char *path, const char *extension, int flags, char *listbuf, int bufsize);
extern int FS_FOpenFileByMode(const char *filename, int *f, int mode);
extern int FS_Read(void *buffer, int len, int f);
extern void FS_FCloseFile(fileHandle_t f);
extern const char *va(const char *format, ...);
extern void *Hunk_AllocateTempMemoryInternal(int size);
extern void *Hunk_AllocateTempMemoryHighInternal(int size);
extern void Hunk_ClearTempMemoryHigh(void);
extern void Hunk_ClearTempMemory(void);
extern void *Hunk_AllocAlignInternal(int size, int align);
extern void Com_BeginParseSession(const char *name);
extern void Com_EndParseSession(void);
extern const char *Com_Parse(const char **buf);
extern const char *Com_ParseOnLine(const char **buf);
extern void Com_SkipRestOfLine(const char **buf);
extern void Com_SetCSV(int csv);
extern void Com_Printf(const char *fmt, ...);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern int Com_SurfaceTypeFromName(const char *name);
extern const char *Com_SurfaceTypeToName(int type);
extern int stricmp(const char *s1, const char *s2);
extern void *FX_RegisterEffect(const char *name);

static const char *g_TypeName[11] = {
    "bullet_small_normal",
    "bullet_small_reflect",
    "bullet_large_normal",
    "bullet_large_reflect",
    "shotgun_normal",
    "shotgun_reflect",
    "bullet_ap_normal",
    "bullet_ap_reflect",
    "grenade_bounce",
    "grenade_explode",
    "rocket_explode"
};

static EffectTemplate *CG_EnlargeGrenadeExplosion(const EffectTemplate *original)
{
    EffectTemplate *effect;
    int i;

    if (!original)
        return NULL;
    effect = Hunk_AllocAlignInternal(sizeof(*effect), 4);
    memcpy(effect, original, sizeof(*effect));
    for (i = 0; i < effect->mPrimitiveCount; ++i) {
        const PrimitiveTemplate *source = original->mPrimitives[i];
        PrimitiveTemplate *particle;
        if (!source || (source->mType != PT_PARTICLE &&
                       source->mType != PT_ORIENTEDPARTICLE && source->mType != PT_CLOUD))
            continue;
        /* A grenade-only variant keeps shared impact templates untouched.
         * Reuse the original curves/media, counts and lifetimes; enlarge
         * their visible fire, dust and smoke without extra particles. */
        particle = Hunk_AllocAlignInternal(sizeof(*particle), 4);
        memcpy(particle, source, sizeof(*particle));
        particle->mFxChannels[FXCHAN_SIZE].scaleRange.mMin *= 1.35f;
        particle->mFxChannels[FXCHAN_SIZE].scaleRange.mMax *= 1.35f;
        if (source->mType == PT_CLOUD) {
            particle->mFxChannels[FXCHAN_LENGTH].scaleRange.mMin *= 1.35f;
            particle->mFxChannels[FXCHAN_LENGTH].scaleRange.mMax *= 1.35f;
        } else if (source->mNonUniformScale) {
            particle->mFxChannels[FXCHAN_SIZE2].scaleRange.mMin *= 1.35f;
            particle->mFxChannels[FXCHAN_SIZE2].scaleRange.mMax *= 1.35f;
        }
        particle->spawnFrustumCullRadius *= 1.35f;
        effect->mPrimitives[i] = particle;
    }
    return effect;
}

static int compare_impact_files(const int *pe0, const int *pe1)
{
    return stricmp((const char *)(*pe0), (const char *)(*pe1));
}

static int CG_RegisterImpactEffectsForDir(const char *dir, char *(*szEffectFile)[23])
{
    char listbuf[65536];

    int iListCount = FS_GetFileList(dir, "csv", 0, listbuf, 0x10000);
    if (iListCount == 0)
        return 0;

    if (iListCount > 4096)
        iListCount = 4096;

    const char *pszFiles[4096];
    const char *psz = listbuf;
    int i;
    for (i = 0; i < iListCount; i++) {
        pszFiles[i] = psz;
        psz += strlen(psz) + 1;
    }

    qsort(pszFiles, iListCount, 4, (int (*)(const void *, const void *))compare_impact_files);

    for (i = 0; i < iListCount; i++) {
        const char *filename = va("%s/%s", dir, pszFiles[i]);

        int f;
        int fileSize = FS_FOpenFileByMode(filename, &f, 0);
        if (fileSize < 0) {
            continue;
        }

        char *buf = (char *)Hunk_AllocateTempMemoryHighInternal(fileSize + 1);
        FS_Read(buf, fileSize, f);
        FS_FCloseFile(f);
        buf[fileSize] = '\0';

        Com_BeginParseSession(filename);
        Com_SetCSV(1);

        char *bufPtr = buf;
        const char *pszErrorMsg = NULL;

        for (;;) {
            const char *token = Com_Parse((const char **)&bufPtr);
            if (bufPtr == NULL)
                break;

            while (*token == '\0' || *token == '#') {
                Com_SkipRestOfLine((const char **)&bufPtr);
                token = Com_Parse((const char **)&bufPtr);
                if (bufPtr == NULL)
                    goto done_parsing;
            }

            int iEffectType;
            for (iEffectType = 0; iEffectType < 11; iEffectType++) {
                if (stricmp(g_TypeName[iEffectType], token) == 0)
                    break;
            }

            if (iEffectType >= 11) {
                pszErrorMsg = va("unknown effect type '%s' in first column of file '%s'", token, filename);
                break;
            }

            token = Com_ParseOnLine((const char **)&bufPtr);
            if (*token == '\0') {
                pszErrorMsg = va("missing surface type in second column of file '%s'", filename);
                break;
            }

            int iSurfaceType = Com_SurfaceTypeFromName(token);
            if (iSurfaceType < 0) {
                pszErrorMsg = va("unknown surface type '%s' in second column of file '%s'", token, filename);
                break;
            }

            token = Com_ParseOnLine((const char **)&bufPtr);
            int tokenLen = strlen(token);
            if (tokenLen > 63) {
                pszErrorMsg = va("effect filename '%s' in third column of file '%s' is longer than %i characters", token, filename, 63);
                break;
            }

            char *effectName = (char *)Hunk_AllocateTempMemoryInternal(tokenLen + 1);
            (*szEffectFile)[iEffectType * 23 + iSurfaceType] = effectName;
            strcpy(effectName, token);

            Com_SkipRestOfLine((const char **)&bufPtr);
        }

    done_parsing:
        Com_EndParseSession();
        Hunk_ClearTempMemoryHigh();

        if (pszErrorMsg) {
            Com_Printf("^1ERROR: %s", pszErrorMsg);
        }
    }

    return 0;
}

FxImpactTable *CG_RegisterImpactEffects(const char *mapname)
{
    char listbuf[65536];

    char *szEffectFile[11 * 23];
    memset(szEffectFile, 0, sizeof(szEffectFile));

    CG_RegisterImpactEffectsForDir("fx", (char *(*)[23])szEffectFile);

    if (mapname != NULL) {
        char mapdir[64];
        Com_sprintf(mapdir, 64, "fx/maps/%s", mapname);
        CG_RegisterImpactEffectsForDir(mapdir, (char *(*)[23])szEffectFile);
    }

    FxImpactTable *fx = (FxImpactTable *)Hunk_AllocAlignInternal(8, 4);
    fx->table = (FxImpactEntry *)Hunk_AllocAlignInternal(sizeof(szEffectFile), 4);

    int typeIdx;
    int iBadCount = 0;
    int offset = 0;
    const EffectTemplate *grenadeOriginals[23];
    EffectTemplate *grenadeVariants[23];
    int grenadeVariantCount = 0;

    for (typeIdx = 0; typeIdx < 11; typeIdx++) {
        FxImpactEntry *entries = (FxImpactEntry *)((byte *)fx->table + offset);
        const char *pszTypeName = g_TypeName[typeIdx];
        int typeBadCount = 0;

        int surfIdx;
        for (surfIdx = 0; surfIdx < 23; surfIdx++) {
            char *effectFile = szEffectFile[typeIdx * 23 + surfIdx];
            if (effectFile == NULL) {
                Com_Printf("no entry for effect type '%s' on surface type '%s'\n",
                           pszTypeName, Com_SurfaceTypeToName(surfIdx));
                typeBadCount++;
                ((int *)entries)[surfIdx] = 0;
            } else if (*effectFile == '\0') {
                ((int *)entries)[surfIdx] = 0;
            } else {
                EffectTemplate *effect = FX_RegisterEffect(effectFile);
                if (typeIdx == 9 && effect) {
                    int variant;
                    for (variant = 0; variant < grenadeVariantCount; ++variant)
                        if (grenadeOriginals[variant] == effect)
                            break;
                    if (variant == grenadeVariantCount) {
                        grenadeOriginals[variant] = effect;
                        grenadeVariants[variant] = CG_EnlargeGrenadeExplosion(effect);
                        ++grenadeVariantCount;
                    }
                    effect = grenadeVariants[variant];
                }
                ((int *)entries)[surfIdx] = (int)effect;
            }
        }

        iBadCount += typeBadCount;
        offset += 23 * 4;
    }

    if (iBadCount != 0) {
        Com_Printf("^1ERROR: %i missing entries in effect CSV files (see console for details)\n", iBadCount);
        fx = NULL;
    } else {
        *(const char **)fx = "illusionoftypes";
    }

    Hunk_ClearTempMemory();

    return fx;
}
