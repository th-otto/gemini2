/*
 *	@(#)Mupfel/smallcmd.c
 *	@(#)Julian F. Reschke & Stefan Eissing, 14. MÑrz 1994
 *
 * jr 9.9.95
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\strerror.h"
#include "mglob.h"

#include "chario.h"
#include "getopt.h"
#include "locate.h"
#include "mdef.h"
#include "parsinpt.h"
#include "parsutil.h"
#include "shellvar.h"
#include "smallcmd.h"

/* internal texts
 */
#define NlsLocalSection "M.smallcmd"
enum NlsLocalText{
GO_OOMEMORY,	/*%s: Nicht genug Speicher\n*/
OPT_basename,	/*zeichenkette [suffix]*/
HLP_basename,	/*Dateiname aus vollem Pfad extrahieren*/
OPT_dirname,	/*[pfadname]*/
HLP_dirname,	/*Ordnername aus vollem Pfad extrahieren*/
OPT_eval,	/*zeichenkette*/
HLP_eval,	/*Zeichenkette als Befehl ausfÅhren*/
OPT_dot,		/*name*/
HLP_dot,		/*Kommandos von Datei name lesen*/
DOT_CANTOPEN,	/*%s: kann %s nicht îffnen\n*/
DOT_NOSCRIPT,	/*%s: %s ist kein Shell-Script\n*/
HLP_false,	/*Exit-Status auf falsch(1) setzen*/
OPT_getopt,	/*legale-Optionen [Optionen]*/
HLP_getopt,	/*Optionen testen und auflîsen*/
HLP_true,	/*Exit-Status auf wahr(0) setzen*/
HLP_empty,	/*leeres Kommando*/
OPT_break,	/*[ n ]*/
HLP_break,	/*Beende n for/while Schleifen*/
OPT_continue,	/*[ n ]*/
HLP_continue,	/*Setze die n-te for/while Schleife fort*/
OPT_return,	/*[ n ]*/
HLP_return,	/*Verlasse Funktion mit Returnwert n*/
BAD_RETURN,	/*return ist nur in einer Funktion erlaubt!\n*/
OPT_exit,	/*[ n ]*/
HLP_exit,	/*Shell mit Returnwert n verlassen*/
OPT_shift,	/*[zahl]*/
HLP_shift,	/*interne Variablen um <zahl> oder 1 verschieben*/
OPT_echo,	/*[args]*/
HLP_echo,	/*Argumente ausgeben*/
};


/*
 * display str. may contain escape sequences:
 *	%t	TAB
 *  %b  Backspace
 *	%f  Formfeed
 *  %r  CR
 *	%n	CR/LF
 *  %%	%
 *	%c	(only at end of last arg) suppresses CR/LF
 *	%0	lead-in for up aribrary character, represented as max 3
 *		octal digits
 */
static void echoArg (MGLOBAL *M, char *str, int lastarg, int *newline)
{
	char ch;
	int last_was_cr = FALSE;
	
	while (*str != '\0')
	{
		if (*str == M->tty.escape_char && *(str + 1) != '\0')
		{
			++str;
			switch (*str)
			{
				case 't':
					canout (M, '\t');
					break;
				case 'b':
					canout (M, '\b');
					break;
				case 'f':
					canout (M, '\f');
					break;
				case 'r':
					canout (M, '\r');
					break;
				case 'c':
					if (lastarg && *(str + 1) == '\0')
						*newline = FALSE;
					else
						canout(M, *str);
					break;
				case 'n':
					crlf (M);
					break;
				case '0':
					++str;
					ch = '\0';
					while (*str >= '0' && *str <= '7')
					{
						ch = (ch*8) + (*str - '0');
						++str;
					}
					--str;
					canout (M, ch);
					break;
				default:
					canout (M, '%');
					canout (M, *str);
					break;
			}
		}
		else if ((*str == '\n') && !last_was_cr)
			crlf (M);
		else
			canout (M, *str);
		
		last_was_cr = (*str == '\r');
		++str;
	}
}

