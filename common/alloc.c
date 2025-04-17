/*
 * malloc.c  -  K&R memory allocation with usage checks
 *
 * @(#) Mupfel\alloc.h
 * @(#) Gereon Steffen & Stefan Eissing, 2. Dezember 1992
 *
 * jr 96/06/23 added mem guards (#define MEMGUARD)
 */
 
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "cookie.h"

#define ALLOCABORT	 0		/* Terminate if malloc fails */
#define SHOWMORECORE 0		/* Show morecore() calls */
#define SHOWSYSFREE  0		/* Show Mfree() calls */

#define NALLOC		1024	/* NALLOC * sizeof(Header) => 
							 * smallest arg for Malloc()
							 */
							
#define DebugMsg	printf
#ifndef FALSE
#define FALSE	(0)
#define TRUE	(!FALSE)
#endif

#ifdef MEMGUARD
#define	__mmalloc(a,b)	_mmalloc((a),(b),filename,ln)
#else
#define	__mmalloc(a,b)	mmalloc((a),(b))
#endif



static void *
#ifdef MEMGUARD
mymalloc (ALLOCINFO *A, size_t nbytes, const char *filename, long ln)
#else
mymalloc (ALLOCINFO *A, size_t nbytes)
#endif
{
	Header *p, *prevp;
	Header *morecore (ALLOCINFO *, size_t);
	size_t nunits;

#ifdef MEMGUARD
	nbytes += sizeof (MEMGUARD);
#endif

	nunits = ((nbytes + sizeof(Header)-1) / sizeof(Header)) + 1;

	if ((prevp = A->freep) == NULL)
	{
		A->base.ptr = A->freep = prevp = &A->base;
		A->base.size = 0;
	}

	for (p = prevp->ptr; ; prevp = p, p = p->ptr)
	{
		if (p->size >= nunits)
		{
			if (p->size == nunits)
				prevp->ptr = p->ptr;
			else
			{
				p->size -= nunits;
				p += p->size;
				p->size = nunits;
			}
			A->freep = prevp;

#ifdef MEMGUARD
			p->origsize = nbytes - sizeof (MEMGUARD);
			p->filename = filename;
			p->line = ln;
			memcpy ((void *)((long)(p + 1) + nbytes - sizeof (MEMGUARD)),
				MEMGUARD, sizeof (MEMGUARD));
#endif
			return (void *)(p+1);
		}
		
		if (p == A->freep)
		{
			if ((p = morecore (A, nunits)) == NULL)
				return NULL;
		}
	}
}


static Header *
freeSystemBlock (ALLOCINFO *A, Header *act)
{
	int i;
	Header *last = act + act->size;
	Header *prev, *newprev, *newnext;
	
	newprev = newnext = NULL;

	/* Ermittle den freien Block, der vor act liegt.
	 */
	prev = A->freep;
	while (prev->ptr != act)
		prev = prev->ptr;
	
	for (i = 0; i < SYSSTACK; ++i)
	{
		if ((act <= A->sysmalloc[i]) && (last > A->sysmalloc[i]))
		{
			/* A->sysmalloc[i] liegt innerhalb von act und last.
			 * Wenn jetzt auch das Ende von A->sysmalloc[i]
			 * in den Grenzen liegt, kann der Block freigegeben
			 * werden.
			 */
			Header *syslast;
			
			syslast = A->sysmalloc[i] + A->syssize[i];
			
			if (syslast <= last)
			{
				/* Jep!
				 */
				if (act < A->sysmalloc[i])
				{
					size_t newsize;
					
					newprev = act;
					act = A->sysmalloc[i];
					act->ptr = newprev->ptr;
					newprev->ptr = act;
					newsize = act - newprev;
					act->size = newprev->size - newsize;
					newprev->size = newsize;
					
					prev = newprev;
				}
				
#if SHOWSYSFREE
{
	extern int dprintf (char *s, ...);
	
	dprintf ("sysFree(%ld)\n", A->syssize[i] * sizeof (Header));
}
#endif
				if (syslast < last)
				{
					newnext = syslast;
					newnext->size = last - syslast;
					act->size = A->syssize[i];
					newnext->ptr = act->ptr;
					act->ptr = newnext;
				}
				
				prev->ptr = act->ptr;
				A->sysmalloc[i] = NULL;
				Mfree (act);
				break;
			}
		}
	}
	
	if (newnext != NULL && newnext->size >= NALLOC)
	{
/*dprintf ("free upper?\n");
*/		freeSystemBlock (A, newnext);
	}
	
	if (newprev != NULL)
	{
/*dprintf ("free lower?\n");
*/		prev = A->freep;
		while ((prev->ptr != newprev) && (prev->ptr != A->freep))
			prev = prev->ptr;
	}

	return prev;
}


