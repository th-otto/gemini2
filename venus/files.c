/*
 * @(#) Gemini\files.c
 * @(#) Stefan Eissing, 29. Januar 1995
 *
 * description: Filesstuff in windows for venus
 *
 * jr 1.4.95
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <aes.h>
#include <tos.h>

#include "..\common\cookie.h"

#include "vs.h"
#include "stdbox.rh"

#include "window.h"
#include "filewind.h"
#include "files.h"
#include "myalloc.h"
#include "icondraw.h"
#include "iconinst.h"
#include "iconrule.h"
#include "filedraw.h"
#include "select.h"
#include "sortfile.h"


/* externals
 */
extern OBJECT *pstdbox;

static void fetchobj (FileWindow_t *fwp);



static void fillFileUnion (FileWindow_t *fwp, FileInfo *pf)
{
	OBJECT *po;
	char *pc;
	word xdiff, ydiff;
	char color;
	int text_display = (fwp->display == DisplayText);

	pf->flags.text_display = text_display;
	
	switch (fwp->display)
	{
		case DisplayText:
			if (pf->attrib & FA_FOLDER)
				po = GetStdFolderIcon (TextFont.hchar);
			else
				po = NULL;
			color = 0x10;
			break;
			
		case DisplaySmallIcons:
	 		po = GetSmallIconObject (pf->attrib & FA_FOLDER,
								pf->name, &color);
			break;
		
		case DisplayBigIcons:
	 		po = GetBigIconObject (pf->attrib & FA_FOLDER,
								pf->name, &color);
			break;
	}

	pf->icon.mainlist = NULL;
	if (po != NULL)
	{
		char is_folder = pf->icon.flags.is_folder;
		
		FillIconObject (&pf->icon, po);
		pf->icon.flags.is_folder = is_folder;
		pf->icon.flags.is_symlink = pf->flags.is_symlink;
	}
	else
		memset (&pf->icon.icon, 0, sizeof (pf->icon.icon));

	pf->icon.icon.ib_char = (color << 8) | 
		(pf->icon.icon.ib_char & 0x00FF);

	if (fwp->display == DisplayBigIcons)
	{
		pf->icon.flags.is_icon = TRUE;
		
		pf->icon.icon.ib_ptext = pf->name;
		
		/* Setze Offset des Icons im Objekt, damit das Icon unten
		 * zentriert im Fenster erscheint.
		 */
		xdiff = (fwp->stdobj.ob_width - po->ob_width) / 2;
		if (xdiff > 0)
		{
			/* Das Icon ist kleiner als ein Standardobjekt
			 */
			pf->icon.icon.ib_xicon += xdiff;
			pf->icon.icon.ib_xtext += xdiff;
		}
		else if (xdiff < 0)
		{
			/* Das Icon ist gr”žer als ein Standardobjekt
			 * Was tun? stevie
			 */
		}

		ydiff = fwp->stdobj.ob_height - po->ob_height;
		if (ydiff > 0)
		{
			pf->icon.icon.ib_yicon += ydiff;
			pf->icon.icon.ib_ytext += ydiff;
		}

		/* Setze Breite der Icon-Fahne
		 */
		SetIconTextWidth (&pf->icon.icon, NULL);
	}
	else
	{
		FILEFONTINFO *ffi;
		
		ffi = text_display? &TextFont : &SIconFont;
		pf->icon.flags.is_icon = !text_display;

		pf->icon.icon.ib_ptext = NULL;
		
		if (po != NULL)
		{
			/* Setze Offset des Icons im Objekt, damit das Icon links
			 * am Rand erscheint.
			 */
			pf->icon.icon.ib_xicon = 0;
			xdiff = (fwp->icon_max_width
				 - pf->icon.icon.ib_wicon) / 2;
			if (xdiff)
				pf->icon.icon.ib_xicon += xdiff;
		
			ydiff = (fwp->stdobj.ob_height - 
				po->ob_spec.iconblk->ib_hicon) / 2;
			if (ydiff)
				pf->icon.icon.ib_yicon += ydiff;
		}

		/* Flle den Textbereich
		 */
		memset (pf->icon.string, ' ', fwp->max_textname_len - 1);
		pf->icon.string[fwp->max_textname_len - 1] = '\0';

		pf->icon.x = fwp->icon_max_width + fwp->icon_text_distance;
		pf->icon.icon.ib_xtext = pf->icon.x;

		ydiff = (fwp->stdobj.ob_height - ffi->hchar) / 2;
		if (ydiff > 0)
		{
			pf->icon.y = ydiff;
			pf->icon.icon.ib_ytext = pf->icon.y;
		}
		else
		{
			pf->icon.y = 0;
			pf->icon.icon.ib_ytext = 0;
		}

		pc = pf->icon.string;
		{
			size_t len, next_tab;
			char *cp = NULL;

			if (!fwp->flags.is_8and3
				|| (pf->name[0] == '.' && pf->name[1] == '.')
				|| (cp = strrchr (pf->name, '.')) == NULL)
			{
				len = strlen (pf->name);
				next_tab = fwp->max_filename_len + 1;
				pf->icon.freelength = (uword) next_tab;
			}
			else
			{
				len = cp - pf->name;
				next_tab = 9;
				pf->icon.freelength = 0;
			}
			
			strncpy (pc, pf->name, len);
			
			len = max (next_tab, len) + 1;
			
			if (cp != NULL)
			{
				char *t;
				
				pc += len;
				t = pc;
				
				while (*(++cp))
					*(t++) = *cp;
				len = 4;
			}
			
			pc += len;
		}
		
		/* jr: sprintf anstelle hand-conversion, ganz grože
		Dateil„ngen als xxxK anzeigen */
		
		if (ffi->show.size)
		{
			if (pf->size > 99999999L)
				pc += sprintf (pc, "%8luK", pf->size / 1024);
			else if (pf->size || !(pf->attrib & FA_FOLDER))
				pc += sprintf (pc, "%9lu", pf->size);
			else
				pc += sprintf (pc, "         ");
		}

		/* jr: sprintf anstelle hand-conversion, fehlendes
		Datum als Blanks (ein Datum 'fehlt', wenn die
		Jahresangabe ungltig ist */

		if (ffi->show.date)
		{
			*pc++ = ' ';
		
			if (pf->date)
			{
				word year, month, day;

				year = ((pf->date & 0xFE00) >> 9) + 80;
				month = (pf->date & 0x01E0) >> 5;
				day = pf->date & 0x001F;
			
				pc += IDTSprintf (pc, year, month, day);
			}
			else
				pc += sprintf (pc, "         ");
		}

		/* jr: sprintf anstelle hand-conversion, fehlendes
		Datum als Blanks (ein Datum 'fehlt', wenn die
		Jahresangabe ungltig ist */

		if (ffi->show.time)
		{
			if (pf->date)
			{
				word hour, min, sec;

				hour = ((pf->time & 0xF800) >> 11) % 24;
				min = ((pf->time & 0x07E0) >> 5) % 60;
				sec = ((pf->time & 0x001F) << 1) % 60;

				pc += sprintf (pc, " %02d:%02d:%02d", hour, min, sec);			
			}
			else
				pc += sprintf (pc, "         ");
		}
		
		while (*pc == ' ')
			--pc;
		pc[1] = '\0';

		/* Setze Koordinaten des Textes in die Icon-Fahne, damit
		 * unser ObjectFind entsprechend reagiert.
		 */
		pf->icon.icon.ib_wtext = fwp->stdobj.ob_width
			 - pf->icon.x - ffi->kursiv_offset_rechts;
		pf->icon.icon.ib_htext = ffi->hchar; 

		if (!text_display)
		{
			--pf->icon.icon.ib_xtext;
			++pf->icon.icon.ib_wtext;
			++pf->icon.icon.ib_htext;
			--pf->icon.icon.ib_ytext;
		}

		if (pf->icon.flags.is_symlink)
		{
			pf->icon.icon.ib_xtext -= 
				ffi->kursiv_offset_links;
			pf->icon.icon.ib_wtext += 
				ffi->kursiv_offset_links;
		}
	}

	pf->icon.flags.noObject = FALSE;
}

