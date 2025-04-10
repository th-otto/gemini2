/*
 * @(#) Gemini\infofile.c
 * @(#) Stefan Eissing, 24. Januar 1994
 *
 * description: handle the venus.inf file
 *
 * jr 21.4.95
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <tos.h> 
#include <flydial\flydial.h> 
#include <nls\nls.h> 

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\memfile.h"
			
#include "vs.h"
#include "stand.h"
#include "menu.rh"
/* #include "sorttyp.rh" */
/* #include "sorticon.rh" */

#include "window.h"
#include "applmana.h"
#include "deskwind.h"
#include "filewind.h"
#include "filedraw.h"
#include "fileutil.h"
#include "iconhash.h"
#include "iconinst.h"
#include "drives.h"
#include "iconrule.h"
#include "infofile.h"
#include "..\common\alloc.h"
#include "..\common\memfile.h"
#include "menu.h"
#include "message.h"
#if MERGED
#include "conwind.h"
#include "mvwindow.h"
#endif
#include "myalloc.h"
#include "redraw.h"
#include "util.h"
#include "venuserr.h"
#include "wildcard.h"
#include "windstak.h"
#include "..\mupfel\mupfel.h"



/* interal texts
 */
#define NlsLocalSection "G.infofile"
enum NlsLocalText{
T_FORMAT,		/*Unbekanntes Format in der Datei %s!*/
T_CREATE,		/*Kann die Datei %s nicht anlegen!*/
T_NOBUFFER,		/*Es ist nicht genug Speicher vorhanden, um 
den Status zu sichern!*/
T_ERROR,		/*Fehler beim Schreiben von %s!*/
T_NO_GIN,		/*%s ist keine von Gemini erzeugte Datei. Das
 Laden wird abgebrochen.*/
};

/* L„nge des zu allozierenden Buffers fr sprintf
 */
#define BUFFLEN		1024

/* L„nge einer Zeile fr Config-Info
 */
#define CONF_LEN	256

/* Configuration List
 */
typedef struct confInfo
{
	struct confInfo *nextconf;
	char *line;
} ConfInfo;


/* internals
 */
static ConfInfo *firstconf = NULL;

static void freeConfInfo (void)
{
	ConfInfo *aktconf;
	
	while (firstconf)
	{
		aktconf = firstconf->nextconf;
		free (firstconf->line);			/* string freigeben */
		free (firstconf);				/* structure freigeben */
		firstconf = aktconf;
	}
}


