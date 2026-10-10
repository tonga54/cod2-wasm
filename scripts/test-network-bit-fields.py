#!/usr/bin/env python3
"""Compare production packet bit I/O against the original mixed byte/bit cursors."""
from pathlib import Path
import json
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/qcommon/msg_mp.c').read_text()


def function(name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


support = r'''
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef unsigned char byte;
typedef struct { int overflowed; byte *data; int maxsize,cursize,readcount,bit; } msg_t;
/* The original one-bit loops are the independent wire/cursor reference.
 * Read accumulation is unsigned to make its 32-bit sign bit well-defined. */
static void OldWrite(msg_t *msg,int value,int bits) {
    if(msg->maxsize-msg->cursize<=3){msg->overflowed=1;return;}
    for(int i=0;i<bits;i++) {
        int bit=msg->bit&7;
        if(!bit){msg->bit=msg->cursize*8;msg->data[msg->cursize++]=0;}
        if(value&1)msg->data[msg->bit>>3]|=1<<bit;
        msg->bit++;value>>=1;
    }
}
static int OldRead(msg_t *msg,int bits) {
    unsigned value=0;
    for(int i=0;i<bits;i++) {
        int bit=msg->bit,rem=bit&7;
        if(!rem) {
            if(msg->readcount>=msg->cursize){msg->overflowed=1;return -1;}
            bit=msg->readcount*8;msg->readcount++;msg->bit=bit;
        }
        value|=((unsigned)(msg->data[bit>>3]>>rem)&1u)<<i;
        msg->bit=bit+1;
    }
    return (int)value;
}
static uint32_t rng=123456789;
static unsigned Random(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void Equal(const msg_t *a,const msg_t *b) {
    assert(a->bit==b->bit && a->cursize==b->cursize && a->readcount==b->readcount && a->overflowed==b->overflowed);
}
static double Seconds(void) {struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
'''
checks = r'''
typedef struct { int bits,value; } operation_t;
static byte oldData[65536],newData[65536];
static operation_t operations[128];
int main(int argc,char **argv) {
    int schedules=0;
    /* Exhaust every initial bit offset, field width and high-bit value, with
     * a byte append in the middle of an otherwise partially filled bit byte. */
    for(int offset=0;offset<8;offset++)for(int width=0;width<=32;width++)
    for(int v=0;v<6;v++) {
        int values[]={0,1,-1,INT32_MIN,INT32_MAX,(int)0xa5a5a5a5u};
        memset(oldData,0xa5,sizeof(oldData));memset(newData,0xa5,sizeof(newData));
        msg_t a={.data=oldData,.maxsize=128},b={.data=newData,.maxsize=128};
        OldWrite(&a,-1,offset);MSG_WriteBits(&b,-1,offset);
        MSG_WriteByte_core(&a,0x73);MSG_WriteByte_core(&b,0x73);
        OldWrite(&a,values[v],width);MSG_WriteBits(&b,values[v],width);
        Equal(&a,&b);assert(!memcmp(oldData,newData,sizeof(oldData)));
        a.bit=b.bit=0;a.readcount=b.readcount=0;
        assert(OldRead(&a,offset)==MSG_ReadBits(&b,offset));
        assert(MSG_ReadByte_core(&a)==MSG_ReadByte_core(&b));
        assert(OldRead(&a,width)==MSG_ReadBits(&b,width));Equal(&a,&b);
        schedules++;
    }
    /* Bounded and full buffers; each schedule interleaves raw byte writes with
     * signed 1..32 bit values, and reads every possible truncated byte length. */
    for(int run=0;run<4096;run++) {
        int capacity=run%2?1024:run%65;
        memset(oldData,0xa5,sizeof(oldData));memset(newData,0xa5,sizeof(newData));
        msg_t a={.data=oldData,.maxsize=capacity},b={.data=newData,.maxsize=capacity};
        for(int op=0;op<128;op++) {
            int bits=Random()%36-2; if(bits==33)bits=-3;
            operations[op]=(operation_t){bits,(int)Random()};
            if(bits<0) {MSG_WriteByte_core(&a,operations[op].value);MSG_WriteByte_core(&b,operations[op].value);}
            else {OldWrite(&a,operations[op].value,bits);MSG_WriteBits(&b,operations[op].value,bits);}
            Equal(&a,&b);assert(!memcmp(oldData,newData,sizeof(oldData)));
        }
        for(int length=0;length<=a.cursize;length++) {
            msg_t x={.data=oldData,.cursize=length},y={.data=newData,.cursize=length};
            for(int op=0;op<128;op++) {
                int oldValue,newValue;
                if(operations[op].bits<0){oldValue=MSG_ReadByte_core(&x);newValue=MSG_ReadByte_core(&y);}
                else {oldValue=OldRead(&x,operations[op].bits);newValue=MSG_ReadBits(&y,operations[op].bits);}
                assert(oldValue==newValue);Equal(&x,&y);
            }
            schedules++;
        }
    }
    if(argc>1) {
        const int widths[]={1,2,7,8,10,13,16,24,32};
        volatile unsigned checksum=0;
        double writeOld=1e9,writeNew=1e9,readOld=1e9,readNew=1e9;
        for(int repeat=0;repeat<4;repeat++) {
            for(int variant=0;variant<2;variant++) {
                double t=Seconds();
                for(int packet=0;packet<40000;packet++) {
                    msg_t m={.data=newData,.maxsize=sizeof(newData)};
                    for(int f=0;f<256;f++) {
                        int width=widths[f%9];int value=packet*137+f*31;
                        if(variant)MSG_WriteBits(&m,value,width);else OldWrite(&m,value,width);
                        if(f%7==0)MSG_WriteByte_core(&m,f);
                    }
                    checksum+=m.cursize+newData[0];
                }
                double elapsed=Seconds()-t;
                if(variant){if(elapsed<writeNew)writeNew=elapsed;}else if(elapsed<writeOld)writeOld=elapsed;
            }
            msg_t encoded={.data=newData,.maxsize=sizeof(newData)};
            for(int f=0;f<256;f++){OldWrite(&encoded,f*73,widths[f%9]);if(f%7==0)MSG_WriteByte_core(&encoded,f);}
            for(int variant=0;variant<2;variant++) {
                double t=Seconds();
                for(int packet=0;packet<40000;packet++) {
                    msg_t m={.data=newData,.cursize=encoded.cursize};
                    for(int f=0;f<256;f++) {
                        checksum+=variant?MSG_ReadBits(&m,widths[f%9]):OldRead(&m,widths[f%9]);
                        if(f%7==0)checksum+=MSG_ReadByte_core(&m);
                    }
                }
                double elapsed=Seconds()-t;
                if(variant){if(elapsed<readNew)readNew=elapsed;}else if(elapsed<readOld)readOld=elapsed;
            }
        }
        assert(checksum);
        printf("{\"schedules\":%d,\"fieldsPerRun\":10240000,\"writeOriginalMs\":%.3f,\"writeBatchedMs\":%.3f,\"writeSpeedup\":%.2f,\"readOriginalMs\":%.3f,\"readBatchedMs\":%.3f,\"readSpeedup\":%.2f}\n",schedules,writeOld*1000,writeNew*1000,writeOld/writeNew,readOld*1000,readNew*1000,readOld/readNew);
    } else printf("PASS: %d packet schedules; all offsets/widths/sign bits, interleaved bytes, truncation, partial overflow/cursors and full buffer canaries\n",schedules);
}
'''
code = support
for name in ('MSG_WriteBits_core', 'MSG_WriteBits', 'MSG_ReadBits_core', 'MSG_ReadBits',
             'MSG_WriteByte_core', 'MSG_ReadByte_core'):
    code += function(name)
code += checks
with tempfile.TemporaryDirectory(prefix='cod2-network-bits-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(code)
    subprocess.run(['cc', '-O1', '-fsanitize=address,undefined', str(path / 'test.c'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
    for mutant in (code.replace('value >>= take;', 'value >>= 1;'),
                   code.replace('bit = msg->readcount * 8;', 'bit = msg->bit;')):
        assert mutant != code
        (path / 'mutant.c').write_text(mutant)
        subprocess.run(['cc', '-O1', str(path / 'mutant.c'), '-o', str(path / 'mutant')], check=True)
        assert subprocess.run([str(path / 'mutant')], capture_output=True).returncode != 0
    if '--benchmark' in sys.argv:
        subprocess.run(['cc', '-O2', str(path / 'test.c'), '-o', str(path / 'bench')], check=True)
        report = json.loads(subprocess.check_output([str(path / 'bench'), 'benchmark']))
        (root / 'out/online-performance-bit-fields.json').write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps(report))
print('PASS: incorrect multi-bit consumption and mixed byte/bit cursor regressions are detected')
