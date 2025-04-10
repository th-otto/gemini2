/*
 * @(#) Gemini\conwind.c
 * @(#) Stefan Eissing, 08. Februar 1994
 *
 * jr 22.4.95
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <aes.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\strerror.h"

#include "vs.h"
#include "menu.rh"

#include "window.h"
#include "conselec.h"
#include "conwind.h"
#include "clipbrd.h"
#include "draglogi.h"
#include "filewind.h"
#include "fileutil.h"
#include "message.h"
#include "mvwindow.h"
#include "myalloc.h"
#include "stand.h"
#include "terminal.h"
#include "termio.h"
#include "timeevnt.h"
#include "util.h"
#include "venuserr.h"
#include "windstak.h"
#include "wintool.h"

#include "..\mupfel\mupfel.h"

#if STANDALONE
#error conwind.c in standalone
#endif

/* Externals
 */
extern OBJECT *pmenu;


/*
 * internal texts 
 */
#define NlsLocalSection "G.conwind"
enum NlsLocalText{
T_NOMEM,		/*Es steht nicht genug Speicher zur Verfgung!*/
T_CONSOLE,		/* Console */
T_CONOPEN,		/*Kann das Console-Fenster nicht ”ffnen!*/
T_NOPROC,		/*Kann keinen neuen Prozež erzeugen!*/
T_TERMEXIT,		/*--beendet--*/
};


#define MAX_TERM	32

static long term_mask;
static WindInfo *term_windows[MAX_TERM];


static int terminalEcho (WindInfo *wp, const char *buffer, size_t len)
{
	ConsoleWindow_t *cwp = wp->kind_info;

	if (cwp->terminal->flags.is_console)
	{	
		PasteStringToConsole (buffer);
		return TRUE;
	}

	GrafMouse (M_OFF, NULL);	
	WT_BuildRectList (wp->handle);

	while (len--)
	{
		disp_canchar (cwp->terminal, *buffer);
		++buffer;
	}
	
	WT_BuildRectList (wp->handle);
	GrafMouse (M_ON, NULL);

	return TRUE;
}

#define TERMREADSIZE	256

static int termFeedKey (WindInfo *wp, word c, word kstate)
{
	ConsoleWindow_t *cwp = wp->kind_info;
	long code = (c & 0x00ff) | (((long)c & 0x0000ff00) << 8L) |
		 ((long)kstate << 24L);
	long r;
	char buf[TERMREADSIZE+1];
		
	if (cwp->terminal->pty)
	{
		r = Foutstat (cwp->terminal->pty);
		
		if (r <= 0)
		{
			long i = 0;
			
			while ((i < TERMREADSIZE) 
				&& ((r = Fread (cwp->terminal->pty, TERMREADSIZE, buf + i)) > 0))
			{
				i += r;
			}
			if (i)
				terminalEcho (wp, buf, i);

			Fselect(500,0L,0L,0L);
			r = Foutstat (cwp->terminal->pty);
		}
		
		if (r > 0)
		{
			Fputchar (cwp->terminal->pty, code, 0);
			return TRUE;
		}
	}
	
	return FALSE;
}

static void rebuildMask (long mask)
{
	WindInfo *wp;
	int i;
	
	for (i = 0; i < 32; ++i)
	{
		if ((mask & (1L << i)) && (Finstat (i) < 0))
		{
			/* Dieses Handle ist gestorben.
			 */
			term_mask &= ~(1L << i);
			wp = term_windows[i];
			if (wp != NULL && wp->kind == WK_TERMINAL)
			{
				ConsoleWindow_t *cwp = wp->kind_info;
				
				terminalEcho (wp, NlsStr (T_TERMEXIT),
					strlen (NlsStr (T_TERMEXIT)));
				Fclose (cwp->terminal->pty);
				cwp->terminal->pty = 0;
			}
		}
	}	
}

int updateRectList (long p)
{
	WindInfo *wp;
	
	(void)p;		
	wp = GetTopWindInfoOfKind (WK_CONSOLE);
	if (wp)
		WT_BuildRectList (wp->handle);
	
	return wp != NULL;
}


