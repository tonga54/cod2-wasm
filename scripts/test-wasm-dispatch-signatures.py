#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parent.parent
game = root / 'src/PC/game_mp'
sources = {path.name: path.read_text() for path in game.glob('*.c')}
sources['sv_client_mp.c'] = (root / 'src/PC/server_mp/sv_client_mp.c').read_text()


def table(text, start):
    begin = text.index(start)
    return text[begin:text.index('\n};', begin)]


def definition(name):
    pattern = re.compile(r'^(?:static\s+)?([A-Za-z_][A-Za-z0-9_ ]*?[\s*]+)' + re.escape(name) + r'\s*\(([^)]*)\)\s*$', re.M)
    for text in sources.values():
        match = pattern.search(text)
        if match:
            result = 'void' if match.group(1).strip() == 'void' else 'i32'
            params = [part for part in match.group(2).split(',') if part.strip() not in ('', 'void')]
            return '(' + ','.join('i32' for _ in params) + ')->' + result
    raise AssertionError('no definition for ' + name)


handlers = table(sources['g_main_mp.c'], 'entityHandler_t entityHandlers[20] = {')
slots = ('think', 'reached', 'blocked', 'touch', 'use', 'pain', 'die', 'controller')
expected = {'think': '(i32)->void', 'reached': '(i32)->void', 'blocked': '(i32,i32)->void',
            'touch': '(i32,i32,i32)->void', 'use': '(i32,i32,i32)->void',
            'pain': '(i32,i32,i32,i32,i32,i32,i32)->void',
            'die': '(i32,i32,i32,i32,i32,i32,i32,i32,i32)->void', 'controller': '(i32,i32)->void'}
members = {slot: set() for slot in slots}
for row in re.findall(r'\{([^{}]*)\}', handlers):
    for slot, value in zip(slots, row.split(',')):
        name = value.strip().lstrip('&')
        if name != '0x0':
            members[slot].add(name)
spawn_table = table(sources['g_spawn_mp.c'], 'spawn_t spawns[24] = {')
members['spawn'] = set(re.findall(r',\s*&([A-Za-z_][A-Za-z0-9_]*)\s*\}', spawn_table))
expected['spawn'] = '(i32)->void'
members['ucmd'] = set(re.findall(r',\s*&([A-Za-z_][A-Za-z0-9_]*)\s*\}', table(sources['sv_client_mp.c'], 'ucmd_t ucmds[12] = {')))
expected['ucmd'] = '(i32)->void'
exceptions = {'G_FreeEntity'}

for slot, names in members.items():
    for name in names - exceptions:
        assert definition(name) == expected[slot], (slot, name, definition(name))
assert {'G_FreeEntity'} <= members['think'] and {'G_FreeEntity'} <= members['spawn']
assert len(members['spawn']) == 19 and len(members['ucmd']) == 11
assert definition('G_FreeEntity') == '(i32)->i32'

think = sources['g_main_mp.c']
think = think[think.index('G_RunThink_core(gentity_t *ent)\n{'):]
think = think[:think.index('\n}\n')]
assert ('if ((void *)think == (void *)G_FreeEntity)\n        G_FreeEntity(ent);\n    else\n'
        '        ((void (*)(gentity_t *))think)(ent);') in think
assert 'int (*)(gentity_t *)' not in think
assert '((void (*)(gentity_t *, gentity_t *, gentity_t *))useFn)(useEnt, ent, ent);' in sources['player_use_mp.c']
combat = sources['g_combat_mp.c']
assert 'void (*pain)(gentity_t *, gentity_t *, int, const vec_t *, int, const vec_t *, hitLocation_t) =' in combat
assert 'pain(targ, attacker, damage, point, mod, localdir, hitLoc);' in combat
assert 'void (*die)(gentity_t *, gentity_t *, gentity_t *, int, int, int, const vec_t *, hitLocation_t, int) =' in combat
spawn = sources['g_spawn_mp.c']
assert ('if ((void *)spawns[i].spawn == (void *)G_FreeEntity)\n                G_FreeEntity(ent);\n'
        '            else\n                ((void (*)(gentity_t *))spawns[i].spawn)(ent);') in spawn
assert '(void (*)(gentity_t *))spawn->spawn != (void (*)(gentity_t *))imp_G_FreeEntity' in spawn
bots = (root / 'src/PC/server_mp/sv_bot_ai.inc').read_text()
assert bots.index('extern float Dvar_GetFloat(const char *dvarName);') < bots.index('Dvar_GetFloat("')

if len(sys.argv) > 1:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from wasm_signatures import signatures
    compiled = signatures(Path(sys.argv[1]) / 'cod2.wasm')
    builtins = sources['g_scr_main_mp.c']
    tables = {'()->void': [table(builtins, 'BuiltinFunctionDef functions[145] = {')],
              '(i32)->void': [table(builtins, 'BuiltinMethodDef methods[59] = {'),
                              table(sources['g_client_script_cmd_mp.c'], 'static const BuiltinMethodDef player_methods[]'),
                              table(sources['g_scr_mover_mp.c'], 'static const BuiltinMethodDef methods[] = {'),
                              table(sources['g_hudelem_mp.c'], 'static const BuiltinMethodDef methods[]')]}
    checked = 0
    for signature, texts in tables.items():
        for text in texts:
            for name in re.findall(r'\{\s*"[^"]+"\s*,\s*(?:\([A-Za-z_ *]+\))?\s*&?([A-Za-z_][A-Za-z0-9_]*)', text):
                if name in compiled:
                    assert compiled[name] == signature, (name, compiled[name], signature)
                    checked += 1
    for slot, names in members.items():
        for name in names - exceptions:
            if name in compiled:
                assert compiled[name] == expected[slot], (slot, name, compiled[name])
                checked += 1
    assert compiled['G_FreeEntity'] == '(i32)->i32' and checked > 300
print('PASS: entity handler, spawn, client command and script builtin tables match the WebAssembly signatures their dispatchers call')
