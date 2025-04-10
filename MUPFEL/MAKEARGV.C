/*
 * @(#) Mupfel\MakeArgv.c
 * @(#) Stefan Eissing, 28. Mai 1992
 */


#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"

#include "mglob.h"

#include "expand.h"
#include "makeargv.h"
#include "names.h"
#include "partypes.h"
#include "subst.h"


int StoreInExpArgv (ARGVINFO *A, const char *str)
{
	return StoreInArgv (A, str);
}

/*
 * Splitte den substituierten String anhand des IFS auf.
 * Jeder so erhaltenen Eintrag wird dann expandiert, wenn
 * m”glich.
 */
static int splitExpandAndStore (MGLOBAL *M, ARGVINFO *A, 
	char *subst_string,	const char *subst_quoted)
{
	char *word_start = subst_string;
	char *word_end = subst_string;
	const char *quote_start = subst_quoted;
	const char *quote_end;
	const char *ifs = GetIFSValue (M);
	
	/* Solange in dem String etwas enthalten ist...
	 */
	while ((*word_start != '\0') || *quote_start)
	{
		/* šberspringe Startzeichen, die nicht gequoted sind und
		 * im IFS enthalten sind.
		 */
		while (!*quote_start 
			&& *word_start 
			&& (strchr (ifs, *word_start) != NULL))
		{
			++word_start;
			++quote_start;	
		}
		word_end = word_start;
		quote_end = quote_start;
		
		/* šberspringe nun Zeichen, die entweder gequoted sind
		 * oder nicht im IFS enthalten sind.
		 */
		while (*quote_end 
			|| (*word_end && (strchr (ifs, *word_end) == NULL)))
		{
			++word_end;
			++quote_end;
		}
		
		/* <word_start> zeigt nun auf den Anfang eines Wortes,
		 * <word_end> auf dessen Ende. Analog ist es mit den
		 * Quote-Informationen. Wenn die Pointer ungleich sind,
		 * ist wirklich ein Wort vorhanden. Diese wird dann
		 * expandiert. Wenn die Pointer gleich sind, kann
		 * eigentlich nur ein 0 Zeichen vorliegen. Wenn dies
		 * gequoted ist, wurde "" oder '' eingegeben und wir
		 * mssen es speichern.
		 */
		if ((word_start != word_end) || *quote_start)
		{
			char save_char = *word_end;
			
			*word_end = '\0';
			if (!ExpandAndStore (M, A, word_start, quote_start))
				return FALSE;

			*word_end = save_char;
			word_start = word_end;
			quote_start = quote_end;
		}
	}
	
	return TRUE;
}


int MakeAndExpandArgv (MGLOBAL *M, WORDINFO *name_list, ARGVINFO *A)
{
	int broken = FALSE;
	
	memset (A, 0, sizeof (ARGVINFO));
	A->alloc = &M->alloc;
	
	while (!broken && name_list)
	{
		char *subst_string, *subst_quoted;
		
		if (!SubstWithCommand (M, name_list, &subst_string, 
			&subst_quoted))
		{
			broken = TRUE;
			break;
		}

		if (!splitExpandAndStore (M, A, subst_string, subst_quoted))
			broken = TRUE;
		
		mfree (&M->alloc, subst_string);
		mfree (&M->alloc, subst_quoted);
		name_list = name_list->next;
	}
	
	if (broken)
	{
		FreeArgv (A);
		return FALSE;
	}
	
	return TRUE;
}

void FreeExpArgv (ARGVINFO *A)
{
	FreeArgv (A);
}