#ifndef R_FONT_WWII_H
#define R_FONT_WWII_H

/* Every stock CoD2 font redrawn in the WWII captures' lettering, Barlow. Each
 * keeps the stock font's units: its pixel height, cap height and baseline, so
 * text keeps its size and place. Returns NULL, and the caller loads the stock
 * font, for names that are not a stock font, with r_wwiiFonts off, or when
 * the font cannot be built. */
FontHandle R_LoadWwiiFont(const char *fontName);
void R_ShutdownWwiiFonts(void);

#endif
