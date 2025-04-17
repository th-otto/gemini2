/*
 * @(#) Gemini\select.c
 * @(#) Stefan Eissing, 29. Januar 1995
 *
 * description: stuff for selecting objects etc.
 */

#include <string.h>
#include <aes.h>

#include "..\common\alloc.h"
#include "..\common\argvinfo.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\genargv.h"
#include "..\common\wildmat.h"

#include "vs.h"
#include "menu.rh"

#include "window.h"
#include "filewind.h"
#include "icondrag.h"
#include "iconinst.h"
#include "myalloc.h"
#include "redraw.h"
#include "scroll.h"
#include "select.h"
#include "util.h"
#include "vaproto.h"
#include "venuserr.h"



#define DRAG		0x4000	/* zu draggendes Icon, Marker im Typ */


int SelectObject (WindInfo *wp, word obj, int draw)
{
	if (wp && wp->select_object)
		return wp->select_object (wp, obj, draw);
	else
		return FALSE;
}


int DeselectObject (WindInfo *wp, word obj, int draw)
{
	if (wp && wp->deselect_object)
		return wp->deselect_object (wp, obj, draw);
	else
		return FALSE;
}


int SelectAll (WindInfo *wp)
{
	if (wp && wp->select_all)
		return wp->select_all (wp);
	else
		return FALSE;
}


void DeselectAll (WindInfo *wp)
{
	if (wp && wp->deselect_all)
		wp->deselect_all (wp);
}


void DeselectAllExceptWindow (WindInfo *wp)
{
	WindInfo *wplist;

	wplist = &wnull;

	while (wplist != NULL)
	{
		if (wplist != wp)
			DeselectAll (wplist);
			
		wplist = wplist->nextwind;
	} 
}

void DeselectAllObjects (void)
{
	WindInfo *wp;
	
	wp = &wnull;

	while (wp != NULL)
	{
		DeselectAll (wp);
		wp = wp->nextwind;
	}
}

int ThereAreSelected (WindInfo **wpp)
{
	if (NewDesk.total_selected < 1)
		return FALSE;

	*wpp = &wnull;

	while (*wpp != NULL)
	{
		if ((*wpp)->selectAnz)
			return TRUE;

		*wpp = (*wpp)->nextwind;
	}

	return FALSE;
}



/* get the only selected Object, if there is one
 */
word GetOnlySelectedObject (WindInfo **wpp, word *objnr)
{
	register WindInfo *wp;
	register word i;
	
	if (NewDesk.total_selected != 1)
	{
		return FALSE;
	}
	
	wp = &wnull;
	while (wp != NULL)
	{
		if ((wp->selectAnz >= 1) && wp->window_get_tree)
		{
			OBJECT *tree;
				
			tree = wp->window_get_tree (wp);
			
			if (tree != NULL)
			{
				for (i = tree[0].ob_head; i > 0; i = tree[i].ob_next)
				{
					if (tree[i].ob_state & SELECTED)
					{
						*objnr = i;
						*wpp = wp;
						return TRUE;
					}
				}
			}
		}
		
		wp = wp->nextwind;
	}
	
	return FALSE;
}

/*
 * deselect all objects in wp which were not dragged
 */
void DeselectNotDraggedObjects (WindInfo *wp)
{
	register word j;
	OBJECT *tree = NULL;
	
	
	if (wp == NULL || wp->selectAnz < 1)
		return;

	if (wp->window_get_tree)
		tree = wp->window_get_tree (wp);
		
	if (tree == NULL)
		return;
			
	for (j = tree[0].ob_head; j > 0; j = tree[j].ob_next)
	{
		if ((tree[j].ob_state & SELECTED)
			&& (!(tree[j].ob_type & DRAG)))
		{
			DeselectObject (wp, j, TRUE);
		}
	}
	
	if (wp->kind == WK_FILE)
	{
		FileWindow_t *fwp = wp->kind_info;
		FileBucket *bucket = fwp->files;
		uword i;
		
		while (bucket)
		{
			for (i = 0; i < bucket->usedcount; ++i)
			{
				if (bucket->finfo[i].flags.selected
					&& (!bucket->finfo[i].flags.dragged))
				{
					bucket->finfo[i].flags.selected = FALSE;
					--NewDesk.total_selected;
					--wp->selectAnz;
					fwp->selectSize -= bucket->finfo[i].size;
				}
			}
			
			bucket = bucket->nextbucket;
		}

		wp->update |= WU_WINDINFO;
	}
	
	flushredraw ();
}


