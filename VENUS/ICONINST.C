/*
 * @(#) Gemini\Iconinst.c
 * @(#) Stefan Eissing, 01. November 1994
 *
 * description: Installation von Icons auf dem Desktop
 *
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <flydial\flydial.h>
#include <flydial\listman.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\argvinfo.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"

#include "vs.h"
#include "trashbox.rh"
#include "scrapbox.rh"
#include "shredbox.rh"

#include "window.h"
#include "color.h"
#include "filewind.h"
#include "fileutil.h"
#include "gemtrees.h"
#include "icondraw.h"
#include "iconinst.h"
#include "drives.h"
#include "iconrule.h"
#include "myalloc.h"
#include "order.h"
#include "redraw.h"
#include "select.h"
#include "scut.h"
#include "stand.h"
#include "util.h"
#include "venuserr.h"


/* externals
 */
extern OBJECT *pshredbox,*ptrashbox,*pscrapbox;

/*
 * internal strings
 */
#define NlsLocalSection "G.iconinst"
enum NlsLocalText{
T_ERASER,	/*REISSWOLF*/
T_PAPER,	/*PAPIERKORB*/
T_CLIP,		/*KLEMMBRETT*/
T_NOSTAT,	/*Konnte Attribute von Object %s nicht ermitteln.*/
};


/* Schrittweite der Vergr”žerung des Desktopbaums
 */
#define DESKINCR		10

/* defaultm„žige Farbe fr Hintergrundicons
 */
#define DEFCOLOR		0x10


/*
 * make Pointer to IconInfo from pointer
 * to its inner ICONBLK
 */
IconInfo *GetIconInfo (OBJECT *po)
{
	IconInfo *pii;
	
	if (!po || !po->ob_spec.index)
		return NULL;
		
	pii = (IconInfo *)(po->ob_spec.index - offsetof (IconInfo, user));

	if (pii->magic != ICON_MAGIC)
	{
		venusDebug ("getIconInfo at %p failed!", po);
		return NULL;
	}

	return pii;
}


void FreeIconInfo (IconInfo *pii)
{
	free (pii->fullname);
	free (pii);
}

 
/*
 * switch RSC-NewDesk on own array, so we can fumble around
 * with it
 */
void CopyNewDesk (OBJECT *pnewdesk)
{
	NewDesk.maxobj = NewDesk.objanz = countObjects (pnewdesk, 0);
	if(NewDesk.objanz < DESKINCR)
		NewDesk.maxobj = DESKINCR;
	
	NewDesk.tree = (OBJECT *)malloc (NewDesk.maxobj * sizeof (OBJECT));
	
	memcpy (NewDesk.tree, pnewdesk, NewDesk.objanz * sizeof (OBJECT));
}

/*
 * get number of first HIDETREE Object in tree
 * or -1 if none exists
 */
static word getHiddenObject (OBJECT *tree)
{
	word index = 1;
	
	if(tree[0].ob_head != -1)
	{
		do
		{
			if(tree[index].ob_flags & HIDETREE)
				return index;
			index = tree[index].ob_next;
		} 
		while (index != 0);			/* back at root of tree */
	}
	
	return -1;
}

/*
 * add an icon to the object tree for window 0
 */
