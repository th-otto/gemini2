/*
 * @(#) Mupfel\trap.c
 * @(#) Stefan Eissing, 24. Januar 1994
 *
 *  -  Trap handling
 *
 * jr 1.7.1996
 */

#include <stddef.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <mint\mintbind.h>
#include <mint\ioctl.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "getopt.h"
#include "parsinpt.h"
#include "parsutil.h"
#include "trap.h"

/* internal texts
 */
#define NlsLocalSection "M.trap"
enum NlsLocalText{
OPT_trap,	/*[ Befehle ] Name ...*/
HLP_trap,	/*Signal Name mit Befehl belegen/ignorieren*/
TR_NOMEM,	/*%s: nicht genug Speicher\n*/
TR_ILLTRAP,	/*%s: ungÅltiges Signal %s\n*/
};

#define SI_IGNORED			1
#define SI_TERMINATE		2
#define SI_INTER_IGNORE		4

typedef struct
{
	int number;
	const char *name;
	int flags;
} SigInfo;

static SigInfo sigs[NSIG] =
{
{SIGNULL, "EXIT", SI_IGNORED},
{SIGHUP, "HUP", SI_TERMINATE},
{SIGINT, "INT", SI_IGNORED},
{SIGQUIT, "QUIT", SI_TERMINATE}, 
{SIGILL, "ILL", 0},
{SIGTRAP, "TRAP", 0}, 
{SIGABRT, "ABRT", 0}, 
{SIGPRIV, "PRIV", 0},
{SIGFPE, "FPE", 0},
{SIGKILL, "KILL", 0},
{SIGBUS, "BUS", 0},
{SIGSEGV, "SEGV", 0},
{SIGSYS, "SYS", SI_TERMINATE},
{SIGPIPE, "PIPE", SI_TERMINATE},
{SIGALRM, "ALRM", SI_IGNORED},
{SIGTERM, "TERM", SI_INTER_IGNORE},
{SIGURG, "URG", 0},
{SIGSTOP, "STOP", SI_IGNORED},
{SIGTSTP, "TSTP", SI_INTER_IGNORE},
{SIGCONT, "CONT", SI_IGNORED},
{SIGCHLD, "CHLD", SI_IGNORED},
{SIGTTIN, "TTIN", SI_IGNORED},
{SIGTTOU, "TTOU", SI_IGNORED},
{SIGIO, "IO", 0},
{SIGXCPU, "XCPU", 0}, 
{SIGXFSZ, "XFSZ", 0},
{SIGVTALRM, "VTALRM", 0},
{SIGPROF, "PROF", 0},
{SIGWINCH, "WINCH", SI_IGNORED},
{SIGUSR1, "USR1", 0},
{SIGUSR2, "USR2", 0},
};

static int
sigName2Number (const char *name, short *number)
{
	short i;
	
	for (i = 0; i < NSIG; ++i)
		if (!strcmp (name, sigs[i].name))
		{
			*number = sigs[i].number;
			return TRUE;
		}

	return FALSE;
}

static const char *
sigNumber2Name (short number)
{
	short i;
	
	for (i = 0; i < NSIG; ++i)
		if (number == sigs[i].number)
			return sigs[i].name;

	return NULL;
}

static int
getSignalFlags (short number)
{
	short i;
	
	for (i = 0; i < NSIG; ++i)
		if (number == sigs[i].number)
			return sigs[i].flags;

	return 0;
}

static void cdecl
processSignal (long arg)
{
	MGLOBAL *M;
	short signal = (short)arg;

	M = (MGLOBAL *)Pusrval (-1L);
	if (M == NULL || M == (MGLOBAL *)EINVFN)
	{
dprintf	("Pusrval returned %p\n", M);
		return;
	}
	
	if (signal < 0 || signal >= MAX_TRAP)
	{
dprintf	("illegal signal %d\n", signal);
		return;
	}
	
	if (signal == SIGINT && M->trap[SIGINT].command == NULL)
		M->tty.ctrlc = TRUE;

	if (signal == SIGCHLD)
		Fcntl (-1, (long)&M->jobs.term_pgrp, TIOCSPGRP);
		
	M->trap[signal].flags.was_raised = 1;

	if (M->break_buffer_set)
	{
		Psigreturn ();
		longjmp (M->break_buffer, 2);
	}
}

/* Initialisiere die Informationen fÅr Traps. */

void
InitTrap (MGLOBAL *M)
{
	memset (&M->trap, 0, sizeof (M->trap));

	if (Pusrval ((long)M) != EINVFN)
	{
		short i;
		
		for (i = 0; i < NSIG; ++i)
			if (sigs[i].flags && (sigs[i].flags != SI_INTER_IGNORE))
				Psignal (sigs[i].number, processSignal);

		if (M->shell_flags.interactive) {
			Psignal (SIGTERM, (void *)SIG_IGN);
			Psignal (SIGTSTP, (void *)SIG_IGN);
		}
	}
}


void
ReInitTrap (MGLOBAL *M, MGLOBAL *new)
{
	int i;

	memcpy (new->trap, M->trap, sizeof (new->trap));

	for (i = 0; i < MAX_TRAP; ++i)
		if (M->trap[i].command)
			new->trap[i].command = 
				StrDup (&new->alloc, M->trap[i].command);

	Pusrval ((long)new);
}


/* Gib den Speicher fÅr Traps wieder frei. */

void
ExitTrap (MGLOBAL *M, MGLOBAL *old)
{
	int i;
	
	Pusrval ((long)old);

	for (i = 0; i < MAX_TRAP; ++i)
	{
		if (M->trap[i].command)
		{
			mfree (&M->alloc, M->trap[i].command);
			M->trap[i].command = NULL;
		}
	}
}


