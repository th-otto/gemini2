/*
 * @(#) Mupfel\rm.c
 * @(#) Stefan Eissing & Gereon Steffens, 10. Februar 1993
 *
 *  -  internal "rm" function
 *
 * jr 27.5.96
 */

#include <ctype.h> 
#include <ext.h> 
#include <stdio.h> 
#include <string.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\walkdir.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "chmod.h"
#include "getopt.h"
#include "mdef.h"
#include "rm.h"
#include "rmdir.h"


/* internal texts */

#define NlsLocalSection "M.rm"
enum NlsLocalText{
OPT_rm,		/*[-bfirRv] datei...*/
HLP_rm,		/*Dateien l”schen*/
RM_ILLNAME,	/*%s: `%s' ist kein gltiger Name\n*/
RM_DONE,	/* fertig.\n*/
RM_CHOICE,	/*JNAQ*/
RM_SHOWRM,	/*L”sche `%s'...*/
RM_ISDIR,	/*rm: `%s' ist ein Verzeichnis\n*/
RM_REMOVE,	/*`%s' wirklich l”schen (j/n/a/q)? */
RM_DESCEND,	/*`%s' rekursiv durchgehen (j/n/a/q)? */
};

typedef struct
{
	const char *cmd;
	short recursive;
	short interactive;
	short brute_force;
	short dont_fail;
	short verbose;
	short quit;
	
	size_t name_size;
	char *realname;
	char *origname;

	short attrib;
		
	int broken;
	
} RMINFO;


static int
checkNameLen (MGLOBAL *M, RMINFO *R, size_t min_len)
{
	char *real, *orig;
	
	min_len += MAX_FILENAME_LEN + 4;
	
	if (R->name_size > min_len)
		return TRUE;
	
	if (min_len < AVERAGE_PATH_LEN)
		min_len = AVERAGE_PATH_LEN;
		
	real = mmalloc (&M->alloc, min_len);
	if (real)
	{
		orig = mmalloc (&M->alloc, min_len);
		if (!orig)
		{
			mfree (&M->alloc, real);
			R->broken = TRUE;
			return FALSE;
		}
	}
	else
	{	
		R->broken = TRUE;
		return FALSE;
	}
	
	if (R->realname)
	{
		strcpy (real, R->realname);
		strcpy (orig, R->origname);
		mfree (&M->alloc, R->realname);
		mfree (&M->alloc, R->origname);
	}
	
	R->realname = real;
	R->origname = orig;
	R->name_size = min_len;
	
	return TRUE;
}

static void
vcrlf (MGLOBAL *M, RMINFO *R)
{
	if (R->verbose) crlf (M);
}

static int
ask_rm (MGLOBAL *M, RMINFO *R, const char *mess)
{
	char answer;
	
	if (M->shell_flags.system_call) return 'Y';	
		
	eprintf (M, mess, R->origname);
	
	answer = CharSelect (M, NlsStr(RM_CHOICE));
		
	switch (answer)
	{
		case 'J':
		case 'Y':
			return TRUE;
			
		case 'A':
			R->interactive = FALSE;
			return TRUE;

		case 'Q':
			R->quit = TRUE;

		case 'N':
			return FALSE;
	}
	
	return FALSE;
}

static long
removeFile (MGLOBAL *M, RMINFO *R)
{
	int rmstat, retcode;
	int do_prompt = R->interactive;
	
	/* If the file is not of type directory, the -f option is
	not specified, and either the permissions of file do not
	permit writing and the standard input is a terminal or the
	-i option is specified, write a prompt to the standard error
	and read a line from standard input. If the answer is not
	affirmative, do nothing more with the file and go on to
	any remaining files (P1003.2, 4.53.2) */
	
	/* xxx: this needs 2b fixed for real file permissions */
	if (R->attrib & FA_READONLY)
	{
		if (! R->brute_force ||
			SetWriteMode (R->realname, TRUE) < E_OK)
		{
			if (isatty (1))
				do_prompt = TRUE;
		}
	}
	
	if (do_prompt && !ask_rm (M, R, NlsStr(RM_REMOVE)))
		return 1;
	
	if (R->verbose)
		mprintf (M, NlsStr(RM_SHOWRM), R->origname);

	/* Try to set access mode so the file can be deleted */
	if (R->attrib & FA_READONLY) SetWriteMode (R->realname, TRUE);

	rmstat = Fdelete (R->realname);
	
	switch (M->oserr = rmstat)
	{
		case EPTHNF:
		case EFILNF:
			
			if (IsDirectory (R->realname, &R->broken))
			{
				vcrlf (M, R);
				eprintf (M, NlsStr(RM_ISDIR), R->origname);
			}
			else
			{
				if (R->dont_fail)
				{
					retcode = 0;
					break;
				}
					
				vcrlf (M, R);
				eprintf (M, "%s: `%s' -- %s\n", R->cmd, R->origname,
					StrError (rmstat));
			}
				
			retcode = 1;
			break;

		case E_OK:
			if (!M->shell_flags.system_call)
				DirtyDrives |= DriveBit (R->realname[0]);

			retcode = 0;
			break;

		default:
			if (IsBiosError (rmstat))
			{
				vcrlf (M, R);
				retcode = (int)ioerror(M, "rm", R->origname, rmstat);
			}
			else
			{
				vcrlf (M, R);
				eprintf (M, "%s: `%s' -- %s\n", R->cmd, R->origname,
					StrError (rmstat));
				retcode = 1;
			}
			break;
	}
		
	if (R->verbose && retcode == 0)
		mprintf (M, NlsStr(RM_DONE));
		
	return retcode;
}

