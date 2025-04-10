/*
 * @(#) MParse\ParsUtil.c
 * @(#) Stefan Eissing, 10. April 1994
 *
 * jr 22.4.95
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"
#include "mglob.h"

#include "chario.h"
#include "exec.h"
#include "exectree.h"
#include "mdef.h"
#include "maketree.h"
#include "mparse.h"
#include "nameutil.h"
#include "parsinpt.h"
#include "parsutil.h"
#include "screen.h"
#include "shellvar.h"
#include "stand.h"
#include "trap.h"
#if GEMINI
#include "..\venus\gemini.h"
#endif


/* internal texts
 */

void ParsAndExec (MGLOBAL *M)
{
	PARSINFO P;
	TREEINFO *tree;

	while (!ParsEOFReached (M))
	{
		tree = Parse (M, &P, '\n');
		
		if (M->exit_shell
			|| ((M->exit_shell = CheckSignal (M)) != FALSE))
		{
			FreeTreeInfo (M, tree);
			break;
		}
		
		if (tree)
		{
			ExecTree (M, tree, M->shell_flags.break_on_false,
				NULL, NULL);
			FreeTreeInfo (M, tree);
			
			if (M->exit_shell)
			{
				M->exit_value = M->exit_exit_value;
				break;
			}
			
			if ((M->shell_flags.break_on_false && (M->exit_value != 0))
				|| M->shell_flags.only_one_command)
			{
				break;
			}
		}

		if (M->exit_shell 
			|| (!M->shell_flags.interactive && P.L.broken))
			break;
	}
}

int ExecScript (MGLOBAL *M, const char *full_name, int argc, 
	const char **argv, WORDINFO *var_list, void *start_parameter,
	int *broken)
{
	MGLOBAL *new;
	EXECPARM parms;
	SCREENINFO screen;
	char start_path[MAX_PATH_LEN];
	
	start_path[0] = '\0';
	*broken = FALSE;
	
	new = MakeNewShellLevel (M);
	if (!new)
	{
		*broken = TRUE;
		return 1;
	}

	new->program_name = argv[0];
	new->argv = (char **)argv;
	new->argc = argc;
	
	new->shell_flags.interactive = 0;
	
	if (start_parameter != NULL)
		parms = *(EXECPARM *)start_parameter;
	else
		*broken = !InitExecParameter (new, &parms, full_name, FALSE);

	if (SetExecParameter (new, &parms, &screen, full_name, 
		start_path, broken))
	{	
		PARSINPUTINFO PSave;
		
#if !STANDALONE
		const char *ext = Extension (full_name);
		int prepared = 0;
		int wasOpened, oldTopWindow;
		
		if (!ext || !stricmp (ext, "mup"))
		{
			if (ScreenWindowsAreOpen() 
				&& M->checkIfConsoleIsOpen)
			{
				PrepareInternalStart (&wasOpened, &oldTopWindow, 
					TRUE);
				prepared = TRUE;
				M->checkIfConsoleIsOpen = FALSE;
			}
		}
#endif
		if (SetWordList (new, var_list) 
			&& InstallShellVars (new, &argv[1], argc - 1, TRUE, TRUE))
		{
			long ret = ParsFromFile (new, &PSave, full_name);

			if (ret == E_OK)
			{
				ParsAndExec (new);
				ParsRestoreInput (new, &PSave);
			}
			else
			{
				eprintf (new, "%s: `%s' -- %s\n", new->argv[0],
					argv[0], StrError (ret));
				*broken = TRUE;
				new->exit_value = 1;
			}
		
			DeinstallShellVars (new);
		}
		else
		{
			*broken = TRUE;
			new->exit_value = 1;
		}

		UnsetExecParameter (new, &parms, &screen, broken, FALSE);
#if !STANDALONE
		if (prepared)
		{
			PrepareInternalStart (&wasOpened, &oldTopWindow, 
				FALSE);
		}	
#endif
	}
	else
		*broken = TRUE;
	
	M->exit_value = new->exit_value;
	FreeMadeShellLevel (M, new);
	
	return M->exit_value;
}

