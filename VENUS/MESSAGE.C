/*
 * @(#)Gemini\Message.c
 * @(#)Stefan Eissing, $Id$
 *
 * project: venus
 *
 * description: modul for handling messages
 *
 * jr 7.10.95
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <aes.h>
#include <flydial\flydial.h>
#include <flydial\evntevnt.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\cookie.h"
#include "..\common\fileutil.h"
#include "..\common\genargv.h"

#include "..\mupfel\mupfel.h"

#include "vs.h"

#include "window.h"
#include "accwind.h"
#include "conwind.h"
#include "copymove.h"
#include "cpkobold.h"
#include "dispatch.h"
#include "draglogi.h"
#include "dragdrop.h"
#include "filewind.h"
#include "filedraw.h"
#include "fileutil.h"
#include "greeting.h"
#include "menu.h"
#include "message.h"
#include "stand.h"
#if MERGED
#include "mvwindow.h"
#endif
#include "redraw.h"
#include "scroll.h"
#include "select.h"
#include "util.h"
#include "vaproto.h"
#include "venuserr.h"

#include "switchof.rh"


/* internal texts
 */
#define NlsLocalSection "G.appledit.c"
enum NlsLocalText{
T_MEMVIOLATION,	/*Applikation %d hat falschen Speicherschutz und
 kann daher nicht voll am VA-Protokoll teilnehmen!*/ 
};

/*internals
 */
typedef struct
{
	word accId;
	char name[9];
	word can_xacc;
	word protoStatus;
	char *status;
	char *message;
} AccInfo;

static word accMax = 0;
static word accCount = 0;
static AccInfo *accArray = NULL;
static char *xacc_name = NULL;

static AccInfo *getAccInfo (word accid, const char *name)
{
	word i;

	if (accid >= 0)
	{
		for (i = 0; i < accCount; ++i)
		{
			if (accArray[i].accId == accid)
				return &accArray[i];
		}
	}

	if (name)
	{
		for (i = 0; i < accCount; ++i)
		{
			if (!strcmp (accArray[i].name, name))
				return &accArray[i];
		}
	}

	return NULL;
}

static AccInfo *makeOrGetAccInfo (word accid, word proto, 
	const char *name)
{
	AccInfo *entry;

	if (!MemoryIsReadable (name))
	{
		venusErr (NlsStr (T_MEMVIOLATION), accid);
		return NULL;
	}

	entry = getAccInfo (accid, name);

	if (entry == NULL)
	{
		int entry_nr = -1;
		int i;

		for (i = 0; i < accCount; ++i)
		{
			if (accArray[i].accId == -1)
			{
				entry_nr = i;
				break;
			}
		}

		if (entry_nr < 0)
		{
			if (accCount >= accMax)
			{
				/* Array vergrîûern
				 */
				AccInfo *tmp;

				tmp = calloc (accMax + 6, sizeof (AccInfo));
				if (!tmp)
					return NULL;

				if (accArray)
				{
					memcpy (tmp, accArray, accMax * sizeof (AccInfo));
					free (accArray);
				}
				accMax += 6;
				accArray = tmp;
			}

			entry_nr = accCount++;
		}

		entry = &accArray[entry_nr];
	}

	entry->accId = (accid >= 0)? accid : -2;
	if (name)
	{
		/* Wir haben eine VA-Registrierung mit Namen des Accs.
		 * Wenn auf der ID dieses Accs vorher schon ein Acc 
		 * installiert war (Chameleon), dann mÅssen wir den
		 * Status-String lîschen, da sonst das neue Acc den falschen
		 * Status bekommt.
		 */
		if (strcmp (entry->name, name) && entry->status)
		{
			free (entry->status);
			entry->status = NULL;
		}
		strcpy (entry->name, name);
	}
	else
		entry->name[0] = '\0';

	entry->protoStatus = proto;

	return entry;
}

static void freeAccInfo (word accId)
{
	AccInfo *entry = getAccInfo (accId, NULL);

	if (entry)
	{
		entry->accId = -1;
		free (entry->status);
		entry->status = NULL;
	}
}

static word storeAccInfoStatus (AccInfo *entry, const char *line)
{
	char *cp;

	free (entry->status);
	entry->status = NULL;

	if (!line)
		return TRUE;

	cp = malloc (strlen (line));
	if (!cp)
		return FALSE;

	strcpy (cp, line);
	entry->status = cp;
	return TRUE;
}

word WriteAccInfos (MEMFILEINFO *mp, char *buffer)
{
	word i;

	for (i = 0; i < accCount; ++i)
	{
		if ((accArray[i].name[0]) && (accArray[i].protoStatus & 1)
			&& (accArray[i].status))
		{
			strcpy (buffer, "#V");
			strcat (buffer, accArray[i].name);
			strcat (buffer, accArray[i].status);
			if (mputs (mp, buffer))
				return FALSE;
		}
	}
	return TRUE;
}

