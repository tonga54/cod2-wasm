#!/usr/bin/env python3
"""Exercise the actual controller polling and binding code with simulated pads."""
from pathlib import Path
import re
import subprocess
import tempfile
import sys

root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / 'scripts'))
from browser_settings_menu import apply_settings_menu


def run(code, label):
    with tempfile.TemporaryDirectory(prefix='cod2-gamepad-') as directory:
        path = Path(directory)
        (path / 'test.c').write_text(code)
        subprocess.run(['cc', '-std=c11', '-fsanitize=address,undefined',
                        str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
        subprocess.run([str(path / 'test')], check=True)
    print('PASS:', label)


source = (root / 'src/PC/client_mp/cl_gamepad.c').read_text()
source = re.sub(r'^\s*#\s*include[^\n]*', '', source, flags=re.M)
prefix = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#define COD2_FEATURE_GAMEPAD 1
#define COD2_FEATURE_RUMBLE 0
#define __EMSCRIPTEN__ 1
#define EM_ASM_INT(...) focused
static int focused=1;
typedef int qboolean;
typedef int sysEventType_t;
#define SE_KEY 1
#define BUTTON_SPRINT 2
typedef struct {int keyCatchers,stanceHeld,stancePosition,stanceTime,usingAds;struct {struct {int pm_type;} ps;} snap;float viewangles[3],cgameSensitivity,cgameMaxYawSpeed,cgameMaxPitchSpeed;} clientActive_t;
typedef struct {int frametime,realtime;} clientStatic_t;
clientStatic_t cls;
static clientActive_t client,*clientPtr=&client;
void *imp_cl=&clientPtr;
typedef struct {union {int enabled;float value;const char *string;} current;} dvar_t;
static struct {char name[80];dvar_t dvar;} vars[16];
static int numVars;
static dvar_t *var(const char *name) {
 for(int i=0;i<numVars;i++) if(!strcmp(vars[i].name,name)) return &vars[i].dvar;
 strcpy(vars[numVars].name,name);return &vars[numVars++].dvar;
}
const dvar_t *Dvar_RegisterBool_mac(const char *n,int v,int f){
 for(int i=0;i<numVars;i++)if(!strcmp(vars[i].name,n))return &vars[i].dvar;
 dvar_t *d=var(n);d->current.enabled=v;return d;
}
const dvar_t *Dvar_RegisterFloat(const char *n,float v,float a,float b,int f){
 for(int i=0;i<numVars;i++)if(!strcmp(vars[i].name,n))return &vars[i].dvar;
 dvar_t *d=var(n);d->current.value=v;return d;
}
void Dvar_SetBoolByName(const char *n,int v){var(n)->current.enabled=v;}
static const char *bindings[256];
void Key_SetBinding(int k,const char *c){bindings[k]=c;}
const char *Key_GetBinding(int k){return bindings[k];}
void Cbuf_AddText(const char *s){(void)s;}
void Com_Printf(const char *fmt,...){(void)fmt;}
static int pending;
qboolean Display_KeyBindPending(void){return pending;}
static int classMenu;
qboolean UI_ClassMenuFocused(void){return classMenu;}
/* The focused menu takes every key, as the pause menu does. */
static int menuTakesKeys;
qboolean UI_checkKeyExec(int key){(void)key;return menuTakesKeys;}
static struct {int key,down;} events[1024];static int eventCount;
void Sys_QueEvent(int t,sysEventType_t type,int key,int down,int len,void *p){
 assert(eventCount<1024);events[eventCount].key=key;events[eventCount++].down=down;
}
typedef struct {int buttons;signed char rightmove,forwardmove;} usercmd_t;
typedef struct {int buttons[16],axes[6];} SDL_GameController;
typedef int SDL_JoystickID;
static SDL_GameController pad;
static int present=1;
enum {SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_X,
 SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_GUIDE,
 SDL_CONTROLLER_BUTTON_START,SDL_CONTROLLER_BUTTON_LEFTSTICK,SDL_CONTROLLER_BUTTON_RIGHTSTICK,
 SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
 SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN,
 SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,SDL_CONTROLLER_BUTTON_MAX};
enum {SDL_CONTROLLER_AXIS_LEFTX,SDL_CONTROLLER_AXIS_LEFTY,SDL_CONTROLLER_AXIS_RIGHTX,
 SDL_CONTROLLER_AXIS_RIGHTY,SDL_CONTROLLER_AXIS_TRIGGERLEFT,SDL_CONTROLLER_AXIS_TRIGGERRIGHT};
enum {SDL_INIT_GAMECONTROLLER,SDL_CONTROLLERDEVICEADDED,SDL_CONTROLLERDEVICEREMOVED};
static int SDL_InitSubSystem(int a){return 0;}
static void SDL_QuitSubSystem(int a){}
static int SDL_NumJoysticks(void){return present;}
static int SDL_IsGameController(int a){return present;}
static SDL_GameController *SDL_GameControllerOpen(int a){return &pad;}
static void SDL_GameControllerClose(SDL_GameController *p){}
static SDL_GameController *SDL_GameControllerGetJoystick(SDL_GameController *p){return p;}
static int SDL_JoystickInstanceID(SDL_GameController *p){return 0;}
static const char *SDL_GameControllerName(SDL_GameController *p){return "Test pad";}
static const char *SDL_GetError(void){return "";}
static int SDL_GameControllerGetAxis(SDL_GameController *p,int a){return p->axes[a];}
static int SDL_GameControllerGetButton(SDL_GameController *p,int b){return p->buttons[b];}
'''
run(prefix + source + r'''
int main(void){
 client.cgameSensitivity=1.0f;CL_Gamepad_Init();assert(CL_Gamepad_IsConnected());
 assert(!strcmp(bindings[K_BUTTON_RTRIG],"+attack"));
 assert(!strcmp(bindings[K_BUTTON_B],"+stance"));
 // Existing defaults migrate once; saved remaps and unbound controls survive.
 Key_SetBinding(K_BUTTON_B,"togglecrouch");var("cl_gamepadStanceInitialized")->current.enabled=0;
 CL_Gamepad_Shutdown();CL_Gamepad_Init();assert(!strcmp(bindings[K_BUTTON_B],"+stance"));
 Key_SetBinding(K_BUTTON_A,"+melee");Key_SetBinding(K_BUTTON_B,"");
 CL_Gamepad_Shutdown();CL_Gamepad_Init();
 assert(!strcmp(bindings[K_BUTTON_A],"+melee") && !bindings[K_BUTTON_B][0]);
 // Raw capture includes face buttons, d-pad and both triggers.
 client.keyCatchers=8;CL_Gamepad_Frame();eventCount=0;pending=1;
 pad.buttons[SDL_CONTROLLER_BUTTON_A]=1;CL_Gamepad_Frame();
 assert(eventCount==1 && events[0].key==K_BUTTON_A && events[0].down);
 pending=0;pad.buttons[SDL_CONTROLLER_BUTTON_A]=0;CL_Gamepad_Frame();
 assert(events[1].key==K_BUTTON_A && !events[1].down);
 eventCount=0;pad.buttons[SDL_CONTROLLER_BUTTON_A]=1;CL_Gamepad_Frame();
 assert(events[0].key==K_ENTER && events[0].down);
 pending=1;pad.buttons[SDL_CONTROLLER_BUTTON_A]=0;CL_Gamepad_Frame();
 assert(events[1].key==K_ENTER && !events[1].down);
 // The bumpers reach a menu that takes every key, as the pause menu's tabs;
 // any other menu would run their gameplay binds, so they stay quiet there.
 pending=0;eventCount=0;pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSHOULDER]=1;CL_Gamepad_Frame();
 pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSHOULDER]=0;CL_Gamepad_Frame();assert(eventCount==0);
 menuTakesKeys=1;
 pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSHOULDER]=1;CL_Gamepad_Frame();
 assert(eventCount==1 && events[0].key==K_BUTTON_LSHLDR && events[0].down);
 pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSHOULDER]=0;CL_Gamepad_Frame();
 assert(eventCount==2 && events[1].key==K_BUTTON_LSHLDR && !events[1].down);
 pad.buttons[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]=1;CL_Gamepad_Frame();
 assert(eventCount==3 && events[2].key==K_BUTTON_RSHLDR && events[2].down);
 pad.buttons[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]=0;CL_Gamepad_Frame();
 client.keyCatchers=8|0x10;eventCount=0;
 pad.buttons[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]=1;CL_Gamepad_Frame();
 pad.buttons[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]=0;CL_Gamepad_Frame();assert(eventCount==0);
 client.keyCatchers=8;menuTakesKeys=0;pending=1;
 // Y is the class screens' Customize; other menus get nothing for it.
 pending=0;eventCount=0;pad.buttons[SDL_CONTROLLER_BUTTON_Y]=1;CL_Gamepad_Frame();
 pad.buttons[SDL_CONTROLLER_BUTTON_Y]=0;CL_Gamepad_Frame();assert(eventCount==0);
 classMenu=1;pad.buttons[SDL_CONTROLLER_BUTTON_Y]=1;CL_Gamepad_Frame();
 assert(eventCount==1 && events[0].key==K_BUTTON_Y && events[0].down);
 classMenu=0;pad.buttons[SDL_CONTROLLER_BUTTON_Y]=0;CL_Gamepad_Frame();
 assert(eventCount==2 && events[1].key==K_BUTTON_Y && !events[1].down);
 pending=1;eventCount=2;
 pad.axes[4]=pad.axes[5]=32767;CL_Gamepad_Frame();
 assert(g_buttonDown[K_BUTTON_LTRIG] && g_buttonDown[K_BUTTON_RTRIG]);
 // Opening a menu releases attack; held controls wait for release.
 client.keyCatchers=0;CL_Gamepad_Frame();
 assert(!g_buttonDown[K_BUTTON_RTRIG]);
 CL_Gamepad_Frame();assert(!g_buttonDown[K_BUTTON_RTRIG]);
 pad.axes[4]=pad.axes[5]=0;CL_Gamepad_Frame();
 pad.axes[5]=32767;CL_Gamepad_Frame();assert(g_buttonDown[K_BUTTON_RTRIG]);
 focused=0;CL_Gamepad_Frame();assert(!g_buttonDown[K_BUTTON_RTRIG]);
 focused=1;CL_Gamepad_Frame();assert(!g_buttonDown[K_BUTTON_RTRIG]);
 pad.axes[5]=0;CL_Gamepad_Frame();pad.axes[5]=32767;CL_Gamepad_Frame();
 var("cl_gamepadEnable")->current.enabled=0;CL_Gamepad_Frame();
 assert(!g_buttonDown[K_BUTTON_RTRIG]);var("cl_gamepadEnable")->current.enabled=1;
 pad.axes[5]=0;CL_Gamepad_Frame();pad.axes[5]=32767;CL_Gamepad_Frame();
 CL_Gamepad_HandleDeviceEvent(SDL_CONTROLLERDEVICEREMOVED,0);
 assert(!CL_Gamepad_IsConnected() && !g_buttonDown[K_BUTTON_RTRIG]);
 CL_Gamepad_HandleDeviceEvent(SDL_CONTROLLERDEVICEADDED,0);
 pad.axes[5]=0;CL_Gamepad_Frame();
 // A stick click latches sprint after release, until neutral or an action.
 usercmd_t run={0};pad.axes[1]=-32767;
 pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSTICK]=1;CL_Gamepad_Frame();CL_Gamepad_Move(&run);
 assert(run.buttons&BUTTON_SPRINT);
 pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSTICK]=0;CL_Gamepad_Frame();
 for(int i=0;i<600;i++){run=(usercmd_t){0};CL_Gamepad_Move(&run);assert(run.buttons&BUTTON_SPRINT);}
 pad.axes[1]=0;run=(usercmd_t){0};CL_Gamepad_Move(&run);assert(!(run.buttons&BUTTON_SPRINT));
 pad.axes[1]=-32767;run=(usercmd_t){0};CL_Gamepad_Move(&run);assert(!(run.buttons&BUTTON_SPRINT));
 const int stops[]={1,4,0x100,0x200,0x1000,0x10000};
 for(int i=0;i<6;i++){
  pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSTICK]=1;CL_Gamepad_Frame();run=(usercmd_t){0};CL_Gamepad_Move(&run);
  assert(run.buttons&BUTTON_SPRINT);pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSTICK]=0;CL_Gamepad_Frame();
  run=(usercmd_t){.buttons=stops[i]};CL_Gamepad_Move(&run);assert(!(run.buttons&BUTTON_SPRINT));
  run=(usercmd_t){0};CL_Gamepad_Move(&run);assert(!(run.buttons&BUTTON_SPRINT));
 }
 // Remapping sprint retains click-to-run, while other L3 assignments do not.
 Key_SetBinding(K_BUTTON_A,"+sprint");Key_SetBinding(K_BUTTON_LSTICK,"+melee");
 pad.buttons[SDL_CONTROLLER_BUTTON_A]=1;CL_Gamepad_Frame();run=(usercmd_t){0};CL_Gamepad_Move(&run);
 assert(run.buttons&BUTTON_SPRINT);pad.buttons[SDL_CONTROLLER_BUTTON_A]=0;CL_Gamepad_Frame();
 focused=0;CL_Gamepad_Frame();focused=1;run=(usercmd_t){0};CL_Gamepad_Move(&run);
 assert(!(run.buttons&BUTTON_SPRINT));
 pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSTICK]=1;CL_Gamepad_Frame();run=(usercmd_t){0};CL_Gamepad_Move(&run);
 assert(!(run.buttons&BUTTON_SPRINT));pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSTICK]=0;CL_Gamepad_Frame();
 Key_SetBinding(K_BUTTON_B,"+stance");pad.buttons[SDL_CONTROLLER_BUTTON_B]=1;CL_Gamepad_Frame();
 client.stanceHeld=1;client.keyCatchers=8;CL_Gamepad_Frame();assert(!client.stanceHeld);
 pad.buttons[SDL_CONTROLLER_BUTTON_B]=0;CL_Gamepad_Frame();client.keyCatchers=0;CL_Gamepad_Frame();
 // Analog motion preserves small fractions, deadzone and frame-rate independence.
 usercmd_t cmd={0};cls.frametime=10;
 pad.axes[0]=pad.axes[1]=1000;CL_Gamepad_Move(&cmd);
 assert(cmd.forwardmove==0 && cmd.rightmove==0);
 pad.axes[0]=32767;pad.axes[1]=-32767;CL_Gamepad_Move(&cmd);
 assert(cmd.forwardmove>80 && cmd.rightmove>80);
 pad.axes[0]=pad.axes[1]=0;pad.axes[2]=pad.axes[3]=16384;
 client.viewangles[0]=client.viewangles[1]=0;
 for(int i=0;i<100;i++)CL_Gamepad_Move(&cmd);
 float yaw=client.viewangles[1],pitch=client.viewangles[0];
 assert(yaw<0 && pitch>0);
 client.viewangles[0]=client.viewangles[1]=0;cls.frametime=20;
 for(int i=0;i<50;i++)CL_Gamepad_Move(&cmd);
 assert(fabsf(client.viewangles[1]-yaw)<.001f);
 var("cl_gamepadInvertY")->current.enabled=1;client.viewangles[0]=0;
 CL_Gamepad_Move(&cmd);assert(client.viewangles[0]<0);
 client.cgameMaxYawSpeed=1.0f;client.cgameMaxPitchSpeed=2.0f;
 yaw=client.viewangles[1];pitch=client.viewangles[0];CL_Gamepad_Move(&cmd);
 assert(fabsf(client.viewangles[1]-yaw)<=.0201f && fabsf(client.viewangles[0]-pitch)<=.0401f);
 focused=0;yaw=client.viewangles[1];CL_Gamepad_Move(&cmd);assert(client.viewangles[1]==yaw);
 focused=1;client.keyCatchers=8;CL_Gamepad_Frame();pending=0;eventCount=0;
 pad.axes[1]=-32767;CL_Gamepad_Frame();assert(events[0].key==K_UPARROW);
 cls.realtime=351;CL_Gamepad_Frame();assert(eventCount==2 && events[1].down);
 pending=1;CL_Gamepad_Frame();assert(!events[2].down);
 CL_Gamepad_Shutdown();return 0;
}
''', 'controller hotplug, saved mapping migration, click-to-sprint/cancellation, raw capture, bumpers for a menu '
     'that takes every key, triggers, focus, analog sticks and repeat')

# Vibration reaches the pad only while it is the device being played with, in
# a focused page, with the controller and its vibration both enabled; still
# motors are told once.
rumble_prefix = prefix.replace('#define COD2_FEATURE_RUMBLE 0', '#define COD2_FEATURE_RUMBLE 1') + r'''
static float hapticStrong=-1,hapticWeak=-1;static int hapticCalls;
void WebHaptics_Rumble(float strong,float weak){hapticStrong=strong;hapticWeak=weak;hapticCalls++;}
'''
rumble_main = r'''
static void press(void){
 pad.buttons[SDL_CONTROLLER_BUTTON_A]=1;CL_Gamepad_Frame();pad.buttons[SDL_CONTROLLER_BUTTON_A]=0;CL_Gamepad_Frame();
}
/* Turning motors stop once the condition holds, and stay still while it does. */
static void silenced(void (*apply)(int)){
 int calls=hapticCalls;
 CL_Gamepad_SetRumble(.8f,.4f);assert(hapticCalls==calls+1&&hapticStrong==.8f&&hapticWeak==.4f);
 apply(1);
 CL_Gamepad_SetRumble(.8f,.4f);assert(hapticCalls==calls+2&&!hapticStrong&&!hapticWeak);
 CL_Gamepad_SetRumble(.8f,.4f);assert(hapticCalls==calls+2);
 apply(0);
}
static void setting(int on){var("cl_gamepadRumble")->current.enabled=!on;}
static void controller(int on){var("cl_gamepadEnable")->current.enabled=!on;}
static void blur(int on){focused=!on;}
static void keyboard(int on){if(on)CL_Gamepad_NoteKbmActive();else press();}
static void unplug(int on){
 CL_Gamepad_HandleDeviceEvent(on?SDL_CONTROLLERDEVICEREMOVED:SDL_CONTROLLERDEVICEADDED,0);if(!on)press();
}
int main(void){
 client.cgameSensitivity=1;CL_Gamepad_Init();
 assert(var("cl_gamepadRumble")->current.enabled);
 CL_Gamepad_SetRumble(1,.5f);CL_Gamepad_SetRumble(0,0);assert(hapticCalls==0);
 press();
 CL_Gamepad_SetRumble(1,.5f);assert(hapticCalls==1&&hapticStrong==1&&hapticWeak==.5f);
 CL_Gamepad_SetRumble(2,-1);assert(hapticCalls==2&&hapticStrong==1&&hapticWeak==0);
 CL_Gamepad_SetRumble(NAN,.25f);assert(hapticCalls==3&&hapticStrong==0&&hapticWeak==.25f);
 CL_Gamepad_SetRumble(0,0);assert(hapticCalls==4&&!hapticStrong&&!hapticWeak);
 CL_Gamepad_SetRumble(0,0);CL_Gamepad_SetRumble(-1,0);assert(hapticCalls==4);
 silenced(setting);silenced(controller);silenced(blur);silenced(keyboard);silenced(unplug);
 CL_Gamepad_SetRumble(.8f,.4f);assert(hapticStrong==.8f&&hapticWeak==.4f);
 // Firing with a trigger after touching the mouse is controller play too.
 for(int axis=4;axis<=5;axis++){
  CL_Gamepad_NoteKbmActive();assert(!CL_Gamepad_IsActiveDevice());
  pad.axes[axis]=32767;CL_Gamepad_Frame();assert(CL_Gamepad_IsActiveDevice());
  pad.axes[axis]=0;CL_Gamepad_Frame();
 }
 CL_Gamepad_Shutdown();return 0;
}
'''
run(rumble_prefix + source + rumble_main, 'vibration only from the pad in use, in a focused page, with the controller '
    'and its vibration on; clamped motors, told once when still')


def passes(code):
    with tempfile.TemporaryDirectory(prefix='cod2-gamepad-') as directory:
        path = Path(directory)
        (path / 'test.c').write_text(code)
        subprocess.run(['cc', '-std=c11', '-w', '-fsanitize=address,undefined', str(path / 'test.c'), '-lm',
                        '-o', str(path / 'test')], check=True)
        return subprocess.run([str(path / 'test')], capture_output=True).returncode == 0


rumble_mutants = [
    ('!cl_gamepadRumble->current.enabled ||', ''),
    ('!CL_Gamepad_IsActiveDevice() || ', ''),
    (' || !Gamepad_HasFocus()))', '))'),
    ('!g_gamepadInit || !cl_gamepadEnable->current.enabled || ', '!g_gamepadInit || '),
    ('    if (lowFreq == 0.0f && highFreq == 0.0f && !g_rumbleSent)\n        return;\n', ''),
    ('    g_rumbleSent = lowFreq > 0.0f || highFreq > 0.0f;', '    g_rumbleSent = 1;'),
    ('    if (!(lowFreq > 0.0f))', '    if (lowFreq < 0.0f)'),
    ('    else if (lowFreq > 1.0f)\n        lowFreq = 1.0f;\n', ''),
    ('        if ((ltrig && !g_buttonDown[K_BUTTON_LTRIG]) || (rtrig && !g_buttonDown[K_BUTTON_RTRIG]))\n'
     '            g_gamepadActiveDevice = 1;\n', ''),
    ('(ltrig && !g_buttonDown[K_BUTTON_LTRIG]) || ', ''),
]
for old, new in rumble_mutants:
    assert source.count(old) == 1, old
    assert not passes(rumble_prefix + source.replace(old, new) + rumble_main), old
print(f'PASS: {len(rumble_mutants)} vibration gating mutants fail')

def input_function(name):
    text = (root / 'src/PC/client_mp/cl_input.c').read_text()
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', text, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'

stance_support = r'''
typedef struct {int down[2],downtime,msec;unsigned char active,wasPressed,padding[2];} kbutton_t;
static kbutton_t kb[28];
typedef struct {int cl_stance,cl_stanceTemp;} LegacyHacks;
static LegacyHacks legacy,*legacyPtr=&legacy;
static void *imp_legacyHacks=&legacyPtr;
static int frameTime;
static void *imp_com_frameTime=&frameTime;
static struct {struct {int integer;} current;} hold={.current.integer=300},*cl_stanceHoldTime=&hold;
typedef unsigned char byte;
static int CL_KeyMoveValue(kbutton_t *b){return b->active?127:0;}
static int ClampChar(int n){return n>127?127:n<-127?-127:n;}
static int PM_IsBinocularsADS(void *ps){return 0;}
'''
stance_body = ''.join(input_function(n) for n in (
    'IN_Stance_Down', 'CL_UpdateHeldStance', 'IN_Stance_Up', 'CL_KeyMove'))
run(prefix + source + stance_support + stance_body + r'''
int main(void) {
 client.cgameSensitivity=1;CL_Gamepad_Init();
 int cases=0;
 for(int stance=1;stance<=2;stance++)for(int held=0;held<4;held++)
 for(int delay=0;delay<=240;delay+=16) {
  Gamepad_ReleaseAll();memset(&pad,0,sizeof(pad));CL_Gamepad_Frame();
  memset(kb,0,sizeof(kb));eventCount=0;legacy=(LegacyHacks){stance,1};
  frameTime=cls.realtime=1000;client.stanceHeld=0;
  if(held<3)kb[(int[]){11,25,26}[held]]=(kbutton_t){.active=1,.down={32}};
  else {IN_Stance_Down();frameTime+=310;CL_UpdateHeldStance();}
  pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSTICK]=1;CL_Gamepad_Frame();
  usercmd_t cmd={0};CL_KeyMove(&cmd); // Click before the stick leaves neutral.
  assert(!cmd.forwardmove);
  pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSTICK]=0;CL_Gamepad_Frame();
  cls.realtime+=delay;pad.axes[1]=-32767;cmd=(usercmd_t){0};CL_KeyMove(&cmd);
  assert((cmd.buttons&BUTTON_SPRINT)&&!(cmd.buttons&0x300)&&cmd.forwardmove==127);
  assert(!legacy.cl_stance&&!legacy.cl_stanceTemp&&!client.stanceHeld);
  assert(!kb[11].down[0]&&!kb[25].down[0]&&!kb[26].down[0]);
  IN_Stance_Up(); // Releasing the old long hold cannot put us back on the floor.
  for(int i=0;i<120;i++){cmd=(usercmd_t){0};CL_KeyMove(&cmd);assert((cmd.buttons&BUTTON_SPRINT)&&!(cmd.buttons&0x300));}
  // A new crouch/prone command intentionally cancels sprint.
  legacy.cl_stance=stance;cmd=(usercmd_t){0};CL_KeyMove(&cmd);assert(!(cmd.buttons&BUTTON_SPRINT));
  cases++;
 }
 Gamepad_ReleaseAll();memset(&pad,0,sizeof(pad));CL_Gamepad_Frame();
 legacy.cl_stance=2;cls.realtime=5000;
 pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSTICK]=1;CL_Gamepad_Frame();
 usercmd_t cmd={0};CL_KeyMove(&cmd);
 pad.buttons[SDL_CONTROLLER_BUTTON_LEFTSTICK]=0;CL_Gamepad_Frame();
 cls.realtime+=251;pad.axes[1]=-32767;cmd=(usercmd_t){0};CL_KeyMove(&cmd);
 assert(!(cmd.buttons&BUTTON_SPRINT)&&legacy.cl_stance==2);
 printf("PASS: %d combined controller/stance replays, buffered click, sustained sprint, stance release and timeout\n",cases);
 return 0;
}
''', 'production gamepad movement and posture command integration')

ui = (root / 'src/PC/ui_mp/ui_shared_mp.c').read_text()
signature = 'qboolean Item_Bind_HandleKey(displayContextDef_t *dc, itemDef_t *item, int key, qboolean down)\n{'
a=ui.index(signature);b=ui.index('\nstatic int Item_HandleKey_RectContainsPoint',a)
bind=ui[a:b]
run(r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
#define COD2_FEATURE_GAMEPAD 1
typedef int qboolean;typedef unsigned char byte;
typedef struct {float x,y,w,h;int horzAlign,vertAlign;} rectDef_t;
typedef struct {const char *name;} windowDef_t;
typedef struct {windowDef_t window;} menuDef_t;
typedef struct {rectDef_t rect;const char *dvar;menuDef_t *parent;} itemDef_t;
typedef struct displayContextDef_s {int cursorx,cursory;} displayContextDef_t;
typedef displayContextDef_t displayContextDef_s;
typedef struct {const char *command;int defaultbind1,defaultbind2,bind1,bind2;} bind_t;
static bind_t g_bindings[56];static int g_waitingForKey,saves;static itemDef_t *g_bindItem;
static char bindings[256][256];
static int I_stricmp(const char *a,const char *b){return strcmp(a,b);}
static void Key_SetBinding(int key,const char *value){assert(key>=0 && key<256);strcpy(bindings[key],value);}
static void Key_GetBindingBuf(int key,char *buf,int size){snprintf(buf,size,"%s",bindings[key]);}
static void Cbuf_ExecuteText(int a,const char *s){saves++;}
static void Controls_SetConfig(int restart){
 for(int i=0;i<56;i++)if(g_bindings[i].command){
  if(g_bindings[i].bind1!=-1)Key_SetBinding(g_bindings[i].bind1,g_bindings[i].command);
  if(g_bindings[i].bind2!=-1)Key_SetBinding(g_bindings[i].bind2,g_bindings[i].command);
 }saves++;
}
static void CalcScreenX(float *x,int a){}
static void CalcScreenY(float *x,int a){}
static void CalcScreenPlacement(float *x,float *y,float *w,float *h,int a,int b){}
''' + bind + r'''
int main(void){
 displayContextDef_t dc={0};menuDef_t menu={{"browser_gamepad"}};
 itemDef_t row={.dvar="+attack",.parent=&menu};
 for(int i=0;i<56;i++)g_bindings[i]=(bind_t){.bind1=-1,.bind2=-1};
 g_bindings[0]=(bind_t){"+attack",-1,-1,200,201};
 Key_SetBinding(200,"+attack");Key_SetBinding(201,"+attack");Key_SetBinding(207,"+gostand");
 assert(Item_Bind_HandleKey(&dc,&row,13,1) && g_waitingForKey);
 Item_Bind_HandleKey(&dc,&row,207,0);assert(g_waitingForKey && !saves);
 Item_Bind_HandleKey(&dc,&row,'x',1);assert(g_waitingForKey && !saves);
 Item_Bind_HandleKey(&dc,&row,207,1);assert(!g_waitingForKey && saves==1);
 assert(!strcmp(bindings[207],"+attack"));
 assert(!strcmp(bindings[200],"+attack") && !strcmp(bindings[201],"+attack"));
 Item_Bind_HandleKey(&dc,&row,13,1);Item_Bind_HandleKey(&dc,&row,222,1);
 assert(!bindings[207][0] && !strcmp(bindings[222],"+attack"));
 Item_Bind_HandleKey(&dc,&row,13,1);Item_Bind_HandleKey(&dc,&row,27,1);
 assert(!g_waitingForKey && !strcmp(bindings[222],"+attack"));
 Item_Bind_HandleKey(&dc,&row,13,1);Item_Bind_HandleKey(&dc,&row,127,1);
 assert(!bindings[222][0] && !strcmp(bindings[200],"+attack"));
 menu.window.name="options_shoot";
 Item_Bind_HandleKey(&dc,&row,13,1);Item_Bind_HandleKey(&dc,&row,208,1);
 assert(!strcmp(bindings[208],"+attack") && g_bindings[0].bind2==201);
 Item_Bind_HandleKey(&dc,&row,13,1);Item_Bind_HandleKey(&dc,&row,'f',1);
 assert(!bindings[201][0] && !strcmp(bindings['f'],"+attack"));
 assert(!strcmp(bindings[208],"+attack"));return 0;
}
''', 'controller reassignment/clear/cancel preserve both keyboard slots; key-up never binds; typed x64 entries')

