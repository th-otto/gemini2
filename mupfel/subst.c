/*
 * @(#) Mupfel\Subst.c
 * @(#) Stefan Eissing, 28. Januar 1995
 *
 * jr 8.8.94
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tos.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\wildmat.h"
#include "mglob.h"

#include "partypes.h"
#include "subst.h"
#include "names.h"
#include "nameutil.h"
#include "maketree.h"
#include "mparse.h"
#include "parsinpt.h"
#include "exectree.h"
#include "redirect.h"
#include "mdef.h"
#include "shellvar.h"
#include "chario.h"


#define BUFFER_GROW		512


typedef struct
{
	WORDINFO *name;			/* Das Wort, das wir bearbeiten */
	SUBSTLISTINFO *act_list;/* Comm.-Subst. Liste in der Reihe */
	size_t name_index;		/* aktueller Index in name->name */
	
	char *result_name;		/* Ergebnis-String */
	char *result_quoted;	/* Ergebniszeichen, die gequoted sind */
	
	size_t result_size;		/* gemallocte LÑnge von result */
	size_t result_index;	/* aktuelle Schreibposition */
	
	int quoted;				/* Zeichen in <name> waren gequoted */
	int was_at;				/* $@ in der substitution */
	
	struct
	{
		unsigned command_subst : 1; /* Kommando-Subst. ist erlaubt */
		unsigned quote_it : 1;	/* Quote beim Speichern in result */
	} flags;
	
} SUBINFO;


/* Einige VorwÑrts-Deklarationen
 */
static int copySubstTill (MGLOBAL *M, SUBINFO *S, char end_char);
static int skipSubstTill (MGLOBAL *M, SUBINFO *S, char end_char);
static int substCommand (MGLOBAL *M, SUBINFO *S, int take_from_list);


/* Entferne gequotete 0-Zeichen aus <name>. Nur eine ungequotete
 * 0 markiert das Ende einer Substitution! <name> und <quoted>
 * werden entsprechend komprimiert. <name> ist danach ein gÅltiger
 * C-String.
 */
static size_t removeQuotedZeros (size_t start, size_t end, 
	char *name, char *quoted)
{
	size_t read_index, write_index;
	size_t i;
	
	write_index = read_index = start;
	
	for (i = start; i < end; ++i, ++read_index)
	{
		name[write_index] = name[read_index];
		quoted[write_index] = quoted[read_index];

		if (name[read_index])
			++write_index;
	}

	if (write_index < read_index)
		name[write_index] = quoted[write_index] = '\0';
	
	return (read_index - write_index);
}

/* Speichere ein Zeichen in S->result_name mit Quoting-Informationen
 * in S->result_quoted ab. Der Speicher fÅr diese Strings wird
 * automatisch vergrîûert.
 * Ist nicht genug Speicher vorhanden, wird FALSE zurÅckgeliefert.
 */
static int storeInResult (MGLOBAL *M, SUBINFO *S, char c, char quoted)
{
	/* Genug Speicher vorhanden?
	 */
	if ((!S->result_name)
		 || ((S->result_index + 1) >= S->result_size))
	{
		S->result_size += BUFFER_GROW;

		S->result_name = mrealloc (&M->alloc, S->result_name, 
			S->result_size);
		S->result_quoted = mrealloc (&M->alloc, S->result_quoted, 
			S->result_size);
		
		if (!S->result_name || !S->result_quoted)
		{
			return FALSE;
		}
	}

	S->result_name[S->result_index] = c;
	S->result_quoted[S->result_index] = quoted;
	++S->result_index;
	S->result_name[S->result_index] = '\0';
	S->result_quoted[S->result_index] = '\0';
		
	return TRUE;
}

/* HÑnge an S->result eine 0 an, damit der String definitiv ein
 * Ende hat. VerÑndere aber den Schreibindex nicht!
 */
static int appendZeroToResult (MGLOBAL *M, SUBINFO *S)
{
	if (!storeInResult (M, S, '\0', 0))
		return FALSE;
	
	--S->result_index;
	return TRUE;
}


