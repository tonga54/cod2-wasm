#include "common_types.h"
#include "imports.h"
#include <string.h>

extern NetField hudElemFields[32];
extern NetField objectiveFields[8];

int msg_hData[256] = { 0x3d1cb, 0xa0e9, 0x1894, 0x1bc2, 0xe92, 0xea6, 0x17de, 0x5af3, 0x8225, 0x1b26, 0x1e9e, 0x25f2, 0x2429, 0x436b, 0xf6d, 0x6f2, 0x2060, 0x644, 0x636, 0x67f, 0x44c, 0x4bd, 0x4d6, 0x46e, 0x6d5, 0x423, 0x4de, 0x47d, 0x4f9, 0x1186, 0xaf5, 0xd90, 0x553b, 0x487, 0x686, 0x42a, 0x413, 0x3f4, 0x41d, 0x42e, 0x6be, 0x378, 0x49c, 0x352, 0x3c0, 0x30c, 0x6d8, 0xce0, 0x2986, 0x11a2, 0x16f9, 0xa7d, 0x122a, 0xefd, 0x82d, 0x74b, 0xa18, 0x79d, 0x7b4, 0x3ac, 0x46e, 0x6fc, 0x686, 0x4b6, 0x1657, 0x17f0, 0x1c36, 0x19fe, 0xe7e, 0xed3, 0x5d4, 0x5f4, 0x8a7, 0x474, 0x54b, 0x3cb, 0x884, 0x4e0, 0x530, 0x4ab, 0x6ea, 0x436, 0x4f0, 0x4f2, 0x490, 0x3c5, 0x483, 0x4a2, 0x543, 0x4cc, 0x5f9, 0x640, 0xa39, 0x800, 0x9f2, 0xccb, 0x96a, 0xe01, 0x9c8, 0xaf0, 0xa73, 0x1802, 0xe4f, 0xb18, 0x37ad, 0xc5c, 0x8ad, 0x697, 0xc88, 0xab3, 0xdb8, 0x12bc, 0xffb, 0xdbb, 0x14a8, 0xfb0, 0x1f01, 0x178f, 0x14f0, 0xf54, 0x131c, 0xe9f, 0x11d6, 0x12c7, 0x16dc, 0x1900, 0x1851, 0x2063, 0x5acb, 0x1e9e, 0x1ba1, 0x22e7, 0x153d, 0x1183, 0xe39, 0x1488, 0x14c0, 0x14d0, 0x14fa, 0xda4, 0x99a, 0x69e, 0x71d, 0x849, 0x77c, 0x47d, 0x5ec, 0x557, 0x4d4, 0x405, 0x4ea, 0x450, 0x4dd, 0x3ee, 0x47d, 0x401, 0x4d9, 0x3b8, 0x507, 0x3e5, 0x6b1, 0x3f1, 0x4a3, 0x36f, 0x44b, 0x3a1, 0x436, 0x3b7, 0x678, 0x3a2, 0x481, 0x406, 0x4ee, 0x426, 0x4be, 0x424, 0x655, 0x3a2, 0x452, 0x390, 0x40a, 0x37c, 0x486, 0x3de, 0x497, 0x352, 0x461, 0x387, 0x43f, 0x398, 0x478, 0x420, 0xd86, 0x8c0, 0x112d, 0x2f68, 0x1e4e, 0x541, 0x51b, 0xcce, 0x79e, 0x376, 0x3ff, 0x458, 0x435, 0x412, 0x425, 0x42f, 0x5cc, 0x3e9, 0x448, 0x393, 0x41c, 0x3e3, 0x42e, 0x36c, 0x457, 0x353, 0x423, 0x325, 0x458, 0x39b, 0x44f, 0x331, 0x76b, 0x750, 0x3d0, 0x349, 0x467, 0x3bc, 0x487, 0x3b6, 0x1e6f, 0x3ba, 0x509, 0x3a5, 0x467, 0xc87, 0x3fc, 0x39f, 0x54b, 0x300, 0x410, 0x2e9, 0x3b8, 0x325, 0x431, 0x2e4, 0x3f5, 0x325, 0x3f0, 0x31c, 0x3e4, 0x421, 0x2cc1, 0x34c0 };


extern int msg_hData[256];
static char string_00f0e860[1024];
static char string_00f0ec60[8192];
static char string_00f10c60[1024];
static struct huffman_t msgHuff;
static qboolean msgInit;
extern unsigned int kbitmask[33];
extern void *imp_cl_shownet;
extern void Com_Printf(const char *fmt, ...);
extern int I_CleanChar(int ch);
extern void I_strncpyz(char *dest, const char *src, int destsize);

COD2_ASSERT_FIELD(struct huffman_t, decompressor, 28700);
COD2_ASSERT_FIELD(struct huffman_t, decompressor.tree, 28708);

#define PSF(x) #x, __builtin_offsetof(playerState_t, x)
#define CSF(x) #x, __builtin_offsetof(clientState_t, x)
#define AEF(x) #x, __builtin_offsetof(archivedEntity_t, x)
#define ESF(x) #x, __builtin_offsetof(entityState_t, x)

static const NetField playerStateFields[105] = {
    { PSF(commandTime), 32 },
    { PSF(origin[1]), 0 },
    { PSF(origin[0]), 0 },
    { PSF(bobCycle), 8 },
    { PSF(viewangles[1]), -100 },
    { PSF(origin[2]), 0 },
    { PSF(velocity[1]), 0 },
    { PSF(velocity[0]), 0 },
    { PSF(viewangles[0]), -100 },
    { PSF(movementDir), -8 },
    { PSF(velocity[2]), 0 },
    { PSF(eventSequence), 8 },
    { PSF(legsAnim), 10 },
    { PSF(aimSpreadScale), 0 },
    { PSF(weaponTime), -16 },
    { PSF(pm_flags), 27 },
    { PSF(events[0]), 8 },
    { PSF(events[1]), 8 },
    { PSF(events[2]), 8 },
    { PSF(events[3]), 8 },
    { PSF(weapAnim), 10 },
    { PSF(viewHeightCurrent), 0 },
    { PSF(torsoTimer), 16 },
    { PSF(torsoAnim), 10 },
    { PSF(eFlags), 24 },
    { PSF(fWeaponPosFrac), 0 },
    { PSF(holdBreathScale), 0 },
    { PSF(weaponstate), 5 },
    { PSF(viewHeightTarget), -8 },
    { PSF(weaponDelay), -16 },
    { PSF(legsTimer), 16 },
    { PSF(viewHeightLerpTarget), -8 },
    { PSF(groundEntityNum), 10 },
    { PSF(pm_time), -16 },
    { PSF(eventParms[3]), 8 },
    { PSF(eventParms[1]), 8 },
    { PSF(eventParms[0]), 8 },
    { PSF(eventParms[2]), 8 },
    { PSF(weapon), 7 },
    { PSF(weapons[0]), 32 },
    { PSF(viewHeightLerpDown), 1 },
    { PSF(weaponslots[0]), 32 },
    { PSF(delta_angles[0]), 16 },
    { PSF(delta_angles[1]), 16 },
    { PSF(cursorHintString), -8 },
    { PSF(offHandIndex), 7 },
    { PSF(clientNum), 8 },
    { PSF(viewlocked_entNum), 16 },
    { PSF(viewmodelIndex), 8 },
    { PSF(viewHeightLerpTime), 32 },
    { PSF(speed), 16 },
    { PSF(mins[1]), 0 },
    { PSF(mins[0]), 0 },
    { PSF(maxs[2]), 0 },
    { PSF(maxs[1]), 0 },
    { PSF(maxs[0]), 0 },
    { PSF(gravity), 16 },
    { PSF(damageTimer), 16 },
    { PSF(cursorHint), 8 },
    { PSF(mantleState.flags), 4 },
    { PSF(flinchYaw), 16 },
    { PSF(fWaistPitch), 0 },
    { PSF(mantleState.timer), 32 },
    { PSF(fTorsoPitch), 0 },
    { PSF(proneTorsoPitch), 0 },
    { PSF(holdBreathTimer), 16 },
    { PSF(jumpTime), 32 },
    { PSF(viewangles[2]), -100 },
    { PSF(foliageSoundTime), 32 },
    { PSF(weapons[1]), 32 },
    { PSF(damageEvent), 8 },
    { PSF(damageDuration), 16 },
    { PSF(damageYaw), 8 },
    { PSF(proneDirection), 0 },
    { PSF(proneDirectionPitch), 0 },
    { PSF(mantleState.yaw), 0 },
    { PSF(mantleState.transIndex), 4 },
    { PSF(fTorsoHeight), 0 },
    { PSF(damagePitch), 8 },
    { PSF(jumpOriginZ), 0 },
    { PSF(pm_type), 8 },
    { PSF(viewlocked), 8 },
    { PSF(weaponrechamber[0]), 32 },
    { PSF(vLadderVec[0]), 0 },
    { PSF(weaponslots[4]), 32 },
    { PSF(weaponRestrictKickTime), -16 },
    { PSF(vLadderVec[1]), 0 },
    { PSF(viewAngleClampRange[1]), 0 },
    { PSF(viewAngleClampRange[0]), 0 },
    { PSF(viewAngleClampBase[1]), 0 },
    { PSF(weaponrechamber[1]), 32 },
    { PSF(leanf), 0 },
    { PSF(damageCount), 7 },
    { PSF(grenadeTimeLeft), -16 },
    { PSF(deltaTime), 32 },
    { PSF(shellshockTime), 32 },
    { PSF(shellshockIndex), 4 },
    { PSF(shellshockDuration), 16 },
    { PSF(vLadderVec[2]), 0 },
    { PSF(delta_angles[2]), 16 },
    { PSF(viewHeightLerpPosAdj), 0 },
    { PSF(mins[2]), 0 },
    { PSF(viewAngleClampBase[0]), 0 },
    { PSF(adsDelayTime), 32 },
    { PSF(iCompassFriendInfo), 32 },
};

