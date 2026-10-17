#!/usr/bin/env python3
"""Exercise the production four-part crosshair, its opacity, sprint spread and HUD gates."""
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


draw_source = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
constants = dict(re.findall(r'^#define (CG_CROSSHAIR_\w+) ([0-9.]+)f$', draw_source, re.M))
assert set(constants) == {'CG_CROSSHAIR_SPRINT_SPREAD', 'CG_CROSSHAIR_SPREAD_ALPHA_MIN'}, constants
assert float(constants['CG_CROSSHAIR_SPRINT_SPREAD']) > 1
assert 0 < float(constants['CG_CROSSHAIR_SPREAD_ALPHA_MIN']) < 1

support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
typedef int qboolean; typedef float vec_t; typedef float vec4_t[4];
typedef int MaterialHandle;
enum {WEAPTYPE_GRENADE=1,OFFHAND_CLASS_FRAG_GRENADE=1};
typedef struct {int weapon,eFlags,viewlocked_entNum,pm_flags,weaponstate,grenadeTimeLeft;
    float fWeaponPosFrac,aimSpreadScale;} playerState_t;
typedef struct {char *unused;float fAdsCrosshairInFrac,fAdsCrosshairOutFrac;
    int iReticleCenterSize,iReticleSideSize,iReticleMinOfs,weapType,bCookOffHold,offhandClass;
    float fHipReticleSidePos;} WeaponDef;
#include "cod2_grenade.h"
#include "cod2_sprint.h"
''' + ''.join(f'#define {name} {value}f\n' for name, value in constants.items()) + r'''
typedef struct {MaterialHandle hReticleCenter,hReticleSide;} weaponInfo_t;
typedef struct {struct {int enabled,integer;float value;} current;} dvar_t;
static struct {playerState_t predictedPlayerState;int renderingThirdPerson,time;
    struct {int bPositionToADS;} playerEntity;struct {float fov_y;} refdef;} cgStorage,*cg=&cgStorage;
static struct {int viewWidth,viewHeight;} cgsStorage,*cgs=&cgsStorage;
static dvar_t enabled={.current={.enabled=1,.value=1}},disabled,paused,highMin={.current={.value=.8f}};
static dvar_t *cg_crosshairAlpha=&enabled,*cg_crosshairAlphaMin=&disabled;
static dvar_t *cg_drawGun=&enabled,*cg_drawCrosshair=&enabled,*cg_paused=&paused;
static dvar_t *cg_crosshairDynamic=&disabled;
static WeaponDef weapon={.fAdsCrosshairInFrac=.5f,.fAdsCrosshairOutFrac=.5f,
    .iReticleSideSize=8,.iReticleMinOfs=10};
