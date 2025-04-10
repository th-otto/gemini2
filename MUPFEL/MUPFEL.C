/*
 * @(#) Mupfel\mupfel.c
 * @(#) Stefan Eissing, 23. Oktober 1994
 *
 * jr 18.5.96
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <aes.h>
#include <tos.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\cookie.h"
#include "..\common\fileutil.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "exec.h"
#include "exectree.h"
#include "gemutil.h"
#include "instsys.h"
#include "ioutil.h"
#include "mdef.h"
#include "names.h"
#include "nameutil.h"
#include "parsinpt.h"
#include "parsutil.h"
#include "shellvar.h"
#include "stand.h"
#include "subst.h"		/* jr */
#if GEMINI
#include "..\venus\gemini.h"
#endif

/* internal texts
 */
#define NlsLocalSection "M.mupfel"
enum NlsLocalText{
M_CANTSET,	/*%s: kann Variablen nicht setzen\n*/
M_APPLINIT,	/*[3][|Das AES scheint mich nicht zu m”gen!][Abbruch]*/
M_OPENERR,	/*%s: Fehler beim Suchen nach %s\n*/
M_VERSION,	/*1.a2\341*/
};

/* jr: Diese Variablen werden in argvmain ein fr allemal abgefragt
   und exportiert. Sie k”nnen global sein, weil sich die Versions-
   nummern im laufenden Betrieb unm”glich „ndern k”nnen */
   
unsigned long MiNTVersion;
unsigned short GEMDOSVersion, MagiCVersion;
unsigned short TOSVersion, TOSCountry, TOSDate;
SYSHDR *SysBase;

/* jr: what soll ja schliežlich was melden */

const char
*sccsid (void)
{
	return "@(#)Mupfel "MUPFELVERSION", Copyright (c) Eissing, Steffens & Reschke "__DATE__;
}


#if GEMINI
int StartGeminiProgram (void *MupfelInfo, const char *full_name, 
	char *command, int show_path, int gem_start, 
	int not_in_window, int wait_key, int overlay, int single_mode,
	int parallel)
{
	MGLOBAL *M = MupfelInfo;
	EXECPARM parms;
	ARGVINFO A;
	int broken = FALSE;
	
	GrafMouse (M_OFF, NULL);	
	if (!InitExecParameter (M, &parms, full_name, parallel))
		return -1;
	
	parms.show_full_path = (show_path != 0);
	if (not_in_window != PARM_EGAL)
		parms.in_window = !not_in_window;

	parms.wait_key = (wait_key != 0);
	parms.overlay |= (overlay != 0);
	parms.is_gem &= (gem_start != 0);
	parms.single_mode |= (single_mode != 0);
	
	/* Wenn ein Programm explizit in der Console gestartet werden
	 * soll, dann tu das auch.
	 */
	if (parms.in_window == PARM_YES)
		parallel = FALSE;

	if (_GemParBlk.global[1] == 1)
	{
		parms.single_mode = FALSE;
		if (parms.is_gem)
			parms.in_window = PARM_NO;
	}
	else if (!parms.single_mode)
	{
		parms.in_window = PARM_YES;
		if (parms.is_gem)
		{
			parms.cursor_on = 0;
			parms.wait_key = 0;
			parms.mouse_on = 1;
		}
	}
		
	if (!MakeGenericArgv (&M->alloc, &A, FALSE, 0, NULL))
	{
		GrafMouse (M_ON, NULL);	
		return -1;
	}
	
	broken = !StoreInArgv (&A, full_name);
	
	while (!broken && command && command[0])
	{
		char *cp = strchr (command, ' ');
		
		if (cp)
			*cp++ = '\0';
			
		broken = !StoreInArgv (&A, command);
		
		if (!broken && cp)
		{
			while (*cp && *cp == ' ')
				++cp;
		}
		
		command = cp;
	}	
		
	if (broken)
	{
		FreeArgv (&A);
		GrafMouse (M_ON, NULL);	
		return -1;
	}	
	
	M->checkIfConsoleIsOpen = TRUE;
	ExecArgv (M, A.argc, A.argv, FALSE, NULL, CMD_ALL, &parms, FALSE,
		parallel);
	
	GrafMouse (M_ON, NULL);	

	FreeArgv (&A);
	return M->exit_value;
}

int DoingOverlay (MGLOBAL *M)
{
	return M->shell_flags.doing_overlay;
}
#endif


