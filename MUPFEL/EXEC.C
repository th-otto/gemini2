/*
 * @(#) Mupfel\Exec.c
 * @(#) Gereon Steffens & Stefan Eissing, 20. November 1994
 *
 * -  execute external command
 *
 * jr 22.4.95
 */

#include <ctype.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vdi.h>
#include <aes.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\cookie.h"
#include "..\common\genargv.h"
#include "..\common\fileutil.h"
#include "..\common\memfile.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "exec.h"
#include "gemutil.h"
#include "ioutil.h"
#include "mcd.h"
#include "mdef.h"
#include "names.h"
#include "nameutil.h"
#include "screen.h"
#include "stand.h"
#include "vt52.h"
#if GEMINI
#include "..\venus\gemini.h"
#endif


/* internal texts
 */
#define NlsLocalSection "M.exec"
enum NlsLocalText{
EX_USEXARG,	/*Zu viele Argumente, xArg/ARGV wird benutzt\n*/
EX_BADCD,	/*Kann nicht ins Verzeichnis `%s' wechseln\n*/
EX_OOPS,	/*Oh!*/
EX_PRESSCR,	/*DrÅcken Sie RETURN...*/
EX_CRASH,	/*`%s' ist abgestÅrzt! Weitermachen kann gefÑhrlich sein.*/
EX_OK,		/*Weiter*/
EX_REBOOT,	/*Neustart*/
EX_NORUNNER,	/*Kann `runner2.app' nicht finden\n*/
EX_RUNNERNOTFOUND,	/*`%s' ist nicht vorhanden. Bitte kopieren Sie `runner2.app' dorthin.\n*/
EX_NOTMPFILE,	/*Konnte keine temporÑre Datei fÅr die ParameterÅbergabe anlegen.\n*/
EX_NOSINGLE,	/*Kann Programm nicht im Single-Modus starten!\n*/
T_NOGEM,		/*GEM-Programme kînnen nicht gestartet werden!\n*/
};

#define ARGV_WARNING	0
#define USE_XARG		0

typedef struct
{
	long xarg_magic;
	int xargc;
	const char **xargv;
	const char *xiovector;
	BASPAG *xparent;
} XARG;

typedef struct execinfo_struct
{
	struct execinfo_struct *next;
	
	/* Pointer zur globalen Shell-Struktur */
	MGLOBAL *M;
	
#if USE_XARG
	/* Xarg-Struktur, wenn xArg fÅr den Aufruf gemacht wird. */
	XARG xarg;
#endif
	
	/* Argumente fÅr das aufgerufene Programm */
	int argc;
	const char **argv;
	
	/* Voller Name des Programms (kompletter Pfad) */
	char full_name[MAX_PATH_LEN];
	
	/* Parameter fÅr den Start von full_name */
	EXECPARM parms;
	
	/* Wie der Schirm bei diesem Kommando aussehen soll */
	SCREENINFO screen;
	
	/* COMMAND-Struktur fÅr den Start von full_name */
	COMMAND command_line;
	
	/* Pfad, in dem full_name gestartet wurde. */
	char start_path[MAX_PATH_LEN];
	
	/* Was als nÑchstes zu starten ist. */
	char next_command[MAX_PATH_LEN];
	COMMAND next_command_line;
	
	/* Variable, die bei einem schweren Fehler auf TRUE gesetzt wird. */
	int broken;	
	
} EXECINFO;

static int
isGemProgram (MGLOBAL *M, const char *extension, int *broken)
{
	const char *env;
	
	if ((env = GetEnv(M, "GEMSUFFIX")) == NULL)	
	{
		return !stricmp (extension, "PRG") 
			|| !stricmp (extension, "APP")
			|| !stricmp (extension, "ACC")
			|| !stricmp (extension, "GTP");
	}
	else
	{
		char *gs, *s;
		char *save_token;
		
		gs = StrDup (&M->alloc, env);
		if (!gs)
		{
			*broken = TRUE;
			return FALSE;
		}
		s = StrToken (gs, ";,", &save_token);

		while (s != NULL)
		{
			if (!stricmp (extension, s))
			{
				mfree (&M->alloc, gs);
				return TRUE;
			}
			s = StrToken (NULL, ";,", &save_token);
		}

		mfree (&M->alloc, gs);
	}
	
	return FALSE;
}

