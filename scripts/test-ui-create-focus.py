#!/usr/bin/env python3
"""Replay real menu dispatch/editing across the creation form's sibling menus."""
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
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int qboolean; typedef unsigned char byte;
typedef float vec4_t[4]; typedef void *FontHandle;
typedef struct {float x,y,w,h;int horzAlign,vertAlign;} rectDef_t;
typedef struct {rectDef_t rect[1];int staticFlags,dynamicFlags[1],style,cinematic,ownerDraw;const char *name;} windowDef_t;
typedef windowDef_t Window;
typedef struct menuDef_t menuDef_t;
typedef struct {int startPos[1],cursorPos[1];float elementWidth,elementHeight;int notselectable;const char *doubleClick;} listBoxDef_t;
typedef listBoxDef_t listBoxDef_s;
typedef struct {int maxChars,maxCharsGotoNext,paintOffset,maxPaintChars;} editFieldDef_t;
typedef struct itemDef_s {windowDef_t window;int type;const char *text,*dvar,*action,*onAccept;
 rectDef_t textRect[1];menuDef_t *parent;float special;int cursorPos[1];
 int fontEnum,textStyle;float textscale;
 union {listBoxDef_t *listBox;editFieldDef_t *editField;} typeData;} itemDef_t;
typedef itemDef_t itemDef_s;
struct menuDef_t {windowDef_t window;int fullScreen,itemCount;itemDef_t *items[4];const char *onClose,*onESC;};
typedef struct displayContextDef_s {int cursorx,cursory,openMenuCount,menuCount,realTime;menuDef_t *menuStack[4],*Menus[4];} displayContextDef_t;
typedef displayContextDef_t displayContextDef_s;
static int g_editingField,g_waitingForKey,inHandleKey,debugMode,lastListBoxClickTime;
static itemDef_t *g_editItem,*g_bindItem;
static int scale,overstrike,selectedMap,selectedMode,creates;
static int createdMap,createdMode;static char hostname[1024],createdName[1024];
static char paintedText[1024];static int paintedCursor,paintedMax;
static void CalcScreenX(float *x,int a){(void)a;*x*=scale;}
static void CalcScreenY(float *y,int a){(void)a;*y*=scale;}
static void CalcScreenPlacement(float *x,float *y,float *w,float *h,int a,int b){
 (void)a;(void)b;*x*=scale;*y*=scale;*w*=scale;*h*=scale;}
static void Window_RemoveDynamicFlags(void *p,int f){((Window *)p)->dynamicFlags[0]&=~f;}
static void Window_AddDynamicFlags(void *p,int f){((Window *)p)->dynamicFlags[0]|=f;}
static int contains(rectDef_t *r,int x,int y){return x>=r->x&&x<=r->x+r->w&&y>=r->y&&y<=r->y+r->h;}
static qboolean Menu_HandleMouseMove(displayContextDef_t *dc,menuDef_t *m,float x,float y){
 if(g_editingField)return 0;
 itemDef_t *target=NULL;
 for(int i=0;i<m->itemCount;i++){itemDef_t *it=m->items[i];
  if((it->window.dynamicFlags[0]&4)&&contains(it->window.rect,x,y))target=it;}
 if(!target)return 0;
 for(int i=0;i<m->itemCount;i++)Window_RemoveDynamicFlags(&m->items[i]->window,2);
 Window_AddDynamicFlags(&target->window,2);
 if(target->type==6){listBoxDef_t *lb=target->typeData.listBox;
  lb->cursorPos[0]=(int)((y-target->window.rect[0].y)/lb->elementHeight);}
 (void)dc;return 1;
}
static qboolean Display_MouseMove(displayContextDef_t *dc,void *p,int x,int y){
 assert(!p);for(int i=dc->openMenuCount-1;i>=0;i--)
  if(Menu_HandleMouseMove(dc,dc->menuStack[i],x,y))return 1;return 1;
}
static int UI_FeederCount(float id){assert(id==4.0f);return 2;}
static void UI_FeederSelection(float id,int index){assert(id==4.0f&&index>=0&&index<2);selectedMap=index;}
static listBoxDef_t *Item_GetListBoxDef(itemDef_t *i){return i->typeData.listBox;}
static editFieldDef_t *Item_GetEditFieldDef(itemDef_t *i){return i->typeData.editField;}
static int Item_ListBox_MaxScroll(itemDef_t *i){(void)i;return 0;}
static void UI_OverrideCursorPos(itemDef_t *i){(void)i;}
static void ListBox_SetCursorPos(itemDef_t *p,int v){((listBoxDef_t *)p)->cursorPos[0]=v;}
static void ListBox_SetStartPos(itemDef_t *p,int v){((listBoxDef_t *)p)->startPos[0]=v;}
static void Item_SetCursorPos(itemDef_t *i,int v){i->cursorPos[0]=v;}
static int ListBox_HasValidCursorPos(itemDef_t *p){int v=((listBoxDef_t *)p)->cursorPos[0];return v>=0&&v<2;}
static void Item_RunScript(displayContextDef_t *dc,itemDef_t *i,const char *s){
 (void)dc;(void)i;assert(!strcmp(s,"create"));creates++;createdMap=selectedMap;
 createdMode=selectedMode;strcpy(createdName,hostname);}