static const NetField clientStateFields[22] = {
    { CSF(team), 2 },
    { CSF(name[0]), 32 },
    { CSF(name[4]), 32 },
    { CSF(modelindex), 8 },
    { CSF(attachModelIndex[1]), 8 },
    { CSF(attachModelIndex[0]), 8 },
    { CSF(name[8]), 32 },
    { CSF(name[12]), 32 },
    { CSF(name[16]), 32 },
    { CSF(name[20]), 32 },
    { CSF(name[24]), 32 },
    { CSF(name[28]), 32 },
    { CSF(attachTagIndex[5]), 5 },
    { CSF(attachTagIndex[0]), 5 },
    { CSF(attachTagIndex[1]), 5 },
    { CSF(attachTagIndex[2]), 5 },
    { CSF(attachTagIndex[3]), 5 },
    { CSF(attachTagIndex[4]), 5 },
    { CSF(attachModelIndex[2]), 8 },
    { CSF(attachModelIndex[3]), 8 },
    { CSF(attachModelIndex[4]), 8 },
    { CSF(attachModelIndex[5]), 8 },
};

static const NetField archivedEntityFields[68] = {
    { AEF(r.absmin[1]), 0 },
    { AEF(r.absmax[1]), 0 },
    { AEF(r.absmin[0]), 0 },
    { AEF(r.absmax[0]), 0 },
    { AEF(r.absmin[2]), 0 },
    { AEF(r.absmax[2]), 0 },
    { AEF(s.pos.trBase[1]), 0 },
    { AEF(s.pos.trBase[0]), 0 },
    { AEF(s.eType), 8 },
    { AEF(s.eFlags), 24 },
    { AEF(s.pos.trBase[2]), 0 },
    { AEF(r.svFlags), 32 },
    { AEF(s.groundEntityNum), 10 },
    { AEF(s.apos.trBase[1]), 0 },
    { AEF(s.clientNum), 8 },
    { AEF(s.apos.trBase[0]), 0 },
    { AEF(s.index), 10 },
    { AEF(s.apos.trBase[2]), 0 },
    { AEF(s.eventSequence), 8 },
    { AEF(s.events[0]), 8 },
    { AEF(s.legsAnim), 10 },
    { AEF(s.events[1]), 8 },
    { AEF(s.events[2]), 8 },
    { AEF(s.events[3]), 8 },
    { AEF(s.weapon), 7 },
    { AEF(s.pos.trType), 8 },
    { AEF(s.pos.trTime), 32 },
    { AEF(s.apos.trType), 8 },
    { AEF(s.solid), 24 },
    { AEF(s.pos.trDuration), 32 },
    { AEF(s.eventParms[0]), 8 },
    { AEF(s.torsoAnim), 10 },
    { AEF(s.pos.trDelta[0]), 0 },
    { AEF(s.pos.trDelta[1]), 0 },
    { AEF(s.angles2[1]), 0 },
    { AEF(s.angles2[0]), 0 },
    { AEF(s.animMovetype), 4 },
    { AEF(s.pos.trDelta[2]), 0 },
    { AEF(s.otherEntityNum), 10 },
    { AEF(s.eventParms[1]), 8 },
    { AEF(s.surfType), 8 },
    { AEF(s.eventParm), 8 },
    { AEF(s.eventParms[2]), 8 },
    { AEF(s.scale), 8 },
    { AEF(s.eventParms[3]), 8 },
    { AEF(s.fTorsoHeight), 0 },
    { AEF(s.fWaistPitch), 0 },
    { AEF(s.fTorsoPitch), 0 },
    { AEF(s.apos.trTime), 32 },
    { AEF(s.apos.trDelta[0]), 0 },
    { AEF(s.apos.trDelta[2]), 0 },
    { AEF(r.clientMask[0]), 32 },
    { AEF(r.clientMask[1]), 32 },
    { AEF(s.leanf), 0 },
    { AEF(s.apos.trDelta[1]), 0 },
    { AEF(s.loopSound), 8 },
    { AEF(s.attackerEntityNum), 10 },
    { AEF(s.iHeadIcon), 4 },
    { AEF(s.iHeadIconTeam), 2 },
    { AEF(s.apos.trDuration), 32 },
    { AEF(s.time), 32 },
    { AEF(s.time2), 32 },
    { AEF(s.origin2[0]), 0 },
    { AEF(s.origin2[1]), 0 },
    { AEF(s.origin2[2]), 0 },
    { AEF(s.angles2[2]), 0 },
    { AEF(s.constantLight), 32 },
    { AEF(s.dmgFlags), 32 },
};

static const NetField entityStateFields[59] = {
    { ESF(pos.trTime), 32 },
    { ESF(pos.trBase[1]), 0 },
    { ESF(pos.trBase[0]), 0 },
    { ESF(pos.trDelta[0]), 0 },
    { ESF(pos.trDelta[1]), 0 },
    { ESF(angles2[1]), 0 },
    { ESF(apos.trBase[1]), -100 },
    { ESF(pos.trDelta[2]), 0 },
    { ESF(pos.trBase[2]), 0 },
    { ESF(apos.trBase[0]), -100 },
    { ESF(eventSequence), 8 },
    { ESF(legsAnim), 10 },
    { ESF(eType), 8 },
    { ESF(eFlags), 24 },
    { ESF(otherEntityNum), 10 },
    { ESF(surfType), 8 },
    { ESF(eventParm), 8 },
    { ESF(scale), 8 },
    { ESF(clientNum), 8 },
    { ESF(torsoAnim), 10 },
    { ESF(groundEntityNum), 10 },
    { ESF(events[0]), 8 },
    { ESF(events[1]), 8 },
    { ESF(events[2]), 8 },
    { ESF(angles2[0]), 0 },
    { ESF(events[3]), 8 },
    { ESF(apos.trBase[2]), -100 },
    { ESF(pos.trType), 8 },
    { ESF(fWaistPitch), 0 },
    { ESF(fTorsoPitch), 0 },
    { ESF(apos.trTime), 32 },
    { ESF(solid), 24 },
    { ESF(apos.trDelta[0]), 0 },
    { ESF(apos.trType), 8 },
    { ESF(animMovetype), 4 },
    { ESF(fTorsoHeight), 0 },
    { ESF(apos.trDelta[2]), 0 },
    { ESF(weapon), 7 },
    { ESF(index), 10 },
    { ESF(apos.trDelta[1]), 0 },
    { ESF(eventParms[0]), 8 },
    { ESF(eventParms[1]), 8 },
    { ESF(eventParms[2]), 8 },
    { ESF(eventParms[3]), 8 },
    { ESF(iHeadIcon), 4 },
    { ESF(pos.trDuration), 32 },
    { ESF(iHeadIconTeam), 2 },
    { ESF(time), 32 },
    { ESF(leanf), 0 },
    { ESF(attackerEntityNum), 10 },
    { ESF(time2), 32 },
    { ESF(loopSound), 8 },
    { ESF(origin2[2]), 0 },
    { ESF(origin2[0]), 0 },
    { ESF(origin2[1]), 0 },
    { ESF(angles2[2]), 0 },
    { ESF(constantLight), 32 },
    { ESF(apos.trDuration), 32 },
    { ESF(dmgFlags), 32 },
};

void MSG_BeginReading(msg_t *msg);
void MSG_WriteBits(msg_t *msg, int value, int bits);
void MSG_WriteBit0(msg_t *msg);
void MSG_WriteBit1(msg_t *msg);
int MSG_ReadBits(msg_t *msg, int bits);
int MSG_ReadBit(msg_t *msg);
int MSG_WriteBitsCompress(byte *from, byte *to, int size);
int MSG_ReadBitsCompress(byte *from, byte *to, int size);
void MSG_WriteByte(msg_t *msg, int c);
void MSG_WriteData(msg_t *buf, const void *data, int length);
int MSG_ReadByte(msg_t *msg);
int MSG_ReadShort(msg_t *msg);
int MSG_ReadLong(msg_t *msg);
void MSG_ReadData(msg_t *msg, void *data, int len);
void MSG_WriteReliableCommandToBuffer(const char *pszCommand, char *pszBuffer, int iBufferSize);
void MSG_SetDefaultUserCmd(playerState_t *ps, usercmd_t *cmd);
char *MSG_ReadString(msg_t *msg);
char *MSG_ReadBigString(msg_t *msg);
char *MSG_ReadStringLine(msg_t *msg);
void MSG_WriteShort(msg_t *msg, int c);
void MSG_WriteLong(msg_t *msg, int c);
void MSG_Init(msg_t *buf, byte *data, int length);
void MSG_WriteString(msg_t *sb, const char *s);
void MSG_WriteBigString(msg_t *sb, const char *s);
void MSG_ReadDeltaField(msg_t *msg, byte *from, byte *to, const NetField *field, qboolean print);
static qboolean __attribute_regparm__(3) MSG_ReadDeltaStruct(msg_t *msg, byte *from, byte *to, unsigned int number, int numFields, int indexBits, const NetField *stateFields);
qboolean MSG_ReadDeltaClient(msg_t *msg, clientState_t *from, clientState_t *to, int number);
qboolean MSG_ReadDeltaArchivedEntity(msg_t *msg, archivedEntity_t *from, archivedEntity_t *to, int number);
qboolean MSG_ReadDeltaEntity(msg_t *msg, entityState_t *from, entityState_t *to, int number);
static void __attribute_regparm__(3) MSG_ReadDeltaHudElems(msg_t *msg, hudelem_t *from, hudelem_t *to, int count);
void MSG_ReadDeltaPlayerstate(msg_t *msg, playerState_t *from, playerState_t *to);
void MSG_ReadDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from, usercmd_t *to);
void MSG_WriteDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from, usercmd_t *to);
static void __attribute_regparm__(3) MSG_WriteDeltaField(msg_t *msg, byte *from, byte *to, const NetField *field);
static void __attribute_regparm__(3) MSG_WriteDeltaHudElems(msg_t *msg, hudelem_t *from, hudelem_t *to, int count);
static void __attribute_regparm__(3) MSG_WriteDeltaStruct(msg_t *msg, byte *from, byte *to, qboolean force, int numFields, int indexBits, const NetField *stateFields, qboolean bChangeBit);
void MSG_WriteDeltaClient(msg_t *msg, clientState_s *from, clientState_s *to, qboolean force);
void MSG_WriteDeltaArchivedEntity(msg_t *msg, archivedEntity_t *from, archivedEntity_t *to, qboolean force);
void MSG_WriteDeltaEntity(msg_t *msg, entityState_s *from, entityState_s *to, qboolean force);
void MSG_WriteDeltaPlayerstate(msg_t *msg, playerState_s *from, playerState_s *to);

