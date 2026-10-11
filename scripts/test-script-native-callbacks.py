#!/usr/bin/env python3
"""Check reused VM frames when a native method invokes a yielding script callback."""
import re
import subprocess
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / 'src/PC/script/scr_vm.c').read_text()


def function(name):
    match = re.search(r'^.*\b' + name + r'\([^;{}]*?\)\n\{', source, re.M)
    assert match, name
    start, end = match.start(), match.end()
    if source[start:end].lstrip().startswith(name + '('):
        start = source.rfind('\n', 0, start - 1) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


support = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define __attribute_regparm__(n)
typedef union {int intValue;} VariableUnion;
typedef struct {VariableUnion u; int type;} VariableValue;
struct function_stack_t {
 const char *pos; unsigned localId,localVarCount; VariableValue *top,*startTop;
};
typedef struct {struct function_stack_t fs;} Frame;
static struct scrVarPub_t {unsigned tempVariable;} varPub;
static void *imp_scrVarPub=&varPub;
static unsigned char scrVmGlob[2048];
static struct {
 unsigned outparamcount,inparamcount; int function_count;
 unsigned *localVars; VariableValue *top;
 Frame *function_frame,*function_frame_start;
} scrVmPub;
static Frame frames[32];
static VariableValue stack[32];
static unsigned callerLocals;
static void RemoveRefToValue(int type,VariableUnion value){(void)type;(void)value;}
static void ClearVariableValue(unsigned id){(void)id;}
static void Scr_KillThread(unsigned id){(void)id;assert(0);}
static void RuntimeError(const char *pos,int index,const char *msg,void *unused){
 (void)pos;(void)index;(void)msg;(void)unused;assert(0);
}
static unsigned VM_Execute(struct function_stack_t fs);
'''
checks = r'''
static unsigned VM_Execute(struct function_stack_t fs) {
 // Production external-call setup must discard stale counts from both the
 // callback and the native boundary, preserving the suspended caller's base.
 assert(scrVmPub.function_frame->fs.localVarCount==0);
 if(callerLocals) assert((scrVmPub.function_frame-1)->fs.localVarCount==0);
 unsigned *before=scrVmPub.localVars;
 for(unsigned i=0;i<3;i++) *++scrVmPub.localVars=500+i;
 assert(VM_CurrentFrameLocalCacheCount()==3);
 // This is the cache unwind performed when the callback waits. A stale
 // boundary count undercounts it and leaves callback locals in the caller.
 scrVmPub.localVars-=VM_CurrentFrameLocalCacheCount();
 assert(scrVmPub.localVars==before);
 scrVmPub.function_frame--;scrVmPub.function_count--;
 return fs.localId;
}
int main(void) {
 unsigned *base=(unsigned *)(scrVmGlob+24);
 for(unsigned repeat=0;repeat<64;repeat++) {
  memset(&scrVmPub,0,sizeof(scrVmPub));memset(frames,0,sizeof(frames));
  callerLocals=repeat%2?7:0;
  scrVmPub.function_frame_start=frames;
  scrVmPub.function_count=callerLocals?1:0;
  scrVmPub.function_frame=frames+scrVmPub.function_count;
  frames[1].fs.localVarCount=callerLocals;
  // Dirty slots emulate unrelated callbacks that previously used these frames.
  frames[2].fs.localVarCount=9;frames[3].fs.localVarCount=17;
  if(!callerLocals)frames[1].fs.localVarCount=11;
  scrVmPub.localVars=base+callerLocals;
  for(unsigned i=1;i<=callerLocals;i++)base[i]=100+i;
  scrVmPub.top=stack+8;scrVmPub.inparamcount=2;
  assert(VM_ExecuteExtCall(41,"callback",2)==41);
  assert(scrVmPub.localVars==base+callerLocals);
  assert(scrVmPub.function_count==(callerLocals?1:0));
  assert(scrVmPub.function_frame==frames+scrVmPub.function_count);
  for(unsigned i=1;i<=callerLocals;i++)assert(base[i]==100+i);
 }
 puts("PASS: repeated native callback waits preserve caller locals and clear reused boundary/frame counts");
}
'''
body = function('VM_CurrentFrameLocalCacheCount') + function('VM_ExecuteExtCall')
# Every builtin shape must publish its current local count before reentrant code.
for call in ('VM_CandidateCallBuiltin(&pos, &top, opcode - VMOP_CallBuiltin0)',
             'VM_CandidateCallBuiltin(&pos, &top, paramCount)',
             'VM_CandidateCallBuiltinMethod(&pos, &top, opcode - VMOP_CallBuiltinMethod0)',
             'VM_CandidateCallBuiltinMethod(&pos, &top, paramCount)'):
    at = source.index(call)
    assert 'fs.localVarCount = localVarCount;' in source[at - 100:at]
with tempfile.TemporaryDirectory(prefix='cod2-native-callbacks-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(support + body + checks)
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True, timeout=30)
