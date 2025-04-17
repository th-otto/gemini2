/*
 * @(#) Mupfel\Exec.h 
 * @(#) Gereon Steffens & Stefan Eissing, 10. April 1994
 * 
 * - definitions for exec.c
 */

#ifndef __M_EXEC__
#define __M_EXEC__

#define PARM_YES		1
#define PARM_NO			0
#define PARM_EGAL		2

typedef struct
{
	unsigned initialized : 1;
	unsigned cursor_on : 1;
	unsigned mouse_on : 1;
	unsigned grey_back : 1;
	unsigned cursor_blink : 1;
	char window_open;
} SCREENINFO;

typedef struct
{
	unsigned is_gem : 1;		/* Ist ein GEM-Programm */
	unsigned is_app : 1;		/* Hat die Endung APP */
	unsigned is_acc : 1;		/* Hat die Endung APP */
	unsigned ex_arg_used : 1;	/* ex_arg wurde benutzt */
	unsigned show_full_path : 1;/* bei GEM-Programmen voller Pfad */
	
	unsigned mouse_on : 1;		/* M: turn Mouse on? */
	unsigned cursor_on : 1;		/* C: turn alpha Cursor on? */
	unsigned grey_back : 1;		/* B: draw a grey Background? */
	unsigned aes_args : 1;		/* S: pass args using Shel_write()? */
	unsigned ch_dir : 1;		/* D: Cd to where the program lives? */
	unsigned wait_key : 1;		/* K: wait for Key after TOS pgms? */
	unsigned overlay : 1;		/* O: Overlay mupfel with program (using RUNNER)? */
	unsigned cursor_blink : 1;	/* L: cursor bLinking? */
	unsigned silent_xarg : 1;	/* implicit value from PRGNAME_EXT/???DEFAULT */
	unsigned single_mode : 1;	/* Single Mode for MagiX */
	char ex_arg;				/* X: use Atari's EXARG scheme? */
	char in_window;				/* W: only used for GEMINI */
} EXECPARM;


int InitExecParameter (MGLOBAL *M, EXECPARM *parms,
	const char *full_name, int parallel);

int SetExecParameter (MGLOBAL *M, EXECPARM *parms, SCREENINFO *screen,
	const char *full_name, char start_path[], int *broken);

void UnsetExecParameter (MGLOBAL *M, EXECPARM *parms, 
	SCREENINFO *screen,	int *broken, int parallel);


/* exec external command
 */
int ExecExternal (MGLOBAL *M, const char *full_name, int argc, 
	const char **argv, int var_argc, const char **var_argv, 
	EXECPARM *start_parameter, int overlay_process, int parallel,
	int *broken);



#endif /* __M_EXEC__ */