void MSG_BeginReading(msg_t *msg)
{
    msg->overflowed = 0;
    msg->readcount = 0;
    msg->bit = 0;
}

static inline __attribute__((always_inline)) void MSG_WriteBits_core(msg_t *msg, int value, int bits)
{
    int bit, take;

    if (__builtin_expect(msg->maxsize - msg->cursize <= 3, 0)) {
        msg->overflowed = 1;
        return;
    }

    while (bits > 0) {
        bit = msg->bit & 7;
        if (bit == 0) {
            msg->bit = msg->cursize * 8;
            msg->data[msg->cursize] = 0;
            msg->cursize++;
        }
        /* Bit fields share a byte, but byte/string writes can append elsewhere
         * before that byte is full. Keep the original cursor rules and consume
         * a whole remaining byte fragment instead of branching for every bit. */
        take = 8 - bit;
        if (take > bits) take = bits;
        msg->data[msg->bit >> 3] |= ((unsigned int)value & ((1u << take) - 1u)) << bit;
        msg->bit += take;
        value >>= take;
        bits -= take;
    }
}

static inline __attribute__((always_inline)) void MSG_WriteBit0_core(msg_t *msg)
{
    if (msg->cursize >= msg->maxsize) {
        msg->overflowed = 1;
        return;
    }
    if (!(msg->bit & 7)) {
        msg->bit = msg->cursize * 8;
        msg->data[msg->cursize] = 0;
        msg->cursize++;
    }
    msg->bit++;
}

static inline __attribute__((always_inline)) void MSG_WriteBit1_core(msg_t *msg)
{
    int bit;

    if (msg->cursize >= msg->maxsize) {
        msg->overflowed = 1;
        return;
    }

    bit = msg->bit & 7;
    if (bit == 0) {
        msg->bit = msg->cursize * 8;
        msg->data[msg->cursize] = 0;
        msg->cursize++;
    }
    msg->data[msg->bit >> 3] |= (1 << bit);
    msg->bit++;
}

void MSG_WriteBits(msg_t *msg, int value, int bits)
{
    MSG_WriteBits_core(msg, value, bits);
}

void MSG_WriteBit0(msg_t *msg)
{

    if (msg->cursize >= msg->maxsize) {
        msg->overflowed = 1;
        return;
    }
    if (!(msg->bit & 7)) {
        msg->bit = msg->cursize * 8;
        msg->data[msg->cursize] = 0;
        msg->cursize++;
    }
    msg->bit++;
}

void MSG_WriteBit1(msg_t *msg)
{
    int bit;

    if (msg->cursize >= msg->maxsize) {
        msg->overflowed = 1;
        return;
    }

    bit = msg->bit & 7;
    if (bit == 0) {
        msg->bit = msg->cursize * 8;
        msg->data[msg->cursize] = 0;
        msg->cursize++;
    }
    msg->data[msg->bit >> 3] |= (1 << bit);
    msg->bit++;
}

static inline __attribute__((always_inline)) int MSG_ReadBits_core(msg_t *msg, int bits)
{
    unsigned int value = 0;
    int shift = 0;

    while (bits > 0) {
        int bit = msg->bit;
        int rem = bit & 7;
        int take;
        if (rem == 0) {
            if (msg->readcount >= msg->cursize) {
                msg->overflowed = 1;
                return -1;
            }
            bit = msg->readcount * 8;
            msg->bit = bit;
            msg->readcount++;
        }
        take = 8 - rem;
        if (take > bits) take = bits;
        value |= ((unsigned int)(msg->data[bit >> 3] >> rem) & ((1u << take) - 1u)) << shift;
        msg->bit = bit + take;
        shift += take;
        bits -= take;
    }
    return (int)value;
}

int MSG_ReadBits(msg_t *msg, int bits)
{
    return MSG_ReadBits_core(msg, bits);
}

static inline __attribute__((always_inline)) int MSG_ReadBit_core(msg_t *msg)
{
    int bit = msg->bit;
    int rem = bit & 7;
    int value;

    if (rem == 0) {
        if (msg->readcount >= msg->cursize) {
            msg->overflowed = 1;
            return -1;
        }
        bit = msg->readcount * 8;
        msg->bit = bit;
        msg->readcount++;
    }
    value = (msg->data[bit >> 3] >> rem) & 1;
    msg->bit = bit + 1;
    return value;
}

int MSG_ReadBit(msg_t *msg)
{
    return MSG_ReadBit_core(msg);
}

extern void Huff_offsetTransmit(void *huff, int ch, byte *fout, int *offset);
extern void Huff_offsetReceive(void *node, int *ch, byte *fin, int *offset);
static unsigned int msgHuffCodes[256];
static byte msgHuffLengths[256];
static qboolean msgHuffCodesReady;
#define MSG_HUFF_DECODE_BITS 11
static unsigned short msgHuffDecode[1 << MSG_HUFF_DECODE_BITS];
static qboolean msgHuffDecodeReady;

static void MSG_BuildHuffmanCodes(void)
{
    int i;
    /* The seeded tree stays fixed. Derive the wire codes from the original
     * encoder, including its LSB-first bit order, rather than duplicating it. */
    for (i = 0; i < 256; ++i) {
        byte encoded[32] = {0};
        int bits = 0;
        Huff_offsetTransmit(&msgHuff, i, encoded, &bits);
        if (bits < 1 || bits > 24)
            return;
        msgHuffCodes[i] = (unsigned int)encoded[0]
                       | ((unsigned int)encoded[1] << 8)
                       | ((unsigned int)encoded[2] << 16);
        msgHuffLengths[i] = (byte)bits;
    }
    msgHuffCodesReady = 1;

    /* The current retail seed has codes of 2..11 bits. A 4 KiB prefix table
     * replaces tree traversal on both client snapshots and server commands.
     * Longer future codes or the unused NYT prefix keep the original decoder. */
    msgHuffDecodeReady = 0;
    memset(msgHuffDecode, 0, sizeof(msgHuffDecode));
    for (i = 0; i < 256; ++i)
        if (msgHuffLengths[i] > MSG_HUFF_DECODE_BITS) return;
    for (i = 0; i < 256; ++i) {
        unsigned int prefix;
        unsigned int step = 1u << msgHuffLengths[i];
        for (prefix = msgHuffCodes[i]; prefix < (1u << MSG_HUFF_DECODE_BITS); prefix += step)
            msgHuffDecode[prefix] = (unsigned short)((msgHuffLengths[i] << 8) | i);
    }
    msgHuffDecodeReady = 1;
}

int MSG_WriteBitsCompress(byte *from, byte *to, int size)
{
    int bit = 0;
    byte *p;
    byte *end;

    if (msgHuffCodesReady) {
        unsigned int pending = 0;
        int pendingBits = 0;
        byte *output = to;
        int i;
        for (i = 0; i < size; ++i) {
            unsigned int symbol = from[i];
            /* At most seven pending bits plus a 24-bit code fit in uint32. */
            pending |= msgHuffCodes[symbol] << pendingBits;
            pendingBits += msgHuffLengths[symbol];
            while (pendingBits >= 8) {
                *output++ = (byte)pending;
                pending >>= 8;
                pendingBits -= 8;
            }
        }
        if (pendingBits)
            *output++ = (byte)pending;
        return (int)(output - to);
    }

    if (__builtin_expect(size != 0, 0)) {
        p = from;
        end = from + size;
        do {
            Huff_offsetTransmit(&msgHuff, *p, to, &bit);
            p++;
        } while (p != end);
    }

    return (bit + 7) >> 3;
}

int MSG_ReadBitsCompress(byte *from, byte *to, int size)
{
    int bits = size * 8;
    int get;
    int bit = 0;
    byte *data = to;

    if (msgHuffDecodeReady) {
        const byte *input = from;
        const byte *end = from + size;
        unsigned int pending = 0;
        int pendingBits = 0;
        for (;;) {
            unsigned short entry;
            /* Refill two bytes together; at most ten bits remain beforehand.
             * All lookup reads stay within the packet. The original tree
             * decoder handles its partial final code and padding below. */
            if (pendingBits < MSG_HUFF_DECODE_BITS) {
                if (end - input < 2) break;
                pending |= ((unsigned int)input[0] | ((unsigned int)input[1] << 8)) << pendingBits;
                input += 2;
                pendingBits += 16;
            }
            entry = msgHuffDecode[pending & ((1u << MSG_HUFF_DECODE_BITS) - 1u)];
            if (!entry) break;
            *data++ = (byte)entry;
            pending >>= entry >> 8;
            pendingBits -= entry >> 8;
        }
        bit = (int)(input - from) * 8 - pendingBits;
    }

    if (__builtin_expect(bits > bit, 0)) {
        do {

            Huff_offsetReceive((void *)msgHuff.decompressor.tree, &get, from, &bit);
            *data++ = (byte)get;
        } while (bits > bit);
    }
    return (int)(data - to);
}

