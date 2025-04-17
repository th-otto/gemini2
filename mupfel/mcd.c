/*
 * @(#) Mupfel\mcd.c
 * @(#) Stefan Eissing, 08. Januar 1994
 *
 * Setze und verwalte den aktuellen Pfad
 *
 * jr 22.4.1995
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "hash.h"
#include "mcd.h"
#include "mdef.h"
#include "nameutil.h"

/* internal texts
 */
#define NlsLocalSection "M.mcd"
enum NlsLocalText{
OPT_cd,		/*[dir]*/
HLP_cd,		/*akt. Verzeichnis in dir oder $HOME wechseln*/
HLP_dirs,	/*Directory-Stack anzeigen*/
OPT_pushd,	/*[dir]*/
HLP_pushd,	/*dir bzw . auf Directory-Stack speichern*/
HLP_popd,	/*cd ins letzte gemerkte Verzeichnis*/
HLP_pwd,	/*aktuelles Verzeichnis ausgeben*/
CD_NODIR,	/*%s: kein Argument ist ein Verzeichnis\n*/
CD_RESTRICTED,	/*%s ist nicht erlaubt\n*/
CD_DRIVEARGS,	/*%s: Argumente sind nicht erlaubt\n*/
CD_NOHOME,	/*%s: kein HOME-Verzeichnis gesetzt\n*/
CD_EMPTY,	/*%s: leeres Argument nicht erlaubt\n*/
CD_BADCHANGE,	/*%s: Fehler beim Setzen von %s\n*/
CD_NOMEM,	/*%s: nicht genug freier Speicher vorhanden\n*/
CD_BADDIR,	/*%s: Verzeichnis %s nicht gefunden\n*/
CD_BADDRIVE, /*%c: ist kein gÅltiges Laufwerk\n*/
PU_BADDIR,	/*%s: %s ist kein Verzeichnis\n*/
PU_NODIR,	/*%s: es ist kein Verzeichnis gepusht\n*/
ERR_NOMEM,	/*%s: nicht genug freier Speicher\n*/
};

static int setPWDEnv (MGLOBAL *M)
{
	char tmp[MAX_PATH_LEN + 8];
	const char *value;

	value = GetEnv (M, "PWD");
	if (!value)
		value = "";

	sprintf (tmp, "OLDPWD=%s", value);
		
	if (!PutEnv (M, tmp))
	{
		eprintf (M, NlsStr(CD_NOMEM), "cd");
		return FALSE;
	}
	
	sprintf (tmp, "PWD=%c:%s", M->current_drive, M->current_dir);

	if (!PutEnv (M, tmp))
	{
		eprintf (M, NlsStr(CD_NOMEM), "cd");
		return FALSE;
	}

	return TRUE;
}

int InitCurrentPath (MGLOBAL *M)
{
	char *tmp;
	int drive_nr;
	
	tmp = mmalloc (&M->alloc, MAX_PATH_LEN);
	if (!tmp)
		return FALSE;
	
	drive_nr = Dgetdrv ();
	
	if (GetPathOfDrive (tmp, MAX_PATH_LEN, drive_nr, FALSE))
	{
		mfree (&M->alloc, tmp);
		return FALSE;
	}
	
	M->current_drive = drive_nr + 'a';
	M->current_dir = tmp;
	M->start_path = mmalloc (&M->alloc, MAX_PATH_LEN);
	if (M->start_path)
	{
		sprintf (M->start_path, "%c:%s", M->current_drive,
			M->current_dir);
	}
	
	return setPWDEnv (M);
}

int ReInitCurrentPath (MGLOBAL *old, MGLOBAL *new)
{
	new->current_drive = old->current_drive;
	new->current_dir = StrDup (&new->alloc, old->current_dir);
	new->start_path = NULL;
	
	return (new->current_dir != NULL);
}

void ExitCurrentPath (MGLOBAL *M)
{
	mfree (&M->alloc, M->current_dir);
	mfree (&M->alloc, M->start_path);
	M->current_dir = NULL;
}

