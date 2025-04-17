/*
 * @(#) Gemini\Init.c
 * @(#) Stefan Eissing, 30. Juli 1994
 *
 * description: all the Initialisation for the Desktop
 *
 * jr 22.4.95
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <aes.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "..\common\walkdir.h"
#include "..\common\strerror.h"

#include "vs.h"
#include "menu.rh"
#include "deskback.rh"
#include "pattbox.rh"

#include "window.h"
#include "clipbrd.h"
#include "color.h"
#include "deskwind.h"
#include "dispatch.h"
#include "draglogi.h"
#include "drives.h"
#include "erase.h"
#include "fileutil.h"
#include "gemini.h"
#include "icondrag.h"
#include "iconinst.h"
#include "image.h"
#include "redraw.h"
#include "select.h"
#include "util.h"
#include "venuserr.h"

/* Externals
 */
extern OBJECT *pnewdesk, *pstdbox, *pmenu;
extern OBJECT *psetdesk, *ppattbox;


/* internal texts
 */
#define NlsLocalSection "G.deskwind"
enum NlsLocalText{
T_IMAGE,	/*Hintergrundbild ausw„hlen*/
T_NOLOAD,	/*Bild konnte nicht geladen werden.*/
};


typedef struct
{
	word planes;
	
	unsigned char color;
	unsigned char pattern;
	unsigned char use_file;
	unsigned char use_orig_colors;
	unsigned char center_image;
	
	char filename[MAX_PATH_LEN];
	OBSPEC pattern_spec;
	IMAGEVIEW *iv;
	USERBLK userblk;
} DeskBackInfo;

static DeskBackInfo DB;

typedef struct
{
	char planes;
	char *info_line;
} PlaneInfo;

static PlaneInfo planeInfos[] =
{
	{1, NULL},
	{2, NULL},
	{4, NULL},
	{8, NULL},
	{15, NULL},
	{16, NULL},
	{24, NULL},
	{32, NULL},
};


OBJECT EmptyBackground;

static void setEmptyBackground (void)
{
	EmptyBackground = NewDesk.tree[0];
	EmptyBackground.ob_head = EmptyBackground.ob_tail = -1;
}


static int cdecl imagePaint (PARMBLK *pb)
{
	DeskBackInfo *db;
	int clip[4];
	
	db = (DeskBackInfo *)pb->pb_parm;

	if (db->iv != NULL)
	{

		clip[0] = pb->pb_xc;
		clip[1] = pb->pb_yc;
		clip[2] = clip[0] + pb->pb_wc - 1;
		clip[3] = clip[1] + pb->pb_hc - 1;
	
		db->iv->draw (db->iv, clip, db->center_image, db->pattern);
	}

	return pb->pb_currstate;
}


static int installPattern (void)
{
	DB.pattern_spec.obspec.interiorcol = DB.color;
	DB.pattern_spec.obspec.fillpattern = DB.pattern;
	NewDesk.tree[0].ob_type = G_BOX;
	NewDesk.tree[0].ob_spec = DB.pattern_spec;
	
	if (DB.iv != NULL)
	{
		DB.iv->free (DB.iv);
		DB.iv = NULL;
	}
	
	wind_set (0, WF_NEWDESK, NewDesk.tree, 0, 0);
	return TRUE;
}

static int installFile (void)
{
	if (DB.iv != NULL)
	{
		DB.iv->free (DB.iv);
		DB.iv = NULL;
	}
	
	DB.iv = LoadImage (DB.filename, vdi_handle);
	if (DB.iv == NULL)
	{
		venusErr (NlsStr (T_NOLOAD), DB.filename);
		DB.use_file = FALSE;
		installPattern ();
		return FALSE;
	}
	
	DB.iv->work[0] = Desk.g_x;
	DB.iv->work[1] = Desk.g_y;
	DB.iv->work[2] = Desk.g_x + Desk.g_w - 1;
	DB.iv->work[3] = Desk.g_y + Desk.g_h - 1;
	DB.iv->alternate_color = DB.color;
	
	if (DB.use_orig_colors)
		DB.iv->switch_colors (DB.iv, TRUE);
	
	NewDesk.tree[0].ob_spec.userblk = &DB.userblk;
	NewDesk.tree[0].ob_type = G_USERDEF;
	wind_set (0, WF_NEWDESK, NewDesk.tree, 0, 0);
	
	return TRUE;
}

