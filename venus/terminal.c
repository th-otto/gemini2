/*
 * @(#) Gemini\Terminal.c
 * @(#) Arnd Beissner & Stefan Eissing, 18. MÑrz 1993
 *
 * description: functions for mupfel window
 *
 * jr 27. Oktober 1995
 */
 
#include <stddef.h>
#include <stdlib.h>
#include <tos.h>
#include <aes.h>
#include <string.h>
#include <mint\ioctl.h>
#include <mint\mintbind.h>
#include <nls\nls.h>

#include <common/cookie.h>

#include <mupfel/mupfel.h> /* xxx */

#include "vs.h"

#include "ascii.h"
#include "terminal.h"
#include "termio.h"
#include "console.h"
#include "venuserr.h"


TermInfo BiosTerminal;

/* jr: Line A stuff */

extern void LineAInit (void), LineAExit (void);
extern word *LineACelMX, *LineACelMY, *LineACurX, *LineACurY;
extern word *LineASavX, *LineASavY;


/* internal texts */
#define NlsLocalSection "G.appledit.c"
enum NlsLocalText{
T_NOPTY,	/*Konnte kein virtuelles Terminal îffnen!*/
};

static void _level0 (TermInfo *terminal, char chr);
static void _level1 (TermInfo *terminal, char chr);
static void _level2 (TermInfo *terminal, char chr);
static void _level3 (TermInfo *terminal, char chr);


static void
sizeScreenBuffer (TermInfo *terminal, int diff_cursor_y, int allocate)
{
	int y;
	int diffcols = terminal->columns - terminal->lastcols;
	char *nscreen, *nattrib;

/* venusErr ("ssb: %d %d", terminal->columns, terminal->rows); */
	
	if (diff_cursor_y > 0)
	{
		/* Wir verkleinern die Hîhe des Fensters. Es sollten aber
		 * die unteren Zeilen zu sehen sein. Deshalb mÅssen wir
		 * sie hochschieben.
		 */
		TermInfo tmp;
		
		tmp.pscreen = terminal->pscreen;
		tmp.pattrib = terminal->pattrib;
		tmp.columns = terminal->lastcols;
		tmp.rows = terminal->lastrows;
		for (y = 0; y < terminal->rows; ++y)
		{
			memcpy (ti_sline (&tmp, y),
				ti_sline (&tmp, y + diff_cursor_y),
				tmp.columns);
			memcpy (ti_aline (&tmp, y),
				ti_aline (&tmp, y + diff_cursor_y),
				tmp.columns);
		}
	}
	
	if (allocate)
	{
		int maxy, maxx;
		long amount = terminal->rows * (terminal->columns + 2L);
		
		/* neuen Speicher allozieren */
		nscreen = malloc (amount);
		nattrib = malloc (amount);
		if (!nscreen || !nattrib) {
			free (nscreen);
			free (nattrib);
			terminal->columns = terminal->lastcols;
			terminal->rows = terminal->lastrows;
			return;
		}
		
		maxy = (terminal->rows > terminal->lastrows)? 
				terminal->lastrows : terminal->rows;
		maxx = (terminal->columns > terminal->lastcols)? 
				terminal->lastcols : terminal->columns;

		/* bisherigen Inhalt rÅberkopieren */
		for (y = 0; y < maxy; ++y)
		{
			memmove (nscreen + (y * (terminal->columns + 2)),
				terminal->pscreen  + (y * (terminal->lastcols + 2)),
				maxx);
			memmove (nattrib + (y * (terminal->columns + 2)),
				terminal->pattrib  + (y * (terminal->lastcols + 2)),
				maxx);
			
			if (maxx < terminal->columns)
			{
				memset (nscreen + (y * (terminal->columns + 2)) + maxx,
					' ', terminal->columns - maxx);
				memset (nattrib + (y * (terminal->columns + 2)) + maxx,
					V_NORMAL, terminal->columns - maxx);
			}
			nscreen[(y * (terminal->columns + 2)) 
				+ terminal->columns] = '\0';
		}
		
		for ( ; y < terminal->rows; ++y)
		{
			memset (nscreen + (y * (terminal->columns + 2)),
				' ', terminal->columns);
			nscreen[(y * (terminal->columns + 2)) 
				+ terminal->columns] = '\0';
			memset (nattrib + (y * (terminal->columns + 2)),
				V_NORMAL, terminal->columns);
		}
		
		free (terminal->pscreen);
		free (terminal->pattrib);
		terminal->pscreen = nscreen;
		terminal->pattrib = nattrib;
	}

	/* Erzeuge leere Spalten am Ende */
	for (y = 0; y < terminal->rows; ++y)
	{
		if (diffcols > 0)
		{
			memset (&ti_sline(terminal,y)[terminal->lastcols], ' ', diffcols);
			memset (&ti_aline(terminal,y)[terminal->lastcols], V_NORMAL, diffcols);
		}
		
		ti_sline (terminal, y)[terminal->columns] = '\0';
	}

	/* Erzeuge leere Zeilen am Ende */
	for (y = terminal->rows; y < terminal->rows; ++y)
	{
		memset (ti_sline (terminal, y), ' ', terminal->columns);
		memset (ti_aline (terminal, y), V_NORMAL, terminal->columns);

		ti_sline (terminal, y)[terminal->columns] = '\0';
	}
	
	terminal->lastcols = terminal->columns;
	terminal->lastrows = terminal->rows;
	
	/* jr: Grîûe in Line-A eintragen */
	if (terminal->flags.is_console && LineACelMX && LineACelMY)
	{
		*LineACelMX = terminal->columns - 1;
		*LineACelMY = terminal->rows - 1;
	}
}


