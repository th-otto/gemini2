/*
 * @(#) Common\genargv.h
 * @(#) Stefan Eissing, 12. November 1994
 */



#ifndef Genargv__
#define Genargv__

#include "..\common\argvinfo.h"

int StoreInArgv (ARGVINFO *A, const char *str);
void FreeArgv (ARGVINFO *A);
void InitArgv (ARGVINFO *A);

typedef int GenerateArgvFunc (ARGVINFO *A, void *entry);

int MakeGenericArgv (ALLOCINFO *alloc, ARGVINFO *A, int no_doubles,
	GenerateArgvFunc *func,	void *func_entry);

int String2Argv (ALLOCINFO *alloc, ARGVINFO *A, const char *string,
	int removeQuotes);

char *Argv2String (ALLOCINFO *alloc, ARGVINFO *A, int quoting);

#endif