/*
 * @(#) Mupfel\MGlobal.c
 * @(#) Stefan Eissing, $Id: mglobal.c,v 1.1 1995/04/21 15:25:16 user Exp user $
 */
 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <aes.h>
#include <vdi.h>
#include <tos.h>
#include <mint\mintbind.h>
#include <mint\ioctl.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\cookie.h"
#include "..\common\toserr.h"

#include "mglob.h"

#include "alias.h"
#include "chario.h"
#include "cookie.h"
#include "gemutil.h"
#include "hash.h"
#include "history.h"
#include "internal.h"
#include "ioutil.h"
#include "ls.h"	/* xxxx */
#include "mcd.h"
#include "mdef.h"
#include "mset.h"
#include "names.h"
#include "redirect.h"
#include "shellvar.h"
#include "shrink.h"
#include "stand.h"
#include "times.h"
#include "trap.h"
#include "vt52.h"


/* internal texts
 */
#define NlsLocalSection "M.mglobal"
enum NlsLocalText{
T_ILLPAR	/*Gebrauch: mupfel [[+-][acefhiklnrstuvxSCG]] [parameter]\n*/
};

unsigned long DirtyDrives;

static MGLOBAL firstMGlobal;

MGLOBAL *InitMGlobal (int argc, char **argv, int gem_allowed,
	int is_system_call, unsigned long stack_limit, BASPAG *my_base,
	int first_mglobal)
{
	MGLOBAL *M;
	short memory_mode = (PROT_PRIV|MAL_PREFALT);

/* fprintf (stderr, "entering Initmglobal\n"); */
	
	if (!first_mglobal)
	{
		M = Malloc (sizeof (MGLOBAL));
		if (!M)
			return NULL;
	}
	else
	{
		M = &firstMGlobal;
#if GEMINI
		memory_mode = (PROT_READ|MAL_PREFALT);
#endif
	}

	memset (M, 0, sizeof (MGLOBAL));
	allocinit (&M->alloc, memory_mode);
	
	M->stack_limit = stack_limit;
	M->shell_flags.system_call = is_system_call;
	M->shell_flags.dont_use_gem = !gem_allowed;

	InitTimes (M);
	
	if (!InitRedirection (M, MERGED))
	{
		dprintf ("Initialisierung der Redirection fehlgeschlagen!\n");
		return FALSE;
	}

	if (!InitCharIO (M))
	{
		dprintf ("Konnte Terminal nicht initialisieren!\n");
		ExitRedirection (M);
		return FALSE;
	}
	
	/* šbernehme das Environment
	 */
	if (!InitNames (M, my_base ? my_base->p_env : NULL))
	{
		dprintf ("Konnte Environment nicht bernehmen!\n");
		ExitCharIO (M, NULL);
		ExitRedirection (M);
		return FALSE;
	}

	InitStdErr (M);

	/* Setze die Optionen und ShellVariablen der Kommandozeile
	 */
	if (!SetOptionsAndVars (M, argc, argv, &M->argc, &M->argv))
	{
		eprintf (M, NlsStr(T_ILLPAR), argv[0]);
		ExitNames (M);
		ExitCharIO (M, NULL);
		ExitRedirection (M);
		return FALSE;
	}
	
	if (!InitCurrentPath (M))
	{
		eprintf (M, "Konnte aktuellen Pfad nicht einstellen!\n");
		DeinstallShellVars (M);
		ExitNames (M);
		ExitCharIO (M, NULL);
		ExitRedirection (M);
		return FALSE;
	}
	
	if (!InitHistory (M))
	{
		eprintf (M, "Konnte History nicht initialisieren!\n");
		ExitCurrentPath (M);
		DeinstallShellVars (M);
		ExitNames (M);
		ExitCharIO (M, NULL);
		ExitRedirection (M);
		return FALSE;
	}
	
	if (!InitInternal (M))
	{
		eprintf (M, "Konnte interne Kommandos nicht initialisieren!\n");
		ExitHistory (M);
		ExitCurrentPath (M);
		DeinstallShellVars (M);
		ExitNames (M);
		ExitCharIO (M, NULL);
		ExitRedirection (M);
		return FALSE;
	}
	
	InitAlias (M);
	InitHash (M);
	InitTrap (M);
	InitShrink (M);
	
	if ((argv == NULL) || (argv[0][0] == '\0'))
	{
#if STANDALONE
		M->program_name = (gem_allowed)? "mupfel.app" : "mupfel.ttp";
#else
		M->program_name = "gemini.app";
#endif
	}
	else
		M->program_name = argv[0];
	
	M->real_pid = Pgetpid ();
	if (M->real_pid == EINVFN)
		M->real_pid = (long)getactpd();
	M->our_pid = M->real_pid;
	M->last_async_pid = -1;
		
	Fcntl (-1, (long)&M->jobs.term_pgrp, TIOCGPGRP);
		
	return M;
}

void ExitMGlobal (MGLOBAL *M, int free_mglobal)
{
	SignalTrap (M, SIGEXIT);
	
	ExitShrink (M);
	ExitTrap (M, NULL);
	ExitAlias (M);
	ExitHash (M);
	ExitDirStack (M);
	ExitUsersGroups (M);
	ExitInternal (M);
	ExitHistory (M);
	ExitCurrentPath (M);
	DeinstallShellVars (M);
	ExitNames (M);
	ExitCharIO (M, NULL);
	ExitRedirection (M);
	
	alloccheck (&M->alloc);
	allocexit (&M->alloc);
	if (free_mglobal)
		Mfree (M);
}

int RestoreMGlobalInformations (MGLOBAL *M)
{
	char path[MAX_PATH_LEN];
	int broken;
	
	sprintf (path, "%c:%s", M->current_drive, M->current_dir);
	return ChangeDir (M, path, FALSE, &broken);
}

