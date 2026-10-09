#include "common_types.h"
#include "imports.h"

extern int I_stricmp(const char *s1, const char *s2);
extern void R_Error(int code, const char *fmt, ...);
extern FontHandle R_LoadFont(const char *fontName, int imageTrack);
extern void R_AddCmdDrawTextWithCursor(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style, int cursorPos, int cursor);
extern void R_AddCmdDrawStretchPic(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const vec_t *color, MaterialHandle material);
extern unsigned char ColorIndex(int c);
extern const char *CL_GetHudMsgIconMaterialName(int index);
extern MaterialHandle Material_RegisterHandle(const char *name, int lightmapIndex, int imageTrack);
extern refimport_t ri;

static int registeredFontCount;
static Font *registeredFont[16];
static Glyph nullGlyph;

static FontHandle R_ResolveFont(FontHandle font)
{
    if (font) {
        return font;
    }
    if (registeredFontCount > 0) {
        return registeredFont[0];
    }
    return 0;
}

const Glyph *R_GetCharacterGlyph(FontHandle font, unsigned int letter);
FontHandle R_RegisterFont(const char *fontName, int imageTrack);
int R_DuplicateFont(FontHandle fontCopy, const char *name);
int R_InitFonts(void);
void R_ShutdownFonts(void);
float R_NormalizedTextScale(FontHandle font, float scale);
int R_TextHeight(FontHandle font);
void R_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style);
static const short int *__attribute_regparm__(3) R_GetConsoleString(const short int *string, int *limit, char *text, vec_t *color, Bool *foundIcon);
static const short int *__attribute_regparm__(3) R_GetConsoleIcon(const short int *string, int *maxChars, float *iconWidth, float *iconHeight, MaterialHandle *iconMaterial, vec_t *color, Bool *iconHorzFlip);
int R_TextWidth(const char *text, int maxChars, FontHandle font);
int R_ConsoleTextWidth(const short int *string, int maxChars, FontHandle font);
void R_DrawConsoleText(const short int *string, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style);

static inline __attribute__((always_inline))
const Glyph *
R_GetCharacterGlyph_inl(FontHandle font, unsigned int letter)
{
    Glyph *glyphs;
    int numGlyphs, top, bot, mid;

    if (letter - 0x20 <= 0x5F)
        return &font->glyphs[letter - 0x20];

    numGlyphs = font->glyphCount;
    top = numGlyphs - 1;
    bot = 0x60;

    if (top <= 0x5F) {
        glyphs = font->glyphs;
        return &glyphs[14];
    }

    glyphs = font->glyphs;
    mid = (numGlyphs + 0x5F) / 2;
    if (*(unsigned short *)&glyphs[mid] == letter)
        return (const Glyph *)&glyphs[mid];

    while (top >= bot) {
        mid = (top + bot) / 2;
        if (*(unsigned short *)&glyphs[mid] == letter)
            return (const Glyph *)&glyphs[mid];
        if (letter > *(unsigned short *)&glyphs[mid])
            bot = mid + 1;
        else
            top = mid - 1;
    }

    return &glyphs[14];
}

const Glyph *R_GetCharacterGlyph(FontHandle font, unsigned int letter)
{
    return R_GetCharacterGlyph_inl(font, letter);
}

FontHandle R_RegisterFont(const char *fontName, int imageTrack)
{
    int fontIndex;
    FontHandle font;

    for (fontIndex = 0; fontIndex < registeredFontCount; fontIndex++) {
        if (I_stricmp(fontName, *(const char **)registeredFont[fontIndex]) == 0) {
            return registeredFont[fontIndex];
        }
    }

    if (registeredFontCount > 15) {
        R_Error(1, (const char *)"R_RegisterFont: Too many fonts registered already.\n");
        return 0;
    }

    font = R_LoadFont(fontName, imageTrack);
    if (!font) {
        R_Error(1, (const char *)"R_RegisterFont: Error while reading font '%s'", fontName);
    }

    registeredFont[registeredFontCount] = font;
    registeredFontCount++;

    return font;
}

int R_DuplicateFont(FontHandle fontCopy, const char *name)
{
    int fontIndex;
    FontHandle existing;
    FontHandle newFont;
    char *nameCopy;
    const char *oldName;
    refimport_t *riPtr;

    for (fontIndex = 0; fontIndex < registeredFontCount; fontIndex++) {
        existing = registeredFont[fontIndex];
        if (I_stricmp(name, *(const char **)existing) == 0) {

            oldName = *(const char **)existing;
            memcpy(existing, fontCopy, sizeof(Font_s));
            *(const char **)existing = oldName;
            return 0;
        }
    }

    if (registeredFontCount > 15) {
        R_Error(1, (const char *)"R_DuplicateFont: Too many fonts registered already.\n");
        return 0;
    }

    riPtr = &ri;
    newFont = riPtr->Hunk_AllocInternal(0x14);
    memcpy(newFont, fontCopy, sizeof(Font_s));

    nameCopy = (char *)riPtr->Hunk_AllocInternal(strlen(name) + 1);
    strcpy(nameCopy, name);
    *(const char **)newFont = nameCopy;

    registeredFont[fontIndex] = newFont;
    registeredFontCount++;

    return 0;
}

