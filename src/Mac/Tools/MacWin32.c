/* Linux directory cookies and inode numbers can exceed 32 bits, including
 * the parent entry of a Docker bind mount. readdir/stat must not stop early
 * with EOVERFLOW in the 32-bit dedicated server. Engine pointers stay 32-bit. */
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#endif

#define _GNU_SOURCE

#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <pthread.h>
#include <sched.h>

#include "common_types.h"
#include "imports.h"
#include "stubs/gcc40_compat.h"

MMRESULT timeBeginPeriod(int period)
{
    (void)period;
    return 0;
}

MMRESULT timeEndPeriod(int period)
{
    (void)period;
    return 0;
}

DWORD timeGetTime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (DWORD)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

BOOL QueryPerformanceFrequency(void *lpFrequency)
{
    long long *freq = (long long *)lpFrequency;
    *freq = 1000000LL;
    return 1;
}

#if !defined(_M_X64)  /* x64: use real Win32 (Mac/POSIX emulation collides by name) */
BOOL QueryPerformanceCounter(void *lpPerformanceCount)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long *counter = (long long *)lpPerformanceCount;
    *counter = (long long)tv.tv_sec * 1000000LL + tv.tv_usec;
    return 1;
}
#endif

#if !defined(_M_X64)  /* x64: use real Win32 (Mac/POSIX emulation collides by name) */
int MessageBoxA(void *hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
    (void)hWnd;
    int messageType = uType & 0xf;

    fprintf(stderr, "MessageBox: %s\n", lpText ? lpText : "(null)");
    if (lpCaption && lpCaption[0])
        fprintf(stderr, "  Caption: %s\n", lpCaption);

    if (messageType == 0) {

        return 1;
    } else if (messageType == 1) {

        return 1;
    } else if (messageType == 4) {

        return 6;
    }
    return 1;
}
#endif

BOOL SetFileAttributesA(LPCSTR lpFileName, DWORD dwFileAttributes)
{
    return 0;
}

extern int MacFiles_access(const char *inPath, int inMode);

DWORD GetFileAttributesA(LPCSTR lpFileName)
{

    int r = MacFiles_access(lpFileName, 4);
    return (DWORD)(2 * ((r + 1) != 0) - 1);
}

BOOL SetThreadPriority(HANDLE hThread, int nPriority)
{

    return 1;
}

#if !defined(_M_X64)  /* x64: use real Win32 (Mac/POSIX emulation collides by name) */
DWORD GetCurrentThreadId(void)
{

    return (DWORD)pthread_main_np();
}
#endif

void WinSleep(DWORD dwMilliseconds)
{
    usleep(dwMilliseconds * 1000);
}

LONG InterlockedExchangeAdd(volatile LONG *Addend, LONG Value)
{
#ifdef BYTE_MATCH_DARWIN
    extern int OTAtomicAdd32(int toAdd, int *dest);
    return OTAtomicAdd32(Value, (int *)Addend) - Value;
#else
    return cod2_sync_fetch_and_add_i32((volatile int *)Addend, Value);
#endif
}

LONG InterlockedCompareExchange(volatile LONG *Destination, LONG Exchange, LONG Comperand)
{
#ifdef BYTE_MATCH_DARWIN
    return OTCompareAndSwap32(Comperand, Exchange, (volatile unsigned int *)Destination) ? Comperand : Exchange;
#else
    return cod2_sync_val_compare_and_swap_i32((volatile int *)Destination, Comperand, Exchange);
#endif
}

LONG InterlockedExchange(volatile LONG *Target, LONG Value)
{
#ifdef BYTE_MATCH_DARWIN
    LONG old = *Target;
    return OTCompareAndSwap32(old, Value, (volatile unsigned int *)Target) ? old : Value;
#else
    return cod2_sync_lock_test_and_set_i32((volatile int *)Target, Value);
#endif
}

UINT MapVirtualKeyA(UINT uCode, UINT uMapType)
{

    switch (uCode) {
    case 0xba:
        return 0x3b;
    case 0x92:
    case 0xbb:
        return 0x3d;
    case 0xbc:
        return 0x2c;
    case 0xbd:
        return 0x2d;
    case 0xbe:
        return 0x2e;
    case 0xbf:
        return 0x2f;
    case 0xc0:
        return 0x60;
    case 0xdb:
        return 0x5b;
    case 0xdc:
        return 0x5c;
    case 0xdd:
        return 0x5d;
    case 0xde:
        return 0x27;
    default:
        return 0;
    }
}