static weaponInfo_t weapons[2]={{0},{.hReticleSide=1}};
static weaponInfo_t *weaponArray=weapons;static void *imp_cg_weapons=&weaponArray;
static float sprintViewBlend;static int sprintViewTime;
static int hudHidden,overlayCalls,draws,centerDraws;
static float visibility=1,centerAlpha,centerRgb[3];
/* The teammate labels' color, while labelsDrawn says the WWII labels are drawn. */
static const float labelColor[3]={.386f,.552f,.495f};
static int labelsDrawn=1;
static int CG_GetTeammateNameColor(float *color) {
    if(!labelsDrawn)return 0;
    for(int i=0;i<3;i++)color[i]=labelColor[i];
    return 1;
}
static struct {float x,y,w,h,angle,alpha,rgb[3];} pics[4];
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
    for(int i=0;i<3;i++)centerRgb[i]=color[i];
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
    pics[draws].angle=angle;pics[draws].alpha=color[3];
    for(int i=0;i<3;i++)pics[draws].rgb[i]=color[i];
    draws++;
}
'''
body = ''.join([
    function('src/PC/cgame_mp/cg_hudelem_mp.c', 'CG_AlignHudElemX'),
    function('src/PC/cgame_mp/cg_hudelem_mp.c', 'CG_AlignHudElemY'),
    function('src/PC/cgame_mp/cg_weapons.c', 'CG_ResetSprintView'),
    function('src/PC/cgame_mp/cg_weapons.c', 'CG_SprintViewBlend'),
    function('src/PC/cgame_mp/cg_weapons.c', 'CG_SprintCarryBlend'),
    function('src/PC/cgame_mp/cg_draw_mp.c', 'CG_DrawCrosshair'),
])
checks = r'''
static void reset(void) {draws=centerDraws=overlayCalls=0;}
/* The right mark starts at the side offset and the bottom mark at the same distance. */
static float gap(void) {assert(draws==4&&pics[1].x==pics[2].y);return pics[1].x;}
static float frame(int t) {cg->time=t;reset();CG_DrawCrosshair();return gap();}
static float expected(float degrees) {
    return fmaxf(10,(float)(tan(degrees*0.017453292519943295)*240/tan(30*0.017453292519943295)));
}
static void visible(float alpha) {
    for(int i=0;i<4;i++) {
        assert(fabsf(pics[i].alpha-alpha)<1e-6f);
        assert(pics[i].angle==i*90&&pics[i].w==8&&pics[i].h==8);
        assert(isfinite(pics[i].x)&&isfinite(pics[i].y));
    }
    assert(pics[0].y+8<0&&pics[1].x>0&&pics[2].y>0&&pics[3].x+8<0);
}
static void tint(float r,float g,float b) {
    for(int i=0;i<4;i++)assert(pics[i].rgb[0]==r&&pics[i].rgb[1]==g&&pics[i].rgb[2]==b);
}
static void center(float r,float g,float b) {
    assert(centerDraws==1&&centerRgb[0]==r&&centerRgb[1]==g&&centerRgb[2]==b);
}
int main(void) {
    playerState_t *ps=&cg->predictedPlayerState;
    ps->weapon=1;cg->refdef.fov_y=60;cg->time=1000;
    cgs->viewWidth=1280;cgs->viewHeight=720;
    /* At the hip, all four marks surround screen center at every spread level;
     * movement spread fades them, but only to the minimum fraction. */
    for(int spread=0;spread<=255;spread+=15) {
        ps->aimSpreadScale=spread;frame(1000);
        assert(overlayCalls==1);
        visible(fmaxf(1-spread/255.f,CG_CROSSHAIR_SPREAD_ALPHA_MIN));
        assert(fabsf(gap()-expected(1+4*spread/255.f))<1e-3f);
    }
    /* A higher configured minimum still applies. */
    cg_crosshairAlphaMin=&highMin;frame(1000);visible(.8f);cg_crosshairAlphaMin=&disabled;
    /* Sprint keeps the marks visible and opens them past the hip spread while
     * the weapon lowers; releasing it closes them again. Every frame interval
     * gives a gradual, monotonic transition of the carry pose's duration. */
    const float walk=expected(5),sprint=expected(5*CG_CROSSHAIR_SPRINT_SPREAD);
    assert(sprint>walk+5);
    for(int step=1;step<=66;step++) {
        ps->aimSpreadScale=255;ps->pm_flags=0;ps->weaponstate=0;
        CG_ResetSprintView();int t=1000;
        assert(fabsf(frame(t)-walk)<1e-3f);
        ps->pm_flags=PMF_SPRINT;
        float last=walk;int between=0,opened=-1;
        for(int start=t;t<start+400;) {
            t+=step;float g=frame(t);visible(CG_CROSSHAIR_SPREAD_ALPHA_MIN);
            assert(g>=last-1e-4f&&g<=sprint+1e-3f);
            between|=g>walk+1e-3f&&g<sprint-1e-3f;
            if(opened<0&&fabsf(g-sprint)<1e-3f)opened=t-start;
            last=g;
        }
        assert(between&&opened>=0&&opened<=180+2*step);
        ps->pm_flags=0;between=0;int closed=-1;
        for(int start=t;t<start+300;) {
            t+=step;float g=frame(t);visible(CG_CROSSHAIR_SPREAD_ALPHA_MIN);
            assert(g<=last+1e-4f&&g>=walk-1e-3f);
            between|=g>walk+1e-3f&&g<sprint-1e-3f;
            if(closed<0&&fabsf(g-walk)<1e-3f)closed=t-start;
            last=g;
        }
        assert(between&&closed>=0&&closed<=100+2*step);
    }
    /* Firing ends sprint and shows the actual spread on that frame. */
    ps->pm_flags=PMF_SPRINT;CG_ResetSprintView();frame(1000);
    assert(fabsf(frame(1200)-sprint)<1e-3f);
    ps->pm_flags=0;ps->weaponstate=3;
    assert(fabsf(frame(1216)-walk)<1e-3f);
    ps->weaponstate=0;
    /* Standing still afterwards restores the normal size and full alpha. */
    ps->aimSpreadScale=0;assert(fabsf(frame(1300)-expected(1))<1e-3f);visible(1);tint(1,1,1);
    /* An enemy under the crosshair (0x200000) leaves the marks and the center
     * white; a teammate (0x100000) turns them the teammate labels' color, or
     * the stock green while the stock labels are drawn. */
    weapons[1].hReticleCenter=2;
    ps->pm_flags=0x200000;frame(1300);visible(1);tint(1,1,1);center(1,1,1);
    ps->pm_flags=0x100000;frame(1300);visible(1);
    tint(labelColor[0],labelColor[1],labelColor[2]);center(labelColor[0],labelColor[1],labelColor[2]);
    labelsDrawn=0;frame(1300);visible(1);tint(.25f,1,.25f);center(.25f,1,.25f);labelsDrawn=1;
    ps->pm_flags=0;frame(1300);tint(1,1,1);center(1,1,1);
    weapons[1].hReticleCenter=0;
    /* Both the side and center fade with the weapon-overlay transition. */
    ps->fWeaponPosFrac=.5f;visibility=.4f;
    weapons[1].hReticleCenter=2;reset();CG_DrawCrosshair();
    assert(draws==4&&centerDraws==1&&overlayCalls==1);
    assert(fabsf(centerAlpha-.4f)<1e-6f);
    for(int i=0;i<4;i++)assert(fabsf(pics[i].alpha-.4f)<1e-6f);
    visibility=0;reset();CG_DrawCrosshair();assert(!draws&&!centerDraws);
    visibility=1;ps->fWeaponPosFrac=1;
    reset();CG_DrawCrosshair();assert(!draws&&!centerDraws);
    ps->fWeaponPosFrac=0;
    for(int gate=0;gate<5;gate++) {
        ps->pm_flags=PMF_SPRINT;
        hudHidden=(gate==0);cg->renderingThirdPerson=(gate==1);
        cg_paused->current.integer=(gate==2);cg_drawCrosshair=(gate==3?&disabled:&enabled);
        ps->weaponstate=(gate==4?5:0);
        reset();CG_DrawCrosshair();assert(!draws&&!centerDraws);
    }
    return 0;
}
'''
blend_call = 'spread *= 1.0f + (CG_CROSSHAIR_SPRINT_SPREAD - 1.0f) * CG_SprintCarryBlend(ps);'
alpha_floor = ('fmaxf(cg_crosshairAlphaMin->current.value,\n'
               '                               CG_CROSSHAIR_SPREAD_ALPHA_MIN * cg_crosshairAlpha->current.value)')
white = ('    } else {\n'
         '        color[0] = 1.0f;\n'
         '        color[1] = 1.0f;\n'
         '        color[2] = 1.0f;\n'
         '    }\n')
assert body.count(white) == 1
teammate_color = '        if (!CG_GetTeammateNameColor(color)) {\n'
assert body.count(teammate_color) == 1
mutants = [
    body.replace(teammate_color, '        if (1) {\n'),
    body.replace(teammate_color, '        CG_GetTeammateNameColor(color);\n        {\n'),
    body.replace('if (pmFlags & 0x100000) {', 'if (pmFlags & 0x300000) {'),
    body.replace(white, '    } else if (pmFlags & 0x200000) {\n'
                        '        color[0] = 1.0f;\n'
                        '        color[1] = 0.25f;\n'
                        '        color[2] = 0.25f;\n' + white),
    body.replace('if (pmFlags & 0x100000) {', 'if (0) {'),
    body.replace('color[3] = reticleVisibility *', 'color[3] = posLerp *'),
    body.replace('(alpha >= alphaMin ? alpha : alphaMin) * reticleVisibility',
                 '(alpha >= alphaMin ? alpha : alphaMin)'),
    body.replace('if (cg->renderingThirdPerson)\n',
                 'if (cg->renderingThirdPerson || (ps->pm_flags & PMF_SPRINT))\n'),
    body.replace(blend_call, ''),
    body.replace(blend_call, blend_call.replace('CG_SprintCarryBlend(ps)',
                                                '((ps->pm_flags & PMF_SPRINT) ? 1.0f : 0.0f)')),
    body.replace(alpha_floor, 'cg_crosshairAlphaMin->current.value'),
    body.replace(alpha_floor, 'CG_CROSSHAIR_SPREAD_ALPHA_MIN * cg_crosshairAlpha->current.value'),
]
assert all(mutant != body for mutant in mutants)
variants = [(body, []), (body, ['-D__EMSCRIPTEN__'])] + [(mutant, []) for mutant in mutants]
with tempfile.TemporaryDirectory(prefix='cod2-crosshair-') as directory:
    path = Path(directory) / 'crosshair.c'
    binary = Path(directory) / 'crosshair'
    for index, (variant, flags) in enumerate(variants):
        path.write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', *flags,
                        '-I', str(root / 'src/headers'), str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index < 2), (index, result.stderr)
print('PASS: four visible hip-fire marks at 18 spread levels with a 0.6 alpha floor, sprint opening/closing '
      'over 66 frame intervals (linear and eased), firing out of sprint, white over enemies, the teammate '
      'labels\' color over teammates (green with the stock labels), ADS fade, five HUD gates and '
      f'{len(mutants)} failing regression mutants')
