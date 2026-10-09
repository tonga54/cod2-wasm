#!/usr/bin/env python3
"""Packet entities must receive a current pose even without a new event."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_ents_mp.c').read_text()
body = source[source.index('void CG_AddPacketEntities(void)\n{'):]
support = r'''
#include <assert.h>
typedef struct { int number, eType; } entityState_t;
typedef struct { entityState_t nextState; int poseFrame, localClientNum; } centity_t;
typedef struct { int numEntities; entityState_t entities[4]; struct { int clientNum, pm_flags; } ps; } snapshot_t;
typedef struct {
    float rumbleScale, autoAnglesSlow[3], autoAngles[3], autoAnglesFast[3];
    float autoAxisSlow[9], autoAxis[9], autoAxisFast[9];
    int time;
    struct { int viewlocked_entNum; } predictedPlayerState;
    snapshot_t *nextSnap;
} cg_t;
static cg_t state, *cg = &state;
static centity_t entities[4], *entityBase = entities;
static void *imp_cg_entities = &entityBase;
static int frame, calculated, processed, animated, notified;
struct DObj_s { int frame; } models[4];
static struct DObj_s *Com_GetClientDObj(int num, int local) { return &models[num]; }
static void CG_DObjUpdateInfo(struct DObj_s *obj) { assert(obj->frame != frame); obj->frame=frame; ++animated; }
static void CG_ProcessClientNoteTracks(int num) { assert(models[num].frame==frame); ++notified; }
static void AnglesToAxis(float *angles, float *axis) {}
static void CG_CalcEntityLerpPositions(centity_t *cent) {
    cent->poseFrame = frame;
    ++calculated;
}
static void CG_ProcessEntity(centity_t *cent) {
    assert(cent->poseFrame == frame);
    assert(cent->nextState.number != cg->predictedPlayerState.viewlocked_entNum);
    ++processed;
}
'''
checks = r'''
int main(void) {
    snapshot_t snap = {.numEntities=4};
    state.nextSnap = &snap;
    snap.ps.clientNum = 1; snap.ps.pm_flags = 0xc00000;
    state.predictedPlayerState.viewlocked_entNum = 2;
    for (int i=0; i<4; ++i) {
        snap.entities[i].number = i;
        entities[i].nextState.number = i;
        entities[i].nextState.eType = i == 3 ? 11 : 1;
    }
    for (frame=1; frame<=10; ++frame) {
        calculated = processed = animated = notified = 0;
        state.time = frame * 16;
        CG_AddPacketEntities();
        assert(calculated == 3 && processed == 2);
        assert(animated == 2 && notified == 2);
        assert(models[1].frame == 0);
        assert(entities[2].poseFrame == frame);
        assert(entities[3].poseFrame == 0);
    }
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-packet-positions-') as directory:
    path = Path(directory)
    for mutant in (False, True):
        altered = body.replace('CG_CalcEntityLerpPositions(cent);', '') if mutant else body
        (path / 'test.c').write_text(support + altered + checks)
        subprocess.run(['cc', '-std=c99', str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True)
        assert (result.returncode == 0) != mutant, result.stderr
print('PASS: normal/viewlocked poses and animations update each frame; predicted animation stays separate; old pose behavior fails')
