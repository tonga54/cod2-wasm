#!/usr/bin/env python3
"""Replay directional focus against the real text hit-testing and hover code."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/ui_mp/ui_shared_mp.c').read_text()


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
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef int qboolean;typedef unsigned char byte;typedef int snd_alias_list_t;
typedef struct {float x,y,w,h;int horzAlign,vertAlign;} rectDef_t;
typedef struct {rectDef_t rect[1];int dynamicFlags[1],staticFlags;} Window;
typedef struct menuDef_t menuDef_t;
typedef struct itemDef_t {Window window;int type,dvarFlags;rectDef_t textRect[1];
 const char *text,*onFocus,*leaveFocus;menuDef_t *parent;snd_alias_list_t *focusSound;
 int enabled,shown;} itemDef_t;
typedef itemDef_t itemDef_s;
struct menuDef_t {Window window;int cursorItem[1],itemCount;itemDef_t *items[8];};
typedef struct displayContextDef_s {int cursorx,cursory,openMenuCount;menuDef_t *menuStack[8];} displayContextDef_t;
typedef displayContextDef_t displayContextDef_s;
static void *sharedStorage[32];static void *imp_sharedUiInfo=sharedStorage;
static int scale=1,itemCapture,g_waitingForKey,g_editingField;
static int focuses;static itemDef_t *lastFocus;
static void CalcScreenX(float *x,int a){(void)a;*x*=scale;}
static void CalcScreenY(float *y,int a){(void)a;*y*=scale;}
static void CalcScreenPlacement(float *x,float *y,float *w,float *h,int a,int b){
 *x=(*x+(a==2?150:0))*scale;*y=(*y+(b==2?80:0))*scale;*w*=scale;*h*=scale;}
static void Window_AddDynamicFlags(void *w,int f){((Window *)w)->dynamicFlags[0]|=f;}
static void Window_RemoveDynamicFlags(void *w,int f){((Window *)w)->dynamicFlags[0]&=~f;}
static void Menu_SetCursorItem(menuDef_t *m,int c){m->cursorItem[0]=c;}
static int Item_EnableShowViaDvar(itemDef_t *i,int f){return f==1?i->enabled:i->shown;}
static void Item_RunScript(displayContextDef_t *dc,itemDef_t *i,const char *script){
 (void)dc;if(script&&script==i->onFocus){focuses++;lastFocus=i;}}
static void UI_PlayLocalSoundAlias(void *s){(void)s;}
static void Item_MouseLeave(displayContextDef_t *dc,itemDef_t *i){(void)dc;(void)i;}
static void Item_MouseEnter(displayContextDef_t *dc,itemDef_t *i,float x,float y){(void)dc;(void)x;(void)y;i->window.dynamicFlags[0]|=1;}
qboolean Item_SetFocus(displayContextDef_t *,itemDef_t *,float,float);
qboolean Menu_HandleMouseMove(displayContextDef_t *,menuDef_t *,float,float);
'''
body = ''.join(function(s) for s in (
    'qboolean Item_SetFocus(displayContextDef_t *dc, itemDef_t *item, float x, float y)',
    'static qboolean Rect_ContainsPoint(byte *item, float x, float y)',
    'qboolean Menu_HandleMouseMove(displayContextDef_t *dc, menuDef_t *menu, float x, float y)',
    'static itemDef_t *Menu_MoveCursor(displayContextDef_t *dc, menuDef_t *menu, int direction)',
    'itemDef_t *Menu_SetNextCursorItem(displayContextDef_t *dc, menuDef_t *menu)',
    'itemDef_t *Menu_SetPrevCursorItem(displayContextDef_t *dc, menuDef_t *menu)',
))
checks = r'''
static void selected(menuDef_t *m,int index){
 assert(m->cursorItem[0]==index);
 for(int i=0;i<m->itemCount;i++)assert(!!(m->items[i]->window.dynamicFlags[0]&2)==(i==index));
 assert(lastFocus==m->items[index]);
}
int main(void){
 for(scale=1;scale<=3;scale++)for(int align=0;align<=2;align+=2){
  displayContextDef_t dc={.cursorx=630,.cursory=470};
  menuDef_t m={.window.dynamicFlags={6},.cursorItem={-1},.itemCount=6};
  itemDef_t items[6]={0};
  for(int i=0;i<6;i++){
   m.items[i]=&items[i];items[i].parent=&m;items[i].type=0;
   items[i].window.dynamicFlags[0]=4;items[i].text="Team";items[i].onFocus="preview";
   items[i].window.rect[0]=(rectDef_t){50,60+i*30,160,28,align,align};
   items[i].textRect[0]=(rectDef_t){70,80+i*30,70,15,align,align};
   items[i].enabled=items[i].shown=1;
  }
  items[1].window.staticFlags=0x100000; // Decorative label.
  items[2].dvarFlags=1;items[2].enabled=0;
  items[3].dvarFlags=4;items[3].shown=0;
  // A text button is selectable with the mouse nowhere near it. On-focus
  // updates team/weapon previews, and hover must not erase its gold highlight.
  assert(Menu_SetNextCursorItem(&dc,&m)==&items[0]);selected(&m,0);
  assert(Menu_SetNextCursorItem(&dc,&m)==&items[4]);selected(&m,4);
  Menu_HandleMouseMove(&dc,&m,dc.cursorx,dc.cursory);selected(&m,4);
  assert(Menu_SetNextCursorItem(&dc,&m)==&items[5]);selected(&m,5);
  assert(Menu_SetNextCursorItem(&dc,&m)==&items[0]);selected(&m,0);
  assert(Menu_SetPrevCursorItem(&dc,&m)==&items[5]);selected(&m,5);
  assert(Menu_SetPrevCursorItem(&dc,&m)==&items[4]);selected(&m,4);
  // Up from an unopened menu starts at the last row, including a single row.
  for(int i=0;i<6;i++)items[i].window.dynamicFlags[0]&=~2;
  m.cursorItem[0]=-1;assert(Menu_SetPrevCursorItem(&dc,&m)==&items[5]);selected(&m,5);
  items[5].window.dynamicFlags[0]&=~2;m.itemCount=1;m.cursorItem[0]=-1;
  assert(Menu_SetPrevCursorItem(&dc,&m)==&items[0]);selected(&m,0);
  // All-hidden/all-decoration menus terminate without negative indices.
  items[0].window.staticFlags=0x100000;m.cursorItem[0]=-1;
  assert(!Menu_SetNextCursorItem(&dc,&m));assert(!Menu_SetPrevCursorItem(&dc,&m));
  m.itemCount=0;assert(!Menu_SetPrevCursorItem(&dc,&m));
 }
 puts("PASS: directional text/button focus, team previews, wrap, hidden/disabled rows and scaled/aligned coordinates");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-ui-directional-') as directory:
    p = Path(directory)
    (p / 'test.c').write_text(support + body + checks)
    subprocess.run(['cc', '-std=c11', '-fsanitize=address,undefined',
                    str(p / 'test.c'), '-lm', '-o', str(p / 'test')], check=True)
    subprocess.run([str(p / 'test')], check=True)
