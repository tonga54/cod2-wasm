#!/usr/bin/env python3
"""Run the production 2D HUD element loop with the stock damage-feedback and CTF flag elements."""
from pathlib import Path
import re
import subprocess
import tempfile

from hud_raster import RASTER_C, hud_vertex_definition

root = Path(__file__).resolve().parent.parent
hud_source = (root / 'src/PC/cgame_mp/cg_hudelem_mp.c').read_text()
vector_source = (root / 'src/PC/universal/com_vector.h').read_text()
types_source = (root / 'src/headers/PC/cgame_mp/cg_types.h').read_text()


def function(name):
    match = re.search(r'^([^\n]*\b' + name + r'\([^;]*?\))\s*\{', hud_source, re.M)
    assert match, name
    brace = hud_source.index('{', match.start())
    depth, end = 1, brace + 1
    while depth:
        depth += (hud_source[end] == '{') - (hud_source[end] == '}')
        end += 1
    return hud_source[match.start():end] + '\n'


def definition(source, head):
    start = source.index(head)
    return source[start:source.index('\n};\n', start) + 4]


assert 'CG_GetHudElemColor(elem, cghe->color);' in function('CG_GetHudElemInfo')
constants = re.findall(r'^#define (CG_HIT_MARKER_\w+|CG_HUD_LINES_MAX) (.+)$', hud_source, re.M)
values = dict(constants)
assert set(values) == {
    'CG_HUD_LINES_MAX', 'CG_HIT_MARKER_MATERIAL', 'CG_HIT_MARKER_BOX', 'CG_HIT_MARKER_INNER', 'CG_HIT_MARKER_OUTER',
    'CG_HIT_MARKER_KILL_OUTER', 'CG_HIT_MARKER_WIDTH', 'CG_HIT_MARKER_HOLD_MS', 'CG_HIT_MARKER_FADE_MS',
    'CG_HIT_MARKER_KILL_WINDOW_MS'}, values
# The script's box, and the WWII X measured in it: lines from under 6 units
# out, the kill's longer, under a unit wide.
assert values['CG_HIT_MARKER_BOX'] == '24.0f'
inner, outer, kill_outer = (float(values[name].rstrip('f')) for name in (
    'CG_HIT_MARKER_INNER', 'CG_HIT_MARKER_OUTER', 'CG_HIT_MARKER_KILL_OUTER'))
assert 0 < inner < 6 < outer < kill_outer < 16
assert 0 < float(values['CG_HIT_MARKER_WIDTH'].rstrip('f')) < 1
statics = re.findall(r'^static (?:const )?\w+ s_hitMarker\w+\[\d*\](?: = \{[^}\n]*\})?;$', hud_source, re.M)
assert len(statics) == 3, statics

