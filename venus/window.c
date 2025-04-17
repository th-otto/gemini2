/*
 * @(#) Gemini\Window.c
 * @(#) Stefan Eissing, 13. November 1994
 *
 * description: functions to handle windows
 *
 * jr 28.7.94
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\genargv.h"

#include "vs.h"
#include "menu.rh"

#include "window.h"
#include "fileutil.h"
#include "myalloc.h"
#include "undo.h"
#include "util.h"
#include "select.h"
#include "venuserr.h"
#include "windstak.h"


/* Externals
 */
extern OBJECT *pmenu;


/*
 * internal texts 
 */
#define NlsLocalSection "G.window"
enum NlsLocalText{
T_NOWINDOW,			/*Kann kein Fenster ”ffnen, da das AES keine
 weiteren Fenster zur Verfgung stellt! Sie
 sollten ein unbenutzes Fenster schliežen.*/
T_NOMEM,			/*Es steht nicht genug Speicher zur Verfgung!*/
};


/* Struktur fr Window 0
 */
WindInfo wnull;
static int topWindowChanged;

/* jr: von util.c rbergeholt und den neg_check-Selektor eingefgt
   (wenn der User es wirklich aus dem Bildschirm rausbewegen will,
   dann sei es eben so... */

static word charAlign (word x, word neg_check)
{
	word tmp;
	
	tmp = ((x + 1)/ wchar) * wchar;

#if 0
	/* jr: verstehe ich nicht */
	if (((x + 1) % wchar) * 2 > wchar)
		tmp += wchar;
#endif
	if (neg_check && tmp <= 0)
		tmp = wchar;

	return tmp - 1;
}


/* get Pointer from list of Windows
 */
WindInfo *GetWindInfo (word whandle)
{
	WindInfo *p;
	
	for (p = &wnull; p; p = p->nextwind)
		if (p->handle == whandle)
			return p;

	return NULL;
}


WindInfo *NewWindInfo (void)
{
	WindInfo *p;
	
	p = (WindInfo *)calloc (1, sizeof (WindInfo));

	if (p != NULL)
	{
		if (wnull.nextwind != NULL 
			&& wnull.nextwind->nextwind == NULL)
		{
			/* Es sind jetzt mehr als ein Fenster offen
			 */
			menu_ienable (pmenu, CYCLEWIN, TRUE);
		}

		p->nextwind = wnull.nextwind;
		wnull.nextwind = p;
		topWindowChanged = TRUE;
	}
	
	return p;
}

word FreeWindInfo (WindInfo *wp)
{
	WindInfo *wplist = wnull.nextwind;
	WindInfo *prevwp = &wnull;
	
	while (wplist != NULL)
	{
		if (wplist == wp)
		{
			prevwp->nextwind = wplist->nextwind;
			free (wp);

			if (wnull.nextwind != NULL 
				&& wnull.nextwind->nextwind == NULL)
			{
				/* Es ist jetzt nur noch ein Fenster offen
				 */
				menu_ienable (pmenu, CYCLEWIN, FALSE);
			}

			/* Das oberste Fenster wurde entfernt
			 */
			if (prevwp == &wnull)
				topWindowChanged = TRUE;
			
			return TRUE;
		}
		
		prevwp = wplist;
		wplist = wplist->nextwind;
	}
	
	return FALSE;
}


static void setWpOnTop (word whandle)
{
	WindInfo *cp, *pp;
	
	if (whandle < 1)
		return;
	
	/* wnull.nextwind ist das oberste Fenster. Wir sagen ihm
	 * Bescheid, daž das nun nicht mehr so ist, wenn es es
	 * wnscht.
	 */
	if (wnull.nextwind && wnull.nextwind->window_untopped)
		wnull.nextwind->window_untopped (wnull.nextwind);
		
	pp = &wnull;
	cp = pp->nextwind;
	
	while ((cp != NULL) && (cp->handle != whandle))
	{
		pp = cp;
		cp = pp->nextwind;
	}
	
	if ((cp != NULL) && (pp != &wnull))
	{
		pp->nextwind = cp->nextwind;
		cp->nextwind = wnull.nextwind;
		wnull.nextwind = cp;
	}
}


WindInfo *GetTopWindInfo (void)
{
	if (wnull.nextwind)
		return wnull.nextwind;
	else
		return &wnull;
}


