/*
 * @(#) Mupfel\Internal.h
 * @(#) Stefan Eissing, 17. Januar 1993
 *
 * Verwaltung interner Kommandos
 *
 */


#ifndef __internal__
#define __internal__

#include "..\common\argvinfo.h"

int InitInternal (MGLOBAL *M);
int ReInitInternal (MGLOBAL *M, MGLOBAL *new);
void ExitInternal (MGLOBAL *M);

int IsInternalCommand (MGLOBAL *M, const char *name, int all, 
	int *number);

int ExecInternalCommand (MGLOBAL *M, int number, int argc, 
	const char **argv);

int GetInternalCompletions (MGLOBAL *M, ARGVINFO *A, void *entry);

#endif /* __internal__ */
