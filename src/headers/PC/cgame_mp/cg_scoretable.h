#ifndef CG_SCORETABLE_H
#define CG_SCORETABLE_H

/* The WWII scoreboard: the match title and clock, then each team's block of
 * striped rows with its name, score and emblem, the viewer's side on top. */
void CG_ScoreTableInit(void);
int CG_ScoreTableEnabled(void);
/* While it shows, the scoreboard takes the place of the whole HUD. */
int CG_ScoreTableHidesHud(void);
/* Our server ends the scores command with the kills of the listed players;
 * without them the Kills column stays empty. -1 for a player not listed. */
void CG_ScoreTableClearKills(void);
void CG_ScoreTableSetKills(int clientNum, int kills);
int CG_ScoreTableKills(int clientNum);
/* Then the receiver's own assists and headshots; -1 from a retail server. */
void CG_ScoreTableSetOwnStats(int assists, int headshots);
void CG_ScoreTableOwnStats(int *assists, int *headshots);
/* Builds the team emblems for the current screen size; cheap once built. */
void CG_ScoreTablePrepare(void);
void CG_DrawScoreTable(float fade);
/* At the end of a match: the result over the title and the reason it ended
 * under it, in place of the clock. */
void CG_DrawScoreTableResult(float fade, const char *result, const char *reason);
/* Deathmatch, or a mod's mode where everyone plays for himself. */
int CG_ScoreTableFreeForAll(void);
/* The map's display name without its country, "Toujane". */
void CG_ScoreTableMapName(char *buf, int size);
/* The kind of emblem a team has, -1 for none: the Allies' follows their
 * nation, the Axis have the iron cross. */
int CG_ScoreTableEmblemKind(int team);
/* The emblem of kind as white texels whose alpha covers it: size BGRA texels
 * a side, a texel of margin round it, rows stride bytes apart. */
void CG_ScoreTableRasterizeEmblem(int kind, byte *bgra, int stride, int size);
/* The emblem of kind as a lit medal: size BGRA texels a side, a texel of
 * margin round it, rows stride bytes apart from bgra. Only rows firstRow to
 * firstRow + rows are written, so the work can be spread over frames. */
void CG_ScoreTableRasterizeMedal(int kind, byte *bgra, int stride, int size, int firstRow, int rows);
/* Its lettering, which the killcam shares: where a font's capitals stand, the
 * scale that makes them cap units tall, a text's width in units and how many
 * of its letters fit in room. */
void CG_ScoreTableFontMetrics(FontHandle font, float *cap, float *baseline);
float CG_ScoreTableCapScale(FontHandle font, float cap);
float CG_ScoreTableTextWidth(const char *text, int maxChars, FontHandle font, float scale);
int CG_ScoreTableFit(const char *text, FontHandle font, float scale, float room);

#endif
