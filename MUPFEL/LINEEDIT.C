/*
 * @(#) Mupfel\lineedit.c
 * @(#) Stefan Eissing & Gereon Steffens, 14. MÑrz 1994
 *
 * jr 22.4.95
 */

#include <stddef.h> 
#include <stdio.h> 
#include <ctype.h> 
#include <string.h>
#include <tos.h>
#include <mint\mintbind.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"


#include "mglob.h"

#include "chario.h"
#include "complete.h"
#include "fkey.h"
#include "history.h"
#include "keys.h"
#include "lineedit.h"
#include "makeargv.h"
#include "mcd.h"
#include "mdef.h"
#include "nameutil.h"
#include "redirect.h"
#include "scan.h"
#include "stand.h"
#include "vt52.h"
#if GEMINI
#include "..\venus\gemini.h"
#include "..\venus\console.h"
#endif


/* internal texts
 */
#define NlsLocalSection "M.lineedit"
enum NlsLocalText{
LE_DISPLAY,	/*Wollen Sie alle %d Mîglichkeiten sehen? (j/n) */
LE_ANSWER	/*jJ*/
};

/* Schrittweite, in der der Zeilenbuffer vergrîûert wird */
#define LINE_GROW		256

/* Maximalzahl der mîglichen Komplettierungen, die wir ohne
   Nachfrage anzeigen. */
#define MAX_SHOW_ALWAYS	60


typedef struct
{
	char *line;
	size_t line_size;
	size_t max_offset;
	size_t akt_offset;
	
	int line_start;
	int columns;
	int rows;

	int cursor_x;
	int cursor_y;
	
	struct
	{
		unsigned with_history : 1;
		unsigned fkey_cr : 1;
		unsigned have_tried_completion : 1;
	} flags;
	
	int broken;
	
	int history_save_buffer;
	int history_search;
	const char *save_line;

	PromptFunction *prompt_func;
	const char *prompt;
		
} LINEINFO;


static void
setTerminalProperties (MGLOBAL *M, LINEINFO *L)
{
	L->rows = M->tty.rows;
	L->columns = M->tty.columns;
	if (L->rows <= 0) L->rows = 25;
	if (L->columns <= 0) L->columns = 80;
}


static int
InitLineInfo (MGLOBAL *M, LINEINFO *L)
{
	memset (L, 0, sizeof (LINEINFO));

	L->line = mcalloc (&M->alloc, LINE_GROW, 1);
	if (!L->line) {
		L->broken = TRUE;
		return FALSE;
	}
	
	L->line_size = LINE_GROW;

	setTerminalProperties (M, L);
		
	L->cursor_y = 0;
	L->cursor_x = 0;

	HistoryResetSaveBuffer (M, &L->history_save_buffer);
		
	return TRUE;
}


static void
ExitLineInfo (MGLOBAL *M, LINEINFO *L, int free_line)
{
	mfree (&M->alloc, L->save_line);
	if (free_line) mfree (&M->alloc, L->line);
}


static int
enlargeLineBuffer (MGLOBAL *M, LINEINFO *L)
{
	size_t new_size = L->line_size + LINE_GROW;
	char *tmp;
	
	tmp = mcalloc (&M->alloc, new_size, 1);
	if (!tmp) return FALSE;
	
	strncpy (tmp, L->line, L->max_offset);
	mfree (&M->alloc, L->line);
	L->line = tmp;
	L->line_size = new_size;
	
	return TRUE;
}


static int
outstr (MGLOBAL *M, char *s)
{
	int len = (int)strlen (s);

#if MERGED
	if (ConsoleIsOpen)
		ConsoleDispString (s);
	else
#endif
		while (*s)
			rawout (M, *s++);
			
	return len;
}

static void
cursorRight (LINEINFO *L, int count)
{
	int tmp;
	
	L->cursor_x += count;
	tmp = L->cursor_x / L->columns;
	L->cursor_x %= L->columns;
	L->cursor_y += tmp;
}

