/*
 * @(#) Gemini\mvwindow.c
 * @(#) Stefan Eissing, 06. November 1994
 *
 * description: top layer of functions for mupfel window
 *
 * jr 15.6.95
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\fontsel.h>
#include <mint\ioctl.h>
#include <nls\nls.h>

#include "vs.h"
#include "fontbox.rh"

#include "window.h"
#include "venuserr.h"
#include "util.h"

#include "conwind.h"
#include "mvwindow.h"
#include "terminal.h"
#include "termio.h"
#include "wintool.h"
#include "filewind.h"
#include "filedraw.h"
#include "fileutil.h"
#include "message.h"
#include "redraw.h"
#include "stand.h"
#if MERGED
#include "..\mupfel\mupfel.h"
#endif


/*
 * externals
 */
extern OBJECT *pfontbox;


/* internal texts
 */
#define NlsLocalSection "G.mvwindow"
enum NlsLocalText{
T_COLUMNS,		/*Die Anzahl der Spalten muž gr”žer als 1 sein!*/
T_ROWS,			/*Die Anzahl der Zeilen muž positiv sein!*/
T_TEST,			/*The quick brown fox jumps over the lazy dog.*/
T_WRONGFONT,	/*Der gewnschte Font fr das Consolefenster konnte 
nicht eingestellt werden, %s!*/
T_NOTINST,		/*da er nicht installiert ist*/
T_NOGDOS,		/*da kein GDOS installiert ist*/
};

/* jr: TERMCAP-Kram setzen */

#define TERMCAP_STRING	\
	"TERMCAP=vt52:"		/* Name und alternativer Name */ \
	"al=\\EL:"			/* al = add line */ \
	"am:"				/* am = automatic margins */ \
	"bl=^G:"			/* bl = bell */ \
	"bs:"				/* bs = backspacing */ \
	"cd=\\EJ:"			/* cd = clear to end of display */ \
	"ce=\\EK:"			/* ce = clear to end of line */ \
	"cl=\\EE:"			/* cl = clear screen and home cursor */ \
	"cm=\\EY%+ %+ :" 	/* cm = cursor motion */ \
/*	"co#%02d:"			co = columns */ \
	"dl=\\EM:"			/* dl = delete line */ \
	"do=\\EB:"			/* do = down one line */ \
	"ho=\\EH:"			/* ho = home cursor */ \
	"it#8:"				/* it = initial TAB position */ \
	"le=^H:"			/* le = move left one space */ \
/*	"li#%02d:"			li = lines */ \
	"md=\\EyA:"			/* md = turn on bold mode */ \
	"me=\\Ez_:"			/* me = turn off all attributes */ \
	"mh=\\EyB:"			/* mh = turn on half bright mode */ \
	"mr=\\EyP:"			/* mr = turn on reverse video mode */ \
	"ms:"				/* ms = save to move in standout modes */ \
	"nd=\\EC:"			/* nd = non destructive space */ \
	"pt:" \
	"rc=\\Ek:"			/* rc = restore cursor position */ \
	"sc=\\Ej:"			/* sc = save cursor position */ \
	"se=\\Eq:"			/* se = end standout mode */ \
	"so=\\Ep:"			/* so = begin standout mode */ \
	"sr=\\EI:" \
	"ta=^I:"			/* ta = move to next TAB stop */ \
	"ti=\\Eq\\Ev\\Ee\\Ez_:"	/* ti = terminal init */ \
	"ue=\\EzH:"			/* ue = end underscore mode */ \
	"us=\\EyH:"			/* us = start underscore mode */ \
	"up=\\EA:"			/* up = cursor up */ \
	"ve=\\Ee:"			/* ve = make cursor appear normal */ \
	"vi=\\Ef:"			/* vi = make cursor invisible */

/* internals
 */

/* FONTWORK fr die beiden Workstation des Console-Fensters
 */
FONTWORK stdWork;

void GetConsoleFont (word *id, word *points)
{
	*id = BiosTerminal.font_id;
	*points = BiosTerminal.font_size;
}

void SetInMWindow (SHOWINFO *ps, int give_warning)
{
	if (ps->m_inv) BiosTerminal.inv_type = ps->m_inv;
	BiosTerminal.font_id = ps->m_font;
	BiosTerminal.font_size = ps->m_fsize;

	if ((!SetFont (&stdWork, &BiosTerminal.font_id,
		 &BiosTerminal.font_size, &BiosTerminal.font_width, &BiosTerminal.font_height))
		 && give_warning)
	{
		venusErr (NlsStr (T_WRONGFONT),
			stdWork.addfonts > 0 ?
			NlsStr (T_NOTINST) : NlsStr (T_NOGDOS));
	}
	
	BiosTerminal.work.g_x = ps->m_wx;
	BiosTerminal.work.g_y = ps->m_wy;
	BiosTerminal.work.g_w = BiosTerminal.font_width * BiosTerminal.columns;
	BiosTerminal.work.g_h = BiosTerminal.font_height * BiosTerminal.rows;

	ReInitTerminal (&BiosTerminal, ps->m_cols, ps->m_rows);
	SetMWSize (&BiosTerminal);
}