static void I_strncpyz(char *d,const char *s,int n){snprintf(d,n,"%s",s);}
static const char *Dvar_GetVariantString(const char *name){assert(!strcmp(name,"sv_hostname"));return hostname;}
static void Dvar_SetFromStringByName(const char *name,const char *v){assert(!strcmp(name,"sv_hostname"));strcpy(hostname,v);}
static int Item_GetCursorPosOffset(itemDef_t *i,const char *text,int offset){
 int n=i->cursorPos[0]+offset,len=strlen(text);return n<0?0:(n>len?len:n);}
static int Key_GetOverstrikeMode(void){return overstrike;}
static void Key_SetOverstrikeMode(int v){overstrike=v;}
static int I_isforfilename(int k){return isalnum(k);}
static int I_isdigit(int k){return isdigit(k);}
static char Com_GetDecimalDelimiter(void){return '.';}
int ___toupper(int k){return toupper(k);}
static itemDef_t *Menu_SetNextCursorItem(displayContextDef_t *d,menuDef_t *m){(void)d;(void)m;return NULL;}
static itemDef_t *Menu_SetPrevCursorItem(displayContextDef_t *d,menuDef_t *m){(void)d;(void)m;return NULL;}
static qboolean Item_Bind_HandleKey(displayContextDef_t *d,itemDef_t *i,int k,qboolean b){(void)d;(void)i;(void)k;(void)b;return 0;}
static int Menu_CheckOnKey(displayContextDef_t *d,menuDef_t *m,int k){(void)d;(void)m;(void)k;return 0;}
static int Dvar_GetInt(const char *s){(void)s;return 0;}
static void Cbuf_ExecuteText(int w,const char *s){(void)w;(void)s;assert(0);}
static void UI_Pause(qboolean b){(void)b;assert(0);}
static void CIN_StopCinematic(int h){(void)h;assert(0);}
static const char *UI_TraceScriptItemParentName(itemDef_t *i){(void)i;return NULL;}
static void Com_Printf(const char *s,...){(void)s;assert(0);}
static void Item_Text_Paint(displayContextDef_t *d,itemDef_t *i){(void)d;(void)i;}
static void Item_SetTextExtents(itemDef_t *i,int *w,int *h,const char *s){(void)i;(void)s;*w=*h=1;}
static void Item_TextColor(displayContextDef_t *d,itemDef_t *i,vec4_t *c){(void)d;(void)i;for(int n=0;n<4;n++)(*c)[n]=1;}
static FontHandle UI_GetFontHandle(int f,float scale){(void)f;(void)scale;return (FontHandle)1;}
static void UI_DrawText(const char *s,int max,FontHandle f,float x,float y,int ha,int va,float scale,const float *c,int style){
 (void)f;(void)x;(void)y;(void)ha;(void)va;(void)scale;(void)c;(void)style;
 strcpy(paintedText,s);paintedMax=max;paintedCursor=-1;}