void SetDeskState (char *line)
{
	DeskBackInfo db;
	union
	{
		long l;
		char c[4];
	} state;
	char *s, *dup;
	
	db = DB;
	dup = StrDup (pMainAllocInfo, line);
	strtok (line, "@\n");

	if ((s = strtok (NULL, "@\n")) != NULL)
		state.l = atol (s);

	if ((s = strtok (NULL, "@\n")) != NULL)
		strncpy (db.filename, s, sizeof (db.filename));
	else
		db.filename[0] = '\0';

	if ((s = strtok (NULL, "@\n")) != NULL)
		db.planes = atoi (s);
	else
		db.planes = 0;

	if (db.planes < 1)
		db.planes = number_of_planes;
		
	if (number_of_planes != db.planes)
	{
		int i;
		
		for (i = 0; i < DIM (planeInfos); ++i)
		{
			if (planeInfos[i].planes != db.planes)
				continue;
	
			free (planeInfos[i].info_line);
			
			dup[strlen (dup) - 1] = '\0';
			planeInfos[i].info_line = dup;
			return;
		}
		
		free (dup);
		return;
	}
	
	free (dup);
	db.color = state.c[0];
	db.pattern = state.c[1];
	db.use_file = state.c[2] & 0x01;
	db.center_image = ((state.c[2] & 0x02) != 0);
	db.use_orig_colors = state.c[3];
	
	if (DB.use_file && DB.iv != NULL)
	{
		installPattern ();
	}

	DB = db;
	
	GrafMouse (HOURGLASS, NULL);
	if (DB.use_file)
		installFile ();
	else
		installPattern ();

	setEmptyBackground ();
	GrafMouse (ARROW, NULL);
}

int WriteDeskState (MEMFILEINFO *mp, char *buffer)
{
	int ok = TRUE;
	int i;
	
	for (i = 0; ok && (i < DIM (planeInfos)); ++i)
	{
		if (planeInfos[i].planes == number_of_planes)
		{
			union
			{
				long l;
				char c[4];
			} state;
	
			state.c[0] = DB.color;
			state.c[1] = DB.pattern;
			state.c[2] = (DB.use_file | (DB.center_image << 1));
			state.c[3] = DB.use_orig_colors;

			sprintf (buffer, "#H@%ld@%s@%d", state.l, DB.filename,
				number_of_planes);
	
			ok = (mputs (mp, buffer) == 0L);
		}
		else if (planeInfos[i].info_line != NULL)
		{
			ok = (mputs (mp, planeInfos[i].info_line) == 0L);
		}
	}
	
	return ok;
}

static void deskWindowRedraw (WindInfo *wp, const GRECT *rect)
{
	OBJECT *tree = ((DeskInfo *)wp->kind_info)->tree;
	GRECT wind;

	wind_get (wp->handle, WF_FIRSTXYWH, &wind.g_x, &wind.g_y,
		&wind.g_w, &wind.g_h);

	while (wind.g_w && wind.g_h)
	{
		if (GRECTIntersect (rect, &wind))
		{
			objc_draw (tree, 0, MAX_DEPTH, wind.g_x, 
				wind.g_y, wind.g_w, wind.g_h);
		}	

		wind_get (wp->handle, WF_NEXTXYWH, &wind.g_x, &wind.g_y,
			&wind.g_w, &wind.g_h);
	}
}

static OBJECT *deskWindowGetTree (WindInfo *wp)
{
	return ((DeskInfo *)wp->kind_info)->tree;
}


static void deskWindowTopped (WindInfo *wp)
{
	(void)wp;
	menu_ienable (pmenu, WINDCLOS, FALSE);
	menu_ienable (pmenu, CLOSE, FALSE);
}


static int deskWindowCopy (WindInfo *wp, int only_asking)
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

