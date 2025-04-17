/*
 * @(#) MParse\ExecTree.c
 * @(#) Stefan Eissing, 08. Februar 1994
 *
 * jr 8.8.94
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\cookie.h"
#include "..\common\fileutil.h"
#include "mglob.h"

#include "chario.h"
#include "duptree.h"
#include "exec.h"
#include "exectree.h"
#include "internal.h"
#include "locate.h"
#include "makeargv.h"
#include "maketree.h"
#include "mdef.h"
#include "names.h"
#include "nameutil.h"
#include "partypes.h"
#include "parsutil.h"
#include "prnttree.h"
#include "redirect.h"
#include "shelfork.h"
#include "shellvar.h"
#include "stand.h"
#include "subst.h"
#include "trap.h"
#include "..\common\wildmat.h"

/* internal texts
 */
#define NlsLocalSection "M.exectree"
enum NlsLocalText{
ET_NOMEM,	/*Nicht genug freier Speicher vorhanden\n*/
ET_FINDERR,	/*Fehler beim Suchen von %s\n*/
ET_NOTFOUND, /*%s nicht gefunden\n*/
ET_CANTSET,	/*Fehler beim Setzen der Parameter\n*/
ET_NOTEXEC,	/*%s ist keine Programmdatei\n*/
ET_VARSET,	/*Konnte Variablen nicht setzen\n*/
ET_STACK,	/*Aufruf zu tief verschachtelt\n*/
};

#define CHECK_STACK		1

int execAlienScript (MGLOBAL *M, int argc, const char **argv, 
		const char *executer, const char *scriptname, 
		int break_on_false, WORDINFO *var_list, int cmd_kind,
		void *start_parameter, int overlay_process, int parallel)
{
	char **new_argv;
	int new_argc;
	int retcode;
	
	new_argc = argc + 1;
	new_argv = mcalloc (&M->alloc, new_argc, sizeof (char *));
	if (new_argv == NULL)
	{
		eprintf (M, NlsStr(ET_NOMEM));
		M->exit_value = 1;
		return FALSE;
	}

	new_argv[0] = (char *)executer;
	new_argv[1] = (char *)scriptname;
	if (new_argc > 2)
	{
		memcpy (&new_argv[2], &argv[1], 
			(new_argc - 2) * sizeof (char *));
	}
		
	retcode = ExecArgv (M, new_argc, new_argv, 
		break_on_false, var_list, cmd_kind, 
		start_parameter, overlay_process, parallel);
		
	mfree (&M->alloc, new_argv);

	return retcode;
}

