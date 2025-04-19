/*
 * @(#) Mupfel\MLex.c
 * @(#) Stefan Eissing, 27. Februar 1993
 *
 * Lexer fÅr den Mupfel-Parser mit Bourne-Shell-Syntax
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "..\common\charutil.h"


#include "mglob.h"

#include "alias.h"
#include "chario.h"
#include "ioutil.h"
#include "maketree.h"
#include "mdef.h"
#include "mlex.h"
#include "mparse.h"
#include "parsinpt.h"
#include "subst.h"

/* internal texts
 */
#define NlsLocalSection "M.mlex"
enum NlsLocalText{
LEX_WRITE_ERROR,	/*Fehler beim Schreiben von %s\n*/
};

/* Gibt an, wie kompatibel zur Bourne-Shell wir sind, bzw. ob
 * wir auch `^' fÅr Pipes erkennen.
 */
#define HAT_COMPATIBLE	0

/* Zeichen, die in Doublequotes escaped werden kînnen
 */
#define ESCAPED_CHARS	"$`"

/* Schrittweite, mit der der Token-Buffer wachsen soll
 * (muû gerade sein)
 */
#define TOKEN_BUFFER_GROW	128

/*
 * reservierte Wîrter und deren Tokennummern
 */
typedef struct
{
	char *token_name;
	int token_value;
} SYMBOL;

static SYMBOL reservedWords[] =
{
	{"if", IF},
	{"then", THEN},
	{"else", ELSE},
	{"elif", ELIF},
	{"fi", FI},
	{"case", CASE},
	{"esac", ESAC},
	{"for", FOR},
	{"while", WHILE},
	{"until", UNTIL},
	{"do", DO},
	{"done", DONE},
	{"in", IN},
	{"{", CURLY_OPEN},
	{"}", CURLY_CLOSE},
	{NULL, 0},
};

/* Ist `name' ein reserviertes Wort, daû am Anfang einer Zeile
 * auftauche darf? Wenn ja, gebe den Tokenwert zurÅck.
 */
static int reservedWord (LEXINFO *L)
{
	SYMBOL *sp = reservedWords;
	char name[6];
	int i;
	
	/* Ist das Token zu lang?
	 */
	if (L->token_buffer_offset > 6)
		return FALSE;

	for (i = 0; i < L->token_buffer_offset; ++i)
		name[i] = (char)(L->token_name[i] & 0xFF);
		
	while (sp->token_name)
	{
		if (!strcmp (sp->token_name, name))
			return sp->token_value;
		
		++sp;
	}
	
	return 0;
}

const char *Token2String (int token)
{
	SYMBOL *sp = reservedWords;
	
	while (sp->token_name)
	{
		if (token == sp->token_value)
			return sp->token_name;
		
		++sp;
	}
	
	return "???";
}

int GetReservedCompletions (ARGVINFO *A, void *entry)
{
	SYMBOL *sp = reservedWords;
	char *what = entry;
	
	while (sp->token_name)
	{
		if (strstr (sp->token_name, what) == sp->token_name)
		{
			if (!StoreInArgv (A, sp->token_name))
				return FALSE;
		}
		++sp;
	}
	
	return TRUE;
}


/* Lîscht den Speicher fÅr ein gelesenes Token
 */
static void clearTokenBuffer (MGLOBAL *M, LEXINFO *L)
{
	mfree (&M->alloc, L->token_name);
	L->token_name = NULL;
	L->token_buffer_length = L->token_buffer_offset = 0L;

	FreeSubstListInfo (M, L->token_subst_list);
	L->token_subst_list = NULL;
}

void LexInit (MGLOBAL *M, LEXINFO *L, IOINFO **pending_io_list)
{
	memset (L, 0, sizeof (LEXINFO));
	L->input_line = NULL;
	L->was_malloced = FALSE;
	L->input_line_offset = 0L;
	L->eof_reached = FALSE;
	L->escape_char = M->tty.escape_char;
	
	L->pending_io_list = pending_io_list;
	
	clearTokenBuffer (M, L);
	L->token_number = 0L;
	L->token_value = 0;
	
	L->token.name = NULL;
	L->token.quoted = NULL;
	L->token.list = NULL;
	
	L->peek_char = 0;
	L->peek_next = 0;
	
	L->may_be_reserved = 1;
}


