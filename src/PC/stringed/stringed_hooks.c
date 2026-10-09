#include "common_types.h"
#include "imports.h"

#include <string.h>

extern const dvar_t *loc_language;
extern const dvar_t *loc_forceEnglish;
extern const dvar_t *loc_translate;
extern const dvar_t *loc_warnings;
extern const dvar_t *loc_warningsAsErrors;
static char szErrorString[1024];
static int iCurrString;
static char szStrings[2][1024];

__attribute__((used, packed, aligned(4)))
languageInfo_t g_languages[] = {
    { "english", 0 },
    { "french", 0 },
    { "german", 0 },
    { "italian", 0 },
    { "spanish", 0 },
    { "british", 0 },
    { "russian", 0 },
    { "polish", 0 },
    { "korean", 0 },
    { "taiwanese", 0 },
    { "japanese", 0 },
    { "chinese", 0 },
    { "thai", 0 },
    { "leet", 0 },
    { NULL, 0 },
    { NULL, 0 },
};
static qboolean g_currentAsian;

extern const dvar_t *com_developer;

extern byte *__DefaultRuneLocale;

extern void SE_Init(void);
extern void SE_ShutDown(void);
extern const char *SE_GetString(const char *pszReference);
extern const char *SE_LoadLanguage(int forceEnglish);
extern int FS_LanguageHasAssets(int language);
extern const dvar_t *Dvar_RegisterInt(const char *name, int value, int min, int max, unsigned int flags);
extern const dvar_t *Dvar_RegisterBool(const char *name, qboolean value, unsigned int flags);
extern void Dvar_SetInt(const dvar_t *dvar, int value);
extern int I_stricmp(const char *s1, const char *s2);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void I_strncat(char *dest, const char *src, int destsize);
extern void Com_Printf(const char *fmt, ...);
extern void Com_Error(int level, const char *fmt, ...);
extern const char *va(const char *fmt, ...);

int SEH_GetCurrentLanguage(void);
int SEH_VerifyLanguageSelection(int iLanguageSelection);
void SEH_Init_StringEd(void);
void SEH_Shutdown_StringEd(void);
const char *SEH_GetLanguageName(const int iLanguage);
const char *SEH_StringEd_GetString(const char *pszReference);
qboolean SEH_GetLanguageIndexForName(const char *pszLanguageName, int *piLanguageIndex);
void SEH_InitLanguage(void);
static qboolean SEH_StringEd_SetLanguageStrings(int iLanguage);
void SEH_UpdateLanguageInfo(void);
unsigned int SEH_ReadCharFromString(const char **ppsText, qboolean *pbIsTrailingPunctuation);
int SEH_PrintStrlen(const char *string);
const char *SEH_SafeTranslateString(const char *pszReference);
const char *SEH_LocalizeTextMessage(const char *pszInputBuffer, const char *pszMessageType, msgLocErrType_t errType);

int SEH_GetCurrentLanguage(void)
{
    return loc_language->current.integer;
}

int SEH_VerifyLanguageSelection(int iLanguageSelection)
{
    int i;

    if (g_languages[iLanguageSelection].bPresent) {
        return iLanguageSelection;
    }

    for (i = 0; i < 14; i++) {
        int candidate = (i + iLanguageSelection) % 14;
        if (g_languages[candidate].bPresent) {
            return candidate;
        }
    }

    iLanguageSelection = 0;
    return iLanguageSelection;
}

void SEH_Init_StringEd(void)
{
    SE_Init();
}

void SEH_Shutdown_StringEd(void)
{
    SE_ShutDown();
}

const char *SEH_GetLanguageName(const int iLanguage)
{

    if ((unsigned int)iLanguage > 13) {
        return g_languages[0].pszName;
    }
    return g_languages[iLanguage].pszName;
}

