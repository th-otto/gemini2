/*
 *	@(#) Mupfel/read.c
 *	@(#) Stefan Eissing, 16. September 1991
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "mglob.h"

#include "chario.h"
#include "lineedit.h"
#include "names.h"
#include "read.h"

/* internal texts
 */
#define NlsLocalSection "M.read"
enum NlsLocalText{
OPT_read,	/*[ name ... ]*/
HLP_read,	/*Werte fr name einlesen*/
};


int m_read (MGLOBAL *M, int argc, char **argv)
{
	char *line, *name, *val, *lp;
	const char *ifs;
	int index, broken = FALSE;
	int done;
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_read), 
			NlsStr(HLP_read));
	
	done = FALSE;
	line = NULL;
	while (!done)
	{
		char *tmp;
		size_t len;
		
		tmp = ReadLine (M, FALSE, NULL, (PromptFunction *)NULL, 
			&broken);
	
		if (broken)
		{
			if (line)
				mfree (&M->alloc, line);
			if (tmp)
				mfree (&M->alloc, tmp);
			return 1;
		}
		
		len = strlen (tmp);
		done = !(len >= 2 && tmp[len-1] == '\n' 
				&& tmp[len-2] == M->tty.escape_char);
		
		if (done)
		{
			if (tmp[len-1] == '\n')
			{
				tmp[len-1] = '\0';
				--len;
			}
		}
		else
		{
			tmp[len-2] = '\0';
			len -= 2;
		}
		
		if (line)
		{
			size_t old_len;
			char *cp;
			
			old_len = strlen (line);
				
			cp = mmalloc (&M->alloc, old_len + len + 1);
			if (!cp)
			{
				mfree (&M->alloc, line);
				mfree (&M->alloc, tmp);
				return 1;
			}
			
			strcpy (cp, line);
			strcat (cp, tmp);
			
			mfree (&M->alloc, tmp);
			mfree (&M->alloc, line);
			line = cp;
		}
		else
			line = tmp;
	}

	if (!line)
		return 1;
	
	ifs = GetIFSValue (M);
	if (argc < 2)
	{
		name = "REPLY";
		index = argc;
	}
	else
	{
		name = argv[1];
		index = 2;
	}

	val = lp = line;
	while (index < argc)
	{
		while (*val && strchr (ifs, *val))
			++val;
		lp = val;
		while (*lp && (strchr (ifs, *lp) == NULL))
			++lp;
		
		if (*lp)
		{
			*lp++ = '\0';
		}
			
		if (!InstallName (M, name, val? val : ""))
			break;
		
		val = lp;
		name = argv[index++];
	}

	if (index >= argc)
		InstallName (M, name, val? val : "");
	
	mfree (&M->alloc, line);
	return 0;
}

