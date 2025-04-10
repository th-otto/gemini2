/*
	@(#)Mupfel/puninfo.c
	@(#)Julian F. Reschke, 1. Mai 1995
		
	get pointer to harddisk PUN_INFO structure, if available
	
	jr 23.10.95
*/
 
#include <tos.h>

#include <common/cookie.h>

#include "puninfo.h"

#define PUNID      0x0F
#define PUNRES     0x70
#define PUNUNKNOWN 0x80

/* get pointer to PUN_INFO-structure, if available */

static long getpuninfo (void)
{
	return (long)(*((PUN_INFO **)(0x516L)));
}

PUN_INFO *PunThere (void)
{
	PUN_INFO *P;

	if (CookiePresent ('SVAR', 0)) return 0;

	P = (PUN_INFO *)Supexec (getpuninfo);

	if (P)	/* berhaupt gesetzt? */
		if (P->P_cookie == 0x41484449L)	/* Cookie gesetzt? */
			if (P->P_cookptr == &(P->P_cookie))
				if (P->P_version >= 0x300)
					return P;					
	return 0L;
}

