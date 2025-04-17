/*
 * @(#) Mupfel\nameutil.h
 * @(#) Stefan Eissing, 17. Januar 1993
 *
 * jr 8.8.94
 */


#ifndef __M_NAMEUTIL__
#define __M_NAMEUTIL__

#include "..\common\argvinfo.h"

int SetWordList (MGLOBAL *M, WORDINFO *name);

int NameExists (MGLOBAL *M, const char *name);

int PutEnv (MGLOBAL *M, const char *name);
char *GetEnv (MGLOBAL *M, const char *name);

const char *GetVarValue (MGLOBAL *M, const char *name);
long GetVarInt (MGLOBAL *M, const char *name);

int GetNameCompletions (MGLOBAL *M, ARGVINFO *A, char *what);
int GetFunctionCompletions (MGLOBAL *M, ARGVINFO *A, char *what);

void PrintAllNames (MGLOBAL *M);

int m_export (MGLOBAL *M, int argc, char **argv);
int m_readonly (MGLOBAL *M, int argc, char **argv);
int m_unset (MGLOBAL *M, int argc, char **argv);
int m_setenv (MGLOBAL *M, int argc, char **argv);

#endif /* __M_NAMEUTIL__ */
