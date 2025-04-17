/*
 * @(#) Gemini\order.c
 * @(#) Stefan Eissing, 27. Dezember 1993
 *
 * description: order icons on the desktop
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "divbox.rh"

#include "window.h"
#include "order.h"
#include "venuserr.h"
#include "gemtrees.h"
#include "util.h"
#include "redraw.h"

/* externals
 */
extern OBJECT *pdivbox;

/* internal texts
 */
#define NlsLocalSection "Gmni.order"
enum NlsLocalText{
T_NOMEM,	/*Es ist nicht genug Speicher vorhanden, um den
 Hintergrund aufzurÑumen.*/
};
				

word doDivOptions (void)
{
	word retcode, changed, ok;
	DIALINFO d;

	setSelected (pdivbox, SNAPWIND, ShowInfo.aligned);
	setSelected (pdivbox, DIVLOWER, NewDesk.preferLowerCase);
	setSelected (pdivbox, SHOWHIDD, NewDesk.showHidden);
	setSelected (pdivbox, SNAPALWA, NewDesk.snapAlways);

	DialCenter (pdivbox);
	DialStart (pdivbox,&d);
	DialDraw (&d);

	do
	{
		changed = FALSE;
		ok = TRUE;
		retcode = DialDo (&d, 0) & 0x7FFF;
		setSelected (pdivbox, retcode, FALSE);
	
		if (retcode == DIVOK)
		{	
			word hidden, align, lower;
			
			hidden = isSelected (pdivbox, SHOWHIDD);
			align = isSelected (pdivbox, SNAPWIND);
			lower = isSelected (pdivbox, DIVLOWER);
			
			changed = ((hidden != NewDesk.showHidden)
				|| (lower != NewDesk.preferLowerCase));

			NewDesk.showHidden = hidden;
			ShowInfo.aligned = align;
			NewDesk.snapAlways = isSelected (pdivbox, SNAPALWA);
			NewDesk.preferLowerCase = lower;
		}
		
		fulldraw (pdivbox, retcode);
	}
	while (!ok);

	DialEnd (&d);

	return changed;
}


typedef struct
{
	char *raster;
	word max_width;
	word max_height;
	word rows;
	word columns;
	WindInfo *wp;
	OBJECT *tree;
} OrderInfo;

static OrderInfo *getNewRaster (WindInfo *wp)
{
	OrderInfo *o;
	OBJECT *tree;
	word i, mw, mh, r, c;
	
	if (wp->window_get_tree == NULL)
		return NULL;
	
	tree = wp->window_get_tree (wp);
	mw = NewDesk.snapx;
	mh = NewDesk.snapy;

	if (mw < 8)
		mw = 8;
	if (mh < 8)
		mh = 8;
	
	c = tree[0].ob_width / mw;
	i = tree[0].ob_width % mw;
	if (i)
		mw += i / c;
		
	r = tree[0].ob_height / mh;
	i = tree[0].ob_height % mh;
	if (i)
		mh += i / r;
	
	o = malloc (sizeof (OrderInfo) + (r * c));
	if (o != NULL)
	{
		o->raster = ((char *)o) + sizeof (OrderInfo);
		memset (o->raster, 0, r * c);
		o->max_width = mw;
		o->max_height = mh;
		o->rows = r;
		o->columns = c;
		o->wp = wp;
		o->tree = tree;
	}
	
	return o;
}


static void markRasterForObject (OrderInfo *o, word obj)
{
	word center_x, center_y;
	word in_row, in_column;
	word raster_index, icon_weight;
	OBJECT *po = &o->tree[obj];
	
	center_x = po->ob_x + (po->ob_width / 2);
	center_y = po->ob_y + (po->ob_height / 2);
	
	in_column = center_x / o->max_width;
	in_row = center_y / o->max_height;
	
	raster_index = (in_row * o->columns) + in_column;
	icon_weight = (po->ob_height > (o->max_height / 2))? 0 : 3;
	if (icon_weight == 3)
	{
		center_y = (in_row * o->max_height);
		if (po->ob_y >= (center_y + (o->max_height / 2)))
		{
			/* Objekt lag eher in der unteren HÑlfte
			 */
			icon_weight = 1;
		}
	}
	
	/* Markiere Rasterpunkt als belegt.
	 */
	if (o->raster[raster_index] != 0)
		o->raster[raster_index] = 4;
	else
		o->raster[raster_index] += 4 - icon_weight;
	
}


