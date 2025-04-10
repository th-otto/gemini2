/*
 * @(#) MParse\type.c
 * @(#) Stefan Eissing, 27. Februar 1993
 *
 * Internes Kommando "type"
 */

/* jr: ich habe in printLocation ein verbose-Flag fÅr command -v
   und -V eingebaut, und die Hauptroutine von m_hash ausgelagert,
   damit sie von m_command aufgerufen werden kann */

#include <stddef.h>
#include <string.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "mglob.h"

#include "alias.h"
#include "chario.h"
#include "locate.h"
#include "mdef.h"
#include "prnttree.h"
#include "type.h"


/* internal texts
 */
#define NlsLocalSection "M.type"
enum NlsLocalText{
OPT_type,	/*name*/
HLP_type,	/*Art des Kommandos name bestimmen*/
TY_ALIAS,	/*%s ist ein Alias auf %s\n*/
TY_ERROR,	/*%s: Fehler beim Suchen nach %s\n*/
TY_UNKNOWN,	/*%s ist als Kommando nicht bekannt\n*/
TY_FUNCTION,	/*%s ist eine Funktion\n*/
TY_INTERNAL,	/*%s ist ein internes Kommando\n*/
TY_DATA,		/*%s ist eine Datei (%s)\n*/
TY_ALIEN,	/*%s ist ein %sScript (%s) fÅr %s\n*/
TY_SCRIPT,	/*%s ist ein %sScript (%s)\n*/
TY_BINARY,	/*%s ist ein %sProgramm (%s)\n*/
TY_HASHED,	/*gehashtes */
};

static void printLocation (MGLOBAL *M, LOCATION *L,
	const char *command, int verbose)
{
	if (verbose)
	{
		switch (L->location)
		{
			case Function:
				mprintf (M, NlsStr(TY_FUNCTION), command);
				PrintTreeInfo (M, (TREEINFO *)L->loc.func.n->func);
				mprintf (M, "\n");
				break;
	
			case Internal:
				mprintf (M, NlsStr(TY_INTERNAL), command);
				break;
	
			case Data:
				mprintf (M, NlsStr(TY_DATA), command, L->loc.file.looked_for);
				break;
				
			case AlienScript:
				mprintf (M, NlsStr(TY_ALIEN), command, 
					L->flags.was_hashed? NlsStr(TY_HASHED) : "",
					L->loc.file.looked_for,
					L->loc.file.executer);
				break;
	
			case Script:
				mprintf (M, NlsStr(TY_SCRIPT), command, 
					L->flags.was_hashed? NlsStr(TY_HASHED) : "",
					L->loc.file.looked_for);
				break;
	
			case Binary:
				mprintf (M, NlsStr(TY_BINARY), command, 
					L->flags.was_hashed? NlsStr(TY_HASHED) : "",
					L->loc.file.looked_for);
				break;
			
			case NotFound:
				mprintf (M, NlsStr(TY_UNKNOWN), command);
				break;
		}
	}
	else
	{
		switch (L->location)
		{
			case Function:
			case Internal:
				mprintf (M, "%s\n", command);
				break;
			
			case Data:
			case AlienScript:
			case Script:
			case Binary:
				mprintf (M, "%s\n", L->loc.file.looked_for);
				break;
				
			case NotFound:
				mprintf (M, NlsStr(TY_UNKNOWN), command);
				break;
		}
	}
}

/* jr: Subfunktion fÅr m_hash */

int ShowType (MGLOBAL *M, const char *cmdname, const char *what,
	int verbose)
{
	int retcode = 0;
	const char *alias;
	
	alias = GetAlias (M, what, TRUE);
	if (alias)
	{
		if (verbose)
			mprintf (M, NlsStr(TY_ALIAS), what, alias);
		else
			mprintf (M, "alias %s='%s'\n", what, alias);
	}
	else
	{
		LOCATION L;
		
		if (Locate (M, &L, what, FALSE, LOC_ALL))
		{
			printLocation (M, &L, what, verbose);
			FreeLocation (M, &L);
		}
		else if (L.broken)
		{
			eprintf (M, NlsStr(TY_ERROR), cmdname, what);
			retcode = 2;
		}
		else
		{
			mprintf (M, NlsStr(TY_UNKNOWN), what);
			retcode = ERROR_COMMAND_NOT_FOUND;
		}
	}
	
	return retcode;
}



/* Internes Kommando <type>, das ausgibt, wie etwas als Kommando
 * interpretiert wird.
 */
int m_type (MGLOBAL *M, int argc, char **argv)
{
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_type), 
			NlsStr(HLP_type));
	
	if (argc > 1)
	{
		int i, retcode, ret;
		
		retcode = 0;
		
		for (i = 1; i < argc; ++i)
		{
			ret = ShowType (M, argv[0], argv[i], FALSE);
			if (ret != 0) retcode = ret;
		}
					
		return retcode;
	}
	else
		return PrintUsage (M, argv[0], NlsStr(OPT_type));
}
