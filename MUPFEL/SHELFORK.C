/* @(#) Mupfel\shelfork.c
 * @(#) Stefan Eissing, 08. Februar 1994
 *
 * Make Fork with new Shell-Level
 *
 * jr 29.6.96
 */

#include <stdio.h>
#include <tos.h>
#include <mint\mintbind.h>
#include <mint\ioctl.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\strerror.h"
#include "mglob.h"

#include "chario.h"
#include "duptree.h"
#include "exectree.h"
#include "maketree.h"
#include "redirect.h"
#include "shelfork.h"


/* internal texts
 */
#define NlsLocalSection "M.shelfork"
enum NlsLocalText{
FK_NOFORK,	/*kann keinen neuen Prozež erzeugen\n*/
FK_NOMEM	/*Nicht genug freier Speicher vorhanden\n*/
};

/* jr: made it smaller so that it fits into 2 8K blocks under MP */

#define NEW_STACK_SIZE 15000L

typedef struct
{
	MGLOBAL *old;
	MGLOBAL *new;
	
	TREEINFO *tree;
	IOINFO *pipe_out;
	IOINFO *pipe_in;
	int tree_flags;
	int done;
} FORK_N_GO;


int
fork_n_go (long arg, int wasRealFork)
{
	FORK_N_GO *F = (FORK_N_GO *)arg;
	MGLOBAL *new, *old;
	TREEINFO *tree;
	IOINFO *pout, *pin;
	int retcode, tree_flags;
	
	new = F->new;
	old = F->old;
	tree = F->tree;
	pout = F->pipe_out;
	pin = F->pipe_in;
	tree_flags = F->tree_flags;

	if (wasRealFork)
	{
		if (tree_flags & FPIN)
			Fclose (pout->pipe_handle);
		if (tree_flags & FPOU)
			Fclose (pin->pipe_handle);
	}
	
	new = MakeNewShellLevel (old);
	if (new)
	{
		tree = DupTreeInfo (new, tree);
	}

	F->done = TRUE;

	if (!new || !tree)
	{
		eprintf (new, NlsStr(FK_NOMEM));
		FreeMadeShellLevel (NULL, new);

		return -1;
	}
	
	if (wasRealFork)
	{
		new->shell_flags.interactive = 0;
		new->stack_limit = ((long)&F) - NEW_STACK_SIZE + 1024;

		new->our_pid = Pgetpid ();
		Psignal (SIGTERM, SIG_DFL);
		Psignal (SIGTSTP, SIG_DFL);

		Psetpgrp (0, 0);
		
		old = NULL;
	}
	
	if ((tree->typ & COMMASK) == TCOM)
		tree->typ |= FOVL;
		
	retcode = ExecTree (new, tree, TRUE, NULL, NULL)? 
		new->exit_value : -1;

	FreeTreeInfo (new, tree);
	if (wasRealFork)
		ExitRedirection (new);
		
	FreeMadeShellLevel (old, new);
	return retcode;
}

static void cdecl
startup (BASPAG *b)
{
	int (*func)(long, int);
	long arg;
	extern void SetStack (void *new_stack);

/*	SetStack ((void *) ((long)b->p_hitpa - 4)); xxx: not needed */

	func = (int (*)(long, int))b->p_dbase;
	arg = b->p_dlen;
	Pterm (func (arg, 1));
}


long
tfork (int (*func)(long, int), long arg)
{
	BASPAG *b;
	long pid;
	long ret;

	/* create basepage */

	ret = Pexec (7, (char *)0x7L, "", 0L);
	if (ret == EINVFN) ret = Pexec (5, 0L, "", 0L);
	if (ret <= 0) return ret ? ret : -1L;
	
	b = (BASPAG *) ret;
	
	/* shrink TPA */

	Mshrink (0, b, NEW_STACK_SIZE + sizeof (BASPAG));
	
	b->p_tbase = (char *)startup;
	b->p_dbase = (char *)func;
	b->p_dlen = arg;
	b->p_hitpa = ((char *)b) + NEW_STACK_SIZE + sizeof (BASPAG);
 
	pid = Pexec (104, 0L, b, 0L);

	if (b->p_env) Mfree (b->p_env);

	Mfree (b);
	
	return pid;
}


int ShelFork (MGLOBAL *M, MGLOBAL *new, TREEINFO *tree,
	IOINFO *pipe_out, IOINFO *pipe_in, int tree_flags,
	int *wasRealFork)
{
	FORK_N_GO F;
	long retcode;
	
	F.old = M;
	F.new = new;
	F.tree = tree;
	F.pipe_out = pipe_out;
	F.pipe_in = pipe_in;
	F.tree_flags = tree_flags;
	F.done = FALSE;


	/* jr: workaround fr MagiC 5. Da gibt es Pexec 104, aber
	die Mupfel stirbt mit einer Speicherschutzverletzung, wenn
	sie es benutzt. 2b debugged */

	if (MagiCVersion)	
		retcode = EINVFN;
	else
		retcode = tfork (fork_n_go, (long)&F);

	if (retcode >= 0L)
	{
		*wasRealFork = TRUE;
	}
	else if (retcode == EINVFN)
	{
		*wasRealFork = FALSE;
		retcode = fork_n_go ((long)&F, *wasRealFork) & 0x0000FFFFL;
	}

	if (retcode < 0L)
		eprintf (M, NlsStr(FK_NOFORK));
	else
	{
		while (!F.done)
			;
	}

	return (int)retcode;
}
