/*
 * @(#)Mupfel/complete.c
 * @(#)Stefan Eissing, 18. Dezember 1994
 *
 * Filename completion
 *
 * jr 27.7.94
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "..\common\walkdir.h"

#include "mglob.h"

#include "chario.h"
#include "cmdcompl.h"
#include "complete.h"
#include "mdef.h"
#include "mlex.h"
#include "nameutil.h"
#include "optcompl.h"
#include "subst.h"


static int
setupFileCompletion (MGLOBAL *M, char *what, char **subst,
	int *slash_found, char **newname, char *merkname,
	size_t merkname_len)
{
	WORDINFO w;
	char *subst_quote;
	char *t, *name;
	
	memset (&w, 0, sizeof w);
	w.name = what;
	if (!strlen (w.name))
		w.name = ".\\";
		
	w.quoted = mcalloc (&M->alloc, 1, strlen (what)+1);
	if (!w.quoted) return FALSE;
	
	if (!SubstNoCommand (M, &w, subst, &subst_quote, slash_found))
	{
		mfree (&M->alloc, w.quoted);
		return FALSE;
	}
		
	mfree (&M->alloc, w.quoted);

	if (M->shell_flags.slash_conv)
	{
		char *slash;
		
		while ((slash = strpbrk (*subst,"/"))!=NULL)
			*slash = '\\';
	}
	
	name = NormalizePath (&M->alloc, *subst, M->current_drive,
		M->current_dir);
	
	if (!name)
	{
		mfree (&M->alloc, *subst);
		mfree (&M->alloc, subst_quote);
		return FALSE;
	}
	
	*newname = mmalloc (&M->alloc, strlen (name) + merkname_len + 1);

	if (!*newname)
	{
		mfree (&M->alloc, *subst);
		mfree (&M->alloc, subst_quote);
		mfree (&M->alloc, name);		
		return FALSE;
	}
	
	strcpy (*newname, name);
	mfree (&M->alloc, name);

	merkname[0] = 0;	/* record file name to search for */
	t = strrchr (*newname, '\\');
	
	/* Wenn im normalisierten Namen kein '\\' vorkommt oder der
	Dateiname am Ende l„nger als der maximaler Dateiname ist,
	dann beenden wird hier die Suche. */

	if (!t || strlen (&t[1]) > merkname_len - 1)
	{
		mfree (&M->alloc, *subst);
		mfree (&M->alloc, subst_quote);
		mfree (&M->alloc, *newname);		
		return FALSE;
	}	
	
	strcpy (merkname, &t[1]);
	t[1] = '\0';
	
	return TRUE;
}


/* returns pointer to allocated string containing characters that
   might be inserted. Beeping on error is done locally, may return
   NULL on error */