static void
myfree (ALLOCINFO *A, void *ap, int try_sys_free, int check_guard)
{
	Header *bp, *p;

	bp = (Header *)ap-1;

	for (p = A->freep; !(bp > p && bp < p->ptr); p = p->ptr)
		if (p >= p->ptr && (bp > p || bp < p->ptr))
			break;

#ifdef MEMGUARD
	/* Teste, ob der Guard noch intakt ist */
	if (check_guard && 
		memcmp ((void *)((long)(ap) + bp->origsize),
		MEMGUARD, sizeof (MEMGUARD)))
	{
		extern int dprintf (char *s, ...);
		
		dprintf ("memory block destroyed (at %p, length %ld, allocated from line %ld in\n"
			"%s)!\n", ap, bp->origsize, bp->line, bp->filename);
		Bconin (2);
	}
	
#endif

	/* Versuche einen Merge mit dem n„chsth”heren Block */
	
	if (bp + bp->size == p->ptr)
	{
		bp->size += p->ptr->size;
		bp->ptr = p->ptr->ptr;
	}
	else
		bp->ptr = p->ptr;

	/* Versuche einen Merge mit dem darunterliegenden Block */

	if (p + p->size == bp)
	{
		p->size += bp->size;
		p->ptr = bp->ptr;
		bp = p;
	}
	else
		p->ptr = bp;

	/* p zeigt auf den freien Block unterhalb von bp, oder falls der
	   gleich A->base ist, auf bp selbst. */
	   
	if (try_sys_free && (bp->size >= NALLOC))
	{
		p = freeSystemBlock (A, bp);
	}

	A->freep = p;
}


static Header *
morecore (ALLOCINFO *A, size_t nu)
{
	void *cp;
	Header *up;
	int i;

	nu = ((nu + NALLOC - 1) / NALLOC) * NALLOC;

	if (A->memory_mode != 0)
		cp = Mxalloc (nu * sizeof(Header), A->memory_mode);
	else
		cp = Malloc (nu * sizeof(Header));
		
	if (cp == NULL)
		return NULL;

#if SHOWMORECORE
	{
		extern int dprintf (char *s, ...);
		
		dprintf ("Called Malloc(%ld)\n", nu * sizeof (Header));
	}
#endif
	
	for (i = 0; i < SYSSTACK; ++i)
	{
		if (A->sysmalloc[i] == NULL)
		{
			A->sysmalloc[i] = cp;
			A->syssize[i] = nu;
			break;
		}
	}
	
	if (i == SYSSTACK)
		DebugMsg("Malloc-Stack overflow");
	
	up = (Header *)cp;
	up->size = nu;
	myfree (A, (void *)(up+1), FALSE, FALSE);

	return A->freep;
}

#if ALLOCCHECK
static void
enterm (ALLOCINFO *A, void *m, size_t len)
{
	int i;

	for (i = 0; i < MAXALLOC; ++i)
	{
		if (A->alloctab[i].addr == NULL)
		{
			A->alloctab[i].addr = m;
			A->alloctab[i].len = len;
			return;
		}
	}
	
	DebugMsg("malloc table overflow\n");
}

