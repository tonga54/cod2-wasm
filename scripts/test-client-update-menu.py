#!/usr/bin/env python3
"""Execute the native notification/reload guards and private menu adaptation."""
from pathlib import Path
import re
import subprocess
import tempfile
from browser_settings_menu import apply_settings_menu

root = Path(__file__).resolve().parents[1]
source = (root / 'downstream/wasm/web_platform.c').read_text()

def function(name):
    match = re.search(r'^[^\n]*\b' + name + r'\([^;]*?\)\s*\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

code = r'''
#include <assert.h>
#include <string.h>
#define EMSCRIPTEN_KEEPALIVE
#define CA_CONNECTING 3
#define EM_ASM(...) (++reloads)
static struct {int uiStarted;} cls;
static struct {int state;} clientConnections[1];
static int notice, writes, reloads, menuFree;
static void Dvar_SetBoolByName(const char *name,int value){assert(!strcmp(name,"ui_clientUpdateAvailable"));notice=value;writes++;}
static int Dvar_GetBool(const char *name){assert(!strcmp(name,"ui_clientUpdateAvailable"));return notice;}
static int UI_WWIIMenu_RightPromptFree(void){return menuFree;}
''' + ''.join(function(n) for n in ['web_client_connected', 'web_client_update_available', 'Web_ReloadClient',
                                    'web_right_prompt_free']) + r'''
int main(void){
 menuFree=1;assert(!web_right_prompt_free());
 web_client_update_available(1);assert(!writes);Web_ReloadClient();assert(!reloads);
 cls.uiStarted=1;
 for(int state=0;state<=10;state++)for(int available=0;available<=1;available++){
  clientConnections[0].state=state;web_client_update_available(available);
  assert(notice==(available&&state<CA_CONNECTING));
  int before=reloads;Web_ReloadClient();assert(reloads==before+(available&&state<CA_CONNECTING));
  // A connection can start after the button was painted.
  notice=1;clientConnections[0].state=CA_CONNECTING;before=reloads;
  Web_ReloadClient();assert(reloads==before);
  // The install offer's place: the main menu's own answer, never while connecting or connected.
  for(menuFree=0;menuFree<=1;menuFree++){
   clientConnections[0].state=state;assert(web_right_prompt_free()==(menuFree&&state<CA_CONNECTING));
  }
 }
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-update-menu-') as directory:
    p = Path(directory)
    (p / 'test.c').write_text(code)
    subprocess.run(['cc', '-std=c99', '-fsanitize=address,undefined', str(p / 'test.c'), '-o', str(p / 'test')], check=True)
    subprocess.run([str(p / 'test')], check=True)
menu = b'''{ menuDef { name "main_text" itemDef { text "Original item" } } }'''
patched = apply_settings_menu('ui_mp/main.menu', menu)
assert b'Original item' in patched
assert b'name browser_client_update_notice' in patched and b'name browser_client_update_reload' in patched
assert b'uiScript ReloadClient;' in patched
assert b'text "New client version available"' in patched
assert apply_settings_menu('ui_mp/joinserver.menu', menu) == menu
assert 'Web_ReloadClient();' in (root / 'src/PC/ui_mp/ui_main_mp.c').read_text()
assert '_web_right_prompt_free' in (root / 'downstream/wasm/CMakeLists.txt').read_text()
print('PASS: native client update/reload guards, stale connected clicks, native main-menu message/action and unrelated '
      'menus, and the install offer\'s place only in the main menu while disconnected')