static char *
fileComplete (MGLOBAL *M, char *what)
{
	WalkInfo W;
	char *newname, *tmp;
	char merkname[MAX_FILENAME_LEN];
	char cmp_name[MAX_FILENAME_LEN];
	char filename[MAX_FILENAME_LEN];
	long stat;
	int something_found = FALSE;
	int full_name = FALSE;
	int was_dir = FALSE;
	int slash_found = FALSE;

	if (!setupFileCompletion (M, what, &tmp, &slash_found, 
		&newname, merkname, MAX_FILENAME_LEN))
	{
		beep (M);
		return NULL;
	}
	
	*cmp_name = 0;
	
	stat = InitDirWalk (&W, WDF_ATTRIBUTES|WDF_LOWER|
		WDF_FOLLOW|WDF_IGN_XATTR_ERRORS, newname,
		MAX_FILENAME_LEN, filename);
	
	if (!stat)
	{	
		while ((stat = DoDirWalk (&W)) == 0L)
		{
			int nameMatches;
			
			/* match? */
			if (W.flags.case_sensitiv)
				nameMatches = !strncmp (merkname, filename, strlen (merkname));
			else
				nameMatches = !strnicmp (merkname, filename, strlen (merkname));
			
			if (nameMatches	&& strcmp (filename, ".") 
				&& strcmp (filename, ".."))
			{
				if (!something_found)			/* first match? */
				{
					full_name = TRUE;
					something_found = TRUE;
					strcpy (cmp_name, filename);
					
					if (W.xattr.attr & FA_SUBDIR)
						was_dir = TRUE;
				}
				else
				{
					int i;
					size_t len = strlen (cmp_name);
					
					full_name = FALSE;
					for (i=0; i < len; i++)
					{
						if (cmp_name[i] != filename[i])
						{
							cmp_name[i] = 0;
							break;
						}
					}
				}
			}
		}	
	
		ExitDirWalk (&W);
	}

	mfree (&M->alloc, tmp);

	if (!strlen (cmp_name))
	{
		if (!something_found) beep (M);
		mfree (&M->alloc, newname);
		return NULL;
	}

	strcpy (newname, &cmp_name[strlen(merkname)]);

	/* Haben wir damit schon den vollst„ndigen Namen eingefgt? */
	if (full_name)
		strcat (newname, was_dir ? (slash_found ? "/" : "\\") : " ");

	return newname;
}


typedef struct
{
	MGLOBAL *M;
	char *what;
} CompEntry;

static int
getFileCompletions (ARGVINFO *A, void *entry)
{
	MGLOBAL *M;
	WalkInfo W;
	char *newname, *tmp, *what;
	char merkname[MAX_FILENAME_LEN];
	char filename[MAX_FILENAME_LEN];
	long stat;
	int broken;
	int slash_found = FALSE;

	M = ((CompEntry *)entry)->M;
	what = ((CompEntry *)entry)->what;
	
	if (!setupFileCompletion (M, what, &tmp, &slash_found,
		&newname, merkname, MAX_FILENAME_LEN))
		return FALSE;
	
	broken = FALSE;
	stat = InitDirWalk (&W, WDF_ATTRIBUTES|WDF_LOWER|WDF_FOLLOW|
		WDF_IGN_XATTR_ERRORS, newname, MAX_FILENAME_LEN, filename);

	if (!stat)
	{	
		while ((stat = DoDirWalk (&W)) == 0L)
		{
			int nameMatches;
			
			/* match? */
			if (W.flags.case_sensitiv)
				nameMatches = !strncmp (merkname, filename, strlen (merkname));
			else
				nameMatches = !strnicmp (merkname, filename, strlen (merkname));
			
			if (nameMatches	&& strcmp (filename, ".") 
				&& strcmp (filename, ".."))
			{
				if (W.xattr.attr & FA_SUBDIR)
					strcat (filename, "\\");
				if (!StoreInArgv (A, filename))
				{
					broken = TRUE;
					break;
				} 
			}
		}
		
		ExitDirWalk (&W);
	}

	mfree (&M->alloc, tmp);
	mfree (&M->alloc, newname);

	return !broken;
}

int
GetFileCompletions (MGLOBAL *M, ARGVINFO *A, char *what)
{
	CompEntry ce;
	
	ce.M = M;
	ce.what = what;
	return MakeGenericArgv (&M->alloc, A, FALSE, getFileCompletions, &ce);
}


static char *
getStringComplete (MGLOBAL *M, ARGVINFO *A, char *what)
{
	char *newname;
	int i;
	size_t max_len;
	
	if (A->argc < 1)
	{
		beep (M);
		return NULL;
	}
	
	max_len = 0L;
	for (i = 0; i < A->argc; ++i)
	{
		size_t len = strlen (A->argv[i]);
		
		if (len > max_len)
			max_len = len;
	}
	
	max_len += 4;
	newname = mcalloc (&M->alloc, max_len, 1);
	if (!newname)
		return NULL;
	
	strcpy (newname, A->argv[0]);

	if (A->argc == 1)
	{
		strcat (newname, " ");
	}
	else
	{
		for (i = 1; i < A->argc; ++i)
		{
			int j;
			size_t len = strlen (newname);
			
			for (j = 0; j < len; j++)
			{
				if (newname[j] != A->argv[i][j])
				{
					newname[j] = 0;
					break;
				}
			}
		}
	}
	
	memmove (newname, &newname[strlen (what)], 
		strlen (newname) - strlen (what) + 1);
	
	return newname;
}


