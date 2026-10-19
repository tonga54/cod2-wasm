#!/usr/bin/env python3
"""Run the production freezeControls command filter: a frozen player looks and aims, nothing else."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
pmove = (root / 'src/PC/bgame/bg_pmove.c').read_text()
active = (root / 'src/PC/game_mp/g_active_mp.c').read_text()
inputs = (root / 'src/PC/client_mp/cl_input.c').read_text()
bg_funcs = (root / 'src/headers/PC/bgame/bg_funcs.h').read_text()
server = (root / 'src/PC/server_mp/sv_main_mp.c').read_text()
script = (root / 'src/PC/game_mp/g_client_script_cmd_mp.c').read_text()


def function(text, head):
    assert text.count(head) == 1, head
    start = text.index(head)
    depth, end = 1, text.index('{', start) + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def ordered(text, *parts):
    at = -1
    for part in parts:
        assert text.count(part) == 1, part
        assert text.index(part) > at, part
        at = text.index(part)


# freezeControls sets the flag the server's think copies into the player
# state each command, so prediction sees the same freeze.
assert function(script, 'void PlayerCmd_FreezeControls(scr_entref_t entref)\n{').count(
    '    pSelf->client->bFrozen = Scr_GetInt(0);\n') == 1
# The server filters the command before its buttons reach the scripts, use
# triggers and grenade input, and pmove filters it again for prediction,
# before any of its moves.
ordered(function(active, 'void ClientThink_real(gentity_t *ent, usercmd_t *ucmd)\n{'),
        '    if (client->bFrozen)\n        client->ps.pm_flags |= 0x8000;\n    else\n        client->ps.pm_flags &= ~0x8000;\n'
        '    /* Before the buttons reach the scripts, triggers and grenade input. */\n'
        '    PM_FilterFrozenCommand(&client->ps, ucmd);\n',
        '        SpectatorThink(ent, ucmd);\n',
        '    pm.cmd = *ucmd;\n',
        '    client->buttons = ucmd->buttons;\n    client->latched_buttons',
        '    G_BeginGrenadeInput(ent, &pm.cmd);\n    Pmove(&pm);\n')
ordered(function(pmove, 'void Pmove(pmove_t *pm)\n{'),
        '        ps->commandTime = finalTime - 1000;\n',
        '    PM_FilterFrozenCommand(ps, &pm->cmd);\n',
        '    while (ps->commandTime != finalTime) {\n')
assert bg_funcs.count('void PM_FilterFrozenCommand(const playerState_t *ps, usercmd_t *cmd);\n') == 1
# A frozen player still turns the view with the mouse, as with a controller.
mouse = function(inputs, 'void CL_MouseMove(usercmd_t *cmd)\n{')
assert 'pm_flags' not in mouse and 'cl->viewangles[1] -= yawDelta;' in mouse

# The button masks are the usercmd contract the bots' actions name.
actions = dict(re.findall(r'\{ "(\w+)",\s+(0x[0-9a-f]+) \}', server[server.index('s_botActions[]'):]))
kept = ('ads', 'holdbreath', 'binoculars')
blocked = ('fire', 'melee', 'activate', 'reload', 'usereload', 'leanleft', 'leanright', 'goprone', 'gocrouch',
           'gostand', 'jump', 'frag', 'smoke')
assert set(actions) == set(kept + blocked), sorted(actions)

source = pmove[pmove.index('#define PMF_FROZEN '):pmove.index('#define ENTITYNUM_WORLD')]
source += function(pmove, 'void PM_FilterFrozenCommand(const playerState_t *ps, usercmd_t *cmd)\n{')
support = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char byte;
typedef struct { int pm_type; int pm_flags; } playerState_t;
typedef struct { int serverTime; int buttons; byte weapon; byte offHandIndex; int angles[3]; signed char forwardmove;
                 signed char rightmove; } usercmd_t;
'''
checks = r'''
/* The bots' action masks, then the client's sprint, shellshock and console. */
static const int kept[] = { ''' + ', '.join(actions[name] for name in kept) + r''', 0x800, 0x40000 };
static const int blocked[] = { ''' + ', '.join(actions[name] for name in blocked) + r''', 0x2 };
static usercmd_t command(int buttons) {
    usercmd_t cmd = { 1234, buttons, 7, 2, { 100, -200, 300 }, 127, -127 };
    return cmd;
}
static int same(const usercmd_t *a, const usercmd_t *b) { return !memcmp(a, b, sizeof(*a)); }
int main(void) {
    int all = 0;
    for (unsigned i = 0; i < sizeof(kept) / sizeof(kept[0]); ++i)
        all |= kept[i];
    for (unsigned i = 0; i < sizeof(blocked) / sizeof(blocked[0]); ++i)
        all |= blocked[i];
    /* Not frozen, nothing changes. */
    for (int type = 0; type < 8; ++type) {
        playerState_t ps = { type, 0x10000 | 0x2 };
        usercmd_t cmd = command(all), before = cmd;
        PM_FilterFrozenCommand(&ps, &cmd);
        assert(same(&cmd, &before));
    }
    /* Frozen while playing, or linked: no movement and only looking and
     * aiming survive, each on its own and all together; the view, the weapon
     * and the offhand choice stay. */
    for (int type = 0; type < 2; ++type) {
        playerState_t ps = { type, 0x8000 };
        for (unsigned i = 0; i < sizeof(kept) / sizeof(kept[0]); ++i) {
            usercmd_t cmd = command(kept[i]);
            PM_FilterFrozenCommand(&ps, &cmd);
            assert(cmd.buttons == kept[i] && !cmd.forwardmove && !cmd.rightmove);
        }
        for (unsigned i = 0; i < sizeof(blocked) / sizeof(blocked[0]); ++i) {
            usercmd_t cmd = command(blocked[i]);
            PM_FilterFrozenCommand(&ps, &cmd);
            assert(!cmd.buttons);
        }
        usercmd_t cmd = command(all), before = cmd;
        PM_FilterFrozenCommand(&ps, &cmd);
        assert(cmd.buttons == (0x1000 | 0x8000 | 0x4000 | 0x800 | 0x40000));
        before.buttons = cmd.buttons;
        before.forwardmove = before.rightmove = 0;
        assert(same(&cmd, &before));
    }
    /* Noclip, ufo, spectators, the intermission and the dead keep their command. */
    for (int type = 2; type < 8; ++type) {
        playerState_t ps = { type, 0x8000 };
        usercmd_t cmd = command(all), before = cmd;
        PM_FilterFrozenCommand(&ps, &cmd);
        assert(same(&cmd, &before));
    }
    puts("PASS");
    return 0;
}
'''

mutants = [
    ('    if (!(ps->pm_flags & PMF_FROZEN) || ps->pm_type > 1)', '    if (!(ps->pm_flags & PMF_FROZEN))'),
    ('    if (!(ps->pm_flags & PMF_FROZEN) || ps->pm_type > 1)', '    if (!(ps->pm_flags & PMF_FROZEN) || ps->pm_type > 0)'),
    ('    if (!(ps->pm_flags & PMF_FROZEN) || ps->pm_type > 1)', '    if (ps->pm_type > 1)'),
    ('    cmd->forwardmove = 0;\n', ''),
    ('    cmd->rightmove = 0;\n', ''),
    ('    cmd->buttons &= PMF_FROZEN_BUTTONS;\n', '    cmd->buttons &= PMF_FROZEN_BUTTONS | 0x400;\n'),
    ('    cmd->buttons &= PMF_FROZEN_BUTTONS;\n', '    cmd->buttons = 0;\n'),
    ('(0x800 | 0x1000 | 0x4000 | 0x8000 | 0x40000)', '(0x800 | 0x1000 | 0x4000 | 0x8000 | 0x40000 | 0x1)'),
    ('(0x800 | 0x1000 | 0x4000 | 0x8000 | 0x40000)', '(0x800 | 0x4000 | 0x8000 | 0x40000)'),
    ('#define PMF_FROZEN 0x00008000', '#define PMF_FROZEN 0x00000800'),
]
with tempfile.TemporaryDirectory(prefix='cod2-frozen-') as directory:
    path = Path(directory) / 'frozen.c'
    binary = Path(directory) / 'frozen'
    variants = [source] + [source.replace(old, new) for old, new in mutants]
    for (old, _), variant in zip(mutants, variants[1:]):
        assert source.count(old) == 1 and variant != source, old
    for index, variant in enumerate(variants):
        path.write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if index == 0 else '-w', str(path), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-1000:])
print('PASS: a player held by freezeControls keeps looking, aiming, binoculars and holding breath while every move, '
      'shot, melee, use, reload, lean, stance change, jump and grenade is dropped, on the server before its scripts '
      'and triggers see the buttons and in pmove for prediction; the dead, spectators and noclip keep their commands, '
      f'and the mouse still turns a frozen view; {len(mutants)} failing regression mutants')