# Exercise the actual menu transformation with a small retail-shaped fixture.
fixture = b'''{ menuDef { name options_menu onClose { close options_voice; }
itemDef { text "@MENU_LOOK" action { close options_voice; open options_look; } }
itemDef { text "@MENU_VOICECHAT" rect 400 285 240 13 action { open options_voice; } }
} }'''
menu = apply_settings_menu('ui_mp/options.menu', fixture).decode()
assert menu.count('name "browser_gamepad"') == 1
assert menu.count('type ITEM_TYPE_BIND') == 16
assert 'open browser_gamepad;' in menu and 'close browser_gamepad;' in menu
assert '@MENU_VOICECHAT' not in menu
assert menu.count('{') == menu.count('}')
print('PASS: controller settings are reachable, close with sibling menus, and expose 16 separate button assignments')

# Preserve the retail surplus-brace shape that previously hid the new panel.
retail = fixture.replace(b'} }', b'} } }')
menu = apply_settings_menu('ui_mp/options.menu', retail).decode()
assert menu.index('name "browser_gamepad"') < menu.index('name options_menu')
print('PASS: controller menu is parsed before the retail surplus closing brace')

# Every visible widget must fit the artwork's content area, including the
# lowest button and help text, on all three pages. This rejects the old tall list.
from browser_gamepad_menu import controller_menu
from browser_settings_menu import items
rows = []
items(controller_menu(), lambda item: rows.append(item) or item)
page_bindings = [0, 0, 0]
for item in rows:
    x, y, width, height = map(float, re.search(
        r'\brect\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)', item).groups())
    assert 48 <= x and x + width <= 350, item
    assert 64 <= y and y + height <= 416, item
    if 'type ITEM_TYPE_BIND' in item:
        page = int(re.search(r'\bshowDvar\s*\{\s*"(\d)"', item)[1])
        page_bindings[page] += 1
assert page_bindings == [0, 8, 8]
print('PASS: all controller pages fit the original safe area; at most eight assignments per page')

# The settings page has the vibration switch, and none of its rows overlap.
def rect(item):
    return tuple(map(float, re.search(r'\brect\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)', item).groups()))
rumble = [item for item in rows if 'dvar "cl_gamepadRumble"' in item]
assert len(rumble) == 1 and 'type ITEM_TYPE_YESNO' in rumble[0]
assert 'dvarTest "ui_gamepadPage" showDvar { "0" }' in rumble[0]
settings = [item for item in rows if ('dvarTest "ui_gamepadPage" showDvar { "0" }' in item and 'name "tab_' not in item)
            or 'dvarTest' not in item]
for i, a in enumerate(settings):
    for b in settings[i + 1:]:
        (ax, ay, aw, ah), (bx, by, bw, bh) = rect(a), rect(b)
        assert ax + aw <= bx or bx + bw <= ax or ay + ah <= by or by + bh <= ay, (a, b)
print('PASS: the settings page offers vibration with no overlapping rows')
