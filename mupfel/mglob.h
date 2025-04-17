/*
 * @(#) Mupfel\MGlob.h
 * @(#) Stefan Eissing, 5. Februar 1995
 *
 * jr 26.5.1996
 */
 

#ifndef M_GLOB__
#define M_GLOB__

#define MUPFELVERSION	"1.a2\341"

#include <setjmp.h>
#include <tos.h>

#include "..\mupfel\partypes.h"
#include "..\common\memfile.h"

/* Type 16 Bit
 */
#define WORD	int

/* Struktur f�r Shellvars
 */
typedef struct shellvar
{
	struct shellvar *next;		/* n�chste (�ltere) Struktur */
	const char **argv;			/* Tabelle der Argumente */
	int argc;					/* Gesamtanzahl von Argumenten */
	struct
	{
		unsigned was_malloced : 1;	/* argv ist alloziert worden */
	} flags;
} SHELLVAR;


/* Strukturen f�r ParsInput
 */
#define PARSINFOBUFFLEN		300

typedef enum
{
	fromStdin,
	fromString,
	fromFile,
	fromArgv
} InputType;

typedef struct
{
	InputType input_type;
	struct
	{
		unsigned eof_reached : 1;
		unsigned first_line : 1;
	} flags;
	size_t line_count;

	const char *pushed_line;
	const char *input_line;
	int current;
	int argc;
	const char** argv;
	MEMFILEINFO *mem_file;
	
	char buffer[PARSINFOBUFFLEN];
} PARSINPUTINFO;


/* Strukturen f�r Redirection
 */
typedef struct
{
	int handle;
	unsigned isatty : 1;
	unsigned was_opened : 1;
} STREAMINFO;

#define S_STDIN		(0)
#define S_STDOUT	(1)
#define S_STDERR	(2)
#define S_STDPRN	(3)
#define S_STD4		(4)
#define S_STD5		(5)

/* stevie: Support f�r Handles 4 und 5 vorerst ausgelassen
 */
#define MAX_STREAM	4		

typedef struct
{
	STREAMINFO	stream[MAX_STREAM];
} REDIRINFO;


/* Strukturen f�r Chario
 */
#define TYPEAHEADMAX	128	/* Maximaler Typeahead */
#define FKEYMAX			20	/* Anzahl unterst�tzer Funktionstasten */

typedef struct
{
	int break_char;			/* reset parser: ^C */
	int kill_char;			/* clear line: Esc */
	int completion;			/* filename completion: Tab */
	int meta_completion;	/* display possible compl. Help */

	unsigned char escape_char;		/* Was '\' f�r UNIX ist */
	char *fkeys[FKEYMAX];			/* Belegung der Funktionstasten */
	
	char ctrlc;
	char was_ctrlc;
	
	int ta_in, ta_out;
	long ta_buffer[TYPEAHEADMAX];
	
	void (*gemdos_out)(int c);

	int rows;				/* Zeilen-/Spaltenzahl des Terminals */
	int columns;
	
	void (*error_function) (const char *str);
	
} TTYINFO;


/* Strukturen f�r dirs, pushd und popd */
typedef struct dir_stack
{
	struct dir_stack *next;
	
	/* gepushtes Verzeichnis */
	const char *dir;
	
} DIRSTACK;


/* Struktur f�r history */

typedef struct
{
	unsigned int first, last;
	const char *str;
} HISTLINE;

typedef struct
{
	unsigned int size;

	HISTLINE *lines;
	
	struct
	{
		unsigned save_history;
		unsigned no_doubles;
	} flags;
} HISTORYINFO;

/* Struktur f�r Gruppen- und Usernamen (ls) */
typedef struct id_info
{
	struct id_info *next;
	
	int id;
	char name[16];
		
} IDINFO;



/* Struktur f�r die Traps
 */
#define MAX_TRAP	31
typedef struct
{
	struct
	{
		unsigned was_raised : 1;
	} flags;
	const char *command;
} TRAPINFO[MAX_TRAP];


/* Struktur f�r Job-Control
 */
typedef struct
{
	long term_pgrp;
} JOBINFO;

