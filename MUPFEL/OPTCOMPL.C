/*
 * @(#) Mupfel\optcompl.c
 * @(#) Stefan Eissing, 27. Dezember 1993
 *
 * Option Completion
 *
 * jr 21.4.95
 */

#include <stddef.h>
#include <string.h>

#include "..\common\alloc.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\walkdir.h"
#include "..\common\wildmat.h"

#include "mglob.h"

#include "chario.h"
#include "makeargv.h"
#include "mdef.h"
#include "mlex.h"
#include "nameutil.h"
#include "optcompl.h"


#define OPTION_EXT		".opt"
#define OPT_LIB_PATTERN	"*.opl"


char *GetCommandFromLine (MGLOBAL *M, const char *line,
	const char *scan_from)
{
	const char *start, *end;

	if (!scan_from)
		return NULL;

	start = end = scan_from;
	
	/* Suche nach dem Token, daû nach dem ersten Kommando-
	 * separator kommt. Das sollte das Kommando sein.
	 */
	while (start != line && !CommandSeparator (*start))
	{
		--start;
		/* Wenn wir einen normalen Separator (whitespace) hatten,
		 * dann merke Dir diese Position.
		 */
		if (!EofOrSeparator (*start) && EofOrSeparator (start[1]))
			end = start + 1;
			
	}
	
	if ((end == scan_from) || (start+1 == end))
	{
		/* Wir haben kein neues Token gefunden und geben NULL
		 * zurÅck.
		 */
		return NULL;
	}
	
	/* öberspringe nun fÅhrende Whitespace-Zeichen
	 */
	while (EofOrSeparator (*start))
	{
		if (start == end)
		{
			return NULL;
		}
		++start;
	}

	/* Nun sollte zwischen <start> und <end> das Kommando liegen
	 */
	{
		size_t len;
		char *command;
		
		len = end - start;
		
		command = mcalloc (&M->alloc, len + 1, 1);
		if (!command)
			return NULL;
			
		strncpy (command, start, len);
		
		return command;
	}
}


#define OPT_BUFF_LEN	256

static int getOptCompletions (ARGVINFO *A, char *what, 
	MEMFILEINFO *mp, const char *filename)
{
	int retcode = FALSE;
	char buffer[OPT_BUFF_LEN];
	size_t len;
	
	if (filename != NULL)
	{
		len = strlen (filename);
		
		while (mgets (buffer, OPT_BUFF_LEN - 1, mp))
		{
			if (!strncmp (buffer, filename, len))
			{
				if (!mgets (buffer, OPT_BUFF_LEN - 1, mp)
					|| strncmp (buffer, "begin", 5))
				{
					return FALSE;
				}
				
				break;
			}
		}
	}
	
	while (mgets (buffer, OPT_BUFF_LEN - 1, mp))
	{
		if (!strncmp (buffer, "end", 3))
			break;
			
		len = strlen (buffer);

		if (!len || strstr (buffer, what) != buffer)
			continue;
		
		/* Wir haben eine passende Option gefunden.
		 */
		retcode = TRUE;
		
		if (buffer[len - 1] == '\n')
			buffer[len - 1] = '\0';
		
		if (!StoreInArgv (A, buffer))
			return FALSE;
	}
	
	return retcode;
}


static int getOptionsFromFile (MGLOBAL *M, ARGVINFO *A, char *what,
	char *filename, char *find_name_in_file)
{
	char *path, *orig_path;
	char *save_buffer;
	int retcode = FALSE;
	
	path = GetEnv (M, "OPTIONPATH");
	if (!path || !*path)
		return FALSE;
	
	orig_path = path = StrDup (&M->alloc, path);
	
	if (!orig_path)
		return FALSE;
	
	path = StrToken (path, ";,", &save_buffer);
	while (!retcode && path)
	{
		char *realname;
		MEMFILEINFO *mp;
		
		realname = mmalloc (&M->alloc, 
			strlen (path) + MAX_FILENAME_LEN + 2);
		
		if (!realname)
			break;
		
		strcpy (realname, path);
		AddFileName (realname, filename);
		
		mp = mopen (&M->alloc, realname, NULL);
		mfree (&M->alloc, realname);
		if (mp)
		{
			retcode = getOptCompletions (A, what, mp, 
				find_name_in_file);
			
			mclose (&M->alloc, mp);
			break;
		}
	
		path = StrToken (NULL, ";,", &save_buffer);
	}

	mfree (&M->alloc, orig_path);
	return retcode;
}
	

static int getOptionsFromLib (MGLOBAL *M, ARGVINFO *A, char *what,
	char *find_name_in_file)
{
	char *path, *orig_path;
	char *save_buffer;
	int retcode = FALSE;
	
	path = GetEnv (M, "OPTIONPATH");
	if (!path || !*path)
		return FALSE;
	
	orig_path = path = StrDup (&M->alloc, path);
	
	if (!orig_path)
		return FALSE;
	
	path = StrToken (path, ";,", &save_buffer);
	while (path && !retcode)
	{
		WalkInfo W;
		long stat;
		char filename[MAX_FILENAME_LEN];
		
		stat = InitDirWalk (&W, WDF_LOWER|WDF_FOLLOW|
			WDF_IGN_XATTR_ERRORS, path, MAX_FILENAME_LEN, filename);

		if (!stat)
		{	
			while (!retcode && ((stat = DoDirWalk (&W)) == 0L))
			{
				/* match? */
				
				if (wildmatch (filename, OPT_LIB_PATTERN,
					W.flags.case_sensitiv))
				{
					retcode = getOptionsFromFile (M, A, what, 
								filename, find_name_in_file);
				}
			}
			
			ExitDirWalk (&W);
		}
		
		path = StrToken (NULL, ";,", &save_buffer);
	}

	mfree (&M->alloc, orig_path);
	return retcode;
}


int GetOptionCompletions (MGLOBAL *M, ARGVINFO *A, char *what,
	const char *command)
{
	char *cp;
	const char *ccp;
	char filename[MAX_FILENAME_LEN];
	
	/* Ermittle den Namen der Datei aus <command>, in der wir
	 * nach Optionen schauen.
	 */
	ccp = strrchr (command, '\\');
	if (!ccp)
		ccp = command;
	
	if (strlen (ccp) >= MAX_FILENAME_LEN)
		return FALSE;

	strcpy (filename, ccp);
	
	/* Schneide eine vorhandene Endung ab.
	 */
	cp = strchr (filename, '.');
	if (cp)
		*cp = '\0';
	
	/* Dies ist ein kleiner Hack, da wir alles bis auf die ersten
	 * MAX_FILENAME_LEN -7 Zeichen abschneiden. Aber: was solls?
	 */
	filename[MAX_FILENAME_LEN - 7] = '\0';
	strcat (filename, OPTION_EXT);
	
	/* Initialisiere <A>
	 */
	if (!MakeGenericArgv (&M->alloc, A, TRUE, 0, NULL))
		return FALSE;

	if (getOptionsFromFile (M, A, what, filename, NULL)
		|| getOptionsFromFile (M, A, what, "default.opt", filename)
		|| getOptionsFromLib (M, A, what, filename))
		return TRUE;
	else
		beep (M);
	
	FreeArgv (A);
	return FALSE;
}

