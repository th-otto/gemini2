/*
 * @(#) Gemini\Venus.c
 * @(#) Stefan Eissing, 06. MÑrz 1994
 *
 * description: main modul for the desktop venus
 *
 * jr 19.5.95
 */

#include <vdi.h>
#include <stdio.h>
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\evntevnt.h>
#include <nls\nls.h>
#include <tos.h>

#include "..\common\alloc.h"
#include "..\common\cookie.h"
#include "..\common\fileutil.h"

#include "vs.h"

#include "venus.h"
#include "menu.h"
#include "window.h"
#include "conwind.h"
#include "fileutil.h"
#include "filewind.h"
#include "redraw.h"
#include "scroll.h"
#include "init.h"
#include "venuserr.h"
#include "myalloc.h"
#include "keylogi.h"
#include "util.h"
#include "infofile.h"
#include "erase.h"
#include "iconinst.h"
#include "drives.h"
#include "iconrule.h"
#include "stand.h"
#include "windstak.h"
#include "greeting.h"
#include "filedraw.h"
#include "overlay.h"
#include "message.h"
#if MERGED
#include "mvwindow.h"
#endif
#include "wildcard.h"
#include "deskwind.h"
#include "applmana.h"
#include "gemini.h"
#include "timeevnt.h"

#include "..\mupfel\mupfel.h"
#include "..\mupfel\mglob.h"


char venusSCCSid[] = "@(#)"PGMNAME", Copyright (c) S. Eissing "__DATE__;
char venusVersionString[] = "Gemini "MUPFELVERSION"  "__DATE__;

/* Wie angezeigt wird
 */
SHOWINFO ShowInfo =
{	1, 1, 1, 0, 0, TRUE, SORTNAME, 80, 24, 1, 1, 6, 20, 20 };

/* Desktophintergrund
 * und allgemeines
 */
DeskInfo NewDesk;

char BootPath[MAX_PATH_LEN];					/* Pfad beim Programmstart */

word apid;								/* Application Id */
GRECT Desk;								/* Desktopkoordinaten */
word wchar, hchar, wbox, hbox;			/* Charakterinformationen */

word phys_handle;						/* Handle von graf_handle */
word vdi_handle;						/* eigener VDI-Handle */
word MultiAES;							/* Ataris MultiTOS ist da */
word number_of_settable_colors;
word number_of_planes;
unsigned long number_of_colors;
long ScaleFactor;


OBJECT *pmenu,*pcopyinfo,*pstdbox;
OBJECT *pshowbox,*pwildbox, *pconselec;
OBJECT *prulebox,*pinstbox,*prenamebox;
OBJECT *pcopybox,*pnamebox,*perasebox;
OBJECT *pfoldbox,*pfileinfo,*pfoldinfo;
OBJECT *pdrivinfo,*papplbox,*pttpbox;
OBJECT *pchangebox,*pshredbox,*ptrashbox;
OBJECT *pscrapbox,*piconfile,*ppopopbox;
OBJECT *pfrmtbox,*pspecinfo,*poptiobox;
OBJECT *pinitbox,*popendbox,*pdivbox;
OBJECT *pfontbox, *pnogdos, *pweditbox;
OBJECT *pruleedit, *pappledit, *pshortcut;
OBJECT *pcolorbox, *pfmtbox, *pnewdesk;
OBJECT *ptotalinf, *pginsave, *psetdesk;
OBJECT *ppattbox, *pdrivesel, *pswitchoff;

/* Pointer zur Variable der Mupfel, die anzeigt, welche Laufwerke
 * "dirty" sind.
 */
unsigned long *pDirtyDrives;

/* TRUE, wenn die Mupfel ihren Prompt neu anzeigen soll
 */
int MupfelShouldDisplayPrompt;

/* Pointer, der an Mupfel-Funktionen Åbergeben werden muû
 */
void *MupfelInfo;
/* Pointer zur AllocInfo von MupfelInfo
 */
void *pMainAllocInfo;


/* internal texts
 */
