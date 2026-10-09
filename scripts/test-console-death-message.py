#!/usr/bin/env python3
"""Decode native death messages into text and weapon icons, under sanitizers."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parent.parent
s=(root/'src/PC/gfx_d3d/r_font.c').read_text()
def function(name):
 a=s.index('    const short int *'+name+'(')
 b=s.index('{',a);e=b+1;depth=1
 while depth:
  depth+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[a:e]
code=r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef float vec_t;typedef int Bool;typedef void *MaterialHandle;
static unsigned char ColorIndex(int c){return c-'0';}
static const char *CL_GetHudMsgIconMaterialName(int n){assert(n==3);return "headshot";}
static MaterialHandle Material_RegisterHandle(const char*s,int a,int b){assert(!strcmp(s,"headshot"));return (void*)1;}
'''+function('R_GetConsoleString')+'\n'+function('R_GetConsoleIcon')+r'''
int main(void){
 for(int flip=0;flip<2;flip++){
  short line[]={0x0aff,0x0b80,0x0c40,0x0747,0x0761,0x0773,0x0720,0x0dff,0x0eff,0x0fff,0x1020,0x1120,0x1203,0x0720,0x0a40,0x0b80,0x0cff,0x0752,0x0769,0x0776,0x0761,0x076c,0x0720};
  if(flip)line[10]=0x1320;
  int remaining=23;char text[1024];float color[4]={1,1,1,1},w,h;Bool icon,flipped;MaterialHandle material;
  const short *p=R_GetConsoleString(line,&remaining,text,color,&icon);
  assert(p==line+7 && icon && !strcmp(text,"Gas "));assert(color[0]==1 && color[1]>.49 && color[1]<.51);
  p=R_GetConsoleIcon(p,&remaining,&w,&h,&material,color,&flipped);
  assert(p==line+13 && remaining==10 && material==(void*)1 && w==1 && h==1 && flipped==flip);
  assert(color[0]==1 && color[1]==1 && color[2]==1);
  assert(!R_GetConsoleString(p,&remaining,text,color,&icon) && !icon && !strcmp(text," Rival"));
  assert(color[2]==1 && color[0]>.24 && color[0]<.26);
 }
 puts("PASS: attacker, headshot icon, victim, RGB colors, horizontal flip and bounded decoding");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-death-text-') as d:
 p=Path(d)
 for mutant in (False,True):
  (p/'test.c').write_text(code.replace('switch (entry >> 8)','switch (value)') if mutant else code)
  subprocess.run(['cc','-O1','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
  r=subprocess.run([str(p/'test')],capture_output=True,text=True)
  assert (r.returncode==0)!=mutant,r.stderr
  if not mutant:print(r.stdout,end='')
