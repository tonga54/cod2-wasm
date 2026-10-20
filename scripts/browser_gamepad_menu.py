"""Controller settings rendered by the original game's menu system."""
import re


def controller_menu():
    actions = (
        ('Fire', '+attack'), ('Aim (hold)', '+speed'),
        ('Jump / stand', '+gostand'), ('Crouch / hold prone', '+stance'),
        ('Go prone', 'goprone'), ('Use / reload', '+usereload'),
        ('Switch weapon', 'weapnext'), ('Melee', '+melee'),
        ('Sprint / hold breath', '+sprint'), ('Frag grenade', '+frag'),
        ('Smoke grenade', '+smoke'), ('Binoculars', '+breath_binoculars'),
        ('Lean left', '+leanleft'), ('Lean right', '+leanright'),
        ('Scoreboard', '+scores'), ('Pause menu', 'togglemenu'),
    )

    def row(name, label, y, kind, extra='', *, page=None, x=50, width=300,
            height=18, scale='.25', baseline=14):
        visibility = '' if page is None else (
            f'dvarTest "ui_gamepadPage" showDvar {{ "{page}" }}')
        return f'''
        itemDef {{
            name "{name}"
            text "{label}"
            type {kind}
            rect {x} {y} {width} {height}
            textfont UI_FONT_NORMAL
            textstyle ITEM_TEXTSTYLE_SHADOWED
            textscale {scale}
            textaligny {baseline}
            forecolor .9 .9 .9 1
            visible 1
            {visibility}
            {extra}
        }}
'''

    # The artwork's safe content area is y=64..416 in the original 640x480
    # coordinates. Keep each page inside it rather than shrinking all 16 rows.
    result = '''
    menuDef {
        name "browser_gamepad"
        rect 0 0 370 480
        focusColor GLOBAL_FOCUSED_COLOR
        onESC { close browser_gamepad; }
        onOpen {
            uiScript loadControls;
            setdvar ui_gamepadPage "0";
            setdvar ui_background_gradient_show "1";
        }
'''
    result += row('title', 'Controller', 64, 'ITEM_TYPE_TEXT', 'decoration',
                  height=26, scale='.40', baseline=22)
    result += row('connected', 'Controller connected', 96, 'ITEM_TYPE_TEXT',
                  'decoration dvarTest "ui_gamepadConnected" showDvar { "1" }',
                  scale='.22')
    result += row('disconnected', 'Connect a controller and press a button', 96, 'ITEM_TYPE_TEXT',
                  'decoration dvarTest "ui_gamepadConnected" showDvar { "0" }',
                  scale='.22')
    for page, label in enumerate(('Settings', 'Buttons 1', 'Buttons 2')):
        # Active tabs use the same gold as focused retail menu controls.
        for active, color in ((False, '.9 .9 .9 1'), (True, '.98 .827 .58 1')):
            condition = 'showDvar' if active else 'hideDvar'
            result += row(f'tab_{page}_{int(active)}', label, 120, 'ITEM_TYPE_BUTTON',
                          f'dvarTest "ui_gamepadPage" {condition} {{ "{page}" }} '
                          f'forecolor {color} '
                          f'action {{ setdvar ui_gamepadPage "{page}"; }}',
                          x=50 + page * 100, width=94, height=20, baseline=15)
    result += row('enabled', 'Enable controller', 158, 'ITEM_TYPE_YESNO',
                  'dvar "cl_gamepadEnable"', page=0)
    result += row('sensitivity', 'Look sensitivity', 188, 'ITEM_TYPE_SLIDER',
                  'dvarFloat "cl_gamepadLookSensitivity" 8 .1 50', page=0)
    result += row('deadzone', 'Stick deadzone', 218, 'ITEM_TYPE_SLIDER',
                  'dvarFloat "cl_gamepadDeadzone" .2 0 .9', page=0)
    result += row('invert', 'Invert vertical look', 248, 'ITEM_TYPE_YESNO',
                  'dvar "cl_gamepadInvertY"', page=0)
    result += row('rumble', 'Vibration', 278, 'ITEM_TYPE_YESNO',
                  'dvar "cl_gamepadRumble"', page=0)
    result += row('aim_assist', 'Aim assist', 308, 'ITEM_TYPE_YESNO',
                  'dvar "cl_gamepadAimAssist"', page=0)
    result += row('stick_help', 'Left stick: move / navigate. Right stick: look.', 326,
                  'ITEM_TYPE_TEXT', 'decoration', page=0, scale='.23')
    for i, (label, command) in enumerate(actions):
        result += row(f'pad_bind_{i}', label, 158 + (i % 8) * 22,
                      'ITEM_TYPE_BIND', f'dvar "{command}"', page=1 + i // 8)
    result += row('instructions', 'Select an action, then press a controller button.', 344,
                  'ITEM_TYPE_TEXT', 'decoration', scale='.22')
    result += row('clear_help', 'Esc cancels an assignment; Backspace clears it.', 362,
                  'ITEM_TYPE_TEXT', 'decoration', scale='.22')
    result += row('defaults', 'Restore controller defaults', 390, 'ITEM_TYPE_BUTTON',
                  'action { exec "gamepad_defaults"; }', width=235)
    result += row('back', 'Back', 390, 'ITEM_TYPE_BUTTON',
                  'action { close browser_gamepad; }', x=300, width=50)
    result += '\n    }\n'
    return result


def add_controller_menu(text, transform_items):
    # Reuse the voice-chat row's space and sibling close/open scripts so the
    # controller page participates in all normal menu navigation and cleanup.
    def transform(item):
        if '@MENU_VOICECHAT' in item:
            return item.replace('@MENU_VOICECHAT', 'Controller').replace(
                'open options_voice;', 'open browser_gamepad;')
        return item

    text = transform_items(text, transform)
    text = text.replace('close options_voice;', 'close options_voice; close browser_gamepad;')
    # Retail options.menu has a surplus closing brace at the end. Insert
    # before its first menu so the engine reaches this definition before EOF.
    start = re.search(r'\bmenuDef\b', text).start()
    return text[:start] + controller_menu() + text[start:]
