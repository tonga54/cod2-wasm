#!/usr/bin/env python3
"""Run the production main menu adaptation on a retail-shaped ui_mp/main.menu:
the browser's column keeps no Quit and offers no fullscreen, Options moves up
under Create a Class, and the other menus keep their items."""
import re
from browser_settings_menu import apply_settings_menu, items


def button(name, text, top, action, dvar=None, value=None):
    """A main_text button as the retail file writes them."""
    test = f'\t\t\tdvarTest "{dvar}"\n\t\t\tshowDvar {{ "{value}" }}\n' if dvar else ''
    return f'''
		itemDef
		{{
			name		{name}
			text		"{text}"
			type		ITEM_TYPE_BUTTON
			style		WINDOW_STYLE_EMPTY
			rect		MAIN_RECT_X {top} MAIN_RECT_WIDTH MAIN_RECT_HEIGHT MAIN_RECT_HORZALIGN MAIN_RECT_VERTALIGN
			visible		1
			mouseEnter
			{{
				play "mouse_over";
			}}
			action
			{{
				play "mouse_click";
				{action}
			}}
{test}		}}'''


retail = '''#include "ui/menudef.h"

#define MAIN_RECT_X			385
#define MAIN_RECT_WIDTH		210
#define MAIN_RECT_HEIGHT	20

{
	menuDef
	{
		name			main
		fullScreen		1
		onOpen
		{
			open bg;
			open main_text;
			uiScript addPlayerProfiles;
			uiScript openMenuOnDvar com_playerProfile "" player_profile;
			uiScript openMenuOnDvarNot ui_playerProfileCount 1 player_profile;
		}
	}

	menuDef
	{
		name "main_text"
		fullScreen 0
		onESC
		{
//			open quit_popmenu
			close mods_menu;
		}
/*
		itemDef { name resume text "@MENU_JOIN_AN_INTERNET_GAME" rect 385 160 210 15 }
*/''' + ''.join((
    button('resume', '@MENU_BACKTOGAME', 190, 'ingameclose main;', 'cl_ingame', 1),
    button('play', '@MENU_JOIN_GAME', 190, 'close main_text; open joinserver', 'cl_ingame', 0),
    button('play', '@MENU_DISCONNECT', 220, 'exec "disconnect";', 'cl_ingame', 1),
    button('play', '@MENU_START_NEW_SERVER', 220, 'close main_text; open createserver;', 'cl_ingame', 0),
    button('player_select', '@MENU_SELECT_PROFILE', 250, 'open player_profile;', 'cl_ingame', 0),
    button('player_select', '@MENU_SELECT_PROFILE', 250, 'open player_profile_ingame_warning;', 'cl_ingame', 1),
    button('options', '@MENU_MULTIPLAYER_OPTIONS', 280, 'open options_multi;'),
    button('options', '@MENU_OPTIONS', 310, 'close main_text; open options_menu;'),
    button('playselection', '@MENU_SINGLE_PLAYER', 340, 'open single_popmenu;'),
    button('quit', '@MENU_QUIT', 370, 'close mods_menu; open quit_popmenu;'),
    button('play', '@MENU_AUTO_UPDATE', 430, 'open auconfirm', 'cl_updateavailable', 1),
)) + '''
		itemDef
		{
			name 		esrb
			text 		"@MENU_GAME_EXPERIENCE_MAY_CHANGE"
			rect 		20 390 MAIN_RECT_WIDTH MAIN_RECT_HEIGHT
			decoration
		}
	}

	menuDef
	{
		name "player_profile_ingame_warning"
		popup
		itemDef
		{
			name confirm
			text "@MENU_QUIT_CURRENT_GAME"
			decoration
		}
		itemDef
		{
			name yes
			text "@MENU_YES"
			action { exec "disconnect"; }
		}
	}
}
'''

menu = apply_settings_menu('ui_mp/main.menu', retail.replace('\n', '\r\n').encode()).decode()
start = menu.index('name "main_text"')
main_text, others = [], []
items(menu[start:menu.index('menuDef', start)], lambda item: main_text.append(item) or item)
items(menu[menu.index('menuDef', start):], lambda item: others.append(item) or item)


def text(item):
    return re.search(r'\btext\s+"([^"]*)"', item)[1]


column = sorted((int(re.search(r'\brect\s+MAIN_RECT_X\s+(\d+)', item)[1]), text(item))
                for item in main_text if 'MAIN_RECT_X' in item)
# UI_ConfigureBrowserMenu drops Single Player when the engine loads the menu.
assert column == [(190, '@MENU_BACKTOGAME'), (190, '@MENU_JOIN_GAME'), (220, '@MENU_DISCONNECT'),
                  (220, '@MENU_START_NEW_SERVER'), (250, 'Create a Class'), (280, '@MENU_OPTIONS'),
                  (340, '@MENU_SINGLE_PLAYER'), (380, 'New client version available'),
                  (402, 'Update game'), (430, '@MENU_AUTO_UPDATE')], column
options = next(item for item in main_text if text(item) == '@MENU_OPTIONS')
assert 'open options_menu;' in options
print('PASS: the browser column is Join/Back to Game, Start New Server/Disconnect, Create a Class and Options, '
      'with the update notice under them; Single Player is left to the engine filter')

# Nothing opens the quit popup, and no item offers fullscreen. The profile
# warning's own Quit text is a different string and stays.
assert '"@MENU_QUIT"' not in menu and 'quit_popmenu' not in menu
assert not any('fullscreen' in item.lower() for item in main_text + others)
assert sorted(text(item) for item in others) == ['@MENU_QUIT_CURRENT_GAME', '@MENU_YES']
assert menu.count('{') == menu.count('}')
print('PASS: the main menu has no Quit or fullscreen item; @MENU_QUIT_CURRENT_GAME and the other menus survive')