/* Lîsche alle hÑngenden Signals, damit beim nÑchsten Check diese
   nicht ausgefÅhrt werden. */

static void
clearSignals (MGLOBAL *M)
{
	int i;
	
	for (i = 0; i < MAX_TRAP; ++i)
		M->trap[i].flags.was_raised = FALSE;
}

static int
installSignal (MGLOBAL *M, short signal)
{
	int flags = getSignalFlags (signal);
		
	if ((M->trap[signal].command != NULL) 
		|| (flags && (flags != SI_INTER_IGNORE 
			|| M->shell_flags.interactive)))
	{
		Psignal (signal, processSignal);
	}
	else
		Psignal (signal, SIG_DFL);
	
	return TRUE;
}


static int
defSignalAction (MGLOBAL *M, short signal)
{
	switch (signal)
	{
		case SIGWINCH:
			SetRowsAndColumns (M, 25, 80);
			break;
		
		case SIGCHLD:
			break;
		
		case SIGINT:
			M->tty.ctrlc = TRUE;
			break;
		
		default:
			break;
	}
	
	return FALSE;
}

/* Checke, ob ein Signal anliegt und fÅhre es aus. Dabei wird von
   dieser Funktion zurÅckgeliefert, ob die Shell danach verlassen
   werden muû. */

int
CheckSignal (MGLOBAL *M)
{
	int i = MAX_TRAP - 1;
	int retcode = FALSE;
	
	/* Die Signal werden "von oben nach unten" durchgecheckt. Signale
	   mit hîherer Nummer haben also eine hîhere PrioritÑt. */

	for (; (i >= 0) && !retcode; --i)
	{
		if (!M->trap[i].flags.was_raised)
			continue;

		M->trap[i].flags.was_raised = FALSE;

		if (M->trap[i].command == NULL)
		{
			int flags = getSignalFlags (i);
			
			if (flags == SI_TERMINATE)
				retcode = TRUE;
			else if ((flags == SI_INTER_IGNORE)
				&& !M->shell_flags.interactive)
				retcode = TRUE;
			else
				retcode = defSignalAction (M, i);
		}
		else if (!M->trap[i].command[0])
			retcode = FALSE;
		else
		{
			PARSINPUTINFO PSave;
			
			M->exec_break = M->exit_shell = FALSE;
			clearintr (M);
			
			ParsFromString (M, &PSave, M->trap[i].command);
			
			ParsAndExec (M);
	
			ParsRestoreInput (M, &PSave);

			retcode = (M->exit_shell || i == SIGEXIT);
		}
	}
	
	clearSignals (M);
	return retcode;
}


/* Merkt sich, daû das Signal <signal> aufgetreten ist und gibt
   zurÅck, ob die Shell terminiert werden muû. */
   
int
SignalTrap (MGLOBAL *M, int signal)
{
	/* unbekanntes Signal */
	if (signal < 0 || signal >= MAX_TRAP)
		return FALSE;
	
	M->trap[signal].flags.was_raised = TRUE;

	return CheckSignal (M);
}


/* Internes Kommando "trap". Dient zum Setzen, lîschen und anzeigen
   der Belegung von Signalen. */
   
int
m_trap (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	struct option long_option[] =
	{
		{ NULL, 0,0,0},
	};
	int opt_index = 0;
	int c;
	int reset_traps = FALSE; 

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_trap), 
			NlsStr(HLP_trap));
	
	/* jr: Das sieh zwar etwas unglÅcklich aus, sorgt aber fÅr das
	Parsen von -- */

	optinit (&G);

	while ((c = getopt_long (M, &G, argc, argv, "", 
		long_option, &opt_index)) != EOF)
	{
		if (!c)
			c = long_option[opt_index].val;
			
		switch (c)
		{
			case 0:
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_trap));
		}
	}

	/* jr: keine Argumente? */
	
	if (G.optind >= argc)
	{
		int i;
		
		for (i = 0; i < MAX_TRAP; ++i)
			if (M->trap[i].command)
			{
				mprintf (M, "trap -- '");
				domprint (M, (char *)M->trap[i].command, 
					strlen (M->trap[i].command));
				mprintf (M, "' %s\n", sigNumber2Name (i));
			}
		
		return 0;
	}

	/* jr: Option - */
	
	if (argc > G.optind && !strcmp (argv[G.optind], "-"))
	{
		G.optind++;
		reset_traps = TRUE;
	}

	if ((reset_traps && G.optind >= argc) ||
		(!reset_traps && G.optind != argc - 2))
		return PrintUsage (M, argv[0], NlsStr(OPT_trap));
	else
	{
		int i;
		char *set_to;
		
		if (!reset_traps)
			set_to = argv[G.optind++];
		
		for (i = G.optind; i < argc; ++i)
		{
			short signal;
			
			if (!sigName2Number (argv[i], &signal)
				&& ((!isdigit (argv[i][0]))
				|| ((signal = atoi (argv[i])) >= MAX_TRAP)))
			{
				eprintf (M, NlsStr(TR_ILLTRAP), argv[0], argv[i]);
				return 1;
			}
			
			if (M->trap[signal].command)
			{
				mfree (&M->alloc, M->trap[signal].command);
				M->trap[signal].command = NULL;
			}
			
			if (!reset_traps)
			{
				M->trap[signal].command = StrDup (&M->alloc, set_to);
				if (!M->trap[signal].command)
				{
					eprintf (M, NlsStr(TR_NOMEM), argv[0]);
					return 1;
				}
			}
			
			installSignal (M, signal);
		}
	}
	
	return 0;
}