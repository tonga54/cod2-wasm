#!/usr/bin/env python3
"""Run production view-weapon and item movement against deterministic inputs."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent

def function(path, name):
    source = (root/path).read_text()
    match = re.search(r'^([^\n]*\b'+re.escape(name)+r'\([^;]*?\))\s*\{', source, re.M)
    assert match, name
    start = match.start()
    brace = source.index('{', match.start())
    depth, end = 1, brace+1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]+'\n'

mathfile = 'src/PC/universal/com_math.c'
mathbody = ''.join(function(mathfile, name) for name in [
    'Vec3Cross_core', 'Vec3Cross', 'Vec3Normalize_core', 'Vec3Normalize', 'Vec3DistanceSq',
    'vectoangles', 'AngleVectors', 'AnglesToAxis', 'AxisToAngles', 'MatrixMultiply'])
common = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __attribute_regparm__(x)
#define __attribute_sseregparm__
typedef float vec_t; typedef float vec3_t[3]; typedef int qboolean;
static void AxisCopy(vec3_t *in,vec3_t *out) {memcpy(out,in,9*sizeof(float));}
'''
viewtypes = r'''
#define PMF_SPRINT 0x02000000
typedef struct {int pm_type,pm_flags,eFlags,commandTime,weapon,weaponstate;float fWeaponPosFrac;} playerState_t;
static float sprintViewBlend;
static int sprintViewTime;
typedef struct {playerState_t *ps;float xyspeed,frametime;vec3_t vLastMoveAng;float fLastIdleFactor;
    int time,damageTime;float v_dmg_pitch,v_dmg_roll;vec3_t vGunOffset,vGunSpeed,swayAngles;int *weapIdleTime;} weaponState_t;
typedef struct {int renderFxFlags;vec3_t axis[3],origin;} GfxEntity;
typedef struct {struct {int enabled;float value;} current;} dvar_t;
typedef struct {vec3_t vLastMoveAng;float fLastIdleFactor;} centity_t;
typedef struct {struct {vec3_t viewaxis[3],vieworg;} refdef;vec3_t refdefViewAngles;
    int renderingThirdPerson,cubemapShot,frametime,time,damageTime,weapIdleTime;
    struct {int hasSavedScreen;} shellshock;centity_t playerEntity,predictedPlayerEntity;
    float xyspeed,v_dmg_pitch,v_dmg_roll,gunPitch,gunYaw,gunXOfs,gunYOfs,gunZOfs;
    vec3_t swayViewAngles,swayOffset,swayAngles,vGunOffset,vGunSpeed;} cg_t;
static cg_t storage,*cg=&storage;
static dvar_t draw={.current.enabled=1},zero;
static dvar_t *cg_drawGun=&draw,*zeroPtr=&zero;
static void *imp_cg_gun_x=&zeroPtr,*imp_cg_gun_y=&zeroPtr,*imp_cg_gun_z=&zeroPtr;
static float CG_DvarValue(void *p) {return (*(dvar_t **)p)->current.value;}
static int BG_GetViewmodelWeaponIndex(playerState_t *ps) {return ps->weapon;}
static int CG_GetWeapReticleZoom(float *zoom) {*zoom=0;return 0;}
static int BG_IsAimDownSightWeapon(int weapon) {return 1;}
static void BG_CalculateWeaponPosition_Sway(playerState_t *ps,vec_t *a,vec_t *b,vec_t *c,float scale,int time) {}
static GfxEntity lastHand;static int draws;
static void CG_AddPlayerWeapon(GfxEntity *hand,playerState_t *ps,centity_t *cent,int drawGun) {lastHand=*hand;draws+=drawGun;}
'''
recoil = function('src/PC/bgame/bg_weapons.c', 'BG_CalculateWeaponPosition_GunRecoil_SingleAngle')
viewstub = r'''
static void BG_CalculateWeaponAngles(weaponState_t *ws,vec_t *angles) {
    assert(ws->time==cg->time);
    for(int a=0;a<2;a++) BG_CalculateWeaponPosition_GunRecoil_SingleAngle(
        &ws->vGunOffset[a],&ws->vGunSpeed[a],ws->frametime,8,50,100,8,10);
    ws->vLastMoveAng[0]+=ws->frametime;
    ws->fLastIdleFactor+=ws->frametime;
    angles[0]=ws->vGunOffset[0];angles[1]=ws->vGunOffset[1];angles[2]=0;
}
'''
viewbody = ''.join(function('src/PC/cgame_mp/cg_weapons.c', name) for name in [
    'CG_Madd','CG_ResetViewWeaponOffsets','CG_ResetSprintView','CG_SprintViewBlend','CG_AddViewWeapon'])
viewchecks = r'''
int main(void) {
    playerState_t ps={.weapon=1,.fWeaponPosFrac=1};cg->frametime=16;cg->time=10000;
    for(int pitch=-60;pitch<=60;pitch+=15) for(int yaw=-180;yaw<=180;yaw+=10) {
        cg->refdefViewAngles[0]=pitch;cg->refdefViewAngles[1]=yaw;
        AnglesToAxis(cg->refdefViewAngles,cg->refdef.viewaxis);
        cg->vGunOffset[0]=cg->vGunOffset[1]=0;
        cg->vGunSpeed[0]=cg->vGunSpeed[1]=0;
        CG_AddViewWeapon(&ps);
        vec3_t actual={cg->gunPitch,cg->gunYaw,cg->refdefViewAngles[2]},forward;
        AngleVectors(actual,forward,NULL,NULL);
        assert(Vec3DistanceSq(forward,cg->refdef.viewaxis[0])<1e-9f);
    }
    cg->playerEntity.fLastIdleFactor=0;cg->playerEntity.vLastMoveAng[0]=0;
    for(int frame=0;frame<10000;frame++) {
        if(frame%6==0) cg->vGunSpeed[0]+=30;
        cg->time+=16;ps.commandTime=cg->time-20;CG_AddViewWeapon(&ps);
        assert(fabsf(cg->vGunSpeed[0])<=100);
        assert(fabsf(cg->vGunOffset[0])<=8);
        assert(isfinite(cg->gunPitch)&&isfinite(cg->gunYaw));
        /* Angular recoil must not move the weapon origin away from the eye. */
        assert(Vec3DistanceSq(lastHand.origin,cg->refdef.vieworg)==0);
    }
    for(int frame=0;frame<1000;frame++) {cg->time+=16;CG_AddViewWeapon(&ps);}
    assert(cg->vGunSpeed[0]==0&&cg->vGunOffset[0]==0);
    assert(cg->playerEntity.fLastIdleFactor>170&&cg->playerEntity.vLastMoveAng[0]>170);
    assert(draws>11000);
    return 0;
}
'''
itemtypes = r'''
enum {IT_WEAPON=1};
typedef struct {int trType,trTime,trDuration;vec3_t trBase,trDelta;} trajectory_t;
typedef struct {struct {int groundEntityNum;trajectory_t pos,apos;struct {int item;} index;} s;
    struct {int inuse,ownerNum;vec3_t currentOrigin,currentAngles,mins,maxs;} r;
    int spawnflags,tagInfo,clipmask,active;} gentity_t;
typedef struct {int giType;} gitem_t;
typedef struct {float fraction;int startsolid,entityNum;vec3_t normal;} trace_t;
static struct {int time;gentity_t gentities[1024];} level;
static gitem_t items[1]={{IT_WEAPON}};static void *imp_bg_itemlist=items;
static int freeOnThink,freed,links;static vec3_t landingNormal={0,0,1};
static void BG_EvaluateTrajectory(trajectory_t *tr,int time,vec_t *out) {
    float dt=(time-tr->trTime)*.001f;
    for(int a=0;a<3;a++)out[a]=tr->trBase[a]+tr->trDelta[a]*dt;
    if(tr->trType==5)out[2]-=400*dt*dt;
}
static void G_TraceCapsule(trace_t *tr,const vec_t *a,const vec_t *mins,const vec_t *maxs,
    const vec_t *b,int owner,int mask) {*tr=(trace_t){.fraction=0,.entityNum=1022};memcpy(tr->normal,landingNormal,sizeof(vec3_t));}
static void SV_LinkEntity(gentity_t *ent) {links++;}
static void G_RunThink(gentity_t *ent) {if(freeOnThink)ent->r.inuse=0;}
static int SV_PointContents(vec_t *p,int owner,int mask) {return 0;}
static void G_FreeEntity(gentity_t *ent) {ent->r.inuse=0;freed++;}
static void G_SetAngle(gentity_t *ent,const vec_t *a) {memcpy(ent->r.currentAngles,a,sizeof(vec3_t));ent->s.apos.trType=0;}
static void G_SetOrigin(gentity_t *ent,const vec_t *p) {memcpy(ent->r.currentOrigin,p,sizeof(vec3_t));ent->s.pos.trType=0;}
'''
itembody = ''.join(function('src/PC/game_mp/g_items_mp.c',name) for name in [
    'G_SetVec3','G_ItemGroundAngles','G_RunItem'])
itemchecks = r'''
int main(void) {
    for(int slope=0;slope<=60;slope+=10) for(int yaw=0;yaw<360;yaw+=15) {
        landingNormal[0]=sinf(slope*.01745329252f);landingNormal[2]=cosf(slope*.01745329252f);
        gentity_t ent={0};ent.r.inuse=1;ent.active=0;ent.s.groundEntityNum=1023;
        ent.s.pos.trType=5;ent.s.apos.trType=2;ent.s.apos.trBase[1]=yaw;
        ent.s.apos.trDelta[1]=30;level.time=500;
        G_RunItem(&ent);
        assert(ent.s.pos.trType==0&&ent.s.apos.trType==0&&ent.s.groundEntityNum==1022);
        vec3_t axis[3];AnglesToAxis(ent.r.currentAngles,axis);
        /* Weapons rest on their side: the left row is the surface normal. */
        assert(Vec3DistanceSq(axis[1],landingNormal)<1e-8f);
        for(int a=0;a<3;a++)assert(isfinite(ent.r.currentAngles[a]));
        vec3_t saved;memcpy(saved,ent.r.currentAngles,sizeof(saved));
        level.time+=500;G_RunItem(&ent);assert(!memcmp(saved,ent.r.currentAngles,sizeof(saved)));
    }
    assert(freed==0);
    freeOnThink=1;gentity_t dead={0};dead.r.inuse=1;dead.active=1;dead.s.pos.trType=5;
    dead.s.groundEntityNum=1023;dead.s.apos.trType=2;G_RunItem(&dead);
    assert(dead.s.pos.trType!=0&&dead.s.apos.trType==2);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-weapon-regression-') as directory:
    tmp=Path(directory)
    suites=[('view',viewtypes+mathbody+recoil+viewstub,viewbody,viewchecks,[
        viewbody.replace('cg->gunPitch = worldAngles[0];','cg->gunPitch = weaponAngles[0];')
                .replace('cg->gunYaw = worldAngles[1];','cg->gunYaw = weaponAngles[1];'),
        viewbody.replace('cg->vGunSpeed[i] = ws.vGunSpeed[i];',''),
        viewbody.replace('ws.time = cg->time;','ws.time = cg->time - playerState->commandTime;')]),
        ('ground',mathbody+itemtypes,itembody,itemchecks,[
        itembody.replace('ent->r.inuse && tr.fraction','ent->active && tr.fraction')])]
    for name,support,body,checks,mutants in suites:
        for index,variant in enumerate([body]+mutants):
            path=tmp/(name+'.c');path.write_text(common+support+variant+checks)
            subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(path),'-lm','-o',str(tmp/name)],check=True)
            result=subprocess.run([str(tmp/name)],capture_output=True,text=True)
            assert (result.returncode==0)==(index==0),(name,index,result.stderr)
print('PASS: 333 world-space scope directions, 11,000 recoil frames, 168 slope/yaw landings, freed entities and 4 failing regression mutants')
