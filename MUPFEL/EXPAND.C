/*
 * @(#) Mupfel\Expand.c
 * @(#) Stefan Eissing, 31. Januar 1993
 *
 * jr 27.7.94
 */


#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tos.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\walkdir.h"
#include "mglob.h"

#include "expand.h"
#include "makeargv.h"
#include "mdef.h"
#include "..\common\wildmat.h"


#define DIRARRAYSIZE	30

#define FA_NOLABELORHIDDEN	(FA_SYSTEM|FA_SUBDIR)

typedef struct
{
	char drive[4];
	char *dir_wild[DIRARRAYSIZE];
	const char *dir_quot[DIRARRAYSIZE];
	char *file_wild;
	const char *file_quot;
	
	int akt_index;
	int dir_size;
	int dir_expanded;
	
	ARGVINFO *A;
} EXPANDINFO;

static void clearExpandInfo (EXPANDINFO *E)
{
	memset (E, 0, sizeof (EXPANDINFO));
}

static int makeExpandInfo (EXPANDINFO *E, ARGVINFO *A,
		char *string, const char *quoted)
{
	size_t start_index, scan_index;
	
	clearExpandInfo (E);
	
	/* ARGVINFO merken 
	 */
	E->A = A;
	
	/* Extahiere zuerst die Laufwerks-Information.
	 */
	start_index = 0;
	if (*string && (string[1] == ':'))	/* Laufwerks-Bezeichner? */
	{
		start_index = (string[2] == '\\') ? 3 : 2;
		strncpy (E->drive, string, start_index);
		E->drive[start_index] = '\0';
		
		if (!LegalDrive (E->drive[0]))
		{
			return FALSE;
		}
	}
	else
	{
		if (*string == '\\')	/* Root-Verzeichnis? */
		{
			strcpy (E->drive, "\\");
			start_index = 1;
		}
	}
	
	/* Nun zerlege den Rest in seine einzelnen Ordner-Teile, die
	 * durch \ getrennt sein mssen. Doppelte Backslashes werden
	 * ignoriert.
	 */
	scan_index = start_index;
	while (string[scan_index])
	{
		if (string[scan_index] == '\\')
		{
			if (E->dir_size >= DIRARRAYSIZE)
				return FALSE;

			string[scan_index] = '\0';
			E->dir_wild[E->dir_size] = &string[start_index];
			E->dir_quot[E->dir_size] = &quoted[start_index];
			++E->dir_size;
			start_index = scan_index + 1;
		}
		++scan_index;
	}
	/* Wenn noch ein Rest vorhanden ist, dann ist das der 
	 * Dateiname.
	 */
	E->file_wild = &string[start_index];
	E->file_quot = &quoted[start_index];
	
	return TRUE;
}

static char *makeBaseDir (MGLOBAL *M, EXPANDINFO *E, 
	const char *filename)
{
	size_t len;
	int i;
	char *dir;
	
	len = strlen (filename) + 6;
	for (i = 0; i < E->akt_index; ++i)
	{
		len += strlen (E->dir_wild[i]) + 1;
	}
	
	dir = mmalloc (&M->alloc, len);
	if (!dir)
		return NULL;
	
	strcpy (dir, E->drive);
	for (i = 0; i < E->akt_index; ++i)
	{
		strcat (dir, E->dir_wild[i]);
		strcat (dir, "\\");
	}

	strcat (dir, filename);
	RemoveDoubleBackslash (dir);
	return dir;
}

static int expandFile (MGLOBAL *M, EXPANDINFO *E, int *expanded)
{
	char *base_dir;
	int has_wildcard, retcode;
	int broken = FALSE;
	long stat;
	
	has_wildcard = ContainsWildcard (E->file_wild, E->file_quot);
	
	/* Kein Wildcard und kein Directory expandiert: es wird nicht
	 * weiter nachgeschaut.
	 */
	if (!has_wildcard && !E->dir_expanded)
	{
		return TRUE;
	}
	
	retcode = TRUE;
	
	/* Es ist ein Wildcard enthalten: laufe durch das Verzeichnis
	 * und berprfe alle Dateien.
	 */
	if (has_wildcard)
	{
		WalkInfo W;
		char filename[MAX_FILENAME_LEN];
		
		base_dir = makeBaseDir (M, E, "");
		if (base_dir == NULL) return FALSE;
		
		stat = InitDirWalk (&W, WDF_ATTRIBUTES|WDF_LOWER|
			WDF_FOLLOW|WDF_IGN_XATTR_ERRORS, base_dir,
			MAX_FILENAME_LEN, filename);

		if (!stat)
		{
			while ((stat = DoDirWalk (&W)) == 0L)
			{
				/* ".", ".." und versteckte Dateien werden nicht 
				 * durchgelassen.
				 */
				if (!strcmp (filename, ".") 
					|| !strcmp (filename, ".."))
				{
					continue;
				}
				if (W.xattr.attr & FA_HIDDEN)
				{
					continue;
				}
					
				if (wildmatch (filename, E->file_wild, W.flags.case_sensitiv))
				{
					char *new_name;
					
					/* Passende Datei gefunden: speichere sie in
					 * ARGVINFO ab.
					 */
					new_name = makeBaseDir (M, E, filename);
					if (new_name)
					{
						retcode = StoreInExpArgv (E->A, new_name);
						mfree (&M->alloc, new_name);
						*expanded = TRUE;
					}
					else
					{
						retcode = FALSE;
						break;
					}
				}
			}
			
			ExitDirWalk (&W);
		}
	}
	else
	{
		/* Kein Wildcard: wir berprfen nur, ob eine solche
		 * Datei existiert.
		 */
		base_dir = makeBaseDir (M, E, E->file_wild);
		retcode = (base_dir != NULL);
		
		if (retcode)
		{
			if ((E->file_wild[0] == '\0')
				|| access (base_dir, A_EXIST, &broken))
			{
				*expanded = TRUE;
				retcode = StoreInExpArgv (E->A, base_dir);
			}
		}
	}

	mfree (&M->alloc, base_dir);
	return retcode;
}