int CheckTerminals (long p)
{
	(void)p;
	
	if (term_mask)
	{
		long read_fds, checkdead, stat;
		int i, r;
		char buf[TERMREADSIZE+1];
		
		read_fds = term_mask;
		checkdead = 0L;
		
		if ((r = Fselect (1, &read_fds, 0L, 0L)) > 0)
		{
			for (i = 0; i < MAX_TERM; ++i)
			{
				ConsoleWindow_t *cwp;
				WindInfo *wp = term_windows[i];
				
				if (wp == NULL)
					continue;
				
				cwp = wp->kind_info;	

				if (read_fds & (1L << cwp->terminal->pty))
				{
					long r;
					stat = 0;
					
					while ((stat < TERMREADSIZE) 
						&& ((r = Fread (cwp->terminal->pty, TERMREADSIZE, buf + stat)) > 0))
					{
						stat += r;
					}
					if (stat)
						terminalEcho (wp, buf, stat);
					else
					{
						checkdead |= (1L << cwp->terminal->pty);
					}
				}
			}
		}

		if (checkdead)
			rebuildMask (checkdead);

		if (r == EIHNDL)
		{
			rebuildMask (term_mask);
		}
	}
	
	return (term_mask != 0);
}

#define TERM_CHECK_DELAY	20L
#define CONSOLE_UPDATE		2000L

static void registerTerminal (WindInfo *wp)
{
	ConsoleWindow_t *cwp = wp->kind_info;
	
	if (wp->kind == WK_CONSOLE)
	{
		InstallTimerEvent (CONSOLE_UPDATE, 0L, updateRectList);
		return;
	}
		
	if (term_mask == 0L)
	{
		/* Es war noch kein Terminal da.
		 */
		InstallTimerEvent (TERM_CHECK_DELAY, 0L, CheckTerminals);
	}
	
	term_mask |= (0x01L << cwp->terminal->pty);
	term_windows[cwp->terminal->pty] = wp;
}

static void unregisterTerminal (WindInfo *wp)
{
	ConsoleWindow_t *cwp = wp->kind_info;
	
	if (wp->kind == WK_CONSOLE)
	{
		RemoveTimerEvent (0L, updateRectList);
		return;
	}

	term_mask &= ~(0x01L << cwp->terminal->pty);
	term_windows[cwp->terminal->pty] = NULL;

	if (term_mask == 0L)
	{
		/* Es ist kein Terminal mehr da.
		 */
		RemoveTimerEvent (TERM_CHECK_DELAY, CheckTerminals);
	}
}

static void conWindowClosed (WindInfo *wp, word kstate)
{
	(void)kstate;
	WindowDestroy (wp->handle, TRUE, FALSE);
}


static void conWindowDestroyed (WindInfo *wp)
{
	ConsoleWindow_t *cwp = wp->kind_info;
	
	unregisterTerminal (wp);

	PushWindBox (&wp->wind, wp->kind);

	WT_ClearRectList (wp->handle);
	if (wp->kind == WK_TERMINAL)
		ExitTerminal (cwp->terminal);
	
	free (cwp);
}


static void conWindowUpdate (WindInfo *wp)
{
	ConsoleWindow_t *cwp = wp->kind_info;
	
	if (wp->update & NAME)
	{
		wind_set (wp->handle, WF_NAME, cwp->title);
	}
}


static void conWindowRedraw (WindInfo *wp, const GRECT *rect)
{
	ConsoleWindow_t *cwp = wp->kind_info;
	word req[4], draw[4], count;
	
	req[0] = rect->g_x;
	req[1] = rect->g_y;
	req[2] = rect->g_x + rect->g_w - 1;
	req[3] = rect->g_y + rect->g_h - 1;

	GrafMouse (M_OFF, NULL);	
	WT_BuildRectList (wp->handle);

	count = 0;
	while (WT_GetRect (count++, draw))
	{
		if (VDIRectIntersect (req, draw))
			TM_RedrawTerminal (cwp->terminal, draw);
	}
	
	WT_BuildRectList (wp->handle);
	GrafMouse (M_ON, NULL);
}