static void fileWindowSelectChanged (WindInfo *wp, int redraw)
{
	FileWindow_t *fwp = wp->kind_info;
	FileInfo *pf;
	int i, selected;
	char changed = FALSE;
	
	for (i = fwp->tree[0].ob_head; i > 0; i = fwp->tree[i].ob_next)
	{
		pf = GetFileInfo (&fwp->tree[i]);
		selected = (fwp->tree[i].ob_state & SELECTED) != 0;
		
		if (selected ^ (pf->flags.selected != 0))
		{
			changed = TRUE;
			if (pf->flags.selected)
				fwp->tree[i].ob_state |= SELECTED;
			else
				fwp->tree[i].ob_state &= ~SELECTED;
			
			if (redraw)
				redrawObj (wp, i);
		}
	}
	
	if (changed && redraw)
		flushredraw ();
}


/*
 * select an Object in window wp by giving its name
 * works only in file windows
 */
const char *StringSelect (WindInfo *wp, char *pattern, int redraw)
{
	static char overlap[MAX_PATH_LEN];
	FileWindow_t *fwp = wp->kind_info;
	FileBucket *bucket;
	uword nr, first_nr, aktIndex;
	word found;
	char changed;
	
	if (wp->kind != WK_FILE)
		return 0;
		
	nr = first_nr = 0;
	found = changed = FALSE;
	bucket = fwp->files;					/* start of list */
	*overlap = 0;
	overlap[MAX_PATH_LEN-1] = 0;
	
	while (bucket != NULL)
	{
		for (aktIndex = 0; aktIndex < bucket->usedcount; ++aktIndex)
		{
			if (wildmatch (bucket->finfo[aktIndex].name, pattern, 
				fwp->flags.case_sensitiv))
			{
				if (!found)
				{
					first_nr = nr;
					strncpy (overlap, bucket->finfo[aktIndex].name,
						MAX_PATH_LEN-1);
				}
				else
				{
					int i;
					size_t len = strlen (overlap);
					for (i = 0; i < len; ++i)
					{
						if (overlap[i] 
							!= bucket->finfo[aktIndex].name[i])
						{
							overlap[i] = 0;
							break;
						}
					}
				}
				
				found = TRUE;
				
				if (!bucket->finfo[aktIndex].flags.selected)
				{
					fwp->selectSize += bucket->finfo[aktIndex].size;
					++wp->selectAnz;
					++NewDesk.total_selected;
					changed = TRUE;
					bucket->finfo[aktIndex].flags.selected = 1;
				}
			}
			else
			{
				if (bucket->finfo[aktIndex].flags.selected)
				{
					fwp->selectSize -= bucket->finfo[aktIndex].size;
					--wp->selectAnz;
					--NewDesk.total_selected;
					changed = TRUE;
					bucket->finfo[aktIndex].flags.selected = 0;
				}
			}
			
			++nr;
		}
		
		bucket = bucket->nextbucket;
	}

	if (found || changed)
	{
		fileWindowSelectChanged (wp, redraw);
		DeselectAllExceptWindow (wp);
		wp->update |= WU_WINDINFO;
	}
		
	if (found)
	{
		GRECT work;
		word new_yskip, selected_line;
		int fully_seen_lines;
	
		work = wp->work;

		fully_seen_lines = work.g_h / (fwp->stdobj.ob_height + 1);
		if (fully_seen_lines == 0)
			fully_seen_lines = 1;

		selected_line = first_nr / fwp->xanz;
		new_yskip = CalcYSkip (fwp, &work, selected_line);

		if ((new_yskip != fwp->yskip)
			&& ((selected_line < fwp->yskip) 
			|| (selected_line >= fwp->yskip + fully_seen_lines)))
		{
			word divskip;
			
			divskip = fwp->yskip - new_yskip;
			fwp->yskip = new_yskip;
			FileWindowScroll (wp, divskip, redraw);
		}
	}
	
	return found? overlap : 0;
}

word SelectAllInTopWindow (void)
{
	WindInfo *wp;
	word windhandle;
	
	wind_get (0, WF_TOP, &windhandle);
	wp = GetWindInfo (windhandle);
	if (!wp || (wp->owner != apid))
	{
		wp = wnull.nextwind;
		while (wp != NULL)
		{
			if (wp->owner == apid)
				break;
				
			wp = wp->nextwind;
		}
		
		if (wp == NULL)
			wp = &wnull;
	}
	
	return SelectAll (wp);
}


