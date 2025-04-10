/*
 * @(#) Gemini\redraw.c
 * @(#) Stefan Eissing, 16. August 1992
 *
 * description: functions to redraw windows
 */

#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"

#include "window.h"
#include "redraw.h"
#include "venuserr.h"



/* internals
*/
static GRECT act_rect;				/* local rectangle for buffered */
static WindInfo *currwp = NULL;		/* redraw */


/*
 * merge currw with r into currw
 */
static void mergerect (const GRECT *rect)
{
	word x1, y1, x2, y2;
	
	x1 = min (act_rect.g_x, rect->g_x);
	y1 = min (act_rect.g_y, rect->g_y);
	x2 = max (act_rect.g_x + act_rect.g_w, rect->g_x + rect->g_w);
	y2 = max (act_rect.g_y + act_rect.g_h, rect->g_y + rect->g_h);
	act_rect.g_x = x1;
	act_rect.g_y = y1;
	act_rect.g_w = x2 - x1;
	act_rect.g_h = y2 - y1;
}

/*
 * flush redraw which was buffered via buffredraw
 */
void flushredraw (void)
{
	if (currwp != NULL)
	{
		if (currwp->window_redraw)
			currwp->window_redraw (currwp, &act_rect);
		currwp = NULL;
	}
}

/*
 * same as redraw(wp,r), but it buffers the redraws until
 * wp->handle changes or flushredraw is called.
 */
void buffredraw (WindInfo *wp, const GRECT *rect)
{
	if (currwp == NULL)
	{
		act_rect = *rect;
		currwp = wp;
	}
	else
	{
		if(wp != currwp)
		{
			flushredraw ();
			act_rect = *rect;
			currwp = wp;
		}
		else
			mergerect (rect);
	}
}


void redrawObj (WindInfo *wp, word objnr)
{
	GRECT rect;
	OBJECT *tree;

	if (wp->window_get_tree)
	{
		tree = wp->window_get_tree (wp);

		objc_offset (tree, objnr, &rect.g_x, &rect.g_y);
		rect.g_w = tree[objnr].ob_width;
		rect.g_h = tree[objnr].ob_height;
		buffredraw (wp, &rect);
	}
}

/*
 * update all things marked in upflag in Window wp->handle
 */
void rewindow (WindInfo *wp, word upflag)
{
	wp->update = upflag;		/* neue Updates setzen */
	
	WindowUpdateData (wp);
}

/*
 * update things marked in upflag in all Windows
 * except Window 0
 */
void allrewindow (word upflag)
{
	WindInfo *wp;
	word num;
	MFORM form;
	
	wp = wnull.nextwind;
	GrafGetForm (&num, &form);
	GrafMouse (HOURGLASS, NULL);

	while (wp != NULL)
	{
		rewindow (wp, upflag);
		wp = wp->nextwind;		
	}
	
	GrafMouse (num, &form);
}


/*
 * read files for all windows and
 * update them (keeping slider position)
 * but don't do any redraw on screen
 */
void allFileChanged (void)
{
	WindInfo *wp;
	
	wp = wnull.nextwind;
	while (wp != NULL)
	{
		if (wp->add_to_display_path != NULL)
		{
			wp->add_to_display_path (wp, NULL);
		}
		wp = wp->nextwind;		
	}

	/* wir haben alles neu eingelesen, also kein Update durch
	 * CommInfo.dirty mehr
	 */
	if (pDirtyDrives != NULL)
		*pDirtyDrives = 0;
}
