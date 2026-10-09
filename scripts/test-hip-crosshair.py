#!/usr/bin/env python3
"""Exercise the production four-part crosshair, its opacity and HUD gates."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent


def function(path, name):
    source = (root / path).read_text()
    match = re.search(r'^([^\n]*\b' + name + r'\([^;]*?\))\s*\{', source, re.M)
    assert match, name
    brace = source.index('{', match.start())
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
typedef int qboolean; typedef float vec_t; typedef float vec4_t[4];
typedef int MaterialHandle;
enum {WEAPTYPE_GRENADE=1};
typedef struct {int weapon,eFlags,viewlocked_entNum,pm_flags,weaponstate,grenadeTimeLeft;
    float fWeaponPosFrac,aimSpreadScale;} playerState_t;
typedef struct {char *unused;float fAdsCrosshairInFrac,fAdsCrosshairOutFrac;
    int iReticleCenterSize,iReticleSideSize,iReticleMinOfs,weapType,bCookOffHold;
    float fHipReticleSidePos;} WeaponDef;
typedef struct {MaterialHandle hReticleCenter,hReticleSide;} weaponInfo_t;
typedef struct {struct {int enabled,integer;float value;} current;} dvar_t;
static struct {playerState_t predictedPlayerState;int renderingThirdPerson;
    struct {int bPositionToADS;} playerEntity;struct {float fov_y;} refdef;} cgStorage,*cg=&cgStorage;
static struct {int viewWidth,viewHeight;} cgsStorage,*cgs=&cgsStorage;
static dvar_t enabled={.current={.enabled=1,.value=1}},disabled,paused;
static dvar_t *cg_crosshairAlpha=&enabled,*cg_crosshairAlphaMin=&disabled;
static dvar_t *cg_drawGun=&enabled,*cg_drawCrosshair=&enabled,*cg_paused=&paused;
static dvar_t *cg_crosshairDynamic=&disabled,*cg_crosshairEnemyColor=&enabled;
static WeaponDef weapon={.fAdsCrosshairInFrac=.5f,.fAdsCrosshairOutFrac=.5f,
    .iReticleSideSize=8,.iReticleMinOfs=10};
static weaponInfo_t weapons[2]={{0},{.hReticleSide=1}};
static weaponInfo_t *weaponArray=weapons;static void *imp_cg_weapons=&weaponArray;
static int hudHidden,overlayCalls,draws,centerDraws;
static float visibility=1,centerAlpha;
static struct {float x,y,w,h,angle,alpha;} pics[4];
static int CG_AreHudMenusHidden(void) {return hudHidden;}
static void CG_DrawTurretCrossHair(void) {}
static int BG_GetViewmodelWeaponIndex(playerState_t *ps) {return ps->weapon;}
static void *BG_GetWeaponDef(int index) {return &weapon;}
static float CG_DrawWeapReticle(void) {overlayCalls++;return visibility;}
static void CG_CalcCrosshairPosition(float *x,float *y) {*x=3;*y=-2;}
static int CL_IsRenderingSplitScreen(void) {return 0;}
static void CL_DrawStretchPic(float x,float y,float w,float h,int ax,int ay,
    float s0,float t0,float s1,float t1,const vec_t *color,MaterialHandle material) {
    centerDraws++;centerAlpha=color[3];
}
static void BG_GetSpreadForWeapon(playerState_t *ps,int index,float *lo,float *hi) {*lo=1;*hi=5;}
static float GetRealWidthFromVirtualWidth(float x) {return x;}
static float GetRealHeightFromVirtualHeight(float x) {return x;}
static float GetVirtualWidthFromRealWidth(float x) {return x;}
static float GetVirtualHeightFromRealHeight(float x) {return x;}
static void CG_DrawRotatedPic(float x,float y,float w,float h,int ax,int ay,
    float angle,const vec_t *color,MaterialHandle material) {
    assert(draws<4);assert(ax==2&&ay==2&&material==1);
    pics[draws].x=x;pics[draws].y=y;pics[draws].w=w;pics[draws].h=h;
    pics[draws].angle=angle;pics[draws].alpha=color[3];draws++;
}
'''
body = ''.join([
    function('src/PC/cgame_mp/cg_hudelem_mp.c', 'CG_AlignHudElemX'),
    function('src/PC/cgame_mp/cg_hudelem_mp.c', 'CG_AlignHudElemY'),
    function('src/PC/cgame_mp/cg_draw_mp.c', 'CG_DrawCrosshair'),
])
checks = r'''
static void reset(void) {draws=centerDraws=overlayCalls=0;}
int main(void) {
    cg->predictedPlayerState.weapon=1;cg->refdef.fov_y=60;
    cgs->viewWidth=1280;cgs->viewHeight=720;
    /* At the hip, all four marks have nonzero alpha and surround screen center. */
    for(int spread=0;spread<255;spread+=17) {
        cg->predictedPlayerState.aimSpreadScale=spread;reset();CG_DrawCrosshair();
        assert(draws==4&&overlayCalls==1);
        for(int i=0;i<4;i++) {
            assert(pics[i].alpha>0&&pics[i].alpha<=1);
            assert(pics[i].angle==i*90&&pics[i].w==8&&pics[i].h==8);
            assert(isfinite(pics[i].x)&&isfinite(pics[i].y));
        }
        assert(pics[0].y+8<0&&pics[1].x>0&&pics[2].y>0&&pics[3].x+8<0);
    }
    /* Both the side and center fade with the weapon-overlay transition. */
    cg->predictedPlayerState.aimSpreadScale=0;
    cg->predictedPlayerState.fWeaponPosFrac=.5f;visibility=.4f;
    weapons[1].hReticleCenter=2;reset();CG_DrawCrosshair();
    assert(draws==4&&centerDraws==1&&overlayCalls==1);
    assert(fabsf(centerAlpha-.4f)<1e-6f);
    for(int i=0;i<4;i++)assert(fabsf(pics[i].alpha-.4f)<1e-6f);
    visibility=0;reset();CG_DrawCrosshair();assert(!draws&&!centerDraws);
    visibility=1;cg->predictedPlayerState.fWeaponPosFrac=1;
    reset();CG_DrawCrosshair();assert(!draws&&!centerDraws);
    cg->predictedPlayerState.fWeaponPosFrac=0;
    for(int gate=0;gate<5;gate++) {
        hudHidden=(gate==0);cg->renderingThirdPerson=(gate==1);
        cg_paused->current.integer=(gate==2);cg_drawCrosshair=(gate==3?&disabled:&enabled);
        cg->predictedPlayerState.weaponstate=(gate==4?5:0);
        reset();CG_DrawCrosshair();assert(!draws&&!centerDraws);
    }
    return 0;
}
'''
mutants = [
    body.replace('color[3] = reticleVisibility *', 'color[3] = posLerp *'),
    body.replace('(alpha >= alphaMin ? alpha : alphaMin) * reticleVisibility',
                 '(alpha >= alphaMin ? alpha : alphaMin)'),
]
assert all(mutant != body for mutant in mutants)
with tempfile.TemporaryDirectory(prefix='cod2-crosshair-') as directory:
    path = Path(directory) / 'crosshair.c'
    binary = Path(directory) / 'crosshair'
    for index, variant in enumerate([body] + mutants):
        path.write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr)
print('PASS: four visible hip-fire marks at 15 spread levels, ADS fade, five HUD gates and two failing regression mutants')
