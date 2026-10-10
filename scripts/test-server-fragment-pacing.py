#!/usr/bin/env python3
"""Compare actual fragment headers/payloads and ensure LAN batching is bounded."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parent.parent
sv=(root/'src/PC/server_mp/sv_net_chan_mp.c').read_text();net=(root/'src/PC/qcommon/net_chan_mp.c').read_text()
def function(s,name):
 m=re.search(r'(?:static )?[^\n;]*\b'+name+r'\([^;]*?\)\n\{',s);assert m,name
 e=m.end();depth=1
 while depth:depth+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[m.start():e]+'\n'
support=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char byte;typedef int Bool;typedef int qboolean;
typedef struct {int lan;} netadr_t;
typedef struct {int dummy;} netProfileStream_t;typedef struct {netProfileStream_t send;} netProfileInfo_t;
typedef struct {int outgoingSequence,sock,qport,unsentFragmentStart,unsentLength,unsentFragments;byte unsentBuffer[16384];netadr_t remoteAddress;netProfileInfo_t *pProf;} netchan_t;
typedef struct {byte *data;int cursize,maxsize;} msg_t;
typedef struct {struct {int integer,enabled;} current;} dvar_t;
static dvar_t show,*showpackets=&show,*net_showprofile=&show;static int net_iProfilingOn;
static const char *netsrcString[2]={"client","server"};
static void NetProf_PrepProfiling(netProfileInfo_t **p){}static void NetProf_AddPacket_core(netProfileStream_t *p,int size,int fragment){}
static void Com_Printf(const char *fmt,...){}
static void MSG_Init(msg_t *m,byte *data,int size){*m=(msg_t){.data=data,.maxsize=size};}
static void MSG_WriteData(msg_t *m,const void *data,int size){assert(size>=0 && m->cursize+size<=m->maxsize);memcpy(m->data+m->cursize,data,size);m->cursize+=size;}
static void MSG_WriteLong(msg_t *m,int value){byte data[4]={value,value>>8,value>>16,value>>24};MSG_WriteData(m,data,4);}
static void MSG_WriteShort(msg_t *m,int value){byte data[2]={value,value>>8};MSG_WriteData(m,data,2);}
static int packets,failAt=-1;static byte wire[32][1400];static int lengths[32];
static Bool NET_SendPacket(int sock,int size,const void *data,netadr_t to){assert(sock==1&&size<=1400&&packets<32);lengths[packets]=size;memcpy(wire[packets],data,size);return ++packets!=failAt;}
static qboolean Sys_IsLANAddress(netadr_t address){return address.lan;}
'''
checks=r'''
int main(void){
 int cases=0;
 for(int size=1300;size<=16384;size+=17)for(int lan=0;lan<2;lan++){
  netchan_t reference={.outgoingSequence=77,.sock=1,.unsentLength=size,.unsentFragments=1,.remoteAddress={lan}},test;
  for(int i=0;i<size;i++)reference.unsentBuffer[i]=(byte)(i*37+size);test=reference;
  packets=0;while(reference.unsentFragments)assert(Netchan_TransmitNextFragment(&reference));
  byte expected[32][1400];int expectedLengths[32],expectedPackets=packets;memcpy(expected,wire,sizeof(wire));memcpy(expectedLengths,lengths,sizeof(lengths));
  packets=0;int rounds=0;
  while(test.unsentFragments){int before=packets;assert(SV_Netchan_TransmitNextFragment(&test));assert(packets-before<=(lan?8:1));rounds++;}
  assert(packets==expectedPackets && !memcmp(expectedLengths,lengths,packets*sizeof(int)));
  for(int i=0;i<packets;i++)assert(!memcmp(expected[i],wire[i],lengths[i]));
  assert(test.outgoingSequence==78 && rounds==(expectedPackets+(lan?7:0))/(lan?8:1));cases++;
 }
 netchan_t c={.outgoingSequence=5,.sock=1,.unsentLength=16384,.unsentFragments=1,.remoteAddress={1}};
 packets=0;failAt=3;assert(!SV_Netchan_TransmitNextFragment(&c));assert(packets==3 && c.unsentFragments);
 // Full-size final chunks need the protocol's zero-byte terminator.
 c=(netchan_t){.outgoingSequence=5,.sock=1,.unsentLength=2600,.unsentFragments=1,.remoteAddress={1}};
 packets=0;failAt=-1;assert(SV_Netchan_TransmitNextFragment(&c));assert(packets==3 && lengths[2]==10 && !c.unsentFragments && c.outgoingSequence==6);
 printf("PASS: %d original-wire fragment comparisons, LAN batch limit, public UDP pacing, terminators and send failure\n",cases);
}
'''
body=function(net,'Netchan_TransmitNextFragment')+function(sv,'SV_Netchan_SendFragments')+function(sv,'SV_Netchan_TransmitNextFragment')
# The initial send has already emitted one packet, leaving seven in its batch.
assert 'SV_Netchan_SendFragments(netchan, 7)' in function(sv,'SV_Netchan_Transmit')
with tempfile.TemporaryDirectory(prefix='cod2-fragments-') as d:
 p=Path(d);(p/'test.c').write_text(support+body+checks)
 subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
 (p/'mutant.c').write_text(support+body.replace('? 8 : 1','? 32 : 1')+checks);subprocess.run(['cc','-O1',str(p/'mutant.c'),'-o',str(p/'mutant')],check=True);assert subprocess.run([str(p/'mutant')],capture_output=True).returncode!=0
