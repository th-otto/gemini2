/*
 * @(#) MParse\shell.h
 * @(#) Stefan Eissing, 25. April 1992
 *
 * Header fÅr system-Aufrufe via shell_p
 */


#ifndef __M_SHELL__
#define __M_SHELL__

#include "..\common\xbra.h"


extern XBRA	SystemXBRA;

void SetStack (void *newsp);

int cdecl SystemHandler (const char *command);

void InitReset (void);
void ExitReset (void);

#endif	/* __M_SHELL__ */