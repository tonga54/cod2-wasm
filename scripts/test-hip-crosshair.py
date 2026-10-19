#!/usr/bin/env python3
"""Exercise the production four-part crosshair, its pixels, opacity, sprint spread, side images and HUD gates."""
from pathlib import Path
import re
import subprocess
import tempfile

from hud_raster import RASTER_C, hud_vertex_definition

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
hud_source = (root / 'src/PC/cgame_mp/cg_hudelem_mp.c').read_text()
constants = dict(re.findall(r'^#define (CG_CROSSHAIR_\w+) ([0-9.]+)f$', draw_source, re.M))
assert set(constants) == {'CG_CROSSHAIR_SPRINT_SPREAD', 'CG_CROSSHAIR_SPREAD_ALPHA_MIN', 'CG_CROSSHAIR_LINE_LENGTH',
                          'CG_CROSSHAIR_LINE_WIDTH', 'CG_CROSSHAIR_LINE_ALPHA'}, constants
assert float(constants['CG_CROSSHAIR_SPRINT_SPREAD']) > 1
assert 0 < float(constants['CG_CROSSHAIR_SPREAD_ALPHA_MIN']) < 1
# The WWII marks: under a unit wide, a few units long, translucent.
assert 0 < float(constants['CG_CROSSHAIR_LINE_WIDTH']) < 1
assert 3 < float(constants['CG_CROSSHAIR_LINE_LENGTH']) < 6
assert 0 < float(constants['CG_CROSSHAIR_LINE_ALPHA']) < 1
lines_max = re.search(r'^#define CG_HUD_LINES_MAX \d+$', hud_source, re.M)
assert lines_max

