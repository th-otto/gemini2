/*
 * @(#) Gemini\mvwindow.c
 * @(#) Stefan Eissing, 02. Juni 1993
 *
 * description: top layer of functions for mupfel window
 */

#ifndef G_MVWINDOW__
#define G_MVWINDOW__

#include "..\common\memfile.h"

void SetMWSize (TermInfo *terminal);

void SizeOfMWindow (TermInfo *terminal, GRECT *wind);

int TerminalDarstellung (WindInfo *wp, int only_asking);

word ConsoleInit (void);
void ConsoleExit (void);

void getInMWindow (SHOWINFO *ps);
void SetInMWindow (SHOWINFO *ps, int give_warning);
void ExecConsoleInfo (char *line);

int WriteConsoleInfo (MEMFILEINFO *mp, char *buffer);

void GetConsoleFont (word *id, word *points);

#endif /* !G_MVWINDOW__ */