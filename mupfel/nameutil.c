/*
 * @(#) Mupfel\nameutil.c
 * @(#) Stefan Eissing, 17. januar 1993
 *
 * jr 8.8.94
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"


#include "mglob.h"

#include "chario.h"
#include "getopt.h"
#include "makeargv.h"
#include "names.h"
#include "partypes.h"
#include "prnttree.h"
#include "subst.h"

/* internal texts
 */
#define NlsLocalSection "M.nameutil"
enum NlsLocalText{
OPT_export,	/*[ -p | name[=value] ... ]*/
HLP_export,	/*Variable fr Export auf value setzen*/
OPT_setenv,	/*[ name [value] ]*/
HLP_setenv,	/*Variable fr Export auf value setzen*/
OPT_readonly,	/*[ -p | name[=value] ... ]*/
HLP_readonly,	/*Variable mit Schreibschutz versehen*/
OPT_unset,	/*[ name ... ]*/
HLP_unset,	/*Variable/Funktionen entfernen*/
};


int NameExists (MGLOBAL *M, const char *name)
{
	return GetNameInfo (M, name) != NULL;
}

char *GetEnv (MGLOBAL *M, const char *name)
{
	NAMEINFO *n = GetNameInfo (M, name);

	if (n)
		return (char *)NameInfoValue (n);
	else
		return NULL;
}

int PutEnv (MGLOBAL *M, const char *name)
{
	char *scan;
	int retcode = FALSE;
	
	if ((scan = strchr (name, '=')) != NULL)
	{
		*scan = '\0';
		retcode = ExportName (M, name, scan + 1);
		*scan = '=';
	}
	
	return retcode;
}

int SetWordList (MGLOBAL *M, WORDINFO *name)
{
	while (name)
	{
		char *subst_string, *subst_quoted;
		char *value;
		int stored;
		
		if (!SubstWithCommand (M, name, &subst_string, &subst_quoted))
			return FALSE;
		
		value = strchr (subst_string, '=');
		if (value)
		{
			*value = '\0';
			++value;
		}
		else
			value = "";
			
		stored = InstallName (M, subst_string, value);
		
		mfree (&M->alloc, subst_string);
		mfree (&M->alloc, subst_quoted);
		if (!stored)
			return FALSE;
			
		name = name->next;
	}
	
	return TRUE;
}

/* jr: xxx: die macht dasselbe wie GetEnv! */
const char *GetVarValue (MGLOBAL *M, const char *name)
{
	return GetEnv (M, name);
#if 0
	NAMEINFO *n;
	
	n = GetNameInfo (M, name);
	if (n == NULL)
		return NULL;
	
	return NameInfoValue (n);
#endif
}

long GetVarInt (MGLOBAL *M, const char *name)
{
	const char *val;
	
	val = GetVarValue (M, name);
	
	if (val)
		return atol (val);
	else
		return 0;
}

typedef struct
{
	MGLOBAL *M;
	char *what;
} NAMECOMPINFO;

#define MAX_COMP_LEN	128

static int getCompletions (ARGVINFO *A, void *entry)
{
	MGLOBAL *M;
	NAMEINFO *n;
	char *what;
	char tmp[MAX_COMP_LEN + 3];
	int closeBrace = 0;
	
	M = ((NAMECOMPINFO *)entry)->M;
	what = ((NAMECOMPINFO *)entry)->what;
	if (*what == '{')
	{
		closeBrace = 1;
		++what;
	}
		
	for (n = M->name_list; n; n = n->next)
	{
		if (!n->flags.function 
			&& (!*what || (strstr (n->name, what) == n->name)))
		{
			if (closeBrace && (strlen (n->name) < MAX_COMP_LEN))
			{
				sprintf (tmp, "{%s}", n->name);
				if (!StoreInArgv (A, tmp))
					return FALSE;
			}
			else if (!StoreInArgv (A, n->name))
				return FALSE;
		}
	}
	
	return TRUE;
}

int GetNameCompletions (MGLOBAL *M, ARGVINFO *A, char *what)
{
	NAMECOMPINFO nc;
	
	nc.M = M;
	nc.what = what;
	
	return MakeGenericArgv (&M->alloc, A, FALSE, getCompletions, &nc);
}

