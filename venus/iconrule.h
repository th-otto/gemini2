/*
 * @(#) Gemini\iconrule.h
 * @(#) Stefan Eissing, 01. November 1994
 *
 * description: Header File for iconrule.c
 */

#ifndef G_ICONRULE__
#define G_ICONRULE__

#include "..\common\memfile.h"

#include "infofile.h"

typedef struct displayRule
{

	struct displayRule *prevrule;		/* doppelt */
	struct displayRule *nextrule;		/* verkettete Liste */
	struct displayRule *nextHash;		/* nÑchste gehashte rule */

	char wasHashed;						/* wurde gehasht */
	char isFolder;						/* Flag for Folders only */

	char wildcard[WILDLEN];				/* Wildcard fÅr Filename */

	word iconNr;						/* Nummer des Icons */

	char truecolor;						/* gewÅnschte Farbe */
	char color;							/* reale Farbe des Icons */

} DisplayRule;


int InitIcons (const char *name);		/* must be called before
								 		 * loading desktop.rsc */
void ExitIcons (void);

void FixIcons (void);

int EditIconRule (void);

int WriteDisplayRules (MEMFILEINFO *mp, char *buffer);

int AddIconRule (const char *line);
void FreeIconRules (void);
void InsDefIconRules (void);

/* Setzt die Breite der Textfahne des Icons <*pib>. Wenn <po>
 * ungleich NULL ist, wird auch die Breite des Objektes angepaût.
 */
void SetIconTextWidth (ICONBLK *pib, OBJECT *po);

void GetDeskIconMinMax (int *minIconNr, int *maxIconNr);
OBJECT *GetDeskObject (word nr);
OBJECT *GetStdDeskIcon (void);
OBJECT *GetStdBigIcon (void);
OBJECT *GetStdSmallIcon (void);
OBJECT *GetStdFolderIcon (int big_one);
OBJECT *GetBigIconObject (word isFolder, char *fname, char *color);
OBJECT *GetSmallIconObject (word isFolder, char *fname, char *color);

DisplayRule *GetFirstDisplayRule (void);

#endif /* !G_ICONRULE__ */