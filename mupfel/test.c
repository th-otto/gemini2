/*
 *	@(#)Mupfel/test.c
 *	@(#) Stefan Eissing, J. Reschke, 31. Juni 1992
 *
 * jr 18.5.1996
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"

#include "mglob.h"

#include "chario.h"
#include "getopt.h"
#include "mdef.h"
#include "redirect.h"
#include "test.h"

/* internal texts
 */
#define NlsLocalSection "M.test"
enum NlsLocalText{
OPT_test,	/*ausdruck*/
OPT_testk,	/*ausdruck ]*/
HLP_test,	/*Bedingung testen*/
TE_ARGEXP,	/*Argument fehlt\n*/
TE_ARGEXPF,	/*Argument erwartet, %s gefunden\n*/
TE_INTEXP,	/*%s `%s' wird ein ganzzahliger Ausdruck erwartet\n*/
TE_BEFORE,	/*vor*/
TE_AFTER,	/*nach*/
TE_MISSCHAR,/*Zeichen `%s' fehlt\n*/
TE_TOOMANY,	/*%s: zu viele Argumente\n*/
};

#define R_OK A_READ
#define W_OK A_WRITE
#define X_OK A_EXEC
#define F_OK A_EXIST


/* The following few defines control the truth and false output of
   each stage.
   
   TRUE and FALSE are what we use to compute the final output value.

   SHELL_BOOLEAN is the form which returns truth or falseness in
   shell terms.
   
   TRUTH_OR is how to do logical or with TRUE and FALSE.

   TRUTH_AND is how to do logical and with TRUE and FALSE..

   Default is TRUE = 1, FALSE = 0, TRUTH_OR = a | b,
   TRUTH_AND = a & b, SHELL_BOOLEAN = (!value). 
*/

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define SHELL_BOOLEAN(value) (!(value))
#define TRUTH_OR(a, b) ((a) | (b))
#define TRUTH_AND(a, b) ((a) & (b))

typedef struct
{
	int pos;				/* position in list	*/
	int glob_argc;			/* number of args from command line	*/
	char **glob_argv;		/* the argument list */

	int broken;
} TESTINFO;

static int
test_syntax_error (MGLOBAL *M, TESTINFO *T, const char *format,
	const char *arg1, const char *arg2)
{
	T->broken = TRUE;
	eprintf (M, "%s: ", T->glob_argv[0]);
	eprintf (M, format, arg1, arg2);
	return 1;
}

/* beyond - call when we're beyond the end of the argument list (an
   error condition) */

static
int beyond (MGLOBAL *M, TESTINFO *T)
{
	test_syntax_error (M, T, NlsStr(TE_ARGEXP), "", "");
	return 1; 
}

/* advance - increment our position in the argument list. Check that
   we're not past the end of the argument list. This check is
   suppressed if the argument is FALSE. */

static int
advance (MGLOBAL *M, TESTINFO *T, int f)
{
	++T->pos;
	if (f && T->pos >= T->glob_argc) return beyond (M, T);
	return 0;
}

#define unary_advance(M, T) (advance (M, T, 1),++T->pos)

/* int_expt_err - when an integer argument was expected, but
    something else was found. */

static void
int_expt_err (MGLOBAL *M, TESTINFO *T, const char *where,
	const char *pch)
{
	test_syntax_error (M, T, NlsStr(TE_INTEXP), where, pch);
}

/* isint - is the argument whose position in the argument vector is
   'm' an int? Convert it for me too, returning it's value in 'pl'. */

static int
isint (TESTINFO *T, int m, long *pl)
{
	char *pch;

	pch = T->glob_argv[m];

	/* Skip leading whitespace characters. */
	while (*pch == '\t' || *pch == ' ') pch++;
	
	/* accept negative numbers but not '-' alone */
	if ('-' == *pch && '\000' == *++pch)
		return 0;

	while (*pch)
	{
		if (! isdigit (*pch)) return FALSE;

		++pch;
	}
	
	*pl = atol (T->glob_argv[m]);
	
	return TRUE;
}

/* age_of - find the age of the given file.  Return YES or NO
   depending on whether the file exists, and if it does, fill in
   the age with the modify time.

   jr: fixed to use the modification time (96/05/18) */

