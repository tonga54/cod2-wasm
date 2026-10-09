#!/usr/bin/env python3
"""Hidden scoreboards must not suppress the HUD; intermission always shows it."""
from pathlib import Path
import subprocess,tempfile
s=(Path(__file__).resolve().parent.parent/'src/PC/cgame_mp/cg_scoreboard_mp.c').read_text()
a=s.index('qboolean CG_ScoreboardDisplayed(void)\n{');b=s.index('{',a);e=b+1;depth=1
while depth: depth+=(s[e]=='{')-(s[e]=='}');e+=1
visibility=s[a:e]
a=s.index('    if (cg_paused->current.integer',s.index('qboolean CG_DrawScoreboard('))
b=s.index('\n            fade = fadePtr[0];',a)
guard=s[a:b]+'\n return 2;} return 2;}}\n'
code=r'''
#include <assert.h>
#include <stddef.h>
typedef int qboolean;
static struct{struct{int pm_type;}ps;}snapshot;
static struct{int showScores,scoreFadeTime;char killerName[32];typeof(snapshot)*nextSnap;}state,*cg=&state;
static struct{struct{int integer;}current;}paused,*cg_paused=&paused;
static float *CG_FadeColor(int a,int b,int c){static float color[4]={1,1,1,1};return a?color:NULL;}
'''+visibility+'\nstatic int guard(void){float *fadePtr;'+guard+r'''
int main(void){cg->nextSnap=&snapshot;
 for(int paused=0;paused<2;paused++)for(int intermission=0;intermission<2;intermission++)for(int tab=0;tab<2;tab++)for(int fading=0;fading<2;fading++){
  cg_paused->current.integer=paused;snapshot.ps.pm_type=intermission?5:0;cg->showScores=tab;cg->scoreFadeTime=fading;
  assert(CG_ScoreboardDisplayed()==(intermission||tab));
  int expected=(paused&&!intermission)||(!intermission&&!tab&&!fading)?0:2;
  assert(guard()==expected);
 }return 0;}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)
 for mutant in (False,True):
  (p/'t.c').write_text(code.replace('cg->killerName[0] = 0;\n                return 0;','cg->killerName[0] = 0;\n                return 2;') if mutant else code)
  subprocess.run(['cc','-std=gnu99',str(p/'t.c'),'-o',str(p/'t')],check=True)
  r=subprocess.run([str(p/'t')],capture_output=True)
  assert (r.returncode==0)!=mutant,r.stderr
print('PASS: 16 paused/intermission/TAB/fade states; invisible-scoreboard HUD suppression regression rejected')
