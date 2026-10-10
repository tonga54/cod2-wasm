#!/usr/bin/env python3
"""Exercise the real i386 directory code with wide Linux inode/cookie values."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
harness = r'''
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

extern char **Sys_ListFiles(const char *, const char *, const char *, int *, int);
extern void Sys_FreeFileList(char **);
extern int Sys_DirectoryHasContents(const char *);
struct Cursor { int next; int empty; };
static const char *names[] = {".", "..", ".hidden", "cod2.iwd", "localized.IWD", "notes.txt"};
DIR *__wrap_opendir(const char *path) {
  struct Cursor *c = calloc(1, sizeof(*c));
  c->empty = !strcmp(path, "/empty");
  return (DIR *)c;
}
int __wrap_closedir(DIR *d) { free(d); return 0; }
struct dirent64 *__wrap_readdir64(DIR *d) {
  static struct dirent64 entry;
  struct Cursor *c = (struct Cursor *)d;
  if (c->next >= (c->empty ? 2 : 6)) return NULL;
  memset(&entry, 0, sizeof(entry));
  entry.d_ino = 0x200000001ULL + c->next;
  entry.d_off = 0x400000000LL + c->next;
  entry.d_reclen = sizeof(entry);
  strcpy(entry.d_name, names[c->next++]);
  return &entry;
}
/* Model libc's narrow conversion failure, including on a dot entry before IWDs. */
struct dirent *__wrap_readdir(DIR *d) { (void)d; errno = EOVERFLOW; return NULL; }
int __wrap_stat64(const char *path, struct stat64 *st) {
  (void)path;
  memset(st, 0, sizeof(*st));
  st->st_mode = S_IFREG | 0644;
  st->st_ino = 0x300000001ULL;
  return 0;
}
int __wrap_stat(const char *path, struct stat *st) {
  (void)path; (void)st; errno = EOVERFLOW; return -1;
}
int Com_sprintf(char *out, int n, const char *fmt, ...) {
  va_list ap; va_start(ap, fmt); int r = vsnprintf(out, n, fmt, ap); va_end(ap); return r;
}
int Com_FilterPath(const char *pattern, const char *name, int sensitive) {
  (void)sensitive; return fnmatch(pattern, name, 0) == 0;
}
char *CopyStringInternal(const char *s) { return strdup(s); }
void *Z_MallocInternal(int n) { return malloc(n); }
void Z_FreeInternal(void *p) { free(p); }
int I_stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
int main(void) {
  assert(sizeof(void *) == 4);
  int n = -1;
  char **files = Sys_ListFiles("/main", ".iwd", NULL, &n, 0);
  if (!files || n != 2) { fprintf(stderr, "IWD enumeration stopped at a wide directory entry\n"); return 1; }
  assert(!strcmp(files[0], "cod2.iwd") && !strcmp(files[1], "localized.IWD") && !files[2]);
  Sys_FreeFileList(files);
  files = Sys_ListFiles("/main", NULL, NULL, &n, 0);
  assert(n == 3); Sys_FreeFileList(files);
  files = Sys_ListFiles("/main", "/", NULL, &n, 0);
  assert(n == 0 && !files);
  files = Sys_ListFiles("/main", ".iwd", "*.iwd", &n, 0);
  if(n != 1) { fprintf(stderr,"filtered count=%d\n",n); for(int i=0;i<n;i++)fprintf(stderr,"%s\n",files[i]); return 1; }
  assert(!strcmp(files[0], "/cod2.iwd")); Sys_FreeFileList(files);
  assert(Sys_DirectoryHasContents("/main") && !Sys_DirectoryHasContents("/empty"));
  files = Sys_ListFiles("/empty", ".iwd", NULL, &n, 0);
  assert(n == 0 && !files);
  puts("PASS: actual i386 file listing survives 64-bit directory cookies/inodes; extension/filter/empty checks pass");
}
'''
source = (root / 'src/Mac/Main/mac_common.c').read_text()
(root / 'out').mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='native-file-listing-', dir=root / 'out') as work:
    path = Path(work)
    (path / 'check.c').write_text(harness)
    # A narrow libc ABI must fail the same enumeration, proving this catches the regression.
    (path / 'narrow.c').write_text(source.replace('#define _FILE_OFFSET_BITS 64', '#define _FILE_OFFSET_BITS 32'))
    command = r'''
set -eu
cc -m32 -std=gnu99 -fno-pie -no-pie -ffunction-sections -fdata-sections -w \
  -I/repo/src -I/repo/src/headers -I/repo -I/usr/include/SDL2 \
  -c /repo/src/Mac/Main/mac_common.c -o wide.o
cc -m32 -std=gnu99 -fno-pie -no-pie -ffunction-sections -fdata-sections -w \
  -I/repo/src -I/repo/src/headers -I/repo -I/usr/include/SDL2 -c narrow.c -o narrow.o
for mode in wide narrow; do
  cc -m32 -fno-pie -no-pie check.c "$mode.o" -Wl,--gc-sections \
    -Wl,--wrap=opendir,--wrap=closedir,--wrap=readdir,--wrap=readdir64,--wrap=stat,--wrap=stat64 -o "$mode"
done
./wide
if ./narrow > narrow.log 2>&1; then echo 'Narrow ABI mutant unexpectedly passed' >&2; exit 1; fi
echo 'PASS: former narrow readdir/stat ABI rejected'
'''
    subprocess.run(['docker', 'run', '--rm', '--platform', 'linux/amd64',
                    '-v', f'{root}:/repo:ro', '-v', f'{path}:/work', '-w', '/work',
                    'local/cod2-native-build:dev', 'bash', '-c', command], check=True)
