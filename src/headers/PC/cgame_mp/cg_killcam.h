#ifndef CG_KILLCAM_H
#define CG_KILLCAM_H

#define KILLCAM_MAX_CLIENTS 64

/* The WWII killcam over the killer's replayed view: letterbox bars, the title
 * with the key that respawns or skips, the score between killer and victim,
 * the players who hurt the victim and the weapon that killed them. */
void CG_KillcamInit(void);
/* While a replay of another player plays with the setting on. */
int CG_KillcamActive(void);
/* The stock killcam's bars, title, prompt and timer, which it stands in for. */
int CG_KillcamHidesHudElem(const hudelem_t *elem);
/* Every obituary once: attacker is -1 without a killer, eventParm the event's
 * weapon or, with 0x80 set, its means of death. */
void CG_KillcamObituary(int attacker, int target, int eventParm);
/* Our server's list for the viewer's death: the killer, then the other
 * players who hurt them in the order they first hit. */
void CG_KillcamSetAttackers(int killer, const int *others, int count);
void CG_DrawKillcam(void);

#endif
