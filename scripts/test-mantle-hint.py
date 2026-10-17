#!/usr/bin/env python3
"""Run the native vault prompt with real drawing and binding dispatch guards."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_newDraw_mp.c').read_text()
start = source.index('void CG_DrawMantleHint(')
start = source.index('void CG_DrawMantleHint(', start + 1)
brace = source.index('{', start)
end, depth = brace + 1, 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
body = source[start:end]
support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef float vec4_t[4];typedef unsigned char byte;
struct Font_s {int unused;};
typedef struct {float x,y,w,h;int horzAlign,vertAlign;} rectDef_t;
static struct {int renderingThirdPerson;struct {int pm_type,pm_flags,stats[1];struct {int flags;} mantleState;} predictedPlayerState;} state,*cg=&state;
static struct {struct {int mantleHint;} media;} graphics,*cgs=&graphics;
static struct {struct {int enabled;} current;} enabled={{1}},*cg_drawMantleHint=&enabled;
static vec4_t color; /* Uninitialized shared color made the old prompt invisible. */
static int primary=1,fallback,draws,pics,queries,configs;static float textX,iconX;
static void Controls_GetConfig(void){configs++;}
static int GetKeyBindingLocalizedString(const char *command,char *binding){
 queries++;int bound=!strcmp(command,"+gostand")?primary:fallback;
 strcpy(binding,bound==2?"Mouse 4":bound?"Space":"UNBOUND");return bound;
}
static const char *UI_SafeTranslateString(const char *key){assert(!strcmp(key,"PLATFORM_MANTLE"));return "Press [&&1] to  ";}
static const char *UI_ReplaceConversionString(const char *text,const char *binding){
 static char result[128];snprintf(result,sizeof(result),"Press [%s] to  ",binding);return result;
}
static int UI_TextWidth(const char *text,int n,struct Font_s *font,float scale){return strlen(text)*7;}
static int UI_TextHeight(struct Font_s *font,float scale){return 20;}
static void UI_DrawText(const char *text,int n,struct Font_s *font,float x,float y,int ha,int va,float scale,const float *tint,int style){
 assert(strstr(text,primary==2?"Mouse 4":"Space"));assert(ha==2&&va==2&&style==3);
 for(int i=0;i<4;i++)assert(tint[i]==1);textX=x;draws++;
}
static void UI_DrawHandlePic(float x,float y,float w,float h,int ha,int va,const float *tint,int material){
 assert(material==7&&w==40&&h==40&&ha==2&&va==2);
 for(int i=0;i<4;i++)assert(tint[i]==1);iconX=x;pics++;
}
/* The WWII prompt draws (1) or declines (0) with the key the stock hint found. */
static int promptActive,promptResult,promptDraws;
static int CG_MantlePromptActive(void){return promptActive;}
static int CG_DrawMantlePrompt(const rectDef_t *rect,const char *key){
 assert(rect->y==130&&!strcmp(key,primary==2?"Mouse 4":"Space"));promptDraws++;return promptResult;
}
'''
checks = r'''
int main(void){
 rectDef_t rect={0,130,40,40,2,2};
 /* The prompt switched off (even if it would draw), drawn, or declining. */
 for(int mode=0;mode<3;mode++)for(int gate=0;gate<9;gate++){
  memset(&state,0,sizeof(state));state.predictedPlayerState.stats[0]=100;
  state.predictedPlayerState.mantleState.flags=8;enabled.current.enabled=1;
  graphics.media.mantleHint=7;primary=1;fallback=0;draws=pics=queries=configs=promptDraws=0;
  promptActive=mode>0;promptResult=mode<2;
  if(gate==1)enabled.current.enabled=0;
  if(gate==2)state.renderingThirdPerson=1;
  if(gate==3)state.predictedPlayerState.pm_type=6;
  if(gate==4)state.predictedPlayerState.stats[0]=0;
  if(gate==5)state.predictedPlayerState.pm_flags=4;
  if(gate==6)graphics.media.mantleHint=0;
  if(gate==7)state.predictedPlayerState.mantleState.flags=0;
  if(gate==8)primary=0;
  CG_DrawMantleHint(&rect,0,.21f,3);
  assert(promptDraws==(gate==0&&mode>0));assert(draws==(gate==0&&mode!=1)&&pics==draws);
 }
 promptActive=0;promptResult=1;
 primary=0;fallback=1;draws=pics=0;CG_DrawMantleHint(&rect,0,.21f,3);assert(draws==1&&pics==1);
 primary=2;fallback=0;draws=pics=0;CG_DrawMantleHint(&rect,0,.21f,3);assert(draws==1&&pics==1);
 assert(fabsf((textX+iconX+40)*.5f)<.001f);
 promptActive=1;draws=pics=promptDraws=0;
 primary=0;fallback=1;CG_DrawMantleHint(&rect,0,.21f,3);assert(promptDraws==1&&!draws&&!pics);
 primary=2;fallback=0;CG_DrawMantleHint(&rect,0,.21f,3);assert(promptDraws==2&&!draws&&!pics);
}
'''
hand_off = 'if (CG_MantlePromptActive() && CG_DrawMantlePrompt(rect, binding))\n        return;\n'
assert body.count(hand_off) == 1
assert body.index('GetKeyBindingLocalizedString("+moveup", binding)') < body.index(hand_off)
mutants = [
    ('        hintColor,', '        color,'),
    ('if (CG_MantlePromptActive() && CG_DrawMantlePrompt(', 'if (CG_DrawMantlePrompt('),
    (hand_off, 'if (CG_MantlePromptActive() && CG_DrawMantlePrompt(rect, binding))\n        ;\n'),
    ('CG_DrawMantlePrompt(rect, binding)', '1'),
]
variants = [body]
for old, new in mutants:
    assert old in body, old
    variants.append(body.replace(old, new))
with tempfile.TemporaryDirectory(prefix='cod2-mantle-hint-') as directory:
    path = Path(directory)
    for variant in variants:
        (path/'test.c').write_text(support+variant+checks)
        subprocess.run(['cc','-std=c99','-O1','-fsanitize=address,undefined',str(path/'test.c'),'-lm','-o',str(path/'test')],check=True)
        result=subprocess.run([str(path/'test')],capture_output=True,text=True)
        assert (result.returncode==0)==(variant==body),result.stderr
print('PASS: opaque native mantle text/icon, localized rebound key, fallback binding, centered layout and live '
      'first-person guards; with cg_hudMantlePrompt on the WWII prompt gets the found key behind the same guards, '
      f'and the stock hint draws only when it is off or declines; {len(mutants)} failing regression mutants')
