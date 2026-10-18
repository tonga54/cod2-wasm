#ifndef UI_CLASS_MENU_H
#define UI_CLASS_MENU_H

/* Create a Class and the class choice a match opens: native screens over the
 * create_a_class and browser_class menus, which only open and close them.
 * The host keeps each gametag's classes. */
void UI_ClassMenu_Open(displayContextDef_t *dc, int choose);
/* Whether the menu is a class menu, which keeps every key from the binds as
 * an edit field does: F1, a name's letters. */
int UI_ClassMenu_WantsKeys(const menuDef_t *menu);
/* 1 when a class menu has the focus: it takes every key. */
int UI_ClassMenu_KeyEvent(displayContextDef_t *dc, menuDef_t *menu, int key, int down);
void UI_ClassMenu_Paint(displayContextDef_t *dc, menuDef_t *menu);
/* The host's replies, by HTTP status; 0 when it was unreachable. */
void UI_ClassMenu_Loaded(int generation, int status, const char *lines);
void UI_ClassMenu_Saved(int generation, int status);

#endif
