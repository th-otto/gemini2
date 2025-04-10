/*
 * @(#) Gemini\Keylogi.c
 * @(#) Stefan Eissing, 06. M„rz 1995
 *
 * description: functions to dispatch keyboard events
 *
 * jr 7.10.95
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"

#include "vs.h"
#include "menu.rh"

#include "window.h"
#include "conwind.h"
#include "dialog.h"
#include "keylogi.h"
#include "filewind.h"
#include "fileutil.h"
#include "menu.h"
#include "redraw.h"
#include "scancode.h"
#include "scroll.h"
#include "scut.h"
#include "util.h"
#include "venuserr.h"
#include "message.h"



/* externals
 */
extern OBJECT *pmenu;

/* internal texts
 */
#define NlsLocalSection "G.keylogi"
enum NlsLocalText{
T_HELP,		/*Hilfe zu den einzelnen Funktionen finden Sie in 
den Dokumenten GEMINI.DOC, VENUS.DOC und MUPFEL.DOC. Falls diese 
Dateien fehlen, sollten Sie sich noch einmal ein komplettes
 Gemini besorgen.*/
T_NODRIVE,	/*Das Laufwerk %c: ist GEMDOS nicht bekannt!*/
T_FILEOPEN,	/*Auf diesem Laufwerk sind noch Dateien offen. Daher
 wird kein echter Medienwechsel erzwungen, es werden nur
 die Daten neu eingelesen.*/
};

/* internals
 */
const char **FunctionKeys;
 
typedef struct
{
	word key;				/* ausl”sender Key */
	word mess0;				/* Nachrichtennummer */
	word mess3;				/* Nachrichtenwort 3 */
	word mess4;				/* dito			   4 */
	char killrep;			/* number of further kills of this key */
	char console;			/* ist auch fr console fenster */
	char filewind;			/* ist auch fr datei fenster */
} KeyMessage;

/*
 * Strukturen mit Shortcuts fr Alternate-, Control- und ohne diese
 * beiden Tasten.
 */
static KeyMessage altShortCuts[] = 
{
	{'a',		MN_SELECTED,	OPTIONS,APPL,		0,	1, 1},
	{'d',		MN_SELECTED,	SHOW,	SORTDATE,	0,	1, 1},
	{'f',		MN_SELECTED,	OPTIONS,RULEOPT,	0,	1, 1},
	{'g',		MN_SELECTED,	OPTIONS,PGESPRAE,	0,	1, 1},
	{'h',		MN_SELECTED,	OPTIONS,DESKSET,	0,	1, 1},
	{'i',		MN_SELECTED,	SHOW,	PDARSTEL,	0,	1, 1},
	{'l',		MN_SELECTED,	OPTIONS,DISK,		0,	1, 1},
	{'m',		MN_SELECTED,	OPTIONS,TOMUPFEL,	0,	1, 1},
	{'n',		MN_SELECTED,	SHOW,	SORTNAME,	0,	1, 1},
	{'o',		MN_SELECTED,	SHOW,	ORDEDESK,	0,	1, 1},
	{'q',		MN_SELECTED,	OPTIONS,SHUTDOWN,	0,	1, 1},
	{'p',		MN_SELECTED,	OPTIONS,PPROGRAM,	0,	1, 1},
	{'s',		MN_SELECTED,	OPTIONS,SAVEWORK,	0,	1, 1},
	{'t',		MN_SELECTED,	SHOW,	SORTTYPE,	0,	1, 1},
	{'u',		MN_SELECTED,	SHOW,	UNSORT,		0,	1, 1},
	{'v',		MN_SELECTED,	OPTIONS,PDIVERSE,	0,	1, 1},
};

/* added page up/down */