int ExecArgv (MGLOBAL *M, int argc, const char **argv, 
	int break_on_false, WORDINFO *var_list, int cmd_kind,
	void *start_parameter, int overlay_process, int parallel)
{
	LOCATION L;
	int retcode = TRUE;
	int where_to_look = 0;
	
	if (argc < 1)
		return TRUE;
		
#if CHECK_STACK
	/* Wir versuchen hier einen Stack-Check mittels der in TCSTART.O
	 * gesetzten Variable _StkLim. Dies geht natÅrlich nur mit diesem
	 * Startupcode und diesem Prozessortyp (Stackrichtung).
	 */
	{
		int dummy;
		
		if (((unsigned long)&dummy) <= M->stack_limit)
		{
			dprintf (NlsStr(ET_STACK));
			return FALSE;
		}
	}
#endif

	if (cmd_kind & CMD_FUNCTION)
		where_to_look |= LOC_FUNCTION;
	if (cmd_kind & CMD_INTERNAL)
		where_to_look |= LOC_INTERNAL;
	if (cmd_kind & CMD_EXTERNAL)
		where_to_look |= LOC_EXTERNAL;

	if (!Locate (M, &L, argv[0], TRUE, where_to_look))
	{
		if (L.broken)
			eprintf (M, NlsStr(ET_FINDERR), argv[0]);
		else
			eprintf (M, NlsStr(ET_NOTFOUND), argv[0]);
			
		M->exit_value = ERROR_COMMAND_NOT_FOUND;
		return !L.broken;
	}

	if (L.location != Binary && L.location != AlienScript
		&& L.location != NotFound && !SetWordList (M, var_list))
	{
		FreeLocation (M, &L);
		return FALSE;
	}
		
	switch (L.location)
	{
		case Function:
			if (InstallShellVars (M, &(argv[1]), argc - 1, 
				TRUE, FALSE))
			{
				++M->function_count;
				
				retcode = ExecTree (M, (TREEINFO *)
					L.loc.func.n->func->commands, break_on_false,
					NULL, NULL);
				
				--M->function_count;
				M->exec_break = FALSE;
				DeinstallShellVars (M);
			}
			else
			{
				eprintf (M, NlsStr(ET_CANTSET));
				retcode = FALSE;
			}
			break;

		case Data:
			eprintf (M, NlsStr(ET_NOTEXEC), L.loc.file.full_name);
			M->exit_value = ERROR_NOT_EXECUTABLE;
			break;
			
		case Internal:
			M->exit_value = ExecInternalCommand (M, 
					L.loc.intern.command_number, argc, argv);
			break;

		case AlienScript:
		{
			retcode = execAlienScript (M, argc, argv, 
						L.loc.file.executer, L.loc.file.full_name,
						break_on_false, var_list, CMD_ALL,
						start_parameter, overlay_process, parallel);
			break;
		}

		case Script:
		case Binary:
		{
			if (!M->shell_flags.no_execution)
			{
				if (M->shell_flags.print_as_exec)
				{
					const char *prompt = GetPS4Value (M);
					const char *xprompt = StringVarSubst (M, prompt);
					
					eprintf (M, "%s", xprompt ? xprompt : prompt);
					if (xprompt) mfree (&M->alloc, xprompt);

					printArgv (M, argc, argv);
				}
				
				if (L.location == Binary)
				{
					ARGVINFO A;
					
					if (MakeAndExpandArgv (M, var_list, &A))
					{
						M->exit_value = ExecExternal (M, 
							L.loc.file.full_name, argc, argv, 
							A.argc, A.argv, start_parameter,
							overlay_process, parallel, &L.broken);
						
						FreeExpArgv (&A);
					}
					else
					{
						eprintf (M, NlsStr(ET_VARSET));
						retcode = FALSE;
					}
				}
				else
				{
#if GEMINI
					EXECPARM parms;
					int start_external = FALSE;
					const char *shell = GetEnv (M, "SHELL");

					if (InitExecParameter (M, &parms, 
						L.loc.file.full_name, parallel))
					{
						start_external = (parms.in_window != PARM_YES);
					}
						
					if (parallel)
					{
						/* Wir haben ein parallel auszufÅhrendes
						 * Shell-Script. Wenn wir das nicht wirklich
						 * kînnen, dann starten wir besser keine
						 * neue Shell, sondern fÅhren es in der
						 * derzeitigen Shell aus.
						 */
						parallel = MiNTVersion || MagiCVersion;
					}
					
					if (parallel && start_external && shell 
						&& access (shell, A_EXEC, &L.broken))
					{
						retcode = execAlienScript (M, argc, argv, 
							shell, L.loc.file.full_name,
							break_on_false, var_list, CMD_EXTERNAL,
							start_parameter, overlay_process, parallel);
					}
					else
#endif
					{
						M->exit_value = ExecScript (M, 
							L.loc.file.full_name, argc, argv, 
							var_list, start_parameter, &L.broken);
					}
				}
			}
			break;
		}
		
		case NotFound:
			eprintf (M, NlsStr(ET_NOTFOUND), argv[0]);
			M->exit_value = ERROR_COMMAND_NOT_FOUND;
			break;
	}
	
	FreeLocation (M, &L);
	return retcode;
}