void FreeMadeShellLevel (MGLOBAL *M, MGLOBAL *new)
{
	SignalTrap (new, SIGEXIT);

	ExitShrink (new);
	ExitTrap (new, M);
	ExitAlias (new);
	ExitHash (new);
	ExitDirStack (new);
	ExitUsersGroups (new);
	ExitInternal (new);
	ExitHistory (new);
	ExitCurrentPath (new);
	DeinstallShellVars (new);
	ExitNames (new);
	ExitCharIO (new, M);
	if (M->real_pid != new->real_pid)
		ExitRedirection (new);
		
	alloccheck (&new->alloc);
	allocexit (&new->alloc);
	
	Mfree (new);

	if (M != NULL)
		RestoreMGlobalInformations (M);
}


MGLOBAL *
MakeNewShellLevel (MGLOBAL *M)
{
	MGLOBAL *new;
	int retcode;
	
	new = Malloc (sizeof (MGLOBAL));
	if (!new) return NULL;
	
	/* šbernehme die Werte aus <M> */
	
	memcpy (new, M, sizeof (MGLOBAL));
	new->break_buffer_set = 0;
	new->shell_flags.interactive = FALSE;

	/* stevie: dies ist selbst unter MiNT ein Problem, da wir
	keinen neuen Prozež forken, aber in der Bourne-Shell hier
	ein anderer Wert steht. */

	retcode = Pgetpid ();
	if (retcode >= 0)
		new->real_pid = new->our_pid = retcode;

	/* Neue ALLOCINFO initialisieren */
	allocinit (&new->alloc, (PROT_PRIV|MAL_PREFALT));

	/* In einem neuen Prozež brauchen wir neue Handles
	fr die Standardkan„le */

	if (M->real_pid != new->real_pid)
		ReInitRedirection (M, new);
	else
		new->our_pid = M->our_pid + 1;

	InitTimes (new);
	
	/* Initialisiere neue TTYINFO und andere Sachen */
	
	if (!ReInitCharIO (M, new)
		|| !ReInitNames (M, new)
		|| !ReInitShellVars (M, new)
		|| !ReInitDirStack (M, new)
#if GEMINI
		/* Wir k”nnen in Gemini nicht einfach den alten Pfad erben,
		da ein Shell-script auch von gemini in dessen aktuellem
		Verzeichnis gestartet werden kann. Dann muž aber auch
		PWD stimmen. */

		|| !InitCurrentPath (new)
#else
		|| !ReInitCurrentPath (M, new)
#endif
		|| !ReInitHistory (M, new)
		|| !ReInitHash (M, new)
		|| !ReInitAlias (M, new)
		|| !ReInitInternal (M, new))
	{
		FreeMadeShellLevel (M, new);
		return NULL;
	}
	
	ReInitTrap (M, new);
	InitShrink (new);
	
	return new;
}


int
InitGEM (MGLOBAL *M)
{
	WORD workin[11] = { 1,1,1,1,1,1,1,1,1,1,2 };
	WORD workout[57];
	WORD dummy;

	if (M->shell_flags.gem_initialized) return TRUE;

	_GemParBlk.global[0] = 0;
	M->ap_id = appl_init ();
	
	if (M->ap_id < 0 || _GemParBlk.global[0] == 0)
		return FALSE;
	
	strcpy (M->runner, "RUNNER2.APP");
	if (!shel_find (M->runner))
		M->runner[0] = '\0';
	
	workin[0] = Getrez () + 2;
	M->vdi_handle = graf_handle (&M->char_w, &M->char_h, 
		&M->box_w, &M->box_h);

	v_opnvwk (workin, &M->vdi_handle, workout);
	if (M->vdi_handle <= 0)
	{
		appl_exit ();
		return FALSE;
	}

	vst_alignment (M->vdi_handle, 0, 5, &dummy, &dummy);
	
	wind_get (0, WF_WORKXYWH, &M->desk_x, &M->desk_y, &M->desk_w, 
		&M->desk_h);

	MupfelWindUpdate (BEG_UPDATE);

#if STANDALONE
	if (_GemParBlk.global[1] == 1)
	{
		MupfelGrafMouse (M_OFF);
		clearscreen (M);
		cursor_on (M);
	}
	else
		MupfelWindUpdate (BEG_UPDATE);
#endif

	M->shell_flags.gem_initialized = 1;
	return TRUE;
}

void ExitGEM (MGLOBAL *M)
{
	if (!M->shell_flags.gem_initialized)
		return;
	
	if (!M->shell_flags.doing_overlay)
	{
		static COMMAND shtail = { 0, "" };
		
		if (_GemParBlk.global[0] >= 0x104)
		{
			shel_write (0, 1, 1, "", (char *)&shtail);
		}
		else
		{
			char command[MAX_PATH_LEN];
			
			shtail.length = 2;
			strcpy( shtail.command_tail, "-q" );
			
			/* Wir haben kein AES, das bei shel_write ein L”schen
			 * des Shell-Buffers erlaubt. Also versuchen wir
			 * EXIT.PRG zu starten.
			 */
			strcpy (command, "EXIT.PRG");
			if (!shel_find (command))
				strcpy (command, "EXIT.PRG");
			
			shel_write (1, 1, 1, command, (char *)&shtail);
		}
	}

	v_clsvwk (M->vdi_handle);

#if STANDALONE
	cursor_off (M);
	MupfelGrafMouse (M_ON);

	if (_GemParBlk.global[1] != 1)	
		MupfelWindUpdate (BEG_UPDATE);
#endif
	MupfelWindUpdate (END_UPDATE);

	appl_exit ();
	
	M->shell_flags.gem_initialized = 0;
}
