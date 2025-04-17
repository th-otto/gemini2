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

/* Struktur fÅr Shellvars
 */
typedef struct shellvar
{
	struct shellvar *next;		/* nÑchste (Ñltere) Struktur */
	const char **argv;			/* Tabelle der Argumente */
	int argc;					/* Gesamtanzahl von Argumenten */
	struct
	{
		unsigned was_malloced : 1;	/* argv ist alloziert worden */
	} flags;
} SHELLVAR;


/* Strukturen fÅr ParsInput
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


/* Strukturen fÅr Redirection
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

/* stevie: Support fÅr Handles 4 und 5 vorerst ausgelassen
 */
#define MAX_STREAM	4		

typedef struct
{
	STREAMINFO	stream[MAX_STREAM];
} REDIRINFO;


/* Strukturen fÅr Chario
 */
#define TYPEAHEADMAX	128	/* Maximaler Typeahead */
#define FKEYMAX			20	/* Anzahl unterstÅtzer Funktionstasten */

typedef struct
{
	int break_char;			/* reset parser: ^C */
	int kill_char;			/* clear line: Esc */
	int completion;			/* filename completion: Tab */
	int meta_completion;	/* display possible compl. Help */

	unsigned char escape_char;		/* Was '\' fÅr UNIX ist */
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


/* Strukturen fÅr dirs, pushd und popd */
typedef struct dir_stack
{
	struct dir_stack *next;
	
	/* gepushtes Verzeichnis */
	const char *dir;
	
} DIRSTACK;


/* Struktur fÅr history */

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

/* Struktur fÅr Gruppen- und Usernamen (ls) */
typedef struct id_info
{
	struct id_info *next;
	
	int id;
	char name[16];
		
} IDINFO;



/* Struktur fÅr die Traps
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


/* Struktur fÅr Job-Control
 */
typedef struct
{
	long term_pgrp;
} JOBINFO;

typedef struct
{
	/* Struktur, die alle Informationen fÅr mmalloc, mcalloc,
	 * mrealloc und mfree enthÑlt.
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
		
		/* GEM ist initialisiert, GEM-Aufrufe kînnen getÑtigt
		 * werden, vdi_handle ist initialisiert.
		 */
		unsigned gem_initialized : 1;
		
		/* Es wird ein Overlay ausgefÅhrt.
		 */
		unsigned doing_overlay : 1;
	} shell_flags;
	
	/* Einstellungen, die das tty betreffen (chario.c)
	 */
	TTYINFO tty;

	/* Prozeû-IDs
	 */
	long real_pid;
	long our_pid;
	long last_async_pid;

	/* Unterste Stackgrenze, die erreicht werden darf
	 */
	unsigned long stack_limit;
	
	/* Globales Flag fÅr einen Fehler (stevie: BAD)
	 */
	int oserr;
				
	/* Returnwert des letzten synchron ausgefÅhrten Kommandos
	 */
	int exit_value;
	
	/* Wenn TRUE, dann verlasse diese Shell sofort.
	 */
	int exit_shell;
	
	/* Returnwert des letzten "exit" Kommandos. Dient zum öber-
	 * schreiben von exit_value;
	 */
	int exit_exit_value;
	
	/* ZÑhler, in wieviel Schleifen sich die Shell befindet
	 */
	int loop_count;
	
	/* ZÑhler, der die Anzahl der aktiven Funktionen angibt
	 */
	int function_count;
	
	/* ZÑhler, wieviele "break"s noch abzuarbeiten sind
	 */
	int break_count;
	
	/* TRUE, wenn ein break (in Schleifen) gemacht werden muû.
	 */
	int exec_break;
	
	/* Environment/Namespace dieser Shell
	 */
	NAMEINFO *name_list;
	/* Einige Pointer fÅr schnelleren Zugriff
	 */
	NAMEINFO *path_name;
	NAMEINFO *ifs_name;
	NAMEINFO *ps1_name;
	NAMEINFO *ps2_name;
	NAMEINFO *ps4_name;
	
	/* Information fÅr Input des Parsers
	 */
	PARSINPUTINFO pars_input_info;
	
	/* Informationen fÅr Redirection
	 */
	REDIRINFO redir;
	
	/* Informationen fÅr $0 ... $9, $*
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
	
	/* Wenn wir GEM benutzen, haben wir hier die nîtigen 
	 * Informationen
	 */
	WORD ap_id;
	WORD vdi_handle;
	WORD desk_x, desk_y, desk_w, desk_h;
	WORD char_w, char_h, box_w, box_h;
	
	/* Informationen fÅr dirs, pushd und popd */
	DIRSTACK *dir_stack;
	
	/* Informationen fÅr die History */
	HISTORYINFO history;
	
	/* Informationen fÅr trap */
	TRAPINFO trap;
	
	/* Informationen fÅr Job-Control */
	JOBINFO jobs;
	
	/* Informationen Åber User- und Group-IDs */
	IDINFO *users, *groups;
	
	/* Informationen fÅrs Hashing */
	void *hash;
	
	/* Informationen fÅr Aliase */
	void *alias;
	
	/* Informationen fÅr times */
	unsigned long times;
	
	/* Informationen zu internen Kommandos */
	char *internal_flags;
	
	/* Informationen fÅr Shrink	 */
	void *shrink_address;
	size_t shrink_size;
	
	/* Jump-Buffer fÅr Interrupts (Control-C) */
	int break_buffer_set;
	jmp_buf break_buffer;
	
	/* Flag fÅrs Checken des offenen Console-Fensters */
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