#ifndef CLEAN_PC_CLIENT_MP_CL_CMD_HISTORY_H
#define CLEAN_PC_CLIENT_MP_CL_CMD_HISTORY_H

#include "common_types.h"

/* The client makes a command each frame. cl.cmds keeps 128, half a second at
 * 250 frames a second: a TCP stall that held the server's acknowledgement
 * longer left prediction nothing to replay, so the player froze. Prediction
 * reads these instead, 4 s at 250 frames a second. A power of two. */
#define CL_CMD_HISTORY 1024

/* The connection reads as interrupted once the newest command is this many
 * milliseconds ahead of the last one the server acknowledged, whatever the
 * frame rate. */
#define CL_CONNECTION_INTERRUPTED_MS 1000

void CL_KeepUserCmd(int cmdNumber, const usercmd_t *cmd);

#endif