#define NlsLocalSection "G.Venus.c"
enum NlsLocalText{
T_CONWINDOW,	/*Kann das Console-Fenster nicht initialisieren!*/
T_MUPFEL,	/*Es ist keine Mupfel installiert! Operationen 
wie Kopieren oder Lîschen sind nicht mîglich!*/
T_NORSC,		/*Konnte die Resource-Datei `%s' nicht laden!*/
T_RSC,		/*Es ist ein Fehler in der Resource-Datei `%s' aufgetreten!*/
T_APPLINIT,	/*[3][Das AES scheint mich nicht zu mîgen!][Abbruch]*/
T_VDI,		/*Kann keine virtuelle Workstation îffnen!*/
T_BUTT,		/*[Oh*/
T_ASKQUIT,	/*Wollen Sie Gemini wirklich verlassen?*/
T_NOTFOUND,	/*Konnte die Datei `%s' nicht finden. Aktion wird 
abgebrochen.*/
T_GIN_READ,	/*Beim Einlesen von `%s' werden alle Fenster und Icons
 entfernt und durch die Informationen in dieser Datei ersetzt.
 Wollen Sie dies wirklich?*/
};


/* overlay exit
 */
static int its_an_overlay = FALSE;

/* Pointer fÅrs Pasten in das Console-Fenster
 */
const char *paste_string = NULL;
const char *paste_pointer = NULL;


static void updateDirtyDrives (void)
{
	unsigned long dirty;
	word i;
	char path[4];
	
	if (pDirtyDrives == NULL)
		return;
		
	dirty = *pDirtyDrives;
	
	if (!dirty)
		return;
		
	strcpy (path, "A:\\");
	
	for (i = 0; i < 26; ++i)
	{
		path[0] = 'A' + (char)i;
		if (LegalDrive (path[0]) && (dirty & 1))
			BuffPathUpdate (path);
		dirty = dirty >> 1;
	}
	
	FlushPathUpdate ();
	*pDirtyDrives = 0;
}


void PasteStringToConsole (const char *line)
{
	char *new_string;
	
	new_string = malloc ((paste_string? strlen (paste_string) : 0)
			+ strlen (line) + 1);
	if (paste_string)
		strcpy (new_string, paste_string);
	else
		new_string[0] = '\0';
		
	strcat (new_string, line);
	free (paste_string);
	paste_string = new_string;
	paste_pointer = paste_string;
}


/* kÅmmere dich um alle Haupt-Events und liefere Zeichen fÅr die
 * Mupfel an diese zurÅck. Wenn Gemini beendet werden soll, wird
 * TRUE zurÅckgeliefert.
 */
int GetCharFromGemini (int *key_code, int *reinit_line)
{
	static MEVENT E;
	word evnttype;
	int silent = 0;
	word messbuff[8];
	word done, got_key, fwind;
	WindInfo *wp;

	GrafMouse (M_ON, NULL);
	
	E.e_flags = MU_BUTTON | MU_MESAG | MU_KEYBD;
	E.e_time = 0L;
	E.e_bclk = 2;
	E.e_bmsk = 1;
	E.e_bst = 1;
	E.e_mepbuf = messbuff;
	GrafMouse (ARROW, NULL);

	*reinit_line = FALSE;
	done = got_key = FALSE;
	
	while (!done && !got_key)
	{
		updateDirtyDrives ();

		UpdateAllWindowData ();
		ManageMenu ();			/* (de)select Menuentries */

		if (MupfelShouldDisplayPrompt)
		{
			MupfelShouldDisplayPrompt = FALSE;
			*reinit_line = TRUE;
			got_key = TRUE;
			continue;
		}
		
		if (paste_string != NULL)
		{
			unsigned char c;
				
			c = *paste_pointer++;
			if (c)
			{
				*key_code = c;
				got_key = TRUE;
				continue;
			}
			else
			{
				free (paste_string);
				paste_string = paste_pointer = NULL;
			}
		}
		
		EnableTimerEvent (&E.e_flags, &E.e_time);
		
		WindUpdate (END_UPDATE);

		evnttype = evnt_event (&E);

		WindUpdate (BEG_UPDATE);

		wind_get (0, WF_TOP, &fwind);
		IsTopWindow (fwind);
		
		if (evnttype & MU_MESAG)
		{
			done = !HandleMessage (messbuff, E.e_ks, &silent);
		}

		if (!done)
		{
			if (evnttype & MU_BUTTON)
			{
				fwind = wind_find (E.e_mx, E.e_my);
				wp = GetWindInfo (fwind);
				if (wp != NULL && wp->mouse_click != NULL)
				{
					wp->mouse_click (wp, E.e_br, E.e_mx, 
						E.e_my, E.e_ks);
				}
			}
			
			/* Tastatur-Nachricht simulieren?
			 */
			if ((evnttype & MU_MESAG) && (messbuff[0] == 0x4710))	
			{
				evnttype |= MU_KEYBD;
				E.e_ks = messbuff[3];
				E.e_kr = messbuff[4];
			}
			
			if (!got_key && (evnttype & MU_KEYBD))
			{
				done = !doKeys (E.e_kr, E.e_ks, &got_key, &silent);
				if (got_key)
					*key_code = E.e_kr;
			}
			
			if (evnttype & MU_TIMER)
			{
				RaisedTimerEvent ();
			}
		}
		
		if (doOverlay (&its_an_overlay))
		{
			done = TRUE;
		}
		else if (done && !silent && NewDesk.askQuit)
			done = venusChoice (NlsStr (T_ASKQUIT), PGMNAME);
	}

	GrafMouse (M_OFF, NULL);
	return done;
}


