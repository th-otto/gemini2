/*
 * @(#) Gemini\filewind.c
 * @(#) Stefan Eissing, 13. November 1994
 *
 * jr 22.4.95
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <aes.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\genargv.h"
#include "..\common\strerror.h"

#include "vs.h"
#include "menu.rh"
#include "stdbox.rh"

#include "window.h"
#include "clipbrd.h"
#include "dispatch.h"
#include "draglogi.h"
#include "erase.h"
#include "filewind.h"
#include "filenew.h"
#include "files.h"
#include "fileutil.h"
#include "filedraw.h"
#include "icondrag.h"
#include "myalloc.h"
#include "redraw.h"
#include "select.h"
#include "scroll.h"
#include "sortfile.h"
#include "stand.h"
#include "undo.h"
#include "util.h"
#include "venuserr.h"
#include "wildcard.h"
#include "windstak.h"

#if MERGED
#include "..\mupfel\mupfel.h"
#endif


/* externs 
 */
extern OBJECT *pstdbox, *pmenu;

/*
 * internal texts 
 */
#define NlsLocalSection "G.filewind"
enum NlsLocalText{
T_INFONORM,			/* %d %s, %s %s*/
T_INFONORMSELECTED,	/* %d %s mit %s %s selektiert*/
T_INFOSTRINGSELECTED,	/* %s: %d %s mit %s %s selektiert*/
T_INFOEMPTY,		/* Leeres Verzeichnis*/
T_OBJECT,			/*Objekt*/
T_OBJECTS,			/*Objekte*/
T_BYTE,				/*Byte*/
T_BYTES,			/*Bytes*/
T_NODRIVE,			/*Kann kein Fenster ”ffnen, da das Laufwerk 
%c: GEMDOS nicht bekannt ist!*/
T_NOLABEL,		/*Kann das Label von Laufwerk `%c:' nicht lesen (%s)!*/
T_NOFOLDER,		/*Kann den Pfad `%s' nicht setzen! Das entsprechende
 Fenster wird geschlossen, bzw. nicht ge”ffnet.*/
T_NESTED,			/*Operation wird abgebrochen, da die Schachtelung
 der Ordner zu tief ist.*/
};


#define FILEWIND	(NAME|INFO|CLOSER|FULLER|SIZER|MOVER|VSLIDE| \
						UPARROW|DNARROW)


static void popPositionInfo (FileWindow_t *fwp)
{
	PositionInfo *pi;
		
	pi = fwp->lastPos;
	if (pi)
	{
		fwp->lastPos = pi->next;
		free (pi);
	}
}

static void freePositionInfo (FileWindow_t *fwp)
{
	while (fwp->lastPos)
		popPositionInfo (fwp);
}


static int savePositionInfo (FileWindow_t *fwp)
{
	PositionInfo *pi;
	
	pi = malloc (sizeof (PositionInfo));
	if (pi == NULL) return FALSE;
	
	pi->xskip = fwp->xskip;
	pi->yskip = fwp->yskip;
	pi->next = fwp->lastPos;
	fwp->lastPos = pi;
	
	return TRUE;
}


static void restorePositionInfo (FileWindow_t *fwp)
{
	if (fwp->lastPos == NULL)
	{
		/* Der Slider wird resettet
		 */
		fwp->yskip = fwp->xskip = 0;
	}
	else
	{
		fwp->xskip = fwp->lastPos->xskip;
		fwp->yskip = fwp->lastPos->yskip;
		popPositionInfo (fwp);
	}
}


static void fileWindowMoved (WindInfo *wp)
{
	FileWindow_t *fwp = wp->kind_info;
	
	if (fwp->tree)
	{			
		fwp->tree[0].ob_x = wp->work.g_x;
		fwp->tree[0].ob_y = wp->work.g_y;
	}
}


/* make sure that the window has a sufficient size
 */