void ExecConsoleInfo (char *line)
{
	char *tp;

	strtok (line, "@\n");

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_cols = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_rows = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_inv = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_font = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_fsize = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_wx = atoi (tp);

	if ((tp = strtok (NULL, "@\n")) != NULL)
		ShowInfo.m_wy = atoi (tp);

	SetInMWindow (&ShowInfo, TRUE);
}


void getInMWindow (SHOWINFO *ps)
{
	ps->m_cols = BiosTerminal.columns;
	ps->m_rows = BiosTerminal.rows;
	ps->m_inv = BiosTerminal.inv_type;
	ps->m_font = BiosTerminal.font_id;
	ps->m_fsize = BiosTerminal.font_size;
	ps->m_wx = BiosTerminal.work.g_x;
	ps->m_wy = BiosTerminal.work.g_y;
}


int WriteConsoleInfo (MEMFILEINFO *mp, char *buffer)
{
	getInMWindow (&ShowInfo);
	sprintf (buffer, "#C@%d@%d@%d@%d@%d@%d@%d", 
			ShowInfo.m_cols, ShowInfo.m_rows, ShowInfo.m_inv, 
			ShowInfo.m_font, ShowInfo.m_fsize, ShowInfo.m_wx, 
			ShowInfo.m_wy);
		
	return !mputs (mp, buffer);
}


word ConsoleInit (void)
{
	if (InitTerminal (TRUE, &stdWork.sysfonts) == NULL)
		return FALSE;
	
	InitBiosOutput ();
	
	BiosTerminal.font_id = 1;
	BiosTerminal.font_size = 8;
	stdWork.handle = std_handle;
	stdWork.loaded = FALSE;
	stdWork.addfonts = 0;
	stdWork.list = NULL;

	FontLoad (&stdWork);
	SetFont (&stdWork, &BiosTerminal.font_id, &BiosTerminal.font_size,
		&BiosTerminal.font_width, &BiosTerminal.font_height);

	BiosTerminal.work.g_x = BiosTerminal.work.g_y = BiosTerminal.work.g_w = 
	BiosTerminal.work.g_h = 0;

	return TRUE;
}

void ConsoleExit (void)
{
	FontUnLoad (&stdWork);
	ExitTerminal (&BiosTerminal);

	ExitBiosOutput ();
}


/*
 * Set Mupfel's idea of how large the window is
 */
void SetMWSize (TermInfo *terminal)
{
	if (terminal->flags.is_console)
	{
		SetRowsAndColumns (MupfelInfo, terminal->rows, terminal->columns);

		/* TERM und TERMCAP genau dann setzen, wenn sie noch
		nicht gesetzt sind */

		if (!GetEnv (MupfelInfo, "TERM"))
			PutEnv (MupfelInfo, "TERM=vt52");
			
		if (!GetEnv (MupfelInfo, "TERMCAP"))
			PutEnv (MupfelInfo, TERMCAP_STRING);
	}
	else
	{
		struct winsize tw;

		tw.ws_xpixel = tw.ws_ypixel = 0;
		tw.ws_row = terminal->rows;
		tw.ws_col = terminal->columns;
		Fcntl (terminal->pty, (long)&tw, TIOCSWINSZ);
	}
}


void SizeOfMWindow (TermInfo *terminal, GRECT *wind)
{
	word workx, worky, workw, workh;
	
	if (terminal->work.g_x && terminal->work.g_y)
	{
		workx = terminal->work.g_x;
		worky = terminal->work.g_y;
	}
	else
	{
		wind_calc (WC_WORK, CONWIND, 
			wind->g_x, wind->g_y, wind->g_w, wind->g_h,
			&workx, &worky, &workw, &workh);
	}

	terminal->work.g_w = workw = terminal->font_width * 
		terminal->columns;
	terminal->work.g_h = workh = terminal->font_height * 
		terminal->rows;

	wind_calc (WC_BORDER, CONWIND, workx, worky, workw, workh,
		&wind->g_x, &wind->g_y, &wind->g_w, &wind->g_h);
}

static void fontRedrawWindow (WindInfo *wp, TermInfo *terminal,
	word new_columns, word new_rows)
{
	word messbuff[8];

	wp->work.g_w = terminal->font_width * new_columns;
	wp->work.g_h = terminal->font_height * new_rows;
	wind_calc (WC_BORDER, CONWIND,
		wp->work.g_x, wp->work.g_y, wp->work.g_w, wp->work.g_h,
		&wp->wind.g_x, &wp->wind.g_y, &wp->wind.g_w, &wp->wind.g_h);
		
	WindowSize (wp->handle, &wp->wind);
			
	messbuff[0] = WM_REDRAW;
	messbuff[1] = apid;
	messbuff[2] = 0;
	messbuff[3] = wp->handle;
	messbuff[4] = wp->work.g_x;
	messbuff[5] = wp->work.g_y;
	messbuff[6] = wp->work.g_w;
	messbuff[7] = wp->work.g_h;
	appl_write (wp->owner, 16, messbuff);
}


