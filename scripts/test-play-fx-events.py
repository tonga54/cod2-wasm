#!/usr/bin/env python3
"""The client's playFx and playFxOnTag events (cg_event_mp.c): each plays the effect the server named by its loadFx id,
from the client's own table of them, cgs.fxs. The browser runs CoD2 1.3's cgs, whose game state grew from 0x3e80 to
0x20000 characters; reading the table at the 1.0 layout's offsets read that game state instead, and so the retail
minefields, the S&D bomb, the HQ radio and every exploder played the Missing FX sprite. playFx read one entry early
besides. playFxOnTag bolts its effect to the tag it names on the entity."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
events = (root / 'src/PC/cgame_mp/cg_event_mp.c').read_text()
assert '(char *)cgs + CGS_FX' not in events and 'CGS_FX_DEATHFX' not in events and 'CGS_FX_CUSTOM' not in events


def case(head):
    assert events.count(head) == 1, head
    start = events.index(head)
    end, depth = events.index('{', start) + 1, 1
    while depth:
        depth += (events[end] == '{') - (events[end] == '}')
        end += 1
    return events[start:end] + '\n'


block = case('        case 0xc2:\n') + case('        case 0xc3:\n')

support = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef unsigned short scr_string_t;
typedef struct { float trBase[3]; } trajectory_t;
typedef struct { int number; int eventParm; trajectory_t apos; } entityState_t;
typedef struct { entityState_t nextState; } centity_t;
typedef struct EffectTemplate { int id; } EffectTemplate;
/* The 1.3 client's cgs: the game state and the fields after it up to the
 * effects table at 0x22940, where the 1.0 layout has it at 0x67c0. */
typedef struct { unsigned char beforeFxs[0x22940]; EffectTemplate *fxs[64]; } cgs_t;
static cgs_t cgsState, *cgs = &cgsState;
static EffectTemplate effects[64];

static int errors;
static void Com_Printf(const char *fmt, ...) { (void)fmt; ++errors; }
static float angledForward[3] = { 0.0f, 0.0f, 1.0f };
static const float *anglesSeen;
static void AngleVectors(float *angles, float *forward, void *right, float *up)
{
    assert(!right);
    anglesSeen = angles;
    memcpy(forward, angledForward, sizeof(angledForward));
    up[0] = 1.0f; up[1] = up[2] = 0.0f;
}
static int plays, playedEffect;
static const void *playedOrigin;
static float playedDir[3];
static void FX_PlayEffect(int effectId, void *origin, float *dir)
{
    ++plays;
    playedEffect = effectId;
    playedOrigin = origin;
    memcpy(playedDir, dir, sizeof(playedDir));
}
static int entityPlays, entityEffect, entityBone, bolted[2];
static const void *entityOrigin;
static void FX_PlayEntityEffect(int effectId, void *origin, int boneIndex, int *bolt)
{
    ++entityPlays;
    entityEffect = effectId;
    entityOrigin = origin;
    entityBone = boneIndex;
    bolted[0] = bolt[0];
    bolted[1] = bolt[1];
}
static const char *configStrings[0x500];
static const char *CL_GetConfigString(int index)
{
    assert(index >= 0 && index < 0x500);
    return configStrings[index] ? configStrings[index] : "";
}
/* Tags by name, as script strings; each taken is given back. */
static char tagNames[8][32];
static int tagCount, liveTags, boneOf = 3, bonesAsked;
static unsigned short SL_GetString(const char *str, int user)
{
    assert(user == 0 && tagCount < 8);
    snprintf(tagNames[tagCount], sizeof(tagNames[0]), "%s", str);
    ++liveTags;
    return (unsigned short)(++tagCount);
}
static void Scr_SetString(scr_string_t *str, unsigned int value)
{
    assert(*str && value == 0);
    --liveTags;
    *str = 0;
}
static int FX_GetBoneIndex(int entNum, int tag)
{
    ++bonesAsked;
    assert(entNum == 17 && tag == tagCount);
    return boneOf;
}

static float positionVec[3] = { 100.0f, 200.0f, 300.0f };
static void Event(centity_t *cent, int event)
{
    entityState_t *es = &cent->nextState;
    char *position = (char *)positionVec;
    float forward[3], up[3];
    unsigned short tagName;
    switch (event) {
'''

checks = r'''
    }
}

static void reset(void)
{
    plays = entityPlays = errors = bonesAsked = 0;
    playedEffect = entityEffect = 0;
    tagCount = 0;
}

int main(void)
{
    /* Where the 1.0 layout's table would be the game state holds text. */
    memset(cgsState.beforeFxs, 'Z', sizeof(cgsState.beforeFxs));
    for (int i = 1; i < 64; ++i) {
        effects[i].id = i;
        cgs->fxs[i] = &effects[i];
    }
    centity_t cent = { { 17, 0, { { -90.0f, 0.0f, 0.0f } } } };

    /* playFx plays the effect of its id, 1 to 63, along the angles it sent. */
    for (int id = 1; id < 64; ++id) {
        reset();
        cent.nextState.eventParm = id;
        Event(&cent, 0xc2);
        assert(plays == 1 && playedEffect == (int)(intptr_t)&effects[id] && !errors);
        assert(playedOrigin == positionVec && anglesSeen == cent.nextState.apos.trBase);
        assert(!memcmp(playedDir, angledForward, sizeof(playedDir)));
    }
    /* Any other id plays nothing and says so. */
    static const int invalid[] = { 0, 64, 200, 255 };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        reset();
        cent.nextState.eventParm = invalid[i];
        Event(&cent, 0xc2);
        assert(!plays && errors == 1);
    }

    /* playFxOnTag's config string names the id and the tag: the effect plays
     * bolted to that tag of the entity, and the tag's name is given back. */
    static const struct { const char *config; int id; const char *tag; } tagged[] = {
        { "07tag_flash", 7, "tag_flash" }, { "01tag_origin", 1, "tag_origin" }, { "63j_head", 63, "j_head" },
        { "40tag_weapon_right", 40, "tag_weapon_right" },
    };
    for (unsigned i = 0; i < sizeof(tagged) / sizeof(tagged[0]); ++i) {
        reset();
        cent.nextState.eventParm = 5 + (int)i;
        configStrings[0x38e + 5 + i] = tagged[i].config;
        Event(&cent, 0xc3);
        assert(entityPlays == 1 && entityEffect == (int)(intptr_t)&effects[tagged[i].id]);
        assert(entityOrigin == positionVec && entityBone == 0 && bolted[0] == 17 && bolted[1] == 3);
        assert(!strcmp(tagNames[0], tagged[i].tag) && !liveTags && bonesAsked == 1);
    }
    /* Without the tag on the entity nothing plays, and the name is still given back. */
    boneOf = -1;
    reset();
    cent.nextState.eventParm = 5;
    Event(&cent, 0xc3);
    assert(!entityPlays && !liveTags && bonesAsked == 1);
    boneOf = 3;
    /* An id outside 1 to 63 plays nothing and asks for no tag. */
    static const char *const badConfigs[] = { "00tag_flash", "64tag_flash", "99tag_flash", "", "x1tag_flash" };
    for (unsigned i = 0; i < sizeof(badConfigs) / sizeof(badConfigs[0]); ++i) {
        reset();
        cent.nextState.eventParm = 20;
        configStrings[0x38e + 20] = badConfigs[i];
        Event(&cent, 0xc3);
        assert(!entityPlays && !bonesAsked && !liveTags);
    }
    puts("ok");
    return 0;
}
'''

mutants = [
    # The 1.0 layout's table, and playFx's id one early, as the client read them.
    ('FX_PlayEffect((int)(intptr_t)cgs->fxs[fxIndex], position, forward);',
     'FX_PlayEffect(*(int *)((char *)cgs + 0x67c0 + (fxIndex - 1) * 4), position, forward);'),
    ('FX_PlayEffect((int)(intptr_t)cgs->fxs[fxIndex], position, forward);',
     'FX_PlayEffect((int)(intptr_t)cgs->fxs[fxIndex - 1], position, forward);'),
    ('FX_PlayEntityEffect((int)(intptr_t)cgs->fxs[fxIdx2], position, 0, bolt);',
     'FX_PlayEntityEffect(*(int *)((char *)cgs + 0x67c0 + fxIdx2 * 4), position, 0, bolt);'),
    ('            if ((unsigned int)(fxIndex - 1) > 62) {', '            if ((unsigned int)(fxIndex - 1) > 63) {'),
    ('            if ((unsigned int)(fxIndex - 1) > 62) {', '            if (fxIndex > 63) {'),
    ('(csStr2[0] - \'0\') * 10 + (csStr2[1] - \'0\')', '(csStr2[1] - \'0\') * 10 + (csStr2[0] - \'0\')'),
    ('csStr2[0] >= \'0\' && csStr2[0] <= \'9\' && csStr2[1] >= \'0\' && csStr2[1] <= \'9\' ?',
     'csStr2[1] >= \'0\' && csStr2[1] <= \'9\' ?'),
    ('                if ((unsigned int)(fxIdx2 - 1) > 62)\n                    return;\n', ''),
    ('                if (bolt[1] < 0)\n                    return;\n', ''),
    ('bolt[1] = FX_GetBoneIndex(bolt[0], (int)tagName);', 'bolt[1] = FX_GetBoneIndex(bolt[0], (int)tagName) ? 0 : 0;'),
    ('                Scr_SetString(&tagName, 0);\n\n                if (bolt[1] < 0)\n                    return;\n',
     '                if (bolt[1] < 0)\n                    return;\n\n                Scr_SetString(&tagName, 0);\n'),
]

with tempfile.TemporaryDirectory(prefix='cod2-play-fx-') as directory:
    for index, source in enumerate([block] + [None] * len(mutants)):
        if index:
            old, new = mutants[index - 1]
            assert block.count(old) == 1, old
            source = block.replace(old, new)
        path = Path(directory) / f'test{index}.c'
        path.write_text(support + source + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if not index else '-w', '-Wno-unused-function', str(path), '-o',
                        str(path.with_suffix(''))], check=True)
        result = subprocess.run([str(path.with_suffix(''))], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-1500:])
print(f'PASS: playFx plays the effect of each id 1 to 63 from the client\'s own table, along its angles, and nothing for '
      f'any other id; playFxOnTag bolts the effect its config string names to the tag on the entity, gives the tag\'s '
      f'name back and plays nothing for a missing tag or an id out of range; {len(mutants)} mutants fail, the 1.0 '
      f'layout\'s offsets among them')