static void
cursorLeft (LINEINFO *L, int count)
{
	int tmp;
	
	L->cursor_x -= count;

	if (L->cursor_x < 0) {
		tmp = (L->cursor_x / L->columns) - 1;
		L->cursor_x %= L->columns;
		L->cursor_x += L->columns;
		L->cursor_y += tmp;
	}
}

static void
backspace (MGLOBAL *M, LINEINFO *L, int count, int destructive)
{
	int old_x, old_y;
	int on_old_line;
	
	old_x = L->cursor_x;
	old_y = L->cursor_y;
	
	cursorLeft (L, count);

	on_old_line = TRUE;
	while (old_y > L->cursor_y)
	{
		if (destructive)
		{
			if (on_old_line)
				erase_to_line_start (M);
			else
			{
				erase_line (M);
				/* Sch... vt52-Emulator im TOS. Bei erase_line wird
				der Cursor nicht auf Position 0, sondern auf 1
				gesetzt. */
				cursor_left (M);
				old_x = 0;
			}
		}
		
		cursor_up (M);
		--old_y;
		on_old_line = FALSE;
	}
	
	while (old_x < L->cursor_x) {
		cursor_right (M);
		++old_x;
	}
	
	while (old_x > L->cursor_x) {
		cursor_left (M);
		--old_x;
	}
	
	if (destructive) clear_to_end_of_line (M);
}

static void
moveToLineEnd (MGLOBAL *M, LINEINFO *L)
{
	int old_x, old_y;
	size_t len;
	
	len = L->max_offset - L->akt_offset;
	if (len <= 0) return;
	
	old_x = L->cursor_x;
	old_y = L->cursor_y;
	cursorRight (L, (int)len);
	
	while (old_y < L->cursor_y) {
		cursor_down (M);
		++old_y;
	}
	
	if (old_x < L->cursor_x)
	{
		len = L->cursor_x - old_x;
		
		outstr (M, &L->line[L->max_offset - len]);
	}
	else
	{
		while (old_x > L->cursor_x)
		{
			cursor_left (M);
			--old_x;
		}
	}
	
	L->akt_offset = L->max_offset;
}

/* jr: das schlieût leider Umlaute aus */


static int
wordChar (char ch)
{
	return ch != ' ';	/* testtesttest*/
/*	return isalnum (ch) || strchr ("$,=.:\\\"\'<>!", ch); */
}


static void wordRight (MGLOBAL *M, LINEINFO *L)
{
	if (L->akt_offset == L->max_offset)
	{
		beep (M);
		return;
	}
	
	while (L->akt_offset < L->max_offset 
		&& wordChar (L->line[L->akt_offset]))
	{
		rawout (M, L->line[L->akt_offset]);
		++L->akt_offset;
		cursorRight (L, 1);
	}

	while (L->akt_offset < L->max_offset 
		&& !wordChar (L->line[L->akt_offset]))
	{
		rawout (M, L->line[L->akt_offset]);
		++L->akt_offset;
		cursorRight (L, 1);
	}
}


static void wordLeft (MGLOBAL *M, LINEINFO *L)
{
	size_t move_offset;
	
	if (L->akt_offset == 0)
	{
		beep (M);
		return;
	}
	
	move_offset = L->akt_offset;
	while (move_offset > 0 
		&& !wordChar (L->line[move_offset - 1]))
	{
		--move_offset;
	}

	while (move_offset > 0 
		&& wordChar (L->line[move_offset - 1]))
	{
		--move_offset;
	}

	backspace (M, L, (int)(L->akt_offset - move_offset), FALSE);
	L->akt_offset = move_offset;
}


static void clearInputLine (MGLOBAL *M, LINEINFO *L)
{
	moveToLineEnd (M, L);
	backspace (M, L, (int)L->max_offset, TRUE);
	
	memset (L->line, 0, L->line_size);
	L->akt_offset = L->max_offset = 0;
}


