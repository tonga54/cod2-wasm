#!/usr/bin/env python3
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'downstream/wasm/web_host.c').read_text()
source = source.replace('#include "common_types.h"\n', '').replace('#include <emscripten.h>\n', '')
source, count = re.subn(r'EM_JS\(void, WebP2P_StartHost, \(void\), \{.*?\n\}\);\n', '', source, flags=re.S)
assert count == 1
code = r'''
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define EMSCRIPTEN_KEEPALIVE
typedef struct {struct {int enabled;} current;} dvar_t;
const dvar_t *com_sv_running;
const unsigned char web_browser_bots[] = "bots script";
const unsigned int web_browser_bots_size = 11;
static char root[512], command[1024];
static int hosts, commands, thirdPerson = 1, failWrites, operators;
static char *rooted(const char *path) {
    static char full[1024];
    snprintf(full, sizeof full, "%s%s", root, path);
    return full;
}
static int test_mkdir(const char *path, mode_t mode) { return mkdir(rooted(path), mode); }
static FILE *test_fopen(const char *path, const char *mode) { return failWrites ? NULL : fopen(rooted(path), mode); }
static void WebP2P_StartHost(void) { ++hosts; }
void Cbuf_ExecuteText(int when, const char *text) { assert(when == 2); snprintf(command, sizeof command, "%s", text); ++commands; }
void Dvar_SetBoolByName(const char *name, unsigned char value) { assert(!strcmp(name, "cg_thirdPerson")); thirdPerson = value; }
void SV_AddOperatorCommands(void) { ++operators; }
#define mkdir test_mkdir
#define fopen test_fopen
''' + source + r'''
static int contains(const char *text) { return strstr(command, text) != NULL; }
int main(int argc, char **argv) {
    char bots[64] = "";
    FILE *file;
    assert(argc == 2);
    snprintf(root, sizeof root, "%s", argv[1]);
    assert(!mkdir("/profile", 0755));

    Web_StartLocalServer("mp_carentan", "tdm", 0, 1);
    assert(commands == 1 && hosts == 1 && operators == 1 && thirdPerson == 0);
    assert(contains("set g_gametype tdm;"));
    assert(contains("set sv_mapRotation \"gametype tdm map mp_toujane map mp_carentan map mp_dawnville\";"));
    assert(contains("set sv_mapRotationCurrent \"gametype tdm map mp_dawnville\";"));
    assert(contains("set scr_bot_count 0; set scr_bot_difficulty 1; wait; wait; map mp_carentan\n"));
    puts(command);

    Web_StartLocalServer("mp_dawnville", "ctf", 0, 0);
    assert(contains("set sv_mapRotationCurrent \"\";") && contains("map mp_dawnville\n"));
    Web_StartLocalServer("mp_toujane", "sd", 8, 2);
    assert(contains("set sv_mapRotationCurrent \"gametype sd map mp_carentan map mp_dawnville\";"));
    assert(contains("set scr_bot_count 8; set scr_bot_difficulty 2;"));
    file = fopen("/profile/raw/maps/mp/browser_bots.gsc", "rb");
    assert(file && fread(bots, 1, sizeof bots, file) == 11 && !strcmp(bots, "bots script"));
    fclose(file);

    Web_StartLocalServer("mp_toujane", "hq", 32, 2);
    assert(contains("set scr_bot_count 32; set scr_bot_difficulty 2;"));
    Web_StartLocalServer("mp_toujane", "hq", 33, 5);
    assert(contains("set scr_bot_count 0; set scr_bot_difficulty 1;"));
    failWrites = 1;
    Web_StartLocalServer("mp_toujane", "dm", 4, 0);
    assert(contains("set scr_bot_count 0; set scr_bot_difficulty 0;"));
    failWrites = 0;
    assert(commands == 6 && hosts == 6 && operators == 6);

    Web_StartLocalServer("mp_unknown", "tdm", 0, 1);
    Web_StartLocalServer("mp_toujane", "ftag", 0, 1);
    assert(commands == 6 && hosts == 6 && operators == 6);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-local-host-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(code)
    (path / 'fs').mkdir()
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wno-unused-function', '-fsanitize=address,undefined',
                    str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    first = subprocess.run([str(path / 'test'), str(path / 'fs')], check=True, capture_output=True, text=True).stdout
for line in (root / 'server.cfg').read_text().splitlines():
    name, value = re.fullmatch(r'set (\S+) "(.*)"', line).groups()
    if name in ('sv_hostname', 'sv_mapRotation', 'g_gametype'):
        continue
    assert f'set {name} {value};' in first or f'set {name} "{value}";' in first, name
print('PASS: Local host restores server commands and starts the listen server with dedicated-room settings, selected rotation and validated bots; invalid choices do not launch')