static KeyMessage normalShortCuts[] = 
{
	{UNDO,		MN_SELECTED,	EDIT,	DOUNDO,		1,	1, 1},
	{DELETE,	MN_SELECTED,	FILES,	WINDCLOS,	10,	0},
	{CUR_UP,	WM_ARROWED,		0,		WA_UPLINE,	0,	0, 1},
	{CUR_DOWN,	WM_ARROWED,		0,		WA_DNLINE,	0,	0, 1},
	{SHFT_CU,	WM_ARROWED,		0,		WA_UPPAGE,	0,	0, 1},
	{0x4900,	WM_ARROWED,		0,		WA_UPPAGE,	0,	0, 1},
	{SHFT_CD,	WM_ARROWED,		0,		WA_DNPAGE,	0,	0, 1},
	{0x5100,	WM_ARROWED,		0,		WA_DNPAGE,	0,	0, 1},
	{HOME,		WM_VSLID,		0,		0,			0,	0, 1},
	{SHFT_HOME,	WM_VSLID,		0,		1000,		0,	0, 1},
	{'\t',		MN_SELECTED,	FILES,	GETINFO,	0,	1},
	{'\r',		MN_SELECTED,	FILES,	OPEN,		10,	0},
	{'\b',		MN_SELECTED,	FILES,	CLOSE,		0,	0},
};

static KeyMessage normalShiftShortCuts[] = 
{
	{DELETE,	MN_SELECTED,	EDIT,	MNDEL,		1,	1, 1},
};

static KeyMessage controlShortCuts[] = 
{
	{'A',		MN_SELECTED,	EDIT,	SELECALL,	1,	0, 1},
	{'C',		MN_SELECTED,	EDIT,	MNCOPY,		10,	0, 1},
	{'D',		MN_SELECTED,	FILES,	WINDCLOS,	10,	0, 1},
	{'E',		MN_SELECTED,	FILES,	INITDISK,	0,	1, 1},
	{'F',		WM_FULLED,		0,		0,			10,	1, 1},
	{'G',		MN_SELECTED,	SHOW,	BYNOICON,	10,	0, 1},
	{'H',		MN_SELECTED,	FILES,	CLOSE,		10,	1, 1},
	{'I',		MN_SELECTED,	FILES,	GETINFO,	0,	1, 1},
	{'K',		MN_SELECTED,	SHOW,	BYSMICON,	10,	0, 1},
	{'L',		MN_SELECTED,	FILES,	ALIAS,		10,	0, 1},
	{'N',		MN_SELECTED,	FILES,	FOLDER,		0,	0, 1},
	{'O',		MN_SELECTED,	FILES,	OPEN,		10,	1, 1},
	{'P',		MN_SELECTED,	SHOW,	WILDCARD,	0,	0, 1},
	{'Q',		MN_SELECTED,	FILES,	QUIT,		0,	1, 1},
	{'T',		MN_SELECTED,	SHOW,	BYTEXT,		10,	0, 1},
	{'U',		MN_SELECTED,	FILES,	WINDCLOS,	10,	1, 1},
	{'V',		MN_SELECTED,	EDIT,	MNPASTE,	10,	1, 1},
	{'W',		MN_SELECTED,	FILES,	CYCLEWIN,	10,	1, 1},
	{'X',		MN_SELECTED,	EDIT,	MNCUT,		10,	0, 1},
	{'Z',		MN_SELECTED,	OPTIONS,TOMUPFEL,	0,	0, 1},
	{BACKSPACE,	MN_SELECTED,	FILES,	CLOSE,		0,	0, 1},
};

/* Dummy frs Beenden von doKey
 */
static KeyMessage key_done_message = 
	{ 0, 0, 0, 0, 0, 1, 1};

/* Wird auf >0 gesetzt, wenn zu einer anderen Aufl”sung umgeschaltet
 * werden soll
 */
static int switch_to_resolution = 0;

int ResolutionSwitchRequest (void)
{
	return switch_to_resolution;
}

int SwitchResolution (void)
{
	static int record [4];

	if (!switch_to_resolution)
		return FALSE;

	record[0] = 0;
	record[1] = 3;
	record[2] = ALT_1 + ((switch_to_resolution - 1) << 8);
	record[3] = K_ALT;

	appl_tplay (record, 1, 10);

	return TRUE;	
}

