#include "common_types.h"
#include "imports.h"
#include <stdarg.h>
#include <string.h>

extern int rand(void);
extern int unzOpenCurrentFile(unzFile file);
extern unzFile unzReOpen(const char *path, unzFile file);
extern int unzReadCurrentFile(unzFile file, voidp buf, unsigned int len);
extern int unzSetCurrentFileInfoPosition(unzFile file, long unsigned int pos);
extern long int unztell(unzFile file);

int fs_numServerIwds = 0x0;
int fs_packFiles = 0x0;

#ifdef __EMSCRIPTEN__
#    include <ctype.h>
#endif

extern char fs_gamedir[256];
extern const dvar_t *fs_debug;
extern const dvar_t *fs_homepath;
extern const dvar_t *fs_basepath;
extern const dvar_t *fs_basegame;
extern const dvar_t *fs_cdpath;
extern const dvar_t *fs_copyfiles;
extern const dvar_t *fs_gameDirVar;
extern const dvar_t *fs_restrict;
extern searchpath_t *fs_searchpaths;
extern int fs_loadStack;
extern int fs_packFiles;
extern int fs_fakeChkSum;
extern int fs_checksumFeed;
extern fileHandleData_t fsh[74];
extern int fs_numServerIwds;
extern int fs_serverIwds[1024];
extern char *fs_serverIwdNames[1024];
extern int fs_numServerReferencedIwds;
extern int fs_serverReferencedIwds[1024];
extern char *fs_serverReferencedIwdNames[1024];
extern int com_fileAccessed;
extern const dvar_t *fs_useOldAssets;
extern const dvar_t *fs_ignoreLocalized;
extern char lastValidBase[256];
extern char lastValidGame[256];
extern void *imp_com_sv_running;
static char szIwdLanguageName[2][64];
static int iString;
extern void Hunk_FreeTempMemory(void *buf);
extern void *Hunk_AllocateTempMemoryInternal(int size);
extern void FS_DisplayPath(qboolean bLanguageCull);
extern int SEH_GetCurrentLanguage(void);
extern const char *SEH_GetLanguageName(int iLanguage);
extern int FS_GetModList(char *listbuf, int bufsize);
extern void *Z_MallocInternal(int size);
extern const char *Dvar_GetString(const char *name);
extern const char *va(const char *fmt, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern void Z_FreeInternal(void *ptr);
extern const dvar_t *Dvar_RegisterInt(const char *dvarName, int value, int min, int max, unsigned short flags);
extern const dvar_t *Dvar_RegisterBool_mac(const char *dvarName, int value, unsigned short flags);
extern const dvar_t *Dvar_RegisterString_mac(const char *dvarName, const char *value, unsigned short flags);
extern const char *Sys_DefaultCDPath(void);
extern const char *Sys_DefaultInstallPath(void);
extern const char *Sys_DefaultHomePath(void);
extern void Com_Printf(const char *fmt, ...);
extern const char *Com_GetExtensionSubString(const char *filename);
extern void Com_StartupVariable(const char *match);
extern void SEH_InitLanguage(void);
extern void SEH_Init_StringEd(void);
extern void SEH_UpdateLanguageInfo(void);
extern void FS_SetRestrictions(void);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern qboolean Com_SafeMode(void);
extern void Cbuf_AddText(const char *text);
extern void Com_ReadCDKey(void);
extern void FS_AddCommands(void);
extern void Dvar_ClearModified(const dvar_t *dvar);
extern void Dvar_SetString(const dvar_t *dvar, const char *value);
extern void Dvar_SetBool(const dvar_t *dvar, qboolean value);
extern void FS_PureServerSetLoadedIwds(const char *iwdSums, const char *iwdNames);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern int Sys_DirectoryHasContents(const char *path);
extern char **Sys_ListFiles(const char *directory, const char *extension, const char *filter, int *numfiles, qboolean wantsubs);
extern void Sys_FreeFileList(char **list);
extern int I_strncmp(const char *s0, const char *s1, int n);
extern char *I_strlwr(char *s);
extern unzFile unzOpen(const char *path);
extern int unzGetGlobalInfo(unzFile file, unz_global_info *pglobal_info);
extern int unzGetCurrentFileInfo(unzFile file, unz_file_info *pfile_info, char *szFileName, uLong fileNameBufferSize, double *extraField, uLong extraFieldBufferSize, char *szComment, uLong commentBufferSize);
extern int unzGetCurrentFileInfoPosition(unzFile file, long unsigned int *pos);
extern int unzGoToFirstFile(unzFile file);
extern int unzGoToNextFile(unzFile file);
extern unsigned int Com_BlockChecksum(const void *buffer, int length);
extern unsigned int Com_BlockChecksumKey(void *buffer, int length, int key);
extern qboolean SEH_GetLanguageIndexForName(const char *pszLanguageName, int *piLanguageIndex);
#ifdef __EMSCRIPTEN__
#    include <ctype.h>
#else
extern int isalpha(int c);
#endif
extern void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));

extern size_t FS_FileRead(void *ptr, size_t size, size_t nitems, FILE *stream);
extern size_t FS_FileWrite(const void *ptr, size_t size, size_t nitems, FILE *stream);
extern void *malloc(size_t size);
extern void free(void *ptr);
extern int remove(const char *path);
extern DWORD GetFileAttributesA(LPCSTR lpFileName);
extern BOOL SetFileAttributesA(LPCSTR lpFileName, DWORD dwFileAttributes);
extern void Sys_StreamSeek(fileHandle_t f, int offset, int origin);

static qboolean bLanguagesListed;

qboolean FS_Initialized(void);
void FS_CheckFileSystemStarted(void);
int FS_LoadStack(void);
qboolean FS_UseSearchPath(const searchpath_t *pSearch);
qboolean FS_LanguageHasAssets(int iLanguage);
long int FS_HashFileName(const char *fname, int hashSize);
fileHandle_t FS_HandleForFile(qboolean streamThread);
FILE *FS_FileForHandle(fileHandle_t f);
qboolean FS_CreatePath(char *OSPath);
void FS_Remove(const char *osPath);
qboolean FS_FilenameCompare(const char *s1, const char *s2);
qboolean FS_PureIgnoresExtension(const char *extension);
void FS_ResetFiles(void);
void FS_ConvertPath(char *s);
void FS_ShutdownServerIwdNames(void);
void FS_ShutdownServerReferencedIwds(void);
Bool FS_RegisterDvars(void);
void FS_ClearIwdReferences(void);
const char *GetBspExtension(void);
void FS_FreeFile(void *buffer);
void FS_FreeFileList(const char **list, int allocTrackType);
int FS_filelength(fileHandle_t f);
void FS_FCloseFile(fileHandle_t h);
void FS_Shutdown(qboolean closemfp);
int FS_Read(void *buffer, int len, fileHandle_t h);
int FS_Write(const void *buffer, int len, fileHandle_t h);
int FS_Seek(fileHandle_t f, long int offset, int origin);
int FS_FTell(fileHandle_t f);
void FS_Flush(fileHandle_t f);
static Bool __attribute_regparm__(3) FS_SanitizeFilename(const char *filename, char *sanitizedName, unsigned int sanitizedNameSize);
static void __attribute_regparm__(3) FS_BuildOSPath_Internal(const char *base, const char *game, const char *qpath, char *ospath, qboolean streamThread);
void FS_BuildOSPath(const char *base, const char *game, const char *qpath, char *ospath);
qboolean FS_FileExists(const char *file);
const char *FS_ShortOSFilePath(const char *filename);
int FS_GetFileOsPath(const char *filename, char *ospath);
void FS_DisplayPath(qboolean bLanguageCull);
void FS_Path_f(void);
void FS_FullPath_f(void);
void FS_Printf(fileHandle_t h, const char *fmt, ...);
const char **FS_ListFilteredFiles(searchpath_t *searchPath, const char *path, const char *extension, const char *filter, FsListBehavior behavior, int *numfiles, int allocTrackType);
const char **FS_ListFiles(const char *path, const char *extension, FsListBehavior behavior, int *numfiles, int allocTrackType);
int FS_GetFileList(const char *path, const char *extension, FsListBehavior behavior, char *listbuf, int bufsize);
void FS_SortFileList(const char **filelist, int numfiles);
fileHandle_t FS_OpenFileOverwrite(const char *qpath);
static int iwdsort(const float *a, const float *b);
static void FS_AddIwdFilesForGameDirectory(const char *path, const char *pszGameFolder);
static void __attribute_regparm__(3) FS_AddGameDirectory(const char *path, const char *dir, qboolean bLanguageDirectory, int iLanguage);
void FS_CopyFile(char *fromOSPath, char *toOSPath);
static int __attribute_regparm__(3) FS_FOpenFileRead_Internal(const char *filename, fileHandle_t *file, qboolean uniqueFILE, qboolean streamThread, qboolean fsOnly);
int FS_FOpenFileRead(const char *filename, fileHandle_t *file, qboolean uniqueFILE);
int FS_FOpenFileReadStream(const char *filename, fileHandle_t *file, qboolean uniqueFILE);
qboolean FS_TouchFile(const char *name);
int FS_ReadFile(const char *qpath, void **buffer);
fileHandle_t FS_FOpenFileWrite(const char *filename);
qboolean FS_WriteFile(const char *qpath, const void *buffer, int size);
fileHandle_t FS_FOpenTextFileWrite(const char *filename);
fileHandle_t FS_FOpenFileAppend(const char *filename);
int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, fsMode_t mode);
void FS_Startup(const char *gameName);
void FS_Restart(int checksumFeed);
qboolean FS_ConditionalRestart(int checksumFeed);
void FS_InitFilesystem(void);

qboolean FS_Initialized(void)
{
    return fs_searchpaths != 0;
}

void FS_CheckFileSystemStarted(void)
{
}

int FS_LoadStack(void)
{
    return fs_loadStack;
}

qboolean FS_UseSearchPath(const searchpath_t *pSearch)
{
    if (!pSearch->bLocalized)
        return 1;

    if (fs_ignoreLocalized->current.enabled)
        return 0;

    if (pSearch->language != SEH_GetCurrentLanguage())
        return 0;

    return 1;
}

qboolean FS_LanguageHasAssets(int iLanguage)
{
    searchpath_t *sp;
    for (sp = fs_searchpaths; sp; sp = (searchpath_t *)(uintptr_t)sp->next) {
        if (sp->bLocalized && sp->language == iLanguage)
            return 1;
    }
    return 0;
}

extern int ___tolower(int c);
long int FS_HashFileName(const char *fname, int hashSize)
{
    int hash, i, letter;
    char c;

    hash = 0;
    i = 119;
    c = fname[0];
    while (c) {
        letter = ___tolower((signed char)c);
        if (letter == '.')
            break;
        if (letter == '\\')
            letter = '/';
        hash += letter * i;
        c = fname[i - 118];
        i++;
    }

    hash = (hash >> 20) ^ (hash ^ (hash >> 10));
    return hash & (hashSize - 1);
}

fileHandle_t FS_HandleForFile(qboolean streamThread)
{
    int first, count, i;

    if (streamThread) {
        first = 61;
        count = 13;
    } else {
        first = 1;
        count = 60;
    }

    if (!fsh[first].handleFiles.file.o) {
        return first;
    }

    for (i = 1; i < count; i++) {
        if (!fsh[first + i].handleFiles.file.o) {
            return first + i;
        }
    }

    for (i = 1; i < 74; i++) {
        Com_Printf((const char *)"FILE %2i: '%s'\n", i, fsh[i].name);
    }
    Com_Error(1, (const char *)"\x15"
                               "FS_HandleForFile: none free");
    return -1;
}

FILE *FS_FileForHandle(fileHandle_t f)
{
#if defined(__x86_64__) || defined(_M_X64)

    return (FILE *)fsh[f].handleFiles.file.o;
#else
    return *(FILE **)((byte *)fsh + f * 284);
#endif
}

extern char *strstr(const char *, const char *);
extern void Sys_Mkdir(const char *path);
extern void Com_Printf(const char *fmt, ...);
qboolean FS_CreatePath(char *OSPath)
{
    char *ofs;

    if (strstr(OSPath, "..") || strstr(OSPath, "::")) {
        Com_Printf("WARNING: refusing to create relative path \"%s\"\n", OSPath);
        return 1;
    }

    for (ofs = OSPath + 1; *ofs; ofs++) {
        if (*ofs == '/') {
            *ofs = '\0';
            Sys_Mkdir(OSPath);
            *ofs = '/';
        }
    }
    return 0;
}

void FS_Remove(const char *osPath)
{
    remove(osPath);
}

extern Bool I_islower(int c);
qboolean FS_FilenameCompare(const char *s1, const char *s2)
{
    int c1, c2;

    do {
        c1 = (signed char)*s1++;
        c2 = (signed char)*s2++;

        if (I_islower(c1))
            c1 -= 0x20;
        if (I_islower(c2))
            c2 -= 0x20;

        if (c1 == '\\' || c1 == ':')
            c1 = '/';
        if (c2 == '\\' || c2 == ':')
            c2 = '/';

        if (c1 != c2)
            return -1;
    } while (c1);

    return 0;
}

extern int stricmp(const char *, const char *);
extern int I_stricmp(const char *, const char *);
extern int I_strnicmp(const char *, const char *, int);
extern char *CopyStringInternal(const char *in);
extern Bool Com_FilterPath(const char *filter, const char *name, int casesensitive);
qboolean FS_PureIgnoresExtension(const char *extension)
{

    if (*extension == '.')
        extension++;

    if (!stricmp(extension, "cfg"))
        return 1;
    if (!I_stricmp(extension, "menu"))
        return 1;
    if (!I_stricmp(extension, "str"))
        return 1;
    if (!I_stricmp(extension, "roq"))
        return 1;
    if (!I_stricmp(extension, ".dm_NETWORK_PROTOCOL_VERSION"))
        return 1;
    return 0;
}

