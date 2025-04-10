/*
 * @(#) MParse\Wildmat.c
 * @(#) Stefan Eissing & Gereon Steffens, 31. januar 1993
 *
 *  -  wildcard matching
 *  -  ! zusÑtzlich zu ^ zur Verneinung von [...] eingefÅhrt
 *
 *  -  Beschleunigung fÅr *irgendwas eingefÅhrt.
 */
 
/*
**  Do shell-style pattern matching for ?, \, [], and * characters.
**  Might not be robust in face of malformed patterns; e.g., "foo[a-"
**  could cause a segmentation violation.
**
**  Written by Rich $alz, mirror!rs, Wed Nov 26 19:03:17 EST 1986.
*/

#include <stddef.h>
#include <string.h>

#include "wildmat.h"

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 256
#endif

#ifndef TRUE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

int matchpattern (const char *text, const char *pattern);

static int star (const char *s, const char *p)
{
	if (strchr ("%?*[", *p) == NULL)
	{
		do
		{
			while (*s && *s != *p)
				++s;
			if (!*s)
				return FALSE;

			if (matchpattern (s, p))
				return TRUE;
		} 
		while (*++s != '\0');
		
		return FALSE;		
	}
	else
	{
		while (!matchpattern (s, p))
			if (*++s == '\0')
				return FALSE;
	}
	
	return TRUE;
}

static int matchpattern (const char *s, const char *p)
{
	int last, matched, reverse;

	for ( ; *p; s++, p++)
	switch (*p)
	{
		case '%':
			/* Literal match following character; fall through. */
			p++;

		default:
			if (*s != *p)
		    		return FALSE;
			continue;

		case '?':
			/* Match anything. */
			if (*s == '\0')
				return FALSE;
			continue;

		case '*':
			/* Trailing star matches everything. */
			if (*s == '\0')
			{
				if (*(p+1) == '\0')
					return TRUE;
				else
					return FALSE;
			}
			else
				return (*++p ? star(s, p) : TRUE);

		case '[':
			/* [^....] means inverse character class. */
			reverse = (p[1] == '^') || (p[1] == '!');
			if (reverse)
				++p;

			for (last = 0400, matched = FALSE; *++p && *p != ']'; last = *p)
			{
				/* This next line requires a good C compiler. */
				if (*p == '-' ? *s <= *++p && *s >= last : *s == *p)
					matched = TRUE;
			}
			if (matched == reverse)
				return FALSE;
			continue;
	}
	
	return *s == '\0';
}

int wildmatch (const char *text, const char *pattern, 
	int case_sensitiv)
{
	int complement = FALSE;
	
	if ((*pattern == '^' || *pattern == '!')
		&& strpbrk (pattern, "*?["))
	{
		complement = TRUE;
		++pattern;
	}
	
	if (!case_sensitiv)
	{
		char t[MAX_PATH_LEN];
		char p[MAX_PATH_LEN];

		if (strlen (text) < MAX_PATH_LEN 
			&& strlen (pattern) < MAX_PATH_LEN)
		{
			strcpy (t, text);
			strcpy (p, pattern);
			strupr (t);
			strupr (p);
			return complement? !matchpattern (t, p)
				: matchpattern (t, p);
		}
	}
	
	return complement? !matchpattern (text, pattern)
		: matchpattern (text, pattern);
}
