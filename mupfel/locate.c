/*
 * @(#) Mupfel\Locate.c
 * @(#) Stefan Eissing, 30. Dezember 1993
 *
 * Bestimme die Art und den Aufenthaltsort eines Kommandos
 *
 * jr 22.4.95
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"
#include "..\common\walkdir.h"

#include "mglob.h"

#include "chario.h"
#include "hash.h"
#include "internal.h"
#include "locate.h"
#include "mdef.h"
#include "nameutil.h"


/* internal texts
 */
#define NlsLocalSection "M.locate"
enum NlsLocalText{
LOC_RESTRICTED /*Kommandos mit Pfadangabe sind nicht erlaubt\n*/
};

#define MYSELF			"mupfel"
#define SUFFIX_DEFAULT	"prg;app;gtp;tos;ttp;mup"


/* Wir wissen, daû L eine richtige Datei bezeichnet. 
 * L->loc.file.full_name und L->loc.file.looked_for sind schon
 * richtig besetzt.
 * GefÅllt werden L->location und L->executer.
 */
static int decideFileType (MGLOBAL *M, LOCATION *L)
{
	char *ext;
	
	L->location = NotFound;
	
	/* Versuche zu entscheiden, ob es eine .MUP-Datei ist.
	 */
	ext = strrchr (L->loc.file.full_name, '.');
	if (ext)
	{
		if (!stricmp (ext + 1, "MUP"))
		{
			L->location = Script;
			return TRUE;
		}
	}
	
	/* Ist es ein ausfÅhrbares Binary?
	 */
	if (access (L->loc.file.full_name, A_EXEC, &L->broken))
	{
		L->location = Binary;
		return TRUE;
	}
	else if (L->broken)
		return FALSE;
	
	/* OK, er will es also auf die harte Tour!
	 * Linsen wir doch mal rein...
	 */
	L->loc.file.executer = GetExecuterFromFile (&M->alloc, 
		L->loc.file.full_name, &L->broken);
		
	if (L->loc.file.executer)
	{
		if (M->shell_flags.slash_conv)
			ConvertSlashes (L->loc.file.executer, NULL);
		
		if (!stricmp (L->loc.file.executer, MYSELF)
			|| strstr (L->loc.file.executer, "\\bin\\sh"))
		{
			L->location = Script;
			mfree (&M->alloc, L->loc.file.executer);
			L->loc.file.executer = NULL;
		}
		else
			L->location = AlienScript;
		
		return TRUE;
	}
	else if (L->broken)
	{
		return FALSE;
	}
	
	L->location = Data;
	return TRUE;
}

/* Suche nach einem Kommando <L->command> im Verzeichnis <path>,
 * das die Endung <suffix> hat. Wenn gefunden, fÅlle die Struktur
 * <L> mit den Informationen.
 * Ansonsten (auch im Fehlerfall) gib FALSE zurÅck.
 */
static int lookForExternal (MGLOBAL *M, LOCATION *L, 
				const char *path, const char *suffix)
{
	size_t len;
	char *tmp;
	
	len = strlen (L->command) + strlen(path) + strlen (suffix) + 3;
	tmp = mmalloc (&M->alloc, len);
	if (!tmp)
	{
		L->broken = TRUE;
		return FALSE;
	}
	
	if (*path)
	{
		strcpy (tmp, path);
		AddFileName (tmp, L->command);
	}
	else
		strcpy (tmp, L->command);
		
	if (*suffix)
	{
		strcat (tmp, ".");
		strcat (tmp, suffix);
	}
	
	{
		char *normal;
		
		normal = NormalizePath (&M->alloc, tmp, M->current_drive, 
			M->current_dir);

		if (!normal)
		{
			mfree (&M->alloc, tmp);
			/* L->broken = TRUE;
			 *
			 * Dies ist auskommentiert, damit die Suche auch
			 * bei einem nichtexistierenden Verzeichnis weitergehen
			 * kann. Normalerweise sollte an dieser Stelle noch nicht
			 * der Critical Error Handler aktiv geworden sein.
			 */
			return FALSE;
		}
	
		++L->costs;

		if (IsRealFile (normal, &L->broken))
		{
			L->loc.file.looked_for = tmp;
			L->loc.file.full_name = normal;
			if (normal[1] == ':')
				normal[0] = toupper (normal[0]);
			L->loc.file.executer = NULL;

			if (decideFileType (M, L))
				return TRUE;
		}
		
		mfree (&M->alloc, normal);
	}
	
	mfree (&M->alloc, tmp);
	return FALSE;
}

