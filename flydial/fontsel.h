/*
	@(#)MausTausch/fontsel.h
	
	Julian F. Reschke, 2. Oktober 1993
*/


#ifndef __fontsel__
#define __fontsel__

#include <stdlib.h>

#include "flydial/listman.h"

typedef struct
{
	LISTINFO L;			/* Listinfo fÅr die Liste der Fontnamen */
	LISTINFO P;			/*    "              "       Fontgrîûen */
	LISTINFO S;			/*    "              "         Schnitte */
	OBJECT *tree;		/* Dialogbox							*/
	int fontbox;		/* Index der Box fÅr Namen				*/
	int fontbgbox;		/* Index der Hintergrundbox fÅr Namen	*/
	int formbox;
	int formbgbox;
	int pointbox;		/*               fÅr Grîûen				*/
	int pointbgbox;		/*               fÅr Grîûen				*/
	int showbox;		/* Index der Box fÅr Teststring			*/
	FONTWORK *fw;		/* Pointer auf FONTWORK-Struktur 		*/
	char *test;			/* Teststring fÅr Ausgabe				*/
} FONTSELINFO;


int FontSelInit (FONTSELINFO *F, FONTWORK *fw, OBJECT *tree,
				int fontbox, int fontbgbox, int pointbox,
				int pointbgbox, int formbox, int formbgbox,
				int showbox, char *teststring,
				int proportional, int *actfont, int *actsize);

void FontSelDraw (FONTSELINFO *F, int font, int size);

void FontShowFont (FONTWORK *F, OBJECT *tree, int frame, int font,
	int size, char *teststring);

/* Konstanten fÅr `clicks' */

#define FONT_CL_UP		(-1)
#define FONT_CL_DOWN	(-2)

int FontClFont (FONTSELINFO *F, int clicks, int *font, int *size);
int FontClForm (FONTSELINFO *F, int clicks, int *font, int *size);
int FontClSize (FONTSELINFO *F, int clicks, int *font, int *size);

void FontSelExit (FONTSELINFO *F);
void FontSelectSize (LISTINFO *P, int *size, int redraw);

#endif