static int expandDirs (MGLOBAL *M, EXPANDINFO *E, int *expanded)
{
	/* šberspringe alle Verzeichnisse, die keinen Wildcard enthalten
	 */
	while ((E->akt_index < E->dir_size) && 
		!ContainsWildcard (E->dir_wild[E->akt_index], 
		E->dir_quot[E->akt_index]))
	{
		++E->akt_index;
	}
	
	/* Wenn der Index kleiner als die Gesamtgr”že ist, dann haben
	 * wir ein Verzeichnis mit einem Wildcard, den wir expandieren
	 * mssen
	 */
	if (E->akt_index < E->dir_size)
	{
		WalkInfo W;
		char *base_dir;
		char filename[MAX_FILENAME_LEN];
		int retcode = TRUE;
		long stat;

		base_dir = makeBaseDir (M, E, "");
		if (!base_dir) return FALSE;
		
		stat = InitDirWalk (&W, WDF_ATTRIBUTES|WDF_LOWER|
			WDF_FOLLOW|WDF_IGN_XATTR_ERRORS, base_dir,
			MAX_FILENAME_LEN, filename);
			
		if (!stat)
		{
			while ((stat = DoDirWalk (&W)) == 0L)
			{
				if (!(W.xattr.attr & FA_SUBDIR))
					continue;
					
				if (!strcmp (filename, ".") 
					|| !strcmp (filename, ".."))
				{
					continue;
				}
				
				if (!wildmatch (filename, E->dir_wild[E->akt_index], 
					W.flags.case_sensitiv))
				{
					continue;
				}
				
				/* Hier haben wir nun einen Match auf den Wildcard
				 * gefunden. Wir retten den Wildcard, tragen den
				 * Match ein und rufen uns erneut auf.
				 */
				E->dir_expanded = TRUE;
				{
					char *dir_save;
					int index_save;
					
					dir_save = E->dir_wild[E->akt_index];
					E->dir_wild[E->akt_index] = filename;
					index_save = E->akt_index;
					
					retcode = expandDirs (M, E, expanded);
					
					E->akt_index = index_save;
					E->dir_wild[E->akt_index] = dir_save;
				}
			}
			
			ExitDirWalk (&W);
		}

		mfree (&M->alloc, base_dir);
		return retcode;
	}
	else
		return expandFile (M, E, expanded);
}

static int cmpString (const char **p1, const char **p2)
{
	return strcmp (*p1, *p2);
}

/* Expandiere, wenn erforderlich und erlaubt, <subst_string> und
 * trage es in <A> ein.
 * Gibt bei zuwenig Speicher FALSE zurck.
 */
int ExpandAndStore (MGLOBAL *M, ARGVINFO *A, char *subst_string,
		const char *subst_quoted)
{
	char *string;
	int retcode, expanded;
	int old_argc;

	string = StrDup (&M->alloc, subst_string);
	if (!string)
		return FALSE;
		
	expanded = FALSE;
	old_argc = A->argc;
	if (!M->shell_flags.no_globber)	/* Expansion erlaubt? */
	{
		EXPANDINFO E;
		int expand_broken = FALSE;

		if (makeExpandInfo (&E, A, string, subst_quoted))
		{
			expand_broken = !expandDirs (M, &E, &expanded);
		}
		
		if (expand_broken)
		{
			mfree (&M->alloc, string);
			return FALSE;
		}
	}
	
	if (expanded)
	{
		int new_entries = A->argc - old_argc;
		
		retcode = TRUE;
		
		if (new_entries > 1)
		{
			/* Wir haben des ursprnglichen String in mehr als
			 * ein Element expandiert. Also sortieren wir es.
			 */
			qsort (&(A->argv[old_argc]), new_entries,
				sizeof (char *), cmpString);
		}
	}
	else
	{
		retcode = StoreInExpArgv (A, subst_string);
	}
		
	mfree (&M->alloc, string);
	return retcode;
}