static inline __attribute__((always_inline)) void MSG_WriteByte_core(msg_t *msg, int c)
{

    if (__builtin_expect(msg->cursize >= msg->maxsize, 0)) {
        msg->overflowed = 1;
        return;
    }
    msg->data[msg->cursize] = (byte)c;
    msg->cursize++;
}

void MSG_WriteByte(msg_t *msg, int c)
{
    MSG_WriteByte_core(msg, c);
}

static inline __attribute__((always_inline)) void MSG_WriteData_core(msg_t *buf, const void *data, int length)
{
    int newsize = buf->cursize + length;
    if (newsize > buf->maxsize) {
        buf->overflowed = 1;
        return;
    }
    memcpy(buf->data + buf->cursize, data, length);
    buf->cursize = newsize;
}

void MSG_WriteData(msg_t *buf, const void *data, int length)
{

    int newsize = buf->cursize + length;
    if (newsize > buf->maxsize) {
        buf->overflowed = 1;
        return;
    }
    memcpy(buf->data + buf->cursize, data, length);
    buf->cursize = newsize;
}

static inline __attribute__((always_inline)) int MSG_ReadByte_core(msg_t *msg)
{
    int readcount = msg->readcount;
    int result;

    if (__builtin_expect(readcount >= msg->cursize, 0)) {
        msg->overflowed = 1;
        return -1;
    }

    result = (unsigned char)msg->data[readcount];
    msg->readcount = readcount + 1;
    return result;
}

int MSG_ReadByte(msg_t *msg)
{
    return MSG_ReadByte_core(msg);
}

static inline __attribute__((always_inline)) int MSG_ReadShort_core(msg_t *msg)
{
    int readcount = msg->readcount;
    int next = readcount + 2;
    int result;
    if (__builtin_expect(next > msg->cursize, 0)) {
        msg->overflowed = 1;
        return -1;
    }

    result = *(short *)(msg->data + readcount);
    msg->readcount = next;
    return result;
}

int MSG_ReadShort(msg_t *msg)
{
    return MSG_ReadShort_core(msg);
}

static inline __attribute__((always_inline)) int MSG_ReadLong_core(msg_t *msg)
{
    int readcount = msg->readcount;
    int next = readcount + 4;
    int result;
    if (__builtin_expect(next > msg->cursize, 0)) {
        msg->overflowed = 1;
        return -1;
    }
    result = *(int *)(msg->data + readcount);
    msg->readcount = next;
    return result;
}

int MSG_ReadLong(msg_t *msg)
{
    return MSG_ReadLong_core(msg);
}

void MSG_ReadData(msg_t *msg, void *data, int len)
{
    int newcount = msg->readcount + len;
    if (__builtin_expect(newcount > msg->cursize, 0)) {
        msg->overflowed = 1;
        memset(data, -1, len);
        return;
    }
    memcpy(data, msg->data + msg->readcount, len);
    msg->readcount = newcount;
}

void MSG_WriteReliableCommandToBuffer(const char *pszCommand, char *pszBuffer, int iBufferSize)
{
    int len = strlen(pszCommand);
    int i;

    if (len >= iBufferSize) {
        Com_Printf((const char *)"WARNING: Reliable command is too long (%i/%i) and will be truncated: '%s'\n", len, iBufferSize, pszCommand);
    }

    if (len == 0) {
        Com_Printf((const char *)"WARNING: Empty reliable command\n");
    }

    if (iBufferSize > 0) {
        for (i = 0; i < iBufferSize; i++) {
            char ch = pszCommand[i];
            if (!ch) {
                pszBuffer[i] = '\0';
                return;
            }
            ch = (char)I_CleanChar((int)(signed char)ch);
            if (ch == '%')
                ch = '.';
            pszBuffer[i] = ch;
        }
    }

    pszBuffer[iBufferSize - 1] = '\0';
}

void MSG_SetDefaultUserCmd(playerState_t *ps, usercmd_t *cmd)
{
    int i;
    const float angleScale = 182.04444885253906f;

    memset(cmd, 0, sizeof(*cmd));

    cmd->weapon = (byte)ps->weapon;
    cmd->offHandIndex = (byte)ps->offHandIndex;

    for (i = 0; i < 2; ++i) {
        cmd->angles[i] = (unsigned short)((int)(ps->viewangles[i] * angleScale) - ps->delta_angles[i]);
    }

    if (ps->pm_flags & 0x800000) {
        if (ps->eFlags & 8) {
            cmd->buttons |= 0x100;
        } else if (ps->eFlags & 4) {
            cmd->buttons |= 0x200;
        }

        if (ps->leanf > 0.0f) {
            cmd->buttons |= 0x80;
        } else if (ps->leanf < 0.0f) {
            cmd->buttons |= 0x40;
        }

        if (ps->fWeaponPosFrac != 0.0f) {
            cmd->buttons |= 0x1000;
        }
    }
}

char *MSG_ReadString(msg_t *msg)
{
    unsigned int l = 0;

    while (1) {

        int c = (signed char)MSG_ReadByte_core(msg);
        if (__builtin_expect(c == -1, 0)) {
            c = 0;
        }

        if (l <= 1023) {
            string_00f0e860[l] = (char)I_CleanChar((signed char)c);
        }

        if (!c) {
            break;
        }

        l++;
    }

    string_00f0e860[1023] = '\0';
    return string_00f0e860;
}

char *MSG_ReadBigString(msg_t *msg)
{
    unsigned int l = 0;

    while (1) {

        int c = (signed char)MSG_ReadByte_core(msg);
        if (c == '%') {
            c = '.';
        }
        if (__builtin_expect(c == -1, 0)) {
            c = 0;
        }

        if (l <= 8191) {
            string_00f0ec60[l] = (char)I_CleanChar((signed char)c);
        }

        if (!c) {
            break;
        }

        l++;
    }

    string_00f0ec60[8191] = '\0';
    return string_00f0ec60;
}

char *MSG_ReadStringLine(msg_t *msg)
{
    unsigned int l = 0;

    while (1) {

        int c = (signed char)MSG_ReadByte_core(msg);
        if (c == '%') {
            c = '.';
        }
        if (c == '\n' || c == -1) {
            c = 0;
        }

        if (l <= 1023) {
            string_00f10c60[l] = (char)I_CleanChar((signed char)c);
        }

        if (!c) {
            break;
        }

        l++;
    }

    string_00f10c60[1023] = '\0';
    return string_00f10c60;
}

static inline __attribute__((always_inline)) void MSG_WriteShort_core(msg_t *msg, int c)
{
    int cursize = msg->cursize;
    int newsize = cursize + 2;
    if (__builtin_expect(newsize > msg->maxsize, 0)) {
        msg->overflowed = 1;
        return;
    }
    *(short *)(msg->data + cursize) = (short)c;
    msg->cursize = newsize;
}

void MSG_WriteShort(msg_t *msg, int c)
{
    int cursize = msg->cursize;
    int newsize = cursize + 2;
    if (__builtin_expect(newsize > msg->maxsize, 0)) {
        msg->overflowed = 1;
        return;
    }
    *(short *)(msg->data + cursize) = (short)c;
    msg->cursize = newsize;
}

static inline __attribute__((always_inline)) void MSG_WriteLong_core(msg_t *msg, int c)
{
    int cursize = msg->cursize;
    int newsize = cursize + 4;
    if (__builtin_expect(newsize > msg->maxsize, 0)) {
        msg->overflowed = 1;
        return;
    }
    *(int *)(msg->data + cursize) = c;
    msg->cursize = newsize;
}

void MSG_WriteLong(msg_t *msg, int c)
{
    int cursize = msg->cursize;
    int newsize = cursize + 4;
    if (__builtin_expect(newsize > msg->maxsize, 0)) {
        msg->overflowed = 1;
        return;
    }
    *(int *)(msg->data + cursize) = c;
    msg->cursize = newsize;
}

extern void Huff_Init(void *huff);
extern void Huff_addRef(void *huff, int ch);
void MSG_Init(msg_t *buf, byte *data, int length)
{
    int i, j;

    if (!msgInit) {
        msgInit = 1;
        Huff_Init(&msgHuff);
        for (i = 0; i < 256; i++) {
            for (j = 0; j < msg_hData[i]; j++) {
                Huff_addRef(&msgHuff, (byte)i);

                Huff_addRef(&msgHuff.decompressor, (byte)i);
            }
        }
        MSG_BuildHuffmanCodes();
    }

    memset(buf, 0, sizeof(msg_t));
    buf->data = data;
    buf->maxsize = length;
}

void MSG_WriteString(msg_t *sb, const char *s)
{
    char string[1024];
    int l = strlen(s);
    int i;

    if (l > 1023) {
        Com_Printf("MSG_WriteString: MAX_STRING_CHARS");
        MSG_WriteByte_core(sb, 0);
        return;
    }

    for (i = 0; i < l; i++) {
        string[i] = (char)I_CleanChar((signed char)s[i]);
    }
    string[i] = '\0';

    MSG_WriteData_core(sb, string, l + 1);
}

void MSG_WriteBigString(msg_t *sb, const char *s)
{
    char string[8192];
    int l = strlen(s);
    int i;


    if (l > 8191) {
        Com_Printf("MSG_WriteString: BIG_INFO_STRING");
        MSG_WriteByte(sb, 0);
        return;
    }

    I_strncpyz(string, s, sizeof(string));
    for (i = 0; i < l; i++) {
        string[i] = (char)I_CleanChar((signed char)string[i]);
    }

    MSG_WriteData_core(sb, string, l + 1);
}

static inline __attribute__((always_inline)) int MSG_ReadValue(msg_t *msg, int bits)
{
    qboolean sign = bits < 0;
    int partialBits, shift, value;

    if (bits == -98) {
        bits = 32;
        sign = 0;
    } else if (sign) {
        bits = -bits;
    }

    partialBits = bits & 7;
    value = partialBits ? MSG_ReadBits_core(msg, partialBits) : 0;
    for (shift = partialBits; shift < bits; shift += 8)
        value |= MSG_ReadByte_core(msg) << shift;

    if (sign && (value & (1 << (bits - 1))))
        value |= -1 << bits;

    return value;
}