static int
removem (ALLOCINFO *A, void *m)
{
	int i;
	
	if (m == NULL)
	{
		DebugMsg ("freeing 0\n");
		return FALSE;
	}
	for (i = 0; i < MAXALLOC; ++i)
	{
		if (A->alloctab[i].addr == m)
		{
			A->alloctab[i].addr = NULL;
			A->alloctab[i].len = 0;
			return TRUE;
		}
	}
	
	DebugMsg ("freeing unallocated memory (%p)\n", m);
	return FALSE;
}
#endif

void *
#ifdef MEMGUARD
_mmalloc (ALLOCINFO *A, size_t size, const char *filename, long ln)
#else
mmalloc (ALLOCINFO *A, size_t size)
#endif
{
#ifdef MEMGUARD
	void *m = mymalloc (A, size, filename, ln);
#else
	void *m = mymalloc (A, size);
#endif

	if (m == NULL)
	{
#if ALLOCABORT
		fatal("out of memory, needed %lu bytes\n",size);
#endif
		return m;
	}
#if ALLOCCHECK
	enterm (A, m, size);
#endif
	return m;
}

void *
#ifdef MEMGUARD
_mcalloc(ALLOCINFO *A, size_t nitems, size_t size, const char *filename, long ln)
#else
mcalloc(ALLOCINFO *A, size_t nitems, size_t size)
#endif
{
	size_t tsize = nitems * size;
#ifdef MEMGUARD
	void *m = _mmalloc (A, tsize, filename, ln);
#else
	void *m = mmalloc (A, tsize);
#endif
	
	if (m != NULL) memset (m, 0, tsize);

	return m;
}

static size_t getsize(void *m)
{
	Header *mp = (((Header *)m)-1);
	
	return (mp->size-1) * sizeof(Header);
}

static size_t min(size_t x,size_t y)
{
	return x<y ? x : y;
}

void *
#ifdef MEMGUARD
_mrealloc (ALLOCINFO *A, void *block, size_t newsize, const char *filename, long ln)
#else
mrealloc (ALLOCINFO *A, void *block, size_t newsize)
#endif
{
	void *m;

	if (block == NULL)
		return __mmalloc (A, newsize);

	if ((m = __mmalloc(A, newsize)) == NULL) return NULL;

	memcpy (m, block, min (newsize, getsize (block)));
	mfree (A, block);
	return m;
}

void
#ifdef MEMGUARD
_mfree (ALLOCINFO *A, void *m, const char *filename, long ln)
#else
mfree (ALLOCINFO *A, void *m)
#endif
{
	if (m == NULL) return;
		
#if ALLOCCHECK
	if (removem(A, m))
#endif
		myfree(A, m, TRUE, TRUE);
}

int alloccheck(ALLOCINFO *A)
{
#if ALLOCCHECK
	int i, retcode = TRUE;
	
	for (i = 0; i < MAXALLOC; ++i)
	{
		if (A->alloctab[i].addr != NULL)
		{
			retcode = FALSE;
			DebugMsg("unfreed memory at %p, size=%ld contains %s\n",
				A->alloctab[i].addr,
				A->alloctab[i].len,
				A->alloctab[i].addr);
			DebugMsg ("Return...\n");
			Cconin ();
		}
	}
	
	return retcode;
#else
	(void) A;
	return TRUE;
#endif
}


extern unsigned long MiNTVersion;	/* jr: from mupfel.c */

void allocinit (ALLOCINFO *A, short memory_mode)
{
	memset (A, 0, sizeof (ALLOCINFO));
	
	if (MiNTVersion) A->memory_mode = memory_mode;
}

void allocexit (ALLOCINFO *A)
{
	int i;
	
	for (i = 0; i < SYSSTACK; ++i)
	{
		if (A->sysmalloc[i] != NULL)
		{
			if (Mfree (A->sysmalloc[i]))
				DebugMsg("Mfree() error\n");
		}
	}
}