static void fileWindowCalcWindRect (WindInfo *wp, GRECT *wind)
{
#define MIN_OBJECT_SIZE 0
#if MIN_OBJECT_SIZE
	FileWindow_t *fwp = wp->kind_info;
	word min_width = fwp->stdobj.ob_width;
	word min_height = fwp->stdobj.ob_height;
#else
	word min_width = SystemNormFont.char_width * 4;
	word min_height = SystemNormFont.char_height * 3;
#endif
	GRECT work;
	
	wind_calc (WC_WORK, wp->type, wind->g_x, wind->g_y,
		wind->g_w, wind->g_h, &work.g_x, &work.g_y,
		&work.g_w, &work.g_h);
		
	if (work.g_w < min_width)
	{
		work.g_w = min_width;
		if (wp->work.g_w < work.g_w)
			work.g_w = wp->work.g_w;
	}
	if (work.g_h < min_height)
	{
		work.g_h = min_height;
		if (wp->work.g_h < work.g_h)
			work.g_h = wp->work.g_h;
	}

	wind_calc (WC_BORDER, wp->type, work.g_x, work.g_y,
		work.g_w, work.g_h, &wind->g_x, &wind->g_y,
		&wind->g_w, &wind->g_h);
}

static void fileWindowSized (WindInfo *wp)
{
	FileWindow_t *fwp = wp->kind_info;
	word old_columns, old_lines;
	
	old_columns = fwp->xanz;
	old_lines = fwp->yanz;
	
	wp->update |= (WU_VESLIDER|WU_DISPLAY);
	WindowUpdateData (wp);
	

	/* Wenn sich an der Anzahl der Spalten im Fenster etwas ge„ndert
	 * hat oder sich die Anzahl der Zeilen erh”ht hat, mssen wir 
	 * immer das Fenster komplett neu zeichnen. Das
	 * k”nnen wir nur durch eine Nachricht an uns selbst erzwingen.
	 */
	if ((fwp->xanz != old_columns)
		|| ((fwp->yanz > old_lines))) /* && (wp->vslpos > 999))) */
	{
		word messbuff[8];
		
		messbuff[0] = WM_REDRAW;
		messbuff[1] = apid;
		messbuff[2] = 0;
		messbuff[3] = wp->handle;
		messbuff[4] = wp->work.g_x;
		messbuff[5] = wp->work.g_y;
		messbuff[6] = wp->work.g_w;
		messbuff[7] = wp->work.g_h;
		appl_write (wp->owner, 16, messbuff);
	}
}


static void setTOPWIND (const char *path)
{
#if MERGED
	char str[2 * MAX_PATH_LEN];
	
	sprintf (str, "TOPWIND=%s", path);
	PutEnv (MupfelInfo, str);
#endif
}


static long setPathInfoOfWindow (FileWindow_t *fwp)
{
	long ret;

#if DEBUG
venusInfo ("set full path: %s", fwp->path);
#endif

	ret = SetFullPath (fwp->path);
	if (ret != E_OK)
	{
		GetFullPath (fwp->path, sizeof (fwp->path));
		venusErr (StrError (ret));
	}
	
	setTOPWIND (fwp->path);
	return ret;
}


static void fileWindowTopped (WindInfo *wp)
{
	FileWindow_t *fwp = wp->kind_info;

	menu_ienable (pmenu, WINDCLOS, TRUE);
	menu_ienable (pmenu, CLOSE, TRUE);
	
	setPathInfoOfWindow (fwp);
}


static void resetSelectInfo (WindInfo *wp)
{
	FileWindow_t *fwp = wp->kind_info;
	
	NewDesk.total_selected -= wp->selectAnz;
	wp->selectAnz = 0;
	fwp->selectSize = 0L;
	fwp->search_string[0] = '\0';
}


static void fileWindowDestroyed (WindInfo *wp)
{
	FileWindow_t *fwp = wp->kind_info;
	WindInfo *next;
	
	PushWindBox (&wp->wind, wp->kind);

	for (next = wp; next; next = next->nextwind)
	{
		if (next->kind == WK_FILE)
		{
			setTOPWIND (((FileWindow_t *)next->kind_info)->path);
			break;
		}
	}
	
	freePositionInfo (fwp);
	free (fwp->aesname);
	
	resetSelectInfo (wp);

	FreeFiles (fwp);
	FreeFileTree (fwp);
	free (fwp);
}

static const char *bytesToString (long bytes)
{
	static char buffer[20];
	
	if (bytes > 1023L)
		sprintf (buffer, "%ldK", bytes / 1024L);
	else
		sprintf (buffer, "%ld", bytes);
	return buffer;
}

