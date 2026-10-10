#!/usr/bin/env python3
"""Run actual trigger initialization and scripted movement of an off-map bomb."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent


def extract(source, signature):
    start = source.index(signature + '\n{')
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


initialize = extract((root / 'src/PC/game_mp/g_mover_mp.c').read_text(),
                     'static void trigger_use_shared(gentity_t *ent)')
move = extract((root / 'src/PC/game_mp/g_scr_main_mp.c').read_text(),
               'void Scr_SetOrigin(gentity_t *ent, int offset)')
spawn=(root / 'src/PC/game_mp/g_spawn_mp.c').read_text()
fields=spawn.split('static const ent_field_t fields[11] = {',1)[1].split('\n};',1)[0]
fields='static const ent_field_t fields[11] = {'+fields+'\n};'
dispatch=extract(spawn,'static qboolean Scr_SetEntityField(int entnum, int offset)')
angles=extract((root / 'src/PC/game_mp/g_scr_main_mp.c').read_text(),'void Scr_SetAngles(gentity_t *ent, int offset)')
health=extract((root / 'src/PC/game_mp/g_scr_main_mp.c').read_text(),'void Scr_SetHealth(gentity_t *ent, int offset)')
support = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
typedef float vec3_t[3];
typedef unsigned char byte;typedef int qboolean;
static float newOrigin[3];
#define F_STRING 1
#define F_VECTOR 2
#define F_MODEL 3
#define F_INT 4
typedef struct {struct {int stats[1];}ps;}gclient_t;
typedef struct gentity_s {int classname,model,spawnflags,target,targetname,count,health,maxHealth,damage;
 gclient_t *client;struct {int singleUserEntIndex;}trigger;int handler;
 struct {int dmgFlags;unsigned char scale;struct {int trType;float trBase[3];}pos;}s;
 struct {int linked,svFlags,contents;float currentOrigin[3],currentAngles[3],absmin[3];}r;
}gentity_t;
typedef void (*ScriptCallbackEnt)(gentity_t*,int);
typedef struct {const char *name;int ofs,type;ScriptCallbackEnt callback;}ent_field_t;
void Scr_SetOrigin(gentity_t*,int);void Scr_SetAngles(gentity_t*,int);void Scr_SetHealth(gentity_t*,int);
static gentity_t *entities;
static gentity_t *G_Entities(void){return entities;}
static void Scr_SetClientField(gclient_t *client,int offset){(void)client;(void)offset;assert(0);}
static int Scr_GetInt(int index){assert(index==0);return 63;}
static void Scr_SetGenericField(byte *ent,int type,int offset){
 if(type==F_VECTOR)memcpy(ent+offset,newOrigin,sizeof(vec3_t));
 else if(type==F_INT)*(int*)(ent+offset)=63;
}
#define _ENT(e) (e)
#define VectorCopy(a,b) memcpy(b,a,sizeof(vec3_t))
static const char *hintStrings[6]={NULL,"HINT_ACTIVATE","HINT_BOMB",NULL,NULL,NULL};
static int links,phase;
static void SV_LinkEntity(gentity_t *ent){
 links++;
 // The original BSP bombtrigger starts outside the leaf tree. Server-only
 // svFlag 1 lets its use contents register there and move later via script.
 if(ent->r.svFlags!=1){ent->r.linked=0;return;}
 assert(ent->r.contents==0x200000&&ent->handler==0x12);
 ent->r.linked=1;memcpy(ent->r.absmin,ent->r.currentOrigin,sizeof(vec3_t));
}
static void SV_SetBrushModel(gentity_t *ent){SV_LinkEntity(ent);}
static int G_SpawnString(const char *name,const char *fallback,const char **out){
 (void)fallback;
 if(phase&&strcmp(name,"hintstring")==0){*out="Hold to defuse";return 1;}
 return 0;
}
static int I_stricmp(const char *a,const char *b){return strcmp(a,b);}
static void SV_GetConfigstring(int index,char *buffer,int size){(void)index;(void)size;buffer[0]=0;}
static void SV_SetConfigstring(int index,const char *text){assert(index==0x4fe&&text);}
static void Com_Error(int type,const char *message,int max){(void)type;(void)message;(void)max;abort();}
static void Scr_GetVector(int index,float *out){assert(index==0);memcpy(out,newOrigin,sizeof(vec3_t));}
static void G_SetOrigin(gentity_t *ent,const float *at){memcpy(ent->r.currentOrigin,at,sizeof(vec3_t));}
static void G_SetAngle(gentity_t *ent,const float *at){memcpy(ent->r.currentAngles,at,sizeof(vec3_t));}
'''
checks = r'''
int main(void){
 for(phase=0;phase<2;phase++){
  gentity_t ent={0};links=0;ent.r.currentOrigin[0]=-2144;ent.r.currentOrigin[1]=6176;ent.r.currentOrigin[2]=528;
  trigger_use_shared(&ent);
  assert(ent.r.linked&&links==2&&ent.s.dmgFlags==2&&ent.trigger.singleUserEntIndex==1023);
  assert(ent.s.scale==(phase?0:255));
  newOrigin[0]=1104;newOrigin[1]=2750;newOrigin[2]=104;
  entities=&ent;Scr_SetEntityField(0,1);assert(links==3);
  for(int i=0;i<3;i++)assert(ent.r.absmin[i]==newOrigin[i]);
  Scr_SetEntityField(0,9);assert(links==4&&ent.r.currentAngles[0]==newOrigin[0]);
  gclient_t client={0};ent.client=&client;Scr_SetEntityField(0,7);
  assert(ent.health==63&&client.ps.stats[0]==63);
  ent.r.linked=0;newOrigin[2]=-10000;Scr_SetEntityField(0,1);assert(links==4);
 }
 puts("PASS: off-map defuse trigger registers final flags/contents, survives hint setup and follows script origin; intentionally unlinked entities stay unlinked");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-use-linkage-') as directory:
    path = Path(directory)
    for name in ('fixed', 'early-link', 'raw-origin-field', 'raw-health-field'):
        current = initialize if name != 'early-link' else initialize.replace(
            '    SV_LinkEntity(ent);', '').replace('    SV_SetBrushModel(ent);',
                                                   '    SV_SetBrushModel(ent);\n    SV_LinkEntity(ent);')
        code = path / (name + '.c')
        binary = path / name
        callbacks=fields
        if name=='raw-origin-field':callbacks=fields.replace('(ScriptCallbackEnt)Scr_SetOrigin','NULL')
        if name=='raw-health-field':callbacks=fields.replace('(ScriptCallbackEnt)Scr_SetHealth','NULL')
        code.write_text(support + current + move + angles + health + callbacks + dispatch + checks)
        subprocess.run(['cc', '-std=c99', '-O2', str(code), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
        if name == 'fixed':
            assert result.returncode == 0, result.stderr
            print(result.stdout.strip())
        else:
            assert result.returncode != 0, 'Old linkage unexpectedly passed'
            print('PASS: rejected '+name)
