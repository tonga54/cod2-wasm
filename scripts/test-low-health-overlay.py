#!/usr/bin/env python3
"""Exercise the real blood pulse, alpha interpolation and blood preference."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_newDraw_mp.c').read_text()

def function(signature):
    start = source.index(signature + '\n{')
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

support = r'''
#include <assert.h>
#include <string.h>
typedef float vec_t;
typedef int MaterialHandle,rectDef_t;
typedef struct { union { float value; int integer,enabled; } current; } dvar_t;
static dvar_t blood={.current.enabled=1},healthbar={.current.enabled=0};
static const dvar_t *cg_blood=&blood,*cg_drawHealth=&healthbar;
static struct {
    int time,healthOverlayLastHitTime,healthOverlayPulseIndex,healthOverlayPulseTime,
        healthOverlayPulseDuration,healthOverlayHurt,healthOverlayPulsePhase;
    float healthOverlayOldHealth,healthOverlayFromAlpha,healthOverlayToAlpha;
} state,*cg=&state;
static float health=1,lastAlpha;
static int draws;
static float CG_CalcPlayerHealth(void) { return health; }
static void CG_CopyColor(float *dst,const float *color,float alpha) {
    memcpy(dst,color,4*sizeof(float)); dst[3]=alpha;
}
static void CG_DrawHudPic(const rectDef_t *rect,MaterialHandle material,float *color) {
    assert(material==123);assert(color[3]>0 && color[3]<=1);lastAlpha=color[3];draws++;
}
'''
values = {
    'regenPauseTime':('integer','5000'), 'pulseStart':('value','.35f'),
    'phaseOne_pulseDuration':('integer','150'), 'phaseTwo_toAlphaMultiplier':('value','.7f'),
    'phaseTwo_pulseDuration':('integer','320'), 'phaseThree_toAlphaMultiplier':('value','.6f'),
    'phaseThree_pulseDuration':('integer','400'), 'phaseEnd_toAlpha':('value','0'),
    'phaseEnd_pulseDuration':('integer','700')}
for name,(kind,value) in values.items():
    support += f'static const dvar_t var_{name}={{.current.{kind}={value}}};\n'
    support += f'static const dvar_t *hud_healthOverlay_{name}=&var_{name};\n'
table = re.search(r'static const float pulseMags\[4\][^;]*;',source).group()
body = table + '\n' + function('void CG_PulseLowHealthOverlay(float healthRatio)') + function(
    'static inline __attribute__((always_inline)) void CG_DrawLowHealthOverlay(const rectDef_t *rect, MaterialHandle material, vec_t *color)')
checks = r'''
int main(void) {
    rectDef_t rect=0;float color[4]={1,1,1,1};
    state.healthOverlayOldHealth=1;
    CG_DrawLowHealthOverlay(&rect,123,color);assert(draws==0);
    health=.30f;state.time=100;CG_DrawLowHealthOverlay(&rect,123,color);
    assert(state.healthOverlayLastHitTime==100);
    state.time=175;CG_DrawLowHealthOverlay(&rect,123,color);
    assert(draws==1 && lastAlpha>.4f && lastAlpha<.6f);
    /* Blood is visible even with the health bar off, and respects cg_blood=0. */
    blood.current.enabled=0;int before=draws;
    state.time=200;CG_DrawLowHealthOverlay(&rect,123,color);assert(draws==before);
    blood.current.enabled=1;state.time=250;CG_DrawLowHealthOverlay(&rect,123,color);
    assert(draws>before);
    health=1;
    for(state.time=260;state.time<8000;state.time+=10) CG_DrawLowHealthOverlay(&rect,123,color);
    assert(!state.healthOverlayHurt && state.healthOverlayToAlpha==0);
    before=draws;CG_DrawLowHealthOverlay(&rect,123,color);assert(draws==before);
    /* A new severe hit restarts the blood pulse after recovery. */
    health=.2f;state.time=8100;CG_DrawLowHealthOverlay(&rect,123,color);
    state.time=8175;CG_DrawLowHealthOverlay(&rect,123,color);assert(draws>before);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-blood-overlay-') as directory:
    path=Path(directory)
    disabled=body.replace('if (!cg_blood->current.enabled)', 'if (!cg_drawHealth->current.enabled)')
    invisible=body.replace(table, 'static const float pulseMags[4];')
    for index,variant in enumerate((body,disabled,invisible)):
        (path/'test.c').write_text(support+variant+checks)
        subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',
                        str(path/'test.c'),'-o',str(path/'test')],check=True)
        result=subprocess.run([str(path/'test')],capture_output=True,text=True)
        assert (result.returncode==0)==(index==0),result.stderr
print('PASS: severe damage draws and pulses blood, respects its preference, fades after recovery and restarts on a new hit; both invisible-overlay mutants fail')
