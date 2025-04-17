/*
 * @(#) MParse\MLex.h
 * @(#) Stefan Eissing, 17. Januar 1993
 *
 * Header fÅr
 * Lexer fÅr den Mupfel-Parser mit Bourne-Shell-Syntax
 */

#ifndef MupfelLex__
#define MupfelLex__

#include <stdlib.h>

#include "partypes.h"
#include "..\common\argvinfo.h"



#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif



/* Tokennummern fÅr reservierte Symbole
 */
#define DO			0405
#define FI			0420
#define ELIF		0422
#define ELSE		0421
#define IN			0412
#define CURLY_OPEN	0406
#define CURLY_CLOSE	0450
#define THEN		0444
#define DONE		0441
#define ESAC		0442
#define IF			0436
#define FOR			0435
#define WHILE		0433
#define UNTIL		0427
#define CASE		0417
#define AND_AND 	04273
#define OR_OR 		04274
#define GREATER_GREATER 04275
#define HERE_DOC 	04276
#define LESS_AND 	00277
#define GREATER_AND 04272
#define SEMI_SEMI 	04271
#define HERE_DOC_MINUS 0262
#define AND_GREATER 0261
#define EOF_TOKEN 	02000

#define SYMREP	04000
#define SYMFLG	0400

/* Ende des Inputs */
#define EOF_CHAR	0

/* Flag zum Quoten von Chars in token_name
 */
#define QUOTED		(0x8000)

typedef struct
{
	/* Zeigt an, daû die Eingabe abgebrochen werden muûte
	 */
	int broken;
	
	/* Grund fÅr den Abbruch
	 */
	int break_reason;
	
	/* Wonach der Lexer gesucht hat
	 */
	char looking_for;
	
	/* Pointer auf die Eingabezeile */
	const char *input_line;

	/* Eingabezeile wurde von Lexer alloziert. */
	int was_malloced;
	
	/* Ende der Eingabe erreicht
	 */
	int eof_reached;
		
	size_t input_line_offset;
	
	/* mîgliche Startposition eines Alias
	 */
	size_t possible_alias_start;
	
	/* Escape-Zeichen ( '\\' unter UNIX) */
	unsigned char escape_char;
	
	/* Zeiger auf Zeiger auf Liste von IOINFOs, die beim Parsen
	 * einer Zeile entstanden sind und am Ende einer Zeile
	 * bearbeitet werden mÅssen.
	 */
	IOINFO **pending_io_list;
	
	/* peek character */
	short peek_char;
	
	/* noch ein peek character, der aber <peek_char> immer Åberschreibt */
	short peek_next;
	
	/* Character des gelesenen Tokens mit QUOTED-Flag im Highbyte */
	short *token_name;
	
	/* LÑnge des allozierten Buffers fÅr <token_name> */
	size_t token_buffer_length;

	/* aktueller Offset in <token_name> */
	size_t token_buffer_offset;
		
	/* Wenn Zahl gelesen, deren Wert */
	long token_number;
	
	/* Wert des Token */
	int token_value;

	/* Gibt an, ob ein assignment im Token vorkommt '=' */
	unsigned token_assignment : 1;
	
	/* Gibt an, ob gequotete Zeichen in Token vorkommen */
	unsigned token_quoted_chars : 1;
	
	/* Gibt an, ob ein ungequotetes '$' in Token vorkommt */
	unsigned token_dollar_present : 1;
	
	/* Liste von geparsten Substitution-Infos */
	SUBSTLISTINFO *token_subst_list;
	
	/* WORD, das das Token beschreibt */
	WORDINFO token;
	
	/* Ob wir nach einem reservierten Wort suchen */
	int may_be_reserved;
	
	/* Ist gesetzt, wenn wir zwar nicht nach einem reservierten
	 * Wort Ausschau halten, aber trotzdem Aliase erlaubt sind.
	 */
	int may_be_alias;
	
} LEXINFO;


const char *Token2String (int token);

int EofOrSeparator (unsigned char c);
int CommandSeparator (unsigned char c);

int GetReservedCompletions (ARGVINFO *A, void *entry);

int NextChar (MGLOBAL *M, LEXINFO *L, char quote_char, short *c);
int SkipWhitespace (MGLOBAL *M, LEXINFO *L, short *d);

void LexInit (MGLOBAL *M, LEXINFO *L, IOINFO **pending_io_list);
void LexExit (MGLOBAL *M, LEXINFO *L);

int ReadToken (MGLOBAL *M, LEXINFO *L);


#if LEX_DEBUG

void PrintToken (LEXINFO *L);

#endif /* LEX_DEBUG */

#endif /* MupfelLex__ */