static int
getEnvParameter (MGLOBAL *M, EXECPARM *parms, const char *env_name,
	int *broken)
{
	const char *env_value;
	
	if ((env_value = GetEnv (M, env_name)) != NULL)
	{
		char *pv, *pv2 = StrDup (&M->alloc, env_value);
		char *save_token;
		
		if (!pv2)
		{
			*broken = TRUE;
			return FALSE;
		}
		
		pv = StrToken (pv2, ";,", &save_token);
		while (pv != NULL)
		{
			if (strlen (pv) > 2 && pv[1] == ':')
			{
				unsigned int val, gemval;
				
				val = toupper (pv[2]) == 'Y';
				gemval = val? !M->shell_flags.dont_use_gem : 0;
				
				switch (toupper (pv[0]))
				{
					case 'M':
						parms->mouse_on = gemval;
						break;
					case 'C':
						parms->cursor_on = val;
						break;
					case 'B':
						parms->grey_back = gemval;
						break;
					case 'S':
						parms->aes_args = gemval;
						break;
					case 'D':
						parms->ch_dir = val;
						break;
					case 'W':
						parms->in_window = val? PARM_YES : PARM_NO;
						break;
					case 'X':
						parms->ex_arg = val? PARM_YES : PARM_NO;
						parms->silent_xarg = TRUE;
						break;
					case 'K':
						parms->wait_key = val;
						break;
					case 'O':
						parms->overlay = gemval;
						break;
					case 'L':
						parms->cursor_blink = val;
						break;
					case 'I':
						parms->single_mode = gemval;
						break;
				}
			}
			pv = StrToken (NULL, ";,", &save_token);
		}
		
		mfree (&M->alloc, pv2);
		return TRUE;
	}
	
	return TRUE;
}

int
InitExecParameter (MGLOBAL *M, EXECPARM *parms, const char *full_name,
	int parallel)
{
	const char *filename, *extension;
	const char *env_default = "TOSDEFAULT";
	int broken = FALSE;
	
	memset (parms, 0, sizeof (EXECPARM));
	parms->show_full_path = 1;
	
	filename = strrchr (full_name, '\\');
	if (!filename)
		filename = full_name;
	else
		++filename;
		
	extension = Extension (filename);
	if (extension)
	{
		parms->is_gem = isGemProgram (M, extension, &broken);
		if (parms->is_gem)
		{
			parms->is_acc = !stricmp ("ACC", extension);
			parms->is_app = !stricmp ("APP", extension);
			env_default = parms->is_app? "APPDEFAULT" : "GEMDEFAULT";
		}
		else if (broken)
			return FALSE;
		else if (!stricmp ("MUP", extension))
			env_default = "MUPDEFAULT";
	}
	
	if (M->shell_flags.dont_use_gem && parms->is_gem
		&& MagiCVersion)
	{
		/* Wir dÅrfen kein GEM benutzen, haben aber ein GEM-Programm
		 * und laufen unter MagiC. Dann tun wir so, als ob es sich
		 * um ein normales TOS-Programm handelt, da MagiC das
		 * verkraftet.
		 */
		parms->is_gem = parms->is_app = parms->is_acc = 0;
	}
	
	if (parms->is_gem)
	{
		parms->mouse_on = TRUE;
		parms->grey_back = TRUE;
		parms->aes_args = TRUE;
		parms->ch_dir = TRUE;
		parms->in_window = 
			((_GemParBlk.global[1] == 1 || parms->single_mode)
				&& !parms->is_acc) ? PARM_NO : PARM_YES;
	}
	else
	{
		parms->cursor_on = TRUE;
		parms->in_window = PARM_EGAL;
	}
	parms->ex_arg = PARM_EGAL;
	
	getEnvParameter (M, parms, env_default, &broken);
	
	if (!broken)
	{
		char *point;
		char env_name[6 + MAX_FILENAME_LEN];
		
		strcpy (env_name, "OPT_");
		strncat (env_name, filename, MAX_FILENAME_LEN);
		if ((point = strchr (env_name, '.')) != NULL)
			*point = '_';
		
		/* Wir suchen nur Parameter fÅr den Dateinamen in
		 * Groûbuchstaben. 
		 */
		strupr (env_name); 
		if (getEnvParameter (M, parms, env_name, &broken)
			&& !parallel 
			&& !parms->is_gem 
			&& (_GemParBlk.global[1] != 1))
		{
			/* Wir starten ein TOS-Programm unter einer GEM-Multi-
			 * tasking Umgebung. Da kînnen wir dieses Progamm nicht
			 * speziell auf dem TOS-Bildschirm starten. Daher lassen
			 * wir die Fenstereinstellungen wie sie sind.
			 */
			parms->in_window = PARM_EGAL;
		}

		return !broken;
	}
	else
		return FALSE;
}

