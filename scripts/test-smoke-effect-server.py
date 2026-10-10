#!/usr/bin/env python3
"""Check dedicated smoke media registration and combined FX cleanup flags."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
s=(root/'src/PC/EffectsCore/FxTemplate.c').read_text();start=s.index('Bool PrimitiveTemplate_ParseModels(',s.index('Bool PrimitiveTemplate_ParseModels(')+1);end=s.index('{',start)+1;depth=1
while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
models=s[start:end]
s=(root/'src/PC/game_mp/g_main_mp.c').read_text();start=s.index('    if ((ent->s.eFlags & GMAIN_EFLAGS_UNKNOWN)');end=s.index('\n    if (level.time - ent->eventTime',start);cleanup=s[start:end]
support=r'''
#include <assert.h>
#include <string.h>
typedef int Bool;typedef unsigned char byte;
typedef struct {int handles;} MediaHandles;
typedef struct {MediaHandles mMediaHandles;} PrimitiveTemplate;
typedef struct GPValue {struct GPValue*next;const char*text;struct GPValue*list;} GPValue;
struct XModel {int n;};static struct XModel model;
typedef union {struct XModel *model;} TMediaElement;
#define GPV_NEXT(x) ((x)->next)
#define GPV_STRING(x) ((x)->text)
#define GPV_LIST(x) ((x)->list)
static int g_rendererExists,calls;
static int GPValue_IsList(GPValue*v){return v->list!=0;}
static const char *GPValue_GetTopValue(GPValue*v){return v->text;}
static int Com_ValidXModelName(const char*s){return s&&!strncmp(s,"xmodel/",7);}
static void FX_Print(const char*s,...){ }
static struct XModel *FX_XModelPrecache(const char*s){assert(g_rendererExists);calls++;return &model;}
static void MediaHandles_AddHandle(MediaHandles*m,TMediaElement e){m->handles++;}
typedef struct {struct {int eFlags,eType,time2;}s;}gentity_t;
static struct {int time;}level;static int freed;
enum {GMAIN_EFLAGS_UNKNOWN=65536,GMAIN_ET_GENERAL=0};
static void G_FreeEntity(gentity_t*e){freed++;}
static void cleanup(gentity_t *ent){
'''
checks=r'''
}
int main(void){
 PrimitiveTemplate primitive={0};GPValue single={.text="xmodel/weapon_us_smoke_grenade_burnt"};GPValue list={.list=&single};
 g_rendererExists=0;assert(PrimitiveTemplate_ParseModels(&primitive,&single));assert(PrimitiveTemplate_ParseModels(&primitive,&list));assert(!calls&&!primitive.mMediaHandles.handles);
 g_rendererExists=1;assert(PrimitiveTemplate_ParseModels(&primitive,&single));assert(PrimitiveTemplate_ParseModels(&primitive,&list));assert(calls==2&&primitive.mMediaHandles.handles==2);
 gentity_t smoke={.s={65536|32,0,60000}};level.time=60000;cleanup(&smoke);assert(!freed);level.time++;cleanup(&smoke);assert(freed==1);
 smoke.s.eType=9;cleanup(&smoke);assert(freed==1);smoke.s.eType=0;smoke.s.eFlags=32;cleanup(&smoke);assert(freed==1);
 return 0;
}
'''
# Model registration and the frame cleanup have independent translation units.
source=support.split('typedef struct {struct {int eFlags')[0]+models+support[support.index('typedef struct {struct {int eFlags'):]+cleanup+checks
with tempfile.TemporaryDirectory(prefix='cod2-smoke-server-') as d:
 p=Path(d);(p/'test.c').write_text(source);subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
# Imported dvars are pointers to pointers. Read the actual dedicated branches.
server_source=(root/'src/PC/server_mp/sv_init_mp.c').read_text()
import re
reads=re.findall(r'isDedicated = ([^;]+);',server_source)
assert len(reads)==4
for expression in reads:
    assert expression == '(*(const dvar_t **)imp_com_dedicated)->current.integer'
print('PASS: dedicated smoke skips renderer callbacks; clients load burnt grenade models; combined effect flags expire without freeing turrets')