static void fileWindowCalcData (WindInfo *wp)
{
	FileWindow_t *fwp = wp->kind_info;
	word seen_lines, total_lines, lost_lines;
	
	if (wp->update & WU_WINDINFO)
	{
		static const char *t_object = NULL;
		static const char *t_objects = NULL;
		static const char *t_byte = NULL;
		static const char *t_bytes = NULL;
		static const char *t_infonormselected = NULL;
		static const char *t_infostringselected = NULL;
		static const char *t_infonorm = NULL;
		static const char *t_infoempty = NULL;
		word fileanz;
		
		if (!t_object)
		{
			t_object = NlsStr(T_OBJECT);
			t_objects = NlsStr(T_OBJECTS);
			t_byte = NlsStr(T_BYTE);
			t_bytes = NlsStr(T_BYTES);
			t_infonormselected = NlsStr(T_INFONORMSELECTED);
			t_infostringselected = NlsStr(T_INFOSTRINGSELECTED);
			t_infonorm = NlsStr(T_INFONORM);
			t_infoempty = NlsStr(T_INFOEMPTY);
		}

		fileanz = fwp->fileanz;
		if (fwp->files 
			&& !strcmp (fwp->files->finfo[0].name, ".."))
			fileanz--;
		
		if (fwp->search_string[0])
		{
			sprintf (fwp->info, t_infostringselected, 
				fwp->search_string, wp->selectAnz,
				(wp->selectAnz != 1)? t_objects : t_object,
				bytesToString (fwp->selectSize), 
				(fwp->selectSize != 1)? t_bytes : t_byte);
		}
		else if (wp->selectAnz)
		{
			sprintf (fwp->info, t_infonormselected, wp->selectAnz,
				(wp->selectAnz != 1)? t_objects : t_object,
				bytesToString(fwp->selectSize), 
				(fwp->selectSize != 1)? t_bytes : t_byte);
		}
		else if (fileanz)
		{
			sprintf (fwp->info,t_infonorm,
				fileanz, (fileanz != 1)? t_objects : t_object,
				bytesToString (fwp->dirsize),
				(fwp->dirsize != 1)? t_bytes : t_byte);
		}
		else
		{
			sprintf (fwp->info, t_infoempty);
		}
	}
	
	if (wp->update & WU_SLIDERRESET)
		fwp->xskip = fwp->yskip = 0;

	fwp->desiredDisplay = ShowInfo.showtext? DisplayText :
		(ShowInfo.normicon? DisplayBigIcons : DisplaySmallIcons);
	if ((wp->update & WU_DISPLAY) 
		|| (fwp->display != fwp->desiredDisplay))
	{
		/* built Objecttree
		 */
		fwp->desiredSort = MenuEntryToSortMode (ShowInfo.sortentry);
		MakeFileTree (fwp, &wp->work, wp->update & WU_SHOWTYPE);
	}

	if (fwp->tree == NULL)
	{
		fwp->tree = pstdbox;
		fwp->objanz = countObjects (fwp->tree, BOX);
	}

	if (wp->update & WU_VESLIDER)
	{
		wp->vslpos = 0;
		if (fwp->fileanz)
		{
			seen_lines = wp->work.g_h / (fwp->stdobj.ob_height + 1);
			total_lines = fwp->fileanz / fwp->xanz;
			if (fwp->fileanz % fwp->xanz)	/* got a remainder */
				total_lines++;
			lost_lines = total_lines - seen_lines;
			if ((lost_lines > 0) && (total_lines > 1))
			{
				wp->vslsize = (word)((1000L * seen_lines) / total_lines);
				wp->vslpos = (word)((1000L * fwp->yskip) / lost_lines);
			}
			else
			{
				wp->vslsize = 1000;
			} 
		}
		else
			wp->vslsize = 1000;
	}
}


static void fileWindowUpdate (WindInfo *wp)
{
	FileWindow_t *fwp = wp->kind_info;
	char *name;
	size_t len;
	
	fileWindowCalcData (wp);
	
	if (wp->update & WU_WINDNAME)
	{
		free (fwp->aesname);
		fwp->aesname = NULL;
		
		len = strlen (fwp->title) + strlen (fwp->wildcard) + 4;
		fwp->aesname = malloc (len);
		
		if (fwp->aesname)
		{
			strcpy (fwp->aesname, " ");
			strcat (fwp->aesname, fwp->title);
			if (strcmp (fwp->wildcard, "*"))
				AddFileName (fwp->aesname, fwp->wildcard);
			strcat (fwp->aesname, " ");
			name = fwp->aesname;
		}
		else
			name = fwp->title;
			
		wind_set (wp->handle, WF_NAME, name);
	}

	if (wp->update & WU_WINDINFO)
	{
		wind_set (wp->handle, WF_INFO, fwp->info);
	}
	
	if (wp->update & WU_REDRAW)
	{
		FileWindowRedraw (wp->handle, wp->kind_info, &wp->work);
	}
}


