/*
 * @(#) MParse\MParse.c
 * @(#) Stefan Eissing, 21. November 1992
 *
 * Mupfel-Parser mit Bourne-Shell-Syntax
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "mglob.h"

#include "mlex.h"
#include "partypes.h"
#include "duptree.h"
#include "maketree.h"
#include "mparse.h"
#include "parsinpt.h"
#include "chario.h"


/* internal texts
 */
#define NlsLocalSection "M.mparse"
enum NlsLocalText{
MP_NOMEM,	/*Nicht genug freier Speicher\n*/
MP_EOF,		/*Unerwartetes Ende der Datei*/
MP_LOOKING_FOR, /*, ich suchte nach `%c'\n*/
MP_SYNTAX,		/*Syntax Fehler: */
MP_SYNTAXLINE,	/*Syntax Fehler in Zeile %ld: */
MP_NORESERVED,	/*das geht hier so nicht\n*/
MP_EXPBRACE,		/*ich hatte eigentlich `)' oder `|' erwartet\n*/
MP_EXPCOMMAND,	/*ich hatte ein (reserviertes) Kommando erwartet\n*/
MP_RETURN,		/*ich hatte `;' oder eine neue Zeile erwartet\n*/
MP_FUNCBRACE,	/*hier muû ein `)' hin\n*/
MP_COMMANDMISSING,	/*hier fehlt ein Kommando\n*/
MP_BADSUBST,		/*Fehler in der Kommandosubstitution\n*/
};

#define DEBUG 0

#define MAY_BE_NEWLINE		0x01
#define NO_COMMAND			0x02

/* Einige Forward-Deklarationen
 * In <flags> kînnen MAY_BE_NEWLINE und NO_COMMAND
 * gesetzt sein.
 */
static TREEINFO *command (MGLOBAL *M, PARSINFO *P, int symbol, 
	int flag);


/* Meldet einen Syntax-Fehler. Die Meldung muû noch genauer
 * werden. Weiterhin wird man hier nur mit einem long_jmp
 * wieder raus dÅrfen
 */
static void reportError (MGLOBAL *M, PARSINFO *P, 
	const char *error, ...)
{
	char tmp[1024];
	va_list argpoint;
	const char *errstring = NULL;
	
	if (P->L.broken)
	{
		switch (P->L.break_reason)
		{
			case BROKEN_USER:
				return;

			case BROKEN_EOF:
				if (M->pars_input_info.input_type != fromStdin)
				{
					eprintf (M, NlsStr(MP_EOF));
					if (P->L.looking_for)
						eprintf (M, NlsStr(MP_LOOKING_FOR), P->L.looking_for);
					else
						eprintf (M, "\n");
						
					break;
				}
				else
					return;
							
			case BROKEN_MALLOC:
				errstring = MP_NOMEM;
				break;
			
			case BROKEN_SUBST:
				errstring = NlsStr(MP_BADSUBST);
				break;
		}
	}
	else
	{
		/* Wir haben einen Syntax-Fehler vorliegen.
		 */
		P->L.broken = TRUE;
		
		if (M->shell_flags.interactive)
			eprintf (M, NlsStr(MP_SYNTAX));
		else
		{
			eprintf (M, NlsStr(MP_SYNTAXLINE), ParsLineCount (M));
		}
		
		if (error)
		{
			va_start (argpoint, error);	
			vsprintf (tmp, error, argpoint);
			va_end (argpoint);
			errstring = tmp;
		}
		
		M->exit_value = 2;
	}

	if (errstring != NULL)
		eprintf (M, errstring);

	/* Wenn der Returnwert noch OK (== 0) ist, dann setzen
	 * wir ihn auf 1.
	 */
	if (!M->exit_value)
		M->exit_value = 1;
}

/* Ein Lesen von Tokens hat nicht geklappt. Setze broken auf TRUE und
 * Åbernehme den Fehlergrund, den der Lexer gemeldet hat.
 */
static void *lexFailed (MGLOBAL *M, PARSINFO *P, TREEINFO *tree)
{
	P->L.broken = TRUE;
	FreeTreeInfo (M, tree);
	return NULL;
}

