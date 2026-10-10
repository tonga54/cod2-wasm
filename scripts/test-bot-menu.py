#!/usr/bin/env python3
"""Exercise the production browser menu filter with real bot controls."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/ui_mp/ui_shared_mp.c').read_text()
a=source.index('static void UI_ConfigureBrowserMenu(')
b=source.index('\n#endif',a)
code=r'''
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#define I_stricmp strcasecmp
typedef struct {const char *name;int ownerDraw;} window_t;
typedef struct {window_t window;const char *text,*dvar;int type;void *action;} itemDef_t;
typedef struct {window_t window;int itemCount;itemDef_t *items[12];} menuDef_t;
'''+source[a:b]+r'''
int main(void){
 itemDef_t entries[]={
 {{0,245}," ",0,12,0},{{0,0}," ","sv_hostname",4,0},
 {{0,0},"Bots",0,0,0},{{0,0}," ","ui_botCount",12,0},
 {{0,0},"Bot Difficulty",0,0,0},{{0,0}," ","ui_botDifficulty",12,0},
 {{0,0},"@MENU_MAXIMUM_PLAYERS",0,0,0},{{0,0}," ","sv_maxclients",4,(void*)1},
 {{0,0},"@MENU_SERVER_SETTINGS",0,0,0},{{0,0}," ","sv_pure",11,0}};
 menuDef_t menu={{"createserver_serversettings",0},10,{0}};
 for(int i=0;i<10;i++)menu.items[i]=&entries[i];
 UI_ConfigureBrowserMenu(&menu);assert(menu.itemCount==9);
 for(int i=0;i<9;i++)assert(menu.items[i]==&entries[i]);
 assert(entries[3].type==12&&entries[5].type==12);
 assert(entries[7].type==0&&entries[7].dvar==NULL&&entries[7].action==NULL&&!strcmp(entries[7].text,"64"));
 UI_ConfigureBrowserMenu(&menu);assert(menu.itemCount==9);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-bot-menu-') as directory:
 p=Path(directory);(p/'test.c').write_text(code)
 subprocess.run(['cc','-std=c99','-Wall','-Wextra',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: bot count/difficulty labels and native multi controls survive browser filtering, fixed slots and repeated configuration')
