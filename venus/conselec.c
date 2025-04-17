/*
 * @(#) Gemini\conselec.c
 * @(#) Stefan Eissing, 11. MÑrz 1993
 *
 * description: selection in console window
 *
 * jr 21.4.95
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vdi.h>
#include <aes.h>
#include <tos.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"

#include "window.h"
#include "conwind.h"

#include "stand.h"
#include "clipbrd.h"
#include "conselec.h"
#include "termio.h"
#include "wintool.h"
#include "draglogi.h"
#include "conselec.rh"
#include "venuserr.h"
#include "util.h"
#include "fileutil.h"
#include "redraw.h"

#if STANDALONE
#error conselec.c compilieren in STANDALONE?
#endif


/* externals
 */
extern OBJECT *pconselec;


/* internals
 */
static word maxcol, maxrow, rancx, lancx, ancy;


static void invertLine (TermInfo *terminal, const GRECT *work, 
	word x1, word x2, word y)
{
	word pxy[4], rect[4];
	word count;
	
	pxy[0] = work->g_x + terminal->font_width * x1;
	pxy[1] = work->g_y + terminal->font_height * y;
	pxy[2] = work->g_x + (terminal->font_width * (x2+1)) - 1;
	pxy[3] = pxy[1] + terminal->font_height - 1;
	
    count = 0;
    while (WT_GetRect (count++, rect))
	{
		if (VDIRectIntersect (pxy, rect))
			vr_recfl (vdi_handle, rect); 
	}
}

static int isHigher (word x1, word y1, word x2, word y2)
{
	if (y1 == y2)
		return x1 > x2;
	else
		return y1 > y2;
}

static int isLower (word x1, word y1, word x2, word y2)
{
	if (y1 == y2)
		return x1 < x2;
	else
		return y1 < y2;
}

static void dec (word x1, word y1, word *x, word *y)
{
	if (x1 || y1)
	{
		--x1;
		if (x1 < 0)
		{
			x1 = maxcol;
			--y1;
		}
		*x = x1;
		*y = y1;
	}
}

static void inc (word x1, word y1, word *x, word *y)
{
	if ((x1 == maxcol) && (y1 == maxrow))
		return;
	
	++x1;
	if (x1 > maxcol)
	{
		x1 = 0;
		++y1;
	}
	*x = x1;
	*y = y1;
}

static void mouse2Cells (TermInfo *terminal, const GRECT *work, 
	word mx, word my, word *cx, word *cy)
{
	word checkright, checkleft;
	
	checkright = checkleft = FALSE;
	
	if (mx < work->g_x)
	{
		checkleft = TRUE;
		mx = 0;
	}
	else if (mx >= work->g_x + work->g_w)
	{
		checkright = TRUE;
		mx = work->g_w - 1;
	}
	else
		mx -= work->g_x;
	
	if (my < work->g_y)
		my = 0;
	else if (my >= work->g_y + work->g_h)
		my = work->g_h - 1;
	else
		my -= work->g_y;
	
	*cx = mx / terminal->font_width;
	*cy = my / terminal->font_height;
	
	if (checkleft)
	{
		if (isHigher (*cx, *cy, rancx, ancy))
			dec (*cx, *cy, cx, cy);
	}
	else if (checkright)
	{
		if (isLower (*cx, *cy, lancx, ancy))
			inc (*cx, *cy, cx, cy);
	}
}

static void invertBetween (TermInfo *terminal, const GRECT *work, 
	word x1, word y1, word x2, word y2)
{
	GrafMouse (M_OFF, NULL);
	
	if (isHigher (x1, y1, x2, y2))
	{
		word tmp;		/* Punkte vertauschen */
		
		tmp = y1; y1 = y2; y2 = tmp;
		tmp = x1; x1 = x2; x2 = tmp;
	}
	
	if (y1 == y2)			/* selbe Zeile */
	{
		invertLine (terminal, work, x1, x2, y1);
	}
	else
	{
		word i;
		
		invertLine (terminal, work, x1, maxcol, y1);	/* erste Zeile */
		for (i = y1 + 1; i < y2; ++i)
		{
			invertLine (terminal, work, 0, maxcol, i);
		}
		invertLine (terminal, work, 0, x2, y2);
	}

	GrafMouse (M_ON, NULL);
}

#define GO_RIGHT	0
#define GO_LEFT		1

static word go2Word (TermInfo *terminal, word x, word y, word direction)
{
	word lastx;
	char c;

	c = ti_sline (terminal, y)[x];
	lastx = x;
	while ((x >= 0) && (x < terminal->columns) 
			&& (isalnum (c) || strchr ("._-+ÑÅîéöôû", c)))
	{
		lastx = x;
		(direction == GO_LEFT)? --x : ++x;
		c = ti_sline (terminal, y)[x];
	}
	
	return lastx;
}

