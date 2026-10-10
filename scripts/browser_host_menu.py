import re


def apply_host_menu(data):
    data, count = re.subn(rb'(text\s+)"@MENU_VOICE_CHAT"(\s+textfont\s+UI_FONT_NORMAL\s+rect\s+)0 120 1 13',
                          rb'\1"Game Host"\g<2>0 75 1 13', data)
    assert count == 1
    data, count = re.subn(rb'type(\s+)ITEM_TYPE_YESNO(\s+text\s+" "\s+textfont\s+UI_FONT_NORMAL\s+dvar\s+)"sv_voice"'
                          rb'(\s+rect\s+)0 120 OPTIONS_BIND_SIZE\s+maxChars\s+12\s+maxPaintChars\s+12',
                          rb'type\1ITEM_TYPE_MULTI\2"ui_gameHost"\r\n\t\t\tdvarFloatList { "Server" 0 "Local" 1 }'
                          rb'\g<3>0 75 OPTIONS_BIND_SIZE', data)
    assert count == 1
    return data