/* TRUE, wenn name Extension hat und diese in SUFFIX vorkommt.
 */
int ExtInSuffix (MGLOBAL *M, const char *name)
{
	const char *env_suffix, *extension;
	char *suffix, *orig_suffix;
	char *save_token;
	
	extension = strrchr (name, '.');
	if (!extension)
		return FALSE;
	++extension;
	
	env_suffix = GetEnv (M, "SUFFIX");
	if (!env_suffix)
		env_suffix = SUFFIX_DEFAULT;
	
	orig_suffix = suffix = StrDup (&M->alloc, env_suffix);
	if (!suffix)
		return FALSE;

	suffix = StrToken (suffix, ",;", &save_token);
	while (suffix)
	{
		if (!stricmp (extension, suffix))
			break;
	
		/* NÑchstes Element
		 */
		suffix = StrToken (NULL, ",;", &save_token);
	}
	
	mfree (&M->alloc, orig_suffix);

	return (suffix != NULL);
}


/* Suche nach <L->command> in dem Verzeichnis <path> und hÑnge
 * dazu Endungen an <command>, die in SUFFIX stehen.
 * Gib bei Fehlern oder nicht gefunden FALSE zurÅck.
 */
static int locateWithSuffix (MGLOBAL *M, LOCATION *L, 
	const char *path)
{
	const char *env_suffix;
	char *suffix, *orig_suffix;
	char *save_token;
	
	if (Extension (L->command))
		return FALSE;
	
	env_suffix = GetEnv (M, "SUFFIX");
	if (!env_suffix)
		env_suffix = SUFFIX_DEFAULT;
	
	orig_suffix = suffix = StrDup (&M->alloc, env_suffix);
	if (!suffix)
	{
		L->broken = TRUE;
		return FALSE;
	}

	suffix = StrToken (suffix, ",;", &save_token);
	
	while (suffix)
	{
		if (lookForExternal (M, L, path, suffix))
		{
			mfree (&M->alloc, orig_suffix);
			return TRUE;
		}
		else if (L->broken)
		{
			mfree (&M->alloc, orig_suffix);
			return FALSE;
		}
	
		/* NÑchstes Element
		 */
		suffix = StrToken (NULL, ",;", &save_token);
	}
	
	mfree (&M->alloc, orig_suffix);
	return FALSE;
}


/* Suche ein Kommando im Dateisystem. Benutze dazu, wenn kein Pfad
 * angegeben ist, die Variable PFAD und wenn keine Endung da ist
 * die Variable SUFFIX fÅr mîgliche Extensions.
 * Bei Fehlern oder nicht gefunden wird FALSE zurÅckgeliefert.
 */