/* Terminal initialisieren */
TermInfo *InitTerminal (int is_console, word *sysfonts)
{
	TermInfo *terminal;
	
	if (is_console)
		terminal = &BiosTerminal;
	else
	{
		terminal = calloc (1, sizeof (TermInfo));
		terminal->font_id = BiosTerminal.font_id;
		terminal->font_size = BiosTerminal.font_size;
		terminal->font_width = BiosTerminal.font_width;
		terminal->font_height = BiosTerminal.font_height;
	}
		
	if (terminal == NULL) return NULL;

	terminal->flags.is_console = is_console;
	
	if (!TM_Init (terminal, sysfonts))
	{
		if (!is_console) free (terminal);
		return NULL;
	}
	
	terminal->_x_saved = 0;
	terminal->_y_saved = 0;
	terminal->p_x_saved = &(terminal->_x_saved);
	terminal->p_y_saved = &(terminal->_y_saved);

	terminal->term_out = _level0;
	terminal->lastrows = terminal->lastcols = 0;
	
	terminal->_cur_x = 0;
	terminal->_cur_y = 0;
	terminal->p_cur_x = &(terminal->_cur_x);
	terminal->p_cur_y = &(terminal->_cur_y);

	terminal->wr_mode = V_NORMAL;
	terminal->inv_type = V_STANDOUT;
	terminal->cur_on = TRUE;
	terminal->columns = 80;
	terminal->rows = 25;
	terminal->wrap = TRUE;
	terminal->tab_size = 8;
	terminal->tab_destructive = FALSE;
	terminal->back_destructive = FALSE;
	terminal->window_handle = 0;
	terminal->pscreen = NULL;
	terminal->pattrib = NULL;

	sizeScreenBuffer (terminal, 0, 1);
	
	if (is_console) return terminal;
	
	{
		int i;
		
		strcpy (terminal->termname, "U:\\pipe\\pty.gA");
		for (i = 0; i < 16; i++)
		{
			terminal->termname[13] = 'A' + i;
			terminal->pty = (int)Fcreate (terminal->termname, 
				FA_SYSTEM|FA_HIDDEN);
			if (terminal->pty > 0)
			{
				/* set to non-delay mode, so Fread() won't block
				 */
				Fcntl (terminal->pty, O_NDELAY, F_SETFL);

				{
					struct winsize tw;

					tw.ws_xpixel = tw.ws_ypixel = 0;
					tw.ws_row = terminal->rows;
					tw.ws_col = terminal->columns;
					Fcntl (terminal->pty, (long)&tw, TIOCSWINSZ);
				}

				break;
			}
		}
	}
	
	if (terminal->pty < 0)
	{
		venusErr (NlsStr (T_NOPTY));
		if (!is_console) free (terminal);
		return NULL;
	}
	
	return terminal;
}

