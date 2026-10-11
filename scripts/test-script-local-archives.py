#!/usr/bin/env python3
"""Exercise production script local archives under concurrent waits and cancellation."""
import re, subprocess, tempfile
from pathlib import Path
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/script/scr_vm.c').read_text()
def function(name):
    m=re.search(r'^.*\b'+name+r'\([^;{}]*?\)\n\{',source,re.M)
    assert m,name
    start=m.start()
    if source[start:m.end()].lstrip().startswith(name+'('):start=source.rfind('\n',0,start-1)+1
    at=m.end();depth=1
    while depth:depth+=(source[at]=='{')-(source[at]=='}');at+=1
    return source[start:at]+'\n'
support=r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define __attribute_regparm__(n)
#define VAR_UNDEFINED 0
typedef unsigned char byte;
typedef union {int intValue;unsigned stackValue;const char *codePosValue;} VariableUnion;
typedef struct {VariableUnion u;int type;} VariableValue;
typedef struct __attribute__((packed)) {const char *pos;unsigned short size,localId,bufLen;byte time;char buf[1];} VariableStackBuffer;
static struct scrVarPub_t {unsigned time,timeArrayId;} varPub;
static const void *imp_scrVarPub=&varPub;
static struct {unsigned localVarCount;const char *pos;} frameFs;
static struct {struct {unsigned localVarCount;const char *pos;}fs;} frames[2048];
static struct {unsigned *localVars;int function_count;typeof(frames[0]) *function_frame;}scrVmPub;
static unsigned vars[512],cacheCount,fallbackCount,refs[65534];
static unsigned parents[65534];
static void *MT_Alloc(int n,int flags){(void)flags;void*p=calloc(1,n);assert(p);return p;}
static int MT_Realloc(int oldSize,int newSize){(void)oldSize;(void)newSize;return 0;}
static void MT_Free(void *p,int size){(void)size;free(p);}
#define SCR_STACK_ENC(p) ((unsigned)(uintptr_t)(p))
static void AddRefToValue(int type,VariableUnion value){(void)value;assert(type==6);}
static void RemoveRefToValue(int type,VariableUnion value){(void)value;assert(type==0||type==6);}
static unsigned killedFrame,queuedStack;
static void Scr_KillThread(unsigned id){killedFrame=id;}
static void RemoveRefToObject(unsigned id){(void)id;}
static void Scr_SetThreadWaitTime(unsigned id,unsigned time){(void)id;(void)time;}
static unsigned GetVariable(unsigned parent,unsigned key){(void)parent;(void)key;return 1;}
static unsigned GetArray(unsigned id){return id;}
static unsigned GetNewObjectVariable(unsigned parent,unsigned key){(void)parent;(void)key;return 1;}
static void SetNewVariableValue(unsigned id,VariableValue*value){assert(id==1&&value->type==10);queuedStack=value->u.stackValue;}
#define SCR_CODEPOS_GET(u) ((u).codePosValue)
static void Com_Error(int level,const char *s){(void)level;fprintf(stderr,"%s\n",s);abort();}
static unsigned GetParentLocalId(unsigned id){assert(id>0);return id-1;}
static unsigned GetSafeParentLocalId(unsigned id){assert(id<65534);return parents[id];}
static unsigned FindPrevSibling(unsigned id){(void)id;return 0;}
static unsigned VM_CurrentFrameLocalCacheCount(void){return cacheCount;}
static void AddRefToObject(unsigned id){assert(id<65534);refs[id]++;}
static unsigned VM_RestoreLocalVarsFromSibling(unsigned id){(void)id;fallbackCount++;return 0;}
'''
region=source[source.index('#define VM_STACKBUF_HEADER_SIZE'):source.index('\nvoid Scr_ClearErrorMessage')]
body=region+function('VM_ArchiveLocalVars')+function('VM_ArchiveStack')+function('VM_TerminateStack')+function('VM_NotifyAppendStackParams')+function('VM_RestoreArchivedFrameLocalVars')+function('VM_RestoreArchivedLocalVars')
checks=r'''
static VariableStackBuffer *archive(unsigned id,unsigned count,unsigned value){
 cacheCount=count;scrVmPub.localVars=vars+128;scrVmPub.function_count=1;scrVmPub.function_frame=frames+1;
 for(unsigned j=0;j<count;j++)scrVmPub.localVars[(int)j+1-(int)count]=value+j;
 VariableValue unused={{0},0};unsigned localId=id;
 VariableStackBuffer*p=VM_ArchiveStack(0,NULL,&unused,count,&localId);
 assert(p->localId==id&&localId==id&&refs[id]>0);return p;
}
static void restore(VariableStackBuffer*p,unsigned count,unsigned value){
 scrVmPub.localVars=vars+128;
 assert(VM_RestoreArchivedLocalVars(p)==count);
 for(unsigned j=0;j<count;j++)assert(vars[129+j]==value+j);
 assert(!vmLocalVarArchives[p->localId].localId);
 free(p);
}
int main(void){
 _Static_assert(offsetof(VariableStackBuffer,buf)==11,"original i386 stack ABI");
 // More sleeping scripts than the old 256-slot eviction ring can retain.
 VariableStackBuffer *live[1024];
 for(unsigned j=0;j<1024;j++)live[j]=archive(10+j,j%64+1,10000+j*40);
 for(unsigned j=0;j<1024;j++)restore(live[j],j%64+1,10000+j*40);
 assert(!fallbackCount);
 // Nested waits retain the caller's variable order as well as the callee's.
 // Sibling reconstruction can reverse the caller's cached variable indices.
 parents[60001]=60000;cacheCount=3;scrVmPub.localVars=vars+128;
 scrVmPub.function_count=2;scrVmPub.function_frame=frames+2;
 frames[1].fs.localVarCount=4;frames[1].fs.pos="caller";
 for(unsigned j=0;j<7;j++)vars[122+j]=500+j;
 VariableValue boundary={{0},7};unsigned nestedId=60001;
 VariableStackBuffer*nested=VM_ArchiveStack(1,"callee",&boundary,3,&nestedId);
 assert(nestedId==60000&&nested->localId==60001);
 scrVmPub.localVars=vars+128;
 assert(VM_RestoreArchivedFrameLocalVars(nested,60000)==4);
 assert(VM_RestoreArchivedLocalVars(nested)==3);
 for(unsigned j=0;j<7;j++)assert(vars[129+j]==500+j);
 free(nested);assert(!fallbackCount);
 // Cancelling a child resumes its caller. Keep that caller's exact locals
 // while invalidating the terminated child before its allocator id is reused.
 parents[60003]=60002;cacheCount=3;scrVmPub.localVars=vars+128;
 scrVmPub.function_count=2;scrVmPub.function_frame=frames+2;
 frames[1].fs.localVarCount=4;frames[1].fs.pos="resume caller";
 for(unsigned j=0;j<7;j++)vars[122+j]=800+j;
 nestedId=60003;nested=VM_ArchiveStack(1,"cancel child",&boundary,3,&nestedId);
 VM_TerminateStack(60003,60002,nested);
 assert(killedFrame==60003&&queuedStack==SCR_STACK_ENC(nested)&&nested->localId==60002);
 assert(!vmLocalVarArchives[60003].stackValue&&vmLocalVarArchives[60002].stackValue==nested);
 scrVmPub.localVars=vars+128;assert(VM_RestoreArchivedLocalVars(nested)==4);
 for(unsigned j=0;j<4;j++)assert(vars[129+j]==800+j);
 free(nested);assert(!fallbackCount);
 // A notification can enlarge and move a sleeping stack. Its saved local
 // variables must follow the allocation before the old storage is freed.
 for(unsigned j=0;j<256;j++){
  VariableStackBuffer*p=archive(2000+j,64,4000+j*70);
  VariableUnion ref={0};ref.stackValue=SCR_STACK_ENC(p);
  VariableValue args[2]={{{0},8},{{17},6}};
  VM_NotifyAppendStackParams(&ref,&p,args+1);
  assert(p->size==1&&ref.stackValue==SCR_STACK_ENC(p));
  VM_NotifyAppendStackParams(&ref,&p,args+1);
  assert(p->size==2&&vmLocalVarArchives[p->localId].stackValue==p);
  restore(p,64,4000+j*70);
 }
 assert(!fallbackCount);
 // The same id can refer to a cancelled frame and a different live stack.
 VariableStackBuffer*old=archive(7,3,10),*replacement=archive(7,5,20);
 VM_ForgetLocalVarArchive(old);
 assert(vmLocalVarArchives[7].stackValue==replacement);
 scrVmPub.localVars=vars+128;assert(VM_RestoreArchivedLocalVars(old)==0&&fallbackCount==1);
 restore(replacement,5,20);free(old);
 // Partial cancellation must discard its exact frame before id reuse.
 old=archive(65533,64,100);VM_ForgetLocalVarArchive(old);
 assert(!vmLocalVarArchives[65533].localId);
 replacement=archive(65533,2,300);restore(replacement,2,300);free(old);
 for(unsigned j=0;j<VM_LOCAL_ARCHIVE_SLOTS;j++)assert(!vmLocalVarArchives[j].localId);
 puts("PASS: 1,024 concurrent script waits retain exact locals, cancellation and stack identity protect reused ids, and highest valid id resumes correctly");
}
'''
# Both actual cancellation paths discard their archive before unwinding/freeing.
f=function('VM_TrimStack')
assert f.index('VM_ForgetLocalVarArchive(stackValue)')<f.index('while (count != size)')
f=function('VM_TerminateStack')
assert f.index('VM_ForgetArchivedFrame(stackValue, localId)')<f.index('Scr_KillThread(localId)')
assert 'memset(vmLocalVarArchives, 0, sizeof(vmLocalVarArchives))' in function('Scr_InitSystem')
assert 'VM_TerminateStack(0, startLocalId, stackValue)' in function('Scr_ShutdownSystem')
with tempfile.TemporaryDirectory(prefix='cod2-local-archives-') as d:
    p=Path(d);(p/'test.c').write_text(support+body+checks)
    subprocess.run(['cc','-std=gnu11','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True,timeout=30)
