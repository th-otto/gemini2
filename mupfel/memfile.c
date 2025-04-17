/*
 * @(#) Mupfel\memfile.c
 * @(#) Stefan Eissing, 17. September 1991
 *
 * description: functions for textfiles in memory
 */

#include <stddef.h>
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "memfile.h"


MEMFILEINFO *mopen (ALLOCINFO *A, const char *name)
{
	MEMFILEINFO *mp;
	size_t fsize, bufsize;
	int fhandle;
	char *cp;
	
	if ((fhandle = Fopen (name, 0)) > 0)
	{
		fsize = Fseek (0L, fhandle, 2);	/* seek to end of file */
		Fseek (0L, fhandle, 0);			/* and rewind */

		bufsize = sizeof (MEMFILEINFO) + (fsize);
		if((mp = mmalloc (A, bufsize)) != NULL)
		{
			cp = (char *)mp + sizeof (MEMFILEINFO);
			mp->curP = mp->bufStart = cp;
			mp->bufEnd = cp + fsize - 1;

			if (Fread (fhandle, fsize, cp) == fsize)
			{
				Fclose (fhandle);
				return mp;
			}
			else
			{
				mfree (A, mp);	/* free allocated memory */
			}
		}
		
		Fclose (fhandle);	/* come here only on failure! */
	}

	return NULL;
}

void mclose (ALLOCINFO *A, MEMFILEINFO *mp)
{
	mfree (A, mp);
}

char *mgets (char *str, size_t n, MEMFILEINFO *mp)
{
	long most;
	char c, *cp;
	
	most = mp->bufEnd - mp->curP;

	if (most > 0)				/* still something to read */
	{
		if (most < n)
			n = most;
			
		cp = str;
		while (n > 0)
		{
			*cp++ = c = *mp->curP++;	/* copy char */
			if ((strchr ("\n\r", c) != NULL) || c == '\0')
			{
				if (c == '\r')
				{
					cp[-1] = '\n';
					if (*mp->curP == '\n')
					{
						mp->curP++;		/* Atari ST cr+lf match */
						--n;
					}
				}

				break;
			}
			--n;
		}
		
		*cp = '\0';			/* always give a terminator */

		return str;
	}
	else
		return NULL;
}