static inline __attribute__((always_inline)) const char *SEH_StringEd_GetString_core(const char *pszReference)
{
    if (!loc_translate || !loc_translate->current.enabled) {
        return pszReference;
    }
    if (pszReference[0] == '\0' || pszReference[1] == '\0') {
        return pszReference;
    }
    return SE_GetString(pszReference);
}

const char *SEH_StringEd_GetString(const char *pszReference)
{
    return SEH_StringEd_GetString_core(pszReference);
}

qboolean SEH_GetLanguageIndexForName(const char *pszLanguageName, int *piLanguageIndex)
{
    int i;

    for (i = 0; i < 14; i++) {
        if (I_stricmp(pszLanguageName, g_languages[i].pszName) == 0) {
            *piLanguageIndex = i;
            return 1;
        }
    }

    *piLanguageIndex = 0;
    return 0;
}

void SEH_InitLanguage(void)
{
    loc_language = Dvar_RegisterInt("loc_language", 0, 0, 13, 0x1021);
    loc_forceEnglish = Dvar_RegisterBool("loc_forceEnglish", 0, 0x1021);
    loc_translate = Dvar_RegisterBool("loc_translate", 1, 0x1020);
    loc_warnings = Dvar_RegisterBool("loc_warnings", 0, 0x1000);
    loc_warningsAsErrors = Dvar_RegisterBool("loc_warningsAsErrors", 0, 0x1000);

    g_currentAsian = ((unsigned int)(loc_language->current.integer - 8) <= 4);
}

static qboolean SEH_StringEd_SetLanguageStrings(int iLanguage)
{
    const char *pszError;

    if (!g_languages[iLanguage].bPresent) {
        return 0;
    }

    pszError = SE_LoadLanguage(loc_forceEnglish ? loc_forceEnglish->current.enabled : 0);
    if (!pszError) {
        return 1;
    }

#if defined(__x86_64__) || defined(_M_X64)

    if (com_developer && com_developer->current.enabled) {
        return 0;
    }
#else
    if (com_developer->current.enabled) {
        return 0;
    }
#endif
    if (!loc_warnings->current.enabled) {
        return 0;
    }

    if (loc_warningsAsErrors->current.enabled) {
        Com_Error(6, "Could not load localization strings for %s: %s", SEH_GetLanguageName(iLanguage), pszError);
        return 0;
    } else {
        Com_Printf("^3WARNING: Could not load localization strings for %s: %s\n", SEH_GetLanguageName(iLanguage), pszError);
        return 0;
    }
}

void SEH_UpdateLanguageInfo(void)
{
    int i;
    int iNumLanguages;

    if (!loc_language) {
        SEH_InitLanguage();
    }

    Dvar_RegisterInt(loc_language->name, 0, 0, 13, 0x1021);
    Dvar_RegisterBool(loc_forceEnglish->name, 0, 0x1021);

    g_currentAsian = ((unsigned int)(loc_language->current.integer - 8) <= 4);

    iNumLanguages = 0;
    for (i = 0; i < 14; i++) {
        if (FS_LanguageHasAssets(i)) {
            g_languages[i].bPresent = 1;
            iNumLanguages++;
        } else {
            g_languages[i].bPresent = 0;
        }
    }

    if (iNumLanguages <= 0) {
        Com_Printf("^1ERROR: No languages available because no localized assets were found\n");
    }

    if (SEH_StringEd_SetLanguageStrings(loc_language->current.integer)) {
        return;
    }

    for (i = 0; i < 14; i++) {
        Dvar_SetInt(loc_language, i);
        g_currentAsian = ((unsigned int)(loc_language->current.integer - 8) <= 4);
        if (SEH_StringEd_SetLanguageStrings(i)) {
            return;
        }
    }

    Dvar_SetInt(loc_language, 0);
    g_currentAsian = ((unsigned int)(loc_language->current.integer - 8) <= 4);
}

static int SEH_IsJapaneseDoubleByte(unsigned char lead, unsigned char follow)
{
    if ((unsigned char)(lead + 0x50) <= 0x18 || (unsigned char)(lead + 0x20) <= 0x0F) {
        if (follow > 0xA0 && follow != 0xFF) {
            return 1;
        }
    }
    return 0;
}

