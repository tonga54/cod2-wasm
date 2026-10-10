#!/usr/bin/env python3
"""Exercise clicks between sibling menus without releasing UI input capture."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent


def function(path, signature):
    source = (root / path).read_text()
    start = source.index(signature + '\n{')
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


click = function('src/PC/ui_mp/ui_shared_mp.c',
                 'void Menus_HandleOOBClick(displayContextDef_t *dc, menuDef_t *menu, int key, qboolean down)')
pause = function('src/PC/ui_mp/ui_main_mp.c', 'void UI_Pause(qboolean b)')
list_key = function('src/PC/ui_mp/ui_shared_mp.c',
                    'qboolean Item_ListBox_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down, qboolean force)')
code = r'''
#include <assert.h>
#include <stddef.h>
#include <string.h>
typedef int qboolean;
typedef unsigned char byte;
typedef struct {float x,y,w,h;int horzAlign,vertAlign;} rectDef_t;
typedef struct {rectDef_t rect[1];int staticFlags,dynamicFlags[1],style,cinematic,ownerDraw;} windowDef_t;
typedef windowDef_t Window;
typedef struct menuDef_t menuDef_t;
typedef struct listBoxDef_s {int startPos[1],cursorPos[1];float elementWidth,elementHeight;int notselectable;const char *doubleClick;} listBoxDef_t;
typedef listBoxDef_t listBoxDef_s;
typedef struct itemDef_s {windowDef_t window;int type;const char *text;rectDef_t textRect[1];menuDef_t *parent;float special;int cursorPos[1];listBoxDef_t *typeData;} itemDef_t;
typedef itemDef_t itemDef_s;
struct menuDef_t {windowDef_t window;int fullScreen,itemCount;itemDef_t *items[4];const char *onClose;};
typedef struct displayContextDef_s {int cursorx,cursory,openMenuCount,menuCount,realTime;menuDef_t *menuStack[4],*Menus[4];} displayContextDef_t;
typedef displayContextDef_t displayContextDef_s;
static int catcher,cleared,paused,scale,dispatches,closed,stops,selectedMap,lastListBoxClickTime;
static menuDef_t *dispatched;
static int Key_GetCatcher(void) {return catcher;}
static void Key_SetCatcher(int value) {catcher=value;}
static void Key_ClearStates(void) {++cleared;}
static void Dvar_SetIntByName(const char *name,int value) {assert(!strcmp(name,"cl_paused"));paused=value;}
static void CalcScreenX(float *x,int align) {(void)align;*x*=scale;}
static void CalcScreenY(float *y,int align) {(void)align;*y*=scale;}
static void CalcScreenPlacement(float *x,float *y,float *w,float *h,int ha,int va) {
    (void)ha;(void)va;*x*=scale;*y*=scale;*w*=scale;*h*=scale;
}
static void Window_RemoveDynamicFlags(void *w,int flags) {((Window *)w)->dynamicFlags[0]&=~flags;}
static void Window_AddDynamicFlags(void *w,int flags) {((Window *)w)->dynamicFlags[0]|=flags;}
static void Item_RunScript(displayContextDef_t *dc,itemDef_t *item,const char *script) {
    (void)dc;assert(item->parent);assert(script==item->parent->onClose);++closed;
}
static void Display_MouseMove(displayContextDef_t *dc,void *menu,int x,int y) {(void)dc;(void)menu;(void)x;(void)y;}
static void Menu_HandleMouseMove(displayContextDef_t *dc,menuDef_t *menu,float x,float y) {
    (void)dc;(void)x;(void)y;
    if(menu->itemCount) menu->items[0]->window.dynamicFlags[0]|=2;
}
static int UI_FeederCount(float id) {return id==4.0f?2:0;}
static void UI_FeederSelection(float id,int index) {assert(id==4.0f && index>=0 && index<2);selectedMap=index;}
static listBoxDef_t *Item_GetListBoxDef(itemDef_t *item) {return item->typeData;}
static int Item_ListBox_MaxScroll(itemDef_t *item) {(void)item;return 0;}
static void UI_OverrideCursorPos(itemDef_t *item) {(void)item;}
static void ListBox_SetCursorPos(itemDef_t *list,int value) {((listBoxDef_t *)list)->cursorPos[0]=value;}
static void ListBox_SetStartPos(itemDef_t *list,int value) {((listBoxDef_t *)list)->startPos[0]=value;}
static void Item_SetCursorPos(itemDef_t *item,int value) {item->cursorPos[0]=value;}
static int ListBox_HasValidCursorPos(itemDef_t *list) {int index=((listBoxDef_t *)list)->cursorPos[0];return index>=0 && index<2;}
qboolean Item_ListBox_HandleKey(displayContextDef_t *,itemDef_t *,int,qboolean,qboolean);
static void Menu_HandleKey(displayContextDef_t *dc,menuDef_t *menu,int key,qboolean down) {
    assert(key==0xc8 && down);dispatched=menu;++dispatches;
    assert(Item_ListBox_HandleKey(dc,menu->items[0],key,down,0));
}
static void CIN_StopCinematic(int handle) {(void)handle;++stops;}
'''
code += list_key + '\n' + pause + '\n' + click + r'''
int main(void) {
    for(scale=1;scale<=3;scale++) {
        menuDef_t background={0},maps={0},settings={0};
        itemDef_t list={0};displayContextDef_t dc={0};listBoxDef_t choices={0};
        background.window.rect[0]=(rectDef_t){0,0,640,480,0,0};
        maps.window.rect[0]=(rectDef_t){404,137,200,280,0,0};
        settings.window.rect[0]=(rectDef_t){0,2,370,400,0,0};
        list.window.rect[0]=(rectDef_t){404,271,192,130,0,0};
        list.window.dynamicFlags[0]=4;list.type=6;list.parent=&maps;
        list.special=4.0f;list.typeData=&choices;
        choices.elementWidth=119;choices.elementHeight=20;choices.cursorPos[0]=1;
        maps.items[0]=&list;maps.itemCount=1;
        background.window.dynamicFlags[0]=maps.window.dynamicFlags[0]=4;
        settings.window.dynamicFlags[0]=6;
        background.window.style=5;background.window.cinematic=7;
        dc.Menus[0]=dc.menuStack[0]=&background;
        dc.Menus[1]=dc.menuStack[1]=&maps;
        dc.Menus[2]=dc.menuStack[2]=&settings;
        dc.menuCount=dc.openMenuCount=3;
        catcher=9;cleared=closed=stops=dispatches=0;paused=1;
        dc.cursorx=480;dc.cursory=300;
        Menus_HandleOOBClick(&dc,&settings,0xc8,1);
        assert(dispatches==1 && dispatched==&maps);
        assert(selectedMap==1 && list.cursorPos[0]==1);
        assert(maps.window.dynamicFlags[0]==6);
        assert(catcher==9 && cleared==0 && paused==1 && stops==0);
        assert(Item_ListBox_HandleKey(&dc,&list,0x9a,1,0));
        assert(selectedMap==0 && list.cursorPos[0]==0);
        assert(Item_ListBox_HandleKey(&dc,&list,0x9b,1,0));
        assert(selectedMap==1 && list.cursorPos[0]==1);

        /* Clicking empty space must also leave visible menus usable. */
        dc.cursorx=650;dc.cursory=450;
        Menus_HandleOOBClick(&dc,&maps,0xc8,1);
        assert(dispatches==1 && catcher==9 && cleared==0 && stops==0);

        /* Release capture and close cinematics only after every menu hides. */
        background.window.dynamicFlags[0]=maps.window.dynamicFlags[0]=settings.window.dynamicFlags[0]=0;
        Menus_HandleOOBClick(&dc,&settings,0xc8,1);
        assert(catcher==1 && cleared==1 && paused==0 && stops==1);
        assert(background.window.cinematic==-1);

        /* A popup configured to close on an outside click still closes. */
        settings.window.staticFlags=0x2000000;
        settings.window.dynamicFlags[0]=6;settings.onClose="close";
        catcher=9;
        Menus_HandleOOBClick(&dc,&settings,0xc8,1);
        assert(closed==1 && settings.window.dynamicFlags[0]==0 && catcher==1);
    }
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='cod2-ui-clicks-') as directory:
    path = Path(directory)
    for mutant, variant in enumerate((code,
            code.replace('if (visCount == 0) {', 'if (visCount > 0) {'),
            code.replace('UI_FeederCount(((itemDef_t *)it)->special)',
                         'UI_FeederCount((*(int *)&((itemDef_t *)it)->special))'))):
        (path / 'test.c').write_text(variant)
        subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                        '-fsanitize=address,undefined', str(path / 'test.c'),
                        '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True)
        assert (result.returncode == 0) == (mutant == 0), result.stderr.decode()
print('PASS: both map selections, sibling clicks, empty space and popup closure at three scales; capture-loss and feeder-type regressions rejected')