static void fileWindowRedraw (WindInfo *wp, const GRECT *rect)
{
	FileWindowRedraw (wp->handle, wp->kind_info, rect);
}


static OBJECT *fileWindowGetTree (WindInfo *wp)
{
	return ((FileWindow_t *)wp->kind_info)->tree;
}


static void fileWindowGetState (WindInfo *wp, char *buffer)
{
	FileWindow_t *fwp = wp->kind_info;
	word wx, wy, ww, wh;

	wx = (word)scale123 (ScaleFactor, wp->wind.g_x - Desk.g_x, Desk.g_w);
	ww = (word)scale123 (ScaleFactor, wp->wind.g_w - Desk.g_x, Desk.g_w);
	wy = (word)scale123 (ScaleFactor, wp->wind.g_y - Desk.g_y, Desk.g_h);
	wh = (word)scale123 (ScaleFactor, wp->wind.g_h - Desk.g_y, Desk.g_h);

	sprintf (buffer,
			"#W@%d@%d@%d@%d@%d@%d@%s@%s@%s@%s",
			wx, wy, ww, wh, fwp->yskip, wp->kind, fwp->path,
			fwp->wildcard, fwp->title, fwp->label);
}


static int fileWindowCopy (WindInfo *wp, int only_asking)
{
	ARGVINFO A;
	char *names;
	
	if (only_asking)
		return wp->selectAnz > 0;
	
	InitArgv (&A);
	if (!wp->getSelected || !wp->getSelected (wp, &A, FALSE))
		return FALSE;

	names = Argv2String (pMainAllocInfo, &A, FALSE);		
	if (names != NULL)
	{
		PasteString (names, FALSE, NULL);
		free (names);
	}
	FreeArgv (&A);
	
	return TRUE;
}


static int fileWindowMakeReadyToDrag (WindInfo *wp)
{
	if (wp->kind != WK_FILE)
		return TRUE;
		
	{
		FileWindow_t *fwp = wp->kind_info;
		FileInfo *pf;
		
		if (fwp->files->finfo[0].flags.selected)
		{
			/* Dies sollte eigentlich eine Routine in select.c sein
			 */
			pf = &(fwp->files->finfo[0]);
			if (!strcmp (pf->name, ".."))	/* It's the .. folder */
			{
				if (wp->selectAnz == 1)	/* the only selected */
				{
					return FALSE;
				}
				else
				{
					if (pf == GetFileInfo (&fwp->tree[1]))
					{
						/* deselect: no move or copy
						 */
						DeselectObject (wp, 1, TRUE); 
						flushredraw ();
					}
					else
					{
						pf->flags.selected = 0;
						fwp->selectSize -= pf->size;
						--wp->selectAnz;
						--NewDesk.total_selected;
						wp->update |= WU_WINDINFO;
					}
				}
			}
		}
	}

	UpdateAllWindowData ();
	return TRUE;
}


static int fileWindowDelete (WindInfo *wp, int only_asking)
{
	FileWindow_t *fwp = wp->kind_info;
	int retcode = FALSE;
	
	if (only_asking)
		return ((wp->selectAnz > 0)
			&& ((wp->selectAnz > 1)
				|| (!fwp->files->finfo[0].flags.selected)
				|| strcmp (fwp->files->finfo[0].name, "..")));

	if (fileWindowMakeReadyToDrag (wp))
	{
		MarkDraggedObjects (wp);
		retcode = DeleteDraggedObjects (wp);
		UnMarkDraggedObjects (wp);
	}

	return retcode;
}


static void fileWindowMouseClick (WindInfo *wp, word clicks,
	word mx, word my, word kstate)
{
	FileWindow_t *fwp = wp->kind_info;

	if (!PointInGRECT (mx, my, &wp->work))
	{
		if (my < wp->work.g_y)
		{
			/* W„hle neuen Wildcard, etc
			 */
			FileWindowSpecials (wp, mx, my);
		}
	}
	else if (clicks == 2)
	{
		word x, y, bs, ks;
		
		if (ButtonPressed (&x, &y, &bs, &ks))
		{
			while (!ButtonReleased (&x, &y, &bs, &ks))
				;
		}
		
		doDclick (wp, fwp->tree, FindObject (fwp->tree, mx, my), 
			kstate);
	}
	else
	{
		doIcons (wp, mx, my, kstate);
	}
}


