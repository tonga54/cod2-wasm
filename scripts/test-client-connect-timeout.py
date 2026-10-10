#!/usr/bin/env python3
"""Exercise the real accepted-handshake and receive-timeout branches."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/PC/client_mp/cl_main_mp.c').read_text()
a = source.index('    if (I_stricmp(cmd, (const char *)"connectResponse") == 0) {')
b = source.index('\n    if (I_stricmp(cmd, (const char *)"infoResponse")', a)
handshake = source[a:b]
a = source.index('    t = cls.realtime - clc_p->lastPacketTime;')
b = source.index('\nL137:', a)
timeout = source[a:b]
support = r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
#define CA_CHALLENGING 4
#define CA_CONNECTED 5
#define NS_CLIENT1 0
struct Connection {int state,lastPacketTime,lastPacketSentTime,netchan,serverAddress;} storage;
struct {int realtime;} cls;
struct Active {int timeoutcount;} active;
struct {struct {float value;} current;} setting={.current.value=200},*cl_timeout=&setting;
int qport,failed,setups;void *imp_g_qport=&qport;
int I_stricmp(const char *a,const char *b){return strcmp(a,b);}
int NET_CompareBaseAdr(int a,int b){return a==b;}
const char *NET_AdrToString(int a){return "fixture";}
void Com_Printf(const char *fmt,...){}
void Netchan_Setup(int source,int *netchan,int from,int port){setups++;}
void Com_Error(int code,const char *msg){assert(code==1&&!strcmp(msg,"EXE_ERR_SERVER_TIMEOUT"));failed=1;}
'''
functions = '''int Accept(int from){struct Connection *conn=&storage;int result=0;const char *cmd="connectResponse";\n''' + handshake + '''\nfinish:return result;}\nvoid Frame(void){struct Connection *clc_p=&storage;struct Active *cl_p=&active;int t;\n''' + timeout + '''\nLmain0:cl_p->timeoutcount=0;\nLmain1:return;}\n'''
checks = r'''
int main(void){
 const int clocks[]={0,190000,300000,3600000,86400000};
 for(unsigned i=0;i<sizeof(clocks)/sizeof(clocks[0]);i++){
  memset(&storage,0,sizeof(storage));memset(&active,0,sizeof(active));failed=0;
  cls.realtime=clocks[i];storage.state=CA_CHALLENGING;storage.serverAddress=42;storage.lastPacketTime=-9999;
  assert(!Accept(43)&&storage.state==CA_CHALLENGING&&storage.lastPacketTime==-9999);
  assert(Accept(42)&&storage.state==CA_CONNECTED&&storage.lastPacketTime==clocks[i]);
  for(int f=0;f<10;f++)Frame();assert(!failed);
  int accepted=storage.lastPacketTime;cls.realtime+=1000;
  assert(!Accept(42)&&storage.lastPacketTime==accepted); // Duplicates cannot extend the deadline.
  cls.realtime=accepted+199999;Frame();assert(!failed);
  cls.realtime=accepted+200001;for(int f=0;f<6;f++)Frame();assert(failed);
 }
 assert(setups==5);puts("PASS: valid late handshakes get a fresh timeout; wrong/duplicate replies do not; silent servers still time out");
}
'''
code = support + functions + checks
with tempfile.TemporaryDirectory(prefix='cod2-connect-timeout-') as tmp:
    p = Path(tmp) / 'test.c'
    for mutant, variant in enumerate((code, code.replace('conn->lastPacketTime = cls.realtime;', 'conn->lastPacketTime = -9999;'))):
        p.write_text(variant)
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', str(p), '-o', str(p.with_suffix(''))], check=True)
        result = subprocess.run([str(p.with_suffix(''))], capture_output=True, text=True, timeout=30)
        assert (result.returncode == 0) == (mutant == 0), result.stderr
        if not mutant: print(result.stdout.strip())
        else: print('PASS: stale handshake timestamp mutant rejected')