static void conWindowGetState (WindInfo *wp, char *buffer)
{
	ConsoleWindow_t *cwp = wp->kind_info;
	word wx, wy, ww, wh;

	wx = (word)scale123 (ScaleFactor, wp->wind.g_x - Desk.g_x, Desk.g_w);
	ww = (word)scale123 (ScaleFactor, wp->wind.g_w - Desk.g_x, Desk.g_w);
	wy = (word)scale123 (ScaleFactor, wp->wind.g_y - Desk.g_y, Desk.g_h);
	wh = (word)scale123 (ScaleFactor, wp->wind.g_h - Desk.g_y, Desk.g_h);

	sprintf (buffer,
			"#W@%d@%d@%d@%d@0@%d@%s@ @%s@",
			wx, wy, ww, wh, wp->kind, cwp->path, cwp->title);
}


static void conWindowTopped (WindInfo *wp)
{
	ConsoleWindow_t *cwp = wp->kind_info;
	
	cwp->terminal->window_handle = wp->handle;
	WT_BuildRectList (wp->handle);

	cwp->terminal->work = wp->work;
	SetMWSize (cwp->terminal);

	menu_ienable (pmenu, WINDCLOS, TRUE);
	menu_ienable (pmenu, CLOSE, TRUE);
}


static void conWindowCalcWindrect (WindInfo *wp, GRECT *wind)
{
	ConsoleWindow_t * cwp = wp->kind_info;
	GRECT work;
	int columns, rows;
	
	wind_calc (WC_WORK, CONWIND,
		wind->g_x, wind->g_y, wind->g_w, wind->g_h,
		&work.g_x, &work.g_y, &work.g_w, &work.g_h);

	columns = work.g_w / cwp->terminal->font_width;
	rows = work.g_h / cwp->terminal->font_height;

#if 0	
	if (columns > SCREEN_MAX_COLUMNS)
		columns = SCREEN_MAX_COLUMNS;
	
	if (rows > SCREEN_MAX_LINES)
		rows = SCREEN_MAX_LINES;
#endif
	
	work.g_w = columns * cwp->terminal->font_width;
	work.g_h = rows * cwp->terminal->font_height;
	
	wind_calc (WC_BORDER, CONWIND,
		work.g_x, work.g_y, work.g_w, work.g_h,
		&wind->g_x, &wind->g_y, &wind->g_w, &wind->g_h);
}


static void conWindowMoved (WindInfo *wp)
{
	ConsoleWindow_t * cwp = wp->kind_info;
	
	cwp->terminal->work = wp->work;
	WT_BuildRectList (wp->handle);
}


static void conWindowSized (WindInfo *wp)
{
	ConsoleWindow_t *cwp = wp->kind_info;
	int old_columns = cwp->terminal->columns;
	int old_rows = cwp->terminal->rows;
	int newcols, newlines;
	
	newcols = wp->work.g_w / cwp->terminal->font_width;
	newlines = wp->work.g_h / cwp->terminal->font_height;

#if 0	
	if (cwp->terminal->columns > SCREEN_MAX_COLUMNS)
		cwp->terminal->columns = SCREEN_MAX_COLUMNS;
		
	if (cwp->terminal->rows > SCREEN_MAX_LINES)
		cwp->terminal->rows = SCREEN_MAX_LINES;
#endif
		
	wp->work.g_w = newcols * cwp->terminal->font_width;
	wp->work.g_h = newlines * cwp->terminal->font_height;
		
	ReInitTerminal (cwp->terminal, newcols, newlines);
	SetMWSize (cwp->terminal);
	MupfelShouldDisplayPrompt = 
		cwp->terminal->flags.is_console &&
		((old_columns != cwp->terminal->columns)
			|| (old_rows > cwp->terminal->rows));
	
	conWindowMoved (wp);
}


