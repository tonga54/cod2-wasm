#!/usr/bin/env python3
"""Run the production start of CG_ServerCommand: our cod2_ commands reach their
handlers and never the retail ones, which dispatch on the first letter alone."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_servercmds_mp.c').read_text()
start = source.index('void CG_ServerCommand(void)\n{')
end = source.index('    switch (cmd[0]) {', start)
head = source[start:end] + '    reached = cmd[0];\n}\n'

support = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int argc, reached, popups, obituaries;
static char argv[8][32];
static int Cmd_Argc(void) { return argc; }
static const char *CG_Argv(int i) { return i < argc ? argv[i] : ""; }
static void CG_ScorePopupAdd(int event, int points) { assert(event == 0 && points == 100); popups++; }
void CG_QueueObituary(int victim, int attacker, int weapon) { obituaries++; }
/* The deathmatch scores the panel received, "C" for a clear. */
static char scores[64];
static void CG_ScorePanelClearDmScores(void) { strcat(scores, "C"); }
static void CG_ScorePanelSetDmScore(int clientNum, int score) {
    snprintf(scores + strlen(scores), sizeof(scores) - strlen(scores), "%d=%d,", clientNum, score);
}
/* The attacker lists the killcam received, "killer:other,other;" each. */
#define KILLCAM_MAX_CLIENTS 64
static char attackers[64];
static void CG_KillcamSetAttackers(int killer, const int *others, int count) {
    snprintf(attackers + strlen(attackers), sizeof(attackers) - strlen(attackers), "%d:", killer);
    for (int i = 0; i < count; i++)
        snprintf(attackers + strlen(attackers), sizeof(attackers) - strlen(attackers), "%s%d", i ? "," : "", others[i]);
    strcat(attackers, ";");
}
'''

checks = r'''
static int run(const char *line) {
    char copy[128];
    argc = reached = 0;
    snprintf(copy, sizeof(copy), "%s", line);
    for (char *t = strtok(copy, " "); t; t = strtok(NULL, " ")) {
        assert(argc < 8);
        snprintf(argv[argc++], 32, "%s", t);
    }
    CG_ServerCommand();
    return reached;
}
int main(void) {
    /* Commands this client does not handle are dropped, not announced. */
    assert(!run("cod2_future") && !run("cod2_obituary 1 2") && !run("cod2_points 1"));
    /* Deathmatch scores reach the panel: one score, or the whole table, which
     * replaces the old one; a malformed one changes nothing. */
    assert(!run("cod2_dmscore 5 3") && !strcmp(scores, "5=3,"));
    scores[0] = 0;
    assert(!run("cod2_dmscore 5") && !run("cod2_dmscore 5 3 1") && !scores[0]);
    assert(!run("cod2_dmscores 0 1 2 -3") && !strcmp(scores, "C0=1,2=-3,"));
    scores[0] = 0;
    assert(!run("cod2_dmscores") && !strcmp(scores, "C"));
    scores[0] = 0;
    assert(!run("cod2_dmscores 0 4 7") && !strcmp(scores, "C0=4,"));
    assert(!run("cod2_points 0 100") && popups == 1);
    /* The killcam gets the killer, then the others in the server's order; a
     * bare command changes nothing. */
    assert(!run("cod2_attackers 3 1 2") && !strcmp(attackers, "3:1,2;"));
    attackers[0] = 0;
    assert(!run("cod2_attackers 5") && !strcmp(attackers, "5:;"));
    attackers[0] = 0;
    assert(!run("cod2_attackers") && !attackers[0]);
#ifdef __EMSCRIPTEN__
    assert(!run("cod2_obituary 1 2 3") && obituaries == 1);
#endif
    /* Retail commands still reach their letter. */
    assert(run("c hello") == 'c' && run("cod2 x") == 'c' && run("cod2x 1") == 'c' && run("b 0 1 2") == 'b');
    return 0;
}
'''

mutants = [
    ('    if (!strncmp(cmd, "cod2_", 5))\n        return;\n', ''),
    ('if (!strncmp(cmd, "cod2_", 5))', 'if (!strncmp(cmd, "cod2", 4))'),
    ('if (!strncmp(cmd, "cod2_", 5))', 'if (!strcmp(cmd, "cod2_dmscore"))'),
    ('        if (Cmd_Argc() == 3)\n            CG_ScorePanelSetDmScore',
     '        if (Cmd_Argc() >= 3)\n            CG_ScorePanelSetDmScore'),
    ('        CG_ScorePanelClearDmScores();\n', ''),
    ('for (i = 1; i + 1 < Cmd_Argc(); i += 2)', 'for (i = 1; i < Cmd_Argc(); i += 2)'),
    ('CG_ScorePanelSetDmScore(atoi(CG_Argv(i)), atoi(CG_Argv(i + 1)));',
     'CG_ScorePanelSetDmScore(atoi(CG_Argv(i + 1)), atoi(CG_Argv(i)));'),
    ('for (i = 2; i < Cmd_Argc() && count < KILLCAM_MAX_CLIENTS; ++i)',
     'for (i = 1; i < Cmd_Argc() && count < KILLCAM_MAX_CLIENTS; ++i)'),
    ('        if (Cmd_Argc() >= 2)\n            CG_KillcamSetAttackers', '        CG_KillcamSetAttackers'),
]

with tempfile.TemporaryDirectory(prefix='cod2-command-prefix-') as directory:
    path = Path(directory)
    for index, (old, new) in enumerate([(None, None)] + mutants):
        if old:
            assert head.count(old) == 1, old
        body = head.replace(old, new) if old else head
        failed = False
        for flags in ([], ['-D__EMSCRIPTEN__']):
            (path / 'test.c').write_text(support + body + checks)
            subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', *flags,
                            str(path / 'test.c'), '-o', str(path / 'test')], check=True)
            result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
            if index == 0:
                assert result.returncode == 0, (flags, result.stderr[-1500:])
            failed |= result.returncode != 0
        assert index == 0 or failed, index
print('PASS: unhandled cod2_ commands return before the retail first-letter handlers, natively and in the browser; '
      'cod2_points, cod2_obituary, the deathmatch scores (one score, or a whole table that replaces the old one) and '
      'the killcam\'s attackers (killer first) still reach their handlers and retail commands their letter; '
      f'{len(mutants)} failing mutants')