int TerminalDarstellung (WindInfo *wp, int only_asking)
{
	TermInfo *terminal;
	DIALINFO d;
	FONTSELINFO F;
	char *pftrows,*pftcols;
	word retcode,selinv;
	word id, x, y, size, clicks;
	word draw, item;
	char changed = FALSE;
	int edit_object = FTBGBOX;

	if (wp->kind != WK_CONSOLE && wp->kind != WK_TERMINAL)
		return FALSE;

	if (only_asking) return TRUE;
		
	terminal = ((ConsoleWindow_t *)wp->kind_info)->terminal;
	
	pftrows = pfontbox[FTROWS].ob_spec.tedinfo->te_ptext;
	pftcols = pfontbox[FTCOLS].ob_spec.tedinfo->te_ptext;
	sprintf (pftrows, "%3d", terminal->rows);
	sprintf (pftcols, "%3d", terminal->columns);

	setSelected (pfontbox, FTINV, terminal->inv_type == V_STANDOUT);
	setSelected (pfontbox, FTUNDER, terminal->inv_type == V_UNDERLINE);
	setSelected (pfontbox, FTBOLD, terminal->inv_type == V_BOLD);

	DialCenter(pfontbox);

	id = terminal->font_id;
	size = terminal->font_size;
	
	GrafMouse(HOURGLASS, NULL);
	FontGetList(&stdWork, TRUE, FALSE);

	/* Setze std_handle auf keine Text-Effekte
	 */
	SetStdHandleEffect (0);
	FontSelInit(&F, &stdWork, pfontbox, FTBOX, FTBGBOX, FTPBOX,
		FTPBGBOX, FTST, FTSTB, FTFONT, (char *)NlsStr(T_TEST), 
		0, &id, &size);
	GrafMouse(ARROW, NULL);
	DialStart(pfontbox,&d);
	DialDraw(&d);
	FontSelDraw(&F, id, size);
	
	do
	{
		word key;
	
		draw = TRUE;
		item = -1;
		retcode = DialXDo (&d, &edit_object, NULL, &key, NULL, NULL, NULL);
		
		clicks = (retcode&0x8000)? 2 : 1;
		retcode &= 0x7FFF;
		
		/* jr: Support fr tastaturbediente Liste */
		
		if (retcode == FTBOX || retcode == FTBGBOX ||
			retcode == FTST || retcode == FTSTB ||
			retcode == FTPBOX || retcode == FTPBGBOX)
		{
			if ((key & 0xff00) == 0x4800)
				clicks = FONT_CL_UP;
			else if ((key & 0xff00) == 0x5000)
				clicks = FONT_CL_DOWN;
			else if (key != 0)
				retcode = -1;
		}
		
		if (retcode == FTBOX || retcode == FTBGBOX)
		{
			edit_object = FTBGBOX;
			item = FontClFont (&F, clicks, &id, &size);
		}
		else if (retcode == FTPBOX || retcode == FTPBGBOX)
		{
			edit_object = FTPBGBOX;
			item = FontClSize (&F, clicks, &id, &size);
		}
		else if (retcode == FTST || retcode == FTSTB)
		{
			edit_object = FTSTB;
			item = FontClForm (&F, clicks, &id, &size);
		}
		
		if ((item >= 0) && (clicks == 2))
		{
			retcode = FTOK;
			draw = FALSE;
		}
		
		if (retcode == FTOK)
		{
			y = atoi (pftrows);
			x = atoi (pftcols);

			if (y < 1)
			{
				venusErr(NlsStr(T_ROWS));
				setSelected(pfontbox, retcode, FALSE);
				fulldraw(pfontbox, retcode);
				retcode = 0;
			}
			else if (x < 2)
			{
				venusErr(NlsStr(T_COLUMNS));
				setSelected(pfontbox, retcode, FALSE);
				fulldraw(pfontbox, retcode);
				retcode = 0;
			}
		}
	} while ((retcode != FTCANCEL) && (retcode != FTOK));

	if (draw)
	{
		setSelected(pfontbox, retcode, FALSE);
		fulldraw(pfontbox, retcode);
	}
	
	if (retcode == FTOK)
	{
		if (isSelected(pfontbox, FTINV))
			selinv = V_STANDOUT;
		else if (isSelected(pfontbox, FTUNDER))
			selinv = V_UNDERLINE;
		else
			selinv = V_BOLD;
		
		if ((id != terminal->font_id) || (size != terminal->font_size)
			|| (x != terminal->columns) || (y != terminal->rows)
			|| (selinv != terminal->inv_type))
		{
			terminal->inv_type = selinv;
			terminal->font_id = id;
			terminal->font_size = size;
			
			changed = TRUE;
		}
	}
	
	SetFont(&stdWork, &terminal->font_id, &terminal->font_size,
		&terminal->font_width, &terminal->font_height);

	FontSelExit(&F);
	DialEnd(&d);

	if (changed)
	{
		fontRedrawWindow (wp, terminal, x, y);
		if (terminal == &BiosTerminal)
			FontChanged (TextFont.id, TextFont.size,
				terminal->font_id, terminal->font_size);
	}
	
	return changed;
}

