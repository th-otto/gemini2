/*
 * @(#) Gemini\accwind.c
 * @(#) Stefan Eissing, 29. September 1993
 *
 * description: functions to manage accessorie windows
 */

#include <stdlib.h>
#include <string.h>
#include <aes.h>

#include "vs.h"
#include "menu.rh"

#include "accwind.h"
#include "window.h"
#include "util.h"


/* Externals
 */
extern OBJECT *pmenu;


static void sendWindClose (WindInfo *wp, word kstate)
{
	word messbuff[8];
	
	(void)kstate;
	messbuff[0] = WM_CLOSED;
	messbuff[1] = apid;
	messbuff[2] = 0;
	messbuff[3] = wp->handle;
	
	appl_write (wp->owner, 16, messbuff);
}

static void accWindowTopped (WindInfo *wp)
{
	(void)wp;
	menu_ienable (pmenu, WINDCLOS, TRUE);
	menu_ienable (pmenu, CLOSE, TRUE);
}


static void accWindowUntopped (WindInfo *wp)
{
	(void)wp;
	UpdateSpecialIcons ();
}

word InsertAccWindow (word accid, word whandle)
{
	WindInfo *wp;
	word tophandle;
	
	if (whandle <= 0)
		return FALSE;
	
	wp = GetWindInfo (whandle);
	if (wp == NULL)
		wp = WindowCreate (0, whandle);

	if (!wp)
		return FALSE;

	wp->handle = whandle;
	wp->kind = WK_ACC;
	wp->owner = accid;
	
	wp->window_topped = accWindowTopped;
	wp->window_untopped = accWindowUntopped;
	wp->window_closed = sendWindClose;

	wind_get (0, WF_TOP, &tophandle);
	IsTopWindow (tophandle);
		
	return TRUE;
}

word RemoveAccWindow (word accid, word whandle)
{
	WindInfo *wp;

	if (whandle <= 0)
		return FALSE;	

	wp = GetWindInfo (whandle);
	if (!wp || (wp->owner != accid))
		return FALSE;
		
	WindowDestroy (wp->handle, FALSE, FALSE);
	UpdateSpecialIcons ();
	return TRUE;
}
