/* Linux directory cookies and inode numbers can exceed 32 bits, including
 * the parent entry of a Docker bind mount. readdir/stat must not stop early
 * with EOVERFLOW in the 32-bit dedicated server. Engine pointers stay 32-bit. */
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#endif

#include "common_types.h"
#include "imports.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

static char cwd[256];

extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern qboolean Com_FilterPath(const char *filter, const char *name, int casesensitive);
extern char *CopyStringInternal(const char *in);
extern void *Z_MallocInternal(int size);
extern void Z_FreeInternal(void *ptr);
extern int I_stricmp(const char *s1, const char *s2);
extern int FS_Read(void *buffer, int len, fileHandle_t f);
extern int FS_Seek(fileHandle_t f, long int offset, int origin);
extern OSStatus MacFiles_RemoveDirectoryA(const char *inPath);

void Sys_Mkdir(const char *path)
{
    /* mode 7 (R|W|X_OK) is invalid for Windows _access (X_OK unsupported) -> CRT
       invalid-parameter -> __fastfail/abrupt process exit. We only need an existence check. */
    if (access(path, 0) != 0) {
        mkdir(path, 0777);
    }
}

qboolean Sys_RemoveDirTree(const char *path)
{
    return MacFiles_RemoveDirectoryA(path) == 0;
}

char *Sys_DefaultCDPath(void)
{
    return (char *)"";
}

char *Sys_DefaultHomePath(void)
{
    return NULL;
}

char *Sys_DefaultInstallPath(void)
{
    getcwd(cwd, 255);
    cwd[255] = '\0';
    return cwd;
}

void Sys_ListFilteredFiles(const char *basedir, const char *subdirs, const char *filter, char **list, int *numfiles)
{
    char search[256];
    char filename[256];
    char newsubdirs[256];
    struct stat st;
    DIR *fdir;
    struct dirent *d;

    if (*numfiles >= 0xFFE)
        return;

    if (subdirs[0]) {
        Com_sprintf(search, 256, "%s/%s", basedir, subdirs);
    } else {
        Com_sprintf(search, 256, "%s", basedir);
    }

    fdir = opendir(search);
    if (!fdir)
        return;

    while ((d = readdir(fdir)) != NULL) {
        Com_sprintf(filename, 256, "%s/%s", search, d->d_name);

        if (stat(filename, &st) == -1)
            continue;

        if (S_ISDIR(st.st_mode)) {
            if (I_stricmp(d->d_name, ".") == 0 || I_stricmp(d->d_name, "..") == 0)
                goto check_file;

            if (subdirs[0]) {
                Com_sprintf(newsubdirs, 256, "%s/%s", subdirs, d->d_name);
            } else {
                Com_sprintf(newsubdirs, 256, "%s", d->d_name);
            }
            Sys_ListFilteredFiles(basedir, newsubdirs, filter, list, numfiles);
        }

    check_file:
        if (*numfiles >= 0xFFE)
            break;

        Com_sprintf(filename, 256, "%s/%s", subdirs, d->d_name);

        if (!Com_FilterPath(filter, filename, 0))
            continue;

        list[*numfiles] = CopyStringInternal(filename);
        (*numfiles)++;
    }

    closedir(fdir);
}

char **Sys_ListFiles(const char *directory, const char *extension, const char *filter, int *numfiles, qboolean wantsubs)
{
    char *list[4096];
    char search[256];
    struct stat st;
    DIR *fdir;
    struct dirent *d;
    int nfiles;
    int dironly;
    char **listCopy;
    int i;
    int extLen, nameLen;

    if (filter) {
        nfiles = 0;
        Sys_ListFilteredFiles(directory, "", filter, list, &nfiles);
        list[nfiles] = NULL;
        *numfiles = nfiles;

        if (nfiles == 0)
            return NULL;

#if defined(__x86_64__) || defined(_M_X64)
        listCopy = (char **)Z_MallocInternal((nfiles + 1) * sizeof(char *));
#else
        listCopy = (char **)Z_MallocInternal((nfiles + 1) * 4);
#endif
        for (i = 0; i < nfiles; i++) {
            listCopy[i] = list[i];
        }
        listCopy[nfiles] = NULL;
        return listCopy;
    }

    if (!extension)
        extension = "";

    if (extension[0] == '/' && extension[1] == '\0') {
        dironly = 1;
        extension = "";
    } else {
        dironly = wantsubs;
    }

    nfiles = 0;

    fdir = opendir(directory);
#ifdef W32_FSTRACE
    fprintf(stderr, "[w32-ls] dir='%s' ext='%s' opendir=%p\n", directory, extension, (void *)fdir);
    fflush(stderr);
#endif
    if (!fdir) {
        *numfiles = 0;
        return NULL;
    }

    while ((d = readdir(fdir)) != NULL) {
        if (d->d_name[0] == '.')
            continue;

        Com_sprintf(search, 256, "%s/%s", directory, d->d_name);

        if (stat(search, &st) == -1)
            continue;

        if (dironly) {
            if (!S_ISDIR(st.st_mode))
                continue;
        } else {
            if (S_ISDIR(st.st_mode))
                continue;
        }

        if (extension[0]) {
            nameLen = strlen(d->d_name);
            extLen = strlen(extension);
            if (nameLen < extLen)
                continue;
            if (I_stricmp(d->d_name + nameLen - extLen, extension) != 0)
                continue;
        }

        if (nfiles == 0xFFF)
            break;

        list[nfiles] = CopyStringInternal(d->d_name);
        nfiles++;
    }

    closedir(fdir);

    *numfiles = nfiles;

    if (nfiles == 0)
        return NULL;

#if defined(__x86_64__) || defined(_M_X64)
    listCopy = (char **)Z_MallocInternal((nfiles + 1) * sizeof(char *));
#else
    listCopy = (char **)Z_MallocInternal((nfiles + 1) * 4);
#endif
    for (i = 0; i < nfiles; i++) {
        listCopy[i] = list[i];
    }
    listCopy[nfiles] = NULL;
    return listCopy;
}

void Sys_FreeFileList(char **list)
{
    int i;

    if (!list)
        return;

    for (i = 0; list[i]; i++) {
        Z_FreeInternal(list[i]);
    }
    Z_FreeInternal(list);
}

qboolean Sys_DirectoryHasContents(const char *directory)
{
    DIR *dir;
    struct dirent *d;

    dir = opendir(directory);
    if (!dir)
        return 0;

    while ((d = readdir(dir)) != NULL) {
        if (d->d_name[0] == '.')
            continue;
        closedir(dir);
        return 1;
    }

    closedir(dir);
    return 0;
}

void Sys_InitStreamThread(void)
{

}

void Sys_BeginStreamedFile(fileHandle_t f, int readAhead)
{

}

void Sys_EndStreamedFile(fileHandle_t f)
{

}

int Sys_StreamedRead(void *buffer, int size, int count, fileHandle_t f)
{
    return FS_Read(buffer, size * count, f);
}

void Sys_StreamSeek(fileHandle_t f, int offset, int origin)
{
    FS_Seek(f, offset, origin);
}
