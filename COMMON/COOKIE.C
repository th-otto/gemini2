/*
 * @(#) Common\Cookie.c
 * @(#) Stefan Eissing, 08. Februar 1994
 *
 * jr 22.04.95
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <tos.h>

#include "cookie.h"
#include "strerror.h"

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#define _P_COOKIES		((long **)0x5a0L)
#define Ssystem(a,b,c)	gemdos(0x154,(int)a,(long)b,(long)c)	


int LegalCookie (void *cj)
{
	return cj != NULL && ((size_t)cj % 2) == 0 && (long)cj > 0;
}


static long
cookieptr (void)
{
	return (long)(*_P_COOKIES);
}

int
CookiePresent (long cookie, long *value)
{
	long *cookiejar = (long *) Ssystem (10, 0x5a0, NULL);
	
	if (cookiejar == (long *) EINVFN)
		cookiejar = (long *) Supexec (cookieptr);
	
	if (!LegalCookie (cookiejar))
		return FALSE;

	while (*cookiejar != 0L)
	{
		if (*cookiejar == cookie)
		{
			if (value != NULL)
				*value = cookiejar[1];
			return TRUE;
		}
			
		cookiejar += 2;
	}
	
	return FALSE;
}


static jmp_buf mem_violation;

void mv_handler (void)
{
	Psigreturn ();
	longjmp (mem_violation, 1);
}


int MemoryIsReadable (void *mem)
{
	void *old_handler;
	int is_readable;
	
	if (mem == NULL)
		return TRUE;
		
	old_handler = Psignal (10, mv_handler);
	if ((long)old_handler == EINVFN)
		return TRUE;
	
	if (setjmp (mem_violation) != 0)
		is_readable = FALSE;
	else
	{
		is_readable = ((char *)mem)[0];
		
		is_readable = TRUE;
	}
	Psignal (10, old_handler);
	
	return is_readable;
}

/* jr: Routine, die sprintf des Datums im Sinne des _IDT-Cookies
   simuliert */
   
int IDTSprintf (char *str, int year, int month, int day)
{
	static long _idt_cookie;
	static int have_it = 0;
	int delimiter;
	
	year %= 100;

	if (!have_it)
	{
		if (!CookiePresent ('_IDT', &_idt_cookie))
			_idt_cookie = 0x1100 | '.';
/*			_idt_cookie = '/';	ENGLISH */
		have_it = 1;
	}

	delimiter = (int) _idt_cookie & 0xff;
	if (!delimiter) delimiter = '/';	/* default */
	
	switch ((int)(_idt_cookie & 0x0f00) >> 8)
	{
		case 3:	return sprintf (str, "%02d%c%02d%c%02d",
							year, delimiter, day, delimiter, month);
							
		case 2:	return sprintf (str, "%02d%c%02d%c%02d",
							year, delimiter, month, delimiter, day);
							
		case 1:	return sprintf (str, "%02d%c%02d%c%02d",
							day, delimiter, month, delimiter, year);
							
		default: return sprintf (str, "%02d%c%02d%c%02d",
							month, delimiter, day, delimiter, year);
	}
}

/* return default language */

int
DefaultLanguage (int def)
{
	long akp_val;
	
	if (CookiePresent ('_AKP', &akp_val))
		return (int) ((akp_val & 0xff00) >> 8);
	else
		return def;
}