/*
 * internes "echo" Kommando
 * Åbergebe jedes Argument an echoArg()
 */
int m_echo (MGLOBAL *M, int argc, char **argv)
{
	int i, newline;
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_echo), 
			NlsStr(HLP_echo));
	
	newline = TRUE;
	
	for (i = 1; i < argc && !intr (M); ++i)
	{
		echoArg (M, argv[i], (i==argc-1), &newline);
		if (i < argc - 1)
			canout(M, ' ');
	}
	if (newline)
		crlf (M);
		
	return 0;
}


int m_shift (MGLOBAL *M, int argc, char **argv)
{
	int count;
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_shift), 
			NlsStr(HLP_shift));
	
	count = (argc > 1)? atoi (argv[1]) : 1;
	
	return !ShiftShellVars (M, count);
}


int m_true (MGLOBAL *M, int argc, char **argv)
{
	if (!argc)
		return PrintHelp (M, argv[0], "", NlsStr(HLP_true));
	
	return 0;
}

int m_false (MGLOBAL *M, int argc, char **argv)
{
	if (!argc)
		return PrintHelp (M, argv[0], "", NlsStr(HLP_false));
	
	return 1;
}

int m_empty (MGLOBAL *M, int argc, char **argv)
{
	if (!argc)
		return PrintHelp (M, argv[0], "", NlsStr(HLP_empty));
	
	return 0;
}

/* jr: neue Implementation lt. POSIX 1003.2 D11.2 S. 395 unten */

int m_dirname (MGLOBAL *M, int argc, char **argv)
{
	char *name, *last;
	int drivesep = FALSE;
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_dirname), 
			NlsStr(HLP_dirname));
	
	/* jr: keine Argumente: lt. POSIX Resultat unspecified :-) */
	
	if (argc == 1)
	{
		mprintf (M, ".\n");
		return 0;
	}
	
	if (argc != 2)
		return PrintUsage (M, argv[0], NlsStr(OPT_dirname));

	/* jr: name und drivesep setzen */

	name = argv[1];

	if (strlen (name) >= 2 && name[1] == ':')
	{
		name += 2;
		drivesep = TRUE;
	}

	/* (1) If string is //, skip steps (2) through (5) */
	
	if (strcmp (name, "\\\\"))
	{
		/* (2) If string consists entirely of slash characters,
		string shall be set to a single slash character. In this
		case, skip steps (3) trough (8) */
		
		int only_slashes = (*name == '\\');
		char *c = name;
		
		while (*c)
			if (*c++ != '\\')
				only_slashes = FALSE;
				
		if (only_slashes)
			name[1] = '\0';
		else
		{
			/* (3) If there are any trailing slash characters in
			string, they shall be removed */
			
			last = name;
			while (*last) last++;
			while (last != name && '\\' == *(last - 1))
				*--last = '\0';

			/* (4) If there are no slash characters remaining in
			string, string shall be set to a single period
			character. In this case, skip steps (5) through (8) */
			
			if (! strchr (name, '\\'))
			{
				if (drivesep) mprintf (M, "%c:", argv[1][0]);
				mprintf (M, ".\n");
				return 0;
			}
		
			/* (5) If there are any trailing nonslash characters
			in string, they shall be removed */
			
			last = name;
			while (*last) last++;
			while (last != name && '\\' != *(last - 1))
				*--last = '\0';
		}
	}
	
	/* (6) If the remaing string is //, it is implementation
	defined whether steps (7) and (8)are skipped or processed */
			
	/* (7) If there are any trailing slash characters in string,
	they shall be removed */
			
	last = name;
	while (*last) last++;
	while (last != name && '\\' == *(last - 1))
		*--last = '\0';
				
	/* (8) If the remaining string is empty, string shall be set
	to a single slash character */
			
	if (*name == '\0') name = "\\";
	
	/* The resulting string shall be written to standard output */

	if (drivesep) mprintf (M, "%c:", argv[1][0]);
	mprintf (M, "%s\n", name);
	return 0;
}


