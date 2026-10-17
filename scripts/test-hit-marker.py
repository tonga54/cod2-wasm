#!/usr/bin/env python3
"""Run the production 2D HUD element loop with the stock damage-feedback and CTF flag elements."""
from pathlib import Path
import re
import subprocess
import tempfile

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
constants = re.findall(r'^#define (CG_HIT_MARKER_\w+) (.+)$', hud_source, re.M)
assert {name for name, _ in constants} == {
    'CG_HIT_MARKER_MATERIAL', 'CG_HIT_MARKER_SCALE', 'CG_HIT_MARKER_HOLD_MS', 'CG_HIT_MARKER_FADE_MS'}

support = r'''
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef int qboolean; typedef unsigned char byte; typedef float vec4_t[4]; typedef int FontHandle;
typedef struct inflate_huft inflate_huft;
''' + definition(vector_source, 'enum he_type_t {') + r'''
typedef enum he_type_t he_type_t;
''' + definition(vector_source, 'union hudelem_color_t {') + r'''
typedef union hudelem_color_t hudelem_color_t;
''' + definition(vector_source, 'struct hudelem_s {') + r'''
typedef struct hudelem_s hudelem_t;
''' + definition(types_source, 'struct cg_hudelem_t {') + r'''
typedef struct cg_hudelem_t cg_hudelem_t;
''' + ''.join(f'#define {name} {value}\n' for name, value in constants) + r'''
/* 640x480 virtual HUD on a 1280x720 canvas: 1.5 physical pixels per unit. */
#define SCALE 1.5f
static struct {int time;void *snap;} cgStorage,*cg=&cgStorage;
static byte snapshot[0x0C+0x7a8+62*sizeof(hudelem_t)] __attribute__((aligned(16)));
static hudelem_t *const hud=(hudelem_t *)(snapshot+0x0C+0x7a8);
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
static void CG_DrawHudElemString(const char *text,const hudelem_t *elem,cg_hudelem_t *cghe) {abort();}
/* Center-aligned elements, as the scripts create them; color comes from the real fade. */
static void CG_GetHudElemInfo(const hudelem_t *elem,cg_hudelem_t *cghe,char *hudElemString) {
    memset(cghe,0,sizeof(*cghe));
    cghe->x=640+elem->x*SCALE;cghe->y=360+elem->y*SCALE;cghe->fontHeight=16;
    CG_GetHudElemColor(elem,cghe->color);
}
'''
body = ''.join(function(name) for name in (
    'cg_time', 'cg_snap', 'clamp01', 'CG_GetDimValue', 'CG_GetMoveLerpFactor', 'CG_AlignY',
    'CG_GetWidth', 'CG_GetHeight', 'compare_hudelems', 'CG_GetHudElemColor', 'CG_DrawHitMarker',
    'CG_Draw2dHudElems'))
