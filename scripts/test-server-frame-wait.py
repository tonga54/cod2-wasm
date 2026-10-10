#!/usr/bin/env python3
"""Exercise real dedicated wait policy and UDP-select early wakeups."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parent.parent
sv=(root/'src/PC/server_mp/sv_main_mp.c').read_text()
common=(root/'src/PC/qcommon/common.c').read_text()
net=(root/'src/PC/win32/win_net.c').read_text()
def function(s,name):
 m=re.search(r'[^\n;]*\b'+name+r'\([^;]*?\)\n\{',s);assert m,name
 end=m.end();depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[m.start():end]+'\n'
policy=common[common.index('    if (com_dedicated->current.integer && !com_fixedtime'):common.index('    do {\n        com_frameTime = Com_EventLoop();',common.index('BM_NOINLINE void Com_Frame_Try_Block_Function'))]
# Keep the packet-dispatch-before-wait structure of the real event loop.
assert common.index('com_frameTime = Com_EventLoop();',common.index('BM_NOINLINE void Com_Frame_Try_Block_Function'))<common.index('NET_Sleep(minMsec - rawMsec);')
assert 'SV_PacketEvent(evFrom, &buf);' in function(common,'Com_EventLoop')
support=r'''
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
typedef struct {struct {int integer,enabled;float value;} current;} dvar_t;
static dvar_t fps,running,dedicated,fixed,timescale,maxfps;
static const dvar_t *sv_fps=&fps,*com_sv_running=&running,*com_dedicated=&dedicated,*com_fixedtime=&fixed,*com_timescale=&timescale,*com_maxfps=&maxfps;
static void *imp_com_sv_running=&com_sv_running;
static float com_codeTimeScale=1;static struct {int timeResidual;} sv;
static int ip_socket;
'''
checks=r'''
static int waitPolicy(void){int minMsec;
POLICY
return minMsec;}
static double seconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
static struct sockaddr_in address;
static void *sendPacket(void *unused){struct timespec delay={0,10000000};nanosleep(&delay,0);int s=socket(AF_INET,SOCK_DGRAM,0);assert(s>=0);assert(sendto(s,"x",1,0,(void*)&address,sizeof(address))==1);close(s);return 0;}
int main(void){
 running.current.enabled=1;dedicated.current.integer=1;timescale.current.value=1;
 for(int rate=10;rate<=1000;rate++)for(int residual=0;residual<=1000/rate+5;residual++){
  fps.current.integer=rate;sv.timeResidual=residual;int expected=1000/rate-residual;if(expected<=0)expected=1;
  assert(SV_FrameWaitMilliseconds()==expected && waitPolicy()==expected);
 }
 fps.current.integer=20;sv.timeResidual=0;assert(waitPolicy()==50);
 fixed.current.integer=5;assert(waitPolicy()==1);fixed.current.integer=0;
 timescale.current.value=.5f;assert(waitPolicy()==1);timescale.current.value=1;
 com_codeTimeScale=.5f;assert(waitPolicy()==1);com_codeTimeScale=1;
 dedicated.current.integer=0;maxfps.current.integer=125;assert(waitPolicy()==8);dedicated.current.integer=1;
 running.current.enabled=0;assert(waitPolicy()==50);running.current.enabled=1;
 // Real NET_Sleep uses select: an incoming packet must wake it before the tick.
 ip_socket=socket(AF_INET,SOCK_DGRAM,0);assert(ip_socket>0);address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(0x7f000001u);
 assert(!bind(ip_socket,(void*)&address,sizeof(address)));socklen_t len=sizeof(address);assert(!getsockname(ip_socket,(void*)&address,&len));
 pthread_t thread;assert(!pthread_create(&thread,0,sendPacket,0));double before=seconds();NET_Sleep(1000);double elapsed=seconds()-before;
 assert(elapsed<.5);char packet;assert(recv(ip_socket,&packet,1,0)==1);assert(!pthread_join(thread,0));close(ip_socket);
 puts("PASS: all 10..1000 Hz residual deadlines, client/timescale policy and actual UDP-select immediate wakeup");
}
'''.replace('POLICY',policy)
with tempfile.TemporaryDirectory(prefix='cod2-frame-wait-') as d:
 p=Path(d);body=function(sv,'SV_FrameWaitMilliseconds');(p/'test.c').write_text(support+body+function(net,'NET_Sleep')+checks)
 subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined','-pthread',str(p/'test.c'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
 (p/'mutant.c').write_text(support+body.replace('return remaining > 0 ? remaining : 1;','return 1;')+function(net,'NET_Sleep')+checks)
 subprocess.run(['cc','-O1','-pthread',str(p/'mutant.c'),'-o',str(p/'mutant')],check=True);assert subprocess.run([str(p/'mutant')],capture_output=True).returncode!=0