static word execScrapInfo (char *line)
{
	char *cp;
	word obx,oby;
	word shortcut = 0;
	char color = 0x10;
	char name[MAX_FILENAME_LEN];
	
	strtok (line, "@\n");
	if ((cp = strtok (NULL, "@\n")) != NULL)
	{
		obx = atoi (cp);
		if ((cp = strtok (NULL,"@\n")) != NULL)
		{
			oby = atoi (cp);
			if ((cp = strtok (NULL, "@\n")) != NULL)
			{
				strcpy (name, cp);
				
				if ((cp = strtok (NULL, "@\n")) != NULL)
					shortcut = atoi (cp);
					
				if ((cp = strtok (NULL, "@\n")) != NULL)
					color = (char) atoi (cp);
					
				obx = checkPromille (obx, 0);
				oby = checkPromille (oby, 0);
				obx = (word)scale123 (Desk.g_w, obx, ScaleFactor);
				oby = (word)scale123 (Desk.g_h, oby, ScaleFactor);

				InstScrapIcon (obx, oby, name, shortcut, color);
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

static word execTrashInfo (char *line)
{
	char *cp;
	word obx, oby;
	word shortcut = 0;
	char color = 0x10;
	char name[MAX_FILENAME_LEN];
	
	strtok (line, "@\n");
	if ((cp = strtok (NULL, "@\n")) != NULL)
	{
		obx = atoi (cp);
		if ((cp = strtok (NULL, "@\n")) != NULL)
		{
			oby = atoi (cp);
			if ((cp = strtok (NULL, "@\n")) != NULL)
			{
				strcpy (name, cp);

				if ((cp = strtok (NULL, "@\n")) != NULL)
					shortcut = atoi (cp);
					
				if ((cp = strtok (NULL,"@\n")) != NULL)
					color = (char)atoi (cp);
					
				obx = checkPromille (obx, 0);
				oby = checkPromille (oby, 0);
				obx = (word)scale123 (Desk.g_w, obx, ScaleFactor);
				oby = (word)scale123(Desk.g_h, oby, ScaleFactor);

				InstTrashIcon (obx, oby, name, shortcut, color);
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

static word execShredderInfo (char *line)
{
	char *cp;
	word obx,oby;
	char color = 0x10;
	char name[MAX_FILENAME_LEN];
	
	strtok (line, "@\n");
	if((cp = strtok (NULL, "@\n")) != NULL)
	{
		obx = atoi (cp);
		if ((cp = strtok (NULL, "@\n")) != NULL)
		{
			oby = atoi (cp);
			if ((cp = strtok (NULL, "@\n")) != NULL)
			{
				strcpy (name, cp);

				if ((cp = strtok (NULL, "@\n")) != NULL)
					color = (char)atoi (cp);
				
				obx = checkPromille (obx, 0);
				oby = checkPromille (oby, 0);
				obx = (word)scale123 (Desk.g_w, obx, ScaleFactor);
				oby = (word)scale123 (Desk.g_h, oby, ScaleFactor);

				InstShredderIcon (obx, oby, name, color);
				return TRUE;
			}
		}
	}
	
	return FALSE;
}


static int execBoxInfo (char *line)
{
	GRECT box;
	word kind = WK_FILE;
	char *cp;
	
	strtok (line, "@\n");
	if ((cp = strtok (NULL, "@\n")) != NULL)
	{
		box.g_x = atoi (cp);
		if ((cp = strtok (NULL, "@\n")) != NULL)
		{
			box.g_y = atoi (cp);
			if ((cp = strtok (NULL, "@\n")) != NULL)
			{
				box.g_w = atoi (cp);
				if ((cp = strtok (NULL, "@\n")) != NULL)
				{
					box.g_h = atoi (cp);

					if ((cp = strtok (NULL, "@\n")) != NULL)
						kind = atoi (cp);
						
					box.g_x = checkPromille (box.g_x, 0);
					box.g_y = checkPromille (box.g_y, 0);
					
					box.g_x = Desk.g_x + 
						(word)scale123 (Desk.g_w, box.g_x, ScaleFactor);
					box.g_w = Desk.g_x + 
						(word)scale123 (Desk.g_w, box.g_w, ScaleFactor);
					box.g_y = Desk.g_y + 
						(word)scale123 (Desk.g_h, box.g_y, ScaleFactor);
					box.g_h = Desk.g_y + 
						(word)scale123 (Desk.g_h, box.g_h, ScaleFactor);

					PushWindBox (&box, kind);
					
					return TRUE;
				}
			}
		}
	}
	
	return FALSE;
}


static void execInfoLine (char *line)
{
	char *tp;

	strtok (line, "@\n");

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.aligned = atoi(tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.normicon = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		TextFont.show.size = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		TextFont.show.date = atoi (tp);

	if((tp = strtok (NULL, "@\n")) != NULL)
		TextFont.show.time = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.showtext = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.sortentry = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.emptyPaper = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.waitKey = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.silentCopy = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.replaceExisting = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_cols = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_rows = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_inv = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_font = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_fsize = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_wx = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_wy = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.silentRemove = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.showHidden = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.askQuit = atoi (tp);

	if ((tp = strtok(NULL, "@\n")) != NULL)
		NewDesk.saveState = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.ovlStart = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.silentGINRead = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.dont_save_gin_file = atoi (tp);
	else
		ShowInfo.dont_save_gin_file = 1;

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.format_info = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.snapAlways = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.useCopyKobold = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.useDeleteKobold = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.useFormatKobold = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.minCopyKobold = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.minDeleteKobold = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		NewDesk.preferLowerCase = atoi (tp);

	SetShowInfo ();
	SetInMWindow (&ShowInfo, FALSE);
}


static void execScaleFactor (char *line)
{
	char *tp;

	strtok (line, "@\n");

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ScaleFactor = atol (tp);
	
	if (ScaleFactor <= 0)
		ScaleFactor = 1000L;
}


ConfInfo *execLine (ConfInfo *aktconf, char *tmpline, int *ok, 
	int *got_deficons, int *got_iconrules, const char *filename)
{
	if (tmpline[0] == '#')
	{
		switch (tmpline[1])
		{
			case 'W':				/* Window */
			case 'P':				/* ProgramIcon */
				aktconf->nextconf = 
						(ConfInfo *)malloc(sizeof(ConfInfo));
				if (aktconf->nextconf)
				{
					aktconf->nextconf->line = 
						malloc (strlen (tmpline) + 1);
					if (aktconf->nextconf->line)
					{
						aktconf = aktconf->nextconf;
						strcpy (aktconf->line, tmpline);
						aktconf->nextconf = NULL;
					}
					else		/* malloc failed */
					{
						free (aktconf->nextconf);
						aktconf->nextconf = NULL;
					}
				}
				break;

			case 'A':		/* Applicationrules */
				AddApplRule (tmpline);
				break;

			case 'B':			/* windowbox */
				execBoxInfo (tmpline);
				break;

			case 'C':
				ExecConsoleInfo (tmpline);
				break;
				
			case 'D':				/* Driveicon */
				ExecDriveInfo (tmpline);
				break;

			case 'E':				/* Eraser */
				*got_deficons = execShredderInfo (tmpline);
				break;

			case 'F':
				execFontInfo (tmpline);
				break;

			case 'G':
				/* Newline abschneiden
				 */
				tmpline[strlen (tmpline) - 1] = '\0';
				strncpy (ShowInfo.gin_name, &tmpline[2], 8);
				ShowInfo.gin_name[8] = '\0';
				break;
			
			case 'H':		/* Desktop-Hintergrund */
				SetDeskState (tmpline);
				break;
				
			case 'I':
				execInfoLine (tmpline);
				break;

			case 'K':
				execScaleFactor (tmpline);
				break;
				
			case 'M':
				ExecPatternInfo (tmpline);
				break;

			case 'R':		/* Iconrule */
				AddIconRule (tmpline);
				*got_iconrules = TRUE;
				break;

			case 'S':				/* Scrapicon */
				*got_deficons = execScrapInfo (tmpline);
				break;

			case 'T':				/* Trashcan */
				*got_deficons = execTrashInfo (tmpline);
				break;

			case 'V':
				tmpline[strlen (tmpline) - 1] = '\0';
				ExecAccInfo (tmpline);
				break;

			default:
				venusErr (NlsStr (T_FORMAT), filename);
				*ok = FALSE;
				break;
		}
	}
	
	return aktconf;
}

int ReadInfoFile (const char *inf_name, const char *gin_name, 
	word *tmp)
{
	MEMFILEINFO *mp;
	ConfInfo *aktconf, myconfig;
	word got_deficons, got_iconrules;
	MFORM form;
	char filename[MAX_PATH_LEN];
	char *home;
	word num, ok = TRUE;
	int broken = FALSE;
	int tmpgin;
	
	GrafGetForm (&num, &form);
	GrafMouse (HOURGLASS, NULL);

	/* free (existing?) list
	 */
	freeConfInfo ();
	myconfig.nextconf = NULL;
	aktconf = &myconfig;
	*tmp = tmpgin = FALSE;

	/* Wo suchen wir?
	 */
	home = GetEnv (MupfelInfo, "HOME");
	if (home == NULL)
		home = BootPath;
	else
	{
		strcpy (filename, home);
		AddFileName (filename, "gin");
		if (!IsDirectory (filename, &broken))
			home = BootPath;
	}
	
	/* Skalierung auf Default setzen
	 */
	ScaleFactor = 1000L;
	
	if (inf_name != NULL)
	{
		char tmpline[CONF_LEN];
		
		strcpy (filename, home);
		AddFileName (filename, inf_name);
		strcat (filename, ".tmp");
		
		*tmp = access (filename, A_READ, &broken);
		
		if (!*tmp)
		{
			char *env_name;
			
			env_name = GetEnv (MupfelInfo, "GEMINI_INF");
			if (env_name != NULL)
				strcpy (filename, env_name);
			else
			{
				strcpy (filename, home);
				AddFileName (filename, inf_name);
				strcat (filename, ".inf");
			}
		}
		
		got_deficons = got_iconrules = FALSE;
	
		if ((mp = mopen (pMainAllocInfo, filename, NULL)) != NULL)
		{
			while (ok && (mgets (tmpline, CONF_LEN, mp) != NULL))
			{
				tmpline[CONF_LEN - 1] = '\0';
				aktconf = execLine (aktconf, tmpline, &ok, 
					&got_deficons, &got_iconrules, filename);
			}
			
			mclose (pMainAllocInfo, mp);
			
			if (*tmp)
				delFile (filename);
		}
		else
			ok = FALSE;
	}
	else
	{
		/* Wir lesen nicht die INF-Datei, sondern nur die GIN-Datei.
		 * Daher nehmen wir an, daž die Anzeige-Regeln schon da sind.
		 */
		got_iconrules = TRUE;
	}

	if (inf_name != NULL)
	{
		strcpy (filename, home);
		AddFolderName (filename, "gin");
		AddFileName (filename, "temp.gin");
		
		tmpgin = access (filename, A_EXIST, &broken);
		*tmp |= tmpgin;
		
		if (!tmpgin)
		{
			char res_name[MAX_FILENAME_LEN];
			char *env_name;
			
			env_name = GetEnv (MupfelInfo, "GEMINI_GIN");
			if (env_name != NULL)
			{
				char tmp[MAX_FILENAME_LEN];
		
				strcpy (filename, env_name);
				GetBaseName (env_name, tmp, sizeof (tmp));
				strncpy (ShowInfo.gin_name, tmp, 8);
			}
			else
			{
				strcpy (filename, home);
				AddFolderName (filename, "gin");
				sprintf (res_name, "%04d%04d", Desk.g_x + Desk.g_w,
					Desk.g_y + Desk.g_h);
				AddFileName (filename, res_name);
				strcat (filename, ".gin");
			}
			
			if (!access (filename, A_READ, &broken))
			{
				strcpy (filename, home);
				AddFolderName (filename, "gin");
				AddFileName (filename, gin_name);
				strcat (filename, ".gin");
			}
		}
	}
	else
	{
		strcpy (filename, gin_name);
	}

	/* Skalierung auf Default setzen
	 */
	ScaleFactor = 1000L;

	if ((mp = mopen (pMainAllocInfo, filename, NULL)) != NULL)
	{
		char tmpline[CONF_LEN];

		ok = TRUE;
		if ((mgets (tmpline, CONF_LEN, mp) == NULL)
			|| strnicmp (tmpline, "GIN", 3))
		{
			venusErr (NlsStr (T_NO_GIN), filename);
			ok = FALSE;
		}
		
		while (ok && (mgets (tmpline, CONF_LEN, mp) != NULL))
		{
			tmpline[CONF_LEN - 1] = '\0';
			aktconf = execLine (aktconf, tmpline, &ok, 
				&got_deficons, &got_iconrules, filename);
		}
		
		mclose (pMainAllocInfo, mp);

	}
	else
		ok = FALSE;

	/* tempor„re Datei
	 */
	if (tmpgin)
		delFile (filename);
	
	if (!got_iconrules)
		InsDefIconRules ();
		
	/* Hashtable for Icon Rules
	 */
	BuiltIconHash ();

	if (!got_deficons)
		AddDefIcons ();

	firstconf = myconfig.nextconf;
	GrafMouse (num, &form);
	
	return ok;
}


static void execWindLine (const char *line)
{
	GRECT wind;
	word slpos, kind;
	char wpath[MAX_PATH_LEN], title[MAX_PATH_LEN];
	char wcard[WILDLEN], label[MAX_FILENAME_LEN], *cp, *tp;
	
	cp = malloc (strlen (line) + 1);
	if (cp == NULL)
		return;

	/* default
	 */
	memset (&wind, 0, sizeof (GRECT));
	slpos = 0;
	kind = WK_FILE;

	strcpy (cp, line);
	strtok (cp, "@\n");			/* skip first token */
	
	if ((tp = strtok (NULL, "@\n")) != NULL)
		wind.g_x = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		wind.g_y = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		wind.g_w = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		wind.g_h = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		slpos = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		kind = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		strcpy (wpath, tp);
	else
	{
		free (cp);
		return;
	}

	if ((tp = strtok (NULL, "@\n")) != NULL)
		strcpy (wcard, tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		strcpy (title, tp);
	else
		*title = '\0';

	if ((tp = strtok (NULL, "@\n")) != NULL)
		strcpy (label, tp);
	else
		*label = '\0';

	free (cp);

	wind.g_x = checkPromille (wind.g_x, 0);
	wind.g_y = checkPromille (wind.g_y, 0);
	wind.g_x = Desk.g_x + (word)scale123 (Desk.g_w, wind.g_x, ScaleFactor);
	wind.g_w = Desk.g_x + (word)scale123 (Desk.g_w, wind.g_w, ScaleFactor);
	wind.g_y = Desk.g_y + (word)scale123 (Desk.g_h, wind.g_y, ScaleFactor);
	wind.g_h = Desk.g_y + (word)scale123 (Desk.g_h, wind.g_h, ScaleFactor);

	PushWindBox (&wind, kind);
	switch (kind)
	{
		case WK_CONSOLE:
#if MERGED
			TerminalWindowOpen (wpath, title, TRUE);
#endif
			break;
	
		case WK_FILE:
			FileWindowOpen (wpath, label, title, wcard, slpos);
			break;
	}
}

static void execPrgLine (const char *line)
{
	word obx, oby, normicon, isfolder, ok;
	word shortcut = 0;
	char path[MAX_PATH_LEN], name[MAX_FILENAME_LEN];
	char label[MAX_FILENAME_LEN], *cp, *tp;
	
	cp = malloc (strlen (line) + 1);

	if (cp == NULL)
		return;

	ok = TRUE;
	
	obx = oby = 0;
	strcpy (cp, line);
	strtok (cp, "@\n");		/* skip first */
	
	if ((tp = strtok (NULL, "@\n")) != NULL)
		obx = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		oby = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		normicon = atoi (tp);
		
	if((tp = strtok (NULL, "@\n")) != NULL)
		isfolder = atoi (tp);
		
	if ((tp = strtok (NULL, "@\n")) != NULL)
		strcpy (path, tp);
	else
		ok = FALSE;
					
	if ((tp = strtok (NULL, "@\n")) != NULL)
		strcpy (name, tp);
	else
		ok = FALSE;

	if ((tp = strtok (NULL, "@\n")) != NULL)
	{
		strcpy (label, tp);
		if (!strcmp (label, " "))
			*label = '\0';
	}
	else
		*label = '\0';
	
	if ((tp = strtok (NULL, "@\n")) != NULL)
		shortcut = atoi (tp);
	
	free (cp);

	if (!ok)
		return;
		
	obx = checkPromille (obx, 0);
	oby = checkPromille (oby, 0);
	obx = (word)scale123 (Desk.g_w, obx, ScaleFactor);
	oby = (word)scale123 (Desk.g_h, oby, ScaleFactor);

	InstPrgIcon (obx, oby, FALSE, normicon, isfolder,
					path, name, label, shortcut);
}

void ExecConfInfo (word todraw)
{
	ConfInfo *aktconf;
	word num;
	MFORM form;
	
	GrafGetForm (&num, &form);
	GrafMouse (HOURGLASS, NULL);
	
	aktconf = firstconf;
	
	while (aktconf != NULL)
	{
		if (aktconf->line[0] == '#'
			&& aktconf->line[1] == 'P')
		{
			execPrgLine (aktconf->line);
		}

		aktconf = aktconf->nextconf;
	}

	if (todraw)
	{
		form_dial (FMD_FINISH, 0, 0, 0, 0,
			Desk.g_x, Desk.g_y, Desk.g_x + Desk.g_w, 
			Desk.g_y + Desk.g_h);
	}
	
	aktconf = firstconf;
	
	/* Arbeite alle Nachrichten, die bis hierhin angefallen sind
	 * ab. Ich hoffe, daž ich auch alle VA_PROTO, etc. hier schon
	 * bekommen, damit die Accs ihre Fenster zuerst ”ffnen.
	 */
	HandlePendingMessages (TRUE);
	
	while (aktconf)
	{
		if ((aktconf->line[0] == '#')
			&& (aktconf->line[1] == 'W'))
		{
			execWindLine (aktconf->line);
		}
		
		aktconf = aktconf->nextconf;
	}

	/* liste wieder freigeben
	 */
	freeConfInfo ();
	
	/* Windows sind alle uptodate, also kein Update wegen dirty
	 */
	if (pDirtyDrives)
		*pDirtyDrives = 0;
	
	GrafMouse (num, &form);
}


static ConfInfo *makeConfWp (ConfInfo *aktconf, WindInfo *wp,
					char *buffer)
{
	if (wp != NULL)
	{
		aktconf = makeConfWp (aktconf, wp->nextwind, buffer);
		
		if (wp->window_get_statusline)
			wp->window_get_statusline (wp, buffer);
		else
			buffer[0] = '\0';

		if (buffer[0] != '\0')
		{
			aktconf->nextconf = (ConfInfo *)malloc(sizeof(ConfInfo));
			if (aktconf->nextconf != NULL)
			{
				aktconf->nextconf->line = malloc (strlen (buffer) + 1);
				if(aktconf->nextconf->line)
				{
					aktconf = aktconf->nextconf;
					aktconf->nextconf = NULL;
					strcpy (aktconf->line, buffer);
				}
				else
				{							/* malloc failed */
					free (aktconf->nextconf);
					aktconf->nextconf = NULL;
				}
				
			}
		}
	}
	
	return aktconf;
}

/* Buffer fr sprintf()
 */
#define BUFFER_LEN		1024

/* Schreibe die aktuelle Konfiguration in ConfInfo-Strukturen
 * Mittlerweile degeneriert zur Speicherung des Fensterinformationen
 */
int MakeConfInfo (void)
{
	ConfInfo myconfig;
	word num;
	MFORM form;
	char buffer[BUFFER_LEN];
	
	GrafGetForm (&num, &form);
	GrafMouse (HOURGLASS, NULL);

	/* free (existing?) list
	 */
	freeConfInfo ();
	myconfig.nextconf = NULL;

	/* Skalierung auf neue Gr”že setzen
	 */
	ScaleFactor = NEW_SCALE_FACTOR;

	makeConfWp (&myconfig, wnull.nextwind, buffer);

	firstconf = myconfig.nextconf;

	GrafMouse (num, &form);
	
	return TRUE;
}



int WriteInfoFile (const char *inf_name, const char *gin_name, 
	word update)
{
	MEMFILEINFO *mp;
	char *buffer, *home, *env_name;
	char filename[MAX_PATH_LEN];
	word num;
	MFORM form;
	int ok = TRUE;
	
	GrafGetForm (&num, &form);
	GrafMouse (HOURGLASS, NULL);

	home = GetEnv (MupfelInfo, "HOME");
	if (home == NULL)
		home = BootPath;
	else
	{
		int broken;
		
		strcpy (filename, home);
		AddFileName (filename, "gin");
		if (!IsDirectory (filename, &broken))
			home = BootPath;
	}

	env_name = GetEnv (MupfelInfo, "GEMINI_INF");
	if (env_name != NULL)
	{
		strcpy (filename, env_name);
	}
	else
	{
		strcpy (filename, home);
		AddFileName (filename, inf_name);
	}
	
	buffer = malloc (BUFFLEN);
	if (!buffer)
	{
		venusErr (NlsStr (T_NOBUFFER));
		return FALSE;
	}
	
	/* Skalierung auf neue Gr”že setzen
	 */
	ScaleFactor = NEW_SCALE_FACTOR;
	MakeConfInfo ();
	
	if ((mp = mcreate (pMainAllocInfo, filename)) != NULL)
	{
#if MERGED
		getInMWindow (&ShowInfo);
#endif
		sprintf (buffer, "#I@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d"
				"@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d"
				"@%d@%d@%d@%d@%d@%d",
				ShowInfo.aligned, 
				ShowInfo.normicon, TextFont.show.size, 
				TextFont.show.date, TextFont.show.time, 
				ShowInfo.showtext, ShowInfo.sortentry,	
				NewDesk.emptyPaper, NewDesk.waitKey, 
				NewDesk.silentCopy, NewDesk.replaceExisting, 
				ShowInfo.m_cols, ShowInfo.m_rows, ShowInfo.m_inv, 
				ShowInfo.m_font, ShowInfo.m_fsize, 
				ShowInfo.m_wx, ShowInfo.m_wy, 
				NewDesk.silentRemove, NewDesk.showHidden,
				NewDesk.askQuit, NewDesk.saveState, 
				NewDesk.ovlStart, NewDesk.silentGINRead,
				ShowInfo.dont_save_gin_file, NewDesk.format_info,
				NewDesk.snapAlways, NewDesk.useCopyKobold,
				NewDesk.useDeleteKobold, NewDesk.useFormatKobold,
				NewDesk.minCopyKobold, NewDesk.minDeleteKobold,
				NewDesk.preferLowerCase);
		
		ok = (mputs (mp, buffer) == 0L);
		
		if (ok)
		{
			sprintf (buffer, "#K@%ld", ScaleFactor);
			ok = (mputs (mp, buffer) == 0L);
		}
		
		if (ok)
			ok = WriteDisplayRules (mp, buffer);
		if (ok)
			ok = WriteApplRules (mp, buffer);
		if (ok)
			ok = WriteWildPattern (mp, buffer);
		
		mclose (pMainAllocInfo, mp);
		
		if (!ok)
			venusErr (NlsStr (T_ERROR), filename);
		else if (gin_name != NULL)
		{
			int broken;
			char *env_name;
			
			env_name = GetEnv (MupfelInfo, "GEMINI_GIN");
			if (env_name != NULL)
			{
				char tmp[MAX_FILENAME_LEN];
		
				strcpy (filename, env_name);
				GetBaseName (env_name, tmp, sizeof (tmp));
				strncpy (ShowInfo.gin_name, tmp, 8);
			}
			else
			{
				strcpy (filename, home);
				if (IsDirectory (filename, &broken))
				{
					AddFileName (filename, "gin");
					if (!IsDirectory (filename, &broken) && !broken)
					{
						CreateFolder (filename);
					}
				}
				
				AddFileName (filename, gin_name);
				strcat (filename, ".gin");
			}
			
			/* Skalierung auf neue Gr”že setzen */
			ScaleFactor = NEW_SCALE_FACTOR;

			if ((mp = mcreate (pMainAllocInfo, filename)) != NULL)
			{
				ConfInfo *aktconf;
		
				strcpy (buffer, "GIN: ");
				strcat (buffer, venusVersionString);
				ok = (mputs (mp, buffer) == 0L);
				
				/* Ab hier kommen die aufl”sungsabh„ngigen Teile, 
				   die in einer anderen Datei stehen. */
				if (ok)
				{
					sprintf (buffer, "#K@%ld", ScaleFactor);
					ok = (mputs (mp, buffer) == 0L);
				}

				if (ok)
				{
					strcpy (buffer, "#G");
					strcat (buffer, ShowInfo.gin_name);
					ok = (mputs (mp, buffer) == 0L);
				}
				if (ok)
					ok = WriteAccInfos (mp, buffer);
				if (ok)
					ok = WriteFontInfo (mp, buffer);
				if (ok)
					ok = WriteDeskState (mp, buffer);
				if (ok)
					ok = WriteConsoleInfo (mp, buffer);
				if (ok)
					ok = WriteDeskIcons (mp, buffer);
				if (ok)
					ok = WriteDriveInfos (mp, buffer);
				if (ok)
					ok = WriteBoxes (mp, buffer);
		
				aktconf = firstconf;
				while (ok && aktconf != NULL)
				{
					ok = (mputs (mp, aktconf->line) == 0L);
					aktconf = aktconf->nextconf;
				}

				mclose (pMainAllocInfo, mp);
		
				if (!ok)
					venusErr (NlsStr (T_ERROR), filename);
			}
			else
			{
				venusErr (NlsStr (T_CREATE), filename);
				ok = FALSE;
			}
		}

		if (update)
			BuffPathUpdate (BootPath);
	}
	else
	{
		venusErr (NlsStr (T_CREATE), filename);
		ok = FALSE;
	}

	freeConfInfo ();
	free (buffer);

	GrafMouse (num, &form);
	return ok;
}