int ChangeDir (MGLOBAL *M, const char *path, int store, int *broken)
{
	char *norm;
	char new_drive;
	
	*broken = FALSE;
	
	norm = NormalizePath (&M->alloc, path, M->current_drive, 
		M->current_dir);
	if (!norm) return FALSE;
	
	if (IsDirectory (norm, broken))
	{
		char *dir = &norm[2];
		size_t len = strlen (dir);

		/* Entferne letzten Backslash, falls vorhanden und nicht
		   das Root-Directory */

		if ((len > 1) && (dir[len - 1] == '\\'))
			dir[len - 1] = '\0';

		new_drive = norm[0];
		
		Dsetdrv (toupper (new_drive) - 'A');
		Dsetpath (dir);

		if (store)
		{
			char *tmp;
			
			tmp = StrDup (&M->alloc, dir);
			mfree (&M->alloc, norm);
			
			if (!tmp) return FALSE;
				
			M->current_drive = new_drive;
			mfree (&M->alloc, M->current_dir);
			M->current_dir = tmp;
			
			/* Wir benachrichtigen das Hashing, daû sich der aktuelle
			   Pfad verÑndert hat, damit relative Kommandos erneut
			   gesucht werden. */

			NoticeHash (M);
			
			return setPWDEnv (M);
		}
		else
			mfree (&M->alloc, norm);

		return TRUE;
	}
	
	mfree (&M->alloc, norm);
	return FALSE;
}

int m_cd (MGLOBAL *M, int argc, char **argv)
{
	const char *dir = NULL;
	int is_drive_command, broken;
	char tmp[4];
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_cd), NlsStr(HLP_cd));
	
	if (M->shell_flags.restricted)
	{
		eprintf (M, NlsStr(CD_RESTRICTED), argv[0]);
		return 1;
	}
	
	/* Ist es das "cd" oder "x:" Kommando?
	 */
	is_drive_command = (2 == strlen (argv[0]) && argv[0][1] == ':');
	
	if (is_drive_command && (argc > 1))
	{
		eprintf (M, NlsStr(CD_DRIVEARGS), argv[0]);
		return 1;
	}
	
	if (argc == 1)
	{
		if (!is_drive_command)
		{
			dir = GetEnv (M, "HOME");
			if (!dir)
			{
				eprintf (M, NlsStr(CD_NOHOME), argv[0]);
				return 1;
			}
		}
		else
		{
			/* Wir haben ein "x:" Kommando.
			 */
			strcpy (tmp, "X:\\");
			if (islower (argv[0][0]))
				tmp[2] = '\0';

			tmp[0] = tolower (argv[0][0]);
			dir = tmp;
		}
	}
	else if (argc == 2)
	{
		/* Ein Argument, prÅfe, ob es leer ist.
		 */
		if (argv[1][0])
			dir = argv[1];
		else
		{
			eprintf (M, NlsStr(CD_EMPTY), argv[0]);
			return 1;
		}
	}
	else
	{
		/* Wir haben mehrere Argumente bekommen. Hole das erste
		 * Verzeichnis darunter und versuche es zu setzen.
		 */
		int i, broken = FALSE;

		for (i = 1; i < argc; ++i)
		{
			if (IsDirectory (argv[i], &broken))
			{
				dir = argv[i];
				break;
			}
			else if (broken)
				return (int)ioerror (M, argv[0], argv[i], broken);
		}
		
		if (!dir)
		{
			eprintf (M, NlsStr(CD_NODIR), argv[0]);
			return 1;
		}
	}
	
	if (dir)
	{
		if (ChangeDir (M, dir, TRUE, &broken))
			return 0;
		else if (broken)
		{
			eprintf (M, NlsStr(CD_BADCHANGE), argv[0], dir);
			return 1;
		}
	}
	
	/* Wenn wir hier ankommen, konnten wir <dir> nicht so setzen.
	 * Also holen wir uns CDPATH und versuchen es darÅber.
	 */

	if (*dir != '\\' && *dir != '.')
	{
		char *cdpath = GetEnv (M, "CDPATH");
		
		if (cdpath)
		{
			char *save_token;
			char *orig_cdpath;
			char *tmp;
			
			cdpath = StrDup (&M->alloc, cdpath);
			if (!cdpath)
			{
				eprintf (M, NlsStr(CD_NOMEM), argv[0]);
				return 1;
			}
			orig_cdpath = cdpath;
			
			cdpath = StrToken (cdpath, ",;", &save_token);
			while (cdpath)
			{
				tmp = mmalloc (&M->alloc, 
					strlen (cdpath) + strlen (dir) + 4);
				if (!tmp)
					break;

				strcpy (tmp, cdpath);
				AddFileName (tmp, dir);
				
				if (*cdpath && ChangeDir (M, tmp, TRUE, &broken))
				{
					if (M->shell_flags.interactive)
						mprintf (M, "%s\n", tmp);
					mfree (&M->alloc, tmp);
					mfree (&M->alloc, orig_cdpath);
					return 0;
				}
				else if (broken)
				{
					eprintf (M, NlsStr(CD_BADCHANGE), argv[0], dir);
					mfree (&M->alloc, tmp);
					mfree (&M->alloc, orig_cdpath);
					return 1;
				}
				
				/* nÑchstes Element
				 */
				mfree (&M->alloc, tmp);
				cdpath = StrToken (NULL, ",;", &save_token);
			}
			
			mfree (&M->alloc, orig_cdpath);
		}
	}
	
	if (is_drive_command && !LegalDrive (argv[0][0]))
		eprintf (M, NlsStr(CD_BADDRIVE), argv[0][0]);
	else
		eprintf (M, NlsStr(CD_BADDIR), argv[0], dir);

	return 1;
}

