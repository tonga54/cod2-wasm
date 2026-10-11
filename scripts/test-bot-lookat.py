#!/usr/bin/env python3
"""Preserve bot look-at brush interactions without computing unused HUD traces."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/game_mp/player_use_mp.c').read_text()
start = source.index('static qboolean Player_BotNeedsLookAtTrace(')
end = source.index('\nvoid Player_UpdateLookAtEntity', start)
helper = source[start:end]
# Humans retain the complete original locational query; the shortcut is bot-only.
assert 'if(SV_BotIsTestClient(ent->s.number) && !Player_BotNeedsLookAtTrace(start,forward)) return;' in source
assert 'G_LocationalTrace(&trace, start, end, ent->s.number, 0x22802801, priorityMap);' in source
support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef int qboolean;typedef float vec_t;typedef float vec3_t[3];
typedef struct {unsigned classname;struct {int inuse,linked,contents;vec3_t absmin,absmax;}r;}gentity_t;
static gentity_t g_entities[1024];
static struct {int time,maxclients,num_entities;}level={1,4,1024};
static struct {unsigned trigger_lookat;}scr_const={123};
#define SCR_CONST() (&scr_const)
'''
checks = r'''
int main(void){
 vec3_t start={0,0,50},forward={1,0,0};
 assert(!Player_BotNeedsLookAtTrace(start,forward));
 gentity_t *trigger=&g_entities[100];trigger->classname=123;
 trigger->r.inuse=trigger->r.linked=1;trigger->r.contents=0x20000000;
 memcpy(trigger->r.absmin,(float[3]){100,-10,40},sizeof(vec3_t));
 memcpy(trigger->r.absmax,(float[3]){120,10,60},sizeof(vec3_t));
 level.time++;assert(Player_BotNeedsLookAtTrace(start,forward));
 // Script callbacks can unlink/delete/move a brush within the cached tick.
 trigger->r.linked=0;assert(!Player_BotNeedsLookAtTrace(start,forward));trigger->r.linked=1;
 trigger->r.inuse=0;assert(!Player_BotNeedsLookAtTrace(start,forward));trigger->r.inuse=1;
 forward[0]=-1;assert(!Player_BotNeedsLookAtTrace(start,forward));forward[0]=1;
 start[1]=11;assert(!Player_BotNeedsLookAtTrace(start,forward));start[1]=10;
 assert(Player_BotNeedsLookAtTrace(start,forward));start[1]=0;
 start[0]=110;assert(Player_BotNeedsLookAtTrace(start,forward));start[0]=0;
 trigger->r.linked=0;level.time++;assert(!Player_BotNeedsLookAtTrace(start,forward));
 trigger->r.linked=1;trigger->r.contents=0;level.time++;
 assert(!Player_BotNeedsLookAtTrace(start,forward));
 trigger->r.contents=0x20000000;trigger->r.absmin[0]=15001;trigger->r.absmax[0]=15020;
 level.time++;assert(!Player_BotNeedsLookAtTrace(start,forward));
 // Original ray range, grazing faces and diagonal directions remain eligible.
 trigger->r.absmin[0]=15000;level.time++;assert(Player_BotNeedsLookAtTrace(start,forward));
 trigger->r.absmin[1]=.004f;trigger->r.absmax[1]=.006f;
 forward[1]=.0000003f;assert(Player_BotNeedsLookAtTrace(start,forward));forward[1]=0;
 trigger->r.absmin[0]=100;trigger->r.absmax[0]=120;
 trigger->r.absmin[1]=90;trigger->r.absmax[1]=110;
 forward[0]=forward[1]=.70710678f;level.time++;
 assert(Player_BotNeedsLookAtTrace(start,forward));
 trigger->r.inuse=0;level.time++;assert(!Player_BotNeedsLookAtTrace(start,forward));
 puts("PASS: bot-only unused look-at culling; linked/moved/deleted brushes, original range, ray direction, diagonal/grazing/inside bounds; original human hitbox query retained");
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-bot-lookat-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(support + helper + checks)
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-O2',
                    '-fsanitize=undefined', str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True, timeout=10)
