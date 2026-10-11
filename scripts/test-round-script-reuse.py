#!/usr/bin/env python3
"""Round restarts retain compiled callbacks, field metadata and animation scripts."""
import re,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parent.parent
source=(root/'src/PC/game_mp/g_main_mp.c').read_text()
def block(text,at):
    start=text.rfind('    if (!restart) {',0,at)
    assert start>=0
    end=text.index('{',start)+1;depth=1
    while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    assert start<=at<end
    return text[start:end]
support=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte;
_Alignas(8) static unsigned char scrVarPub[128];
static unsigned char code[32];static unsigned scripts,anims,consts,fieldWrites;
static const char *str_dbg_load="load",*str_dbg_endload="end";
static void *Hunk_AllocLowInternal(int n){assert(n==0);fieldWrites++;return code;}
static void Scr_BeginLoadScripts(void){assert(!scripts);scripts++;code[0]=91;}
static void GScr_LoadScripts(int n){assert(n==1);code[7]=17;}
static void DBG_PrintFreeVars(const char *s){(void)s;}
static void Scr_PostCompileScripts(void){}static void Scr_EndLoadScripts(void){}
static void GScr_LoadConsts(void){consts++;}static void G_ParseHitLocDmgTable(void){}
static void Scr_FreeScripts(int n){assert(n==1);}static void Scr_BeginLoadAnimScripts(void){assert(!anims);anims++;}
static void GScr_LoadAnimScripts(void){}static void Scr_EndLoadAnimScripts(void){}
'''
checks=r'''
int main(void){
 load(0);assert(scripts==1&&anims==1&&consts==1&&fieldWrites==1&&code[0]==91&&code[7]==17);
 unsigned char saved[sizeof(code)];memcpy(saved,code,sizeof(code));void*field=*(void**)scrVarPub;
 for(int round=0;round<32;round++){load(1);assert(!memcmp(saved,code,sizeof(code)));assert(*(void**)scrVarPub==field);}
 assert(scripts==1&&anims==1&&consts==1&&fieldWrites==1);
 puts("PASS: 32 round restarts retain compiled callbacks, fields and animation scripts without recompiling");
}
'''
variants=[]
# Native and browser G_InitGame retain the same compiled state on restart.
positions=[m.start() for m in re.finditer(re.escape('        int *p = Hunk_AllocLowInternal(0);'),source)]
assert len(positions)==2
for index,at in enumerate(positions):
    hunk=block(source,at)
    scripts_at=source.index('        Scr_BeginLoadScripts();',at)
    anim_at=source.index('        GScr_LoadConsts();',scripts_at)
    blocks=hunk+'\n'+block(source,scripts_at)+'\n'+block(source,anim_at)
    variants.append((str(index),blocks))
with tempfile.TemporaryDirectory(prefix='cod2-round-code-') as d:
    p=Path(d)
    for name,blocks in variants:
        for mutant in (False,True):
            body=blocks.replace('if (!restart)','if (1)') if mutant else blocks
            c=p/(name+str(mutant)+'.c');exe=c.with_suffix('')
            c.write_text(support+'static void load(int restart){\n'+body+'\n}\n'+checks)
            subprocess.run(['cc','-std=c11','-O1','-g','-fsanitize=address,undefined',str(c),'-o',str(exe)],check=True)
            r=subprocess.run([str(exe)],capture_output=True,text=True,timeout=30)
            assert (r.returncode!=0)==mutant,r.stderr
            if not mutant:print(r.stdout.strip())
            else:print('PASS: repeated-recompile mutation invalidates the round lifecycle')