int m_dirs (MGLOBAL *M, int argc, char **argv)
{
	DIRSTACK *ds = M->dir_stack;

	if (!argc)
		return PrintHelp (M, argv[0], "", NlsStr(HLP_dirs));

	if (argc != 1)
		return PrintUsage (M, argv[0], "");
	
		
	mprintf (M, "%c:%s\n", M->current_drive, M->current_dir);

	while (ds)
	{
		mprintf (M, "%s\n", ds->dir);
		ds = ds->next;
	}
	

	return 0;
}

int m_pushd (MGLOBAL *M, int argc, char **argv)
{
	char *actdir, *newdir;
	int retcode;
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_pushd), NlsStr(HLP_pushd));

	if (argc > 2)
		return PrintUsage (M, argv[0], NlsStr(OPT_pushd));
	
	actdir = mmalloc (&M->alloc, strlen (M->current_dir) + 4);
	if (!actdir)
	{
		eprintf (M, NlsStr(ERR_NOMEM), argv[0]);
		return 1;
	}
	sprintf (actdir, "%c:%s", M->current_drive, M->current_dir);
	
	if (argc == 2)
	{
		DIRSTACK *ds;
		
		newdir = argv[1];

		/* erzeuge einen neuen Eintrag im Stack
		 */
		ds = mcalloc (&M->alloc, 1, sizeof (DIRSTACK));
		if (!ds)
		{
			eprintf (M, NlsStr(ERR_NOMEM), argv[0]);
			mfree (&M->alloc, actdir);
			return 1;
		}
		
		ds->dir = NULL;
		ds->next = M->dir_stack;
		M->dir_stack = ds;
	}
	else
	{
		if (M->dir_stack == NULL)
		{
			eprintf (M, NlsStr(PU_NODIR), argv[0]);
			mfree (&M->alloc, actdir);
			return 1;
		}
		
		/* has to be, newdir will be thrown away afterwards
		 */
		newdir = (char *)M->dir_stack->dir;
	}
		
	/* Vertausche newdir und actdir
	 */
	{
		char *new_argv[2];

		M->dir_stack->dir = actdir;
		new_argv[0] = argv[0];
		new_argv[1] = (char *)newdir;
		
		retcode = m_cd (M, 2, new_argv);

		if (retcode)
		{
			mfree (&M->alloc, actdir);
			M->dir_stack->dir = newdir;
			if (argc == 2)
			{
				DIRSTACK *tmp;
				
				tmp = M->dir_stack;
				M->dir_stack = tmp->next;
				mfree (&M->alloc, tmp);
			}
		}
	}
	
	return retcode;
}

