#!/usr/bin/env python3
"""Load the original hit-location table and exercise weapon multiplier selection."""
from pathlib import Path
import json
import subprocess
import tempfile
import zipfile

root = Path(__file__).resolve().parent.parent
combat = (root / 'src/PC/game_mp/g_combat_mp.c').read_text()
shared = (root / 'src/PC/universal/q_shared.c').read_text()

def function(source, name):
    start = source.index(name + '(')
    while source.find(';', start) < source.find('{', start):
        start = source.index(name + '(', start + len(name))
    start = source.rfind('\n', 0, start) + 1
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

with zipfile.ZipFile(root / 'data/browser/main/cod2_browser_renderer.iwd') as z:
    original = z.read('info/mp_lochit_dmgtable').decode('ascii')
support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef unsigned char byte;
typedef unsigned short scr_string_t;
typedef int qboolean,hitLocation_t;
#define qfalse 0
#define qtrue 1
#define WEAPTYPE_BULLET 0
typedef struct { const char *szName;int iOffset,iFieldType; } cspField_t;
typedef struct { int weapType;float locationDamageMultipliers[19]; } WeaponDef;
static WeaponDef weapon;
static float g_fHitLocDamageMult[19];
static scr_string_t g_HitLocConstNames[19];
static int valueindex,readOffset,strings;
static char value1[2][8192];
static void Com_Error(int level,const char *fmt,...) { fprintf(stderr,"%s\n",fmt);abort(); }
static int I_strnicmp_core(const char *a,const char *b,int n) { return strncasecmp(a,b,n); }
static void *BG_GetWeaponDef(int n) { return n==99 ? NULL : &weapon; }
static scr_string_t Scr_AllocString(const char *s,int user) { assert(s && *s && user==1);return ++strings; }
static int FS_FOpenFileByMode(const char *name,int *h,int mode) { assert(!strcmp(name,"info/mp_lochit_dmgtable"));*h=1;readOffset=0;return strlen(original); }
static int FS_Read(void *dest,int len,int h) { assert(h==1 && readOffset+len<=strlen(original));memcpy(dest,original+readOffset,len);readOffset+=len;return len; }
static void FS_FCloseFile(int h) { assert(h==1); }
'''
names = combat[combat.index('const char *g_HitLocNames[]'):combat.index('static scr_string_t g_HitLocConstNames')]
body = names + '\n'.join(function(shared, n) for n in (
    'Info_Validate', 'Info_ValueForKey', 'ParseConfigStringToStruct'))
body += '\n'.join(function(combat, n) for n in (
    'G_HitLocStrcpy', 'G_ParseHitLocDmgTable', 'G_GetHitLocDamageMult',
    'G_GetHitLocationString', 'G_GetHitLocationIndexFromString'))
checks = r'''
int main(void) {
 for(int round=0;round<12;round++) {
  memset(g_fHitLocDamageMult,0,sizeof(g_fHitLocDamageMult));strings=0;
  G_ParseHitLocDmgTable();assert(strings==19);
  assert(fabsf(g_fHitLocDamageMult[5]-.8f)<.0001f);
  assert(fabsf(g_fHitLocDamageMult[2]-1.5f)<.0001f);
  assert(g_fHitLocDamageMult[18]==0);
  for(int i=0;i<19;i++) {
   assert(G_GetHitLocationString(i)!=0);
   assert(G_GetHitLocationIndexFromString(G_GetHitLocationString(i))==i);
   weapon.locationDamageMultipliers[i]=i*.125f+1;
   weapon.weapType=WEAPTYPE_BULLET;
   assert(G_GetHitLocDamageMult(1,i)==weapon.locationDamageMultipliers[i]);
   weapon.weapType=1;
   assert(G_GetHitLocDamageMult(1,i)==g_fHitLocDamageMult[i]);
   assert(G_GetHitLocDamageMult(0,i)==g_fHitLocDamageMult[i]);
   assert(G_GetHitLocDamageMult(99,i)==g_fHitLocDamageMult[i]);
  }
 }
 return 0;
}
'''
init = (root / 'src/PC/game_mp/g_main_mp.c').read_text()
assert init.count('GScr_LoadConsts();\n    G_ParseHitLocDmgTable();') == 2
with tempfile.TemporaryDirectory(prefix='cod2-hitloc-damage-') as directory:
    path = Path(directory)
    variants = (body, body.replace('sizeof("LOCDMGTABLE") - 1', 'sizeof("LOCDMGTABLE")'),
                body.replace('->weapType != WEAPTYPE_BULLET', '->weapType == WEAPTYPE_BULLET'))
    for index, variant in enumerate(variants):
        (path / 'test.c').write_text('static const char original[]=' + json.dumps(original) + ';\n' + support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wno-incompatible-pointer-types',
                        '-fsanitize=address,undefined', str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True)
        assert (result.returncode == 0) == (index == 0), result.stderr.decode()
print('PASS: original damage table, 19 hit locations across 12 loads, bullet/explosive fallback; malformed header and inverted selector fail')