/* jr: neue Implementation lt. POSIX 1003.2 D11.2 S. 297 unten */

int m_basename (MGLOBAL *M, int argc, char **argv)
{
	char *name, *suffix = NULL;
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_basename), 
			NlsStr(HLP_basename));
	
	if (argc < 2 || argc > 3)
		return PrintUsage (M, argv[0], NlsStr(OPT_basename));

	/* jr: name und suffix setzen */

	name = argv[1];
	if (argc > 2) suffix = argv[2];

	if (strlen (name) >= 2 && name[1] == ':')
		name += 2;

	/* (1) If string is //, it is implementation defined whether
	steps (2) through (5) are processed */
	
	{
		/* (2) If string consists entirely of slash characters,
		string shall be set to a single slash character. In this
		case, skip steps (3) through (5) */
		
		int only_slashes = (*name == '\\');
		char *c = name;
		
		while (*c)
			if (*c++ != '\\')
				only_slashes = FALSE;
				
		if (only_slashes)
			name[1] = '\0';
		else
		{
			/* (3) If there are any trailing slash characters
			in string, they shall be removed */
			
			char *last;
			
			last = name;
			while (*last) last++;
			while (last != name && '\\' == *(last - 1))
				*--last = '\0';

			/* (4) If there are any slash characters remaining in
			string, the prefix of string up to and including
			the last slash character in string shall be removed */
			
			last = strrchr (name, '\\');
			if (last) name = last + 1;
			
			/* (5) If the suffix operand is present, is not
			identical to the characters remaining in string, and
			is identical to a suffix of the characters remaining
			in string, the suffix suffix shall be removed from
			string. Otherwise, string shall not be modified by
			this step. It shall not be considered an error if suffix
			is not found in string */
		
			if (suffix)
			{
				size_t lsuffix = strlen (suffix);
				size_t lname = strlen (name);
				
				if (lname != lsuffix &&
					!strcmp (name + lname - lsuffix, suffix))
					name[lname-lsuffix] = '\0';
			}
		}
	}

	mprintf (M, "%s\n", name);
	return 0;
}


int m_getopt (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	int	c;
	int	errflg = 0;
	char tmpstr[4];
	char *outstr;
	int local_argc;
	char **local_argv;
	char *option_string;

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_getopt), 
			NlsStr(HLP_getopt));
	
	if (argc < 2)
		return PrintUsage (M, argv[0], NlsStr(OPT_getopt));

	local_argc = argc - 1;
	local_argv = mcalloc (&M->alloc, argc + 1, sizeof (char *));

	outstr = mmalloc (&M->alloc, 5120);	/* SYSV command line limit */

	if (!outstr || !local_argv)
	{
		if (outstr) 
			mfree (&M->alloc, outstr);
		if (local_argv) 
			mfree (&M->alloc, local_argv);
			
		eprintf (M, NlsStr(GO_OOMEMORY), "getopt");
		return 2;
	}

	*outstr = 0;

	/* Argumentliste kopieren */
	
	{
		int i;

		for (i = 1; i < argc; i++)
			local_argv[i] = argv [i+1];

		local_argv[++i] = NULL;
		local_argv[0] = argv[0];
	}
	
	option_string = argv[1];
	
	if (*option_string == '-') option_string += 1;
	
	optinit (&G);

	while ((c = getopt (M, &G, local_argc, local_argv, option_string))
		!= EOF)
	{
		if (c == '?')
		{
			errflg++;
			continue;
		}

		sprintf (tmpstr, "-%c ", c);
		strcat (outstr, tmpstr);

		if (*(strchr(argv[1], c)+1) == ':')
		{
			strcat (outstr, G.optarg);
			strcat (outstr, " ");
		}
	}

	if (errflg)
	{
		mfree (&M->alloc, outstr);
		mfree (&M->alloc, local_argv);
		return 2;
	}
	
	strcat (outstr, "-- ");

	while (G.optind < local_argc)
	{
		strcat (outstr, local_argv[G.optind++]);
		strcat (outstr, " ");
	}
	
	{
		int i = 0;

		while (outstr[i])
			mprintf (M, "%c", outstr[i++]);
	}
	crlf (M);
	
	mfree (&M->alloc, outstr);
	mfree (&M->alloc, local_argv);
	return 0;
}


