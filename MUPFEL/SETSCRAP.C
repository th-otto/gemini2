/*
 * @(#) Mupfel\setscrap.c
 * @(#) Stefan Eissing, 31. Januar 1993
 *
 * Scrap setzen/lesen
 */


#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <aes.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"

#include "mglob.h"
#include "mdef.h"

#include "chario.h"
#include "getopt.h"
#include "setscrap.h"


/* internal texts
 */
#define NlsLocalSection "M.setscrap"
enum NlsLocalText{
OPT_setscrap,	/*[-fq] [pfad]*/
HLP_setscrap,	/*Klemmbrett setzen/abfragen*/
SC_CANTGET,	/*%s: kann Scrap-Pfad nicht ermitteln\n*/
SC_CURRSCRP,	/*aktueller Scrap-Pfad ist %s\n*/
SC_NOSCRP,	/*Scrap-Pfad nicht gesetzt\n*/
SC_OLDSCRP,	/*%s: Scrap-Pfad ist auf %s gesetzt\n*/
SC_ABSPATH,	/*absolute Pfadangabe erforderlich\n*/
SC_NODIR,	/*%s: Verzeichnis %s existiert nicht\n*/
SC_CANTSET,	/*%s: kann Scrap-Pfad nicht setzen\n*/
SC_NOMEM,	/*%s: nicht genug freier Speicher\n*/
};

typedef struct
{
	MGLOBAL *M;
	const char *cmd;

	struct
	{
		unsigned force_scrap : 1;
		unsigned quiet : 1;
	} flags;
	
} SCRAPINFO;

static int showscrap (SCRAPINFO *S)
{
	char scrap[MAX_PATH_LEN];
	
	if (!scrp_read (scrap))
	{
		eprintf (S->M, NlsStr(SC_CANTGET), S->cmd);
		return 1;
	}
	
	if (*scrap)
		mprintf (S->M, NlsStr(SC_CURRSCRP), scrap);
	else
		mprintf (S->M, NlsStr(SC_NOSCRP));
		
	return 0;
}

static int setscrap (SCRAPINFO *S, const char *scrap)
{
	int retcode, broken;
	char *newscrap;
	
	broken = FALSE;
	
	if (!S->flags.quiet)
	{
		if (showscrap (S))
			return 1;
	}
	
	newscrap = NormalizePath (&S->M->alloc, scrap, 
		S->M->current_drive, S->M->current_dir);

	if (newscrap == NULL)
	{
		if (!S->flags.quiet)
			eprintf (S->M, NlsStr(SC_NOMEM), S->cmd);
		return 1;
	}
	
	if (newscrap[1] == ':')
		newscrap[0] = toupper (newscrap[0]);
		
	if (!IsDirectory (newscrap, &broken))
	{
		if (!S->flags.quiet)
			eprintf (S->M, NlsStr(SC_NODIR), S->cmd, scrap);
		
		mfree (&S->M->alloc, newscrap);
		return 1;
	}
	
	retcode = scrp_write (newscrap);
	
	if (!S->flags.quiet && !retcode)
		eprintf (S->M, NlsStr(SC_CANTSET), S->cmd);	
	
	mfree (&S->M->alloc, newscrap);
	return retcode;
}

int m_setscrap (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	SCRAPINFO S;
	int c;
	int opt_index = 0;

	static struct option long_options[] =
	{
		{ "force", 0, NULL, 'f'},
		{ "quiet", 0, NULL, 'q'},
		{ NULL,0,0,0 },
	};
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_setscrap), 
			NlsStr(HLP_setscrap));
	
	memset (&S, 0, sizeof (SCRAPINFO));
	S.M = M;
	S.cmd = argv[0];
	optinit (&G);
	
	while ((c = getopt_long (M, &G, argc, argv, "fq", long_options,
		&opt_index)) != EOF)
	{
		switch (c)
		{
			case 0:
				break;

			case 'f':
				S.flags.force_scrap = TRUE;
				break;

			case 'q':
				S.flags.quiet = TRUE;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_setscrap));
		}
	}

	switch (argc - G.optind)
	{
		case 0:
			return !showscrap (&S);

		case 1:
			return !setscrap(&S, argv[G.optind]);

		default:
			return PrintUsage (M, argv[0], NlsStr(OPT_setscrap));
	}
}