void FS_ResetFiles(void)
{
    fs_loadStack = 0;
}

void FS_ConvertPath(char *s)
{
    while (*s) {
        if (*s == '\\' || *s == ':')
            *s = '/';
        s++;
    }
}

void FS_ShutdownServerIwdNames(void)
{
    int i;
    for (i = 0; i < fs_numServerIwds; i++) {
        if (fs_serverIwdNames[i])
            Z_FreeInternal(fs_serverIwdNames[i]);
        fs_serverIwdNames[i] = 0;
    }
    fs_numServerIwds = 0;
}

void FS_ShutdownServerReferencedIwds(void)
{
    int i;
    for (i = 0; i < fs_numServerReferencedIwds; i++) {
        if (fs_serverReferencedIwdNames[i])
            Z_FreeInternal(fs_serverReferencedIwdNames[i]);
        fs_serverReferencedIwdNames[i] = 0;
    }
    fs_numServerReferencedIwds = 0;
}

Bool FS_RegisterDvars(void)
{
    const char *homePath;

    if (fs_debug)
        return 0;

    fs_debug = Dvar_RegisterInt("fs_debug", 0, 0, 2, 0x1000);
    fs_copyfiles = Dvar_RegisterBool_mac("fs_copyfiles", 0, 0x1010);
    fs_cdpath = Dvar_RegisterString_mac("fs_cdpath", Sys_DefaultCDPath(), 0x1010);
    fs_basepath = Dvar_RegisterString_mac("fs_basepath", Sys_DefaultInstallPath(), 0x1010);
    fs_basegame = Dvar_RegisterString_mac("fs_basegame", "", 0x1010);
    fs_useOldAssets = Dvar_RegisterBool_mac("fs_useOldAssets", 0, 0x1000);

    homePath = Sys_DefaultHomePath();
    if (!homePath || !homePath[0]) {
#if defined(__x86_64__) || defined(_M_X64)

        homePath = fs_basepath->current.string;
#else
        homePath = *(const char **)(*(byte **)&fs_basepath + 8);
#endif
    }

    fs_homepath = Dvar_RegisterString_mac("fs_homepath", homePath, 0x1010);
    fs_gameDirVar = Dvar_RegisterString_mac("fs_game", "", 0x101c);
    fs_restrict = Dvar_RegisterBool_mac("fs_restrict", 0, 0x1010);
    fs_ignoreLocalized = Dvar_RegisterBool_mac("fs_ignoreLocalized", 0, 0x10a0);

    return 1;
}

void FS_ClearIwdReferences(void)
{
    byte *search;

    for (search = (byte *)fs_searchpaths; search; search = *(byte **)search) {
        byte *pack = *(byte **)(search + 4);
        if (pack)
            *(byte *)(pack + 0x310) = 0;
    }
}

const char *GetBspExtension(void)
{
    const char *ext = Dvar_GetString((const char *)"gfx_driver");
    if (*ext)
        return va("%sbsp", ext);
    return va((const char *)"d3dbsp");
}

void FS_FreeFile(void *buffer)
{
    fs_loadStack--;
    Hunk_FreeTempMemory(buffer);
}

void FS_FreeFileList(const char **list, int allocTrackType)
{
    if (!list)
        return;

    const char **p;
    for (p = list; *p; p++) {
        Z_FreeInternal((void *)*p);
    }
    Z_FreeInternal((void *)list);
    return;
}

extern int FS_FileSeek(FILE *stream, long int offset, int origin);
int FS_filelength(fileHandle_t f)
{
    fileHandleData_t *entry = &fsh[f];
    FILE *h;
    int pos, end;

#ifdef __EMSCRIPTEN__
    if (entry->zipFile) {
        unz_file_info info;
        if (unzGetCurrentFileInfo(entry->handleFiles.file.z, &info,
                                  NULL, 0, NULL, 0, NULL, 0) != 0) {
            return -1;
        }
        return (int)info.uncompressed_size;
    }
#endif

    if (entry->zipFile)
#if defined(__x86_64__) || defined(_M_X64)

        return (int)((unz_s *)entry->handleFiles.file.z)->cur_file_info.uncompressed_size;
#else
        return *(int *)(*(byte **)entry + 0x44);
#endif

    h = entry->handleFiles.file.o;
    pos = ftell(h);
    FS_FileSeek(h, 0, 2);
    end = ftell(h);
    FS_FileSeek(h, pos, 0);
    return end;
}

extern int FS_FileClose(FILE *stream);
extern void Sys_EndStreamedFile(fileHandle_t h);
extern int unzCloseCurrentFile(unzFile file);
extern int unzClose(unzFile file);
extern void Com_Memcpy(void *dest, const void *src, int count);
extern void Com_Memset(void *dest, int val, int count);
extern void SND_StopSounds(int fadeTime);
extern void SEH_Shutdown_StringEd(void);
extern void Cmd_RemoveCommand(const char *cmdName);
void FS_FCloseFile(fileHandle_t h)
{
    fileHandleData_t *entry = &fsh[h];

    if (entry->streamed)
        Sys_EndStreamedFile(h);

    if (entry->zipFile) {

        unzCloseCurrentFile(entry->handleFiles.file.z);
        if (entry->handleFiles.unique) {

            unzClose(entry->handleFiles.file.z);
        }
    } else if (h) {

        FS_FileClose(entry->handleFiles.file.o);
    }

    Com_Memset(entry, 0, sizeof(fileHandleData_t));
}

#if !defined(__EMSCRIPTEN__) || __SIZEOF_POINTER__ == 4
void FS_Shutdown(qboolean closemfp)
{
    int i;
    searchpath_t *p;
    searchpath_t *next;

    (void)closemfp;

    SND_StopSounds(8);
    SEH_Shutdown_StringEd();

    for (i = 1; i < 74; ++i) {
        if (fsh[i].fileSize)
            FS_FCloseFile(i);
    }

    for (p = fs_searchpaths; p; p = next) {
        next = (searchpath_t *)(uintptr_t)p->next;

        if (p->pack) {
            unzClose(p->pack->handle);
            Z_FreeInternal(p->pack->buildBuffer);
            Z_FreeInternal(p->pack);
        }

        if (p->dir)
            Z_FreeInternal(p->dir);

        Z_FreeInternal(p);
    }

    fs_searchpaths = NULL;

    Cmd_RemoveCommand("path");
    Cmd_RemoveCommand("fullpath");
    Cmd_RemoveCommand("dir");
    Cmd_RemoveCommand("fdir");
    Cmd_RemoveCommand("touchFile");
}

int FS_Read(void *buffer, int len, fileHandle_t h)
{
    fileHandleData_t *entry;
    byte *buf;
    FILE *file;
    int remaining;
    int tries;

    if (!h) {
        return 0;
    }

    entry = &fsh[h];
    if (entry->zipFile) {
        return unzReadCurrentFile(entry->handleFiles.file.z, buffer, len);
    }

    file = entry->handleFiles.file.o;
    buf = (byte *)buffer;
    if (!len) {
        return len;
    }

    remaining = len;
    tries = 0;
    while (remaining) {
        int read = (int)FS_FileRead(buf, 1, remaining, file);
        if (!read) {
            if (tries) {
                return len - remaining;
            }
            tries = 1;
            continue;
        }

        if (read == -1) {
            if ((unsigned int)(h - 61) <= 12) {
                return -1;
            }
            Com_Error(0, "\x15"
                         "FS_Read: -1 bytes read");
        }

        buf += read;
        remaining -= read;
    }

    return len;
}

int FS_Write(const void *buffer, int len, fileHandle_t h)
{
    int tries;
    FILE *f;
    const byte *buf;
    int remaining;

    if (!h) {
        return (len = 0);
    }

    f = fsh[h].handleFiles.file.o;
    buf = (const byte *)buffer;

    if (len) {
        remaining = len;
        tries = 0;
        while (remaining) {
            int wrote = (int)FS_FileWrite(buf, 1, remaining, f);
            if (!wrote) {
                if (tries) {
                    return (len = 0);
                }
                tries = 1;
            }
            if (wrote == -1) {
                return (len = 0);
            }
            buf += wrote;
            remaining -= wrote;
        }
    }

    if (fsh[h].handleSync) {
        fflush(f);
    }

    return len;
}

int FS_Seek(fileHandle_t f, long int offset, int origin)
{
    fileHandleData_t *entry;
    int skip;

    entry = &fsh[f];

    if (entry->streamed) {
        entry->streamed = 0;
        Sys_StreamSeek(f, offset, origin);
        entry->streamed = 1;
    }

    if (!entry->zipFile) {
        int whence;

        if (origin == 0) {
            whence = 1;
        } else if (origin == 1) {
            whence = 2;
        } else if (origin == 2) {
            whence = 0;
        } else {
            return 0;
        }

        return FS_FileSeek(entry->handleFiles.file.o, offset, whence);
    }

    if (!offset) {
        if (origin == 2) {
            unzSetCurrentFileInfoPosition(entry->handleFiles.file.z, entry->zipFilePos);
            return unzOpenCurrentFile(entry->handleFiles.file.z);
        }
        if (origin == 0) {
            return 0;
        }
    }

    {
        int current = unztell(entry->handleFiles.file.z);

        if (origin == 0) {
            if (offset < 0) {
                unzSetCurrentFileInfoPosition(entry->handleFiles.file.z, entry->zipFilePos);
                unzOpenCurrentFile(entry->handleFiles.file.z);
                skip = current + offset;
            } else {
                skip = offset;
            }
        } else if (origin == 1) {
            int target = FS_filelength(f) + offset;

            if (current > target) {
                unzSetCurrentFileInfoPosition(entry->handleFiles.file.z, entry->zipFilePos);
                unzOpenCurrentFile(entry->handleFiles.file.z);
                skip = target;
            } else {
                skip = target - current;
            }
        } else if (origin == 2) {
            if (offset < current) {
                unzSetCurrentFileInfoPosition(entry->handleFiles.file.z, entry->zipFilePos);
                unzOpenCurrentFile(entry->handleFiles.file.z);
                skip = offset;
            } else {
                skip = offset - current;
            }
        } else {
            return -1;
        }
    }

    return unzReadCurrentFile(entry->handleFiles.file.z, NULL, skip) ? 0 : -1;
}

int FS_FTell(fileHandle_t f)
{
    fileHandleData_t *entry = &fsh[f];

    if (entry->zipFile) {
        return unztell(entry->handleFiles.file.z);
    }

    return ftell(entry->handleFiles.file.o);
}

void FS_Flush(fileHandle_t f)
{
#    if defined(__x86_64__) || defined(_M_X64)

    fflush((FILE *)fsh[f].handleFiles.file.o);
#    else

    fflush(*(FILE **)((byte *)&fsh + f * 71 * 4));
#    endif
    return;
}

static __attribute_regparm__(3)
    Bool __attribute_regparm__(3) FS_SanitizeFilename(const char *filename, char *sanitizedName, unsigned int sanitizedNameSize)
{
    unsigned int srcIndex = 0;
    unsigned int dstIndex = 0;
    unsigned char c;

    (void)sanitizedNameSize;

    while (filename[srcIndex] == '/' || filename[srcIndex] == '\\') {
        ++srcIndex;
    }

    if (filename[srcIndex] == '\0') {
        sanitizedName[0] = '\0';
        return 1;
    }

    while ((c = (unsigned char)filename[srcIndex]) != '\0') {
        if ((c == '.' && filename[srcIndex + 1] == '.') ||
            (c == ':' && filename[srcIndex + 1] == ':')) {
            return 0;
        }

        if (c == '.') {
            unsigned char next = (unsigned char)filename[srcIndex + 1];

            if (next == '\0' || next == '/' || next == '\\') {
                ++srcIndex;
                continue;
            }
        }

        if (c == '/' || c == '\\') {
            sanitizedName[dstIndex++] = '/';
            do {
                ++srcIndex;
                c = (unsigned char)filename[srcIndex];
            } while (c == '/' || c == '\\');
            continue;
        }

        sanitizedName[dstIndex++] = (char)c;
        ++srcIndex;
    }

    sanitizedName[dstIndex] = '\0';
    return 1;
}

static __attribute_regparm__(3) void FS_BuildOSPath_Internal(const char *base, const char *game, const char *qpath, char *ospath, qboolean streamThread)
{
    const char *useGame;
    int lenBase;
    int lenGame;
    int lenQpath;
    const char *src;
    char *dst;
    qboolean sawSlash;

    useGame = (game && game[0]) ? game : fs_gamedir;
    lenBase = strlen(base);
    lenGame = strlen(useGame);
    lenQpath = strlen(qpath);

    if (lenBase + lenGame + lenQpath + 2 > 0xff) {
        if (streamThread) {
            ospath[0] = '\0';
            return;
        }
        Com_Error(0, "\025FS_BuildOSPath: os path length exceeded\n");
    }

    memcpy(ospath, base, lenBase);
    ospath[lenBase] = '/';
    memcpy(ospath + lenBase + 1, useGame, lenGame);
    ospath[lenBase + lenGame + 1] = '/';
    memcpy(ospath + lenBase + lenGame + 2, qpath, lenQpath + 1);

    src = ospath;
    dst = ospath;
    sawSlash = 0;

    while (*src) {
        char c = *src++;

        if (c == '\\' || c == '/'
#    ifndef _WIN32
            || c == ':'
#    endif
        ) {
            if (sawSlash) {
                continue;
            }
            *dst++ = '/';
            sawSlash = 1;
            continue;
        }
        *dst++ = c;
        sawSlash = 0;
    }
    *dst = '\0';

    return;
}

