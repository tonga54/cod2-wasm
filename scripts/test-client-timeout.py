#!/usr/bin/env python3
"""Exercise CL_Frame's real timeout branch, including a regression mutation."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/client_mp/cl_main_mp.c').read_text()
start = source.index('void CL_Frame(int msec)\n{')
end = source.index('Lmain1:', start) + len('Lmain1:')
branch = source[start:end] + '\n    return;\nL183: return;\nLuserinfo: return;\n}\n'
definitions = r'''
#include <stdio.h>
typedef struct { int state, connectTime, lastPacketTime; } clientConnection_t;
typedef struct { int timeoutcount; } clientActive_t;
typedef struct { struct { int enabled; float value; } current; } dvar_t;
typedef struct { int cl_running; } LegacyHacks;
static clientConnection_t clientConnections[1];
static clientActive_t clients[1], *cl = clients;
static struct { int realtime; } cls;
static dvar_t paused, svPaused, timeout = {{0, 200.0f}}, *cl_timeout = &timeout;
static const dvar_t *pausedPtr = &paused, *svPausedPtr = &svPaused;
static LegacyHacks legacy = {1}, *legacyPtr = &legacy;
static int modifiedFlags, errors;
static void *imp_legacyHacks = &legacyPtr, *imp_cl = &cl;
static void *imp_cl_paused = &pausedPtr, *imp_sv_paused = &svPausedPtr;
static void *imp_dvar_modifiedFlags = &modifiedFlags;
static void Voice_GetLocalVoiceData(void *unused) { (void)unused; }
static void Voice_Playback(void) {}
static void CL_UpdateColor(void) {}
static void Com_Error(int code, const char *message) { (void)code; (void)message; ++errors; }
'''
checks = r'''
int main(void) {
    clientConnections[0].state = 8;
    clientConnections[0].connectTime = 0;
    cls.realtime = 1000000;
    clientConnections[0].lastPacketTime = cls.realtime - 10;
    for (int i = 0; i < 20; ++i) CL_Frame(16);
    if (errors || clients[0].timeoutcount) return 1;
    clientConnections[0].lastPacketTime = cls.realtime - 200001;
    for (int i = 0; i < 5; ++i) CL_Frame(16);
    if (errors) return 2;
    CL_Frame(16);
    if (errors != 1) return 3;
    clientConnections[0].lastPacketTime = cls.realtime;
    CL_Frame(16);
    if (clients[0].timeoutcount) return 4;
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-client-timeout-') as directory:
    path = Path(directory)
    for mutant in (False, True):
        body = branch.replace('clc_p->lastPacketTime', 'clc_p->connectTime') if mutant else branch
        (path / 'test.c').write_text(definitions + body + checks)
        subprocess.run(['cc', '-std=c99', str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')])
        assert result.returncode == (1 if mutant else 0), (mutant, result.returncode)
print('PASS: active connections do not age out; missing packets time out; recovery resets; old code fails')