char *
completion (MGLOBAL *M, char *line, int linepos, ARGVINFO *A)
{
	ARGVINFO localA;
	char *last_blank;
	char next_c;
	char *found;
	int maybe_command = FALSE;
	int show_completion, try = 0;
	
	show_completion = (A != NULL);
	if (!show_completion) A = &localA;
	
	next_c = line[linepos];
	line[linepos] = '\0';

	if (linepos)
	{
		last_blank = &line[linepos - 1];

		while (!EofOrSeparator (*last_blank) && last_blank != line)
			--last_blank;
		
		if (last_blank == line)
		{
			maybe_command = TRUE;
		}
		else
		{
			char *sep = last_blank;
			
			++last_blank;
			while (EofOrSeparator (*sep))
			{
				if (CommandSeparator (*sep) || sep == line)
				{
					maybe_command = TRUE;
					break;
				}
				
				--sep;
			}
		}
	}
	else
	{
		maybe_command = TRUE;
		last_blank = line;
	}
		
	switch (last_blank[0])
	{
		case '-':
		case '+':
		{
			char *command;
			
			command = GetCommandFromLine (M, line, last_blank);
			
			if (command == NULL)
			{
				found = NULL;
				beep (M);
				break;
			}
			else if (GetOptionCompletions (M, A, last_blank, command))
			{
				if (show_completion)
					found = (char *)1;
				else
				{
					found = getStringComplete (M, A, last_blank);
					FreeArgv (A);
				}
			}
			else
				found = NULL;
				
			mfree (&M->alloc, command);
			break;
		}
		
		case '$':
			if (strpbrk (last_blank, "\\:}"))
			{
				try = 1;
				break;
			}
			
			if (GetNameCompletions (M, A, &last_blank[1]))
			{
				if (show_completion)
					found = (char *)1;
				else
				{
					found = getStringComplete (M, A, &last_blank[1]);
					FreeArgv (A);
				}
			}
			else
				found = NULL;
			break;

		case '`':
			maybe_command = TRUE;
		case '!':
			last_blank = &last_blank[1];
			/* fall through */
		
		case '~':
			maybe_command = FALSE;
		default:
			try = 1;
			break;
	}
	
	if (try)
	{
		if (strchr (last_blank, '\\')
			|| (M->shell_flags.slash_conv && strchr (last_blank, '/'))
			|| strchr (last_blank, ':'))
		{
			maybe_command = FALSE;
		}
			
		if (maybe_command)
		{
			if (GetCommandCompletions (M, A, last_blank))
			{
				if (show_completion)
					found = (char *)1;
				else
				{
					found = getStringComplete (M, A, last_blank);
					FreeArgv (A);
				}
			}
			else
				found = NULL;
		}
		else
		{
			if (show_completion)
				found = (char *)GetFileCompletions (M, A, last_blank);
			else
				found = fileComplete (M, last_blank);
		}
	}
	
	line[linepos] = next_c;
	clearintr (M);
	
	return found;
}


/* returns pointer to allocated string containing characters that
   might be inserted. Beeping on error is done locally, may return
   NULL on error */

char *
CompleteWord (MGLOBAL *M, char *line, int linepos)
{
	return completion (M, line, linepos, NULL);
}

int
GetCompletions (MGLOBAL *M, char *line, int linepos, ARGVINFO *A)
{
	return completion (M, line, linepos, A) != NULL;
}