static int
age_of (TESTINFO *T, int posit, unsigned long *age)
{
	XATTR X;

	if (GetXattr (T->glob_argv[posit], TRUE, FALSE, &X))
		return FALSE;
	
	((int *)age)[0] = X.mdate;
	((int *)age)[1] = X.mtime;

	return TRUE;
}

/* jr: helper function for parsing of -l */

static void
get_arg (MGLOBAL *M, TESTINFO *T, int before, long *res)
{
	const char *where = before ? NlsStr(TE_BEFORE) : NlsStr(TE_AFTER);
	int index = before ? -1 : 1;

	if (!isint (T, T->pos + 1 + index, res))
		int_expt_err (M, T, where, T->glob_argv[T->pos + 1]);
}


/*
 * term - parse a term and return 1 or 0 depending on whether the term
 *	evaluates to true or false, respectively.
 *
 * term ::=
 *	'-'('h'|'d'|'f'|'r'|'s'|'w'|'c'|'b'|'p'|'u'|'g'|'k') filename
 *	'-'('L'|'x') filename
 * 	'-t' [ int ]
 *	'-'('z'|'n') string
 *	string
 *	string ('!='|'=') string
 *	<int> '-'(eq|ne|le|lt|ge|gt) <int>
 *	file '-'(nt|ot|ef) file
 *	'(' <expr> ')'
 * int ::=
 *	positive and negative integers
 */

int expr (MGLOBAL *M, TESTINFO *T);

