#!/usr/bin/env python3
"""Verify every corpse slot owns a tree and never aliases a live player slot."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/game_mp/g_main_mp.c').read_text()
start = source.index('static void G_InitPlayerAnimTrees(qboolean restart)\n{')
body = source[start:source.index('\n#ifndef __EMSCRIPTEN__', start)]
assert source.count('G_InitPlayerAnimTrees(restart);') == 2
support = r'''
#include <assert.h>
#include <string.h>
typedef int qboolean;
typedef struct { int slot; } XAnimTree;
typedef struct { XAnimTree *pXAnimTree; } clientInfo_t;
typedef struct { XAnimTree *tree;int entnum,time;clientInfo_t ci;int falling; } corpseInfo_t;
typedef struct { corpseInfo_t playerCorpseInfo[8]; } scr_data_t;
static struct { struct { struct { void *anims; } animTree; } animScriptData;clientInfo_t clientinfo[64]; } level_bgs;
static scr_data_t data;
static void *imp_g_scr_data=&data;
static XAnimTree pool[72];
static int allocated,loaded,mantleLoaded;
typedef void *(*MantleAnimAlloc)();
static void Hunk_AllocXAnimServer(void) {}
static void Mantle_CreateAnims(MantleAnimAlloc alloc) {
 assert(alloc==(MantleAnimAlloc)Hunk_AllocXAnimServer);mantleLoaded++;
}
static void BG_LoadAnim(void) { loaded++;level_bgs.animScriptData.animTree.anims=pool; }
static void *XAnimCreateTree(void *anims,void *alloc) {
 assert(anims==pool && allocated<72 && alloc==Hunk_AllocXAnimServer);
 pool[allocated].slot=allocated;return &pool[allocated++];
}
'''
checks = r'''
int main(void) {
 for(int round=0;round<12;round++) {
  memset(&data,0,sizeof(data));memset(&level_bgs,0,sizeof(level_bgs));allocated=loaded=mantleLoaded=0;
  G_InitPlayerAnimTrees(0);assert(loaded==1 && mantleLoaded==1 && allocated==72);
  for(int i=0;i<64;i++) assert(level_bgs.clientinfo[i].pXAnimTree==&pool[i]);
  for(int restart=0;restart<8;restart++) {
   for(int i=0;i<8;i++) {
    corpseInfo_t *c=&data.playerCorpseInfo[i];
    assert(c->tree==&pool[64+i] && c->ci.pXAnimTree==c->tree);
    assert(c->entnum==-1 && c->time==0 && c->falling==0);
    c->entnum=100+i;c->time=123;c->falling=1;
   }
   G_InitPlayerAnimTrees(1);assert(loaded==1 && mantleLoaded==1 && allocated==72);
  }
 }
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-corpse-init-') as directory:
    path = Path(directory)
    variants = (body, body.replace('corpse->tree = XAnimCreateTree(anims, (void *)Hunk_AllocXAnimServer);', ''),
                body.replace('corpse->entnum = -1;', 'corpse->entnum = 0;'))
    for index, variant in enumerate(variants):
        (path / 'test.c').write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True)
        assert (result.returncode == 0) == (index == 0), result.stderr.decode()
print('PASS: 64 live trees and 8 corpse trees across 12 loads/96 restarts; missing trees and zero entity sentinel fail')