/* PrÅfe auf mîgliche Tilde-Expansion und fÅhre sie ggfs. durch
 */
static int makeTildeExpansion (MGLOBAL *M, SUBINFO *S, char end_char,
	int first_character)
{
	char *name, *quoted;
	const char *value = NULL;
	size_t forget;
	
	if (end_char == '~')
		return TRUE;
	
	name = &S->name->name[S->name_index];
	quoted = &S->name->quoted[S->name_index];
	if (!*quoted && *name == '~')
	{
		/* Wir haben eine ungequotete Tilde vorliegen!
		 *
		 * Wir prÅfen auf:
		 * ~ ~/ ~\           wird zu     $HOME
		 */
		if (!quoted[1])
		{
			if (name[1] == '\\' 
				|| (first_character && (name[1] == end_char))
				|| (M->shell_flags.slash_conv && name[1] == '/'))
			{
				value = GetEnv (M, "HOME");
				forget = 1;
			}
			else if (name[1] == '+')
			{
				value = GetEnv (M, "PWD");
				forget = 2;
			}
			else if (name[1] == '-')
			{
				value = GetEnv (M, "OLDPWD");
				forget = 2;
			}
		}
	}
	
	if (value)
	{		
		S->name_index += forget;

		while (*value)
		{
			if (!storeInResult (M, S, *value, 1))
				return FALSE;
			++value;
		}
	}
	
	return TRUE;
}


/* Lese ein Zeichen aus S->name. Kann nichts schiefgehen.
 */
static void getSChar (SUBINFO *S, char *c, char *c_quoted)
{
	*c = S->name->name[S->name_index];
	*c_quoted = S->name->quoted[S->name_index];
	++S->name_index;
}

				
/* Remove Suffix pattern
 */
static int removeSuffix (MGLOBAL *M, SUBINFO *S, char *string, 
	const char *pattern, int largest)
{
	size_t i, delta;
	int found = FALSE;
	size_t max_len;
	char save, *result;
	
	if (!string)
		string = "";
	
	max_len = strlen (string);	
	if (largest)
	{
		i = 0;
		delta = +1;
	}
	else
	{
		i = max_len;
		delta = -1;
	}
	
	for (; (i >= 0) && (i <= max_len); i += delta)
	{
		if (wildmatch (string + i, pattern, TRUE))
		{
			save = string[i];
			string[i] = '\0';
			found = TRUE;
			break;
		}
	}

	/* Speichere das Wort
	 */
	result = string;
	while (*result)
	{
		if (!storeInResult (M, S, *result, S->flags.quote_it))
			return FALSE;
		++result;
	}
	
	if (found)
		string[i] = save;
	
	return TRUE;
}
				

/* Remove Prefix pattern
 */
static int removePrefix (MGLOBAL *M, SUBINFO *S, char *string, 
	const char *pattern, int largest)
{
	size_t i, delta;
	int found = FALSE;
	size_t max_len;
	char save, *result;
	
	if (!string)
		string = "";
	
	max_len = strlen (string);	
	if (!largest)
	{
		i = 0;
		delta = +1;
	}
	else
	{
		i = max_len;
		delta = -1;
	}
	
	for (; (i >= 0) && (i <= max_len); i += delta)
	{
		save = string[i];
		string[i] = '\0';
		found = wildmatch (string, pattern, TRUE);
		string[i] = save;
		
		if (found)
			break;
	}

	/* Speichere das Wort
	 */
	if (found)
		result = string + i;
	else
		result = string;
		
	while (*result)
	{
		if (!storeInResult (M, S, *result, S->flags.quote_it))
			return FALSE;
		++result;
	}
	
	return TRUE;
}
				