static inline __attribute__((always_inline)) void MSG_WriteValue(msg_t *msg, int value, int bits)
{
    int partialBits, shift;

    if (bits == -98)
        bits = 32;
    else if (bits < 0)
        bits = -bits;

    partialBits = bits & 7;
    if (partialBits) {
        MSG_WriteBits_core(msg, value, partialBits);
        value >>= partialBits;
    }
    for (shift = partialBits; shift < bits; shift += 8) {
        MSG_WriteByte_core(msg, value);
        value >>= 8;
    }
}

void MSG_ReadDeltaField(msg_t *msg, byte *from, byte *to, const NetField *field, qboolean print)
{
    int *fromF;
    int *toF;
    int bits;
    int value;
    int trunc;

    fromF = (int *)(from + field->offset);
    toF = (int *)(to + field->offset);

    if (!MSG_ReadBit_core(msg)) {
        *toF = *fromF;
        return;
    }

    bits = field->bits;

    if (bits == 0) {
        if (!MSG_ReadBit_core(msg)) {
            *toF = 0;
            return;
        }

        if (MSG_ReadBit_core(msg)) {
            *toF = MSG_ReadLong_core(msg);
            if (print)
                Com_Printf("%s:%f ", field->name, *(float *)toF);
            return;
        }

        trunc = MSG_ReadBits_core(msg, 5);
        value = trunc + (MSG_ReadByte_core(msg) << 5) - 4096;
        *(float *)toF = (float)value;
        if (print)
            Com_Printf("%s:%i ", field->name, value);
        return;
    }

    if (bits == -99) {
        if (!MSG_ReadBit_core(msg)) {
            *toF = 0;
            return;
        }

        if (MSG_ReadBit_core(msg)) {
            *toF = MSG_ReadLong_core(msg);
            if (print)
                Com_Printf("%s:%f ", field->name, *(float *)toF);
            return;
        }

        trunc = MSG_ReadBits_core(msg, 2);
        value = trunc + (MSG_ReadByte_core(msg) << 2) - 512;
        *(float *)toF = (float)value;
        if (print)
            Com_Printf("%s:%i ", field->name, value);
        return;
    }

    if (bits == -100) {
        if (!MSG_ReadBit_core(msg)) {
            *toF = 0;
            return;
        }

        *(float *)toF = (float)MSG_ReadShort_core(msg) * 0.0054931640625f;
        return;
    }

    if (!MSG_ReadBit_core(msg)) {
        *toF = 0;
        return;
    }

    value = MSG_ReadValue(msg, bits);
    *toF = value;
    if (print)
        Com_Printf("%s:%i ", field->name, value);
}

static qboolean __attribute_regparm__(3)
    MSG_ReadDeltaStruct(msg_t *msg, byte *from, byte *to, unsigned int number, int numFields, int indexBits, const NetField *stateFields)
{
    int lc;
    int i;
    qboolean print;
    const dvar_t *cl_shownet;

    (void)indexBits;

    if (MSG_ReadBit_core(msg)) {
        cl_shownet = *(const dvar_t **)imp_cl_shownet;
        if (cl_shownet && (cl_shownet->current.integer > 1 || cl_shownet->current.integer == -1))
            Com_Printf("%3i: #%-3i remove\n", msg->readcount, number);
        return 1;
    }

    if (!MSG_ReadBit_core(msg)) {
        memcpy(to, from, (numFields + 1) * sizeof(int));
        return 0;
    }

    lc = MSG_ReadByte_core(msg);
    if (lc > numFields) {
        msg->overflowed = 1;
        return 0;
    }

    cl_shownet = *(const dvar_t **)imp_cl_shownet;
    print = (cl_shownet && (cl_shownet->current.integer > 1 || cl_shownet->current.integer == -1));
    if (print)
        Com_Printf("%3i: #%-3i ", msg->readcount, *(int *)to);

    *(int *)to = number;

    for (i = 0; i < lc; ++i)
        MSG_ReadDeltaField(msg, from, to, &stateFields[i], print);

    for (i = lc; i < numFields; ++i)
        *(int *)(to + stateFields[i].offset) = *(int *)(from + stateFields[i].offset);

    return 0;
}

qboolean MSG_ReadDeltaClient(msg_t *msg, clientState_t *from, clientState_t *to, int number)
{
    clientState_t dummy;

    if (!from) {
        memset(&dummy, 0, sizeof(dummy));
        from = &dummy;
    }

    return MSG_ReadDeltaStruct(msg, (byte *)from, (byte *)to, number, 0x16, 6, clientStateFields);
}

qboolean MSG_ReadDeltaArchivedEntity(msg_t *msg, archivedEntity_t *from, archivedEntity_t *to, int number)
{
    return MSG_ReadDeltaStruct(msg, (byte *)from, (byte *)to, number, 0x44, 0xa, archivedEntityFields);
}

qboolean MSG_ReadDeltaEntity(msg_t *msg, entityState_t *from, entityState_t *to, int number)
{
    return MSG_ReadDeltaStruct(msg, (byte *)from, (byte *)to, number, 0x3b, 0xa, entityStateFields);
}

static void __attribute_regparm__(3)
    MSG_ReadDeltaHudElems(msg_t *msg, hudelem_t *from, hudelem_t *to, int count)
{
    int inuse;
    int i;
    int j;
    int lc;

    inuse = MSG_ReadBits_core(msg, 5);

    for (i = 0; i < inuse; ++i) {
        lc = MSG_ReadBits_core(msg, 5);

        if (lc >= 0) {
            for (j = 0; j <= lc; ++j) {
                MSG_ReadDeltaField(msg, (byte *)&from[i], (byte *)&to[i], &hudElemFields[j], 0);
            }
        } else {
            j = 0;
        }

        for (; j < 32; ++j) {
            *(int *)((byte *)&to[i] + hudElemFields[j].offset) =
                *(int *)((byte *)&from[i] + hudElemFields[j].offset);
        }
    }

    memset(&to[inuse], 0, (count - inuse) * sizeof(*to));
}

static inline __attribute__((always_inline)) qboolean MSG_PlayerstateDeltaPrint(void)
{
    const dvar_t *cl_shownet;

    cl_shownet = *(const dvar_t **)imp_cl_shownet;
    return cl_shownet && (cl_shownet->current.integer > 1 || cl_shownet->current.integer == -2);
}

static inline __attribute__((always_inline)) qboolean MSG_PlayerstateExtraPrint(void)
{
    const dvar_t *cl_shownet;

    cl_shownet = *(const dvar_t **)imp_cl_shownet;
    return cl_shownet && cl_shownet->current.integer == 4;
}

static inline __attribute__((always_inline)) void MSG_ReadDeltaPlayerstateField(msg_t *msg, byte *from, byte *to, const NetField *field, qboolean print)
{
    int *fromF;
    int *toF;
    int bits;
    int value;
    int trunc;

    fromF = (int *)(from + field->offset);
    toF = (int *)(to + field->offset);

    if (!MSG_ReadBit_core(msg)) {
        *toF = *fromF;
        return;
    }

    bits = field->bits;

    if (bits == 0) {
        if (MSG_ReadBit_core(msg)) {
            *toF = MSG_ReadLong_core(msg);
            if (print) {
                Com_Printf("%s:%f ", field->name, *(float *)toF);
            }
            return;
        }

        trunc = MSG_ReadBits_core(msg, 5);
        value = (MSG_ReadByte_core(msg) << 5) + trunc - 4096;
        *(float *)toF = (float)value;
        if (print) {
            Com_Printf("%s:%i ", field->name, value);
        }
        return;
    }

    if (bits == -100) {
        if (!MSG_ReadBit_core(msg)) {
            *toF = 0;
            return;
        }

        *(float *)toF = (float)MSG_ReadShort_core(msg) * 0.0054931640625f;
        return;
    }

    {
        qboolean sign;
        int partialBits;
        int shift;

        sign = bits < 0;
        if (bits == -98) {
            bits = 32;
            sign = 0;
        } else if (sign) {
            bits = -bits;
        }

        partialBits = bits & 7;
        value = partialBits ? MSG_ReadBits_core(msg, partialBits) : 0;
        for (shift = partialBits; shift < bits; shift += 8) {
            value |= MSG_ReadByte_core(msg) << shift;
        }

        if (sign && (value & (1 << (bits - 1)))) {
            value |= -1 << bits;
        }
    }

    *toF = value;
    if (print) {
        Com_Printf("%s:%i ", field->name, value);
    }
}

static inline __attribute__((always_inline)) void MSG_ReadDeltaPlayerstateShortArray(msg_t *msg, int *values, int groupCount, const char *printName)
{
    int group;
    int bit;
    int mask;

    if (!MSG_ReadBit_core(msg)) {
        return;
    }

    for (group = 0; group < groupCount; ++group) {
        if (MSG_ReadBit_core(msg)) {
            if (printName && MSG_PlayerstateExtraPrint()) {
                Com_Printf("%s ", printName);
            }

            mask = MSG_ReadShort_core(msg);
            for (bit = 0; bit < 16; ++bit) {
                if (mask & (1 << bit)) {
                    values[group * 16 + bit] = MSG_ReadShort_core(msg);
                }
            }
        }
    }
}

static inline __attribute__((always_inline)) void MSG_ReadDeltaPlayerstateShortArrayNoLead(msg_t *msg, int *values, int groupCount, const char *printName)
{
    int group;
    int bit;
    int mask;

    for (group = 0; group < groupCount; ++group) {
        if (MSG_ReadBit_core(msg)) {
            if (printName && MSG_PlayerstateExtraPrint()) {
                Com_Printf("%s ", printName);
            }

            mask = MSG_ReadShort_core(msg);
            for (bit = 0; bit < 16; ++bit) {
                if (mask & (1 << bit)) {
                    values[group * 16 + bit] = MSG_ReadShort_core(msg);
                }
            }
        }
    }
}

