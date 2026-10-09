#include "common_types.h"
#include "imports.h"

extern const char *va(const char *fmt, ...);
extern void Scr_Error(const char *msg);
extern unsigned short Scr_GetConstString(int index);
extern const char *SL_ConvertToString(int stringId);
extern void Scr_AddConstString(int stringId);
extern int Scr_GetInt(int index);
extern const char *Scr_GetString(int index);
extern float Scr_GetFloat(int index);
extern void Scr_AddFloat(float value);
extern void Scr_AddInt(int value);
extern void Scr_AddString(const char *str);
extern void Scr_AddClassField(int classnum, const char *name, unsigned int offset);
extern void Scr_SetGenericField(gclient_t *client, int type, int offset);
extern void Scr_GetGenericField(gclient_t *client, int type, int offset);
extern void ClientUserinfoChanged(int clientNum);
extern void CalculateRanks(void);
extern int GScr_GetStatusIconIndex(const char *name);
extern int GScr_GetHeadIconIndex(const char *name);
extern void SV_GetConfigstring(int index, char *buf, int bufSize);

extern level_locals_t level;
extern gentity_t g_entities[];

#define SCR_CONST() ((const scr_const_t *)imp_scr_const)

static void ClientScr_ReadOnly(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_SetSessionTeam(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_GetSessionTeam(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_SetSessionState(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_GetSessionState(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_SetMaxHealth(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_SetScore(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_SetSpectatorClient(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_SetStatusIcon(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_GetStatusIcon(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_SetHeadIcon(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_GetHeadIcon(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_SetHeadIconTeam(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_GetHeadIconTeam(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_SetArchiveTime(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_GetArchiveTime(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_SetPSOffsetTime(gclient_t *pSelf, const client_fields_s *pField);
static void ClientScr_GetPSOffsetTime(gclient_t *pSelf, const client_fields_s *pField);
void GScr_AddFieldsForClient(void);
void Scr_SetClientField(gclient_t *client, int offset);
void Scr_GetClientField(gclient_t *client, int offset);

static client_fields_t fields[14];
static int fields_inited = 0;
#define CF(i, n, o, t, s, g)                          \
    do {                                              \
        fields[i].name = n;                           \
        fields[i].ofs = o;                            \
        fields[i].type = t;                           \
        fields[i].setter = (ScriptCallbackClient)(s); \
        fields[i].getter = (ScriptCallbackClient)(g); \
    } while (0)

static inline __attribute__((always_inline)) level_locals_t *G_Level(void)
{
    return (level_locals_t *)imp_level;
}

static inline __attribute__((always_inline)) gentity_t *G_Entities(void)
{
    return (gentity_t *)imp_g_entities;
}

static inline __attribute__((always_inline)) int ClientNum(const gclient_t *client)
{
    return (int)(client - G_Level()->clients);
}

static inline __attribute__((always_inline)) gentity_t *ClientEntity(const gclient_t *client)
{
    return &G_Entities()[ClientNum(client)];
}

static void ClientScr_ReadOnly(gclient_t *pSelf, const client_fields_s *pField)
{
    Scr_Error(va("player field %s is read-only", pField->name));
}

static void ClientScr_SetSessionTeam(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    unsigned short str = Scr_GetConstString(0);
    const scr_const_t *sc = SCR_CONST();

    if (str == sc->axis) {
        client->sess.cs.team = 1;
    } else if (str == sc->allies) {
        client->sess.cs.team = 2;
    } else if (str == sc->spectator) {
        client->sess.cs.team = 3;
    } else if (str == sc->none) {
        client->sess.cs.team = 0;
    } else {
        Scr_Error(va("'%s' is an illegal sessionteam string. Must be allies, axis, none, or spectator.", SL_ConvertToString((unsigned short)str)));
    }

    ClientUserinfoChanged(ClientNum(client));
    { (void)(CalculateRanks()); return; }
}

static void ClientScr_GetSessionTeam(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    int team = client->sess.cs.team;

    switch (team) {
    case 1:
        { (void)(Scr_AddConstString(SCR_CONST()->axis)); return; }
    case 2:
        { (void)(Scr_AddConstString(SCR_CONST()->allies)); return; }
    case 3:
        { (void)(Scr_AddConstString(SCR_CONST()->spectator)); return; }
    case 0:
        { (void)(Scr_AddConstString(SCR_CONST()->none)); return; }
    default:
        { (void)(team); return; }
    }
}

static void ClientScr_SetSessionState(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    unsigned short str = Scr_GetConstString(0);
    const scr_const_t *sc = SCR_CONST();

    if (str == sc->playing) {
        client->sess.sessionState = 0;
    } else if (str == sc->dead) {
        client->sess.sessionState = 1;
    } else if (str == sc->spectator) {
        client->sess.sessionState = 2;
    } else if (str == sc->intermission) {
        ((gclient_t *)client)->ps.eFlags ^= 2;
        client->sess.sessionState = 3;
    } else {
        Scr_Error(va("'%s' is an illegal sessionstate string. Must be playing, dead, spectator, or intermission.", SL_ConvertToString((unsigned short)str)));
    }
}

static void ClientScr_GetSessionState(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    int state = client->sess.sessionState;

    switch (state) {
    case 0:
        { (void)(Scr_AddConstString(SCR_CONST()->playing)); return; }
    case 1:
        { (void)(Scr_AddConstString(SCR_CONST()->dead)); return; }
    case 2:
        { (void)(Scr_AddConstString(SCR_CONST()->spectator)); return; }
    case 3:
        { (void)(Scr_AddConstString(SCR_CONST()->intermission)); return; }
    default:
        { (void)(state); return; }
    }
}

static void ClientScr_SetMaxHealth(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    int val;
    int health;

    val = Scr_GetInt(0);
    val = (val > 0) ? val : 1;

    client->sess.maxHealth = val;

    if (client->ps.stats[0] > val) {
        client->ps.stats[0] = val;
        health = val;
    } else {
        health = client->ps.stats[0];
    }

    {
        gentity_s *ent = ClientEntity(client);
        ent->health = health;

        client->ps.stats[2] = client->sess.maxHealth;
    }
}

static void ClientScr_SetScore(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    client->sess.score = Scr_GetInt(0);
    { (void)(CalculateRanks()); return; }
}

static void ClientScr_SetSpectatorClient(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    int iNewSpectatorClient = Scr_GetInt(0);

    if ((unsigned int)(iNewSpectatorClient + 1) > 0x40) {
        Scr_Error("spectatorclient can only be set to -1, or a valid client number");
    }

    client->sess.forceSpectatorClient = iNewSpectatorClient;
}

static void ClientScr_SetStatusIcon(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    client->sess.status_icon = GScr_GetStatusIconIndex(Scr_GetString(0));
}

static void ClientScr_GetStatusIcon(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    char szConfigString[1024];

    if (client->sess.status_icon == 0) {
        Scr_AddString("");
        return;
    }

    SV_GetConfigstring(client->sess.status_icon + 0x16, szConfigString, 1024);
    Scr_AddString(szConfigString);
}

static void ClientScr_SetHeadIcon(gclient_t *pSelf, const client_fields_s *pField)
{

    gentity_s *ent = ClientEntity(pSelf);
    ((ent)->s.iHeadIcon) = GScr_GetHeadIconIndex(Scr_GetString(0));
}

static void ClientScr_GetHeadIcon(gclient_t *pSelf, const client_fields_s *pField)
{
    int clientNum = ClientNum(pSelf);
    gentity_s *ent = &G_Entities()[clientNum];
    char szConfigString[1024];
    int icon = ((ent)->s.iHeadIcon);

    if (icon == 0) {
        Scr_AddString("");
        return;
    }
    if (icon > 0xf) {
        return;
    }

    SV_GetConfigstring(icon + 0x1e, szConfigString, 1024);
    Scr_AddString(szConfigString);
}

static void ClientScr_SetHeadIconTeam(gclient_t *pSelf, const client_fields_s *pField)
{
    gentity_s *ent = ClientEntity(pSelf);
    unsigned short str = Scr_GetConstString(0);
    const scr_const_t *sc = SCR_CONST();

    if (str == sc->none) {
        ((ent)->s.iHeadIconTeam) = 0;
    } else if (str == sc->allies) {
        ((ent)->s.iHeadIconTeam) = 2;
    } else if (str == sc->axis) {
        ((ent)->s.iHeadIconTeam) = 1;
    } else if (str == sc->spectator) {
        Scr_Error(va("'%s' is an illegal head icon team string.", SL_ConvertToString((unsigned short)str)));
    } else {
        ((ent)->s.iHeadIconTeam) = 3;
    }
}

static void ClientScr_GetHeadIconTeam(gclient_t *pSelf, const client_fields_s *pField)
{
    int clientNum = ClientNum(pSelf);
    gentity_s *ent = &G_Entities()[clientNum];
    int team = ((ent)->s.iHeadIconTeam);
    const scr_const_t *sc = SCR_CONST();

    switch (team) {
    case 2:
        { (void)(Scr_AddConstString(sc->allies)); return; }
    case 3:
        { (void)(Scr_AddConstString(sc->spectator)); return; }
    case 1:
        { (void)(Scr_AddConstString(sc->axis)); return; }
    default:
        { (void)(Scr_AddConstString(sc->none)); return; }
    }
}

static void ClientScr_SetArchiveTime(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    client->sess.archiveTime = (int)(Scr_GetFloat(0) * 1000.0f);
}

static void ClientScr_GetArchiveTime(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    { (void)(Scr_AddFloat((float)client->sess.archiveTime * 0.001f)); return; }
}

static void ClientScr_SetPSOffsetTime(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    client->sess.psOffsetTime = Scr_GetInt(0);
}

static void ClientScr_GetPSOffsetTime(gclient_t *pSelf, const client_fields_s *pField)
{
    gclient_s *client = (gclient_s *)pSelf;
    { (void)(Scr_AddInt(client->sess.archiveTime)); return; }
}

void GScr_AddFieldsForClient(void)
{
    const char *name;
    int byteOffset;
    const client_fields_t *next;

    if (!fields_inited) {
        fields_inited = 1;
        CF(0, "name", 0x2784, F_LSTRING, ClientScr_ReadOnly, NULL);
        CF(1, "sessionteam", 0, F_STRING, ClientScr_SetSessionTeam, ClientScr_GetSessionTeam);
        CF(2, "sessionstate", 0, F_STRING, ClientScr_SetSessionState, ClientScr_GetSessionState);
        CF(3, "maxhealth", 0x2728, F_INT, ClientScr_SetMaxHealth, NULL);
        CF(4, "score", 0x26b8, F_INT, ClientScr_SetScore, NULL);
        CF(5, "deaths", 0x26bc, F_INT, NULL, NULL);
        CF(6, "statusicon", 0, F_STRING, ClientScr_SetStatusIcon, ClientScr_GetStatusIcon);
        CF(7, "headicon", 0, F_STRING, ClientScr_SetHeadIcon, ClientScr_GetHeadIcon);
        CF(8, "headiconteam", 0, F_STRING, ClientScr_SetHeadIconTeam, ClientScr_GetHeadIconTeam);
        CF(9, "spectatorclient", 0x26ac, F_INT, ClientScr_SetSpectatorClient, NULL);
        CF(10, "archivetime", 0x26b4, F_FLOAT, ClientScr_SetArchiveTime, ClientScr_GetArchiveTime);
        CF(11, "psoffsettime", 0x27a4, F_INT, ClientScr_SetPSOffsetTime, ClientScr_GetPSOffsetTime);
        CF(12, "pers", 0x26c0, F_OBJECT, ClientScr_ReadOnly, NULL);
    }

    name = fields[0].name;
    byteOffset = 0;   /* reused as the field index */
    next = &fields[1];

    while (name) {
        /* same x64 fix as hudelem: the original hash encodes (fieldIndex | 0xc000) via a stride
           = sizeof(client_fields_t)/4 that is even on x64 (no inverse mod 2^16) -> the index isn't
           recovered. Store the index directly (identical to the x86 hash result). */
        Scr_AddClassField(0, name, (unsigned short)(byteOffset | 0xc000));

        name = next->name;
        byteOffset++;
        next = (const client_fields_t *)((const char *)next + sizeof(client_fields_t));
    }
}

void Scr_SetClientField(gclient_t *client, int offset)
{
    const client_fields_t *field = &fields[offset];
    void (*setter)(gclient_t *, const client_fields_s *) =
        (void (*)(gclient_t *, const client_fields_s *))field->setter;

    if (setter != NULL) {
        setter(client, field);
    } else {
        Scr_SetGenericField(client, field->type, field->ofs);
    }
}

void Scr_GetClientField(gclient_t *client, int offset)
{
    const client_fields_t *field = &fields[offset];
    void (*getter)(gclient_t *, const client_fields_s *) =
        (void (*)(gclient_t *, const client_fields_s *))field->getter;

    if (getter != NULL) {
        getter(client, field);
    } else {
        Scr_GetGenericField(client, field->type, field->ofs);
    }
}