void ExecAccInfo (const char *line)
{
	word accid;
	AccInfo *entry;
	char name[9];
	size_t len;

	strncpy (name, &line[2], 8);
	name[8] = '\0';

	accid = appl_find (name);

	entry = makeOrGetAccInfo (accid, 1, name);
	if (entry == NULL)
		return;

	len = strlen (line);
	if (len > 10)
		storeAccInfoStatus (entry, &line[10]);
}

static void sendAccStatus (word messbuff[8])
{
	word mbuff[8];
	word i;

	mbuff[0] = VA_SETSTATUS;
	mbuff[1] = apid;
	mbuff[2] = 0;

	for (i = 0; i < accCount; ++i)
	{
		if ((accArray[i].accId == messbuff[1]) 
			&& (accArray[i].protoStatus & 1))
		{
			*(char **)(mbuff+3) = accArray[i].status;
			appl_write (messbuff[1], 16, mbuff);
		}
	}
}

static void initAcc (word apid, word accid, word menu_id, int acc_version,
	char *acc_name)
{
	word i, messbuff[8];
	AccInfo *entry;
	int multiple = (_GemParBlk.global[1] != 1);

	entry = getAccInfo (accid, acc_name);
	if (entry == NULL)
	{
		entry = makeOrGetAccInfo (accid, 0, NULL);
		if (entry == NULL)
			return;
	}

	entry->can_xacc = TRUE;

	WindUpdate (END_UPDATE);

	if (!multiple)
	{
		messbuff[0] = ACC_ACC;
		messbuff[1] = apid;
		messbuff[2] = 0;
		messbuff[3] = acc_version;
		*((char **)(messbuff+4)) = acc_name;
		messbuff[6] = menu_id;
		messbuff[7] = accid;

		for (i = 0; i < accCount; i++)
		{
			if ((accArray[i].accId != accid) && accArray[i].can_xacc)
				appl_write (accArray[i].accId, 16, messbuff);
		}

	}

/* XACC-Makros zur vereinfachten Konstruktion eines "Extended Names" */

#define XACC_NAME(a)                        a "\0"
#define XACC_IDENTIFIER                     "XDSC\0"
#define XACC_APP_TYPE_HUMAN(a)              "1" a "\0"
#define XACC_APP_TYPE_MACHINE(a)            "2" a "\0"
#define XACC_EXT_FEATURE(a)                 "X" a "\0"
#define XACC_GENERIC_NAME(a)                "N" a "\0"

#define XACC_APP_TYPE_WORD_PROCESSOR        XACC_APP_TYPE_MACHINE ("WP")
#define XACC_APP_TYPE_DTP                   XACC_APP_TYPE_MACHINE ("DP")
#define XACC_APP_TYPE_TEXT_EDITOR           XACC_APP_TYPE_MACHINE ("ED")
#define XACC_APP_TYPE_DATABASE              XACC_APP_TYPE_MACHINE ("DB")
#define XACC_APP_TYPE_SPREADSHEET           XACC_APP_TYPE_MACHINE ("SS")
#define XACC_APP_TYPE_RASTER_APP            XACC_APP_TYPE_MACHINE ("RG")
#define XACC_APP_TYPE_VECTOR_APP            XACC_APP_TYPE_MACHINE ("VG")
#define XACC_APP_TYPE_GRAPHICS_APP          XACC_APP_TYPE_MACHINE ("GG")
#define XACC_APP_TYPE_MUSIC_APP             XACC_APP_TYPE_MACHINE ("MU")
#define XACC_APP_TYPE_CAD                   XACC_APP_TYPE_MACHINE ("CD")
#define XACC_APP_TYPE_DATA_COMMUNICATION    XACC_APP_TYPE_MACHINE ("DC")
#define XACC_APP_TYPE_DESKTOP               XACC_APP_TYPE_MACHINE ("DT")
#define XACC_APP_TYPE_PROGRAMMING_ENV       XACC_APP_TYPE_MACHINE ("PE")

	if (xacc_name == NULL)
	{
		xacc_name = malloc (256);
		if (xacc_name)
		{
			static char *xname = 
			    XACC_NAME ("Gemini  ")
			    XACC_IDENTIFIER
			    XACC_APP_TYPE_HUMAN ("Desktop")
			    XACC_APP_TYPE_DESKTOP
			    XACC_EXT_FEATURE ("AV")
			    XACC_GENERIC_NAME ("Gemini");
			memcpy (xacc_name, xname, DIM(xname));
		}
	}

	messbuff[0] = multiple? ACC_ACC : ACC_ID;
	messbuff[1] = apid;
	messbuff[2] = 0;	/* jr 6.7.95 */
	messbuff[3] = 0x1900;  /* Version 1.9, Protokollstufe 0 */
	*((char **)(messbuff+4)) = xacc_name;
	messbuff[6] = 0;      /* oder etwas anderes */
	messbuff[7] = 0;      /* oder etwas anderes */

	appl_write (accid, 16, messbuff);
	WindUpdate (BEG_UPDATE);
}