void AddDeskIcon (word obx, word oby, word todraw,
					OBJECT *po, USERBLK *user)
{
	OBJECT *tmp;
	word objnr, oldnext;
	
	if ((objnr = getHiddenObject (NewDesk.tree)) > 0)
	{	
		/* ein hidden Objekt gefunden
		 */
		oldnext = NewDesk.tree[objnr].ob_next;
		memcpy (&NewDesk.tree[objnr], po, sizeof (OBJECT));
		NewDesk.tree[objnr].ob_next = oldnext;
	}
	else 
	{
		/* Array vergr”žern
		 */
		if(NewDesk.objanz == NewDesk.maxobj)
		{
			NewDesk.maxobj += DESKINCR;
			tmp = malloc (NewDesk.maxobj * sizeof (OBJECT));

			if (tmp == NULL)
			{
				/* nicht genug Speicher
				 */
				NewDesk.maxobj -= DESKINCR;
				return;	
			}

			memcpy (tmp, NewDesk.tree, NewDesk.objanz * sizeof (OBJECT));
			free (NewDesk.tree);
			NewDesk.tree = tmp;
			wind_set (0, WF_NEWDESK, NewDesk.tree, 0, 0);
		}
		
		memcpy (&NewDesk.tree[NewDesk.objanz], po, sizeof (OBJECT));
	
		/* Tree war leer
		 */
		if(NewDesk.tree[0].ob_head == -1)
		{
			objnr = NewDesk.objanz;
			NewDesk.tree[0].ob_head = objnr;
		}
		else
		{
			/* fge es in die Kette von Objekten ein
			 */
			objnr = NewDesk.tree[0].ob_tail;
			objnr = NewDesk.tree[objnr].ob_next = NewDesk.objanz;
		}
		
		NewDesk.tree[0].ob_tail = objnr;
		NewDesk.tree[objnr].ob_next = 0;
		NewDesk.tree[objnr].ob_head = -1;
		NewDesk.tree[objnr].ob_tail = -1;
	}

	NewDesk.objanz = countObjects (NewDesk.tree, 0);
	NewDesk.tree[objnr].ob_type = G_USERDEF;
	NewDesk.tree[objnr].ob_spec.userblk = user;
	
	/* Setze die Breite der Iconfahne
	 */
	SetIconTextWidth (&((ICON_OBJECT *)user->ub_parm)->icon,
		&NewDesk.tree[objnr]);
	NewDesk.tree[objnr].ob_x = obx;
	NewDesk.tree[objnr].ob_y = oby;
	
	if (!todraw || !NewDesk.snapAlways)
	{
		/* Plaziere es erneut, da sich die Gr”že ge„ndert haben kann
		 */
		deskAlign (&NewDesk.tree[objnr]);
	}
	else
	{
		SnapObject (&wnull, objnr, todraw);
		todraw = FALSE;
	}

	sortTree (NewDesk.tree, 0, 1, D_SOUTH, D_WEST);
	
	/* Markiere das letzte Objekt mit LASTOB
	 */
	SetLastObject (NewDesk.tree);
	
	if (todraw)
		redrawObj (&wnull, objnr);
}


void FillIconObject (ICON_OBJECT *icon, OBJECT *po)
{
	memset (&icon->flags, 0, sizeof (icon->flags));
	memcpy (&icon->icon, po->ob_spec.iconblk, sizeof (ICONBLK));
	if ((po->ob_type & 0xff) == G_CICON)
	{
		icon->mainlist = 
			((CICONBLK *)po->ob_spec.free_string)->mainlist;
	}
	else
		icon->mainlist = NULL;
	icon->flags.is_icon = 1;
}

int IconsAreEqual (const OBJECT *po, const ICONBLK *icon2,
	char color)
{
	ICONBLK *icon1 = po->ob_spec.iconblk;
	int new_char = (color << 8) | (icon2->ib_char & 0x00FF);

	return ((icon1->ib_pmask == icon2->ib_pmask)
		&& (icon1->ib_pdata == icon2->ib_pdata)
		&& (new_char == icon2->ib_char));
}

void ChangeIconObject (ICON_OBJECT *icon, OBJECT *new_icon,
	char color)
{
	ICONBLK *new_ib = new_icon->ob_spec.iconblk;
	int new_char = (color << 8) | (new_ib->ib_char & 0x00FF);
	int tmp;

	icon->icon.ib_pdata = new_ib->ib_pdata;
	icon->icon.ib_pmask = new_ib->ib_pmask;
	icon->icon.ib_char = new_char;

	icon->icon.ib_xchar = new_ib->ib_xchar;
	icon->icon.ib_ychar = new_ib->ib_ychar;

	tmp = (icon->icon.ib_wicon - new_ib->ib_wicon) / 2;
	icon->icon.ib_xicon += tmp;
	tmp = (icon->icon.ib_hicon - new_ib->ib_hicon) / 2;
	icon->icon.ib_yicon += tmp;
	icon->icon.ib_wicon = new_ib->ib_wicon;
	icon->icon.ib_hicon = new_ib->ib_hicon;
	
	icon->icon.ib_ytext = new_ib->ib_ytext;
	if ((new_icon->ob_type & 0xff) == G_CICON)
	{
		icon->mainlist = 
			((CICONBLK *)new_icon->ob_spec.free_string)->mainlist;
	}
	else
		icon->mainlist = NULL;
}