static int
initExecInfo (MGLOBAL *M, EXECINFO *E, const char *full_name,
	int argc, const char **argv, EXECPARM *start_parameter,
	int parallel)
{
	memset (E, 0, sizeof (EXECINFO));
	E->M = M;
	
	strcpy (E->full_name, full_name);
	if (E->full_name[1] == ':')
		E->full_name[0] = toupper (E->full_name[0]);
	E->start_path[0] = '\0';
	
	E->argc = argc;
	E->argv = argv;
	
	if (start_parameter)
	{
		E->parms = *start_parameter;
		return TRUE;
	}
	else
		return InitExecParameter (E->M, &E->parms, E->full_name, 
			parallel);
}

static void
buildCommandLine (EXECINFO *E, COMMAND *c)
{
	E->parms.ex_arg_used = 0;
	memset (c, 0, sizeof (COMMAND));
	strcpy (c->command_tail+1, "\r\n");

	/* Wir versuchen ParameterÅbergabe mittels ARGV nur dann, wenn
	wir Parameter haben, oder es ein TOS-Programm ist. Die meisten 
	Probleme gibt es ja mit GEM-Programmen von MWC */

	if (E->parms.ex_arg == PARM_YES || E->argc > 1 || !E->parms.is_gem)
	{
		size_t remain = 124;
		int i;
		
		/* Kopiere soviel wie geht in die Kommandozeile */
		for (i = 1; 
			(i < E->argc) && (strlen (E->argv[i]) + 1 < remain);
			 ++i)
		{
			if (i > 1)
				strcat (c->command_tail, " ");

			/* jr: pass empty params as '' */
			strcat (c->command_tail, E->argv[i][0] ? E->argv[i] : "''");
			remain = 123 - strlen (c->command_tail);
		}
		
		/* Wenn i < argc ist, dann paûte nicht alles hinein
		 */
		if (i < E->argc)
		{
#if ARGV_WARNING
			/* Wir mÅssen xArg machen, wenn wir dÅrfen. Auf jeden
			 * Fall dÅrfen wir keine unvollstÑndige Zeile
			 * Åbergeben.
			 */
			if (!E->M->shell_flags.system_call 
				&& !E->parms.silent_xarg)
			{
				eprintf(E->M, EX_USEXARG);
			}
#endif /* ARGV_WARNING */
			
			memset (c, 0, sizeof (COMMAND));
		}
		
		c->length = (char)strlen (c->command_tail);
		
		if ((E->parms.ex_arg == PARM_YES)
			|| ((i < E->argc) && (E->parms.ex_arg != PARM_NO)))
		{
			/* Wir machen ARGV-öbergabe
			 */
			c->length = (char)127;
			E->parms.ex_arg_used = 1;
		}
	}
}