static long
removeDir (MGLOBAL *M, RMINFO *R)
{
	WalkInfo W;
	char filename[MAX_FILENAME_LEN];
	long stat = 0L;
	int has_failed = FALSE;
	
	if (R->interactive && !ask_rm (M, R, NlsStr(RM_DESCEND)))
		return 1;
	
	stat = InitDirWalk (&W, WDF_ATTRIBUTES|WDF_LOWER,
		R->realname, MAX_FILENAME_LEN, filename);

	if (!stat)
	{
		while (!stat && !R->broken && (stat = DoDirWalk (&W)) == 0L)
		{
			if (!strcmp (filename, ".")
				|| !strcmp (filename, "..")
				|| (W.xattr.attr & FA_VOLUME))
			{
				continue;
			}
			
			R->attrib = W.xattr.attr;
			
			if (checkNameLen (M, R, strlen (R->realname)))
			{
				AddFileName (R->realname, filename);
				AddFileName (R->origname, filename);

				if (R->attrib & FA_SUBDIR)
				{
					stat = removeDir (M, R);
				}
				else 
				{
					stat = removeFile (M, R);
				}	

				StripFileName (R->realname);
				StripFileName (R->origname);
				has_failed = (stat != 0L);
			}
			else
			{
				eprintf (M, "rm: %s\n", strerror (EPTHOV));
				R->broken = 1;
			}
			
		}
		
		ExitDirWalk (&W);
	}

	if (!has_failed && !R->broken && !R->quit)
	{
		stat = rmdir (M, R->realname, R->origname, R->verbose,
			&R->broken);
	}
	
	return stat;
}

static int
removeAll (MGLOBAL *M, RMINFO *R)
{
	XATTR X;
	long stat;
	
	if (E_OK != (stat = GetXattr (R->realname, FALSE, FALSE, &X)))
	{
		if (R->dont_fail)
		{
			return 0;
		}
		else if (!ValidFileName (R->realname))
		{
			eprintf (M, NlsStr(RM_ILLNAME), R->cmd, R->origname);
			return 1;
		}
		else if (IsBiosError (stat))
		{
			R->broken = (int)stat;
			return (int)ioerror (M, "rm", R->origname, R->broken);
		}
		else
		{
			eprintf (M, "%s: `%s' -- %s\n", R->cmd, R->origname, StrError (stat));
			return 1;
		}
	}
	else
	{
		R->attrib = X.attr;
		
		if (R->recursive && (X.attr & FA_SUBDIR))
			return (int) removeDir (M, R);
		else 
			return (int) removeFile (M, R);
	}
}

int
m_rm (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	RMINFO R;
	int c, retcode;
	int opt_index = 0;
	static struct option long_option[] =
	{
		{ "bruteforce", 0, NULL, 'b' },
		{ "force", 0, NULL, 'f' },
		{ "interactive", 0, NULL, 'i' },
		{ "recursive", 0, NULL, 'r' },
		{ "verbose", 0, NULL, 'v' },
		{ NULL, 0, 0, 0 },
	};


	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_rm), 
			NlsStr(HLP_rm));
	
	if (argc == 1)
		return PrintUsage (M, argv[0], NlsStr(OPT_rm));

	memset (&R, 0, sizeof (RMINFO));
	R.cmd = argv[0];

	optinit (&G);
	
	while ((c = getopt_long (M, &G, argc, argv, "bfirRv", long_option,
		&opt_index)) != EOF)
	{
		if (!c)
			c = long_option[opt_index].val;
			
		switch (c)
		{
			case 0:
				break;
			case 'i':
				R.dont_fail = FALSE;	/* P1003.2 4.53.3 */
				R.interactive = TRUE;
				break;
			case 'r':
			case 'R':
				R.recursive = TRUE;
				break;
			case 'f':
				R.interactive = FALSE;	/* P1003.2 4.53.3 */
				R.dont_fail = TRUE;
				break;
			case 'v':
				R.verbose = TRUE;
				break;
			case 'b':
				R.brute_force = TRUE;
				break;
			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_rm));
		}
	}

	if (G.optind == argc) return PrintUsage (M, argv[0], NlsStr(OPT_rm));

	retcode = 0;
	
	R.name_size = 0;
	R.realname = R.origname = NULL;

	for (; G.optind < argc && !R.broken && !R.quit && !intr (M); 
		++G.optind)
	{
		char *realname = NULL;
		
		if (!ContainsGEMDOSWildcard (argv[G.optind]))
		{
			realname = NormalizePath (&M->alloc, argv[G.optind],
				M->current_drive, M->current_dir);
		}
		
		if (!realname)
		{
			if (!R.dont_fail)
				eprintf (M, NlsStr(RM_ILLNAME), argv[0], argv[G.optind]);

			retcode = 1;
			continue;
		}

		RemoveAppendingBackslash (realname);

		if (!checkNameLen (M, &R, strlen (realname)))
		{
			eprintf (M, "rm: %s\n", strerror (EPTHOV));
			retcode = 1;
			break;
		}
		else
		{
			strcpy (R.realname, realname);
			strcpy (R.origname, argv[G.optind]);
		
			retcode = removeAll (M, &R);
		}

		mfree (&M->alloc, realname);
	}
	
	mfree (&M->alloc, R.realname);
	mfree (&M->alloc, R.origname);
	
	return R.broken ? R.broken : retcode;
}
