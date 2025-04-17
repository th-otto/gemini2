/*
 * @(#) MParse\MParse.h
 * @(#) Stefan Eissing, 16. Januar 1992
 *
 */


#ifndef __MParse__
#define __MParse__

#include "partypes.h"
#include "mlex.h"


typedef struct
{
	/* Informationen fr den Lexer
	 */
	LEXINFO L;
	
	/* Tiefe der Rekursion der command() - Funktion
	 */
	int command_level;
	
	/* Pending IO Strukturen fr HERE_DOCs, die am Ende einer
	 * Zeile in MLex.c in eine tempor„re Datei gelesen werden
	 * mssen. <pending_io> wird dabei vom Lexer gel”scht.
	 * IGITTT!
	 */
	IOINFO *pending_io;
	
	/* Momentan aufgebauter Baum ist fr eine Funktion
	 */
	int is_function_def;
	
	/* != 0, wenn nicht gehasht werden soll
	 */
	int no_hash;
	
} PARSINFO;


/* Parse die Eingabezeilen und liefere einen Tree von Kommandos
 * zurck.
 * <input_function> ist eine Funktion, die eine String von der
 * Eingabe liest. Bei Ende der Eingabe wird NULL zurckgeliefert.
 * Ist <broken> TRUE, so mute die Eingabe aus welchen Grnden
 * auch immer abgebrochen werden. Es wird dann kein Syntax-Fehler
 * gemeldet.
 *
 * <legal_parse_end> ist ein erlaubtes Zeichen fr das Ende des
 * Parsens, normalerweise ein Newline.
 */
TREEINFO *Parse (MGLOBAL *M, PARSINFO *P, int legal_parse_end);


#endif /* __MParse */