/* Die absolute HÑrte-Giga-Multi-Routine! Hier geht die Post ab.
 * Dies ist das Herz der Substitution und nicht so einfach zu
 * verstehen:
 * Im Prinzip wird ein Zeichen aus S->name gelesen und zurÅck-
 * geliefert. Dies geschieht immer dann, wenn das Zeichen aus
 * S->name gequoted ist, oder *nicht* eins der Zeichen $, ` oder
 * " ist. Ungequoted mÅssen diese Zeichen gesondert behandelt
 * werden.
 *
 * $ - Dieser Fall ist am einfachsten. Entweder haben wir $word,
 *     oder ${...}. 
 * ` - Dieses Zeichen leitet eine Kommando-Substitution ein. Die
 *     Routine substCommand erledigt das.
 * " - Man merkt sich, daû die kommenden Zeichen bis zum nÑchsten
 *     " gequoted werden mÅssen (quote_it ^= 1) und daû der Name
 *     Åberhaupt gequoted war.
 *
 * Der Witz ist, daû in allen FÑllen wieder getSubChar aufgerufen 
 * wird und somit verschachtelte Substitutionen auftreten kînnen.
 * 
 * Interessant ist noch, daû ja nicht immer bis zum Stringende
 * substituiert werden soll. "${name:-`echo test`blub}bla" ist so ein
 * Kandidat. Zuerst subst. man bis zum Stringende. Innen soll dann,
 * falls <name> nicht gesetzt ist bis zum "}" subst. werden. 
 * Andernfalls spielt "}" bei "test {}" keine besondere Rolle.
 * Um dieses Problem zu lîsen, wird zusÑtzlich noch das Zeichen
 * Åbergeben, an dem die Substitution aufhîren soll.
 * Alles klar?
 */