void FS_BuildOSPath(const char *base, const char *game, const char *qpath, char *ospath)
{
    return FS_BuildOSPath_Internal(base, game, qpath, ospath, 0);
}

extern FILE *FS_FileOpen(const char *path, const char *mode);
extern int FS_FileClose(FILE *stream);
qboolean FS_FileExists(const char *file)
{
    char testpath[256];
    FILE *handle;

    FS_BuildOSPath(fs_homepath->current.string, fs_gamedir, file, testpath);
    handle = FS_FileOpen(testpath, "rb");
    if (!handle)
        return 0;
    FS_FileClose(handle);
    return 1;
}

const char *FS_ShortOSFilePath(const char *filename)
{
    searchpath_t *search;
    char netpath[256];

    for (search = fs_searchpaths; search; search = (searchpath_t *)(uintptr_t)search->next) {
        FILE *handle;
        const char *base;

        if (search->bLocalized) {
            if (fs_ignoreLocalized->current.enabled) {
                continue;
            }
            if (search->language != SEH_GetCurrentLanguage()) {
                continue;
            }
        }

        if (!search->dir) {
            continue;
        }

        base = search->dir->gamedir;
        FS_BuildOSPath_Internal(base, NULL, filename, netpath, 0);
        handle = FS_FileOpen(netpath, "rb");
        if (handle) {
            FS_FileClose(handle);
            return va("%s/%s", base, filename);
        }
    }

    return NULL;
}

int FS_GetFileOsPath(const char *filename, char *ospath)
{
    char sanitizedName[256];
    searchpath_t *search;
    FILE *file;

    if (!FS_SanitizeFilename(filename, sanitizedName, sizeof(sanitizedName)))
        return -1;

    for (search = fs_searchpaths; search; search = (searchpath_t *)(uintptr_t)search->next) {
        if (search->pack)
            continue;
        if (!search->dir)
            continue;
        if (!FS_UseSearchPath(search))
            continue;

        FS_BuildOSPath_Internal(search->dir->path, search->dir->gamedir, sanitizedName, ospath, 0);
        file = FS_FileOpen(ospath, "rb");
        if (file) {
            FS_FileClose(file);
            return 0;
        }
    }

    return -1;
}

void FS_DisplayPath(qboolean bLanguageCull)
{
    searchpath_t *s;
    int i;

    Com_Printf("Current language: %s\n", SEH_GetLanguageName(SEH_GetCurrentLanguage()));
    if (fs_ignoreLocalized->current.enabled) {
        Com_Printf("    localized assets are being ignored\n");
    }

    Com_Printf("Current search path:\n");
    for (s = fs_searchpaths; s; s = (searchpath_t *)(uintptr_t)s->next) {
        if (bLanguageCull && !FS_UseSearchPath(s)) {
            continue;
        }

        if (s->pack) {
            qboolean onPureList = 0;

            Com_Printf("%s (%i files)\n", s->pack->iwdFilename, s->pack->numfiles);
            if (s->bLocalized) {
                Com_Printf("    localized assets iwd file for %s\n", SEH_GetLanguageName(s->language));
            }

            for (i = 0; i < fs_numServerIwds; i++) {
                if (s->pack->checksum == fs_serverIwds[i]) {
                    onPureList = 1;
                    break;
                }
            }
            Com_Printf(onPureList ? "    on the pure list\n" : "    not on the pure list\n");
        } else if (s->dir) {
            Com_Printf("%s/%s\n", s->dir->path, s->dir->gamedir);
            if (s->bLocalized) {
                Com_Printf("    localized assets game folder for %s\n", SEH_GetLanguageName(s->language));
            }
        }
    }

    Com_Printf("\nFile Handles:\n");
    for (i = 1; i < 74; i++) {
        if (*(int *)&fsh[i].handleFiles) {
            Com_Printf("handle %i: %s\n", i, fsh[i].name);
        }
    }
}

void FS_Path_f(void)
{
    FS_DisplayPath(1);
}

void FS_FullPath_f(void)
{
    FS_DisplayPath(0);
}

void FS_Printf(fileHandle_t h, const char *fmt, ...)
{
    int tries;
    FILE *f;
    const byte *buf;
    int remaining;
    char msg[4096];
    va_list args;

    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    remaining = (int)strlen(msg);
    if (!h) {
        return;
    }

    f = fsh[h].handleFiles.file.o;
    buf = (const byte *)msg;

    if (remaining) {
        tries = 0;
        while (remaining) {
            int wrote = (int)FS_FileWrite(buf, 1, remaining, f);
            if (!wrote) {
                if (tries) {
                    return;
                }
                tries = 1;
            }
            if (wrote == -1) {
                return;
            }
            buf += wrote;
            remaining -= wrote;
        }
    }

    if (fsh[h].handleSync) {
        fflush(f);
    }
}

static int FS_ListFilteredFilesDepth(const char *name, int *zpathLen)
{
    int depth;
    int len;
    int lastSep;

    depth = 0;
    len = 0;
    lastSep = 0;

    if (name[0]) {
        do {
            if (name[len] == '/' || name[len] == '\\') {
                ++depth;
                lastSep = len;
            }
            ++len;
        } while (name[len]);
    }

    if (zpathLen) {
        *zpathLen = lastSep;
    }

    if (len == lastSep + 1) {
        --depth;
    }

    return depth;
}

static int FS_ListFilteredFilesPathDepth(const char *path)
{
    int pathDepth;

    pathDepth = FS_ListFilteredFilesDepth(path, NULL);
    if (path[0]) {
        ++pathDepth;
    }

    return pathDepth;
}

static void FS_ListFilteredFilesAdd(const char *name, const char **list, int *nfiles)
{
    int i;

    if (*nfiles == 0xfff) {
        return;
    }

    for (i = 0; i < *nfiles; ++i) {
        if (!I_stricmp(name, list[i])) {
            return;
        }
    }

    list[*nfiles] = CopyStringInternal(name);
    ++*nfiles;
}

static void FS_ListFilteredFilesAddOrCap(const char *name, const char **list, int *nfiles)
{
    if (*nfiles == 0xfff) {
        *nfiles = 0xfff;
        return;
    }

    FS_ListFilteredFilesAdd(name, list, nfiles);
}

static void FS_ListFilteredIwdFile(const char *name, const char *sanitizedPath, int pathLength, int pathDepth, const char *extension, int extensionLen, qboolean isDirSearch, const char *filter, const char **list, int *nfiles)
{
    char trimmedName[256];
    int zpathLen;
    int depth;
    int nameLen;
    int start;

    if (filter) {
        if (Com_FilterPath(filter, name, 0)) {
            FS_ListFilteredFilesAddOrCap(name, list, nfiles);
        }
        return;
    }

    depth = FS_ListFilteredFilesDepth(name, &zpathLen);
    if (depth != pathDepth) {
        return;
    }

    if (pathLength > zpathLen) {
        return;
    }

    if (pathLength > 0 && name[pathLength] != '/') {
        return;
    }

    if (I_strnicmp(name, sanitizedPath, pathLength)) {
        return;
    }

    nameLen = (int)strlen(name);

    if (isDirSearch) {
        if (nameLen <= 0 || name[nameLen - 1] != '/') {
            return;
        }
    } else if (extensionLen) {
        if (extensionLen >= nameLen) {
            return;
        }
        if (name[nameLen - extensionLen - 1] != '.') {
            return;
        }
        if (I_stricmp(name + nameLen - extensionLen, extension)) {
            return;
        }
    }

    start = pathLength ? pathLength + 1 : 0;

    if (isDirSearch) {
        strcpy(trimmedName, name + start);
        nameLen = (int)strlen(trimmedName);
        if (nameLen > 0) {
            trimmedName[nameLen - 1] = '\0';
        }
        FS_ListFilteredFilesAddOrCap(trimmedName, list, nfiles);
    } else {
        FS_ListFilteredFilesAddOrCap(name + start, list, nfiles);
    }
}

const char **FS_ListFilteredFiles(searchpath_t *searchPath, const char *path, const char *extension, const char *filter, FsListBehavior behavior, int *numfiles, int allocTrackType)
{
    char sanitizedPath[256];
    char ospath[256];
    const char *list[4096];
    searchpath_t *search;
    int pathLength;
    int pathDepth;
    int extensionLen;
    qboolean isDirSearch;
    int nfiles;
    const char **filelist;
    int i;

    (void)allocTrackType;

    if (!path) {
        *numfiles = 0;
        return NULL;
    }

    if (!extension) {
        extension = "";
    }

    if (!FS_SanitizeFilename(path, sanitizedPath, sizeof(sanitizedPath))) {
        *numfiles = 0;
        return NULL;
    }

    isDirSearch = !I_stricmp(extension, "/");

    pathLength = (int)strlen(sanitizedPath);
    if (pathLength > 0 && (sanitizedPath[pathLength - 1] == '\\' || sanitizedPath[pathLength - 1] == '/')) {
        --pathLength;
    }

    extensionLen = (int)strlen(extension);
    pathDepth = FS_ListFilteredFilesPathDepth(sanitizedPath);

    nfiles = 0;
    for (search = searchPath; search; search = (searchpath_t *)(uintptr_t)search->next) {
        if (!FS_UseSearchPath(search)) {
            continue;
        }

        if (search->pack) {
            pack_t *pack;
            fileInPack_t *buildBuffer;

            pack = search->pack;
            if (!search->bLocalized && fs_numServerIwds > 0) {
                qboolean pure;

                pure = 0;
                for (i = 0; i < fs_numServerIwds; ++i) {

                    if (fs_serverIwds[i] == pack->checksum) {
                        pure = 1;
                        break;
                    }
                }
                if (!pure) {
                    continue;
                }
            }

            buildBuffer = pack->buildBuffer;
            for (i = 0; i < pack->numfiles; ++i) {
                FS_ListFilteredIwdFile(buildBuffer[i].name, sanitizedPath, pathLength, pathDepth, extension, extensionLen, isDirSearch, filter, list, &nfiles);
            }
        } else if (search->dir) {
            char **sysFiles;
            int numSysFiles;

            if ((fs_restrict->current.enabled || fs_numServerIwds) && behavior == FS_LIST_PURE_ONLY) {
                continue;
            }

            FS_BuildOSPath_Internal(search->dir->path, search->dir->gamedir, sanitizedPath, ospath, 0);
            sysFiles = Sys_ListFiles(ospath, extension, filter, &numSysFiles, isDirSearch);

            for (i = 0; i < numSysFiles; ++i) {
                FS_ListFilteredFilesAddOrCap(sysFiles[i], list, &nfiles);
            }

            Sys_FreeFileList(sysFiles);
        }
    }

    *numfiles = nfiles;
    if (!nfiles) {
        return NULL;
    }

    filelist = (const char **)Z_MallocInternal((nfiles + 1) * sizeof(*filelist));
    for (i = 0; i < nfiles; ++i) {
        filelist[i] = list[i];
    }
    filelist[nfiles] = NULL;

    return filelist;
}

const char **FS_ListFiles(const char *path, const char *extension, FsListBehavior behavior, int *numfiles, int allocTrackType)
{
    return FS_ListFilteredFiles(fs_searchpaths, path, extension, NULL, behavior, numfiles, allocTrackType);
}

int FS_GetFileList(const char *path, const char *extension, FsListBehavior behavior, char *listbuf, int bufsize)
{
    int fileCount;
    int i;
    int nTotal;
    const char **fileNames;

    listbuf[0] = '\0';
    fileCount = 0;

    if (!I_stricmp(path, "$modlist")) {
        return FS_GetModList(listbuf, bufsize);
    }

    fileNames = FS_ListFilteredFiles(fs_searchpaths, path, extension, NULL, behavior, &fileCount, 3);
    nTotal = 0;

    for (i = 0; i < fileCount; i++) {
        int len = (int)strlen(fileNames[i]) + 1;
        nTotal += len;
        if (nTotal + 1 >= bufsize) {
            fileCount = i;
            break;
        }
        strcpy(listbuf, fileNames[i]);
        listbuf += len;
    }

    if (fileNames) {
        const char **p;
        for (p = fileNames; *p; p++) {
            Z_FreeInternal((void *)*p);
        }
        Z_FreeInternal((void *)fileNames);
    }
    return fileCount;
}

