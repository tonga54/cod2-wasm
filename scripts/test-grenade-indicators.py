#!/usr/bin/env python3
"""Exercise production grenade warning drawing against actual splash boundaries."""
from pathlib import Path
import re
import subprocess
import tempfile
import zipfile
from weapon_balance import weapon_fields

root = Path(__file__).resolve().parent.parent
draw_source = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
combat_source = (root / 'src/PC/game_mp/g_combat_mp.c').read_text()
main_source = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()


def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


distance = function(draw_source, 'CG_GrenadeThreatDistance')
draw = function(draw_source, 'CG_DrawGrenadeIndicators')
splash = function(combat_source, 'G_RadiusDamage')
rows = []
with zipfile.ZipFile(root / 'data/browser/main/cod2_browser_renderer.iwd') as archive:
    for name in sorted(n for n in archive.namelist() if n.startswith('weapons/mp/frag_grenade_')):
        fields = weapon_fields(archive.read(name))
        assert fields['weaponType'] == 'grenade' and fields['offhandClass'] == 'Frag Grenade'
        rows.append('{' + ','.join(fields[k] for k in
                    ('explosionRadius', 'explosionInnerDamage', 'explosionOuterDamage')) + '}')
    assert len(rows) == 3
    for name in ('hud_grenadeicon', 'hud_grenadepointer'):
        assert archive.getinfo('materials/' + name).file_size > 0
    for name in ('grenadeicon', 'grenadepointer'):
        assert archive.read('images/' + name + '.iwi').startswith(b'IWi\x05')

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define WEAPTYPE_GRENADE 1
#define OFFHAND_CLASS_FRAG_GRENADE 1
typedef int qboolean,MaterialHandle;
typedef float vec_t,vec3_t[3],vec4_t[4];
typedef struct {int number,eType,eFlags,time,weapon;} entityState_t;
typedef struct {entityState_t nextState;int nextValid;vec3_t lerpOrigin;} centity_t;
typedef struct {int pm_type,weapon,stats[1];float fWeaponPosFrac;vec3_t origin;} playerState_t;
typedef struct {int numEntities;entityState_t entities[256];} snapshot_t;
typedef struct {snapshot_t *snap,*nextSnap;playerState_t predictedPlayerState;
 int time,renderingThirdPerson;vec3_t refdefViewAngles;} cg_t;
typedef struct {struct {int grenadeIcon,grenadePointer;} media;} cgs_t;
typedef struct {int weapType,offhandClass,iExplosionRadius,overlayReticle;} WeaponDef;
typedef struct {struct {float value,vector[4];int enabled;} current;} dvar_t;
static cg_t state,*cg=&state;static cgs_t media,*cgs=&media;
static centity_t entities[1024],*cg_entities=entities;static snapshot_t snap,nextSnap;
static WeaponDef defs[7];static int nullWeapon,hidden;
static int BG_GetNumWeapons(void){return 6;}
static WeaponDef *BG_GetWeaponDef(int index){assert(index>=1&&index<=6);return index==nullWeapon?NULL:&defs[index];}
static int CG_AreHudMenusHidden(void){return hidden;}
static float vectoyaw(const vec_t *v){return atan2f(v[1],v[0])*57.295779513f;}
static dvar_t range,height,scope,offset,iconW,iconH,pointerW,pointerH,pivot,freq,pulseMax,pulseMin;
static dvar_t *cg_hudGrenadeIconMaxRange=&range,*cg_hudGrenadeIconMaxHeight=&height,
 *cg_hudGrenadeIconInScope=&scope,*cg_hudGrenadeIconOffset=&offset,
 *cg_hudGrenadeIconWidth=&iconW,*cg_hudGrenadeIconHeight=&iconH,
 *cg_hudGrenadePointerWidth=&pointerW,*cg_hudGrenadePointerHeight=&pointerH,
 *cg_hudGrenadePointerPivot=&pivot,*cg_hudGrenadePointerPulseFreq=&freq,
 *cg_hudGrenadePointerPulseMax=&pulseMax,*cg_hudGrenadePointerPulseMin=&pulseMin;
