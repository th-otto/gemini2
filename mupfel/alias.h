/*
 * @(#) MParse\alias.h
 * @(#) Stefan Eissing, 24. Januar 1994
 *
 * Aliase fÅr Kommandos
 */


#ifndef __M_ALIAS__
#define __M_ALIAS__

#include "..\common\argvinfo.h"

void InitAlias (MGLOBAL *M);
int ReInitAlias (MGLOBAL *M, MGLOBAL *new);
void ExitAlias (MGLOBAL *M);

const char *GetAlias (MGLOBAL *M, const char *name, int first_search);

int m_alias (MGLOBAL *M, int argc, char **argv);
int m_unalias (MGLOBAL *M, int argc, char **argv);

int GetAliasCompletions (MGLOBAL *M, ARGVINFO *A, void *entry);


#endif	/* __M_ALIAS__ */