static void setAnchor (TermInfo *terminal, word cx, word cy, word doubleClick)
{
	ancy = cy;
	if (doubleClick)
	{
		rancx = go2Word (terminal, cx, cy, GO_RIGHT);
		lancx = go2Word (terminal, cx, cy, GO_LEFT);
	}
	else
		lancx = rancx = cx;
}

static void cell2Word (TermInfo *terminal, word cx, word cy, word *zx, word *zy)
{
	word direction;
	
	direction = isHigher (cx, cy, rancx, ancy)? GO_RIGHT : GO_LEFT;

	*zx = go2Word (terminal, cx, cy, direction);
	*zy = cy;
}

/* jr: Attributbits vom internen Format zum GEM-Format wandeln */

static int conv_attr (TermInfo *terminal, char *str, size_t len)
{
	int changed = 0;

	while (len--)
	{
		int mask = *str, bits = 0;
		
		if (mask & V_STANDOUT) mask |= terminal->inv_type;

		if (mask & V_BOLD) bits |= 1;
		if (mask & V_LIGHT) bits |= 2;
		if (mask & V_ITALIC) bits |= 4;
		if (mask & V_UNDERLINE) bits |= 8;
		
		*str++ = bits;
		changed |= bits;
	}
	
	return changed;
}

/* Textstring in Puffer kopieren. Wenn `attr_string' gesetzt
   ist, werden auch die Attribute ermittelt. Wenn sowieso
   keine gesetzt sind, wird dort ein Nullzeiger zurÅckgeliefert. */ 

static char *getString (TermInfo *terminal, word x1, word y1, 
	word x2, word y2, word newline, char **attr_string)
{
	size_t len = 1;
	char *string;
	int changed = 0;
	
	if (isHigher (x1, y1, x2, y2))
	{
		word tmp;
		
		tmp = y1; y1 = y2; y2 = tmp;
		tmp = x1; x1 = x2; x2 = tmp;
	}
	
	/* get the length of selected string */
	{
		word i;
		
		for (i = y1; i <= y2; i++)
		{
			word start = 0;
			word end = (word) strlen (ti_sline (terminal, i)) - 1;

			if (i == y1) start = x1;
			if (i == y2) end = x2;
			
			len += end - start + 1;
			if (newline) len += 1;
		}
	}
	
	/* keine Zeichen selektiert
	 */
	if (len == 1) return NULL;
	
	/* Wir allozieren ein Byte mehr, damit wir bei CONSEXEC noch
	 * ein Newline anhÑngen kînnen.
	 */
	string = malloc (len + 2);
	if (!string) return NULL;
	
	if (attr_string) *attr_string = malloc (len + 2);

	{
		word i;
		char *d = string;
		char *e = NULL;

		if (attr_string) e = *attr_string;
		
		for (i = y1; i <= y2; i++)
		{
			word start = 0;
			word end = (word) strlen (ti_sline (terminal, i)) - 1;

			if (i == y1) start = x1;
			if (i == y2) end = x2;
			
			strncpy (d, ti_sline (terminal, i) + start, end - start + 1);
			d += end - start + 1;
			
			if (i != y2)
			{
				if (newline) *d++ = '\n';
			}
			else
				*d++ = '\0';
			
			if (e)
			{
				memcpy (e, ti_aline (terminal, i) + start, end - start + 1);
				changed |= conv_attr (terminal, e, end - start + 1);
				e += end - start + 1;
				
				if (i != y2)
				{
					if (newline) *e++ = '\n';
				}
				else
					*e++ = '\0';
			}
		}
	}
	
	if (attr_string && *attr_string && !changed)
	{
		free (*attr_string);
		*attr_string = NULL;
	}
	
	return string;
}

static void sendString (TermInfo *terminal, word x1, word y1, 
	word x2, word y2, int exec)
{
	char *string;
	
	string = getString (terminal, x1, y1, x2, y2, FALSE, NULL);
	if (string)
	{
		if (exec)
			strcat (string, "\r");

		PasteStringToConsole (string);
	}
}

static void clipString (TermInfo *terminal, word x1, word y1, 
	word x2, word y2)
{
	char *string, *attribs;
	
	string = getString (terminal, x1, y1, x2, y2, TRUE, &attribs);
	if (!string) return;

	PasteString (string, TRUE, attribs);
		
	free (string);
	if (attribs) free (attribs);
}

static void doPopResult (TermInfo *terminal, word index, 
	word fx, word fy, word tx, word ty)
{
	switch (index)
	{
		case CONSCOPY:
			clipString (terminal, fx, fy, tx, ty);
			break;
		case CONSPAST:
		case CONSEXEC:
			sendString (terminal, fx, fy, tx, ty, index == CONSEXEC);
			break;
		default:
			break;	
	}
}