static int pushBackInput (MGLOBAL *M, LEXINFO *L)
{
	int retcode = TRUE;
	
	L->peek_next = L->peek_char = 0;
	
	/* Nur wenn der Lexer wirklich noch etwas in der Eingabezeile
	 * hatte, wird diese zurÅckgepusht.
	 */
	if ((L->input_line != NULL) 
		&& (L->input_line[L->input_line_offset] != '\0'))
	{
		retcode = PushBackInputLine (M, 
			&L->input_line[L->input_line_offset]);
	}
	
	if (L->was_malloced && L->input_line)
	{
		mfree (&M->alloc, L->input_line);
	}

	L->input_line = NULL;
	L->was_malloced = FALSE;
	L->input_line_offset = 0L;
	L->eof_reached = FALSE;

	return retcode;
}


void LexExit (MGLOBAL *M, LEXINFO *L)
{
	clearTokenBuffer (M, L);
	mfree (&M->alloc, L->token.name);
	mfree (&M->alloc, L->token.quoted);
	FreeSubstListInfo (M, L->token.list);
	L->token.list = NULL;

	pushBackInput (M, L);
}


static void storeNewInputLine (MGLOBAL *M, LEXINFO *L, 
	const char *line, int was_malloced)
{
	if (L->was_malloced)
		mfree (&M->alloc, L->input_line);
	
	L->input_line = line;
	L->was_malloced = was_malloced;
	L->possible_alias_start = L->input_line_offset = 0L;
	L->eof_reached = FALSE;
}


static int readChar (MGLOBAL *M, LEXINFO *L, short *c)
{
	const char *line;
	
	if (L->peek_next)
	{
		L->peek_char = L->peek_next;
		L->peek_next = 0;
	}
	if (L->peek_char)
	{
		*c = L->peek_char;
		L->peek_char = 0;
		return TRUE;
	}
	
	if (L->eof_reached)
	{
		*c = EOF_CHAR;
		return TRUE;
	}
	
	if (L->input_line != NULL)
	{
		*c = (unsigned char)L->input_line[L->input_line_offset];
		++L->input_line_offset;

		if (*c)
			return TRUE;
	}

	line = GetParsInputLine (M, &L->broken);
	if (line != NULL)
	{
		storeNewInputLine (M, L, line, FALSE);
		L->input_line_offset = 1L;
		*c = (unsigned char)L->input_line[0L];
		return TRUE;
	}
	else if (L->broken)
	{
		/* Da Fehler beim Lesen direkt bei ihrem Auftreten
		 * gemeldet werden sollten, gehen wir davon aus, daû
		 * der Benutzer abgebrochen hat und wir keinen Syntax-
		 * Fehler melden sollen.
		 */
		L->break_reason = BROKEN_EOF;
		return FALSE;
	}
	
	/* Wenn wir einen Fehler melden, dann ist der Default das
	 * unerwartete Ende der Datei
	 */
	L->break_reason = BROKEN_EOF;
	*c = EOF_CHAR;
	L->eof_reached = TRUE;
	
	return TRUE;
}