static void sentProtoStatus (word messbuff[8])
{
	AccInfo *entry;
	word buff[8];
	char *name = NULL;

	entry = makeOrGetAccInfo (messbuff[1], messbuff[3], 
		*(char **)(messbuff+6));
	if (entry != NULL)
	{
		free (entry->message);
		name = entry->message = StrDup (pMainAllocInfo, "GEMINI  ");
	}

	if (name == NULL)
		name = "GEMINI  ";

	buff[0] = VA_PROTOSTATUS;
	buff[1] = apid;
	buff[2] = 0;
	buff[3] = 0x7FFF;
	buff[4] = buff[5] = 0;
	*(char **)(buff+6) = name;

	appl_write (messbuff[1], 16, buff);
}

static void saveAccStatus (word messbuff[8])
{
	AccInfo *entry;
	char *line;

	entry = getAccInfo (messbuff[1], NULL);
	if (entry == NULL)
	{
		venusDebug ("AccStatus: Acc wurde nicht gefunden!");
		return;
	}

	line = *(char **)(messbuff+3);

	if (!MemoryIsReadable (line))
	{
		venusErr (NlsStr (T_MEMVIOLATION), messbuff[1]);
		return;
	}

	storeAccInfoStatus (entry, line);
}

static void sendFileFont (word messbuff[8])
{
	word buff[8];

	buff[0] = VA_FILEFONT;
	buff[1] = apid;
	buff[2] = 0;

	GetFileFont (&buff[3], &buff[4]);
	appl_write (messbuff[1], 16, buff);
}

#if MERGED
static void sendConsoleFont (word messbuff[8])
{
	word buff[8];

	buff[0] = VA_CONFONT;
	buff[1] = apid;
	buff[2] = 0;

	GetConsoleFont (&buff[3], &buff[4]);
	appl_write (messbuff[1], 16, buff);
}

static void openConsole (word messbuff[8])
{
	int opened;
	word buff[8], old_top;

	buff[0] = VA_CONSOLEOPEN;
	buff[1] = apid;
	buff[2] = 0;

	buff[3] = MakeConsoleTopWindow (&opened, &old_top);
	appl_write (messbuff[1], 16, buff);
}
#endif

void FontChanged (word fileId, word fileSize, 
	word conId, word conSize)
{
	word buff[8];
	int i;

	buff[0] = VA_FONTCHANGED;
	buff[1] = apid;
	buff[2] = 0;

	buff[3] = fileId;
	buff[4] = fileSize;
	buff[5] = conId;
	buff[6] = conSize;

	WindUpdate (END_UPDATE);
	for (i = 0; i < accCount; ++i)
	{
		if ((accArray[i].name[0]) && (accArray[i].protoStatus & 8))
		{
			appl_write (accArray[i].accId, 16, buff);
		}
	}
	WindUpdate (BEG_UPDATE);
}

static void askObject (word messbuff[8])
{
	word buff[8];
	char **cpp;
	char *names;
	AccInfo *entry;
	ARGVINFO A;

	entry = makeOrGetAccInfo (messbuff[1], 0, NULL);
	if (entry == NULL)
		return;

	free (entry->message);

	buff[0] = VA_OBJECT;
	buff[1] = apid;
	buff[2] = 0;

	if (WindGetSelected (&A, FALSE))
	{
		names = Argv2String (pMainAllocInfo,
			&A, VA_ACC_QUOTING (entry->protoStatus));
	}
	else
		names = NULL;

	cpp = (char **)&buff[3];

	entry->message = names;
	*cpp = names;

	appl_write (messbuff[1], 16, buff);
	FreeArgv (&A);
}

static void startProg (word messbuff[8])
{
	word buff[8];
	char name[MAX_PATH_LEN];
	char *ppath, *pcommand;
	char failed = FALSE;
	ARGVINFO args;
	AccInfo *entry;

	entry = makeOrGetAccInfo (messbuff[1], 0, NULL);
	if (entry == NULL)
		return;

	buff[0] = VA_PROGSTART;
	buff[1] = apid;
	buff[2] = 0;
	buff[3] = 0;			/* nicht gestartet ist default */
	buff[4] = 0;			/* Returncode 0 ist default */
	buff[7] = messbuff[7];	/* Kennung aus AV_PROGSTART */

	ppath = *((char **)&messbuff[3]);
	pcommand = *((char **)&messbuff[5]);
	InitArgv (&args);

	if (!MemoryIsReadable (ppath) || !MemoryIsReadable (pcommand))
	{
		venusErr (NlsStr (T_MEMVIOLATION), messbuff[1]);
		return;
	}

	if (ppath)
	{
		ARGVINFO cmd;

		failed = !String2Argv (pMainAllocInfo, &cmd, ppath, 
				VA_ACC_QUOTING (entry->protoStatus));

		if (!failed && pcommand)
		{
			failed = !String2Argv (pMainAllocInfo, &args, pcommand, 
				VA_ACC_QUOTING (entry->protoStatus));
		}

		if (!failed && GetBaseName (cmd.argv[0], name, 
			sizeof (name) - 1))
		{
			StripFileName ((char *)cmd.argv[0]);
			buff[3] = StartFile (NULL, 0, TRUE, "", 
				(char *)cmd.argv[0], name, &args, 0, &buff[4]);
		}

		FreeArgv (&cmd);
	}

	FreeArgv (&args);
	appl_write (messbuff[1], 16, buff);
}