void MSG_ReadDeltaPlayerstate(msg_t *msg, playerState_t *from, playerState_t *to)
{
    playerState_t dummy;
    qboolean print;
    int lc;
    int i;
    int statbits;

    if (!from) {
        memset(&dummy, 0, sizeof(dummy));
        from = &dummy;
    }

    memcpy(to, from, sizeof(*to));

    print = MSG_PlayerstateDeltaPrint();
    if (print) {
        Com_Printf("%3i: playerstate ", msg->readcount);
    }

    lc = MSG_ReadByte_core(msg);
    if (lc > 0) {
        for (i = 0; i < lc; ++i) {
            MSG_ReadDeltaPlayerstateField(msg, (byte *)from, (byte *)to, &playerStateFields[i], print);
        }
    }

    if (MSG_ReadBit_core(msg)) {
        if (MSG_PlayerstateExtraPrint()) {
            Com_Printf("%s ", "PS_STATS");
        }

        statbits = MSG_ReadBits_core(msg, 6);
        if (statbits & 1) {
            to->stats[0] = MSG_ReadShort_core(msg);
        }
        if (statbits & 2) {
            to->stats[1] = MSG_ReadShort_core(msg);
        }
        if (statbits & 4) {
            to->stats[2] = MSG_ReadShort_core(msg);
        }
        if (statbits & 8) {
            to->stats[3] = MSG_ReadBits_core(msg, 6);
        }
        if (statbits & 0x10) {
            to->stats[4] = MSG_ReadShort_core(msg);
        }
        if (statbits & 0x20) {
            to->stats[5] = MSG_ReadByte_core(msg);
        }
    }

    MSG_ReadDeltaPlayerstateShortArray(msg, to->ammo, 4, "PS_AMMO");
    MSG_ReadDeltaPlayerstateShortArrayNoLead(msg, to->ammoclip, 4, "PS_AMMOCLIP");

    if (MSG_ReadBit_core(msg)) {
        for (i = 0; i < 16; ++i) {
            to->objective[i].state = (objectiveState_t)MSG_ReadBits_core(msg, 3);
            if (MSG_ReadBit_core(msg)) {
                int j;
                for (j = 0; j < 6; ++j) {
                    MSG_ReadDeltaField(msg, (byte *)&from->objective[i], (byte *)&to->objective[i], &objectiveFields[j], 0);
                }
            }
        }
    }

    if (MSG_ReadBit_core(msg)) {
        MSG_ReadDeltaHudElems(msg, from->hud.archival, to->hud.archival, 31);
        MSG_ReadDeltaHudElems(msg, from->hud.current, to->hud.current, 31);
    }
}

static int MSG_ReadDeltaKeyShort(msg_t *msg, int key, int oldV)
{
    if (MSG_ReadBit(msg)) {
        return (unsigned short)(short)(MSG_ReadShort(msg) ^ key);
    }
    return (unsigned short)oldV;
}

static int MSG_ReadDeltaKeyBits(msg_t *msg, int key, int oldV, int bits)
{
    if (MSG_ReadBit(msg)) {
        return MSG_ReadBits(msg, bits) ^ (key & kbitmask[bits]);
    }
    return oldV;
}

static inline __attribute__((always_inline)) void MSG_UsercmdDecodeHorMove(usercmd_t *to, int horMove)
{
    if (horMove & 1) {
        to->forwardmove = 127;
    } else if (horMove & 2) {
        to->forwardmove = -127;
    } else {
        to->forwardmove = 0;
    }

    if (horMove & 4) {
        to->rightmove = 127;
    } else if (horMove & 8) {
        to->rightmove = -127;
    } else {
        to->rightmove = 0;
    }
}

static int MSG_UsercmdReadMoveBits(signed char forwardmove, signed char rightmove)
{
    int bits = 0;

    if (forwardmove > 10) {
        bits = 1;
    } else if (forwardmove <= -11) {
        bits = 2;
    }

    if (rightmove > 10) {
        bits |= 4;
    } else if (rightmove <= -11) {
        bits |= 8;
    }

    return bits;
}

void MSG_ReadDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from, usercmd_t *to)
{
    int fieldKey;
    int hasExtra;
    int horFromMove;
    int horToMove;

    *to = *from;

    if (MSG_ReadBit(msg)) {
        to->serverTime = from->serverTime + MSG_ReadByte(msg);
    } else {
        to->serverTime = MSG_ReadLong(msg);
    }

    if (MSG_ReadBit(msg) == (key & kbitmask[1])) {
        return;
    }

    to->buttons &= ~1;

    if (MSG_ReadBit(msg) == (key & kbitmask[1])) {
        fieldKey = key ^ to->serverTime;
        hasExtra = 0;
    } else {
        fieldKey = key;
        hasExtra = 1;
    }

    to->buttons |= MSG_ReadBit(msg) ^ (fieldKey & kbitmask[1]);

    to->angles[0] = MSG_ReadDeltaKeyShort(msg, fieldKey, from->angles[0]);
    to->angles[1] = MSG_ReadDeltaKeyShort(msg, fieldKey, from->angles[1]);

    horFromMove = MSG_UsercmdReadMoveBits(from->forwardmove, from->rightmove);
    horToMove = MSG_ReadDeltaKeyBits(msg, fieldKey, horFromMove, 4);
    MSG_UsercmdDecodeHorMove(to, horToMove);

    if (hasExtra) {
        int serverKey = key ^ to->serverTime;

        to->angles[2] = MSG_ReadDeltaKeyShort(msg, serverKey, from->angles[2]);
        to->buttons = (to->buttons & 1) |
                      (MSG_ReadDeltaKeyBits(msg, serverKey, from->buttons >> 1, 18) << 1);
        to->weapon = (byte)MSG_ReadDeltaKeyBits(msg, serverKey, from->weapon, 7);
        to->offHandIndex = (byte)MSG_ReadDeltaKeyBits(msg, serverKey, from->offHandIndex, 7);
    }
}

static inline __attribute__((always_inline)) int MSG_UsercmdMoveBits(signed char forwardmove, signed char rightmove)
{
    int moveBits = 0;

    if (forwardmove > 10) {
        moveBits = 1;
    } else if (forwardmove <= -11) {
        moveBits = 2;
    }

    if (rightmove > 10) {
        moveBits |= 4;
    } else if (rightmove <= -11) {
        moveBits |= 8;
    }

    return moveBits;
}

static inline __attribute__((always_inline)) void MSG_WriteDeltaKeyShort(msg_t *msg, int key, int oldV, int newV)
{
    if ((unsigned short)oldV == (unsigned short)newV) {
        MSG_WriteBit0_core(msg);
    } else {
        MSG_WriteBit1_core(msg);
        MSG_WriteShort_core(msg, newV ^ key);
    }
}

static inline __attribute__((always_inline)) void MSG_WriteDeltaKeyBits(msg_t *msg, int key, int oldV, int newV, int bits)
{
    if (oldV == newV) {
        MSG_WriteBit0_core(msg);
    } else {
        MSG_WriteBit1_core(msg);
        MSG_WriteBits_core(msg, newV ^ key, bits);
    }
}

void MSG_WriteDeltaUsercmdKey(msg_t *msg, int key, usercmd_t *from, usercmd_t *to)
{
    int serverTimeDelta;
    int fromMoveBits;
    int toMoveBits;
    int fieldKey;

    serverTimeDelta = to->serverTime - from->serverTime;

    if ((unsigned int)serverTimeDelta <= 0xff) {
        MSG_WriteBit1_core(msg);
        MSG_WriteByte_core(msg, serverTimeDelta);
        fieldKey = key ^ to->serverTime;
    } else {
        MSG_WriteBit0_core(msg);
        MSG_WriteLong_core(msg, to->serverTime);
        fieldKey = key;
    }

    toMoveBits = MSG_UsercmdMoveBits(to->forwardmove, to->rightmove);
    fromMoveBits = MSG_UsercmdMoveBits(from->forwardmove, from->rightmove);

    if ((from->buttons >> 1) == (to->buttons >> 1) &&
        from->weapon == to->weapon &&
        from->offHandIndex == to->offHandIndex &&
        from->angles[2] == to->angles[2] &&
        from->angles[0] == to->angles[0] &&
        from->angles[1] == to->angles[1] &&
        ((from->buttons ^ to->buttons) & 1) == 0 &&
        fromMoveBits == toMoveBits) {
        MSG_WriteBits_core(msg, key, 1);
        return;
    }

    MSG_WriteBits_core(msg, key ^ 1, 1);

    if ((unsigned int)serverTimeDelta > 0xff) {
        MSG_WriteBits_core(msg, key ^ 1, 1);
    } else {
        MSG_WriteBits_core(msg, key, 1);
    }
    MSG_WriteBits_core(msg, fieldKey ^ to->buttons, 1);
    MSG_WriteDeltaKeyShort(msg, fieldKey, from->angles[0], to->angles[0]);
    MSG_WriteDeltaKeyShort(msg, fieldKey, from->angles[1], to->angles[1]);
    MSG_WriteDeltaKeyBits(msg, fieldKey, fromMoveBits, toMoveBits, 4);

    if ((unsigned int)serverTimeDelta > 0xff) {
        int serverTimeKey = key ^ to->serverTime;

        MSG_WriteDeltaKeyShort(msg, serverTimeKey, from->angles[2], to->angles[2]);
        MSG_WriteDeltaKeyBits(msg, serverTimeKey, from->buttons >> 1, to->buttons >> 1, 18);
        MSG_WriteDeltaKeyBits(msg, serverTimeKey, from->weapon, to->weapon, 7);
        MSG_WriteDeltaKeyBits(msg, serverTimeKey, from->offHandIndex, to->offHandIndex, 7);
    }
}