int NextChar (MGLOBAL *M, LEXINFO *L, char quote_char, short *ret)
{
	short c;
	short d;

	if (!readChar (M, L, &c))
		return FALSE;
	d = c;
	
	if (d == L->escape_char)
	{
		if (!readChar (M, L, &c))
			return FALSE;
		
		/* Wir haben den escape_char (`\' unter UNIX) eingelesen
		 * und mÅssen abhÑngig vom nÑchsten Zeichen entweder:
		 * - den escape_char durchlassen und das Zeichen merken
		 * - oder das Zeichen gequoted zurÅckgeben.
		 * - oder die nÑchste Zeile anfangen.
		 */
		if ((c == '\n') || (c == '\r'))
		{
			/* Hier sollte eine neue Zeile gelesen werden
			 */
			return NextChar (M, L, quote_char, ret);
		}
		else if (quote_char && (c != quote_char) 
			&& (strchr (ESCAPED_CHARS, c) == NULL)
			&& (c != L->escape_char))
		{
			/* Die Bedingung lÑût sich so lesen:
			 * Wenn wir dabei sind zu quoted (quote_char)
			 * und nicht den Quote escapen wollen (c != )
			 * und c nicht "`$" ist (ESCAPE_CHARS)
			 * und c nicht der escape_char selbst ist,
			 * dann lassen den escape_char durch
			 */
			L->peek_char = c;
		}
		else
		{
			/* ansonsten: escape_char diente zum quoten von c!
			 */
			d = c | QUOTED;
		}
	}
	
	*ret = d;
	return TRUE;
}

/* Lese bis Zeichen Whitespace enthalten oder Ende der Zeile
 * erreicht.
 */
int SkipWhitespace (MGLOBAL *M, LEXINFO *L, short *d)
{
	do
	{
		if (!NextChar (M, L, 0, d))
			return FALSE;
	}
	while (*d && !(*d & QUOTED) && strchr (" \t", *d));

	return TRUE;
}

/* Speichere <Character> in <L->token_name>. Vergrîûere den
 * Speicher, wenn nîtig. Gibt FALSE zurÅck, wenn kein Speicher
 * mehr vorhanden.
 */
static int storeInToken (MGLOBAL *M, LEXINFO *L, short character)
{
	/* Reicht der Speicher noch?
	 */
	if (L->token_buffer_offset >= L->token_buffer_length)
	{
		short *tmp;
		
		L->token_buffer_length += TOKEN_BUFFER_GROW;
		tmp = mrealloc (&M->alloc, L->token_name, 
			L->token_buffer_length * sizeof(short));

		if (!tmp)
		{
			clearTokenBuffer (M, L);
			L->broken = TRUE;
			L->break_reason = BROKEN_MALLOC;
			return FALSE;
		}
		L->token_name = tmp;
		
		/* Erste Allozierung: String initialisieren
		 */
		if (L->token_buffer_length == TOKEN_BUFFER_GROW)
			L->token_name[0] = '\0';
	}
	
	L->token_name[L->token_buffer_offset++] = character;
	L->token_quoted_chars = 
		(L->token_quoted_chars || (character & QUOTED));
	L->token_dollar_present = 
		(L->token_dollar_present || (character == '$'));

	return TRUE;
}

int EofOrSeparator (unsigned char c)
{
	return ((c == EOF_CHAR)
#if HAT_COMPATIBLE
		|| (strchr (" \t\n|^()&;<>", c) != NULL));
#else
		|| (strchr (" \t\n|()&;<>", c) != NULL));
#endif
}

int CommandSeparator (unsigned char c)
{
	return ((c == EOF_CHAR)
#if HAT_COMPATIBLE
		|| (strchr ("\n|^(;`", c) != NULL));
#else
		|| (strchr ("\n|(;`", c) != NULL));
#endif
}

static int fillWordInfo (MGLOBAL *M, LEXINFO *L)
{
	char *name, *quoted;
	size_t i, len;
	
	/* Gib allozierte Arrays wieder frei
	 */
	mfree (&M->alloc, L->token.name);
	mfree (&M->alloc, L->token.quoted);
	FreeSubstListInfo (M, L->token.list);
	
	len = L->token_buffer_offset;
	name = mmalloc (&M->alloc, len + 1);
	quoted = mmalloc (&M->alloc, len + 1);

	if (!quoted || !name)
	{
		L->broken = TRUE;
		L->break_reason = BROKEN_MALLOC;
		mfree (&M->alloc, name);
		mfree (&M->alloc, quoted);
		return FALSE;
	}

	for (i = 0; i < len; ++i)
	{
		name[i] = (char)(L->token_name[i] & 0xFF);
		quoted[i] = ((L->token_name[i] & 0xFF00) != 0);
	}
	name[len] = quoted[len] = '\0';
	
	L->token.name = name;
	L->token.quoted = quoted;

	L->token.list = L->token_subst_list;
	L->token_subst_list = NULL;

	L->token.flags.assignment = L->token_assignment;
	L->token.flags.quoted_chars = L->token_quoted_chars;
	L->token.flags.dollar_present = L->token_dollar_present;

	/* Tokenbuffer wieder freigeben
	 */
	mfree (&M->alloc, L->token_name);
	L->token_name = NULL;
	L->token_buffer_offset = L->token_buffer_length = 0L;

	return TRUE;
}