/* Funktion, die BROKEN_MALLOC setzt, den Åbergebenen TREEINFO wieder
 * freigibt und immer NULL zurÅckliefert.
 */
static void *mallocFailed (MGLOBAL *M, PARSINFO *P, TREEINFO *tree)
{
	if (!P->L.broken)
	{
		P->L.broken = TRUE;
		P->L.break_reason = BROKEN_MALLOC;
	}
	
	return FreeTreeInfo (M, tree);
}

/* Das nÑchste Token darf nur ein normales Wort sein.
 * Ansonsten wird ein Syntax-Fehler gemeldet
 */
static int checkWord (MGLOBAL *M, PARSINFO *P)
{
	if (!ReadToken (M, &P->L))
	{
		lexFailed (M, P, NULL);
		return FALSE;
	}
	
	if (P->L.token_value != 0)
	{
		reportError (M, P, NlsStr(MP_NORESERVED));
		return FALSE;
	}
	
	return TRUE;
}

/* öberprÅft, ob das nÑchste Token gleich <symbol> ist.
 * Wenn nicht, gibt es einen Syntax-Fehler.
 */
static int checkSymbol (MGLOBAL *M, PARSINFO *P, int symbol)
{
	int x = symbol & P->L.token_value;

	if (((x & SYMFLG) ? x : symbol) != P->L.token_value)
	{
#if DEBUG
dprintf ("checkSymbol: erwaretete %d bekam %d\n", symbol, 
	P->L.token_value);
#endif
		reportError (M, P, NlsStr(MP_EXPCOMMAND));
		return FALSE;
	}
	
	return TRUE;
}

/* Liest das nÑchste Token und Åberspringt Newlines.
 * Liest also solange, bis ein Token != '\n' auftritt.
 */
static int skipNl (MGLOBAL *M, PARSINFO *P)
{
	while (P->L.may_be_reserved++, ReadToken(M, &P->L))
	{
		if (P->L.token_value != '\n')
			break;
	}

	if (P->L.broken)
	{
		return FALSE;
	}
	else
		return P->L.token_value != '\n';
}


/* Behandelt IO-Redirection. Wenn eine solche vorliegt, wird
 * eine neue IOINFO-Struktur alloziert und in die Liste von
 * solche eingehÑngt.
 * Bei Fehlern wird NULL zurÅckgeliefert.
 */
static IOINFO *inout (MGLOBAL *M, PARSINFO *P, IOINFO *lastio)
{
	int	iof;
	IOINFO *iop;
	short c;

	iof = (int)P->L.token_number;
	switch (P->L.token_value)
	{
		case HERE_DOC_MINUS:	/*  <<- */
			iof |= IO_STRIP;
		case HERE_DOC:			/*	<<	*/
			iof |= IO_DOC;
			break;

		case GREATER_GREATER:	/*	>>	*/
		case '>':
			/* Wenn kein Ausgabekanal angegeben war, dann kann es
			 * nur stdout (1) sein. Wurde allerdings stdin (0)
			 * angegeben, merken wir den Fehler nicht
			 */
			if (P->L.token_number == 0)
				iof |= 1;
				
			iof |= IO_PUT;
			if (P->L.token_value == GREATER_GREATER)
			{
				iof |= IO_APP;
				break;
			}
			/* Fall through
			 */
			 
		case '<':
			if (!NextChar (M, &P->L, 0, &c))
				return lexFailed (M, P, NULL);
				
			if (c == '&')
				iof |= IO_MOV;
			else if (c == '>')
				iof |= IO_RDW;
			else if (c == '|')
				iof |= IO_CLOBBER;
			else
				P->L.peek_char = c;
			break;

		default:
			/* Ist keine IO-Redirection, gib die unverÑnderte
			 * Liste wieder zurÅck.
			 */
			return lastio;
	}

	if (!checkWord (M, P))
		return NULL;
		
	iop = mcalloc (&M->alloc, 1, sizeof (IOINFO));
	if (!iop)
		return mallocFailed (M, P, NULL);

	iop->name = DupWordInfo (M, &P->L.token);
	if (!iop->name)
		return (FreeIoInfo (M, iop), mallocFailed (M, P, NULL));
	
	iop->filename = NULL;
	
	iop->file = iof;
	if (iof & IO_DOC)
	{
		iop->here_next = P->pending_io;
		P->pending_io = iop;
	}
	
	if (!ReadToken (M, &P->L))
	{
		if (P->pending_io == iop)
			P->pending_io = iop->here_next;
			
		return (FreeIoInfo (M, iop), lexFailed (M, P, NULL));
	}
	iop->next = inout (M, P, lastio);
	if (P->L.broken)
	{
		if (P->pending_io == iop)
			P->pending_io = iop->here_next;
			
		return FreeIoInfo (M, iop);
	}
	return iop;
}


