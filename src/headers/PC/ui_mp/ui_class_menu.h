#ifndef UI_CLASS_MENU_H
#define UI_CLASS_MENU_H

#include "headers/PC/ui_mp/ui_class_data.h"

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

/* For the pause menu's Class tab: the player's five classes, loaded from the
 * host on first use, or the game's own five (the defaults). */
const ClassLoadout *UI_ClassMenu_Classes(int standard);
/* What the host's storage says under the list, or NULL. */
const char *UI_ClassMenu_Note(void);
const char *UI_ClassMenu_WeaponName(int weapon);
/* The weapon's picture as large as it fits in the box, centered. */
void UI_ClassMenu_DrawWeapon(int weapon, float x, float y, float w, float h, int horzAlign, const vec_t *color);
/* Sends a class as the class menu's response, as if chosen there, and keeps
 * it as the one chosen; 1 when sent. */
int UI_ClassMenu_Choose(int standard, int index);
/* The class chosen last and its list, -1 for none. */
int UI_ClassMenu_Chosen(int *standard);
/* Opens the editor on one of the player's classes; Back returns to the
 * pause menu. */
void UI_ClassMenu_EditFromPause(displayContextDef_t *dc, int index);

#endif
