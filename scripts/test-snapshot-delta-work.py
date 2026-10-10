#!/usr/bin/env python3
"""Compare optimized snapshot field/array/objective work with the original encoding."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/qcommon/msg_mp.c').read_text()
def function(name):
    m=re.search(r'[^\n;]*\b'+name+r'\([^;]*?\)\n\{',source);assert m,name
    end,depth=m.end(),1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[m.start():end]+'\n'
support=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char byte;typedef int qboolean;
typedef struct {char *name;int offset,bits;} NetField;
typedef struct {int overflowed;byte *data;int maxsize,cursize,readcount,bit;} msg_t;
typedef struct {int state,fields[6],ignored;} objective_t;
typedef struct {objective_t objective[16];} playerState_t;
static NetField objectiveFields[8]={{0,4,0},{0,8,0},{0,12,0},{0,16,0},{0,20,0},{0,24,0}};
static uint32_t rng=123456789;
static unsigned Random(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void ReferenceArray(msg_t *msg,int *from,int *to,int groups,int lead){
 int masks[4]={0},changed=0;
 for(int g=0;g<groups;g++)for(int i=0;i<16;i++)if(from[g*16+i]!=to[g*16+i])masks[g]|=1<<i;
 for(int g=0;g<groups;g++)changed|=masks[g];
 if(lead){if(!changed){MSG_WriteBit0_core(msg);return;}MSG_WriteBit1_core(msg);}
 for(int g=0;g<groups;g++){
  if(!masks[g]){MSG_WriteBit0_core(msg);continue;}
  MSG_WriteBit1_core(msg);MSG_WriteShort_core(msg,masks[g]);
  for(int i=0;i<16;i++)if(masks[g]&(1<<i))MSG_WriteShort_core(msg,to[g*16+i]);
 }
}
static int ReferenceObjectives(playerState_t *a,playerState_t *b){
 for(int i=0;i<16;i++){
  if(a->objective[i].state!=b->objective[i].state)return 1;
  for(int j=0;j<6;j++)if(a->objective[i].fields[j]!=b->objective[i].fields[j])return 1;
 }
 return 0;
}
'''
# Reference functions use the actual production bit cursors, so comparisons
# include partial bytes, buffer limits and overflowing writes, not just values.
bitio=''.join(function(n) for n in ('MSG_WriteBits_core','MSG_WriteBit0_core','MSG_WriteBit1_core','MSG_WriteShort_core'))
body=''.join(function(n) for n in ('MSG_LastChangedField','MSG_WriteDeltaPlayerstateShortArray','MSG_WriteDeltaPlayerstateShortArrayNoLead','MSG_PlayerstateObjectivesChanged'))
checks=r'''
int main(void){
 NetField fields[105];int a[128],b[128];
 for(int run=0;run<10000;run++){
  for(int i=0;i<128;i++){a[i]=(int)Random();b[i]=a[i];}
  for(int i=0;i<105;i++)fields[i].offset=(Random()%128)*4;
  int changes=run%20;for(int i=0;i<changes;i++)b[Random()%128]=(int)Random();
  int sizes[]={22,59,68,105};
  for(int n=0;n<4;n++){
   int expected=0,count=sizes[n];
   for(int i=0;i<count;i++)if(a[fields[i].offset/4]!=b[fields[i].offset/4])expected=i+1;
   assert(MSG_LastChangedField((byte*)a,(byte*)b,fields,count)==expected);
  }
 }
 // Every possible last changed field, not just random combinations.
 for(int count=0;count<=105;count++)for(int last=-1;last<count;last++){
  memset(a,0,sizeof(a));memset(b,0,sizeof(b));
  for(int i=0;i<count;i++)fields[i].offset=i*4;
  if(last>=0)b[last]=(int)0x80000000u;
  assert(MSG_LastChangedField((byte*)a,(byte*)b,fields,count)==last+1);
 }
 byte old[512],fresh[512];
 for(int run=0;run<4096;run++){
  int from[64],to[64];for(int i=0;i<64;i++){from[i]=(int)Random();to[i]=from[i];}
  for(int i=0;i<run%17;i++)to[Random()%64]=(int)Random();
  for(int groups=0;groups<=4;groups++)for(int lead=0;lead<2;lead++)for(int offset=0;offset<8;offset++){
   memset(old,0xa5,sizeof(old));memset(fresh,0xa5,sizeof(fresh));
   int max=run%2?512:run%128;
   msg_t x={.data=old,.maxsize=max},y={.data=fresh,.maxsize=max};
   MSG_WriteBits_core(&x,0x55,offset);MSG_WriteBits_core(&y,0x55,offset);
   ReferenceArray(&x,from,to,groups,lead);
   if(lead)MSG_WriteDeltaPlayerstateShortArray(&y,from,to,groups);
   else MSG_WriteDeltaPlayerstateShortArrayNoLead(&y,from,to,groups);
   assert(x.bit==y.bit&&x.cursize==y.cursize&&x.overflowed==y.overflowed&&!memcmp(old,fresh,sizeof(old)));
  }
 }
 playerState_t p,q;
 for(int run=0;run<10000;run++){
  memset(&p,0,sizeof(p));q=p;
  if(run%3==1)q.objective[Random()%16].ignored=(int)Random();
  else if(run%3==2){int i=Random()%16,j=Random()%7;int *v=&q.objective[i].state;v[j]=(int)Random();}
  assert(MSG_PlayerstateObjectivesChanged(&p,&q)==ReferenceObjectives(&p,&q));
 }
 puts("PASS: 40,000 field-table comparisons and every last-field boundary; 327,680 identical ammo/clip encodings including partial bytes/overflow; 10,000 objective changes/ignored fields");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-snapshot-delta-') as tmp:
    p=Path(tmp);(p/'test.c').write_text(support[:support.index('static void ReferenceArray')]+bitio+support[support.index('static void ReferenceArray'):]+body+checks)
    subprocess.run(['cc','-O1','-g','-fsanitize=address,undefined','-fno-sanitize=alignment',str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
