/*
 * @(#) Mupfel\ShellVar.c
 * @(#) Stefan Eissing, 28. Mai 1992
 */


#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "mglob.h"

#include "chario.h"
#include "shellvar.h"



void GetOptionString (MGLOBAL *M, char buffer[OPTION_LEN])
{
	char *str = buffer;
	
	if (M->shell_flags.export_all)
		*(str++) = 'a';

	if (M->shell_flags.exec_argv)
		*(str++) = 'c';

	if (M->shell_flags.break_on_false)
		*(str++) = 'e';

	if (M->shell_flags.no_globber)
		*(str++) = 'f';

	if (M->shell_flags.hash_function_def)
		*(str++) = 'h';

	if (M->shell_flags.interactive)
		*(str++) = 'i';

	if (M->shell_flags.keywords_everywhere)
		*(str++) = 'k';

	if (M->shell_flags.local_block)
		*(str++) = 'l';

	if (M->shell_flags.no_execution)
		*(str++) = 'n';

	if (M->shell_flags.restricted)
		*(str++) = 'r';

	if (M->shell_flags.only_one_command)
		*(str++) = 't';

	if (M->shell_flags.no_empty_vars)
		*(str++) = 'u';

	if (M->shell_flags.verbose)
		*(str++) = 'v';

	if (M->shell_flags.print_as_exec)
		*(str++) = 'x';

	if (M->shell_flags.dont_clobber)
		*(str++) = 'C';

	if (M->shell_flags.debug_io)
		*(str++) = 'D';

	if (M->shell_flags.dont_use_gem)
		*(str++) = 'G';

	if (M->shell_flags.slash_conv)
		*(str++) = 'S';

	*str = '\0';
}

static void freeArrayContents (MGLOBAL *M, SHELLVAR *vars, 
	int first, int last)
{
	int i;
	
	if (vars->flags.was_malloced)
	{
		for (i = first; i <= last; ++i)
		{
			if (vars->argv[i])
				mfree (&M->alloc, vars->argv[i]);
		}
	}
}

static void freeShellVars (MGLOBAL *M, SHELLVAR *vars)
{
	if (vars->flags.was_malloced)
	{
		freeArrayContents (M, vars, 0, vars->argc - 1);
		if (vars->argv)
			mfree (&M->alloc, vars->argv);
	}
	mfree (&M->alloc, vars);
}

int InstallShellVars (MGLOBAL *M, const char **args, int arg_count, 
		int allocate, int replace)
{
	SHELLVAR *vars;
	int i = 0;
	int broken = FALSE;

	vars = mcalloc (&M->alloc, 1, sizeof (SHELLVAR));
	if (!vars)
		return FALSE;
		
	if (allocate)
	{
		if (arg_count < 1)
		{
			args = NULL;
		}
		else
		{
			const char **tmp;
			
			tmp = mcalloc (&M->alloc, arg_count, sizeof (char *));
			if (!tmp)
			{
				mfree (&M->alloc, vars);
				return FALSE;
			}
			
			for (i = 0; i < arg_count; ++i)
			{
				tmp[i] = StrDup (&M->alloc, args[i]);
				if (!tmp[i])
				{
					broken = TRUE;
					break;
				}
			}
			
			args = tmp;
		}
	}
	
	vars->argv = args;
	vars->argc = arg_count;
	vars->flags.was_malloced = allocate;

	if (allocate && broken)
	{
		freeShellVars (M, vars);
		return FALSE;
	}

	if (replace)
	{
		DeinstallShellVars (M);
	}

	vars->next = M->shell_vars;
	M->shell_vars = vars;
	
	return TRUE;
}

/* Kopiere ShellVars von <old> nach <new>. Kopiere nur den
 * obersten Level, da die tieferen gar nicht erreicht werden k”nnen.
 */
int ReInitShellVars (MGLOBAL *old, MGLOBAL *new)
{
	new->shell_vars = NULL;

	if (old->shell_vars)
	{
		new->shell_vars = NULL;
		
		return InstallShellVars (new, old->shell_vars->argv,
			old->shell_vars->argc, TRUE, TRUE);
	}
	else
		new->shell_vars = NULL;
	
	return TRUE;
}

void DeinstallShellVars (MGLOBAL *M)
{
	SHELLVAR *vars = M->shell_vars;

	if (M->shell_vars)
	{
		M->shell_vars = vars->next;
		freeShellVars (M, vars);
	}
}

const char *GetShellVar (MGLOBAL *M, int which)
{
	if (which == 0)
	{
		return M->program_name;
	}
	else if (!M->shell_vars || (which < 0) 
		|| (which > M->shell_vars->argc))
	{
		return NULL;
	}
	else
		return M->shell_vars->argv[which - 1];
}

int GetShellVarCount (MGLOBAL *M)
{
	if (!M->shell_vars)
		return 0;
		
	return M->shell_vars->argc;
}

/* Shifte die Shell-Variablen um <how_many> Stellen herunter.
 */
int ShiftShellVars (MGLOBAL *M, int how_many)
{
	int source;
	int last_arg;
	
	if (!M->shell_vars || (how_many <= 0))
		return TRUE;
	
	last_arg = M->shell_vars->argc - 1;
	
	/* Die erste Shell-Variable, die erhalten bleibt
	 */
	source = how_many;
	
	/* Wenn wir nicht genug Variablen haben, l”schen wir alle.
	 */
	if (source > last_arg)
	{
		freeArrayContents (M, M->shell_vars, 0, last_arg);
		M->shell_vars->argc = 0;

		return FALSE;
	}
	else
	{
		int target = 0;
		
		/* Gebe Speicher fr nicht mehr ben”tigte Elemente
		 * zurck.
		 */
		freeArrayContents (M, M->shell_vars, 0, source - 1);
		
		/* Kopiere nun die Argumente "herunter".
		 */
		while (source < M->shell_vars->argc)
		{
			M->shell_vars->argv[target++] = 
				M->shell_vars->argv[source++];
		}
		M->shell_vars->argc -= how_many;
		
		return TRUE;
	}
}

void LinkShellVars (MGLOBAL *M, const char ***pargv, int *pargc)
{
	if (M->shell_vars)
	{
		*pargv = M->shell_vars->argv;
		*pargc = M->shell_vars->argc;
	}
	else
		*pargc = 0;
}