static int fileWindowSelectObject (WindInfo *wp, word obj, int draw)
{
	FileWindow_t *fwp = wp->kind_info;
	FileInfo *pfi;
	
	pfi = GetFileInfo (&fwp->tree[obj]);
	if (!pfi || pfi->flags.selected)
		return TRUE;
	fwp->tree[obj].ob_state |= SELECTED;
	pfi->flags.selected = TRUE;

	fwp->selectSize += pfi->size;
	wp->update |= WU_WINDINFO;

	++wp->selectAnz;
	++NewDesk.total_selected;
	if (draw)
		redrawObj (wp, obj);
	
	return TRUE;
}


static int fileWindowDeselectObject (WindInfo *wp, word obj, int draw)
{
	FileWindow_t *fwp = wp->kind_info;
	FileInfo *pfi;

	pfi = GetFileInfo (&fwp->tree[obj]);
	if (!pfi || !pfi->flags.selected)
		return TRUE;
	fwp->tree[obj].ob_state &= ~SELECTED;
	pfi->flags.selected = 0;
	
	fwp->selectSize -= pfi->size;
	--wp->selectAnz;
	wp->update |= WU_WINDINFO;

	if (draw)
	{
		/* Objekt deselektieren
		 */
		redrawObj (wp, obj);
	}

	--NewDesk.total_selected;
	if (NewDesk.total_selected < 0)
		NewDesk.total_selected = 0;
	
	return TRUE;
}


static void fileWindowDeselectAll (WindInfo *wp)
{
	FileWindow_t *fwp = wp->kind_info;
	register word j;
	
	if (wp->selectAnz < 1)
		return;
		
	for (j = fwp->tree[0].ob_head; j > 0; j = fwp->tree[j].ob_next)
	{
		if (fwp->tree[j].ob_state & SELECTED)
		{
			DeselectObject (wp, j, TRUE);
		}
	}
	
	if (wp->selectAnz)
	{
		FileBucket *bucket = fwp->files;
		uword i;
		
		while (bucket)
		{
			for (i = 0; i < bucket->usedcount; ++i)
			{
				bucket->finfo[i].flags.selected = FALSE;
			}
			
			bucket = bucket->nextbucket;
		}
		wp->update |= WU_WINDINFO;
	}
	
	resetSelectInfo (wp);		
	
	flushredraw ();
}


static word fileWindowSelectAll (WindInfo *wp)
{
	FileWindow_t *fwp = wp->kind_info;
	FileBucket *bucket;
	uword i, new_selected;
	
	if (fwp->fileanz <= 0)
		return TRUE;
		
	DeselectAllExceptWindow (wp);
	
	fwp->search_string[0] = '\0';
	bucket = fwp->files;
	
	new_selected = FALSE;
	while (bucket)
	{
		for (i = 0; i < bucket->usedcount; ++i)
		{
			if (bucket->finfo[i].flags.selected)
				continue;
			if (!strcmp (bucket->finfo[i].name, ".."))
				continue;
			
			new_selected = TRUE;
			bucket->finfo[i].flags.selected = TRUE;
			++wp->selectAnz;
			++NewDesk.total_selected;
			fwp->selectSize += bucket->finfo[i].size;
		}
		
		bucket = bucket->nextbucket;
	}

	for (i = fwp->tree[0].ob_head; i > 0; i = fwp->tree[i].ob_next)
	{
		if (!isSelected (fwp->tree, i))
		{
			FileInfo *pf = GetFileInfo (&fwp->tree[i]);
			
			if (pf && strcmp (pf->name, ".."))
			{
				setSelected (fwp->tree, i, TRUE);
				redrawObj (wp, i);
			}
		}
	}
	
	flushredraw ();
	if (new_selected)
		rewindow (wp, WU_WINDINFO);
		
	return TRUE;
}



static int checkForExistingPath (FileWindow_t *fwp)
{
	char tmp[MAX_PATH_LEN];
	int broken = FALSE;
	
	strcpy (tmp, fwp->path);
	while (!IsDirectory (fwp->path, &broken))
	{
		if (broken || !StripFolderName (fwp->title)
			|| !StripFolderName (fwp->path))
		{
			venusErr (NlsStr (T_NOFOLDER), tmp);
			return FALSE;
		}
		else
			restorePositionInfo (fwp);
	}
	
	return TRUE;
}

/*
 * get files in wp->path and display them in window wp->handle
 */