support = r'''
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef int qboolean; typedef unsigned char byte; typedef float vec_t; typedef float vec4_t[4]; typedef int FontHandle;
typedef const void *MaterialHandle;
typedef struct inflate_huft inflate_huft;
''' + definition(vector_source, 'enum he_type_t {') + r'''
typedef enum he_type_t he_type_t;
''' + definition(vector_source, 'union hudelem_color_t {') + r'''
typedef union hudelem_color_t hudelem_color_t;
''' + definition(vector_source, 'struct hudelem_s {') + r'''
typedef struct hudelem_s hudelem_t;
''' + definition(types_source, 'struct cg_hudelem_t {') + r'''
typedef struct cg_hudelem_t cg_hudelem_t;
''' + ''.join(f'#define {name} {value}\n' for name, value in constants) + hud_vertex_definition(root) + r'''
/* The snapshot's server time, then its player state: its client, and from
 * 0x7a8 the current and archived HUD elements the loop reads. */
typedef struct {int snapFlags,ping,serverTime;struct {int clientNum;} ps;} snapshot_t;
#define SNAPSHOT_BYTES (0x0C+0x7a8+62*sizeof(hudelem_t))
static struct {int time;snapshot_t *snap,*nextSnap;} cgStorage,*cg=&cgStorage;
static byte snapshots[2][SNAPSHOT_BYTES] __attribute__((aligned(16)));
static hudelem_t *hud;
static const char whiteTag[]="white";
static struct {struct {MaterialHandle whiteMaterial;} media;} cgsStorage={.media={.whiteMaterial=whiteTag}},*cgs=&cgsStorage;
/* A 640x480 virtual HUD centered on a 1280x720 canvas, SCALE physical pixels a unit. */
static float SCALE=1.5f;
static const char *const materials[]={"","damage_feedback","hudflag_axis"};
static struct {float x,y,w,h,color[4];const void *material;} draws[8];
static int drawCount;
static void CG_GetHudElemColor(const hudelem_t *elem,vec4_t color);
static int CG_ServerMaterialName(int index,char *name,int size) {
    if(index<1||index>2)return 0;
    strncpy(name,materials[index],size-1);name[size-1]=0;return 1;
}
static void *CL_RegisterMaterial(const char *name,int imageTrack) {
    for(int i=1;i<3;i++)if(!strcmp(name,materials[i]))return (void *)&materials[i];
    abort();
}
static void *CL_RegisterMaterialNoMip(const char *name,int imageTrack) {abort();}
static void I_strncat(char *dest,int size,const char *src) {abort();}
static float AngleNormalize360(float angle) {abort();}
static int I_stricmp(const char *a,const char *b) {
    while(*a&&tolower((byte)*a)==tolower((byte)*b))a++,b++;
    return tolower((byte)*a)-tolower((byte)*b);
}
static void CalcScreenPlacement(float *x,float *y,float *w,float *h,int horzAlign,int vertAlign) {
    *x*=SCALE;*y*=SCALE;*w*=SCALE;*h*=SCALE;
}
static void CL_DrawStretchPicPhysical(float x,float y,float w,float h,float s0,float t0,float s1,float t1,
    vec4_t *color,void *material) {
    assert(drawCount<8&&s0==0&&t0==0&&s1==1&&t1==1);
    draws[drawCount].x=x;draws[drawCount].y=y;draws[drawCount].w=w;draws[drawCount].h=h;
    memcpy(draws[drawCount].color,*color,sizeof(vec4_t));draws[drawCount++].material=material;
}
static void CG_DrawRotatedPicPhysical(float x,float y,float w,float h,float angle,vec4_t *color,void *material) {abort();}
/* The score panel stands in for elements of this material. */
static int panelMaterial;
static int CG_ScorePanelHidesHudElem(const hudelem_t *elem) {return panelMaterial&&elem->materialIndex==panelMaterial;}
/* And the killcam for elements of this one. */
static int killcamMaterial;
static int CG_KillcamHidesHudElem(const hudelem_t *elem) {return killcamMaterial&&elem->materialIndex==killcamMaterial;}
static void CG_DrawHudElemString(const char *text,const hudelem_t *elem,cg_hudelem_t *cghe) {abort();}
/* Center-aligned elements, as the scripts create them; color comes from the real fade. */
static void CG_GetHudElemInfo(const hudelem_t *elem,cg_hudelem_t *cghe,char *hudElemString) {
    memset(cghe,0,sizeof(*cghe));
    cghe->x=640+elem->x*SCALE;cghe->y=360+elem->y*SCALE;cghe->fontHeight=16;
    CG_GetHudElemColor(elem,cghe->color);
}
''' + RASTER_C + '\n'.join(statics) + '\n'
body = ''.join(function(name) for name in (
    'cg_time', 'cg_snap', 'clamp01', 'CG_GetDimValue', 'CG_GetMoveLerpFactor', 'CG_AlignY',
    'CG_GetWidth', 'CG_GetHeight', 'compare_hudelems', 'CG_GetHudElemColor', 'CG_DrawHudLines',
    'CG_HitMarkerInit', 'CG_HitMarkerObituary', 'CG_HitMarkerShowsKill', 'CG_DrawHitMarker',
    'CG_Draw2dHudElems'))