static int SEH_IsKoreanDoubleByte(unsigned char hi, unsigned char lo)
{
    if ((unsigned char)(hi + 0x5F) > 0x25) {
        if ((unsigned char)(hi + 0x37) > 0x30) {
            return 0;
        }
    }
    if ((unsigned char)(lo - 0x40) > 0x3E) {
        if ((unsigned char)(lo + 0x5F) > 0x5D) {
            return 0;
        }
    }
    return 1;
}

static int SEH_IsChineseDoubleByte(unsigned char lead, unsigned char follow)
{
    if ((unsigned char)(lead + 0x7F) > 0x1E) {
        if ((unsigned char)(lead + 0x20) > 0x0F) {
            return 0;
        }
    }
    if ((unsigned char)(follow - 0x40) > 0x3E) {
        if ((unsigned char)(follow + 0x80) > 0x7C) {
            return 0;
        }
    }
    return 1;
}

static int SEH_IsThaiDoubleByte(unsigned char hi, unsigned char lo)
{
    if ((unsigned char)(hi + 0x5F) > 0x56) {
        return 0;
    }
    if (lo <= 0xA0) {
        return 0;
    }
    if ((unsigned char)(lo + 1) == 0) {
        return 0;
    }
    return 1;
}

unsigned int SEH_ReadCharFromString(const char **ppsText, qboolean *pbIsTrailingPunctuation)
{
    const char *p = *ppsText;
    unsigned int uiLetter;
    unsigned char lead, follow;
    int language;

    if (g_currentAsian) {
        language = loc_language->current.integer;

        if (language == 8) {

            follow = (unsigned char)p[1];
            lead = (unsigned char)p[0];
            if (SEH_IsJapaneseDoubleByte(lead, follow)) {
                uiLetter = ((unsigned int)lead << 8) | follow;
                *ppsText = p + 2;
                if (pbIsTrailingPunctuation) {
                    *pbIsTrailingPunctuation = 0;
                }
                return uiLetter;
            }
        } else if (language == 9) {

            uiLetter = ((unsigned int)(unsigned char)p[0] << 8) | (unsigned char)p[1];
            if (SEH_IsKoreanDoubleByte((unsigned char)(uiLetter >> 8), (unsigned char)uiLetter)) {
                *ppsText = p + 2;
                if (pbIsTrailingPunctuation) {
                    *pbIsTrailingPunctuation = ((unsigned int)(uiLetter - 0xA140) <= 0x13);
                }
                return uiLetter;
            }
        } else if (language == 10) {

            follow = (unsigned char)p[1];
            lead = (unsigned char)p[0];
            if (SEH_IsChineseDoubleByte(lead, follow)) {
                uiLetter = ((unsigned int)lead << 8) | follow;
                *ppsText = p + 2;
                if (pbIsTrailingPunctuation) {
                    *pbIsTrailingPunctuation = ((unsigned int)(uiLetter - 0x8140) <= 0x11);
                }
                return uiLetter;
            }
        } else if (language == 11) {

            uiLetter = ((unsigned int)(unsigned char)p[0] << 8) | (unsigned char)p[1];
            if (SEH_IsThaiDoubleByte((unsigned char)(uiLetter >> 8), (unsigned char)uiLetter)) {
                *ppsText = p + 2;
                if (pbIsTrailingPunctuation) {
                    *pbIsTrailingPunctuation = ((unsigned int)(uiLetter - 0xA1A1) <= 0x0C);
                }
                return uiLetter;
            }
        }
    }

    uiLetter = (unsigned char)*p;
    *ppsText = p + 1;

    if (pbIsTrailingPunctuation) {
        if (uiLetter == '!' || uiLetter == '?' || uiLetter == ',' ||
            uiLetter == '.' || uiLetter == ';' || uiLetter == ':') {
            *pbIsTrailingPunctuation = 1;
        } else {
            *pbIsTrailingPunctuation = 0;
        }
    }

    return uiLetter;
}