static int realErase (WindInfo *wp)
{
	OBJECT *tree;
	IconInfo *pii;
	word i, realerase;
	GRECT rect;
	
	tree = ((DeskInfo *)wp->kind_info)->tree;
	realerase = FALSE;

	for(i = tree[0].ob_head; i > 0; i = tree[i].ob_next)
	{
		if (!ObjectWasDragged (&tree[i]))
			continue;

		if ((pii = GetIconInfo (&tree[i])) == NULL)
			continue;
			
		switch (pii->type)
		{
			case DI_SHREDDER:
				break;

			case DI_FILESYSTEM:
			case DI_PROGRAM:
			case DI_FOLDER:
				UnMarkDraggedObject (wp, &tree[i]);
				DeselectObject (wp, i, TRUE);
				objc_offset (tree, i, &rect.g_x, &rect.g_y);
				rect.g_w = tree[i].ob_width;
				rect.g_h = tree[i].ob_height;

				RemoveDeskIcon (i);
				if (pii->type == DI_FILESYSTEM)
					DriveWasRemoved (pii->fullname[0]);

				buffredraw (wp, &rect);
				break;

			default:
				realerase = TRUE;
				break;
		}
	}
	
	flushredraw ();	
	return realerase;
}

static int deskWindowDelete (WindInfo *wp, int only_asking)
{
	int retcode = FALSE;
	
	if (only_asking)
		return wp->selectAnz > 0;

	MarkDraggedObjects (wp);	
	if (realErase (wp))
		retcode = DeleteDraggedObjects (wp);
		
	UnMarkDraggedObjects (wp);
	return retcode;
}


void ReInstallBackground (int redraw)
{
	if (DB.iv != NULL && DB.iv->orig_colors_set)
		DB.iv->switch_colors (DB.iv, TRUE);
		
	wind_set (0, WF_NEWDESK, NewDesk.tree, 0, 0);
	if (redraw)
		deskWindowRedraw (&wnull, &Desk);
}


static void deskWindowMouseClick (WindInfo *wp, word clicks,
	word mx, word my, word kstate)
{
	if (!PointInGRECT (mx, my, &Desk))
		return;
		
	if (clicks == 2)
	{
		word x, y, bs, ks;
		
		if (ButtonPressed (&x, &y, &bs, &ks))
		{
			while (!ButtonReleased (&x, &y, &bs, &ks))
				;
		}
		
		doDclick (wp, NewDesk.tree, FindObject (NewDesk.tree, mx, my), 
			kstate);
	}
	else
	{
		doIcons (wp, mx, my, kstate);
	}
}


static int deskWindowSelectObject (WindInfo *wp, word obj, int draw)
{
	DeskInfo *dip = wp->kind_info;
			
	dip->tree[obj].ob_state |= SELECTED;
	++wp->selectAnz;
	++NewDesk.total_selected;
	if (draw)
		redrawObj (wp, obj);
	
	return TRUE;
}


