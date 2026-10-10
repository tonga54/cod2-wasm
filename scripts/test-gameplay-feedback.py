#!/usr/bin/env python3
"""Exercise the actual cooling and HUD code with sanitizers and game clocks."""
from pathlib import Path
import subprocess
import tempfile
import re

root = Path(__file__).resolve().parent.parent

def function(path, name):
    source = (root / path).read_text()
    start = source.index(name + '(')
    # Forward declarations precede some definitions.
    while source.find(';', start) < source.find('{', start):
        start = source.index(name + '(', start + len(name))
    start = source.rfind('\n', 0, start) + 1
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

def run(source, name):
    with tempfile.TemporaryDirectory(prefix='cod2-' + name) as directory:
        p = Path(directory)
        (p/'test.c').write_text(source)
        subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-lm','-o',str(p/'test')], check=True)
        subprocess.run([str(p/'test')], check=True)

misc = 'src/PC/game_mp/g_misc_mp.c'
assert dict(re.findall(r'(GMISC_TURRET_(?:HEAT_MAX|SHOT_HEAT_SCALE|COOLING_PER_MS)) = (\d+)', (root/misc).read_text())) == {
    'GMISC_TURRET_HEAT_MAX':'40000', 'GMISC_TURRET_SHOT_HEAT_SCALE':'8',
    'GMISC_TURRET_COOLING_PER_MS':'5'}
run(r'''
#include <assert.h>
#include <string.h>
typedef int qboolean;
typedef struct {int inuse,fireTime;} turretInfo_s;
typedef struct {int heat,lastTime; qboolean overheated;} turretHeat_t;
static turretHeat_t turretHeat[32]; static turretInfo_s turretInfo[32];
typedef struct {int iFireTime;} WeaponDef;
typedef struct {int buttons;struct {int viewlocked;}ps;} client_t;
typedef struct {turretInfo_s *pTurretInfo;client_t *client;struct {int eFlags,time2,weapon;}s;} gentity_t;
static struct {int time;} level;
enum {GMISC_EF_FIRING=64,GMISC_BUTTON_ATTACK=1,GMISC_PLAYERVIEWLOCK_FULL=1,
 GMISC_TURRET_HEAT_MAX=40000,GMISC_TURRET_SHOT_HEAT_SCALE=8,GMISC_TURRET_COOLING_PER_MS=5};
static WeaponDef weapon={50}; static int shots;
static WeaponDef *BG_GetWeaponDef(int n){return &weapon;}
static void turret_clientaim(gentity_t*a,gentity_t*b){}
static void G_PlayerTurretPositionAndBlend(gentity_t*a,gentity_t*b){}
static void turret_shoot_internal(gentity_t*a,gentity_t*b){shots++;}
''' + function(misc,'turret_UpdateHeat') + function(misc,'turret_track') + r'''
int main(void){
 client_t client={.buttons=1};gentity_t player={.client=&client};gentity_t gun={.pTurretInfo=&turretInfo[0]};
 for(int i=0;i<100;i++){level.time+=50;turret_track(&gun,&player);}
 assert(shots==100 && turretHeat[0].overheated && gun.s.time2==(256|100));
 for(int i=0;i<159;i++){level.time+=50;turret_track(&gun,&player);assert(shots==100);}
 level.time+=50;turret_track(&gun,&player);assert(shots==101 && !turretHeat[0].overheated);
 // A slow firing weapon also builds heat between its individual shots.
 memset(turretHeat,0,sizeof(turretHeat));weapon.iFireTime=100;shots=0;gun.pTurretInfo->fireTime=0;
 for(int i=0;i<99;i++){level.time+=50;turret_track(&gun,&player);}
 assert(shots==50 && turretHeat[0].overheated);
 // Cooling stays on the gun when its user gets off, including long hitches.
 level.time+=1000;turret_UpdateHeat(&gun,0);assert(turretHeat[0].heat==35000);
 level.time+=10000;turret_UpdateHeat(&gun,0);assert(!turretHeat[0].heat && !turretHeat[0].overheated);
 gentity_t second={.pTurretInfo=&turretInfo[31]};turretHeat[31].heat=1000;turretHeat[31].lastTime=level.time;
 turret_UpdateHeat(&second,0);assert(turretHeat[31].heat==1000 && !turretHeat[0].heat);
 return 0;
}
''', 'turret')

