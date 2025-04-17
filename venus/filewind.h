/*
 * @(#) Gemini\filewind.h
 * @(#) Stefan Eissing, 01. November 1994
 *
 */


#ifndef G_filewind__
#define G_filewind__


#include "userdefs.h"

typedef struct
{
	long magic;						/* filemagic "FilE" */
	char *name;						/* der Dateiname */

	struct
	{
		unsigned selected : 1;		/* file ist selektiert */
		unsigned dragged  : 1;		/* wurde verschoben */
		unsigned is_symlink : 1;	/* ist ein symbolischer Link */
		unsigned text_display : 1;  /* Textdarstellung */
	} flags;
	
	char attrib;					/* fileattribut */
	long size;						/* length of file */
	uword date;
	uword time;
	uword number;					/* laufende Nummer fÅr unsort */

	USERBLK	user_block;
	ICON_OBJECT icon;				/* enthÑlt Icon-Informationen */

} FileInfo;


#define FILEBUCKETSIZE	40

typedef struct fileBucket
{
	struct fileBucket *nextbucket;
	word usedcount;
	FileInfo finfo[FILEBUCKETSIZE];

} FileBucket;


typedef struct positionInfo
{
	struct positionInfo *next;
	word xskip, yskip;
} PositionInfo;

typedef enum { 
	DisplayNone, DisplayText, DisplaySmallIcons, DisplayBigIcons
} DisplayMode;

typedef struct
{
	OBJECT *tree;					/* Pointer auf Hintergrund */
	word objanz;					/* Anzahl der Objekte im Baum */
	long selectSize;				/* Gesamtgrîûe der selektierten Objekte */
	word maxlines;					/* maximale Anzahl von Zeilen */
	word xanz, yanz;				/* Anzahl der Spalten/Zeilen */
	word xdist;						/* x-Distanz zwischen 2 Objekten
									 * y-Distanz ist immer 1 */
	word xskip, yskip;				/* Wieviel Objekte bei der Anzeige
									 * links/oben unsichtbar sind */
	PositionInfo *lastPos;			/* Letzte Position der Anzeige */
	OBJECT stdobj;					/* Standardobjekt fÅr Anzeige */
	word icon_text_distance;
	word std_is_small;				/* Kleines Icon, wenn verwendet? */
	word icon_max_width;			/* Dessen maximalen Ausmaûe */
	word icon_max_height;
	
	word sort;						/* wie sortiert wurde */
	word desiredSort;				/* wie sortiert werden soll */

	DisplayMode display;			/* wie dargestellt wird */
	DisplayMode desiredDisplay;		/* wie dargestellt werden soll */
		
	word extratitle;				/* TRUE -> Titelstring ist gÅltig */
	char *aesname;					/* titel fÅr das AES */
	char title[MAX_PATH_LEN];		/* Titelzeile */
	char info[MAX_PATH_LEN];		/* Infozeile */

	char path[MAX_PATH_LEN];				/* Pfad des Windows */
	char label[MAX_FILENAME_LEN];	/* label of drive, files came from */
	char wildcard[WILDLEN];			/* Wildcard fÅr Files */
	char search_string[WILDLEN];	/* Wildcard zum Suchen */

	FileBucket *files;				/* Liste der Files */
	word fileanz;					/* Anzahl der Files */
	long dirsize;					/* Gesamtgrîûe des Directories */

	/* Maximale LÑnge der dargestellten Dateinamen
	 */
	word max_filename_len;
	word max_textname_len;
	
	struct
	{
		unsigned go_to_parent : 1;
		unsigned case_sensitiv : 1;
		unsigned is_8and3 : 1;
	} flags;	
	
} FileWindow_t;


#define FW_NEWWINDOW	\
	(WU_SHOWTYPE|WU_WINDNAME|WU_WINDINFO|WU_VESLIDER|WU_DISPLAY)

#define FW_NEW_DISPLAY_TYPE	(WU_SHOWTYPE|WU_DISPLAY|WU_VESLIDER|WU_REDRAW)

#define FW_NEW_SORT_TYPE	(WU_DISPLAY|WU_REDRAW)

#define FW_NEW_RULES	(WU_SHOWTYPE|WU_DISPLAY|WU_REDRAW)
#define FW_NEW_FONT		(WU_SHOWTYPE|WU_DISPLAY|WU_REDRAW|WU_VESLIDER)

#define FW_PATH_CHANGED	(FW_NEWWINDOW|WU_REDRAW)


WindInfo *FileWindowOpen (const char *path, char *label, 
	const char *title, char *wildcard, word skip_lines);

FileInfo *GetFileInfo (OBJECT *po);
FileInfo *GetSelectedFileInfo (FileWindow_t *fwp);


void WindNewLabel (char drive, const char *oldlabel, 
	const char *newlabel);
	
WindInfo *FindFileWindow (const char *path);

#endif /* !G_filewind__ */