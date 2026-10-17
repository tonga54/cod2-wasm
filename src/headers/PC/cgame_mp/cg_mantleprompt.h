#ifndef CG_MANTLEPROMPT_H
#define CG_MANTLEPROMPT_H

/* The WWII prompt for climbing over a ledge: the jump key in gold, then "Press
 * to" and a white curved arrow, in the player labels' lettering. It stands in
 * for the stock mantle hint. */
void CG_MantlePromptInit(void);
int CG_MantlePromptActive(void);
/* Returns 0, having drawn nothing, when the stock hint should be drawn. */
int CG_DrawMantlePrompt(const rectDef_t *rect, const char *key);

#endif
