#include "common_types.h"
#include "imports.h"

static char buf[1024];
static char fs_map_basename[64];
static char info2[8192];
static char info3[8192];
static char info4[8192];
static char info5[8192];
static char info6[8192];
static char info8[8192];
extern fileHandleData_t fsh[74];
extern const dvar_t *fs_basepath;
extern const dvar_t *fs_cdpath;
extern const dvar_t *fs_debug;
extern const dvar_t *fs_homepath;

fileHandle_t FS_SV_FOpenFileWrite(const char *filename);
int FS_SV_FOpenFileRead(const char *filename, fileHandle_t *fp);
void FS_SV_Rename(const char *from, const char *to);
char *FS_ShiftStr(const char *string, int shift);
void FS_Dir_f(void);
void FS_NewDir_f(void);
void FS_TouchFile_f(void);
qboolean FS_iwIwd(char *iwd, char *base);
void FS_AddCommands(void);
void FS_SetRestrictions(void);
const char *FS_LoadedIwdChecksums(void);
const char *FS_LoadedIwdNames(void);
const char *FS_LoadedIwdPureChecksums(void);
const char *FS_ReferencedIwdChecksums(void);
const char *FS_ReferencedIwdNames(void);
const char *FS_ReferencedIwdPureChecksums(void);
void FS_PureServerSetLoadedIwds(const char *iwdSums, const char *iwdNames);
void FS_PureServerSetReferencedIwds(const char *iwdSums, const char *iwdNames);
const char *FS_GetMapBaseName(const char *mapname);
int FS_GetModList(char *listbuf, int bufsize);
qboolean FS_CompareIwds(char *needediwds, int len, qboolean dlstring);

