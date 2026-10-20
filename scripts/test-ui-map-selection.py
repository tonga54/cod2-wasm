#!/usr/bin/env python3
"""Exercise actual native UI creation/vote branches for all five map indices."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/ui_mp/ui_main_mp.c').read_text()


def branch(name):
    start = source.index(f'    if (I_stricmp(name, "{name}") == 0) {{')
    position = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[position] == '{') - (source[position] == '}')
        position += 1
    return source[start:position]


create = branch('StartServer').split('#ifdef __EMSCRIPTEN__\n', 1)[1].split('#endif', 1)[0]
harness = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
typedef struct {const char *mapName, *mapLoadName, *imageName, *opponentName;
    int teamMembers, typeBits, cinematic, timeToBeat[32], levelShot, active;} mapInfo;
static struct {int mapCount, numGameTypes; mapInfo mapList[128];
    struct {const char *gameType;} gameTypes[1];} sharedUiInfo;
static struct {struct {int integer;} current;} selectedMap, selectedType;
#define ui_currentNetMap (&selectedMap)
#define ui_netGameType (&selectedType)
static char output[256];
static int calls;
static int I_stricmp(const char *a, const char *b) {return strcmp(a,b);}
static void UI_MatchMenu_RoomName(char *name, int size) {assert(size==64); snprintf(name,(size_t)size,"Test room");}
static int Dvar_GetInt(const char *name) {
    if(!strcmp(name,"ui_botCount")) return 8;
    assert(!strcmp(name,"ui_botDifficulty")); return 2;
}
static void UI_MatchMenu_Rules(const char *gameType, int *timeLimit, int *scoreLimit) {
    assert(!strcmp(gameType,"tdm")); *timeLimit=30; *scoreLimit=50;
}
void Web_CreateServer(const char *name, const char *map, const char *gameType, int botCount, int botDifficulty,
                      int timeLimit, int scoreLimit) {
    assert(!strcmp(gameType,"tdm") && botCount==8 && botDifficulty==2 && timeLimit==30 && scoreLimit==50);
    assert(!strcmp(name,"Test room")); ++calls; snprintf(output,sizeof output,"%s",map);
}
static const char *va(const char *format, ...) {
    static char buffer[256]; va_list args; va_start(args,format);
    vsnprintf(buffer,sizeof buffer,format,args); va_end(args); return buffer;
}
static void Cbuf_ExecuteText(int when, const char *command) {
    assert(when==2); ++calls; snprintf(output,sizeof output,"%s",command);
}
'''
harness += '\nstatic void create(void) {\n' + create + '\n}\n'
harness += '\nstatic void vote(const char *name) {\n' + branch('voteMap') + '\n' + branch('voteTypeMap') + '\n}\n'
harness += r'''
int main(void) {
    const char *maps[]={"mp_toujane","mp_carentan","mp_dawnville","mp_burgundy","mp_railyard"};
    sharedUiInfo.mapCount=5; sharedUiInfo.numGameTypes=1; sharedUiInfo.gameTypes[0].gameType="tdm";
    for(int i=0;i<5;++i) sharedUiInfo.mapList[i].mapLoadName=maps[i];
    for(int i=0;i<5;++i) {
        selectedMap.current.integer=i; create(); assert(!strcmp(output,maps[i]));
        char expected[256];
        vote("voteMap"); snprintf(expected,sizeof expected,"callvote map %s\n",maps[i]);
        assert(!strcmp(output,expected));
        vote("voteTypeMap"); snprintf(expected,sizeof expected,"callvote typemap tdm %s\n",maps[i]);
        assert(!strcmp(output,expected));
    }
    for(int i=-1;i<=128;++i) if(i<0 || i>=5) {
        selectedMap.current.integer=i; int before=calls;
        create(); vote("voteMap"); vote("voteTypeMap"); assert(calls==before);
    }
    puts("PASS: actual UI creation/map/type-map votes select all five indices and reject invalid indices");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-ui-maps-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(harness)
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                    str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
