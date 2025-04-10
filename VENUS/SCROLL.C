/*
 * @(#) Gemini\scroll.c
 * @(#) Stefan Eissing, 28. Januar 1995
 *
 * description: Routinen zum Scrolling in Desktopwindows 
 */

#include <string.h> 
#include <ctype.h> 
#include <vdi.h>
#include <flydial\flydial.h>

#include "vs.h"
#include "menu.rh"

#include "window.h"
#include "filewind.h"
#include "filedraw.h"
#include "myalloc.h"
#include "redraw.h"
#include "scroll.h"
#include "select.h"
#include "timeevnt.h"
#include "util.h"
#include "venuserr.h"


static void setScrollData (WindInfo *wp)
{
	/* neues Update setzen
	 */
	wp->update |= (WU_VESLIDER|WU_DISPLAY);
	WindowUpdateData (wp);
}

int FileWindowScroll (WindInfo *wp, word div_skip, word dodraw)
{
	FileWindow_t *fwp = wp->kind_info;
	MFDB src_mfdb, dest_mfdb;
	word srcr[4], pxy[256], screen[4];
	word div_pix, wind_rect[4];
	GRECT r;
	register word i;
	
	if (wp->kind != WK_FILE)
		return 0;
		
	/* Um wieviel Pixel muž gescrollt werden?
	 */	
	div_pix = div_skip * (fwp->stdobj.ob_height + 1);

	if (!dodraw)
	{
		setScrollData (wp);
		return div_pix;
	}

	/* Clipping auf Bildschirmgrenze einschalten
	 */
	screen[0] = Desk.g_x;
	screen[1] = Desk.g_y;
	screen[2] = Desk.g_x + Desk.g_w - 1;
	screen[3] = Desk.g_y + Desk.g_h - 1;

	GrafMouse (M_OFF, NULL);

	setScrollData (wp);

	wind_get (wp->handle, WF_FIRSTXYWH, &wind_rect[0], &wind_rect[1],
		&wind_rect[2], &wind_rect[3]);

	while (wind_rect[2] && wind_rect[3])
	{
		wind_rect[2] += (wind_rect[0] - 1);
		wind_rect[3] += (wind_rect[1] - 1);

		if (!VDIRectIntersect (screen, wind_rect))
		{
			wind_get (wp->handle, WF_NEXTXYWH, &wind_rect[0], 
				&wind_rect[1], &wind_rect[2], &wind_rect[3]);
			continue;
		}
		
		vs_clip (vdi_handle, FALSE, wind_rect);
			
		/* Blitten auf dem Bildschirm
		 */
		src_mfdb.fd_addr = dest_mfdb.fd_addr = NULL;
		
		r.g_x = srcr[0] = pxy[4] = wind_rect[0];
		r.g_w = wind_rect[2] - wind_rect[0] + 1;
		srcr[2] = pxy[6] = wind_rect[2];
	
		if (div_skip > 0)				/* scrolling up */
		{
			r.g_y = srcr[1] = wind_rect[1];
			srcr[3] = wind_rect[3] - div_pix;
			r.g_h = (wind_rect[3] - wind_rect[1] + 1) - (srcr[3] - srcr[1]);
		}
		else							/* scrolling down */
		{
			srcr[1] = wind_rect[1] - div_pix;
			srcr[3] = wind_rect[3];
			r.g_y = wind_rect[1] + (srcr[3] - srcr[1]);
			r.g_h = (wind_rect[3] - wind_rect[1] + 1) - (srcr[3] - srcr[1]);
		}
		
		if(srcr[1] < srcr[3])			/* ist noch was zu sehen */
		{
			for (i = 0; i < 4; i++)
				pxy[i] = srcr[i];
					
			pxy[5] = srcr[1] + div_pix;
			pxy[7] = srcr[3] + div_pix;
				
			if (VDIRectIntersect (wind_rect, pxy)
				&& VDIRectIntersect (wind_rect, &pxy[4]))
			{
				vro_cpyfm (vdi_handle, 3, pxy, &src_mfdb, &dest_mfdb);
			}
		
			FileWindowDrawRect (fwp, &r);
		}
		else
		{
			GRECT wind;
			
			wind.g_x = wind_rect[0];
			wind.g_y = wind_rect[1];
			wind.g_w = wind_rect[2] - wind_rect[0] + 1;
			wind.g_h = wind_rect[3] - wind_rect[1] + 1;
			FileWindowDrawRect (fwp, &wind);
		}

		wind_get (wp->handle, WF_NEXTXYWH, &wind_rect[0], 
			&wind_rect[1], &wind_rect[2], &wind_rect[3]);
	}

	GrafMouse (M_ON, NULL);
	return div_pix;
}

