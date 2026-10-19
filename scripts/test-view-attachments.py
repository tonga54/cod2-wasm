#!/usr/bin/env python3
"""The first-person gun wears the class's visible attachments: cg_attachmodels_mp.c finds each one's model once,
puts it on the bone it rides and swaps the gun for the one without its magazine under a longer magazine, or puts
a rifle's box magazine on its receiver, and cg_weapons.c rebuilds the gun's models whenever the attachments it
should show change."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
module = (root / 'src/PC/cgame_mp/cg_attachmodels_mp.c').read_text()
weapons = (root / 'src/PC/cgame_mp/cg_weapons.c').read_text()

stub_types = r'''
#pragma once
typedef int qboolean;
struct XModel { const char *name; int bad; };
typedef struct DObjModel_s { struct XModel *model; const char *boneName; qboolean ignoreCollision; } DObjModel_s;
'''
checks = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common_types.h"
#include "headers/PC/bgame/bg_attachment_rules.h"
#include "headers/PC/cgame_mp/cg_attachmodels.h"

/* The archives' models, one of them broken. */
static const char *present[] = {
    "xmodel/viewmodel_thompson__suppressor", "xmodel/viewmodel_thompson__reflex", "xmodel/viewmodel_thompson__extmag",
    "xmodel/viewmodel_thompson__nomag", "xmodel/viewmodel_colt__extmag", "xmodel/viewmodel_mp44__lens",
    "xmodel/viewmodel_mp44__optic4x", "xmodel/viewmodel_kar98k__extbox", "xmodel/viewmodel_kar98k__optic4x",
    "xmodel/viewmodel_thompson__extbox",
};
static const char *broken = "xmodel/viewmodel_mp44__lens";
static char readNames[64][96];
static int reads, registers;
static struct XModel loaded[32];

int FS_ReadFile(const char *qpath, void **buffer) {
    assert(!buffer && reads < 64);
    snprintf(readNames[reads++], sizeof(readNames[0]), "%s", qpath);
    for (unsigned i = 0; i < sizeof(present) / sizeof(present[0]); i++)
        if (!strcmp(present[i], qpath))
            return 1000;
    return -1;
}
struct XModel *CL_RegisterModel(const char *name) {
    assert(registers < 32);
    loaded[registers].name = strdup(name);
    loaded[registers].bad = !strcmp(name, broken);
    return &loaded[registers++];
}
int XModelBad(void *model) { return ((struct XModel *)model)->bad; }

static struct XModel hands = { "xmodel/viewmodel_hands_cloth", 0 }, gun = { "xmodel/viewmodel_thompson", 0 };
static DObjModel_s models[CG_VIEW_DOBJ_MODELS];
static int build(int weapon, const char *gunModel, unsigned mask, int count) {
    memset(models, 0, sizeof(models));
    models[0].model = &hands;
    models[1].model = &gun;
    models[1].boneName = "tag_weapon";
    return CG_ViewModelAttachmentModels(weapon, gunModel, mask, models, count);
}
static int read(const char *name) {
    int count = 0;
    for (int i = 0; i < reads; i++)
        count += !strcmp(readNames[i], name);
    return count;
}
static void on(int index, const char *name, const char *bone) {
    assert(models[index].model && !strcmp(models[index].model->name, name));
    assert(!strcmp(models[index].boneName, bone) && models[index].ignoreCollision == 1);
}

int main(void) {
    int cases = 0;

    /* What changes the gun's model: the can, the three sights and the longer magazine. */
    unsigned visible = ATT_BIT(ATT_SUPPRESSOR) | ATT_BIT(ATT_REFLEX) | ATT_BIT(ATT_LENS) | ATT_BIT(ATT_OPTIC4X) |
                       ATT_BIT(ATT_EXTMAG);
    assert(CG_VisibleAttachments(~0u) == visible && CG_VisibleAttachments(ATT_BIT(ATT_GRIP) | ATT_BIT(ATT_FMJ)) == 0);
    cases++;

    /* Each on its bone, in order; under the longer magazine the gun without its own. */
    unsigned mask = ATT_BIT(ATT_GRIP) | ATT_BIT(ATT_EXTMAG) | ATT_BIT(ATT_REFLEX) | ATT_BIT(ATT_SUPPRESSOR);
    assert(build(5, "viewmodel_thompson", mask, 2) == 5);
    assert(models[0].model == &hands && !strcmp(models[1].model->name, "xmodel/viewmodel_thompson__nomag"));
    assert(!strcmp(models[1].boneName, "tag_weapon"));
    on(2, "xmodel/viewmodel_thompson__suppressor", "tag_flash");
    on(3, "xmodel/viewmodel_thompson__reflex", "j_gun");
    on(4, "xmodel/viewmodel_thompson__extmag", "j_clip");
    assert(registers == 4 && reads == 4);
    cases++;

    /* Looked up once: a model is registered once, a missing one never, even asked again. */
    assert(build(5, "viewmodel_thompson", mask | ATT_BIT(ATT_LENS), 2) == 5 && registers == 4 && reads == 5);
    assert(read("xmodel/viewmodel_thompson__lens") == 1);
    assert(build(5, "viewmodel_thompson", ATT_BIT(ATT_LENS), 2) == 2 && models[1].model == &gun && reads == 5);
    cases++;

    /* The longer magazine needs the gun without its own: without it, the gun keeps its magazine and the longer one
     * is not looked for, nor shown; a box magazine is, and with none the Extended Mag shows nothing. */
    assert(build(6, "viewmodel_colt", ATT_BIT(ATT_EXTMAG), 2) == 2 && models[1].model == &gun && !models[2].model);
    assert(read("xmodel/viewmodel_colt__nomag") == 1 && read("xmodel/viewmodel_colt__extmag") == 0);
    assert(read("xmodel/viewmodel_colt__extbox") == 1);
    cases++;

    /* A rifle loaded through the top keeps its gun and its stripper clip, its box magazine on the receiver before the
     * sights; a gun with the longer magazine never looks for one. */
    assert(build(8, "viewmodel_kar98k", ATT_BIT(ATT_EXTMAG) | ATT_BIT(ATT_OPTIC4X), 2) == 4 && models[1].model == &gun);
    on(2, "xmodel/viewmodel_kar98k__extbox", "j_gun");
    on(3, "xmodel/viewmodel_kar98k__optic4x", "j_gun");
    assert(read("xmodel/viewmodel_kar98k__extmag") == 0 && read("xmodel/viewmodel_thompson__extbox") == 0);
    assert(build(8, "viewmodel_kar98k", ATT_BIT(ATT_EXTMAG), CG_VIEW_DOBJ_MODELS) == CG_VIEW_DOBJ_MODELS);
    assert(read("xmodel/viewmodel_kar98k__extbox") == 1);
    cases++;

    /* A broken model is left off; the others still go on. */
    assert(build(7, "viewmodel_mp44", ATT_BIT(ATT_LENS) | ATT_BIT(ATT_OPTIC4X), 2) == 3);
    on(2, "xmodel/viewmodel_mp44__optic4x", "j_gun");
    cases++;

    /* The view DObj's room is kept. */
    assert(build(5, "viewmodel_thompson", ATT_BIT(ATT_SUPPRESSOR) | ATT_BIT(ATT_REFLEX), CG_VIEW_DOBJ_MODELS - 1) ==
           CG_VIEW_DOBJ_MODELS);
    on(CG_VIEW_DOBJ_MODELS - 1, "xmodel/viewmodel_thompson__suppressor", "tag_flash");
    assert(build(5, "viewmodel_thompson", ATT_BIT(ATT_SUPPRESSOR), CG_VIEW_DOBJ_MODELS) == CG_VIEW_DOBJ_MODELS);
    cases++;

    /* A weapon registered again looks its models up again; no weapon, or no gun model, has none. */
    int before = reads;
    CG_ViewModelAttachmentsForget(5);
    assert(build(5, "viewmodel_thompson", ATT_BIT(ATT_SUPPRESSOR), 2) == 3 && reads == before + 1);
    assert(build(0, "viewmodel_thompson", visible, 2) == 2 && build(128, "viewmodel_thompson", visible, 2) == 2);
    assert(build(9, "", visible, 2) == 2 && build(9, NULL, visible, 2) == 2 && reads == before + 1);
    CG_ViewModelAttachmentsForget(0);
    CG_ViewModelAttachmentsForget(128);
    cases++;

    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    ('        if (FS_ReadFile(name, NULL) > 0) {', '        if (1) {'),
    ('        s_cgAttachLooked[weapon][ride] = 1;\n', ''),
    ('            models[1].model = nomag;\n', '            (void)nomag;\n'),
    ('            if (model && !XModelBad(model))', '            if (model)'),
    ('i < CG_ATTACH_RIDES && count < CG_VIEW_DOBJ_MODELS;', 'i < CG_ATTACH_RIDES;'),
    ('        models[count].boneName = s_cgAttachRides[i].bone;\n        models[count].ignoreCollision = 1;',
     '        models[count].boneName = s_cgAttachRides[i].bone;\n        models[count].ignoreCollision = 0;'),
    ('                models[count].ignoreCollision = 1;', '                models[count].ignoreCollision = 0;'),
    ('    memset(s_cgAttachLooked[weapon], 0, sizeof(s_cgAttachLooked[weapon]));\n', ''),
    ('    if (weapon <= 0 || weapon >= CG_ATTACH_WEAPONS || !gunModel || !gunModel[0])\n        return NULL;',
     '    if (!gunModel)\n        return NULL;'),
    ('        if (nomag && CG_AttachModel(weapon, ride, gunModel, g_attachments[ATT_EXTMAG].name))',
     '        if (CG_AttachModel(weapon, ride, gunModel, g_attachments[ATT_EXTMAG].name))'),
    ('    { ATT_SUPPRESSOR, "tag_flash" },', '    { ATT_SUPPRESSOR, "j_gun" },'),
    ('"xmodel/%s__%s"', '"xmodel/%s_%s"'),
    ('            if (box && count < CG_VIEW_DOBJ_MODELS) {', '            if (box) {'),
    ('                models[count].boneName = "j_gun";', '                models[count].boneName = "j_clip";'),
    ('            models[1].model = nomag;\n        } else {', '            models[1].model = nomag;\n        }\n        {'),
    ('#define CG_ATTACH_EXTBOX (CG_ATTACH_RIDES + 1)', '#define CG_ATTACH_EXTBOX CG_ATTACH_NOMAG'),
]

with tempfile.TemporaryDirectory(prefix='cod2-view-attachments-') as directory:
    path = Path(directory)
    (path / 'common_types.h').write_text(stub_types)
    (path / 'checks.c').write_text(checks)

    def run(source, name):
        (path / f'{name}.c').write_text(source)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-I', str(path), '-I', str(root / 'src'),
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-Wall', '-Werror',
                        str(path / 'checks.c'), str(path / f'{name}.c'), str(root / 'src/PC/bgame/bg_attachment_rules.c'),
                        '-o', str(path / name)], check=True)
        return subprocess.run([str(path / name)], capture_output=True, text=True)

    result = run(module, 'attach')
    assert result.returncode == 0, result.stderr[-3000:]
    cases = int(result.stdout)
    for index, (old, new) in enumerate(mutants):
        assert module.count(old) == 1, old
        assert run(module.replace(old, new), f'mutant{index}').returncode != 0, f'mutant survived: {new!r}'


def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


# The view weapon follows the player's attachments each frame, before it is drawn.
add = function(weapons, 'CG_AddPlayerWeapon')
view = add.index('    if (bViewModel) {\n')
update = add.index('        CG_UpdateViewModelAttachments(weaponNum, BG_GetWeaponAttachments(playerState, weaponNum));\n')
assert view < update < add.index('        if (bDrawGun) {', view)
# It rebuilds the gun only when what shows changes, with the hands it has, whose name may lack the folder.
changed = function(weapons, 'CG_UpdateViewModelAttachments')
assert '    mask = CG_VisibleAttachments(mask);\n' in changed and 'mask == s_viewModelAttachments[weaponIdx])\n        return;' in changed
assert 'Com_sprintf(handModel, sizeof(handModel), "xmodel/%s", weapInfo->handModel);' in changed
assert changed.rstrip().endswith('CG_BuildViewModelDObj(weaponIdx, handModel, mask);\n}')
# Built on the hands and the gun, with the attachments unless either model is broken, then remembered.
built = function(weapons, 'CG_BuildViewModelDObj')
assert built.index('    if (XModelBad(dobjModels[0].model) || XModelBad(dobjModels[1].model)) {\n'
                   '        CG_Weapons_SetToDefault(weaponIdx, dobjModels);\n        mask = 0;\n    } else {\n'
                   '        count = CG_ViewModelAttachmentModels(weaponIdx, weapDef->szGunXModel, mask, dobjModels, count);\n'
                   '    }\n') < built.index('    Com_ClientDObjCreate(dobjModels, count, pAnimTree, dobjHandle);\n')
assert '        s_viewModelAttachments[weaponIdx] = mask;\n' in built and 'DObjModel_s dobjModels[CG_VIEW_DOBJ_MODELS];' in built
# New hands keep the attachments; a weapon registered again starts without them.
hands = function(weapons, 'CG_UpdateHandViewmodels')
assert 'CG_BuildViewModelDObj(weaponIdx, handModel,' in hands and '? s_viewModelAttachments[weaponIdx]' in hands
register = function(weapons, 'CG_RegisterWeapon')
assert register.index('I_strncpyz(weapInfo->handModel, weapDef->szHandXModel, sizeof(weapInfo->handModel));\n'
                      '        CG_ViewModelAttachmentsForget(weaponNum);\n') > 0
assert '            s_viewModelAttachments[weaponNum] = 0;\n' in register
print(f'PASS: {cases} cases: the can, the sights and the longer magazine on their bones, the gun without its magazine '
      f'under the longer one, a rifle\'s box magazine on its receiver with its gun kept, each model looked up once and '
      f'a missing or broken one left off, the view DObj\'s room '
      f'kept; {len(mutants)} mutants fail; the view weapon is rebuilt when its visible attachments change, keeps them '
      f'through new hands and drops them when registered again')
