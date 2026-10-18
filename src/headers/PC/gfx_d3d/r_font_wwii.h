#ifndef R_FONT_WWII_H
#define R_FONT_WWII_H

/* Every stock CoD2 font redrawn in the WWII captures' lettering, Barlow. Each
 * keeps the stock font's units: its pixel height, cap height and baseline, so
 * text keeps its size and place. Returns NULL, and the caller loads the stock
 * font, for names that are not a stock font, with r_wwiiFonts off, or when
 * the font cannot be built. */
FontHandle R_LoadWwiiFont(const char *fontName);
void R_ShutdownWwiiFonts(void);
/* Registers a font only the WWII lettering has, such as "$regularfont":
 * NULL, where R_RegisterFont would stop on an error, with r_wwiiFonts off or
 * no font free. */
FontHandle R_RegisterWwiiFont(const char *fontName);
/* A font lettered in Barlow elsewhere, such as the cgame's name font, drawn
 * like the WWII fonts until they shut down. */
void R_AddWwiiLettering(FontHandle font);
int R_IsWwiiFont(FontHandle font);
/* The captures' warm, translucent white for text the game draws white in a
 * WWII font: 1 with toned filled, else 0. A NULL color is white. */
int R_ToneWwiiText(FontHandle font, const vec_t *color, vec_t *toned);

#endif