#define PASTE_MAX	1024

static int conWindowPaste (WindInfo *wp, int only_asking)
{
	char *buffer;
	
	if (only_asking) return TRUE;
	
	buffer = malloc (PASTE_MAX + 1);
	if (buffer == NULL)
		return FALSE;
	
	(void)wp;
	if (GetClipText (buffer, PASTE_MAX))
	{
		buffer[PASTE_MAX] = '\0';
		PasteStringToConsole (buffer);
	}

	return TRUE;
}


static void conWindowMouseClick (WindInfo *wp, word clicks,
	word mx, word my, word kstate)
{
	if (!PointInGRECT (mx, my, &wp->work))
		return;
		
	if ((clicks == 2) && (kstate == K_ALT))
	{
		const char *pwd;
			
		pwd = GetEnv (MupfelInfo, "PWD");
		if (pwd && *pwd)
			FileWindowOpen (pwd, "", "", "*", 0);
	}
	else
	{
		WindUpdate (BEG_MCTRL);
		ConSelection (wp->handle, mx, my, kstate, (clicks != 1));
		WindUpdate (END_MCTRL);
	}
}


WindInfo *TerminalWindowOpen (const char *path, const char *title, 
	int is_console)
{
	WindInfo *wp;
	ConsoleWindow_t *cwp;
	GRECT wind;
	int update_position;
	
	cwp = calloc (1, sizeof (ConsoleWindow_t));
	if (cwp == NULL)
	{
		venusErr (NlsStr (T_NOMEM));
		return NULL;
	}

	if (is_console)
		cwp->terminal = &BiosTerminal;
	else
	{
		word dummy;
		
		cwp->terminal = InitTerminal (FALSE, &dummy);
	}
	
	if (cwp->terminal == NULL)
	{
		free (cwp);
		return NULL;
	}

	wind.g_x = Desk.g_x;
	wind.g_y = Desk.g_y;
	wind.g_w = cwp->terminal->font_width * cwp->terminal->columns;
	wind.g_h = cwp->terminal->font_height * cwp->terminal->rows;

	update_position =  !PopWindBox (&wind, 
		is_console? WK_CONSOLE : WK_TERMINAL);
	
	SizeOfMWindow (cwp->terminal, &wind);
	
	if (wind.g_x < Desk.g_x || wind.g_y < Desk.g_y
		|| wind.g_x >= (Desk.g_x + Desk.g_w)
		|| wind.g_y >= (Desk.g_y + Desk.g_h))
	{
		wind.g_x = Desk.g_x;
		wind.g_y = Desk.g_y;
	}
	
	strncpy (cwp->path, path, MAX_PATH_LEN);
	cwp->path[MAX_PATH_LEN - 1] = '\0';
		
	if (!strlen (title))				/* kein Titel da */
		strcpy (cwp->title, NlsStr (T_CONSOLE));
	else
	{
		strncpy (cwp->title, title, MAX_PATH_LEN);
		cwp->title[MAX_PATH_LEN - 1] = '\0';
	}
		
	/* Die g„ngigen Informationen sind gesetzt. Wir holen uns
	 * ein neue Fenster.
	 */
	
	wp = WindowCreate (CONWIND, -1);
	
	if (wp == NULL)
	{
		if (!is_console)
			ExitTerminal (cwp->terminal);
		free (cwp);
		return NULL;
	}
	
	if (is_console)
		wp->kind = WK_CONSOLE;
	else
		wp->kind = WK_TERMINAL;

	if (update_position)		
		wp->update |= WU_POSITION;
		
	wp->kind_info = cwp;
	wp->wind = wind;

	wp->window_calc_windrect = conWindowCalcWindrect;
	wp->window_topped = conWindowTopped;
	wp->window_moved = conWindowMoved;
	wp->window_sized = conWindowSized;
	wp->window_closed = conWindowClosed;
	wp->window_destroyed = conWindowDestroyed;
	wp->window_update = conWindowUpdate;
	wp->window_redraw = conWindowRedraw;
	wp->window_get_statusline = conWindowGetState;

	wp->window_feed_key = termFeedKey;
	wp->paste = conWindowPaste;
	wp->mouse_click = conWindowMouseClick;

	wp->darstellung = TerminalDarstellung;
	
	wp->update |= NAME;

	if (!WindowOpen (wp))
	{
		WindowDestroy (wp->handle, TRUE, FALSE);
		return NULL;
	}

	registerTerminal (wp);

	return wp;
}