WindInfo *GetTopWindInfoOfKind (word kind)
{
	WindInfo *wp = &wnull;
	
	while (wp)
	{
		if (wp->kind == kind)
			break;
		
		wp = wp->nextwind;
	}
	
	return wp;
}


/*
 * setze hinterstes Fenster als neues oberster Fenster, via
 * appl_write() (auch an Accessories)
 */
void cycleWindow (void)
{
	WindInfo *wp;
	
	wp = wnull.nextwind;
	if (wp == NULL)
		return;
		
	while (wp->nextwind)
		wp = wp->nextwind;

	topWindowChanged = TRUE;

	if (wp->owner != apid)
	{
		word messbuff[8];

		messbuff[0] = WM_TOPPED;
		messbuff[1] = apid;
		messbuff[2] = 0;
		messbuff[3] = wp->handle;

		WindUpdate (BEG_UPDATE);
		appl_write (wp->owner, 16, messbuff);
		WindUpdate (END_UPDATE);
		setWpOnTop (wp->handle);
		if (wp->window_topped)
			wp->window_topped (wp);
	}
	else
	{
		DoTopWindow (wp->handle);
	}
}


static void centerWindow (WindInfo *wp, GRECT *wind)
{
	word dx, dy;
	
	/* center over Rectangle
	 */
	dx = (wind->g_w - wp->wind.g_w) >> 1;
	
	if (wind->g_y + wp->wind.g_h < Desk.g_y + Desk.g_h)
		dy = 0;
	else
		dy = (wind->g_h - wp->wind.g_h) >> 1;

	wp->wind.g_x = wind->g_x + dx;
	wp->wind.g_y = wind->g_y + dy;
	
	/* align with desk Workarea
	 */
	if (wp->wind.g_x < Desk.g_x)
		wp->wind.g_x = Desk.g_x;

	if (wp->wind.g_y < Desk.g_y)
		wp->wind.g_y = Desk.g_y;

	if (wp->wind.g_x > (dx = Desk.g_x + Desk.g_w - wp->wind.g_w))
		wp->wind.g_x = dx;

	if (wp->wind.g_y > (dy = Desk.g_y + Desk.g_h - wp->wind.g_h))
		wp->wind.g_y = dy;
}


static void PosOnRect (WindInfo *wp)
{
	GRECT wind, rect;
	word max = 0;
	word horizontal, new, mx, my, bstate, kstate;
	
	ButtonPressed (&mx, &my, &bstate, &kstate);
	horizontal = (wp->wind.g_w > wp->wind.g_h);
	
	wind_get (0, WF_FIRSTXYWH, &rect.g_x, &rect.g_y, &rect.g_w, 
		&rect.g_h);
	while (rect.g_w && rect.g_h)
	{
		new = (horizontal)? rect.g_w : rect.g_h;
		
		if (PointInGRECT (mx, my, &rect))
			new /= 16;		/* m”glichst nicht auf Icon */
			
		if (max <= new)
		{
			max = new;
			wind = rect;
		}
		
		wind_get (0, WF_NEXTXYWH, &rect.g_x, &rect.g_y, &rect.g_w, 
			&rect.g_h);
	}
	
	if (max <= 0)
	{
		wind.g_w = Desk.g_w / 2;
		wind.g_h = Desk.g_h / 2;
		wind.g_x = Desk.g_x + (wind.g_w / 2);
		wind.g_y = Desk.g_x + (wind.g_h / 2);
	}
	
	centerWindow (wp, &wind);
		
	wp->update &= ~WU_POSITION;
}

/*
 * Schliežt alle noch offenen Fenster, auch wenn sie uns nicht
 * geh”ren. Dies ist natrlich unter einem MultiGEM absolut falsch.
 * Nur, wenn es eine gleichzeitig aktive Applikation gibt, mssen
 * wir so vorgehen.
 */