int FileWindLineDown (WindInfo *wp)
{
	FileWindow_t *fwp;
	if (wp->kind != WK_FILE)
		return 0;
	fwp = wp->kind_info;
	
	if ((wp->vslpos < 1000) && (wp->vslsize < 1000))
	{
		fwp->yskip++;
		return FileWindowScroll (wp, -1, TRUE);
	}
	return 0;
}

int FileWindLineUp (WindInfo *wp)
{
	FileWindow_t *fwp;
	if (wp->kind != WK_FILE)
		return 0;
	fwp = wp->kind_info;
	
	if (fwp->yskip)
	{
		fwp->yskip--;
		return FileWindowScroll (wp, 1, TRUE);
	}
	return 0;
}

int FileWindPageUp (WindInfo *wp)
{
	word seen_lines, old_skip;
	FileWindow_t *fwp;
	if (wp->kind != WK_FILE)
		return 0;
	fwp = wp->kind_info;	
	
 	if (wp->vslpos > 0 && wp->vslsize < 1000)
	{
		seen_lines = wp->work.g_h / (fwp->stdobj.ob_height + 1);
		old_skip = fwp->yskip;
	
		if (fwp->yskip > seen_lines)
			fwp->yskip -= seen_lines;
		else
			fwp->yskip = 0;

		if (old_skip != fwp->yskip)
			return FileWindowScroll (wp, old_skip - fwp->yskip, TRUE);
	}
	return 0;
}

int FileWindPageDown (WindInfo *wp)
{
	word total_lines, seen_lines;
	word old_skip;
	FileWindow_t *fwp;
	if (wp->kind != WK_FILE)
		return 0;
	fwp = wp->kind_info;
	
	if (wp->vslpos < 1000 && wp->vslsize < 1000)
	{
		total_lines = fwp->fileanz / fwp->xanz;
		if (fwp->fileanz % fwp->xanz)
			total_lines++;
		seen_lines = wp->work.g_h / (fwp->stdobj.ob_height + 1);
		old_skip = fwp->yskip;	

		if (fwp->yskip + seen_lines <= total_lines - seen_lines)
			fwp->yskip += seen_lines;
		else
			fwp->yskip = total_lines - seen_lines;

		if (old_skip != fwp->yskip)
			return FileWindowScroll (wp, old_skip - fwp->yskip, TRUE);
	}
	return 0;
}

void DoArrowed (word whandle, word desire)
{
	WindInfo *wp;
	
	if ((wp = GetWindInfo (whandle)) == NULL)
		return;

	switch (desire)
	{
		case WA_UPPAGE:		/* Seite aufw„rts */
			FileWindPageUp (wp);
			break;

		case WA_DNPAGE:		/* Seite abw„rts */
			FileWindPageDown (wp);
			break;

		case WA_UPLINE:		/* Zeile aufw„rts */
			FileWindLineUp (wp);
			break;

		case WA_DNLINE:		/* Zeile abw„rts */
			FileWindLineDown (wp);
			break;
	}
}

void DoVerticalSlider (word whandle, word position)
{
	WindInfo *wp;
	FileWindow_t *fwp;
	word lost_lines, total_lines, seen_lines;
	word old_skip, new_yskip;

	if (((wp = GetWindInfo (whandle)) == NULL)
		|| (wp->kind != WK_FILE))
		return;
	
	fwp = wp->kind_info;
	
	if ((fwp->fileanz) && (position != wp->vslpos))
	{
		seen_lines = wp->work.g_h / (fwp->stdobj.ob_height + 1);
		total_lines = fwp->fileanz / fwp->xanz;
		if (fwp->fileanz % fwp->xanz)
			total_lines++;

		lost_lines = total_lines - seen_lines;
		if (lost_lines > 0)
		{
			new_yskip = (word)((long)position * lost_lines / 1000L);
			if (((long)position * lost_lines) % 1000L > 499L)
				new_yskip++;
				
			if (new_yskip != fwp->yskip)
			{
				old_skip = fwp->yskip;
				fwp->yskip = new_yskip;
				FileWindowScroll (wp, old_skip - new_yskip, TRUE);
			}
		}
	}
}


/* Diese Funktione berechnet die Zeile, zu der gesprungen werden
 * darf, damit die Zeile <toskip> ganz sichtbar ist. <work> 
 * bezeichnet die "Innenmaže" des Fensters <fwp>
 */