typedef struct {float x,y,w,h,angle;vec4_t color;} pic_t;
static pic_t icons[8],arrows[8];static int iconCount,arrowCount;
static void UI_DrawHandlePic(float x,float y,float w,float h,int ha,int va,const float *color,int material){
 assert(material==1&&ha==7&&va==7&&iconCount<8);pic_t *p=&icons[iconCount++];
 *p=(pic_t){x,y,w,h,0};memcpy(p->color,color,sizeof(p->color));
}
static void CG_DrawRotatedPic(float x,float y,float w,float h,int ha,int va,float angle,const float *color,int material){
 assert(material==2&&ha==7&&va==7&&arrowCount<8);pic_t *p=&arrows[arrowCount++];
 *p=(pic_t){x,y,w,h,angle};memcpy(p->color,color,sizeof(p->color));
 assert(color[3]>=0&&color[3]<=1&&isfinite(color[3]));
}
/* Execute the server's actual splash code with a visible target, so HUD and
 * damage sphere boundaries are compared rather than duplicating a formula. */
typedef struct {int takedamage;void *client;struct {int bmodel;vec3_t currentOrigin,absmin,absmax;} r;} gentity_t;
typedef struct {float fraction;int startsolid;} trace_t;
static gentity_t g_entities[2];static struct {int bPlayerIgnoreRadiusDamage;} level;
static int g_phys_world,damageCalls;
static int CM_AreaEntities(const vec_t *mins,const vec_t *maxs,int *list,int size,int type){assert(size==1024);list[0]=0;return 1;}
static float CanDamage(gentity_t *target,const vec_t *origin){return 1;}
static int LogAccuracyHit(gentity_t *target,gentity_t *attacker){return 1;}
static void G_TraceCapsule(trace_t *trace,const vec_t *a,const vec_t *b,const vec_t *c,const vec_t *d,int skip,int mask){assert(0);}
static void G_Damage(gentity_t *target,gentity_t *inflictor,gentity_t *attacker,const vec_t *dir,const vec_t *origin,int points,int flags,int mod,int loc,int time){assert(points>0);damageCalls++;}
'''

checks = 'static int retail[][3]={' + ','.join(rows) + '};\n' + r'''
static void near(float a,float b){assert(fabsf(a-b)<.002f);}
static void reset(void){
 memset(cg,0,sizeof(*cg));memset(&snap,0,sizeof(snap));memset(&nextSnap,0,sizeof(nextSnap));
 memset(entities,0,sizeof(entities));memset(defs,0,sizeof(defs));memset(g_entities,0,sizeof(g_entities));
 cg_entities=entities;cg->snap=&snap;cg->nextSnap=&nextSnap;
 cg->predictedPlayerState=(playerState_t){.weapon=6,.stats={100}};
 media.media.grenadeIcon=1;media.media.grenadePointer=2;hidden=nullWeapon=0;
 for(int i=1;i<=3;i++)defs[i]=(WeaponDef){WEAPTYPE_GRENADE,1,retail[i-1][0],0};
 defs[4]=(WeaponDef){WEAPTYPE_GRENADE,2,256,0};defs[5]=(WeaponDef){0,1,256,0};defs[6]=defs[1];defs[6].overlayReticle=1;
 range.current.value=height.current.value=256;scope.current.enabled=1;offset.current.value=50;
 iconW.current.value=iconH.current.value=pointerW.current.value=25;pointerH.current.value=12;
 pivot.current.vector[0]=12;pivot.current.vector[1]=27;freq.current.value=1.7f;
 pulseMin.current.value=.3f;pulseMax.current.value=1.85f;iconCount=arrowCount=0;damageCalls=0;
 g_entities[0].takedamage=1;g_entities[0].client=&g_entities[0];
}
static centity_t *grenade(int index,int weapon,float x,float y,float z){
 centity_t *cent=&entities[index];
 *cent=(centity_t){.nextState={.number=index,.eType=4,.weapon=weapon},.nextValid=1,.lerpOrigin={x,y,z}};
 nextSnap.entities[nextSnap.numEntities++]=cent->nextState;return cent;
}
static void drawCount(int expected){iconCount=arrowCount=0;CG_DrawGrenadeIndicators();assert(iconCount==expected&&arrowCount==expected);}
int main(void){
 int cases=0;
 for(int weapon=1;weapon<=3;weapon++)for(int heading=0;heading<360;heading+=15)
 for(int turn=0;turn<360;turn+=45)for(int d=0;d<3;d++){
  reset();float r=heading*.01745329252f,dist=(float[]){32,180,255}[d];
  grenade(64,weapon,cosf(r)*dist,sinf(r)*dist,0);cg->refdefViewAngles[1]=turn;drawCount(1);
  float angle=(heading-turn)*.01745329252f;
  near(icons[0].x+12.5f,-sinf(angle)*50);near(icons[0].y+12.5f,-cosf(angle)*50);
  /* The triangle points away from the crosshair, toward the threat. */
  near(sinf(arrows[0].angle*.01745329252f),-sinf(angle));
  near(cosf(arrows[0].angle*.01745329252f),cosf(angle));
  near(arrows[0].x+12.5f,-sinf(angle)*71+cosf(angle)*.5f);
  near(arrows[0].y+6,-cosf(angle)*71-sinf(angle)*.5f);
  cases++;
 }
 /* Warning and real damage agree across each weapon's 3D blast boundary. */
 for(int weapon=1;weapon<=3;weapon++)for(int axis=0;axis<3;axis++)for(int sign=-1;sign<=1;sign+=2)
 for(int d=0;d<6;d++){
  reset();centity_t *cent=grenade(64,weapon,0,0,0);
  float dist=(float[]){0,104.1f,250.1f,255.9f,256,300}[d];cent->lerpOrigin[axis]=sign*dist;
  CG_DrawGrenadeIndicators();
  G_RadiusDamage(cent->lerpOrigin,&g_entities[1],&g_entities[1],retail[weapon-1][1],retail[weapon-1][2],retail[weapon-1][0],NULL,4);
  assert(iconCount==damageCalls&&arrowCount==damageCalls);cases++;
 }
 reset();centity_t *cent=grenade(1021,6,200,200,0);drawCount(0);
 cent->lerpOrigin[0]=cent->lerpOrigin[1]=120;drawCount(1);cent->lerpOrigin[2]=220;drawCount(0);
 /* Lifecycle: explosion, deletion, pickup and invalid/future state clear immediately. */
 for(int reason=0;reason<12;reason++){
  reset();cent=grenade(64,1,30,0,0);drawCount(1);
  if(reason==0)cent->nextState.eType=0;
  else if(reason==1)cent->nextState.eFlags=0x20;
  else if(reason==2)cent->nextValid=0;
  else if(reason==3)nextSnap.numEntities=0;
  else if(reason==4)cent->nextState.weapon=0;
  else if(reason==5)cent->nextState.weapon=7;
  else if(reason==6)cent->nextState.weapon=4;
  else if(reason==7)cent->nextState.weapon=5;
  else if(reason==8)nullWeapon=1;
  else if(reason==9)cent->nextState.time=1;
  else if(reason==10)defs[1].iExplosionRadius=0;
  else defs[1].iExplosionRadius=20;
  drawCount(0);
 }
 for(int gate=0;gate<9;gate++){
  reset();grenade(64,1,30,0,0);
  if(gate==0)cg->predictedPlayerState.pm_type=4;
  else if(gate==1)cg->predictedPlayerState.pm_type=6;
  else if(gate==2)cg->predictedPlayerState.stats[0]=0;
  else if(gate==3)hidden=1;
  else if(gate==4)cg->renderingThirdPerson=1;
  else if(gate==5)cg->snap=cg->nextSnap=NULL;
  else if(gate==6)media.media.grenadeIcon=0;
  else if(gate==7)media.media.grenadePointer=0;
  else cg_entities=NULL;
  drawCount(0);
 }
 reset();grenade(64,1,200,0,0);cg->predictedPlayerState.fWeaponPosFrac=1;drawCount(1);
 scope.current.enabled=0;drawCount(0);cg->predictedPlayerState.fWeaponPosFrac=.9f;drawCount(1);
 cg->predictedPlayerState.fWeaponPosFrac=1;cg->predictedPlayerState.weapon=5;drawCount(1);
 range.current.value=150;drawCount(0);range.current.value=0;drawCount(0);
 reset();cent=grenade(64,1,0,0,150);height.current.value=100;drawCount(0);
 /* Fixed draw budget selects the closest warnings in a crowded snapshot. */
 reset();for(int i=0;i<9;i++)grenade(64+i,1,(float[]){90,60,20,80,40,10,70,30,50}[i],0,0);
 drawCount(4);for(int i=0;i<4;i++)near(icons[i].x,-12.5f);
 /* Inspect selection using different quadrants at known distances. */
 reset();grenade(64,1,80,0,0);grenade(65,1,0,60,0);grenade(66,1,-40,0,0);grenade(67,1,0,-20,0);grenade(68,1,200,0,0);
 drawCount(4);near(icons[0].x+12.5f,50);near(icons[1].y+12.5f,50);
 near(icons[2].x+12.5f,-50);near(icons[3].y+12.5f,-50);
 /* Snapshot limits and identifiers remain safe even on malformed input. */
 reset();grenade(1021,6,30,0,0);nextSnap.numEntities=9999;
 for(int i=1;i<256;i++)nextSnap.entities[i].number=(int[]){-1,1022,1023,9000}[i%4];drawCount(1);
 nextSnap.numEntities=-1;drawCount(0);nextSnap.numEntities=1;
 snap=nextSnap;cg->nextSnap=NULL;drawCount(1);
 for(int t=0;t<10000;t+=7){cg->time=t;drawCount(1);assert(arrows[0].color[3]>=.299f);}
 puts("PASS: grenade lifecycle, smoke/rocket exclusion, live HUD/scoped gates, 3D splash boundaries, rotating directions, pulse, closest-four budget and malformed snapshots");
 printf("PASS: %d retail direction/radius cases against production drawing and native splash code\n",cases);
}
'''

variants = [
    ('production', distance, draw),
    ('horizontal-only range', distance.replace(' + dz * dz', ''), draw),
    ('persistent explosion warning', distance.replace('(state->eFlags & 0x20)', '0'), draw),
    ('smoke warning', distance.replace('weapon->offhandClass != OFFHAND_CLASS_FRAG_GRENADE', '0'), draw),
    ('reversed pointer', distance, draw.replace('7, 7, -angle,', '7, 7, angle,')),
    ('unbounded warnings', distance, draw.replace('MAX_WARNINGS = 4', 'MAX_WARNINGS = 6')),
]
with tempfile.TemporaryDirectory(prefix='cod2-grenade-indicators-') as directory:
    path = Path(directory)
    for name, measure, paint in variants:
        (path / 'test.c').write_text(support + measure + paint + splash + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (name == 'production'), (name, result.stdout, result.stderr)
        print(result.stdout.strip() if name == 'production' else f'PASS: {name} regression rejected')

assert function(draw_source, 'CG_Draw2D').count('CG_DrawGrenadeIndicators();') == 1
assert 'G_Trace' not in distance + draw and 'CG_Trace' not in distance + draw
for name in ('cg_hudGrenadeIconMaxRange', 'cg_hudGrenadeIconMaxHeight'):
    assert f'Dvar_RegisterFloat("{name}", 256.0f,' in main_source
assert 'Dvar_RegisterBool_mac("cg_hudGrenadeIconInScope", 1,' in main_source
print('PASS: HUD dispatch, full-radius/scoped defaults and original private grenade materials')
