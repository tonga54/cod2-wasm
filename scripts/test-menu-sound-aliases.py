#!/usr/bin/env python3
"""Which sound aliases a load keeps by their loadspec: a map's set takes the menus' own sounds too, so the class and
pause menus drawn in a match still play mouse_over and mouse_click."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/universal/com_sndalias_load_obj.c').read_text()


def function(name):
    match = re.search(r'static int ' + name + r'\([^;]*?\)\n\{', source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


body = function('Com_SoundAliasLoadspecNames') + function('Com_SoundAliasLoadspecKeep')
support = r'''
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
static int printed;
static void Com_Printf(const char *fmt, ...) { (void)fmt; printed++; }
static char *strlwr(char *text) { for (char *c = text; *c; c++) *c = (char)tolower((unsigned char)*c); return text; }
'''
checks = r'''
static int menu(const char *token) { return Com_SoundAliasLoadspecKeep("menu", "all_mp", token, "test.csv"); }
static int map(const char *token) { return Com_SoundAliasLoadspecKeep("mp_toujane", "all_mp", token, "test.csv"); }
int main(void) {
    /* The menu keeps its own aliases, as retail: of the negated lists only
     * those naming it. */
    assert(menu("menu") && menu("MENU") && !menu("all_mp") && !menu("mp_toujane") && !menu("mainmenu"));
    assert(!menu("!mp_toujane") && menu("!menu"));
    /* A map keeps its own, all_mp, and now the menus' own, alone or listed. */
    assert(map("mp_toujane") && map("mp_carentan mp_toujane") && map("all_mp") && !map("mp_carentan"));
    assert(map("menu") && map("mp_carentan menu") && map("menu mp_carentan"));
    /* Only the word: no other loadspec that holds it. */
    assert(!map("mainmenu") && !map("menus") && !map("menu_mp"));
    /* Negations keep their meaning: everywhere but the maps they name. */
    assert(!map("!mp_toujane") && map("!mp_carentan") && map("!menu") && map("!mp_carentan !menu"));
    assert(!printed);
    puts("ok");
    return 0;
}
'''
MUTANTS = [
    ('    if (strcmp(loadspec, "menu") && loadlist[0] != \'!\' && Com_SoundAliasLoadspecNames(loadlist, "menu"))\n'
     '        return keep;\n', ''),
    ("loadlist[0] != '!' && Com_SoundAliasLoadspecNames", 'Com_SoundAliasLoadspecNames'),
    ("if ((match == list || match[-1] <= '!') && match[len] <= ' ')", 'if (1)'),
]
with tempfile.TemporaryDirectory(prefix='cod2-menu-sound-aliases-') as directory:
    path = Path(directory)
    for index, variant in enumerate([body] + [body.replace(old, new) for old, new in MUTANTS]):
        if index:
            assert body.count(MUTANTS[index - 1][0]) == 1, MUTANTS[index - 1][0]
        (path / 'test.c').write_text(support + variant + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stdout, result.stderr[-1500:])
print(f'PASS: the menu keeps its own aliases; a map keeps its own, all_mp and the menus\' own, the word alone or '
      f'listed, not a longer loadspec holding it, negations unchanged; {len(MUTANTS)} mutants fail')