int m_popd (MGLOBAL *M, int argc, char **argv)
{
	if (!argc)
		return PrintHelp (M, argv[0], "", NlsStr(HLP_popd));

	if (argc > 1)
		return PrintUsage (M, argv[0], "");
	
	if (!M->dir_stack)
	{
		eprintf (M, NlsStr(PU_NODIR), argv[0]);
		return 1;
	}
	
	{
		char *new_argv[2];
		DIRSTACK *ds;
		int retcode;
		
		new_argv[0] = argv[0];
		new_argv[1] = (char *)M->dir_stack->dir;
		
		retcode = m_cd (M, 2, new_argv);
		
		ds = M->dir_stack;
		M->dir_stack = ds->next;
		mfree (&M->alloc, ds->dir);
		mfree (&M->alloc, ds);

		return retcode;
	}
}

int ReInitDirStack (MGLOBAL *old, MGLOBAL *new)
{
	DIRSTACK *list, **pds;
	
	list = old->dir_stack;
	pds = &new->dir_stack;

	new->dir_stack = NULL;
	
	while (list)
	{
		DIRSTACK *d;
		
		d = mcalloc (&new->alloc, 1, sizeof (DIRSTACK));
		if (!d)
			return FALSE;
		
		d->dir = StrDup (&new->alloc, list->dir);
		if (!d->dir)
		{
			mfree (&new->alloc, d);
			return FALSE;
		}
		
		d->next = NULL;
		*pds = d;
		pds = &d->next;
	
		list = list->next;
	}
	
	return TRUE;
}

void ExitDirStack (MGLOBAL *M)
{
	DIRSTACK *next, *ds = M->dir_stack;
	
	while (ds)
	{
		next = ds->next;
		
		mfree (&M->alloc, ds->dir);
		mfree (&M->alloc, ds);
		
		ds = next;
	}
	
	M->dir_stack = NULL;
}


/* jr: get name of current working directory. Cares about
   lowercasing case insensitive parts */

static long
get_wd (char *path, size_t size)
{
	char temp_name[MAX_PATH_LEN + 8];
	int gemdos_return;
	
	temp_name[0] = 'a' + Dgetdrv ();
	temp_name[1] = ':';

	gemdos_return = Dgetpath (temp_name + 2, 0);

	/* special case: root path */
	
	if (strlen (temp_name) == 2)
		strcat (temp_name, "\\");
	
	if (gemdos_return != E_OK) return gemdos_return;

	/* lowercase every case insensitive path component */
	{
		int last = 0;
		char *c = strchr (temp_name, '\\');
		
		while (!last)
		{
			char hold, *start;
			long dpret;
			
			if (!c) {
				last = 1;
				c = temp_name + strlen (temp_name);
			}
			
			hold = *c;
			*c = '\0';
			start = strrchr (temp_name, '\\');
			if (start)
				start += 1;
			else
				start = temp_name;

			dpret = Dpathconf (temp_name, 6);
			if (dpret == EINVFN || dpret == 1)
				strlwr (start);
							
			*c = hold;

			c = strchr (c + 1, '\\');
		}	
	}		
	
	if ((strlen (temp_name) >= size)) return ERANGE;

	strcpy (path, temp_name);
	return E_OK;
}



int m_pwd (MGLOBAL *M, int argc, char **argv)
{
#if 0
	const char *pwd;
	
	if (!argc)
		return PrintHelp (M, argv[0], "", NlsStr(HLP_pwd));

	pwd = GetEnv (M, "PWD");
	if (!pwd) return 1;

	mprintf (M, "%s\n", pwd);
#endif

	char path[MAX_PATH_LEN + 8];

	/* jr: POSIX requires that the fully expanded and resolved
	path is returned! */

	if (!argc)
		return PrintHelp (M, argv[0], "", NlsStr(HLP_pwd));

	if (E_OK == get_wd (path, sizeof (path)))
	{
		mprintf (M, "%s\n", path);
		return 0;
	}

	return 1;
}