static void openWind (word messbuff[8])
{
	word buff[8];
	char **pppath, **ppwild;
	char title[MAX_PATH_LEN];
	AccInfo *entry;

	entry = makeOrGetAccInfo (messbuff[1], 0, NULL);
	if (entry == NULL)
		return;

	buff[0] = VA_WINDOPEN;
	buff[1] = apid;
	buff[2] = 0;

	pppath = (char **)&messbuff[3];
	ppwild = (char **)&messbuff[5];

	if (!MemoryIsReadable (*pppath) || !MemoryIsReadable (*ppwild))
	{
		venusErr (NlsStr (T_MEMVIOLATION), messbuff[1]);
		return;
	}

	if (*pppath && *ppwild)
	{
		ARGVINFO A;

		String2Argv (pMainAllocInfo, &A, *pppath, 
				VA_ACC_QUOTING (entry->protoStatus));

		if (!GetBaseName (A.argv[0], title, MAX_FILENAME_LEN))
			strcpy (title, A.argv[0]);
		else
			strcat (title, "\\");

		buff[3] = (0 < FileWindowOpen (A.argv[0], "", title, *ppwild, 0));
		FreeArgv (&A);
	}
	else
		buff[3] = 0;

	appl_write (messbuff[1], 16, buff);
}

/* AV_XWIND: Venus soll ein Datei-Fenster îffnen (eXtended).
 * Dies sollte auch nur geschehen, wenn die Ursache fÅr den
 * Benutzer ersichtlich ist.
 * Word 3+4 (Pointer) Pfad fÅr das Fenster (s.o.).
 * Word 5+6 (Pointer) Wildcard als Filter fÅr Anzeige
 * Word 7   Bitmaske  0x0001 - toppe evtl. vorhandenes Fenter
 *                    0x0002 - Wildcard soll nur selektieren
 *                           - alle anderen Bits auf 0 setzen!
 */
static void openXWind (word messbuff[8])
{
	word buff[8];
	char **pppath, **ppwild;
	char title[MAX_PATH_LEN];
	int what = messbuff[7];
	AccInfo *entry;

	entry = makeOrGetAccInfo (messbuff[1], 0, NULL);
	if (entry == NULL) return;

	buff[0] = VA_XOPEN;
	buff[1] = apid;
	buff[2] = 0;

	pppath = (char **)&messbuff[3];
	ppwild = (char **)&messbuff[5];

/*	venusErr ("pppath: %s, ppwild: %s", *pppath, *ppwild); */

	if (!MemoryIsReadable (*pppath) || !MemoryIsReadable (*ppwild))
	{
		venusErr (NlsStr (T_MEMVIOLATION), messbuff[1]);
		return;
	}

	if (*pppath)
	{
		ARGVINFO A;
		WindInfo *wp = NULL;

		String2Argv (pMainAllocInfo, &A, *pppath, 
				VA_ACC_QUOTING (entry->protoStatus));

		if (!GetBaseName (A.argv[0], title, MAX_FILENAME_LEN))
			strcpy (title, A.argv[0]);
		else
			strcat (title, "\\");

		/* Versuche ein Fenster mit dem Pfad zu finden */

		if ((what & 0x01) && 
			NULL != (wp = FindFileWindow (A.argv[0])))
		{
			DoTopWindow (wp->handle);
		}
		else
		{
			wp = FileWindowOpen (A.argv[0], "", 
					title, (what & 0x02)? "*" : *ppwild, 0);
		}

		if ((what & 0x02) && *ppwild && wp)
		{
			/* Selektieren Dateien per Wildcard */
			StringSelect (wp, *ppwild, 1);
		}

		buff[3] = (wp != NULL);
		FreeArgv (&A);
	}
	else
		buff[3] = 0;

	appl_write (messbuff[1], 16, buff);
}

static void accWindowOpened (word messbuff[8])
{
	InsertAccWindow (messbuff[1], messbuff[3]);
}

static void accWindowClosed (word messbuff[8])
{
	RemoveAccWindow (messbuff[1], messbuff[3]);
}

/*
 * Falls Objekte auf ein Acc.-Fenster gezogen wurden, so
 * sende deren Namen an das Acc.; ansonsten gib FALSE zurÅck
 */
word PasteAccWindow (word to_window, word mx, word my, 
	word kstate, ARGVINFO *A)
{
	word buff[8];
	WindInfo *accwp = GetWindInfo (to_window);
	AccInfo *entry;
	int unknownWindow;

	unknownWindow = ((accwp == NULL) || (accwp->kind != WK_ACC));

	if (unknownWindow)
		return TryAtariDragAndDrop (-1, to_window, A, 
			mx, my, kstate);

	entry = getAccInfo (accwp->owner, NULL);
	if (entry == NULL)
		return FALSE;

	free (entry->message);
	entry->message = Argv2String (pMainAllocInfo, A, 
		VA_ACC_QUOTING (entry->protoStatus));
	if (!entry->message)
		return FALSE;

	buff[0] = VA_DRAGACCWIND;
	buff[1] = apid;
	buff[2] = 0;
	buff[3] = accwp->handle;
	buff[4] = mx;
	buff[5] = my;
	*((char **)&buff[6]) = entry->message;

	WindUpdate (END_UPDATE);
	appl_write (accwp->owner, 16, buff);
	WindUpdate (BEG_UPDATE);

	return TRUE;
}

