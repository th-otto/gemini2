/* 
 * @(#) Gemini\icondrag.c
 * @(#) Stefan Eissing, 20. MÑrz 1993
 *
 *	Iconstuff for Desktop-Icons (Dragging, Marking, Movin')
 *
 * jr 26.1.97
 */

#include <stdlib.h>
#include <vdi.h>
#include <string.h>
#include <limits.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include <common/cookie.h>
#include <common/strerror.h>
#include <mupfel/mupfel.h>

#include "vs.h"

#include "window.h"
#include "draglogi.h"
#include "fileutil.h"
#include "filewind.h"
#include "message.h"
#include "myalloc.h"
#include "wintool.h"
#include "redraw.h"
#include "scroll.h"
#include "select.h"
#include "util.h"
#include "venuserr.h"


/*
 * Gibt zurÅck, ob der Punkt (mx, my) sich auf dem Icon
 * objnr im Tree tree befindet. BerÅcksichtigt Maske und
 * Textfahne des Objektes.
 * ACHTUNG: Diese Version funktioniert nur mit Objekten in
 * der ersten Ebene des Baumes!
 * 
 */
static int isOnIcon (word mx, word my, word selected, ICON_OBJECT *icon)
{
	if ((mx >= icon->icon.ib_xicon)
		&& (mx < icon->icon.ib_xicon + icon->icon.ib_wicon)
		&& (my >= icon->icon.ib_yicon)
		&& (my < icon->icon.ib_yicon + icon->icon.ib_hicon))
	{
		word *mask;
		
		mx -= icon->icon.ib_xicon;
		my -= icon->icon.ib_yicon;
		mask = ((icon->mainlist != NULL)?
				((selected && icon->mainlist->sel_data)? 
				icon->mainlist->sel_mask : icon->mainlist->col_mask)
				 : icon->icon.ib_pmask) 
				+ (my * ((icon->icon.ib_wicon + 15) / 16))
				+ (mx / 16);
		mx = 0x8000 >> (mx % 16);
		return *mask & mx;
	}
	else
	{
		return ((mx >= icon->icon.ib_xtext)
			&&(mx < icon->icon.ib_xtext + icon->icon.ib_wtext)
			&&(my >= icon->icon.ib_ytext)
			&&(my < icon->icon.ib_ytext + icon->icon.ib_htext));
	}
}


#define FIND_SENSIBLE	1

/*
 * decides if mx, my is on object objnr in tree
 */ 
static word isOnObject (word mx, word my, OBJECT *tree, word objnr)
{
	word obx, oby;

	if (tree[objnr].ob_flags & HIDETREE)
		return FALSE;
		
	objc_offset (tree, objnr, &obx, &oby);

	switch (tree[objnr].ob_type & 0xFF)
	{
		case G_USERDEF:
		{
			ICON_OBJECT *icon = 
				(ICON_OBJECT *)tree[objnr].ob_spec.userblk->ub_parm;

#if !FIND_SENSIBLE
			if (icon->flags.is_icon)
#endif
				return isOnIcon (mx - obx, my - oby, 
					tree[objnr].ob_state & SELECTED, icon);
#if !FIND_SENSIBLE
			else
				return ((mx >= obx)
					 && (mx <= obx + tree[objnr].ob_width)
					 && (my >= oby)
					 && (my <= oby + tree[objnr].ob_height));
#endif
		}
		
		default:
			return FALSE;
	}
}

/*
 * ermittelt das Icon an Position x, y. Befinden sich mehrere
 * Icons an der Position, so wird das zuletzt gefundene
 * zurÅckgegeben, da von unten nach oben sortiert wird.
 */
