#!/usr/bin/env python3
"""Exercise the production sprint movement, key actions and carry animation."""
from pathlib import Path
import re, subprocess, tempfile
root = Path(__file__).resolve().parent.parent

def function(path, name):
    source = (root / path).read_text()
    m = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert m, name
    end = m.end(); depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}'); end += 1
    return source[m.start():end] + '\n'

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define PMF_MANTLE 4
#define PMF_LADDER 32
#define ENTITYNUM_NONE 1023
#define BUTTON_SPRINT 2
#define PMF_SPRINT 0x02000000
#define SPRINT_SPEED_SCALE 1.45f
typedef unsigned char byte;
typedef int qboolean;
typedef struct {int pm_type,pm_flags,groundEntityNum,eFlags,weaponstate,speed;float leanf,fWeaponPosFrac;} playerState_t;
typedef struct {int buttons,forwardmove,rightmove;} usercmd_t;
typedef struct {playerState_t *ps;usercmd_t cmd;} pmove_t;
typedef struct {int walking;} pml_t;
static void *player_spectateSpeedScale;
static float PM_DvarFloat(void *p,float d) {return d;}
static int PM_GetEffectiveStance(playerState_t *p) {return p->pm_flags&1?1:p->pm_flags&2?2:0;}
typedef struct {int down[2],downtime,msec;byte active,wasPressed,padding[2];} kbutton_t;
static kbutton_t kb[28],sprintButton;
static void IN_KeyDown(kbutton_t *b) {b->active=1;}
static void IN_KeyUp(kbutton_t *b) {b->active=0;}
typedef struct {int keyCatchers,cgameInShellshock;struct {playerState_t ps;} snap;} clientActive_t;
static clientActive_t inputClient,*inputClientPtr=&inputClient;
static void *imp_cl=&inputClientPtr;
static struct {struct {int enabled;} current;} bypassStorage,*cl_bypassMouseInput=&bypassStorage;
static float sprintViewBlend;
static int sprintViewTime;
typedef struct {void *onKey;} menuDef_t;
static menuDef_t controlsMenu;
static void *uiInfo;
static int editingField,waitingForKey;
static void *imp_g_editingField=&editingField;
static void *Menu_GetFocused(void *p) {return &controlsMenu;}
static int Display_KeyBindPending(void) {return waitingForKey;}
'''
body = ''.join(function('src/PC/bgame/bg_pmove.c', n) for n in ('PM_AbsInt','PM_CmdScale','PM_UpdateSprint'))
body += ''.join(function('src/PC/client_mp/cl_input.c', n) for n in ('IN_Breath_Down','IN_Breath_Up','IN_Sprint_Down','IN_Sprint_Up','IN_Melee_Down','IN_Melee_Up'))
body += ''.join(function('src/PC/client_mp/cl_input.c', n) for n in ('CL_ConsumeButtonPress','CL_CmdButtons'))
body += ''.join(function('src/PC/cgame_mp/cg_weapons.c', n) for n in ('CG_ResetSprintView','CG_SprintViewBlend'))
body += function('src/PC/ui_mp/ui_main_mp.c','UI_CheckExecKey')
checks = r'''
int main(void) {
    /* Pending remaps must consume letters/modifiers instead of letting
     * gameplay bindings execute underneath the controls screen. */
    for(int key=0;key<256;key++) {
        waitingForKey=1;assert(UI_CheckExecKey(key));
        waitingForKey=0;editingField=1;assert(UI_CheckExecKey(key));
        editingField=0;
    }
    assert(!UI_CheckExecKey('j')&&!UI_CheckExecKey(0xa0));
    assert(UI_CheckExecKey(0x0d)&&UI_CheckExecKey(0xc8));
    playerState_t p={.groundEntityNum=1022,.speed=190};
    pml_t ground={.walking=1};pmove_t pm={.ps=&p,.cmd={.buttons=BUTTON_SPRINT|0x8000,.forwardmove=127}};
    PM_UpdateSprint(&pm,&ground);assert(p.pm_flags&PMF_SPRINT);
    float normal=190.f,fast=normal*1.45f;
    assert(fabsf(PM_CmdScale(&p,&pm.cmd)*127-fast)<.001f);
    pm.cmd.rightmove=127;
    assert(fabsf(PM_CmdScale(&p,&pm.cmd)*sqrtf(2*127.f*127.f)-fast)<.001f);
    for(int block=0;block<27;block++) {
        p=(playerState_t){.groundEntityNum=1022,.speed=190,.pm_flags=PMF_SPRINT};
        ground.walking=1;pm.cmd=(usercmd_t){.buttons=2|0x8000,.forwardmove=127};
        if(block<10)pm.cmd.buttons|=(int[]){1,4,8,16,32,64,128,0x4000,0x10000,0x20000}[block];
        else if(block<17)p.pm_flags|=(int[]){1,2,4,32,64,0x800,0x10000}[block-10];
        else if(block==17)p.eFlags=0x100;
        else if(block==18)p.leanf=.5f;
        else if(block==19)p.fWeaponPosFrac=.01f;
        else if(block==20)p.weaponstate=5;
        else if(block==21)p.pm_type=4;
        else if(block==22)ground.walking=0;
        else if(block==23)p.groundEntityNum=1023;
        else if(block==24)pm.cmd.forwardmove=-127;
        else if(block==25)pm.cmd.forwardmove=0;
        else pm.cmd.buttons=0;
        PM_UpdateSprint(&pm,&ground);assert(!(p.pm_flags&PMF_SPRINT));
    }
    /* Two clients replay the same commands, including toggles and diagonals. */
    for(int frame=1;frame<=66;frame++) {
        playerState_t client={.groundEntityNum=1022,.speed=190},server=client;
        float x[2]={0};ground.walking=1;
        for(int t=0;t<6000;t+=frame) {
            usercmd_t cmd={.buttons=(t/500%2?2:0),.forwardmove=127,.rightmove=t/700%2?127:0};
            for(int side=0;side<2;side++) {
                pm.ps=side?&server:&client;pm.cmd=cmd;PM_UpdateSprint(&pm,&ground);
                x[side]+=PM_CmdScale(pm.ps,&cmd)*127*frame*.001f;
            }
            assert(x[0]==x[1]&&client.pm_flags==server.pm_flags);
        }
    }
    IN_Sprint_Down();assert(sprintButton.active&&kb[15].active&&!kb[19].active);
    usercmd_t keys={0};CL_CmdButtons(&keys);assert(keys.buttons==(BUTTON_SPRINT|0x8000));
    p=(playerState_t){.groundEntityNum=1022,.speed=190};pm.ps=&p;
    pm.cmd=keys;pm.cmd.forwardmove=127;PM_UpdateSprint(&pm,&ground);assert(p.pm_flags&PMF_SPRINT);
    IN_Sprint_Up();assert(!sprintButton.active&&!kb[15].active&&!kb[19].active);
    IN_Melee_Down();assert(kb[19].active&&!sprintButton.active&&!kb[15].active);
    keys=(usercmd_t){0};CL_CmdButtons(&keys);assert(keys.buttons==4);IN_Melee_Up();
    p=(playerState_t){.pm_flags=PMF_SPRINT};
    for(int frame=1;frame<=66;frame++) {
        CG_ResetSprintView();int t=1000;CG_SprintViewBlend(&p,t);
        for(;t<1300;t+=frame){float v=CG_SprintViewBlend(&p,t);assert(v>=0&&v<=1);}
        assert(CG_SprintViewBlend(&p,t)==1);
        p.pm_flags=0;
        for(int end=t+200;t<end;t+=frame)CG_SprintViewBlend(&p,t);
        assert(CG_SprintViewBlend(&p,t)==0);p.pm_flags=PMF_SPRINT;
    }
    CG_ResetSprintView();CG_SprintViewBlend(&p,1000);assert(CG_SprintViewBlend(&p,1200)==1);
    p.weaponstate=3;assert(CG_SprintViewBlend(&p,1201)==0);p.weaponstate=0;
    assert(CG_SprintViewBlend(&p,500)==0);
    puts("PASS: sprint speed/diagonals, 27 interruptions, 66 replay schedules, Shift breath/V melee, 256 remap keys and carry blends");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-sprint-') as directory:
    d=Path(directory); (d/'test.c').write_text(support+body+checks)
    subprocess.run(['cc','-std=c99','-O1','-fsanitize=address,undefined',str(d/'test.c'),'-lm','-o',str(d/'test')],check=True)
    subprocess.run([str(d/'test')],check=True)