static void __attribute_regparm__(3)
    MSG_WriteDeltaField(msg_t *msg, byte *from, byte *to, const NetField *field)
{
    int *fromF;
    int *toF;
    int bits;
    int value;
    int trunc;

    fromF = (int *)(from + field->offset);
    toF = (int *)(to + field->offset);

    if (*fromF == *toF) {
        MSG_WriteBit0_core(msg);
        return;
    }

    MSG_WriteBit1_core(msg);
    bits = field->bits;

    if (bits == 0) {
        float f;

        f = *(float *)toF;
        trunc = (int)f;
        if (f == 0.0f) {
            MSG_WriteBit0_core(msg);
            return;
        }

        MSG_WriteBit1_core(msg);
        value = trunc + 4096;
        if (f == (float)trunc && value >= 0 && value <= 8191) {
            MSG_WriteBit0_core(msg);
            MSG_WriteBits_core(msg, value, 5);
            MSG_WriteByte_core(msg, value >> 5);
            return;
        }

        MSG_WriteBit1_core(msg);
        MSG_WriteLong_core(msg, *toF);
        return;
    }

    if (bits == -99) {
        float f;

        f = *(float *)toF;
        trunc = (int)f;
        if (f == 0.0f) {
            MSG_WriteBit0_core(msg);
            return;
        }

        MSG_WriteBit1_core(msg);
        value = trunc + 512;
        if (f == (float)trunc && value >= 0 && value <= 1023) {
            MSG_WriteBit0_core(msg);
            MSG_WriteBits_core(msg, value, 2);
            MSG_WriteByte_core(msg, value >> 2);
            return;
        }

        MSG_WriteBit1_core(msg);
        MSG_WriteLong_core(msg, *toF);
        return;
    }

    if (bits == -100) {
        if (!*toF) {
            MSG_WriteBit0_core(msg);
            return;
        }

        MSG_WriteBit1_core(msg);
        MSG_WriteShort_core(msg, (int)(*(float *)toF * 182.04444885253906f));
        return;
    }

    if (!*toF) {
        MSG_WriteBit0_core(msg);
        return;
    }

    MSG_WriteBit1_core(msg);
    MSG_WriteValue(msg, *toF, bits);
}

static void __attribute_regparm__(3)
    MSG_WriteDeltaHudElems(msg_t *msg, hudelem_t *from, hudelem_t *to, int count)
{
    typedef void(__attribute_regparm__(3) * WriteDeltaFieldFn)(msg_t *, byte *, byte *, const NetField *);
    WriteDeltaFieldFn writeDeltaField = (WriteDeltaFieldFn)MSG_WriteDeltaField;
    int inuse;
    int i;
    int j;
    int lc;

    for (inuse = 0; inuse < count && to[inuse].type; ++inuse) {
        ;
    }

    MSG_WriteBits(msg, inuse, 5);

    for (i = 0; i < inuse; ++i) {
        lc = 0;
        for (j = 0; j < 32; ++j) {
            const int ofs = hudElemFields[j].offset;
            if (*(int *)((byte *)&from[i] + ofs) != *(int *)((byte *)&to[i] + ofs)) {
                lc = j;
            }
        }

        MSG_WriteBits(msg, lc, 5);

        for (j = 0; j <= lc; ++j) {
            writeDeltaField(msg, (byte *)&from[i], (byte *)&to[i], &hudElemFields[j]);
        }
    }
}

/* Only the last changed field sets the transmitted prefix length. Searching
 * from the tail can stop as soon as it finds that field; the encoding below
 * still writes every field in the original order, with identical bit rules. */
static inline int MSG_LastChangedField(const byte *from, const byte *to,
                                     const NetField *fields, int count)
{
    while (count > 0) {
        int offset = fields[count - 1].offset;
        if (*(const int *)(from + offset) != *(const int *)(to + offset))
            break;
        --count;
    }
    return count;
}

static void __attribute_regparm__(3)
    MSG_WriteDeltaStruct(msg_t *msg, byte *from, byte *to, qboolean force, int numFields, int indexBits, const NetField *stateFields, qboolean bChangeBit)
{
    int i;
    int lc;

    if (!to) {
        if ((*(dvar_t **)imp_cl_shownet) && ((*(dvar_t **)imp_cl_shownet)->current.integer > 1 || (*(dvar_t **)imp_cl_shownet)->current.integer == -1)) {
            Com_Printf("W|%3i: #%-3i remove\n", msg->cursize, *(int *)from);
        }

        if (bChangeBit) {
            MSG_WriteBit1(msg);
        }

        MSG_WriteBits(msg, *(int *)from, indexBits);
        MSG_WriteBit1(msg);
        return;
    }

    lc = MSG_LastChangedField(from, to, stateFields, numFields);

    if (!lc) {
        if (!force) {
            return;
        }

        if (bChangeBit) {
            MSG_WriteBit1(msg);
        }

        MSG_WriteBits(msg, *(int *)to, indexBits);
        MSG_WriteBit0(msg);
        MSG_WriteBit0(msg);
        return;
    }

    if (bChangeBit) {
        MSG_WriteBit1(msg);
    }

    MSG_WriteBits(msg, *(int *)to, indexBits);
    MSG_WriteBit0(msg);
    MSG_WriteBit1(msg);
    MSG_WriteByte(msg, lc);

    for (i = 0; i < lc; ++i) {
        MSG_WriteDeltaField(msg, from, to, &stateFields[i]);
    }
}

void MSG_WriteDeltaClient(msg_t *msg, clientState_s *from, clientState_s *to, qboolean force)
{
    clientState_t dummy;

    if (!from) {
        memset(&dummy, 0, sizeof(dummy));
        from = &dummy;
    }

    MSG_WriteDeltaStruct(msg, (byte *)from, (byte *)to, force, 0x16, 6, clientStateFields, 1);
}

void MSG_WriteDeltaArchivedEntity(msg_t *msg, archivedEntity_t *from, archivedEntity_t *to, qboolean force)
{
    MSG_WriteDeltaStruct(msg, (byte *)from, (byte *)to, force, 0x44, 0xa, archivedEntityFields, 0);
}

void MSG_WriteDeltaEntity(msg_t *msg, entityState_s *from, entityState_s *to, qboolean force)
{
    MSG_WriteDeltaStruct(msg, (byte *)from, (byte *)to, force, 0x3b, 0xa, entityStateFields, 0);
}

static inline __attribute__((always_inline)) void MSG_WriteDeltaPlayerstateField(msg_t *msg, byte *from, byte *to, const NetField *field)
{
    int *fromF;
    int *toF;
    int bits;
    int value;
    int trunc;

    fromF = (int *)(from + field->offset);
    toF = (int *)(to + field->offset);

    if (*fromF == *toF) {
        MSG_WriteBit0_core(msg);
        return;
    }

    MSG_WriteBit1_core(msg);
    bits = field->bits;

    if (bits == 0) {
        float f;

        f = *(float *)toF;
        trunc = (int)f;
        value = trunc + 4096;
        if (f == (float)trunc && value >= 0 && value <= 8191) {
            MSG_WriteBit0_core(msg);
            MSG_WriteBits_core(msg, value, 5);
            MSG_WriteByte_core(msg, value >> 5);
            return;
        }

        MSG_WriteBit1_core(msg);
        MSG_WriteLong_core(msg, *toF);
        return;
    }

    if (bits == -100) {
        if (!*toF) {
            MSG_WriteBit0_core(msg);
            return;
        }

        MSG_WriteBit1_core(msg);
        MSG_WriteShort_core(msg, (int)(*(float *)toF * 182.04444885253906f));
        return;
    }

    value = *toF;
    if (bits == -98) {
        bits = 32;
    } else if (bits < 0) {
        bits = -bits;
    }

    {
        int partialBits = bits & 7;
        int remaining;

        if (partialBits) {
            MSG_WriteBits_core(msg, value, partialBits);
            value >>= partialBits;
        }

        for (remaining = bits - partialBits; remaining; remaining -= 8) {
            MSG_WriteByte_core(msg, value);
            value >>= 8;
        }
    }
}

static inline __attribute__((always_inline)) void MSG_WriteDeltaPlayerstateStats(msg_t *msg, playerState_t *from, playerState_t *to)
{
    int bits;

    bits = 0;
    if (from->stats[0] != to->stats[0]) {
        bits |= 1;
    }
    if (from->stats[1] != to->stats[1]) {
        bits |= 2;
    }
    if (from->stats[2] != to->stats[2]) {
        bits |= 4;
    }
    if (from->stats[3] != to->stats[3]) {
        bits |= 8;
    }
    if (from->stats[4] != to->stats[4]) {
        bits |= 0x10;
    }
    if (from->stats[5] != to->stats[5]) {
        bits |= 0x20;
    }

    if (!bits) {
        MSG_WriteBit0_core(msg);
        return;
    }

    MSG_WriteBit1_core(msg);
    MSG_WriteBits_core(msg, bits, 6);
    if (bits & 1) {
        MSG_WriteShort_core(msg, to->stats[0]);
    }
    if (bits & 2) {
        MSG_WriteShort_core(msg, to->stats[1]);
    }
    if (bits & 4) {
        MSG_WriteShort_core(msg, to->stats[2]);
    }
    if (bits & 8) {
        MSG_WriteBits_core(msg, to->stats[3], 6);
    }
    if (bits & 0x10) {
        MSG_WriteShort_core(msg, to->stats[4]);
    }
    if (bits & 0x20) {
        MSG_WriteByte_core(msg, to->stats[5]);
    }
}