#if USE_XARG
static int initXarg (EXECINFO *E)
{
	char xenv[32];

	E->xarg.xarg_magic = 'xArg';
	E->xarg.xargc = E->argc;
	E->xarg.xargv = E->argv;
	E->xarg.xiovector = NULL;
	E->xarg.xparent = getactpd ();
	sprintf (xenv, "xArg=%08lX", (ulong)&E->xarg);

	return PutEnv (E->M, xenv);
}
#endif


static void
initStartPath (MGLOBAL *M, char *start_path, const char *full_name,
	int change_directory)
{
	(void)M;
	
	if (change_directory)
	{
		char *tmp;

		strcpy (start_path, full_name);
		tmp = strrchr (start_path, '\\');
		if (tmp)
			tmp[1] = '\0';
		else
			*start_path = '\0';
	}
	else
	{
		size_t len;
		
		start_path[0] = Dgetdrv () + 'A';
		start_path[1] = ':';
		start_path[2] = '\0';
		Dgetpath (&start_path[2], 0);
		len = strlen (start_path);
		if (start_path[len - 1] != '\\')
			strcat (start_path, "\\");
	}
}


int
SetExecParameter (MGLOBAL *M, EXECPARM *parms, SCREENINFO *screen,
	const char *full_name, char *start_path, int *broken)
{
#if USE_XARG
	if (E->argc && E->argv)
		initXarg (E);
#endif

	screen->mouse_on = parms->mouse_on;
	screen->cursor_on = parms->cursor_on;
	screen->cursor_blink = parms->cursor_blink;
	screen->grey_back = parms->grey_back;
	screen->window_open = parms->in_window;
	SetScreen (M, screen, full_name, parms->show_full_path);
	
	if (!start_path[0])
		initStartPath (M, start_path, full_name, parms->ch_dir);
	
	if (parms->ch_dir)
	{
		if (*start_path != '\0' 
			&& !ChangeDir (M, start_path, FALSE, broken))
		{
			eprintf (M, NlsStr(EX_BADCD), start_path);
			UnsetScreen (M, screen);
			return FALSE;
		}
	}
	
	return TRUE;
}

void
UnsetExecParameter (MGLOBAL *M, EXECPARM *parms, SCREENINFO *screen,
	int *broken, int parallelStart)
{
	/* remove $IMV2 from env */
#if USE_XARG
	UnsetNameInfo (M, "xArg");
#endif
	
	if (!M->shell_flags.system_call && !parallelStart
		&& parms->wait_key)
	{
		if (!parms->is_gem)
			mprintf(M, "\n%s", NlsStr(EX_PRESSCR));
		inchar (M, FALSE);
	}

	UnsetScreen (M, screen);

	if (parms->ch_dir)
	{
		char tmp[MAX_PATH_LEN];
		sprintf (tmp, "%c:%s", M->current_drive, M->current_dir);
		ChangeDir (M, tmp, FALSE, broken);
	}
}


/* shel_write modes */

#define SHW_IMMED		0							/* PC-GEM 2.x	*/
#define SHW_CHAIN		1							/* TOS		*/
#define SHW_DOS			2							/* PC-GEM 2.x	*/
#define SHW_PARALLEL	100							/* MAGIX		*/
#define SHW_SINGLE		101							/* MAGIX		*/