static int FS_IwdLanguageChar(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static const char *FS_ExtractIwdLanguageName(const char *iwdName)
{
    int slot = iString ^ 1;
    int i;

    iString = slot;
    if (strlen(iwdName) <= 9) {
        szIwdLanguageName[slot][0] = '\0';
        return szIwdLanguageName[slot];
    }

    memset(szIwdLanguageName[slot], 0, sizeof(szIwdLanguageName[slot]));
    for (i = 0; i < 54 && iwdName[i + 10] && FS_IwdLanguageChar((unsigned char)iwdName[i + 10]); ++i) {
        szIwdLanguageName[slot][i] = iwdName[i + 10];
    }

    return szIwdLanguageName[slot];
}

static int FS_SortFileCompare(const char *s1, const char *s2)
{
    int c1;
    int c2;

    do {
        c1 = (signed char)*s1++;
        c2 = (signed char)*s2++;

        if (I_islower(c1))
            c1 -= 0x20;
        if (I_islower(c2))
            c2 -= 0x20;

        if (c1 == '\\' || c1 == ':')
            c1 = '/';
        if (c2 == '\\' || c2 == ':')
            c2 = '/';

        if (c1 < c2)
            return -1;
        if (c1 > c2)
            return 1;
    } while (c1);

    return 0;
}

void FS_SortFileList(const char **filelist, int numfiles)
{
    const char **sortedlist;
    int numsortedfiles;

    sortedlist = (const char **)Z_MallocInternal((numfiles + 1) * sizeof(*sortedlist));
    sortedlist[0] = NULL;

    for (numsortedfiles = 0; numsortedfiles < numfiles; numsortedfiles++) {
        const char *s1 = filelist[numsortedfiles];
        int j;

        for (j = 0; j < numsortedfiles; j++) {
            if (FS_SortFileCompare(s1, sortedlist[j]) < 0) {
                break;
            }
        }

        if (j < numsortedfiles) {
            int k;
            for (k = numsortedfiles; k > j; k--) {
                sortedlist[k] = sortedlist[k - 1];
            }
        }
        sortedlist[j] = s1;
    }

    Com_Memcpy(filelist, sortedlist, numfiles * sizeof(*filelist));
    Z_FreeInternal(sortedlist);
}

fileHandle_t FS_OpenFileOverwrite(const char *qpath)
{
    char ospath[280];
    DWORD attrs;
    DWORD writableAttrs;
    FILE *fp;
    fileHandle_t f;
    fileHandleData_t *entry;

    if (FS_GetFileOsPath(qpath, ospath) < 0) {
        Com_Error(1, "FS_FOpenFileOverWrite: Failed to open %s for writing.  It either does not exist or is in a iwd file.", qpath);
        return 0;
    }

    if (fs_debug->current.integer) {
        Com_Printf("FS_FOpenFileOverWrite: %s\n", ospath);
    }

    attrs = GetFileAttributesA(ospath);
    writableAttrs = attrs & ~1u;
    if (writableAttrs != attrs) {
        SetFileAttributesA(ospath, writableAttrs);
    }

    fp = FS_FileOpen(ospath, "wb");
    if (!fp) {
        return 0;
    }

    for (f = 1; fsh[f].handleFiles.file.o; f++) {
        if (f == 60) {
            int i;
            for (i = 1; i < 74; i++) {
                Com_Printf((const char *)"FILE %2i: '%s'\n", i, fsh[i].name);
            }
            Com_Error(1, (const char *)"\x15"
                                       "FS_HandleForFile: none free");
            f = -1;
            break;
        }
    }
    entry = &fsh[f];
    entry->zipFile = NULL;
    entry->handleFiles.file.o = fp;
    I_strncpyz(entry->name, qpath, sizeof(entry->name));
    entry->handleSync = 0;

    return f;
}

static int iwdsort(const float *a, const float *b)
{
    const char *aa = *(const char *const *)a;
    const char *bb = *(const char *const *)b;

    if (!I_strncmp(aa, "          ", 10) && !I_strncmp(bb, "          ", 10)) {
        const char *pszLanguageA = FS_ExtractIwdLanguageName(aa);
        const char *pszLanguageB = FS_ExtractIwdLanguageName(bb);
        int aIsEnglish = !I_stricmp(pszLanguageA, "english");
        int bIsEnglish = !I_stricmp(pszLanguageB, "english");

        if (aIsEnglish && !bIsEnglish) {
            return -1;
        }
        if (!aIsEnglish && bIsEnglish) {
            return 1;
        }
    }

    return FS_SortFileCompare(aa, bb);
}

static void FS_AddSearchPathIwd(searchpath_t *search)
{
    searchpath_t **insert;

    if (!search->bLocalized) {
        search->next = (intptr_t)fs_searchpaths;
        fs_searchpaths = search;
        return;
    }

    insert = &fs_searchpaths;
    while (*insert && !(*insert)->bLocalized) {
        insert = (searchpath_t **)&(*insert)->next;
    }

    search->next = (intptr_t)*insert;
    *insert = search;
}

static void FS_PrintSupportedLanguagesOnce(void)
{
    int iSuppLang;

    if (bLanguagesListed) {
        return;
    }

    Com_Printf("Supported languages are:\n");
    for (iSuppLang = 0; iSuppLang < 14; iSuppLang++) {
        Com_Printf("    %s\n", SEH_GetLanguageName(iSuppLang));
    }
    bLanguagesListed = 1;
}

static void FS_GetIwdLanguageName(const char *pszIwdFileName, char *outName)
{
    int len;
    const char *src;

    outName[0] = '\0';

    if (strlen(pszIwdFileName) <= 9) {
        return;
    }

    memset(outName, 0, 64);
    src = pszIwdFileName + 10;
    for (len = 0; src[len] && isalpha((unsigned char)src[len]) && len != 54; len++) {
        outName[len] = src[len];
    }
}

static pack_t *FS_LoadIwd(const char *iwdfile, const char *basename, const char *pszGameFolder)
{
    char filename_inzip[256];
    unz_file_info file_info;
    unz_global_info gi;
    unzFile uf;
    pack_t *iwd;
    fileInPack_t *buildBuffer;
    fileInPack_t *file;
    fileInPack_t **namePtr;
    int *fs_headerLongs;
    int fs_numHeaderLongs;
    int len;
    int hashSize;
    unsigned int i;

    uf = unzOpen(iwdfile);
    if (unzGetGlobalInfo(uf, &gi)) {
        return NULL;
    }

    fs_packFiles += gi.number_entry;
    unzGoToFirstFile(uf);

    len = 0;
    for (i = 0; i < gi.number_entry; i++) {
        if (!unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), 0, 0, 0, 0)) {
            len += strlen(filename_inzip) + 1;
        }
        unzGoToNextFile(uf);
    }

    buildBuffer = (fileInPack_t *)Z_MallocInternal(gi.number_entry * sizeof(fileInPack_t) + len);
    namePtr = (fileInPack_t **)((char *)buildBuffer + gi.number_entry * sizeof(fileInPack_t));
    fs_headerLongs = (int *)Z_MallocInternal(gi.number_entry * sizeof(int));

    for (hashSize = 1, i = 11; i && hashSize <= (int)gi.number_entry; i--) {
        hashSize <<= 1;
    }

    iwd = (pack_t *)Z_MallocInternal(sizeof(pack_t) + hashSize * sizeof(fileInPack_t *));
    iwd->hashSize = hashSize;
    iwd->hashTable = (fileInPack_t **)((char *)iwd + sizeof(pack_t));
    for (i = 0; i < (unsigned int)iwd->hashSize; i++) {
        iwd->hashTable[i] = NULL;
    }

    I_strncpyz(iwd->iwdFilename, iwdfile, sizeof(iwd->iwdFilename));
    I_strncpyz(iwd->iwdBasename, basename, sizeof(iwd->iwdBasename));
    len = strlen(iwd->iwdBasename);
    if (len > 4 && !I_stricmp(iwd->iwdBasename + len - 4, ".iwd")) {
        iwd->iwdBasename[len - 4] = '\0';
    }

    iwd->handle = uf;
    iwd->numfiles = gi.number_entry;
    unzGoToFirstFile(uf);

    file = buildBuffer;
    fs_numHeaderLongs = 0;
    for (i = 0; i < gi.number_entry; i++, file++) {
        if (unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), 0, 0, 0, 0)) {
            break;
        }

        if (file_info.uncompressed_size) {
            fs_headerLongs[fs_numHeaderLongs++] = file_info.crc;
        }

        I_strlwr(filename_inzip);
        file->name = (char *)namePtr;
        strcpy((char *)namePtr, filename_inzip);
        namePtr = (fileInPack_t **)((char *)namePtr + strlen(filename_inzip) + 1);
        unzGetCurrentFileInfoPosition(uf, &file->pos);

        hashSize = FS_HashFileName(filename_inzip, iwd->hashSize);
        file->next = (intptr_t)iwd->hashTable[hashSize];
        iwd->hashTable[hashSize] = file;

        unzGoToNextFile(uf);
    }

    iwd->checksum = Com_BlockChecksum(fs_headerLongs, fs_numHeaderLongs * sizeof(int));
    iwd->pure_checksum = Com_BlockChecksumKey(fs_headerLongs, fs_numHeaderLongs * sizeof(int), fs_checksumFeed);
    Z_FreeInternal(fs_headerLongs);
    iwd->buildBuffer = buildBuffer;
    strcpy(iwd->iwdGamename, pszGameFolder);
    return iwd;
}

static void FS_AddIwdFilesForGameDirectory(const char *path, const char *pszGameFolder)
{
    char iwdfile[256];
    char *sorted[1024];
    char **iwdfiles;
    char *pszIwdFileName;
    char *basename;
    int numfiles;
    int i;

    FS_BuildOSPath_Internal(path, pszGameFolder, "", iwdfile, 0);
    iwdfile[strlen(iwdfile) - 1] = '\0';

    iwdfiles = Sys_ListFiles(iwdfile, "iwd", NULL, &numfiles, 0);
    if (numfiles > 1024) {
        Com_Printf("WARNING: Exceeded max number of iwd files in %s/%s (%1/%1)\n", path, pszGameFolder, numfiles, 1024);
        numfiles = 1024;
    }

    for (i = 0; i < numfiles; i++) {
        sorted[i] = iwdfiles[i];
        if (!I_strncmp(sorted[i], "localized_", 10)) {
            memset(sorted[i], ' ', 10);
        }
    }

    qsort(sorted, numfiles, sizeof(sorted[0]), (int (*)(const void *, const void *))iwdsort);

    for (i = 0; i < numfiles; i++) {
        qboolean bIwdLocalized = 0;
        int iIwdLanguage = 0;
        pack_t *iwd;
        searchpath_t *search;

        pszIwdFileName = sorted[i];
        if (!I_strncmp(pszIwdFileName, "          ", 10)) {
            char *languageName;

            memcpy(pszIwdFileName, "localized_", 10);
            languageName = szIwdLanguageName[iString ^ 1];
            iString ^= 1;
            FS_GetIwdLanguageName(pszIwdFileName, languageName);

            if (!languageName[0]) {
                Com_Printf("WARNING: Localized assets iwd file %s/%s/%s has invalid name (no language specified). Proper naming convention is: localized_[language]_iwd#.iwd\n", path, pszGameFolder, pszIwdFileName);
                continue;
            }

            if (!SEH_GetLanguageIndexForName(languageName, &iIwdLanguage)) {
                Com_Printf("WARNING: Localized assets iwd file %s/%s/%s has invalid name (bad language name specified). Proper naming convention is: localized_[language]_iwd#.iwd\n", path, pszGameFolder, pszIwdFileName);
                FS_PrintSupportedLanguagesOnce();
                continue;
            }

            bIwdLocalized = 1;
        }

        FS_BuildOSPath_Internal(path, pszGameFolder, pszIwdFileName, iwdfile, 0);
        basename = pszIwdFileName;
        iwd = FS_LoadIwd(iwdfile, basename, pszGameFolder);
        if (!iwd) {
            continue;
        }

        search = (searchpath_t *)Z_MallocInternal(sizeof(searchpath_t));
        search->pack = iwd;
        search->dir = NULL;
        search->bLocalized = bIwdLocalized;
        search->language = iIwdLanguage;
        FS_AddSearchPathIwd(search);
    }

    Sys_FreeFileList(iwdfiles);
}

static void __attribute_regparm__(3)
    FS_AddGameDirectory(const char *path, const char *dir, qboolean bLanguageDirectory, int iLanguage)
{
    char szGameFolder[64];
    char ospath[256];
    searchpath_t *search;
    searchpath_t **insert;

    if (bLanguageDirectory) {
        Com_sprintf(szGameFolder, sizeof(szGameFolder), "%s/%s", dir, SEH_GetLanguageName(iLanguage));
    } else {
        I_strncpyz(szGameFolder, dir, sizeof(szGameFolder));
    }

    for (search = fs_searchpaths; search; search = *(searchpath_t **)&search->next) {
        if (!search->dir) {
            continue;
        }

        if (I_stricmp(search->dir->path, path)) {
            continue;
        }

        if (I_stricmp(search->dir->gamedir, szGameFolder)) {
            continue;
        }

        if (search->bLocalized != bLanguageDirectory) {
            Com_Printf("WARNING: game folder %s/%s added as both localized & non-localized. Using folder as %s\n", path, szGameFolder, search->bLocalized ? "localized" : "non-localized");
        }

        if (search->bLocalized && search->language != iLanguage) {
            Com_Printf("WARNING: game golder %s/%s re-added as localized folder with different language\n", path, szGameFolder);
        }
        return;
    }

    if (bLanguageDirectory) {
        FS_BuildOSPath_Internal(path, szGameFolder, "", ospath, 0);
        ospath[strlen(ospath) - 1] = '\0';
        if (!Sys_DirectoryHasContents(ospath)) {
            return;
        }
    } else {
        I_strncpyz(fs_gamedir, szGameFolder, sizeof(fs_gamedir));
    }

    search = (searchpath_t *)Z_MallocInternal(sizeof(*search));
    search->dir = (directory_t *)Z_MallocInternal(sizeof(*search->dir));
    I_strncpyz(search->dir->path, path, sizeof(search->dir->path));
    I_strncpyz(search->dir->gamedir, szGameFolder, sizeof(search->dir->gamedir));
    search->bLocalized = bLanguageDirectory;
    search->language = iLanguage;

    if (bLanguageDirectory && fs_searchpaths && !fs_searchpaths->bLocalized) {
        for (insert = &fs_searchpaths; *insert && !(*insert)->bLocalized; insert = (searchpath_t **)&(*insert)->next) {
            ;
        }
    } else {
        insert = &fs_searchpaths;
    }

    search->next = (intptr_t)*insert;
    *insert = search;

    FS_AddIwdFilesForGameDirectory(path, szGameFolder);
}

void FS_CopyFile(char *fromOSPath, char *toOSPath)
{
    FILE *in;
    FILE *out;
    void *buffer;
    int len;

    in = FS_FileOpen(fromOSPath, "rb");
    if (!in) {
        return;
    }

    FS_FileSeek(in, 0, 2);
    len = ftell(in);
    FS_FileSeek(in, 0, 0);

    buffer = malloc(len);
    if ((int)FS_FileRead(buffer, 1, len, in) != len) {
        Com_Error(0, "\x15Short read in FS_CopyFile()\n");
    }
    FS_FileClose(in);

    if (!FS_CreatePath(toOSPath)) {
        out = FS_FileOpen(toOSPath, "wb");
        if (out) {
            if ((int)FS_FileWrite(buffer, 1, len, out) != len) {
                Com_Error(0, "\x15Short write in FS_CopyFile()\n");
            }
            FS_FileClose(out);
        }
    }

    free(buffer);
}