/* Parse den Body eines Case-Konstruktes. <end_symbol> ist das
 * Symbol, das diesen Body abschlieût. Gib diese Liste von einzelnen
 * FÑllen zurÅck.
 */
static REGINFO *synCase (MGLOBAL *M, PARSINFO *P, int end_symbol)
{
	/* Das nÑchste Token kann auf einer neuen Zeile stehen.
	 */
	if (!skipNl (M, P))
		return NULL;
	
#if DEBUG
dprintf ("synCase: start\n");
#endif
	if (P->L.token_value == end_symbol)
	{
		/* Ein leerer Body ist erlaubt. (Warum auch immer...)
		 */
#if DEBUG
dprintf ("synCase: leerer Body\n");
#endif
		return NULL;
	}
	else
	{
		REGINFO *regtree;
		WORDINFO *argp;

		regtree = mcalloc (&M->alloc, 1, sizeof (REGINFO));
		if (!regtree)
			return mallocFailed (M, P, NULL);
		
		regtree->matches = NULL;
	
		for (;;)
		{
			argp = DupWordInfo (M, &P->L.token);
			if (!argp)
			{
				return (FreeRegInfo (M, regtree), 
					mallocFailed (M, P, NULL));
			}
			
			/* Trage die WORDINFO in die Liste von Matches ein
			 */
			argp->next = regtree->matches;
			regtree->matches = argp;
			
			/* Wenn wir ein reserviertes Wort hatten oder ein
			 * spezielles Token, dann ist das hier falsch!
			 */
			if (P->L.token_value)
			{
				reportError (M, P, NlsStr(MP_NORESERVED));
				return FreeRegInfo (M, regtree);
			}
			else
			{
				/* Wenn das nÑchste Token kein ')' oder '|' ist,
				 * dann ist das ein Syntax-Fehler.
				 */
				if (!ReadToken (M, &P->L))
				{
					return (FreeRegInfo (M, regtree), 
						lexFailed (M, P, NULL));
				}

				if (P->L.token_value != ')'
					&& P->L.token_value != '|')
				{
					reportError (M, P, NlsStr(MP_EXPBRACE));
					return FreeRegInfo (M, regtree);
				}
			}
			
			
			if (P->L.token_value == '|')
			{
				/* Parse das nÑchste Pattern
				 */
				if (!ReadToken (M, &P->L))
				{
					return (FreeRegInfo (M, regtree), 
						lexFailed (M, P, NULL));
				}
			}
			else
			{
				/* Wir hatten ein ')' und haben alle Patterns.
				 * Verlasse die Schleife.
				 */
				break;
			}
		}
		
		/* Wir haben eine Liste von Patterns gelesen. Parse nun
		 * das Kommando dazu. 
		 */
		regtree->commands = command (M, P, 0, 
			MAY_BE_NEWLINE|NO_COMMAND);
		if (P->L.broken)
			return FreeRegInfo (M, regtree);

		if (P->L.token_value == SEMI_SEMI)
		{
			/* Letztes Token war ';;', parse den nÑchsten Fall
			 */
			regtree->next = synCase (M, P, end_symbol);
			if (P->L.broken)
				return FreeRegInfo (M, regtree);
		}
		else
		{
			/* In diesem Fall muû das Ende des Case aufgetaucht
			 * sein.
			 */
			if (!checkSymbol (M, P, end_symbol))
				return FreeRegInfo (M, regtree);
			regtree->next = NULL;
		}
		
		return regtree;
	}
}


