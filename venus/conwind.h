/*
 * @(#) Gemini\conwind.h
 * @(#) Stefan Eissing, 17. M„rz 1993
 *
 * description: functions to manage the console window
 *
 * jr 23.4.95
 */


#ifndef G_conwind__
#define G_conwind__

#include <tos.h>

#include "terminal.h"	/* XXX BOGUS: NO DEPENDENCY IN PROJECT!!! */

typedef struct
{
	char title[MAX_PATH_LEN];				/* Titelzeile */
	char path[MAX_PATH_LEN];				/* Infozeile */

	TermInfo *terminal;

} ConsoleWindow_t;

#define CONWIND		(NAME|CLOSER|MOVER|FULLER)

WindInfo *TerminalWindowOpen (const char *path, const char *title, 
	int is_console);

int MakeConsoleTopWindow (int *was_opened, word *old_top_window);
int RestoreConsolePosition (int was_opened, word old_top_window);

int StartProgramInWindow (const char *name, const COMMAND *args, 
	const char *dir, const char *env);

#endif	/* !G_conwind */