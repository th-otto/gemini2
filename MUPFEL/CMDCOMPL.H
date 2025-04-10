/*
 * @(#) MParse\cmdcompl.h
 * @(#) Stefan Eissing, 17. Januar 1993
 *
 * Command Completion
 */


#ifndef __M_CMDCOMPL__
#define __M_CMDCOMPL__

#include "..\common\argvinfo.h"

int GetCommandCompletions (MGLOBAL *M, ARGVINFO *A, char *what);

#endif /* __M_CMDCOMPL__ */