static void copyObjectsFromAcc (word messbuff[8])
{
	word buff[8];
	char *target = *((char **)&messbuff[4]);
	AccInfo *entry;
	ARGVINFO A;
	ARGVINFO Source;

	if (!MemoryIsReadable (target))
	{
		venusErr (NlsStr (T_MEMVIOLATION), messbuff[1]);
		return;
	}

	buff[0] = VA_COPY_COMPLETE;
	buff[1] = apid;
	buff[2] = 0;

	entry = getAccInfo (messbuff[1], NULL);
	if (entry == NULL)
		return;

	String2Argv (pMainAllocInfo, &A, target,
		VA_ACC_QUOTING (entry->protoStatus));

	/* Die zu kopierenden Dateien stehen im message string vom
	 * letzten VA_DRAGACCWIND
	 */
	if (entry->message)
		String2Argv (pMainAllocInfo, &Source, entry->message,
			VA_ACC_QUOTING (entry->protoStatus));
	else
		memset (&Source, 0, sizeof Source);

	if (Source.argc && A.argc)
	{
		buff[3] = CopyArgv (&Source, (char *)A.argv[0], 
			messbuff[3] & K_CTRL, messbuff[3] & K_ALT);
	}
	else
		buff[3] = FALSE;		/* nicht impl. -> FALSE */

	FreeArgv (&A);
	FreeArgv (&Source);

	appl_write (messbuff[1], 16, buff);
}


static void AV_PathUpdate (word messbuff[8])
{
	char **pppath;
	AccInfo *entry;

	entry = makeOrGetAccInfo (messbuff[1], 0, NULL);
	if (entry == NULL)
		return;

	pppath = (char **)&messbuff[3];

	if (!MemoryIsReadable (*pppath))
	{
		venusErr (NlsStr (T_MEMVIOLATION), messbuff[1]);
		return;
	}

	if (*pppath != NULL)
	{
		ARGVINFO A;
		int i;

		String2Argv (pMainAllocInfo, &A, *pppath,
			VA_ACC_QUOTING (entry->protoStatus));
		for (i = 0; i < A.argc; ++i)
		{
			PathUpdate (A.argv[i]);
		}
		FreeArgv (&A);
	}
}


static void AV_WhatIzIt (word messbuff[8])
{
	char *names;
	word buff[8];
	AccInfo *entry;

	entry = makeOrGetAccInfo (messbuff[1], 0, NULL);
	if (entry == NULL)
		return;

	free (entry->message);
	entry->message = NULL;

	buff[0] = VA_THAT_IZIT;
	buff[1] = apid;
	buff[2] = 0;

	names = WhatIzIt (messbuff[3], messbuff[4], &buff[4], &buff[3]);
	if (VA_ACC_QUOTING (entry->protoStatus))
	{
		ARGVINFO A;

		String2Argv (pMainAllocInfo, &A, names, 0);
		free (names);
		names = Argv2String (pMainAllocInfo, &A, 1);
		FreeArgv (&A);
	}
	entry->message = names;
	*((char **)&buff[5]) = entry->message;

	appl_write (messbuff[1], 16, buff);
}


static void AV_DragOnWindow (word messbuff[8])
{
	int stat = 1;
	char **files;
	int buff[8];

	files = (char **)&messbuff[6];

	if (!MemoryIsReadable (*files))
	{
		venusErr (NlsStr (T_MEMVIOLATION), messbuff[1]);
		return;
	}

	if (*files != NULL)
	{
		ARGVINFO A;
		AccInfo *entry;

		entry = makeOrGetAccInfo (messbuff[1], 0, NULL);
		if (entry == NULL)
			return;

		stat = String2Argv (pMainAllocInfo, &A, *files, 
			VA_ACC_QUOTING (entry->protoStatus));

		if (stat)
			stat = DropFiles (messbuff[3], messbuff[4],
				messbuff[5], &A);
		FreeArgv (&A);
	}

	buff[0] = VA_DRAG_COMPLETE;
	buff[1] = apid;
	buff[2] = 0;
	buff[3] = stat;

	appl_write (messbuff[1], 16, buff);
}

static void AV_Exit (word messbuff[8])
{
	freeAccInfo (messbuff[3]);
}

/***************************************************************/
/* jr: Support fÅr Shutdown-Protokoll                          */
/***************************************************************/