checks = r'''
static const float killColor[3]={0.86f,0.16f,0.13f};
static void setColor(hudelem_color_t *color,byte r,byte g,byte b,byte a) {
    byte rgba[4]={r,g,b,a};memcpy(color,rgba,4);
}
/* Draws the snapshot shown at time; returns the stretched pictures and line draws. */
static int frame(int time) {
    cg->time=time;drawCount=0;rasterClear(640-RASTER_SIZE/2,360-RASTER_SIZE/2);
    CG_Draw2dHudElems(0);
    return drawCount+rasterCalls;
}
static int find(int material) {
    for(int i=0;i<drawCount;i++)if(draws[i].material==&materials[material])return i;
    return -1;
}
static void near(float a,float b) {assert(fabsf(a-b)<1e-4f);}
/* The flag icon keeps its white color and 24x24 units, scaled to the canvas. */
static void flag(float alpha) {
    int i=find(2);assert(i>=0);
    near(draws[i].x,640+100*SCALE);near(draws[i].y,360+50*SCALE);
    near(draws[i].w,24*SCALE);near(draws[i].h,24*SCALE);
    near(draws[i].color[0],1);near(draws[i].color[1],1);near(draws[i].color[2],1);
    near(draws[i].color[3],alpha);
}
/* The X as drawn, about the pixel center where the crosshair's lines meet:
 * each canvas pixel's distance along and across the diagonal of its corner.
 * Through the middle of a line, its opacity ramps over a pixel about its
 * sides; along its middle, over a pixel about its ends. Thinner than a pixel,
 * a line is drawn a pixel wide and fainter. Four equal lines of the color,
 * nothing past their ramps. */
static void marker(float alpha,int kill) {
    const float cx=640.5f,cy=360.5f,unit=SCALE,diagonal=0.70710678f;
    const float lo=CG_HIT_MARKER_INNER*unit,hi=(kill?CG_HIT_MARKER_KILL_OUTER:CG_HIT_MARKER_OUTER)*unit;
    float width=CG_HIT_MARKER_WIDTH*unit,strength=alpha;
    if(width<1){strength*=width;width=1;}
    const float *color=kill?killColor:(const float[3]){1,1,1};
    float coverage=0,corner[4]={0},nearest=1e9f,farthest=0;
    int middles=0,ends=0;
    assert(find(1)<0&&rasterCalls==1&&rasterMaterial==whiteTag);
    for(int y=0;y<RASTER_SIZE;y++)for(int x=0;x<RASTER_SIZE;x++) {
        const float *p=raster[y][x];
        float dx=rasterLeft+x+.5f-cx,dy=rasterTop+y+.5f-cy;
        float along=(fabsf(dx)+fabsf(dy))*diagonal,across=(fabsf(dx)-fabsf(dy))*diagonal;
        if(along>=lo+.5f&&along<=hi-.5f) {
            float side=fminf(1,fmaxf(0,(width+1)*.5f-fabsf(across)));
            assert(fabsf(p[3]-strength*side)<5e-4f);
            middles+=side>0;
        }
        if(fabsf(across)<=(width-1)*.5f) {
            float end=fminf(1,fmaxf(0,fminf(along-lo+.5f,hi+.5f-along)));
            assert(fabsf(p[3]-strength*end)<5e-4f);
            ends+=end>0&&end<1;
        }
        if(p[3]<=1e-6f)continue;
        assert(along>=lo-.5f-1e-3f&&along<=hi+.5f+1e-3f);
        assert(fabsf(across)<=(width+1)*.5f+1e-3f);
        for(int k=0;k<3;k++)assert(fabsf(p[k]/p[3]-color[k])<1e-4f);
        coverage+=p[3];corner[(dx>0)+2*(dy>0)]+=p[3];
        nearest=fminf(nearest,along);farthest=fmaxf(farthest,along);
    }
    for(int k=0;k<4;k++)assert(fabsf(corner[k]-coverage/4)<=1e-4f*coverage);
    assert(nearest<=lo+.5f&&farthest>=hi-.5f&&middles>=4*(int)(hi-lo-1)&&ends>=4);
}
static void none(void) {assert(find(1)<0&&rasterCalls==0);}
static void hit(int time) {
    /* _damagefeedback.gsc: alpha = 1; fadeOverTime(1); alpha = 0; */
    setColor(&hud[0].fromColor,255,255,255,255);setColor(&hud[0].color,255,255,255,0);
    hud[0].fadeStartTime=time;hud[0].fadeTime=1000;
}
/* The next snapshot arrives with the server time and, delivered with it,
 * the obituaries; it is drawn from its time on, the marker as it has it. */
static snapshot_t *shown,*arriving;
static void arrive(int time) {arriving->serverTime=time;cg->nextSnap=arriving;}
static void transition(void) {
    memcpy((byte *)arriving+0x0C+0x7a8,(byte *)shown+0x0C+0x7a8,62*sizeof(hudelem_t));
    arriving->ps.clientNum=shown->ps.clientNum;
    snapshot_t *swap=shown;shown=arriving;arriving=swap;
    cg->snap=shown;hud=(hudelem_t *)((byte *)shown+0x0C+0x7a8);
}
int main(void) {
    shown=(snapshot_t *)snapshots[0];arriving=(snapshot_t *)snapshots[1];
    cg->snap=shown;hud=(hudelem_t *)((byte *)shown+0x0C+0x7a8);
    shown->ps.clientNum=3;CG_HitMarkerInit();
    hud[0]=(hudelem_t){.type=HE_TYPE_MATERIAL,.x=-12,.y=-12,.width=24,.height=24,.materialIndex=1};
    hud[1]=(hudelem_t){.type=HE_TYPE_MATERIAL,.x=100,.y=50,.width=24,.height=24,.materialIndex=2,.sort=1};
    setColor(&hud[0].color,255,255,255,0);
    /* ctf.gsc fades the carried-flag icon in over 0.2 s: fromColor holds the
     * transparent start and color the opaque target. */
    setColor(&hud[1].fromColor,255,255,255,0);setColor(&hud[1].color,255,255,255,255);
    hud[1].fadeStartTime=1000;hud[1].fadeTime=200;
    assert(frame(900)==0&&frame(1000)==0);
    assert(frame(1050)==1);flag(.25f);
    assert(frame(1100)==1);flag(.5f);
    assert(frame(1199)==1);flag(199/200.f);
    assert(frame(1200)==1);flag(1);
    assert(frame(4000)==1);flag(1);
    /* A fade also blends the color channels from the start to the target. */
    setColor(&hud[1].fromColor,255,0,0,255);setColor(&hud[1].color,0,0,255,255);
    hud[1].fadeStartTime=2000;hud[1].fadeTime=1000;frame(2500);
    int i=find(2);near(draws[i].color[0],.5f);near(draws[i].color[1],0);near(draws[i].color[2],.5f);
    setColor(&hud[1].color,255,255,255,255);hud[1].fadeTime=0;
    /* Each hit shows WWII's white X instead of the stock picture, at full
     * strength, holds it, then fades it out long before the script's
     * one-second fade ends. The flag icon is drawn unchanged next to it. */
    hit(5000);
    for(int t=4990;t<=5000+CG_HIT_MARKER_HOLD_MS;t+=10) {
        assert(frame(t)==2);marker(1,0);flag(1);
    }
    assert(frame(5000+CG_HIT_MARKER_HOLD_MS+CG_HIT_MARKER_FADE_MS/2)==2);marker(.5f,0);
    float last=1;
    for(int t=5000+CG_HIT_MARKER_HOLD_MS;t<5000+CG_HIT_MARKER_HOLD_MS+CG_HIT_MARKER_FADE_MS;t++) {
        assert(frame(t)==2&&rasterCalls==1);
        float alpha=1-(float)(t-5000-CG_HIT_MARKER_HOLD_MS)/CG_HIT_MARKER_FADE_MS;
        assert(alpha>0&&alpha<=last);last=alpha;
        if(t%20==0)marker(alpha,0);
    }
    assert(CG_HIT_MARKER_HOLD_MS+CG_HIT_MARKER_FADE_MS<=400);
    for(int t=5000+CG_HIT_MARKER_HOLD_MS+CG_HIT_MARKER_FADE_MS;t<=7000;t+=7) {
        assert(frame(t)==1);none();flag(1);
    }
    /* The next hit restarts it. */
    hit(7100);assert(frame(7100)==2);marker(1,0);
    assert(frame(7100+CG_HIT_MARKER_HOLD_MS+CG_HIT_MARKER_FADE_MS)==1);
    /* A script that shows the marker without a fade sets its strength. */
    setColor(&hud[0].color,255,255,255,153);hud[0].fadeTime=0;
    assert(frame(9000)==2);marker(.6f,0);
    /* Thinner than a pixel on a small canvas, the lines are a pixel wide and fainter. */
    SCALE=.75f;assert(frame(9000)==2);marker(.6f,0);SCALE=1.5f;
    setColor(&hud[0].color,255,255,255,0);assert(frame(9000)==1);
    /* Elements the score panel replaces are not drawn; the others still are. */
    panelMaterial=2;assert(frame(9000)==0);
    setColor(&hud[0].color,255,255,255,153);assert(frame(9000)==1&&rasterCalls==1&&drawCount==0);
    panelMaterial=0;assert(frame(9000)==2);
    /* So are those the killcam stands in for. */
    killcamMaterial=2;assert(frame(9000)==1&&find(2)<0&&rasterCalls==1);
    killcamMaterial=0;assert(frame(9000)==2);
    setColor(&hud[0].color,255,255,255,0);hud[1].type=HE_TYPE_FREE;
    /* Client 3 hits client 5 at 10000, then kills them at 10050 with the
     * snapshot after. That snapshot brings the obituary, but the marker drawn
     * until it is shown is the white one of the hit before; from then on the
     * killing hit's X is red and longer, and it holds and fades the same. */
    shown->serverTime=10000;hit(10000);assert(frame(10000)==1);marker(1,0);
    arrive(10050);CG_HitMarkerObituary(3,5);
    assert(frame(10030)==1);marker(1,0);
    transition();hit(10050);
    assert(frame(10050)==1);marker(1,1);
    assert(frame(10050+CG_HIT_MARKER_HOLD_MS+CG_HIT_MARKER_FADE_MS/2)==1);marker(.5f,1);
    assert(frame(10050+CG_HIT_MARKER_HOLD_MS+CG_HIT_MARKER_FADE_MS)==0);
    /* The next hit is white again, however soon. */
    arrive(10100);transition();hit(10100);assert(frame(10100)==1);marker(1,0);
    /* Other players' kills, suicides, deaths without a killer and attackers
     * that are not players leave the viewer's X white. */
    const int others[][2]={{4,5},{3,3},{-1,3},{64,5},{1022,5}};
    for(int k=0;k<5;k++) {
        int t=11000+k*1000;
        arrive(t);CG_HitMarkerObituary(others[k][0],others[k][1]);transition();hit(t);
        assert(frame(t)==1);marker(1,0);
    }
    /* A snapshot that brings the obituary late still turns the killing hit's X
     * red within the window, but not a hit before it. */
    arrive(20000);CG_HitMarkerObituary(3,6);transition();
    hit(20000-CG_HIT_MARKER_KILL_WINDOW_MS);assert(frame(20000-CG_HIT_MARKER_KILL_WINDOW_MS)==1);marker(1,1);
    hit(20000-CG_HIT_MARKER_KILL_WINDOW_MS-1);assert(frame(20000-CG_HIT_MARKER_KILL_WINDOW_MS-1)==1);marker(1,0);
    /* Without a snapshot arriving there is no kill to show. */
    cg->nextSnap=0;CG_HitMarkerObituary(3,7);
    arrive(21000);transition();hit(21000);assert(frame(21000)==1);marker(1,0);
    /* Following client 4 shows their kills, not the local player's. */
    shown->ps.clientNum=4;arrive(22000);CG_HitMarkerObituary(3,6);transition();
    hit(22000);assert(frame(22000)==1);marker(1,0);
    arrive(22100);CG_HitMarkerObituary(4,5);transition();
    hit(22100);assert(frame(22100)==1);marker(1,1);
    /* A new map forgets them, even for a hit as it starts. */
    CG_HitMarkerInit();assert(frame(22100)==1);marker(1,0);
    shown->serverTime=0;hit(0);assert(frame(0)==1);marker(1,0);
    return 0;
}
'''
color = function('CG_GetHudElemColor')
inverted = color
for channel in range(4):
    fixed = (f'((float)(int)fromColor[{channel}] + ((float)(int)toColor[{channel}] - '
             f'(float)(int)fromColor[{channel}]) * lerp)')
    assert fixed in inverted
    inverted = inverted.replace(fixed, f'((float)(int)toColor[{channel}] + ((float)(int)fromColor[{channel}] - '
                                       f'(float)(int)toColor[{channel}]) * lerp)')
