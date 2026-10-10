#!/usr/bin/env python3
"""Compare the actual cached encoder to the original wire format; benchmark both."""
from pathlib import Path
import json, re, subprocess, tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/qcommon/msg_mp.c').read_text()
def function(name):
    m=re.search(r'(?:static )?[^\n;]*\b'+name+r'\([^;]*?\)\n\{',source)
    assert m,name
    end=m.end();depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[m.start():end]+'\n'
huffman=(root/'src/PC/qcommon/huffman.c').read_text()
huffman=re.sub(r'^#include "(?:common_types|imports).h"\n','',huffman,flags=re.M)
huffman=huffman.replace('(int)NULL','(intptr_t)NULL')
# The engine targets 32-bit x86/WASM. Match native pointer storage in the host fixture.
huffman=re.sub(r'#if defined\(__x86_64__\).*?#endif','#define NODE_PTR_CAST (intptr_t)',huffman,count=1,flags=re.S)
seed=re.search(r'int msg_hData\[256\] = .*?;',source,re.S).group()
declarations=source[source.index('static unsigned int msgHuffCodes'):source.index('static void MSG_BuildHuffmanCodes')]
support=r'''
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef unsigned char byte;typedef int qboolean;
typedef struct nodetype {intptr_t left,right,parent,next,prev,head;int weight,symbol;} node_t;
typedef struct {int blocNode,blocPtrs;node_t *tree,*lhead,*ltail,*loc[257];node_t **freelist;node_t nodeList[768];node_t *nodePtrs[768];} huff_t;
typedef struct huffman_t {huff_t compressor,decompressor;} huffman_t;
typedef struct {byte *data;int maxsize;} msg_t;
static huffman_t msgHuff;static int msgInit;
void Com_Memset(void *p,int value,int size){memset(p,value,size);}
'''
checks=r'''
static byte input[65536],old[131104],fresh[131104],decodedOld[262208],decodedNew[262208];
static uint32_t rng=123456789;
static uint32_t random32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static int original(byte *from,byte *to,int size){int bits=0;for(int i=0;i<size;i++)Huff_offsetTransmit(&msgHuff.compressor,from[i],to,&bits);return(bits+7)/8;}
static int originalDecode(byte *from,byte *to,int size){int bit=0,n=0;while(bit<size*8){int symbol;Huff_offsetReceive(msgHuff.decompressor.tree,&symbol,from,&bit);to[n++]=(byte)symbol;}return n;}
static double seconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
int main(int argc,char **argv){
 msg_t msg;MSG_Init(&msg,input,sizeof(input));assert(msgHuffCodesReady);
 int cases=0,min=100,max=0;
 for(int s=0;s<256;s++){assert(msgHuffLengths[s]);if(msgHuffLengths[s]<min)min=msgHuffLengths[s];if(msgHuffLengths[s]>max)max=msgHuffLengths[s];}
 for(int mode=0;mode<4;mode++)for(int c=0;c<1024;c++){
  int size=c<257?c:(int)(random32()%65537);
  for(int i=0;i<size;i++){uint32_t r=random32();input[i]=mode==0?0:mode==1?(byte)i:mode==2?(r%5?(byte)(r%16):(byte)r):(byte)r;}
  memset(old,0xa5,sizeof(old));memset(fresh,0xa5,sizeof(fresh));
  int a=original(input,old,size),b=MSG_WriteBitsCompress(input,fresh,size);
  assert(a==b && !memcmp(old,fresh,sizeof(old)));
  memset(decodedOld,0xa5,sizeof(decodedOld));memset(decodedNew,0xa5,sizeof(decodedNew));
  int na=originalDecode(old,decodedOld,a),nb=MSG_ReadBitsCompress(fresh,decodedNew,b);
  assert(na==nb && !memcmp(decodedOld,decodedNew,sizeof(decodedOld)));
  assert(na>=size && !memcmp(decodedNew,input,size));
  cases++;
 }
 msgHuffCodesReady=0;assert(original(input,old,256)==MSG_WriteBitsCompress(input,fresh,256));assert(!memcmp(old,fresh,original(input,old,256)));msgHuffCodesReady=1;
 msgHuffDecodeReady=0;int fallbackSize=original(input,old,256);assert(originalDecode(old,decodedOld,fallbackSize)==MSG_ReadBitsCompress(old,decodedNew,fallbackSize));msgHuffDecodeReady=1;
 /* Every possible prefix, truncated packet lengths and tail-byte alignments,
  * including the unused tree prefix, retain the original decode behavior. */
 for(int prefix=0;prefix<2048;prefix++)for(int n=0;n<5;n++){
  memset(old,0,sizeof(old));old[0]=prefix;old[1]=prefix>>8;
  memset(decodedOld,0xa5,sizeof(decodedOld));memset(decodedNew,0xa5,sizeof(decodedNew));
  int a=originalDecode(old,decodedOld,n),b=MSG_ReadBitsCompress(old,decodedNew,n);
  assert(a==b && !memcmp(decodedOld,decodedNew,sizeof(decodedOld)));
 }
 if(argc>1){
  printf("{\"cases\":%d,\"codeBits\":[%d,%d],\"benchmarks\":[",cases,min,max);
  for(int mode=0;mode<3;mode++){
   for(int i=0;i<4096;i++){uint32_t r=random32();input[i]=mode==0?0:mode==1?(r%5?(byte)(r%16):(byte)r):(byte)r;}
   double oldBest=1e9,newBest=1e9,decodeOld=1e9,decodeNew=1e9;volatile int total=0;
   for(int repeat=0;repeat<4;repeat++){
    double t=seconds();for(int i=0;i<2000;i++)total+=original(input,old,4096);double elapsed=seconds()-t;if(elapsed<oldBest)oldBest=elapsed;
    t=seconds();for(int i=0;i<2000;i++)total+=MSG_WriteBitsCompress(input,fresh,4096);elapsed=seconds()-t;if(elapsed<newBest)newBest=elapsed;
    int compressed=original(input,old,4096);
    t=seconds();for(int i=0;i<2000;i++)total+=originalDecode(old,decodedOld,compressed);elapsed=seconds()-t;if(elapsed<decodeOld)decodeOld=elapsed;
    t=seconds();for(int i=0;i<2000;i++)total+=MSG_ReadBitsCompress(old,decodedNew,compressed);elapsed=seconds()-t;if(elapsed<decodeNew)decodeNew=elapsed;
   }
   printf("%s{\"distribution\":\"%s\",\"originalMs\":%.3f,\"cachedMs\":%.3f,\"speedup\":%.2f,\"decodeOriginalMs\":%.3f,\"decodeCachedMs\":%.3f,\"decodeSpeedup\":%.2f}",mode?",":"",mode==0?"zeros":mode==1?"snapshot-like":"random",oldBest*1000,newBest*1000,oldBest/newBest,decodeOld*1000,decodeNew*1000,decodeOld/decodeNew);assert(total);
  }
  puts("]}");
 }else printf("PASS: %d Huffman encode/decode wire comparisons, every symbol, 10240 prefix/truncation cases, tail padding, fallback and buffer canaries; widths %d..%d bits\n",cases,min,max);
}
'''
# Adapt void-pointer import declarations to the actual typed Huffman definitions in this TU.
body=function('MSG_BuildHuffmanCodes')+function('MSG_WriteBitsCompress')+function('MSG_ReadBitsCompress')+function('MSG_Init')
body=body.replace('Huff_offsetTransmit(&msgHuff,','Huff_offsetTransmit(&msgHuff.compressor,').replace('Huff_addRef(&msgHuff,','Huff_addRef(&msgHuff.compressor,')
with tempfile.TemporaryDirectory(prefix='cod2-huffman-') as d:
 p=Path(d);code=support+huffman+seed+declarations+body+checks;(p/'test.c').write_text(code)
 subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
 # A broken bit order must fail the comparison, even if its own decoder could agree.
 (p/'mutant.c').write_text(code.replace('pending |= msgHuffCodes[symbol] << pendingBits;','pending |= (msgHuffCodes[symbol] ^ 1u) << pendingBits;'))
 subprocess.run(['cc','-O1',str(p/'mutant.c'),'-o',str(p/'mutant')],check=True)
 assert subprocess.run([str(p/'mutant')],capture_output=True).returncode!=0
 (p/'mutant.c').write_text(code.replace('entry = msgHuffDecode[pending &', 'entry = msgHuffDecode[(pending >> 1) &'))
 subprocess.run(['cc','-O1',str(p/'mutant.c'),'-o',str(p/'mutant')],check=True)
 assert subprocess.run([str(p/'mutant')],capture_output=True).returncode!=0
 if '--benchmark' in __import__('sys').argv:
  subprocess.run(['cc','-O3',str(p/'test.c'),'-o',str(p/'bench')],check=True)
  report=json.loads(subprocess.check_output([str(p/'bench'),'benchmark']))
  (root/'out/huffman-performance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
