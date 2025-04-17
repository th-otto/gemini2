/*
 * @(#) MParse\ParTypes.h
 * @(#) Stefan Eissing, 21. November 1992
 *
 * jr 31.7.94
 */

#ifndef __ParTypes__
#define __ParTypes__


#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

/* GrÅnde fÅr Abbruch des Parsens/Lexens
 */
#define BROKEN_USER		0x01
#define BROKEN_EOF		0x02
#define BROKEN_MALLOC	0x03
#define BROKEN_SUBST	0x04

/* Liste von geparsten Trees, die fÅr die Command-Substitution
 * mittels $() genutzt werden.
 */
typedef struct SubstListInfo
{
	struct SubstListInfo *next;
	struct trenod *tree;
} SUBSTLISTINFO;

/* Strukturen, die vom Parser aufgebaut und spÑter dann von
 * ExecTree abgearbeitet werden
 */
typedef struct WordInfo
{
	struct WordInfo *next;
	char *name;
	char *quoted;
	SUBSTLISTINFO *list;
	struct
	{
		unsigned assignment : 1;
		unsigned quoted_chars : 1;
		unsigned dollar_present : 1;
	} flags;
} WORDINFO;


/* io nodes
 */
#define 	USERIO			10
#define 	IO_UFD			15
#define 	IO_DOC			16
#define 	IO_PUT			32
#define 	IO_APP			64
#define 	IO_MOV			128
#define 	IO_RDW			256
#define		IO_STRIP		512
#define		IO_CLOBBER		1024
#define 	IO_IN_PIPE		1
#define 	IO_OUT_PIPE		0

typedef struct ioinfo
{
	struct ioinfo *next;		/* nÑchste in der Liste */
	struct ioinfo *here_next;	/* Liste von HERE-DOC-IoInfos,
								 * die nur im Parser benutzt wird. */

	int file;					/* Flags und Stream-Nummer */
	WORDINFO *name;				/* zu expandierender Name */

	char is_our_tmpfile;		/* von uns erzeugtes TMP-File */
	char *filename;				/* wirklicher Name der Datei */

	char is_pipe;				/* Ist eine Pipe */
	int pipe_handle;			/* Dateihandle fÅr die Pipe */
	
	int save_handle;			/* Zwischenspeicher fÅr Redirection */
	int save_tty;				/* save_handle war tty */
	int has_been_opened;		/* wurde von uns geîffnet */
} IOINFO;


/* Typen von Kommandos, wie er in <tree->typ> steht. Den genauen
 * Typ der Datenstruktur erhÑlt man durch undieren mit COMMASK.
 */
#define 	FPRS		0x0100
#define 	FINT		0x0200
#define 	FAMP		0x0400
#define 	FPIN		0x0800
#define 	FPOU		0x1000
#define 	FPCL		0x2000
#define 	FCMD		0x4000
#define		FOVL		0x8000
#define 	COMMASK		0x00F0

#define 	TCOM		0x0000
#define 	TPAR		0x0010
#define 	TFIL		0x0020
#define 	TLIST		0x0030
#define 	TIF			0x0040
#define 	TWHILE		0x0050
#define 	TUNTIL		0x0060
#define 	TCASE		0x0070
#define 	TAND		0x0080
#define 	TORF		0x0090
#define 	TFORK		0x00A0
#define 	TFOR		0x00B0
#define		TFUNC		0x00C0

/* Dies ist ein Schablonen-Typ fÅr weitere, die folgen:
 */
typedef struct trenod
{
	int typ;
	IOINFO *io;
} TREEINFO;

/*typedef struct dolinfo
{
	struct dolinfo *next;
	int	doluse;
	char *name;
} DOLINFO;
*/

/* HauptsÑchlich da, um IO-Redirection fÅr den daran hÑngenden
 * Baum zu erlauben.
 */
typedef struct forkinfo
{
	int	typ;
	IOINFO *io;
	TREEINFO *tree;
} FORKINFO;