int m_dot (MGLOBAL *M, int argc, char **argv)
{
	int retcode;
	
	if (argc <= 1)
		return PrintHelp (M, argv[0], NlsStr(OPT_dot), 
			NlsStr(HLP_dot));
	
	retcode = M->exit_value;
		
	if (argc > 1)
	{
		LOCATION L;
		PARSINPUTINFO PSave;
		int found, script;
		
		found = Locate (M, &L, argv[1], TRUE, LOC_EXTERNAL);
		script = found && (L.location == Script);

		if (script && E_OK == ParsFromFile (M, &PSave, L.loc.file.full_name))
		{
			ParsAndExec (M);
			ParsRestoreInput (M, &PSave);
		}
		else if (!found)
		{
			eprintf (M, NlsStr(DOT_CANTOPEN), argv[0], argv[1]);
			retcode = ERROR_COMMAND_NOT_FOUND;
		}
		else
		{
			eprintf (M, NlsStr(DOT_NOSCRIPT), argv[0], argv[1]);
			retcode = ERROR_NOT_EXECUTABLE;
		}

		FreeLocation (M, &L);
	}
	
	return retcode;
}

int m_eval (MGLOBAL *M, int argc, char **argv)
{

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_eval), 
			NlsStr(HLP_eval));
	
	M->exit_value = 0;
		
	if (argc > 1)
	{
		PARSINPUTINFO PSave;
		
		ParsFromArgv (M, &PSave, argc - 1, &argv[1]);
		
		ParsAndExec (M);

		ParsRestoreInput (M, &PSave);
	}
	
	return M->exit_value;
}

int m_break (MGLOBAL *M, int argc, char **argv)
{
	if (!argc || argc > 2)
		return PrintHelp (M, argv[0], NlsStr(OPT_break), 
			NlsStr(HLP_break));
		
	if (M->loop_count)
	{
		int count;
		
		count = argc > 1 ? atoi (argv[1]) : 1;
		
		if (count < 1) return 1;
			
		M->exec_break = 1;
		M->break_count = count;
		if (M->break_count > M->loop_count)
			M->break_count = M->loop_count;
	}
	
	return 0;
}

int m_continue (MGLOBAL *M, int argc, char **argv)
{
	if (!argc || argc > 2)
		return PrintHelp (M, argv[0], NlsStr(OPT_continue), 
			NlsStr(HLP_continue));
		
	if (M->loop_count)
	{
		int count;
		
		count = (argc > 1)? atoi (argv[1]) : 1;
		if (count < 1)
			return 1;
		
		M->exec_break = 1;
		M->break_count = count;
		if (M->break_count > M->loop_count)
			M->break_count = M->loop_count;

		M->break_count = -M->break_count;
	}
	
	return 0;
}

int m_return (MGLOBAL *M, int argc, char **argv)
{
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_return), 
			NlsStr(HLP_return));
		
	if (M->function_count == 0)
	{
		eprintf (M, NlsStr(BAD_RETURN));
		return 1;
	}

	M->exec_break = 1;
	return (argc > 1) ? atoi(argv[1]) : M->exit_value;
}

int m_exit (MGLOBAL *M, int argc, char **argv)
{
	int retcode;
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_exit), 
			NlsStr(HLP_exit));
		
	M->exec_break = M->exit_shell = 1;
	retcode = (argc > 1) ? atoi(argv[1]) : M->exit_value;
	M->exit_exit_value = retcode;
	
	return retcode;
}