void InitIconInfo (IconInfo *pi, OBJECT *po, 
	const char *name, char true_color)
{
	FillIconObject (&pi->icon, po);
	
	if (strcmp (name, "\a"))
		strcpy (pi->iconname, name);
	else
		pi->iconname[0] = '\0';
		
	pi->icon.icon.ib_ptext = pi->iconname;
	
	pi->icon.icon.ib_char = (pi->icon.icon.ib_char & 0xFF)
		| (ColorMap (true_color) << 8);

	pi->user.ub_parm = (long)&pi->icon;
	pi->user.ub_code = DrawIconObject;

	pi->truecolor = true_color;
}


/*
 * install icon for scrapdir on desktop at position obx,oby
 * with name 'name'
 */
void InstScrapIcon (word obx, word oby, const char *name,
					word shortcut, char true_color)
{
	IconInfo *pi;
	OBJECT *po;
	
	if ((pi = malloc (sizeof (IconInfo))) != NULL)
	{
		pi->magic = ICON_MAGIC;
		pi->type = DI_SCRAPDIR;
		pi->defnumber = 4;
		pi->altnumber = 5;
		pi->shortcut = shortcut;
		SCutInstall (pi->shortcut);

		pi->fullname = getSPath (NULL);
		if (pi->fullname == NULL)
		{
			free (pi);
			return;
		}
		ReadLabel (pi->fullname[0], pi->label, MAX_FILENAME_LEN);

		if (isEmptyDir (pi->fullname))
			po = GetDeskObject (pi->defnumber);
		else
			po = GetDeskObject (pi->altnumber);
	
		InitIconInfo (pi, po, name, true_color);		
		 
		AddDeskIcon (obx, oby, FALSE, po, &pi->user);
		NewDesk.scrapNr = getScrapIcon ();
	}
	
	flushredraw();
}

/*
 * install shredder icon on desktop
 */
void InstShredderIcon (word obx, word oby, const char *name, 
	char truecolor)
{
	IconInfo *pi;
	OBJECT *po;
	
	if((pi = malloc (sizeof (IconInfo))) != NULL)
	{
		pi->magic = ICON_MAGIC;
		pi->type = DI_SHREDDER;
		pi->defnumber = 1;
		pi->altnumber = 0;
		pi->shortcut = 0;
		pi->fullname = NULL;
		
		po = GetDeskObject (pi->defnumber);
		
		InitIconInfo (pi, po, name, truecolor);

		AddDeskIcon (obx, oby, FALSE, po, &pi->user);
	}
	flushredraw();
}

/*
 * install icon for paperbasket on desktop
 */
void InstTrashIcon (word obx, word oby, const char *name,
					word shortcut, char truecolor)
{
	IconInfo *pii;
	OBJECT *po;
	
	if ((pii = malloc (sizeof (IconInfo))) != NULL)
	{
		pii->magic = ICON_MAGIC;
		pii->type = DI_TRASHCAN;
		pii->defnumber = 2;
		pii->altnumber = 3;
		pii->shortcut = shortcut;
		SCutInstall (pii->shortcut);
		
		strcpy (pii->iconname, name);

		pii->fullname = getTPath (NULL);
		if (pii->fullname == NULL)
		{
			free (pii);
			return;
		}
		ReadLabel (pii->fullname[0], pii->label, MAX_FILENAME_LEN);

		if (isEmptyDir (pii->fullname))
			po = GetDeskObject (pii->defnumber);
		else
			po = GetDeskObject (pii->altnumber);

		InitIconInfo (pii, po, name, truecolor);

		AddDeskIcon (obx, oby, FALSE, po, &pii->user);
		NewDesk.trashNr = getTrashIcon ();
	}
	flushredraw();
}


/*
 * install icon for program on desktop
 */