typedef struct
{
	/* Struktur, die alle Informationen f�r mmalloc, mcalloc,
	 * mrealloc und mfree enth�lt.
	 */
	ALLOCINFO alloc;
	
	struct							/* Commandline/set-Switches */
	{
		unsigned export_all : 1;				/* -a */
		unsigned exec_argv : 1;					/* -c */
		unsigned break_on_false : 1;			/* -e */
		unsigned no_globber : 1;				/* -f */
		unsigned hash_function_def : 1;			/* -h */
		unsigned interactive : 1;				/* -i */
		unsigned keywords_everywhere : 1;		/* -k */
		unsigned local_block : 1;				/* -l */
		unsigned no_execution : 1;				/* -n */
		unsigned restricted : 1;                /* -r */
		unsigned only_one_command : 1;			/* -t */
		unsigned no_empty_vars : 1;				/* -u */
		unsigned verbose : 1;					/* -v */
		unsigned print_as_exec : 1;				/* -x */
		
		unsigned dont_clobber : 1;				/* -C */
		unsigned debug_io : 1;					/* -D */
		unsigned dont_use_gem : 1;				/* -G */
		unsigned slash_conv : 1;				/* -S */

		/* Gibt an, ob wir per system() aufgerufen wurden.
		 */
		unsigned system_call : 1;
		
		/* GEM ist initialisiert, GEM-Aufrufe k�nnen get�tigt
		 * werden, vdi_handle ist initialisiert.
		 */
		unsigned gem_initialized : 1;
		
		/* Es wird ein Overlay ausgef�hrt.
		 */
		unsigned doing_overlay : 1;
	} shell_flags;
	
	/* Einstellungen, die das tty betreffen (chario.c)
	 */
	TTYINFO tty;

	/* Proze�-IDs
	 */
	long real_pid;
	long our_pid;
	long last_async_pid;

	/* Unterste Stackgrenze, die erreicht werden darf
	 */
	unsigned long stack_limit;
	
	/* Globales Flag f�r einen Fehler (stevie: BAD)
	 */
	int oserr;
				
	/* Returnwert des letzten synchron ausgef�hrten Kommandos
	 */
	int exit_value;
	
	/* Wenn TRUE, dann verlasse diese Shell sofort.
	 */
	int exit_shell;
	
	/* Returnwert des letzten "exit" Kommandos. Dient zum �ber-
	 * schreiben von exit_value;
	 */
	int exit_exit_value;
	
	/* Z�hler, in wieviel Schleifen sich die Shell befindet
	 */
	int loop_count;
	
	/* Z�hler, der die Anzahl der aktiven Funktionen angibt
	 */
	int function_count;
	
	/* Z�hler, wieviele "break"s noch abzuarbeiten sind
	 */
	int break_count;
	
	/* TRUE, wenn ein break (in Schleifen) gemacht werden mu�.
	 */
	int exec_break;
	
	/* Environment/Namespace dieser Shell
	 */
	NAMEINFO *name_list;
	/* Einige Pointer f�r schnelleren Zugriff
	 */
	NAMEINFO *path_name;
	NAMEINFO *ifs_name;
	NAMEINFO *ps1_name;
	NAMEINFO *ps2_name;
	NAMEINFO *ps4_name;
	
	/* Information f�r Input des Parsers
	 */
	PARSINPUTINFO pars_input_info;
	
	/* Informationen f�r Redirection
	 */
	REDIRINFO redir;
	
	/* Informationen f�r $0 ... $9, $*
	 */
	const char *program_name;
	int argc;
	char **argv;
	SHELLVAR *shell_vars;
	char runner[128];
	
	/* Informationen, auf welchem Laufwerk wir uns in welchem
	 * Ordner befinden. (und beim Start befunden haben)
	 */
	char current_drive;
	char *current_dir;
	char *start_path;
	
	/* Wenn wir GEM benutzen, haben wir hier die n�tigen 
	 * Informationen
	 */
	WORD ap_id;
	WORD vdi_handle;
	WORD desk_x, desk_y, desk_w, desk_h;
	WORD char_w, char_h, box_w, box_h;
	
	/* Informationen f�r dirs, pushd und popd */
	DIRSTACK *dir_stack;
	
	/* Informationen f�r die History */
	HISTORYINFO history;
	
	/* Informationen f�r trap */
	TRAPINFO trap;
	
	/* Informationen f�r Job-Control */
	JOBINFO jobs;
	
	/* Informationen �ber User- und Group-IDs */
	IDINFO *users, *groups;
	
	/* Informationen f�rs Hashing */
	void *hash;
	
	/* Informationen f�r Aliase */
	void *alias;
	
	/* Informationen f�r times */
	unsigned long times;
	
	/* Informationen zu internen Kommandos */
	char *internal_flags;
	
	/* Informationen f�r Shrink	 */
	void *shrink_address;
	size_t shrink_size;
	
	/* Jump-Buffer f�r Interrupts (Control-C) */
	int break_buffer_set;
	jmp_buf break_buffer;
	
	/* Flag f�rs Checken des offenen Console-Fensters */
	int checkIfConsoleIsOpen;
	
} MGLOBAL;

extern unsigned long DirtyDrives;

extern unsigned long MiNTVersion;
extern unsigned short GEMDOSVersion, MagiCVersion;
extern unsigned short TOSVersion, TOSDate, TOSCountry;


MGLOBAL *InitMGlobal (int argc, char **argv, int gem_allowed,
	int is_system_call, unsigned long stack_limit, BASPAG *my_base,
	int first_mglobal);
void ExitMGlobal (MGLOBAL *M, int free_mglobal);

int InitGEM (MGLOBAL *M);
void ExitGEM (MGLOBAL *M);

MGLOBAL *MakeNewShellLevel (MGLOBAL *M);
void FreeMadeShellLevel (MGLOBAL *M, MGLOBAL *new);

#endif /* M_GLOB__ */