/* Ein Kommando (extern oder intern) mit einer Liste von IO-
 * Redirections <io>, einer Liste von Argumenten <args>, in der
 * das erste das Kommando beschreibt, und einer Liste von zu
 * setzenden Variablen, die nur fÅr dieses Kommando gelten.
 */
typedef struct commandinfo
{
	int	typ;
	IOINFO *io;
	WORDINFO *args;
	WORDINFO *set;
} COMMANDINFO;

/* Beschreibt eine Funtionsdefinition. <name> ist der Name der
 * Funktion und <commands> die dazugehîrigen Kommandos. 
 */
typedef struct funcinfo
{
	int typ;
	WORDINFO *name;
	TREEINFO *commands;
} FUNCINFO;

/* Einfach zu verstehen.
 */
typedef struct ifinfo
{
	int	typ;
	TREEINFO *condition;
	TREEINFO *then_case;
	TREEINFO *else_case;
} IFINFO;

typedef struct whileinfo
{
	int typ;
	TREEINFO *condition;
	TREEINFO *commands;
} WHILEINFO;

/* Eine For-Schleife. Die Laufvariable ist <name>, die der Reihe
 * nach mit <list> besetzt wird. Ist <list> leer, werden die
 * Shell-Variablen $1 ... benutzt.
 */
typedef struct forinfo
{
	int typ;
	TREEINFO *commands;
	WORDINFO *name;
	COMMANDINFO *list;
} FORINFO;

/* Dies ist ein regulÑrer Ausdruck, wie er fÅr ein Case-Statement
 * benutzt wird.
 */
typedef struct reginfo
{
	struct reginfo *next;
	WORDINFO *matches;
	TREEINFO *commands;
} REGINFO;

/* Die Case-Struktur: <name> ist die zu testende Variable und in
 * <case_list> stehen die einzelnen Bedingungen.
 */
typedef struct caseinfo
{
	int typ;
	WORDINFO *name;
	REGINFO *case_list;
} CASEINFO;

/* ParInfo bezeichnet nur ein ( cmd ) -Konstrukt, daû in einer
 * Subshell ausgefÅhrt werden muû.
 */
typedef struct parinfo
{
	int	typ;
	TREEINFO *tree;
} PARINFO;

/* ListInfo's kînnen verschiedene Typen beinhalten. Mit ihr werden
 * &&, ||, | und ; verwirklicht.
 * <left> ist das Kommando, das zuerst ausgefÅhrt werden muû.
 */
typedef struct listinfo
{
	int	typ;
	TREEINFO *left;
	TREEINFO *right;
} LISTINFO;


/* Und hier die Struktur, die fÅrs Environment benîtigt wird.
 * Es gibt verschiedene ZustÑnde dieser Struktur:
 * 1. function  - func ist gÅltig und Åberschattet alles.
 * 2. exported  - nameval und envval zeigen auf den gleichen String
 * 3. readonly  - der Wert der Struktur darf nicht verÑndert werden
 *
 * Ist val ungleich NULL, so zeigt es immer auf einen
 * allozierten Wert.
 */
typedef struct nameinfo
{
	struct nameinfo *next;
	const char *name;					/* Name der Variable */
	struct
	{
		unsigned exported : 1;	/* envval ist gleich nameval */
		unsigned readonly : 1;	/* Darf nicht verÑndert werden */
		unsigned function : 1;  /* Ist eine Funktion */

		/* Wird beim Starten von Programmen benutzt, um zu
		 * verhindern, daû eine Variable doppelt exportiert wird.
		 * bei "var=value command" wird var `overridden'
		 */
		unsigned overridden : 1; 

		/* Mit diesem Flag wird gemerkt, ob die Variable vom
		User verÑndert worden ist (dadurch verliert sie ihre
		spezielle Bedeutung, siehe zB LINENO oder SECONDS) */

		unsigned autoset : 1;
	} flags;
	const char *val;			/* Wert in der Shell */
	FUNCINFO *func;				/* oder Funktion */
} NAMEINFO;

#endif /* __ParTypes__ */