static int
term (MGLOBAL *M, TESTINFO *T)
{
	unsigned long int ul, ur;
	long int l, r;
	int value;
	long fd;

	if (T->pos >= T->glob_argc) beyond (M, T);
	
	/* Deal with leading "not"'s. */
	while (T->pos < T->glob_argc &&
		!strcmp (T->glob_argv[T->pos], "!"))
	{
		advance (M, T, 1);
		
		/* This has to be rewritten to handle the TRUTH and FALSE stuff. */
		return !term (M, T);
	}

	if (!strcmp (T->glob_argv[T->pos], "("))
	{
		advance (M, T, 1);
		value = expr (M, T);
		
		if (T->pos >= T->glob_argc)
			test_syntax_error (M, T, NlsStr(TE_MISSCHAR), ")", "");
		else if (strcmp (T->glob_argv[T->pos], ")"))
			test_syntax_error (M, T, NlsStr(TE_ARGEXPF), 
				T->glob_argv[T->pos], "");

		advance (M, T, 0);
		return TRUE == (value);
	}

	/* are there enough arguments left that this could be dyadic? */
	if (T->pos + 2 < T->glob_argc)
	{
		int op = T->pos + 1;
		
		if (!strcmp (T->glob_argv[op], "-lt"))
		{
			get_arg (M, T, TRUE, &l);
			get_arg (M, T, FALSE, &r);

			T->pos += 3;
			return TRUE == (l < r);
		}
		else if (!strcmp (T->glob_argv[op], "-le"))
		{
			get_arg (M, T, TRUE, &l);
			get_arg (M, T, FALSE, &r);

			T->pos += 3;
			return TRUE == (l <= r);
		}
		else if (!strcmp (T->glob_argv[op], "-gt"))
		{
			get_arg (M, T, TRUE, &l);
			get_arg (M, T, FALSE, &r);

			T->pos += 3;
			return TRUE == (l > r);
		}		
		else if (!strcmp (T->glob_argv[op], "-ge"))
		{
			get_arg (M, T, TRUE, &l);
			get_arg (M, T, FALSE, &r);
			
			T->pos += 3;
			return TRUE == (l >= r);
		}
		else if (!strcmp (T->glob_argv[op], "-ne"))
		{
			get_arg (M, T, TRUE, &l);
			get_arg (M, T, FALSE, &r);

			T->pos += 3;
			return TRUE == (l != r);
		}
		else if (!strcmp (T->glob_argv[op], "-eq"))
		{
			get_arg (M, T, TRUE, &l);
			get_arg (M, T, FALSE, &r);

			T->pos += 3;
			return TRUE == (l == r);
		}
		else if (!strcmp (T->glob_argv[op], "-nt"))
		{
			/* nt - newer than */
			T->pos += 3;

			if (age_of (T, op - 1, &ul) && age_of (T, op + 1, &ur))
				return TRUE == (ul > ur);

			return FALSE;
		}
		else if (!strcmp (T->glob_argv[op], "-ot"))
		{
			/* ot - older than */
			T->pos += 3;

			if (age_of (T, op - 1, &ul) && age_of (T, op + 1, &ur))
				return TRUE == (ul < ur);
			
			return FALSE;
		}

		if (!strcmp (T->glob_argv[op], "="))
		{
			value = (0 == strcmp (T->glob_argv[op - 1], 
				T->glob_argv[op + 1]));
			T->pos += 3;
			return TRUE == value;
		}

		if (!strcmp (T->glob_argv[op], "!="))
		{
			value = 0 != strcmp (T->glob_argv[op - 1], 
				T->glob_argv[op + 1]);
			T->pos += 3;
			return TRUE == value;
		}
	}
	
	/* Might be a switch type argument */
	if ('-' == T->glob_argv[T->pos][0] 
		&& '\000' == T->glob_argv[T->pos][2] /* && pos < glob_argc-1 */ )
	{
		XATTR X;
		long stat;
		char realname[MAX_PATH_LEN];
		
		if (T->pos < (T->glob_argc - 1))
		{
			char *cp = NULL;
			char *origname = T->glob_argv[T->pos + 1];
			
			/* jr: don't convert device names */
			if (strlen (origname) != 4 && origname[3] != ':')
				cp = NormalizePath (&M->alloc, origname, 
						M->current_drive, M->current_dir);

			if (cp != NULL)
			{
				strncpy (realname, cp, MAX_PATH_LEN - 1);
				mfree (&M->alloc, cp);
			}
			else
				strncpy (realname, origname, MAX_PATH_LEN - 1);
		}

		switch (T->glob_argv[T->pos][1])
		{
			default:
				break;

			/* All of the following unary operators use unary_advance (),
			   which checks to make sure that there an argument, and
			   then advances pos right past it. This means that
			   pos - 1 is the location of the argument. */

			case 'r':		/* file is readable? */
				unary_advance (M, T);
				value = access (realname, R_OK, &T->broken) ? TRUE : FALSE;
				return TRUE == value;

			case 'w':		/* File is writeable? */
				unary_advance (M, T);
				value = access (realname, W_OK, &T->broken)? TRUE : FALSE;
				return TRUE == value;

			case 'x':		/* File is executable? */
				unary_advance (M, T);
				value = access (realname, X_OK, &T->broken)? TRUE : FALSE;
				return TRUE == value;

			case 'O':		/* File is owned by you? */
				unary_advance (M, T);
				value = access (realname, A_EXIST, &T->broken);
				return TRUE == value;

			case 'f':		/* File is a file? */
				unary_advance (M, T);
				stat = GetXattr (realname, FALSE, FALSE, &X);
				
				if (stat)
					return FALSE;
				else
					return (X.mode & S_IFMT) == S_IFREG;

			case 'd':		/* File is a directory? */
				unary_advance (M, T);
				return IsDirectory (realname, &T->broken);

			case 's':		/* File has something in it? */
				unary_advance (M, T);
				stat = GetXattr (realname, TRUE, FALSE, &X);
				
				if (stat)
					return FALSE;
				else
					return X.size > 0L;
				
			case 'S':		/* File is a socket? */
				unary_advance (M, T);
				return FALSE;

			case 'c': /* File is character special? xxx */
				unary_advance (M, T);
				if ((fd = Fopen(realname, 0)) >= 0)
				{
					value = (TRUE == (isatty ((int) fd) != 0));
					Fclose ((int) fd);
				}
				else
					value = FALSE;
				
				return value;

			case 'b':		/* File is block special? */
				unary_advance (M, T);
				return FALSE;

			case 'p':		/* File is a named pipe? */
				unary_advance (M, T);
				stat = GetXattr (realname, FALSE, FALSE, &X);
				
				if (stat)
					return FALSE;
				else
					return (X.mode & S_IFMT) == S_IFIFO;

			case 'L':		/* Same as -h  */
			case 'h':		/* File is a symbolic link? */
				unary_advance (M, T);
				stat = GetXattr (realname, FALSE, FALSE, &X);
				
				if (stat) return FALSE;

				return (X.mode & S_IFMT) == S_IFLNK;

			case 'u':		/* File is setuid? */
				unary_advance (M, T);
				stat = GetXattr (realname, FALSE, FALSE, &X);
				
				if (stat) return FALSE;

				return X.mode & S_ISUID;

			case 'g':		/* File is setgid? */
				unary_advance (M, T);
				stat = GetXattr (realname, FALSE, FALSE, &X);
				
				if (stat) return FALSE;

				return X.mode & S_ISGID;

			case 'k':		/* File has sticky bit set? */
				unary_advance (M, T);
				stat = GetXattr (realname, FALSE, FALSE, &X);
				
				if (stat) return FALSE;

				return X.mode & S_ISGID;

			case 't':		/* File (fd) is a terminal?  
							 * (fd) defaults to stdout. 
							 */
				advance (M, T, 0);
				if (T->pos < T->glob_argc && isint (T, T->pos, &r))
				{
					advance (M, T, 0);
					return TRUE == (isatty ((int) r) != 0);
				}
				return TRUE == (isatty (stdin->Handle) != 0);

			case 'n':		/* True if arg has some length. */
				++T->pos;
				if (T->pos < T->glob_argc)
				{
					++T->pos;
					return TRUE == 
						(0 != strlen (T->glob_argv[T->pos - 1]));
				}
				else
				{	/* kein Argument: wir gehen davon aus, daž
				     * das Argument durch ARGV verschluckt wurde.
				     */
					return FALSE;
				}

			case 'z':		/* True if arg has no length. */
				++T->pos;
				if (T->pos < T->glob_argc)
				{
					++T->pos;
					return TRUE == 
						(0 == strlen (T->glob_argv[T->pos - 1]));
				}
				else
				{	/* kein Argument: wir gehen davon aus, daž
				     * das Argument durch ARGV verschluckt wurde.
				     */
					return TRUE;
				}
		}
	}
	value = 0 != strlen (T->glob_argv[T->pos]);
	advance (M, T, 0);
	return value;
}