static __attribute_regparm__(3) int FS_FOpenFileRead_Internal(const char *filename, fileHandle_t *file, qboolean uniqueFILE, qboolean streamThread, qboolean fsOnly)
{
    char sanitizedName[256];
    char netpath[256];
    const char *extension;
    searchpath_t *search;
    pack_t *impureIwd;
    qboolean wasSkipped;

    if (!FS_SanitizeFilename(filename, sanitizedName, sizeof(sanitizedName))) {
        if (file) {
            *file = 0;
        }
        return -1;
    }

    if (!file) {
        for (search = fs_searchpaths; search; search = (searchpath_t *)(uintptr_t)search->next) {
            if (!FS_UseSearchPath(search)) {
                continue;
            }

            if (search->pack) {
                pack_t *iwd = search->pack;
                fileInPack_t *iwdFile;
                long hash = FS_HashFileName(sanitizedName, iwd->hashSize);

                for (iwdFile = iwd->hashTable[hash]; iwdFile; iwdFile = (fileInPack_t *)(uintptr_t)iwdFile->next) {
                    if (!FS_FilenameCompare(iwdFile->name, sanitizedName)) {
                        return 1;
                    }
                }
            }

            if (search->dir) {
                FILE *handle;

                FS_BuildOSPath_Internal(search->dir->path, search->dir->gamedir, sanitizedName, netpath, streamThread);
                handle = FS_FileOpen(netpath, "rb");
                if (handle) {
                    FS_FileClose(handle);
                    return 1;
                }
            }
        }

        return -1;
    }

    *file = FS_HandleForFile(streamThread);
    fsh[*file].handleFiles.unique = uniqueFILE;

    impureIwd = NULL;
    wasSkipped = 0;
    extension = Com_GetExtensionSubString(sanitizedName);

    for (search = fs_searchpaths; search; search = (searchpath_t *)(uintptr_t)search->next) {
        if (!FS_UseSearchPath(search)) {
            continue;
        }

        if (search->pack && !fsOnly) {
            pack_t *iwd = search->pack;
            fileInPack_t *iwdFile;
            long hash = FS_HashFileName(sanitizedName, iwd->hashSize);

            for (iwdFile = iwd->hashTable[hash]; iwdFile; iwdFile = (fileInPack_t *)(uintptr_t)iwdFile->next) {
                if (FS_FilenameCompare(iwdFile->name, sanitizedName)) {
                    continue;
                }

                if (!search->bLocalized && fs_numServerIwds) {
                    int i;
                    qboolean pureMatch = 0;

                    for (i = 0; i < fs_numServerIwds; ++i) {
                        if (iwd->checksum == fs_serverIwds[i]) {
                            pureMatch = 1;
                            break;
                        }
                    }

                    if (!pureMatch) {
                        impureIwd = iwd;
                        goto next_searchpath;
                    }
                }

                if (!iwd->referenced) {
                    static const char *const ignoredReferenceExts[] = {
                        ".hlsl", ".txt", ".cfg", ".levelshots", ".menu", ".arena", ".str", ""
                    };
                    const char *const *ext;
                    size_t nameLen = strlen(sanitizedName);
                    qboolean ignoredReference = 0;

                    for (ext = ignoredReferenceExts; (*ext)[0]; ++ext) {
                        size_t extLen = strlen(*ext);
                        if (nameLen >= extLen && !I_stricmp(sanitizedName + nameLen - extLen, *ext)) {
                            ignoredReference = 1;
                            break;
                        }
                    }

                    if (!ignoredReference) {
                        iwd->referenced = 1;
                    }
                }

                if (uniqueFILE) {
                    fsh[*file].handleFiles.file.z = unzReOpen(iwd->iwdFilename, iwd->handle);
                    if (!fsh[*file].handleFiles.file.z) {
                        if (streamThread) {
                            FS_FCloseFile(*file);
                            *file = 0;
                            return -1;
                        }
                        Com_Error(0, "\x15"
                                     "Couldn't reopen %s",
                                  iwd->iwdFilename);
                    }
                } else {
                    fsh[*file].handleFiles.file.z = iwd->handle;
                }

                I_strncpyz(fsh[*file].name, sanitizedName, sizeof(fsh[*file].name));
                fsh[*file].zipFile = iwd;
                fsh[*file].zipFilePos = (int)iwdFile->pos;
                unzSetCurrentFileInfoPosition(fsh[*file].handleFiles.file.z, iwdFile->pos);
                unzOpenCurrentFile(fsh[*file].handleFiles.file.z);

                if (fs_debug->current.integer && !streamThread) {
                    Com_Printf("FS_FOpenFileRead: %s (found in '%s')\n", sanitizedName, iwd->iwdFilename);
                }

                return FS_filelength(*file);
            }
        }

        if (search->dir) {
            FILE *handle;

            if (fs_restrict->current.enabled || fs_numServerIwds) {
                if (!search->bLocalized && !FS_PureIgnoresExtension(extension)) {
                    if (wasSkipped) {
                        continue;
                    }

                    FS_BuildOSPath_Internal(search->dir->path, search->dir->gamedir, sanitizedName, netpath, streamThread);
                    handle = FS_FileOpen(netpath, "rb");
                    if (!handle) {
                        continue;
                    }
                    FS_FileClose(handle);
                    wasSkipped = 1;
                    continue;
                }
            }

            FS_BuildOSPath_Internal(search->dir->path, search->dir->gamedir, sanitizedName, netpath, streamThread);
            handle = FS_FileOpen(netpath, "rb");
            fsh[*file].handleFiles.file.o = handle;
            if (!handle) {
                continue;
            }

            if (!search->bLocalized && !FS_PureIgnoresExtension(extension)) {
                fs_fakeChkSum = rand() + 1;
            }

            I_strncpyz(fsh[*file].name, sanitizedName, sizeof(fsh[*file].name));
            fsh[*file].zipFile = NULL;

            if (fs_debug->current.integer && !streamThread) {
                Com_Printf("FS_FOpenFileRead: %s (found in '%s/%s')\n", sanitizedName, search->dir->path, search->dir->gamedir);
            }

            if (fs_copyfiles->current.enabled && !I_stricmp(search->dir->path, fs_cdpath->current.string)) {
                char copyPath[256];

                FS_BuildOSPath_Internal(fs_basepath->current.string, search->dir->gamedir, sanitizedName, copyPath, streamThread);
                FS_CopyFile(netpath, copyPath);
            }

            return FS_filelength(*file);
        }

    next_searchpath:;
    }

    if (fs_debug->current.integer && !streamThread) {
        Com_Printf("Can't find %s\n", filename);
    }

    *file = 0;

    if (impureIwd) {
        Com_Error(1, va("EXE_UNPURECLIENTDETECTED\x15\n%s", impureIwd->iwdFilename));
    }

    if (wasSkipped) {
        return -2;
    }

    return -1;
}

int FS_FOpenFileRead(const char *filename, fileHandle_t *file, qboolean uniqueFILE)
{
    com_fileAccessed = 1;

    if (__builtin_expect(FS_PureIgnoresExtension(Com_GetExtensionSubString(filename)) != 0, 0)) {
        int len = FS_FOpenFileRead_Internal(filename, file, uniqueFILE, 0, 1);
        if ((unsigned int)(len + 2) > 1) {
            return len;
        }
    }

    if (filename && filename[0] == '$') {
        char fontPath[256];
        const char *base = (I_stricmp(filename, "$font") == 0) ? "normalFont" : filename + 1;
        Com_sprintf(fontPath, sizeof(fontPath), "fonts/%s", base);
        return FS_FOpenFileRead_Internal(fontPath, file, uniqueFILE, 0, 0);
    }

    return FS_FOpenFileRead_Internal(filename, file, uniqueFILE, 0, 0);
}

int FS_FOpenFileReadStream(const char *filename, fileHandle_t *file, qboolean uniqueFILE)
{
    return FS_FOpenFileRead_Internal(filename, file, uniqueFILE, 1, 0);
}

qboolean FS_TouchFile(const char *name)
{
    fileHandle_t f;

    com_fileAccessed = 1;

    if (FS_PureIgnoresExtension(Com_GetExtensionSubString(name)) != 0) {
        int len = FS_FOpenFileRead_Internal(name, &f, 0, 0, 1);
        if ((unsigned int)(len + 2) > 1) {
            goto opened;
        }
    }

    FS_FOpenFileRead_Internal(name, &f, 0, 0, 0);

opened:
    if (f) {
        FS_FCloseFile(f);
        return 1;
    }

    return 0;
}

int FS_ReadFile(const char *qpath, void **buffer)
{
    fileHandle_t h;
    int len;

    if (!qpath || !qpath[0]) {
        Com_Error(0, "\x15"
                     "FS_ReadFile with empty name\n");
    }

    com_fileAccessed = 1;
    if (FS_PureIgnoresExtension(Com_GetExtensionSubString(qpath))) {
        len = FS_FOpenFileRead_Internal(qpath, &h, 0, 0, 1);
        if ((unsigned int)(len + 2) <= 1) {
            len = FS_FOpenFileRead_Internal(qpath, &h, 0, 0, 0);
        }
    } else {
        len = FS_FOpenFileRead_Internal(qpath, &h, 0, 0, 0);
    }

    if (!h) {
        if (buffer) {
            *buffer = NULL;
        }
        return -1;
    }

    if (buffer) {
        byte *buf;

        ++fs_loadStack;
        buf = (byte *)Hunk_AllocateTempMemoryInternal(len + 1);
        *buffer = buf;
        FS_Read(buf, len, h);
        buf[len] = '\0';
    }

    FS_FCloseFile(h);
    return len;
}

fileHandle_t FS_FOpenFileWrite(const char *filename)
{
    char ospath[280];
    FILE *fp;
    fileHandle_t f;
    fileHandleData_t *entry;

    FS_BuildOSPath_Internal(fs_homepath->current.string, fs_gamedir, filename, ospath, 0);

    if (fs_debug->current.integer) {
        Com_Printf("FS_FOpenFileWrite: %s\n", ospath);
    }

    if (FS_CreatePath(ospath)) {
        return 0;
    }

    fp = FS_FileOpen(ospath, "wb");
    if (!fp) {
        return 0;
    }

    f = FS_HandleForFile(0);
    entry = &fsh[f];
    entry->zipFile = NULL;
    entry->handleFiles.file.o = fp;
    I_strncpyz(entry->name, filename, sizeof(entry->name));
    entry->handleSync = 0;

    return f;
}

qboolean FS_WriteFile(const char *qpath, const void *buffer, int size)
{
    fileHandle_t h;
    int actualSize;

    h = FS_FOpenFileWrite(qpath);
    if (!h) {
        Com_Printf("Failed to open %s\n", qpath);
        return 0;
    }

    {
        fileHandleData_t *entry = &fsh[h];
        FILE *f = entry->handleFiles.file.o;
        const byte *buf = (const byte *)buffer;
        int remaining;
        int tries;

        actualSize = size;
        if (size) {
            remaining = size;
            tries = 0;
            while (remaining) {
                int wrote = (int)FS_FileWrite(buf, 1, remaining, f);
                if (!wrote) {
                    if (tries) {
                        actualSize = 0;
                        break;
                    }
                    tries = 1;
                    continue;
                }
                if (wrote == -1) {
                    actualSize = 0;
                    break;
                }
                buf += wrote;
                remaining -= wrote;
            }
        }

        if (entry->handleSync) {
            fflush(f);
        }
    }

    FS_FCloseFile(h);

    if (actualSize != size) {
        if (qpath[0]) {
            char ospath[280];

            FS_BuildOSPath_Internal(fs_homepath->current.string, fs_gamedir, qpath, ospath, 0);
            remove(ospath);
        }
        return 0;
    }

    return 1;
}

fileHandle_t FS_FOpenTextFileWrite(const char *filename)
{
    char ospath[280];
    FILE *fp;
    fileHandle_t f;
    fileHandleData_t *entry;

    for (f = 1; fsh[f].handleFiles.file.o; f++) {
        if (f == 60) {
            int i;
            for (i = 1; i < 74; i++) {
                Com_Printf((const char *)"FILE %2i: '%s'\n", i, fsh[i].name);
            }
            Com_Error(1, (const char *)"\x15"
                                       "FS_HandleForFile: none free");
            f = -1;
            break;
        }
    }
    entry = &fsh[f];
    entry->zipFile = NULL;

    FS_BuildOSPath_Internal(fs_homepath->current.string, fs_gamedir, filename, ospath, 0);

    if (fs_debug->current.integer) {
        Com_Printf("FS_FOpenFileWrite: %s\n", ospath);
    }

    if (FS_CreatePath(ospath)) {
        return 0;
    }

    fp = FS_FileOpen(ospath, "wt");
    entry->handleFiles.file.o = fp;
    I_strncpyz(entry->name, filename, sizeof(entry->name));
    entry->handleSync = 0;

    if (!fp) {
        return 0;
    }

    return f;
}