word FindObject (OBJECT *tree, word x, word y)
{
	word i, fobj = -1;
	
	x -= tree[0].ob_x;
	y -= tree[0].ob_y;
	
	for (i = tree[0].ob_head; i > 0; i = tree[i].ob_next)
	{
		if (tree[i].ob_flags & HIDETREE)
			continue;
			
		switch (tree[i].ob_type & 0xFF)
		{
			case G_USERDEF:
			{
				ICON_OBJECT *icon = 
					(ICON_OBJECT *)tree[i].ob_spec.userblk->ub_parm;

#if !FIND_SENSIBLE
				if (icon->flags.is_icon)
				{
#endif
					if (isOnIcon (x - tree[i].ob_x, y - tree[i].ob_y, 
						tree[i].ob_state & SELECTED, icon))
					{
							fobj = i;
					}
#if !FIND_SENSIBLE
				}
				else if ((x >= tree[i].ob_x)
						 && (x <= tree[i].ob_x + tree[i].ob_width)
						 && (y >= tree[i].ob_y)
						 && (y <= tree[i].ob_y + tree[i].ob_height))
				{
					fobj = i;
				}
#endif
				break;
			}
		}
	}
	
	return fobj;
}

/* Gibt TRUE zurÅck, wenn auf das Objekt etwas gezogen werden kann
 */
static int canDragTo (WindInfo *wp, OBJECT *po)
{
	if (wp->kind == WK_FILE)
	{
		FileInfo *myinfo;
	
		if ((myinfo = GetFileInfo (po)) == NULL)
			return FALSE;
		
		if (myinfo->icon.flags.is_folder != 0)
			return TRUE;
		else
			return IsExecutable (myinfo->name);
	}
	
	return TRUE;
}

/*
 * built an array of rectangles for objects of type deftype
 * for anz objects and calculate the size of a box all ghosts
 * will fit in.
 */
static void builtGhosts (WindInfo *wp, word deftype, word ghosts[], 
	word *anz, word max[4])
{
	word i, j, obx, oby;
	OBJECT *tree;
	ICONBLK *picon;
	word *pw;
	
	if (wp->window_get_tree)
		tree = wp->window_get_tree (wp);
	else
		return;
		
	setBigClip (vdi_handle);
	*anz = j = 0;

	for (i = tree[0].ob_head; i > 0; i = tree[i].ob_next)
	{
		if (isSelected (tree, i) && !isHidden (tree, i))
		{
			obx = tree[0].ob_x + tree[i].ob_x;
			oby = tree[0].ob_y + tree[i].ob_y;

			picon = NULL;
			if (deftype == G_USERDEF)
			{
				ICON_OBJECT *icon = 
					(ICON_OBJECT *)tree[i].ob_spec.userblk->ub_parm;

				picon = &icon->icon;
			}
			
			if (picon != NULL)
			{
				*anz += 8;
				ghosts[j++] = obx + picon->ib_xicon;
				ghosts[j++] = oby + picon->ib_yicon;
				ghosts[j++] = picon->ib_wicon;
				ghosts[j++] = picon->ib_hicon;
				ghosts[j++] = obx + picon->ib_xtext;
				ghosts[j++] = oby + picon->ib_ytext;
				ghosts[j++] = (!picon->ib_ptext || picon->ib_ptext[0])?
					picon->ib_wtext : 0;
				ghosts[j++] = picon->ib_htext;
			}
			else
			{
				*anz += 4;
				ghosts[j++] = obx;
				ghosts[j++] = oby;
				ghosts[j++] = tree[i].ob_width;
				ghosts[j++] = tree[i].ob_height;
			}
		}
	}

	/* Berechne die maximale Grîûe der Box, die alle Ghosts mit
	 * einschlieût
	 */
	max[0] = Desk.g_x + Desk.g_w - 1;
	max[1] = Desk.g_y + Desk.g_h - 1;
	max[2] = Desk.g_x;
	max[3] = Desk.g_y;
	pw = ghosts;
	
	for (i = 0; i < *anz; i += 4, pw += 4)
	{
		word tmp;
		
		if (max[0] > pw[0])
			max[0] = pw[0];
		if (max[1] > pw[1])
			max[1] = pw[1];
		if (max[2] < (tmp = pw[0] + pw[2] - 1))
			max[2] = tmp;
		if (max[3] < (tmp = pw[1] + pw[3] - 1))
			max[3] = tmp;
	}
}