int SEH_PrintStrlen(const char *string)
{
    const char *p;
    int len;
    unsigned int c;

    if (!string) {
        return 0;
    }

    p = string;
    len = 0;
    while (*p) {
        c = SEH_ReadCharFromString(&p, NULL);
        if (c == '^') {
            if (p && *p != '^' && *p > '/' && *p <= '9') {
                p++;
                continue;
            }
        } else if (c == '\n' || c == '\r') {
            continue;
        }
        len++;
    }
    return len;
}

const char *SEH_SafeTranslateString(const char *pszReference)
{
    const char *result;

    result = SEH_StringEd_GetString_core(pszReference);
    if (result) {
        return result;
    }

    if (loc_warnings->current.enabled) {
        if (loc_warningsAsErrors->current.enabled) {
            Com_Error(6, "Could not translate exe string \"%s\"", pszReference);
        } else {
            Com_Printf("^3WARNING: Could not translate exe string \"%s\"\n", pszReference);
        }
        strcpy(szErrorString, "^1UNLOCALIZED(^7");
        I_strncat(szErrorString, pszReference, 1024);
        I_strncat(szErrorString, "^1)^7", 1024);
        return szErrorString;
    }

    I_strncpyz(szErrorString, pszReference, 1024);
    return szErrorString;
}

static int SEH_IsDigit(int c)
{
    if ((unsigned int)c > 0xFF) {
        return 0;
    }
    return (((unsigned long *)((byte *)__DefaultRuneLocale + 0x34))[(unsigned char)c] >> 0) & 4;
}