replacements = [
    (color, inverted),
    ('if (!I_stricmp(materialName, CG_HIT_MARKER_MATERIAL))', 'if (0)'),
    ('        fadeElapsed -= CG_HIT_MARKER_HOLD_MS;\n', ''),
    ('color[3] = fadeElapsed > 0 ? 1.0f - (float)fadeElapsed / (float)CG_HIT_MARKER_FADE_MS : 1.0f;',
     'color[3] = fadeElapsed > 0 ? 1.0f - (float)fadeElapsed / (float)(2 * CG_HIT_MARKER_FADE_MS) : 1.0f;'),
    ('        if (CG_ScorePanelHidesHudElem(elem))\n            continue;\n', ''),
    ('        if (CG_KillcamHidesHudElem(elem))\n            continue;\n', ''),
    # The kill's X: its color and length, which hits it is, and whose kills count.
    ('        outer = CG_HIT_MARKER_KILL_OUTER;\n', ''),
    ('        color[1] = s_hitMarkerKillColor[1];\n', ''),
    ('    if (CG_HitMarkerShowsKill(elem)) {', '    if (0) {'),
    ('cg->snap->serverTime - killTime >= 0 && ', ''),
    ('killTime - elem->fadeStartTime >= 0 &&', '1 &&'),
    ('<= CG_HIT_MARKER_KILL_WINDOW_MS;', '<= CG_HIT_MARKER_KILL_WINDOW_MS / 2;'),
    ('attacker == target || ', ''),
    ('attacker >= 64 || ', 'attacker > 1022 || '),
    ('s_hitMarkerKillTime[attacker] = cg->nextSnap->serverTime;', 's_hitMarkerKillTime[attacker] = cg->snap->serverTime;'),
    ('int viewer = cg->snap->ps.clientNum;', 'int viewer = 3;'),
    ('    memset(s_hitMarkerKilled, 0, sizeof(s_hitMarkerKilled));\n', ''),
    # The X's geometry: centered on a pixel, on the diagonals, at the measured width.
    ('centerX = floorf(x + width * 0.5f) + 0.5f;', 'centerX = x + width * 0.5f;'),
    ('float unit = height / CG_HIT_MARKER_BOX * 0.70710678f;', 'float unit = height / CG_HIT_MARKER_BOX;'),
    ('CG_DrawHudLines(lines, 4, height / CG_HIT_MARKER_BOX * CG_HIT_MARKER_WIDTH, 1.0f, color);',
     'CG_DrawHudLines(lines, 4, CG_HIT_MARKER_WIDTH, 1.0f, color);'),
    ('CG_DrawHudLines(lines, 4, height / CG_HIT_MARKER_BOX * CG_HIT_MARKER_WIDTH, 1.0f, color);',
     'CG_DrawHudLines(lines, 4, height / CG_HIT_MARKER_BOX * CG_HIT_MARKER_WIDTH, 0.5f, color);'),
    # Lines without their feathered sides and ends, or not thinned below a pixel.
    ('(i == 0 || i == 3 || j == 0 || j == 3) ? 0.0f : alpha;', 'alpha;'),
    ('        alpha *= width;\n', ''),
    ('        along[3] = length + 0.5f;\n', '        along[3] = length;\n'),
]
mutants = []
for old, new in replacements:
    assert body.count(old) >= 1, old
    mutants.append(body.replace(old, new))
assert all(mutant != body for mutant in mutants)
with tempfile.TemporaryDirectory(prefix='cod2-hit-marker-') as directory:
    path = Path(directory) / 'hit_marker.c'
    binary = Path(directory) / 'hit_marker'
    for index, variant in enumerate([body] + mutants):
        path.write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', str(path), '-lm',
                        '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: script fades run from the start color to the target (CTF flag fade-in and color blend); '
      'each hit draws WWII\'s thin white X, rasterized as four equal antialiased diagonals of the measured '
      'length and width about the crosshair\'s pixel, which holds, fades out within 300 ms and restarts on the '
      'next hit; the viewed player\'s killing hit draws it red and longer once the snapshot with the obituary is '
      'shown, within the late-snapshot window only, and not for others\' kills, suicides or world deaths; '
      'other material elements are unchanged and those the score panel or the killcam replaces are skipped; '
      f'{len(mutants)} failing regression mutants')