int fileWindowAddToPath (WindInfo *wp, const char *name)
{
	FileWindow_t *fwp = wp->kind_info;
	char label[MAX_FILENAME_LEN];
	word num;
	MFORM form;
	int new_path;
	long stat;
		
	GrafGetForm (&num, &form);
	GrafMouse (HOURGLASS, NULL);
	new_path = (name != NULL);
	if (new_path)
	{
		storePathUndo (wp);
		if (name[1] == ':' && name[2] == '\\')
		{
			/* Dies ist ein absoluter Pfad.
			 * Die alten Positionsinfos sind berflssig.
			 */
			freePositionInfo (fwp);
			strcpy (fwp->path, name);
			strcpy (fwp->title, name);
		}
		else
		{
			/* Der Pfad wird verl„ngert; sichere die aktuelle
			 * Position der Slider
			 */
			savePositionInfo (fwp);
			AddFolderName (fwp->path, name);
			AddFolderName (fwp->title, name);
		}
	}
	
	stat = DriveAccessible (fwp->path[0], label, MAX_FILENAME_LEN);
	if (stat != E_OK)
	{
		venusErr (NlsStr (T_NOLABEL), fwp->path[0], StrError (stat));
		GrafMouse (num, &form);
		return FALSE;
	}

	if (!checkForExistingPath (fwp))
	{
		WindowDestroy (wp->handle, TRUE, FALSE);
		GrafMouse (num, &form);
		return FALSE;
	}

	/* jr: check for accessibility of path and set current
	   path if top window */
	   /* xxx muž noch berdacht werden */
	{
#ifdef __PUREC__
#pragma warn -aus
#endif
		WindInfo *top_file = GetTopWindInfoOfKind (WK_FILE);
		int success = 1;
		
		if (top_file == wp)
		{
			success = E_OK == setPathInfoOfWindow (wp->kind_info);
		}
#if 0	
		else
		{
			char tmp[MAX_PATH_LEN];
			
			GetFullPath (tmp, sizeof (tmp));
			success = E_OK == SetFullPath (wp->kind_info);
			SetFullPath (tmp);
		}
		
		if (!success)
		{
			WindowDestroy (wp->handle, TRUE, FALSE);
			GrafMouse (num, &form);
			return FALSE;
		}
#endif
	}

	/* Deselektiere alle Eintr„ge, da wir sie neu einlesen
	 */
	resetSelectInfo (wp);
	
	if (GetFiles (fwp))
		wp->update = new_path? (FW_PATH_CHANGED|WU_SLIDERRESET)
			: FW_PATH_CHANGED;
	else
		wp->update = FW_NEWWINDOW;
	WindowUpdateData (wp);
	

	if (strcmp (label, fwp->label))
	{
		WindNewLabel (fwp->path[0], fwp->label, label);
	}
	
	GrafMouse (num, &form);
	return TRUE;
}


static int fileWindowGetPattern (WindInfo *wp, char *pattern)
{
	FileWindow_t *fwp = wp->kind_info;

	strncpy (pattern, fwp->wildcard, WILDLEN - 1);
	pattern[WILDLEN - 1] = '\0';
	return TRUE;
}

static int fileWindowSetPattern (WindInfo *wp, const char *pattern)
{
	FileWindow_t *fwp = wp->kind_info;
	
	strncpy (fwp->wildcard, pattern, WILDLEN - 1);
	fwp->wildcard[WILDLEN - 1] = '\0';
	
	return fileWindowAddToPath (wp, NULL);
}


static void fileWindowClosed (WindInfo *wp, word kstate)
{
	FileWindow_t *fwp = wp->kind_info;
	char *first_slash, *last_slash;
	int destroy_window;
	
	destroy_window = (kstate & K_ALT);
	first_slash = strchr (fwp->title,'\\');
	last_slash = strrchr (fwp->title,'\\');
	
	if (first_slash == last_slash)
	{
		if (fwp->flags.go_to_parent)
		{
			strcpy (fwp->title, fwp->path);
			first_slash = strchr (fwp->title, '\\');
			last_slash = strrchr (fwp->title, '\\');
		}
		
		destroy_window |= (first_slash == last_slash);
	}

	if (destroy_window)
	{
		WindowDestroy (wp->handle, TRUE, FALSE);
		return;
	}

	fwp->flags.go_to_parent = FALSE;
	
	storePathUndo (wp);				/* store undo cd .. */

	StripFolderName (fwp->title);
	StripFolderName (fwp->path);

	resetSelectInfo (wp);
	
	restorePositionInfo (fwp);
	fileWindowAddToPath (wp, NULL);
}