static int parseSpecialCommandSubst (MGLOBAL *M, LEXINFO *L)
{
	SUBSTLISTINFO *list, **last;
	PARSINFO P;
	
	list = mcalloc (&M->alloc, 1, sizeof (SUBSTLISTINFO));
	if (!list)
		return FALSE;
		
	if (!pushBackInput (M, L))
		return FALSE;
	
	list->tree = Parse (M, &P, ')');
	
	if (list->tree == NULL && P.L.broken)
	{
		mfree (&M->alloc, list);
		L->break_reason = P.L.break_reason;
		return FALSE;
	}
	
	last = &L->token_subst_list;
	while (*last != NULL)
	{
		last = &((*last)->next);
	}
	*last = list;
	
	return TRUE;
}


/*
 * Bearbeite HEREDOCS.
 */
static int copyIoList (MGLOBAL *M, LEXINFO *L, IOINFO *io)
{
	char *end_string, *subst_string, *subst_quoted;
	int strip_input, was_quoted;
	int fhandle;
	int retcode;
#define BUFFLEN 512
	char *buffer, *quoted;
	short c, d;
	char c_quoted;
	int offset;

	/* Ende der List erreicht.
	 */
	if (!io)
		return TRUE;

	/* Wir haben beim parsen die IOINFOs an den Anfang der Liste
	 * eingefÅgt. Bearbeite sie also in umgekehrter Reihenfolge.
	 */
	if (!copyIoList (M, L, io->here_next))
		return FALSE;
	
	/* Ermittle den String, bis zu dem wir lesen mÅssen. Gebrauche
	 * dazu keine command-substitution.
	 */
	was_quoted = io->name->flags.quoted_chars;
	if (!SubstWithCommand (M, io->name, &subst_string, &subst_quoted))
	{
		L->break_reason = BROKEN_USER;
		return FALSE;
	}

	end_string = subst_string;
	
	strip_input = io->file & IO_STRIP;
	
	/* Wenn ein Zeichen von <io->name> gequoted war, dann
	 * werden die Eingabezeichen nicht gesondert behandelt
	 */
	if (was_quoted)
		io->file &= ~IO_DOC;
	
	/* io->filename wird gesetzt, da dies eine von der Shell
	 * angelegte Datei ist, die beim Lîschen dieser IOINFO
	 * auch gelîscht werden muû
	 */
	if ((io->filename = TmpFileName (M)) == NULL)
	{
		mfree (&M->alloc, subst_string);
		mfree (&M->alloc, subst_quoted);
		return FALSE;
	}
	io->is_our_tmpfile = TRUE;

	fhandle = (int)Fcreate (io->filename, 0);
	if (fhandle < 0)
	{
		mfree (&M->alloc, subst_string);
		mfree (&M->alloc, subst_quoted);
		return FALSE;
	}

	if (strip_input)
	{
		/* strippe auch den zu matchenden String
		 */
		io->file &= ~IO_STRIP;
		while ((*end_string == '\t') || (*end_string == ' '))
			++end_string;
	}

	/* Alloziere die IO-Buffer
	 */
	buffer = mmalloc (&M->alloc, BUFFLEN + 2);	
	quoted = mmalloc (&M->alloc, BUFFLEN + 2);

	if (!quoted || !buffer)
	{
		Fclose (fhandle);
		mfree (&M->alloc, subst_string);
		mfree (&M->alloc, subst_quoted);
		mfree (&M->alloc, buffer);
		mfree (&M->alloc, quoted);
		
		return FALSE;
	}
	
	retcode = TRUE;
	offset = 0;
	
	for (;;)
	{
		if (was_quoted)
		{
			do
			{
				if (!readChar (M, L, &c))
					retcode = FALSE;
			}
			while (retcode && strip_input 
				&& ((c == '\t') || (c == ' ')));
			
			while (retcode && (c != '\n') && (c != EOF_CHAR))
			{
				buffer[offset++] = c;
				if (offset == BUFFLEN)
				{
					retcode = 
						(Fwrite (fhandle, offset, buffer) == offset);
					offset = 0;
				}
				if (!readChar (M, L, &c))
					retcode = FALSE;
			}

			if (!retcode)
				break;
		}
		else
		{
			do
			{
				if (!NextChar (M, L, *end_string, &d))
					retcode = FALSE;
				c = (unsigned char)d & 0xFF;
				c_quoted = ((d >> 8) != 0);
			}
			while (retcode && strip_input && !c_quoted
				&& ((c == '\t') || (c == ' ')));
			
			while (retcode 
				&& (c_quoted || ((c != '\n') && (c != EOF_CHAR))))
			{
				buffer[offset] = c;
				quoted[offset++] = c_quoted;
				
				if (offset == BUFFLEN)
				{
					/* Die Zeile war zu lang.
					 */
					retcode = FALSE;
				}

				if (!NextChar (M, L, *end_string, &d))
					retcode = FALSE;
				
				if ((!c_quoted && c == '$')
					&& (!(d & QUOTED) && (d == '(')))
				{
					/* Hier wird eine Command-Substitution mittels
					 * $() eingeleitet. Wir mÅssen eine neue
					 * Parserumgebung îffnen.
					 */
					if (!parseSpecialCommandSubst (M, L))
					{
						return FALSE;
					}
				}

				c = (unsigned char)d & 0xFF;
				c_quoted = ((d >> 8) != 0);
			}

			if (!retcode)
				break;
		}

		buffer[offset] = '\0';	/* fÅr strcmp */
		if ((c == EOF_CHAR) || !strcmp (buffer, end_string))
		{
			if (c == EOF_CHAR)
				Fwrite (fhandle, offset, buffer);
			break;
		}
		else
		{
			if (offset && !was_quoted)
			{
				char *sub, *qot;
				WORDINFO w;
				size_t len;
				
				memset (&w, 0, sizeof (WORDINFO));
				w.name = buffer;
				w.quoted = quoted;
				w.list = L->token_subst_list;
				L->token_subst_list = NULL;

				if (!SubstWithCommand (M, &w, &sub, &qot))
				{
					L->break_reason = BROKEN_SUBST;
					retcode = FALSE;
					break;
				}
				
				len = strlen (sub);
				if (Fwrite (fhandle, len, sub) != len)
				{
					eprintf (M, NlsStr(LEX_WRITE_ERROR), io->filename);
					break;
				}
				
				offset = 0;
					
				mfree (&M->alloc, sub);
				mfree (&M->alloc, qot);
			}
		
			buffer[offset++] = '\r';
			buffer[offset++] = '\n';

			if (Fwrite (fhandle, offset, buffer) != offset)
			{
				eprintf (M, NlsStr(LEX_WRITE_ERROR), io->filename);
				break;
			}
		}
		
		offset = 0;
	}

	Fclose (fhandle);
	mfree (&M->alloc, buffer);
	mfree (&M->alloc, quoted);
	mfree (&M->alloc, subst_string);
	mfree (&M->alloc, subst_quoted);
	
	return retcode;
}