/* Terminal deinitialisieren */
void ExitTerminal (TermInfo *terminal)
{
	TM_Deinit (terminal);

	if (terminal->pty > 0) Fclose (terminal->pty);
	
	free (terminal->pscreen); terminal->pscreen = NULL;
	free (terminal->pattrib); terminal->pattrib = NULL;
	
	if (!terminal->flags.is_console) free (terminal);
}



/* Terminal updaten */
void ReInitTerminal (TermInfo *terminal, int newcol, int newlines)
{
	int diff_cursor_y = 0;
	
	/* Cursor soll sichtbar bleiben */
	if ((*terminal->p_cur_y) >= newlines)
	{
		diff_cursor_y = (*terminal->p_cur_y) - newlines + 1;
		(*terminal->p_cur_x) = 0;
		(*terminal->p_cur_y) = newlines - 1;
	}
	else if ((*terminal->p_cur_x) >= newcol)
	{
		(*terminal->p_cur_x) = newcol - 1;
	}

	terminal->columns = newcol;
	terminal->rows = newlines;	
	sizeScreenBuffer (terminal, diff_cursor_y, 1);
}
		

/* Bildschirm-Buffer lîschen */	
void DelScreenBuffer (TermInfo *terminal)
{
	terminal->lastcols = terminal->lastrows = 0;
	sizeScreenBuffer (terminal, 0, 0);
}


/* Terminalfenster eine Zeile hinaufscrollen
 */
static void scroll_up (TermInfo *terminal)
{
	word y;

	/* eine Zeile im Buffer einfÅgen
	 */
	for (y = 1; y < terminal->rows; y++)
	{
		memcpy (ti_sline (terminal, y-1), ti_sline (terminal, y), terminal->columns);
		memcpy (ti_aline (terminal, y-1), ti_aline (terminal, y), terminal->columns);
	}

	/* letzte Bufferzeile lîschen */
	memset (ti_sline (terminal, terminal->rows - 1), ' ', terminal->columns);
	memset (ti_aline (terminal, terminal->rows - 1), V_NORMAL, terminal->columns);
	
	/* Fensterinhalt hochschieben */
	TM_DelLine (terminal, 0);
}

/* Terminalfenster um eine Zeilen abwÑrtsscrollen
 */
static void scroll_down (TermInfo *terminal)
{
	word y;

	/* eine Zeile im Buffer einfÅgen
	 */
	for (y = terminal->rows - 1; y >= 0; y--)
	{
		memcpy (ti_sline (terminal, y), ti_sline (terminal, y - 1), terminal->columns);
		memcpy (ti_aline (terminal, y), ti_aline (terminal, y - 1), terminal->columns);
	}
	 
	/* erste Bufferzeile lîschen */
	memset (ti_sline (terminal, 0), ' ', terminal->columns);
	memset (ti_aline (terminal, 0), V_NORMAL, terminal->columns);
	
	/* Fensterinhalt runterschieben */
	TM_InsLine (terminal, 0);
}


/* VT-52: terminal->wrap-Around einschalten
 */
void wrap_on (TermInfo *terminal)
{
	terminal->wrap = TRUE;
}

/* VT-52: terminal->wrap-Around ausschalten */
void wrap_off (TermInfo *terminal)
{
	terminal->wrap = FALSE;
}

/* VT-52: Textcursor logisch einschalten */
void show_cursor (TermInfo *terminal)
{
	terminal->cur_on = TRUE;
}
	

/* VT-52: Textcursor logisch ausschalten */
void hide_cursor (TermInfo *terminal)
{
	terminal->cur_on = FALSE;
}

	
void clear_screen (TermInfo *terminal)
{
	DelScreenBuffer (terminal);
	TM_YErase (terminal, 0, terminal->rows - 1);
}



/* VT-52 Funktion: Carriage Return */
void vt_cr (TermInfo *terminal)
{
	(*terminal->p_cur_x) = 0;
}

	
/* VT-52 Funktion: Line Feed */
void vt_lf (TermInfo *terminal)
{
	if ((*terminal->p_cur_y) < terminal->rows - 1)
		(*terminal->p_cur_y)++;
	else
		scroll_up (terminal);
}


void vt_c_right (TermInfo *terminal)
{
	if ((*terminal->p_cur_x) < terminal->columns - 1)
		(*terminal->p_cur_x)++;
} 


void vt_c_left (TermInfo *terminal)
{
	if ((*terminal->p_cur_x) > 0)
		(*terminal->p_cur_x)--;
} 


