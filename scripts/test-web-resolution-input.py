#!/usr/bin/env python3
"""Exercise real mouse routing across browser resolutions and UI placement."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parent.parent
source = (root/'src/PC/client_mp/cl_input.c').read_text()
start = source.index('void CL_MouseEventAbsolute(const int x,')
end = source.index('\nvoid CL_WriteVoicePacket', start)
code = r'''
#include <assert.h>
typedef struct {int keyCatchers, mouseIndex, mouseDx[2], mouseDy[2];} clientActive_t;
typedef struct {struct {int width,height;} vidConfig;} clientStatic_t;
typedef struct {struct {int enabled;} current;} dvar_t;
static clientActive_t client; static clientActive_t *clientPtr = &client;
static void *imp_cl = &clientPtr; static clientStatic_t cls;
static dvar_t bypass; static dvar_t *cl_bypassMouseInput = &bypass;
static int cursorX, cursorY;
static void UI_MouseEventAbsolute(int x,int y){cursorX=x;cursorY=y;}
'''+source[start:end]+r'''
int main(void){
 const int modes[][2]={{640,480},{800,600},{1024,768},{1280,720},{1600,900},{1920,1080}};
 for(int i=0;i<6;i++){
  cls.vidConfig.width=modes[i][0];cls.vidConfig.height=modes[i][1];
  client.keyCatchers=8;
  CL_MouseEventAbsolute(modes[i][0]/2,modes[i][1]/2,17,-23);
  assert(cursorX==320 && cursorY==240);
  CL_MouseEventAbsolute(modes[i][0],modes[i][1],17,-23);
  assert(cursorX==640 && cursorY==480);
  // A right-aligned HD menu hit must round-trip to its displayed position.
  if(i==3){CL_MouseEventAbsolute(850,347,0,0);assert(cursorX==425 && cursorY==231);}
  client.keyCatchers=0;client.mouseDx[0]=client.mouseDy[0]=0;
  CL_MouseEventAbsolute(900,400,17,-23);
  assert(client.mouseDx[0]==17 && client.mouseDy[0]==-23);
  client.keyCatchers=8;bypass.current.enabled=1;
  CL_MouseEventAbsolute(900,400,17,-23);
  assert(client.mouseDx[0]==34 && client.mouseDy[0]==-46);bypass.current.enabled=0;
 }
 cls.vidConfig.width=cls.vidConfig.height=0;client.keyCatchers=8;
 CL_MouseEventAbsolute(200,100,0,0);assert(cursorX==200 && cursorY==100);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-resolution-input-') as directory:
    p=Path(directory);(p/'test.c').write_text(code)
    subprocess.run(['cc','-D__EMSCRIPTEN__','-std=c99','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
print('PASS: UI hits scale correctly in all six resolutions; pointer-lock/gameplay deltas and bypass routing retain their sensitivity')