void delAccWindows (void)
{
	word topwind;
	char path[MAX_PATH_LEN];
	
	/* Mehr als eine atkive Applikation? Dann haben wir hier nichts
	 * zu tun.
	 */
	if (_GemParBlk.global[1] != 1) return;
		
	/* Rette zur Sicherheit den Pfad, und stelle ihn nachher wieder
	 * her. Dies ist notwendig, da andere Routinen denken k”nnten,
	 * es sei ein anderes Fenster oben, und dann selbst den Pfad neu
	 * setzen. Dies ist aber beim Starten von externen TOS-Programmen
	 * absolut nicht erwnscht.
	 */
	GetFullPath (path, MAX_PATH_LEN);

	if (GotGEM14 ())
	{
		/* Wenn wir wind_new haben, r„umen wir damit auf. Das geht
		 * am schnellsten.
		 */
		wind_new ();
		WindRestoreControl ();
	}
	else
	{
		/* Kein wind_new(). Wir machen es mhsam von Hand und holen
		 * uns nacheinander das Handle des obersten Fensters und
		 * schliežen es.
		 */
		wind_get (0, WF_TOP, &topwind);
		while (topwind > 0)
		{
			if (GetWindInfo (topwind) != NULL)
			{
				WindowDestroy (topwind, TRUE, TRUE);
			}
			else
			{
				wind_close (topwind);
				wind_delete (topwind);
			}
			wind_get (0, WF_TOP, &topwind);
		}
	}
	
	SetFullPath (path);
}


/* Schliežt alle eigenen Fenster. Wenn <wind_new()> zur Verfgung
 * steht, werden diese nicht richtig beim AES geschlossen, um
 * unn”tiges Neuzeichnen von Fenstern, die sowieso geschlossen
 * werden zu vermeiden.
 */
void CloseAllWind (int close_accs_too)
{
	WindInfo *wp, *savwp;
	word oldanz, anz, phys_del;
	word save_kind;
	GRECT dummy;
	
	phys_del = !close_accs_too || !GotGEM14 () || 
		(_GemParBlk.global[1] != 1);

	anz = 0;
	do
	{
		wp = wnull.nextwind;
		oldanz = anz;
	
		while (wp != NULL)
		{
			savwp = wp->nextwind;
			save_kind = wp->kind;
			
			if (close_accs_too || wp->owner == apid)
			{
				anz++;
				WindowDestroy (wp->handle, phys_del, phys_del);
				PopWindBox (&dummy, save_kind);
			}
	
			wp = savwp;
		}
	}
	while (close_accs_too && (oldanz != anz));
	
	if (phys_del)
	{
		/* kill redraw-events (hope so)
		 */
		killEvents (MU_MESAG, anz);
	}
	
	if (close_accs_too)
		delAccWindows ();
}


/* Setze die Informationen, die zum Fenster <topwindow> geh”ren,
 * da das nun das oberste Fenster sein wird.
 */
void IsTopWindow (word topwindow)
{
	WindInfo *wp;
	
	if (((wp = GetWindInfo (topwindow)) != NULL) && topWindowChanged)
	{
		setWpOnTop (wp->handle);
		if (wp->window_topped)
			wp->window_topped (wp);
		topWindowChanged = FALSE;
	}
}

/* Žhnlich wie SetTopWindowInfo, nur wird auch das Fenster <whandle>
 * mittels des AES als oberstes Fenster installiert.
 */
void DoTopWindow (word whandle)
{
	wind_set (whandle, WF_TOP);
	topWindowChanged = TRUE;
	IsTopWindow (whandle);
}


void DoBottomWindow (word whandle)
{
	WindInfo *wp, *prev;
	
#ifndef WF_BOTTOM
#define WF_BOTTOM	25
#endif
	wind_set (whandle, WF_BOTTOM);
	
	wp = wnull.nextwind;
	prev = &wnull;
	while (wp != NULL)
	{
		if (wp->handle == whandle)
		{
			prev->nextwind = wp->nextwind;
			while (prev->nextwind)
				prev = prev->nextwind;
			
			prev->nextwind = wp;
			wp->nextwind = NULL;
			
			break;
		}
		
		prev = wp;
		wp = wp->nextwind;
	}

	if (wnull.nextwind)
		IsTopWindow (wnull.nextwind->handle);	
}


