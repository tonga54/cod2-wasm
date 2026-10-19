#!/usr/bin/env python3
"""The player console: disabled in every release, open only in a local
debugging build (COD2_DEV_CONSOLE=1 scripts/build-web.sh), which the static
suite and the Pi release refuse."""
from pathlib import Path
import json
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
console = (root / 'src/PC/client_mp/cl_console_mp.c').read_text()
ui = (root / 'src/PC/client_mp/cl_ui_mp.c').read_text()


def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


support = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {int widthInPixels;float charHeight;int fixedSize;char buffer[256];} field_t;
typedef struct {int keyCatchers,displayHUDWithKeycatchUI;} clientActive_t;
typedef struct {struct {int enabled;} current;} dvar_t;
static struct {int outputVisible;} con;
static clientActive_t clients,*clientPointer=&clients;
static void *imp_cl=&clientPointer;
static int keyStates[0x800/4];static void *keyPointer=keyStates,*imp_keys=&keyPointer;
static field_t consoleField;static void *imp_g_consoleField=&consoleField;
static int g_console_field_width=78;static float g_console_char_height=16;
static dvar_t restricted,*con_restricted=&restricted;
static int clears;
static void Field_Clear(void *field){clears++;memset(field,0,sizeof(field_t));}
'''
body = (function(console, 'Con_ToggleConsole_f') + function(console, 'Con_ToggleConsoleOutput') +
        function(ui, 'Key_SetCatcher'))
checks = r'''
int main(void){
#ifdef COD2_DEV_CONSOLE
 /* The toggle opens and closes it with a fresh field, and its output window;
  * a menu opened over it keeps it. */
 Con_ToggleConsole_f();assert(clients.keyCatchers==1&&clears==1&&consoleField.fixedSize==1&&
  consoleField.widthInPixels==78&&consoleField.charHeight==16);
 Key_SetCatcher(8);assert(clients.keyCatchers==9);
 Con_ToggleConsole_f();assert(clients.keyCatchers==8);
 Con_ToggleConsoleOutput();assert(con.outputVisible==1);Con_ToggleConsoleOutput();assert(!con.outputVisible);
 /* con_restricted opens it only with its key held. */
 restricted.current.enabled=1;clients.keyCatchers=0;Con_ToggleConsole_f();assert(!clients.keyCatchers);
 keyStates[0x780/4]=1;Con_ToggleConsole_f();assert(clients.keyCatchers==1);
 puts("PASS: the debugging build opens the console and its output, keeps it under menus and honours con_restricted");
#else
 /* Bindings, commands and menus never open it, and anything that did is closed. */
 for(int i=0;i<4;i++){Con_ToggleConsole_f();assert(!(clients.keyCatchers&1));}
 clients.keyCatchers=1|8;Con_ToggleConsole_f();assert(clients.keyCatchers==8&&!clears);
 clients.keyCatchers=1;Key_SetCatcher(9);assert(clients.keyCatchers==8);
 con.outputVisible=1;Con_ToggleConsoleOutput();assert(!con.outputVisible);Con_ToggleConsoleOutput();assert(!con.outputVisible);
 puts("PASS: a release never opens the console or its output, nor keeps it under menus");
#endif
 return 0;
}
'''
mutants = [
    ('release', 'ptr->keyCatchers = catcher & ~1;', 'ptr->keyCatchers = catcher;'),
    ('release', '    clients.keyCatchers &= ~1;\n    con.outputVisible = 0;\n#endif',
     '    clients.keyCatchers ^= 1;\n    con.outputVisible = 0;\n#endif'),
    ('release', 'void Con_ToggleConsole_f(void)\n{\n#ifdef COD2_DEV_CONSOLE', 'void Con_ToggleConsole_f(void)\n{\n#ifndef COD2_DEV_CONSOLE'),
    ('release', '#else\n    con.outputVisible = 0;\n#endif', '#else\n    con.outputVisible = !con.outputVisible;\n#endif'),
    ('debug', '    if (ptr->keyCatchers & 1)\n        ptr->keyCatchers = catcher | 1;', '    if (0)\n        ptr->keyCatchers = catcher | 1;'),
]


def run(directory, name, text, debug, passes):
    source = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    source.write_text(support + text + checks)
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-Wall', '-Werror',
                    '-Wno-unused-function', '-Wno-unused-label', '-Wno-unused-variable'] + (['-DCOD2_DEV_CONSOLE=1'] if debug else []) +
                   [str(source), '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert (result.returncode == 0) == passes, (name, result.stdout, result.stderr)
    if passes:
        print(result.stdout.strip())


with tempfile.TemporaryDirectory(prefix='cod2-dev-console-') as directory:
    run(directory, 'release', body, False, True)
    run(directory, 'debug', body, True, True)
    for index, (mode, before, after) in enumerate(mutants):
        assert body.count(before) == 1, before
        run(directory, f'mutant{index}', body.replace(before, after), mode == 'debug', False)

    # build-web.sh: the console is off unless COD2_DEV_CONSOLE=1.
    script = (root / 'scripts/build-web.sh').read_text()
    case = re.search(r'^case "\$\{COD2_DEV_CONSOLE:-0\}" in\n.*?^esac\n', script, re.M | re.S)
    assert case, 'build-web.sh decides the console in one case block'
    assert '-DCOD2_DEV_CONSOLE="${dev_console}"' in script
    for value, expected in ((None, 'OFF'), ('0', 'OFF'), ('1', 'ON'), ('yes', None)):
        env = {key: item for key, item in os.environ.items() if key != 'COD2_DEV_CONSOLE'}
        if value is not None:
            env['COD2_DEV_CONSOLE'] = value
        result = subprocess.run(['bash', '-c', case.group(0) + 'echo "${dev_console}"'], env=env,
                                capture_output=True, text=True)
        if expected is None:
            assert result.returncode == 2 and 'must be 0 or 1' in result.stderr, result
        else:
            assert result.returncode == 0 and result.stdout.strip() == expected, (value, result)
    cmake = (root / 'downstream/wasm/CMakeLists.txt').read_text()
    assert re.search(r'^option\(COD2_DEV_CONSOLE "[^"]+" OFF\)$', cmake, re.M)
    assert cmake.count('COD2_DEV_CONSOLE=1') == 1 and re.search(
        r'^if\(COD2_DEV_CONSOLE\)\n  target_compile_definitions\(cod2_client_objects PRIVATE COD2_DEV_CONSOLE=1\)\nendif\(\)$',
        cmake, re.M)

    # build-info.json says which kind it is.
    site = Path(directory) / 'site'
    site.mkdir()
    (site / 'cod2.wasm').write_bytes(b'\0asm')
    for value, expected in ((None, False), ('1', True), ('0', False)):
        env = {key: item for key, item in os.environ.items() if key != 'COD2_DEV_CONSOLE'}
        if value is not None:
            env['COD2_DEV_CONSOLE'] = value
        subprocess.run(['python3', str(root / 'scripts/write-build-info.py'), str(site)], env=env, check=True,
                       capture_output=True)
        assert json.loads((site / 'build-info.json').read_text())['devConsole'] is expected

# The static suite and the Pi release refuse a console build, and nothing that
# releases asks for one.
assert "assert.equal(build.devConsole, false," in (root / 'scripts/test-static.sh').read_text()
release = (root / 'scripts/build-pi-release.sh').read_text()
assert "build['devConsole'] is False" in release and ' test-dev-console ' in release
for path in [root / 'scripts/build-pi-release.sh', root / 'scripts/build-docker.sh',
             *sorted((root / '.github/workflows').glob('*.yml'))]:
    assert 'COD2_DEV_CONSOLE' not in path.read_text(), path
print('PASS: build-web.sh builds the console only for COD2_DEV_CONSOLE=1, build-info.json records it, and the '
      f'static suite and Pi release refuse it; {len(mutants)} console mutants fail')
