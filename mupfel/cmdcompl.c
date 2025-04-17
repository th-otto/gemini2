/*
 * @(#) MParse\cmdcompl.c
 * @(#) Stefan Eissing, 24. Februar 1993
 *
 * Command Completion
 */


#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "..\common\alloc.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "mglob.h"

#include "alias.h"
#include "chario.h"
#include "cmdcompl.h"
#include "internal.h"
#include "locate.h"
#include "mdef.h"
#include "mlex.h"
#include "nameutil.h"

static int
cmpStringPointer (char **s1, char **s2)
{
	return strcmp (*s1, *s2);
}

int
GetCommandCompletions (MGLOBAL *M, ARGVINFO *A, char *what)
{
	if (!MakeGenericArgv (&M->alloc, A, TRUE, GetReservedCompletions, what))
		return FALSE;
	
	if (!GetAliasCompletions (M, A, what)
		|| !GetFunctionCompletions (M, A, what)
		|| !GetInternalCompletions (M, A, what)
		|| !GetExternalCompletions (M, A, what))
	{
		FreeArgv (A);
		return FALSE;
	}
	
	if (A->argc > 1)
		qsort (A->argv, A->argc, sizeof (char *), cmpStringPointer);

	return TRUE;
}