static inline __attribute__((always_inline)) void MSG_WriteDeltaPlayerstateShortArray(msg_t *msg, int *from, int *to, int groupCount)
{
    int groupMasks[4];
    int anyChanged;
    int group;
    int bit;
    int mask;

    if (groupCount > 0 && !memcmp(from, to, (size_t)groupCount * 16 * sizeof(*from))) {
        MSG_WriteBit0_core(msg);
        return;
    }
    anyChanged = 0;
    for (group = 0; group < groupCount; ++group) {
        mask = 0;
        for (bit = 0; bit < 16; ++bit) {
            if (from[group * 16 + bit] != to[group * 16 + bit]) {
                mask |= 1 << bit;
            }
        }
        groupMasks[group] = mask;
        anyChanged |= mask;
    }

    if (!anyChanged) {
        MSG_WriteBit0_core(msg);
        return;
    }

    MSG_WriteBit1_core(msg);
    for (group = 0; group < groupCount; ++group) {
        mask = groupMasks[group];
        if (!mask) {
            MSG_WriteBit0_core(msg);
            continue;
        }

        MSG_WriteBit1_core(msg);
        MSG_WriteShort_core(msg, mask);
        for (bit = 0; bit < 16; ++bit) {
            if (mask & (1 << bit)) {
                MSG_WriteShort_core(msg, to[group * 16 + bit]);
            }
        }
    }
}

static inline __attribute__((always_inline)) void MSG_WriteDeltaPlayerstateShortArrayNoLead(msg_t *msg, int *from, int *to, int groupCount)
{
    int group;
    int bit;
    int mask;

    if (groupCount > 0 && !memcmp(from, to, (size_t)groupCount * 16 * sizeof(*from))) {
        for (group = 0; group < groupCount; ++group)
            MSG_WriteBit0_core(msg);
        return;
    }
    for (group = 0; group < groupCount; ++group) {
        mask = 0;
        for (bit = 0; bit < 16; ++bit) {
            if (from[group * 16 + bit] != to[group * 16 + bit]) {
                mask |= 1 << bit;
            }
        }

        if (!mask) {
            MSG_WriteBit0_core(msg);
            continue;
        }

        MSG_WriteBit1_core(msg);
        MSG_WriteShort_core(msg, mask);
        for (bit = 0; bit < 16; ++bit) {
            if (mask & (1 << bit)) {
                MSG_WriteShort_core(msg, to[group * 16 + bit]);
            }
        }
    }
}

static inline __attribute__((always_inline)) qboolean MSG_PlayerstateObjectivesChanged(playerState_t *from, playerState_t *to)
{
    int i;
    int j;

    if (!memcmp(from->objective, to->objective, sizeof(from->objective)))
        return 0;
    for (i = 0; i < 16; ++i) {
        if (from->objective[i].state != to->objective[i].state) {
            return 1;
        }
        for (j = 0; j < 6; ++j) {
            const int ofs = objectiveFields[j].offset;
            if (*(int *)((byte *)&from->objective[i] + ofs) != *(int *)((byte *)&to->objective[i] + ofs)) {
                return 1;
            }
        }
    }

    return 0;
}

static inline __attribute__((always_inline)) void MSG_WriteDeltaPlayerstateObjectives(msg_t *msg, playerState_t *from, playerState_t *to)
{
    int i;
    int j;

    if (!MSG_PlayerstateObjectivesChanged(from, to)) {
        MSG_WriteBit0_core(msg);
        return;
    }

    MSG_WriteBit1_core(msg);
    for (i = 0; i < 16; ++i) {
        qboolean fieldsChanged;

        MSG_WriteBits_core(msg, to->objective[i].state, 3);

        fieldsChanged = 0;
        for (j = 0; j < 6; ++j) {
            const int ofs = objectiveFields[j].offset;
            if (*(int *)((byte *)&from->objective[i] + ofs) != *(int *)((byte *)&to->objective[i] + ofs)) {
                fieldsChanged = 1;
                break;
            }
        }

        if (!fieldsChanged) {
            MSG_WriteBit0_core(msg);
            continue;
        }

        MSG_WriteBit1_core(msg);
        for (j = 0; j < 6; ++j) {
            MSG_WriteDeltaField(msg, (byte *)&from->objective[i], (byte *)&to->objective[i], &objectiveFields[j]);
        }
    }
}

void MSG_WriteDeltaPlayerstate(msg_t *msg, playerState_s *from, playerState_s *to)
{
    playerState_t dummy;
    int lc;
    int i;

    if (!from) {
        memset(&dummy, 0, sizeof(dummy));
        from = &dummy;
    }

    lc = MSG_LastChangedField((byte *)from, (byte *)to, playerStateFields, 105);

    MSG_WriteByte_core(msg, lc);
    for (i = 0; i < lc; ++i) {
        MSG_WriteDeltaPlayerstateField(msg, (byte *)from, (byte *)to, &playerStateFields[i]);
    }

    MSG_WriteDeltaPlayerstateStats(msg, from, to);
    MSG_WriteDeltaPlayerstateShortArray(msg, from->ammo, to->ammo, 4);
    MSG_WriteDeltaPlayerstateShortArrayNoLead(msg, from->ammoclip, to->ammoclip, 4);
    MSG_WriteDeltaPlayerstateObjectives(msg, from, to);

    if (!memcmp(&from->hud, &to->hud, sizeof(from->hud))) {
        MSG_WriteBit0_core(msg);
        return;
    }

    MSG_WriteBit1_core(msg);
    MSG_WriteDeltaHudElems(msg, from->hud.archival, to->hud.archival, 31);
    MSG_WriteDeltaHudElems(msg, from->hud.current, to->hud.current, 31);
}

NetField hudElemFields[32] = {
    { (char *)&str_002acda0, 0x8, 0xffffff9d },
    { (char *)&str_0021eb5c, 0x0, 0x4 },
    { (char *)&str_002acda4, 0x20, 0x20 },
    { (char *)&str_002acdb0, 0x4, 0xffffff9d },
    { (char *)&str_002acdb4, 0x1c, 0x6 },
    { (char *)&str_002acdc0, 0x10, 0x0 },
    { (char *)&str_002acdcc, 0x3c, 0x8 },
    { (char *)&str_0021a778, 0x34, 0xa },
    { (char *)&str_0021a588, 0x38, 0xa },
    { (char *)&str_002acddc, 0x28, 0x20 },
    { (char *)&str_002acdec, 0x24, 0x20 },
    { (char *)&str_002acdfc, 0x2c, 0x10 },
    { (char *)&str_002ace08, 0x70, 0x0 },
    { (char *)&str_002ace10, 0x68, 0x20 },
    { (char *)&str_002ace18, 0xc, 0xffffff9d },
    { (char *)&str_002ace1c, 0x18, 0x4 },
    { (char *)&str_002ace28, 0x78, 0x0 },
    { (char *)&str_002ace30, 0x74, 0x8 },
    { (char *)&str_002ace38, 0x14, 0x4 },
    { (char *)&str_002ace40, 0x48, 0x20 },
    { (char *)&str_002ace50, 0x4c, 0x10 },
    { (char *)&str_002ace5c, 0x44, 0xa },
    { (char *)&str_002ace68, 0x30, 0x8 },
    { (char *)&str_002ace70, 0x40, 0xa },
    { (char *)&str_002ace7c, 0x60, 0x20 },
    { (char *)&str_002ace8c, 0x64, 0x10 },
    { (char *)&str_002ace98, 0x50, 0xffffff9d },
    { (char *)&str_002acea0, 0x54, 0xffffff9d },
    { (char *)&str_002acea8, 0x5c, 0x6 },
    { (char *)&str_002aceb8, 0x58, 0x4 },
    { (char *)&str_002acec8, 0x6c, 0x20 },
    { (char *)&str_002aced4, 0x7c, 0x1 }
};
NetField objectiveFields[8] = {
    { (char *)&str_002acee0, 0x4, 0x0 },
    { (char *)&str_002aceec, 0x8, 0x0 },
    { (char *)&str_002acef8, 0xc, 0x0 },
    { (char *)&str_002acf04, 0x18, 0xc },
    { (char *)&str_002acf0c, 0x10, 0xa },
    { (char *)&str_002acf14, 0x14, 0x4 },
    { 0, 0x0, 0x0 },
    { 0, 0x0, 0x0 }
};

const char str_0021a588[] = "height";
const char str_0021a778[] = "width";
const char str_0021eb5c[] = "type";
const char str_002acda0[] = "y";
const char str_002acda4[] = "color.rgba";
const char str_002acdb0[] = "x";
const char str_002acdb4[] = "alignScreen";
const char str_002acdc0[] = "fontScale";
const char str_002acdcc[] = "materialIndex";
const char str_002acddc[] = "fadeStartTime";
const char str_002acdec[] = "fromColor.rgba";
const char str_002acdfc[] = "fadeTime";
const char str_002ace08[] = "value";
const char str_002ace10[] = "time";
const char str_002ace18[] = "z";
const char str_002ace1c[] = "alignOrg";
const char str_002ace28[] = "sort";
const char str_002ace30[] = "text";
const char str_002ace38[] = "font";
const char str_002ace40[] = "scaleStartTime";
const char str_002ace50[] = "scaleTime";
const char str_002ace5c[] = "fromHeight";
const char str_002ace68[] = "label";
const char str_002ace70[] = "fromWidth";
const char str_002ace7c[] = "moveStartTime";
const char str_002ace8c[] = "moveTime";
const char str_002ace98[] = "fromX";
const char str_002acea0[] = "fromY";
const char str_002acea8[] = "fromAlignScreen";
const char str_002aceb8[] = "fromAlignOrg";
const char str_002acec8[] = "duration";
const char str_002aced4[] = "foreground";
const char str_002acee0[] = "origin[0]";
const char str_002aceec[] = "origin[1]";
const char str_002acef8[] = "origin[2]";
const char str_002acf04[] = "icon";
const char str_002acf0c[] = "entNum";
const char str_002acf14[] = "teamNum";