/* FÅhrt ein Alias-Expansion durch, falls das gelesene Token
 * ein Alias ist. Es wird nicht auf reservierte Wîrter geprÅft.
 */
static int makeAliasExpansion (MGLOBAL *M, LEXINFO *L, 
	int first_alias_search, int *is_alias)
{
	*is_alias = FALSE;

	/* Zuweisung -> kein Alias
	 */
	if (L->token_assignment)
		return TRUE;
	
	/* gequotete Wîrter -> kein Alias
	 */
	if (L->token_quoted_chars)
		return TRUE;
	
	if (L->input_line == NULL
		|| (L->input_line_offset <= L->possible_alias_start))
	{
		return TRUE;
	}
	
	/* Wir haben also etwas gelesen, daû ein alias sein kînnte
	 */
	{
		const char *alias;
		char *line;
		size_t len;
		int retcode = TRUE;
		
		len = L->input_line_offset - L->possible_alias_start - 1;
		line = mcalloc (&M->alloc, len + 2, 1);
	
		if (!line)
			return FALSE;
					
		strncat (line, &L->input_line[L->possible_alias_start], len);
		
		alias = GetAlias (M, line, first_alias_search);

		if (alias)
		{
			char *new_line;
			
			*is_alias = TRUE;
			
			/* L->input_line_offset muû dekrementiert werden, da
			 * das Zeichen, das das Token terminiert hat, schon
			 * gelesen wurde, aber nicht verloren gehen darf!
			 */
			new_line = mmalloc (&M->alloc, strlen (alias) + 2 +
				strlen (&L->input_line[--L->input_line_offset]));
			
			if (new_line)
			{
				strcpy (new_line, alias);
				strcat (new_line, 
					&L->input_line[L->input_line_offset]);

				storeNewInputLine (M, L, new_line, TRUE);
			}
			else
			{
				L->break_reason = BROKEN_MALLOC;
				L->broken = TRUE;
				retcode = FALSE;
			}
		}
		
		mfree (&M->alloc, line);
		
		return retcode;
	}
}

