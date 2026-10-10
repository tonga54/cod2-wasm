#!/usr/bin/env python3
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from browser_host_menu import apply_host_menu

label = (b'\t\titemDef \r\n\t\t{\r\n\t\t\tname\t\t\texpert\r\n\t\t\tgroup\t\t\tserversettings\r\n'
         b'\t\t\ttype\t\t\tITEM_TYPE_BUTTON\r\n\t\t\ttext\t\t\t"@MENU_VOICE_CHAT"\r\n'
         b'\t\t\ttextfont\t\tUI_FONT_NORMAL\r\n\t\t\trect\t\t0 120 1 13 \r\n\t\t}\r\n')
control = (b'\t\titemDef \r\n\t\t{\r\n\t\t\tname\t\t\texpert\r\n\t\t\tgroup\t\t\tserversettings\r\n'
           b'\t\t\ttype\t\t\tITEM_TYPE_YESNO\r\n\t\t\ttext\t\t\t" "\r\n\t\t\ttextfont\t\tUI_FONT_NORMAL\r\n'
           b'\t\t\tdvar\t\t\t"sv_voice"\r\n\t\t\trect\t\t\t0 120 OPTIONS_BIND_SIZE\r\n'
           b'\t\t\tmaxChars\t\t12\r\n\t\t\tmaxPaintChars\t\t12\r\n'
           b'\t\t\taction\t\t\t{ play "mouse_click"; }\r\n\t\t}\r\n')
password = b'\t\titemDef \r\n\t\t{\r\n\t\t\tdvar\t\t\t"g_password"\r\n\t\t\trect\t\t\t0 135 OPTIONS_BIND_SIZE\r\n\t\t}\r\n'
menu = label + control + password
patched = apply_host_menu(menu)
assert b'"@MENU_VOICE_CHAT"' not in patched and b'"sv_voice"' not in patched
assert patched.count(b'text\t\t\t"Game Host"\r\n\t\t\ttextfont\t\tUI_FONT_NORMAL\r\n\t\t\trect\t\t0 75 1 13 ') == 1
assert patched.count(b'type\t\t\tITEM_TYPE_MULTI\r\n') == 1
assert patched.count(b'dvar\t\t\t"ui_gameHost"\r\n\t\t\tdvarFloatList { "Server" 0 "Local" 1 }\r\n'
                     b'\t\t\trect\t\t\t0 75 OPTIONS_BIND_SIZE\r\n') == 1
assert b'maxChars' not in patched and b'maxPaintChars' not in patched
assert patched.endswith(password)
assert b'{ play "mouse_click"; }' in patched

for broken in (control + password, label + password, menu + label):
    try:
        apply_host_menu(broken)
    except AssertionError:
        continue
    raise AssertionError('unexpected retail layout was accepted')
print('PASS: the hidden voice row becomes a visible Game Host Server/Local control; unexpected layouts are rejected')