void vt_c_up (TermInfo *terminal)
{
	if ((*terminal->p_cur_y) > 0)
		(*terminal->p_cur_y)--;
}
 
	
void vt_c_down (TermInfo *terminal)
{
	if ((*terminal->p_cur_y) < terminal->rows - 1)
		(*terminal->p_cur_y)++;
}


void vt_c_home (TermInfo *terminal)
{
	(*terminal->p_cur_x) = 0;
	(*terminal->p_cur_y) = 0;
}


void vt_cl_home (TermInfo *terminal)
{
	vt_c_home (terminal);
	clear_screen (terminal);
}
	
	
void vt_c_set (TermInfo *terminal, int y, int x)
{
	(*terminal->p_cur_x) = x - ' ';
	(*terminal->p_cur_y) = y - ' ';

	/* prÅfen, ob Koordinaten gÅltig, andernfalls korrigieren */
	if ((*terminal->p_cur_x) < 0)
		(*terminal->p_cur_x) = 0;
	else if ((*terminal->p_cur_x) > terminal->columns - 1)
		(*terminal->p_cur_x) = terminal->columns - 1;

	if ((*terminal->p_cur_y) < 0)
		(*terminal->p_cur_y) = 0;
	else if ((*terminal->p_cur_y) > terminal->rows - 1)
		(*terminal->p_cur_y) = terminal->rows - 1;
}


void vt_cui (TermInfo *terminal)
{
	if ((*terminal->p_cur_y) > 0)
		(*terminal->p_cur_y)--;
	else
		scroll_down (terminal);
}


void vt_c_save (TermInfo *terminal)
{
	(*terminal->p_x_saved) = (*terminal->p_cur_x);
	(*terminal->p_y_saved) = (*terminal->p_cur_y);
}
	
	
void vt_c_restore (TermInfo *terminal)
{
	(*terminal->p_cur_x) = (*terminal->p_x_saved);
	(*terminal->p_cur_y) = (*terminal->p_y_saved);
}


/* VT-52 Funktion: vor der Cursorzeile eine Leerzeile einfÅgen */
void vt_ins_line (TermInfo *terminal)
{
	word y;
	
	/* Bufferausschnitt verschieben */
	for (y = terminal->rows - 1; y > (*terminal->p_cur_y); y--)
	{
		memcpy (ti_sline (terminal, y), ti_sline (terminal, y-1), terminal->columns);
		memcpy (ti_aline (terminal, y), ti_aline (terminal, y-1), terminal->columns);
	}
	
	/* Bufferzeile lîschen */
	memset (ti_sline (terminal, *terminal->p_cur_y), ' ', terminal->columns);
	memset (ti_aline (terminal, *terminal->p_cur_y), V_NORMAL, terminal->columns);
	
	TM_InsLine (terminal, (*terminal->p_cur_y));
	 	
	(*terminal->p_cur_x) = 0;
}
	
	
/* VT-52 Funktion: die aktuelle Zeile entfernen */
void vt_del_line (TermInfo *terminal)
{
	word y;
	
	/* Bufferausschnitt verschieben */
	for (y = (*terminal->p_cur_y); y < terminal->rows - 1; y++)
	{
		memcpy (ti_sline (terminal, y), ti_sline (terminal, y+1), terminal->columns);
		memcpy (ti_aline (terminal, y), ti_aline (terminal, y+1), terminal->columns);
	}

	/* Bufferzeile lîschen */	
	memset (ti_sline (terminal, terminal->rows-1), ' ', terminal->columns);
	memset (ti_aline (terminal, terminal->rows-1), V_NORMAL, terminal->columns);
	
	TM_DelLine (terminal, (*terminal->p_cur_y));
	
	(*terminal->p_cur_x) = 0;
}
	

/* VT-52-Funktion: aktuelle Zeile lîschen */
void vt_erline (TermInfo *terminal)
{
	memset (ti_sline (terminal, *terminal->p_cur_y), ' ', terminal->columns);
	memset (ti_aline (terminal, *terminal->p_cur_y), V_NORMAL, terminal->columns);

	TM_XErase (terminal, (*terminal->p_cur_y), 0, terminal->columns);
	
	(*terminal->p_cur_x) = 0;
}

	
/* VT-52-Funktion: erase to line start */
void vt_erlst (TermInfo *terminal)
{
	if ((*terminal->p_cur_x) == 0) return;

	memset (ti_sline (terminal, *terminal->p_cur_y), ' ', *terminal->p_cur_x);
	memset (ti_aline (terminal, *terminal->p_cur_y), V_NORMAL, *terminal->p_cur_x);

	TM_XErase (terminal, (*terminal->p_cur_y), 0, (*terminal->p_cur_x));
}