fileHandle_t FS_FOpenFileAppend(const char *filename)
{
    char ospath[280];
    FILE *fp;
    fileHandle_t f;
    fileHandleData_t *entry;

    for (f = 1; fsh[f].handleFiles.file.o; f++) {
        if (f == 60) {
            int i;
            for (i = 1; i < 74; i++) {
                Com_Printf((const char *)"FILE %2i: '%s'\n", i, fsh[i].name);
            }
            Com_Error(1, (const char *)"\x15"
                                       "FS_HandleForFile: none free");
            f = -1;
            break;
        }
    }
    entry = &fsh[f];
    entry->zipFile = NULL;
    I_strncpyz(entry->name, filename, sizeof(entry->name));

    FS_BuildOSPath_Internal(fs_homepath->current.string, fs_gamedir, filename, ospath, 0);

    if (fs_debug->current.integer) {
        Com_Printf("FS_FOpenFileAppend: %s\n", ospath);
    }

    if (FS_CreatePath(ospath)) {
        return 0;
    }

    fp = FS_FileOpen(ospath, "at");
    entry->handleFiles.file.o = fp;
    entry->handleSync = 0;

    if (!fp) {
        return 0;
    }

    return f;
}

int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
    int len;
    qboolean sync;

    switch (mode) {
    case FS_READ:
        com_fileAccessed = 1;
        if (FS_PureIgnoresExtension(Com_GetExtensionSubString(qpath))) {
            len = FS_FOpenFileRead_Internal(qpath, f, 1, 0, 1);
            if ((unsigned int)(len + 2) > 1) {
                sync = 0;
                break;
            }
        }
        len = FS_FOpenFileRead_Internal(qpath, f, 1, 0, 0);
        sync = 0;
        break;

    case FS_WRITE:
        *f = FS_FOpenFileWrite(qpath);
        len = *f ? 0 : -1;
        sync = 0;
        break;

    case FS_APPEND:
        sync = 0;
        *f = FS_FOpenFileAppend(qpath);
        len = (*f < 1) ? -1 : 0;
        break;

    case FS_APPEND_SYNC:
        sync = 1;
        *f = FS_FOpenFileAppend(qpath);
        len = (*f < 1) ? -1 : 0;
        break;

    default:
        Com_Error(0, "\x15"
                     "FSH_FOpenFile: bad mode");
        len = 6969;
        sync = 0;
        break;
    }

    if (f && *f) {
        fileHandleData_t *entry = &fsh[*f];
        entry->fileSize = len;
        entry->streamed = 0;
        entry->handleSync = sync;
    }

    return len;
}

static void FS_AddGameDirectoryAllLanguages(const char *path, const char *dir)
{
    int i;

    for (i = 13; i >= 0; --i) {
        FS_AddGameDirectory(path, dir, 1, i);
    }
    FS_AddGameDirectory(path, dir, 0, 0);
}

static void FS_AddRawGameDirectories(const char *path)
{
    FS_AddGameDirectoryAllLanguages(path, "devraw_shared");
    FS_AddGameDirectoryAllLanguages(path, "devraw");
    FS_AddGameDirectoryAllLanguages(path, "raw_shared");
    FS_AddGameDirectoryAllLanguages(path, "raw");
}

void FS_Startup(const char *gameName)
{
    const char *path;
    const char *dir;

    Com_Printf("----- FS_Startup -----\n");
    FS_RegisterDvars();

    if (fs_useOldAssets->current.enabled) {
        path = fs_basepath->current.string;
        if (*path) {
            FS_AddGameDirectoryAllLanguages(path, "tempcod");
        }

        path = fs_homepath->current.string;
        if (*path) {
            FS_AddGameDirectoryAllLanguages(path, "tempcod");
        }
    }

    path = fs_basepath->current.string;
    if (*path) {
        FS_AddRawGameDirectories(path);
    }

    path = fs_homepath->current.string;
    if (*path) {
        FS_AddRawGameDirectories(path);
    }

    path = fs_cdpath->current.string;
    if (*path) {
        FS_AddRawGameDirectories(path);
        FS_AddGameDirectoryAllLanguages(path, gameName);
    }

    path = fs_basepath->current.string;
    if (*path) {
        FS_AddGameDirectoryAllLanguages(path, gameName);
    }

    dir = fs_basegame->current.string;
    if (*dir && !I_stricmp(gameName, "main") && I_stricmp(dir, gameName)) {
        path = fs_cdpath->current.string;
        if (*path) {
            FS_AddGameDirectoryAllLanguages(path, dir);
        }

        path = fs_basepath->current.string;
        if (*path) {
            FS_AddGameDirectoryAllLanguages(path, dir);
        }

        path = fs_homepath->current.string;
        if (*path && I_stricmp(path, fs_basepath->current.string)) {
            FS_AddGameDirectoryAllLanguages(path, dir);
        }
    }

    dir = fs_gameDirVar->current.string;
    if (*dir && !I_stricmp(gameName, "main") && I_stricmp(dir, gameName)) {
        path = fs_cdpath->current.string;
        if (*path) {
            FS_AddGameDirectoryAllLanguages(path, dir);
        }

        path = fs_basepath->current.string;
        if (*path) {
            FS_AddGameDirectoryAllLanguages(path, dir);
        }

        path = fs_homepath->current.string;
        if (*path && I_stricmp(path, fs_basepath->current.string)) {
            FS_AddGameDirectoryAllLanguages(path, dir);
        }
    }

    Com_ReadCDKey();
    FS_AddCommands();
    FS_DisplayPath(1);
    Dvar_ClearModified(fs_gameDirVar);
    Com_Printf("----------------------\n");
    Com_Printf("%d files in iwd files\n", fs_packFiles);
}

void FS_Restart(int checksumFeed)
{
    searchpath_t *search;

    FS_Shutdown(0);
    fs_checksumFeed = checksumFeed;

    for (search = fs_searchpaths; search; search = *(searchpath_t **)&search->next) {
        if (search->pack) {
            search->pack->referenced = 0;
        }
    }

    FS_Startup("main");
    SEH_Init_StringEd();
    SEH_UpdateLanguageInfo();
    FS_SetRestrictions();

    if (FS_ReadFile("default_mp.cfg", NULL) <= 0) {
        if (lastValidBase[0]) {
            FS_PureServerSetLoadedIwds("", "");
            Dvar_SetString(fs_basepath, lastValidBase);
            Dvar_SetString(fs_gameDirVar, lastValidGame);
            lastValidBase[0] = 0;
            lastValidGame[0] = 0;
            Dvar_SetBool(fs_restrict, 0);
            FS_Restart(checksumFeed);
            Com_Error(1, "Invalid game folder\n");
        }

        Com_Error(0, "Couldn't load %s.  Make sure Call of Duty is run from the correct folder.", "default_mp.cfg");
    }

    if (I_stricmp(fs_gameDirVar->current.string, lastValidGame) && !Com_SafeMode()) {
        Cbuf_AddText(va("exec %s\n", "config_mp.cfg"));
    }

    I_strncpyz(lastValidBase, fs_basepath->current.string, sizeof(lastValidBase));
    I_strncpyz(lastValidGame, fs_gameDirVar->current.string, sizeof(lastValidGame));
}

qboolean FS_ConditionalRestart(int checksumFeed)
{
    const dvar_t *sv_running = *(const dvar_t **)imp_com_sv_running;

    if (sv_running->current.enabled)
        return 0;

    if (fs_gameDirVar->modified || fs_checksumFeed != checksumFeed) {
        FS_Restart(checksumFeed);
        return 1;
    }

    return 0;
}

void FS_InitFilesystem(void)
{
    Com_StartupVariable("fs_cdpath");
    Com_StartupVariable("fs_basepath");
    Com_StartupVariable("fs_homepath");
    Com_StartupVariable("fs_game");
    Com_StartupVariable("fs_copyfiles");
    Com_StartupVariable("fs_restrict");
    Com_StartupVariable("loc_language");

    SEH_InitLanguage();
    FS_Startup("main");
    SEH_Init_StringEd();
    SEH_UpdateLanguageInfo();
    FS_SetRestrictions();

    if (FS_ReadFile("default_mp.cfg", NULL) <= 0) {
        Com_Error(0, "Couldn't load %s.  Make sure Call of Duty is run from the correct folder.", "default_mp.cfg");
    }

    I_strncpyz(lastValidBase, fs_basepath->current.string, sizeof(lastValidBase));
    I_strncpyz(lastValidGame, fs_gameDirVar->current.string, sizeof(lastValidGame));
}
#else
extern void *Hunk_AllocateTempMemoryInternal(int size);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern qboolean Com_SafeMode(void);
extern void Cbuf_AddText(const char *text);
extern void Com_ReadCDKey(void);
extern void FS_AddCommands(void);
extern void Dvar_ClearModified(const dvar_t *dvar);
extern void Dvar_SetString(const dvar_t *dvar, const char *value);
extern void Dvar_SetBool(const dvar_t *dvar, qboolean value);
extern void FS_PureServerSetLoadedIwds(const char *iwdSums, const char *iwdNames);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern int Sys_DirectoryHasContents(const char *path);
extern char **Sys_ListFiles(const char *directory, const char *extension, const char *filter, int *numfiles, qboolean wantsubs);
extern void Sys_FreeFileList(char **list);
extern int I_strncmp(const char *s0, const char *s1, int n);
extern char *I_strlwr(char *s);
extern unzFile unzOpen(const char *path);
extern int unzGetGlobalInfo(unzFile file, unz_global_info *pglobal_info);
extern int unzGetCurrentFileInfo(unzFile file, unz_file_info *pfile_info, char *szFileName, uLong fileNameBufferSize, double *extraField, uLong extraFieldBufferSize, char *szComment, uLong commentBufferSize);
extern int unzGetCurrentFileInfoPosition(unzFile file, long unsigned int *pos);
extern int unzGoToFirstFile(unzFile file);
extern int unzGoToNextFile(unzFile file);
extern unsigned int Com_BlockChecksum(const void *buffer, int length);
extern unsigned int Com_BlockChecksumKey(void *buffer, int length, int key);
extern qboolean SEH_GetLanguageIndexForName(const char *pszLanguageName, int *piLanguageIndex);
#    ifdef __EMSCRIPTEN__
#        include <ctype.h>
#    else
extern int isalpha(int c);
#    endif
extern void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern size_t FS_FileWrite(const void *ptr, size_t size, size_t nitems, FILE *stream);
extern char **Sys_ListFiles(const char *directory, const char *extension, const char *filter, int *numfiles, qboolean wantsubs);
extern void Sys_FreeFileList(char **list);
extern int unzReadCurrentFile(unzFile file, voidp buf, unsigned int len);
extern long int unztell(unzFile file);
extern int unzCloseCurrentFile(unzFile file);
extern int unzOpenCurrentFile(unzFile file);
extern int unzSetCurrentFileInfoPosition(unzFile file, long unsigned int pos);
extern int unzGetCurrentFileInfoPosition(unzFile file, long unsigned int *pos);
extern int unzGetCurrentFileInfo(unzFile file, unz_file_info *pfile_info, char *szFileName, uLong fileNameBufferSize, double *extraField, uLong extraFieldBufferSize, char *szComment, uLong commentBufferSize);
extern int unzGoToFirstFile(unzFile file);
extern int unzGoToNextFile(unzFile file);
extern unzFile unzOpen(const char *path);
extern int unzClose(unzFile file);
extern void Com_StartupVariable(const char *match);
extern void SEH_InitLanguage(void);
extern void SEH_Init_StringEd(void);
extern void SEH_UpdateLanguageInfo(void);
extern void FS_SetRestrictions(void);
extern qboolean SEH_GetLanguageIndexForName(const char *pszLanguageName, int *piLanguageIndex);

typedef struct webIwdFile_s {
    char name[256];
    long unsigned int pos;
    int size;
    struct webIwd_s *iwd;
    struct webIwdFile_s *next;
} webIwdFile_t;

typedef struct webIwd_s {
    char path[256];
    qboolean localized;
    int language;
    webIwdFile_t *files;
    struct webIwd_s *next;
} webIwd_t;

static webIwd_t *fs_webIwds;
static qboolean fs_webStarted;
static qboolean fs_webLanguageSearchPathAdded[14];

static int FS_WebStringCompare(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;

    do {
        ca = (unsigned char)tolower(*a++);
        cb = (unsigned char)tolower(*b++);
        if (ca == '\\') {
            ca = '/';
        }
        if (cb == '\\') {
            cb = '/';
        }
        if (ca != cb) {
            return (int)ca - (int)cb;
        }
    } while (ca && cb);

    return 0;
}

static void FS_WebNormalizePath(char *path)
{
    char *p;

    for (p = path; *p; p++) {
        if (*p == '\\') {
            *p = '/';
        } else {
            *p = (char)tolower(*p);
        }
    }
}

static char *FS_WebCopyString(const char *text)
{
    int len;
    char *copy;

    len = strlen(text) + 1;
    copy = (char *)Z_MallocInternal(len);
    Com_Memcpy(copy, text, len);
    return copy;
}

static qboolean FS_WebAppendListItem(char **items, int *count, const char *name)
{
    int i;

    if (!name || !name[0]) {
        return 0;
    }

    for (i = 0; i < *count; i++) {
        if (!I_stricmp(items[i], name)) {
            return 0;
        }
    }

    if (*count >= 4095) {
        return 0;
    }

    items[*count] = FS_WebCopyString(name);
    (*count)++;
    return 1;
}

static void FS_WebFreePendingListItems(char **items, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        Z_FreeInternal(items[i]);
    }
}

static qboolean FS_WebJoinPath(char *out, int outSize, const char *dir, const char *name)
{
    int written;

    if (!dir || !dir[0] || !strcmp(dir, ".")) {
        written = Com_sprintf(out, outSize, "%s", name);
    } else {
        written = Com_sprintf(out, outSize, "%s/%s", dir, name);
    }

    return written >= 0 && written < outSize;
}