static int fileWindowFeedKey (WindInfo *wp, word code, word kstate)
{
	if (kstate & K_CTRL)
		return FALSE;

	if (strchr ("[]*+-_.:,;|~'`#^ž?=)(/&%$Ý\"!\b\t ", code)
			|| isalnum (code))
	{
		return FileWindowCharScroll (wp, (char)(code & 0xFF), kstate);
	}
	
	return FALSE;
}

WindInfo *FileWindowOpen (const char *path, char *label, 
	const char *title, char *wildcard, word skip_lines)
{
	WindInfo *wp;
	FileWindow_t *fwp;
	char physical_label[MAX_FILENAME_LEN];
	long stat;

/*	venusErr ("path: %s, label: %s, title: %s, wildcard: %s",
		path, label, title, wildcard); */
	
	fwp = calloc (1, sizeof (FileWindow_t));
	if (fwp == NULL)
	{
		venusErr (StrError (ENSMEM));
		return NULL;
	}

	strncpy (fwp->path, path, MAX_PATH_LEN - 1);
	fwp->path[MAX_PATH_LEN - 2] = '\0';
		
	if (!LegalDrive (fwp->path[0]))
	{
		venusErr (NlsStr (T_NODRIVE), fwp->path[0]);
		free (fwp);
		return NULL;
	}

	if (fwp->path[strlen (fwp->path) - 1] != '\\')
		strcat (fwp->path, "\\"); 

	strcpy (fwp->wildcard, wildcard);
	fwp->yskip = skip_lines;
	fwp->lastPos = NULL;
	fwp->desiredSort = MenuEntryToSortMode(ShowInfo.sortentry);
	fwp->display = DisplayNone;
	fwp->desiredDisplay = ShowInfo.showtext? DisplayText :
		(ShowInfo.normicon? DisplayBigIcons : DisplaySmallIcons);

	if (label)
		strcpy (fwp->label, label);
	else
		fwp->label[0] = '\0';
				
	if (!strlen (title))				/* kein Titel da */
		strcpy (fwp->title, fwp->path);	/* Pfad als Titel */
	else
	{
		strncpy (fwp->title, title, MAX_PATH_LEN);
		fwp->title[MAX_PATH_LEN - 1] = '\0';
	}
		
	stat = DriveAccessible (fwp->path[0], physical_label, MAX_FILENAME_LEN);
	if (stat != E_OK)
	{
		free (fwp);
		venusErr (NlsStr (T_NOLABEL), path[0], StrError (stat));
		return NULL;
	}
			
	if (*fwp->label)
	{
		if (!sameLabel (physical_label, fwp->label))
		{
			GrafMouse (ARROW, NULL);
					
			if (!changeDisk (path[0], fwp->label))
			{
				free (fwp);
				return NULL;
			}
					
			GrafMouse (HOURGLASS, NULL);
		}
	}
	else
		strcpy (fwp->label, physical_label);
	
	if (!checkForExistingPath (fwp))
	{	
		free (fwp);
		return NULL;
	}
			
	/* Die g„ngigen Informationen sind gesetzt. Wir holen uns
	 * ein neues Fenster.
	 */
	
	wp = WindowCreate (FILEWIND, -1);
	
	if (wp == NULL)
	{
		free (fwp);
		return NULL;
	}
	
	wp->kind = WK_FILE;
	wp->kind_info = fwp;

	if (!PopWindBox (&wp->wind, WK_FILE))
	{
		/* Wir haben keine gr”že und auch auf dem Fensterstack
		 * ist nichts zu finden. šberlasse also den Window-Routinen
		 * die Positionierung.
		 */
		memset (&wp->wind, 0, sizeof (wp->wind));
		wp->update |= WU_POSITION;

		/* Dies ist ein kleiner, aber verzeihlicher Eingriff in
		 * die internen Strukturen der Window-Verwaltung, um die
		 * Breite und H”he des letzten Fensters zu ermitteln
		 */
		if (wnull.nextwind->nextwind
			&& (wnull.nextwind->nextwind->kind == WK_FILE))
		{
			wp->wind.g_w = wnull.nextwind->nextwind->wind.g_w;
			wp->wind.g_h = wnull.nextwind->nextwind->wind.g_h;
		}
		else
		{
			if (fwp->display == DisplayText)
			{
					wp->wind.g_w = Desk.g_w / 2;
					wp->wind.g_h = Desk.g_h / 2;
			}
			else
			{
					wp->wind.g_w = Desk.g_w / 3;
					wp->wind.g_h = Desk.g_h;
			}
		}
	}

	/* Some methods for a filewindow object. ANSI C's way of
	 * doing object orientation.
	 */
	wp->window_calc_windrect = fileWindowCalcWindRect;
	wp->window_sized = fileWindowSized;
	wp->window_moved = fileWindowMoved;
	wp->window_topped = fileWindowTopped;
	wp->window_closed = fileWindowClosed;
	wp->window_destroyed = fileWindowDestroyed;
	wp->window_update = fileWindowUpdate;
	wp->window_redraw = fileWindowRedraw;
	wp->window_get_tree = fileWindowGetTree;
	wp->window_get_statusline = fileWindowGetState;

	wp->make_new_object = fileWindowMakeNewObject;
	wp->make_alias = fileWindowMakeAlias;

	wp->window_feed_key = fileWindowFeedKey;
		
	wp->make_ready_to_drag = fileWindowMakeReadyToDrag;
	wp->getSelected = fileWindowGetSelected;
	wp->getDragged = fileWindowGetDragged;
	
	wp->copy = fileWindowCopy;
	wp->delete = fileWindowDelete;
	
	wp->mouse_click = fileWindowMouseClick;
	wp->select_all = fileWindowSelectAll;
	wp->deselect_all = fileWindowDeselectAll;
	wp->deselect_object = fileWindowDeselectObject;
	wp->select_object = fileWindowSelectObject;
	
	wp->get_display_pattern = fileWindowGetPattern;
	wp->set_display_pattern = fileWindowSetPattern;
	wp->add_to_display_path = fileWindowAddToPath;

	wp->darstellung = FileWindowDarstellung;
	
	/* So, nun holen wir uns nur noch die darzustellenden Objekte
	 * und markieren, was neu gesetzt werden muž und ”ffnen es.
	 */
	GetFiles (fwp);
	wp->update |= FW_NEWWINDOW;

	if (!WindowOpen (wp))
	{
		WindowDestroy (wp->handle, TRUE, FALSE);
		return NULL;
	}
	
	return wp;
}


