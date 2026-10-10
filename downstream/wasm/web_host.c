#include "common_types.h"
#include <emscripten.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

extern const dvar_t *com_sv_running;
extern const unsigned char web_browser_bots[];
extern const unsigned int web_browser_bots_size;
extern void Cbuf_ExecuteText(int when, const char *text);
extern void Dvar_SetBoolByName(const char *dvarName, unsigned char value);
extern void SV_AddOperatorCommands(void);

static const char *const web_host_maps[] = {"mp_toujane", "mp_carentan", "mp_dawnville"};
static const char *const web_host_gametypes[] = {"dm", "tdm", "ctf", "hq", "sd"};

EM_JS(void, WebP2P_StartHost, (void), {
    Module.cod2P2P.startHost();
});

EMSCRIPTEN_KEEPALIVE int web_local_server_running(void) {
    return com_sv_running && com_sv_running->current.enabled;
}

static int WebHost_MakeDirectory(const char *path) {
    return !mkdir(path, 0755) || errno == EEXIST;
}

static int WebHost_WriteBots(void) {
    FILE *file;
    size_t written;
    if (!WebHost_MakeDirectory("/profile/raw") || !WebHost_MakeDirectory("/profile/raw/maps") ||
        !WebHost_MakeDirectory("/profile/raw/maps/mp"))
        return 0;
    file = fopen("/profile/raw/maps/mp/browser_bots.gsc", "wb");
    if (!file) return 0;
    written = fwrite(web_browser_bots, 1, web_browser_bots_size, file);
    return !fclose(file) && written == web_browser_bots_size;
}

void Web_StartLocalServer(const char *mapName, const char *gameType, int botCount, int botDifficulty) {
    static char command[1024];
    char rotation[128];
    char remaining[128];
    int selected = -1;
    int valid = 0;
    int i;

    for (i = 0; i < 3; ++i)
        if (!strcmp(mapName, web_host_maps[i])) selected = i;
    for (i = 0; i < 5; ++i)
        if (!strcmp(gameType, web_host_gametypes[i])) valid = 1;
    if (selected < 0 || !valid) return;
    if (botCount < 0 || botCount > 32) botCount = 0;
    if (botDifficulty < 0 || botDifficulty > 2) botDifficulty = 1;
    if (botCount && !WebHost_WriteBots()) botCount = 0;

    snprintf(rotation, sizeof(rotation), "gametype %s map %s map %s map %s",
             gameType, web_host_maps[0], web_host_maps[1], web_host_maps[2]);
    remaining[0] = '\0';
    if (selected < 2) {
        snprintf(remaining, sizeof(remaining), "gametype %s", gameType);
        for (i = selected + 1; i < 3; ++i) {
            size_t used = strlen(remaining);
            snprintf(remaining + used, sizeof(remaining) - used, " map %s", web_host_maps[i]);
        }
    }

    SV_AddOperatorCommands();
    Dvar_SetBoolByName("cg_thirdPerson", 0);
    snprintf(command, sizeof(command),
             "set sv_maxclients 64; set sv_pure 0; set sv_allowDownload 0; set sv_floodProtect 1; "
             "set sv_voice 0; set sv_timeout 30; set g_gametype %s; "
             "set scr_tdm_timelimit 15; set scr_tdm_scorelimit 100; set scr_allies \"\"; set scr_axis \"\"; "
             "set scr_teambalance 0; set scr_forcerespawn 1; set scr_killcam 1; "
             "set sv_mapRotation \"%s\"; set sv_mapRotationCurrent \"%s\"; "
             "set scr_bot_count %d; set scr_bot_difficulty %d; wait; wait; map %s\n",
             gameType, rotation, remaining, botCount, botDifficulty, mapName);
    Cbuf_ExecuteText(2, command);
    WebP2P_StartHost();
}
