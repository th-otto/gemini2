/*
 * @(#) MParse\Locate.h
 * @(#) Stefan Eissing, 31. Januar 1993
 *
 * Bestimme die Art und den Aufenthaltsort eines Kommandos
 *
 */


#ifndef __locate__
#define __locate__

#include "..\common\argvinfo.h"
#include "names.h"

typedef enum 
{
	Function,			/* interne Funktion */
	Internal,			/* internes Kommando */
	Binary,				/* GEMDOS-Executable */
	Script,				/* Batch-File fÅr uns */
	AlienScript,		/* Batch-File fÅr anderes Programm */
	Data,				/* Einfach nur Daten */

	NotFound,			/* unbekannt */
} LocationType;


typedef struct
{
	/* Zeiger auf NameInfo, die Funktion enthÑlt
	 */
	NAMEINFO *n;
} LOC_FUNCTION;


typedef struct
{
	/* interne Nummer des Kommandos
	 */
	int command_number;
} LOC_INTERNAL;


typedef struct
{
	/* Kompletter Pfad auf das Programm
	 */
	const char *looked_for;
	const char *full_name;

	/* kompletter Pfad auf das ausfÅhrende Programm. Nur bei
	 * alien Scripts benutzt.
	 */
	char *executer;
} LOC_FILE;



typedef struct
{
	/* Das eigentliche Kommando
	 */
	const char *command;
	
	/* Was es ist.
	 */
	LocationType location;
	
	/* HÑngt von <location> ab, und beschreibt das Kommando
	 * nÑher (s.o.)
	 */
	union
	{
		LOC_FUNCTION func;		/* Function */
		LOC_INTERNAL intern;	/* Internal */
		LOC_FILE file;			/* sonst */
	} loc;
	
	/* interne Verwaltungsinformationen
	 */
	int costs;
	struct
	{
		unsigned was_hashed : 1;
		unsigned can_be_hashed : 1;
	} flags;

	/* Gibt an, daû die Suche wegen Fehlern aufgegeben werden muûte
	 */
	int broken;	
} LOCATION;

/* Orte, an denen gesucht wird
 */
#define	LOC_FUNCTION	0x01
#define	LOC_INTERNAL	0x02
#define	LOC_EXTERNAL	0x04
#define LOC_ALL			(LOC_FUNCTION|LOC_INTERNAL|LOC_EXTERNAL)


int Locate (MGLOBAL *M, LOCATION *L, const char *command, int used,
	int where_to_look);
void FreeLocation (MGLOBAL *M, LOCATION *L);

int ExtInSuffix (MGLOBAL *M, const char *name);

int GetExternalCompletions (MGLOBAL *M, ARGVINFO *A, char *what);


#endif /* __locate__ */