/* Liefert das nÑchste Token zurÅck
 */
static int readToken (MGLOBAL *M, LEXINFO *L, int first_alias_search)
{
	short c, d;
	char may_be_assignment = TRUE;
	
	L->token_number = 0L;
	L->token_assignment = FALSE;
	L->token_quoted_chars = FALSE;
	L->token_dollar_present = FALSE;
	
	/* öberspringe Whitespace und Kommentare vorm Start eines
	 * Tokens
	 */
	while (TRUE)
	{
		if (!SkipWhitespace (M, L, &d))
			return FALSE;

		if (d == '#')
		{
			do
			{
				if (!readChar (M, L, &c))
					return FALSE;
			}
			while ((c != '\n') && (c != EOF_CHAR));

			L->peek_char = c;
			d = c;
		}
		else
			break;
	}
	
	/* Entweder fÑngt jetzt ein Token an, oder die Zeile ist
	 * schon zu Ende, oder wir haben einen Separator vor uns,
	 * der einzelne Tokens auch ohne Blanks trennt.
	 */
	if ((d & QUOTED) || !EofOrSeparator (d))
	{
		/* Wenn wir nach einem reservierten Wort suchen, kann es
		 * sich auch um eine Alias-Expansion handeln. Daher mÅssen
		 * wir uns die Stelle im String merken, an der wir jetzt
		 * stehen.
		 */
		if (L->may_be_reserved || L->may_be_alias)
			L->possible_alias_start = L->input_line_offset - 1;
			
		do
		{
			/* Zuerst einmal kÅmmern wir uns ums Quoten mit 
			 * Single-Quotes. Diese werden in Double-Quotes
			 * verpackt und alle Zeichen darin werden mit
			 * QUOTED markiert.
			 */
			if (d == '\'')
			{
				if (!storeInToken (M, L, '"'))
					return FALSE;
				
				if (!readChar (M, L, &c))
					return FALSE;
					
				while (c != '\'')
				{
					if (c == EOF_CHAR)
					{
						L->broken = TRUE;
						L->looking_for = '\'';
						L->break_reason = BROKEN_EOF;
						return FALSE;
					}
						
					if (!storeInToken (M, L, c | QUOTED))
						return FALSE;
					
					if (!readChar (M, L, &c))
						return FALSE;
				}
				d = c;
				
				if (!storeInToken (M, L, '"'))
					return FALSE;
			}
			else
			{
				if (!storeInToken (M, L, d))
					return FALSE;
				
				if (d == '=')
					L->token_assignment |= may_be_assignment;
				
				c = (unsigned char)(d & 0xFF);
				if (!(IsLetter (c) || isdigit (c)))
					may_be_assignment = FALSE;
				
				/* Hier wird erkannt, ob es sich um ein Quoten mit
				 * " oder ` oder $( handelt. Wenn ja, wird bis zum
				 * entsprechenden Zeichen weitergelesen.
				 */
				if ((!(d & QUOTED)) && d == '$')
				{
					if (!NextChar (M, L, 0, &d))
						return FALSE;
					
					if (!(d && ((d & QUOTED) || (d == '(') || !EofOrSeparator (d))))
						break;
						
					if (!storeInToken (M, L, d))
						return FALSE;
					
					if ((!(d & QUOTED)) && d == '(')
					{
						/* Hier wird eine Command-Substitution mittels
						 * $() eingeleitet. Wir mÅssen eine neue
						 * Parserumgebung îffnen.
						 */
						if (!parseSpecialCommandSubst (M, L))
						{
							return FALSE;
						}
					}
				}
				else if ((!(d & QUOTED)) && strchr ("`\"", d))
				{
					short quote_char = d;
					short read_char;
					
					L->looking_for = (char)d;
					
					do
					{
						if (!NextChar (M, L, quote_char, &read_char))
							return FALSE;

						if ((!(read_char & QUOTED)) 
							&& read_char == '$')
						{
							if (!storeInToken (M, L, read_char))
								return FALSE;

							if (!NextChar (M, L, quote_char, &read_char))
								return FALSE;
							
							if ((!(read_char & QUOTED)) 
								&& read_char == '(')
							{
								/* Hier wird eine Command-Substitution mittels
								 * $() eingeleitet. Wir mÅssen eine neue
								 * Parserumgebung îffnen.
								 */
								if (!parseSpecialCommandSubst (M, L))
								{
									return FALSE;
								}
							}
						}
						else if (read_char == EOF_CHAR)
						{
							L->broken = TRUE;
							L->looking_for = quote_char;
							L->break_reason = BROKEN_EOF;
							return FALSE;
						}

						if (!storeInToken (M, L, read_char))
							return FALSE;
					}
					while (read_char != quote_char);
				}
			}
			
			if (!NextChar (M, L, 0, &d))
				return FALSE;
		}
		while (d && ((d & QUOTED) || !EofOrSeparator (d)));
			
		/* Hier habe wir ein komplettes Wort gelesen. Es wird
		 * jetzt geguckt, was fÅr ein Zeichen das Lesen
		 * unterbrochen hat.
		 */
		if (!storeInToken (M, L, 0))
			return FALSE;
			
		if (!IsLetter (L->token_name[0]))
			L->token_assignment = FALSE;
		
		/* Zuletzt gelesenes Zeichen merken
		 */
		L->peek_next = d;
		if ((L->token_name[1] == 0)
			&& (isdigit (L->token_name[0]))
			&& (d == '>' || d == '<'))
		{
			/* Wir haben eine einstellige Zahl gelesen, der
			 * ein '<' oder '>' folgt. Es muû sich um eine
			 * IO-Redirection handeln
			 */
			if (!readToken (M, L, first_alias_search))
				return FALSE;
			
			/* Wir mÅssen den Tokenbuffer lîschen, damit die
			 * gelesene Zahl daraus verschwindet. Diese merken
			 * wir uns in <token_number>
			 */
			L->token_number = L->token_name[0] - '0';
			clearTokenBuffer (M, L);
		}
		else
		{
			/* Suche nach reservierten Wîrtern.
			 */
			if (L->may_be_reserved
				&& ((L->token_value = reservedWord (L)) != 0))
			{
				clearTokenBuffer (M, L);
			}
			else
			{
				/*
				 * Hier ist die Stelle, an der nach aliasen
				 * gesucht wir, wenn reservierte Wîrter zugelassen
				 * sind.
				 */
				if (L->may_be_reserved || L->may_be_alias)
				{
					int is_alias;

					if (!makeAliasExpansion (M, L, 
						first_alias_search, &is_alias))
					{
						return FALSE;
					}
					
					if (is_alias)
					{
						clearTokenBuffer (M, L);
						L->peek_char = L->peek_next = 0;
						return readToken (M, L, FALSE);
					}
				}

				L->token_value = 0;
				if (!fillWordInfo (M, L))
					return FALSE;
			}
			
		}
	}
	else if ((d != EOF_CHAR) && !(d & QUOTED) && strchr ("|&;<>", d))
	{
		/* In diesem Fall haben wir einen echten Separator
		 * gelesen. Also geben wir das Token dafÅr zurÅck
		 * FÑlle wie >> und << mÅssen gesondert behandelt
		 * werden.
		 */
		c = d;
		if (!NextChar (M, L, 0, &d))
			return FALSE;
		
		if (d == c)
		{
			switch (c)
			{
				case '<':
					/* Wir haben ein "<<" gelesen
					 */
					if (!NextChar (M, L, 0, &d))
						return FALSE;
					
					if (d == '-')
						L->token_value = HERE_DOC_MINUS;
					else
					{
						L->peek_next = d;
						L->token_value = HERE_DOC;
					}
					break;

				case '>':
					L->token_value = GREATER_GREATER;
					break;
				
				case '&':
					L->token_value = AND_AND;
					break;
				
				case '|':
					L->token_value = OR_OR;
					break;
				
				case ';':
					L->token_value = SEMI_SEMI;
					break;
			}
		}
		else
		{
			L->peek_next = d;
			L->token_value = c;
		}
	}
	else
	{
		if (d == EOF_CHAR)
		{
			L->token_value = EOF_TOKEN;
		}
		else
			L->token_value = d;

		/* Behandle pending IO, wenn ein Zeile zu Ende gelesen ist.
		 */
		if (*L->pending_io_list && (d == '\n' || d == EOF_CHAR))
		{
			/* Lese HERE_DOCs
			 */
			if (!copyIoList (M, L, *L->pending_io_list))
				return FALSE;

			*L->pending_io_list = 0;
		}
	}

	L->may_be_reserved = FALSE;
	return TRUE;
}