/*
 * item
 *
 *	( cmd ) [ < in  ] [ > out ]
 *	word word* [ < in ] [ > out ]
 *	if ... then ... else ... fi
 *	for ... while ... do ... done
 *	case ... in ... esac
 *	begin ... end
 */
static TREEINFO *item (MGLOBAL *M, PARSINFO *P, int may_have_io,
	int look_for_io)
{
	TREEINFO *tree;
	IOINFO *io, *tmpio;

	if (may_have_io)
	{
		io = inout (M, P, NULL);
		if (P->L.broken)
			return NULL;
	}
	else
		io = NULL;
	
#if DEBUG
dprintf ("item: switch Token %d\n", P->L.token_value);
#endif
	/* Was haben wir denn nun gelesen? PrÅfe zuerst auf reservierte
	 * Wîrter.
	 */	
	switch (P->L.token_value)
	{
		/* Ein case ... in ... esac wird eingeleitet.
		 */
		case CASE:
		{
			CASEINFO *ctree;
			
#if DEBUG
dprintf ("item: parse case\n");
#endif
			/* Allozire ein CASEINFO und setze tree auf diese.
			 */
			ctree = mcalloc (&M->alloc, 1, sizeof (CASEINFO));
			if (!ctree)
			{
				return (FreeIoInfo (M, io), 
					mallocFailed (M, P, NULL));
			}
			tree = (TREEINFO *)ctree;
			ctree->typ = TCASE;

#if DEBUG
dprintf ("item: tree alloziert\n");
#endif
			/* Lese den Case-Parameter. Hier sind keine reservierten
			 * Wîrter erlaubt.
			 */
			if (!checkWord (M, P))
				return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
			
#if DEBUG
dprintf ("item: parameter gelesen\n");
#endif
			/* Trage das gelesene Token, das laut checkWord ein
			 * normales Wort ist, als Parameter von Case ein.
			 */
			ctree->name = DupWordInfo (M, &P->L.token);
			if (!ctree->name)
				return (FreeIoInfo (M, io), lexFailed (M, P, tree));
#if DEBUG
dprintf ("item: token dupliziert\n");
#endif

			/* Das IN kann auch auf einer anderen Zeile stehen.
			 * Skippe also Newlines.
			 */
			if (!skipNl (M, P))
				return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
#if DEBUG
dprintf ("item: nach skipNl for IN\n");
#endif
			
			/* Schau nach, ob wir IN oder '{' vorliegen haben.
			 */
			if (!checkSymbol (M, P, IN | CURLY_OPEN))
				return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
			
#if DEBUG
dprintf ("item: in/{ gelesen\n");
#endif
			/* Ja, das ist der Fall. Wir kînnen also anfangen den
			 * Rest der Case-Struktur zu lesen. Das erlaubte Symbol,
			 * das den Case beschlieût wird Åbergeben.
			 */
			ctree->case_list = synCase (M, P, P->L.token_value == IN ? 
				ESAC : CURLY_CLOSE);
			if (P->L.broken)
				return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
#if DEBUG
dprintf ("item: synCase zurÅck mit %p\n", ctree->case_list);
#endif
			
			break;
		}
	
		/* Ein if ... then ... else|elif ... fi wird eingeleitet.
		 */
		case IF:
		{
			int w;
			IFINFO *iftree;

#if DEBUG
dprintf ("item: parse if\n");
#endif
			iftree = mcalloc (&M->alloc, 1, sizeof (IFINFO));
			if (!iftree)
				return (FreeIoInfo (M, io), mallocFailed (M, P, NULL));
			tree = (TREEINFO *)iftree;
			iftree->typ = TIF;

			iftree->condition = 
				command (M, P, THEN, MAY_BE_NEWLINE);
			if (P->L.broken)
				return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));

			iftree->then_case = 
				command (M, P, ELSE|FI|ELIF, MAY_BE_NEWLINE);
			if (P->L.broken)
				return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));

			if ((w = P->L.token_value) == ELSE)
			{
				iftree->else_case = command (M, P, FI, MAY_BE_NEWLINE);
				if (P->L.broken)
					return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
			}
			else
			{
				if (w == ELIF)
				{
					P->L.token_value = IF;
					iftree->else_case = item (M, P, FALSE, FALSE);
					if (P->L.broken)
					{
						return (FreeIoInfo (M, io), 
							FreeTreeInfo (M, tree));
					}
				}
				else
					iftree->else_case = NULL;
			}
			
			break;
		}

		/* Ein for ... in do ... done wird eingeleitet.
		 */
		case FOR:
		{
			FORINFO *fortree;

#if DEBUG
dprintf ("item: parse for\n");
#endif
			fortree = mcalloc (&M->alloc, 1, sizeof (FORINFO));
			if (!fortree)
				return (FreeIoInfo (M, io), mallocFailed (M, P, NULL));
			tree = (TREEINFO *)fortree;

			fortree->typ = TFOR;
			fortree->list = NULL;

			if (!checkWord (M, P))
				return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
			
			fortree->name = DupWordInfo (M, &P->L.token);	
			if (!fortree->name)
			{
				return (FreeIoInfo (M, io), 
					mallocFailed (M, P, tree));
			}

			if (!skipNl (M, P))
				return (FreeIoInfo (M, io), lexFailed (M, P, tree));

			if (P->L.token_value == IN)
			{
				checkWord (M, P);

				P->no_hash++;
				fortree->list = (COMMANDINFO *)item (M, P, 0, TRUE);
				P->no_hash--;

				if (P->L.broken)
					return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
					
				if (P->L.token_value != '\n' 
					&& P->L.token_value != ';')
				{
					reportError (M, P, NlsStr(MP_RETURN));
					return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
				}
				
				if (!skipNl (M, P))
					return (FreeIoInfo (M, io), lexFailed (M, P, tree));
			}
			
			if (!checkSymbol (M, P, DO | CURLY_OPEN))
				return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
			
			fortree->commands = command (M, P, 
				(P->L.token_value == DO)? 
				DONE : CURLY_CLOSE, MAY_BE_NEWLINE);
				
			if (P->L.broken)
				return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
				
			break;
		}
	
		/* Ein while|until ... do ... done wird eingeleitet.
		 */
		case WHILE:
		case UNTIL:
		{
			WHILEINFO *wtree;

#if DEBUG
dprintf ("item: parse while/until\n");
#endif
			wtree = mcalloc (&M->alloc, 1, sizeof (WHILEINFO));
			if (!wtree)
				return (FreeIoInfo (M, io), mallocFailed (M, P, NULL));
			tree = (TREEINFO *)wtree;

			wtree->typ = (P->L.token_value == WHILE ? TWHILE : TUNTIL);

			wtree->condition = command (M, P, DO, MAY_BE_NEWLINE);
			if (P->L.broken)
				return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));

			wtree->commands = command (M, P, DONE, MAY_BE_NEWLINE);
			if (P->L.broken)
				return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
			break;
		}
	
		/* Ein { ...; } wird eingeleitet.
		 */
		case CURLY_OPEN:
