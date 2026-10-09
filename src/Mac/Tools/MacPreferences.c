#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common_types.h"
#include "imports.h"

#define PREFS_DIR ".cod2"
#define PREFS_FILE ".cod2/preferences"
#define MAX_LINE 1024

static char prefs_path[512] = { 0 };

static void ensure_prefs_path(void)
{
    const char *home;
    if (prefs_path[0])
        return;
    home = getenv("HOME");
    if (!home)
        home = ".";
    snprintf(prefs_path, sizeof(prefs_path), "%s/%s", home, PREFS_FILE);

    {
        char dir[512];
        snprintf(dir, sizeof(dir), "%s/%s", home, PREFS_DIR);
        mkdir(dir, 0755);
    }
}

unsigned char MacPreferences_GetString(const char *inKey, char *outString, int inMaxString, const char *inDefault)
{
    FILE *f;
    char line[MAX_LINE];
    int keylen;

    ensure_prefs_path();
    keylen = strlen(inKey);

    f = fopen(prefs_path, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, inKey, keylen) == 0 && line[keylen] == '=') {
                char *val = line + keylen + 1;

                char *nl = strchr(val, '\n');
                if (nl)
                    *nl = '\0';
                strncpy(outString, val, inMaxString - 1);
                outString[inMaxString - 1] = '\0';
                fclose(f);
                return 1;
            }
        }
        fclose(f);
    }

    if (inDefault) {
        strcpy(outString, inDefault);
    } else {
        outString[0] = '\0';
    }
    return 0;
}

void MacPreferences_PutString(const char *inKey, const char *inString)
{
    FILE *f;
    char tmppath[520];
    char line[MAX_LINE];
    int keylen;
    int found = 0;

    ensure_prefs_path();
    keylen = strlen(inKey);

    snprintf(tmppath, sizeof(tmppath), "%s.tmp", prefs_path);

    {
        FILE *out = fopen(tmppath, "w");
        if (!out)
            return 0;

        f = fopen(prefs_path, "r");
        if (f) {
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, inKey, keylen) == 0 && line[keylen] == '=') {
                    fprintf(out, "%s=%s\n", inKey, inString);
                    found = 1;
                } else {
                    fputs(line, out);
                }
            }
            fclose(f);
        }

        if (!found)
            fprintf(out, "%s=%s\n", inKey, inString);

        fclose(out);
    }

    rename(tmppath, prefs_path);
    return 1;
}

int MacPreferences_GetInteger(const char *inKey, int inDefault)
{
    char buffer[256];
    int result = inDefault;
    MacPreferences_GetString(inKey, buffer, sizeof(buffer), NULL);
    if (buffer[0] != '\0')
        result = atoi(buffer);
    return result;
}

void MacPreferences_PutInteger(const char *inKey, int inNumber)
{
    char buffer[256];
    sprintf(buffer, "%d", inNumber);
    MacPreferences_PutString(inKey, buffer);
}

void MacPreferences_Synchronize(void)
{
    /* PutString writes and closes the portable preferences file directly. */
    fflush(NULL);
}

unsigned char MacPreferences_PutRect(const char *inKey, const CGRect *inRect)
{
    char subKey[280];
    char buffer[256];
    float *r = (float *)inRect;

    sprintf(subKey, "%s.origin.x", inKey);
    sprintf(buffer, "%f", (double)r[0]);
    MacPreferences_PutString(subKey, buffer);

    sprintf(subKey, "%s.origin.y", inKey);
    sprintf(buffer, "%f", (double)r[1]);
    MacPreferences_PutString(subKey, buffer);

    sprintf(subKey, "%s.size.width", inKey);
    sprintf(buffer, "%f", (double)r[2]);
    MacPreferences_PutString(subKey, buffer);

    sprintf(subKey, "%s.size.height", inKey);
    sprintf(buffer, "%f", (double)r[3]);
    MacPreferences_PutString(subKey, buffer);

    return 1;
}

unsigned char MacPreferences_GetRect(const char *inKey, CGRect *outRect)
{
    char subKey[280];
    char buffer[256];
    float *r = (float *)outRect;

    sprintf(subKey, "%s.origin.x", inKey);
    MacPreferences_GetString(subKey, buffer, sizeof(buffer), NULL);
    r[0] = (buffer[0] != '\0') ? (float)atof(buffer) : 0.0f;

    sprintf(subKey, "%s.origin.y", inKey);
    MacPreferences_GetString(subKey, buffer, sizeof(buffer), NULL);
    r[1] = (buffer[0] != '\0') ? (float)atof(buffer) : 0.0f;

    sprintf(subKey, "%s.size.width", inKey);
    MacPreferences_GetString(subKey, buffer, sizeof(buffer), NULL);
    r[2] = (buffer[0] != '\0') ? (float)atof(buffer) : 0.0f;

    sprintf(subKey, "%s.size.height", inKey);
    MacPreferences_GetString(subKey, buffer, sizeof(buffer), NULL);
    r[3] = (buffer[0] != '\0') ? (float)atof(buffer) : 0.0f;

    return 1;
}

void MacPreferences_PutBoolean(const char *inKey, int inBoolean)
{
    char buffer[256];
    sprintf(buffer, "%d", (unsigned char)inBoolean);
    MacPreferences_PutString(inKey, buffer);
}

unsigned char MacPreferences_GetBoolean(const char *inKey, int inDefault)
{
    char buffer[256];
    int result = (unsigned char)inDefault;

    MacPreferences_GetString(inKey, buffer, sizeof(buffer), NULL);
    if (buffer[0] != '\0')
        result = atoi(buffer);

    return result != 0;
}
