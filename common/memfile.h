/*
 * @(#) Mupfel\memfile.h
 * @(#) Stefan Eissing, 14. Februar 1993
 *
 * jr 21.4.95
 */

#if  !defined( __M_MEMFILE__ )
#define __M_MEMFILE__

#include "..\common\alloc.h"


typedef struct
{
	char *bufStart;			/* Start of Buffer */
	char *bufEnd;			/* End of Buffer */
	char *curP;				/* current pointer */
	
	long bufSize;
	int file_handle;
	char dirty;
} MEMFILEINFO;

MEMFILEINFO *mopen (ALLOCINFO *A, const char *name, long *gemdoserr);
MEMFILEINFO *mcreate (ALLOCINFO *alloc, const char *filename);

long mflush (MEMFILEINFO *mp);
void mclose (ALLOCINFO *A, MEMFILEINFO *mp);

char *mgets (char *str, size_t n, MEMFILEINFO *mp);

long mwrite (MEMFILEINFO *mp, long len, const char *buf);
long mputs (MEMFILEINFO *mp, const char *string);

#endif	/* __M_MEMFILE__ */