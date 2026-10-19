#ifndef UI_WWII_MENU_H
#define UI_WWII_MENU_H

/* The main menu in the WWII captures' layout: the retail main_text buttons in
 * their column and lettering over bg's background in gray, with our own logo,
 * version and key prompts, and a gold cursor; the menu files stay as they
 * are. ui_wwiiMainMenu switches it while the menus are up. */
void UI_WWIIMenu_RegisterDvars(void);
/* Whether ui_wwiiMainMenu is on. */
int UI_WWIIMenu_LayoutOn(void);
/* Forgets the menus before dc loads a new set, when dc is the one main_text
 * and bg came from: cgame's HUD menus set up a context of their own. */
void UI_WWIIMenu_Reset(const displayContextDef_t *dc);
void UI_WWIIMenu_AddMenu(const displayContextDef_t *dc, menuDef_t *menu);
/* Moves main_text's items to the layout, or back, when ui_wwiiMainMenu
 * changes; before the menus are painted. */
void UI_WWIIMenu_Update(displayContextDef_t *dc);
/* 1 when the item has been drawn here or the layout hides it. */
int UI_WWIIMenu_PaintItem(displayContextDef_t *dc, itemDef_t *item);
/* 1 when the item's text has been drawn here. */
int UI_WWIIMenu_PaintText(displayContextDef_t *dc, itemDef_t *item, const char *text);
/* After a menu's items: the layout's logo, version and key prompts. */
void UI_WWIIMenu_PaintMenu(displayContextDef_t *dc, menuDef_t *menu);
/* 1 when the gold cursor has been drawn at the UI's cursor, in the 640 by
 * 480 units the whole screen stretches over. */
int UI_WWIIMenu_DrawCursor(int x, int y);
/* Laid out, main_text's buttons sit by the screen's left edge, outside the
 * 4:3 rectangle the menu covers on a wide screen, so it takes clicks over the
 * whole screen. */
int UI_WWIIMenu_CoversScreen(const menuDef_t *menu);
/* The item after (direction 1) or before (-1) cursor on the arrow keys, from
 * the first or the last for a cursor off the items: the menu's order, but
 * laid out, main_text's column goes by its rows. */
int UI_WWIIMenu_NextItem(const menuDef_t *menu, int cursor, int direction);

/* The layout's lettering, key prompts and colors, for other WWII screens. */
enum { WWII_COLOR_IDLE, WWII_COLOR_GOLD, WWII_COLOR_TITLE, WWII_COLOR_LABEL, WWII_COLOR_WHITE };
const float *UI_WWIIMenu_Color(int color);
/* Barlow Regular or Bold capitals cap units tall, tracking capitals apart. */
void UI_WWIIMenu_Text(const char *text, int bold, float x, float baseline, int horzAlign, int vertAlign, float cap,
                      float tracking, const vec_t *color);
float UI_WWIIMenu_Width(const char *text, int bold, float cap, float tracking);
/* A row of prompts along the bottom: from the left edge, then one at the
 * right edge when rightKey is set. */
void UI_WWIIMenu_Prompts(const char *const *keys, const char *const *labels, int count, const char *rightKey,
                         const char *rightLabel);

#endif