static int locateExternal (MGLOBAL *M, LOCATION *L)
{
	char *path, *orig_path;
	char *save_token;
	
	/* Ist es vielleicht ein Kommando mit Pfadangabe?
	 */
	if (strchr (L->command,'\\') != NULL 
		|| L->command[0] == '.' 
		|| L->command[1] == ':')
	{
		L->flags.can_be_hashed = 0;
		
		if (M->shell_flags.restricted)
		{
			eprintf (M, NlsStr(LOC_RESTRICTED));
			return FALSE;
		}
		else
			return (lookForExternal (M, L, "", "")
			|| (!L->broken && locateWithSuffix (M, L, "")));
	}
	
	orig_path = path = StrDup (&M->alloc, GetPATHValue (M));
	if (!path)
	{
		L->broken = TRUE;
		return FALSE;
	}

	path = StrToken (path, ",;", &save_token);	
	while (path && !intr (M) && !checkintr (M))
	{
		if (lookForExternal (M, L, path, "")
			|| (!L->broken && locateWithSuffix (M, L, path)))
		{
			mfree (&M->alloc, orig_path);
			L->flags.can_be_hashed = 1;
			
			return TRUE;
		}
		else if (L->broken)
		{
			mfree (&M->alloc, orig_path);
			return FALSE;
		}
		
		/* nÑchstes Element
		 */
		path = StrToken (NULL, ",;", &save_token);	
	}
	
	mfree (&M->alloc, orig_path);
	return FALSE;
}


/* Sucht das Kommando <command> und fÅllt, wenn gefunden, die
 * Struktur <L> mit den Informationen.
 * <used> ist TRUE, wenn das Kommando wirklich ausgefÅhrt werden
 * soll und wird fÅr die Trefferrate beim Hashing benutzt.
 *
 * Konnte das Kommando nicht gefunden werden, oder ist ein
 * Fehler aufgetreten, wird FALSE zurÅckgeliefert.
 */
int Locate (MGLOBAL *M, LOCATION *L, const char *command, int used,
	int where_to_look)
{
	memset (L, 0, sizeof (LOCATION));
	L->location = NotFound;
	L->broken = FALSE;

	L->command = StrDup (&M->alloc, command);

	if (L->command == NULL)
	{
		L->broken = TRUE;
		return FALSE;
	}
	
	/* Zuerst suchen wir nach einer Funktion gleichen Namens.
	 */
	if (where_to_look & LOC_FUNCTION)
	{
		NAMEINFO *n;
		
		n = GetNameInfo (M, L->command);
		
		if (n && n->flags.function)
		{
			L->location = Function;
			L->loc.func.n = n;
			return TRUE;
		}
	}
	
	/* Es ist keine Funktion. Vielleicht ein internes Kommando?
	 */
	if (where_to_look & LOC_INTERNAL)
	{
		int intern_command_nr;
		
		if (IsInternalCommand (M, L->command, FALSE, &intern_command_nr))
		{
			L->location = Internal;
			L->loc.intern.command_number = intern_command_nr;
			return TRUE;
		}
	}

	/* Ist auch kein internes Kommando. Suchen wir also in der
	 * Hashtabelle.
	 */
	if (where_to_look & LOC_EXTERNAL)
	{
		const HASHINFO *H;
		
		H = GetHashedCommand (M, L->command, used);
		if (H != NULL)
		{
			char *fullname;
			
			fullname = NormalizePath (&M->alloc, H->full_name, 
						M->current_drive, M->current_dir);
			
			if ((!H->flags.is_absolute && H->flags.dir_changed)
				|| (fullname && !access (fullname, A_READ, &L->broken)))
			{
				RemoveHash (M, L->command);
				mfree (&M->alloc, fullname);
			}
			else
			{
				L->location = H->location;
				L->costs = H->costs;
				L->flags.was_hashed = 1;
	
				L->loc.file.looked_for = StrDup (&M->alloc, 
					H->full_name);
				L->loc.file.full_name = fullname;

				if (!L->loc.file.looked_for
					|| !L->loc.file.full_name)
				{
					L->broken = TRUE;
					FreeLocation (M, L);
					return FALSE;
				}
				
				if (H->executer)
				{
					L->loc.file.executer = StrDup (&M->alloc, H->executer);
					if (!L->loc.file.executer)
					{
						L->broken = TRUE;
						FreeLocation (M, L);
						return FALSE;
					}
				}
				else
					L->loc.file.executer = NULL;
	
				return TRUE;
			}
		}
	}
	
	/* Hmmm. Nicht gehasht. Dann suchen wir in Filesystem mittels
	 * PATH.
	 */
	if (!L->broken && (where_to_look & LOC_EXTERNAL))
	{
		if (locateExternal (M, L))
		{
			if (L->flags.can_be_hashed && L->location != NotFound
				&& L->location != Data)
			{
				/* Das Kommando kann gehasht werden, da es Åber
				 * PATH lokalisiert wurde.
				 */
				EnterHash (M, L->command, L->loc.file.looked_for, 
					L->loc.file.executer, L->location, L->costs,
					used? 1 : 0);
			}
			return TRUE;
		}
	}
	
	/* Wenn wir hier angekommen sind, ist das Kommando nicht zu
	 * finden bzw. existiert nicht.
	 */
	
	mfree (&M->alloc, L->command);
	L->command = NULL;
	return FALSE;
}