int GetFunctionCompletions (MGLOBAL *M, ARGVINFO *A, char *entry)
{
	NAMEINFO *n;
	char *what = entry;
	
	for (n = M->name_list; n; n = n->next)
	{
		if (n->flags.function 
			&& (!*what || (strstr (n->name, what) == n->name)))
		{
			if (!StoreInArgv (A, n->name))
				return FALSE;
		}
	}
	
	return TRUE;
}


static void printName (MGLOBAL *M, NAMEINFO *n, char *prefix,
	char *suffix)
{
	char *val;

	if (n->flags.autoset) UpdateAutoVariable (M, n);	
	val = (char *)NameInfoValue (n);
	mprintf (M, prefix, n->name);
	domprint (M, val, strlen (val));
	mprintf (M, suffix);
	
}

static void printExports (MGLOBAL *M)
{
	NAMEINFO *n;

	for (n = M->name_list; n && !intr (M); n = n->next)		
		if (n->flags.exported)
			printName (M, n, "export %s='", "'\n");
}

int m_export (MGLOBAL *M, int argc, char **argv)
{
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_export), 
			NlsStr(HLP_export));

	if (argc == 1 || (argc == 2 
		&& (!strcmp (argv[1], "-p") || !strcmp (argv[1], "--print"))))
	{
		printExports (M);
	}
	else
	{
		int i;
		
		for (i = 1; i < argc; ++i)
		{
			char *sep = strchr (argv[i], '=');
	
			if (sep)
				*sep++ = '\0';
	
			if (*argv[i] == '\0')
				return 1;
		
			if (!ExportName (M, argv[i], sep))
				return 1;
		}
	}
	
	return 0;
}


int m_setenv (MGLOBAL *M, int argc, char **argv)
{
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_setenv), 
			NlsStr(HLP_setenv));

	if (argc > 1)
	{
		char *value = NULL;
		
		if (argc > 3)
			return PrintUsage (M, argv[0], NlsStr(OPT_setenv));
		else if (argc == 3)
			value = argv[2];
		
		if (!ExportName (M, argv[1], value))
			return 1;
	}
	else
		printExports (M);
	
	return 0;
}


int m_readonly (MGLOBAL *M, int argc, char **argv)
{
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_readonly), 
			NlsStr(HLP_readonly));

	if (argc == 1 || (argc == 2 
		&& (!strcmp (argv[1], "-p") || !strcmp (argv[1], "--print"))))
	{
		NAMEINFO *n;
		
		for (n = M->name_list; n && !intr (M); n = n->next)
			if (n->flags.readonly)
				printName (M, n, "readonly %s='", "'\n");
	}
	else
	{
		int i;
		
		for (i = 1; i < argc; ++i)
		{
			char *sep = strchr (argv[i], '=');
	
			if (sep)
				*sep++ = '\0';
	
			if (*argv[i] == '\0')
				return 1;
		
			if (!ReadonlyName (M, argv[i], sep))
				return 1;
		}
	}
	
	return 0;
}

int m_unset (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	struct option long_option[] =
	{
		{ "function", FALSE, NULL, 'f' },
		{ "variable", FALSE, NULL, 'v' },
		{ NULL,0,0,0 },
	};
	int opt_index = 0;
	int c, which, retcode;
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_unset), 
			NlsStr(HLP_unset));

	optinit (&G);
	which = 0;

	while ((c = getopt_long (M, &G, argc, argv, "vf", 
		long_option, &opt_index)) != EOF)
	{
		if (!c)
			c = long_option[opt_index].val;
			
		switch (c)
		{
			case 0:
				break;

			case 'v':
				which |= UNSET_VARIABLE;
				break;

			case 'f':
				which |= UNSET_FUNCTION;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_unset));
		}
	}

	if (!which)
		which = UNSET_VARIABLE|UNSET_FUNCTION;
	
	retcode = 0;
	for (; G.optind < argc; ++G.optind)
	{
		if (!UnsetNameInfo (M, argv[G.optind], which))
			retcode = 1;
	}
	
	return retcode;
}


void PrintAllNames (MGLOBAL *M)
{
	NAMEINFO *n;
	
	for (n = M->name_list; n && !intr (M); n = n->next)
	{
		if (n->flags.function)
		{
			PrintTreeInfo (M, (TREEINFO *)n->func);
			mprintf (M, "\n");
		}
		else
			printName (M, n, "%s='", "'\n");
	}
}