static int displayNewLine (MGLOBAL *M, LINEINFO *L, const char *line)
{
	char *tmp;
	size_t len;
	
	len = strlen (line);
	
	if (len > L->line_size)
	{
		tmp = mcalloc (&M->alloc, len + 3, 1);
		if (!tmp)
			return FALSE;
		
		L->line_size = len + 3;
		mfree (&M->alloc, L->line);
		L->line = tmp;
	}
	
	cursor_off (M);

	clearInputLine (M, L);

	strcpy (L->line, line);
	outstr (M, L->line);
	cursorRight (L, (int)len);

	cursor_on (M);
	
	L->max_offset = L->akt_offset = len;

	return TRUE;
}

static int processScancode (MGLOBAL *M, LINEINFO *L, int scan)
{
	static int insertChar (MGLOBAL *M, LINEINFO *L, int chr, int scan);
	int downward = TRUE;
	
	if (L->flags.with_history)
	{
		if ((scan >= F1 && scan <= F10) 
			|| (scan >= SF1 && scan <= SF10))
		{
			const char *fkey;
			int key_no = ConvKey (scan);
			
			L->flags.fkey_cr = 0;
			fkey = GetFKey (M, key_no);

			while (*fkey)
			{
				if (*fkey == '|' && *(fkey + 1) == '\0')
				{
					++fkey;
					L->flags.fkey_cr = 1;
				}
				else
					insertChar (M, L, *fkey++, 0);
			}

			return TRUE;
		}
	}
	
	switch (scan)
	{
		case LT_ARROW:
			if (L->akt_offset > 0)
			{
				int c;
				
				c = L->cursor_x == 0;
				
				if (c)
					cursor_off (M);

				backspace (M, L, 1, FALSE);
				--L->akt_offset;
				
				if (c)
					cursor_on (M);
			}
			else
				beep (M);
			break;
			
		case RT_ARROW:
			if (L->akt_offset < L->max_offset)
			{
				rawout (M, L->line[L->akt_offset]);
				cursorRight (L, 1);
				++L->akt_offset;
			}
			else
			{	
				if (L->flags.with_history)
				{
					const char *last_line;
					
					last_line = GetLastHistoryLine (M);

					if (last_line != NULL
						&& strlen (last_line) > L->akt_offset)
					{
						insertChar (M, L, last_line[L->akt_offset],0);
					}
					else
						beep (M);
				}
				else
					beep (M);
			}
			break;

		case UP_ARROW:
			downward = FALSE;
		case DN_ARROW:
			if (L->flags.with_history)
			{
				const char *hist;
				
				L->history_search = TRUE;

				if (!L->save_line)
				{
					L->save_line = StrDup (&M->alloc, L->line);
				}
				
				hist = GetHistoryLine (M, !downward, 
					&L->history_save_buffer, NULL);

				if (hist)
				{
					displayNewLine (M, L, hist);
				}
				else if (downward && L->save_line
					&& strcmp (L->save_line, L->line))
				{
					displayNewLine (M, L, L->save_line);
					HistoryResetSaveBuffer (M, 
						&L->history_save_buffer);
				}
				else
					beep (M);
			}
			else
				beep (M);
			break;

		case SUP_ARROW:
			downward = FALSE;
		case SDN_ARROW:
			if (L->flags.with_history)
			{
				const char *hist;
				
				L->history_search = TRUE;

				if (!L->save_line)
				{
					L->save_line = StrDup (&M->alloc, L->line);
				}

				hist = GetHistoryLine (M, !downward, 
					&L->history_save_buffer, L->save_line);

				if (hist)
				{
					displayNewLine (M, L, hist);
				}
				else if (downward && L->save_line
					&& strcmp (L->save_line, L->line))
				{
					displayNewLine (M, L, L->save_line);
					HistoryResetSaveBuffer (M, 
						&L->history_save_buffer);
				}
				else
					beep (M);
			}
			else
				beep (M);
			break;

		case SRT_ARROW:
			if (L->akt_offset < L->max_offset)
			{
				cursor_off (M);
				moveToLineEnd (M, L);
				cursor_on (M);
			}
			break;

		case SLT_ARROW:
			if (L->akt_offset > 0)
			{
				cursor_off (M);
				backspace (M, L, (int)L->akt_offset, FALSE);
				L->akt_offset = 0;
				cursor_on (M);
			}
			break;

		case CLT_ARROW:
			cursor_off (M);
			wordLeft (M, L);
			cursor_on (M);
			break;

		case CRT_ARROW:
			cursor_off (M);
			wordRight (M, L);
			cursor_on (M);
			break;

		default:
			beep (M);
			break;
	}
	
	return TRUE;
}