void InstPrgIcon (word obx, word oby, word todraw, word normicon,
				word isfolder, const char* path, const char *name,
				char *label, word shortcut)
{
	IconInfo *pi;
	OBJECT *po;
	char fname[MAX_FILENAME_LEN];
	
	if ((pi = malloc (sizeof (IconInfo))) != NULL)
	{
		char color;
		
		GetBaseName (path, fname, MAX_FILENAME_LEN);
		pi->magic = ICON_MAGIC;
		pi->type = (isfolder)? DI_FOLDER : DI_PROGRAM;
		pi->defnumber = normicon;	/* grož oder klein */
		pi->altnumber = 0;
		pi->shortcut = shortcut;
		SCutInstall (pi->shortcut);

		pi->fullname = StrDup (pMainAllocInfo, path);
		if (pi->fullname == NULL)
		{
			free (pi);
			return;
		}

		if (label)
			strcpy (pi->label, label);
		else
			pi->label[0] = '\0';

		
		if (normicon)
			po = GetBigIconObject (isfolder, fname, &color);
		else
			po = GetSmallIconObject (isfolder, fname, &color);
		
		InitIconInfo (pi, po, name, color);
		AddDeskIcon (obx, oby, todraw, po, &pi->user);
	}
	flushredraw();
}

/*
 * install default icon on desktop, called if there is
 * nothing in venus.inf
 */
void AddDefIcons (void)
{
	OBJECT *obp;
	
	obp = GetStdDeskIcon ();
	
	InstShredderIcon (0, 0, NlsStr (T_ERASER), DEFCOLOR);
	InstTrashIcon (0, obp->ob_height, NlsStr (T_PAPER), 0, DEFCOLOR);
	InstScrapIcon (0, 2 * obp->ob_height, NlsStr (T_CLIP), 0, 
		DEFCOLOR);
	InstDriveIcon (0, 3 * obp->ob_height, FALSE, 6, BootPath[0],
		DEFAULTDEVICE, 0, DEFCOLOR);
}

/*
 * remove an icon from desktop object tree
 */
void RemoveDeskIcon (word objnr)
{
	IconInfo *pii;
	
	if ((NewDesk.tree[objnr].ob_type & 0xFF) != G_USERDEF)
		return;
	
	if (isSelected (NewDesk.tree, objnr))
		DeselectObject (&wnull, objnr, FALSE);
	
	pii = GetIconInfo (&NewDesk.tree[objnr]);
	if (pii)
	{
		SCutRemove (pii->shortcut);
		FreeIconInfo (pii);
	}
	/* Hide this object */
	NewDesk.tree[objnr].ob_flags |= HIDETREE;
	/* Object has harmless type
	 */
	NewDesk.tree[objnr].ob_type = G_BOX;
	/* and no state
	 */
	NewDesk.tree[objnr].ob_state = 0;
	NewDesk.tree[objnr].ob_spec.free_string = NULL;
	
	NewDesk.objanz = countObjects (NewDesk.tree, 0);
}


