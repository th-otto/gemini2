/*
 * @(#) MParse\chario.c
 * @(#) Stefan Eissing & Gereon Steffens, 27. Dezember 1993
 *
 *  -  Character I/O primitives
 *
 * jr 11.4.95
 */

#include <stddef.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <tos.h>
#include <vdi.h>
#include <mint\mintbind.h>
#include <mint\ioctl.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\strerror.h"
#include "mglob.h"

#include "chario.h"
#include "keys.h"
#include "mupfel.h"
#include "nameutil.h"
#include "redirect.h"
#include "stand.h"
#include "trap.h"
#if MERGED
#include "..\venus\console.h"
#endif

/* internal texts
 */
#define NlsLocalSection "M.chario"
enum NlsLocalText{
MI_IOFMT, 	/*%s%sI/O-Fehler %ld%s%s\n*/
MI_IOACC,	/* beim Zugriff auf */
};

#define ALTERNATE	0x08	/* bit to test in Kbshift() value */
#define CON			2 		/* BIOS screen device */
#define RAWCON		5		/* BIOS raw screen */

#define altpressed()	(Kbshift (-1) & ALTERNATE)
#define charpresent()	(Cconis () != 0)
#define waitaltchr()	while (altpressed() && !charpresent())
#define mkchr(a)		((char)(a & 0xFF)-'0')


void
InstallErrorFunction (MGLOBAL *M, void (*f)(const char *))
{
	M->tty.error_function = f;
}

void
DeinstallErrorFunction (MGLOBAL *M)
{
	M->tty.error_function = NULL;
}

static void
doCconws (int c)
{
	char *str = "x";
	
	*str = (char)c;
	Cconws (str);
}

void
clearintr (MGLOBAL *M)
{
	M->tty.ctrlc = M->tty.was_ctrlc = FALSE;
}

int
InitCharIO (MGLOBAL *M)
{
	M->tty.break_char = '\003';			/* Control-C */
	M->tty.kill_char = 0x1B;			/* Escape */
	M->tty.completion = '\t';			/* Tab */
	M->tty.meta_completion = '\014';	/* Control-L */
	M->tty.escape_char = '%';			/* UNIX: Backslash */

	memset (M->tty.fkeys, 0, sizeof (M->tty.fkeys));

	M->tty.ctrlc = M->tty.was_ctrlc = FALSE;
	M->tty.ta_in = M->tty.ta_out = -1;
	
	if (GEMDOSVersion < 0x15)	/* altes 1.0 und Blitter-TOS */
		M->tty.gemdos_out = doCconws;
	else
		M->tty.gemdos_out = Cconout;

	/* Setze die Werte fr Zeilen/Spalten erst einmal auf Defaults */
	M->tty.rows = M->tty.columns = 0;
	
	return TRUE;
}

void
SetRowsAndColumns (MGLOBAL *M, int rows, int columns)
{
/* stevie */
#if !GEMINI
	{
		struct winsize win;
		
		if (0 == Fcntl (-1, (long)&win, TIOCGWINSZ))
		{
			rows = win.ws_row;
			columns = win.ws_col;
		}
	}
#endif

	if (rows <= 0) rows = 25;
	if (columns <= 0) columns = 80;
	
	M->tty.rows = rows;
	M->tty.columns = columns;
}



/* Versuche, die exakte Anzahl von Zeilen/Spalten zu ermitteln */

void
InitRowsAndColumns (MGLOBAL *M)
{
	int rows, columns;
	
	rows = (int) GetVarInt (M, "ROWS");
	if (rows <= 0) rows = (int) GetVarInt (M, "LINES");
	columns = (int)GetVarInt (M, "COLUMNS");
	
	if (rows <= 0 || columns <= 0)
		if (M->shell_flags.gem_initialized)
			vq_chcells (M->vdi_handle, &rows, &columns);

	SetRowsAndColumns (M, rows, columns);
}

/* Setzte stderr */
	
void
InitStdErr (MGLOBAL *M)
{
	if (!NameExists (M, "STDERR"))
	{
		SetupStdErr (M);
		PutEnv (M, "STDERR=");
	}
}