static int insertChar (MGLOBAL *M, LINEINFO *L, int chr, int scan)
{
	/* PrÅfe erst auf Control-Codes */
	switch (chr)
	{
		case CTRLA:
			chr = '4';
			scan = LT_ARROW;
			break;
			
		case CTRLE:
			chr = '6';
			scan = RT_ARROW;
			break;
		
		case CTRLF:
			chr = 0;
			scan = RT_ARROW;
			break;
		
		case CTRLB:
			chr = 0;
			scan = LT_ARROW;
			break;
		
		case CTRLP:
			chr = 0;
			scan = UP_ARROW;
			break;
			
		case CTRLN:
			chr = 0;
			scan = DN_ARROW;
			break;
	}

	/* is it a shifted arrow key? */
	if (chr == '6' && scan == RT_ARROW
		|| chr == '4' && scan == LT_ARROW
		|| chr == '8' && scan == UP_ARROW
		|| chr == '2' && scan == DN_ARROW)
	{
		chr = 0;
		scan = -scan;
	}

	/* Dieser Test filter Control- und NUL-Zeichen nach ASCII.
	 * Ich glaube kaum, daû wir mal etwas anderes bekommen.
	 */
	if (chr >= 0 && chr < ' ')
	{
		/* Process scan codes
		 */
		if (scan)
			return processScancode (M, L, scan);
		else
		{
			L->history_search = TRUE;
			return TRUE;
		}
	}
	
	/* Erst einmal prÅfen, ob wir noch genug Platz im Buffer
	   haben. */

	if ((L->max_offset >= L->line_size - 3)
		&& !enlargeLineBuffer (M, L))
	{
		L->broken = TRUE;
		return FALSE;
	}
		
	if (L->max_offset == L->akt_offset)
	{
		/* Der Cursor steht am Ende der Zeile. Dies ist der 
		einfachste Fall. */

		rawout (M, chr);
		cursorRight (L, 1);
		L->line[L->akt_offset] = chr;
		++L->akt_offset;
		++L->max_offset;
	}
	else
	{
		/* Wir fÅgen ein Zeichen in der Mitte der Zeile ein */
		int len;

		memmove (&L->line[L->akt_offset + 1], &L->line[L->akt_offset],
			 L->max_offset - L->akt_offset + 1);
		L->line[L->akt_offset] = chr;
		++L->max_offset;
		L->line[L->max_offset] = '\0';
		
		cursor_off (M);
		
		rawout (M, chr);
		cursorRight (L, 1);
		++L->akt_offset;
		
		len = outstr (M, &L->line[L->akt_offset]);
		cursorRight (L, len);
		backspace (M, L, len, FALSE);

		cursor_on (M);
	}
	
	return TRUE;
}