int MakeConsoleTopWindow (int *was_opened, word *old_top_window)
{
	WindInfo *mwp;

	*old_top_window = -1;
	*was_opened = FALSE;
	
	if ((mwp = GetTopWindInfoOfKind (WK_CONSOLE)) != NULL)
	{
		wind_get (0, WF_TOP, old_top_window);
		
		if (*old_top_window != mwp->handle)
			DoTopWindow (mwp->handle);
		else
			*old_top_window = -1;
	}
	else
	{
		if (TerminalWindowOpen ("", "", TRUE) > 0)
		{
			mwp = GetTopWindInfoOfKind (WK_CONSOLE);
			*was_opened = TRUE;
		}
		else
		{
			venusErr (NlsStr (T_CONOPEN));
			return FALSE;
		}
	}

	/* Behandle Redraw-Messages, die noch im System h„ngen
	 */
	HandlePendingMessages (TRUE);

	return TRUE;
}


int RestoreConsolePosition (int was_opened, word old_top_window)
{
	WindInfo *mwp = GetTopWindInfoOfKind (WK_CONSOLE);

	if (mwp == NULL)
		return FALSE;
		
	if (was_opened)
	{
		WindowClose (mwp->handle, 0);
	}
	else if (old_top_window > 0)
	{
		/* Das Console-Fenster war offen, aber nicht oben.
		 * Also bringen wir das vorherige Top-Fenster wieder
		 * dorthin, wo es war.
		 */
		DoTopWindow (old_top_window);
		WT_BuildRectList (mwp->handle);
	}
	
	return TRUE;
}



extern long xPvfork (void *new_stack);

int StartProgramInWindow (const char *name, const COMMAND *args, 
	const char *dir, const char *env)
{
	WindInfo *wp;
	ConsoleWindow_t *cwp;
	int i;
	long pmask, kidfd;

	wp = TerminalWindowOpen (dir, name, FALSE);
	if (wp == NULL)
		return FALSE;
	
	cwp = wp->kind_info;
	
	pmask = Psigblock (1L << SIGCHLD);

 	i = Pfork ();
	if (i < 0)
	{
		Psigsetmask (pmask);
		WindowDestroy (wp->handle, TRUE, FALSE);
		venusErr (NlsStr (T_NOPROC));
		return FALSE;
	} 
	
	if (i == 0)
	{
		Psetpgrp (0, 0);
		Psignal (SIGTERM, SIG_DFL);
		Psignal (SIGTSTP, SIG_DFL);

		kidfd = Fopen (cwp->terminal->termname, 2);
		if (kidfd < 0) Pterm (998);
		
		Fforce (-1, (int) kidfd);
		Fforce (0, (int) kidfd);
		Fforce (1, (int) kidfd);
		Fforce (2, (int) kidfd);
		Fclose ((int) kidfd);
		SetFullPath (dir);
		i = (int)Pexec (200, (char *)name, (char *)args, (char *)env);
		Pterm (i);
	}
	
/*	Mfree (newstack); */
	
	Psigsetmask (pmask);
	/* reap the exit code of the just terminated process
	 */
#if 0
#define WNOHANG 1
	do
	{
		r = Pwait3 (WNOHANG, (void *)0);
	}
	while ((((r & 0xffff0000L) >> 16L) != i) && (r != EFILNF));

	i = (int)(r & 0x0000ffffL);
#endif
	cwp->terminal->process_grp = i;

	return TRUE;
}