static void UI_DrawTextWithCursor(const char *s,int max,FontHandle f,float x,float y,int ha,int va,float scale,const float *c,int style,int cursor,int ch){
 UI_DrawText(s,max,f,x,y,ha,va,scale,c,style);assert(ch=='_'||ch=='|');paintedCursor=cursor;}
qboolean Item_ListBox_HandleKey(displayContextDef_t *,itemDef_t *,int,qboolean,qboolean);
void Menu_HandleKey(displayContextDef_t *,menuDef_t *,int,qboolean);
void Menus_HandleOOBClick(displayContextDef_t *,menuDef_t *,int,qboolean);
static qboolean Item_HandleKey(displayContextDef_t *d,itemDef_t *i,int k,qboolean down){
 if(!down)return 0;
 if(i->type==6)return Item_ListBox_HandleKey(d,i,k,down,0);
 if(i->type==8&&(k==0xc8||k==0xc9||k==13)){selectedMode=(selectedMode+(k==0xc9?4:1))%5;return 1;}
 return 0;
}
'''
body = ''.join(function(signature) for signature in (
    'void Item_TextField_Paint(displayContextDef_t *dc, itemDef_t *item)',
    'qboolean Item_ListBox_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down, qboolean force)',
    'qboolean Item_TextField_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key)',
    'void Menu_HandleKey(displayContextDef_t *dc, menuDef_t *menu, int key, qboolean down)',
    'void Menus_HandleOOBClick(displayContextDef_t *dc, menuDef_t *menu, int key, qboolean down)',
))
checks = r'''
static menuDef_t *focused(displayContextDef_t *dc){
 for(int i=dc->openMenuCount-1;i>=0;i--)if((dc->menuStack[i]->window.dynamicFlags[0]&6)==6)return dc->menuStack[i];
 assert(0);return NULL;
}
static void click(displayContextDef_t *dc,int x,int y){dc->cursorx=x;dc->cursory=y;
 Display_MouseMove(dc,NULL,x,y);Menu_HandleKey(dc,focused(dc),0xc8,1);
 Menu_HandleKey(dc,focused(dc),0xc8,0);}
static void type(displayContextDef_t *dc,const char *text){
 Menu_HandleKey(dc,focused(dc),0xa5,1);
 int oldLength=strlen(hostname);for(int i=0;i<oldLength;i++)Menu_HandleKey(dc,focused(dc),0xa2,1);
 for(;*text;text++)Menu_HandleKey(dc,focused(dc),*text|0x400,1);}