static void FS_WebDetectLocalizedIwd(webIwd_t *iwd, const char *path)
{
    const char *base;
    const char *prefix;
    char languageName[64];
    int len;
    int language;

    iwd->localized = 0;
    iwd->language = 0;

    base = strrchr(path, '/');
    base = base ? base + 1 : path;
    prefix = "localized_";
    if (I_strnicmp(base, prefix, strlen(prefix))) {
        return;
    }

    base += strlen(prefix);
    len = 0;
    while (base[len] && base[len] != '_' && base[len] != '.' && len < (int)sizeof(languageName) - 1) {
        languageName[len] = base[len];
        len++;
    }
    languageName[len] = '\0';

    if (SEH_GetLanguageIndexForName(languageName, &language)) {
        iwd->localized = 1;
        iwd->language = language;
    }
}

static void FS_WebAddLanguageSearchPath(int language)
{
    searchpath_t *search;

    if ((unsigned int)language >= 14 || fs_webLanguageSearchPathAdded[language]) {
        return;
    }

    search = (searchpath_t *)Z_MallocInternal(sizeof(*search));
    Com_Memset(search, 0, sizeof(*search));
    search->next = (intptr_t)(uintptr_t)fs_searchpaths;
    search->bLocalized = 1;
    search->language = language;
    fs_searchpaths = search;
    fs_webLanguageSearchPathAdded[language] = 1;
}

static void FS_WebAddIwd(const char *path)
{
    unzFile uf;
    webIwd_t *iwd;
    int err;

    uf = unzOpen(path);
    if (!uf) {
        return;
    }

    iwd = (webIwd_t *)Z_MallocInternal(sizeof(*iwd));
    Com_Memset(iwd, 0, sizeof(*iwd));
    I_strncpyz(iwd->path, path, sizeof(iwd->path));
    FS_WebDetectLocalizedIwd(iwd, path);

    err = unzGoToFirstFile(uf);
    while (err == 0) {
        unz_file_info info;
        char filename[256];
        long unsigned int pos;

        if (!unzGetCurrentFileInfo(uf, &info, filename, sizeof(filename), NULL, 0, NULL, 0) &&
            !unzGetCurrentFileInfoPosition(uf, &pos)) {
            int len;

            filename[sizeof(filename) - 1] = '\0';
            FS_WebNormalizePath(filename);
            len = strlen(filename);
            if (len > 0 && filename[len - 1] != '/') {
                webIwdFile_t *file = (webIwdFile_t *)Z_MallocInternal(sizeof(*file));
                Com_Memset(file, 0, sizeof(*file));
                I_strncpyz(file->name, filename, sizeof(file->name));
                file->pos = pos;
                file->size = (int)info.uncompressed_size;
                file->iwd = iwd;
                file->next = iwd->files;
                iwd->files = file;
            }
        }

        err = unzGoToNextFile(uf);
    }

    unzClose(uf);

    iwd->next = fs_webIwds;
    fs_webIwds = iwd;

    if (iwd->localized) {
        FS_WebAddLanguageSearchPath(iwd->language);
    }
}

static void FS_WebScanIwdDirectory(const char *dir)
{
    char **files;
    int numFiles;
    int i;

    files = Sys_ListFiles(dir, "iwd", NULL, &numFiles, 0);
    if (!files) {
        return;
    }

    for (i = 0; i < numFiles; i++) {
        char path[512];

        if (FS_WebJoinPath(path, sizeof(path), dir, files[i])) {
            FS_WebAddIwd(path);
        }
    }

    Sys_FreeFileList(files);
}

static void FS_WebShutdownIwds(void)
{
    webIwd_t *iwd;
    int i;

    iwd = fs_webIwds;
    while (iwd) {
        webIwd_t *nextIwd = iwd->next;
        webIwdFile_t *file = iwd->files;

        while (file) {
            webIwdFile_t *nextFile = file->next;
            Z_FreeInternal(file);
            file = nextFile;
        }

        Z_FreeInternal(iwd);
        iwd = nextIwd;
    }

    fs_webIwds = NULL;
    fs_webStarted = 0;
    for (i = 0; i < 14; i++) {
        fs_webLanguageSearchPathAdded[i] = 0;
    }
}

static void FS_WebIndexIwds(void)
{
    const char *game;

    FS_WebShutdownIwds();

    game = fs_gameDirVar && fs_gameDirVar->current.string && fs_gameDirVar->current.string[0]
               ? fs_gameDirVar->current.string
               : fs_gamedir;
    if (!game || !game[0]) {
        game = "main";
    }

    FS_WebScanIwdDirectory(".");
    FS_WebScanIwdDirectory("main");
    if (strcmp(game, "main")) {
        FS_WebScanIwdDirectory(game);
    }

    fs_webStarted = 1;
}

static const webIwdFile_t *FS_WebFindIwdFile(const char *qpath)
{
    char normalized[256];
    webIwd_t *iwd;

    if (!fs_webStarted) {
        FS_WebIndexIwds();
    }

    I_strncpyz(normalized, qpath, sizeof(normalized));
    FS_WebNormalizePath(normalized);

    for (iwd = fs_webIwds; iwd; iwd = iwd->next) {
        webIwdFile_t *file;

        if (iwd->localized) {
            if (fs_ignoreLocalized && fs_ignoreLocalized->current.enabled) {
                continue;
            }
            if (iwd->language != SEH_GetCurrentLanguage()) {
                continue;
            }
        }

        for (file = iwd->files; file; file = file->next) {
            if (!FS_WebStringCompare(file->name, normalized)) {
                return file;
            }
        }
    }

    return NULL;
}

static fileHandleData_t *FS_WebHandle(fileHandle_t h)
{
    if (h <= 0 || h >= 74) {
        return NULL;
    }
    return &fsh[h];
}

static const char *FS_WebDvarString(const dvar_t *dvar, const char *fallback)
{
    if (dvar && dvar->current.string && dvar->current.string[0]) {
        return dvar->current.string;
    }
    return fallback;
}

static qboolean FS_WebPathExists(const char *path)
{
    FILE *file;

    if (!path || !path[0]) {
        return 0;
    }

    file = fopen(path, "rb");
    if (!file) {
        return 0;
    }

    fclose(file);
    return 1;
}

static qboolean FS_WebBuildPath(char *out, int outSize, const char *prefix, const char *qpath)
{
    if (!qpath || !qpath[0]) {
        return 0;
    }

    if (!prefix || !prefix[0] || !strcmp(prefix, ".")) {
        Com_sprintf(out, outSize, "%s", qpath);
    } else if (prefix[strlen(prefix) - 1] == '/') {
        Com_sprintf(out, outSize, "%s%s", prefix, qpath);
    } else {
        Com_sprintf(out, outSize, "%s/%s", prefix, qpath);
    }

    FS_ConvertPath(out);
    return 1;
}

static FILE *FS_WebOpenReadPath(const char *qpath, char *resolved, int resolvedSize)
{
    const char *base;
    const char *home;
    const char *game;
    char candidate[512];
    FILE *file;

    if (!qpath || !qpath[0]) {
        return NULL;
    }

    if (FS_WebBuildPath(candidate, sizeof(candidate), "", qpath)) {
        file = fopen(candidate, "rb");
        if (file) {
            I_strncpyz(resolved, candidate, resolvedSize);
            return file;
        }
    }

    game = fs_gameDirVar && fs_gameDirVar->current.string && fs_gameDirVar->current.string[0]
               ? fs_gameDirVar->current.string
               : fs_gamedir;
    if (!game || !game[0]) {
        game = "main";
    }

    if (FS_WebBuildPath(candidate, sizeof(candidate), game, qpath)) {
        file = fopen(candidate, "rb");
        if (file) {
            I_strncpyz(resolved, candidate, resolvedSize);
            return file;
        }
    }

    if (strcmp(game, "main") && FS_WebBuildPath(candidate, sizeof(candidate), "main", qpath)) {
        file = fopen(candidate, "rb");
        if (file) {
            I_strncpyz(resolved, candidate, resolvedSize);
            return file;
        }
    }

    base = FS_WebDvarString(fs_basepath, "");
    home = FS_WebDvarString(fs_homepath, base);

    if (base[0]) {
        Com_sprintf(candidate, sizeof(candidate), "%s/%s/%s", base, game, qpath);
        FS_ConvertPath(candidate);
        file = fopen(candidate, "rb");
        if (file) {
            I_strncpyz(resolved, candidate, resolvedSize);
            return file;
        }
    }

    if (home[0] && I_stricmp(home, base)) {
        Com_sprintf(candidate, sizeof(candidate), "%s/%s/%s", home, game, qpath);
        FS_ConvertPath(candidate);
        file = fopen(candidate, "rb");
        if (file) {
            I_strncpyz(resolved, candidate, resolvedSize);
            return file;
        }
    }

    return NULL;
}

static int FS_WebFileLength(FILE *file)
{
    long pos;
    long end;

    if (!file) {
        return -1;
    }

    pos = ftell(file);
    if (fseek(file, 0, SEEK_END)) {
        return -1;
    }
    end = ftell(file);
    fseek(file, pos, SEEK_SET);
    return (int)end;
}

static fileHandle_t FS_WebOpenWritePath(const char *qpath, const char *mode)
{
    char ospath[512];
    const char *base;
    fileHandle_t h;
    fileHandleData_t *entry;

    base = FS_WebDvarString(fs_homepath, ".");
    if (!base[0]) {
        base = ".";
    }

    if (!fs_gamedir[0]) {
        I_strncpyz(fs_gamedir, "main", sizeof(fs_gamedir));
    }

    Com_sprintf(ospath, sizeof(ospath), "%s/%s/%s", base, fs_gamedir, qpath);
    FS_ConvertPath(ospath);
    if (FS_CreatePath(ospath)) {
        return 0;
    }

    h = FS_HandleForFile(0);
    if (h <= 0) {
        return 0;
    }

    entry = &fsh[h];
    Com_Memset(entry, 0, sizeof(*entry));
    entry->handleFiles.file.o = fopen(ospath, mode);
    if (!entry->handleFiles.file.o) {
        Com_Memset(entry, 0, sizeof(*entry));
        return 0;
    }

    I_strncpyz(entry->name, qpath, sizeof(entry->name));
    return h;
}

static int FS_FOpenFileRead_Internal(const char *filename, fileHandle_t *file, qboolean uniqueFILE, qboolean streamThread, qboolean fsOnly)
{
    char resolved[512];
    FILE *fp;
    fileHandle_t h;
    fileHandleData_t *entry;
    int len;

    (void)fsOnly;

    if (file) {
        *file = 0;
    }

    if (!filename || !filename[0]) {
        return -1;
    }

    fp = FS_WebOpenReadPath(filename, resolved, sizeof(resolved));
    if (!fp) {
        const webIwdFile_t *iwdFile = FS_WebFindIwdFile(filename);
        unzFile zip;

        if (!iwdFile) {
            return -1;
        }

        h = FS_HandleForFile(streamThread);
        if (h <= 0) {
            return -1;
        }

        zip = unzOpen(iwdFile->iwd->path);
        if (!zip) {
            return -1;
        }
        if (unzSetCurrentFileInfoPosition(zip, iwdFile->pos) ||
            unzOpenCurrentFile(zip)) {
            unzClose(zip);
            return -1;
        }

        entry = &fsh[h];
        Com_Memset(entry, 0, sizeof(*entry));
        entry->handleFiles.file.z = zip;
        entry->handleFiles.unique = 1;
        entry->fileSize = iwdFile->size;
        entry->zipFilePos = (int)iwdFile->pos;
        entry->zipFile = (pack_t *)iwdFile->iwd;
        entry->streamed = streamThread;
        I_strncpyz(entry->name, filename, sizeof(entry->name));

        if (file) {
            *file = h;
        }

        if (fs_debug && fs_debug->current.integer) {
            Com_Printf("FS_FOpenFileRead: %s (found in '%s')\n", filename, iwdFile->iwd->path);
        }

        return iwdFile->size;
    }

    h = FS_HandleForFile(streamThread);
    if (h <= 0) {
        fclose(fp);
        return -1;
    }

    len = FS_WebFileLength(fp);
    entry = &fsh[h];
    Com_Memset(entry, 0, sizeof(*entry));
    entry->handleFiles.file.o = fp;
    entry->handleFiles.unique = uniqueFILE;
    entry->fileSize = len;
    entry->streamed = streamThread;
    I_strncpyz(entry->name, filename, sizeof(entry->name));

    if (file) {
        *file = h;
    }

    if (fs_debug && fs_debug->current.integer) {
        Com_Printf("FS_FOpenFileRead: %s (found as '%s')\n", filename, resolved);
    }

    return len;
}

int FS_Read(void *buffer, int len, fileHandle_t h)
{
    fileHandleData_t *entry;
    byte *buf;
    int remaining;
    int total;

    entry = FS_WebHandle(h);
    if (!entry || !buffer || len <= 0) {
        return 0;
    }

    if (entry->zipFile) {
        return unzReadCurrentFile(entry->handleFiles.file.z, buffer, len);
    }

    buf = (byte *)buffer;
    remaining = len;
    total = 0;

    while (remaining > 0) {
        size_t n = fread(buf + total, 1, remaining, entry->handleFiles.file.o);
        if (!n) {
            if (ferror(entry->handleFiles.file.o)) {
                return total ? total : -1;
            }
            break;
        }
        total += (int)n;
        remaining -= (int)n;
    }

    return total;
}

int FS_Write(const void *buffer, int len, fileHandle_t h)
{
    fileHandleData_t *entry;
    int written;

    entry = FS_WebHandle(h);
    if (!entry || !buffer || len <= 0) {
        return 0;
    }

    written = (int)fwrite(buffer, 1, len, entry->handleFiles.file.o);
    if (entry->handleSync) {
        fflush(entry->handleFiles.file.o);
    }

    return written;
}