/* VT-52 Funktion: bis zum Ende der Zeile lîschen */
void vt_cleol (TermInfo *terminal)
{
	memset (ti_sline (terminal, *terminal->p_cur_y) + *terminal->p_cur_x,
		' ', terminal->columns - *terminal->p_cur_x);
	memset (ti_aline (terminal, *terminal->p_cur_y) + *terminal->p_cur_x,
		V_NORMAL, terminal->columns - *terminal->p_cur_x);
		
	TM_XErase (terminal, (*terminal->p_cur_y), (*terminal->p_cur_x), 
		terminal->columns);
}


/* VT-52 Funktion: bis zum Ende der Seite lîschen */
void vt_ereop (TermInfo *terminal)
{
	word y;

	vt_cleol (terminal);
	if ((*terminal->p_cur_y) == terminal->rows - 1) return;

	for (y = (*terminal->p_cur_y) + 1; y < terminal->rows; y++)
	{
		memset (ti_sline (terminal, y), ' ', terminal->columns);
		memset (ti_aline (terminal, y), V_NORMAL, terminal->columns);
	}
	TM_YErase (terminal, (*terminal->p_cur_y) + 1, terminal->rows - 1);
}


/* VT-52 Funktion: bis zum Anfang der Seite lîschen */
void vt_ersop (TermInfo *terminal)
{
	word y;
	
	vt_erlst (terminal);	/* bis zum Anfang der Zeile lîschen */
	if ((*terminal->p_cur_y) == 0) return;
	
	for (y = 0; y < (*terminal->p_cur_y); y++)
	{
		memset (ti_sline (terminal, y), ' ', terminal->columns);
		memset (ti_aline (terminal, y), V_NORMAL, terminal->columns);
	}
	TM_YErase (terminal, 0, (*terminal->p_cur_y) - 1);
}


/* forward declaration */
void disp_char (TermInfo *terminal, char ch);

/* Tabulator ausgeben */
void tabulate (TermInfo *terminal)
{
	word new_x,i;
	
	if (terminal->tab_destructive)
	{
		new_x = ((*terminal->p_cur_x) / terminal->tab_size)
				 * terminal->tab_size + terminal->tab_size;
		if (new_x >= terminal->columns)
			new_x = terminal->columns - 1;

		for (i = 0; i < (new_x - (*terminal->p_cur_x)); i++)
		{
			disp_char (terminal, ' ');
		}
	}
	else
	{
		new_x = ((*terminal->p_cur_x) / terminal->tab_size)
			 * terminal->tab_size + terminal->tab_size;

		if (new_x >= terminal->columns)
			new_x = terminal->columns - 1;
	}
		
	(*terminal->p_cur_x) = new_x;
}

	
/* Backspace ausgeben */
void t_backspace (TermInfo *terminal)
{
	vt_c_left (terminal);

	if (terminal->back_destructive)
	{
		disp_char (terminal, ' ');
		vt_c_left (terminal);
	}
}


/* eine Zeile ausgeben */
static void _disp_str (TermInfo *terminal, char *str)
{
	size_t len = strlen (str);
	
	/* Zeichen mit Attribut speichern */
	memcpy (ti_sline (terminal, *terminal->p_cur_y) + *terminal->p_cur_x,
		str, len);
	memset (ti_aline (terminal, *terminal->p_cur_y) + *terminal->p_cur_x,
		terminal->wr_mode, len);

	/* Zeichen ausgeben */
	TM_DispString (terminal, str);

	/* Cursor neu positionieren */
	if ((*terminal->p_cur_x) + len < terminal->columns)
		(*terminal->p_cur_x) += (int) len;
	else
	{
		if (terminal->wrap == 1)
		{
			(*terminal->p_cur_x) = terminal->columns - (*terminal->p_cur_x) - (int) len;
			if ((*terminal->p_cur_y) < terminal->rows-1)
				(*terminal->p_cur_y)++;
			else
				scroll_up (terminal);
		}
		else
			(*terminal->p_cur_x) = terminal->columns - 1;
	}
}

