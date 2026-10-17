#ifndef CG_KILLFEED_H
#define CG_KILLFEED_H

/* Bottom-left kill feed: "attacker [weapon] victim", colored by relation to
 * the local player. A headshot adds the headshot icon after the weapon. */
void CG_KillFeedInit(void);
int CG_KillFeedEnabled(void);
void CG_KillFeedAdd(int attacker, int victim, const char *iconShader, int wideIcon, int flipIcon, int headshot);
void CG_DrawKillFeed(void);

#endif
