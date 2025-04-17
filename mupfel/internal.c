/*
 * @(#) Mupfel\Internal.c
 * @(#) Stefan Eissing, 23. Oktober 1994
 *
 * Verwaltung interner Kommandos
 *
 * jr 22.4.95
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\cookie.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "exectree.h"
#include "getopt.h"	/* jr! */
#include "internal.h"
#include "mdef.h"
#include "nameutil.h"
#include "names.h"	/* jr */
#include "subst.h"	/* jr */
#include "stand.h"

#include "ls.h"
#include "mcd.h"
#include "mkdir.h"
#include "mset.h"
#include "smallcmd.h"
#include "test.h"
#include "read.h"
#include "date.h"
#include "rmdir.h"
#include "rm.h"
#include "chmod.h"
#include "cpmv.h"
#include "backup.h"
#include "rename.h"
#include "history.h"
#include "fkey.h"
#include "trap.h"
#include "hash.h"
#include "cookie.h"
#include "alias.h"
#include "type.h"
#include "version.h"
#include "free.h"
#include "label.h"
#include "times.h"
#include "init.h"
#include "format.h"
#include "shrink.h"

#if MERGED
#include "setscrap.h"
#endif


/* internal texts
 */
#define NlsLocalSection "M.internal"
enum NlsLocalText{
OPT_help,	/*[cmd]*/
HLP_help,	/*Liste aller Kommandos oder Hilfe zu cmd*/
OPT_builtin,	/*cmd*/
HLP_builtin, /*internes Kommando cmd ausfÅhren*/
OPT_command, /*[-vV] cmd [ arg... ]*/
HLP_command, /*Kommando (nicht Funktion) cmd ausfÅhren bzw. Info Åber cmd anzeigen\n*/
OPT_umask,	/*Maske*/
HLP_umask,	/*Zugriffsrechte bei erzeugten Dateien festlegen*/
OPT_enable,	/*[ cmd ... ]*/
HLP_enable,	/*interne Kommandos cmd einschalten/anzeigen*/
HLP_disable,	/*interne Kommandos cmd abschalten/anzeigen*/
HE_NOCMD,	/*%s: %s ist kein eingebautes Mupfel-Kommando\n*/
HE_LIST,	/*Liste der internen Kommandos (mehr mit: help Kommando)\n*/
HE_NOMEM,	/*%s: nicht genug Speicher vorhanden\n*/
};


int m_builtin (MGLOBAL *M, int argc, char **argv)
{
	if (!argc)
		return PrintHelp (M, argv[0], 
			NlsStr (OPT_builtin), NlsStr (HLP_builtin));
	
	if (argc <= 1)
		return PrintUsage (M, argv[0], NlsStr (OPT_builtin));
	
	ExecArgv (M, argc - 1, &argv[1], M->shell_flags.break_on_false,
		NULL, CMD_INTERNAL, NULL, FALSE, FALSE);
	
	return M->exit_value;
}


/* jr: Flags -v und -V */

int m_command (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	struct option long_option[] =
	{
		{"pathname",	0, NULL, 'v'},
		{"describe",	0, NULL, 'V'},
		{ NULL, 0,0,0},
	};
	int opt_index = 0;
	int c;
	int action = 0;

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr (OPT_command), 
			NlsStr (HLP_command));

	optinit (&G);
	
	/* jr: das '-' am Anfang des optstrings schaltet das
	Umsortieren der Optionen aus */
	
	while ((c = getopt_long (M, &G, argc, argv, "-vV", 
		long_option, &opt_index)) != EOF)
	{
		if (!c)
			c = long_option[opt_index].val;

		if (c == 1)	/* Argument!!! */
		{
			G.optind--;
			break;
		}
			
		switch (c)
		{
			case 0:
				break;
			
			case 'v':
			case 'V':
				action = c;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr (OPT_command));
		}
	}

	if (G.optind > argc - 1)
		return PrintUsage (M, argv[0], NlsStr (OPT_command));
	
	if (!action)	/* nicht -v oder -V */
	{
		ExecArgv (M, argc - G.optind, &argv[G.optind],
			M->shell_flags.break_on_false, NULL,
			CMD_ALL & ~CMD_FUNCTION, NULL, FALSE, FALSE);
		return M->exit_value;
	}
	else
	{
		int retcode = 0;
		int verbose = action == 'V';
		
		while (G.optind < argc)
		{
			int ret = ShowType (M, argv[0], argv[G.optind++], verbose);
			
			if (ret) retcode = ret;
		}

		return retcode;
	}
}