void disp_string (TermInfo *terminal, char *str)
{
	size_t this_line;
	word done = FALSE;
	char save_ch;

	rem_cur (terminal);

	do
	{
		this_line  = strlen (str);
		if (this_line + (*terminal->p_cur_x) <= terminal->columns)
		{
			_disp_str (terminal, str);
			done = TRUE;
		}
		else
		{
			this_line = terminal->columns - (*terminal->p_cur_x);
			save_ch = str[this_line];
			str[this_line] = '\0';
			_disp_str (terminal, str);
			str[this_line] = save_ch;
			str += this_line;
		}
	} 
	while (!done);

	disp_cur (terminal);	
}

/* ein Zeichen ausgeben */
void disp_char (TermInfo *terminal, char ch)
{
	/* Zeichen mit Attribut speichern */
	ti_sline (terminal, *terminal->p_cur_y)[*terminal->p_cur_x] = ch;
	ti_aline (terminal, *terminal->p_cur_y)[*terminal->p_cur_x] = terminal->wr_mode;

	/* Zeichen ausgeben */
	TM_DispChar (terminal, ch);

	/* Cursor neu positionieren */
	if ((*terminal->p_cur_x) < terminal->columns)
		(*terminal->p_cur_x)++;
	if ((*terminal->p_cur_x) == terminal->columns)
	{
		if (terminal->wrap)
		{
			(*terminal->p_cur_x) = 0;
			if ((*terminal->p_cur_y) < terminal->rows - 1)
				(*terminal->p_cur_y)++;
			else
				scroll_up (terminal);
		}
		else
			(*terminal->p_cur_x) = terminal->columns - 1;
	}
}

/*
 * Front-ends for canonical and raw console output
 * (_o_con and _o_rawcon point to these routines)
 */
void disp_canchar (TermInfo *terminal, char ch)
{
	terminal->term_out (terminal, ch);
}

void disp_rawchar (TermInfo *terminal, char ch)
{
	disp_char (terminal, ch);
	disp_cur (terminal);
}

static void _level3 (TermInfo *terminal, char chr)
{
	rem_cur (terminal);
	vt_c_set (terminal, terminal->last_chr, (unsigned char)chr);
	disp_cur (terminal);
	terminal->term_out = _level0;
}

static void _level2 (TermInfo *terminal, char chr)
{
	terminal->last_chr = (unsigned char)chr;
	terminal->term_out = _level3;
}

static void setForeColor (TermInfo *terminal, char chr)
{
	TM_ForeColor (terminal, chr);
	terminal->term_out = _level0;
}

static void setBackColor (TermInfo *terminal, char chr)
{
	TM_BackColor (terminal, chr);
	terminal->term_out = _level0;
}

static void setFontBits (TermInfo *terminal, char chr)
{
	if (chr & 1) terminal->wr_mode |= V_BOLD;
	if (chr & 2) terminal->wr_mode |= V_LIGHT;
	if (chr & 4) terminal->wr_mode |= V_ITALIC;
	if (chr & 8) terminal->wr_mode |= V_UNDERLINE;
	if (chr & 16) terminal->wr_mode |= V_INVERTED;
	terminal->term_out = _level0;
}

static void unsetFontBits (TermInfo *terminal, char chr)
{
	if (chr & 1) terminal->wr_mode &= ~V_BOLD;
	if (chr & 2) terminal->wr_mode &= ~V_LIGHT;
	if (chr & 4) terminal->wr_mode &= ~V_ITALIC;
	if (chr & 8) terminal->wr_mode &= ~V_UNDERLINE;
	if (chr & 16) terminal->wr_mode &= ~V_INVERTED;
	terminal->term_out = _level0;
}

/* VT-52: Invers-Modus einschalten */
void rev_on (TermInfo *terminal)
{
	terminal->wr_mode |= V_STANDOUT;
}
 
/* VT-52: Invers-Modus ausschalten */
void rev_off (TermInfo *terminal)
{
	terminal->wr_mode &= ~V_STANDOUT;
}