/*
 * void drawRect(word r[4])
 * draw a rectangle on the screen, writing mode has to be set before
 */
void drawRect(word r[4])
{
	if (r[2] && r[3])
		RastDotRect (vdi_handle, r[0], r[1], r[2], r[3]);
}

/* 
 * let user drag icons and return last mouseposition, on which
 * window and onject icons were dragged and the final state of
 * the leyboard
 */
static word dragIcons (word fromx, word fromy, word *tox, word *toy,
	word *towind, word *toobj, word *kfinalstate)
{
	word *ghosts;		    /* Speicherbereich fÅr Ghosts*/
	word ghostanz;			/* Anzahl der Ghosts */
	word maxbox[4];			/* grîûte Ausmaûe */
	word mxsav, mysav;		/* saven der Mausposition */

	word lastwind = -1;		/* Window, wo wir zuletzt waren */
	word lastobj = -1;		/* Object, das zuletzt selectiert war */
	OBJECT *last_tree = NULL;
	
	word fwind, fobj;		/* Was wir so finden */
	OBJECT *found_tree;
	
	word i, dx, dy, mx, my, rx, ry, bstate, kstate, pointanz;
	word deftype, fromwind, oldtoggle, newselect;
	WindInfo *wp;


	mx = fromx;
	my = fromy;
	fromwind = wind_find (fromx, fromy);

	if ((wp = GetWindInfo (fromwind)) != NULL)
	{
		switch (wp->kind)
		{
			case WK_FILE:
			case WK_DESK:
				break;
				
			default:
				wp = NULL;
				break;
		}
	}
	
	if (wp == NULL)
	{
		*tox = fromx;
		*toy = fromy;
		*towind = -1;
		*toobj = -1;
		return FALSE;
	}

	if (wp->window_get_tree)
	{
		OBJECT *tmp_tree;
		
		tmp_tree = wp->window_get_tree (wp);
		fobj = FindObject (tmp_tree, fromx, fromy);
	}
	else
		fobj = -1;

	if (fobj <= 0)
	{
		*tox = fromx;
		*toy = fromy;
		*towind = -1;
		*toobj = -1;		
		return FALSE;
	}

	if (wp->make_ready_to_drag != NULL)
	{
		if (!wp->make_ready_to_drag (wp))
		{
			*tox = fromx;		/* return */
			*toy = fromy;
			*towind = -1;
			*toobj = -1;		
			return FALSE;
		}
	}
	
	WindUpdate (BEG_MCTRL);
	/* markiere zum Draggen
	 */
	MarkDraggedObjects (wp);

	{
		int object_count;
		OBJECT *tree;
		
		if (!wp->window_get_tree
			|| (tree = wp->window_get_tree (wp)) == NULL)
		{
			*tox = fromx;
			*toy = fromy;
			*towind = -1;
			*toobj = -1;
			WindUpdate (END_MCTRL);
			return FALSE;
		}
		else
		{
			object_count = countObjects (tree, 0);
			
			switch (tree[fobj].ob_type & 0xFF)
			{
				case G_USERDEF:
					pointanz = 10;
					deftype = G_USERDEF;
					break;
				
				default:
					pointanz = 5;
					deftype = 0;
					break;
			}

			ghosts = malloc (object_count * pointanz * 2 * 
				sizeof (word));
		}
	}
		
	if (ghosts == NULL)
	{
		WindUpdate (END_MCTRL);
		venusErr (StrError (ENSMEM));
		return FALSE;
	}
	
	/* Ghosts berechnen
	 */
	builtGhosts (wp, deftype, ghosts, &ghostanz, maxbox); 

	mxsav = fromx;							/* Mausposition sichern */
	mysav = fromy;
	maxbox[0] = Desk.g_x + (fromx - maxbox[0]);	/* Box fÅr Mausbewegung */
	maxbox[1] = Desk.g_y + (fromy - maxbox[1]);
	maxbox[2] = Desk.g_x + Desk.g_w + (fromx - maxbox[2]) - 1;
	maxbox[3] = Desk.g_y + Desk.g_h + (fromy - maxbox[3]) - 1;
	
	/* nur Bewegung in Box zulassen
	 */
	if (mxsav < maxbox[0])
		mxsav = maxbox[0];
	else if (mxsav > maxbox[2])
		mxsav = maxbox[2];
	if (mysav < maxbox[1])
		mysav = maxbox[1];
	else if (mysav > maxbox[3])
		mysav = maxbox[3];
	
	dx = mxsav - fromx;
	dy = mysav - fromy;
	if (dx || dy)
	{
		for (i = 0; i < ghostanz; i += 4)
		{
			ghosts[i] += dx;		/* Ghosts verschieben */
			ghosts[i+1] += dy;
		}
	}

	GrafMouse (M_OFF, NULL);
	GrafMouse (FLAT_HAND, NULL);

	vswr_mode (vdi_handle, MD_XOR);
	vsl_color (vdi_handle, 1);
	vsf_interior (vdi_handle, FIS_HOLLOW);
	vs_clip (vdi_handle, 0, ghosts);
	for (i = 0; i < ghostanz; i += 4)
		drawRect (&ghosts[i]);

	GrafMouse (M_ON, NULL);

	fwind = fromwind;
	fobj = -1;
	
	while (!ButtonReleased (&mx, &my, &bstate, &kstate))
	{
		rx = mx; 
		ry = my;			/* wir brauchen mx noch */
		
		/* nur Bewegung in Box zulassen
		 */
		if (rx < maxbox[0])
			rx = maxbox[0];
		else if (rx > maxbox[2])
			rx = maxbox[2];
		if (ry < maxbox[1])
			ry = maxbox[1];
		else if (ry > maxbox[3])
			ry = maxbox[3];
		dx = rx - mxsav;
		dy = ry - mysav;
		mxsav = rx; mysav = ry;
		oldtoggle = newselect = FALSE;
		
		fwind = wind_find (mx, my);
			
		if ((wp = GetWindInfo (fwind)) != NULL)
		{
			GRECT work;

			if (wp->window_get_tree)
				found_tree = wp->window_get_tree (wp);
			else
				found_tree = NULL;
			
			wind_get (fwind, WF_WORKXYWH, &work.g_x, &work.g_y, 
				&work.g_w, &work.g_h);

			if (found_tree && PointInGRECT (mx, my, &work))
			{
				fobj = FindObject (found_tree, mx, my);
				if (fobj > 0 && ObjectWasDragged (&found_tree[fobj]))
					fobj = -1;
			}
			else
				fobj = -1;
				
			if (fobj > 0)
			{
				switch (found_tree[fobj].ob_type & 0xFF)
				{
					case G_USERDEF:
						if (!canDragTo (wp, &found_tree[fobj]))
						{
							fobj = -1;
							found_tree = NULL;
						}
						break;
						
					default:
						fobj = -1;
						found_tree = NULL;
						break;
				}
			}
			else
				fobj = -1;
		}
		else
		{
			fobj = -1;				/* nicht unser Window */
			found_tree = NULL;
		}
			
		if ((lastwind >= 0) && last_tree && (lastobj > 0))
		{
			switch (last_tree[lastobj].ob_type & 0xFF)
			{
				case G_USERDEF:
					oldtoggle = ((lastwind != fwind)
						|| (lastobj != fobj));
					break;
			}
		}
		
		if ((fwind >= 0) && (fobj > 0)
			&& (lastwind != fwind || lastobj != fobj))
		{
			switch (found_tree[fobj].ob_type & 0xFF)
			{
				case G_USERDEF:
						newselect = TRUE;
						oldtoggle = (lastobj > 0);
					break;
						
				default: 
					fobj = -1;
					found_tree = NULL;
					break;
			}
		}
		
		/* Maus wurde bewegt
		 */
		if (dx || dy || oldtoggle || newselect)	
		{
			GrafMouse (M_OFF, NULL);
			vswr_mode (vdi_handle, MD_XOR);
			vsl_color (vdi_handle, 1);
			vsf_interior (vdi_handle, FIS_HOLLOW);
			vs_clip (vdi_handle, 0, ghosts);
			for (i = 0; i < ghostanz; i += 4)
			{
				drawRect (&ghosts[i]);
				ghosts[i] += dx;		/* Ghosts verschieben */
				ghosts[i+1] += dy;
			}
			
			if (oldtoggle)
			{
				DeselectObject (GetWindInfo (lastwind), lastobj, TRUE);
				flushredraw ();
			}
			
			if (newselect)
			{
				SelectObject (GetWindInfo (fwind), fobj, TRUE);
				flushredraw ();
			}

			if (oldtoggle || newselect)
			{
				lastwind = fwind;
				lastobj = fobj;	
				last_tree = found_tree;
			}

			vswr_mode (vdi_handle, MD_XOR);
			vsl_color (vdi_handle, 1);
			vsf_interior (vdi_handle, FIS_HOLLOW);
			vs_clip (vdi_handle, 0, ghosts);
			for (i = 0; i < ghostanz; i += 4)
				drawRect (&ghosts[i]);
				
			GrafMouse (M_ON, NULL);
		}
	}
	
	GrafMouse (M_OFF, NULL);
	vswr_mode (vdi_handle, MD_XOR);
	vsl_color (vdi_handle, 1);
	vsf_interior (vdi_handle, FIS_HOLLOW);
	vs_clip (vdi_handle, 0, ghosts);
	for (i = 0; i < ghostanz; i += 4)
		drawRect (&ghosts[i]);

	GrafMouse (ARROW, NULL);
	GrafMouse (M_ON, NULL);
	WindUpdate (END_MCTRL);

	/* Moving only on Window 0
	 */
	if ((fromwind == 0) && (lastobj == -1))
	{
		*tox = mxsav;			
		*toy = mysav;
	}
	else
	{
		*tox = mx;				/* return where we were */
		*toy = my;
	} 
	
	free (ghosts);

	*towind = fwind;
	*toobj = fobj;
	*kfinalstate = kstate;

	return TRUE;
}