/* xxx vgl Manual */

int m_umask (MGLOBAL *M, int argc, char **argv)
{
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr (OPT_umask), 
			NlsStr (HLP_umask));

	if (argc != 2)
		return PrintUsage (M, argv[0], NlsStr (OPT_umask));

	if (argc == 2)
	{
		char *arg = argv[1];
		short i;
		char c;
		
		i = 0;
		while ((c = *arg++) >= '0' && c <= '7')
			i = (i << 3) + c - '0';

		if (Pumask (i) != EINVFN)
			return 0;
		else
			return EINVFN;
	}
	
	return 1;
}

typedef struct
{
	const char *name;			/* Name des Kommandos */
	int  (*func)(MGLOBAL *M, int argc, char **argv);
	
} INTERNALINFO;

int m_help (MGLOBAL *M, int argc, char **argv);
int m_enable (MGLOBAL *M, int argc, char **argv);

static INTERNALINFO internals[] =
{
	{"alias", m_alias},
	{"backup", m_backup},
	{"basename", m_basename},
	{"break", m_break},
	{"builtin", m_builtin},
#if CAT_INTERNAL
	{"cat", m_cat},
#endif
	{"cd", m_cd},
#if CHMOD_INTERNAL
	{"chmod", m_chmod},
#endif
	{"command", m_command},
	{"continue", m_continue},
	{"cookie", m_cookie},
	{"cp", m_cp},
#if DATE_INTERNAL
	{"date", m_date},
#endif
	{"dir", m_ls},
	{"dirname", m_dirname},
	{"dirs", m_dirs},
	{"disable", m_enable},
	{"echo", m_echo},
	{"enable", m_enable},
	{"exit", m_exit},
	{"export", m_export},
	{"eval", m_eval},
	{"false", m_false},
	{"fc", m_fc},
	{"fkey", m_fkey},
	{"format", m_format},
	{"free", m_free},
	{"getopt", m_getopt},
	{"hash", m_hash},
	{"help", m_help},
	{"history", m_history},
	{"init", m_init},
	{"label", m_label},
	{"ls", m_ls},
	{"mkdir", m_mkdir},
	{"mv", m_cp},
	{"popd", m_popd},
	{"pushd", m_pushd},
	{"pwd", m_pwd},
	{"read", m_read},
	{"readonly", m_readonly},
	{"rename", m_rename},
	{"return", m_return},
	{"rm", m_rm},
	{"rmdir", m_rmdir},
#if CMD_RUNOPTS
	{"runopts", m_runopts},
#endif
	{"set", m_set},
	{"setenv", m_setenv},
#if MERGED
	{"setscrap", m_setscrap},
#endif
	{"shift", m_shift},
	{"shrink", m_shrink},
	{"source", m_dot},
	{"test", m_test},
	{"times", m_times},
#if TOUCH_INTERNAL
	{"touch", m_touch},
#endif
		{"trap", m_trap},
	{"true", m_true},
	{"type", m_type},
	{"umask", m_umask},
	{"unalias", m_unalias},
	{"unset", m_unset},
	{"vdir", m_ls},
	{"version", m_version},
	{"[", m_test},
	{":", m_empty},
	{".", m_dot},
};

#define INTERNAL_DISABLED	1


int InitInternal (MGLOBAL *M)
{
	M->internal_flags = mcalloc (&M->alloc, DIM (internals), 1);
	
	return M->internal_flags != NULL;
}

int ReInitInternal (MGLOBAL *M, MGLOBAL *new)
{
	if (!InitInternal (new))
		return FALSE;
	
	memcpy (new->internal_flags, M->internal_flags, DIM (internals));
	return TRUE;
}

void ExitInternal (MGLOBAL *M)
{
	mfree (&M->alloc, M->internal_flags);
}

int IsInternalCommand (MGLOBAL *M, const char *name, int all, 
	int *number)
{
	int i = 0;
	
	/* Sonderbehandlung von "x:", das an das Kommando "cd"
	 * weitergegeben werden soll.
	 */
	if ((strlen (name) == 2) && (name[1] == ':') && (isalpha (*name)))
	{
		name = "cd";
	}
	
	while (i < DIM (internals))
	{
		if ((all || !(M->internal_flags[i] & INTERNAL_DISABLED))
			&& !strcmp (internals[i].name, name))
		{
			*number = i;
			return TRUE;
		}
		
		++i;
	}
	
	return FALSE;
}