/*
 * and:
 *	and '-a' term
 *	term
 */
static int and (MGLOBAL *M, TESTINFO *T)
{
	int value;

	value = term (M, T);
	while (T->pos < T->glob_argc &&
		!strcmp (T->glob_argv[T->pos], "-a"))
	{
		advance (M, T, 0);
		value = TRUTH_AND (value, term (M, T));
	}
	return TRUE == value;
}

/*
 * or:
 *	or '-o' and
 *	and
 */
static int or (MGLOBAL *M, TESTINFO *T)
{
	int value;

	value = and (M, T);
	while (T->pos < T->glob_argc &&
		!strcmp (T->glob_argv[T->pos], "-o"))
	{
		advance (M, T, 0);
		value = TRUTH_OR (value, and (M, T));
	}
	return TRUE == value;
}

/*
 * expr:
 *	or
 */
static int expr (MGLOBAL *M, TESTINFO *T)
{
	int value;

	if (T->pos >= T->glob_argc) beyond (M, T);

	value = FALSE;

	return value ^ or (M, T);		/* Same with this. */
}

/*
 * [:
 *	'[' expr ']'
 * test:
 *	test expr
 */
int m_test (MGLOBAL *M, int argc, char **argv)
{
	TESTINFO T;
	int value;
	int expr (MGLOBAL *M, TESTINFO *T);

	if (!argc)
	{
		const char *opt;
		
		if (!strcmp (argv[0], "["))
			opt = NlsStr(OPT_testk);
		else
			opt = NlsStr(OPT_test);
			
		return PrintHelp (M, argv[0], opt, NlsStr(HLP_test));
	}
	
	T.glob_argv = argv;
	
	if (!strcmp (argv[0], "["))
	{
		--argc;

		if (argv[argc] && strcmp (argv[argc], "]"))
			return test_syntax_error (M, &T, NlsStr(TE_MISSCHAR), 
				"]", "");

		if (argc < 2) return 1;
	}

	T.glob_argc = argc;
	T.pos = 1;
	T.broken = FALSE;

	if (T.pos >= T.glob_argc)
		return 1;

	value = expr (M, &T);
	if (T.pos != T.glob_argc)
	{
		eprintf (M, NlsStr(TE_TOOMANY), argv[0]);
		T.broken = TRUE;
	}

	return T.broken || SHELL_BOOLEAN (value);
}
