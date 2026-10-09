#!/usr/bin/env python3
"""Use the actual HUD placement call to check independent horizontal/vertical anchors."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parent.parent
hud=(root/'src/PC/cgame_mp/cg_hudelem_mp.c').read_text()
call=re.search(r'CalcScreenPlacement\(&cghe->x, &cghe->y, &dummyWidth, &dummyHeight, [^;]+;',hud)[0]
source=(root/'src/PC/client_mp/screen_placement_mp.c').read_text()
a=source.index('void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign)\n{')
body=source[a:]
code=r'''
#include <assert.h>
static struct{float scaleVirtualToReal[2],scaleVirtualToFull[2],scaleRealToVirtual[2],realViewableMin[2],realViewableMax[2],realViewportSize[2],subScreenLeft,virtualScreenOffsetX;}sp;
#define SP (&sp)
'''+body+r'''
static void position(int alignScreen,float *x,float *y){struct{float x,y;}state,*cghe=&state;cghe->x=*x;cghe->y=*y;float dummyWidth=1,dummyHeight=1;
'''+call+r'''
*x=cghe->x;*y=cghe->y;}
int main(void){
 for(int scale=1;scale<=3;scale++){
  sp.scaleVirtualToReal[0]=sp.scaleVirtualToReal[1]=scale;sp.scaleVirtualToFull[0]=sp.scaleVirtualToFull[1]=scale;
  sp.realViewportSize[0]=800*scale;sp.realViewportSize[1]=480*scale;
  sp.realViewableMin[0]=20*scale;sp.realViewableMin[1]=10*scale;
  sp.realViewableMax[0]=780*scale;sp.realViewableMax[1]=470*scale;
  /* Killcam title: x=center_safearea, y=top, not x=left/y=center. */
  float x=0,y=30;position((7<<3)|1,&x,&y);assert(x==400*scale && y==40*scale);
  /* Countdown anchored to the right/bottom. */
  x=-20;y=-30;position((3<<3)|3,&x,&y);assert(x==760*scale && y==440*scale);
  x=100;y=20;position((4<<3)|4,&x,&y);assert(x==100*scale && y==20*scale);
 }
 return 0;}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)
 for mutant in (False,True):
  (p/'t.c').write_text(code.replace('(alignScreen >> 3) & 7, alignScreen & 7','alignScreen & 7, (alignScreen >> 3) & 7') if mutant else code)
  subprocess.run(['cc',str(p/'t.c'),'-o',str(p/'t')],check=True)
  r=subprocess.run([str(p/'t')],capture_output=True)
  assert (r.returncode==0)!=mutant,r.stderr
print('PASS: centered killcam title, corner countdown and fullscreen HUD at three scales; swapped axes fail')