int main(void){int cases=0;
 for(scale=1;scale<=3;scale++)for(int firstMap=0;firstMap<2;firstMap++){
  menuDef_t background={0},maps={0},settings={0};itemDef_t map={0},mode={0},name={0},start={0};
  listBoxDef_t lb={.elementWidth=192,.elementHeight=20};editFieldDef_t edit={.maxChars=64,.maxPaintChars=15};
  displayContextDef_t dc={.openMenuCount=3,.menuCount=3};
  background.window.rect[0]=(rectDef_t){0,0,640,480};background.window.dynamicFlags[0]=4;
  maps.window.rect[0]=(rectDef_t){404,137,200,320};maps.window.dynamicFlags[0]=4;
  settings.window.rect[0]=(rectDef_t){0,2,370,400};settings.window.dynamicFlags[0]=6;
  dc.Menus[0]=dc.menuStack[0]=&background;dc.Menus[1]=dc.menuStack[1]=&maps;dc.Menus[2]=dc.menuStack[2]=&settings;
  map.window.rect[0]=(rectDef_t){404,271,192,130};map.window.dynamicFlags[0]=4;
  map.type=6;map.parent=&maps;map.special=4;map.typeData.listBox=&lb;map.cursorPos[0]=-1;
  start.window.rect[0]=(rectDef_t){500,420,100,24};start.window.dynamicFlags[0]=4;
  start.type=0;start.text="Start";start.textRect[0]=(rectDef_t){520,440,40,20};start.parent=&maps;start.action="create";
  maps.items[0]=&map;maps.items[1]=&start;maps.itemCount=2;
  name.window.rect[0]=(rectDef_t){20,140,320,20};name.window.dynamicFlags[0]=4;
  name.type=4;name.text="Server Name:";name.textRect[0]=(rectDef_t){20,156,80,16};
  name.parent=&settings;name.dvar="sv_hostname";name.typeData.editField=&edit;
  mode.window.rect[0]=(rectDef_t){20,115,320,20};mode.window.dynamicFlags[0]=4;
  mode.type=8;mode.text="Game Type:";mode.textRect[0]=(rectDef_t){20,131,80,16};mode.parent=&settings;
  settings.items[0]=&name;settings.items[1]=&mode;settings.itemCount=2;
  selectedMode=creates=g_editingField=0;g_editItem=NULL;strcpy(hostname,"CoD2Host");
  for(int iteration=0;iteration<10;iteration++){
   int target=(firstMap+iteration)%2;
   click(&dc,480,280+target*20);assert(selectedMap==target&&!g_editingField);
   /* Click each control's value area, well outside its painted label. */
   click(&dc,280,148);assert(g_editingField&&g_editItem==&name);
   type(&dc,"Custom Room QA");assert(!strcmp(hostname,"Custom Room QA"));
   Item_TextField_Paint(&dc,&name);assert(paintedCursor==14&&paintedMax==15);
   int previousMode=selectedMode;click(&dc,280,124);
   assert(!g_editingField&&!g_editItem&&selectedMode==(previousMode+1)%5);
   /* A map click commits the name and selects immediately, without Enter. */
   click(&dc,280,148);type(&dc,"Renamed Room QA");
   click(&dc,480,280+(1-target)*20);assert(selectedMap==1-target&&!g_editingField);
   click(&dc,280,148);assert(g_editingField);type(&dc,"Final Room QA");
   Menu_HandleKey(&dc,focused(&dc),13,1);assert(!g_editingField);
   previousMode=selectedMode;click(&dc,280,124);assert(selectedMode==(previousMode+1)%5);
   /* Text buttons retain their painted-text hit area. */
   int before=creates;click(&dc,502,425);assert(creates==before);
   click(&dc,540,430);assert(creates==before+1&&createdMap==1-target&&createdMode==selectedMode);
   assert(!strcmp(createdName,"Final Room QA"));cases++;
  }
  /* Long names scroll while editing, then show their beginning without a cursor. */
  click(&dc,280,148);type(&dc,"A long custom server name");assert(edit.paintOffset>0);
  Item_TextField_Paint(&dc,&name);assert(paintedCursor==15&&!strcmp(paintedText,hostname+edit.paintOffset));
  Menu_HandleKey(&dc,focused(&dc),13,1);
  Item_TextField_Paint(&dc,&name);assert(paintedCursor==-1&&!strcmp(paintedText,hostname));
  /* A second visible field must not inherit another field's cursor or scroll. */
  g_editingField=1;g_editItem=&mode;Item_TextField_Paint(&dc,&name);
  assert(paintedCursor==-1&&!strcmp(paintedText,hostname));g_editingField=0;g_editItem=NULL;
 }
 printf("PASS: %d map/name/mode/start sequences at three scales; single-click field handoff, name persistence and text-button bounds\n",cases);
}
'''
variants = (body,
            body.replace('if (itemType == 0 && ((itemDef_t *)item)->text)',
                         'if (itemType != 0 && ((itemDef_t *)item)->text)'),
            body.replace('Display_MouseMove(dc, NULL, dc->cursorx, dc->cursory);',
                         'Display_MouseMove(dc, NULL, dc->cursorx, dc->cursory); return;'),
            body.replace('paintOffset = editing ? editPtr->paintOffset : 0;',
                         'paintOffset = editPtr->paintOffset;'),
            body.replace('if (editing) {',
                         'if (g_editingField && (item->window.staticFlags & 6) == 6) {'))
assert len(set(variants)) == 5
with tempfile.TemporaryDirectory(prefix='cod2-create-focus-') as directory:
    path = Path(directory)
    for mutant, variant in enumerate(variants):
        (path / 'test.c').write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (mutant == 0), result.stderr
        if mutant == 0:
            print(result.stdout.strip())
print('PASS: active-field cursor and long-name blur painting; label-only, swallowed-click, stale-scroll and missing-cursor mutations rejected')