int ExecInternalCommand (MGLOBAL *M, int number, int argc, 
	const char **argv)
{
	if ((number < 0) || (number > DIM (internals)))
	{
		eprintf (M, "unbekanntes internes Kommando %d\n", number);
		return -1;
	}
	
	if (!M->shell_flags.no_execution)
	{
		int ret;		

		if (M->shell_flags.print_as_exec)
		{
			const char *prompt = GetPS4Value (M);
			const char *xprompt = StringVarSubst (M, prompt);
					
			eprintf (M, "%s", xprompt ? xprompt : prompt);
			if (xprompt) mfree (&M->alloc, xprompt);
						
			printArgv (M, argc, argv);
		}
		
		ret = internals[number].func (M, argc, (char **)argv);

		if (wasintr (M))
		{
			if (MagiCVersion)
				ret = -68; /* Mag!X: Command interrupted */
			else if (MiNTVersion)
				ret = 512; /* SIGINT << 8 */
			else
				ret = -32; /* TOS: Command interrupted */
		}
		
		return ret;
	}
	
	return 0;
}


int GetInternalCompletions (MGLOBAL *M, ARGVINFO *A, void *entry)
{
	char *what = entry;
	int i;
	
	for (i = 0; i < DIM (internals); ++i)
	{
		if (!(M->internal_flags[i] & INTERNAL_DISABLED)
			&& (!*what
			|| strstr (internals[i].name, what) == internals[i].name))
		{
			if (!StoreInArgv (A, internals[i].name))
				return FALSE;
		}
	}
	
	return TRUE;
}


/*
 * internal "help" command
 * display list of internal commands or help line for each argument
 */
int m_help (MGLOBAL *M, int argc, char **argv)
{
	int i, l;

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr (OPT_help), 
			NlsStr (HLP_help));
	
	if (argc > 1)
	{
		for ( i = 1; i < argc; ++i)
		{
			if (IsInternalCommand (M, argv[i], TRUE, &l))
			{
				ExecInternalCommand (M, l, 0, &argv[i]);
			}
			else
				eprintf (M, NlsStr (HE_NOCMD), argv[0], argv[i]);
		}
	}
	else
	{
		char *narg[] = { "help", NULL };
		const char **myarg;
		
		crlf (M);
		m_version (M, 1, narg);
		mprintf (M, NlsStr (HE_LIST));
		crlf (M);
		
		myarg = mmalloc (&M->alloc, DIM (internals) * sizeof (char *));
		if (myarg)
		{
			for (i = 0; i < DIM (internals); ++i)
				myarg[i] = internals[i].name;
			
			PrintStringArray (M, (int)DIM (internals), myarg);
			mfree (&M->alloc, myarg);
		}
		else
		{
			eprintf (M, NlsStr (HE_NOMEM), argv[0]);
			return 1;
		}
	}
		
	return 0;
}


/*
 * internal "disable" command
 */
int m_enable (MGLOBAL *M, int argc, char **argv)
{
	int i, l;
	int disable = strcmp (argv[0], "enable");
	int retcode = 0;

	if (!argc)
	{
		return PrintHelp (M, argv[0], NlsStr (OPT_enable), 
			disable? NlsStr (HLP_disable) : NlsStr (HLP_enable));
	}
	
	if (argc > 1)
	{
		for (i = 1; i < argc; ++i)
		{
			if (IsInternalCommand (M, argv[i], TRUE, &l))
			{
				if (disable)
					M->internal_flags[l] |= INTERNAL_DISABLED;
				else
					M->internal_flags[l] &= ~INTERNAL_DISABLED;
			}
			else
			{
				eprintf (M, NlsStr (HE_NOCMD), argv[0], argv[i]);
				retcode = 1;
			}
		}
	}
	else
	{
		for (i = 0; i < DIM (internals); ++i)
		{
			if ((!disable)
				^ ((M->internal_flags[i] & INTERNAL_DISABLED) != 0))
			{
				mprintf (M, "%s\n", internals[i].name);
			}
		}
	}
	
	return retcode;
}