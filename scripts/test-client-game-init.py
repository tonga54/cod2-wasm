#!/usr/bin/env python3
"""A gamestate sequence must never be mistaken for the local player index."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/client_mp/cl_cgame_mp.c').read_text()
a=source.index('        clientConnection_t *clui = CLUI_STATE;',source.index('void CL_InitCGame('))
b=source.index('\n    }',a)
body=source[a:b]
support=r'''
#include <assert.h>
typedef struct {int state,clientNum,lastExecutedServerCommand,serverMessageSequence;} clientConnection_t;
static clientConnection_t connection;
#define CLUI_STATE (&connection)
static struct {int cgameInitCalled,cgameInitialized;} active,*cl=&active;
static void CG_Init(int snapshot,int commands,int client){
 assert(snapshot==connection.serverMessageSequence);assert(commands==connection.lastExecutedServerCommand);
 assert(client==connection.clientNum);assert(cl->cgameInitCalled && connection.state==6);
}
static void initialize(void){
'''
checks=r'''
}
int main(void){for(int n=0;n<64;n++){
 connection=(clientConnection_t){0,n,50+n,1000+n*13};active.cgameInitCalled=active.cgameInitialized=0;
 initialize();assert(active.cgameInitialized && connection.state==7);
}return 0;}
'''
with tempfile.TemporaryDirectory(prefix='cod2-cgame-init-') as directory:
 p=Path(directory)
 old=body.replace('CG_Init(clui->serverMessageSequence, clui->lastExecutedServerCommand, clui->clientNum);','CG_Init(clui->clientNum, clui->lastExecutedServerCommand, clui->serverMessageSequence);')
 for i,variant in enumerate((body,old)):
  (p/'test.c').write_text(support+variant+checks)
  subprocess.run(['cc','-std=c99',str(p/'test.c'),'-o',str(p/'test')],check=True)
  result=subprocess.run([str(p/'test')],capture_output=True)
  assert (result.returncode==0)==(i==0),result.stderr.decode()
print('PASS: all 64 local player identities survive initialization with independent snapshot/command sequences; swapped arguments fail')
