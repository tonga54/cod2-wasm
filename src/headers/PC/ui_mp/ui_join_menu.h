#ifndef UI_JOIN_MENU_H
#define UI_JOIN_MENU_H

/* Join Game in the WWII menus' style over the retail joinserver menu, whose
 * items are neither painted nor hovered: the servers by name, map and
 * players, refreshed on their own. */

/* Whether menu is joinserver while it is up. */
int UI_JoinMenu_Covers(displayContextDef_t *dc, const menuDef_t *menu);
/* Whether menu is joinserver, which takes every key while focused. */
int UI_JoinMenu_WantsKeys(const menuDef_t *menu);
/* Menus_Open, after the menu's onOpen ran. */
void UI_JoinMenu_Opened(displayContextDef_t *dc, menuDef_t *menu);
/* 1 when joinserver took the key. */
int UI_JoinMenu_KeyEvent(displayContextDef_t *dc, menuDef_t *menu, int key, int down);
void UI_JoinMenu_Paint(displayContextDef_t *dc, menuDef_t *menu);

#endif
