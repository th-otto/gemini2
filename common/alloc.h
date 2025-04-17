/*
 * @(#) Mupfel\alloc.h
 * @(#) by Gereon Steffens & Stefan Eissing, 27. Januar 1993
 *
 * - definitions for alloc.c
 *
 */

#ifndef __alloc__
#define __alloc__
 
#include <stdlib.h>

#define SYSSTACK	250		/* size of sysmalloc array */

#define ALLOCCHECK	 0		/* Include debugging code */
/*#define MEMGUARD	"MEMG"	*/ /* Use memory guards */

#if ALLOCCHECK
#define MAXALLOC	1024

typedef struct
{
	void *addr;
 	size_t len;
} CHECKINFO;
#endif


typedef struct header
{
	struct header *ptr;
	size_t size;
#ifdef MEMGUARD
	size_t origsize;
	const char *filename;
	long line;
#endif
} Header;


typedef struct
{
	Header base;
	Header *freep;

	Header *sysmalloc[SYSSTACK];
	size_t syssize[SYSSTACK];

	short memory_mode;
		
#if ALLOCCHECK
	CHECKINFO alloctab[MAXALLOC];
#endif
} ALLOCINFO;

#ifdef MEMGUARD

/* allocate memory */
void *_mmalloc (ALLOCINFO *A, size_t size, const char *f, long l);
#define mmalloc(a,b)	_mmalloc((a),(b),__FILE__,__LINE__)

/* alloc nitems*size */
void *_mcalloc (ALLOCINFO *A, size_t nitems, size_t size, const char *f, long l);
#define mcalloc(a,b,c)	_mcalloc((a),(b),(c),__FILE__,__LINE__)

/* chg size of block */
void *_mrealloc (ALLOCINFO *A, void *block, size_t newsize,  const char *f, long l);
#define mrealloc(a,b,c)	_mrealloc((a),(b),(c),__FILE__,__LINE__)

/* free memory */
void _mfree (ALLOCINFO *A, void *mem, const char *f, long l);
#define mfree(a,b)	_mfree((a),(b),__FILE__,__LINE__)

#else

/* allocate memory */
void *mmalloc (ALLOCINFO *A, size_t size);

/* alloc nitems*size */
void *mcalloc (ALLOCINFO *A, size_t nitems, size_t size);

/* chg size of block */
void *mrealloc (ALLOCINFO *A, void *block, size_t newsize);

/* free memory */
void mfree (ALLOCINFO *A, void *mem);

#endif

/* check for unfreed blocks */
int alloccheck (ALLOCINFO *A);



/* Defines fÅr angeforderte Speicherart
 */
#define MAL_ONLYST	0
#define MAL_ONLYALT	1
#define MAL_PREFST	2
#define MAL_PREFALT	3

/* Defines fÅr den Speicherschutz
 */
#define PROT_DEF	0x00
#define PROT_PRIV	0x10
#define PROT_GLOB	0x20
#define PROT_SUPER	0x30
#define PROT_READ	0x40

/* initialize alloc table
 */
void allocinit (ALLOCINFO *A, short memory_modes);

/* Mfree allocated memory
 */
void allocexit(ALLOCINFO *A);

#endif /* __alloc__ */