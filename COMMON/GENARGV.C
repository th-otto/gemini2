/*
 * @(#) Common\GenArgv.c
 * @(#) Stefan Eissing, 12. November 1994
 */


#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "argvinfo.h"
#include "charutil.h"
#include "genargv.h"

#ifndef TRUE
#define FALSE	0
#define TRUE	1
#endif


/* Schrittweite, in der das argv-Array wachsen soll
 */
#define ARGVGROW	512


/* Speicher den String <str> in der ARGVINFO <A> ab. Gibt bei
   Fehlern FALSE zurck. */

int
StoreInArgv (ARGVINFO *A, const char *str)
{
	if (A->flags.no_doubles)
	{
		int i;
		
		for (i = 0; i < A->argc; ++i)
			if (!strcmp (A->argv[i], str))
				return TRUE;
	}
	
	/* Noch genug Platz im Array? */
	if (A->argc >= A->array_size)
	{
		char **temp;
		int new_size = A->array_size + ARGVGROW;
		
		if (A->argv)
			temp = mrealloc (A->alloc, A->argv, 
				new_size * sizeof (char *));
		else
			temp = mmalloc (A->alloc, new_size * sizeof (char *));

		if (!temp) return FALSE;
		
		A->argv = temp;
		A->array_size = new_size;
	}
	
	if ((A->argv[A->argc] = StrDup (A->alloc, str)) == NULL)
		return FALSE;

	++(A->argc);
	
	return TRUE;
}

static void zeroArgvInfo (ARGVINFO *A)
{
	memset (A, 0, sizeof (ARGVINFO));
}

void InitArgv (ARGVINFO *A)
{
	zeroArgvInfo (A);
}

void FreeArgv (ARGVINFO *A)
{
	if (!A->argc || !A->argv)
		return;
		
	for ( ; A->argc; --A->argc)
	{
		mfree (A->alloc, A->argv[A->argc - 1]);
	}
	
	mfree (A->alloc, A->argv);
	zeroArgvInfo (A);
}


int
MakeGenericArgv (ALLOCINFO *alloc, ARGVINFO *A, int no_doubles, 
	GenerateArgvFunc *func, void *func_entry)
{
	zeroArgvInfo (A);

	A->alloc = alloc;
	A->flags.no_doubles = no_doubles;
	
	/* Wenn <func> == NULL ist, dann ist diese Funktion nur eine
	 * Initialisierung von <A>
	 */
	if (!func)
		return TRUE;
		
	if (!func (A, func_entry))
	{
		FreeArgv (A);
		return FALSE;
	}
	
	return TRUE;
}

int String2Argv (ALLOCINFO *alloc, ARGVINFO *A, const char *string,
	int removeQuotes)
{
	char *start, *copy;
	zeroArgvInfo (A);

	A->alloc = alloc;
	A->flags.no_doubles = 0;

	start = copy = StrDup (alloc, string);
	if (!copy)
		return FALSE;
		
	while (*start)
	{
		char *read, *write, c;
		int quoting = 0;
		
		/* šberspringe Leerzeichen am Anfang
		 */
		while (*start == ' ')
			++start;
		
		/* F„ngt das Wort mit ' an und ist Quoting benutzt worden?
		 */
		if (*start == '\'')
		{
			++start;
			quoting = removeQuotes;
		}
		
		for (read = write = start; 
			*read && (quoting || (*read != ' '));
			 ++read)
		{
			/* Wenn Quotes vorhanden sind...
			 */
			if (quoting)
			{
				/* Und zwei ' hintereinander kommen...
				 */
				if ((*read == '\'') && (read[1] != '\''))
				{
					/* Dann mache einen ' daraus
					 */
					++read;
				}
				else
				{
					/* Ansonsten ist das Wort zu Ende.
					 */
					break;
				}
			}

			*write++ = *read;
		}
		
		/* Ende des Wortes erreicht
		 */
		c = *write;
		*write = '\0';
		if (!StoreInArgv (A, start))
		{
			mfree (alloc, copy);
			return FALSE;
		}
		*write = c;
		
		if (!*read)
			break;

		start = read + 1;
	}
	
	mfree (alloc, copy);
	return TRUE;	
}


char *Argv2String (ALLOCINFO *alloc, ARGVINFO *A, int quoting)
{
	size_t len = 1;
	int i;
	char *names;
	
	for (i = 0; i < A->argc; ++i)
	{
		const char *cp = A->argv[i];
		
		len += strlen (cp) + 3;
		
		while ((cp = strchr (cp, '\'')) != NULL)
		{
			++cp;
			++len;
		}
	}
	
	names = mmalloc (alloc, len);
	if (names != NULL)
	{
		*names = '\0';
		
		for (i = 0; i < A->argc; ++i)
		{
			if (i)
				strcat (names, " ");

			if (quoting && strpbrk (A->argv[i], " \'"))
			{
				char *cn;
				const char *ca;
				
				strcat (names, "'");
				cn = strrchr (names, '\'') + 1;
				for (ca = A->argv[i]; *ca; ++ca, ++cn)
				{
					if (*ca == '\'')
						*cn++ = *ca;
					*cn = *ca;
				}
				*cn = '\0';
				strcat (names, "'");
			}
			else
				strcat (names, A->argv[i]);
		}
	}

	return names;
}
