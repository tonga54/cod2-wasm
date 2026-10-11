#!/usr/bin/env python3
"""Exercise real endon cancellation, entity queue ownership and round cleanup."""
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
 union {VariableUnion u;unsigned short next;struct {unsigned short refCount;union {unsigned short size,entnum,self;}u;}o;}u;
 union {unsigned status;}w;union {unsigned short index,next;}v;unsigned short nextSibling;}VariableValueInternal;
static unsigned char scrVarGlob[1048608];static unsigned s_varNetDbg;
static void Scr_TerminalError(const char *s){fprintf(stderr,"%s\n",s);abort();}
static void RemoveRefToObject(unsigned id);
static void AddRefToObject(unsigned id);
static void RemoveRefToEmptyObject(unsigned id);
static void Scr_FreeObjectChildren(unsigned id);
static unsigned Scr_AllocArray(void);
static unsigned strings[65536];
static void SL_AddRefToString(unsigned id){assert(id<65536);strings[id]++;}
static void SL_RemoveRefToString(unsigned id){assert(id<65536&&strings[id]);strings[id]--;}
static unsigned reentrantReleaseParent,reentrantReleaseCount;
static struct {unsigned entArrayId;} g_classMap[4];
static struct {unsigned pauseArrayId,freeEntList;int bInited;} scrVarPub;
static void *imp_scrVarPub=&scrVarPub;
#define scrVarPub_t fixturePub
struct fixturePub {unsigned pauseArrayId,freeEntList;int bInited;};
#define VAR_POINTER 1
typedef int Bool;
typedef struct {VariableUnion u;int type;} VariableValue;
typedef struct {unsigned unused;} VariableStackBuffer;
#define SCR_STACK_PTR(u) ((VariableStackBuffer *)(uintptr_t)(u).intValue)
static void VM_TrimStack(unsigned id,VariableStackBuffer*p,int endon){(void)id;(void)p;(void)endon;abort();}
static void VM_CancelNotify(unsigned owner,unsigned thread);
static void Scr_CancelNotifyList(unsigned owner);
static void RemoveRefToValue_core(int type,VariableUnion v){
 if(type==1){
  if(reentrantReleaseParent&&v.pointerValue==reentrantReleaseParent){
   assert(++reentrantReleaseCount==1);Scr_FreeObjectChildren(reentrantReleaseParent);
  }else RemoveRefToObject(v.pointerValue);
 }else if(type==2)SL_RemoveRefToString(v.stringValue);else assert(type==0||type==6);
}
static void RemoveRefToValue(int type,VariableUnion v){RemoveRefToValue_core(type,v);}
static void Scr_AddRefToArrayNameValue(unsigned name){if(name<=0xffff)SL_AddRefToString(name);else if(name<=0x1fffd)AddRefToObject(name-0x10000);}
'''
names=['Scr_VariableHash','Var_ResetAll','AllocVariable','FreeVariable','ScrVarEntry','ScrVarEntryIndex','Scr_LinkVariableToParent','FindVariableIndexInternal','GetNewVariableIndexInternal3','GetNewVariable','Scr_RemoveRefToArrayNameValue','MakeVariableExternal','FreeChildValue_core','RemoveVariable','Scr_MakeObjectChildrenExternal','Scr_FreeObjectChildren','Scr_AllocArray_core','GetNewObjectVariable','GetNewObjectVariableReverse','GetInternalVariableIndex_core','AddRefToObject','RemoveRefToEmptyObject','RemoveRefToObject','AllocThread','Scr_KillEndonThread','FindObjectVariable','FindNextSibling','RemoveObjectVariable','Scr_KillThread','GetArraySize','Scr_AllocArray','ScrVar_NextSiblingId','CopyArray','GetVariable','GetObjectVariable','GetArray','FindObject','FindVariable','GetVarType','GetVariableKeyObject','GetVariableValueAddress','Scr_SetThreadNotifyName','Scr_RemoveThreadNotifyName','Scr_GetThreadNotifyName','GetStartLocalId','Scr_GetSelf','Scr_GetEntityId','Scr_FreeEntityNum','Scr_FreeEntityList','SetNewVariableValue']
vm_source=(root/'src/PC/script/scr_vm.c').read_text()
def vm_function(name):
    global source
    saved=source;source=vm_source
    result=function(name);source=saved
    return result
vm_body=''.join(vm_function(n) for n in ['VM_CancelNotifyInternal','VM_CancelNotify','Scr_CancelWaittill','Scr_CancelNotifyList'])
checks=r'''
static void addEndon(unsigned root,unsigned owner,unsigned event){
 AddRefToObject(owner);SL_AddRefToString(event);AddRefToObject(root);
 unsigned listener=AllocThread(root);
 unsigned list=GetArray(GetVariable(owner,0x1fffe));
 unsigned names=GetArray(GetVariable(list,event));
 GetObjectVariable(names,listener);RemoveRefToObject(listener);
 unsigned pending=GetArray(GetObjectVariable(scrVarPub.pauseArrayId,root));
 VariableValue value={{0},1};value.u.pointerValue=owner;
 SetNewVariableValue(GetNewObjectVariable(pending,listener),&value);
 Scr_SetThreadNotifyName(listener,event);
}
static void checkEntity(unsigned entity){
 unsigned owners=0;
 for(unsigned n=1;n<=SCRVL_MAX_VARIABLES;n++)if(VG_STATUS(n)&SCRVL_VAR_ALLOCATED){
  unsigned t=VG_STATUS(n)&31;
  if(t==1&&VG_U32(n)==entity)owners++;
  if(t>=15&&t<=18&&VG_SIBLING(n)==entity)owners++;
 }
 for(unsigned n=scrVarPub.freeEntList;n;n=VG_SIBLING(n))if(n==entity)owners++;
 assert((VG_STATUS(entity)&31)==20||(VG_STATUS(entity)&31)==21);
 assert(VG_U16(entity)+1==owners);
}
static void roundNotifications(void){
 for(int round=0;round<8;round++){
  Var_ResetAll();s_varNetDbg=0;scrVarPub.pauseArrayId=Scr_AllocArray();
  scrVarPub.bInited=1;scrVarPub.freeEntList=0;g_classMap[0].entArrayId=Scr_AllocArray();
  unsigned level=Scr_AllocArray(),players=Scr_AllocArray(),entities[32],roots[32][5];
  for(int n=0;n<32;n++){
   unsigned entity=Scr_GetEntityId(n,0);entities[n]=entity;AddRefToObject(entity);
   VariableValue value={{0},1};value.u.pointerValue=entity;
   SetNewVariableValue(GetNewVariable(players,0x800000+n),&value);
   for(int j=0;j<5;j++){
    AddRefToObject(entity);unsigned root=AllocThread(entity);roots[n][j]=root;
    AddRefToObject(entity);SetNewVariableValue(GetNewVariable(root,80+j),&value);
    addEndon(root,entity,300);addEndon(root,level,301);
   }
   checkEntity(entity);
  }
  if(round%2){
   for(int step=0;step<32;step++){int n=(step*13+round)%32;Scr_FreeEntityNum(n,0);checkEntity(entities[n]);}
   Scr_FreeEntityList();
  }
  for(int step=0;step<160;step++){
   int n=(step*13+round)%160;unsigned entity=entities[n/5],root=roots[n/5][n%5];
   Scr_KillThread(root);RemoveRefToObject(root);checkEntity(entity);
  }
  if(!(round%2)){for(int n=0;n<32;n++)Scr_FreeEntityNum(n,0);Scr_FreeEntityList();}
  for(int n=0;n<32;n++)checkEntity(entities[n]);
  RemoveRefToObject(players);RemoveRefToObject(level);
  RemoveRefToObject(scrVarPub.pauseArrayId);RemoveRefToObject(g_classMap[0].entArrayId);
  unsigned count=0;for(unsigned at=VG_U16(0);at;at=VG_U16(VG_ID(at)))assert(++count<=SCRVL_MAX_VARIABLES);
  assert(count==SCRVL_MAX_VARIABLES);assert(!strings[300]&&!strings[301]);
 }
}
int main(void){
 assert(sizeof(VariableValueInternal)==16);roundNotifications();
 puts("PASS: 2,560 real endon listeners, shared level notifications and 32-player round teardown retain exact entity references and recycle every slot");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-variable-collisions-') as d:
 p=Path(d);(p/'test.c').write_text(support+macros+''.join(function(n).split('{',1)[0].strip()+';\n' for n in names)+''.join(function(n) for n in names)+vm_body.replace('unsigned int arrayName = VG_NAME(id);', 'assert(VG_STATUS(id) & SCRVL_VAR_ALLOCATED);\n    unsigned int arrayName = VG_NAME(id);')+checks)
 subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True,timeout=60)
