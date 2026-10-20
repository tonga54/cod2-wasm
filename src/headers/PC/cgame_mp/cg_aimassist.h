#ifndef CG_AIMASSIST_H
#define CG_AIMASSIST_H

/* A minimal aim assist for controller play, applied by CL_Gamepad_Move to the
 * sticks alone. Every frame: finds the visible enemy nearest the crosshair in
 * the local player's own view and hands the client how much to slow the right
 * stick and how much of the enemy's motion to follow. */
void CG_AimAssistFrame(void);

#endif
