#!/usr/bin/env python3
"""Keep game-message commands intact across the engine's two va buffers."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/game_mp/g_scr_main_mp.c').read_text()
start = source.index('static inline __attribute__((always_inline)) void Scr_MakeGameMessage_core(')
core = source[start:source.index('\nvoid Scr_MakeGameMessage(', start)]
start = source.index('void iprintln(void)\n{')
functions = source[start:source.index('\nstatic inline ', start)]
support = r'''
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
static char buffers[2][1024], sent[1024];
static int index_, overlap;
static const char *va(const char *format,...) {
    char *buffer=buffers[index_];index_=(index_+1)%2;
    va_list args;va_start(args,format);
    if(!strcmp(format,"%s \"%s\"")) {
        va_list copy;va_copy(copy,args);
        if(va_arg(copy,const char *)==buffer) overlap=1;
        va_end(copy);
    }
    vsnprintf(buffer,1024,format,args);va_end(args);
    return buffer;
}
static int Scr_GetNumParam(void) { return 2; }
static void Scr_ConstructMessageString(int first,int last,const char *context,char *out,int length) {
    const char *player=va("%s^7","Unknown Soldier");
    snprintf(out,length,"MP_JOINED_ALLIES\025%s",player);
}
static void SV_GameSendServerCommand(int client,int type,const char *text) {
    snprintf(sent,sizeof(sent),"%s",text);
}
'''
checks = r'''
int main(void) {
    iprintln();
    if(overlap) return 1;
    if(strncmp(sent,"f \"MP_JOINED_ALLIES",18)) return 1;
    iprintlnbold();
    if(strncmp(sent,"g \"MP_JOINED_ALLIES",18)) return 2;
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-game-messages-') as directory:
    path = Path(directory)
    for mutant in (False, True):
        body = functions
        if mutant:
            body = body.replace('(-1, "f")', '(-1, va("%c", 0x66))').replace('(-1, "g")', '(-1, va("%c", 0x67))')
        (path / 'test.c').write_text(support + core + body + checks)
        subprocess.run(['cc', '-std=c99', str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')])
        assert result.returncode == (1 if mutant else 0), result.returncode
print('PASS: message opcodes survive interpolation; old code aliases snprintf input/output (undefined behavior)')
