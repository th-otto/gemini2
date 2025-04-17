/*
	@(#)FlyDial/clip.h
	
	Julian F. Reschke, 27. Mai 1995
*/

#include <string.h>

/*
	FindClipFile

	R�ckgabewert:	== 0: Fehler
					!= 0: Erfolg
	Extension:		Namenserweiterung der gesuchten
					Datei
	Filename:		Zeiger auf Char-Array, in das der
                    vollst�ndige Name eingetragen wird
*/

int ClipFindFile (const char *Extension, char *Filename);


/* L�scht alle SCRAP-Dateien bis auf die eine Extension (zb "TXT",
   kann NULL sein */
int ClipClear (char *not);

/* SH_WDRAW und SC_CHANGED verschicken */
void ClipChanged (void);

/* String ins Clipboard schreiben */
int ClipWriteString (const char *str, int append);
