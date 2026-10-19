#ifndef UI_MATCH_MENU_H
#define UI_MATCH_MENU_H

/* Start New Server in the layout of WWII's MATCH screen: a native screen
 * over the retail createserver menu and the menus its onOpen opens, whose
 * items stay hidden and take no input. Mode, map and bots each open a page
 * of their own, and Start Match runs the menu's StartServer script. */
/* Shows the screen from its first page when createserver opens, after its
 * onOpen has loaded the arenas. */
void UI_MatchMenu_Opened(displayContextDef_t *dc, menuDef_t *menu);
/* 1 when the menu is createserver or one of the menus it opens, while the
 * screen is up. */
int UI_MatchMenu_Covers(displayContextDef_t *dc, const menuDef_t *menu);
/* 1 when the screen has taken the key: every key, while a menu it covers
 * has the focus. */
int UI_MatchMenu_KeyEvent(displayContextDef_t *dc, menuDef_t *menu, int key, int down);
/* Paints the screen in createserver's place. */
void UI_MatchMenu_Paint(displayContextDef_t *dc, menuDef_t *menu);
/* The room's name: the gametag without its colors, in the letters the room
 * supervisor keeps. */
void UI_MatchMenu_RoomName(char *name, int size);

#endif
