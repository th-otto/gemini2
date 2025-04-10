/*
 * @(#) MParse\optcompl.h
 * @(#) Stefan Eissing, 17. januar 1993
 *
 * Option Completion
 */


#ifndef M_OPTCOMPL__
#define M_OPTCOMPL__

#include "..\common\argvinfo.h"

char *GetCommandFromLine (MGLOBAL *M, const char *line,
	const char *scan_from);

int GetOptionCompletions (MGLOBAL *M, ARGVINFO *A, char *what,
	const char *command);

#endif /* M_OPTCOMPL__ */