const char *SEH_LocalizeTextMessage(const char *pszInputBuffer, const char *pszMessageType, msgLocErrType_t errType)
{
    char szTokenBuf[1024];
    char szInsertBuf[1024];
    char *pszString;
    const char *pszIn;
    const char *pszScanStart;
    int bLocOn;
    int bInsertEnabled;
    int iInsertLevel;
    int insertIndex;
    int bLocSkipped;
    int outputLen;
    int iTokenLen;
    int iLen;
    int i;
    const char *pszTranslated;

    iCurrString = (iCurrString + 1) % 2;
    pszString = szStrings[iCurrString];
    memset(pszString, 0, 1024);

    pszIn = pszInputBuffer;
    outputLen = 0;
    bLocOn = 1;
    bInsertEnabled = 1;
    iInsertLevel = 0;
    insertIndex = 1;
    bLocSkipped = 0;

    while (*pszIn) {

        pszScanStart = pszIn;
        while (*pszIn && *pszIn != 0x14 && *pszIn != 0x15 && *pszIn != 0x16) {
            pszIn++;
        }

        if (pszScanStart < pszIn) {
            iTokenLen = pszIn - pszScanStart;
            I_strncpyz(szTokenBuf, pszScanStart, iTokenLen + 1);

            if (bLocOn) {

                pszTranslated = NULL;
                if (loc_translate && loc_translate->current.enabled &&
                    szTokenBuf[0] != '\0' && szTokenBuf[1] != '\0') {
                    pszTranslated = SE_GetString(szTokenBuf);
                }

                if (!pszTranslated) {
                    if (!loc_translate || !loc_translate->current.enabled ||
                        szTokenBuf[0] == '\0') {
                        pszTranslated = szTokenBuf;
                    } else {

                        if (loc_warnings && loc_warnings->current.enabled) {
                            if (loc_warningsAsErrors && loc_warningsAsErrors->current.enabled) {
                                if (errType != LOCMSG_NOERR) {
                                    Com_Error(6, "Could not translate part of %s: \"%s\"", pszMessageType, szTokenBuf);
                                }
                            }
                            Com_Printf("^3WARNING: Could not translate part of %s: \"%s\"\n", pszMessageType, szTokenBuf);
                        }

                        if (!loc_warnings || !loc_warnings->current.enabled) {
                            pszTranslated = va("%s", szTokenBuf);
                        } else {
                            pszTranslated = va("^1UNLOCALIZED(^7%s^1)^7", szTokenBuf);
                        }

                        if (errType == LOCMSG_NOERR) {
                            return NULL;
                        }
                    }
                }

                strcpy(szTokenBuf, pszTranslated);
                iTokenLen = strlen(szTokenBuf);
            }

            iLen = iTokenLen + outputLen;
            if (iLen > 1023) {
                if (loc_warnings && loc_warnings->current.enabled &&
                    loc_warningsAsErrors && loc_warningsAsErrors->current.enabled) {
                    if (errType != LOCMSG_NOERR) {
                        Com_Error(1, "%s too long when translated: \"%s\"", pszMessageType, pszInputBuffer);
                    }
                }
                Com_Printf("%s too long when translated: \"%s\"\n", pszMessageType, pszInputBuffer);
            }

            if (iTokenLen - 2 > 0) {
                if (bInsertEnabled) {

                    for (i = 0; i < iTokenLen - 2; i++) {
                        if (szTokenBuf[i] == '&' && szTokenBuf[i + 1] == '&' &&
                            SEH_IsDigit((signed char)szTokenBuf[i + 2])) {
                            iInsertLevel++;
                        }
                    }
                } else {

                    for (i = 0; i < iTokenLen - 2; i++) {
                        if (szTokenBuf[i] == '&' && szTokenBuf[i + 1] == '&' &&
                            SEH_IsDigit((signed char)szTokenBuf[i + 2])) {
                            szTokenBuf[i] = 0x16;
                            bLocSkipped = 1;
                        }
                    }
                }
            }

            if (iInsertLevel > 0 && outputLen > 0) {
                int searchLen = outputLen - 2;
                char *insertPoint;
                int searchIdx;

                if (searchLen <= 0) {
                    insertPoint = pszString;
                    searchLen = 0;
                } else {

                    insertPoint = NULL;
                    for (searchIdx = 0; searchIdx < searchLen; searchIdx++) {
                        insertPoint = &pszString[searchIdx];
                        if (pszString[searchIdx] == '&' && pszString[searchIdx + 1] == '&' &&
                            SEH_IsDigit((signed char)pszString[searchIdx + 2])) {
                            int digit = (signed char)pszString[searchIdx + 2] - '0';
                            if (digit == 0) {
                                Com_Printf("%s cannot have &&0 as conversion format: \"%s\"\n", pszMessageType, pszInputBuffer);
                            }
                            if (digit == insertIndex) {

                                strcpy(szInsertBuf, &pszString[searchIdx + 3]);
                                pszString[searchIdx] = '\0';
                                insertIndex = digit + 1;
                                searchLen = searchIdx;
                                insertPoint = &pszString[searchIdx];
                                goto do_insert;
                            }
                        }
                    }

                    insertPoint = &pszString[searchIdx];
                }

            do_insert:
                strcpy(insertPoint, szTokenBuf);
                strcpy(&pszString[searchLen + iTokenLen], szInsertBuf);
                iInsertLevel--;
                outputLen = outputLen + iTokenLen - 3;
            } else {

                strcpy(&pszString[outputLen], szTokenBuf);
                outputLen = iLen;
            }
        }

        if (*pszIn == 0x14) {
            pszIn++;
            bLocOn = 1;
        } else if (*pszIn == 0x15) {
            pszIn++;
            bLocOn = 0;
        }

        if (*pszIn == 0x16) {
            pszIn++;
            bInsertEnabled = 0;
        }
    }

    if (bLocSkipped && outputLen > 0) {
        for (i = 0; i < outputLen; i++) {
            if (pszString[i] == 0x16) {
                pszString[i] = '%';
            }
        }
    }

    return pszString;
}