static int
startOnMultiTOS (MGLOBAL *M, const char *full_name, COMMAND *command,
	char *start_dir, char *exec_env, int isGem)
{
	int retcode, ex_mode, is_graphic, is_cr;
	const char *ext_parm[5];
	
	(void)M;
	memset (ext_parm, 0, sizeof (ext_parm));
	ex_mode = 1;
	is_graphic = isGem;
	
	if (MagiCVersion > 0 && MagiCVersion < 0x300)
	{
		is_cr = SHW_PARALLEL;
	}
	else if (_GemParBlk.global[0] >= 0x0400 || MagiCVersion >= 0x300)
	{
		char *cp;
		
		/* MultiTOS */
		/* Startup mit Default Directory, Environment und vom
		 * AES definierten Modus.
		 */
		ex_mode = (0x0C00) | 0;
		is_cr = MagiCVersion? SHW_PARALLEL : 0;
		
		ext_parm[0] = full_name;
		ext_parm[3] = start_dir;
		ext_parm[4] = exec_env;
		
		/* Das AES will Pfade ohne einen Backslash am Ende
		 * (auûer natÅrlich bei Laufwerken...
		 */
		if (((cp = strrchr (start_dir, '\\')) != NULL)
			&& (cp[1] == '\0') && (cp[-1] != ':'))
		{
			cp[0] = '\0';
		}
	}	
	else
		is_cr = 0;
	
	MupfelWindUpdate (END_UPDATE);
	retcode = shel_write (ex_mode, is_graphic, is_cr, 
		ext_parm[0]? (char *)ext_parm : (char *)full_name, 
		(char *)command);
	MupfelWindUpdate (BEG_UPDATE);

	if (CookiePresent ('MGEM', NULL))
	{
		/* Hier lÑuft wahrscheinlich MultiGEM
		 */
#define Mfork(a,b)		gemdos (112, (a), 0x5aa7, (b))

		retcode = (int) Mfork (full_name, &command);
	}
		
	return retcode;
}


/* Starte das Programm, das in <E> beschrieben ist. */