/* Ein Laufwerk ist mit einem neuen Label versehen worden. Durchsuche
 * die Liste von WindInfo-Strukturen nach betroffenen Fenstern
 * (irgendwie so betroffen) und „ndere es, falls n”tig.
 */
void WindNewLabel (char drive, const char *oldlabel, 
						const char *newlabel)
{
	WindInfo *wp;
	
	wp = wnull.nextwind;
	while (wp != NULL)
	{
		switch (wp->kind)
		{
			case WK_FILE:
			{
				FileWindow_t *fwp = wp->kind_info;
				
				if ((fwp->path[0] == drive)
					&& (!strcmp (fwp->label, oldlabel)))
				{
					strcpy (fwp->label, newlabel);
				}
				break;
			}
		}
		wp = wp->nextwind;
	}
}

/*
 * get FileInfo pointer from Pointer to union
 */
FileInfo *GetFileInfo (OBJECT *po)
{
	FileInfo *pf;

	if ((po->ob_type & 0xFF) == G_USERDEF)
	{
		pf = (FileInfo *)(po->ob_spec.index - 
			offsetof (FileInfo, user_block));
	}
	else
	{
		pf = (FileInfo *)(po->ob_spec.index
			 - offsetof (FileInfo, icon)
			 - offsetof (ICON_OBJECT, icon));
	}

	if (pf->magic != 'FilE')
		venusDebug ("getfinfo failed!");

	return pf;
}


FileInfo *GetSelectedFileInfo (FileWindow_t *fwp)
{
	FileBucket *bucket;
	uword i;
	
	bucket = fwp->files;
	while (bucket)
	{
		for (i = 0; i < bucket->usedcount; ++i)
		{
			if (bucket->finfo[i].flags.selected)
				return &(bucket->finfo[i]);
		}
	
		bucket = bucket->nextbucket;
	}
	
	return NULL;
}

WindInfo *FindFileWindow (const char *path)
{
	WindInfo *wp = wnull.nextwind;
	
	while (wp)
	{
		if (wp->kind == WK_FILE)
		{
			FileWindow_t *fwp = wp->kind_info;
			if (!strcmp (fwp->path, path))
				break;
		}
		wp = wp->nextwind;
	}
	
	return wp;
}