static int getSubChar (MGLOBAL *M, SUBINFO *S, char end_char, 
	char *result, char *result_quoted)
{
	char c;
	char c_quoted;
	
	makeTildeExpansion (M, S, end_char, FALSE);

	/* Lese ein Zeichen
	 */
	getSChar (S, &c, &c_quoted);
	
	/* Leitet dies Zeichen eine Substitution ein?
	 */
	if (c_quoted || (strchr ("$`\"", c) == NULL))
	{
		*result = c;
		*result_quoted = c_quoted;
		return TRUE;
	}
	
	if (c == '$')
	{
		char c2, c2_quoted;
		
		getSChar (S, &c2, &c2_quoted);
	
		if (c2 && !c2_quoted && IsValidDollarChar (c2))
		{
			int is_bracket, null_flag;
			int is_strlen, is_double;
			const char *id_name, *id_value;
			char id_tmp[OPTION_LEN], id_tmp_name[3];
			int subst_var = 0; 
			long arg_index;
			
			/* Dem Dollar folgt ein ungequotetes Zeichen, das ein
			 * gÅltiges Zeichen nach einem Dollar sein kann.
			 * Versuche den Wert zu ermitteln
			 */
			
			is_bracket = (c2 == '{');
			if (is_bracket)
				getSChar (S, &c2, &c2_quoted);
			
			is_strlen = is_bracket && !c2_quoted && c2 == '#';
			if (is_strlen)
				getSChar (S, &c2, &c2_quoted);

			if (!c2_quoted && IsLetter (c2))
			{
				NAMEINFO *n;
				
				/* Wir haben igendwas wie $name oder ${name...}
				 * Lese den Namen ein. Dazu wird S->result_name
				 * miûbraucht. S->result_index wird gesichert und
				 * nach dem Lesen und Suchen des Namens wiederher-
				 * gestellt.
				 */
				arg_index = S->result_index;
				while (!c2_quoted && (IsLetter (c2) || isdigit (c2)))
				{
					if (!storeInResult (M, S, c2, 0))
						return FALSE;
					getSChar (S, &c2, &c2_quoted);
				}
				if (!appendZeroToResult (M, S))
					return FALSE;
					
				/* Wir haben nun den Identifier in S->result_name
				 * ab Position arg_index eingelesen.
				 */
				n = MakeOrGetNameInfo (M, &S->result_name[arg_index]);
				S->result_index = arg_index;
				if (!n)
					return FALSE;
					
				/* n ist jetzt die NameInfo zu dem Identifier.
				 * Funktionen sind nicht erlaubt.
				 */
				if (n->flags.function)
				{
					eprintf (M, "Substitution einer Funktion nicht erlaubt\n");
					return FALSE;
				}
				
				id_name = n->name;
				id_value = n->val;
				--S->name_index;
			}
			else if (!c2_quoted && c2
				&& (isdigit (c2) || (c2 == '*') || (c2 == '@')))
			{
				/* Wir haben $0 .. $9 oder $* oder $@
				 */
				id_tmp_name[0] = c2;
				id_tmp_name[1] = '\0';
				id_name = id_tmp_name;
				
				if ((c2 == '*') || (c2 == '@'))
				{
					if (c2 == '@')
						++S->was_at;
						
					c2 = '1';
					subst_var = 1;
				}

				c2 -= '0';
				id_value = GetShellVar (M, c2);

				if (id_value == NULL)
					subst_var = 0;

			}
			else if (!c2_quoted && (c2 == '$'))
			{
				/* $$ unsere Prozeû-ID 
				 */
				sprintf (id_tmp, "%ld", M->our_pid);
				id_value = id_tmp;
			}
			else if (!c2_quoted && (c2 == '!'))
			{
				/* $! Prozeû-ID des letzten Hintergrundpr.
				 */
				sprintf (id_tmp, "%ld", M->last_async_pid);
				id_value = id_tmp;
			}
			else if (!c2_quoted && (c2 == '#'))
			{
				/* $# Anzahl der Shell-Parameter */
				itoa (GetShellVarCount (M), id_tmp, 10);
				id_value = id_tmp;
			}
			else if (!c2_quoted && (c2 == '?'))
			{
				/* $? Returnwert des letzten Kommandos */
				itoa (M->exit_value, id_tmp, 10);
				id_value = id_tmp;
			}
			else if (!c2_quoted && (c2 == '-'))
			{
				/* $- String mit Shell-Flags */
				GetOptionString (M, id_tmp);
				id_value = id_tmp;
			}
			else if (is_bracket)
			{
				/* Wir haben ${{, das ist definitiv falsch. Wir
				 * fangen das hier ab.
				 */
				eprintf (M, "falsche Parametersubstitution\n");
				return FALSE;
			}
			else if (!c2_quoted && (c2 == '('))
			{
				/* Mache CommandSubstitution $(cmd)
				 */
				if (!substCommand (M, S, TRUE))
					return FALSE;
				return getSubChar (M, S, end_char, 
					result, result_quoted);
			}
			else
				return getSubChar (M, S, end_char, result, result_quoted);
			
			getSChar (S, &c2, &c2_quoted);
			
			/* Testen wir auf ungesetzte Variablen oder auch auf
			 * leere Variablen? null_flag == "auch leere"
			 */
			null_flag = (!c2_quoted && (c2 == ':') && is_bracket);
			if (null_flag)
			{
				/* Wenn wir in einer Klammer sind und ein ':'
				 * gelesen haben, hole das nÑchste Zeichen.
				 */
				getSChar (S, &c2, &c2_quoted);
			}
			
			if (is_bracket 
				&& (c2_quoted 
					|| (strchr ("+-?=%#}", c2) == NULL)))
			{
				/* Nach einem Wort in ${...} kann nur eines der
				 * obigen Zeichen kommen!
				 */
				eprintf (M, "falsche Parametersubstitution\n");
				return FALSE;
			}
			
			arg_index = -1;
			
			if (is_bracket)
			{
				/* Wir sind in einem ${...} Konstrukt und haben das
				 * erste Wort darin gelesen. Fehler wurden oben schon
				 * abgefangen.
				 */
				if (c2 != '}')
				{
					int null_value;
					
					/* Entweder +,-,? oder =
					 *
					 * Haben wir eine ungesetzte Variable vorliegen?
					 */
					null_value = (!id_value 
						|| (null_flag && !*id_value));
					
					if ((c2 == '%')	|| (c2 == '#'))
					{
						char c3, c3_quoted;
						/* ${parameter:.word}
						 * Suche nach Vorkommen von word in parameter
						 */
						getSChar (S, &c3, &c3_quoted);
						is_double = (!c3_quoted && (c3 == c2));
						if (!is_double)
							--S->name_index;
						arg_index = S->result_index;
						if (!copySubstTill (M, S, '}'))
							return FALSE;
						S->result_index = arg_index;
					}
					else if ((null_value && c2 != '+')
						|| (!null_value && c2 == '+'))
					{
						/* ${parameter:.word}
						 * Setze statt parameter word ein
						 */
						arg_index = S->result_index;
						if (!copySubstTill (M, S, '}'))
							return FALSE;
					}
					else
					{
						/* öberspringe word */
						if (!skipSubstTill (M, S, '}'))
							return FALSE;
					}
				}
			}
			else
			{
				--S->name_index;
				c2 = c2_quoted = 0;
			}
			
			if (!c2_quoted && (c2 == '%'))
			{
				/* Remove Suffix pattern
				 */
				if (!removeSuffix (M, S, (char *)id_value, 
					&S->result_name[arg_index], is_double))
					return FALSE;
			}
			else if (!c2_quoted && (c2 == '#'))
			{
				/* Remove Prefix pattern
				 */
				if (!removePrefix (M, S, (char *)id_value, 
					&S->result_name[arg_index], is_double))
					return FALSE;
			}
			else if (id_value && (!null_flag || *id_value))
			{
				/* id_value zeigt auf den Wert fÅr $name. also
				 * tragen wir ihn in S->result ein. Es sein denn,
				 * daû wir ein $name:+word hatten. Dann werfen wir
				 * id_value weg.
				 */
				
				if (is_strlen)
				{
					sprintf (id_tmp, "%ld", strlen (id_value));
					id_value = id_tmp;
				}
				
				if (c2 != '+')
				{
					for (;;)
					{
						if ((*id_value == '\0') && S->flags.quote_it)
						{
							if (!storeInResult (M, S, '\0', 1))
								return FALSE;
						}
						else
						{
							/* Speichere das Wort
							 */
							while (*id_value)
							{
								if (!storeInResult (M, S, *id_value, 
									S->flags.quote_it))
								{
									return FALSE;
								}
								++id_value;
							}
						}
						
						if ((subst_var == 0)
							|| (++subst_var > GetShellVarCount (M)))
						{
							break;
						}
						else
						{
							char separator = ' ';
							
							if (*id_name != '@')
							{
								separator = (GetIFSValue (M))[0];
							}
							
							id_value = GetShellVar (M, subst_var);
							if (!storeInResult (M, S, separator, 
								(*id_name == '*')? 
								S->flags.quote_it : 0))
							{
								return FALSE;
							}
							
						}
					}
				}
			}
			else if (arg_index >= 0)
			{
				/* Wir hatten keinen Wert fÅr ${name:.word}, also
				 * schauen wir uns an, was word ist. Word liegt in
				 * S->result_name bei argindex.
				 */
				
				if (!c2_quoted && (c2 == '?'))
				{
					eprintf (M, "%s: %s\n", id_name, 
						S->result_name[arg_index]? 
						&S->result_name[arg_index] : 
						"leere Parametersubstitution");
					if (!M->shell_flags.interactive)
						M->exit_shell = TRUE;
					return FALSE;
				}
				else if (!c2_quoted && (c2 == '='))
				{
					size_t count;
					
					count = removeQuotedZeros (
						arg_index,
						S->result_index,
						S->result_name,
						S->result_quoted
						);
					S->result_index -= count;

					if (!InstallName (M, id_name, 
						&S->result_name[arg_index]))
						return FALSE;
				}
			}
			else if (M->shell_flags.no_empty_vars)
			{
				eprintf (M, "leere Substitution\n");
				return FALSE;
			}
			
			return getSubChar(M, S, end_char, result, result_quoted);
		}
		else
		{
			/* Der Dollar stand fÅr sich, setze den Leseindex
			 * zurÅck, und gebe unten c zurÅck.
			 */
			--S->name_index;
		}
	}
	else if (c == end_char)
	{
		*result = c;
		*result_quoted = c_quoted;
		return TRUE;
	}
	else if ((c == '`') && S->flags.command_subst)
	{
		/* Mache CommandSubstitution
		 */
		if (!substCommand (M, S, FALSE))
			return FALSE;
		return getSubChar (M, S, end_char, result, result_quoted);
	}
	else if (c == '\"')
	{
		++S->quoted;
		S->flags.quote_it ^= 1;
		return getSubChar (M, S, end_char, result, result_quoted);
	}
	
	*result = c;
	*result_quoted = c_quoted;
	return TRUE;
}

