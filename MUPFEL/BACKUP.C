/*
 *	@(#)Mupfel/backup.c
 *	@(#)Stefan Eissing & Gereon Steffens, 28. Mai 1992
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"

#include "mglob.h"

#include "backup.h"
#include "chario.h"
#include "cpmv.h"
#include "getopt.h"

/* internal texts
 */
#define NlsLocalSection "M.backup"
enum NlsLocalText{
OPT_backup,	/*[-icv] dateien*/
HLP_backup,	/*Dateien kopieren, Neue Extension ist .bak*/
BA_ILLNAME,	/*%s: %s ist kein gÅltiger Name.\n*/
BA_NOMEM,	/*%s: nicht genÅgend Speicher\n*/
BA_BACKUP,	/*%s: kann Verzeichnis %s nicht sichern\n*/
};

int m_backup (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	COPYINFO C;
	int retcode = 0, i;
	int c, opt_index;
	static struct option long_options[] =
	{
		{ "interactive", FALSE, NULL, 'i'},
		{ "confirm", FALSE, NULL, 'c'},
		{ "verbose", FALSE, NULL, 'v'},
		{ NULL,0,0,0},
	};
		
	memset (&C, 0, sizeof(COPYINFO));
	C.M = M;
	C.flags.is_move = FALSE;
	C.copyfile = cp;
	
	if (!argc)
		return PrintHelp (M, argv[0], 
			NlsStr(OPT_backup), NlsStr(HLP_backup));
	
	C.cmd = argv[0];
	
	optinit (&G);

	while ((c = getopt_long (M, &G, argc, argv, "icv", 
		long_options, &opt_index)) != EOF)
	{
		if (!c)			c = long_options[G.option_index].val;

		switch (c)
		{
			case 0:
				break;

			case 'i':
				C.flags.interactive = TRUE;
				break;

			case 'c':
				C.flags.confirm = TRUE;
				break;

			case 'v':
				C.flags.verbose = TRUE;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_backup));
		}
	}

	if (argc == G.optind)
		return PrintUsage (M, argv[0], NlsStr(OPT_backup));
	
	for (i = G.optind; (i < argc) && !C.broken && !intr (M); ++i)
	{
		char *realname;
		
		realname = NormalizePath (&M->alloc, argv[i], 
				M->current_drive, M->current_dir);

		if (realname == NULL)
		{
			eprintf (M, NlsStr(BA_ILLNAME), argv[0], argv[i]);
			retcode = -1;
			break;
		}
		
		if (!IsDirectory (realname, &C.broken))
		{
			char *newname, *newreal;
			int was_copied;
			
			newname = NewExtension (&M->alloc, argv[i], "bak");
			newreal = NewExtension (&M->alloc, realname, "bak");
			
			if (newname && newreal)
			{
				cp (&C, argv[i], newname, realname, newreal, 
					&was_copied);
			}
			else
			{
				C.broken = TRUE;
				eprintf (M, NlsStr(BA_NOMEM), argv[0]);
			}

			if (newname)
				mfree (&M->alloc, newname);
			if (newreal)
				mfree (&M->alloc, newreal);
		}
		else
			eprintf (M, NlsStr (BA_BACKUP), argv[0], argv[i]);
		
		mfree (&M->alloc, realname);
	}

	FreeCopyInfoContents (&C);

	return retcode;
}