#if DEBUG
dprintf ("item: parse {\n");
#endif
			tree = command (M, P, CURLY_CLOSE, MAY_BE_NEWLINE);
			if (P->L.broken)
				return FreeIoInfo (M, io);
			break;
	
		/* Ein ( ... ) wird eingeleitet.
		 */
		case '(':
		{
			PARINFO *ptree;

#if DEBUG
dprintf ("item: parse (\n");
#endif
			ptree = mcalloc (&M->alloc, 1, sizeof (PARINFO));
			if (!ptree)
				return (FreeIoInfo (M, io), mallocFailed (M, P, NULL));

			ptree->typ = TPAR;
			ptree->tree = command (M, P, ')', MAY_BE_NEWLINE);
			if (P->L.broken)
			{
				return (FreeIoInfo (M, io), 
					FreeTreeInfo (M, (TREEINFO *)ptree));
			}
				
			if ((tree = MakeForkInfo (M, 0, (TREEINFO *)ptree)) == NULL)
			{
				return (FreeIoInfo (M, io), mallocFailed 
					(M, P, (TREEINFO *)ptree));
			}
			break;
		}
	
		/* Dies ist der Fall, wenn weder ein reserviertes Wort, noch
		 * ein Kommando gelesen wurde.
		 * Wenn auch keine IO-Redirection vorliegt, wird NULL
		 * zurÅckgeliefert.
		 */
		default:
#if DEBUG
dprintf ("item: kein bekanntes Token: %d.\n", P->L.token_value);
#endif
			if (io == NULL)
				return NULL;
	
		/* Ein Wort wurde gelesen. Parse die Argumente, sowie
		 * dazugehîrige IO-Redirections. Es kann sich um eine
		 * Funktionsdefinition handeln.
		 */
		case 0:
		{
			COMMANDINFO *ctree;
			WORDINFO *argp;
			WORDINFO **argtail;
			int	key_word = 1;
			int to_read;
			short tmpc;

#if DEBUG
dprintf ("item: parse simple_command\n");
#endif
			/* Parsen wir eine Funktionsdefinition?
			 */
			to_read = (P->L.token_value != '\n');
			if (to_read && !SkipWhitespace (M, &P->L, &tmpc))
				return (FreeIoInfo (M, io), lexFailed (M, P, NULL));
				
			P->L.peek_next = tmpc;
			
			if (to_read && (P->L.peek_next == '('))
			{
				FUNCINFO *func;

				/* Schmeiû den geparsten '(' weg.
				 */
				P->L.peek_next = 0;
				
				/* Wenn wir jetzt kein ')' lesen, hat einer was
				 * falsch gemacht.
				 */
				if (!SkipWhitespace (M, &P->L, &tmpc))
					return (FreeIoInfo (M, io), lexFailed (M, P, NULL));
						
				if (tmpc != ')')
				{
					reportError (M, P, NlsStr(MP_FUNCBRACE));
					return FreeIoInfo (M, io);
				}

				func = mcalloc (&M->alloc, 1, sizeof (FUNCINFO));
				if (!func)
				{
					return (FreeIoInfo (M, io), 
						mallocFailed (M, P, NULL));
				}

				tree = (TREEINFO *)func;

				func->typ = TFUNC;
				func->name = DupWordInfo (M, &P->L.token);
				if (!func->name)
				{
					return (FreeIoInfo (M, io), 
						mallocFailed (M, P, tree));
				}
				
				/* reservierte Wîrter erlaubt
				 */
				P->L.may_be_reserved++;
				if (!skipNl (M, P))
					return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
				
				/* Lese die Kommandos, die zu dieser Funktion
				 * gehîren.
				 */
				P->is_function_def++;
				func->commands = item (M, P, 0, TRUE);
				P->is_function_def--;

				if (P->L.broken)
					return (FreeIoInfo (M, io), FreeTreeInfo (M, tree));
					
				/* Liefere die Funktionsdefinition zurÅck.
				 */
				return tree;
			}
			else
			{
				/* Hier handelt es sich um ein ganz normales
				 * Kommando. Also holen wir uns ein COMMANDINFO.
				 */
				ctree = mcalloc (&M->alloc, 1, sizeof (COMMANDINFO));
				if (!ctree)
				{
					return (FreeIoInfo (M, io), 
						mallocFailed (M, P, NULL));
				}
				
				ctree->typ = TCOM;
				tree = (TREEINFO *)ctree;

				ctree->io = io;				/* initial io chain */
				argtail = &(ctree->args);	/* Ende der Argumente */

				while (P->L.token_value == 0)
				{
					argp = DupWordInfo (M, &P->L.token);
					if (!argp)
						return mallocFailed (M, P, tree);

					/* Wenn wir ein assignment haben und dies an
					 * dieser Stelle behandelt wird, hÑnge diese
					 * in die Liste von Sets.
					 */
					if (argp->flags.assignment && key_word)
					{
						argp->next = ctree->set;
						ctree->set = argp;
					}
					else
					{
						/* argp wird nicht als assignment behandelt.
						 * also muû ans Ende der Liste von Argumenten
						 * gehÑngt werden. argtail zeigt immer auf
						 * das Ende dieser Liste.
						 */
						*argtail = argp;
						argtail = &(argp->next);
						
						/* Falls assignments Åberall erkannt werden,
						 * setzte key_word wieder auf TRUE.
						 */
						key_word = M->shell_flags.keywords_everywhere;
					}

					/* Wenn wir immer noch kein Kommando gelesen
					 * haben, kann im nÑchsten Wort ein Alias
					 * auftauchen, obwohl wir nicht mehr nach
					 * reservierten Worten gucken.
					 */
					P->L.may_be_alias = (ctree->args == NULL);
					
					if (!ReadToken (M, &P->L))
						return mallocFailed (M, P, tree);

					if (may_have_io)
					{
						tmpio = inout (M, P, ctree->io);
						if (P->L.broken)
							return FreeTreeInfo (M, tree);
							
						ctree->io = tmpio;
					}
				}

				P->L.may_be_alias = FALSE;
				*argtail = NULL;

				return tree;
			}
		}
	
	}
	