/* Liest das Ergebnis einer Kommando-Substitution von StdIn
 * ein und speichert das Ergebnis mit storeInResult () ab.
 * Im Fehlerfall wird FALSE zurÅckgeliefert.
 */
static int storeFromStdIn (MGLOBAL *M, SUBINFO *S)
{
#define FILEEOF_CHAR	26
#define BUFSIZE		512L
#define READSIZE	512L
	char buffer[BUFSIZE + 1];
	char *eof = NULL;
	long size_read;
	int got_whitespace = FALSE, stored = FALSE;
	
	while (!eof 
		&& ((size_read = Fread (0, READSIZE, buffer)) > 0L))
	{
		size_t i;
		
		/* Wenn wir FILEEOF_CHAR lesen, so ist das logische Ende 
		 * dieser (Text-)Datei erreicht, auch wenn in der Datei noch
		 * etwas steht.
		 */
		buffer[min (BUFSIZE, size_read)] = '\0';
		eof = strchr (buffer, FILEEOF_CHAR);
		if (eof)
			*eof = '\0';
			
		for (i = 0; i < size_read; ++i)
		{
			if (!buffer[i])
				break;
			
			/* Wenn der zu substituierende Ausdruck gequoted war,
			 * dann findet keine Nachbehandlung des Inputs statt.
			 * Ansonsten wird Whitespace zu Leerzeichen ersetzt.
			 */
			if (S->quoted)
			{
				if (!storeInResult (M, S, buffer[i], 1))
					return FALSE;
			}
			else
			{
				if (strchr (" \t\r\n\f", buffer[i]) != NULL)
				{
					got_whitespace = TRUE;
				}
				else
				{
					if (stored && got_whitespace
						&& !storeInResult (M, S, ' ', 0))
						return FALSE;
						
					if (!storeInResult (M, S, buffer[i], 0))
						return FALSE;

					stored = TRUE;
					got_whitespace = FALSE;
				}
			}
		}

	}
	
	return TRUE;
}