WindInfo *WindowCreate (word attributes, word wind_handle)
{
	WindInfo *wp;

	if (wind_handle <= 0)
	{	
		wind_handle = wind_create (attributes, Desk.g_x, Desk.g_y,
			Desk.g_w, Desk.g_h);
	}
	
	if (wind_handle < 0)
	{
		venusErr (NlsStr (T_NOWINDOW));
		return NULL;
	}
	
	wp = NewWindInfo ();
	if (wp == NULL)
	{
		venusErr (NlsStr (T_NOMEM));
		return NULL;
	}

	wp->handle = wind_handle;
	wp->owner = apid;
	wp->type = attributes;

	return wp;	
}


#if 0
/* jr: Nach wind_open wind_get aufrufen, um die tats„chliche
   Gr”že zu ermitteln (WinX sorgt zB fr eine Mindestgr”že, die
   uU von der von TOS abweicht */
/* klappt noch nicht, da WindowUpdateData vor dem ™ffnen aufgerufen
   werden muž */
int WindowOpen (WindInfo *wp)
{
	if (wp->update & WU_POSITION)
		PosOnRect (wp);			/*freie Fl„che in Window 0*/

	if (ShowInfo.aligned)
		wp->wind.g_x = charAlign (wp->wind.g_x, TRUE);

	wind_open (wp->handle, wp->wind.g_x, wp->wind.g_y, 
		wp->wind.g_w, wp->wind.g_h);

	wind_get (wp->handle, WF_CURRXYWH, &wp->wind.g_x, &wp->wind.g_y,
		&wp->wind.g_w, &wp->wind.g_h);
		
	wind_calc (WC_WORK, wp->type, wp->wind.g_x, wp->wind.g_y,
		wp->wind.g_w, wp->wind.g_h, &wp->work.g_x, 
		&wp->work.g_y, &wp->work.g_w, &wp->work.g_h);

	WindowUpdateData (wp);

	storeWOpenUndo (TRUE, wp);

	if (wp->window_topped)
		wp->window_topped (wp);
	
	return TRUE;
}
#endif

int WindowOpen (WindInfo *wp)
{
	int updateSave;
	
	if (wp->update & WU_POSITION)
		PosOnRect (wp);			/*freie Fl„che in Window 0*/

	if (ShowInfo.aligned)
		wp->wind.g_x = charAlign (wp->wind.g_x, TRUE);

	wind_calc (WC_WORK, wp->type, wp->wind.g_x, wp->wind.g_y,
		wp->wind.g_w, wp->wind.g_h, &wp->work.g_x, 
		&wp->work.g_y, &wp->work.g_w, &wp->work.g_h);

	updateSave = wp->update;
	WindowUpdateData (wp);

	wind_open (wp->handle, wp->wind.g_x, wp->wind.g_y, 
		wp->wind.g_w, wp->wind.g_h);
	wind_get (wp->handle, WF_CURRXYWH, &wp->wind.g_x, &wp->wind.g_y, 
		&wp->wind.g_w, &wp->wind.g_h);
	wind_get (wp->handle, WF_WORKXYWH, &wp->work.g_x, &wp->work.g_y, 
		&wp->work.g_w, &wp->work.g_h);
		
	/* I'll kill those guys at Atari. TOS 4.0x loses sometimes
	 * the sliderposition after we opened the window. So, set
	 * it again here.
	 */
	wp->update = updateSave;
	WindowUpdateData (wp);

	storeWOpenUndo (TRUE, wp);

	if (wp->window_topped)
		wp->window_topped (wp);
	
	return TRUE;
}


int WindowClose (word window_handle, word kstate)
{
	WindInfo *wp;
	
	wp = GetWindInfo (window_handle);
	if (wp == NULL)
		return FALSE;
	
	if (wp->window_closed)
		wp->window_closed (wp, kstate);
		
	return TRUE;
}


int WindowDestroy (word window_handle, int is_still_open,
				int definitely_delete_it)
{
	WindInfo *wp;
	
	wp = GetWindInfo (window_handle);
	if (wp == NULL)
		return FALSE;

	storeWOpenUndo (FALSE, wp);

	if (wp->window_destroyed)
		wp->window_destroyed (wp);
	
	if (is_still_open)
	{
		if (definitely_delete_it || (wp->owner == apid))
		{
			word handle;
		
			wind_close (window_handle);
			wind_delete (window_handle);
		
			wind_get (0, WF_TOP, &handle);
			IsTopWindow (handle);
		}
		else
		{
			WindowClose (window_handle, 0);
			return FALSE;
		}
	}

	FreeWindInfo (wp);

	return TRUE;
}