static struct
{
	unsigned flydials : 1;
	unsigned nls : 1;
	unsigned vdi : 1;
	unsigned icons : 1;
	unsigned resource : 1;
	unsigned console : 1;
	unsigned other_stuff : 1;
} Initialized;


int EarlyInitGemini (void *mupfelInfo, void *allocInfo, int v_handle, 
	unsigned long *pdirty_drives, const char *function_keys[20])
{
	apid = _GemParBlk.global[2];
	if (apid == -1)
	{
		form_alert (1, NlsStr (T_APPLINIT));
		return FALSE;
	}

	MultiAES = (_GemParBlk.global[1] == -1);
	
	MupfelInfo = mupfelInfo;
	pMainAllocInfo = allocInfo;
	pDirtyDrives = pdirty_drives;
	FunctionKeys = function_keys;

	InitMainMalloc (allocInfo);
	
	if (!Initialized.flydials)
	{
		extern int _invert_edit;

		_invert_edit = 1;

		DialInit (malloc, free);
		Initialized.flydials = 1;
	}

	Greetings ();

	if (!Initialized.nls)
	{
		int ok;
		
		ok = getBootPath (BootPath, MSGFILE);
		
		if (ok)
		{
			GrafMouse (HOURGLASS, NULL);

			AddFileName (BootPath, MSGFILE);
			ok = NlsInit (BootPath, NlsDefaultSection,
				malloc, free);
			StripFileName (BootPath);
		}
		
		if (!ok)
		{
			form_alert (1, "[1][Cannot load GEMINI.MSG!][ OK ]");
			return FALSE;
		}
		Initialized.nls = 1;
	}

	if (!Initialized.vdi)
	{
		if (!InitVDI (v_handle))
		{
			DialAlert (ImSqExclamation(), NlsStr (T_VDI),
					0, NlsStr (T_BUTT));
			return FALSE;
		}
		Initialized.vdi = 1;
	}

	if (!Initialized.icons)
	{
		int ok = FALSE;
		
		AddFileName (BootPath, ICONRSC);
		ok = InitIcons (BootPath);
		StripFileName (BootPath);
			
		if (!ok)
		{
			venusErr (NlsStr (T_NORSC), ICONRSC);
			return FALSE;
		}
		
		Initialized.icons = 1;
	}

	Greetings ();

	if (!Initialized.resource)
	{
		int ok;
		
		AddFileName (BootPath, MAINRSC);
		ok = rsrc_load (BootPath);
		StripFileName (BootPath);
		
		if (ok)
			ok = GetTrees ();
		
		if (!ok)
		{
			venusErr (NlsStr (T_RSC), MAINRSC);
			return FALSE;
		}
		Initialized.resource = 1;
	}

	Greetings ();

	if (!Initialized.console)
	{
		if (!ConsoleInit ())
		{
			DialAlert (NULL, NlsStr (T_CONWINDOW), 0, NlsStr (T_BUTT));
			return FALSE;
		}
		SetInMWindow (&ShowInfo, FALSE);
		Initialized.console = 1;
	}

	InitTimerEvents ();
		
	return TRUE;
}


