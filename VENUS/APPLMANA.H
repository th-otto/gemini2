/*
 * @(#) Gemini\applmana.h
 * @(#) Stefan Eissing, 31, Oktober 1994
 *
 * description: Header File for applmana.c
 */

#ifndef G_APPLMANA__
#define G_APPLMANA__

#include "..\common\memfile.h"


int WriteApplRules (MEMFILEINFO *mp, char *buffer);

void ApplDialog (void);
void AddApplRule (const char *line);
void FreeApplRules (void);

word GetApplForData (const char *name, char *program, char *label, 
	int ignore_rules, int interactive, word *mode);


/* Startmodi fÅr Programme
 */
#define TOS_START		0x1000	/* TOS-Programm */
#define GEM_START		0x0001	/* GEM-Programm */
#define TTP_START		0x0002	/* mit Parametern */
#define WCLOSE_START	0x0004	/* Nicht in Console */
#define WAIT_KEY		0x0008	/* Warte auf Tastendruck */
#define OVL_START		0x0010  /* als Overlay starten */
#define SINGLE_START	0x0100	/* Im Single Modus (MAGIX) */
#define CANVA_START		0x0200	/* Versteht VA-START-Nachricht */

word GetStartMode (const char *name, word *mode);

void ApRenamed (const char *oldname, const char *newname, int folder);
void ApNewLabel (char drive, const char *oldlabel,
					const char *newlabel);

word RemoveApplInfo (const char *name);

#endif	/* !G_APPLMANA__ */