int ReInitCharIO (MGLOBAL *old, MGLOBAL *new)
{
	int i;
	
	new->tty = old->tty;
	memset (new->tty.fkeys, 0, FKEYMAX * sizeof (char *));
	
	for (i = 0; i < FKEYMAX; ++i)
	{
		if (old->tty.fkeys[i])
		{
			new->tty.fkeys[i] = StrDup (&new->alloc, old->tty.fkeys[i]);
			if (!new->tty.fkeys[i]) break;
		}
	}

	return (i == FKEYMAX);	
}


int intr (MGLOBAL *M)
{
	return M->tty.ctrlc;
}

int wasintr (MGLOBAL *M)
{
	return M->tty.was_ctrlc;
}

void resetintr (MGLOBAL *M)
{
	M->tty.was_ctrlc = M->tty.ctrlc;
	M->tty.ctrlc = FALSE;
}

int checkintr (MGLOBAL *M)
{
	char c = '\0';

	if (!StdinIsatty (M))
		return FALSE;
	
	if (charpresent ())
		c = Crawcin () & 0xFF;

	return (c == CTRLC);
}

static void typeahead (MGLOBAL *M, long key)
{
	M->tty.ta_buffer[++(M->tty.ta_in)] = key;
	if (M->tty.ta_in == TYPEAHEADMAX-1)
		M->tty.ta_in = -1;
}

static void checkxoff (MGLOBAL *M)
{
	long key;
	char ch;
	int xon;

	if (!StdinIsatty (M) || !Cconis ())
		return;

	key = Crawcin ();
	ch = (char)key;

	if (ch == CTRLS)
	{
		do
		{
			key = Crawcin ();
			xon = ((char)key == CTRLQ) || ((char)key == CTRLC);
			if (!xon)
			{
				typeahead (M, key);
			}
			
		} while (!xon);
	}

	ch = (char)key;

	if (ch == CTRLC)
	{
		M->exit_shell = SignalTrap (M, SIGINT);
	}
	else if (ch != CTRLS && ch != CTRLQ)
	{
		typeahead (M, key);
	}
}

void rawout (MGLOBAL *M, char c)
{
	if (StdoutIsatty (M))
	{
#if MERGED
		if (M->shell_flags.interactive && 
			ConsoleIsOpen && StdoutIsConsole (M))
		{
			checkxoff (M);
			ConsoleDispRawChar (c);
		}
		else
#endif
		if (MiNTVersion != 0)
		{
			Fputchar (1, c, 1);
		}
		else 
		{
			checkxoff (M);
			if ((unsigned char)c != 0xFF)
				Crawio (c);
		}
	}
	else		
		M->tty.gemdos_out (c);
}

void canout (MGLOBAL *M, char c)
{
	if (StdoutIsatty (M))
	{
#if MERGED
		if (M->shell_flags.interactive && 
			ConsoleIsOpen && StdoutIsConsole (M))
		{
			checkxoff (M);
			ConsoleDispCanChar(c);
		}
		else
#endif
		if (MiNTVersion != 0)
		{
			Fputchar (1, c, 1);
		}
		else
		{
			checkxoff (M);
			if ((unsigned char)c != 0xFF)
				Crawio (c);
		}
	}
	else		
		M->tty.gemdos_out(c);
}

void rawoutn (MGLOBAL *M, char c, unsigned int n)
{
#if MERGED
	char tmpstr[301];

	while (n>300)
	{	
		memset (tmpstr, c, 300);
		tmpstr[300] = '\0';
		mprintf (M, tmpstr);
		n -= 300;
	}
	
	memset (tmpstr, c, n);
	tmpstr[n] = '\0';
	mprintf (M, tmpstr);
#else
	while (n--)
		rawout (M, c);
#endif
}

void crlf (MGLOBAL *M)
{
	canout (M, CR);
	canout (M, LF);
}

void beep (MGLOBAL *M)
{
	canout (M, BELL);
}

/* VT-52 escape sequence */
void vt52 (MGLOBAL *M, char c)
{
	canout (M, ESC);
	canout (M, c);
}

/* VT-52 cursor motion sequence */
void vt52cm (MGLOBAL *M, int x, int y)
{
	vt52 (M, 'Y');
	canout (M, y + ' ');
	canout (M, x + ' ');
}