static void _level1 (TermInfo *terminal, char chr)
{
	switch (chr)
	{
		case 'b':	terminal->term_out = setForeColor;	return;
		case 'c':	terminal->term_out = setBackColor;	return;
		case 'd':	
			rem_cur (terminal);
			vt_ersop (terminal);
			disp_cur (terminal);
			break;
		case 'e':
			rem_cur (terminal);
			show_cursor (terminal);
			disp_cur (terminal);
			break;
		case 'f':
			rem_cur (terminal);
			hide_cursor (terminal);
			disp_cur (terminal);
			break;
		case 'j':
			vt_c_save (terminal);
			break;
		case 'k':
			rem_cur (terminal);
			vt_c_restore (terminal);
			disp_cur (terminal);
			break;
		case 'l':
			vt_erline (terminal);
			disp_cur (terminal);
			break; 
		case 'o':
			rem_cur (terminal);
			vt_erlst (terminal);
			disp_cur (terminal);
			break;
		case 'p':
			rev_on (terminal);
			break;
		case 'q':
			rev_off (terminal);
			break;
		case 'v':
			wrap_on (terminal);
			break;
		case 'w':
			wrap_off (terminal);
			break; 
		case 'A':
			rem_cur (terminal);
			vt_c_up (terminal);
			disp_cur (terminal);
			break;
		case 'B':
			rem_cur (terminal);
			vt_c_down (terminal);
			disp_cur (terminal);
			break;
		case 'C':
			rem_cur (terminal);
			vt_c_right (terminal);
			disp_cur (terminal);
			break;
		case 'D':
			rem_cur (terminal);
			vt_c_left (terminal);
			disp_cur (terminal);
			break;
		case 'E':
			vt_cl_home (terminal);
			disp_cur (terminal);
			break;
		case 'H':
			rem_cur (terminal);
			vt_c_home (terminal);
			disp_cur (terminal);
			break;
		case 'I':
			rem_cur (terminal);
			vt_cui (terminal);
			disp_cur (terminal);
			break;
		case 'J':
			rem_cur (terminal);
			vt_ereop (terminal);
			disp_cur (terminal);
			break;
		case 'K':
			rem_cur (terminal);
			vt_cleol (terminal);
			disp_cur (terminal);
			break;
		case 'L':
			rem_cur (terminal);
			vt_ins_line (terminal);
			disp_cur (terminal);
			break;
		case 'M':
			rem_cur (terminal);
			vt_del_line (terminal);
			disp_cur (terminal);
			break;
		case 'Y':
			terminal->term_out = _level2;
			return;
		
		/* jr: MiniWin-Erweiterungen */
		
		case 'y':	terminal->term_out = setFontBits;	return;
		case 'z':	terminal->term_out = unsetFontBits;	return;
	}
	
	terminal->term_out = _level0;
}

static unsigned char belldata[] = 
{
	0,0x34,1,0,2,0,3,0,4,0,5,0,6,0,
	7,0xfe,8,16,9,0,10,0,11,0,12,16,
	13,9,0xff,0
};


static long doBellHook (void)
{
	long *bell_hook = (void *)0x5ACL;
	
	if (*bell_hook != 0L)
	{
		((void (*)(void))(*bell_hook)) ();
		return 1L;
	}
	else
		return 0L;
}

static void JingleBells (void)
{
	if (!Supexec (doBellHook)) Dosound (belldata);
}

static void _level0 (TermInfo *terminal, char chr)
{
	switch (chr)
	{
		case ESC: terminal->term_out = _level1;
			break;

	   	case TAB: 
	   		rem_cur (terminal);
	   		tabulate (terminal);		
			disp_cur (terminal);
	   		break;

		case CR: 
			rem_cur (terminal);
			vt_cr (terminal);		
			disp_cur (terminal);
			break;

		case LF: 
			rem_cur (terminal);
			vt_lf (terminal);		
			disp_cur (terminal);
			break;

		case BEL:
			JingleBells ();	
			break;

		case BS:
			rem_cur (terminal);
			t_backspace (terminal);	
			disp_cur (terminal);
			break;

		case '\0':
			break;
			
		default:
			disp_char (terminal, chr); 
			disp_cur (terminal);
			break;
	}
}

char escon (TermInfo *terminal)
{
	return (terminal->term_out != _level0);
}

void ConsoleDispCanChar (char ch)
{
	disp_canchar (&BiosTerminal, ch);
}

void ConsoleDispRawChar (char ch)
{
	disp_rawchar (&BiosTerminal, ch);
}

void ConsoleDispString (char *str)
{
	disp_string (&BiosTerminal, str);
}


