/*
 * @(#) Mupfel\hash.c
 * @(#) Stefan Eissing, 29. januar 1993
 *
 * Hashing fÅr externe Kommandos
 */


#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"

#include "mglob.h"

#include "chario.h"
#include "getopt.h"
#include "hash.h"
#include "locate.h"


/* internal texts
 */
#define NlsLocalSection "M.hash"
enum NlsLocalText{
OPT_hash,	/*[ -r ] [ name ... ]*/
HLP_hash,	/*Hashtabelle lîschen, anzeigen, name eintragen*/
HA_TITLE,	/*Treffer Aufwand  Kommando       Voller Pfad\n*/
HA_ENTRY,	/*%7d %7d  %-15s%s\n*/
HA_ERROR,	/*%s: Fehler beim Suchen von %s\n*/
HA_NOTFOUND,	/*%s: %s nicht gefunden\n*/
HA_ISFUNC,	/*%s: %s ist eine Funktion\n*/
HA_ISINTERNAL,	/*%s: %s ist ein internes Kommando\n*/
HA_ISDATA,	/*%s: %s ist nicht ausfÅhrbar\n*/
HA_NOTHASHED,	/*%s: %s kann nicht gehasht werden\n*/
};


/* Inititalisiere die Hashing-Informationen in <M>
 */
void InitHash (MGLOBAL *M)
{
	M->hash = NULL;
}


/* Gib einen Pointer auf eine Kopier von <old> zurÅck, oder
 * NULL im Fehlerfall
 */
static HASHINFO *dupHashInfo (MGLOBAL *M, const HASHINFO *old)
{
	HASHINFO *H;
	
	H = mmalloc (&M->alloc, sizeof (HASHINFO));
	if (!H)
		return NULL;
	
	*H = *old;
	
	H->command = StrDup (&M->alloc, old->command);
	if (!H->command)
	{
		mfree (&M->alloc, H);
		return NULL;
	}

	H->full_name = StrDup (&M->alloc, old->full_name);
	if (!H->full_name)
	{
		mfree (&M->alloc, H->command);
		mfree (&M->alloc, H);
		return NULL;
	}

	if (old->executer)
	{
		H->executer = StrDup (&M->alloc, old->executer);
		if (!H->executer)
		{
			mfree (&M->alloc, H->command);
			mfree (&M->alloc, H->full_name);
			mfree (&M->alloc, H);
			return NULL;
		}
	}
	
	return H;
}


/* Vererbe die Hashing-Informationen von der Shell <M> in den
 * neuen Shell-Level <new>. Momentan wird kein Fehler zurÅck-
 * geliefert, da diese Informationen nichts mit der FunktionalitÑt
 * zu tun haben.
 */
int ReInitHash (MGLOBAL *M, MGLOBAL *new)
{
	HASHINFO *akt, **newhash;
	
	new->hash = NULL;

	newhash = &((HASHINFO *)new->hash);
	akt = M->hash;

	while (akt)
	{
		*newhash = dupHashInfo (new, akt);
		if (!*newhash)
			return FALSE;
		
		(*newhash)->next = NULL;
		newhash = &((*newhash)->next);
		akt = akt->next;
	}
	
	return TRUE;
}


/* Gib die HASHINFO <H> wieder frei.
 */
static void freeHashInfo (MGLOBAL *M, HASHINFO *H)
{
	mfree (&M->alloc, H->command);
	mfree (&M->alloc, H->full_name);

	if (H->executer)
		mfree (&M->alloc, H->executer);
		
	mfree (&M->alloc, H);
}


/* Lîsche alle Hashing-Informationen im Shell-Level <M>
 */
void ClearHash (MGLOBAL *M)
{
	HASHINFO *H, *next;
	
	H = M->hash;
	
	while (H)
	{
		next = H->next;
		freeHashInfo (M, H);
		H = next;
	}
	
	M->hash = NULL;
}


/* Wird beim Verlassen der Shell <M> aufgerufen und gibt den
 * Speicherplatz wieder frei
 */
void ExitHash (MGLOBAL *M)
{
	ClearHash (M);
}


/* Liefere einen Pointer auf die Hashinformationen fÅr das
 * Kommando <command> zurÅck. Falls keine vorliegen, wird der 
 * NULL-Zeiger zurÅckgegeben.
 * Wenn <increment> TRUE ist, wird der Hit-ZÑhler inkrementiert.
 */
const HASHINFO *GetHashedCommand (MGLOBAL *M, const char *command,
	int increment)
{
	HASHINFO *H;
	
	H = M->hash;
	
	while (H)
	{
		if (!strcmp (H->command, command))
		{
			if (increment)
				++H->hits;
				
			return H;
		}
		
		H = H->next;
	}
	
	return NULL;
}


/* Entferne die Hashing-Informationen fÅr das Kommando <command>
 * Ergibt FALSE, wenn das Kommando ungehasht war.
 */
int RemoveHash (MGLOBAL *M, const char *command)
{
	HASHINFO *H, **pprev;
	
	pprev = &((HASHINFO *)M->hash);
	H = M->hash;
	while (H)
	{
		if (!strcmp (H->command, command))
		{
			*pprev = H->next;
			freeHashInfo (M, H);
			
			return TRUE;
		}
		
		pprev = &H->next;
		H = H->next;
	}
	
	return FALSE;
}