int ValidResolutionSwitch (int resolution)
{
	char res_trans[] = { 1, 2, 3, -1, 5, -1, 6, 4};
	unsigned int current_resolution;
	
	current_resolution = (unsigned int)Getrez ();
	if (current_resolution >= DIM (res_trans))
		return FALSE;
	
	current_resolution = res_trans[current_resolution];
	if ((current_resolution <= 0)
		|| (current_resolution == resolution))
		return FALSE;
	
	if (_GemParBlk.global[1] == 1)
	{
		switch_to_resolution = resolution;
		return TRUE;
	}
	
	/* Res Switch unter MTOS?
	 */
	return FALSE;
}

/*
 * Schaue nach, ob es eine belegte Funktiontaste ist
 */
word isValidFunctionKey (word kreturn)
{
	word fnr;
	
	if ((kreturn >= F1) && (kreturn <= F10))
	{
		fnr = ((kreturn - F1) / 256) + 1;
	}
	else if ((kreturn >= SHFT_F1) && (kreturn <= SHFT_F10))
	{
		fnr = ((kreturn - SHFT_F1) / 256) + 11;
	}
	else
		return FALSE;

	if (FunctionKeys && (fnr > 0) && (fnr <= 20))
		return (FunctionKeys[fnr - 1] != NULL);
	else
		return FALSE;
}

static KeyMessage *getKeyEntry (KeyMessage *table, size_t max, 
	word kreturn)
{
	word i;

	for (i = 0; i < max; ++i)
	{
		/* Soll der Scancode auch betrachtet werden ? */
		if ((table[i].key & 0xFF00) != 0)
		{
			if (table[i].key == kreturn)
				return &(table[i]);
		}
		else
		{
			if (table[i].key == (kreturn & 0xFF))
				return &(table[i]);
		}
	}
	
	return NULL;
}

static KeyMessage *handleNormalKey (word kreturn, word kstate)
{
	KeyMessage *pk = NULL;
	
	if ((kstate & (K_LSHIFT|K_RSHIFT)) != 0)
	{
		pk = getKeyEntry (normalShiftShortCuts, 
				DIM (normalShiftShortCuts), kreturn);
	}
	
	if (pk == NULL)
		pk = getKeyEntry (normalShortCuts, 
				DIM (normalShortCuts), kreturn);
	return pk;
}


static void openFileWindowOfDrive (char c)
{
	char path[MAX_FILENAME_LEN];
		
	sprintf (path, "%c:\\", c);
	GrafMouse (HOURGLASS, NULL);
	FileWindowOpen (path, "", path, "*", 0);
	GrafMouse (ARROW, NULL);
}


static KeyMessage *handleControlKey (word kreturn, word kstate)
{
	short i;
	char c;
	
	c = (kreturn & 0xFF) + '@';
	
	if (((kstate & (K_LSHIFT|K_RSHIFT)) != 0) && isalpha (c))
	{
		WindInfo *wp;
		
		wp = GetTopWindInfo ();
		if (wp && wp->add_to_display_path)
		{
			if (LegalDrive (c))
			{
				char path[MAX_PATH_LEN];
				
				c = toupper (c);
				GrafMouse (HOURGLASS, NULL);
				SetDrive (c);
				GetFullPath (path, MAX_PATH_LEN);
				wp->add_to_display_path (wp, path);
				GrafMouse (ARROW, NULL);
			}
			else
				venusErr (NlsStr (T_NODRIVE), c);
				
			return &key_done_message;
		}
		
		if (wp->kind == WK_DESK)
		{
			openFileWindowOfDrive (c);
			return &key_done_message;
		}
	}

	for (i=0; i < DIM(controlShortCuts); ++i)
	{
		if ((controlShortCuts[i].key) == c)
			return &(controlShortCuts[i]);
	}
	return NULL;
}

static KeyMessage *handleAltKey (word kreturn, word kstate,
	int *terminate)
{
	KEYTAB *key_table;
	short i, scan_code;
	char c;
	
	*terminate = FALSE;

	key_table = Keytbl ((void *)-1, (void *)-1, (void *)-1);
	scan_code = kreturn >> 8;
	if ((scan_code < 0) || (scan_code > 0x7F))
		return NULL;
		
	c = toupper (key_table->unshift[scan_code]);
	if (((kstate & (K_LSHIFT|K_RSHIFT)) != 0) && isalpha (c))
	{
		openFileWindowOfDrive (c);
		return &key_done_message;
	}

	for (i=0; i < DIM(altShortCuts); ++i)
	{
		if (altShortCuts[i].key == key_table->unshift[scan_code])
			return &(altShortCuts[i]);
	}
	
	if (kreturn >= F1 && kreturn <= F5)
	{
		*terminate = ValidResolutionSwitch (((kreturn - F1) >> 8) + 1);
	}
	
	return NULL;
}

