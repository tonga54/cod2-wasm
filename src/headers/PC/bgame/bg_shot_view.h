#ifndef BG_SHOT_VIEW_H
#define BG_SHOT_VIEW_H

/* How a shooter drew the remote players near its aim: for each, the
 * snapshot samples its playback drew them between and how far along
 * (cg_ents_mp.c). Sent beside each command that may fire (cl_input.c), so
 * the server meets them where they were drawn (g_weapon_mp.c). */

/* The client message's op after a packet's commands; a server that knows
 * nothing of it stops reading at the commands. */
#define BG_SHOT_VIEW_OP 4
#define BG_SHOT_VIEW_ENTRIES 4
/* The newest commands of a packet that carry their views. */
#define BG_SHOT_VIEW_RECORDS 16
/* A blend's fraction in 1/65535ths; a bridge's reach past the newest sample
 * in 1/64 ms, up to the playback's 50 ms. */
#define BG_SHOT_VIEW_FRAC_ONE 65535
#define BG_SHOT_VIEW_BRIDGE_STEPS 64
#define BG_SHOT_VIEW_BRIDGE_MAX 50
#define BG_SHOT_VIEW_MAX_SPAN 1000

enum {
    BG_SHOT_VIEW_BLEND,  /* between two samples */
    BG_SHOT_VIEW_BRIDGE, /* past the newest sample, along its velocity */
    BG_SHOT_VIEW_SAMPLE, /* on the first sample since the player appeared */
    BG_SHOT_VIEW_MODES
};

/* A player's entity state in one snapshot. */
typedef struct {
    int time; /* the player's command time, pos.trTime */
    float origin[3];
    float velocity[3];
    float angles[3];
    float moveDir;
    float lean;
} bgPlayerSample_t;

typedef struct {
    int clientNum;
    int mode;
    int time; /* blend: the older sample's; bridge: the newest's; sample: its own */
    int span; /* blend: the newer sample's time after the older's */
    int frac; /* blend: 0 to 65535 of the way; bridge: 1/64 ms past the newest */
} bgShotViewEntry_t;

typedef struct {
    int serverTime; /* the command's */
    int count;
    bgShotViewEntry_t entries[BG_SHOT_VIEW_ENTRIES];
} bgShotView_t;

typedef struct {
    float origin[3];
    float angles[3];
    float moveDir;
    float lean;
} bgPlayerPose_t;

float BG_PlayerMotionBlend(float a, float b, float va, float vb, float seconds, float f);
void BG_ShotViewPose(const bgShotViewEntry_t *entry, const bgPlayerSample_t *first, const bgPlayerSample_t *second,
                     bgPlayerPose_t *pose);

#endif