/*
 * get AES objects (icons or text) for files in linked
 * list fwp->files
 */
static int fetchObjects (FileWindow_t *fwp)
{
	FileBucket *bucket;
	OBJECT *po;
	word obw;
	
	fwp->max_textname_len = 0;
	
	if (fwp->display == DisplayBigIcons)
	{
		/* Hole das Standardicon fr ein Objekt und passe die
		 * Breite an den l„ngsten Dateinamen in diesem Verzeichnis
		 * an.
		 */
		po = GetStdBigIcon ();
		fwp->stdobj = *po;
		fwp->std_is_small = FALSE;
		fwp->xdist = 2;

		fwp->stdobj.ob_width = fwp->stdobj.ob_spec.iconblk->ib_wicon;
		obw = SystemIconFont.char_width * (fwp->max_filename_len + 1);
		if (obw > fwp->stdobj.ob_width)
			fwp->stdobj.ob_width = obw;
	}
	else
	{
		FILEFONTINFO *ffi;
		
		if (fwp->display == DisplayText)
		{
			ffi = &TextFont;
			SetFileFont (ffi);
			po = GetStdFolderIcon (ffi->hchar);
		}
		else
		{
			ffi = &SIconFont;
			SetFileFont (ffi);
			po = GetStdBigIcon ();
			fwp->std_is_small = FALSE;
			
			if (po->ob_spec.iconblk->ib_hicon > ffi->hchar + 2)
			{
				po = GetStdSmallIcon ();
				fwp->std_is_small = TRUE;
			}
		}

		fwp->stdobj = pstdbox[FISTRING];
		obw = (ffi->show.size? 9 : 0) + (ffi->show.date? 9 : 0)
				 + (ffi->show.time? 9 : 0);
		if (obw)
			++obw;
			
		fwp->icon_text_distance = (ffi->num_wchar / 2);
		if ((ffi->kursiv_offset_links+1) > fwp->icon_text_distance)
			fwp->icon_text_distance = ffi->kursiv_offset_links + 1;
			
		fwp->stdobj.ob_width = 
			((fwp->max_filename_len) * ffi->wchar)
			+ (obw * ffi->num_wchar)
			+ po->ob_spec.iconblk->ib_wicon
			+ ffi->kursiv_offset_links
			+ ffi->kursiv_offset_rechts
			+ fwp->icon_text_distance
			+ 1;
			
		fwp->max_textname_len = fwp->max_filename_len + obw + 2;
		
		fwp->stdobj.ob_height = max (po->ob_height, 
			(fwp->display == DisplayText)? 
				ffi->hchar : ffi->hchar + 2);
		
		fwp->xdist = 8;
	}

	fwp->icon_max_width = po->ob_spec.iconblk->ib_wicon;
	fwp->icon_max_height = po->ob_spec.iconblk->ib_hicon;

	fwp->stdobj.ob_flags &= ~LASTOB;

	for (bucket = fwp->files; bucket != NULL; 
		bucket = bucket->nextbucket)
	{
		uword i;
		
		for (i = 0; i < bucket->usedcount; ++i)
		{
			bucket->finfo[i].icon.flags.noObject = TRUE;

			if (bucket->finfo[i].icon.string != NULL)
			{
				free (bucket->finfo[i].icon.string);
				bucket->finfo[i].icon.string = NULL;
			}
				
			if (fwp->max_textname_len > 0)
			{
				bucket->finfo[i].icon.string = 
					malloc (fwp->max_textname_len + 1);
				
				if (bucket->finfo[i].icon.string == NULL)
					return FALSE;
			}
		}
	}
	
	return TRUE;
}