int R_InitFonts(void)
{
}

void R_ShutdownFonts(void)
{
    registeredFontCount = 0;
}

float R_NormalizedTextScale(FontHandle font, float scale)
{
    return 48.0f * scale / (float)font->pixelHeight;
}

int R_TextHeight(FontHandle font)
{
    return font->pixelHeight;
}

void R_DrawText(const char *text, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style)
{
    R_AddCmdDrawTextWithCursor(text, maxChars, font, x, y, xScale, yScale, color, style, -1, 0);
}

static __attribute_regparm__(3)
    const short int *R_GetConsoleString(const short int *string, int *limit, char *text, vec_t *color, Bool *foundIcon)
{
    int currentColor;
    int markedEnd;
    const short int *stringEndPos;
    int i;
    int pos;
    unsigned short entry;
    unsigned char ch;
    unsigned char type;

    if (*limit > 0x3ff) {
        *limit = 0x3ff;
    }

    text[0] = '\0';
    currentColor = (int)ColorIndex(0x37);
    *foundIcon = 0;

    if (*limit <= 0) {
        markedEnd = -1;
        stringEndPos = 0;
        i = 0;
        pos = 0;
        goto finish;
    }

    stringEndPos = string;
    markedEnd = -1;
    i = 0;
    pos = 0;

    for (i = 0; i < *limit; i++) {
        stringEndPos = &string[i];
        entry = *(const unsigned short *)&string[i];
        ch = (unsigned char)entry;
        type = (unsigned char)(entry >> 8);

        if (type == currentColor) {

            text[pos] = ch;
            if (ch == 0x20) {
                if (markedEnd == -1) {
                    markedEnd = pos;
                }
                pos++;
                continue;
            }
            markedEnd = -1;
            pos++;
            continue;
        }

        if (type == 0x0d || type == 0x10 || type == 0x11 || type == 0x12 || type == 0x13) {
            *foundIcon = 1;
            markedEnd = -1;
            pos = pos;
            goto finish;
        }

        if (type == 0x0a) {
            if (color) {
                color[0] = (float)(unsigned int)ch * 0.003921568859368563f;
            }
            continue;
        }

        if (type == 0x0b) {
            if (color) {
                color[1] = (float)(unsigned int)ch * 0.003921568859368563f;
            }
            continue;
        }

        if (type == 0x0c) {
            if (color) {
                color[2] = (float)(unsigned int)ch * 0.003921568859368563f;
            }
            continue;
        }

        text[pos] = '^';
        text[pos + 1] = (char)(type + 0x30);
        pos += 2;
        currentColor = type;
        stringEndPos = &string[i];

        entry = *(const unsigned short *)&string[i];
        ch = (unsigned char)entry;
        text[pos] = ch;
        if (ch == 0x20) {
            if (markedEnd == -1) {
                markedEnd = pos;
            }
            pos++;
            continue;
        }
        markedEnd = -1;
        pos++;
    }

    stringEndPos = 0;

finish:
    text[pos] = '\0';
    if (markedEnd >= 0) {
        text[markedEnd] = '\0';
    }
    *limit -= i;
    return stringEndPos;
}

static __attribute_regparm__(3)
    const short int *R_GetConsoleIcon(const short int *string, int *maxChars, float *iconWidth, float *iconHeight, MaterialHandle *iconMaterial, vec_t *color, Bool *iconHorzFlip)
{
    int i;

    *iconWidth = 1.0f;
    *iconHeight = 1.0f;
    *iconMaterial = NULL;

    if (iconHorzFlip) {
        *iconHorzFlip = 0;
    }

    if (*maxChars <= 0) {
        return NULL;
    }

    for (i = 0; i < *maxChars; ++i) {
        unsigned short entry = (unsigned short)string[i];
        unsigned char value = (unsigned char)entry;

        switch (entry >> 8) {
        case 0x0d:
            if (color) {
                color[0] = (float)value * 0.003921568859368563f;
            }
            break;

        case 0x0e:
            if (color) {
                color[1] = (float)value * 0.003921568859368563f;
            }
            break;

        case 0x0f:
            if (color) {
                color[2] = (float)value * 0.003921568859368563f;
            }
            break;

        case 0x10:
            *iconWidth *= (float)value * 0.03125f;
            break;

        case 0x11:
            *iconHeight *= (float)value * 0.03125f;
            break;

        case 0x12:
            *iconMaterial = Material_RegisterHandle(CL_GetHudMsgIconMaterialName(value), 0, 7);
            *maxChars -= (i + 1);
            return string + i + 1;

        case 0x13:
            *iconWidth *= (float)value * 0.03125f;
            if (iconHorzFlip) {
                *iconHorzFlip = 1;
            }
            break;

        default:
            *maxChars -= i;
            return string + i;
        }
    }

    return NULL;
}

