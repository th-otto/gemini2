/*
	@(#)Mupfel/complete.h
	@(#)Julian F. Reschke & Stefan Eissing, 17. Januar 1993
	
	Filename completion
*/


#ifndef __M_COMPLETE__
#define __M_COMPLETE__

#include "..\common\argvinfo.h"

char *CompleteWord (MGLOBAL *M, char *line, int linepos);

int GetCompletions (MGLOBAL *M, char *line, int linepos, ARGVINFO *A);

#endif /* __M_COMPLETE__ */