static int execCommand (MGLOBAL *M, COMMANDINFO *cmd, 
			int break_on_false)
{
	ARGVINFO A;
	int retcode = TRUE;
	
	if (!MakeAndExpandArgv (M, cmd->args, &A))
	{
		M->exit_value = 2;
		return FALSE;
	}

	if (!DoRedirection (M, cmd->io))
	{
		FreeExpArgv (&A);
		return FALSE;
	}
	
	if (A.argc)
	{
		retcode = ExecArgv (M, A.argc, A.argv, break_on_false, 
			cmd->set, CMD_ALL, NULL, cmd->typ & FOVL, FALSE);
	}
	else
	{
		retcode = SetWordList (M, cmd->set);
		M->exit_value = 0;
	}

	FreeExpArgv (&A);

	return (UndoRedirection (M, cmd->io) && retcode);
		
}

/* FÅhre eine While/Until-Schleife aus.
 */
static int execWhile (MGLOBAL *M, WHILEINFO *w, int break_on_false)
{
	int until = (w->typ != TWHILE);
	int last_ret_in_loop = 0;
	int retcode = TRUE;
	
	++M->loop_count;
	
	if (!ExecTree (M, w->condition, FALSE, NULL, NULL))
	{
		--M->loop_count;
		return FALSE;
	}
	
	while (!M->exec_break && !checkintr (M) && retcode &&
		((M->exit_value && until) || (!M->exit_value && !until)))
	{
		retcode = ExecTree (M, w->commands, 
			break_on_false, NULL, NULL);
		last_ret_in_loop = M->exit_value;

		if (!retcode || M->break_count)
			break;

		if (!ExecTree (M, w->condition, FALSE, NULL, NULL))
		{
			--M->loop_count;
			return FALSE;
		}
	}
	
	if (M->break_count)
	{
		/* Irgendwo in der Schleife, die gerade abgebrochen wurde,
		 * gab es ein break oder continue. Wenn der break nur noch 
		 * fÅr diese Schleife reichte, setzte M->exec_break auf FALSE.
		 */
		(M->break_count > 0)? --M->break_count : ++M->break_count;
	}
	M->exec_break = (M->break_count != 0);
	
	--M->loop_count;
	M->exit_value = last_ret_in_loop;
	
	return TRUE;
}


/* FÅhre eine FOR-Schleife aus.
 */
static int execFor (MGLOBAL *M, FORINFO *f, int break_on_false)
{
	ARGVINFO A;
	int made_argv = FALSE;
	int i, retcode = TRUE;
	int last_exit_value = 0;

	if (f->list)
	{
		if (!MakeAndExpandArgv (M, f->list->args, &A))
			return FALSE;
		else
			made_argv = TRUE;
	}
	else
	{
		/* Hole Shell-Variablen
		 */
		LinkShellVars (M, &(A.argv), &(A.argc));
	}
			
	++M->loop_count;
	
	for (i = 0; 
		!M->exec_break && (i < A.argc) && retcode && !checkintr (M); 
		++i)
	{
		/* Setzte die Variable auf das Argument
		 */
		if (!InstallName (M, f->name->name, A.argv[i]))
			break;
		 
		retcode = ExecTree (M, f->commands, break_on_false, 
			NULL, NULL);
		last_exit_value = M->exit_value;

		if (M->break_count < 0)
		{
			/* Wir habe hier ein continue gehabt. Wenn der Wert
			 * von continue hîher reicht, als diese Schleife, dann
			 * verlasse sie.
			 */
			M->exec_break = (++M->break_count != 0);
		}
	}
	
	if (M->break_count > 0)
	{
		/* Irgendwo in der Schleife, die gerade abgebrochen wurde,
		 * gab es ein break. Wenn der break nur noch fÅr diese
		 * Schleife reichte, setzte M->exec_break auf FALSE.
		 */
		M->exec_break = (--M->break_count != 0);
	}
	
	if (made_argv)
		FreeExpArgv (&A);
	--M->loop_count;
	M->exit_value = last_exit_value;

	return retcode;
}


/* FÅhre eine If-then-else Bedingung aus.
 */