if (look_for_io)
{
	/* Beim Parsen des nÑchsten Tokens sollen auch reservierte
	 * Wîrter erkannt werden.
	 */
	P->L.may_be_reserved++;
		
	/* Die Behandlung des items ist abgeschlossen. Lese das
	 * nÑchste Token und prÅfe nach, ob es sich um eine
	 * IO-Redirection handelt. Wenn ja, fÅge sie an die Liste
	 * der Redirections fÅr diesen Tree an.
	 */
	if (!ReadToken (M, &P->L))
		return (FreeIoInfo (M, io), lexFailed (M, P, tree));
			
	tmpio = inout (M, P, io);
	if (P->L.broken)
		return FreeTreeInfo (M, tree), FreeIoInfo (M, io);
	io = tmpio;
		
	if (io)
	{
		TREEINFO *tmp;
			
		tmp = MakeForkInfo (M, 0, tree);
		if (!tmp)
			return (FreeIoInfo (M, io), mallocFailed (M, P, tree));
	
		tree = tmp;
		tree->io = io;
	}
}
#if DEBUG
dprintf ("item: returniere %p\n", tree);
#endif
	return tree;
}


/*
 * term
 *	item
 *	item |^ term
 */
static TREEINFO *term (MGLOBAL *M, PARSINFO *P, int flag)
{
	TREEINFO *tree;
	
	P->L.may_be_reserved++;
	if (flag & MAY_BE_NEWLINE)
	{
		if (!skipNl (M, P))
		{
			return NULL;
		}
	}
	else
	{
		if (!ReadToken (M, &P->L))
		{
			return lexFailed (M, P, NULL);
		}
	}
	
	tree = item (M, P, TRUE, TRUE);
#if DEBUG
dprintf ("term: item is %p\n", tree);
#endif
	if (P->L.broken)
		return NULL;
		
	if (tree
		&& (P->L.token_value == '^' || P->L.token_value == '|'))
	{
		TREEINFO *left, *right, *tmp;
		
		left = MakeForkInfo (M, FPOU, tree);
		if (!left)
			return mallocFailed (M, P, tree);
		
		tmp = term (M, P, MAY_BE_NEWLINE);
		if (P->L.broken)
			return FreeTreeInfo (M, left);
			
		right = MakeForkInfo (M, FPIN | FPCL, tmp);
		if (!right)
		{
			return (mallocFailed (M, P, left), 
				mallocFailed (M, P, tmp));
		}

		tmp = MakeListInfo (M, TFIL, left, right);
		if (!tmp)
		{
			return (mallocFailed (M, P, left), 
				mallocFailed (M, P, right));
		}

		tree = MakeForkInfo (M, 0, tmp);
		if (!tree)
			return mallocFailed (M, P, tmp);
	}
	
	return tree;
}

/*
 * list
 *	term
 *	list && term
 *	list || term
 */
