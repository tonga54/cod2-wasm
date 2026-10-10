#!/usr/bin/env python3
"""Exercise the real 32-bit string/allocator restart path under ASan/UBSan."""
from pathlib import Path
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[1]
SUPPORT = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
typedef unsigned char byte;
typedef unsigned short scr_string_t;
typedef struct scrMemTreePub_t {char *mt_buffer;} scrMemTreePub_t;
struct scrMemTreeGlob_t {char nodes[65536*8]; byte leftBits[256],numBits[256],logBits[256]; unsigned short head[17]; int totalAlloc,totalAllocBuckets;};
scrMemTreePub_t scrMemTreePub;
_Alignas(8) unsigned char scrStringGlob[65552],scrMemTreeGlob[sizeof(struct scrMemTreeGlob_t)];
static void *test_restart;
byte *Z_VirtualAllocInternal(int n){return calloc(1,n);} void Z_VirtualFreeInternal(void *p){free(p);}
void Com_Printf(const char *fmt,...){va_list a;va_start(a,fmt);vfprintf(stderr,fmt,a);va_end(a);}
void Com_Error(int code,const char *fmt,...){va_list a;va_start(a,fmt);vfprintf(stderr,fmt,a);va_end(a);abort();}
void Scr_TerminalError(const char *s){fprintf(stderr,"%s",s);abort();}
void Scr_DumpScriptThreads(void){} void Scr_DumpScriptVariables(void){}
'''
CHECKS = r'''
int main(void){
 SL_Init();
 char names[3000][50]; unsigned ids[3000];
 for(int i=0;i<3000;i++){
  snprintf(names[i],50,"animation_%d_abcdefghijklmnopqrstuvwxyz",i);
  ids[i]=SL_GetString_(names[i],4,4);
 }
 unsigned baseline=Scr_GetStringUsage();
 for(int pass=0;pass<100;pass++){
  for(int i=0;i<3000;i++){
   assert(SL_GetString_(names[i],2,4)==ids[i]);
   char n[70];snprintf(n,70,"temporary_%d_%d_abcdefghijklmnop",pass,i);
   unsigned temp=SL_GetString_(n,1,4);
   assert(!strcmp(SL_ConvertToString(temp),n) && !MT_IsNodeCovered(temp));
  }
  SL_ShutdownSystem(2);Scr_ShutdownGameStrings();
  // Scr_Abort/Com_Restart retain permanent constants and discard session refs.
  for(int i=0;i<1000;i++){
   char n[50];snprintf(n,50,"restart_transient_%d",i);SL_GetString_(n,0,4);
  }
  SL_Init();
  assert(Scr_GetStringUsage()==baseline);
  for(int i=0;i<3000;i++){
   assert(!strcmp(SL_ConvertToString(ids[i]),names[i]));
   assert(SL_FindString(names[i])==ids[i] && !MT_IsNodeCovered(ids[i]));
  }
 }
 puts("PASS: 100 script restarts preserve live string identities and reclaim session allocations");
}
'''
parts = []
for name in ('scr_memorytree', 'scr_stringlist'):
    source = (ROOT / f'src/PC/script/{name}.c').read_text()
    source = '\n'.join(line for line in source.splitlines() if not line.startswith('#include'))
    # Select the WASM/i386 reclaim path even on an x64 test host. Keep the
    # restart pointer in an aligned fixture slot instead of its 32-bit ABI slot.
    source = source.replace('#if defined(_M_X64) || defined(__x86_64__)', '#if 0')
    source = source.replace('#define SG_RESTART (*(void **)((char *)&scrStringGlob + 65540))', '#define SG_RESTART test_restart')
    parts.append(source)
body = SUPPORT + '\n'.join(parts) + CHECKS
cases = {'fixed': body}
if '--verify-mutants' in sys.argv:
    cases['stale-free-lists'] = body.replace('MEMTREE_GLOB->head[i] = 0;', '(void)i;', 1).replace('mt_size[i] = 0;', '(void)i;', 1)
    cases['freed-chain-link'] = body.replace('((ecx3_hash[0] & 0x3fff) | 0x8000)', '((esi3_chain & 0x3fff) | 0x8000)')
with tempfile.TemporaryDirectory(prefix='cod2-string-restart-') as tmp:
    for name, source in cases.items():
        file = Path(tmp) / f'{name}.c'
        file.write_text(source)
        binary = file.with_suffix('')
        subprocess.run(['cc', '-std=c11', '-O1', '-g', '-Wno-deprecated-declarations', '-fsanitize=address,undefined', str(file), '-o', str(binary)], check=True)
        try:
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=60 if name == 'fixed' else 15)
        except subprocess.TimeoutExpired:
            assert name != 'fixed', 'fixed restart hung'
            print(f'PASS: {name} mutant hung and was rejected')
            continue
        if name == 'fixed':
            assert result.returncode == 0, result.stderr
            print(result.stdout.strip())
        else:
            assert result.returncode != 0, f'{name} mutant survived'
            print(f'PASS: {name} mutant rejected')
