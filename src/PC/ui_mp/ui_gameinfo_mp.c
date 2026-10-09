#include "common_types.h"
#include "imports.h"
#include <string.h>

extern const char *Com_Parse(const char **buf);
extern const char *Com_ParseOnLine(const char **buf);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void Info_SetValueForKey(char *s, const char *key, const char *value);
extern void Com_Printf(const char *msg, ...);
extern const char *va(const char *format, ...);
extern void *UI_Alloc(int size, int align);
extern int FS_GetFileList(const char *path, const char *extension, int flags, char *listBuf, int bufSize);
extern int FS_FOpenFileByMode(const char *filename, int *f, int mode);
extern int FS_Read(void *buffer, int len, int f);
extern void FS_FCloseFile(fileHandle_t f);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern const char *String_Alloc(const char *str);
extern MaterialHandle CL_RegisterMaterialNoMip(const char *name, int user);
extern void Com_BeginParseSession(const char *name);
extern void Com_EndParseSession(void);
extern int I_stricmp(const char *s1, const char *s2);

extern sharedUiInfo_t sharedUiInfo;

static int ui_numArenas;
static char *ui_arenaInfos[64];

int UI_ParseInfos(const char *buf, int max, char **infos);
void UI_LoadArenas(void);

int UI_ParseInfos(const char *buf, int max, char **infos)
{
    int count = 0;
    char info[0x400];
    char key[0x400];
    const char *token;
    char *vaStr;
    int infoLen;
    int vaLen;
    int allocSize;

    while (1) {
        token = Com_Parse(&buf);
        if (*token == '\0')
            break;

        if (strcmp(token, "{") != 0) {
            Com_Printf("Missing { in info file\n");
            return count;
        }

        if (count == max) {
            Com_Printf("Max infos exceeded\n");
            return count;
        }

        info[0] = '\0';

        while (1) {
            token = Com_Parse(&buf);
            if (*token == '\0') {
                Com_Printf("Unexpected end of info file\n");
                break;
            }
            if (strcmp(token, "}") == 0)
                break;

            I_strncpyz(key, token, 0x400);

            token = Com_ParseOnLine(&buf);
            if (*token == '\0')
                token = "<NULL>";

            Info_SetValueForKey(info, key, token);
        }

        infoLen = strlen(info);
        vaStr = (char *)va("%d", 0x40);
        vaLen = strlen(vaStr) + 1;
        allocSize = vaLen + infoLen + 5;

        infos[count] = (char *)UI_Alloc(allocSize, 1);
        if (infos[count] == NULL)
            continue;

        strcpy(infos[count], info);
        count++;
    }

    return count;
}

void UI_LoadArenas(void)
{
    char dirlist[0x400];
    int numdirs;
    char *dirptr;
    int dirlen;
    int i;
    int f;
    int len;
    char buf[0x2000];
    char filename[0x80];
    byte *uiInfo;
    int n;
    int arenaIdx;
    const char *mapName;
    const char *longName;
    const char *loadscreen;
    const char *gametypes;
    const char *token;
    int j;

    ui_numArenas = 0;

    uiInfo = (byte *)&sharedUiInfo;
    sharedUiInfo.mapCount = 0;

    numdirs = FS_GetFileList("mp", "arena", 0, dirlist, 0x400);

    if (numdirs > 0) {
        dirptr = dirlist;
        for (i = 0; i < numdirs; i++) {
            dirlen = strlen(dirptr);

            sprintf(filename, "%s/%s", "mp", dirptr);

            len = FS_FOpenFileByMode(filename, &f, 0);
            if (f == 0) {
                Com_Printf("^1file not found: %s\n", filename);
            } else if (len > 0x1fff) {
                Com_Printf("^1file too large: %s is %i, max allowed is %i", filename, len, 0x2000);
                FS_FCloseFile(f);
            } else {
                FS_Read(buf, len, f);
                buf[len] = '\0';
                FS_FCloseFile(f);

                ui_numArenas += UI_ParseInfos(buf, 64 - ui_numArenas, &ui_arenaInfos[ui_numArenas]);
            }

            dirptr += dirlen + 1;
        }
    }

    if (ui_numArenas <= 0)
        return;

    uiInfo = (byte *)&sharedUiInfo;
    for (n = 0; n < ui_numArenas; n++) {
        arenaIdx = sharedUiInfo.mapCount;

        mapName = Info_ValueForKey(ui_arenaInfos[n], "map");
#ifdef __EMSCRIPTEN__
        if (I_stricmp(mapName, "mp_toujane") != 0 && I_stricmp(mapName, "mp_carentan") != 0)
            continue;
#endif
        mapName = String_Alloc(mapName);
        {
            int off = arenaIdx * 41;
            sharedUiInfo.mapList[off / 41].mapLoadName = mapName;
        }

        arenaIdx = sharedUiInfo.mapCount;
        longName = Info_ValueForKey(ui_arenaInfos[n], "longname");
        longName = String_Alloc(longName);
        {
            int off = arenaIdx * 41;
            sharedUiInfo.mapList[off / 41].mapName = longName;
        }

        arenaIdx = sharedUiInfo.mapCount;
        {
            int off = arenaIdx * 41;
            sharedUiInfo.mapList[off / 41].cinematic = -1;
            sharedUiInfo.mapList[off / 41].levelShot = 0;
        }

        arenaIdx = sharedUiInfo.mapCount;
        {
            int off = arenaIdx * 41;
            const char *mapStr = sharedUiInfo.mapList[off / 41].mapLoadName;
            loadscreen = String_Alloc(va("loadscreen_%s", mapStr));
            sharedUiInfo.mapList[off / 41].imageName = loadscreen;
        }

        arenaIdx = sharedUiInfo.mapCount;
        {
            const char *ls = sharedUiInfo.mapList[arenaIdx].imageName;
            MaterialHandle mat = CL_RegisterMaterialNoMip(ls, 3);
            sharedUiInfo.mapList[arenaIdx].levelShot = mat;
        }

        gametypes = Info_ValueForKey(ui_arenaInfos[n], "type");
        if (gametypes == NULL || *gametypes == '\0') {

            arenaIdx = sharedUiInfo.mapCount;
            {
                int off = arenaIdx * 41;
                sharedUiInfo.mapList[off / 41].typeBits = -1;
            }
        } else {

            arenaIdx = sharedUiInfo.mapCount;
            {
                int off = arenaIdx * 41;
                sharedUiInfo.mapList[off / 41].typeBits = 0;
            }

            {
                const char *pszText = gametypes;
                const char *gtMap;
                Com_BeginParseSession(va(".arena files : %s", sharedUiInfo.mapList[arenaIdx].mapLoadName));

                while (1) {
                    token = Com_Parse(&pszText);
                    if (token == NULL || *token == '\0')
                        break;

                    int numGT = sharedUiInfo.numGameTypes;
                    for (j = 0; j < numGT; j++) {
                        gtMap = sharedUiInfo.gameTypes[j].gameType;
                        if (I_stricmp(token, gtMap) == 0) {
                            arenaIdx = sharedUiInfo.mapCount;
                            {
                                int off = arenaIdx * 41;
                                sharedUiInfo.mapList[off / 41].typeBits |= (1 << j);
                            }
                            break;
                        }
                    }
                }

                Com_EndParseSession();
            }
        }

        uiInfo = (byte *)&sharedUiInfo;
        arenaIdx = sharedUiInfo.mapCount;
        sharedUiInfo.mapCount = arenaIdx + 1;
        if (arenaIdx + 1 > 0x7f)
            return;
    }
}
