#!/usr/bin/env python3
"""Run the real frame/input dispatch, including the render-dependent active send."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
main = (root / 'src/PC/client_mp/cl_main_mp.c').read_text()
common = (root / 'src/PC/qcommon/common.c').read_text()
inputs = (root / 'src/PC/client_mp/cl_input.c').read_text()

def function(source, name):
    start = source.index('void ' + name + '(void)\n{')
    end = source.index('\n}', start) + 2
    return source[start:end]

start = main.index('\nLtail:\n', main.index('void CL_Frame(int msec)\n{'))
tail = main[start:main.index('\n}', start)]
start = common.index('    CL_Frame(maxMsec);', common.index('void Com_Frame_Try_Block_Function(void)'))
render = common[start:common.index('    SCR_RunCinematic();', start)]
support = r'''
#include <assert.h>
#include <stdio.h>
#define CA_ACTIVE 8
#define EM_ASM_INT(...) hidden
struct Connection {int state;} clientConnections[1];
typedef struct Connection clientConnection_t;
struct Connection *connection=&clientConnections[0];
void *imp_clc=&connection;
int hidden,sends,renders,syncs,clocks;
void CL_SendCmdInternal(void){sends++;}
void CL_SyncGpu(void){syncs++;}
void CL_SetCGameTime(void){clocks++;}
void CL_SwitchToLocalClient(int client){assert(client==0);}
'''
code = support + function(inputs, 'CL_Input') + '\n' + function(inputs, 'CL_SendCmd')
code += '\nvoid CL_Frame(int msec){struct Connection *clc_p=connection;\n' + tail + '\n}\n'
code += 'void SCR_UpdateScreenInternal(void){renders++;CL_Input();}\n'
code += 'void Frame(void){int maxMsec=250;\n' + render + '\n}\n'
code += r'''
int main(void){
 for(int state=0;state<=CA_ACTIVE;state++)for(hidden=0;hidden<=1;hidden++){
  connection->state=state;sends=renders=syncs=clocks=0;
  // Six minutes of real dispatch. Every connected state must keep sending.
  for(int frame=0;frame<1440;frame++)Frame();
  assert(clocks==1440);
  assert(sends==(state>4?1440:0));
#ifdef __EMSCRIPTEN__
  assert(renders==(hidden?0:1440));
  assert(syncs==(!hidden&&state==CA_ACTIVE?1440:0));
#else
  assert(renders==1440);
  assert(syncs==(state==CA_ACTIVE?1440:0));
#endif
 }
 puts("PASS: active/connecting clients send exactly once per frame; hidden web frames skip rendering/GPU sync; native dispatch is unchanged");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-background-frame-') as tmp:
    source = Path(tmp) / 'test.c'
    mutant = code.replace('        CL_SendCmdInternal();\n#endif', '        ;\n#endif')
    assert mutant != code
    for name, variant, flags in [('web', code, ['-D__EMSCRIPTEN__']),
                                 ('native', code, []),
                                 ('render-only-mutant', mutant, ['-D__EMSCRIPTEN__'])]:
        source.write_text(variant)
        executable = source.with_suffix('')
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined',
                        *flags, str(source), '-o', str(executable)], check=True)
        result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
        assert (result.returncode == 0) == (name != 'render-only-mutant'), result.stderr
        print(result.stdout.strip() if result.returncode == 0 else
              'PASS: render-only active-command dispatch mutant rejected')
