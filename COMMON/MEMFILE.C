/*
 * @(#) Mupfel\memfile.c
 * @(#) Stefan Eissing, 17. September 1991
 *
 * description: functions for textfiles in memory
 *
 * jr 23.4.95
 */

#include <stddef.h>
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "strerror.h"

#include "memfile.h"


MEMFILEINFO *mopen (ALLOCINFO *A, const char *name, long *gemdoserr)
{
	MEMFILEINFO *mp;
	size_t fsize, bufsize;
	long fhandle, amount;
	char *cp;
	
	fhandle = Fopen (name, 0);
	if (fhandle < 0) {
		if (gemdoserr) *gemdoserr = fhandle;
		return NULL;
	}
	
	fsize = Fseek (0L, (int) fhandle, 2);
	Fseek (0L, (int) fhandle, 0);

	/* xxx: was ist, wenn Fseek einen Fehler liefert? */
	
	bufsize = sizeof (MEMFILEINFO) + fsize;
	mp = mcalloc (A, 1, bufsize);
	
	if (!mp) {
		Fclose ((int) fhandle);
		if (gemdoserr) *gemdoserr = ENSMEM;
		return NULL;
	}
	
	cp = (char *)mp + sizeof (MEMFILEINFO);
	mp->curP = mp->bufStart = cp;
	mp->bufEnd = cp + fsize;
	mp->bufSize = fsize;

	amount = Fread ((int) fhandle, fsize, cp);
	
	if (amount != fsize) {
		mfree (A, mp);
		Fclose ((int) fhandle);
		if (gemdoserr) *gemdoserr = amount < 0 ? amount : ERROR;
		return NULL;
	}
	
	Fclose ((int) fhandle);
	return mp;
}


long mflush (MEMFILEINFO *mp)
{
	long stat = 0L;
	
	if (mp->dirty && (mp->file_handle > 0) 
		&& (mp->curP != mp->bufStart))
	{
		long size = mp->curP - mp->bufStart;
		
		mp->curP = mp->bufStart;
		mp->dirty = 0;
		
		if (size != Fwrite (mp->file_handle, size, mp->bufStart))
			stat = EWRITF;
	}
	
	return stat;
}


void mclose (ALLOCINFO *A, MEMFILEINFO *mp)
{
	if (mp->file_handle > 0)
	{
		mflush (mp);
		Fclose (mp->file_handle);
	}
		
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


#define MBUF_SIZE	4096L

MEMFILEINFO *mcreate (ALLOCINFO *alloc, const char *filename)
{
	MEMFILEINFO *mp;
	
	mp = mcalloc (alloc, 1, sizeof (MEMFILEINFO) + MBUF_SIZE);
	if (mp == NULL)
		return NULL;
	
	mp->file_handle = (int)Fcreate (filename, 0);
	if (mp->file_handle <= 0L)
	{
		mfree (alloc, mp);
		return NULL;
	}
	
	mp->bufSize = MBUF_SIZE;
	mp->bufStart = (char *)mp + sizeof (MEMFILEINFO);
	mp->curP = mp->bufStart;
	mp->bufEnd = mp->bufStart + mp->bufSize;
	
	return mp;
}


static long mPutByte (MEMFILEINFO *mp, char c)
{
	*(mp->curP++) = c;
	mp->dirty = 1;

	if (mp->curP == mp->bufEnd)
		return mflush (mp);
	
	return 0L;
}

long mwrite (MEMFILEINFO *mp, long len, const char *buf)
{
	long stat = 0L;
	
	while (len-- && !stat)
		stat = mPutByte (mp, *(buf++));
	
	return stat;
}

long mputs (MEMFILEINFO *mp, const char *string)
{
	long stat = 0L;
	
	if (string && *string)
		stat = mwrite (mp, strlen (string), string);
	
	if (!stat)
		stat = mwrite (mp, 2L, "\r\n");
	
	return stat;
}