static TREEINFO *list (MGLOBAL *M, PARSINFO *P, int flag)
{
	TREEINFO *tree, *tmp;
	int connection, is_and;
	
	tree = term (M, P, flag);
#if DEBUG
dprintf ("list: term is %p\n", tree);
#endif
	if (P->L.broken)
		return FreeTreeInfo (M, tree);
	
	connection = (((is_and = (P->L.token_value == AND_AND)) != 0)
		|| (P->L.token_value == OR_OR));
		
	while (tree && connection)
	{
		TREEINFO *next;
		
		next = term(M, P, MAY_BE_NEWLINE);
		if (P->L.broken)
			return FreeTreeInfo (M, tree);
		
		if (!tree && !next)
		{
			reportError (M, P, NlsStr(MP_COMMANDMISSING));
			return NULL;
		}
		tmp = MakeListInfo(M, (is_and ? TAND : TORF), tree, next);
		if (!tmp)
		{
			return (mallocFailed (M, P, tree), 
				mallocFailed (M, P, next));
		}
		tree = tmp;

		connection = (((is_and = (P->L.token_value == AND_AND)) != 0)
			|| (P->L.token_value == OR_OR));
	}
		
	return tree;
}

/*
 * cmd
 *	empty
 *	list
 *	list & [ cmd ]
 *	list [ ; cmd ]
 */
static TREEINFO *command (MGLOBAL *M, PARSINFO *P, int symbol, 
	int flag)
{
	TREEINFO *i, *e, *tmp;
	
	i = list (M, P, flag);

#if DEBUG
dprintf ("command: list is %p\n", i);
#endif
	if (P->L.broken)
		return FreeTreeInfo (M, i);
	
	if (P->L.token_value == '\n')
	{
		if (flag & MAY_BE_NEWLINE)
			P->L.token_value = ';';
	}
	else if ((i == NULL) && ((flag & NO_COMMAND) == 0))
	{
		reportError (M, P, NlsStr(MP_COMMANDMISSING));
		return NULL;
	}

	switch (P->L.token_value)
	{
		case '&':
			if (i)
			{
				tmp = MakeForkInfo (M, FINT|FPRS|FAMP, i);
				if (!tmp)
					return mallocFailed (M, P, i);
				i = tmp;
			}
			else
				reportError (M, P, NlsStr(MP_COMMANDMISSING));
	
		case ';':
			e = command(M, P, symbol, flag | NO_COMMAND);
			if (P->L.broken)
				return FreeTreeInfo (M, i);
			
			if (e)
			{
				tmp = MakeListInfo (M, TLIST, i, e);
				if (!tmp)
				{
					return (mallocFailed (M, P, i), 
						mallocFailed (M, P, e));
				}
				i = tmp;
			}
			else if (i == 0)
			{
				reportError (M, P, NlsStr(MP_COMMANDMISSING));
				return NULL;
			}
			break;
	
		case EOF_TOKEN:
			if (symbol == '\n')
				break;
	
		case ')':
			if (symbol != ')')
			{
				/* Wir haben ein ')' gelesen, daû nicht hierhin
				 * gehîrt, also nicht durch ein '(' eingeleitet
				 * wurde.
				 */
				eprintf (M, "')' ohne passendes '('\n");
				return FreeTreeInfo (M, i);
			}
			break;
			
		default:
			if (symbol)
			{
				if (!checkSymbol (M, P, symbol))
					return FreeTreeInfo (M, i);
			}
	}

	return i;
}

TREEINFO *Parse (MGLOBAL *M, PARSINFO *P, int legal_parse_end)
{
	TREEINFO *tree;
	
	if (legal_parse_end == '\n')
		ResetParsInput (M);
		
	LexInit (M, &P->L, &P->pending_io);

	/* Initialisiere die Abbruchbedingungen
	 */
	P->L.broken = FALSE;
	P->L.break_reason = BROKEN_USER;
	
	P->pending_io = NULL;

	/* Dies sollte im globalen MGLOBAL stehen
	 */
	P->is_function_def = 0;
	P->no_hash = 0;
	
	tree = command (M, P, legal_parse_end, 
		(legal_parse_end == '\n')? NO_COMMAND : MAY_BE_NEWLINE);
	if (P->L.broken)
	{
		reportError (M, P, NlsStr(MP_COMMANDMISSING));
		FreeIoInfo (M, P->pending_io);
		P->pending_io = NULL;
		tree = NULL;
	}
	
	if (P->pending_io)
		dprintf ("pending_io ist %p!\n", P->pending_io);
	
	LexExit (M, &P->L);
	return tree;
}