static void placeObjectInRaster (OrderInfo *o, word obj, int redraw)
{
	word center_x, center_y;
	word new_x, new_y;
	word in_row, in_column;
	word raster_index, icon_weight;
	OBJECT *po = &o->tree[obj];
	
	center_x = po->ob_x + (po->ob_width / 2);
	center_y = po->ob_y + (po->ob_height / 2);
	
	in_column = center_x / o->max_width;
	in_row = center_y / o->max_height;
	
	raster_index = (in_row * o->columns) + in_column;
	icon_weight = (po->ob_height > (o->max_height / 2))? 0 : 3;
	
	if (o->raster[raster_index] > icon_weight)
	{
		/* Dieser Rasterpunkt ist schon belegt. Wir mÅssen
		 * einen neuen suchen.
		 */

		/* Teste erst unten, dann rechts, dann oben, dann links
		 */
		if (in_row < (o->rows - 1)
			&& o->raster[raster_index + o->columns] <= icon_weight)
		{
			++in_row;
		}
		else if (in_column < (o->columns - 1)
			&& o->raster[raster_index + 1] <= icon_weight)
		{
			++in_column;
		}
		else if (in_row > 0
			&& o->raster[raster_index - o->columns] <= icon_weight)
		{
			--in_row;
		}
		else if (in_column > 0
			&& o->raster[raster_index - 1] <= icon_weight)
		{
			--in_column;
		}
		else
		{
			/* Im Umkreis nichts frei, was nun?
			 */
			int j, max = o->rows * o->columns;
			
			for (j = 0; j < max; ++j)
			{
				if (o->raster[j] <= icon_weight)
				{
					in_row = j / o->columns;
					in_column = j % o->columns;
					break;
				}
			}
		}
	}
	
	center_x = (in_column * o->max_width);
	center_y = (in_row * o->max_height);
	raster_index = (in_row * o->columns) + in_column;
	
	if (icon_weight == 0)
	{
		center_x += (o->max_width / 2);
		center_y += (o->max_height / 2);
		new_x = center_x - (po->ob_width / 2);
		new_y = center_y - (po->ob_height / 2);
	}
	else
	{
		word old_y = po->ob_y;
		
		new_x = center_x;
		new_y = center_y;

		if (o->raster[raster_index] == 0)
		{
			/* Wert == 0: Das Feld ist frei
			 */
			if (old_y >= (center_y + (o->max_height / 2)))
			{
				/* Objekt lag eher in der unteren HÑlfte
				 */
				new_y += (o->max_height / 2);
				icon_weight = 1;
			}
			else
				icon_weight = 3;
		}
		else if (o->raster[raster_index] == 1)
		{
			/* Wert == 1: Die obere HÑlfte ist belegt
			 */
			new_y += (o->max_height / 2);
			icon_weight = 1;
		}
		else
			icon_weight = 3;
		
		new_y += ((o->max_height / 2) - po->ob_height) / 2;
	}

	{
		OBJECT o;
		
		o = *po;
		o.ob_x = new_x;
		o.ob_y = new_y;
		deskAlign (&o);
		new_x = o.ob_x;
		new_y = o.ob_y;
	}
	
	if (redraw)
	{
		redrawObj (o->wp, obj);
		po->ob_x = new_x;
		po->ob_y = new_y;
		redrawObj (o->wp, obj);
		flushredraw ();
	}
	else
	{
		po->ob_x = new_x;
		po->ob_y = new_y;
	}

	/* Markiere Rasterpunkt als belegt.
	 */
	o->raster[(in_row * o->columns) + in_column] += 4 - icon_weight;
}


void SnapObject (WindInfo *wp, word objnr, int redraw)
{
	int i;
	OrderInfo *o;

	o = getNewRaster (wp);
	if (o == NULL)
	{
		venusErr (NlsStr (T_NOMEM));
		return;
	}

	for (i = o->tree[0].ob_head; i > 0; 
		i = o->tree[i].ob_next)
	{
		if ((i == objnr) || isHidden (o->tree, i))
			continue;
		
		markRasterForObject (o, i);
	}
	
	placeObjectInRaster (o, objnr, redraw);
	
	free (o);
}


void orderDeskIcons (void)
{
	int i, only_selected;
	OrderInfo *o;

	o = getNewRaster (&wnull);
	if (o == NULL)
	{
		venusErr (NlsStr (T_NOMEM));
		return;
	}

	only_selected = wnull.selectAnz > 0;
	
	for (i = NewDesk.tree[0].ob_head; i > 0; 
		i = NewDesk.tree[i].ob_next)
	{
		if (isHidden (NewDesk.tree, i))
			continue;
		
		if (isSelected (NewDesk.tree, i))
			continue;
		
		if (only_selected)
			markRasterForObject (o, i);
		else
			placeObjectInRaster (o, i, TRUE);
	}
	
	if (only_selected)
	{
		for (i = NewDesk.tree[0].ob_head; i > 0; 
			i = NewDesk.tree[i].ob_next)
		{
			if (isHidden (NewDesk.tree, i))
				continue;
		
			if (!isSelected (NewDesk.tree, i))
				continue;
			
			placeObjectInRaster (o, i, TRUE);
		}
	}
	
	free (o);
}