int
HasShutdown (void)
{
	static int hasit = -1;
	
	if (hasit < 0)
	{
		int dummy, event = 0;

		appl_xgetinfo (10, &event, &dummy, &dummy, &dummy);
		hasit = (event & 0xff) > 8 || MagiCVersion >= 0x300;
	}
	
	return hasit;
}

static int shutdownReason;

/* initiate shutdown */

void Shutdown (int reason)
{
	int ret_complete = 0, ret_partial = 0;

	if (reason == AP_TERM || reason == AP_RESCHG)
		if (!venusChoice ("Wollen Sie das System wirklich ausschalten?"))
			return;

	shutdownReason = reason;

	ret_complete = shel_write (4, 2, 0, NULL, NULL);
	if (!ret_complete)
		ret_partial = shel_write (4, 1, 0, NULL, NULL);

	if (!ret_complete && !ret_partial)
		venusErr ("Zur Zeit kein Shutdown mîglich!");
}

/* warm boot, to be called in super mode */

static long warmBoot (void)
{
	typedef void voidfun (void);
	SYSHDR *Sys;
	voidfun *reset;

	Sys = *((SYSHDR **) 0x4f2L);
	reset = (voidfun *) Sys->os_start;
	reset ();
	return 0L;
}

/* black screen, draw dialog, warm boot it needed */

static void switchOff (void)
{
	int x,y,w,h;
	int xy[4];
	int bt;
	int workout[57];
	extern OBJECT *pswitchoff;
	DIALINFO D;

	pswitchoff[OFFICON].ob_spec.iconblk = GetMachineIcon ();
	
	RemoveMenuBar();
	WindUpdate (BEG_UPDATE);
	WindUpdate (BEG_MCTRL);

	vq_extnd (vdi_handle, 0, workout);
	GrafMouse (M_OFF, 0);
	wind_get (0, WF_CURRXYWH, &x, &y, &w, &h);
	vsf_color (vdi_handle, BLACK);
	xy[0] = 0; xy[1] = 0; xy[2] = workout[0];
	xy[3] = workout[1];
	vs_clip (vdi_handle, 1, xy);
	v_bar (vdi_handle, xy);

	/* Filesystems syncen */
	gemdos (0x150);
	gemdos (0x150);
	gemdos (0x150);

	if (shutdownReason != AP_TERM && shutdownReason != AP_RESCHG)
		Supexec (warmBoot);

	/* Kill Kernel if running under MagiCMac */
	xbios ((int)39, (long) 'AnKr', (int) 0);

	GrafMouse (M_ON, 0);

	WindUpdate (END_MCTRL);
	WindUpdate (END_UPDATE);

	DialCenter (pswitchoff);
	DialStart (pswitchoff, &D);
	DialDraw (&D);
	bt = 0x7fff & DialDo (&D, 0);
	DialEnd (&D);				
	pswitchoff[bt].ob_state &= ~SELECTED;

	InstallMenuBar();
	form_dial (FMD_FINISH, x, y, w, h, x, y, w, h);

	/* abort shutdown mode */
	shel_write (4, 0, 0, NULL, NULL);
}

/* handle shutdown error */

static void shutdownProblem (int aes_id)
{
	char name[10];
	int id, type;
	int found = 0;
	
#if 0
	int owner, open, nextwin, dummy;
#endif

	/* abort shutdown mode */
	shel_write (4, 0, 0, NULL, NULL);

#if 0
	if (_GemParBlk.global[0] < 0x400 && MagiCVersion < 0x300)
		return;

	/* look for an open window of that application */
	
	wind_get (0, WF_TOP, &nextwin);
	wind_get (nextwin, WF_OWNER, &owner, &open, &nextwin);
	
	while (owner != aes_id && open && nextwin)
	{
		venusErr ("owner %d open %d nextwin %d\n", owner, open, nextwin);
		wind_get (nextwin, WF_OWNER, &owner, &open, &dummy, &nextwin);
	}

	/* application found */

	if (aes_id == owner)
	{
		int mb[8];
	
		wind_set (nextwin, WF_TOP);
		Bconout (2, 7);
		
		mb[0] = WM_TOPPED; mb[1] = _GemParBlk.global[2];
		mb[2] = 0; mb[3] = nextwin;
		
		appl_write (owner, 16, mb);
		
		return;
	}
#endif

venusErr ("aesid %d", aes_id);

	if (aes_id == -1)
	{
		int notlast;
	
		notlast = appl_search (0, name, &type, &id);
		found = type == 2 && id != _GemParBlk.global[2];
	
		while (!found && notlast)
		{
			notlast = appl_search (1, name, &type, &id);
			found = type == 2 && id != _GemParBlk.global[2];
		}
	}
	else
	{
		int notlast;
	
		notlast = appl_search (0, name, &type, &id);
		while (notlast && aes_id != id)
			notlast = appl_search (1, name, &type, &id);

		found = aes_id == id;
	}

	if (!found)
	{
		venusErr ("Irgendeine Applikation lÑût sich nicht "
			"beenden!");
	}
	else
	{
		name[8] = '\0';
		while (lastchr (name) == ' ')
			name[strlen (name) - 1] = '\0';
	
		venusErr ("`%s' lÑût sich nicht beenden!", name);
	}
}

