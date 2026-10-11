#!/usr/bin/env python3
"""Active endon cancellation updates the C VM continuation like the reference VM."""
import re, subprocess, tempfile
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = (root / 'src/PC/script/scr_vm.c').read_text()
def function(name):
    match = re.search(r'^static [^;{}]*\b' + name + r'\([^;{}]*\)\n\{', source, re.M)
    assert match, name
    end = match.end(); depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}'); end += 1
    return source[match.start():end] + '\n'
support = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define VAR_POINTER 1
#define VAR_STRING 2
typedef union {unsigned pointerValue,stringValue;int intValue;} VariableUnion;
typedef struct {VariableUnion u;int type;} VariableValue;
typedef struct {struct {const char *pos;unsigned localId;}fs;} Frame;
static struct {int function_count;Frame *function_frame;Frame function_frame_start[32];}scrVmPub;
static char g_EndPos;
static const char code[]="continuation";
static unsigned cancelId,ownerReleases,stringReleases,valueReleases;
static unsigned GetSafeParentLocalId(unsigned id){return id==11?10:0;}
static int IsFieldObject(unsigned id){return id==22;}
static void Scr_SetErrorMessageAndJump(const char *s){fprintf(stderr,"%s\n",s);abort();}
static void RemoveRefToObject(unsigned id){assert(id==22);ownerReleases++;}
static void SL_RemoveRefToString(unsigned id){assert(id==33);stringReleases++;}
static void RemoveRefToValue(int type,VariableUnion v){assert(type==6&&v.intValue==44);valueReleases++;}
'''
notify = r'''
static void VM_Notify(unsigned owner,unsigned name,VariableValue *top){
 assert(owner==22&&name==33&&top->type==6);
 assert(scrVmPub.function_frame->fs.pos==code);
 if(cancelId)VM_NotifyEndLocalThreads(cancelId);
}
'''
checks = r'''
int main(void){
 for(unsigned mode=0;mode<3;mode++){
  scrVmPub.function_count=2;scrVmPub.function_frame=scrVmPub.function_frame_start+2;
  scrVmPub.function_frame_start[1].fs.localId=10;
  scrVmPub.function_frame_start[1].fs.pos=code+1;
  scrVmPub.function_frame->fs.localId=11;scrVmPub.function_frame->fs.pos=NULL;
  cancelId=mode==0?0:mode==1?11:10;
  VariableValue args[5]={{{0},0},{{0},8},{{44},6},{{33},2},{{22},1}};
  VariableValue*top=args+4;const char*pos=code;
  VM_CandidateHandleNotifyCall(&pos,&top);
  assert(top==args&&pos==(cancelId?&g_EndPos:code));
  assert(scrVmPub.function_frame_start[1].fs.pos==(mode==2?&g_EndPos:code+1));
 }
 assert(ownerReleases==3&&stringReleases==3&&valueReleases==3);
 puts("PASS: notifications preserve normal continuation and immediately terminate current/ancestor endon frames; arguments release once");
}
'''
body = support + function('VM_NotifyEndLocalThreads') + notify + function('VM_CandidateHandleNotifyCall') + checks
with tempfile.TemporaryDirectory(prefix='cod2-notify-continuation-') as directory:
    path=Path(directory)
    for mutant in (False,True):
        text=body.replace('*pos = scrVmPub.function_frame->fs.pos;', '/* stale continuation */') if mutant else body
        c=path/('mutant.c' if mutant else 'test.c'); exe=c.with_suffix('')
        c.write_text(text)
        subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(c),'-o',str(exe)],check=True)
        result=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30)
        assert (result.returncode!=0)==mutant,result.stderr
        print('PASS: stale-continuation mutation is rejected' if mutant else result.stdout.strip())
