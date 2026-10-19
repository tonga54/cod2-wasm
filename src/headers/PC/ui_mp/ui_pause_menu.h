#ifndef UI_PAUSE_MENU_H
#define UI_PAUSE_MENU_H

/* The menu Escape opens in a match, laid out as CoD WWII's: tabs for the
 * class, the match overview, the team, leaving and the settings over the
 * blurred view. A native screen over the "ingame" script menu, which only
 * opens and closes it. */
void UI_PauseMenu_Open(displayContextDef_t *dc);
/* Opens it again on the tab it showed, after the class editor or a
 * settings page it opened. */
void UI_PauseMenu_Return(displayContextDef_t *dc);
/* Whether the menu is the pause menu, which keeps every key from the binds. */
int UI_PauseMenu_WantsKeys(const menuDef_t *menu);
/* 1 when the pause menu has the focus: it takes every key. */
int UI_PauseMenu_KeyEvent(displayContextDef_t *dc, menuDef_t *menu, int key, int down);
void UI_PauseMenu_Paint(displayContextDef_t *dc, menuDef_t *menu);
/* The menu to open for name: a settings page the pause menu opened goes back
 * to it, where the page would open the main menu. */
const char *UI_PauseMenu_Redirect(const char *name);

#endif