hud = 'src/PC/cgame_mp/cg_draw_mp.c'
run(r'''
#include <assert.h>
#include <string.h>
typedef float vec4_t[4];typedef int MaterialHandle;typedef int FontHandle;
typedef struct {struct {int enabled;}current;}dvar_t;
static dvar_t draw={{1}},*cg_drawTurretCrosshair=&draw;
static struct {struct {int viewlocked,viewlocked_entNum,weapon;}predictedPlayerState;} state,*cg=&state;
static struct {struct {int whiteMaterial;}media;} media={.media={9}},*cgs=&media;
typedef struct {int nextValid;struct {int eType,weapon,time2;}nextState;}centity_t;
static centity_t cg_entities[1024];
typedef struct {int iReticleCenterSize;}WeaponDef;
typedef struct {int hReticleCenter;}weaponInfo_t;
static WeaponDef weapon={32};static weaponInfo_t weapons[129],*weaponList=weapons;
static void *imp_cg_weapons=&weaponList;
static int pics,reticles,texts,registered,materialAvailable=1;
static int BG_GetNumWeapons(void){return 42;}
static WeaponDef *BG_GetWeaponDef(int index){assert(index==42);return &weapon;}
static void CG_RegisterWeapon(int index){registered=index;weapons[index].hReticleCenter=materialAvailable?77:0;}
static FontHandle UI_GetFontHandle(int a,float b){return 1;}
static int UI_TextWidth(const char*t,int n,int f,float s){return 40;}
static void UI_DrawHandlePic(float x,float y,float w,float h,int ha,int va,const float*c,int mat){
 pics++;assert(ha==0&&va==0);
 if(mat==77){reticles++;assert(x==304&&y==224&&w==32&&h==32&&c[3]==.75f);}
 else {assert(mat==9&&h==5);} // Solid material is only for the heat bar.
}
static void UI_DrawText(const char*t,int n,int f,float x,float y,int ha,int va,float s,const float*c,int style){
 assert(!strcmp(t,"Cooling down..."));texts++;
}
''' + function(hud,'CG_DrawTurretCrossHair') + r'''
int main(void){
 cg->predictedPlayerState.viewlocked=1;cg->predictedPlayerState.viewlocked_entNum=1023;cg->predictedPlayerState.weapon=1;
 cg_entities[1023]=(centity_t){1,{9,42,256|100}};
 CG_DrawTurretCrossHair();assert(registered==42&&reticles==1&&pics==3&&texts==1);
 pics=reticles=texts=0;cg_entities[1023].nextState.time2=50;CG_DrawTurretCrossHair();assert(reticles==1&&pics==3&&!texts);
 pics=reticles=0;materialAvailable=0;CG_DrawTurretCrossHair();assert(!reticles&&pics==2);
 pics=0;cg_entities[1023].nextValid=0;CG_DrawTurretCrossHair();assert(!pics);
 cg_entities[1023].nextValid=1;cg_entities[1023].nextState.eType=1;CG_DrawTurretCrossHair();assert(!pics);
 cg_entities[1023].nextState.eType=9;cg->predictedPlayerState.viewlocked_entNum=1024;CG_DrawTurretCrossHair();assert(!pics);
 cg->predictedPlayerState.viewlocked_entNum=-1;CG_DrawTurretCrossHair();assert(!pics);
 cg->predictedPlayerState.viewlocked_entNum=1023;draw.current.enabled=0;CG_DrawTurretCrossHair();assert(!pics);
 return 0;
}
''', 'turret-reticle')
run(r'''
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
typedef float vec4_t[4];typedef float vec3_t[3];typedef int FontHandle;
typedef struct {struct {float value;int enabled;}current;}dvar_t;
static dvar_t width={{128,0}},height={{64,0}},offset={{128,0}},scope={{0,0}};
static dvar_t *cg_hudDamageIconWidth=&width,*cg_hudDamageIconHeight=&height,*cg_hudDamageIconOffset=&offset,*cg_hudDamageIconInScope=&scope;
typedef struct {int overlayReticle;} WeaponDef;static WeaponDef weapon;
static WeaponDef *BG_GetWeaponDef(int n){return &weapon;}
typedef struct {int time,duration;float yaw;}viewDamage_t;
typedef struct {int infoValid,team;char name[32];}clientInfo_t;
typedef struct {int nextValid;struct {int eType,clientNum,eFlags;}nextState;float lerpOrigin[3];}centity_t;
static centity_t cg_entities[1024];
static struct {int time;viewDamage_t viewDamage[8];struct {int weapon,clientNum;float fWeaponPosFrac;}predictedPlayerState;float refdefViewAngles[3];struct {float fov_x,fov_y;int width,height;float vieworg[3],viewaxis[3][3];}refdef;struct {clientInfo_t clientinfo[64];}bgs;} state,*cg=&state;
static struct {struct {int damageMaterial;}media;} cgsState={.media={1}},*cgs=&cgsState;
static int pics,texts;static float px,py,alpha,rotation,tx,ty;
static void CG_DrawRotatedPic(float x,float y,float w,float h,int ha,int va,float angle,const float*c,int mat){pics++;px=x+w*.5f;py=y+h*.5f;alpha=c[3];rotation=angle;assert(ha==7&&va==7);}
static FontHandle UI_GetFontHandle(int a,float b){return 1;}
static int UI_TextWidth(const char*t,int n,int f,float s){return 0;}
static float GetVirtualWidthFromRealWidth(float x){return x;}
static float GetVirtualHeightFromRealHeight(float x){return x;}
static void UI_DrawText(const char*t,int n,int f,float x,float y,int ha,int va,float s,const float*c,int style){texts++;tx=x;ty=y;assert(!strcmp(t,"Friend")&&ha==7&&va==7);}
''' + function(hud,'CG_DrawDamageIndicators') + function(hud,'CG_DrawFriendlyNames') + r'''
int main(void){
 cg->time=1100;cg->viewDamage[0]=(viewDamage_t){1000,2000,180}; // A source in front produces a backward travel vector.
 CG_DrawDamageIndicators();assert(pics==1&&fabsf(px)<.01&&fabsf(py+128)<.01&&fabsf(alpha-.95f)<.01);
 for(int yaw=0;yaw<=270;yaw+=90){pics=0;cg->viewDamage[0].yaw=yaw;CG_DrawDamageIndicators();assert(pics==1&&rotation==-(yaw+180));if(yaw==90)assert(px>0&&fabsf(py)<.01);if(yaw==0)assert(py>0);if(yaw==270)assert(px<0);}
 cg->refdefViewAngles[1]=90;cg->viewDamage[0].yaw=270;CG_DrawDamageIndicators();assert(fabsf(px)<.01&&py<0);
 pics=0;cg->time=3000;CG_DrawDamageIndicators();assert(!pics);
 cg->time=1100;cg->predictedPlayerState.fWeaponPosFrac=1;CG_DrawDamageIndicators();assert(pics==1);weapon.overlayReticle=1;CG_DrawDamageIndicators();assert(pics==1);
 cg->refdef.fov_x=90;cg->refdef.fov_y=90;cg->refdef.width=1280;cg->refdef.height=720;for(int i=0;i<3;i++)cg->refdef.viewaxis[i][i]=1;
 cg->bgs.clientinfo[0].team=1;cg->bgs.clientinfo[63]=(clientInfo_t){1,1,"Friend"};
 cg_entities[63]=(centity_t){.nextValid=1,.nextState={1,63,0},.lerpOrigin={1000,200,0}};
 CG_DrawFriendlyNames();assert(texts==1&&tx<0&&ty<0); // Visible while away from the crosshair.
 cg->bgs.clientinfo[63].team=2;CG_DrawFriendlyNames();assert(texts==1);
 cg->bgs.clientinfo[63].team=1;cg_entities[63].lerpOrigin[0]=-1000;CG_DrawFriendlyNames();assert(texts==1);
 cg_entities[63].lerpOrigin[0]=1000;cg_entities[63].nextValid=0;CG_DrawFriendlyNames();assert(texts==1);
 cg_entities[63].nextValid=1;cg->bgs.clientinfo[0].team=3;CG_DrawFriendlyNames();assert(texts==1);
 return 0;
}
''', 'hud')
# Compile the real center-print block with drawing stubs and a game clock.
source=(root/hud).read_text();start=source.index('    if (cg->centerPrintTime != 0) {');end=source.index('\n    if (!cg_drawGameMessages',start)
run(r'''
#include <assert.h>
#include <string.h>
typedef int FontHandle;
static struct {int centerPrintTime,centerPrintPriority,centerPrintCharWidth,centerPrintLines;float centerPrintY;char centerPrint[1024];int time;} state,*cg=&state;
static struct {struct {float value;}current;} timer={{3}},*cg_centertime=&timer,*cg_centerPrintY=&timer;
static int draws;
static float *CG_FadeColor(int start,int duration,int fade){static float c[4]={1,1,1,1};if(!start||cg->time-start>=duration)return 0;c[3]=1;return c;}
static int CL_IsRenderingSplitScreen(void){return 0;}
static FontHandle UI_GetFontHandle(int f,float s){return 0;}
static int UI_TextHeight(FontHandle f,float s){return 16;}
static int UI_TextWidth(const char*t,int n,int f,float s){return 40;}
static void UI_DrawText(const char*t,int n,int f,float x,float y,int ha,int va,float s,const float*c,int style){assert(c);draws++;}
static void draw(void){
''' + source[start:end] + r'''
}
int main(void){cg->centerPrintTime=1000;cg->centerPrintPriority=1;cg->centerPrintCharWidth=16;strcpy(cg->centerPrint,"Killed Player");cg->time=3999;draw();assert(draws==1);cg->time=4000;draw();assert(draws==1&&!cg->centerPrintTime&&!cg->centerPrintPriority);return 0;}
''', 'center-print')
print('PASS: MG42 original 32px reticle and 5s firing/8s cooling; independent guns; four damage directions, rotation/expiry/ADS; off-crosshair teammates at slot 63; expired kill text disappears')