int inbuffchar (MGLOBAL *M, long *lp)
{
	if (M->tty.ta_in >= 0 && M->tty.ta_out < M->tty.ta_in)
	{
		*lp = M->tty.ta_buffer[++M->tty.ta_out];
		if (M->tty.ta_out >= M->tty.ta_in)
			M->tty.ta_out = M->tty.ta_in = -1;
		return TRUE;
	}
	return FALSE;
}

long inchar (MGLOBAL *M, int cooked)
{
	long key;

	if (inbuffchar (M, &key))
		return key;
		
	if (MiNTVersion != 0)
	{
		if (StdinIsatty (M))
			Fputchar (1, 0, cooked? 1 : 0);
		
		if (setjmp (M->break_buffer) != 0)
		{
			if (M->exit_shell ||
				((M->exit_shell = CheckSignal (M)) != FALSE))
			{
				return 0x0000FF1AL;
			}
		}

		M->break_buffer_set = 1;
		key = Fgetchar (0, cooked ? 1 : 0);
		M->break_buffer_set = 0;
		
		return key;
	}
	else if (!StdinIsatty (M))
	{
		char c;

		if (1 != Fread (0, 1L, &c))
			c = 26;
	
		return (long)((unsigned char)c);
	}
	
	return Crawcin ();
}

int domprint (MGLOBAL *M, const char *str, size_t len)
{
	char *nl = strchr (str, '\n');
	int lastnl;
	
	lastnl = (nl == NULL || nl == &str[len-1]);

#if MERGED
	if (ConsoleIsOpen && StdoutIsConsole (M) && lastnl)
	{
		if (nl)
			*nl = '\0';
		ConsoleDispString (str);
		if (nl)
		{
			*nl = '\n';
			crlf (M);
		}
	}
	else
#endif
	{
		if (!StdoutIsatty (M) && lastnl)
		{
			if (nl)
				*nl = '\0';
			Fwrite (1, strlen(str), str);

			if (nl)
			{
				*nl='\n';
				crlf (M);
			}
		}
		else if (MiNTVersion != 0)
		{
			while (*str)
			{
				if (*str=='\n')
					Fputchar (1, '\r', 1);

				Fputchar (1, *str, 1);
				++str;
			}
		}
		else
		{
			while (*str)
			{
				if (*str=='\n')
					crlf (M);
				else
					rawout (M, *str);
				++str;
			}
		}
	}
	
	return (int)len;
}

int mprintf (MGLOBAL *M, const char *fmt,...)
{
	va_list argpoint;
	int len;
	char tmpstr[300];
	char *tp;

	if (strchr (fmt, '%'))
	{
		va_start (argpoint, fmt);
		len = vsprintf (tmpstr, fmt, argpoint);
		va_end (argpoint);
		tp = tmpstr;
	}
	else
	{
		tp = (char *)fmt;
		len = (int)strlen (fmt);
	}
	
	return domprint (M, tp, len);
}

int printArgv (MGLOBAL *M, int argc, const char **argv)
{
	int i;
	size_t len, total;
	
	total = 0L;
	for (i = 0; i < argc && !intr (M); ++i)
	{
		len = strlen (argv[i]);
		total += len;
		doeprint (M, (char *)argv[i], len);
		
		++total;
		if (i != argc - 1)
			doeprint (M, " ", 1);
		else
			doeprint (M, "\n", 1);
	}
	
	return (int)total;
}

int doeprint (MGLOBAL *M, const char *tp, size_t len)
{
#if MERGED
	if (M->tty.error_function != NULL)
	{
		M->exit_value = M->oserr;
		if (tp[len - 1] == '\n')
		{
			((char *)tp)[len - 1] = '\0';
			--len;
			M->tty.error_function (tp);
			((char *)tp)[len] = '\n';
		}
		else
			M->tty.error_function (tp);
		
		return (int)len;
	}
#endif

	if (!StderrIsatty (M))
	{
		while (tp && *tp)
		{
			char *nl;
			
			nl = strchr (tp, '\n');
			if (nl)
			{
				*nl = '\0';
				Fwrite (2, strlen(tp), tp);
				Fwrite (2, 2L, "\r\n");
				++len;
				tp = nl + 1;
			}
			else
			{
				Fwrite (2, strlen(tp), tp);
				tp = NULL;
			}

		}
	}
	else
	{
		while (*tp)
		{
			if (*tp == '\n')
			{
				Cauxout ('\r');
				Cauxout ('\n');
				++len;
			}
			else
				Cauxout (*tp);
			++tp;
		}
	}
	
	return (int)len;
}


