/*
 *	@(#)Mupfel/rename.c
 *	@(#)Stefan Eissing & Gereon Steffens, 09. August 1993
 *
 * jr 11.6.1995
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

#include "chario.h"
#include "cpmv.h"
#include "getopt.h"
#include "rename.h"

/* internal texts
 */
#define NlsLocalSection "M.rename"
enum NlsLocalText{
OPT_rename,	/*[-icv] ext dateien*/
HLP_rename,	/*dateien mit Endung ext versehen*/
RE_EXTERR,	/*%s: die Endung `%s' enthÑlt nicht erlaubte Zeichen\n*/
RE_ILLNAME,	/*%s: `%s' ist kein gÅltiger Name.\n*/
RE_RENAMDIR,/*%s: kann Verzeichnis `%s' nicht umbenennen\n*/
RE_NOFILE,	/*%s: Datei `%s' existiert nicht\n*/
RE_EXISTS,	/*%s: `%s' existiert bereits\n*/
};


int m_rename (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	COPYINFO C;
	int retcode = 0, i;
	int c, opt_index;
	char *new_ext;
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
		return PrintHelp (M, argv[0], NlsStr(OPT_rename), 
			NlsStr(HLP_rename));
	
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
				return PrintUsage (M, argv[0], NlsStr(OPT_rename));
		}
	}

	if (argc - G.optind < 2)
		return PrintUsage (M, argv[0], NlsStr(OPT_rename));
	
	new_ext = argv[G.optind];
	if (*new_ext == '.') ++new_ext;
	
	/* Wenn nicht leer, auf GÅltigkeit testen */
	if (strlen (new_ext) && !ValidFileName (new_ext)) {
		eprintf (M, NlsStr(RE_EXTERR), argv[0], new_ext);
		return 1;
	}

	for (i = G.optind + 1; (i < argc) && !C.broken && !intr (M); ++i)
	{
		char *realname, *newname, *newreal;
		
		realname = NormalizePath (&M->alloc, argv[i], 
				M->current_drive, M->current_dir);

		if (realname == NULL)
		{
			eprintf (M, NlsStr(RE_ILLNAME), argv[0], argv[i]);
			retcode = -1;
			break;
		}
		
		if (IsDirectory (realname, &C.broken))
		{
			eprintf (M, NlsStr(RE_RENAMDIR), argv[0], argv[i]);
			mfree (&M->alloc, realname);
			continue;
		}
		
		if (!access (realname, A_READ, &C.broken))
		{
			eprintf (M, NlsStr(RE_NOFILE), argv[0], argv[i]);
			retcode = 1;
			mfree (&M->alloc, realname);
			continue;
		}
		
		newname = NewExtension (&M->alloc, argv[i], new_ext);
		newreal = NewExtension (&M->alloc, realname, new_ext);
		
		if (newname && newreal)
		{
			int try_move = TRUE;
			int was_moved;
			
			if (!C.flags.confirm && !C.flags.interactive)
			{
				if (access (newname, A_READ, &C.broken))
				{
					eprintf (M, NlsStr(RE_EXISTS), argv[i], newname);
					retcode = 1;
					try_move = FALSE;
				}
			}
			
			if (try_move)
				retcode = mv (&C, argv[i], newname, realname, 
					newreal, &was_moved);
		}
		
		if (newname) mfree (&M->alloc, newname);
		if (newreal) mfree (&M->alloc, newreal);

		mfree (&M->alloc, realname);
	}

	FreeCopyInfoContents (&C);

	return retcode;
}