LPVOID VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
    if (lpAddress != NULL)
        return lpAddress;
    return calloc(1, dwSize);
}

BOOL VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
    if (lpAddress == NULL)
        return 1;
    if (dwFreeType == 0x8000)
        free(lpAddress);
    return 1;
}

int GetSystemMetrics(int nIndex)
{

    if (nIndex == 0)
        return 1024;
    if (nIndex == 1)
        return 768;
    return 0;
}

BOOL FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);

typedef struct {
    DIR *dir;
    char dirpath[512];
    char pattern[260];
    int find_dirs;
} FFState;

#if !defined(_M_X64)  /* x64: use real Win32 (Mac/POSIX emulation collides by name) */
HANDLE FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
    FFState *state;
    const char *slash;
    const char *wildcard;

    if (!lpFileName || !lpFindFileData)
        return (HANDLE)-1;

    memset(lpFindFileData, 0, sizeof(_WIN32_FIND_DATAA));

    state = (FFState *)calloc(1, sizeof(FFState));
    if (!state)
        return (HANDLE)-1;

    slash = strrchr(lpFileName, '/');
    if (slash) {
        int dirlen = (int)(slash - lpFileName);
        if (dirlen >= (int)sizeof(state->dirpath))
            dirlen = (int)sizeof(state->dirpath) - 1;
        memcpy(state->dirpath, lpFileName, dirlen);
        state->dirpath[dirlen] = '\0';
        strncpy(state->pattern, slash + 1, sizeof(state->pattern) - 1);
    } else {
        strcpy(state->dirpath, ".");
        strncpy(state->pattern, lpFileName, sizeof(state->pattern) - 1);
    }

    wildcard = strstr(state->pattern, "*");
    if (!wildcard) {

        struct stat st;
        if (stat(lpFileName, &st) != 0) {
            free(state);
            return (HANDLE)-1;
        }
        lpFindFileData->dwFileAttributes = S_ISDIR(st.st_mode) ? 0x10 : 0;
        slash = strrchr(lpFileName, '/');
        strncpy(lpFindFileData->cFileName, slash ? slash + 1 : lpFileName, 259);
        lpFindFileData->nFileSizeLow = (DWORD)(st.st_size & 0xFFFFFFFF);
        lpFindFileData->nFileSizeHigh = (DWORD)(st.st_size >> 32);

        return (HANDLE)state;
    }

    state->dir = opendir(state->dirpath);
    if (!state->dir) {
        free(state);
        return (HANDLE)-1;
    }

    if (FindNextFileA((HANDLE)state, lpFindFileData))
        return (HANDLE)state;

    closedir(state->dir);
    free(state);
    return (HANDLE)-1;
}
#endif

#if !defined(_M_X64)  /* x64: use real Win32 (Mac/POSIX emulation collides by name) */
BOOL FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData)
{
    FFState *state = (FFState *)hFindFile;
    struct dirent *entry;
    struct stat st;
    char fullpath[1024];

    if (!state || !lpFindFileData || !state->dir)
        return 0;

    while ((entry = readdir(state->dir)) != NULL) {

        if (entry->d_name[0] == '.' &&
            (entry->d_name[1] == '\0' ||
             (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
            continue;

        if (fnmatch(state->pattern, entry->d_name, FNM_CASEFOLD) != 0)
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", state->dirpath, entry->d_name);
        memset(lpFindFileData, 0, sizeof(_WIN32_FIND_DATAA));

        if (stat(fullpath, &st) == 0) {
            lpFindFileData->dwFileAttributes = S_ISDIR(st.st_mode) ? 0x10 : 0;
            lpFindFileData->nFileSizeLow = (DWORD)(st.st_size & 0xFFFFFFFF);
            lpFindFileData->nFileSizeHigh = (DWORD)(st.st_size >> 32);
        }
        strncpy(lpFindFileData->cFileName, entry->d_name, 259);
        return 1;
    }
    return 0;
}
#endif

void ZNSt6vectorI6FFItemSaIS0_EE13_M_insert_auxEN9__gnu_cxx17__normal_iteratorIPS0_S2_EERKS0_(void)
{
}