int eprintf (MGLOBAL *M, const char *fmt, ...)
{
	va_list argpoint;
	int len;
	char tmpstr[1024];
	char *tp;

	va_start (argpoint, fmt);
	len = vsprintf (tmpstr, fmt, argpoint);
	va_end (argpoint);
	tp = tmpstr;
	
	return doeprint (M, tp, len);
}


/* Debug Version of mprint. Uses RAWCON regardless of redirection */
void dprintf (const char *fmt, ...)
{
	va_list argpoint;
	char tmp[300], *tp;
	
	va_start (argpoint, fmt);
	vsprintf (tmp, fmt, argpoint);
	va_end (argpoint);
	tp = tmp;
	while (*tp)
	{
		if (*tp=='\n')
		{
			Bconout (CON, '\r');
			Bconout (CON, '\n');
		}
		else
			Bconout (RAWCON, *tp);
		++tp;
	}
}

long
ioerror (MGLOBAL *M, const char *cmd, const char *str, long errcode)
{
	M->oserr = (int)errcode;

	if (cmd) eprintf (M, "%s: ", cmd);
	if (str) eprintf (M, "`%s' -- ", str);
	eprintf (M, "%s\n", StrError (M->oserr));

#if 0
	/* Sorry, but this _has_ to be one single mprint call... */
	eprintf (M, NlsStr (MI_IOFMT),
		cmd == NULL ? "" : cmd,
		cmd == NULL ? "" : ": ",
		errcode,
		str == NULL ? "" : NlsStr (MI_IOACC),
		str == NULL ? "" : str);
#endif
	return errcode;
}

int PrintHelp (MGLOBAL *M, const char *name, const char *options,
	const char *help)
{
	mprintf (M, "%s %s - %s\n", name, options, help);
	return 0;
}

int PrintUsage (MGLOBAL *M, const char *name, const char *usage)
{	
	eprintf (M, "Gebrauch: %s %s\n", name, usage);
	return 1;
}

/* Wait until one of the characters in allowed is typed in. */

char
CharSelect (MGLOBAL *M, const char *allowed)
{
	char ch, uch;
	int legal;
	
	do
	{
		ch = (char) inchar (M, FALSE);
		uch = toupper (ch);
		
		legal = (strchr (allowed, uch) != NULL) && (uch != '\0');
		if (legal)
			rawout (M, ch);
		else
			beep (M);
			
	} while (!legal);
	
	crlf (M);
	return uch;
}


void PrintStringArray (MGLOBAL *M, int argc, const char **argv)
{
	int maxcols, maxlen, scrncols, col = 0;
	int i, l;
	const char *ep;

	if (intr (M))
		return;
			
	if ((ep = GetEnv (M, "COLUMNS")) != NULL)
		scrncols = atoi (ep);
	else
		scrncols = 80;

	maxlen = 0;
	for (i = 0; i < argc; ++i)
	{
		l = (int)strlen (argv[i]);
		if (l > maxlen)
			maxlen = l;
	}
	if (maxlen > scrncols)
		scrncols = maxlen + 2;
		
	maxcols = scrncols / (maxlen += 2);
	for (i = 0; i < argc && !intr (M); ++i)
	{
		l = mprintf (M, "%s", argv[i]);
		if (++col % maxcols == 0)
		{
			col = 0;
			crlf (M);
		}
		else
			rawoutn (M, ' ', maxlen - l);
	}
	if (col % maxcols != 0)
		crlf (M);
}


void
ExitCharIO (MGLOBAL *M, MGLOBAL *parent)
{
	int i;
	
	if (parent != NULL)
	{
		long key;
		
		while (inbuffchar (M, &key))
			typeahead (parent, key);
	}
	
	for (i = 0; i < FKEYMAX; ++i)
		if (M->tty.fkeys[i])
			mfree (&M->alloc, M->tty.fkeys[i]);
}