int DoDefDialog (IconInfo *pii, word objnr, OBJECT *tree,
			const word nameindex, const word okindex, 
			const word foreindex, const word fboxindex,
			const word backindex, const word bboxindex,
			const word scindex, const word forecirc,
			const word backcirc, const word sccirc,
			const word forescsc, const word backscsc,
			const word shortscsc)
{
	DIALINFO d;
	char dialogname[MAX_FILENAME_LEN];
	word retcode, color, exit;
	word shortcut, circle;
	int edit_object = nameindex;
	

	strcpy (dialogname, pii->iconname);
	tree[nameindex].ob_spec.tedinfo->te_ptext = dialogname;
	
	shortcut = pii->shortcut;
	if (scindex > 0)
		tree[scindex].ob_spec = SCut2Obspec (shortcut);
	
	color = pii->truecolor;
	ColorSetup (color, tree, foreindex, fboxindex,
		backindex, bboxindex);
	
	DialCenter (tree);
	DialStart (tree, &d);
	DialDraw (&d);
	
	do
	{
		circle = FALSE;
		exit = TRUE;
		
		retcode = DialDo (&d, &edit_object) & 0x7FFF;
		setSelected (tree, retcode, FALSE);
		
		if (retcode == forecirc)
		{
			retcode = foreindex;
			circle = forecirc;
		}
		
		if ((retcode == foreindex) || (retcode == forescsc))
		{
			color = ColorSelect (color, TRUE, tree,
			 			foreindex, fboxindex, circle);
			 exit = FALSE;
		}
		
		if (retcode == backcirc)
		{
			retcode = backindex;
			circle = backcirc;
		}
		
		if ((retcode == backindex) || (retcode == backscsc))
		{
			color = ColorSelect (color, FALSE, tree,
			 			backindex, bboxindex, circle);
			 exit = FALSE;
		}
		
		if (retcode == sccirc)
		{
			retcode = scindex;
			circle = sccirc;
		}
		
		if ((retcode == scindex) || (retcode == shortscsc))
		{
			word retcut;
			
			retcut = SCutSelect (tree, scindex, 
						shortcut, pii->shortcut, circle);
			if (retcut != shortcut)
			{
				shortcut = retcut;
				tree[scindex].ob_spec = SCut2Obspec (shortcut);
				fulldraw (tree, scindex);
			}

			exit = FALSE;
		}
	}
	while (!exit);
	
	DialEnd (&d);

	if (retcode == okindex)
	{
		redrawObj (&wnull, objnr);
		strcpy (pii->iconname, dialogname);
		pii->shortcut = shortcut;
		pii->truecolor = color;
		pii->icon.icon.ib_char = (pii->icon.icon.ib_char & 0xFF)
							| (ColorMap (color)<<8);
		SetIconTextWidth (&pii->icon.icon, &NewDesk.tree[objnr]);			
		deskAlign (&NewDesk.tree[objnr]);
		redrawObj (&wnull, objnr);
		flushredraw ();
	}

	return (retcode == okindex);
}

int DoShredderDialog (IconInfo *pii, word objnr)
{
	return DoDefDialog (pii, objnr, pshredbox, SHREDNAM, SHREDOK,
			SHREFORE, SHREFBOX, SHREBACK, SHREBBOX,
			-1, SHREFR0, SHREBK0, -1, SHRDFGSC, SHRDBGSC, -1);
}

/*
 * do dialog for installing or editing deskicons
 */
void DoInstDialog (void)
{
	WindInfo *wp;
	IconInfo *pii;
	word objnr;
	
	if (GetOnlySelectedObject (&wp, &objnr) && (wp->kind == WK_DESK))
	{
		DeskInfo *dip = wp->kind_info;
		
		pii = GetIconInfo (&dip->tree[objnr]);
		switch (pii->type)
		{
			case DI_FILESYSTEM:
				DriveInstDialog (pii->fullname);
				break;

			case DI_SHREDDER:
				DoShredderDialog (pii, objnr);
				break;

			case DI_TRASHCAN:
				DoDefDialog (pii, objnr, ptrashbox, TRASHNAM, TRASHOK,
						TRASFORE, TRASFBOX, TRASBACK, TRASBBOX,
						TRASHSC, TRASFR0, TRASBK0, TRASHSC0,
						TRSHFGSC, TRSHBGSC, TRSHSCSC);
				break;

			case DI_SCRAPDIR:
				DoDefDialog (pii, objnr, pscrapbox, SCRAPNAM, SCRAPOK,
						SCRAFORE, SCRAFBOX, SCRABACK, SCRABBOX,
						SCRAPSC, SCRAFR0, SCRABK0, SCRAPSC0,
						SCRPFGSC, SCRPBGSC, SCRPSCSC);
				break;

			default:
				DriveInstDialog (NULL);
		}
	}
	else
		DriveInstDialog (NULL);
}


/*
 * write informations about current desktopicons
 * to file *fp (probably venus.inf)
 */
