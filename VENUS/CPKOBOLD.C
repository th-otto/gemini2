/*
 * @(#) Gemini\CpKobold.c
 * @(#) Stefan Eissing, 29. Januar 1994
 *
 * description: functions to copy/move/delete files via Kobold
 *
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <tos.h>
#include <aes.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"

#include "..\exec_job\exec_job.h"
#include "..\mupfel\mupfel.h"

#include "vs.h"
#include "window.h"

#include "cpkobold.h"
#include "erase.h"
#include "util.h"
#include "venuserr.h"


/* internal texts
 */
#define NlsLocalSection "G.cpkobold"
enum NlsLocalText{
T_NOKOBOLD,	/*Kobold_2 konnte nicht gefunden werden!*/
T_NOMEM,	/*Es ist nicht genug freier Speicher vorhanden!*/
};



char *allocJob (ARGVINFO *A, const char *dest_dir)
{
	int i;
	char *job;
	size_t len = 500;
	
	for (i = 0; i < A->argc; ++i)
	{
		len += strlen (A->argv[i]) + 6;
	}
	
	if (dest_dir != NULL)
		len += strlen (dest_dir) + 6;
		
	job = calloc (1, len);
	return job;
}


static void addSelectCmd (char *job, const char *par, int first)
{
	char *ptr;
	const char *cp;

	(void)first;	
/*	if (0)
	{
		strcat (job, "#0 + \\\n\r");
	}
*/	
	strcat (job, "#0 + ");  /* Quell-Selektionskommando */
	strcat (job, par);      /* Pfadangabe               */

	ptr = strrchr (job, '\\');
	
	if (ptr && ((ptr[1]==0) || (ptr[1]=='*')))
		*ptr = 0;

	strcat (job, "\r\n");
	
	cp = (par[1] == ':') ? &par[2] : par;

	if (!strcmp (cp, "\\") || !strcmp (cp, "\\*.*") || !cp[0])
	{
		/* Alles selektieren, falls Kommando die
		 * Form 'C:\' oder 'C:\*.*' hat
		 */ 
		strcat (job, "#8 *+\r\n");
	}
}


static char *createKoboldCommands (ARGVINFO *A, const char *dest_dir,
	KoboldCopyCommand cmd, int confirmation, int overwrite, 
	int rename,	char *job)
{
	int parameter_found;
	int drives_done[26];

	memset (drives_done, 0, sizeof (drives_done));
	
	strcat (job, "#2 = ");
	if (confirmation)
		strcat (job, "2\r\n");
	else if (!overwrite)
		strcat (job, "1\r\n");
	else
		strcat (job, "0\r\n");
	(void)rename;
	
	/* Zielverzeichnis wÑhlen (nicht bei Lîschkommando)
	 */
	if (cmd != kb_delete)
	{
		char *cp;
		
		strcat (job, "#1 ");
		strcat (job, dest_dir);
		cp = strrchr (job, '\\');
		if (!cp || cp[1] != '\0')
			strcat (job, "\\");
		strcat (job, "\r\n");
	}
	
/* Hier werden nun die Quellpfade selektiert. Da die Pfade unsortiert von
   verschiedenen Laufwerken stammen kînnen, werden mehrere DurchgÑnge gemacht.
   In jedem Durchgang werden nur Pfade eines Laufwerks in Quellselektionen
   umgesetzt und dann in 'drives_done' markiert, bis keine neuen Laufwerke mehr
   vorhanden sind. So hat man anschlieûend ein nach Laufwerken sortierte Liste
   der erforderlichen Selektionen. */

	do
	{
		char drive, current_drive;
		int i;
		
		parameter_found = FALSE;
		
		/* Suche den ersten Parameter, den wir noch nicht hatten
		 */
		for (i = 0; (!parameter_found) && (i < A->argc); ++i)
		{
			drive = toupper (A->argv[i][0]);
			if ((drive >= 'A') && (drive <= 'Z') 
				&& (!drives_done[drive-'A']))
			{
				/* Hier wird das aktuelle Laufwerk fÅr diesen Durchgang bestimmt.
				 * Es ist das erste gefundene Laufwerk, daû noch nicht in
				 * 'drives_done' markiert ist. FÅr dieses Laufwerk wird dann ein
				 * komplettes Selektionskommando (d.h. inkl. Laufwerksangabe) erzeugt.
				 */
			  
				addSelectCmd (job, A->argv[i], 1);
				current_drive = drive;
				drives_done[current_drive - 'A'] = TRUE;
				parameter_found = TRUE;
			}
		}
		
		if (parameter_found)
		{		
			/* FÅr alle anderen Pfade, die zu dem fÅr diesen Durchgang aktuellen
			 * Laufwerk gehîren, werden Selektionen ohne Laufwerksangabe erzeugt
			 * (also nur der Pfad ab &par[2]), da sich dies dann im KOBOLD auf
			 * daû akt. Laufwerk bezieht. Mit Laufwerksangabe wÅrden immer alle
			 * Strukturen neu eingelesen und vorherige Selektionen gingen verloren.
			 */   

			for (; i < A->argc; ++i)
			{
				if (toupper (A->argv[i][0]) == current_drive)
					addSelectCmd (job, &(A->argv[i][2]), 0);
			}
		
			/* Anschlieûend wird das gewÅnschte Kommando erzeugt. Wenn die Pfade
			 * von mehreren Laufwerken kommen, so wird jedes Laufwerk spÑter 
			 * einzeln behandelt, da sich das Kommando nach jedem Block zusammen-
			 * gehîriger Pfade wiederholt.
			 */
								
			switch (cmd)
			{
				case kb_copy:
					strcat (job, "#12 ");
					if (current_drive != toupper (dest_dir[0]))
						strcat (job, "#39");
					break;

				case kb_move:
					strcat (job, "#13 "); 
					if (current_drive != toupper (dest_dir[0]))
						strcat (job, "#39");
					break;

				case kb_delete:
					strcat (job, "#14 "); 
					break;
			}
			
			strcat (job, "\r\n");
		}
		
	}
	while (parameter_found);
	
	strcat (job, ";#0 ?;#1 ?\r\n");
	
	return job;
}