static long
startProgram (EXECINFO *E, int var_argc, const char **var_argv,
	int overlay_process, int parallel)
{
	COMMAND no_tail = {0, ""};
	char *exec_env;
	size_t env_size;
	long excode = E_OK;
	ARGVINFO args;
	
	/* Leere auf jeden Fall next_command, falls noch etwas vom letzten
	Mal Åbrig geblieben ist. */
	E->next_command[0] = '\0';

	args.argc = E->argc;
	args.argv = E->argv;
	
	exec_env = MakeEnvironment (E->M, E->parms.ex_arg_used, 
		E->argc, E->argv, E->full_name, var_argc, var_argv,
		&env_size);
	
	if (exec_env == NULL)
	{	
		E->broken = TRUE;
		excode = ENSMEM;
	}
	else
	{
		/* Verschiedene Dinge sind zu unterscheiden:
		 * GEM-Programme
		 *  - Start unter normalem TOS
		 *  - Start unter MultiAES ohne MiNT
		 *  - Start unter MultiAES mit MiNT
		 * TOS-Programme
		 */
		if (E->parms.is_gem)
		{
#if !STANDALONE
			if (E->parms.is_acc)
			{
				ARGVINFO accargs;
				
				InitArgv (&accargs);
				accargs.argc = args.argc-1;
				accargs.argv = args.argv+1;
				
				if (StartAccFromMupfel (E->full_name, 
					(accargs.argc > 0)? &accargs : NULL, 
					E->command_line.command_tail))
				{
					goto already_started;
				}
			}
#else
			(void)args;
#endif
			args.argv = NULL;
			if ((_GemParBlk.global[1] != 1)
				&& !E->parms.single_mode && !E->parms.overlay)
			{
				/* MultiAES */
				excode = startOnMultiTOS (E->M, E->full_name, 
					&E->command_line, E->start_path, exec_env, 1);
				goto already_started;
			}
			else
			{
				char applname[MAX_FILENAME_LEN], *cp;
	
				if((cp = strrchr (E->full_name, '\\')) != NULL)
					strncpy (applname, ++cp, MAX_FILENAME_LEN);
				else
					strncpy (applname, E->full_name, MAX_FILENAME_LEN);
		
				if ((cp = strchr (applname, '.')) != NULL)
					*cp = '\0';

				applname[8] = '\0';	/* jr: truncate */
				while (strlen (applname) < 8)
					strcat (applname, " ");
				menu_register (-1, applname);
				
				shel_write (1, 1, E->parms.single_mode? SHW_SINGLE : 
					SHW_CHAIN, E->full_name, E->parms.aes_args ? 
					(char *)&E->command_line : (char *)&no_tail);
	
				MupfelWindUpdate (END_UPDATE);
				appl_exit ();
			}
		}
#if !STANDALONE
		else if (ScreenWindowsAreOpen () && parallel)
		{
			if (GetEnv (E->M, "GEMINI_TERMINAL"))
			{
				/* TOS-Programm! */
				excode = E_OK;
				if (StartProgramInWindow (E->full_name, 
					&E->command_line, E->start_path, exec_env))
					goto already_started;
			}
			else if ((_GemParBlk.global[1] != 1)
				&& !E->parms.single_mode && !E->parms.overlay)
			{
				/* MultiAES */
				excode = startOnMultiTOS (E->M, E->full_name, 
					&E->command_line, E->start_path, exec_env, 0);
				goto already_started;
			}
		}
#else
	(void)parallel;
#endif

		excode = EINVFN;
		if (overlay_process)
		{
			excode = Pexec (200, E->full_name, &E->command_line, 
				exec_env);
		}
		
		if (excode == EINVFN)
		{
#if !STANDALONE
			int wasOpened, oldTopWindow, prepared = FALSE;
			
			if (ScreenWindowsAreOpen() 
				&& E->M->checkIfConsoleIsOpen)
			{
				PrepareInternalStart (&wasOpened, &oldTopWindow, 
					TRUE);
				prepared = TRUE;
				E->M->checkIfConsoleIsOpen = FALSE;
			}
#endif
			excode = Pexec (0, E->full_name, &E->command_line, 
				exec_env);
#if !STANDALONE
			if (prepared)
			{
				PrepareInternalStart (&wasOpened, &oldTopWindow, 
					FALSE);
			}	
#endif
		}

		/* Nach einem Programmstart werden alle Laufwerke als
		 * Dirty markiert, damit Verzeichnisse neu eingelesen
		 * werden kînnen.
		 */
		DirtyDrives |= Dsetdrv (Dgetdrv ());
		
		if (E->parms.is_gem)
		{
			appl_init ();
			MupfelWindUpdate (BEG_UPDATE);

#if STANDALONE		
			menu_register (-1, "MUPFEL  ");
#else
			menu_register (-1, "GEMINI  ");
#endif
			if (excode >= 0L)
			{
				if (!shel_read ((char *)&E->next_command, 
					(char *)&E->next_command_line))
				{
					E->next_command[0] = '\0';
				}
			}
		}

		/*	checkcode (); stevie */
	}

already_started:

	mfree (&E->M->alloc, exec_env);

	if (excode < E_OK)
	{
		eprintf (E->M, "%s: %s\n", E->full_name, StrError (excode));

		if (IsBiosError (excode)) E->M->oserr = (int) excode;
	}
#if NEVER
/* stevie: todo */
	else if ((int)excode == ERROR && pgmcrash())
	{
		/* Pexec()==(int)-1  ->  crash */
		if (alert(STOP_ICON,1,2,EX_CRASH,EX_OK,EX_REBOOT,cmdpath)==2)
			reboot();
			/* NOTREACHED */
		else
			clearcrashflag();
	}
#endif

	return excode;
}


/* Bereite alles fÅr einen Overlay-Start des in <E> angegebenen
   Programms vor. Setze die Flags in <E->M> entsprechend. */
   