/* FÅhrt eine Kommando-Substitution aus. Das Ergebnis wird mit
 * storeInResult () in den gerade substituierten String abgespeichert
 * Tritt ein Fehler auf, wird FALSE zurÅckgeliefert.
 */
static int substCommand (MGLOBAL *M, SUBINFO *S, int take_from_list)
{
	TREEINFO *tree = NULL;
	size_t arg_index;
	char c, c_quoted;

	if (take_from_list)
	{
		if (!S->act_list)
			return FALSE;
		tree = S->act_list->tree;
		if (!tree)
			return FALSE;
		S->act_list = S->act_list->next;
	}
	else
	{
		/* Da wir nicht alles doppelt programmieren wollen, benutzen
		 * wir zum Speichern des Strings zwischen `...` auch S->result.	
		 * Dieser muû aber durch das Ergebnis seiner AusfÅhrung ersetzt
		 * werden. Daher merken wir uns die aktelle Schreibposition.
		 */
		arg_index = S->result_index;
		
		/* Lese bis zu einem ungequoteten '`'
		 */
		getSChar (S, &c, &c_quoted);
		while (c_quoted || (c && (c != '`')))
		{
			if (!storeInResult (M, S, c, 1))
				return FALSE;
	
			getSChar (S, &c, &c_quoted);
		}
		if (!appendZeroToResult (M, S))
			return FALSE;
		
		/* Wenn wir etwas gelesen haben, fÅhren wir es aus.
		 */
		if (arg_index < S->result_index)
		{	
			PARSINFO P;
			PARSINPUTINFO PSave;
	
			ParsFromString (M, &PSave, &S->result_name[arg_index]);
		
			tree = Parse (M, &P, '\n');
			S->result_index = arg_index;
			appendZeroToResult (M, S);
			
			ParsRestoreInput (M, &PSave);
		}
	}
		
	/* Ist etwas bei herausgekommen?
	 */
	if (tree)
	{
		IOINFO io_out, io_in;
		
		if (OpenPipe (M, &io_out, &io_in))
		{	
			if (DoRedirection (M, &io_out))
			{
				FORKINFO fork;
				
				fork.typ = TFORK | FPOU;
				fork.tree = tree;
				fork.io = NULL;
				
				ExecTree (M, (TREEINFO *)&fork, TRUE, 
					&io_out, &io_in);
				if (!take_from_list)
					FreeTreeInfo (M, tree);
				
				UndoRedirection (M, &io_out);
			}
			
			if (DoRedirection (M, &io_in))
			{
				storeFromStdIn (M, S);
				UndoRedirection (M, &io_in);
			}

			ClosePipe (M, &io_out, &io_in);
		}
		else
			return FALSE;
	}
		
	return TRUE;
}


