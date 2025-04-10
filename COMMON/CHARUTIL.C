/*
 * @(#) Common\CharUtil.c
 * @(#) Stefan Eissing, 28. Mai 1995
 *
 * jr 20.5.95
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "alloc.h"
#include "charutil.h"

#ifndef TRUE
#define FALSE	0
#define TRUE	(!FALSE)
#endif
 
int IsLetter (int c)
{
	return (c && ((strchr ("_", c) != NULL)
				|| isalpha (c)
				|| (c >= 0x80)
				|| (c < 0)));
}

int IsValidDollarChar (int c)
{
	return (c && ((isdigit (c) 
		|| (strchr ("!$?*@#{(-", c) != NULL) 
		|| IsLetter (c)))); 
}

int IsEnvChar (int c)
{
	return (IsLetter (c) 
		|| isdigit (c) 
		|| (strchr ("+-", c) != NULL));
}

char *StrDup (ALLOCINFO *A, const char *str)
{
	char *s;

#if 0
if (strlen (str) == 1)
	puts (str);
#endif
	
	s = mmalloc (A, strlen (str) + 1L);
	if (!s) return NULL;
		
	return strcpy (s, str);
}

/* return last character in string */
char lastchr (const char *str)
{
	size_t len = strlen (str);
	
	return len ? str[len - 1] : '\0';
}

/* append character c to str */
void chrcat (char *str, char c)
{
	size_t l = strlen (str);
	
	str[l++] = c;
	str[l] = '\0';
}


int ConvertSlashes (char *string, const char *quoted)
{
	int ret = 0;
	
	while (*string)
	{
		if ((*string == '/') && (!quoted || !*quoted))
		{
			*string = '\\';
			ret = 1;
		}
		++string;
		if (quoted)
			++quoted;
	}
	
	return ret;
}

int ContainsWildcard (const char *string, const char *quoted)
{
	int got_open_bracket = 0;
	
	while (*string)
	{
		switch (*string)
		{
			case '*':
			case '?':
				if (!*quoted)
					return TRUE;
				break;
			
			case '[':
				got_open_bracket = !*quoted;
				break;
			
			case ']':
				if (!*quoted && got_open_bracket)
					return TRUE;
				break;
		}
		++string;
		++quoted;
	}
	
	return FALSE;
}


int ContainsGEMDOSWildcard (const char *string)
{
	return (strchr (string, '*') || strchr (string, '?'));
}


void RemoveDoubleBackslash (char *str)
{
	char *write;
	
	/* str durchl„uft den ganzen String und ist die Leseposition.
	 * write ist die letzte Schreibposition. Wenn beide auf einen
	 * Backslash zeigen, wird nichts kopiert und nur str erh”ht sich.
	 */
	write = str++;
	while (*write)
	{
		if ((*str != '\\') || (*write != '\\'))
		{
			++write;
			*write = *str;
		}
		++str;
	}
}

void RemoveAppendingBackslash (char *str)
{
	size_t len = strlen (str);
	
	if (len && --len > 2 && str[len] == '\\')
		str[len] = '\0';
}


char *StrToken (char *string, const char *white, char **save)
{
	char *found;
	
	if (string == NULL)
		string = *save;
	else
		*save = NULL;
	
	if (string == NULL)
		return NULL;
		
	while (*string && strchr (white, *string))
		++string;
	
	found = string;
	
	while (*string && !strchr (white, *string))
		++string;
	
	if (*string)
	{
		*save = string + 1;
		*string = '\0';

		return found;
	}
	else
	{
		*save = NULL;

		return (string == found)? NULL : found;
	}
}