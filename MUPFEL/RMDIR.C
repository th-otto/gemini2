/*
 * @(#) Mupfel\rm.c
 * @(#) Stefan Eissing & Gereon Steffens, 28. Mai 1992
 *
 *  -  internal "rm" and "rmdir" functions
 *
 * jr 23.4.95
 */

#include <stddef.h> 
#include <ctype.h> 
#include <string.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "getopt.h"
#include "mdef.h"
#include "rmdir.h"

/* internal texts
 */
#define NlsLocalSection "M.rmdir"
enum NlsLocalText{
OPT_rmdir,	/*dir ...*/
HLP_rmdir,	/*Verzeichnis dir l”schen*/
RM_SHOWRMD,	/*L”sche Verzeichnis `%s'...*/
RM_ISNODIR,	/*%s: `%s' ist kein Verzeichnis\n*/
RM_NOTEMPTY,/*%s: `%s' enth„lt noch Dateien\n*/
RM_DONE,	/* fertig.\n*/
};


int rmdir (MGLOBAL *M, char *dir, const char *orig_name, 
	int verbose, int *broken)
{
	long retcode = E_OK;
	
	/* Schneide evtl. anh„ngenden Backslash ab */
	if (lastchr (dir) == '\\' && strlen (dir) > 1)
		dir[strlen (dir) - 1] = '\0';
	
	if (verbose) mprintf (M, NlsStr(RM_SHOWRMD), orig_name);
	
	/* Drive falsch? */
	if (!LegalDrive (dir[0])) retcode = EDRIVE;
	
	/* Rootverzeichnis? */
	if (strlen (dir) < 4 && dir[1] == ':') retcode = EACCDN;
	
	if (retcode == E_OK) retcode = Ddelete (dir);

	switch (M->oserr = (int) retcode)
	{
		case E_OK:
			if (!M->shell_flags.system_call)
				DirtyDrives |= DriveBit (dir[0]);

			retcode = M->oserr = 0;
			break;

		default:
			if (verbose) crlf (M);

			if (IsBiosError (M->oserr)) *broken = M->oserr;
			
			/* some special cases */
	
			/* If 'Path not found' and file can be opened */
			if (retcode == EPTHNF && access (dir, A_EXIST, broken))
				eprintf (M, NlsStr(RM_ISNODIR), "rmdir", orig_name);
			else if (retcode == EACCDN && !IsDirEmpty (dir))
				eprintf (M, NlsStr(RM_NOTEMPTY), "rmdir", orig_name);
			else
				ioerror (M, "rmdir", orig_name, M->oserr);
			break;
	}
	
	if (verbose && !retcode) mprintf(M, NlsStr(RM_DONE));
		
	return (int) retcode;
}

int m_rmdir(MGLOBAL *M, int argc, char **argv)
{
	int i, retcode, broken;
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_rmdir), 
			NlsStr(HLP_rmdir));
	
	if (argc == 1)
		return PrintUsage (M, argv[0], NlsStr(OPT_rmdir));

	broken = retcode = 0;
	
	for (i = 1; i < argc && !broken; ++i)
	{
		char *realname = NULL;
		
		if (!ContainsGEMDOSWildcard (argv[i]))
		{
			realname = NormalizePath (&M->alloc, argv[i], 
				M->current_drive, M->current_dir);
		}

		if (realname == NULL)
		{
			eprintf (M, "%s: `%s' -- %s\n", argv[0], argv[i],
				StrError (EPTHNF));
			retcode = 1;
		}
		else
		{
			retcode = rmdir (M, realname, argv[i], FALSE, &broken);

			if (retcode && broken)
				retcode = broken;
			
			mfree (&M->alloc, realname);
		}
	}

	return retcode;
}

