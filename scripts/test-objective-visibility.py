#!/usr/bin/env python3
"""Run the production per-client objective copy from G_RunFrame for every team."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/game_mp/g_main_mp.c').read_text()
marker = '            for (j = 0; j < 16; j++) {\n                objective_t *srcObj = &level.objectives[j];'
assert source.count(marker) == 1
start = source.index(marker)
brace = source.index('{', start)
depth, end = 1, brace + 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
body = source[start:end] + '\n'

support = r'''
#include <assert.h>
#include <string.h>
typedef float vec3_t[3];
typedef enum { OBJST_EMPTY, OBJST_ACTIVE, OBJST_INVISIBLE, OBJST_DONE, OBJST_CURRENT, OBJST_FAILED } objectiveState_t;
typedef struct { objectiveState_t state; vec3_t origin; int entNum; int teamNum; int icon; } objective_t;
typedef struct { struct { objective_t objective[16]; } ps; struct { struct { int team; } cs; } sess; } gclient_t;
static struct { objective_t objectives[16]; } level;
static void update(gclient_t *cli) {
    int j;
    int lastObjId = cli->sess.cs.team;
'''

checks = r'''
}
static int same(const objective_t *a, const objective_t *b) { return !memcmp(a, b, sizeof(*a)); }
int main(void) {
    enum { FREE, AXIS, ALLIES, SPECTATOR };
    static gclient_t clients[4];
    /* A flag at base and an S&D bomb site have no team, a carried flag
     * belongs to its carrier's team, a held HQ radio to its holders. */
    level.objectives[0] = (objective_t){OBJST_CURRENT, {100, 200, 8}, 1023, 0, 5};
    level.objectives[1] = (objective_t){OBJST_CURRENT, {-40, 75, 0}, 12, ALLIES, 6};
    level.objectives[2] = (objective_t){OBJST_EMPTY, {9, 9, 9}, 1023, 0, 7};
    level.objectives[3] = (objective_t){OBJST_ACTIVE, {1, 2, 3}, 1023, AXIS, 8};
    for (int team = FREE; team <= SPECTATOR; team++) {
        gclient_t *cli = &clients[team];
        cli->sess.cs.team = team;
        for (int j = 0; j < 16; j++)
            cli->ps.objective[j] = (objective_t){OBJST_CURRENT, {-1, -1, -1}, 3, team, 2};
        update(cli);
        assert(same(&cli->ps.objective[0], &level.objectives[0]));
        assert(team == ALLIES ? same(&cli->ps.objective[1], &level.objectives[1]) : cli->ps.objective[1].state == OBJST_EMPTY);
        assert(team == AXIS ? same(&cli->ps.objective[3], &level.objectives[3]) : cli->ps.objective[3].state == OBJST_EMPTY);
        /* Hidden or unused slots only lose their state. */
        assert(cli->ps.objective[2].state == OBJST_EMPTY && cli->ps.objective[2].origin[0] == -1 && cli->ps.objective[2].icon == 2);
        for (int j = 4; j < 16; j++) assert(cli->ps.objective[j].state == OBJST_EMPTY && cli->ps.objective[j].entNum == 3);
    }
    return 0;
}
'''


def mutate(old, new):
    assert body.count(old) == 1, old
    return body.replace(old, new)


condition = 'if (srcObj->state != 0 && (srcObj->teamNum == 0 || srcObj->teamNum == lastObjId)) {'
mutants = [
    mutate(condition + '\n                    *dstObj = *srcObj;\n                } else {\n                    dstObj->state = 0;\n                }',
           'if (srcObj->state == 0 || srcObj->teamNum == 0) {\n                    if (srcObj->state != 0 && srcObj->teamNum == lastObjId) {\n'
           '                        *dstObj = *srcObj;\n                    } else {\n                        dstObj->state = 0;\n                    }\n'
           '                } else {\n                    *dstObj = *srcObj;\n                }'),
    mutate(condition, 'if (srcObj->state != 0 && srcObj->teamNum == lastObjId) {'),
    mutate(condition, 'if (srcObj->state != 0 && srcObj->teamNum == 0) {'),
    mutate(condition, 'if (srcObj->teamNum == 0 || srcObj->teamNum == lastObjId) {'),
    mutate(condition, 'if (srcObj->state != 0) {'),
]
with tempfile.TemporaryDirectory(prefix='cod2-objectives-') as directory:
    path = Path(directory) / 'objectives.c'
    binary = Path(directory) / 'objectives'
    for index, variant in enumerate([body] + mutants):
        path.write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        str(path), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: objectives without a team reach free, axis, allied and spectating players; team objectives reach only '
      f'their team; empty and hidden slots only lose their state; {len(mutants)} failing regression mutants '
      '(including the previous inverted copy)')
