#!/usr/bin/env python3
"""Resolve scalar bot-roster entries without accessing stale or invalid clients."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/PC/game_mp/g_scr_main_mp.c').read_text()
m=re.search(r'^static void Scr_BotPlayer\([^;]*?\)\n\{',source,re.M)
assert m
end=m.end();depth=1
while depth:
    depth+=(source[end]=='{')-(source[end]=='}');end+=1
support=r'''
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
typedef struct {struct {int inuse;}r;void *client;} gentity_t;
static struct {int maxclients;} level;
static gentity_t *imp_g_entities;
static int selected,returned;
static int Scr_GetInt(unsigned index){assert(index==0);return selected;}
static void Scr_AddEntity(gentity_t *ent){assert(ent==imp_g_entities+selected);returned++;}
'''
checks=r'''
int main(void){
 for(level.maxclients=1;level.maxclients<=64;level.maxclients++){
  imp_g_entities=calloc(level.maxclients,sizeof(gentity_t));assert(imp_g_entities);
  for(int bits=0;bits<4;bits++){
   for(int i=0;i<level.maxclients;i++){imp_g_entities[i].r.inuse=bits&1;imp_g_entities[i].client=bits&2?(void*)1:NULL;}
   for(selected=-1;selected<=level.maxclients;selected++){returned=0;Scr_BotPlayer();assert(returned==((selected>=0&&selected<level.maxclients&&bits==3)?1:0));}
   selected=INT_MIN;returned=0;Scr_BotPlayer();assert(!returned);
   selected=INT_MAX;Scr_BotPlayer();assert(!returned);
  }
  free(imp_g_entities);
 }
 puts("PASS: scalar roster resolves only live client entities; every 1..64-player capacity, absent clients, disconnected slots and extreme indices are checked under ASan/UBSan");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-bot-roster-') as d:
    p=Path(d);(p/'test.c').write_text(support+source[m.start():end]+checks)
    subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True,timeout=30)