static char *createKoboldJob (ARGVINFO *A, const char *dest_dir,
	KoboldCopyCommand cmd, int confirmation, int overwrite, 
	int rename)
{
	char *job;
	
	job = allocJob (A, dest_dir);
	if (job == NULL)
	{
		venusErr (NlsStr (T_NOMEM));
		return NULL;
	}

	switch (cmd)
	{
		case kb_format:
		{
			char tmp[10];
			
			sprintf (tmp, "#26%c\r\n", toupper (A->argv[0][0]));
			strcat (job, tmp);
			break;
		}
		
		case kb_copy:
		case kb_move:
		case kb_delete:
			job = createKoboldCommands (A, dest_dir, cmd, 
				confirmation, overwrite, rename, job);
			break;
	}
	
	if (job)
		strcat (job, "\r\nQuit\r\n");

	return job;
}


/* ----------- Schnittstellen-Nachrichten -------------------------------------*/

#define KOBOLD_JOB_NO_WINDOW 0x2F11/* Dito, ohne Hauptdialog                   */

#define KOBOLD_ANSWER 0x2F12       /* Antwort vom KOBOLD mit Status in Wort 3  */
                                   /* und Zeile in Wort 4                      */

#define KOBOLD_CLOSE 0x2F16        /* Dient zum expliziten Schlieûen des       */
                                   /* KOBOLD, falls Antwortstatus != FINISHED  */

#define KOBOLD_NAME	"KOBOLD_2"

static ARGVINFO lastArgv;
static KoboldCopyCommand lastCmd;
static char *lastDestDir;

static void freeLastInformations (void)
{
	free (lastDestDir);
	lastDestDir = NULL;

	if (lastArgv.argc > 0)
		FreeArgv (&lastArgv);
	memset (&lastArgv, 0, sizeof (lastArgv));
}


int sendKoboldJob (char *job, ARGVINFO *A, const char *dest_dir,
	KoboldCopyCommand cmd)
{
	word retcode;

	if (!KoboldPresent ())
		return FALSE;
		
	WindUpdate (END_UPDATE);

	if ((retcode = perform_kobold_job (job)) != 0)
	{
		int i;

		freeLastInformations ();
		lastCmd = cmd;
		
		if (MakeGenericArgv (pMainAllocInfo, &lastArgv, TRUE, 0, NULL))
		{
			for (i = 0; i < A->argc; ++i)
				StoreInArgv (&lastArgv, A->argv[i]);
		}
		
		if (dest_dir != NULL)
			lastDestDir = StrDup (pMainAllocInfo, dest_dir);
	}

	WindUpdate (BEG_UPDATE);
	free (job);
	
	return retcode;
}


void GotKoboldAnswer (word messbuff[8])
{
	if (messbuff[0] != KOBOLD_ANSWER)
		return;
	
	WindUpdate (END_UPDATE);
	kobold_job_done (messbuff[3]);
	WindUpdate (BEG_UPDATE);

	if (lastCmd != kb_copy)
	{
		int i;
		char path[MAX_PATH_LEN];
		
		for (i = 0; i < lastArgv.argc; ++i)
		{
			strcpy (path, lastArgv.argv[i]);
			
			if ((messbuff[3] == 0)
				 && (lastCmd == kb_delete || lastCmd == kb_move))
			{
				ObjectWasErased (path, TRUE);
			}
				
			StripFileName (path);
			BuffPathUpdate (path);
		}
	}
	
	if (lastDestDir != NULL)
		BuffPathUpdate (lastDestDir);

	freeLastInformations ();	
	FlushPathUpdate ();
}


#define KOBOLD_ENV_NAME		"KOBOLD_PATH"

int KoboldPresent (void)
{
	return init_kobold (apid, GetEnv (MupfelInfo, KOBOLD_ENV_NAME));
}


int CopyWithKobold (ARGVINFO *A, const char *dest_dir, 
	KoboldCopyCommand cmd, int confirm, int overwrite, int rename)
{
	char *job;
	
	job = createKoboldJob (A, dest_dir, cmd, confirm, overwrite,
		rename);
	if (job == NULL)
	{
		return FALSE;
	}
	
	if (!sendKoboldJob (job, A, dest_dir, cmd))
	{
		free (job);
		venusErr (NlsStr (T_NOKOBOLD));
	}
	
	return TRUE;
}