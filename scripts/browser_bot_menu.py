"""Adapt two unused browser host settings to first-party bot controls."""
import re


def apply_bot_menu(data):
    # Browser rooms already run dedicated and use a fixed asset set.
    # Reuse these two retail rows without moving name/map controls.
    assert data.count(b'"@MENU_DEDICATED"') == 1
    assert data.count(b'"@MENU_PURE"') == 1
    data = data.replace(b'"@MENU_DEDICATED"', b'"Bots"')
    data = data.replace(b'"@MENU_PURE"', b'"Bot Difficulty"')
    data = data.replace(b'"ui_dedicated"', b'"ui_botCount"')
    choices = ' '.join('"' + (str(n) if n else 'None') + '" ' + str(n) for n in range(17))
    data, count = re.subn(rb'dvarFloatList\s*\{ "@MENU_NO" 0 "@MENU_LAN" 1 "@MENU_INTERNET" 2 \}',
                          ('dvarFloatList { ' + choices + ' }').encode(), data)
    assert count == 1
    data, count = re.subn(rb'type\s+ITEM_TYPE_YESNO(\s+text\s+" "\s+dvar\s+)"sv_pure"',
                          rb'type ITEM_TYPE_MULTI\1"ui_botDifficulty"\n'
                          rb'            dvarFloatList { "Easy" 0 "Normal" 1 "Hard" 2 }', data)
    assert count == 1
    data = data.replace(b'show message_dedicated', b'show message_bots')
    data = data.replace(b'hide message_dedicated', b'hide message_bots')
    data = data.replace(b'show message_pure_server', b'show message_botdifficulty')
    data = data.replace(b'hide message_pure_server', b'hide message_botdifficulty')
    return data