/* Werfe die Zeichen bis <end_char> weg. Beachte dabei Schachtelungen
 * von ` und " und {}. Meldet einen
 * Fehler, wenn nicht bis <end_char> gelesen werden konnte.
 */
static int skipSubstTill (MGLOBAL *M, SUBINFO *S, char end_char)
{
	char c, c_quoted;
	
	getSChar (S, &c, &c_quoted);
	while (c_quoted || (c && (c != end_char)))
	{
		if (!c_quoted)
		{
			switch (c)
			{
				case '`':
					if (!skipSubstTill (M, S, '`'))
						return FALSE;
					break;
					
				case '\"':
					if (!skipSubstTill (M, S, '\"'))
						return FALSE;
					break;

				case '{':
					if (!skipSubstTill (M, S, '}'))
						return FALSE;
					break;
			}
		}
		getSChar (S, &c, &c_quoted);
	}
	
	if (!c_quoted && (c != end_char))
	{
		eprintf (M, "falsche Parametersubstitution\n");
		return FALSE;
	}
	return TRUE;
}


/* FÅhre die Substitution in S bis <end_char> durch. Melde einen
 * Fehler, wenn nicht bis <end_char> gelesen werden konnte.
 */
static int copySubstTill (MGLOBAL *M, SUBINFO *S, char end_char)
{
	char c, c_quoted;
	
	if (!makeTildeExpansion (M, S, end_char, TRUE))
		return FALSE;
		
	if (!getSubChar (M, S, end_char, &c, &c_quoted))
		return FALSE;

	while (c_quoted || (c && (c != end_char)))
	{
		/* Dieses Zeichen wurde substituiert, also quote es,
		 * wenn nîtig.
		 */
		if (!storeInResult (M, S, c, c_quoted || S->flags.quote_it))
			return FALSE;

		if (!getSubChar (M, S, end_char, &c, &c_quoted))
			return FALSE;
	}
	
	if (c != end_char || c_quoted)
		eprintf (M, "falsche Parametersubstitution\n");
	
	return appendZeroToResult (M, S);
}