void MarkDraggedObjects (WindInfo *wp)
{
	OBJECT *tree = NULL;
	word i;
	
	switch (wp->kind)
	{
		case WK_DESK:
			tree = ((DeskInfo *)wp->kind_info)->tree;
			break;
		
		case WK_FILE:
			tree = ((FileWindow_t *)wp->kind_info)->tree;
			break;
		
		default:
			return;
	}

	for (i = tree[0].ob_head; i > 0; i = tree[i].ob_next)
	{
		if (tree[i].ob_state & SELECTED)
			tree[i].ob_type |= DRAG;
		else
			tree[i].ob_type &= ~DRAG;
	}
	
	if (wp->kind == WK_FILE)
	{
		FileWindow_t *fwp = wp->kind_info;
		FileBucket *bucket;
		
		bucket = fwp->files;
		while (bucket)
		{
			for (i = 0; i < bucket->usedcount; ++i)
			{
				bucket->finfo[i].flags.dragged = 
					bucket->finfo[i].flags.selected;
			}
			
			bucket = bucket->nextbucket;
		}
	}
}


int ObjectWasDragged (OBJECT *po)
{
	return po->ob_type & DRAG;
}

void MarkDraggedObject (WindInfo *wp, OBJECT *po)
{
	po->ob_type |= DRAG;
	if (wp && wp->kind == WK_FILE)
	{
		FileInfo *pfi = GetFileInfo (po);
		if (pfi)
			pfi->flags.dragged = 1;
	}
}

void UnMarkDraggedObject (WindInfo *wp, OBJECT *po)
{
	po->ob_type &= ~DRAG;
	if (wp && wp->kind == WK_FILE)
	{
		FileInfo *pfi = GetFileInfo (po);
		if (pfi)
			pfi->flags.dragged = 0;
	}
}

/*
 * rub out marks 'DRAG' from all objects in window wp
 */
void UnMarkDraggedObjects (WindInfo *wp)
{
	OBJECT *tree = NULL;
	word i;
	
	if (wp->window_get_tree)
		tree = wp->window_get_tree (wp);

	if (tree == NULL)
		return;
	
	for (i = tree[0].ob_head; i > 0; i = tree[i].ob_next)
	{
		tree[i].ob_type &= ~DRAG;
	}
	
	if (wp->kind == WK_FILE)
	{
		FileWindow_t *fwp = wp->kind_info;
		FileBucket *bucket;
		
		bucket = fwp->files;
		
		while (bucket)
		{
			for (i = 0; i < bucket->usedcount; ++i)
			{
				bucket->finfo[i].flags.dragged = 0;
			}
		
			bucket = bucket->nextbucket;
		}
	}
}


char *WhatIzIt (word x, word y, word *ob_type, word *owner_id)
{
	WindInfo *wp;
	OBJECT *tree = NULL;
	word whandle;
	char *name = NULL;
	word object = -1;
	
	whandle = wind_find (x, y);
	
	wp = GetWindInfo (whandle);
	if (wp == NULL)
	{
		/* Wir kennen das Fenster nicht und k”nnen daher nichts
		 * darber aussagen.
		 * stevie: unter MultiTOS geht das aber!
		 */
		*ob_type = VA_OB_UNKNOWN;
		*owner_id = -1;
		return NULL;
	}
	
	*owner_id = wp->owner;
	*ob_type = VA_OB_WINDOW;

	if (wp->window_get_tree)
		tree = wp->window_get_tree (wp);
	if (tree != NULL)
		object = FindObject (tree, x, y);
		
	switch (wp->kind)
	{
		case WK_DESK:
			if (object > 0)
			{
				IconInfo *pii;
				static char trans_type[][2] = 
				{
					{ DI_SHREDDER, VA_OB_SHREDDER},
					{ DI_TRASHCAN, VA_OB_TRASHCAN},
					{ DI_SCRAPDIR, VA_OB_CLIPBOARD},
					{ DI_PROGRAM, VA_OB_FILE},
					{ DI_FOLDER, VA_OB_FOLDER},
					{ DI_FILESYSTEM, VA_OB_DRIVE},
					{ 0, 0}
				};
				word i;
				
				pii = GetIconInfo (&tree[object]);

				for (i = 0; trans_type[i][0] != 0; ++i)
				{
					if (trans_type[i][0] == pii->type)
					{
						*ob_type = trans_type[i][1];
						break;
					}
				}

				if (*ob_type != VA_OB_SHREDDER)
					name = StrDup (pMainAllocInfo, pii->fullname);
			}
			break;
		
		case WK_FILE:
		{
			char tmp[MAX_PATH_LEN];
			FileWindow_t *fwp = wp->kind_info;

			strcpy (tmp, fwp->path);
			*ob_type = VA_OB_FOLDER;
			
			if (object > 0)
			{
				FileInfo *pfi;
				
				pfi = GetFileInfo (&tree[object]);
				if (pfi->attrib & FA_FOLDER)
				{
					AddFolderName (tmp, pfi->name);
				}
				else
				{
					*ob_type = VA_OB_FILE;
					AddFileName (tmp, pfi->name);
				}
			}
			
			name = NormalizePath (pMainAllocInfo, tmp, tmp[0],
						fwp->path);
			break;
		}
	}
	
	return name;
}