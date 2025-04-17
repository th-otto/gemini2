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
	LISTINFO L;			/* Listinfo f�r die Liste der Fontnamen */
	LISTINFO P;			/*    "              "       Fontgr��en */
	LISTINFO S;			/*    "              "         Schnitte */
	OBJECT *tree;		/* Dialogbox							*/
	int fontbox;		/* Index der Box f�r Namen				*/
	int fontbgbox;		/* Index der Hintergrundbox f�r Namen	*/
	int formbox;
	int formbgbox;
	int pointbox;		/*               f�r Gr��en				*/
	int pointbgbox;		/*               f�r Gr��en				*/
	int showbox;		/* Index der Box f�r Teststring			*/
	FONTWORK *fw;		/* Pointer auf FONTWORK-Struktur 		*/
	char *test;			/* Teststring f�r Ausgabe				*/
} FONTSELINFO;


int FontSelInit (FONTSELINFO *F, FONTWORK *fw, OBJECT *tree,
				int fontbox, int fontbgbox, int pointbox,
				int pointbgbox, int formbox, int formbgbox,
				int showbox, char *teststring,
				int proportional, int *actfont, int *actsize);

void FontSelDraw (FONTSELINFO *F, int font, int size);

void FontShowFont (FONTWORK *F, OBJECT *tree, int frame, int font,
	int size, char *teststring);

/* Konstanten f�r `clicks' */

#define FONT_CL_UP		(-1)
#define FONT_CL_DOWN	(-2)

int FontClFont (FONTSELINFO *F, int clicks, int *font, int *size);
int FontClForm (FONTSELINFO *F, int clicks, int *font, int *size);
int FontClSize (FONTSELINFO *F, int clicks, int *font, int *size);

void FontSelExit (FONTSELINFO *F);
void FontSelectSize (LISTINFO *P, int *size, int redraw);

#endif
