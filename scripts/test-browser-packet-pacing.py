#!/usr/bin/env python3
"""Exercise the real command sender on virtual LAN addresses at high FPS."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/client_mp/cl_input.c').read_text()
match = re.search(r'^void CL_SendCmdInternal\(void\)\n\{', source, re.M)
end, depth = match.end(), 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
body = source[match.start():end]
support = r'''
#include <assert.h>
#include <string.h>
#define __EMSCRIPTEN__ 1
#define CA_LOADING 6
#define CA_PRIMED 7
#define CA_CINEMATIC 9
#define CA_LOGO 10
#define NA_LOOPBACK 0
typedef struct {int serverTime;} usercmd_t;
typedef struct {int p_realtime;} outPacket_t;
typedef struct {int type;} netadr_t;
typedef struct {int state,demoplaying,demowaiting,connectTime;netadr_t serverAddress;struct {int outgoingSequence;} netchan;} clientConnection_t;
typedef struct {int cmdNumber;usercmd_t cmds[128];outPacket_t outPackets[32];} clientActive_t;
typedef struct {struct {int integer,enabled;} current;} dvar_t;
static struct {int realtime;} cls;
static clientConnection_t connection,*cp=&connection;
static clientActive_t client,*ap=&client;
static void *imp_clc=&cp,*imp_cl=&ap;
static dvar_t rate={.current.integer=60},show;
static dvar_t *cl_maxpackets=&rate,*cl_showSend=&show;
static int packets,lastSent,largestBatch;
static int Sys_IsLANAddress(netadr_t a) {return 1;}
static void Com_Printf(const char *s,...) {}
static usercmd_t CL_CreateCmd(void) {return (usercmd_t){.serverTime=cls.realtime};}
static void CL_WritePacket(void) {
    int count=client.cmdNumber-lastSent;
    assert(count>=1&&count<128);
    for(int i=lastSent+1;i<=client.cmdNumber;i++)assert(client.cmds[i&127].serverTime>0);
    if(count>largestBatch)largestBatch=count;
    lastSent=client.cmdNumber;packets++;
    client.outPackets[connection.netchan.outgoingSequence++&31].p_realtime=cls.realtime;
}
'''
checks = r'''
int main(void) {
    for(int frame=4;frame<=16;frame+=4) {
        memset(&client,0,sizeof(client));memset(&connection,0,sizeof(connection));
        connection.state=8;connection.serverAddress.type=1;
        packets=lastSent=largestBatch=0;
        for(int time=frame;time<=9600;time+=frame) {cls.realtime=time;CL_SendCmdInternal();}
        assert(client.cmdNumber==9600/frame);
        assert(packets>=575&&packets<=576);
        assert(client.cmdNumber-lastSent<=(17+frame-1)/frame);
        assert(largestBatch<=(17+frame-1)/frame);
    }
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-packet-pacing-') as directory:
    d = Path(directory)
    for mutant in (False, True):
        tested = body.replace('#ifndef __EMSCRIPTEN__', '#if 1') if mutant else body
        (d / 'test.c').write_text(support + tested + checks)
        subprocess.run(['cc', '-O1', '-fsanitize=address,undefined', str(d / 'test.c'),
                        '-o', str(d / 'test')], check=True)
        result = subprocess.run([str(d / 'test')], capture_output=True)
        assert (result.returncode == 0) != mutant, result.stderr.decode()
print('PASS: 62–250 FPS samples retain every command while pacing gateway packets; LAN bypass mutant fails')
