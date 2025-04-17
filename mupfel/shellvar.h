/*
 * @(#) MParse\ShellVar.h
 * @(#) Stefan Eissing, 11. Mai 1991
 */



#ifndef __shellvar__
#define __shellvar__


int InstallShellVars (MGLOBAL *M, const char **args, int arg_count, 
		int allocate, int replace);
int ReInitShellVars (MGLOBAL *old, MGLOBAL *new);
void DeinstallShellVars (MGLOBAL *M);

void LinkShellVars (MGLOBAL *M, const char ***pargv, int *pargc);

const char *GetShellVar (MGLOBAL *M, int which);
int GetShellVarCount (MGLOBAL *M);
int ShiftShellVars (MGLOBAL *M, int how_many);

#define OPTION_LEN 80
void GetOptionString (MGLOBAL *M, char buffer[OPTION_LEN]);


#endif /* __shellvar__ */