int FS_Seek(fileHandle_t f, long int offset, int origin)
{
    fileHandleData_t *entry = FS_WebHandle(f);

    if (!entry) {
        return -1;
    }

    if (entry->zipFile) {
        byte scratch[1024];
        long target;

        if (origin == SEEK_SET) {
            target = offset;
        } else if (origin == SEEK_CUR) {
            target = unztell(entry->handleFiles.file.z) + offset;
        } else if (origin == SEEK_END) {
            target = entry->fileSize + offset;
        } else {
            return -1;
        }

        if (target < 0 || target > entry->fileSize) {
            return -1;
        }

        unzCloseCurrentFile(entry->handleFiles.file.z);
        if (unzSetCurrentFileInfoPosition(entry->handleFiles.file.z, (long unsigned int)entry->zipFilePos) ||
            unzOpenCurrentFile(entry->handleFiles.file.z)) {
            return -1;
        }

        while (target > 0) {
            int chunk = target > (long)sizeof(scratch) ? (int)sizeof(scratch) : (int)target;
            int got = unzReadCurrentFile(entry->handleFiles.file.z, scratch, chunk);

            if (got <= 0) {
                return -1;
            }
            target -= got;
        }

        return 0;
    }

    return fseek(entry->handleFiles.file.o, offset, origin);
}

int FS_FTell(fileHandle_t f)
{
    fileHandleData_t *entry = FS_WebHandle(f);

    if (!entry) {
        return -1;
    }

    if (entry->zipFile) {
        return unztell(entry->handleFiles.file.z);
    }

    return (int)ftell(entry->handleFiles.file.o);
}

void FS_Flush(fileHandle_t f)
{
    fileHandleData_t *entry = FS_WebHandle(f);

    if (entry && !entry->zipFile && entry->handleFiles.file.o) {
        fflush(entry->handleFiles.file.o);
    }

    return 0.0f;
}

void FS_Printf(fileHandle_t h, const char *fmt, ...)
{
    char msg[4096];
    va_list args;
    int len;

    va_start(args, fmt);
    len = vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    if (len < 0) {
        return;
    }
    if (len >= (int)sizeof(msg)) {
        len = (int)sizeof(msg) - 1;
    }

    FS_Write(msg, len, h);
}

static void FS_WebAppendSysList(char **items, int *count, const char *directory, const char *extension, const char *filter, qboolean wantSubs)
{
    char **list;
    int numFiles;
    int i;

    list = Sys_ListFiles(directory ? directory : "", extension, filter, &numFiles, wantSubs);
    if (!list) {
        return;
    }

    for (i = 0; i < numFiles; i++) {
        FS_WebAppendListItem(items, count, list[i]);
    }

    Sys_FreeFileList(list);
}

static qboolean FS_WebIwdListName(const char *entryName, const char *path, const char *extension, qboolean wantDirs, char *out, int outSize)
{
    char normalizedPath[256];
    const char *rest;
    const char *slash;
    int pathLen;

    if (!path) {
        path = "";
    }

    I_strncpyz(normalizedPath, path, sizeof(normalizedPath));
    FS_WebNormalizePath(normalizedPath);
    pathLen = strlen(normalizedPath);
    while (pathLen > 0 && normalizedPath[pathLen - 1] == '/') {
        normalizedPath[--pathLen] = '\0';
    }

    if (pathLen > 0) {
        if (I_strnicmp(entryName, normalizedPath, pathLen) || entryName[pathLen] != '/') {
            return 0;
        }
        rest = entryName + pathLen + 1;
    } else {
        rest = entryName;
    }

    if (!rest[0]) {
        return 0;
    }

    slash = strchr(rest, '/');
    if (wantDirs) {
        int len;

        if (!slash || slash == rest) {
            return 0;
        }

        len = slash - rest;
        if (len >= outSize) {
            len = outSize - 1;
        }
        memcpy(out, rest, len);
        out[len] = '\0';
        return 1;
    }

    if (slash) {
        return 0;
    }

    if (extension && extension[0]) {
        int nameLen = strlen(rest);
        int extLen = strlen(extension);

        if (nameLen < extLen || I_stricmp(rest + nameLen - extLen, extension)) {
            return 0;
        }
    }

    I_strncpyz(out, rest, outSize);
    return 1;
}

static void FS_WebAppendIwdList(char **items, int *count, const char *path, const char *extension)
{
    qboolean wantDirs;
    webIwd_t *iwd;

    if (!fs_webStarted) {
        FS_WebIndexIwds();
    }

    wantDirs = extension && extension[0] == '/' && extension[1] == '\0';
    for (iwd = fs_webIwds; iwd; iwd = iwd->next) {
        webIwdFile_t *file;

        if (iwd->localized) {
            if (fs_ignoreLocalized && fs_ignoreLocalized->current.enabled) {
                continue;
            }
            if (iwd->language != SEH_GetCurrentLanguage()) {
                continue;
            }
        }

        for (file = iwd->files; file; file = file->next) {
            char name[256];

            if (FS_WebIwdListName(file->name, path, extension, wantDirs, name, sizeof(name))) {
                FS_WebAppendListItem(items, count, name);
            }
        }
    }
}

const char **FS_ListFilteredFiles(searchpath_t *searchPath, const char *path, const char *extension, const char *filter, FsListBehavior behavior, int *numfiles, int allocTrackType)
{
    char *items[4096];
    const char **result;
    char candidate[512];
    const char *game;
    int count;
    int i;
    qboolean wantDirs;

    (void)searchPath;
    (void)behavior;
    (void)allocTrackType;

    if (numfiles) {
        *numfiles = 0;
    }

    count = 0;
    wantDirs = extension && extension[0] == '/' && extension[1] == '\0';
    FS_WebAppendSysList(items, &count, path ? path : "", extension, filter, wantDirs);

    game = fs_gameDirVar && fs_gameDirVar->current.string && fs_gameDirVar->current.string[0]
               ? fs_gameDirVar->current.string
               : fs_gamedir;
    if (!game || !game[0]) {
        game = "main";
    }

    Com_sprintf(candidate, sizeof(candidate), "%s/%s", game, path ? path : "");
    FS_ConvertPath(candidate);
    FS_WebAppendSysList(items, &count, candidate, extension, filter, wantDirs);

    if (strcmp(game, "main")) {
        Com_sprintf(candidate, sizeof(candidate), "main/%s", path ? path : "");
        FS_ConvertPath(candidate);
        FS_WebAppendSysList(items, &count, candidate, extension, filter, wantDirs);
    }

    if (!filter) {
        FS_WebAppendIwdList(items, &count, path, extension);
    }

    if (count <= 0) {
        return NULL;
    }

    result = (const char **)Z_MallocInternal((count + 1) * sizeof(*result));
    if (!result) {
        FS_WebFreePendingListItems(items, count);
        return NULL;
    }

    for (i = 0; i < count; i++) {
        result[i] = items[i];
    }
    result[count] = NULL;

    if (numfiles) {
        *numfiles = count;
    }

    return result;
}

const char **FS_ListFiles(const char *path, const char *extension, FsListBehavior behavior, int *numfiles, int allocTrackType)
{
    return FS_ListFilteredFiles(fs_searchpaths, path, extension, NULL, behavior, numfiles, allocTrackType);
}

int FS_GetFileList(const char *path, const char *extension, FsListBehavior behavior, char *listbuf, int bufsize)
{
    const char **fileNames;
    int fileCount;
    int i;
    int nTotal;
    char *out;

    if (!listbuf || bufsize <= 0) {
        return 0;
    }

    listbuf[0] = '\0';
    fileCount = 0;

    if (I_stricmp(path, "$modlist") == 0) {
        return 0;
    }

    fileNames = FS_ListFilteredFiles(fs_searchpaths, path, extension, NULL, behavior, &fileCount, 3);
    if (fileCount <= 0) {
        FS_FreeFileList(fileNames, 3);
        return fileCount;
    }

    nTotal = 0;
    out = listbuf;
    for (i = 0; i < fileCount; i++) {
        int len = strlen(fileNames[i]) + 1;

        nTotal += len;
        if (nTotal + 1 >= bufsize) {
            fileCount = i;
            break;
        }

        strcpy(out, fileNames[i]);
        out += len;
    }

    FS_FreeFileList(fileNames, 3);
    return fileCount;
}

int FS_FOpenFileRead(const char *filename, fileHandle_t *file, qboolean uniqueFILE)
{
    return FS_FOpenFileRead_Internal(filename, file, uniqueFILE, 0, 0);
}

int FS_FOpenFileReadStream(const char *filename, fileHandle_t *file, qboolean uniqueFILE)
{
    return FS_FOpenFileRead_Internal(filename, file, uniqueFILE, 1, 0);
}

int FS_ReadFile(const char *qpath, void **buffer)
{
    fileHandle_t h;
    int len;

    if (!qpath || !qpath[0]) {
        Com_Error(0, "\025FS_ReadFile with empty name\n");
        return -1;
    }

    len = FS_FOpenFileRead(qpath, &h, 0);
    if (h == 0) {
        if (buffer) {
            *buffer = NULL;
        }
        return -1;
    }

    if (buffer) {
        byte *buf;

        fs_loadStack++;
        buf = (byte *)Hunk_AllocateTempMemoryInternal(len + 1);
        *buffer = buf;
        FS_Read(buf, len, h);
        buf[len] = '\0';
    }

    FS_FCloseFile(h);
    return len;
}

fileHandle_t FS_FOpenFileWrite(const char *filename)
{
    return FS_WebOpenWritePath(filename, "wb");
}

fileHandle_t FS_OpenFileOverwrite(const char *qpath)
{
    return FS_WebOpenWritePath(qpath, "wb");
}

fileHandle_t FS_FOpenTextFileWrite(const char *filename)
{
    return FS_WebOpenWritePath(filename, "w");
}

qboolean FS_WriteFile(const char *qpath, const void *buffer, int size)
{
    fileHandle_t h;
    int written;

    h = FS_FOpenFileWrite(qpath);
    if (!h) {
        return 0;
    }

    written = FS_Write(buffer, size, h);
    FS_FCloseFile(h);
    return written == size;
}

int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
    int len;

    if (f) {
        *f = 0;
    }

    switch (mode) {
    case FS_READ:
        return FS_FOpenFileRead(qpath, f, 1);
    case FS_WRITE:
        if (f) {
            *f = FS_FOpenFileWrite(qpath);
        }
        return (f && *f) ? 0 : -1;
    case FS_APPEND:
    case FS_APPEND_SYNC:
        if (f) {
            *f = FS_WebOpenWritePath(qpath, "ab");
            if (*f && mode == FS_APPEND_SYNC) {
                fsh[*f].handleSync = 1;
            }
        }
        return (f && *f) ? 0 : -1;
    default:
        Com_Error(0, "FS_FOpenFileByMode: bad mode\n");
        len = -1;
        break;
    }

    return len;
}

qboolean FS_FileExists(const char *file)
{
    char resolved[512];
    FILE *fp;

    fp = FS_WebOpenReadPath(file, resolved, sizeof(resolved));
    if (!fp) {
        return FS_WebFindIwdFile(file) != NULL;
    }
    fclose(fp);
    return 1;
}

qboolean FS_TouchFile(const char *name)
{
    fileHandle_t f;
    int len;

    com_fileAccessed = 1;
    len = FS_FOpenFileRead(name, &f, 0);
    if (f) {
        FS_FCloseFile(f);
        return 1;
    }

    return len >= 0;
}

const char *FS_ShortOSFilePath(const char *filename)
{
    static char resolved[512];
    FILE *fp;

    fp = FS_WebOpenReadPath(filename, resolved, sizeof(resolved));
    if (fp) {
        fclose(fp);
        return resolved;
    }

    return filename;
}

void FS_Shutdown(qboolean closemfp)
{
    searchpath_t *search;
    searchpath_t *next;
    int i;

    if (closemfp) {
        for (i = 1; i < 74; ++i) {
            if (fsh[i].handleFiles.file.o) {
                FS_FCloseFile(i);
            }
        }
    }

    FS_WebShutdownIwds();

    for (search = fs_searchpaths; search; search = next) {
        next = (searchpath_t *)(uintptr_t)search->next;
        if (search->pack) {
            if (search->pack->handle) {
                unzClose(search->pack->handle);
            }
            if (search->pack->buildBuffer) {
                Z_FreeInternal(search->pack->buildBuffer);
            }
            Z_FreeInternal(search->pack);
        }
        if (search->dir) {
            Z_FreeInternal(search->dir);
        }
        Z_FreeInternal(search);
    }

    fs_searchpaths = NULL;
    return 0.0f;
}

void FS_Startup(const char *gameName)
{
    FS_RegisterDvars();

    if (!gameName || !gameName[0]) {
        gameName = "main";
    }

    I_strncpyz(fs_gamedir, gameName, sizeof(fs_gamedir));
    FS_WebIndexIwds();
}

void FS_InitFilesystem(void)
{
    Com_StartupVariable("fs_cdpath");
    Com_StartupVariable("fs_basepath");
    Com_StartupVariable("fs_homepath");
    Com_StartupVariable("fs_game");
    Com_StartupVariable("fs_copyfiles");
    Com_StartupVariable("fs_restrict");

    FS_RegisterDvars();
    I_strncpyz(fs_gamedir, "main", sizeof(fs_gamedir));

    SEH_InitLanguage();
    FS_Startup("main");
    SEH_Init_StringEd();
    SEH_UpdateLanguageInfo();
    FS_SetRestrictions();

    if (FS_ReadFile("default_mp.cfg", NULL) <= 0) {
        Com_Error(0, "Couldn't load default_mp.cfg. Make sure the selected folder contains extracted main assets.\n");
    }

    I_strncpyz(lastValidBase, FS_WebDvarString(fs_basepath, ""), sizeof(lastValidBase));
    I_strncpyz(lastValidGame, FS_WebDvarString(fs_gameDirVar, ""), sizeof(lastValidGame));
    return 0.0f;
}
#endif