/* end of shutdown stuff */



/* Neue Nachrichten unter MultiTOS
 */
#define WM_UNTOPPED		30
#define WM_ONTOP		31

#define AP_TERM			50
#define AP_TFAIL		51
#define AP_RESCHG		57

#define SHUT_COMPLETED	60
#define RESCH_COMPLETED	61

#define AP_DRAGDROP		63
#define SH_WDRAW		72

word HandleMessage (word messbuff[8], word kstate, int *silent)
{
	WindInfo *wp;
	GRECT rect;
	word ok = TRUE;
	int remove_update = TRUE;
	void (*va_func)(int mess[8]) = NULL;

	if (messbuff[0] != WM_ARROWED 
		&& ((wp = GetTopWindInfoOfKind (WK_CONSOLE)) != NULL) 
		&& wp->window_moved)
	{
		wp->window_moved (wp);
	}

	switch (messbuff[0])
	{
		case MN_SELECTED: 
			ok = DoMenu (messbuff[3], messbuff[4], kstate);
			break;

		case WM_NEWTOP:
		case WM_TOPPED:
			DoTopWindow (messbuff[3]);
			break;

		case WM_REDRAW:
			rect.g_x = messbuff[4];
			rect.g_y = messbuff[5];
			rect.g_w = messbuff[6];
			rect.g_h = messbuff[7];
			wp = GetWindInfo (messbuff[3]);
			if (wp && wp->window_redraw)
				wp->window_redraw (wp, &rect);
			break;

		case WM_CLOSED:
			WindowClose (messbuff[3], kstate);
			break;

		case WM_SIZED:
			rect.g_x = messbuff[4];
			rect.g_y = messbuff[5];
			rect.g_w = messbuff[6];
			rect.g_h = messbuff[7];
			WindowSize (messbuff[3], &rect);
			break;

		case WM_MOVED:
			rect.g_x = messbuff[4];
			rect.g_y = messbuff[5];
			rect.g_w = messbuff[6];
			rect.g_h = messbuff[7];
			WindowMove (messbuff[3], &rect);
			break;

		case WM_FULLED:
			WindowFull (messbuff[3]);
			break;

		case WM_ARROWED:
			DoArrowed (messbuff[3], messbuff[4]);
			break;

		case WM_HSLID:
			break;

		case WM_VSLID:
			DoVerticalSlider (messbuff[3], messbuff[4]);
			break;

		/* Neue Nachrichten unter MultiTOS
		 */
		case WM_UNTOPPED:
			break;

		case WM_ONTOP:
			IsTopWindow (messbuff[3]);
			break;

#ifndef WM_BOTTOMED
#define WM_BOTTOMED		33
#endif
		case WM_BOTTOMED:
			DoBottomWindow (messbuff[3]);
			break;

		case AP_TERM:
			/* MagiC schickt sich selbst eine Message :-( */
			
			if (messbuff[1] == _GemParBlk.global[2])
				break;

			/* Wenn der Absender Applikation -1 ist, handelt es
			sich um Ctrl-Alt-Del */
#if 0
venusErr ("mb[]=%04x %04x %04x %04x %04x %04x %04x %04x",
	messbuff[0], messbuff[1], messbuff[2], messbuff[3],
	messbuff[4], messbuff[5], messbuff[6], messbuff[7]);
#endif
			if (messbuff[1] == -1) messbuff[5] = -1;

			if (messbuff[5] != AP_TERM && messbuff[5] != AP_RESCHG)
			{
				Shutdown (messbuff[5]);
			}
			else
			{
				ok = FALSE;
				*silent = 1;
			}
			break;

#ifndef SHUT_COMPLETED
#define SHUT_COMPLETED 60
#endif

		case SHUT_COMPLETED:
			if (messbuff[3] == 1)
				switchOff ();
			else
				shutdownProblem (messbuff[4]);
			break;

		case SH_WDRAW:
		{
			char drive;

			if (messbuff[3] < 26)
			{
				drive = messbuff[3] + 'A';

				*pDirtyDrives |= DriveBit (drive);
			}
			else
			{
				/* Alle Laufwerke sind dirty */
				*pDirtyDrives |= Dsetdrv (Dgetdrv ());
			}
			break;
		}

		case CH_EXIT:
			break;

		case SC_CHANGED:
		{
			char *scrapPath = 0;
			scrapPath = getSPath (scrapPath);
			if (scrapPath)
			{
				PathUpdate (scrapPath);
				free (scrapPath);
			}
			break;
		}

		case AP_DRAGDROP:
			ReceiveDragAndDrop (messbuff[1], /* sender */
				messbuff[3],				/* window */
				messbuff[4],				/* mx */
				messbuff[5],				/* my */
				messbuff[6],				/* kstate */
				messbuff[7]);				/* pipe id */

			break;

		/* xAcc-Protokoll der Stufe 0 von Konrad Hinsen
		 */
		case ACC_ID:
			initAcc (apid, messbuff[1], messbuff[6], messbuff[3], 
					*((char **)(messbuff+4)));
			break;


		case KOBOLD_ANSWER:
			GotKoboldAnswer (messbuff);
			break;


		/* Hier beginnt das Venus-Acc-Protokoll
		 */

		case AV_PROTOKOLL:
			va_func = sentProtoStatus;
			break;

		case AV_GETSTATUS:
			va_func = sendAccStatus;
			break;

		case AV_STATUS:
			va_func = saveAccStatus;
			break;

		case AV_ASKFILEFONT:
			va_func = sendFileFont;
			break;

		case AV_ASKOBJECT:
			va_func = askObject;
			break;

		case AV_OPENWIND:
			remove_update = FALSE;
			va_func = openWind;
			break;

		case AV_STARTPROG:
			remove_update = FALSE;
			va_func = startProg;
			break;

		case AV_ACCWINDOPEN:
			va_func = accWindowOpened;
			break;

		case AV_ACCWINDCLOSED:
			va_func = accWindowClosed;
			break;

		case AV_COPY_DRAGGED:
			remove_update = FALSE;
			va_func = copyObjectsFromAcc;
			break;
#if MERGED
		case AV_ASKCONFONT:
			va_func = sendConsoleFont;
			break;

		case AV_OPENCONSOLE:
			remove_update = FALSE;
			va_func = openConsole;
			break;
#endif
		case AV_PATH_UPDATE:
			remove_update = FALSE;
			va_func = AV_PathUpdate;
			break;

		case AV_WHAT_IZIT:
			va_func = AV_WhatIzIt;
			break;

		case AV_DRAG_ON_WINDOW:
			remove_update = FALSE;
			va_func = AV_DragOnWindow;
			break;

		case AV_EXIT:
			va_func = AV_Exit;
			break;

		case AV_STARTED:
			/* Wir tun da nix */
			break;

		case AV_XWIND:
			remove_update = FALSE;
			va_func = openXWind;
			break;

		default:
			break;
	}

	if (va_func != NULL)
	{
		if (remove_update)
			WindUpdate (END_UPDATE);

		va_func (messbuff);

		if (remove_update)
			WindUpdate (BEG_UPDATE);
	}

	return ok;
}

