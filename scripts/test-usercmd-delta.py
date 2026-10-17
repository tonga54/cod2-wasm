#!/usr/bin/env python3
"""Round-trip packets of usercmds through the real client encoder and server decoder."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent


def function(path, name):
    source = (root / path).read_text()
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    body = source[match.start():end] + '\n'
    assert not body.startswith((' ', '\t')), name
    return body


blob = (root / 'src/blobs/data.c').read_text()
table = re.search(r'unsigned char kbitmask\[160\] = \{([^}]*)\}', blob)[1]
masks = bytes(int(value, 16) for value in re.findall(r'0x[0-9a-fA-F]+', table))
assert len(masks) == 160
kbitmask = [int.from_bytes(masks[i:i + 4], 'little') for i in range(0, len(masks), 4)]
assert kbitmask[1] == 1 and kbitmask[4] == 0xf and kbitmask[7] == 0x7f

helpers = ''.join(function('src/PC/qcommon/msg_mp.c', name) for name in (
    'MSG_WriteBits_core', 'MSG_WriteBit0_core', 'MSG_WriteBit1_core',
    'MSG_ReadBits_core', 'MSG_ReadBits', 'MSG_ReadBit_core', 'MSG_ReadBit',
    'MSG_WriteByte_core', 'MSG_ReadByte_core', 'MSG_ReadByte',
    'MSG_ReadShort_core', 'MSG_ReadShort', 'MSG_ReadLong_core', 'MSG_ReadLong',
    'MSG_WriteShort_core', 'MSG_WriteLong_core',
    'MSG_ReadDeltaKeyShort', 'MSG_ReadDeltaKeyBits', 'MSG_UsercmdDecodeHorMove',
    'MSG_UsercmdReadMoveBits', 'MSG_ReadDeltaUsercmdKey',
    'MSG_UsercmdMoveBits', 'MSG_WriteDeltaKeyShort', 'MSG_WriteDeltaKeyBits'))
writer = function('src/PC/qcommon/msg_mp.c', 'MSG_WriteDeltaUsercmdKey')

support = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char byte;
typedef int qboolean;
typedef struct {qboolean overflowed;byte *data;int maxsize,cursize,readcount,bit;} msg_t;
typedef struct {
    int serverTime;int buttons;byte weapon;byte offHandIndex;int angles[3];
    signed char forwardmove;signed char rightmove;
} usercmd_t;
''' + 'static const unsigned int kbitmask[%d] = {%s};\n' % (
    len(kbitmask), ','.join(hex(value) for value in kbitmask))

checks = r'''
#define COUNT(a) ((int)(sizeof(a)/sizeof((a)[0])))
static unsigned int state=0x2545f491u;
static unsigned int next(void) {state^=state<<13;state^=state>>17;state^=state<<5;return state;}
static const int presses[]={0x2,0x4,0x8,0x10,0x20,0x40,0x80,0x100,0x200,0x400,0x800,
    0x1000,0x2000,0x4000,0x8000,0x10000,0x20000,0x40000};
static const signed char moves[]={-127,0,127};
/* Both ends start a packet from MSG_SetDefaultUserCmd: zeroed, or carrying
 * the player's weapon, offhand, view angles and stance. */
static const usercmd_t bases[2]={{0},{.buttons=0x200,.weapon=9,.offHandIndex=3,.angles={1200,64000,0}}};

static void check(const usercmd_t *got,const usercmd_t *sent) {
    assert(got->serverTime==sent->serverTime);
    /* The shipped kbitmask[18] is 0x7ffff, so a decoded 18-bit button field
     * may carry bit 19, which no button uses. */
    assert((got->buttons&0x7ffff)==sent->buttons&&!(got->buttons&~0xfffff));
    assert(got->weapon==sent->weapon&&got->offHandIndex==sent->offHandIndex);
    for(int i=0;i<3;i++) assert(got->angles[i]==sent->angles[i]);
    assert(got->forwardmove==sent->forwardmove&&got->rightmove==sent->rightmove);
}

static void roundtrip(int key,const usercmd_t *base,const usercmd_t *cmds,int count) {
    byte data[2048];
    msg_t out={.data=data,.maxsize=sizeof(data)};
    const usercmd_t *prev=base;
    for(int i=0;i<count;i++) {
        MSG_WriteDeltaUsercmdKey(&out,key,(usercmd_t *)prev,(usercmd_t *)&cmds[i]);
        prev=&cmds[i];
    }
    assert(!out.overflowed);
    msg_t in={.data=data,.maxsize=sizeof(data),.cursize=out.cursize};
    usercmd_t got[32];
    usercmd_t *old=(usercmd_t *)base;
    for(int i=0;i<count;i++) {
        MSG_ReadDeltaUsercmdKey(&in,key,old,&got[i]);
        check(&got[i],&cmds[i]);
        old=&got[i];
    }
    assert(!in.overflowed&&in.readcount==out.cursize);
}

/* 0: unchanged shortcut, 1: short form, 2: long form with the extra fields. */
static int form(int key,const usercmd_t *from,const usercmd_t *to) {
    byte data[64];
    msg_t out={.data=data,.maxsize=sizeof(data)};
    MSG_WriteDeltaUsercmdKey(&out,key,(usercmd_t *)from,(usercmd_t *)to);
    msg_t in={.data=data,.maxsize=sizeof(data),.cursize=out.cursize};
    if(MSG_ReadBit(&in)) MSG_ReadByte(&in); else MSG_ReadLong(&in);
    if(MSG_ReadBit(&in)==(key&1)) return 0;
    return MSG_ReadBit(&in)==(key&1)?1:2;
}

int main(void) {
    int presses_checked=0,fields_checked=0,packets=0,forms=0;
    for(int b=0;b<2;b++)for(int p=0;p<COUNT(presses);p++)for(int count=1;count<=8;count++)
    for(int at=0;at<count;at++)for(int attack=0;attack<2;attack++) {
        usercmd_t cmds[8];
        for(int i=0;i<count;i++) {
            cmds[i]=bases[b];
            cmds[i].serverTime=50000+i*8;
            cmds[i].buttons|=attack;
            if(i==at) cmds[i].buttons^=presses[p];
        }
        roundtrip((int)next(),&bases[b],cmds,count);
        presses_checked++;
    }
    for(int b=0;b<2;b++)for(int field=0;field<3;field++)for(int count=2;count<=8;count++)
    for(int at=1;at<count;at++) {
        usercmd_t cmds[8];
        for(int i=0;i<count;i++) {
            cmds[i]=bases[b];
            cmds[i].serverTime=50000+i*8;
            if(i>=at&&field==0) cmds[i].weapon=17;
            if(i>=at&&field==1) cmds[i].offHandIndex=4;
            if(i>=at&&field==2) cmds[i].angles[2]=300;
        }
        roundtrip((int)next(),&bases[b],cmds,count);
        fields_checked++;
    }
    static const int deltas[]={8,8,8,16,0,1,33,255,256,300,1000,-5};
    for(int n=0;n<6000;n++) {
        const usercmd_t *base=&bases[n&1];
        usercmd_t cmds[32],cur=*base;
        int count=1+next()%32,time=1000+next()%100000;
        for(int i=0;i<count;i++) {
            unsigned int r=next();
            time+=deltas[next()%COUNT(deltas)];
            if(r&1) cur.buttons=(int)(next()&0x7ffff);
            if(r&2) cur.buttons^=1;
            if(r&4) cur.weapon=next()&0x7f;
            if(r&8) cur.offHandIndex=next()&0x7f;
            if(r&16) cur.angles[0]=next()&0xffff;
            if(r&32) cur.angles[1]=next()&0xffff;
            if(r&64) cur.angles[2]=next()&0xffff;
            if(r&128) {cur.forwardmove=moves[next()%3];cur.rightmove=moves[next()%3];}
            if(!(r&0x300)) cur=i?cmds[i-1]:*base;
            cur.serverTime=time;
            cmds[i]=cur;
        }
        roundtrip((int)next(),base,cmds,count);
        packets++;
    }
    for(int k=0;k<64;k++) {
        int key=(int)next();
        usercmd_t from=bases[1],to=bases[1];
        from.serverTime=50000;to.serverTime=50008;
        assert(form(key,&from,&to)==0);
        to.angles[0]=from.angles[0]+91;assert(form(key,&from,&to)==1);to=from;to.serverTime=50008;
        to.angles[1]=from.angles[1]-91;assert(form(key,&from,&to)==1);to=from;to.serverTime=50008;
        to.buttons^=1;assert(form(key,&from,&to)==1);to=from;to.serverTime=50008;
        to.forwardmove=127;assert(form(key,&from,&to)==1);to=from;to.serverTime=50008;
        to.buttons|=0x400;assert(form(key,&from,&to)==2);to=from;to.serverTime=50008;
        to.buttons|=0x2;assert(form(key,&from,&to)==2);to=from;to.serverTime=50008;
        to.weapon=17;assert(form(key,&from,&to)==2);to=from;to.serverTime=50008;
        to.offHandIndex=4;assert(form(key,&from,&to)==2);to=from;to.serverTime=50008;
        to.angles[2]=300;assert(form(key,&from,&to)==2);to=from;
        to.serverTime=from.serverTime+5000;to.angles[0]^=1;
        assert(form(key,&from,&to)==1);
        forms++;
    }
    printf("PASS: %d single-frame button presses/releases and %d weapon/offhand/roll changes at every packet position, "
           "%d random packets, %d keys choosing unchanged/short/long forms\n",
           presses_checked,fields_checked,packets,forms);
}
'''

source = support + helpers + writer + checks
mutants = (
    support + helpers + writer.replace('if (extraChanged) {', 'if ((unsigned int)serverTimeDelta > 0xff) {') + checks,
    support + helpers + writer.replace('        fieldKey = key;\n', '        fieldKey = key ^ to->serverTime;\n') + checks,
    support + helpers + writer.replace('extraChanged = (from->buttons', 'extraChanged = 1 || (from->buttons') + checks,
)
assert all(mutant != source for mutant in mutants)
with tempfile.TemporaryDirectory(prefix='cod2-usercmd-delta-') as directory:
    d = Path(directory)
    for index, variant in enumerate((source, *mutants)):
        (d / 'test.c').write_text(variant)
        subprocess.run(['cc', '-std=c99', '-O1', '-fsanitize=address,undefined', '-fno-sanitize=alignment',
                        '-fno-sanitize-recover=all', str(d / 'test.c'), '-o', str(d / 'test')], check=True)
        result = subprocess.run([str(d / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stdout, result.stderr)
        if not index:
            print(result.stdout, end='')
print('PASS: time-selected long form, wrong field key and always-long encoding mutants fail')
