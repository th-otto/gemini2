/*
 * @(#) MParse\MakeArgv.h
 * @(#) Stefan Eissing, 17. Januar 1993
 */



#ifndef __makeargv__
#define __makeargv__

#include "..\common\argvinfo.h"
#include "partypes.h"


void FreeExpArgv (ARGVINFO *A);

int StoreInExpArgv (ARGVINFO *A, const char *str);

int MakeAndExpandArgv (MGLOBAL *M, WORDINFO *name_list, ARGVINFO *A);

#endif /* __makeargv__ */