extern int shel_wdef (char *cmd, char *dir);

int LateInitGemini (void)
{
	Greetings ();

	if (MagiCVersion)
	{
		shel_wdef (pgmname".app", BootPath);
	}
	
	if (!Initialized.other_stuff)
	{
		word tempinfo;

		initFileFonts ();
		/* bearbeite die geladenen Icons */
		FixIcons ();
		DeskWindowOpen ();
		menu_bar (pmenu, TRUE);
		if (MultiAES) menu_register (apid, "  Gemini");

		/* We know about AP_TERM */
		{
			int event = 0, dummy;
			
			appl_xgetinfo (10, &event, &dummy, &dummy, &dummy);
			if ((event & 0xff) >= 9 || MagiCVersion >= 0x300)
			{
				shel_write (9, 1, 0, NULL, NULL);
			}
		}
		
		ReadInfoFile (pgmname, pgmname, &tempinfo);
		UpdateDrives (TRUE, FALSE);
		ExecConfInfo (TRUE);

		MessInit ();

		if (!tempinfo)
			MupVenus (AUTOEXEC);

		Initialized.other_stuff = 1;
	}

	HandlePendingMessages (FALSE);
	GrafMouse (M_OFF, NULL);

	MupfelShouldDisplayPrompt = FALSE;
	
	return TRUE;
}


void ExitGemini (int mupfel_doing_overlay)
{
	ExitGreetings ();

	if (MagiCVersion && !mupfel_doing_overlay)
	{
 		shel_wdef ("", "");
	}
	
	if (paste_string != NULL)
	{
		free (paste_string);
		paste_string = paste_pointer = NULL;
	}
	
	if (Initialized.other_stuff)
	{
		GrafMouse (M_ON, NULL);

		if (!mupfel_doing_overlay && !its_an_overlay 
			&& NewDesk.emptyPaper)
		{
			emptyPaperbasket ();
		}

		if (NewDesk.saveState)
			WriteInfoFile (pgmname".INF", pgmname, FALSE);
					
		menu_bar (pmenu, FALSE);
		MessExit ();
		CloseAllWind (TRUE);
		DeskWindowClose (TRUE);
		WindUpdate (END_UPDATE);

		FreeIconRules ();
		FreeApplRules ();
		FreeWBoxes ();

		exitFileFonts ();
		Initialized.other_stuff = 0;
	}
	
	ExitTimerEvents ();
	
	if (Initialized.console)
	{
		ConsoleExit ();
		Initialized.console = 0;
	}

	if (Initialized.resource)
	{
		rsrc_free ();
		Initialized.resource = 0;
	}

	if (Initialized.icons)
	{
		ExitIcons ();
		Initialized.icons = 0;
	}
		
	if (Initialized.vdi)
	{
		ExitVDI ();
		Initialized.vdi = 0;
	}

	if (Initialized.nls)
	{
		NlsExit ();
		Initialized.nls = 0;
	}

	if (Initialized.flydials)
	{
		DialExit ();
		Initialized.flydials = 0;
	}

	SwitchResolution ();
	
	ExitMainMalloc ();

	FunctionKeys = MupfelInfo = pDirtyDrives = NULL;
}


#if STANDALONE
word venus (void)
{
	(void)venusSCCSid;

	if (!EarlyInitGemini (NULL, 0, 1, NULL) || !LateInitGemini ())
	{
		ExitGemini ();
		return 1;
	}

	multi_event ();

	ExitGemini ();
	
	return 0;
}
#endif


int ExecGINFile (const char *filename)
{
	int dummy, broken;
	
	if (!access (filename, A_READ, &broken))
	{
		venusErr (NlsStr (T_NOTFOUND), filename);
		return FALSE;
	}
	else if (!NewDesk.silentGINRead 
		&& !venusChoice (NlsStr (T_GIN_READ), filename))
	{
		return FALSE;
	}
	
	CloseAllWind (FALSE);
	DeskWindowClose (FALSE);
	FreeWBoxes ();

	DeskWindowOpen ();
	ReadInfoFile (NULL, filename, &dummy);
	
	UpdateDrives (TRUE, TRUE);
	ExecConfInfo (TRUE);
	
	return TRUE;
}