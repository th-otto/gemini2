/*
 * @(#) Gemini\vs.h
 * @(#) Stefan Eissing, 27. Dezember 1993
 *
 * description: Header File for the new Desktop
 *
 */


#ifndef G_VS__
#define G_VS__

#define store_sccs_id(a) \
	/* char a##_id[] = "@(#) "__FILE__" "__DATE__ */

/*
 * Macros und Defines
 */

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#ifndef NULL
#define NULL ((void *)0L)
#endif

#ifndef max
#define max(a,b)	((a)>(b))?(a):(b)
#define min(a,b)	((a)<=(b))?(a):(b)
#endif

#ifndef DIM
#define DIM(x)		(sizeof(x) / sizeof(x[0]))
#endif


/*
 * Typen
 */
typedef unsigned char byte;			/* 8 Bit unsigned */
typedef unsigned char uchar;		/* 8 Bit unsigned */
typedef int word;					/* 16 Bit signed */
typedef unsigned int uword;			/* 16 Bit unsigned */
typedef unsigned int uint;
typedef unsigned long ulong;		/* 32 Bit unsigned */

/*
 * define the language to be used in Gemini
 */
#define GERMAN		1
#define ENGLISH		(!GERMAN)


#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN	256
#endif

/* maximale L„nge eines Dateinamens (mit NUL-Byte)
 */	/* jr: xxx */
#ifndef MAX_FILENAME_LEN
#define MAX_FILENAME_LEN		15
#endif

/* maximale L„nge fr Wildcards
 */
#define WILDLEN     30


/* Datei-Attribute
 */
#define FA_RDONLY       0x01
#define FA_HIDDEN       0x02
#define FA_SYSTEM       0x04
#define FA_LABEL        0x08
#define FA_FOLDER       0x10
#define FA_ARCHIV       0x20


typedef struct
{
	OBJECT *tree;					/* pointer to tree-array */
	word maxobj;					/* max array entries */
	word objanz;					/* current entries */
	
	word total_selected;
	
	word scrapNr;					/* no. of scrapicon */
	word trashNr;					/* no. of trashicon */
	struct
	{
		unsigned scrap_empty : 1;	/* Klemmbrett ist leer */
		unsigned trash_empty : 1;	/* Papierkorb ist leer */
	} flags;

	char emptyPaper;				/* empty Paperbasket on exit */
	char waitKey;					/* wait for key after TOS Apps */
	char ovlStart;					/* start everything as overlay */
	char silentCopy;				/* Cp/Mv without Dialog */
	char silentRemove;				/* Rm without Dialog */
	char silentGINRead;				/* Lese GIN ohne Nachfrage */
	char replaceExisting;			/* Replace existing Files */

	char useCopyKobold;
	char useDeleteKobold;
	char useFormatKobold;
	int  minCopyKobold;				/* Mindestzahl von Objekten, bei */
	int  minDeleteKobold;			/* denen Kobold aktiviert wird. */
	char preferLowerCase;			/* Kleinbuchstaben in Dateinamen */

	char showHidden;				/* show hidden files */
	char askQuit;					/* ask when quitting */
	char saveState;					/* save state on exiting */
	char snapAlways;				/* R„ume neue Objekte auf */
	word snapx, snapy;				/* snap von Icons beim Aufr„umen*/
	word format_info;				/* Wie Disketten gel”scht werden*/
} DeskInfo;

extern DeskInfo NewDesk;


typedef struct
{
	word aligned;					/* Windows character aligned */
	word normicon;					/* norm or small icons */
	word showtext;					/* checked menuentry for show */
	word sortentry;					/* checked menuentry for sort */
	word m_cols;					/* Werte frs Mupfel Window */
	word m_rows;
	word m_inv;
	word m_font;
	word m_fsize;
	word m_wx;
	word m_wy;
	
	char dont_save_gin_file;			/* wird nicht mitgesichert */
	char gin_name[MAX_FILENAME_LEN];	/* Name der GIN-Datei */
	
} SHOWINFO;

extern SHOWINFO ShowInfo;

/* Pfad, in dem GEMINI.APP liegt
 */
extern char BootPath[];				/* Pfad beim Programmstart */

/* String mit Versionsnummer und Datum */
extern char venusVersionString[];

/* Einige Variablen im Zusammenhang mit AES und VDI */
extern GRECT Desk;						/* Desktopkoordinaten */
extern word apid;						/* Application Id */
extern word phys_handle;				/* Handle von graf_handle */
extern word wchar, hchar, wbox, hbox;	/* Charakterinformationen */
extern word vdi_handle;					/* eigener VDI-Handle */
extern word MultiAES;					/* Ataris MultiTOS ist da */
extern word number_of_settable_colors;
extern word number_of_planes;
extern unsigned long number_of_colors;

/* Skalierungsfaktor fr Koordinaten im INF- und GIN-Dateien */
#define NEW_SCALE_FACTOR	32767
extern long ScaleFactor;

extern void *MupfelInfo;
extern void *pMainAllocInfo;
extern unsigned long *pDirtyDrives;
extern int MupfelShouldDisplayPrompt;

int ExecGINFile (const char *filename);

void PasteStringToConsole (const char *line);


#endif /* !G_VS__ */