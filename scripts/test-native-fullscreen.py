#!/usr/bin/env python3
"""Run the browser/native fullscreen state and menu action against startup guards."""
from pathlib import Path
import re
import subprocess
import tempfile
from browser_settings_menu import apply_settings_menu
root=Path(__file__).resolve().parents[1]
source=(root/'downstream/wasm/web_platform.c').read_text()
def function(name):
    match=re.search(r'^[^\n]*\b'+name+r'\([^;]*?\)\s*\{',source,re.M)
    assert match,name
    end,depth=match.end(),1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]+'\n'
code=r'''
#include <assert.h>
#include <string.h>
#define EMSCRIPTEN_KEEPALIVE
#define EM_ASM(...) (++toggles)
static struct {int uiStarted;} cls;
static int mode,pending,failed,writes,toggles;
static void Dvar_SetIntByName(const char *name,int value){assert(!strcmp(name,"ui_browserFullscreen"));mode=value;writes++;}
static void Dvar_SetBoolByName(const char *name,int value){
 if(!strcmp(name,"ui_browserFullscreenPending"))pending=value;
 else {assert(!strcmp(name,"ui_browserFullscreenFailed"));failed=value;}writes++;
}
'''+function('web_client_fullscreen_state')+function('Web_ToggleFullscreen')+r'''
int main(void){
 assert(!web_client_fullscreen_state(1,0,0)&&!writes);Web_ToggleFullscreen();assert(!toggles);
 cls.uiStarted=1;
 for(int m=0;m<=2;m++)for(int p=0;p<=1;p++)for(int f=0;f<=1;f++){
  assert(web_client_fullscreen_state(m,p,f));assert(mode==m&&pending==p&&failed==f);
 }
 Web_ToggleFullscreen();assert(toggles==1);
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-native-fullscreen-') as directory:
    p=Path(directory);(p/'test.c').write_text(code)
    subprocess.run(['cc','-std=c99','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
menu=apply_settings_menu('ui_mp/main.menu',b'{ menuDef { name "main_text" itemDef { text "Original" } } }')
for mode,label in ((1,b'Fullscreen: Off'),(2,b'Fullscreen: On')):
    assert label in menu and f'name browser_fullscreen_{mode}'.encode() in menu
assert menu.count(b'uiScript ToggleFullscreen;')==2
assert b'Fullscreen unavailable. Try again.' in menu
assert b'Original' in menu
assert 'Web_ToggleFullscreen();' in (root/'src/PC/ui_mp/ui_main_mp.c').read_text()
assert '_web_client_fullscreen_state' in (root/'downstream/wasm/CMakeLists.txt').read_text()
print('PASS: native fullscreen flags/startup guards, original-style main-menu choices, browser denial notice and exported menu action')