int ReadToken (MGLOBAL *M, LEXINFO *L)
{
	L->looking_for = '\0';
	return readToken (M, L, TRUE);
}


#undef LEX_DEBUG
#ifdef LEX_DEBUG

static SYMBOL tnames[] =
{
	{"if", IF},
	{"then", THEN},
	{"else", ELSE},
	{"elif", ELIF},
	{"fi", FI},
	{"case", CASE},
	{"esac", ESAC},
	{"for", FOR},
	{"while", WHILE},
	{"until", UNTIL},
	{"do", DO},
	{"done", DONE},
	{"in", IN},
	{"{", '{'},
	{"}", '}'},
	{"AND_AND",AND_AND},
	{"OR_OR", OR_OR},
	{"GREATER_GREATER", GREATER_GREATER},
	{"HERE_DOC", HERE_DOC},
	{"LESS_AND", LESS_AND},
	{"GREATER_AND", GREATER_AND},
	{"SEMI_SEMI", SEMI_SEMI},
	{"HERE_DOC_MINUS", HERE_DOC_MINUS},
	{"AND_GREATER", AND_GREATER},
	{"CURLY_OPEN", CURLY_OPEN},
	{"CURLY_CLOSE", CURLY_CLOSE},
	{"EOF_TOKEN", EOF_TOKEN},
	{NULL, 0},
};

void PrintToken (MGLOBAL *M, LEXINFO *L)
{
	SYMBOL *sp = tnames;
	
	if (L->token_value)
	{
		while (sp->token_name)
		{
			if (L->token_value == sp->token_value)
			{
				mprintf (M, "Token(%s) ", sp->token_name);
				return;
			}
			++sp;
		}

		mprintf (M, "Tchar(%c) ", (char)L->token_value);
	}
	else
	{
		if (L->token.name)
			mprintf (M, "(%s) ", L->token.name);
		else if (L->token_name)
			mprintf (M, "(%s) ", "NULL");
		else
			mprintf (M, "UNKNOWN ");
	}
}

#endif /* LEX_DEBUG */