/*
 * manage keyboard events
 */
word doKeys (word kreturn, word kstate, int *mupfel_key, int *silent)
{
	WindInfo *topwp;
	word gueltig, killrepeat, eatenByConsole;
	word messbuff[8],topmup, ok;
	word topwindow, receiver;
	KeyMessage *key_message = NULL;
	int terminate = FALSE;
	unsigned char c;
	int override = 0;

	*silent = 0;
	ok = TRUE;
	killrepeat = 0;
	messbuff[1] = apid;
	messbuff[2] = 0;
	eatenByConsole = 1;
	gueltig = FALSE;
	receiver = apid;

	wind_get (0, WF_TOP, &topwindow);
	topwp = GetWindInfo (topwindow);

	if( !topwp )
		topwp = GetTopWindInfo();
		
	if (topwp->kind == WK_ACC)
	{
		/* Wenn ein registriertes Acc-Fenster das oberste ist,
		 * dann haben wir diese Taste wahrscheinlich ber das
		 * VA-Protkoll erhalten.
		 */
		topwp = topwp->nextwind;
		while ((topwp) && (topwp->kind == WK_ACC))
			topwp = topwp->nextwind;
		
		if (!topwp)
			topwp = &wnull;
	}
	
	topmup = (topwp->kind == WK_CONSOLE);

	c = kreturn & 0xFF;
	
	if ((c == 0) && (kstate & K_ALT))
	{
		key_message = handleAltKey (kreturn, kstate, &terminate);
	}
	else if (kstate & K_CTRL)
	{
		key_message = handleControlKey (kreturn, kstate);
	}
	else
		key_message = handleNormalKey (kreturn, kstate);
		
	/* Wenn die Console das oberste Fenster ist, werden Funktions-
	 * tasten an sie weitergeleitet, ansonsten gehen sie als
	 * Gemini-Shortcut durch.
	 */
	if (!(isValidFunctionKey (kreturn) && topmup) 
		&& DoSCut (kstate, kreturn))
	{
		UpdateAllWindowData ();
		return TRUE;
	}
	else if (isValidFunctionKey (kreturn))
	{
		/* Liegt vielleicht eine in der Mupfel
		 * belegte Funktionstaste vor?
		 */
		int foo;
		word dummy;
		
		/* Ja, ”ffne das Console-Fenster...
		 */
		MakeConsoleTopWindow (&foo, &dummy);

		/* und setzte die Variablen, als wenn es schon vorher
		 * oben gewesen w„re
		 */
		wind_get (0, WF_TOP, &topwindow);
		topwp = GetWindInfo (topwindow);
		topmup = ((topwp) && (topwp->kind == WK_CONSOLE));
	}

	if (key_message)
		override = ((topwp->kind == WK_FILE) && key_message->filewind)
			|| ((topwp->kind == WK_CONSOLE) && key_message->console);

	if (!override && (topwp && topwp->window_feed_key))
	{
		if (topwp->window_feed_key (topwp, kreturn, kstate))
			return TRUE;
	}
		
	if (key_message)
	{
		if (key_message == &key_done_message)
			return TRUE;
			
		gueltig = TRUE;
		messbuff[0] = key_message->mess0;
		messbuff[3] = key_message->mess3;
		messbuff[4] = key_message->mess4;
		messbuff[5] = 0;
		killrepeat = key_message->killrep;
		eatenByConsole = !key_message->console;
	}
	else if (terminate)
		return FALSE;
		
	/* Wenn ein Shortcut vorlag (gueltig) und das Console-Fenster
	 * oben ist (topmup), berprfe, ob der Shortcut auch bei
	 * der Console gltig ist oder durchgelassen werden muž.
	 */
	if (gueltig && topmup)
	{
		/* Wenn es Control-I war ist es weiterhin gueltig.
		 * TAB wird dagegen von der Console verschluckt.
		 */
		eatenByConsole |= (!(kstate & K_CTRL)) && (c == '\t');
		gueltig = !eatenByConsole;
	}
	
	/* Wenn es ein gltiger Shortcut war, erzeuge die entsprechende
	 * Nachricht.
	 */
	if (gueltig)
	{
		switch (messbuff[0])
		{
			case MN_SELECTED:
				/* Prfe, ob der Meneintrag anw„hlbar ist
				 * Wenn ja, invertiere den Mentitel
				 */
				if (isDisabled (pmenu, messbuff[4]))
					gueltig = 0;
				else
					menu_tnormal(pmenu,messbuff[3],0);
				break;
				
			case WM_FULLED:
			case WM_ARROWED:
			case WM_VSLID:
			case WM_HSLID:
			case WM_SIZED:
			case WM_MOVED:
			case WM_NEWTOP:
			case WM_TOPPED:
			case WM_CLOSED:
			case WM_REDRAW:
				/* Wenn eins von unseren Fenstern oben ist...
				 */
				if (topwp)
				{
					if (!messbuff[3])
						messbuff[3] = topwp->handle;

					gueltig = (topwp->kind != WK_DESK);
					receiver = topwp->owner;
				}
				else
					gueltig = FALSE;
				break;
				
			default:
				break;
		}

		/* Weiterhin ein gltiger Shortcut? Dann bearbeite oder
		 * verschicke die Nachricht je nachdem, an wen sie gerichtet
		 * ist.
		 */
		if (gueltig)
		{
			if (receiver == apid)
				ok = HandleMessage(messbuff, kstate, silent);
			else
				appl_write(receiver, 16, messbuff);

			if(killrepeat > 0)
				killEvents(MU_KEYBD, killrepeat);
			return ok;
		}
	}

	/* HELP-Taste?
	 */
	if (!topmup && (kreturn == HELP))
	{
		if (!InvokeHyperTextHelp ())
		{
			venusInfo (NlsStr (T_HELP));
		}
		return ok;
	}
	else if ((topwp) && (topwp->kind == WK_FILE))
	{
		/* Oberstes Fenster ist eins von unseren Dateifenstern
		 */
		switch (c)
		{
			/* Escape: lies den Inhalt neu ein; erzeuge einen
			 * Mediachange.
			 */
			case 0x1B:
			{
				FileWindow_t *fwp = topwp->kind_info;

				GrafMouse (HOURGLASS, NULL);					
				if (kstate)
				{
					if (ForceMediaChange (fwp->path) == EACCDN)
						venusErr (NlsStr (T_FILEOPEN));
				}

				/* Dateien neu einlesen...
				 */
				if (topwp->add_to_display_path)
					topwp->add_to_display_path (topwp, NULL);
				GrafMouse (ARROW, NULL);
				killrepeat = 20;
				break;
			}

			default:
				/* Wenn es ein alphanumerisches Zeichen ist, scrolle
				 * an die entsprechende Position...
				 */	
				if (!(kstate & K_CTRL) 
					&& ((c > 127) || isalnum (c) || 
					strchr ("!_#$%^&()+-=~;,<>|[]{}*?. \b\t	", c)))
				{
					/* scroll window to char c
					 */
					FileWindowCharScroll (topwp, c, kstate);
					killrepeat = 0;
					UpdateAllWindowData ();
				}
				
				break;
		}
	}
	else if (topmup && eatenByConsole)
	{
		/* Hier haben wir also bisher nichts gefunden und das
		 * Console-Fenster oben liegen. Also ftterm wir die
		 * Mupfel mit dieser Taste...
		 */
		*mupfel_key = TRUE;
		return ok;
	}
	
	/* Sollten Tastaturevents (nachlaufendes Keyboard) gel”scht
	 * werden?
	 */
	if (killrepeat > 0)
		killEvents (MU_KEYBD, killrepeat);
	
	return ok;
}
