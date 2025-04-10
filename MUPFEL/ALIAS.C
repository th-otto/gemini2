/*
 * @(#) Mupfel\alias.c
 * @(#) Stefan Eissing, 27. Februar 1993
 *
 * jr, 17. Januar 1994: alias -> POSIX.2, unalias
 *
 * Aliase fÅr Kommandos
 */


#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"

#include "mglob.h"

#include "alias.h"
#include "chario.h"
#include "getopt.h"

/* internal texts
 */
#define NlsLocalSection "M.alias"
enum NlsLocalText{
OPT_alias,		/*[Name[=Kommando]]...*/
HLP_alias,		/*Namen mit Kommando versehen*/
OPT_unalias,	/*-a | Name...*/
HLP_unalias,	/*Namen lîschen*/
};

typedef struct aliasInfo
{
	struct aliasInfo *next;
	
	const char *name;
	const char *value;
	
	struct
	{
		unsigned visited;
	} flags;
	
} ALIASINFO;


/* Inititalisiere die Alias-Informationen in <M>
 */
void InitAlias (MGLOBAL *M)
{
	M->alias = NULL;
}


static ALIASINFO *dupAliasInfo (MGLOBAL *M, const ALIASINFO *old)
{
	ALIASINFO *A;
	
	A = mcalloc (&M->alloc, 1, sizeof (ALIASINFO));
	if (!A)
		return NULL;
	
	A->name = StrDup (&M->alloc, old->name);
	A->value = StrDup (&M->alloc, old->value);
	
	if (!A->name || !A->value)
	{
		if (A->name)
			mfree (&M->alloc, A->name);
		if (A->value)
			mfree (&M->alloc, A->value);
		mfree (&M->alloc, A);
			
		return NULL;
	}
	
	return A;
}


int ReInitAlias (MGLOBAL *M, MGLOBAL *new)
{
	ALIASINFO *akt, **newalias;
	
	new->alias = NULL;

	newalias = &((ALIASINFO *)new->alias);
	akt = M->alias;

	while (akt)
	{
		*newalias = dupAliasInfo (new, akt);
		if (!*newalias)
			return FALSE;
		
		(*newalias)->next = NULL;
		newalias = &((*newalias)->next);
		akt = akt->next;
	}
	
	return TRUE;
}


/* Gib die ALIASINFO <A> wieder frei.
 */
static void freeAliasInfo (MGLOBAL *M, ALIASINFO *A)
{
	mfree (&M->alloc, A->name);
	mfree (&M->alloc, A->value);

	mfree (&M->alloc, A);
}


/* Lîsche alle Alias-Informationen im Shell-Level <M>
 */
static void clearAlias (MGLOBAL *M)
{
	ALIASINFO *A, *next;
	
	A = M->alias;
	
	while (A)
	{
		next = A->next;
		freeAliasInfo (M, A);
		A = next;
	}
	
	M->alias = NULL;
}


/* Wird beim Verlassen der Shell <M> aufgerufen und gibt den
 * Speicherplatz wieder frei
 */
void ExitAlias (MGLOBAL *M)
{
	clearAlias (M);
}



const char *getAlias (MGLOBAL *M, const char *name)
{
	ALIASINFO *A;
	
	A = M->alias;
	
	while (A)
	{
		if (!A->flags.visited && !strcmp (A->name, name))
		{
			A->flags.visited = 1;
			return A->value;
		}
		
		A = A->next;
	}
	
	return NULL;
}

const void clearVisited (MGLOBAL *M)
{
	ALIASINFO *A = M->alias;
	
	while (A)
	{
		A->flags.visited = 0;
		A = A->next;
	}
}

const char *GetAlias (MGLOBAL *M, const char *name, int first_search)
{
	const char *found, *alias;
	
	alias = NULL;
	if (first_search)
		clearVisited (M);
	
	do
	{
		found = getAlias (M, name);
		if (found)
			name = alias = found;
	}
	while (found);

	return alias;
}