int WriteDeskIcons (MEMFILEINFO *mp, char *buffer)
{
	IconInfo *pii;
	word i,obx, oby, write;
	char *name;
	
	for (i = NewDesk.tree[0].ob_head; i > 0; 
		i = NewDesk.tree[i].ob_next)
	{
		if (isHidden (NewDesk.tree, i)
			|| ((NewDesk.tree[i].ob_type & 0x00ff) != G_USERDEF))
		{
			continue;
		}

		pii = GetIconInfo (&NewDesk.tree[i]);
		if (!pii)
			continue;
			
		objc_offset (NewDesk.tree, i, &obx, &oby);
		obx = (word)scale123 (ScaleFactor, obx - Desk.g_x, Desk.g_w);
		oby = (word)scale123 (ScaleFactor, oby - Desk.g_y, Desk.g_h);

		write = TRUE;
		if (pii->iconname[0])
			name = pii->iconname;
		else
			name = "\a";
			
		switch (pii->type)
		{
			case DI_FILESYSTEM:
				RegisterDrivePosition (pii->fullname[0], 
					NewDesk.tree[i].ob_x, NewDesk.tree[i].ob_y);
				write = FALSE;
				break;

			case DI_TRASHCAN:
				sprintf (buffer, "#T@%d@%d@%s@%d@%d", obx, oby,
						name, pii->shortcut, (word)pii->truecolor);
				break;

			case DI_SHREDDER:
				sprintf (buffer, "#E@%d@%d@%s@%d", obx, oby,
						name, (word)pii->truecolor);
				break;

			case DI_SCRAPDIR:
				sprintf (buffer, "#S@%d@%d@%s@%d@%d", obx, oby,
						name, pii->shortcut, (word)pii->truecolor);
				break;

			case DI_FOLDER:
			case DI_PROGRAM:
				sprintf (buffer, "#P@%d@%d@%d@%d@%s@%s@%s@%d",
						obx, oby, pii->defnumber,
						(pii->type == DI_FOLDER),
						pii->fullname, name,
						strlen (pii->label)? pii->label : " ",
						pii->shortcut);
				break;

			default:
				write = FALSE;
				break;
		}

		if (write)
		{
			if (mputs (mp, buffer))
				return FALSE;
		}
	}
	
	return TRUE;
}


void InstArgvOnDesktop (word mx, word my, ARGVINFO *A)
{
	OBJECT *stdobj;
	word i, starty;
	word curx, cury;
	char basename[MAX_FILENAME_LEN];
	char first = TRUE;
	int installBigIcons = ShowInfo.normicon;
	
	stdobj = installBigIcons? 
		GetStdBigIcon () : GetStdSmallIcon ();
	
	for (i = 0; i < A->argc; ++i)
	{
		const char *fileName = A->argv[i];
		XATTR X;
		
		GetBaseName (fileName, basename, MAX_FILENAME_LEN);
		if (GetXattr (fileName, FALSE, FALSE, &X))
		{
			venusErr (NlsStr (T_NOSTAT), fileName);
			return;
		}

		if (first)
		{
			curx = mx - Desk.g_x;
			starty = cury = my - Desk.g_y;
		}
		else
		{
			cury += stdobj->ob_height;
			if (cury > Desk.g_h)
			{	
				cury = starty;  
				curx += stdobj->ob_width;
			}
		}				

		if (X.attr & FA_FOLDER)
		{
			if (IsRootDirectory (fileName))
				InstDriveIcon (curx, cury, TRUE, 6, 
					toupper (fileName[0]), fileName, 0, 16);
			else
				InstPrgIcon (curx, cury, TRUE, installBigIcons,
					FALSE, fileName, basename, "", 0);
		}
		else
			InstPrgIcon (curx, cury, TRUE, installBigIcons,
			FALSE, fileName, basename, "", 0);
	}	
}


/*
 * installiere gedraggte Icons auf dem Desktophintergrund
 */
