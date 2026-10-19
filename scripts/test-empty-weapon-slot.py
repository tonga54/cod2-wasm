#!/usr/bin/env python3
"""Restore original S&D empty weapon slots using the production native setter."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/PC/game_mp/g_client_script_cmd_mp.c').read_text()
m=re.search(r'^void PlayerCmd_SetWeaponSlotWeapon\([^;]*?\)\n\{',source,re.M)
assert m
end=m.end();depth=1
while depth:
    depth+=(source[end]=='{')-(source[end]=='}');end+=1
body=source[m.start():end]
support=r'''
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#define WEAPSLOT_PRIMARY 1
#define WEAPSLOT_PRIMARYB 2
#define I_stricmp strcasecmp
typedef struct {unsigned short entnum,classnum;} scr_entref_t;
typedef struct {unsigned char weaponslots[3];int ammo[128];} playerState_t;
typedef struct {playerState_t ps;} gclient_t;
typedef struct {gclient_t *client;} gentity_t;
typedef struct {int weapSlot,iStartAmmo,iAmmoIndex;} WeaponDef;
static gclient_t client;static gentity_t entity={&client};static WeaponDef def={1,30,7};
static int slot,lookups,takes,gives,ammoAdds,owned[128];static const char *name;
static jmp_buf onError;static int errors;
static gentity_t *PlayerCmd_GetPlayerEntity(scr_entref_t ref){assert(!ref.entnum&&!ref.classnum);return &entity;}
static int PlayerCmd_GetWeaponSlotParam(unsigned index){assert(index==0);return slot;}
static const char *Scr_GetString(unsigned index){assert(index==1);return name;}
static int G_GetWeaponIndexForName(const char *s){lookups++;return strcmp(s,"rifle_mp")==0?7:0;}
static void Scr_ParamError(unsigned index,const char *s){(void)s;assert(index==1);errors++;longjmp(onError,1);}
static const char *va(const char *fmt,...){return fmt;}
static WeaponDef *BG_GetWeaponDef(int index){assert(index==7);return &def;}
static unsigned BG_GetSlotAttachments(const playerState_t *ps,int slot){(void)ps;assert(slot==WEAPSLOT_PRIMARY);return 0;}
static void BG_SetSlotAttachments(playerState_t *ps,int slot,unsigned mask){(void)ps;(void)slot;assert(!mask);}
static const char *BG_GetWeaponSlotNameForIndex(int index){assert(index==1||index==2);return "primary";}
static void BG_TakePlayerWeapon(playerState_t *ps,int weapon){takes++;owned[weapon]=0;for(int i=1;i<=2;i++)if(ps->weaponslots[i]==weapon)ps->weaponslots[i]=0;}
static int PlayerCmd_HasWeapon(playerState_t *ps,int weapon){(void)ps;return owned[weapon];}
static void G_GivePlayerWeapon(playerState_t *ps,int weapon){gives++;owned[weapon]=1;ps->weaponslots[ps->weaponslots[1]?2:1]=weapon;}
static void Add_Ammo(gentity_t *ent,int weapon,int amount,int fill){assert(ent==&entity&&weapon==7&&amount>0);(void)fill;ammoAdds++;client.ps.ammo[7]+=amount;}
'''
checks=r'''
int main(void){
 scr_entref_t ref={0,0};const char *empty[]={"","none","NONE","None"};
 for(int pass=0;pass<256;pass++)for(slot=1;slot<=2;slot++)for(int variant=0;variant<4;variant++){
  memset(&client,0,sizeof(client));memset(owned,0,sizeof(owned));
  client.ps.weaponslots[slot]=7;client.ps.weaponslots[3-slot]=8;owned[7]=owned[8]=1;
  client.ps.ammo[7]=17;lookups=takes=gives=ammoAdds=0;name=empty[variant];
  if(setjmp(onError))abort();PlayerCmd_SetWeaponSlotWeapon(ref);
  assert(client.ps.weaponslots[slot]==0&&client.ps.weaponslots[3-slot]==8);
  assert(!owned[7]&&owned[8]&&takes==1&&!lookups&&!gives&&!ammoAdds&&client.ps.ammo[7]==17);
 }
 for(slot=1;slot<=2;slot++){
  memset(&client,0,sizeof(client));memset(owned,0,sizeof(owned));name="rifle_mp";lookups=takes=gives=ammoAdds=0;
  PlayerCmd_SetWeaponSlotWeapon(ref);assert(client.ps.weaponslots[slot]==7&&!client.ps.weaponslots[3-slot]);
  assert(owned[7]&&lookups==1&&gives==1&&ammoAdds==1&&client.ps.ammo[7]==30);
 }
 name="missing_mp";errors=0;slot=1;unsigned old=client.ps.weaponslots[1];
 if(!setjmp(onError)){PlayerCmd_SetWeaponSlotWeapon(ref);abort();}
 assert(errors==1&&client.ps.weaponslots[1]==old);
 puts("PASS: 2,048 S&D empty-slot restores clear only the requested slot; normal weapons, secondary placement, ammo and unknown-name errors are preserved");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-empty-slot-') as d:
    p=Path(d);(p/'test.c').write_text(support+body+checks)
    subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True,timeout=30)