static int deleteChar (MGLOBAL *M, LINEINFO *L, int chr)
{
	if (((chr == '\b') && L->akt_offset < 1) 
		|| ((chr == DEL) && L->akt_offset < 0))
	{
		beep (M);
		return TRUE;
	}

	if (L->akt_offset < L->max_offset)
	{
		int len;
		
		/* Wir sind irgendwo in der Mitte der Zeile */
		if (chr == '\b')
		{
			memmove (&L->line[L->akt_offset - 1], 
				&L->line[L->akt_offset], 
				L->max_offset - L->akt_offset + 1);

			backspace (M, L, 1, FALSE);
			--L->akt_offset;
		}
		else
	 	{
			memmove (&L->line[L->akt_offset], 
				&L->line[L->akt_offset + 1], 
				L->max_offset - L->akt_offset);
		}
		
		--L->max_offset;
		len = outstr (M, &L->line[L->akt_offset]) + 1;
		rawout (M, ' ');

		cursorRight (L, len);
		backspace (M, L, len, FALSE);
	 	L->line[L->max_offset] = '\0';
	}
	else
	{
		/* Der Cursor steht am Ende der Zeile
		 */
		if (chr == '\b' && L->akt_offset > 0)
		{
			backspace (M, L, 1, TRUE);
			--L->akt_offset;
			--L->max_offset;
			L->line[L->max_offset] = '\0';
		}
		else
			beep (M);
	}
	
	return TRUE;
}

static int deleteTillEOL (MGLOBAL *M, LINEINFO *L)
{
	if (L->akt_offset < L->max_offset)
	{
		size_t len;
		int old_x, old_y;
		
		len = L->max_offset - L->akt_offset;
		
		old_x = L->cursor_x;
		old_y = L->cursor_y;
		
		cursorRight (L, (int)len);
		
		if (old_y < L->cursor_y)
		{
			size_t i;
			
			for (i = old_y; i < L->cursor_y; ++i)
				cursor_down (M);
			
			for (i = old_y; i < L->cursor_y; ++i)
			{
				erase_line (M);
				cursor_left (M);
				cursor_up (M);
			}
			
			for (i = 0; i < old_x; ++i)
				cursor_right (M);
		}
		
		clear_to_end_of_line (M);
		cursorLeft (L, (int)len);
		
		memset (&L->line[L->akt_offset], 0, len);
		L->max_offset = L->akt_offset;
	}

	return TRUE;
}


static void restoreLine (MGLOBAL *M, LINEINFO *L)
{
	size_t diff;

	setTerminalProperties (M, L);
	
	cursor_off (M);
	wrapon (M);

	if (L->prompt != NULL)
	{
		L->line_start = (int)L->prompt_func (M, L->prompt);
	}	
	
	mprintf (M, L->line);
	
	diff = L->max_offset - L->akt_offset;
	if (diff > 0)
	{
		cursorRight (L, (int)diff);
		L->akt_offset = L->max_offset;
		backspace (M, L, (int)diff, FALSE);
		L->akt_offset -= diff;
	}

	cursor_on (M);
}


static int ReInitLineOnChanges (MGLOBAL *M, LINEINFO *L)
{
	int old_rows, old_cols;
	
	old_rows = L->rows;
	old_cols = L->columns;
	setTerminalProperties (M, L);
	if ((old_rows > L->rows) || ((old_cols != L->columns) 
		&& (L->max_offset + L->line_start >= min (old_cols, L->columns))))
	{
		size_t old_offset = L->akt_offset;
		int total_offset;
		
		total_offset = (old_cols * L->cursor_y) + L->cursor_x;

		cursor_off (M);
		moveToLineEnd (M, L);
		crlf (M);
		
		/* Zeilenbreite und/oder Hîhe haben sich geÑndert die
		 * Koordinaten fÅr cursor_x und cursor_y stimmen daher
		 * nicht mehr.
		 */
		L->akt_offset = old_offset;
		L->cursor_y = total_offset / L->columns;
		L->cursor_x = total_offset % L->columns;
		
		restoreLine (M, L);
		return TRUE;
	}
	
	return FALSE;
}