#include "..\common\xbra.h"
#include "..\mupfel\sysvars.h"

extern XBRA conoutXBRA, rconoutXBRA, MyXBiosXBRA, MyXBios30XBRA;
extern XBRA costatXBRA, rcostatXBRA;

typedef struct
     {
     long magic;                   /* muû $87654321 sein              */
     void *membot;                 /* Ende der AES- Variablen         */
     void *aes_start;              /* Startadresse                    */
     long magic2;                  /* ist 'MAGX'                      */
     long date;                    /* Erstelldatum ttmmjjjj           */
     void (*chgres)(int res, int txt);  /* Auflîsung Ñndern           */
     long (**shel_vector)(void);   /* residentes Desktop              */
     char *aes_bootdrv;            /* von hieraus wurde gebootet      */
     int  *vdi_device;             /* vom AES benutzter VDI-Treiber   */
     void *reservd1;
     void *reservd2;
     void *reservd3;
     int  version;                 /* z.B. $0201 ist V2.1             */
     int  release;                 /* 0=alpha..3=release              */
     } AESVARS;

/* Cookie MagX --> */

typedef struct
{
	long    config_status;
	void 	*dosvars;
	AESVARS *aesvars;
	void	*bla, *bla2;
 	long	status_bits;    
} MAGX_COOKIE;


static void
init_magic_taskman_var (void)
{
	extern long *PIsTaskman;
	MAGX_COOKIE *mcp;
	
	if (CookiePresent ('MagX', (long *)&mcp)
		&& MagiCVersion >= 0x350) 
		PIsTaskman = &mcp->status_bits;
}



int ConsoleIsOpen;

static long getcputype (void)
{
	return (long)(*_CPUTYPE);
}

void InitBiosOutput (void)
{
	int cpu_type;

	if (ConsoleIsOpen) return;

	init_magic_taskman_var ();
	
	InstallXBRA (&conoutXBRA, _O_CON);
	InstallXBRA (&rconoutXBRA, _O_RAWCON);
	InstallXBRA (&costatXBRA, _OS_CON);
	InstallXBRA (&rcostatXBRA, _OS_RAWCON);

	cpu_type = (word)Supexec (getcputype);
	InstallXBRA (cpu_type? &MyXBios30XBRA : &MyXBiosXBRA, XBIOS);

	/* Zeiger in der Terminal-Struktur auf Line-A-Variablen
	umbiegen */

	LineAInit ();
	*LineACelMX = BiosTerminal.columns - 1;
	*LineACelMY = BiosTerminal.rows - 1;
	*LineACurX = BiosTerminal._cur_x;
	*LineACurY = BiosTerminal._cur_y;
	*LineASavX = BiosTerminal._x_saved;
	*LineASavY = BiosTerminal._y_saved;
	BiosTerminal.p_cur_x = LineACurX;
	BiosTerminal.p_cur_y = LineACurY;
	BiosTerminal.p_x_saved = LineASavX;
	BiosTerminal.p_y_saved = LineASavY;
	
	ConsoleIsOpen = 1;
}

void ExitBiosOutput (void)
{
	int cpu_type;

	if (!ConsoleIsOpen)
		return;
	
	/* Zeiger in der Terminal-Struktur wieder auf lokale Werte
	zurÅckbiegen */
	
	BiosTerminal._cur_x = *LineACurX;
	BiosTerminal._cur_y = *LineACurY;
	BiosTerminal._x_saved = *LineASavX;
	BiosTerminal._y_saved = *LineASavY;
	BiosTerminal.p_cur_x = &BiosTerminal._cur_x;
	BiosTerminal.p_cur_y = &BiosTerminal._cur_y;
	BiosTerminal.p_x_saved = &BiosTerminal._x_saved;
	BiosTerminal.p_y_saved = &BiosTerminal._y_saved;
	LineAExit ();
	
	RemoveXBRA (&conoutXBRA, _O_CON);
	RemoveXBRA (&rconoutXBRA, _O_RAWCON);
	RemoveXBRA (&costatXBRA, _OS_CON);
	RemoveXBRA (&rcostatXBRA, _OS_RAWCON);

	cpu_type = (word)Supexec (getcputype);
	RemoveXBRA (cpu_type? &MyXBios30XBRA : &MyXBiosXBRA, XBIOS);
	
	ConsoleIsOpen = 0;
}