checks = r'''
static void setColor(hudelem_color_t *color,byte r,byte g,byte b,byte a) {
    byte rgba[4]={r,g,b,a};memcpy(color,rgba,4);
}
static int frame(int time) {cg->time=time;drawCount=0;CG_Draw2dHudElems(0);return drawCount;}
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
/* The hit marker is red, grown around the script's 24x24 box at screen center
 * and snapped to whole pixels. */
static void marker(float alpha) {
    int i=find(1);assert(i>=0);
    const float size=24*SCALE*CG_HIT_MARKER_SCALE;
    near(draws[i].w,size);near(draws[i].h,size);
    assert(draws[i].x==floorf(draws[i].x)&&draws[i].y==floorf(draws[i].y));
    assert(fabsf(draws[i].x+size/2-640)<=.5f&&fabsf(draws[i].y+size/2-360)<=.5f);
    near(draws[i].color[0],1);assert(draws[i].color[1]<.3f&&draws[i].color[2]<.3f);
    near(draws[i].color[3],alpha);
}
static void hit(int time) {
    /* _damagefeedback.gsc: alpha = 1; fadeOverTime(1); alpha = 0; */
    setColor(&hud[0].fromColor,255,255,255,255);setColor(&hud[0].color,255,255,255,0);
    hud[0].fadeStartTime=time;hud[0].fadeTime=1000;
}
int main(void) {
    cg->snap=snapshot;
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
    /* Each hit shows the red marker at full strength, holds it, then fades it
     * out long before the script's one-second fade ends. The flag icon is
     * drawn unchanged next to it. */
    hit(5000);
    for(int t=4990;t<=5000+CG_HIT_MARKER_HOLD_MS;t+=10) {
        assert(frame(t)==2);marker(1);flag(1);
    }
    assert(frame(5000+CG_HIT_MARKER_HOLD_MS+CG_HIT_MARKER_FADE_MS/2)==2);marker(.5f);
    float last=1;
    for(int t=5000+CG_HIT_MARKER_HOLD_MS;t<5000+CG_HIT_MARKER_HOLD_MS+CG_HIT_MARKER_FADE_MS;t++) {
        assert(frame(t)==2);i=find(1);
        assert(draws[i].color[3]>0&&draws[i].color[3]<=last);last=draws[i].color[3];
    }
    assert(CG_HIT_MARKER_HOLD_MS+CG_HIT_MARKER_FADE_MS<=400);
    for(int t=5000+CG_HIT_MARKER_HOLD_MS+CG_HIT_MARKER_FADE_MS;t<=7000;t+=7) {
        assert(frame(t)==1&&find(1)<0);flag(1);
    }
    /* The next hit restarts it. */
    hit(7100);assert(frame(7100)==2);marker(1);
    assert(frame(7100+CG_HIT_MARKER_HOLD_MS+CG_HIT_MARKER_FADE_MS)==1);
    /* A script that shows the marker without a fade sets its strength. */
    setColor(&hud[0].color,255,255,255,153);hud[0].fadeTime=0;
    assert(frame(9000)==2);marker(.6f);
    setColor(&hud[0].color,255,255,255,0);assert(frame(9000)==1);
    /* Elements the score panel replaces are not drawn; the others still are. */
    panelMaterial=2;assert(frame(9000)==0);
    setColor(&hud[0].color,255,255,255,153);assert(frame(9000)==1&&find(1)==0);
    panelMaterial=0;assert(frame(9000)==2);
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
marker_body = function('CG_DrawHitMarker')
mutants = [
    body.replace(color, inverted),
    body.replace('if (!I_stricmp(materialName, CG_HIT_MARKER_MATERIAL))', 'if (0)'),
    body.replace('        if (fadeElapsed >= CG_HIT_MARKER_FADE_MS)\n            return;\n', ''),
    body.replace('        fadeElapsed -= CG_HIT_MARKER_HOLD_MS;\n', ''),
    body.replace('x = floorf(x - (markerWidth - width) * 0.5f + 0.5f);', 'x = floorf(x + 0.5f);'),
    body.replace('vec4_t color = {1.0f, 0.2f, 0.15f, alpha};', 'vec4_t color = {1.0f, 0.2f, 0.15f, 1.0f};'),
    body.replace('float markerWidth = width * CG_HIT_MARKER_SCALE;', 'float markerWidth = width;'),
    body.replace('        if (CG_ScorePanelHidesHudElem(elem))\n            continue;\n', ''),
]
assert marker_body in body and all(mutant != body for mutant in mutants)
with tempfile.TemporaryDirectory(prefix='cod2-hit-marker-') as directory:
    path = Path(directory) / 'hit_marker.c'
    binary = Path(directory) / 'hit_marker'
    for index, variant in enumerate([body] + mutants):
        path.write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', str(path), '-lm',
                        '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr)
print('PASS: script fades run from the start color to the target (CTF flag fade-in and color blend); '
      'each hit draws a red, centered marker that holds, fades out within 300 ms and restarts on the next '
      'hit; other material elements are unchanged and those the score panel replaces are skipped; '
      f'{len(mutants)} failing regression mutants')