static int deskWindowDeselectObject (WindInfo *wp, word obj, int draw)
{
	DeskInfo *dip = wp->kind_info;
			
	dip->tree[obj].ob_state &= ~SELECTED;
	--wp->selectAnz;

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


static void deskWindowDeselectAll (WindInfo *wp)
{
	DeskInfo *dip = wp->kind_info;
	register word j;

	if (wp->selectAnz < 1)
		return;

	for (j = dip->tree[0].ob_head; j > 0; j = dip->tree[j].ob_next)
	{
		/* deselektiere type, oder wenn type null ist
		 * deselektiere alle Objekte
		 */
		if (dip->tree[j].ob_state & SELECTED)
		{
			wp->deselect_object (wp, j, TRUE);
		}
	}
	
	flushredraw ();
}


static word deskWindowSelectAll (WindInfo *wp)
{
	DeskInfo *dip = wp->kind_info;
	int i;
	
	DeselectAllExceptWindow (wp);
	
	for (i = dip->tree[0].ob_head; i > 0; i = dip->tree[i].ob_next)
	{
		if (isSelected (dip->tree, i))
			continue;

		setSelected (dip->tree, i, TRUE);
		redrawObj (wp, i);
		++wp->selectAnz;
		++NewDesk.total_selected;
	}
	
	flushredraw ();
	return TRUE;
}


static int deskWindowGetDragged (WindInfo *wp, ARGVINFO *A,
	int preferLowerCase)
{
	DeskInfo *dip = wp->kind_info;
	word objnr;
	IconInfo *pii;
	char *name;
	char drivename[4];

	if (!MakeGenericArgv (pMainAllocInfo, A, TRUE, 0, NULL))
		return FALSE;
	
	/* walk children
	 */
	for (objnr = dip->tree[0].ob_head; 
		objnr > 0;
		objnr = dip->tree[objnr].ob_next)
	{
		if (isHidden (dip->tree, objnr)
			|| !ObjectWasDragged (&dip->tree[objnr]))
			continue;

		pii = GetIconInfo (&dip->tree[objnr]);
		if (!pii)
			continue;
		
		name = NULL;
		switch (pii->type)
		{
			case DI_FILESYSTEM:
				sprintf (drivename, "%c:\\", toupper (pii->fullname[0]));
				name = drivename;
				break;
				
			case DI_TRASHCAN:
			case DI_SCRAPDIR:
			{
				WalkInfo W;
				char path[MAX_PATH_LEN];
				char filename[MAX_PATH_LEN];
				long stat;
				int ok = TRUE;
				
				if (!preferLowerCase)
				{
					name = pii->fullname;
					break;
				}
				
				strcpy (path, pii->fullname);
				
				stat = InitDirWalk (&W, WDF_ATTRIBUTES|WDF_GEMINI_LOWER,
					path, sizeof filename, filename);

				if (!stat)
				{
					while (ok && ((stat = DoDirWalk (&W)) == 0L))
					{
						if (!strcmp (filename, ".")
							|| !strcmp (filename, ".."))
							continue;
						
						strcpy (path, pii->fullname);
						AddFileName (path, filename);
						ok = StoreInArgv (A, path);
					}
					
					ExitDirWalk (&W);
				} 
				else if (stat != EFILNF && stat != EPTHNF
					 && stat != ENMFIL)
				{
					venusErr (StrError (stat));
					ok = FALSE;
				}
				
				if (!ok)
				{
					FreeArgv (A);
					return FALSE;
				}
				break;
			}

			case DI_PROGRAM:
			case DI_FOLDER:
				name = pii->fullname;
				break;
		}
		
		if ((name != NULL) && !StoreInArgv (A, name))
		{
			FreeArgv (A);
			return FALSE;
		}
	}
	
	return TRUE;
}

static int deskWindowGetSelected (WindInfo *wp, ARGVINFO *A,
	int preferLowerCase)
{
	int retcode;
	
	MarkDraggedObjects (wp);
	retcode = deskWindowGetDragged (wp, A, preferLowerCase);
	UnMarkDraggedObjects (wp);
	
	return retcode;
}


static int feedKey (WindInfo *wp, word c, word kstate)
{
	(void) kstate;
	
	if ((c&0xff) != ' ')
		return FALSE;
	
	DeselectAll (wp);
	return TRUE;
}

void DeskWindowOpen (void)
{
	wnull.handle = 0;
	wnull.work = Desk;

	wnull.kind = WK_DESK;
	wnull.kind_info = &NewDesk;
	
	wnull.window_topped = deskWindowTopped;
	wnull.window_redraw = deskWindowRedraw;
	wnull.window_get_tree = deskWindowGetTree;
	wnull.copy = deskWindowCopy;
	wnull.delete = deskWindowDelete;
	wnull.mouse_click = deskWindowMouseClick;
	wnull.select_all = deskWindowSelectAll;
	wnull.deselect_all = deskWindowDeselectAll;
	wnull.deselect_object = deskWindowDeselectObject;
	wnull.select_object = deskWindowSelectObject;

	wnull.window_feed_key = feedKey;
	wnull.getDragged = deskWindowGetDragged;
	wnull.getSelected = deskWindowGetSelected;
	
	NewDesk.tree = pnewdesk;
	NewDesk.objanz = countObjects (NewDesk.tree, 0);
	wnull.selectAnz = NewDesk.total_selected = 0;
	NewDesk.tree[0].ob_x = Desk.g_x;
	NewDesk.tree[0].ob_y = Desk.g_y;
	NewDesk.tree[0].ob_width = Desk.g_w;
	NewDesk.tree[0].ob_height = Desk.g_h;
	pstdbox[0].ob_x = Desk.g_x;
	pstdbox[0].ob_y = Desk.g_y;
	pstdbox[0].ob_width = Desk.g_w;
	pstdbox[0].ob_height = Desk.g_h;

	IsTopWindow (wnull.handle);
	
	/* Desktophintergrund initialisieren
	 */
	DB.pattern = (number_of_colors < 3)? IP_4PATT : IP_SOLID;
	DB.color = GREEN;
	DB.filename[0] = '\0';
	DB.use_file = DB.use_orig_colors = DB.center_image = FALSE;
	DB.userblk.ub_code = imagePaint;
	DB.userblk.ub_parm = (long)&DB;
	DB.iv = NULL;
	DB.pattern_spec = NewDesk.tree[0].ob_spec;

	installPattern ();

	CopyNewDesk (pnewdesk);
	wind_set (0, WF_NEWDESK, NewDesk.tree, 0, 0);
	setEmptyBackground ();
}


void freeDeskTree (void)
{
	IconInfo *pii;
	OBJECT *po;
	int i;
	
	po = NewDesk.tree;
	
	for (i = po[0].ob_head; i > 0; i = po[i].ob_next)
	{
		if (po[i].ob_flags & HIDETREE
			|| po[i].ob_type != G_USERDEF)
			continue;
			
		pii = GetIconInfo (&po[i]);
		if (pii)
			FreeIconInfo (pii);
	}

	free (po);
	NewDesk.tree = NewDesk.tree = NULL;
	NewDesk.maxobj = NewDesk.objanz = 0;
	NewDesk.total_selected -= wnull.selectAnz;
	wnull.selectAnz = 0;
	
	if (DB.iv != NULL)
	{
		DB.iv->free (DB.iv);
		DB.iv = NULL;
	}
	DB.use_file = FALSE;
	
	for (i = 0; i < DIM (planeInfos); ++i)
	{
		free (planeInfos[i].info_line);
		planeInfos[i].info_line = NULL;
	}
}


void DeskWindowClose (int make_background)
{
	freeDeskTree ();
	wind_set (0, WF_NEWDESK, NULL, 0, 0);
	
	if (make_background)
		form_dial (FMD_FINISH, 0, 0, 0, 0, 0, 0,
			Desk.g_x + Desk.g_w,Desk.g_y + Desk.g_h);
}


int SetDeskBackground (void)
{
	DIALINFO D;
	word retcode;
	char color, pattern;
	int exit, draw;
	word cycle_obj;
	char filename[MAX_PATH_LEN];
	
	pattern = DB.pattern;
	psetdesk[DBPATBX].ob_spec = ppattbox[pattern+2].ob_spec;
	
	strcpy (filename, DB.filename);
	GetBaseName (filename, 
		psetdesk[DBNAME].ob_spec.free_string, MAX_FILENAME_LEN);

	color = DB.color;
	ColorSetup (color, psetdesk, -1, -1, DBCOLTXT, DBCOLBX);
	
	setSelected (psetdesk, DBUSEPIC, DB.use_file);
	setDisabled (psetdesk, DBUSEPIC, !filename[0]);
	setSelected (psetdesk, DBPALET, DB.use_orig_colors);
	setSelected (psetdesk, DBCENTER, DB.center_image);
				
	sprintf (psetdesk[DBSNX].ob_spec.tedinfo->te_ptext,
		 "%2d", NewDesk.snapx);
	sprintf (psetdesk[DBSNY].ob_spec.tedinfo->te_ptext,
		 "%2d", NewDesk.snapy);

	DialCenter (psetdesk);

	DialStart (psetdesk, &D);
	DialDraw (&D);
	
	exit = FALSE;
	do
	{
		word shift_state;
	
		draw = FALSE;
		cycle_obj = -1;
		
		retcode = DialXDo (&D, NULL, &shift_state, NULL, NULL, NULL, NULL);
		retcode &= 0x7FFF;

		switch (retcode)
		{
			case DBCOL0:
				cycle_obj = DBCOL0;
			case DBCOLSC:
				if (shift_state & 3)
					cycle_obj = DBCOL0;
			case DBCOLTXT:
				color = ColorSelect (color, FALSE, psetdesk,
							DBCOLTXT, DBCOLBX, cycle_obj);
				break;

			case DBPAT0:
				cycle_obj = DBPAT0;
			case DBPATSC:
				if (shift_state & 3)
					cycle_obj = DBPAT0;
			case DBPATBX:
				if (cycle_obj > 0)
				{
					retcode = JazzCycle (psetdesk, 
						DBPATBX, cycle_obj, ppattbox, 1);
				}
				else
				{
					retcode = JazzId (psetdesk, DBPATBX, -1, 
							ppattbox, 1);
				}
				
				if (retcode > 0)
					pattern = psetdesk[DBPATBX].ob_spec.obspec.fillpattern;
				break;
				
			case DBDATEI:
			case DBNAME:
				/* Wir brauchen leider den Fileselector
				 */
				DialEnd (&D);

				if (GetFileName (filename, "*.IMG", NlsStr (T_IMAGE)))
				{
					GetBaseName (filename, 
						psetdesk[DBNAME].ob_spec.free_string, 
						MAX_FILENAME_LEN);
					setDisabled (psetdesk, DBUSEPIC, !filename[0]);
				}
				
				DialStart (psetdesk, &D);
				DialDraw (&D);
				break;
				
			default:
				draw = exit = TRUE;
				break;
		}

		if (draw)
		{				
			setSelected (psetdesk, retcode, FALSE);
			fulldraw (psetdesk, retcode);
		}

	} 
	while (!exit);

	DialEnd (&D);
	
	if (retcode == DBOK)
	{
		int use_file, use_orig, center;
		char changed, changed_col, changed_patcol,
			changed_center, changed_pattern;
		
		NewDesk.snapx = 
			atoi (psetdesk[DBSNX].ob_spec.tedinfo->te_ptext);
		NewDesk.snapy = 
			atoi (psetdesk[DBSNY].ob_spec.tedinfo->te_ptext);

		use_file = isSelected (psetdesk, DBUSEPIC);
		use_orig = isSelected (psetdesk, DBPALET);
		center = isSelected (psetdesk, DBCENTER);
		
		changed = ((use_file != DB.use_file)
			|| (strcmp (filename, DB.filename)));
		changed_pattern = (DB.pattern != pattern);
		changed_patcol = (color != DB.color);
		changed_col = ((use_orig != DB.use_orig_colors)
			|| changed_patcol);
		changed_center = (center != DB.center_image);
		
		DB.use_file = use_file;
		DB.use_orig_colors = use_orig;
		DB.color = color;
		if (DB.iv != NULL)
			DB.iv->alternate_color = color;
		DB.pattern = pattern;
		DB.center_image = center;
		strcpy (DB.filename, filename);
		
		if (changed)
		{
			GrafMouse (HOURGLASS, NULL);
			if (DB.use_file)
				changed = installFile ();
			else
				changed = installPattern ();
			GrafMouse (ARROW, NULL);
		}
		else if (changed_col || changed_pattern)
		{
			if (DB.use_file)
			{
				if (!changed_patcol || DB.iv->planes == 1)
				{
					DB.iv->switch_colors (DB.iv, DB.use_orig_colors);
					changed = (DB.iv->planes == 1);
				}

				if (center && changed_patcol)
					changed = TRUE;
			}
			else
			{
				installPattern ();
				changed = TRUE;
			}
		}
		
		setEmptyBackground ();
		if (changed || 
			(DB.use_file && (changed_center || changed_pattern)))
			deskWindowRedraw (&wnull, &Desk);
		
		return changed;
	}
	
	return FALSE;
}
