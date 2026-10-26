#!/usr/bin/env python3
"""Exercise the client's command history: prediction replays from it however
long the server takes to acknowledge, and the connection reads as interrupted
by how long a command has waited, not by how many frames were drawn."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
cgame_bridge = (root / 'src/PC/client_mp/cl_cgame_mp.c').read_text()
header = (root / 'src/headers/PC/client_mp/cl_cmd_history.h').read_text()
history = int(re.search(r'#define CL_CMD_HISTORY (\d+)', header).group(1))
assert history >= 1024 and history & (history - 1) == 0
assert '#define CL_CONNECTION_INTERRUPTED_MS 1000' in header

# Each new command is kept as it is made; prediction reaches back through the
# whole history, and the message waits on the oldest unacknowledged command.
cl_input = (root / 'src/PC/client_mp/cl_input.c').read_text()
assert ('        cl->cmds[cl->cmdNumber & 127] = CL_CreateCmd();\n'
        '        CL_KeepUserCmd(cl->cmdNumber, &cl->cmds[cl->cmdNumber & 127]);\n') in cl_input
predict = (root / 'src/PC/cgame_mp/cg_predict_mp.c').read_text()
assert '    oldest = cmdNum - (CL_CMD_HISTORY - 1);\n' in predict and 'cmdNum - 127' not in predict
assert 'return CG_FirstPredictionCommand(latest - (CL_CMD_HISTORY - 1), latest, commandTime);' in predict
draw = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
at = draw.index('void CG_DrawDisconnect(void)\n{')
disconnect = draw[at:draw.index('void CG_PriorityCenterPrint(', at)]
for text in ('first = CG_FirstUnacknowledgedCommand(snap->ps.commandTime);',
             'newest.serverTime - cmd.serverTime < CL_CONNECTION_INTERRUPTED_MS'):
    assert text in disconnect, text
assert '- 127' not in disconnect

start = cgame_bridge.index('static usercmd_t cl_cmdHistory[CL_CMD_HISTORY];')
end = cgame_bridge.index('int CL_GetCurrentCmdNumber(void)', start)
code = cgame_bridge[start:end]

support = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CL_CMD_HISTORY @HISTORY@
#define ERR_DROP 1
typedef int qboolean;
typedef struct { int serverTime; int buttons; unsigned char weapon; int angles[3]; } usercmd_t;
typedef struct { int cmdNumber; } clientActive_t;
static clientActive_t client;
#define CL_LOCAL (&client)
static void Com_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
'''
checks = r'''
static usercmd_t made(int number) { usercmd_t c = {1000 + 4 * number, number, (unsigned char)number, {number, 2, 3}}; return c; }
static void make(int number) { usercmd_t c = made(number); client.cmdNumber = number; CL_KeepUserCmd(number, &c); }
int main(void)
{
    usercmd_t got;
    /* 5,000 commands at 250 a second: the last CL_CMD_HISTORY read back as made,
     * the one before them not at all. */
    for (int n = 1; n <= 5000; ++n)
        make(n);
    for (int n = 5000 - (CL_CMD_HISTORY - 1); n <= 5000; ++n) {
        usercmd_t want = made(n);
        assert(CL_GetUserCmd(n, &got) && !memcmp(&got, &want, sizeof(got)));
    }
    assert(!CL_GetUserCmd(5000 - CL_CMD_HISTORY, &got));
    /* An acknowledgement 0.6 s old at 250 frames a second, 150 commands back:
     * cl.cmds' 128 had lost it, the history has it. */
    assert(CL_GetUserCmd(5000 - 150, &got) && got.serverTime == 1000 + 4 * (5000 - 150));
    /* A new session numbers from 1 again: what came before it reads as empty
     * commands, never the last session's. */
    for (int n = 1; n <= 300; ++n)
        make(n);
    for (int n = 300 - (CL_CMD_HISTORY - 1); n < 1; ++n) {
        assert(CL_GetUserCmd(n, &got));
        assert(got.serverTime == 0 && got.buttons == 0 && got.weapon == 0);
    }
    assert(CL_GetUserCmd(1, &got) && got.serverTime == 1004 && CL_GetUserCmd(300, &got) && got.buttons == 300);
    puts("PASS: the last CL_CMD_HISTORY commands read back as made, an acknowledgement 150 commands old still "
         "replays, and a new session reads what came before it as empty commands, never the last session's");
    return 0;
}
'''

mutants = [
    ('    if (cmdNumber <= currentCmd - CL_CMD_HISTORY) {', '    if (cmdNumber <= currentCmd - 128) {'),
    ('    if (cmdNumber != cl_cmdHistoryLast + 1)\n        cl_cmdHistoryFirst = cmdNumber;\n', ''),
    ('    if (cmdNumber < cl_cmdHistoryFirst || cmdNumber > cl_cmdHistoryLast)\n        memset(ucmd, 0, sizeof(*ucmd));\n    else\n',
     '    if (0)\n        memset(ucmd, 0, sizeof(*ucmd));\n    else\n'),
    ('    cl_cmdHistory[cmdNumber & (CL_CMD_HISTORY - 1)] = *cmd;', '    cl_cmdHistory[cmdNumber & 127] = *cmd;'),
]

with tempfile.TemporaryDirectory(prefix='cod2-command-history-') as tmp:
    path = Path(tmp)

    def run(source, name, strict):
        c = path / f'{name}.c'
        c.write_text(support.replace('@HISTORY@', str(history)) + source + checks)
        flags = ['-O1', '-g', '-Wall', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all'] \
            if strict else ['-O0', '-w']
        subprocess.run(['cc', '-std=c99', *flags, str(c), '-o', str(path / name)], check=True)
        return subprocess.run([str(path / name)], capture_output=not strict)

    assert run(code, 'history', True).returncode == 0
    for index, (before, after) in enumerate(mutants):
        assert code.count(before) == 1, before
        assert run(code.replace(before, after), f'mutant{index}', False).returncode != 0, f'mutant survived: {after!r}'
    print(f'PASS: {len(mutants)} command history mutants fail')