static int removeAlias (MGLOBAL *M, const char *name)
{
	ALIASINFO *A, **pprev;
	
	pprev = &((ALIASINFO *)M->alias);
	A = M->alias;
	while (A)
	{
		if (!strcmp (A->name, name))
		{
			*pprev = A->next;
			freeAliasInfo (M, A);
			
			return TRUE;
		}
		
		pprev = &A->next;
		A = A->next;
	}
	
	return FALSE;
}


static int enterAlias (MGLOBAL *M, const char *name, 
	const char *value)
{
	ALIASINFO old;
	ALIASINFO *A;

	removeAlias (M, name);
	
	if (!value)
		return TRUE;
		
	memset (&old, 0, sizeof (ALIASINFO));
	old.name = name;
	old.value = value;

	A = dupAliasInfo (M, &old);
	if (!A)
		return FALSE;
	
	{
		ALIASINFO *akt, **pprev;
		
		pprev = &((ALIASINFO *)M->alias);
		akt = M->alias;
		while (akt)
		{
			if (strcmp (akt->name, A->name) > 0)
				break;
			
			pprev = &akt->next;
			akt = akt->next;
		}
		
		*pprev = A;
		A->next = akt;
	}
	
	return TRUE;
}



/* Internes Kommando <alias>, das zum Vergeben neuer Namen fÅr
 * Befehle und zum Lîschen derselben dient.
 */
 
/* jr: made it POSIX.2 compliant */
 
int m_alias (MGLOBAL *M, int argc, char **argv)
{
	int retcode = 0;

	if (!argc)
		return PrintHelp (M, argv[0], 
			NlsStr (OPT_alias), NlsStr (HLP_alias));
	
	if (argc == 1)
	{
		ALIASINFO *A;
		
		/* Hier hatten wir keine Parameter und geben
		 * also die Liste der Aliase aus.
		 */
		
		for (A = M->alias; A && !intr (M); A = A->next)
		{
			mprintf (M, "%s='%s'\n", A->name, A->value);
		}
	}	
	else
	{
		int i;
		
		for (i = 1; i < argc; i++)
		{
			char *assignment = strchr (argv[i], '=');
			
			if (assignment)	/* Zuweisung eines Alias */
			{
				*assignment++ = '\0';
				
				if (!enterAlias (M, argv[i], assignment))
					retcode = 1;
			}
			else /* Alias ausgeben */
			{
				ALIASINFO *A;
				int found = FALSE;
				
				for (A = M->alias; A; A = A->next)
				{
					if (!strcmp (argv[i], A->name))
					{
						found = TRUE;
						mprintf (M, "%s='%s'\n", A->name, A->value);
					}
				}
				
				if (!found) retcode = 1;
			}
		}
	}	
	
	return retcode;
}

int m_unalias (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	int retcode = 0;
	int all = FALSE;
	int c;
	ALIASINFO *A;
	
	struct option long_option[] =
	{
		{ "all", FALSE, NULL, 'a' },
		{ NULL, 0,0,0},
	};
	int opt_index = 0;

	optinit (&G);

	if (!argc)
		return PrintHelp (M, argv[0], 
			NlsStr (OPT_unalias), NlsStr (HLP_unalias));
	
	while ((c = getopt_long (M, &G, argc, argv, "a", 
		long_option, &opt_index)) != EOF)
	{
		if (!c)
			c = long_option[opt_index].val;
			
		switch (c)
		{
			case 0:
				break;

			case 'a':
				all = TRUE;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_unalias));
		}
	}

	if ((all && G.optind < argc) ||
		(!all && G.optind >= argc))
		return PrintUsage (M, argv[0], NlsStr(OPT_unalias));

	if (all)
		for (A = M->alias; A; A = A->next)
			removeAlias (M, A->name);
	else
		while (G.optind < argc)
			if (!removeAlias (M, argv[G.optind++]))
				retcode = 1;
	
	return retcode;
}

int
GetAliasCompletions (MGLOBAL *M, ARGVINFO *A, void *entry)
{
	char *what = entry;
	ALIASINFO *alias;
	
	alias = M->alias;
	
	while (alias)
	{
		if (!*what || strstr (alias->name, what) == alias->name)
			if (!StoreInArgv (A, alias->name)) return FALSE;
		
		alias = alias->next;
	}
	
	return TRUE;
}