void InstDraggedIcons (WindInfo *wp, word fromx, word fromy,
	word tox, word toy)
{
	OBJECT *stdobj;
	FileWindow_t *fwp;
	FileInfo *pf;
	word i, obx, oby, startx, starty;
	word curx, cury;
	char path[MAX_PATH_LEN];
	char first = TRUE;
	int installBigIcons;
	
	if (wp->kind != WK_FILE)
		return;
	
	fwp = wp->kind_info;
	if (fwp->display != DisplayText)
		installBigIcons = (fwp->display == DisplayBigIcons);
	else
		installBigIcons = ShowInfo.normicon;
	
	stdobj = installBigIcons? 
		GetStdBigIcon () : GetStdSmallIcon ();
	
	for (i = fwp->tree[0].ob_head; i > 0; 
		i = fwp->tree[i].ob_next)
	{
		if (!ObjectWasDragged (&fwp->tree[i]))
			continue;

		pf = GetFileInfo (&fwp->tree[i]);
		strcpy (path, fwp->path);

		if (first)
		{
			first = FALSE;
			objc_offset (fwp->tree, i, &obx, &oby);
			curx = obx = tox + (obx - fromx) - Desk.g_x;
			cury = oby = toy + (oby - fromy) - Desk.g_y;
			starty = fwp->tree[i].ob_y;
			startx = fwp->tree[i].ob_x;
			
			if (pf->attrib & FA_FOLDER)
				AddFolderName (path, pf->name);
			else
				AddFileName (path, pf->name);
				
			InstPrgIcon (obx, oby, TRUE, installBigIcons,
				pf->attrib & FA_FOLDER,	path, pf->name, 
				fwp->label, 0);
		}
		else
		{
			if (starty == fwp->tree[i].ob_y)
			{
				curx += stdobj->ob_width;
			}
			else
			{
				starty = fwp->tree[i].ob_y;
				if (startx != fwp->tree[i].ob_x)
				{						/* andere Spalten */
					word diff,times;
					
					diff = startx - fwp->tree[i].ob_x;
					times = diff / (fwp->stdobj.ob_width 
									+ fwp->xdist - 1);
					curx = obx - (times * stdobj->ob_width);
				}
				else
					curx = obx;

				cury += stdobj->ob_height;
			}
			
			if (pf->attrib & FA_FOLDER)
				AddFolderName (path, pf->name);
			else
				AddFileName (path, pf->name);

			InstPrgIcon (curx, cury, TRUE, installBigIcons,
				pf->attrib & FA_FOLDER, path, pf->name, 
				fwp->label, 0);
		}
	}
}

void RehashDeskIcon (void)
{
	word i;
	OBJECT *po;
	IconInfo *pii;
	char fname[MAX_FILENAME_LEN], color;
	
	for (i = NewDesk.tree[0].ob_head; i > 0;
		 i = NewDesk.tree[i].ob_next)
	{
		if (isHidden (NewDesk.tree, i)
			|| ((NewDesk.tree[i].ob_type & 0x00ff) != G_USERDEF)
			|| ((pii = GetIconInfo (&NewDesk.tree[i])) == NULL)
			|| ((pii->type != DI_FOLDER) && (pii->type != DI_PROGRAM)))
		{
			continue;
		}
			
		GetBaseName (pii->fullname, fname, MAX_FILENAME_LEN);
		if (pii->defnumber)
		{
			po = GetBigIconObject (pii->type == DI_FOLDER, fname,
					&color);
		}
		else
		{
			po = GetSmallIconObject (pii->type == DI_FOLDER, fname,
					&color);
		}
		
		if (!IconsAreEqual (po, &pii->icon.icon, color))
		{
			redrawObj (&wnull, i);

			ChangeIconObject (&pii->icon, po, color);

			/* Fahnenbreite anpassen
			 */
			SetIconTextWidth (&pii->icon.icon, &NewDesk.tree[i]);

			/* Im sichtbaren Bereich
			 */
			deskAlign (&NewDesk.tree[i]);

			redrawObj (&wnull, i);
		}
	}

	flushredraw ();
}

void DeskIconNewLabel (char drive, const char *oldlabel, 
						const char *newlabel)
{
	word i;
	IconInfo *pii;
	
	for (i = NewDesk.tree[0].ob_head; i > 0; 
		i = NewDesk.tree[i].ob_next)
	{
		if (isHidden (NewDesk.tree, i)
			|| ((NewDesk.tree[i].ob_type & 0x00ff) != G_USERDEF)
			|| ((pii = GetIconInfo (&NewDesk.tree[i])) == NULL))
		{
			continue;
		}
		
		if ((pii->fullname != NULL) && (drive == pii->fullname[0])	
			&& (!strcmp (pii->label, oldlabel)))
		{
			strcpy (pii->label, newlabel);
		}
	}
}