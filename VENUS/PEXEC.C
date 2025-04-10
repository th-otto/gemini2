/*
 * @(#) Gemini\Pexec.c
 * @(#) Stefan Eissing, 08. Januar 1994
 *
 * description: Starten von Programmen
 *
 */
 
#include <vdi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>
#include <flydial\flydial.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"

#include "vs.h"

#include "window.h"
#include "applmana.h"
#include "conwind.h"
#include "deskwind.h"
#include "fileutil.h"
#include "gemini.h"
#include "iconinst.h"
#include "infofile.h"
#include "message.h"
#include "myalloc.h"
#include "pexec.h"
#include "redraw.h"
#include "stand.h"
#include "terminal.h"
#include "util.h"
#include "menu.h"
#include "venuserr.h"

#include "..\mupfel\mupfel.h"

#if 0
static int save_rows;
static int save_lines;
static int save_columns;
#endif

void PrepareInternalStart (int *wasOpened, int *oldTopWindow, 
		int starting)
{
	if (starting)
	{
		*wasOpened = FALSE;
		*oldTopWindow = -1;

		MakeConsoleTopWindow (wasOpened, oldTopWindow);
	}
	else
	{
		RestoreConsolePosition (*wasOpened, *oldTopWindow);
	}
}


void PrepareExternalStart (void *actualInfo, int starting)
{
	char tmp[128];
#if 0
	const char *val;
#endif

	if (starting)
	{
		int rows, columns;
		
		GrafMouse (HOURGLASS, NULL);

		RemoveMenuBar ();

		MakeConfInfo ();
		CloseAllWind (TRUE);
		
#if 0
		val = GetEnv (actualInfo, "LINES");
		save_lines = (val == NULL)? -1: atoi (val);
		val = GetEnv (actualInfo, "ROWS");
		save_rows = (val == NULL)? -1: atoi (val);
		val = GetEnv (actualInfo, "COLUMNS");
		save_columns = (val == NULL)? -1: atoi (val);
#endif
	
		ExitBiosOutput ();
		
		vq_chcells (vdi_handle, &rows, &columns);
		sprintf (tmp, "LINES=%d", rows);
		PutEnv (actualInfo, tmp);
		sprintf (tmp, "ROWS=%d", rows);
		PutEnv (actualInfo, tmp);
		sprintf (tmp, "COLUMNS=%d", columns);
		PutEnv (actualInfo, tmp);

#if 0
		/* jr: Ist's der richtige Ort? */
		PutEnv (actualInfo, "TERM=");
		PutEnv (actualInfo, "TERMCAP=");
#endif
	}
	else
	{
		InitBiosOutput ();
	
		/* jr: Erl„uterung
		
		COLUMNS, LINES und ROWS haben durch die PutEnv()s oben
		ihren Autoset-Status verloren. Was wir hier machen, ist
		ein Hack, aber er wird ohnehin nur unter SingleTOS beim
		Starten von GEM-Programmen ben”tigt. Es fhrt dazu, daž --
		wenn der User *tats„chlich* einer dieser Variablen einen
		Wert zugewiesen haben sollte, er jetzt verloren ginge */
	
#if 0
		if (save_rows > 0)
		{
			sprintf (tmp, "ROWS=%d", save_rows);
			PutEnv (actualInfo, tmp);
		}
		if (save_lines > 0)
		{
			sprintf (tmp, "LINES=%d", save_lines);
 			PutEnv (actualInfo, tmp);
		}
		if (save_columns > 0)
		{
			sprintf (tmp, "COLUMNS=%d", save_columns);
			PutEnv (actualInfo, tmp);
		}
#else
		CreateAutoVar (actualInfo, "COLUMNS", 15);
		CreateAutoVar (actualInfo, "LINES", 15);
		CreateAutoVar (actualInfo, "ROWS", 15);
#endif
		ReInstallBackground (TRUE);

		ExecConfInfo (FALSE);

		allrewindow (WU_WINDINFO);
		
		InstallMenuBar ();
		GrafMouse (ARROW, NULL);

		UpdateSpecialIcons ();
		
		HandlePendingMessages (FALSE);
	}
}


static void envStartMode(char *name, word *startmode)
{
	char tmp[128];
	char *cp;
	
	if ((cp = GetEnv (MupfelInfo, name)) != NULL)
	{
		strncpy (tmp, cp, 127);
		tmp[127] = '\0';
		strupr (tmp);
		if (strstr (tmp, "W:N") != NULL)
			*startmode |= WCLOSE_START;
		if (strstr (tmp, "W:Y") != NULL)
			*startmode &= ~WCLOSE_START;
		if (strstr (tmp, "O:N") != NULL)
			*startmode &= ~OVL_START;
		if (strstr(tmp, "O:Y") != NULL)
			*startmode |= OVL_START;
	}
}

static void checkStartMode (const char *ext, word *startmode)
{
	if (!stricmp ("MUP", ext))
	{
		*startmode &= ~OVL_START;
	}
}

/*
 * execute program "fname" with commandline "command", use
 * showpath to display full path or name only
 * startmode for TOS-Screen or GEM-Background etc.
 */
int executer (word startmode, word showpath, char *fname,
	char *command)
{
	char myname[MAX_FILENAME_LEN];
	char *cp;
	int retcode = 0;
	int close_windows;
	
	GetBaseName (fname, myname, MAX_FILENAME_LEN);
	
	envStartMode ((startmode & GEM_START)? 
				"GEMDEFAULT" : "TOSDEFAULT", &startmode);
	
	{
		char env_name[MAX_PATH_LEN];
		
		if ((cp = strchr (myname, '.')) != NULL)
		{
			*cp = '_';
		}
	
		strcpy (env_name, "OPT_");
		strcat (env_name, myname);
		envStartMode (env_name, &startmode);
	}
	
	if (cp != NULL)
		checkStartMode (cp + 1, &startmode);

	if (startmode & GEM_START)
	{
		close_windows = (startmode & WCLOSE_START)? 
			PARM_EGAL : PARM_NO;
	}
	else
	{
		close_windows = (startmode & WCLOSE_START)? 
			PARM_YES : PARM_EGAL;
	}
	
	/* Falls ben”tigt haben wir jetzt die Console ge”ffnet und
	 * k”nnen nun per Mupfel das Programm starten
	 */

	if (startmode & GEM_START)
		InstallErrorFunction (MupfelInfo, ErrorAlert);
	
	retcode = StartGeminiProgram (MupfelInfo, fname, command, 
		showpath, startmode & GEM_START, close_windows,
		startmode & WAIT_KEY, startmode & OVL_START,
		startmode & SINGLE_START, TRUE);

	if ((_GemParBlk.global[1] == 1) && !(startmode & WCLOSE_START))
		MupfelShouldDisplayPrompt = TRUE;
	
	if (startmode & GEM_START)
		DeinstallErrorFunction (MupfelInfo);
	
	return retcode;
}