static int execIf (MGLOBAL *M, IFINFO *i, int break_on_false)
{
	if (!ExecTree (M, i->condition, FALSE, NULL, NULL)
		|| M->exit_shell)
		return FALSE;
	
	if (!M->exit_value && !intr (M))
	{
		return ExecTree (M, i->then_case, break_on_false, NULL, NULL);
	}
	else if (i->else_case && !intr (M))
	{
		return ExecTree (M, i->else_case, break_on_false, NULL, NULL);
	}
	else
		M->exit_value = 0;
		
	return TRUE;
}

/* FÅhre einen Case aus
 */
static int execCase (MGLOBAL *M, CASEINFO *c, int break_on_false)
{
	REGINFO *reg;
	char *arg_name, *arg_quot;
	int broken = FALSE, retcode = TRUE;
	int last_exit_value = 0;
	
	if (!SubstWithCommand (M, c->name, &arg_name, &arg_quot))
		return FALSE;

	reg = c->case_list;
	while (!broken && reg)
	{
		WORDINFO *matches = reg->matches;
		
		while (matches)
		{
			char *m_name, *m_quot;
			
			if (!SubstWithCommand (M, matches, &m_name, &m_quot))
			{
				broken = TRUE;
				retcode = FALSE;
				break;
			}
			
			if (wildmatch (arg_name, m_name, TRUE))
			{
				retcode = ExecTree (M, reg->commands, break_on_false,
					NULL, NULL);
				last_exit_value = M->exit_value;
				broken = TRUE;
			}
			
			mfree (&M->alloc, m_name);
			mfree (&M->alloc, m_quot);
			matches = matches->next;
		}
		reg = reg->next;
	}	

	mfree (&M->alloc, arg_name);
	mfree (&M->alloc, arg_quot);
	M->exit_value = last_exit_value;
	
	return retcode;	
}


/* FÅhrt die Kommandos in <*treepp> aus und setzt <M->exit_value>
 * entsprechend. Tritt bei der AusfÅhrung der Kommandos ein
 * Fehler auf, so wird FALSE zurÅckgeliefert.
 *
 * Wenn <break_on_false> TRUE ist, wird, wenn ein Kommando einen
 * Returnwert != 0 hat, die Abarbeitung abgebrochen:
 *
 * Ist <free_tree> TRUE, wird nach Abarbeiten der Kommandos <treepp>
 * auf NULL gesetzt und der durch <*trepp> belegte Speicher
 * freigegeben.
 */