/* Trag folgende Informationen fÅr das Kommando <command> in das
 * Hashing ein. 
 * - full_name   -   der volle Name mit Pfad des Kommandos
 * - executer    -   das zustÑnsige Programm, falls es selbst nicht
 *                   ausfÅhrbar ist.
 * - location    -   Informationen fÅr locate.c (Typ des Kommandos)
 * - costs       -   Suchkosten zum Auffinden dieses Kommandos
 */
int EnterHash (MGLOBAL *M, const char *command, const char *full_name,
	const char *executer, int location, int costs, int hits)
{
	HASHINFO old;
	HASHINFO *H;

	if (GetHashedCommand (M, command, FALSE))
	{
		dprintf ("%s already hashed\n", command);
		return FALSE;
	}
	
	memset (&old, 0, sizeof (HASHINFO));
	(const char *)old.command = command;
	(const char *)old.full_name = full_name;
	(const char *)old.executer = executer;
	old.location = location;
	old.costs = costs;
	old.hits = hits;
	old.flags.is_absolute = IsAbsoluteName (full_name);

	H = dupHashInfo (M, &old);
	if (!H)
		return FALSE;
	
	{
		HASHINFO *akt, **pprev;
		
		pprev = &((HASHINFO *)M->hash);
		akt = M->hash;
		while (akt)
		{
			if (strcmp (akt->command, H->command) > 0)
				break;
			
			pprev = &akt->next;
			akt = akt->next;
		}
		
		*pprev = H;
		H->next = akt;
	}
	
	return TRUE;
}


/* Benachrichtige das Hashing, daû sich der aktuelle Pfad geÑndert
 * hat. Dazu wird das Flag dir_changed gesetzt. Ist ein Kommando
 * nur mit relativem Pfad bekannt, muû erneut danach gesucht
 * werden, wenn es gebraucht wird.
 */
void NoticeHash (MGLOBAL *M)
{
	HASHINFO *H;
	
	H = M->hash;
	while (H)
	{
		H->flags.dir_changed = 1;
		H = H->next;
	}
}


/* Internes Kommando <hash>, daû zum hashen von Kommandos, zum 
 * Lîschen der Informationen und zum Ausgeben der Informationen
 * dient
 */
int m_hash (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	int c;
	int clearflag;
	struct option long_option[] =
	{
		{ "remove", FALSE, NULL, 'r'},
		{ NULL,0,0,0 },
	};
	int opt_index = 0;
	int retcode = 0;

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_hash), 
			NlsStr(HLP_hash));
	
	clearflag = FALSE;
	optinit (&G);

	while ((c = getopt_long (M, &G, argc, argv, "r", long_option,
		&opt_index)) != EOF)
	{
		if (!c)
			c = long_option[opt_index].val;
			
		switch (c)
		{
			case 0:
				break;

			case 'r':
				clearflag = TRUE;
				break;

			default:	
				return PrintUsage (M, argv[0], NlsStr(OPT_hash));
		}
	}
	
	/* Hash-Informationen sollen gelîscht werden
	 */
	if (clearflag)
		ClearHash (M);

	/* Noch Parameter da? Dann mÅssen diese Kommando gehasht werden
	 */
	if (G.optind < argc)
	{
		int i;
		LOCATION L;
		
		for (i = G.optind; i < argc && !intr (M); ++i)
		{
			/* Versuche das Kommando zu finden
			 */
			if (!Locate (M, &L, argv[i], FALSE, LOC_ALL))
			{
				if (L.broken)
					eprintf (M, NlsStr(HA_ERROR), argv[0], argv[i]);
				else
					eprintf (M, NlsStr(HA_NOTFOUND), argv[0], argv[i]);

				return 1;
			}
			else
			{
				switch (L.location)
				{
					case Function:
						eprintf (M, NlsStr(HA_ISFUNC), argv[0], argv[i]);
						retcode = 1;
						break;
			
					case Internal:
						eprintf (M, NlsStr(HA_ISINTERNAL), argv[0], argv[i]);
						retcode = 1;
						break;
			
					case Data:
						eprintf (M, NlsStr(HA_ISDATA), argv[0], argv[i]);
						retcode = 1;
						break;
						
					case NotFound:
						eprintf (M, NlsStr(HA_NOTFOUND), argv[0], argv[i]);
						retcode = 1;
						break;
					
					default:
						if (!L.flags.was_hashed
							&& !L.flags.can_be_hashed)
						{
							eprintf (M, NlsStr(HA_NOTHASHED), argv[0], argv[i]);
							retcode = 1;
						}
						break;
				}
			
				FreeLocation (M, &L);
			}
		}
	}
	else if (!clearflag)
	{
		HASHINFO *H;
		
		/* Hier hatten wir keine Paramter und Optionen und geben
		 * also die Liste der gehashten Kommandos aus.
		 */
		mprintf (M, NlsStr(HA_TITLE));

		H = M->hash;
		while (H && !intr (M))
		{
			mprintf (M, NlsStr(HA_ENTRY), H->hits, H->costs,
				H->command, H->full_name);
			
			H = H->next;
		}
	}

	return retcode;
}