static int
prepareOverlay (EXECINFO *E, int var_argc, const char **var_argv)
{
	COMMAND shtail;
	MEMFILEINFO *mp;
	int broken;
	char *exec_env, *parameter_file;
	char line[MAX_PATH_LEN];
	size_t env_size;
	long stat;
	
	if (E->parms.single_mode)
	{
		if (E->M->ap_id != 0)
		{
			eprintf (E->M, NlsStr(EX_NOSINGLE));
			return -1;
		}
			
		shtail.length = 0;
		shtail.command_tail[0] = '\0';
		
		shel_write (1, 1, SHW_SINGLE, E->full_name, 
			E->parms.aes_args ? 
			(char *)&E->command_line : (char *)&shtail);
		
		E->M->shell_flags.doing_overlay = TRUE;
		E->M->exit_shell = TRUE;
	
		return 0;
	}
	
	if (!E->M->runner[0])
	{
		strcpy (E->M->runner, "runner2.app");
		if (!shel_find (E->M->runner))
		{
			if (E->M->start_path)
			{
				strcpy (E->M->runner, E->M->start_path);
				AddFileName (E->M->runner, "runner2.app");
			}
			else
			{
				eprintf (E->M, NlsStr(EX_NORUNNER));
				E->M->runner[0] = '\0';
				return EFILNF;
			}
		}
	}
	
	if (!access (E->M->runner, A_READ, &broken))
	{
		eprintf (E->M, NlsStr(EX_RUNNERNOTFOUND), E->M->runner);
		return EFILNF;
	}

	parameter_file = TmpFileName (E->M);
	if ((parameter_file == NULL) || 
		((mp = mcreate (&E->M->alloc, parameter_file)) == NULL))
	{
		eprintf (E->M, NlsStr(EX_NOTMPFILE));
		return EFILNF;
	}

	memset (&shtail, 0, sizeof (shtail));
	strncpy (shtail.command_tail, parameter_file, sizeof (shtail.command_tail) - 1);
	shtail.length = strlen (shtail.command_tail);
	
	/* Dateikennung
	 */
	stat = mputs (mp, "#!runner2.app");

	/* Name des nachzustartenden Programms
	 */
	if (stat >= 0)
	{
		strcpy (line, E->M->program_name);
		stat = mputs (mp, line);
	}

	/* Parameter fÅr das nachzustartende Programm
	 */
	if (stat >= 0)
		stat = mputs (mp, "");
	
	/* Name des zu startenden Programms
	 */
	if (stat >= 0)
		stat = mputs (mp, E->full_name);
	
	/* Parameter fÅr das zu startende Programm
	 */
	sprintf (line, "%d", E->command_line.length);
	if (stat >= 0)
		stat = mputs (mp, line);
	if (stat >= 0)
		stat = mputs (mp, E->command_line.command_tail);
	
	/* Pfad, in dem das Programm gestartet werden soll
	 */
	initStartPath (E->M, E->start_path, E->full_name, E->parms.ch_dir);
	if (stat >= 0)
		stat = mputs (mp, E->start_path);
	
	/* Starteinstellungen fÅr das Programm
	 */
	line[0] = E->parms.is_gem? '1':'0';
	line[1] = E->parms.wait_key? '1':'0';
	line[2] = 
	line[3] = 
	line[4] = 
	line[5] = 
	line[6] = 
	line[7] = '0';
	line[8] = '\0';
	if (stat >= 0)
		stat = mputs (mp, line);
	
	/* Environment fÅr das zu startende Programm */

	exec_env = MakeEnvironment (E->M, E->parms.ex_arg_used, 
		E->argc, E->argv, E->full_name, var_argc, var_argv,
		&env_size);
	if (exec_env == NULL)
		env_size = 0L;		

	sprintf (line, "%ld", (long)env_size);
	if (stat >= 0)
		stat = mputs (mp, line);
	if (exec_env)
	{
		if (stat >= 0)
			stat = mwrite (mp, env_size, exec_env);
	}
	mfree (&E->M->alloc, exec_env);

	mclose (&E->M->alloc, mp);

	if (stat < 0)
	{
		eprintf (E->M, NlsStr(EX_NOTMPFILE));
		return (int)stat;
	}

	shel_write (1, 1, 1, E->M->runner, (char *)&shtail);

	E->M->shell_flags.doing_overlay = TRUE;
	E->M->exit_shell = TRUE;
	
	return 0;
}


/* Starte das externe Kommando <full_name> mit den Parametern in
   <argv> und gebe den Returnwert davon zurÅck. Wenn ein schwerer 
   Fehler aufgetreten ist, der den Start unmîglich machte, wird 
   <broken> auf TRUE gesezt. */