static void showPossibleCompletions (MGLOBAL *M, LINEINFO *L)
{
	ARGVINFO A;
	
	if (GetCompletions (M, L->line, (int)L->akt_offset, &A))
	{
		if (A.argc)
		{
			size_t diff;
			int print_it = TRUE;
			
			diff = L->max_offset - L->akt_offset;
			if (diff)
			{
				cursor_off (M);
				moveToLineEnd (M, L);
				cursor_on (M);
			}
			crlf (M);
	
			if (A.argc > MAX_SHOW_ALWAYS)
			{
				char c;
				int key_code;
#if GEMINI
				int line_dirty;
#endif
				
				mprintf (M, NlsStr(LE_DISPLAY), A.argc);

#if GEMINI
				L->broken = GetCharFromGemini (&key_code, &line_dirty);
#else
				key_code = (int) inchar (M, FALSE);
#endif
				c = (char)(key_code & 0xFF);
				crlf (M);
				print_it = strchr (NlsStr(LE_ANSWER), c) != NULL;
			}
			
			if (print_it)
			{
				PrintStringArray (M, A.argc, A.argv);
				clearintr (M);
			}
			
			if (diff)
			{
				cursorLeft (L, (int)diff);
				L->akt_offset = L->max_offset - diff;
			}
			restoreLine (M, L);
		}
		else
			beep (M);
		
		FreeArgv (&A);
	}
	else
		beep (M);
}


static char *feedChar (MGLOBAL *M, LINEINFO *L, long lchar)
{
	int chr, scan;
	int done = FALSE;
	
	ReInitLineOnChanges (M, L);
	
	chr = (int)(lchar & 0xFF);
	scan = (int)((lchar >> 16) & 0xFF);

	/* Wenn wir versucht haben, das Wort unter dem Cursor zu
	vervollstÑndigen und dies nicht geklappt hat, dann mÅssen wir
	bei Ankommen eines anderen Buchstabens diesen Status zurÅcksetzen. */

	if (L->flags.have_tried_completion && chr != M->tty.completion)
		L->flags.have_tried_completion = FALSE;
	
	if (M->tty.break_char && chr == M->tty.break_char)
	{
		memset (L->line, 0, L->line_size);
		L->akt_offset = L->max_offset = 0;
		done = TRUE;
		L->broken = TRUE;
	}
	else if (M->tty.kill_char && chr == M->tty.kill_char)
	{
		cursor_off (M);
		clearInputLine (M, L);
		cursor_on (M);
	}
	else if (M->tty.completion && chr == M->tty.completion)
	{
		if (!L->flags.have_tried_completion)
		{
			char *found;
		
			found = CompleteWord (M, L->line, (int)L->akt_offset);

			L->flags.have_tried_completion = !(found && *found);

			if (found)
			{
				char *c = found;
				
				while (*c) insertChar (M, L, *c++, 0);
	
				mfree (&M->alloc, found);
			}
		}
		else
		{
			showPossibleCompletions (M, L);
		}
	}
	else if ((M->tty.meta_completion && chr == M->tty.meta_completion)
		|| (scan == HELP))
	{
		showPossibleCompletions (M, L);
	}
	else
	{
		switch (chr)
		{
			case '\0':
				if (scan == 0)
					break;
				
			default:
				insertChar (M, L, chr, scan);
				if (!L->flags.fkey_cr)
					break;
			
			case '\n':
			case '\r':
				if (L->akt_offset < L->max_offset)
				{
					cursor_off (M);
					moveToLineEnd (M, L);
				}
				L->line[L->max_offset] = '\n';
				done = TRUE;
				break;
			
			case '\b':
			case DEL:
			case CTRLD:
				cursor_off (M);
				deleteChar (M, L, chr);
				cursor_on (M);
				break;
			
			case CTRLDEL:
			case CTRLK:
				deleteTillEOL (M, L);
				break;
		}
	}
	
	if (done)
	{
		crlf (M);
		cursor_on (M);
	}
	
	if (L->flags.with_history && !L->history_search)
		HistoryResetSaveBuffer (M, &L->history_save_buffer);
	
	return done? L->line : NULL;
}


static char *
readFromFile (MGLOBAL *M, LINEINFO *L)
{
	char c;
	int done = FALSE;
	
	while (!done)
	{
		c = (char)inchar (M, TRUE);
	
		switch (c)
		{
			case '\0':
			case 26:
				done = L->broken = TRUE;
				break;
				
			case '\r':
				break;
			
			default:
				if ((L->max_offset >= L->line_size - 3)
					&& !enlargeLineBuffer (M, L))
				{
					done = L->broken = TRUE;
					break;
				}
					
				L->line[L->akt_offset] = c;
				++L->akt_offset;
				++L->max_offset;
				break;

			case '\n':
				L->line[L->akt_offset] = '\n';
				done = TRUE;
				break;
		}
	}
	
	if (L->broken)
	{
		mfree (&M->alloc, L->line);
		L->line = NULL;
	}
	
	return L->line;
}