void FreeLocation (MGLOBAL *M, LOCATION *L)
{
	mfree (&M->alloc, L->command);
	L->command = NULL;
	
	switch (L->location)
	{
		case AlienScript:
			mfree (&M->alloc, L->loc.file.executer);
		case Binary:
		case Script:
		case Data:
			mfree (&M->alloc, L->loc.file.full_name);
			mfree (&M->alloc, L->loc.file.looked_for);
			break;
	}
}


static int getPathCompletions (MGLOBAL *M, ARGVINFO *A, char *what,
	char *path, int filter_suffix)
{
	WalkInfo W;
	char *newname;
	char filename[MAX_FILENAME_LEN];
	int broken;
	long stat;
	
	newname = NormalizePath (&M->alloc, path, M->current_drive,
				M->current_dir);
	if (!newname)
		return FALSE;
	
	broken = FALSE;

	stat = InitDirWalk (&W, WDF_LOWER|WDF_FOLLOW|
		WDF_IGN_XATTR_ERRORS, newname, MAX_FILENAME_LEN, filename);
	
	if (!stat)
	{
		while (!broken && ((stat = DoDirWalk (&W)) == 0L))
		{
			/* match? 
			 */
			if ((!*what || (strstr (filename, what) == filename))
				&& (!filter_suffix || !strchr (filename, '.')
					|| ExtInSuffix (M, filename)))
			{
				XATTR attr;
				char *fullname;
				
				fullname = mmalloc (&M->alloc, strlen (newname)
					+ strlen (filename) + 3UL);
				if (!fullname)
				{
					broken = TRUE;
					break;
				}
				strcpy (fullname, newname);
				AddFileName (fullname, filename);
				
				if ((GetXattr (fullname , FALSE, TRUE, &attr) == 0L)
					&& ((attr.attr & 
					(FA_SUBDIR|FA_HIDDEN|FA_SYSTEM|FA_VOLUME)) == 0))
				{
					if (!StoreInArgv (A, filename))
						broken = TRUE;
				}
				
				mfree (&M->alloc, fullname);
			}
		}
		
		ExitDirWalk (&W);
	}

	if (stat && !broken)
		broken = IsBiosError (stat);
	
	mfree (&M->alloc, newname);
	
	return !broken;
}


int GetExternalCompletions (MGLOBAL *M, ARGVINFO *A, char *what)
{
	char *orig_path, *path, *save_token;
	int filter_suffix;
	
	if (strlen (what) >= MAX_FILENAME_LEN)
		return TRUE;
	
	filter_suffix = (strchr (what, '.') == NULL);
	
	orig_path = path = StrDup (&M->alloc, GetPATHValue (M));
	if (!path)
		return FALSE;

	path = StrToken (path, ",;", &save_token);	
	while (path)
	{
		if (checkintr (M)
			|| !getPathCompletions (M, A, what, path, filter_suffix))
		{
			mfree (&M->alloc, orig_path);
			return FALSE;
		}
		
		/* nÑchstes Element
		 */
		path = StrToken (NULL, ",;", &save_token);	
	}
	
	mfree (&M->alloc, orig_path);
	return TRUE;
}