int
ExecExternal (MGLOBAL *M, const char *full_name, int argc,
	const char **argv, int var_argc, const char **var_argv, 
	EXECPARM *start_parameter, int overlay_process, int parallel,
	int *broken)
{
	EXECINFO *first = NULL;
	int retcode = 0, command_to_exec = TRUE;
	
	while (!*broken && command_to_exec)
	{
		EXECINFO *E;
		
		E = mcalloc (&M->alloc, 1, sizeof (EXECINFO));
		if (!E)
		{
			*broken = TRUE;
			retcode = 1;
			break;
		}

		command_to_exec = FALSE;
		
		if (!initExecInfo (M, E, full_name, argc, argv, 
				start_parameter, parallel)
			|| (M->shell_flags.dont_use_gem && E->parms.is_gem))
		{
			int retcode;
			
			if (!E->broken)
			{
				eprintf (M, NlsStr (T_NOGEM));
				retcode = EPLFMT;
			}
			else
				retcode = 1;
				
			*broken = E->broken;
			mfree (&M->alloc, E);
			return retcode;
		}

		buildCommandLine (E, &E->command_line);
	
		if (!M->shell_flags.system_call 
			&& !M->shell_flags.dont_use_gem
			&& (E->parms.overlay || (E->parms.single_mode 
				&& MagiCVersion != 0)))
		{
			/* Wir haben ein Overlay-Kommando und sind nicht innerhalb
			 * eines system() Calls. Dann mÅssen wir einen Shel_write
			 * machen, um RUNNER nachzustarten und uns beenden.
			 */
			retcode = prepareOverlay (E, var_argc, var_argv);
				
			*broken = E->broken;
			mfree (&M->alloc, E);
			return retcode;
		}
		
		if (!SetExecParameter (M, &E->parms, &E->screen, 
			E->full_name, E->start_path, &E->broken))
		{
			E->broken = TRUE;
			break;
		}

		/* Wir haben jetzt folgende Informationen zusammen:
		   - Das zu startende Programm
		   - COMMAND-Struktur fÅr Pexec()
		   - Die Startparameter fÅr das Programm */

		retcode = (int)startProgram (E, var_argc, var_argv,
			overlay_process, parallel);

		if ((MagiCVersion != 0 && retcode == EBREAK) || retcode == EINVFN)
			M->tty.ctrlc = TRUE;

		if (E->next_command[0]
			&& strcmp (E->next_command, E->full_name)
			&& access (E->next_command, A_EXEC, &E->broken))
		{
			static char *next_argv[2];
			static char next_arg[125];
			static char next_name[MAX_PATH_LEN];
			
			strcpy (next_name, E->next_command);
			full_name = next_name;
			if (E->next_command_line.length > 0
				&& E->next_command_line.length < 125)
			{
				strncpy (next_arg, 
					E->next_command_line.command_tail, 124);
				next_arg[E->next_command_line.length] = '\0';
			}
			else
				next_arg[0] = '\0';
			
			next_argv[0] = next_name;
			next_argv[1] = next_arg;
			argv = next_argv;
			argc = (*next_arg != '\0')? 2 : 1;
			var_argc = 0;
			start_parameter = NULL;
			overlay_process = parallel = FALSE;
			
			command_to_exec = TRUE;
		}
		else if (E->broken)
		{
			/* stevie */
			retcode = 1;
			break;
		}
		
		start_parameter = NULL;
		*broken = E->broken;

		if (first != NULL)
		{
			UnsetExecParameter (E->M, &E->parms, 
				&E->screen, &E->broken, parallel);
			mfree (&M->alloc, E);
		}
		else
			first = E;
	}
	
	if (first != NULL)
	{
		UnsetExecParameter (first->M, &first->parms, 
			&first->screen, &first->broken, parallel);
		mfree (&M->alloc, first);
	}

	return retcode;
}