static void drawRect2 (word x1, word y1, word x2, word y2)
{
	word w, h;
	word r[4], windrect[4];
	word counter = 0;

	if (x1 < x2)
	{
		r[0] = x1;
		r[2] = x2;
	}
	else
	{
		r[0] = x2;
		r[2] = x1;
	}
	if (y1 < y2)
	{
		r[1] = y1;
		r[3] = y2;
	}
	else
	{
		r[1] = y2;
		r[3] = y1;
	}
	
	w = r[2] - r[0] + 1;	
	h = r[3] - r[1] + 1;
	
	vswr_mode (vdi_handle, MD_XOR);
	vsl_color (vdi_handle, BLACK);
	vsf_interior (vdi_handle, FIS_HOLLOW);
	
	while (WT_GetRect (counter++, windrect))
	{
		if (VDIRectIntersect (r, windrect))
		{
			vs_clip (vdi_handle, 1, windrect);
			RastDotRect (vdi_handle, r[0], r[1], w, h);
		}
	}
}

#undef max
#undef min

static word max (word x, word y)
{
	return x > y? x : y;
}

static word min (word x, word y)
{
	return x < y? x : y;
}

static int hotRubber (WindInfo *wp, GRECT *box)
{
	GRECT icon;
	int retcode = 0;
	int i;
	OBJECT *tree = NULL;

	if (wp->window_get_tree)
		tree = wp->window_get_tree (wp);

	if (!tree)
		return 0;	
	
	for (i = tree[0].ob_head; i > 0; i = tree[i].ob_next)
	{
		ICONBLK *picon = NULL;

		switch (tree[i].ob_type & 0xFF)
		{
			case G_USERDEF:
			{
				ICON_OBJECT *icon = 
					(ICON_OBJECT *)tree[i].ob_spec.userblk->ub_parm;

				picon = &icon->icon;
				break;
			}
		}
		
		if (picon != NULL)
		{
			word ix, iy;
			GRECT text;
			int was_dragged = !!ObjectWasDragged (tree + i);
			objc_offset (tree, i, &ix, &iy);
			
			icon.g_x = ix + picon->ib_xicon;
			icon.g_y = iy + picon->ib_yicon;
			icon.g_w = picon->ib_wicon;
			icon.g_h = picon->ib_hicon;
			text.g_x = ix + picon->ib_xtext;
			text.g_y = iy + picon->ib_ytext;
			text.g_w = picon->ib_wtext;
			text.g_h = picon->ib_htext;
			
			
			if ((!!(GRECTIntersect (box, &icon) 
					|| GRECTIntersect (box, &text)))
				^ (was_dragged))
			{
				if (tree[i].ob_state & SELECTED)
					DeselectObject (wp, i, TRUE);
				else
					SelectObject (wp, i, TRUE);

				if (was_dragged)
					UnMarkDraggedObject (wp, tree + i);
				else
					MarkDraggedObject (wp, tree + i);
					
				retcode = 1;
			}
		}
	}

	return retcode;
}