void MessInit (void)
{
}

void MessExit (void)
{
	word i;

	if (accArray)
	{
		for (i = 0; i < accCount; ++i)
		{
			free (accArray[i].status);
			free (accArray[i].message);
		}

		free (accArray);
	}

	free (xacc_name);
	accArray = NULL;
	accMax = accCount = 0;
}

int StartAccFromMupfel (char *acc_name, ARGVINFO *A, char *command)
{
	return StartAcc (NULL, 0, acc_name, A, command);
}

/*
 * try to start an installed accessory
 */
word StartAcc (WindInfo *wp, word fobj, char *name, 
	ARGVINFO *A, char *command)
{
	word i, acc_id;
	word messbuff[8];
	char accname[MAX_FILENAME_LEN], *cp, **pm;
	size_t len;

	if((cp = strrchr (name, '\\')) != NULL)
		strcpy (accname, ++cp);
	else
		strcpy (accname, name);

	if((cp = strchr (accname, '.')) != NULL)
		*cp = '\0';
	len = strlen (accname);
	for (i = 1; i <= (8 - len); i++)
		strcat (accname, " ");

	if (((acc_id = appl_find (accname)) != -1)
		|| ((acc_id = appl_find (strupr (accname))) != -1))
	{							/* acc. installed */
		AccInfo *entry;

		entry = makeOrGetAccInfo (acc_id, 0, accname);
		if (entry == NULL)
			return FALSE;

		free (entry->message);
		entry->message = NULL;

		messbuff[0] = VA_START;
		messbuff[1] = apid;
		messbuff[2] = 0;
		pm = (char **)&messbuff[3];

		if (A)
		{
			entry->message = Argv2String (pMainAllocInfo, A,
				VA_ACC_QUOTING (entry->protoStatus));
			*pm = entry->message;
		}
		else if (command)
		{
			entry->message = malloc (strlen (command) + 1);
			if (entry->message == NULL)
				return FALSE;

			strcpy (entry->message, command);
			*pm = entry->message;
		}
		else
			*pm = "";

		if (wp)
		{
			DeselectAllObjects ();
			SelectObject (wp,fobj, TRUE);
			flushredraw ();
		}

		return appl_write (acc_id, 16, &messbuff);
	}
	else
		return FALSE;
}

void HandlePendingMessages (int with_delay)
{
	MEVENT E;
	word messbuff[8];
	word events;
	int dummy;

	E.e_flags = (MU_MESAG|MU_TIMER);
	E.e_time = 0L;
	E.e_mepbuf = messbuff;

	for (;;)
	{
		WindUpdate (END_UPDATE);
		events = evnt_event(&E);
		WindUpdate (BEG_UPDATE);

		if (!(events & MU_MESAG)) break;

		HandleMessage (messbuff, E.e_kr, &dummy);
		if (with_delay) E.e_time = 200L;
	}
}
