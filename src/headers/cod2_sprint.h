#ifndef COD2_SPRINT_H
#define COD2_SPRINT_H

/* Usercmd bit 1 is reserved for the browser sprint binding. Playerstate bit
 * 25 belongs to spectator permissions; bit 27 carries sprint independently
 * through active-player cleanup and archived spectator snapshots. */
#define BUTTON_SPRINT 0x2
#define PMF_SPRINT 0x08000000
#define SPRINT_SPEED_SCALE 1.45f

#endif
