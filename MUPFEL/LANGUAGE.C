/*
	@(#)mupfel/language.c
	
	Julian F. Reschke, 29. Mai 1996
	Support functions for locales
*/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>

#include "mupfel.h"
#include "mglob.h"

#include "language.h"

#include <common/cookie.h>

static int
default_locale (void)
{
	switch (DefaultLanguage (TOSCountry))
	{
		case 1:
		case 8:		return DO_LOCALE_GERMAN;
		
		case 2:
		case 7:		return DO_LOCALE_FRENCH;
		
		default:	return DO_LOCALE_POSIX;
	}
}

int
GetLocale (MGLOBAL *M, const char *type)
{
	char *found;

	found = GetEnv (M, "LC_ALL");
	if (!found) found = GetEnv (M, type);
	if (!found) found = GetEnv (M, "LANG");
	
	if (!found) return default_locale ();
	
	if (!stricmp (found, "C")) return DO_LOCALE_POSIX;
	if (!stricmp (found, "POSIX")) return DO_LOCALE_POSIX;
	if (!stricmp (found, "GERMAN")) return DO_LOCALE_GERMAN;
	if (!stricmp (found, "FRENCH")) return DO_LOCALE_FRENCH;
	if (!stricmp (found, "ENGLISH")) return DO_LOCALE_ENGLISH;
	if (!stricmp (found, "FEDERATION")) return DO_LOCALE_FED;
	
	/* Spez. lt. ISO */
	
	if ((strlen (found) >= 5) &&
		isupper (found[3]) &&
		isupper (found[4]) &&
		found[2] == '_' &&
		found[0] == 'd' &&
		found[1] == 'e')
		return DO_LOCALE_GERMAN;
	
	return default_locale ();
}