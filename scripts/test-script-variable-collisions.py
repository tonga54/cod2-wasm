#!/usr/bin/env python3
"""Exercise production script-variable hash promotion and cleanup across collisions."""
import re,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/script/scr_variable.c').read_text()
def function(name):
 m=re.search(r'^.*\b'+name+r'\([^;{}]*?\)\n\{',source,re.M)
 assert m,name
 start=m.start()
 if source[start:m.end()].lstrip().startswith(name+'('):start=source.rfind('\n',0,start-1)+1
 at=m.end();depth=1
 while depth:depth+=(source[at]=='{')-(source[at]=='}');at+=1
 return source[start:at]+'\n'
macros=source[source.index('#define SCRVL_SL_'):source.index('COD2_ASSERT_FIELD')]
support=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __attribute_regparm__(n)
typedef unsigned char byte;
typedef union {int intValue;unsigned pointerValue,stringValue;} VariableUnion;
typedef struct {struct {unsigned short id;union {unsigned short prev,prevSibling;}u;}hash;
 union {int u;unsigned short next;struct {unsigned short refCount;union {unsigned short size,entnum,self;}u;}o;}u;
 union {unsigned status;}w;union {unsigned short index,next;}v;unsigned short nextSibling;}VariableValueInternal;
static unsigned char scrVarGlob[1048608];static unsigned s_varNetDbg;
static void Scr_TerminalError(const char *s){fprintf(stderr,"%s\n",s);abort();}
static void RemoveRefToObject(unsigned id);
static void AddRefToObject(unsigned id);
static void RemoveRefToEmptyObject(unsigned id);
static void Scr_FreeObjectChildren(unsigned id);
static unsigned Scr_AllocArray(void);
static void SL_AddRefToString(unsigned id){(void)id;abort();}
static unsigned reentrantReleaseParent,reentrantReleaseCount;
static struct {unsigned entArrayId;} g_classMap[4];
static struct {unsigned pauseArrayId;} scrVarPub;
static void VM_CancelNotify(unsigned owner,unsigned thread){(void)owner;(void)thread;abort();}
static void RemoveRefToValue_core(int type,VariableUnion v){
 if(type==1){
  if(reentrantReleaseParent&&v.pointerValue==reentrantReleaseParent){
   assert(++reentrantReleaseCount==1);Scr_FreeObjectChildren(reentrantReleaseParent);
  }else RemoveRefToObject(v.pointerValue);
 }else assert(type==0||type==6);
}
static void RemoveRefToValue(int type,VariableUnion v){RemoveRefToValue_core(type,v);}
static void Scr_AddRefToArrayNameValue(unsigned name){if(name<=0x1fffd){assert(name>0xffff);AddRefToObject(name-0x10000);}}
'''
names=['Scr_VariableHash','Var_ResetAll','AllocVariable','FreeVariable','ScrVarEntry','ScrVarEntryIndex','Scr_LinkVariableToParent','FindVariableIndexInternal','GetNewVariableIndexInternal3','GetNewVariable','Scr_RemoveRefToArrayNameValue','MakeVariableExternal','FreeChildValue_core','RemoveVariable','Scr_MakeObjectChildrenExternal','Scr_FreeObjectChildren','Scr_AllocArray_core','GetNewObjectVariable','GetNewObjectVariableReverse','GetInternalVariableIndex_core','AddRefToObject','RemoveRefToEmptyObject','RemoveRefToObject','AllocThread','Scr_KillEndonThread','FindObjectVariable','FindNextSibling','RemoveObjectVariable','Scr_KillThread','GetArraySize','Scr_AllocArray','ScrVar_NextSiblingId','CopyArray']
checks=r'''
static unsigned add(unsigned p,unsigned name,int value){unsigned id=GetNewVariable(p,name);VG_STATUS(id)|=6;VG_U32(id)=value;return id;}
static void check(unsigned p,unsigned *names,int *values,int n){
 unsigned at=VG_NEXT_SIBLING(p),prior=VG_NEXT(p);int count=0;
 while(VG_ID(at)!=p){assert(++count<=n);unsigned id=VG_ID(at);assert(VG_PREV(at)==prior);int found=0;
  for(int j=0;j<n;j++)if(names[j]==VG_NAME(id)){assert((int)VG_U32(id)==values[j]);found=1;break;}
  assert(found);prior=at;at=VG_NEXT_SIBLING(id);
 }
 assert(count==n&&VG_PREV(at)==prior&&ScrVarEntry(p)->u.o.u.size==n);
 for(int j=0;j<n;j++){unsigned index=FindVariableIndexInternal(p,names[j]);assert(index);assert((int)VG_U32(VG_ID(index))==values[j]);}
}
static unsigned tree(int depth,int seed){
 unsigned p=Scr_AllocArray_core();
 for(int j=0;j<3;j++){
  unsigned name=0x800000+(seed+j*73)%SCRVL_MAX_VARIABLES;
  unsigned id=GetNewVariable(p,name);
  if(depth){unsigned child=tree(depth-1,seed+j+1);VG_STATUS(id)|=1;VG_U32(id)=child;}
  else{VG_STATUS(id)|=6;VG_U32(id)=seed+j;}
 }
 return p;
}
int main(void){
 assert(sizeof(VariableValueInternal)==16);
 for(int order=0;order<4;order++)for(int count=2;count<=12;count++){
  Var_ResetAll();s_varNetDbg=0;unsigned p=Scr_AllocArray_core();unsigned keys[12];int values[12];
  for(int j=0;j<count;j++){keys[j]=0x800000+j*SCRVL_MAX_VARIABLES;values[j]=j+10;add(p,keys[j],values[j]);}
  check(p,keys,values,count);
  for(int n=count;n>0;n--){int j=order==0?0:order==1?n-1:order==2?n/2:(n*7+3)%n;RemoveVariable(p,keys[j]);
   memmove(keys+j,keys+j+1,(n-j-1)*sizeof(keys[0]));memmove(values+j,values+j+1,(n-j-1)*sizeof(values[0]));check(p,keys,values,n-1);
  }
  FreeVariable(p);
 }
 // Hash collisions between different parents, dense live arrays, repeated
 // clear/reuse and detach-all-before-free exercise actual round teardown.
 Var_ResetAll();s_varNetDbg=0;
 for(int pass=0;pass<200;pass++){
  unsigned parents[32],keys[32][8];int values[32][8];
  for(int p=0;p<32;p++)parents[p]=Scr_AllocArray_core();
  for(int j=0;j<8;j++)for(int p=0;p<32;p++){
   keys[p][j]=0x800000+1000-parents[p]+j*SCRVL_MAX_VARIABLES;values[p][j]=pass*1000+p*10+j;add(parents[p],keys[p][j],values[p][j]);
  }
  for(int p=0;p<32;p++)check(parents[p],keys[p],values[p],8);
  for(int step=0;step<32;step++){int p=(step*13+pass)%32;Scr_FreeObjectChildren(parents[p]);check(parents[p],keys[p],values[p],0);}
  for(int p=0;p<32;p++)FreeVariable(parents[p]);
 }
 // Mix occupied hash buckets with recycled object slots, as overlapping
 // game threads do while round notifications create and destroy locals.
 unsigned seed=1977;
 #define RANDOM() (seed=seed*1664525u+1013904223u)
 unsigned parents2[32]={0},keys2[32][16];int values2[32][16],counts2[32]={0};
 for(int step=0;step<12000;step++){
  unsigned p=(RANDOM()>>16)%32;
  if(!parents2[p])parents2[p]=Scr_AllocArray_core();
  int n=counts2[p];
  if(n && (RANDOM()>>16)%3==0){int j=(RANDOM()>>16)%n;
   RemoveVariable(parents2[p],keys2[p][j]);
   memmove(keys2[p]+j,keys2[p]+j+1,(n-j-1)*sizeof(unsigned));
   memmove(values2[p]+j,values2[p]+j+1,(n-j-1)*sizeof(int));counts2[p]--;
  }else if(n<16){
   unsigned key=0x800000+((RANDOM()>>16)%SCRVL_MAX_VARIABLES);int duplicate=0;
   unsigned other=(RANDOM()>>16)%32;
   if(parents2[other]){unsigned target=VG_NEXT(parents2[other]);
    key=0x800000+((target-1+2*SCRVL_MAX_VARIABLES-parents2[p]-(0x800000%SCRVL_MAX_VARIABLES))%SCRVL_MAX_VARIABLES);
   }
   for(int j=0;j<n;j++)if(keys2[p][j]==key)duplicate=1;
   if(!duplicate){keys2[p][n]=key;values2[p][n]=step;add(parents2[p],key,step);counts2[p]++;}
  }else{Scr_FreeObjectChildren(parents2[p]);FreeVariable(parents2[p]);parents2[p]=0;counts2[p]=0;}
  for(int j=0;j<32;j++)if(parents2[j])check(parents2[j],keys2[j],values2[j],counts2[j]);
 }
 for(int p=0;p<32;p++)if(parents2[p]){Scr_FreeObjectChildren(parents2[p]);FreeVariable(parents2[p]);}
 // Inject a release callback that reenters parent cleanup, as script notify
 // cancellation does. The outer child must already be absent from the ring.
 for(int pass=0;pass<200;pass++){
  unsigned p=Scr_AllocArray_core();reentrantReleaseParent=p;reentrantReleaseCount=0;
  unsigned field=GetNewVariable(p,0x800000+pass);VG_STATUS(field)|=1;VG_U32(field)=p;
  add(p,0x800000+pass+SCRVL_MAX_VARIABLES,pass);
  Scr_FreeObjectChildren(p);assert(reentrantReleaseCount==1);
  reentrantReleaseParent=0;FreeVariable(p);
 }
 // Releasing the last object-key reference recursively cleans up fields
 // in the same collision chain as the array key currently being detached.
 for(int pass=0;pass<300;pass++){
  unsigned parent=Scr_AllocArray_core(),keyObj=Scr_AllocArray_core();
  unsigned field=pass%2?GetNewObjectVariable(parent,keyObj):GetNewObjectVariableReverse(parent,keyObj);
  VG_STATUS(field)|=6;VG_U32(field)=pass;
  unsigned bucket=Scr_VariableHash(parent,keyObj+0x10000);
  unsigned name=0x800000+((bucket-1+2*SCRVL_MAX_VARIABLES-keyObj-(0x800000%SCRVL_MAX_VARIABLES))%SCRVL_MAX_VARIABLES);
  add(keyObj,name,pass+100);add(keyObj,name+SCRVL_MAX_VARIABLES,pass+101);
  RemoveRefToObject(keyObj); // The parent key now owns the last reference.
  RemoveRefToObject(parent);
 }
 // Recursive release moves hash positions while an outer child is being
 // freed. Shared references must keep the surviving array valid.
 for(int pass=0;pass<100;pass++){
  unsigned a=tree(3,pass*119),b=Scr_AllocArray_core();AddRefToObject(a);
  unsigned field=GetNewVariable(b,0x800000+pass);VG_STATUS(field)|=1;VG_U32(field)=a;
  RemoveRefToObject(b);assert((VG_STATUS(a)&31)==SCRVL_VAR_ARRAY&&ScrVarEntry(a)->u.o.u.size==3);
  RemoveRefToObject(a);
 }
 // Endon listeners can be encountered again while recursive cancellation
 // is unwinding. Each transfers its reference once, including shared self.
 for(int pass=0;pass<200;pass++){
  unsigned self=Scr_AllocArray_core();AddRefToObject(self);unsigned a=AllocThread(self);
  AddRefToObject(self);unsigned b=AllocThread(self);RemoveRefToObject(self);
  Scr_KillEndonThread(a);assert((VG_STATUS(self)&31)==SCRVL_VAR_ARRAY);
  Scr_KillEndonThread(b);Scr_KillEndonThread(a);Scr_KillEndonThread(b);
  FreeVariable(a);FreeVariable(b);
 }
 for(int pass=0;pass<200;pass++){
  unsigned self=Scr_AllocArray_core();AddRefToObject(self);unsigned t=AllocThread(self);RemoveRefToObject(self);
  add(t,0x800000+pass,123);add(t,0x800000+pass+SCRVL_MAX_VARIABLES,456);
  Scr_KillThread(t);assert((VG_STATUS(t)&31)==0x17);
  Scr_KillThread(t);FreeVariable(t);
 }

 // Copy-on-read of nested persistent player arrays relocates hash buckets
 // in both the original and its clones while retaining shared actor objects.
 unsigned pinned[512];
 for(int p=0;p<512;p++){
  pinned[p]=Scr_AllocArray_core();
  for(int j=0;j<64;j++)add(pinned[p],0x800000+j,p*1000+j);
 }
 unsigned originals[32],clones[32]={0};
 for(int p=0;p<32;p++){
  unsigned root=Scr_AllocArray_core();originals[p]=root;
  for(int j=0;j<5;j++){
   unsigned sub=Scr_AllocArray_core();unsigned fld=GetNewVariable(root,0x800000+j);VG_STATUS(fld)|=1;VG_U32(fld)=sub;
   for(int k=0;k<3;k++)add(sub,0x800000+k,p*100+j*10+k);
  }
 }
 for(int pass=0;pass<5000;pass++){
  unsigned p=(RANDOM()>>16)%32;
  if(clones[p])RemoveRefToObject(clones[p]);
  unsigned c=Scr_AllocArray_core();CopyArray(originals[p],c);clones[p]=c;
  for(int n=0;n<32;n++)for(int which=0;which<2;which++){
   unsigned root=which?clones[n]:originals[n];if(!root)continue;
   assert(GetArraySize(root)==5);
   for(int j=0;j<5;j++){
    unsigned ix=FindVariableIndexInternal(root,0x800000+j);assert(ix);
    unsigned child=VG_U32(VG_ID(ix));unsigned keys[3]={0x800000,0x800001,0x800002};int values[3]={n*100+j*10,n*100+j*10+1,n*100+j*10+2};
    check(child,keys,values,3);
   }
  }
 }
 for(int p=0;p<32;p++){RemoveRefToObject(originals[p]);if(clones[p])RemoveRefToObject(clones[p]);}
 for(int p=0;p<512;p++)RemoveRefToObject(pinned[p]);
 unsigned freeCount=0,index=VG_U16(0);static unsigned char seen[SCRVL_MAX_VARIABLES+1];
 while(index){assert(index<=SCRVL_MAX_VARIABLES&&!seen[index]);seen[index]=1;++freeCount;index=VG_U16(VG_ID(index));}
 assert(freeCount==SCRVL_MAX_VARIABLES);
 puts("PASS: adjacent/cross-parent hash collisions, head/middle/tail removal, round-style detach/free and slot reuse retain values, array sizes and acyclic sibling rings");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-variable-collisions-') as d:
 p=Path(d);(p/'test.c').write_text(support+macros+''.join(function(n) for n in names).replace('unsigned int arrayName = VG_NAME(id);', 'assert(VG_STATUS(id) & SCRVL_VAR_ALLOCATED);\n    unsigned int arrayName = VG_NAME(id);')+checks)
 subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True,timeout=60)