/* Das Rechteck <r> bestimmt die neuen Ausmaže des Fensters 
 * <window_handle>
 * Das Fenster wird auf die neuen Ausmaže gesetzt und die alten
 * Ausmaže werden im Undo-Buffer gesichert.
 */
int WindowMove (word window_handle, GRECT *wind)
{
	WindInfo *wp;
	
	wp = GetWindInfo (window_handle);
	if (wp == NULL || wp->owner != apid)
		return FALSE;

	if (ShowInfo.aligned)
		wind->g_x = charAlign (wind->g_x, FALSE);
		
	wind_set (wp->handle, WF_CURRXYWH, wind->g_x, wind->g_y, 
		wind->g_w, wind->g_h);
		
	storeWMoveUndo (wp);
	
	wp->wind = *wind;
	
	wind_get (wp->handle, WF_WORKXYWH, &wp->work.g_x, &wp->work.g_y,
			&wp->work.g_w, &wp->work.g_h);
	
	if (wp->window_moved)
		wp->window_moved (wp);
	
	return TRUE;
}


int WindowSize (word window_handle, GRECT *wind)
{
	WindInfo *wp;
	
	wp = GetWindInfo (window_handle);
	if (wp == NULL || wp->owner != apid)
		return FALSE;

	if (wp->window_calc_windrect)
		wp->window_calc_windrect (wp, wind);
		
	if (!WindowMove (window_handle, wind))
		return FALSE;
	
	if (wp->window_sized)
		wp->window_sized (wp);
	
	return TRUE;
}


int WindowFull (word window_handle)
{
	WindInfo *wp;
	GRECT rect;
	
	wp = GetWindInfo (window_handle);
	if (wp == NULL)
		return FALSE;

	if (wp->oldwind.g_w)			/* gleich null -> volle Gr”že */
	{
		rect = wp->oldwind;
		wp->oldwind.g_w = 0;
	}
	else
	{
		rect = Desk;

		if (ShowInfo.aligned)
		{
			word pommes;
			
			pommes = charAlign (rect.g_x, TRUE);	/* jr: ih b„h */
			rect.g_w -= (pommes - rect.g_x);
			rect.g_x = pommes;
		}

		wind_get (wp->handle, WF_CURRXYWH, &wp->oldwind.g_x,
			 &wp->oldwind.g_y, &wp->oldwind.g_w, &wp->oldwind.g_h);
	}
	
	return WindowSize (window_handle, &rect);
}

void WindowUpdateData (WindInfo *wp)
{
	int old_vsize = wp->vslsize;
	int old_hsize = wp->hslsize;

	if (wp->update && wp->window_update)
		wp->window_update (wp);
	
	if ((wp->type & HSLIDE) && (wp->update & WU_HOSLIDER))
	{
		wp->update &= ~WU_HOSLIDER;
		
		wind_set (wp->handle, WF_HSLIDE, wp->hslpos);
		if (old_hsize != wp->hslsize)
			wind_set (wp->handle, WF_HSLSIZE, wp->hslsize);
	}

	if ((wp->type & VSLIDE) && (wp->update & WU_VESLIDER))
	{
		wp->update &= ~WU_VESLIDER;

		wind_set (wp->handle, WF_VSLIDE, wp->vslpos);
		if (old_vsize != wp->vslsize)
			wind_set (wp->handle, WF_VSLSIZE, wp->vslsize);
	}

	wp->update = 0;
}

void UpdateAllWindowData (void)
{
	WindInfo *wp = wnull.nextwind;
	
	while (wp)
	{
		WindowUpdateData (wp);
		
		wp = wp->nextwind;
	}
}

int WindGetSelected (ARGVINFO *A, int preferLowerCase)
{
	WindInfo *wp;
	
	InitArgv (A);
	if (!ThereAreSelected (&wp) || !wp->getSelected)
		return FALSE;
		
	return wp->getSelected (wp, A, preferLowerCase);
}

int WindGetDragged (ARGVINFO *A, int preferLowerCase)
{
	WindInfo *wp;
	
	InitArgv (A);
	if (!ThereAreSelected (&wp) || !wp->getDragged)
		return FALSE;
		
	return wp->getDragged (wp, A, preferLowerCase);
}