static void rubberBox (WindInfo *wp, word mx, word my, GRECT *rect)
{
	word kstate, bstate, curx, cury, oldx, oldy;
	word maxbox[4];
	word num;
	MFORM form;
	int filewind = (wp->kind == WK_FILE);

	WindUpdate (BEG_MCTRL);

	/* Speichere Rechteckliste von Fenster wp
	 */
	WT_BuildRectList (wp->handle);
	
	setBigClip (vdi_handle);
	oldx = mx + 1;
	oldy = my + 1;
	maxbox[0] = wp->work.g_x;
	maxbox[1] = wp->work.g_y;
	maxbox[2] = wp->work.g_x + wp->work.g_w - 1;
	maxbox[3] = wp->work.g_y + wp->work.g_h - 1;

	GrafGetForm (&num, &form);
	GrafMouse (M_OFF, NULL);
	GrafMouse (POINT_HAND, NULL);
	drawRect2 (mx, my, oldx, oldy);
	GrafMouse (M_ON, NULL);
	
	curx = mx;
	cury = my;
	
	while (!ButtonReleased (&curx, &cury, &bstate, &kstate))
	{
		int skip_lines = 0;
		
		if (oldx == curx && oldy == cury)
			continue;

		/* nur Bewegung in Box zulassen
		 */
		if(curx < maxbox[0])		/* Zu weit links */
		{
			curx = maxbox[0];
		}
		else if(curx > maxbox[2])	/* Zu weit rechts */
		{
			curx = maxbox[2];
		}
		if(cury < maxbox[1])		/* Zu weit oben */
		{	
			if (filewind)
			{
				if (((FileWindow_t *)wp->kind_info)->yskip)
					skip_lines = 1;
			}
			cury = maxbox[1];
		}
		else if(cury > maxbox[3])	/* Zu weit unten */
		{
			if ((wp->vslpos < 1000) && (wp->vslsize < 1000))
				skip_lines = -filewind;
			cury = maxbox[3];
		}

		if ((curx - oldx) || (cury - oldy) || skip_lines)
		{
			int div_pix = 0;

			GrafMouse (M_OFF, NULL);
			drawRect2 (mx, my, oldx, oldy);
			oldx = curx;
			oldy = cury;

			if (skip_lines > 0)
				div_pix = FileWindLineUp (wp);
			else if (skip_lines < 0)
				div_pix = FileWindLineDown (wp);
			
			if (div_pix)
			{
				my += div_pix;
				oldy += div_pix;
			}
			
			rect->g_x = min (mx, oldx);
			rect->g_y = min (my, oldy);
			rect->g_w = max (mx, oldx) - rect->g_x + 1;
			rect->g_h = max (my, oldy) - rect->g_y + 1;

			if (hotRubber (wp, rect))
			{
				flushredraw ();
				WindowUpdateData (wp);
			}

			drawRect2 (mx, my, oldx, oldy);
			GrafMouse (M_ON, NULL);
		}
	
	}

	GrafMouse (M_OFF, NULL);
	drawRect2 (mx, my, oldx, oldy);
	GrafMouse (num, &form);
	GrafMouse (M_ON, NULL);
	rect->g_x = min (mx, oldx);
	rect->g_y = min (my, oldy);
	rect->g_w = max (mx, oldx) - rect->g_x + 1;
	rect->g_h = max (my, oldy) - rect->g_y + 1;

	WindUpdate (END_MCTRL);

	if (((wp = GetTopWindInfoOfKind (WK_CONSOLE)) != NULL) 
		&& wp->window_moved)
	{
		wp->window_moved (wp);
	}
}