int ExecTree (MGLOBAL *M, TREEINFO *tree, int break_on_false,
	IOINFO *pipe_out, IOINFO *pipe_in)
{
	int tree_type, tree_flags;
	int retcode = TRUE;
	
	if (!tree)
		return TRUE;
	
	if (!M->exit_shell)
		M->exit_shell = CheckSignal (M);
		
	if (M->exit_shell || M->exec_break || intr (M))
	{
		if (!M->exit_shell && !M->shell_flags.interactive && intr (M))
		{
			M->exit_exit_value = M->exit_value;
			M->exit_shell = 1;
		}
		
		return TRUE;
	}
	
#if CHECK_STACK
	/* Wir versuchen hier einen Stack-Check mittels der in TCSTART.O
	 * gesetzten Variable _StkLim. Dies geht natÅrlich nur mit diesem
	 * Startupcode und diesem Prozessortyp (Stackrichtung).
	 */
	{
		int dummy;
		
		if (((unsigned long)&dummy) <= M->stack_limit)
		{
			dprintf (NlsStr(ET_STACK));
			return FALSE;
		}
	}
#endif

	tree_flags = tree->typ;
	tree_type = tree->typ & COMMASK;
	
	switch (tree_type)
	{
		case TFUNC:
			if (!InstallFunction (M, (FUNCINFO *)tree))
				eprintf (M, NlsStr(ET_NOMEM));

			M->exit_value = 0;
			break;
		
		case TCOM:
			retcode = execCommand (M, (COMMANDINFO *)tree, 
				break_on_false);
			break;
		
		case TFORK:
		{
			FORKINFO *f = (FORKINFO *)tree;
			int sub_shell;
			int realFork = FALSE;
			
			M->exit_value = 0;
			
			sub_shell = (tree_flags & (FAMP | FPOU));

			if (!DoRedirection (M, f->io))
				break;
			
			if (sub_shell)
			{
				retcode = ShelFork (M, NULL, f->tree, pipe_out,
					pipe_in, tree_flags, &realFork);
				if (retcode >= 0)
				{
					M->last_async_pid = retcode;
					retcode = TRUE;
				}
				else
					retcode = FALSE;
			}
			else	
				retcode = ExecTree (M, f->tree, break_on_false,
					pipe_out, pipe_in);
			
			UndoRedirection (M, f->io);
			
			if (realFork && (retcode >= 0) && (tree_flags & FAMP))
			{
				mprintf (M, "%ld\n", M->last_async_pid);
			} 
			break;
		}
		
		case TPAR:
		{
			PARINFO *par = (PARINFO *)tree;
			TREEINFO *new_tree;
			MGLOBAL *new;

			new = MakeNewShellLevel (M);
			if (new != NULL)
				new_tree = DupTreeInfo (new, par->tree);
			
			if (!new || (par->tree && !new_tree))
			{
				eprintf (M, NlsStr(ET_NOMEM));
				if (new)
					FreeMadeShellLevel (M, new);
				retcode = FALSE;
				break;
			}

			retcode = ExecTree (new, new_tree, break_on_false,
				pipe_out, pipe_in);
			FreeTreeInfo (new, new_tree);
			M->exit_value = new->exit_value;
	
			/* stevie: Wenn im neuen Level trap[0] gesetzt ist, wird
			 * es jetzt ausgefÅhrt!
			 */		
			FreeMadeShellLevel (M, new);
			break;
		}
		
		case TFIL:
		{
			LISTINFO *list = (LISTINFO *)tree;
			IOINFO pipe_left, pipe_right;
			
			if (!OpenPipe (M, &pipe_left, &pipe_right))
				break;
	
			if (!DoRedirection (M, &pipe_left))
			{
				ClosePipe (M, &pipe_left, &pipe_right);
				break;
			}
			
			retcode = ExecTree (M, list->left, break_on_false,
				&pipe_left, &pipe_right);
			UndoRedirection (M, &pipe_left);
			
			if (retcode && !M->exec_break && !M->exit_shell)
			{
				if (DoRedirection (M, &pipe_right))
				{
					ExecTree (M, list->right, break_on_false,
						&pipe_left, &pipe_right);
					UndoRedirection (M, &pipe_right);
				}
			}
			
			ClosePipe (M, &pipe_left, &pipe_right);
				
			break;
		}
		
		case TLIST:
		{
			LISTINFO *list = (LISTINFO *)tree;
			
			retcode = (ExecTree (M, list->left, break_on_false,
				pipe_out, pipe_in)
				&& !M->exec_break
				&& !M->exit_shell
				&& ExecTree (M, list->right, break_on_false,
					pipe_out, pipe_in));

			break;
		}

		case TORF:
		case TAND:
		{
			LISTINFO *list = (LISTINFO *)tree;
			int is_and = (tree_type == TAND);

			retcode = ExecTree (M, list->left, break_on_false,
				pipe_out, pipe_in);
			if (retcode	&& !M->exec_break && !M->exit_shell
				&& ((!M->exit_value && is_and) 
					|| (M->exit_value && !is_and)))
			{
				retcode = ExecTree (M, list->right, break_on_false,
					pipe_out, pipe_in);
			}
			break;
		}

		case TFOR:
			execFor (M, (FORINFO *)tree, break_on_false);
			break;
		
		case TWHILE:
		case TUNTIL:
			execWhile (M, (WHILEINFO *)tree, break_on_false);
			break;
		
		case TIF:
			execIf (M, (IFINFO *)tree, break_on_false);
			break;
		
		case TCASE:
		{
			execCase (M, (CASEINFO *)tree, break_on_false);
			break;
		}
		
		default:
			dprintf ("ExecTree: unbekannter Typ!\n");
			break;
	}
	
	if (!M->exit_shell)
		M->exit_shell = CheckSignal (M);
		
	return retcode;
}