support = r'''
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
typedef int qboolean; typedef float vec_t; typedef float vec4_t[4];
typedef int MaterialHandle;
enum {WEAPTYPE_GRENADE=1,OFFHAND_CLASS_FRAG_GRENADE=1};
typedef struct {int weapon,eFlags,viewlocked_entNum,pm_flags,weaponstate,grenadeTimeLeft;
    float fWeaponPosFrac,aimSpreadScale;} playerState_t;
typedef struct {char *unused;float fAdsCrosshairInFrac,fAdsCrosshairOutFrac;
    int iReticleCenterSize,iReticleSideSize,iReticleMinOfs,weapType,bCookOffHold,offhandClass;
    float fHipReticleSidePos;const char *szReticleSide;} WeaponDef;
#include "cod2_grenade.h"
#include "cod2_sprint.h"
''' + ''.join(f'#define {name} {value}f\n' for name, value in constants.items()) + lines_max.group(0) + '\n' + \
    hud_vertex_definition(root) + r'''
typedef struct {MaterialHandle hReticleCenter,hReticleSide;} weaponInfo_t;
typedef struct {struct {int enabled,integer;float value;} current;} dvar_t;
static struct {playerState_t predictedPlayerState;int renderingThirdPerson,time;
    struct {int bPositionToADS;} playerEntity;struct {float fov_y;} refdef;} cgStorage,*cg=&cgStorage;
static struct {struct {MaterialHandle whiteMaterial;} media;int viewWidth,viewHeight;}
    cgsStorage={.media={.whiteMaterial=7},.viewWidth=1280,.viewHeight=720},*cgs=&cgsStorage;
static dvar_t enabled={.current={.enabled=1,.value=1}},disabled,paused,highMin={.current={.value=.8f}};
static dvar_t *cg_crosshairAlpha=&enabled,*cg_crosshairAlphaMin=&disabled;
static dvar_t *cg_drawGun=&enabled,*cg_drawCrosshair=&enabled,*cg_paused=&paused;
static dvar_t *cg_crosshairDynamic=&disabled;
static WeaponDef weapon={.fAdsCrosshairInFrac=.5f,.fAdsCrosshairOutFrac=.5f,
    .iReticleSideSize=8,.iReticleMinOfs=10,.szReticleSide="gfx/reticle/side_skinny_xenon.tga"};
static weaponInfo_t weapons[2]={{0},{.hReticleSide=1}};
static weaponInfo_t *weaponArray=weapons;static void *imp_cg_weapons=&weaponArray;
static float sprintViewBlend;static int sprintViewTime;
static int hudHidden,overlayCalls,centerDraws;
static float visibility=1,centerAlpha,centerRgb[3];
/* The teammate labels' color, while labelsDrawn says the WWII labels are drawn. */
static const float labelColor[3]={.386f,.552f,.495f};
static int labelsDrawn=1;
static int CG_GetTeammateNameColor(float *color) {
    if(!labelsDrawn)return 0;
    for(int i=0;i<3;i++)color[i]=labelColor[i];
    return 1;
}
static int CG_AreHudMenusHidden(void) {return hudHidden;}
static void CG_DrawTurretCrossHair(void) {}
static int BG_GetViewmodelWeaponIndex(playerState_t *ps) {return ps->weapon;}
static void *BG_GetWeaponDef(int index) {return &weapon;}
static void *BG_GetPlayerWeaponDef(playerState_t *ps,int index) {return &weapon;}
static float CG_DrawWeapReticle(void) {overlayCalls++;return visibility;}
static void CG_DrawAttachmentSight(const playerState_t *ps,int weapon,float x,float y) {}
/* Where cg_crosshairDynamic would put the crosshair, off screen center:
 * between pixels at 1080p. */
static float crosshairX=3.1f,crosshairY=-2.0f;
static void CG_CalcCrosshairPosition(float *x,float *y) {*x=crosshairX;*y=crosshairY;}
static int CL_IsRenderingSplitScreen(void) {return 0;}
static void CL_DrawStretchPic(float x,float y,float w,float h,int ax,int ay,
    float s0,float t0,float s1,float t1,const vec_t *color,MaterialHandle material) {
    centerDraws++;centerAlpha=color[3];
    for(int i=0;i<3;i++)centerRgb[i]=color[i];
}
static void BG_GetSpreadForWeapon(playerState_t *ps,int index,float *lo,float *hi) {*lo=1;*hi=5;}
static int I_strnicmp(const char *a,const char *b,size_t n) {
    for(;n;n--,a++,b++) {
        int x=tolower((unsigned char)*a),y=tolower((unsigned char)*b);
        if(x!=y)return x<y?-1:1;
        if(!x)return 0;
    }
    return 0;
}
/* The side images drawn: where, how big, turned how far, how opaque, which. */
typedef struct {float x,y,w,h,angle,alpha;MaterialHandle material;} Pic;
static Pic pics[8];static int picCount;
static void CG_DrawRotatedPic(float x,float y,float w,float h,int horzAlign,int vertAlign,float angle,
    const vec_t *color,MaterialHandle material) {
    assert(horzAlign==2&&vertAlign==2&&picCount<8);
    pics[picCount++]=(Pic){x,y,w,h,angle,color[3],material};
}
/* A canvas of screenWidth x screenHeight pixels, screenScale pixels a unit. */
static float screenScale=1.5f,screenWidth=1280,screenHeight=720;
static void CalcScreenPlacement(float *x,float *y,float *w,float *h,int horzAlign,int vertAlign) {
    assert(horzAlign==2&&vertAlign==2);
    *x=*x*screenScale+screenWidth*.5f;*y=*y*screenScale+screenHeight*.5f;*w*=screenScale;*h*=screenScale;
}
static float GetRealWidthFromVirtualWidth(float w) {return w*screenScale;}
static float GetRealHeightFromVirtualHeight(float h) {return h*screenScale;}
static float GetVirtualWidthFromRealWidth(float w) {return w/screenScale;}
static float GetVirtualHeightFromRealHeight(float h) {return h/screenScale;}
''' + RASTER_C
body = ''.join([
    function('src/PC/cgame_mp/cg_weapons.c', 'CG_ResetSprintView'),
    function('src/PC/cgame_mp/cg_weapons.c', 'CG_SprintViewBlend'),
    function('src/PC/cgame_mp/cg_weapons.c', 'CG_SprintCarryBlend'),
    function('src/PC/cgame_mp/cg_hudelem_mp.c', 'CG_DrawHudLines'),
    function('src/PC/cgame_mp/cg_hudelem_mp.c', 'CG_AlignHudElemX'),
    function('src/PC/cgame_mp/cg_hudelem_mp.c', 'CG_AlignHudElemY'),
    function('src/PC/cgame_mp/cg_draw_mp.c', 'CG_CrosshairSideIsBar'),
    function('src/PC/cgame_mp/cg_draw_mp.c', 'CG_DrawCrosshairSidePics'),
    function('src/PC/cgame_mp/cg_draw_mp.c', 'CG_DrawCrosshairLines'),
    function('src/PC/cgame_mp/cg_draw_mp.c', 'CG_DrawCrosshair'),
])
checks = r'''
/* One mark as drawn: whole pixels across, their middle, their opacity and
 * color, and in pixels the gap to the crosshair's middle and the length. */
typedef struct {int pixels;float middle,alpha,rgb[3],gap,length;} Mark;
static Mark marks[4];
static float aimX(void) {return screenWidth*.5f+(cg_crosshairDynamic->current.enabled?crosshairX*screenScale:0);}
static float aimY(void) {return screenHeight*.5f+(cg_crosshairDynamic->current.enabled?crosshairY*screenScale:0);}
static const float *pixel(int x,int y) {assert(x>=0&&y>=0&&x<RASTER_SIZE&&y<RASTER_SIZE);return raster[y][x];}
/* Measures the top, right, bottom and left marks on the canvas and returns
 * how many there are. Each is found walking out from the crosshair, then
 * crossed at its middle, then measured along from the middle of the others:
 * its gap from where its first pixel ramps up, less the sub-pixel shift its
 * way, its length from its total opacity over its own. */
static int measure(void) {
    static const int dirs[4][2]={{0,-1},{1,0},{0,1},{-1,0}};
    int cx=(int)floorf(aimX()-rasterLeft),cy=(int)floorf(aimY()-rasterTop),found=0;
    static float walk[4][RASTER_SIZE/2];
    for(int i=0;i<4;i++) {
        int dx=dirs[i][0],dy=dirs[i][1],first=-1,last=-1;
        for(int s=0;s<RASTER_SIZE/2-1;s++) {
            walk[i][s]=pixel(cx+dx*s,cy+dy*s)[3];
            if(walk[i][s]>1e-6f){if(first<0)first=s;last=s;}
        }
        marks[i].pixels=0;
        if(first<0)continue;
        found++;
        int x=cx+dx*(first+last)/2,y=cy+dy*(first+last)/2,lo=99,hi=-99;
        float a0=pixel(x,y)[3];
        for(int k=-12;k<=12;k++) {
            float a=pixel(x+(dy?k:0),y+(dx?k:0))[3];
            if(a>1e-6f){assert(fabsf(a-a0)<1e-5f);if(k<lo)lo=k;hi=k;}
        }
        for(int k=lo;k<=hi;k++)assert(pixel(x+(dy?k:0),y+(dx?k:0))[3]>1e-6f);
        marks[i].pixels=hi-lo+1;marks[i].alpha=a0;
        marks[i].middle=(dy?rasterLeft+x:rasterTop+y)+(lo+hi+1)*.5f;
        for(int k=0;k<3;k++)marks[i].rgb[k]=pixel(x,y)[k]/a0;
    }
    if(found<4)return found;
    assert(fabsf(marks[0].middle-marks[2].middle)<1e-4f&&fabsf(marks[1].middle-marks[3].middle)<1e-4f);
    float middleX=marks[0].middle,middleY=marks[1].middle;
    for(int i=0;i<4;i++) {
        int dx=dirs[i][0],dy=dirs[i][1],first=-1;float sum=0;
        for(int s=0;s<RASTER_SIZE/2-1;s++) {
            float d=dx*(rasterLeft+cx+dx*s+.5f-middleX)+dy*(rasterTop+cy+dy*s+.5f-middleY);
            assert(walk[i][s]<=marks[i].alpha+1e-5f);
            if(walk[i][s]<=1e-6f)continue;
            assert(d>0);
            if(first<0) {
                first=s;
                marks[i].gap=d+.5f-walk[i][s]/marks[i].alpha-(dx*rasterShift[0]+dy*rasterShift[1]);
            }
            sum+=walk[i][s];
        }
        marks[i].length=sum/marks[i].alpha;
    }
    return 4;
}
static void reset(void) {
    centerDraws=overlayCalls=picCount=0;
    rasterClear(floorf(aimX())-RASTER_SIZE/2,floorf(aimY())-RASTER_SIZE/2);
}
static float drawnLength=CG_CROSSHAIR_LINE_LENGTH;
/* All four marks are there, the same, a whole number of pixels wide and as
 * opaque as the crosshair alpha, ramped by the line alpha, makes a line
 * CG_CROSSHAIR_LINE_WIDTH units wide; one draw of the white material and no
 * side image. */
static void visible(float alpha) {
    assert(measure()==4&&rasterCalls==1&&rasterMaterial==7&&!picCount);
    float exact=CG_CROSSHAIR_LINE_WIDTH*screenScale,pixels=fmaxf(1,floorf(exact+.5f));
    for(int i=0;i<4;i++) {
        assert(marks[i].pixels==(int)pixels);
        assert(fabsf(marks[i].alpha*pixels-fminf(pixels,alpha*CG_CROSSHAIR_LINE_ALPHA*exact))<1e-4f);
        assert(fabsf(marks[i].length-drawnLength*screenScale)<2e-3f);
        assert(fabsf(marks[i].gap-marks[0].gap)<1e-3f);
    }
    /* Centered within half a pixel of the crosshair. */
    assert(fabsf(marks[0].middle-aimX())<=.5f&&fabsf(marks[1].middle-aimY())<=.5f);
}
static float gap(void) {return marks[0].gap/screenScale;}
static float frame(int t) {cg->time=t;reset();CG_DrawCrosshair();assert(measure()==4);return gap();}
static float expected(float degrees) {
    return fmaxf(10,(float)(tan(degrees*0.017453292519943295)*240/tan(30*0.017453292519943295)));
}
static void tint(float r,float g,float b) {
    for(int i=0;i<4;i++)
        assert(fabsf(marks[i].rgb[0]-r)<1e-5f&&fabsf(marks[i].rgb[1]-g)<1e-5f&&fabsf(marks[i].rgb[2]-b)<1e-5f);
}
static void center(float r,float g,float b) {
    assert(centerDraws==1&&centerRgb[0]==r&&centerRgb[1]==g&&centerRgb[2]==b);
}
int main(void) {
    playerState_t *ps=&cg->predictedPlayerState;
    ps->weapon=1;cg->refdef.fov_y=60;cg->time=1000;
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
    /* Every resolution gets sharp marks of whole pixels at the measured
     * width and opacity, at any spread, with the crosshair off center too,
     * and with either sub-pixel offset of the 2D projection; they still
     * start where the spread puts them. */
    static const float screens[4][3]={{1024,576,1.2f},{1280,720,1.5f},{1920,1080,2.25f},{2560,1440,3}};
    static const float shifts[3][2]={{0,0},{-.075f,-.0375f},{-.15f,-.15f}};
    int widths=0;
    for(int s=0;s<4;s++)for(int dynamic=0;dynamic<2;dynamic++)for(int o=0;o<3;o++)for(int spread=0;spread<=255;spread+=85) {
        screenWidth=screens[s][0];screenHeight=screens[s][1];screenScale=screens[s][2];
        cg_crosshairDynamic=dynamic?&enabled:&disabled;
        rasterShift[0]=shifts[o][0];rasterShift[1]=shifts[o][1];
        ps->aimSpreadScale=spread;cg->time=1000;reset();CG_DrawCrosshair();
        visible(fmaxf(1-spread/255.f,CG_CROSSHAIR_SPREAD_ALPHA_MIN));
        assert(fabsf(gap()-expected(1+4*spread/255.f))<1e-3f);
        widths|=1<<marks[0].pixels;
    }
    assert(widths==(1<<1|1<<2));
    screenWidth=1280;screenHeight=720;screenScale=1.5f;cg_crosshairDynamic=&disabled;
    rasterShift[0]=rasterShift[1]=0;ps->aimSpreadScale=0;
    /* Both the side and center fade with the weapon-overlay transition, and
     * going into ADS shortens the marks with the spread. */
    ps->fWeaponPosFrac=.5f;visibility=.4f;
    weapons[1].hReticleCenter=2;reset();CG_DrawCrosshair();
    visible(.4f);assert(centerDraws==1&&overlayCalls==1);
    assert(fabsf(centerAlpha-.4f)<1e-6f);
    ps->fWeaponPosFrac=.75f;visibility=1;drawnLength=CG_CROSSHAIR_LINE_LENGTH*.75f;
    reset();CG_DrawCrosshair();visible(.75f);
    assert(fabsf(gap()-expected(.75f))<1e-3f);
    drawnLength=CG_CROSSHAIR_LINE_LENGTH;
    visibility=0;reset();CG_DrawCrosshair();assert(!rasterCalls&&!centerDraws);
    visibility=1;ps->fWeaponPosFrac=1;
    reset();CG_DrawCrosshair();assert(!rasterCalls&&!centerDraws);
    ps->fWeaponPosFrac=0;
    for(int gate=0;gate<5;gate++) {
        ps->pm_flags=PMF_SPRINT;
        hudHidden=(gate==0);cg->renderingThirdPerson=(gate==1);
        cg_paused->current.integer=(gate==2);cg_drawCrosshair=(gate==3?&disabled:&enabled);
        ps->weaponstate=(gate==4?5:0);
        reset();CG_DrawCrosshair();assert(!rasterCalls&&!centerDraws);
    }
    ps->pm_flags=0;ps->weaponstate=0;CG_ResetSprintView();
    /* Only the image's file name says it is the bar, in either case and after
     * either slash. */
    static const char *bars[3]={"GFX\\RETICLE\\SIDE_SKINNY.TGA","side_skinny_xenon","gfx/reticle/side_skinny.tga"};
    for(int i=0;i<3;i++) {weapon.szReticleSide=bars[i];frame(1300);visible(1);}
    /* Other side marks keep their image: the launchers' arcs are four images
     * 24 units across, turned a quarter each, at the crosshair alpha, from
     * their minimum offset or the spread out. */
    weapon.szReticleSide="gfx/reticle/hud@side_small_arc";weapon.iReticleSideSize=24;weapon.iReticleMinOfs=17;
    for(int spread=0;spread<=255;spread+=255) {
        ps->aimSpreadScale=spread;reset();CG_DrawCrosshair();
        float m=fmaxf(17,expected(1+4*spread/255.f)),s=24,v=s*.125f;
        static const float angles[4]={0,90,180,270};
        const float at[4][2]={{-s/2,-s-m-v},{m,-s/2},{-s/2-v,m},{-s-m-v,-s/2-v}};
        assert(m>(spread?20:16.9f)&&!rasterCalls&&picCount==4);
        for(int i=0;i<4;i++)
            assert(fabsf(pics[i].x-at[i][0])<1e-4f&&fabsf(pics[i].y-at[i][1])<1e-4f&&pics[i].w==s&&pics[i].h==s&&
                   pics[i].angle==angles[i]&&pics[i].material==1&&
                   fabsf(pics[i].alpha-fmaxf(1-spread/255.f,CG_CROSSHAIR_SPREAD_ALPHA_MIN))<1e-6f);
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
replacements = [
    (teammate_color, '        if (1) {\n'),
    (teammate_color, '        CG_GetTeammateNameColor(color);\n        {\n'),
    ('if (pmFlags & 0x100000) {', 'if (pmFlags & 0x300000) {'),
    (white, '    } else if (pmFlags & 0x200000) {\n'
            '        color[0] = 1.0f;\n'
            '        color[1] = 0.25f;\n'
            '        color[2] = 0.25f;\n' + white),
    ('if (pmFlags & 0x100000) {', 'if (0) {'),
    ('color[3] = reticleVisibility *', 'color[3] = posLerp *'),
    ('(alpha >= alphaMin ? alpha : alphaMin) * reticleVisibility', '(alpha >= alphaMin ? alpha : alphaMin)'),
    ('if (cg->renderingThirdPerson)\n', 'if (cg->renderingThirdPerson || (ps->pm_flags & PMF_SPRINT))\n'),
    (blend_call, ''),
    (blend_call, blend_call.replace('CG_SprintCarryBlend(ps)', '((ps->pm_flags & PMF_SPRINT) ? 1.0f : 0.0f)')),
    (alpha_floor, 'cg_crosshairAlphaMin->current.value'),
    (alpha_floor, 'CG_CROSSHAIR_SPREAD_ALPHA_MIN * cg_crosshairAlpha->current.value'),
    # Fractional or unrounded widths, or no making up the opacity, lose the measured line.
    ('pixels = fmaxf(1.0f, floorf(exact + 0.5f));', 'pixels = fmaxf(1.0f, exact);'),
    ('pixels = fmaxf(1.0f, floorf(exact + 0.5f));', 'pixels = fmaxf(1.0f, floorf(exact));'),
    (' * exact / pixels);', ');'),
    ('reticleColor[3] * CG_CROSSHAIR_LINE_ALPHA *', 'reticleColor[3] *'),
    # Marks off the pixel grid, or softened past it, are blurred.
    ('        x = floorf(x) + 0.5f;\n', '        x += 0.5f;\n'),
    ('        y = floorf(y + 0.5f);\n', ''),
    ('CG_DrawHudLines(lines, 4, pixels, 0.5f, reticleColor);', 'CG_DrawHudLines(lines, 4, pixels, 1.0f, reticleColor);'),
    # The spread and the length, in pixels and through ADS.
    ('    minOfs *= scale;\n', ''),
    ('float length = CG_CROSSHAIR_LINE_LENGTH * transScale;', 'float length = CG_CROSSHAIR_LINE_LENGTH;'),
    ('    lines[1][0] = x + minOfs;\n', '    lines[1][0] = x + minOfs + 1.0f;\n'),
    # Every side image drawn as the bar's lines, or none, or the bar told by
    # its path, its case or only one slash; the images misplaced.
    ('    return !I_strnicmp(file, "side_skinny", 11);\n', '    return 1;\n'),
    ('        if (CG_CrosshairSideIsBar(weapDef->szReticleSide))\n',
     '        if (!CG_CrosshairSideIsBar(weapDef->szReticleSide))\n'),
    ('            file = name + 1;\n', '            file = file;\n'),
    ('    return !I_strnicmp(file, "side_skinny", 11);\n', '    return !strncmp(file, "side_skinny", 11);\n'),
    ("        if (*name == '/' || *name == '\\\\')\n", "        if (*name == '/')\n"),
    ('CG_DrawRotatedPic(minOfs + ax, ay,', 'CG_DrawRotatedPic(ax, ay,'),
    # Lines whose ends are not ramped about them, or whose sides fade too far out.
    ('        along[0] = -0.5f;\n', '        along[0] = 0.0f;\n'),
    ('    across[0] = (width + feather) * -0.5f;\n', '    across[0] = (width + feather) * -1.0f;\n'),
]
mutants = []
for old, new in replacements:
    assert body.count(old) >= 1, old
    mutants.append(body.replace(old, new))
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
        assert (result.returncode == 0) == (index < 2), (index, result.stderr[-2000:])
print('PASS: four thin hip-fire lines rasterized at 18 spread levels with a 0.6 alpha floor, whole pixels wide '
      'and sharp at 576p to 1440p, on and off center and with both sub-pixel 2D offsets, their opacity making up '
      'the measured width, starting exactly at the spread; sprint opening/closing over 66 frame intervals '
      '(linear and eased), firing out of sprint, white over enemies, the teammate labels\' color over teammates '
      '(green with the stock labels), ADS fade and shortening, five HUD gates; the bar told by its file name in any '
      'case, other side images such as the launchers\' arcs drawn as before; '
      f'{len(mutants)} failing regression mutants')