#define DEF_PROFILE		"profile"
#define DEF_RUNCOMMAND	"mupfel.rc"


static int execProfile (MGLOBAL *M, const char *script_name,
	const char *dir_name, int try_elsewhere)
{
	const char *home;
	char file[MAX_PATH_LEN];
	int got_file = FALSE;
	
	home = dir_name;
	
	if ((home != NULL) && *home)
	{
		int broken = FALSE;
		
		strcpy (file, home);
		AddFileName (file, script_name);
		got_file = access (file, A_READ, &broken);
		if (broken)
		{
			eprintf (M, NlsStr (M_OPENERR), M->argv[0], file);
			M->exit_shell = TRUE;
			M->exit_value = 127;
			return FALSE;
		}
	}
	
	if (!got_file)
	{
		if (!try_elsewhere)
			return FALSE;
	
		strcpy (file, script_name);
		if (!M->shell_flags.dont_use_gem && !shel_find (file))
			return FALSE;
	}

	{	
		PARSINPUTINFO PSave;
		long ret = ParsFromFile (M, &PSave, file);

		if (ret == E_OK)
		{
			ParsAndExec (M);
			ParsRestoreInput (M, &PSave);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}

static int execStartupFiles (MGLOBAL *M)
{
	if (GetEnv (M, "MUPFEL_PROFILE_READ"))
	{
		if (M->shell_flags.interactive)
			execProfile (M, DEF_RUNCOMMAND, GetEnv (M, "HOME"), TRUE);
	}
	else
	{
		(execProfile (M, DEF_PROFILE, GetEnv (M, "ETCDIR"), FALSE)
		|| execProfile (M, DEF_PROFILE, GetEnv (M, "ETC"), FALSE)
		|| execProfile (M, DEF_PROFILE, "\\ETC\\", FALSE));
		
		execProfile (M, DEF_PROFILE, GetEnv (M, "HOME"), TRUE);
		PutEnv (M, "MUPFEL_PROFILE_READ=");
	}

	return TRUE;
}


#if 0
/* jr: ENV bearbeiten

   POSIX 1003.2a/D8, 3.5.3
   
   ENV: This variable, when the shell is invoked, shall be
   subjected to parameter expansion (see 3.6.2) by the shell and
   the resulting value shall be used as a pathname of a file
   containing shell commands to execute in the current environment.
   The file need not be executable. If the expanded value of ENV
   is not an absolute filename, the results are unspecified. ENV
   shall be ignored if the user's real and effective user IDs
   or real and effective group IDs are different. [...]
*/

static int execEnvStartup (MGLOBAL *M)
{
	char *env_var = GetEnv (M, "ENV");
	const char *expanded;
	int uid, gid, euid, egid;
	
	uid = Pgetuid ();
	euid = Pgeteuid (); if (euid == EINVFN) euid = uid;
	gid = Pgetgid ();
	egid = Pgetegid (); if (egid == EINVFN) egid = gid;
	
	if (uid != euid || gid != egid) return FALSE;
	
	if (!env_var) return FALSE;
	
	expanded = StringVarSubst (M, env_var);
	if (!expanded) return FALSE;

mprintf (M, "exp: %s\n", expanded);
	
	mfree (&M->alloc, expanded);
	return FALSE;
}
#endif

int StartShell (int argc, char **argv, int gem_allowed, 
	int is_system_call, unsigned long stack_limit, BASPAG *my_base,
	int called_from_main)
{
	MGLOBAL *M;
	int exit_value;

	M = InitMGlobal (argc, argv, gem_allowed, is_system_call,
			stack_limit, my_base, called_from_main);
			
	if (!M) return 1;

	if (M->argc && !InstallShellVars (M, M->argv + 1, M->argc - 1, 
			TRUE, TRUE))
	{
		eprintf (M, NlsStr (M_CANTSET), M->program_name);
		M->exit_shell = TRUE;
		M->exit_value = 1;
	}

	if (!M->shell_flags.dont_use_gem 
		&& !M->shell_flags.gem_initialized)
	{
		/* GEM-Benutzung ist erlaubt (wahrscheinleich wg. "+G", aber
		 * wir haben das GEM noch nicht initialisiert.
		 */
		M->shell_flags.dont_use_gem = !InitGEM (M);
#if GEMINI
		if (M->shell_flags.dont_use_gem)
		{
			if (M->ap_id < 0)
				form_alert (1, NlsStr (M_APPLINIT));
			else if (_GemParBlk.global[0] == 0)
				dprintf ("GEMINI l„uft nicht im AUTO-Ordner!\n");
			
			M->exit_shell = TRUE;
			M->exit_value = 1;
		}
		else
		{
			/* Wir haben das GEM richtig initialisieren k”nnen; nun
			   versuchen wir die Sachen in Venus zu installieren,
			   die wir vor der Ausfhrung des PROFILE brauchen. */

			if (!EarlyInitGemini (M, &M->alloc, M->vdi_handle,
				&DirtyDrives, M->tty.fkeys))
			{
				M->exit_shell = TRUE;
				M->exit_value = 1;
			}
		}
#endif
	}
	
	/* Wir initialisieren erst jetzt ROWS und COLUMNS, da wir erst
	   hier sicher sein k”nnen, dass wir evtl. das VDI dazu benutzen
	   k”nnen. */

	InitRowsAndColumns (M);
	
	if (!M->exit_shell)
	{
		if (M->shell_flags.interactive)
		{
			const char *shell;
	
			shell = GetEnv (M, "SHELL");
			if (shell == NULL || !shell[0])
			{
				char tmp[MAX_PATH_LEN + 10];
				char shell_name[MAX_PATH_LEN];
				
				/* $SHELL ist immer noch nicht vorhanden. Dann mssen
				 * wir es eben selbst setzen.
				 */
				if (argv[0] && strchr (argv[0], '\\'))
					strcpy (shell_name, argv[0]);
				else if (!M->shell_flags.dont_use_gem)
				{
					strcpy (shell_name, "MUPFEL.TTP");
					shel_find (shell_name);
				}
				
				if (shell_name[0])
				{	
					sprintf (tmp, "SHELL=%s", shell_name);
					PutEnv (M, tmp);
				}
			}
	
			execStartupFiles (M);
	
#if GEMINI
			/* Wenn Gemini noch nicht beendet wurde, dann versuchen wir
			 * jetzt, wo das PROFILE gelesen wurde, auch den Rest der
			 * Venus zu initialisieren.
			 */
			if (!M->exit_shell && !M->shell_flags.dont_use_gem 
				&& !LateInitGemini ())
			{
				M->exit_shell = TRUE;
				M->exit_value = 1;
			}
#endif
		}
		else
		{
#if 0
			/* jr: $ENV beachten */
			if (! execEnvStartup (M))
#endif
				execStartupFiles (M);
		}
	}
	
	if (M->shell_flags.restricted)
	{
		/* Setze PATH auf readonly
		 */
		ReadonlyName (M, "PATH", NULL);
	}
	
	/* Wenn noch Argumente in der Kommandozeile stehen, dann sollen
	 * wir entweder ein Script ausfhren, oder ein Kommando
	 * ausfhren.
	 */
	if (M->argc && !M->exit_shell)
	{
		PARSINPUTINFO PSave;

		/* Dieses Flag gibt an, ob es ein Kommando ist (-c)
		 */
		if (M->shell_flags.exec_argv)
		{
			ParsFromString (M, &PSave, M->argv[0]);
			ParsAndExec (M);
			ParsRestoreInput (M, &PSave);
		}
		else
		{
			long ret;

			/* Programmname ist der Name der Datei, von der wir
			 * die Kommandos lesen.
			 */
			
			M->program_name = M->argv[0];
			
			ret = ParsFromFile (M, &PSave, M->argv[0]);
			
			if (ret == E_OK)
			{
				ParsAndExec (M);
				ParsRestoreInput (M, &PSave);
			}
			else
			{
				eprintf (M, "mupfel: `%s' -- %s\n", M->argv[0],
					StrError (ret));
			}
		}
	
	}
	else if (!M->exit_shell)
	{
		PARSINPUTINFO PSave;

		ParsFromStdin (M, &PSave);
		ParsAndExec (M);
		ParsRestoreInput (M, &PSave);
	}
	
	DeinstallShellVars (M);
#if GEMINI
	if (!M->shell_flags.dont_use_gem)
		ExitGemini (M->shell_flags.doing_overlay);
#endif
	ExitGEM (M);

	exit_value = M->exit_value;
	ExitMGlobal (M, !called_from_main);
	
	return exit_value;
}

/* jr: Versionsnummern ermitteln */

#define _SYSBASE		((SYSHDR **)0x4f2L)
#define Ssystem(a,b,c)	gemdos(0x154,(int)a,(long)b,(long)c)	

static long
_getsyshdr (void)
{
	return (long) *_SYSBASE;
}

static SYSHDR *
getsyshdr (void)
{
	SYSHDR *sysbase = (SYSHDR *) Ssystem (10, 0x4f2, NULL);
	
	if (sysbase == (SYSHDR *)EINVFN)
		sysbase = (SYSHDR *) Supexec (_getsyshdr);

	return sysbase;
}

static unsigned short
get_tos_version (unsigned short *country, unsigned short *sysdate)
{
	if (CookiePresent ('SVAR', 0)) return 0;
	
	*sysdate = SysBase->os_gendatg;
	*country = SysBase->os_base->os_palmode >> 1;

	return SysBase->os_version;
}

typedef struct
{
	long	magic;		 /* muž $87654321 sein		*/
	void	*membot;		/* Ende der AES- Variablen	*/
	void	*aes_start;		/* Startadresse		*/
	long	magic2;		/* ist 'MAGX'			*/
	long	date;			/* Erstelldatum ttmmjjjj	*/
	void	(*chgres)(int res, int txt);	/* Aufl”sung „ndern	*/
	long 	(**shel_vector)(void);	/* residentes Desktop		*/
	char	*aes_bootdrv;		/* von hieraus wurde gebootet	*/
	int		*vdi_device;		/* vom AES benutzter VDI-Treiber */
	void	*reservd1;
	void	*reservd2;
	void	*reservd3;
	int		version, release;
} AESVARS;

static int
get_magic_version (void)
{
	if (CookiePresent ('MagX', NULL))
		return ((AESVARS *)SysBase->os_magic)->version;
	
	return 0;		
}

int argvmain (int argc, char **argv)
{
	int retcode, gem_allowed;
	extern unsigned long _StkLim;

#if STANDALONE
	{
		static char tmp [MAX_PATH_LEN];
		
		if (getenv ("MUPFEL_MSG"))
			strncpy (tmp, getenv("MUPFEL_MSG"), MAX_PATH_LEN);
		else
			strcpy (tmp, pgmname".msg");
		NlsInit (tmp, NlsDefaultSection, malloc, free);
		
		if (strcmp (MUPFELVERSION, NlsStr (M_VERSION)))
		{
			fprintf (stderr, "Wrong version of `"pgmname".msg' "
				"(is `%s', should be `%s')!\n",
				NlsStr (M_VERSION), MUPFELVERSION);
			exit (2);
		}
	}
#endif

	if (argc < 1)
	{
		argc = 1;
		argv[0] = "mupfel";
	}

	/* setze MINTVersion, GEMDOSVersion, MagiCVersion, TOSVersion */	
	{
		short dosversion = Sversion ();
		
		SysBase = getsyshdr ();				
		CookiePresent ('MiNT', (long *)&MiNTVersion);
		GEMDOSVersion = ((dosversion & 0xff) << 8) | (dosversion >> 8);
		TOSVersion = get_tos_version (&TOSCountry, &TOSDate);
		MagiCVersion = get_magic_version ();
	}
	
	if (!MiNTVersion) InitSystemCall ();
	
	/* jr: kann man immer machen */
	Pdomain (1);
		
	gem_allowed = !TTP_VERSION;
	if (argv[0] && argv[0][0])
	{
		char *cp;
		
		cp = strrchr (argv[0], '.');
		if (cp != NULL)
		{
#if TTP_VERSION
			gem_allowed = (!strnicmp (cp, ".APP", 4)
				|| !strnicmp (cp, ".GTP", 4));
#else
			gem_allowed = strnicmp (cp, ".TTP", 4);
#endif
		}
	}

/*	if (!gem_allowed)
		gem_allowed = CookiePresent ('MagX', NULL);
*/		

	retcode = StartShell (argc, argv, gem_allowed, FALSE, 
		_StkLim + 1500, _BasPag, TRUE);

	if (!MiNTVersion)
		ExitSystemCall ();

#if STANDALONE
	NlsExit ();
#endif

	return retcode;
}


int SystemCall (char *command, unsigned long stack_limit)
{
	char *argv[4];
	BASPAG *akt_base;
	
	argv[0] = "mupfel";
	argv[1] = "-c";
	argv[2] = command;
	argv[3] = NULL;
	akt_base = getactpd ();
	
	return StartShell (3, argv, FALSE, TRUE, stack_limit, akt_base,
		FALSE);
}