/* ReadLine liest eine Zeile von Stdin und gibt eine allozierte
   Zeile oder NULL zurÅck.
   
   Wenn <with_history> != 0 ist, kann in der History geblÑttert
   werden.
   
   Ist <prompt> != NULL, wird die Funktion <show_prompt> mit 
   diesem Parameter aufgerufen. Die Funktion gibt die Anzahl der 
   ausgegebenen Zeichen der letzten ausgegebenen Zeile zurÅck. */

char *
ReadLine (MGLOBAL *M, int with_history, const char *prompt,
	PromptFunction *show_prompt, int *broken)
{
	LINEINFO L;
	char *read_line = NULL;
	
	*broken = FALSE;
	
	if (!InitLineInfo (M, &L))
		return NULL;
	
	if (!StdinIsatty (M))
	{
		read_line = readFromFile (M, &L);
	}
	else
	{
		L.prompt_func = show_prompt;
		L.prompt = prompt;
		
		if (with_history)
		{
			erase_line (M);
		}
		
		if (prompt != NULL)
		{
			L.line_start = (int)show_prompt (M, prompt);
			cursorRight (&L, L.line_start);
			if (intr (M))
				clearintr (M);
		}	
		
		L.flags.with_history = with_history;
		wrapon (M);
		cursor_on (M);

		do
		{
			long read_char;
			int line_dirty;
			
			L.history_search = FALSE;
			line_dirty = FALSE;
#if GEMINI
			if (inbuffchar (M, &read_char))
			{
				/* do nothing
				 */
			}
			else if (M->shell_flags.interactive
				&& !M->shell_flags.system_call)
			{
				int key_code;
				
				/* Schalte den Prozess auf cooked-Eingaben, damit
				 * alle Control-Chars ankommen.
				 */
				if (MiNTVersion) Fputchar (1, 0, 0);
					
				L.broken = GetCharFromGemini (&key_code, &line_dirty);
				if (L.broken)
				{
					read_line = NULL;
					M->exit_shell = TRUE;
				}
				else
				{
					read_char = (unsigned long)(key_code & 0x00FF) 
						| (((unsigned long)key_code & 0x00FF00L) << 8);
				}
			}
			else
#endif
			{
				read_char = inchar (M, FALSE);
				if (read_char == 0x0000FF1AL)
					L.broken = TRUE;
			}

			if (!L.broken)
			{
				if (line_dirty)
				{
					line_dirty = FALSE;
					
					if (!ReInitLineOnChanges (M, &L))
					{
						cursor_off (M);
						moveToLineEnd (M, &L);
						crlf (M);
						restoreLine (M, &L);
					}
				}
				else
					read_line = feedChar (M, &L, read_char);

				if (!L.history_search && L.save_line)
				{
					mfree (&M->alloc, L.save_line);
					L.save_line = NULL;
				}
			}
		}
		while (!L.broken && read_line == NULL);
		
		if (read_line && read_line[0] 
			&& !L.broken && strcmp (read_line, "\n")
			&& L.flags.with_history)
		{
			EnterHistory (M, read_line);
		}
		
#if GEMINI
		if (!L.broken && read_line != NULL)
		{
			int broken = FALSE;
			char path[MAX_PATH_LEN];
			
			sprintf (path, "%c:%s", M->current_drive, M->current_dir);
			ChangeDir (M, path, FALSE, &broken);
		}
#endif
	}
	
	*broken = L.broken;
	ExitLineInfo (M, &L, read_line == NULL);

	if (MiNTVersion) Fputchar (1, 0, 1);

	return read_line;
}


