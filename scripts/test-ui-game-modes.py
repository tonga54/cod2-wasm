#!/usr/bin/env python3
"""Exercise the native Game Type control and browser creation arguments."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/ui_mp/ui_main_mp.c').read_text()
control=source.split('    case 0xf5:\n',1)[1].split('    case 0xfd:\n',1)[0]
action=source.split('if (I_stricmp(name, "StartServer") == 0) {',1)[1].split('#ifdef __EMSCRIPTEN__',1)[1].split('#endif',1)[0]
key=source.split('static int UI_IsActionKey(int key)\n',1)[1].split('\nstatic int UI_UpdateMapVisibility',1)[0]
code=r'''
#include <assert.h>
#include <string.h>
typedef struct {struct {int integer;} current;} dvar_t;
typedef struct {const char *gameType, *gameTypeName;} gameType_t;
static struct {int numGameTypes,mapCount;gameType_t gameTypes[5];struct {const char *mapLoadName;} mapList[2];} sharedUiInfo;
static dvar_t gt,map;static dvar_t *ui_netGameType=&gt,*ui_currentNetMap=&map;
static char selectedName[32];static char *ui_netGameTypeName=selectedName;
static int visibilityCalls,selectionCalls,creates;static const char *createdMap,*createdType;
static void Dvar_SetInt(dvar_t *var,int value){var->current.integer=value;}
static void Dvar_SetString(char *var,const char *value){strcpy(var,value);}
static const char *Dvar_GetString(const char *name){assert(!strcmp(name,"sv_hostname"));return "Mode test";}
static void UI_UpdateMapVisibility(int value){assert(value==gt.current.integer);visibilityCalls++;}
static void UI_SelectFirstVisibleMap(int value){assert(value==map.current.integer);selectionCalls++;}
static void Web_CreateServer(const char *name,const char *mapName,const char *gameType){assert(!strcmp(name,"Mode test"));createdMap=mapName;createdType=gameType;creates++;}
static int UI_IsActionKey(int key)
'''+key+r'''
static int selectMode(int key){
'''+control+r'''
}
static void createRoom(void){
'''+action+r'''
}
int main(void){
 const char *modes[]={"dm","tdm","ctf","hq","sd"};
 sharedUiInfo.numGameTypes=5;sharedUiInfo.mapCount=2;
 sharedUiInfo.mapList[0].mapLoadName="mp_toujane";sharedUiInfo.mapList[1].mapLoadName="mp_carentan";
 for(int i=0;i<5;i++)sharedUiInfo.gameTypes[i].gameType=modes[i];
 map.current.integer=1;
 for(int i=0;i<5;i++){assert(selectMode(0xd)==1);assert(gt.current.integer==(i+1)%5);assert(!strcmp(selectedName,modes[(i+1)%5]));}
 assert(selectMode(0xc9)==1 && gt.current.integer==4);
 assert(selectMode(0xc8)==1 && gt.current.integer==0);
 assert(selectMode('a')==0 && gt.current.integer==0);
 assert(visibilityCalls==7 && selectionCalls==7 && map.current.integer==1);
 for(int i=0;i<5;i++)for(int m=0;m<2;m++){gt.current.integer=i;map.current.integer=m;createRoom();assert(createdMap==sharedUiInfo.mapList[m].mapLoadName);assert(createdType==modes[i]);}
 assert(creates==10);
 gt.current.integer=5;createRoom();gt.current.integer=-1;createRoom();
 gt.current.integer=1;map.current.integer=2;createRoom();map.current.integer=-1;createRoom();assert(creates==10);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-ui-modes-') as directory:
 p=Path(directory);(p/'test.c').write_text(code)
 subprocess.run(['cc','-std=c99','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: Game Type ownerdraw 245 cycles all five modes in both directions; creation sends selected map/mode; invalid indices do not launch')