static int doSubstitution (MGLOBAL *M, WORDINFO *name, 
	int command_subst, int trim_string, char **subst_string, 
	char **subst_quote, int *slashesConverted)
{
	SUBINFO S;
	
	S.name = name;
	S.act_list = name->list;
	S.name_index = S.result_size = S.result_index = 0L;
	S.result_name = S.result_quoted = NULL;
	S.flags.command_subst = command_subst;
	S.quoted = FALSE;
	S.was_at = 0;
	S.flags.quote_it = 0;
	*slashesConverted = 0;
	
	if (!copySubstTill (M, &S, '\0'))
	{
		mfree (&M->alloc, S.result_name);
		mfree (&M->alloc, S.result_quoted);

		return FALSE;
	}
	
	/* Zur Sicherheit hÑngen wir noch eine ungequotete 0 an, damit
	 * beim Trimmen nicht versehentlich alte Inhalte erhalten
	 * bleiben.
	 */
	if (!appendZeroToResult (M, &S))
	{
		mfree (&M->alloc, S.result_name);
		mfree (&M->alloc, S.result_quoted);
		return FALSE;
	}

	/* Wenn wir den String nicht trimmen sollen (es kînnen
	 * also gequotete Nullen enthalten sein), dann mÅssen
	 * wir bei leerem Ergebnis die 0 am Ende quoten, wenn
	 * S.quoted TRUE ist. Das ist bei "" und '' der Fall,
	 * aber nicht bei "$@".
	 */
	if (S.quoted && !S.was_at && !S.result_index)
	{
		if (!storeInResult (M, &S, '\0', 1))
		{	
			mfree (&M->alloc, S.result_name);
			mfree (&M->alloc, S.result_quoted);
			return FALSE;
		}
	}
	else if (trim_string && (S.result_index > 1))
	{
		removeQuotedZeros (0, S.result_index, 
			S.result_name, S.result_quoted);
	}
	
	if (!appendZeroToResult (M, &S))
	{
		mfree (&M->alloc, S.result_name);
		mfree (&M->alloc, S.result_quoted);
		return FALSE;
	}

	if (M->shell_flags.slash_conv)
	{
		*slashesConverted = ConvertSlashes (S.result_name, 
			S.result_quoted);
	}
	
	*subst_string = S.result_name;
	*subst_quote = S.result_quoted;
	
	return TRUE;
}

/* Substituiere <name> ohne Command-Substitution. Speichere das
   Ergebnis in <subst_string>. */

int
SubstNoCommand (MGLOBAL *M, WORDINFO *name, char **subst_string,
	char **subst_quote, int *slashesConverted)
{
	int conv;

	if (!slashesConverted) slashesConverted = &conv;
		
	return doSubstitution (M, name, FALSE, TRUE, 
			subst_string, subst_quote, slashesConverted);
}

/* Substituiere <name> mit Command-Substitution. Speichere das
 * Ergebnis in <subst_string>.
 */
int SubstWithCommand (MGLOBAL *M, WORDINFO *name, 
		char **subst_string, char **subst_quote)
{
	int conv;
	return doSubstitution (M, name, TRUE, TRUE, 
			subst_string, subst_quote, &conv);
}

/* jr: Mache eine Var-Substitution und liefere einen Zeiger auf
   einen allozierten String oder NULL zurÅck */
   
const char *StringVarSubst (MGLOBAL *M, const char *string)
{
	char *result = NULL;
	char *quotation = NULL;
	WORDINFO w;

	memset (&w, 0, sizeof (w));
	w.name = StrDup (&M->alloc, string);
	w.quoted = mcalloc (&M->alloc, 1, strlen (string) + 1);

	if (!w.name || !w.quoted 
		|| !SubstNoCommand (M, &w, &result,	&quotation, 0))
	{
		mfree (&M->alloc, w.name);
		mfree (&M->alloc, w.quoted);
		return NULL;
	}

	mfree (&M->alloc, w.name);
	mfree (&M->alloc, w.quoted);
	mfree (&M->alloc, quotation);
	
	return result;
}

   
   
