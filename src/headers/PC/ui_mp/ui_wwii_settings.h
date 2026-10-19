#ifndef UI_WWII_SETTINGS_H
#define UI_WWII_SETTINGS_H

/* Options and its pages in the layout of WWII's settings captures, while
 * ui_wwiiMainMenu is on: options_menu's buttons as a list of categories,
 * each page as rows of a name and its value, slider or keys, with the focused
 * setting explained at the right. The retail items keep their dvars, binds
 * and scripts; they are painted and driven here instead. */
void UI_WWIISettings_Reset(const displayContextDef_t *dc);
void UI_WWIISettings_AddMenu(const displayContextDef_t *dc, menuDef_t *menu);
/* Before the menus are painted: forgets the page shown when Options closes. */
void UI_WWIISettings_Update(const displayContextDef_t *dc);
/* Whether the menu is laid out here: its items are neither painted nor
 * hovered by the menus. */
int UI_WWIISettings_Owns(const menuDef_t *menu);
/* Whether a laid-out menu is open, so bg shows only its backdrop. */
int UI_WWIISettings_Covers(const displayContextDef_t *dc);
/* 1 when the focused menu is laid out here and the key has been handled. */
int UI_WWIISettings_KeyEvent(displayContextDef_t *dc, menuDef_t *menu, int key, int down);
/* After a menu's items: the list or the page, when it is the top one. */
void UI_WWIISettings_Paint(displayContextDef_t *dc, menuDef_t *menu);

#endif
