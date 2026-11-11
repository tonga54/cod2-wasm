#!/usr/bin/env python3
"""Run the browser keypad translation on SDL's key codes: arrows and editing
keys while the keypad moves, their digits while it types them, and every other
key unchanged. The browser listener decides which from the key a keypad event
reports, so macOS, where NumLock always reads off, still types digits."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/unix/linux_input.c').read_text()

def function(signature):
    start = source.index(signature + '\n{')
    end = source.index('\n}\n', start) + 3
    return source[start:end]

keypad = function('static int IN_KeypadKey(int navigates, int keypad, int digit)')
translate = function('static int sdl_to_keynum(SDL_Keycode sym)')
defines = '\n'.join(re.findall(r'^#define K_[A-Z0-9_]+[ \t]+0x[0-9a-fA-F]+$', source, re.M))
symbols = sorted(set(re.findall(r'\bSDLK_[A-Z0-9_]+', keypad + translate)))
assert 'IN_KeypadTypesDigits()' in keypad
assert 'SDLK_KP_4' in translate and 'SDLK_KP_7' in translate

support = r'''
#include <assert.h>
#include <stdio.h>
typedef int SDL_Keycode;
''' + 'enum {\n' + ',\n'.join(f'    {s} = 0x{0x1000 + i:x}' for i, s in enumerate(symbols)) + '\n};\n' + r'''
static int numlock;
static int IN_KeypadTypesDigits(void) { return numlock; }
'''

checks = r'''
static void expect(int lock, int key, int want)
{
    numlock = lock;
    assert(sdl_to_keynum(key) == want);
}
int main(void)
{
    /* NumLock off: the keypad's arrows, Home/End, Page Up/Down, Insert and
     * Delete are the keys beside them. */
    expect(0, SDLK_KP_8, K_UPARROW);
    expect(0, SDLK_KP_2, K_DOWNARROW);
    expect(0, SDLK_KP_4, K_LEFTARROW);
    expect(0, SDLK_KP_6, K_RIGHTARROW);
    expect(0, SDLK_KP_7, K_HOME);
    expect(0, SDLK_KP_1, K_END);
    expect(0, SDLK_KP_9, K_PGUP);
    expect(0, SDLK_KP_3, K_PGDN);
    expect(0, SDLK_KP_0, K_INS);
    expect(0, SDLK_KP_PERIOD, K_DEL);
    expect(0, SDLK_KP_5, K_KP_5);
    /* NumLock on: the same keys type their digits, so a field can take a
     * number without the caret moving. */
    expect(1, SDLK_KP_8, '8');
    expect(1, SDLK_KP_2, '2');
    expect(1, SDLK_KP_4, '4');
    expect(1, SDLK_KP_6, '6');
    expect(1, SDLK_KP_7, '7');
    expect(1, SDLK_KP_1, '1');
    expect(1, SDLK_KP_9, '9');
    expect(1, SDLK_KP_3, '3');
    expect(1, SDLK_KP_0, '0');
    expect(1, SDLK_KP_PERIOD, '.');
    expect(1, SDLK_KP_5, '5');
    /* Enter, the operators and the ordinary keys keep their codes either way. */
    expect(0, SDLK_KP_ENTER, K_KP_ENTER);
    expect(1, SDLK_KP_ENTER, K_KP_ENTER);
    expect(0, SDLK_KP_PLUS, K_KP_PLUS);
    expect(1, SDLK_KP_PLUS, K_KP_PLUS);
    expect(0, SDLK_UP, K_UPARROW);
    expect(1, SDLK_UP, K_UPARROW);
    expect(0, SDLK_TAB, 0x09);
    expect(0, SDLK_RETURN, 0x0d);
    assert(sdl_to_keynum('a') == 'a');
    assert(sdl_to_keynum(0x1f) == 0);
    puts("PASS: keypad arrows and editing keys with NumLock off, their digits with it on");
    return 0;
}
'''

def mutate(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


mutants = [
    ('    return IN_KeypadTypesDigits() ? digit : navigates;', '    return navigates;'),
    ('    return IN_KeypadTypesDigits() ? digit : navigates;', '    return digit;'),
    ("    case SDLK_KP_4:         return IN_KeypadKey(K_LEFTARROW, K_KP_LEFTARROW, '4');",
     '    case SDLK_KP_4:         return K_KP_LEFTARROW;'),
    ("    case SDLK_KP_PERIOD:    return IN_KeypadKey(K_DEL, K_KP_DEL, '.');",
     '    case SDLK_KP_PERIOD:    return K_DEL;'),
]

variants = [(keypad + '\n' + translate, None)]
for old, new in mutants:
    body = keypad + '\n' + translate
    variants.append((mutate(body, old, new), (old, new)))

with tempfile.TemporaryDirectory(prefix='cod2-keypad-') as directory:
    path = Path(directory) / 'keypad.c'
    binary = Path(directory) / 'keypad'
    for body, mutant in variants:
        path.write_text(support + defines + '\n' + body + checks)
        subprocess.run(['cc', '-std=c99', '-O1', '-g', '-D__EMSCRIPTEN__',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if mutant is None else '-w', str(path), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (mutant is None), (mutant, result.stderr[-1500:])

listener_start = source.index('EM_JS(int, IN_BrowserKeypadDigits, (void), {')
listener = source[source.index('{', listener_start) + 1:source.index('\n});\n', listener_start)]

listener_checks = r'''
const vm = require('node:vm');
const assert = require('node:assert/strict');
const listeners = [];
const context = { addEventListener: (type, fn, capture) => listeners.push({ type, fn, capture }) };
const ask = new vm.Script('(function () {' + process.argv[1] + '})()');
const digits = () => ask.runInContext(context);
const press = (code, key, numLock) => {
    const event = { code, key, getModifierState: (name) => name === 'NumLock' && numLock };
    for (const l of listeners)
        if (l.type === 'keydown')
            l.fn(event);
    return digits();
};
vm.createContext(context);
assert.equal(digits(), 0);
assert.equal(listeners.length, 1);
assert.equal(listeners[0].capture, true);
digits();
assert.equal(listeners.length, 1);
/* Windows and Linux: NumLock on types digits, off moves. */
assert.equal(press('Numpad8', '8', true), 1);
assert.equal(press('Numpad8', 'ArrowUp', false), 0);
assert.equal(press('NumpadDecimal', '.', true), 1);
assert.equal(press('NumpadDecimal', 'Delete', false), 0);
/* macOS reports NumLock off while the keypad types digits. */
assert.equal(press('Numpad4', '4', false), 1);
/* A locale whose keypad separator is a comma. */
press('Numpad8', 'ArrowUp', false);
assert.equal(press('NumpadDecimal', ',', false), 1);
/* Keys with one meaning leave a held keypad key as it went down. */
assert.equal(press('NumpadAdd', '+', false), 1);
assert.equal(press('NumpadEnter', 'Enter', false), 1);
assert.equal(press('KeyA', 'a', false), 1);
press('Numpad2', 'ArrowDown', false);
assert.equal(press('Digit5', '5', false), 0);
'''

listener_mutants = [
    ('/^[0-9.,]$/.test(event.key)', "event.getModifierState('NumLock')"),
    ('/^Numpad([0-9]|Decimal)$/.test(event.code)', 'true'),
    ('[0-9.,]', '[0-9.]'),
]


def run_listener(body):
    return subprocess.run(['node', '-e', listener_checks, body], capture_output=True, text=True)


result = run_listener(listener)
assert result.returncode == 0, result.stderr[-1500:]
for old, new in listener_mutants:
    assert run_listener(mutate(listener, old, new)).returncode != 0, (old, new)

print(f'PASS: {len(symbols)} SDL key codes; digits/moves split and {len(mutants)} failing regression mutants; '
      f'the browser listener reads the key, macOS included, and {len(listener_mutants)} failing listener mutants')