word CalcYSkip (FileWindow_t *fwp, GRECT *work, int toskip)
{
	word xanz, yanz, fullines;
	word skip, leftfiles, gesamtanz;

	/* xanz = Wieviele Objekte passen in eine Zeile?
	 */
	xanz = work->g_w / (fwp->stdobj.ob_width + fwp->xdist);
	if (!xanz && fwp->fileanz)
		xanz++;
	
	/*	fullines = Wieviele Zeilen sind _ganz_ sichtbar?
	 *  yanz = Wieviele Zeilen passen (auch teilweise) ins Fenster?
	 */	
	fullines = yanz = work->g_h / (fwp->stdobj.ob_height + 1);
	if (work->g_h % (fwp->stdobj.ob_height + 1))
		yanz++;

	/* skip = Anzahl der Objekte, die bis zur Zielzeile liegen
	 */
	skip = xanz * toskip;

	/* gesamtanz = Anzahl der Objekte im Fenster
	 * leftfiles = Anzahl der Objekte unterhalb der Zeile toskip
	 */
	gesamtanz = xanz * yanz;
	leftfiles = fwp->fileanz - skip;

	/* Passen mehr Objekte ins Fenster, als wir noch brig haben?
	 */
	if (gesamtanz > leftfiles)
	{
		/* Ist die Zeile, zu der wir wollen die 0te?
		 */
		if (toskip)
		{
			word usedlines, freelines;
		
			/* usedlines = Wieviele Zeilen brauchen die restlichen
			 * Objekte?
			 */
			usedlines = leftfiles / xanz;
			if ((leftfiles > 0) && (leftfiles % xanz))
				usedlines++;
			
			/* freelines = Anzahl der Zeilen, die nicht durch 
			 * leftfiles gefllt werden k”nnen.
			 */
			freelines = fullines - usedlines;
			
			/* Krze toskip um die Anzahl der leeren Zeilen, so
			 * da welche sind.
			 */	
			if (freelines > 0)
			{
				toskip -= freelines;
				if (toskip < 0)
					toskip = 0;
			}
		}
	}
	
	return toskip;
}


static int clearSearchString (long param)
{
	WindInfo *wp;

	wp = GetWindInfo ((word)param);
	if (wp != NULL && wp->kind == WK_FILE)
	{
		FileWindow_t *fwp = wp->kind_info;
		
		fwp->search_string[0] = '\0';
		wp->update |= WU_WINDINFO;
	}
	return FALSE;
}

int FileWindowCharScroll (WindInfo *wp, char c, word kstate)
{
	FileWindow_t *fwp = wp->kind_info;
	char pattern[WILDLEN];
	size_t len;
	const char *overlap;
	int clearWhenNotFound = 0;
		
	(void)kstate;
	
	if (wp->kind != WK_FILE || fwp->fileanz < 1)
		return FALSE;

	len = strlen (fwp->search_string);
	strcpy (pattern, fwp->search_string);
	
	if (c == 0x1B)
	{
		if (len < 1)
			return FALSE;
			
		fwp->search_string[0] = '\0';
		wp->update |= WU_WINDINFO;
		DeselectAll (wp);
		return TRUE;
	}
	else if (c == '\t')
	{
		if (len < 1)
			return FALSE;
		
		/* fall throught */
	}
	else if ((c == ' ') && (len > 1) 
		&& (fwp->search_string[len-1] == '*'))
	{
		fwp->search_string[len-1] = '\0';
		--len;
		strcpy (pattern, fwp->search_string);
	}
	else
	{
		if (isalpha (c) && !fwp->flags.case_sensitiv)
			c = NewDesk.preferLowerCase? tolower (c) : toupper (c);
		
		if (c == '\b')
		{
			if (len > 1)
			{
				fwp->search_string[len-2] = '*';
				fwp->search_string[len-1] = '\0';
			}
			else if (len == 0)
				return FALSE;
		}
		else
		{
			if (len >= WILDLEN - 2)
			{
				return TRUE;
			}
	
			if (len)
			{
				if (fwp->search_string[len-1] != '*')
					++len;
				fwp->search_string[len-1] = c;
			}
			else
			{
				fwp->search_string[0] = c;
				++len;
			}
			fwp->search_string[len] = '*';
			fwp->search_string[len+1] = '\0';
		}
	}
	
	DeselectAllExceptWindow (wp);
	overlap = StringSelect (wp, fwp->search_string, TRUE);
	if (overlap)
	{
		if (c == '\t')
		{
			char *wild = strpbrk (fwp->search_string, "*?[]");
			if (!wild || (wild[1] == 0))
			{
				if (wp->selectAnz == 1)
					fwp->search_string[0] = 0;
				else if (strlen (overlap) 
					== (strlen (fwp->search_string)-1))
				{
					Bconout (2, '\a');
				}
				else
				{
					strncpy (fwp->search_string, overlap, WILDLEN);
					if (wp->selectAnz > 1)
						strcat (fwp->search_string, "*");
				}
			}
		}
	}
	else
	{
		if (clearWhenNotFound)
		{
			fwp->search_string[0] = 0;
		}
		else
		{
			strcpy (fwp->search_string, pattern);
			StringSelect (wp, fwp->search_string, TRUE);
			Bconout (2, '\a');
		}
	}

	RemoveTimerEvent (wp->handle, clearSearchString);
	InstallTimerEvent (5000L, wp->handle, clearSearchString);
	return TRUE;
}