extern void FS_CheckFileSystemStarted(void);
extern void FS_BuildOSPath(const char *base, const char *game, const char *qpath, char *ospath);
extern fileHandle_t FS_HandleForFile(int flags);
extern int FS_CreatePath(const char *OSPath);
extern void Com_DPrintf(const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern FILE *FS_FileOpen(const char *filename, const char *mode);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int FS_filelength(fileHandle_t f);
extern int I_stricmp(const char *s1, const char *s2);
extern void FS_CopyFile(const char *from, const char *to);
extern void FS_Remove(const char *ospath);
extern int Cmd_Argc(void);
extern char *Cmd_Argv(int arg);
extern char **FS_ListFiles(const char *path, const char *extension, int wantSubs, int *numfiles, int flags);
extern void FS_FreeFileList(char **list, int flags);
extern const char **FS_ListFilteredFiles(searchpath_t *searchPath, const char *path, const char *extension, const char *filter, FsListBehavior behavior, int *numfiles, int allocTrackType);
extern void FS_SortFileList(char **list, int numfiles);
extern void FS_ConvertPath(char *s);
extern qboolean FS_TouchFile(const char *filename);
extern void Cmd_AddCommand(const char *cmdName, void *function);
extern void Dvar_SetBool(const void *dvar, int val);
extern void FS_Shutdown(int flags);
extern void FS_Startup(const char *gameName);
extern int FS_UseSearchPath(void *sp);
extern void Com_Error(int level, const char *fmt, ...);
extern void I_strncat(char *dest, int destsize, const char *src);
extern char *va(const char *fmt, ...);
extern int FS_FilenameCompare(const char *s1, const char *s2);
extern char *strstr(const char *haystack, const char *needle);
extern char *I_strlwr(char *s);
extern void Cmd_TokenizeString(const char *text);
extern int atoi(const char *nptr);
extern char *CopyStringInternal(const char *str);
extern void Z_FreeInternal(void *ptr);
extern void FS_ShutdownServerIwdNames(void);
extern void Com_Memcpy(void *dest, const void *src, int count);
extern void SND_StopSounds(int flags);
extern void FS_ShutdownServerReferencedIwds(void);
extern int I_strnicmp(const char *s1, const char *s2, size_t n);
extern int stricmp(const char *s1, const char *s2);
extern void *Z_MallocInternal(int size);
extern char **Sys_ListFiles(const char *directory, const char *extension, const char *filter, int *numfiles, int wantSubs);
extern void Sys_FreeFileList(char **list);
extern void FS_FCloseFile(fileHandle_t f);
extern FILE *FS_FileForHandle(fileHandle_t f);
extern void Com_Memset(void *dest, int val, int count);
extern int FS_FileRead(void *buf, int len, int count, FILE *f);
extern int FS_FileClose(FILE *f);
extern int Com_sprintf(char *dest, int destsize, const char *fmt, ...);

static inline __attribute__((always_inline)) void FS_SV_BuildOSPath(const char *base, const char *filename, char *ospath, size_t ospathSize)
{
    (void)ospathSize;
    FS_BuildOSPath(base, filename, "", ospath);
    ospath[strlen(ospath) - 1] = '\0';
}

fileHandle_t FS_SV_FOpenFileWrite(const char *filename)
{
    char ospath[260];
    fileHandle_t f;
    fileHandleData_t *entry;

    FS_CheckFileSystemStarted();

    {
        const dvar_t *homepath_dvar = *(const dvar_t **)imp_fs_homepath;
        FS_SV_BuildOSPath(homepath_dvar->current.string, filename, ospath, sizeof(ospath));
    }

    f = FS_HandleForFile(0);
    entry = &((fileHandleData_t *)imp_fsh)[f];
    entry->zipFile = NULL;

    {
        const dvar_t *debug_dvar = *(const dvar_t **)imp_fs_debug;
        if (debug_dvar->current.integer) {
            Com_Printf("FS_SV_FOpenFileWrite: %s\n", ospath);
        }
    }

    if (FS_CreatePath(ospath)) {

        return 0;
    }

    Com_DPrintf("writing to: %s\n", ospath);

    entry->handleFiles.file.o = FS_FileOpen(ospath, "wb");

    I_strncpyz(entry->name, filename, sizeof(entry->name));

    entry->handleSync = 0;

    if (entry->handleFiles.file.o == NULL) {
        return 0;
    }

    return f;
}

int FS_SV_FOpenFileRead(const char *filename, fileHandle_t *fp)
{
    char ospath[260];
    fileHandle_t f;
    fileHandleData_t *entry;
    FILE *file;

    FS_CheckFileSystemStarted();

    f = FS_HandleForFile(0);
    entry = &((fileHandleData_t *)imp_fsh)[f];
    entry->zipFile = NULL;

    I_strncpyz(entry->name, filename, sizeof(entry->name));

    {
        const dvar_t *homepath_dvar = *(const dvar_t **)imp_fs_homepath;
        FS_SV_BuildOSPath(homepath_dvar->current.string, filename, ospath, sizeof(ospath));
    }

    {
        const dvar_t *debug_dvar = *(const dvar_t **)imp_fs_debug;
        if (debug_dvar->current.integer) {
            Com_Printf("FS_SV_FOpenFileRead (fs_homepath): %s\n", ospath);
        }
    }

    file = FS_FileOpen(ospath, "rb");
    entry->handleFiles.file.o = file;
    entry->handleSync = 0;

    if (file != NULL) {
        goto done;
    }

    {
        const dvar_t *basepath_dvar = *(const dvar_t **)imp_fs_basepath;
        const dvar_t *homepath_dvar = *(const dvar_t **)imp_fs_homepath;
        const char *basepath_str = basepath_dvar->current.string;
        const char *homepath_str = homepath_dvar->current.string;

        if (I_stricmp(homepath_str, basepath_str) != 0) {

            FS_SV_BuildOSPath(basepath_str, filename, ospath, sizeof(ospath));

            {
                const dvar_t *debug_dvar = *(const dvar_t **)imp_fs_debug;
                if (debug_dvar->current.integer) {
                    Com_Printf("FS_SV_FOpenFileRead (fs_basepath): %s\n", ospath);
                }
            }

            file = FS_FileOpen(ospath, "rb");
            entry->handleFiles.file.o = file;
            entry->handleSync = 0;

            if (file != NULL) {
                goto done;
            }
        }
    }

    if (entry->handleFiles.file.o != NULL) {
        goto done;
    }

    {
        const dvar_t *cdpath_dvar = *(const dvar_t **)imp_fs_cdpath;
        FS_SV_BuildOSPath(cdpath_dvar->current.string, filename, ospath, sizeof(ospath));
    }

    {
        const dvar_t *debug_dvar = *(const dvar_t **)imp_fs_debug;
        if (debug_dvar->current.integer) {
            Com_Printf("FS_SV_FOpenFileRead (fs_cdpath) : %s\n", ospath);
        }
    }

    file = FS_FileOpen(ospath, "rb");
    entry->handleFiles.file.o = file;
    entry->handleSync = 0;
    if (file == NULL) {
        f = 0;
    }

done:

    *fp = f;

    if (f == 0) {
        return 0;
    }

    return FS_filelength(f);
}

void FS_SV_Rename(const char *from, const char *to)
{
    char from_ospath[260];
    char to_ospath[260];

    FS_CheckFileSystemStarted();

    {
        const dvar_t *homepath_dvar = *(const dvar_t **)imp_fs_homepath;
        FS_BuildOSPath(homepath_dvar->current.string, from, "", from_ospath);
    }

    {
        const dvar_t *homepath_dvar = *(const dvar_t **)imp_fs_homepath;
        FS_BuildOSPath(homepath_dvar->current.string, to, "", to_ospath);
    }

    from_ospath[strlen(from_ospath) - 1] = '\0';
    to_ospath[strlen(to_ospath) - 1] = '\0';

    {
        const dvar_t *debug_dvar = *(const dvar_t **)imp_fs_debug;
        if (debug_dvar->current.integer) {
            Com_Printf("FS_SV_Rename: %s --> %s\n", from_ospath, to_ospath);
        }
    }

    if (rename(from_ospath, to_ospath) != 0) {

        FS_CopyFile(from_ospath, to_ospath);
        FS_Remove(from_ospath);
    }

    return;
}

char *FS_ShiftStr(const char *string, int shift)
{
    int len;
    int i;

    len = (int)strlen(string);

    if (len <= 0) {
        buf[0] = '\0';
        return buf;
    }

    for (i = 0; i < len; i++) {
        buf[i] = (char)((unsigned char)string[i] + (unsigned char)shift);
    }
    buf[len] = '\0';

    return buf;
}

void FS_Dir_f(void)
{
    int argc;
    char *path;
    char *extension;
    char **dirnames;
    int ndirs;
    int i;

    argc = Cmd_Argc();

    if (argc - 1 <= 0 || argc > 3) {

        Com_Printf("usage: dir <directory> [extension]\n");
        return;
    }

    if (argc == 2) {
        path = Cmd_Argv(1);
        extension = "";
    } else {
        path = Cmd_Argv(1);
        extension = Cmd_Argv(2);
    }

    Com_Printf("Directory of %s %s\n", path, extension);
    Com_Printf("---------------\n");

    dirnames = FS_ListFiles(path, extension, 0, &ndirs, 10);

    if (ndirs > 0) {

        for (i = 0; i < ndirs; i++) {
            Com_Printf("%s\n", dirnames[i]);
        }
    }

    FS_FreeFileList(dirnames, 10);

    return;
}

void FS_NewDir_f(void)
{
    char *filter;
    char **dirnames;
    int ndirs;
    int i;

    if (Cmd_Argc() - 1 <= 0) {

        Com_Printf("usage: fdir <filter>\n");
        Com_Printf("example: fdir *q3dm*.bsp\n");
        return;
    }

    filter = Cmd_Argv(1);

    Com_Printf("---------------\n");

    {
        void *searchpaths = *(void **)imp_fs_searchpaths;
        dirnames = (char **)FS_ListFilteredFiles(searchpaths, "", "", filter, (FsListBehavior)0, &ndirs, 10);
    }

    FS_SortFileList(dirnames, ndirs);

    if (ndirs > 0) {
        for (i = 0; i < ndirs; i++) {
            FS_ConvertPath(dirnames[i]);
            Com_Printf("%s\n", dirnames[i]);
        }
    }

    Com_Printf("%d files listed\n", ndirs);
    FS_FreeFileList(dirnames, 10);

    return;
}

void FS_TouchFile_f(void)
{
    if (Cmd_Argc() != 2) {
        Com_Printf("Usage: touchFile <file>\n");
        return;
    }

    FS_TouchFile(Cmd_Argv(1));
    return;
}

qboolean FS_iwIwd(char *iwd, char *base)
{
    int i;
    char *p;
    char szFile[64];

    for (i = 0; i < 25; i++) {
        if (FS_FilenameCompare(iwd, va("%s/iw_%02d", base, i)) == 0) {
            return 1;
        }
    }

    p = strstr(iwd, "localized_");
    if (p == NULL) {
        return 0;
    }

    strcpy(szFile, iwd);
    szFile[(p - iwd) + 10] = '\0';

    if (FS_FilenameCompare(szFile, va("%s/localized_", base)) != 0) {
        return 0;
    }

    strcpy(szFile, p + 10);
    I_strlwr(szFile);

    for (i = 0; i < 25; i++) {
        if (strstr(szFile, va("_iw%02d", i)) != NULL) {
            return 1;
        }
    }

    return 0;
}

void FS_AddCommands(void)
{
    Cmd_AddCommand("path", (void *)imp_FS_Path_f);
    Cmd_AddCommand("fullpath", (void *)imp_FS_FullPath_f);
    Cmd_AddCommand("dir", FS_Dir_f);
    Cmd_AddCommand("fdir", FS_NewDir_f);
    Cmd_AddCommand("touchFile", FS_TouchFile_f);
    return;
}

void FS_SetRestrictions(void)
{
    void *path;
    void *iwd;
    unsigned int checksum;

    {
        const dvar_t *restrict_dvar = *(const dvar_t **)imp_fs_restrict;
        if (restrict_dvar->current.enabled == 0) {
            return;
        }

        Dvar_SetBool(restrict_dvar, 1);
    }

    Com_Printf("Restricting pure client, full media required.\n");

    FS_Shutdown(0);
    FS_Startup("main");

    path = *(void **)imp_fs_searchpaths;
    while (path != NULL) {

        if (FS_UseSearchPath(path)) {
            iwd = ((searchpath_t *)path)->pack;
            if (iwd != NULL) {
                checksum = ((pack_t *)iwd)->checksum;
                if (checksum != 0xb1f595f5u) {

                    Com_Error(0, "Corrupted iw0.iwd: %u", checksum);
                }
            }
        }
        path = (void *)(intptr_t)((searchpath_t *)path)->next;
    }

    return;
}

const char *FS_LoadedIwdChecksums(void)
{
    void *search;

    info2[0] = '\0';

    search = *(void **)imp_fs_searchpaths;
    while (search != NULL) {
        void *iwd = ((searchpath_t *)search)->pack;
        if (iwd != NULL) {
            void *localized = (void *)(intptr_t)((searchpath_t *)search)->bLocalized;
            if (localized == NULL) {
                int checksum = ((pack_t *)iwd)->checksum;
                I_strncat(info2, 0x2000, va("%i ", checksum));
            }
        }
        search = (void *)(intptr_t)((searchpath_t *)search)->next;
    }

    return info2;
}

const char *FS_LoadedIwdNames(void)
{
    void *search;

    info3[0] = '\0';

    search = *(void **)imp_fs_searchpaths;
    while (search != NULL) {
        void *iwd = ((searchpath_t *)search)->pack;
        if (iwd != NULL) {
            void *localized = (void *)(intptr_t)((searchpath_t *)search)->bLocalized;
            if (localized == NULL) {

                if (info3[0] != '\0') {
                    I_strncat(info3, 0x2000, " ");
                }

                I_strncat(info3, 0x2000, ((pack_t *)iwd)->iwdBasename);
            }
        }
        search = (void *)(intptr_t)((searchpath_t *)search)->next;
    }

    return info3;
}

const char *FS_LoadedIwdPureChecksums(void)
{
    void *search;

    info4[0] = '\0';

    search = *(void **)imp_fs_searchpaths;
    while (search != NULL) {
        void *iwd = ((searchpath_t *)search)->pack;
        if (iwd != NULL) {
            void *localized = (void *)(intptr_t)((searchpath_t *)search)->bLocalized;
            if (localized == NULL) {
                int checksum = ((pack_t *)iwd)->pure_checksum;
                I_strncat(info4, 0x2000, va("%i ", checksum));
            }
        }
        search = (void *)(intptr_t)((searchpath_t *)search)->next;
    }

    return info4;
}

const char *FS_ReferencedIwdChecksums(void)
{
    void *search;

    info5[0] = '\0';

    search = *(void **)imp_fs_searchpaths;
    while (search != NULL) {
        void *iwd = ((searchpath_t *)search)->pack;
        if (iwd != NULL) {

            int referenced = ((pack_t *)iwd)->referenced;
            if (!referenced) {

                if (I_strnicmp(((pack_t *)iwd)->iwdGamename, "main", 4) == 0) {

                    search = (void *)(intptr_t)((searchpath_t *)search)->next;
                    continue;
                }

                iwd = ((searchpath_t *)search)->pack;
            }

            {
                int checksum = ((pack_t *)iwd)->checksum;
                I_strncat(info5, 0x2000, va("%i ", checksum));
            }
        }
        search = (void *)(intptr_t)((searchpath_t *)search)->next;
    }

    return info5;
}

const char *FS_ReferencedIwdNames(void)
{
    void *search;

    info8[0] = '\0';

    search = *(void **)imp_fs_searchpaths;
    while (search != NULL) {
        void *iwd = ((searchpath_t *)search)->pack;
        if (iwd != NULL) {
            int referenced = ((pack_t *)iwd)->referenced;
            if (!referenced) {

                if (I_strnicmp(((pack_t *)iwd)->iwdGamename, "main", 4) == 0) {

                    search = (void *)(intptr_t)((searchpath_t *)search)->next;
                    continue;
                }
            }

            if (info8[0] != '\0') {
                I_strncat(info8, 0x2000, " ");
            }

            I_strncat(info8, 0x2000, ((pack_t *)iwd)->iwdGamename);
            I_strncat(info8, 0x2000, "/");

            I_strncat(info8, 0x2000, ((pack_t *)iwd)->iwdBasename);
        }
        search = (void *)(intptr_t)((searchpath_t *)search)->next;
    }

    return info8;
}

const char *FS_ReferencedIwdPureChecksums(void)
{
    void *search;
    int checksum;
    int count;
    int fakeChkSum;

    info6[0] = '\0';

    checksum = *(int *)imp_fs_checksumFeed;

    info6[0] = '@';
    info6[1] = ' ';
    info6[2] = '\0';

    count = 0;
    search = *(void **)imp_fs_searchpaths;
    while (search != NULL) {
        void *iwd = ((searchpath_t *)search)->pack;
        if (iwd != NULL &&
            ((searchpath_t *)search)->bLocalized == 0 &&
            ((pack_t *)iwd)->referenced != 0) {
            I_strncat(info6, 0x2000, va("%i ", ((pack_t *)iwd)->pure_checksum));
            checksum ^= ((pack_t *)iwd)->pure_checksum;
            count++;
        }
        search = (void *)(intptr_t)((searchpath_t *)search)->next;
    }

    fakeChkSum = *(int *)imp_fs_fakeChkSum;
    if (fakeChkSum != 0) {
        I_strncat(info6, 0x2000, va("%i ", fakeChkSum));
    }

    I_strncat(info6, 0x2000, va("%i ", count ^ checksum));
    return info6;
}

void FS_PureServerSetLoadedIwds(const char *iwdSums, const char *iwdNames)
{
    int serverIwds[1024];
    char *serverIwdNames[1024];
    int c;
    int d;
    int i;
    int j;

    Cmd_TokenizeString(iwdSums);
    c = Cmd_Argc();
    if (c > 0x400) {
        c = 0x400;
    }

    if (c > 0) {
        for (i = 0; i < c; i++) {
            serverIwds[i] = atoi(Cmd_Argv(i));
        }
    }

    Cmd_TokenizeString(iwdNames);
    d = Cmd_Argc();
    if (d > 0x400) {
        d = 0x400;
    }

    if (d > 0) {
        for (i = 0; i < d; i++) {
            serverIwdNames[i] = CopyStringInternal(Cmd_Argv(i));
        }
    }

    if (c != d) {
        Com_Error(1, "FS_PureServerSetLoadedIwds: count mismatch");
    }

    {
        int *fs_numServerIwds = (int *)imp_fs_numServerIwds;
        if (c == *fs_numServerIwds) {

            int changed = 0;
            int local_i;

            if (c <= 0) {
                goto free_names_and_return;
            }

            for (local_i = 0; local_i < c; local_i++) {

                int numOld = *fs_numServerIwds;
                if (numOld <= 0) {
                    goto do_reload;
                }

                {
                    int *fs_serverIwds = (int *)imp_fs_serverIwds;
                    char **fs_serverIwdNames = (char **)imp_fs_serverIwdNames;

                    for (j = 0; j < numOld; j++) {

                        if (serverIwds[local_i] == fs_serverIwds[j]) {

                            if (I_stricmp(fs_serverIwdNames[j], serverIwdNames[local_i]) == 0) {
                                break;
                            }
                        }
                    }

                    if (j >= numOld) {

                        goto do_reload;
                    }
                }
            }

            goto free_names_and_return;

        free_names_and_return:

            if (d > 0) {
                for (i = 0; i < d; i++) {
                    Z_FreeInternal(serverIwdNames[i]);
                }
            }
            return;
        }
    }

do_reload:

    SND_StopSounds(8);
    FS_ShutdownServerIwdNames();

    {
        int *fs_numServerIwds = (int *)imp_fs_numServerIwds;
        *fs_numServerIwds = c;

        if (c == 0) {
            return;
        }

        Com_DPrintf("Setting pure server iwds\n");

        Com_Memcpy(imp_fs_serverIwds, serverIwds, c * 4);

        Com_Memcpy(imp_fs_serverIwdNames, serverIwdNames, c * 4);

        *(int *)imp_fs_fakeChkSum = 0;
    }

    return;
}

void FS_PureServerSetReferencedIwds(const char *iwdSums, const char *iwdNames)
{
    int c;
    int d;
    int i;
    int *fs_serverReferencedIwds;
    char **fs_serverReferencedIwdNames;

    Cmd_TokenizeString(iwdSums);
    c = Cmd_Argc();
    if (c >= 0x401) {
        c = 0x400;
    }

    FS_ShutdownServerReferencedIwds();

    fs_serverReferencedIwds = (int *)imp_fs_serverReferencedIwds;
    if (c > 0) {
        for (i = 0; i < c; i++) {
            fs_serverReferencedIwds[i] = atoi(Cmd_Argv(i));
        }
    }

    if (iwdNames == NULL || iwdNames[0] == '\0') {

        if (c != 0) {
            Com_Error(1, "FS_PureServerSetReferencedIwds: count mismatch");
        }

        *(int *)imp_fs_numServerReferencedIwds = c;
        return;
    }

    Cmd_TokenizeString(iwdNames);
    d = Cmd_Argc();
    if (d >= 0x401) {
        d = 0x400;
    }

    if (c != d) {
        Com_Error(1, "FS_PureServerSetReferencedIwds: count mismatch");
    }

    if (d > 0) {
        fs_serverReferencedIwdNames = (char **)imp_fs_serverReferencedIwdNames;
        for (i = 0; i < d; i++) {
            fs_serverReferencedIwdNames[i] = CopyStringInternal(Cmd_Argv(i));
        }
    }

    *(int *)imp_fs_numServerReferencedIwds = c;
    return;
}

const char *FS_GetMapBaseName(const char *mapname)
{
    int len;
    int i;
    char *p;

    if (I_strnicmp(mapname, "maps/mp/", 8) == 0) {
        mapname += 8;
    }

    len = (int)strlen(mapname);

    if (stricmp(mapname + len - 3, "bsp") == 0) {
        len -= 7;
    }

    memcpy(fs_map_basename, mapname, len);
    fs_map_basename[len] = '\0';

    if (len > 0) {
        char *end = fs_map_basename + len;
        p = fs_map_basename;
        do {
            if (*p == '%') {
                *p = '_';
            }
            p++;
        } while (p != end);
    }

    return fs_map_basename;
}

int FS_GetModList(char *listbuf, int bufsize)
{
    char **pFiles0;
    char **pFiles1;
    char **pFiles2;
    char **pFiles;
    int dummy;
    int nTotal;
    int nMods;
    int nFiles;
    int i;
    int j;
    int bDrop;
    char *name;
    int nameLen;
    int descLen;
    char path[256];
    char descPath[256];
    fileHandle_t descHandle;
    FILE *descFile;
    int nIwds;

    listbuf[0] = '\0';

    {
        const dvar_t *homepath_dvar = fs_homepath;
        pFiles0 = Sys_ListFiles(homepath_dvar->current.string, NULL, NULL, &dummy, 1);
    }

    {
        const dvar_t *basepath_dvar = fs_basepath;
        pFiles1 = Sys_ListFiles(basepath_dvar->current.string, NULL, NULL, &dummy, 1);
    }

    {
        const dvar_t *cdpath_dvar = fs_cdpath;
        const char *cdpath_str = cdpath_dvar->current.string;
        if (cdpath_str == NULL || cdpath_str[0] == '\0') {
            pFiles2 = NULL;
        } else {
            pFiles2 = Sys_ListFiles(cdpath_str, NULL, NULL, &dummy, 1);
        }
    }

    {
        int n0 = 0, n1 = 0, n2 = 0;
        if (pFiles0 != NULL && pFiles0[0] != NULL) {
            while (pFiles0[n0] != NULL)
                n0++;
        }
        if (pFiles1 != NULL && pFiles1[0] != NULL) {
            while (pFiles1[n1] != NULL)
                n1++;
        }
        if (pFiles2 != NULL && pFiles2[0] != NULL) {
            while (pFiles2[n2] != NULL)
                n2++;
        }

        pFiles = (char **)Z_MallocInternal((n0 + n1 + n2 + 1) * 4);

        {
            int idx = 0;
            if (pFiles0 != NULL) {
                for (j = 0; pFiles0[j] != NULL; j++) {
                    pFiles[idx++] = pFiles0[j];
                }
            }
            if (pFiles1 != NULL) {
                for (j = 0; pFiles1[j] != NULL; j++) {
                    pFiles[idx++] = pFiles1[j];
                }
            }
            if (pFiles2 != NULL) {
                for (j = 0; pFiles2[j] != NULL; j++) {
                    pFiles[idx++] = pFiles2[j];
                }
            }
            pFiles[idx] = NULL;
        }

        if (pFiles0 != NULL)
            Z_FreeInternal(pFiles0);
        if (pFiles1 != NULL)
            Z_FreeInternal(pFiles1);
        if (pFiles2 != NULL)
            Z_FreeInternal(pFiles2);
    }

    nFiles = 0;
    if (pFiles == NULL || pFiles[0] == NULL) {
        nMods = 0;
        goto cleanup;
    }
    while (pFiles[nFiles] != NULL)
        nFiles++;

    if (nFiles <= 0) {
        nMods = 0;
        goto cleanup;
    }

    bDrop = 0;
    nTotal = 0;
    nMods = 0;

    for (i = 0; i < nFiles; i++) {
        name = pFiles[i];
        nameLen = (int)strlen(name);

        if (i != 0) {

            bDrop = 0;
            for (j = 0; j < i; j++) {
                if (I_stricmp(pFiles[j], name) == 0) {
                    bDrop = 1;
                    break;
                }
            }
        } else {
            bDrop = 0;
        }

        if (bDrop) {
            continue;
        }

        if (I_strnicmp(name, ".", 1) == 0) {
            continue;
        }

        {
            const dvar_t *basepath_dvar = fs_basepath;
            FS_BuildOSPath(basepath_dvar->current.string, name, "", path);
        }

        nIwds = 0;
        Sys_FreeFileList(Sys_ListFiles(path, "iwd", NULL, &nIwds, 0));

        if (nIwds <= 0) {

            {
                const dvar_t *cdpath_dvar = fs_cdpath;
                FS_BuildOSPath(cdpath_dvar->current.string, name, "", path);
            }
            nIwds = 0;
            Sys_FreeFileList(Sys_ListFiles(path, "iwd", NULL, &nIwds, 0));

            if (nIwds <= 0) {

                {
                    const dvar_t *homepath_dvar = fs_homepath;
                    FS_BuildOSPath(homepath_dvar->current.string, name, "", path);
                }
                nIwds = 0;
                Sys_FreeFileList(Sys_ListFiles(path, "iwd", NULL, &nIwds, 0));

                if (nIwds <= 0) {
                    continue;
                }
            }
        }

        nameLen = (int)strlen(name);

        strcpy(descPath, name);
        I_strncat(descPath, 0x100, "/description.txt");

        if (FS_SV_FOpenFileRead(descPath, &descHandle) > 0 && descHandle != 0) {

            descFile = FS_FileForHandle(descHandle);
            Com_Memset(descPath, 0, sizeof(descPath));
            {
                int nread = FS_FileRead(descPath, 1, 0x30, descFile);
                if (nread >= 0) {
                    descPath[nread] = '\0';
                }
            }
            FS_FCloseFile(descHandle);
        } else if (I_stricmp(name, "main") == 0) {

            strcpy(descPath, "Call of 2 Multiplayer");
        } else {

            strcpy(descPath, name);
        }

        descLen = (int)strlen(descPath);

        if (nTotal + nameLen + descLen + 2 >= bufsize) {
            break;
        }

        strcpy(listbuf, name);

        listbuf += nameLen;

        strcpy(listbuf, descPath);
        listbuf += descLen;

        nTotal += nameLen + descLen;
        nMods++;
    }

cleanup:

    Sys_FreeFileList(pFiles);

    return nMods;
}

qboolean FS_CompareIwds(char *needediwds, int len, qboolean dlstring)
{
    int i;
    int numServerIwds;
    int *fs_serverReferencedIwds;
    char **fs_serverReferencedIwdNames;
    char testpath[264];

    numServerIwds = *(int *)imp_fs_numServerReferencedIwds;
    if (numServerIwds == 0) {
        return 0;
    }

    needediwds[0] = '\0';

    if (numServerIwds <= 0) {

        if (needediwds[0] != '\0') {
            goto need_iwds;
        }
        return 0;
    }

    fs_serverReferencedIwdNames = (char **)imp_fs_serverReferencedIwdNames;
    fs_serverReferencedIwds = (int *)imp_fs_serverReferencedIwds;

    for (i = 0; i < numServerIwds; i++) {
        char *iwdName = fs_serverReferencedIwdNames[i];
        int iwdChecksum = fs_serverReferencedIwds[i];

        if (FS_iwIwd(iwdName, "main") != 0) {
            goto next_iwd;
        }

        {
            void *search = *(void **)imp_fs_searchpaths;
            int found = 0;
            while (search != NULL) {
                void *iwd = ((searchpath_t *)search)->pack;
                if (iwd != NULL) {
                    int chk = ((pack_t *)iwd)->checksum;
                    if (chk == iwdChecksum) {
                        found = 1;
                        break;
                    }
                }
                search = (void *)(intptr_t)((searchpath_t *)search)->next;
            }
            if (found) {
                goto next_iwd;
            }
        }

        if (iwdName == NULL || iwdName[0] == '\0') {
            goto next_iwd;
        }

        if (!dlstring) {

            I_strncat(needediwds, len, iwdName);
            I_strncat(needediwds, len, " ");

            {
                char *iwdFile = va("%s.iwd", iwdName);
                const dvar_t *homepath_dvar = fs_homepath;
                FS_BuildOSPath(homepath_dvar->current.string, iwdFile, "", testpath);
                testpath[strlen(testpath) - 1] = '\0';

                {
                    FILE *f = FS_FileOpen(testpath, "rb");
                    if (f != NULL) {
                        FS_FileClose(f);

                        I_strncat(needediwds, len, "@");
                    }
                }
            }

            I_strncat(needediwds, len, " ");
        } else {

            I_strncat(needediwds, len, "@");
            I_strncat(needediwds, len, iwdName);
            I_strncat(needediwds, len, " ");

            I_strncat(needediwds, len, "@");

            {
                char *iwdFile = va("%s.iwd", iwdName);
                const dvar_t *homepath_dvar = fs_homepath;
                FS_BuildOSPath(homepath_dvar->current.string, iwdFile, "", testpath);
                testpath[strlen(testpath) - 1] = '\0';

                {
                    FILE *f = FS_FileOpen(testpath, "rb");
                    if (f != NULL) {
                        FS_FileClose(f);

                        Com_sprintf(testpath, 0x100, "%s.%08x.iwd", iwdName, iwdChecksum);
                        I_strncat(needediwds, len, testpath);
                    } else {

                        I_strncat(needediwds, len, iwdName);
                        I_strncat(needediwds, len, " ");
                    }
                }
            }

            I_strncat(needediwds, len, " ");
        }

    next_iwd:

        ;
    }

    if (needediwds[0] == '\0') {
        return 0;
    }

need_iwds:
    Com_Printf("Need iwds: %s\n", needediwds);
    return 1;
}