static inline __attribute__((always_inline)) int R_TextWidth_inl(const char *text, int maxChars, FontHandle font)
{
    int lineWidth;
    int maxWidth;
    int count;
    unsigned int letter;
    const Glyph *glyph;

    if (maxChars > 0) {

    } else {
        maxChars = 0x7fffffff;
    }

    lineWidth = 0;
    maxWidth = 0;
    count = 0;

    while (*text && count < maxChars) {
        letter = ri.SEH_ReadCharFromString(&text, 0);
        if (letter == '\n') {
            lineWidth = 0;
            continue;
        }
        if (letter == '^') {
            if (text && *text != '^' && *text > '/' && *text <= '9') {
                text++;
                continue;
            }
        }
        glyph = R_GetCharacterGlyph_inl(font, letter);
        lineWidth += glyph->dx;
        if (lineWidth > maxWidth) {
            maxWidth = lineWidth;
        }
        count++;
    }

    return maxWidth;
}

int R_TextWidth(const char *text, int maxChars, FontHandle font)
{
    return R_TextWidth_inl(text, maxChars, font);
}

int R_ConsoleTextWidth(const short int *string, int maxChars, FontHandle font)
{
    const short int *stringRemaining;
    float width;
    char convertedString[1024];
    Bool iconWasFound;
    float iconWidth;
    float iconHeight;
    MaterialHandle iconMaterial;

    if (!string) {
        return 0;
    }

    stringRemaining = string;
    width = 0.0f;

    while (stringRemaining) {
        stringRemaining = R_GetConsoleString(stringRemaining, &maxChars, convertedString, NULL, &iconWasFound);
        width += (float)R_TextWidth_inl(convertedString, maxChars, font);

        if (iconWasFound) {
            stringRemaining = R_GetConsoleIcon(stringRemaining, &maxChars, &iconWidth, &iconHeight, &iconMaterial, NULL, NULL);
            width += iconWidth;
        }
    }

    return (int)width;
}

void R_DrawConsoleText(const short int *string, int maxChars, FontHandle font, float x, float y, float xScale, float yScale, const vec_t *color, int style)
{
    const short int *stringRemaining;
    float xOfs;
    char convertedString[1024];
    Bool iconWasFound;
    vec4_t curColor;
    float iconWidth;
    float iconHeight;
    MaterialHandle iconMaterial;
    Bool iconHorzFlip;
    float fontPixelHeight;
    float scaledW;
    float scaledH;
    float iconY;
    float iconX;
    int textWidth;

    curColor[0] = color[0];
    curColor[1] = color[1];
    curColor[2] = color[2];
    curColor[3] = color[3];

    if (!string) {
        return;
    }

    stringRemaining = string;
    xOfs = 0.0f;

    while (stringRemaining) {
        stringRemaining = R_GetConsoleString(stringRemaining, &maxChars, convertedString, curColor, &iconWasFound);

        if (convertedString[0] != '\0') {
            R_AddCmdDrawTextWithCursor(convertedString, 0x7fffffff, font, x + xOfs, y, xScale, yScale, curColor, style, -1, 0);
        }

        if (!iconWasFound) {
            continue;
        }

        textWidth = R_TextWidth_inl(convertedString, maxChars, font);
        xOfs += (float)textWidth * xScale;

        stringRemaining = R_GetConsoleIcon(stringRemaining, &maxChars, &iconWidth, &iconHeight, &iconMaterial, curColor, &iconHorzFlip);

        fontPixelHeight = (float)font->pixelHeight;
        scaledW = xScale * fontPixelHeight * iconWidth;
        iconWidth = scaledW;
        scaledH = yScale * fontPixelHeight * iconHeight;
        iconHeight = scaledH;
        iconY = y + (yScale * fontPixelHeight * 0.800000011920929f + scaledH) * -0.5f;
        iconX = x + xOfs;

        if (iconHorzFlip) {
            R_AddCmdDrawStretchPic(iconX, iconY, scaledW, scaledH, 1.0f, 0.0f, 0.0f, 1.0f, curColor, iconMaterial);
        } else {
            R_AddCmdDrawStretchPic(iconX, iconY, scaledW, scaledH, 0.0f, 0.0f, 1.0f, 1.0f, curColor, iconMaterial);
        }

        xOfs += scaledW;
    }

}