void ConSelection (word con_window_handle, word mx, word my, 
	word kstate, int doubleClick)
{
	WindInfo *wp;
	ConsoleWindow_t *cwp;
	TermInfo *terminal;
	word bstate;
	word prevx, prevy;
	word curx, cury;
	word tmpx, tmpy;
	word anchorx;
	GRECT work;
	OBJECT *poptree;
	word popindex;
	
	wp = GetWindInfo (con_window_handle);
	if ((wp == NULL) || (wp->kind != WK_CONSOLE))
	{
		venusInfo ("no con wind");
		return;
	}
	
	cwp = wp->kind_info;
	terminal = cwp->terminal;
	work = wp->work;
		
	if (!PointInGRECT (mx, my, &work))
		return;

	maxcol = terminal->columns - 1;
	maxrow = terminal->rows - 1;
	
	/* VDI-Attribte setzen
	 */
	vswr_mode (vdi_handle, MD_XOR);
	vsf_color (vdi_handle, 1);
	vsf_interior (vdi_handle, FIS_SOLID);
	{
		word pxy[4];
		
		vs_clip (vdi_handle, 0, pxy);
	}
	
	mouse2Cells (terminal, &work, mx, my, &prevx, &prevy);
	setAnchor (terminal, prevx, prevy, doubleClick);
	prevx = lancx;
	
	WT_BuildRectList (con_window_handle);
	GrafMouse (M_OFF, NULL);
	rem_cur (terminal);
	GrafMouse (M_ON, NULL);
	
	invertBetween (terminal, &work, lancx, ancy, rancx, ancy);
	
	mouse2Cells (terminal, &work, mx, my, &curx, &cury);
	
	while (ButtonPressed (&mx, &my, &bstate, &kstate))
	{
		if (doubleClick)
			cell2Word (terminal, curx, cury, &curx, &cury);
		 
		if ((curx != prevx) || (cury != prevy))
		{
			if (isHigher (prevx, prevy, rancx, ancy))
			{
				if (isHigher (curx, cury, rancx, ancy))
				{
					if (isHigher (curx, cury, prevx, prevy))
					{
						inc (prevx, prevy, &tmpx, &tmpy);
						invertBetween (terminal, &work, 
							tmpx, tmpy, curx, cury);
					}
					else
					{
						inc (curx, cury, &tmpx, &tmpy);
						invertBetween (terminal, &work, 
							tmpx, tmpy, prevx, prevy);
					}
				}
				else
				{
					inc (rancx, ancy, &tmpx, &tmpy);
					invertBetween (terminal, &work, 
						tmpx, tmpy, prevx, prevy);
					if (isLower (curx, cury, lancx, ancy))
					{
						dec (lancx, ancy, &tmpx, &tmpy);
						invertBetween (terminal, &work, 
							curx, cury, tmpx, tmpy);
					}
				}
			}
			else if (isLower (prevx, prevy, lancx, ancy))
			{
				if (isLower (curx, cury, lancx, ancy))
				{
					if (isLower (curx, cury, prevx, prevy))
					{
						dec (prevx, prevy, &tmpx, &tmpy);
						invertBetween (terminal, &work, 
							tmpx, tmpy, curx, cury);
					}
					else
					{
						dec (curx, cury, &tmpx, &tmpy);
						invertBetween (terminal, &work, 
							prevx, prevy, tmpx, tmpy);
					}
				}
				else
				{
					dec (lancx, ancy, &tmpx, &tmpy);
					invertBetween (terminal, &work, 
						tmpx, tmpy, prevx, prevy);
					if (isHigher (curx, cury, rancx, ancy))
					{
						inc (rancx, ancy, &tmpx, &tmpy);
						invertBetween (terminal, &work, 
							curx, cury, tmpx, tmpy);
					}
				}
			}
			else	/* prev und anc sind gleich */
			{
				if (isHigher (curx, cury, prevx, prevy))
				{
					inc (rancx, ancy, &tmpx, &tmpy);
					invertBetween (terminal, &work, 
						tmpx, tmpy, curx, cury);
				}
				else
				{
					dec (lancx, ancy, &tmpx, &tmpy);
					invertBetween (terminal, &work, 
						tmpx, tmpy, curx, cury);
				}
			}
			
			prevx = curx;
			prevy = cury;
		}
		
		mouse2Cells (terminal, &work, mx, my, &curx, &cury);
	}
	
	JazzUp (pconselec, -1, -1, 1, CONSPAST, 0, &poptree, &popindex);

	if (isHigher (prevx, prevy, rancx, ancy))
		anchorx = lancx;
	else
		anchorx = rancx;
	invertBetween (terminal, &work, anchorx, ancy, prevx, prevy);
	
	GrafMouse (M_OFF, NULL);
	disp_cur (terminal);
	GrafMouse (M_ON, NULL);
	if (poptree)
	{
		setSelected (poptree, popindex, FALSE);
		doPopResult (terminal, popindex, anchorx, ancy, prevx, prevy);
	}
}
