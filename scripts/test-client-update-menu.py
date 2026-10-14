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
static int notice, writes, reloads;
static void Dvar_SetBoolByName(const char *name,int value){assert(!strcmp(name,"ui_clientUpdateAvailable"));notice=value;writes++;}
static int Dvar_GetBool(const char *name){assert(!strcmp(name,"ui_clientUpdateAvailable"));return notice;}
''' + ''.join(function(n) for n in ['web_client_connected', 'web_client_update_available', 'Web_ReloadClient']) + r'''
int main(void){
 web_client_update_available(1);assert(!writes);Web_ReloadClient();assert(!reloads);
 cls.uiStarted=1;
 for(int state=0;state<=10;state++)for(int available=0;available<=1;available++){
  clientConnections[0].state=state;web_client_update_available(available);
  assert(notice==(available&&state<CA_CONNECTING));
  int before=reloads;Web_ReloadClient();assert(reloads==before+(available&&state<CA_CONNECTING));
  // A connection can start after the button was painted.
  notice=1;clientConnections[0].state=CA_CONNECTING;before=reloads;
  Web_ReloadClient();assert(reloads==before);
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
print('PASS: native client update/reload guards, stale connected clicks, native main-menu message/action and unrelated menus')