/*
 * make an object tree from at wp->files starting FileInfos
 * and link this tree to wp->tree
 */
int MakeFileTree (FileWindow_t *fwp, GRECT *work, int update_type)
{
	OBJECT *po;
	word currx, curry, gesamtanz, i;
	word skip;
	FileBucket *bucket;
	word aktIndex = 0;
			
	po = calloc (fwp->fileanz + 1, sizeof (OBJECT));

	if (po == NULL)
		return FALSE;
	
	if (update_type || (fwp->display != fwp->desiredDisplay))
	{
		fwp->display = fwp->desiredDisplay;
		if (!fetchObjects (fwp))
			return FALSE;
	}

	FreeFileTree (fwp);

	if ((fwp->desiredSort != fwp->sort) || update_type)
	{
		SortFileWindow (fwp, fwp->desiredSort);
	}
	
	memcpy (po, pstdbox, sizeof (OBJECT));
	
	po[0].ob_next = po[0].ob_head = po[0].ob_tail = -1;
	po[0].ob_x = work->g_x;
	po[0].ob_y = work->g_y;
	po[0].ob_width = work->g_w;
	po[0].ob_height = work->g_h;
	
	fwp->xanz = work->g_w / (fwp->stdobj.ob_width + fwp->xdist);
	if (!fwp->xanz && fwp->fileanz)
		fwp->xanz++;
		
	currx = fwp->yanz = work->g_h / (fwp->stdobj.ob_height + 1);
	if (work->g_h % (fwp->stdobj.ob_height + 1))
		fwp->yanz++;

	skip = fwp->xanz * fwp->yskip;

	fwp->maxlines = fwp->yanz;
	gesamtanz = fwp->xanz * fwp->yanz;
	curry = fwp->fileanz - skip;

	if (gesamtanz > curry)
	{
		if (fwp->yskip)
		{
			word usedlines,freelines;
		
			usedlines = curry / fwp->xanz;
			if ((curry > 0) && (curry % fwp->xanz))
				usedlines++;
				
			freelines = currx - usedlines;
			
			if (freelines > 0)/* lines in window left blank */
			{
				fwp->yskip -= freelines;
				if (fwp->yskip < 0)
					fwp->yskip = 0;

				skip = fwp->xanz * fwp->yskip;	/* neu berechnen */
				curry = fwp->fileanz - skip;
			}

			if (gesamtanz > curry)
				gesamtanz = curry;	/* now it's exact */
		}
		else
			gesamtanz = curry;
	}

	bucket = fwp->files;				/* berspringe Files */
	i = skip / FILEBUCKETSIZE;
	aktIndex = skip % FILEBUCKETSIZE;

	while (i--)
		bucket = bucket->nextbucket;

	if (gesamtanz)
	{
		FileInfo *pf;
		
		currx = curry = 0;
			
		for (i = 1; i <= gesamtanz; i++)
		{
			memcpy (&po[i], &fwp->stdobj, sizeof(OBJECT));
			
			if (aktIndex == FILEBUCKETSIZE)
			{
				bucket = bucket->nextbucket;
				aktIndex = 0;
			}

			pf = &bucket->finfo[aktIndex];
			++aktIndex;
			
			if (pf->icon.flags.noObject)
				fillFileUnion (fwp, pf);
				
			if (pf->flags.selected)
				po[i].ob_state |= SELECTED;

			po[i].ob_spec.userblk = &pf->user_block;
			po[i].ob_type = G_USERDEF;

			if (pf->flags.dragged)
				MarkDraggedObject (NULL, po + i);

			pf->user_block.ub_parm = (long)&pf->icon;
			if (fwp->display != DisplayBigIcons)
			{
				pf->user_block.ub_code = drawFileText;
			}
			else
			{
				pf->user_block.ub_code = DrawIconObject;
				pf->icon.icon.ib_ptext = pf->name;
			}
									 
			po[i].ob_x = currx * (po[i].ob_width - 1 
							+ fwp->xdist) + fwp->xdist;
			po[i].ob_y = curry * (po[i].ob_height + 1) + 1;

			po[i].ob_head = po[i].ob_tail = -1;
			
			++currx;
			if (currx >= fwp->xanz)
			{
				currx = 0;
				++curry;
			}

#ifndef WHITEBAK
#define WHITEBAK	0x0040
#endif
			po[i].ob_state |= WHITEBAK;
			po[i].ob_next = i + 1;
		}

		/* setze LASTOB auf letztem Objekt des Baums
		 */
		po[gesamtanz].ob_flags |= LASTOB;
		
		po[gesamtanz].ob_next = 0;
		po[0].ob_tail = gesamtanz;
		po[0].ob_head = 1;
	}

	fwp->objanz = gesamtanz + 1;
	fwp->tree = po;
	
	return TRUE;
}


/*
 * free object tree at wp->tree
 */
void FreeFileTree (FileWindow_t *fwp)
{
	if (fwp->tree != pstdbox)
	{
		free (fwp->tree);
		fwp->tree = NULL;
	}
}