/*
 * let user mark icons with rubberbox
 */
static void doRubberBox (WindInfo *wp, word mx, word my)
{
	GRECT box;

	rubberBox (wp, mx, my, &box);
	UnMarkDraggedObjects (wp);
	flushredraw ();
}

/*
 * manage different types of action the user may want to 
 * do with icons (selecting,  dragging etc.)
 */
void doIcons (WindInfo *wp, word mx, word my, word kstate)
{
	word bstate, foo, whandle;
	word fobj, tox, toy, towind, toobj;
	OBJECT *tree;


	if (wp->window_get_tree)
		tree = wp->window_get_tree (wp);
	else
		tree = NULL;
	
	if (tree)
		fobj = FindObject (tree, mx, my);
	else
		fobj = -1;
		
	WindUpdate (BEG_MCTRL);

	if (fobj > 0)
	{
		if (kstate & (K_LSHIFT | K_RSHIFT))  /* Shifttaste gedrÅckt */
		{
			/* alle in anderen Windows deselektieren
			 */
			DeselectAllExceptWindow (wp);
			if (tree[fobj].ob_state & SELECTED)
			{
				word tx, ty;
				
				DeselectObject (wp, fobj, TRUE);
				flushredraw ();
				
				tx = mx; ty = my;
				
				while (!ButtonReleased (&tx, &ty, &bstate, &foo))
					;
			}
			else
				SelectObject (wp, fobj, TRUE);		/* selektieren */

			flushredraw ();
		}
		else if (!isSelected (tree, fobj))
		{
			/* alle Icons deselektieren
			 */
			DeselectAllObjects ();
			/* nur dies selektieren
			 */
			SelectObject (wp, fobj, TRUE);
			flushredraw ();
		}

		
		ButtonPressed (&foo, &foo, &bstate, &foo);

		/* ICON war selektiert  und linke Maustaste gedrÅckt
		 */	
		if (isSelected (tree, fobj)	&& (bstate & 0x0001))
		{
			word kstate;
			
			/* ziehe Icons Åbern Tisch
			 */
			if (dragIcons (mx, my, &tox, &toy, &towind, &toobj, 
					&kstate))
			{
				whandle = wp->handle;
			
				WindUpdate (END_MCTRL);

				doDragLogic (wp, towind, toobj, kstate, mx, my, 
					tox, toy);

				WindUpdate (BEG_MCTRL);
			
				/*
				 * programm kann gestartet worden
				 * sein, deshalb pointer neu holen
				 */
				if ((wp = GetWindInfo (whandle)) != NULL)
					UnMarkDraggedObjects (wp);
			}
		}
	}
	else
	{
		word rubber;
		
		ButtonPressed (&foo, &foo, &bstate, &kstate);
		rubber = bstate & 0x0001;

		DeselectAllExceptWindow (wp);

		/* keine Shifttaste gedrÅckt?
		 */
		if (!(kstate & (K_LSHIFT|K_RSHIFT)))
		{
			DeselectAll (wp);
			/* MenÅleiste toppen! */
			if (MagiCVersion >= 0x350)
				wind_set (-1, WF_TOP, -1);
			else if (CookiePresent ('nAES', NULL))
				appl_control (apid, APC_TOP, NULL);
		}

		if (rubber)
		{
			if (PointInGRECT (mx, my, &wp->work))
			{
				doRubberBox (wp, mx, my);
			}
		}
	}

	WindUpdate (END_MCTRL);
	
	UpdateAllWindowData ();
}


