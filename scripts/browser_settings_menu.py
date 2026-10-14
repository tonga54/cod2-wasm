"""Adapt private retail menus to the persistent single browser profile."""
import re


def items(source, transform):
    # Strip comments before balancing braces; keep quoted strings untouched.
    source = re.sub(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*.*?\*/',
                    lambda m: m[0] if m[0].startswith('"') else '', source, flags=re.S)
    spans = []
    for match in re.finditer(r'\bitemDef\s*\{', source):
        depth, end = 1, match.end()
        tokens = re.finditer(r'"(?:\\.|[^"\\])*"|[{}]', source[end:])
        for token in tokens:
            depth += (token[0] == '{') - (token[0] == '}')
            if depth == 0:
                end += token.end()
                break
        else:
            raise ValueError('Unbalanced menu item')
        spans.append((match.start(), end))
    for start, end in reversed(spans):
        source = source[:start] + transform(source[start:end]) + source[end:]
    return source


def apply_settings_menu(name, data):
    if name not in {'ui_mp/main.menu', 'ui_mp/options.menu', 'ui/options_game.menu',
                    'ui_mp/options_multi.menu', 'ui_mp/background.menu',
                    'ui_mp/menu_background.menu'}:
        return data
    text = data.decode().replace('\r\n', '\n')
    if name == 'ui_mp/main.menu':
        remove = ('@MENU_SELECT_PROFILE', '@MENU_MULTIPLAYER_OPTIONS',
                  '@MENU_GAME_EXPERIENCE_MAY_CHANGE')
        text = items(text, lambda item: '' if any(key in item for key in remove) else item)
        text = re.sub(r'\s*uiScript (?:addPlayerProfiles|openMenuOnDvar[^;]*player_profile|openMenuOnDvarNot[^;]*player_profile);', '', text)
        text = text.replace('MAIN_RECT_X 310', 'MAIN_RECT_X 250').replace('MAIN_RECT_X 370', 'MAIN_RECT_X 280')
        marker = 'name "main_text"'
        if text.count(marker) != 1:
            raise ValueError('Unexpected main menu')
        insertion = text.index('itemDef', text.index(marker))
        text = text[:insertion] + '''
        itemDef {
            name browser_client_update_notice
            type ITEM_TYPE_TEXT
            text "New client version available"
            rect MAIN_RECT_X 330 230 18
            textfont UI_FONT_NORMAL
            textstyle ITEM_TEXTSTYLE_SHADOWED
            textscale .24
            textaligny 18
            forecolor .85 .75 .55 1
            visible 1
            decoration
            dvarTest "ui_clientUpdateAvailable"
            showDvar { "1" }
        }
        itemDef {
            name browser_client_update_reload
            type ITEM_TYPE_BUTTON
            style WINDOW_STYLE_EMPTY
            text "Update game"
            rect MAIN_RECT_X 352 MAIN_RECT_WIDTH MAIN_RECT_HEIGHT
            textfont UI_FONT_NORMAL
            textstyle ITEM_TEXTSTYLE_SHADOWED
            textscale MAIN_TEXTSCALE
            textaligny MAIN_TEXTALIGN_Y
            forecolor MAIN_FORECOLOR
            visible 1
            dvarTest "ui_clientUpdateAvailable"
            showDvar { "1" }
            mouseEnter { play "mouse_over"; }
            action { play "mouse_click"; uiScript ReloadClient; }
        }
''' + text[insertion:]
    elif name == 'ui_mp/options.menu':
        text = items(text, lambda item: '' if '@MENU_VOICECHAT' in item else item)
    elif name == 'ui_mp/options_multi.menu':
        text = items(text, lambda item: '' if '@MENU_VOICE_CHAT_BUTTON' in item or '"+talk"' in item else item)
    elif name == 'ui/options_game.menu':
        text = items(text, lambda item: item)
        # Native edit fields write the archived name dvar directly; the engine
        # updates userinfo in connected matches and saves the same profile.
        marker = 'uiScript loadControls'
        if marker not in text:
            raise ValueError('Unexpected game options menu')
        insert = '''
    itemDef {
        name browser_gametag_label
        type ITEM_TYPE_TEXT
        text "Gametag"
        rect 0 90 1 13
        origin OPTIONS_ITEM_ORIGIN
        textstyle OPTIONS_ITEM_TEXT_STYLE
        textalign OPTIONS_ITEM_ALIGN
        textalignx OPTIONS_ITEM_ALIGN_X
        textaligny OPTIONS_ITEM_ALIGN_Y
        textscale OPTIONS_CONTROL_TXTSCALE
        forecolor OPTIONS_CONTROL_FORECOLOR
        visible 1
        decoration
    }
    itemDef {
        name browser_gametag
        type ITEM_TYPE_EDITFIELD
        dvar "name"
        text " "
        maxChars 31
        maxPaintChars 20
        rect 0 90 OPTIONS_BIND_SIZE
        origin OPTIONS_ITEM_ORIGIN
        textstyle OPTIONS_ITEM_TEXT_STYLE
        textalign OPTIONS_ITEM_ALIGN
        textalignx OPTIONS_BIND_ALIGN_X
        textaligny OPTIONS_ITEM_ALIGN_Y
        textscale OPTIONS_CONTROL_TXTSCALE
        backcolor OPTIONS_CONTROL_BACKCOLOR
        forecolor OPTIONS_CONTROL_FORECOLOR
        visible 1
    }
'''
        # Insert before the first item, inside options_game.
        index = text.index('itemDef')
        text = text[:index] + insert + text[index:]
    else:
        text = text.replace('dvar "shortversion"', 'dvar "ui_browserVersion"')
    return text.encode()
