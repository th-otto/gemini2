/*
	@(#)FlyDial/clip.h
	
	Julian F. Reschke, 27. Mai 1995
*/

#include <string.h>

/*
	FindClipFile

	RÅckgabewert:	== 0: Fehler
					!= 0: Erfolg
	Extension:		Namenserweiterung der gesuchten
					Datei
	Filename:		Zeiger auf Char-Array, in das der
                    vollstÑndige Name eingetragen wird
*/

int ClipFindFile (const char *Extension, char *Filename);


/* Lîscht alle SCRAP-Dateien bis auf die eine Extension (zb "TXT",
   kann NULL sein */
int ClipClear (char *not);

/* SH_WDRAW und SC_CHANGED verschicken */
void ClipChanged (void);

/* String ins Clipboard schreiben */
int ClipWriteString (const char *str, int append);
