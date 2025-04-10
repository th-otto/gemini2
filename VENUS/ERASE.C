/*
 * @(#) Gemini\erase.c
 * @(#) Stefan Eissing, 13. November 1994
 *
 * description: functions to kill things
 *
 * jr 23.4.95
 */

#include <stdio.h>
#include <string.h>
#include <tos.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "..\common\memfile.h"
#include "..\common\walkdir.h"
#include "..\common\strerror.h"

#include "vs.h"
#include "erasebox.rh"

#include "window.h"
#include "cpintern.h"
#include "cpkobold.h"
#include "drives.h"
#include "erase.h"
#include "filewind.h"
#include "fileutil.h"
#include "iconinst.h"
#include "myalloc.h"
#include "redraw.h"
#include "select.h"
#include "undo.h"
#include "util.h"
#include "venuserr.h"


/* externals
 */
extern OBJECT *perasebox;


/* internal texts
 */
#define NlsLocalSection "G.erase"
enum NlsLocalText{
T_CONTAINSFILES,/*enth„lt noch Dateien*/
T_ILLFOLDER,	/*Sie wollen gerade den Ordner l”schen, der von 
dem %s ben”tigt wird. Dieser Ordner wird daher 
beim L”schen bersprungen.*/
T_NODELFOLDER,	/*Der Ordner `%s' l„žt sich nicht l”schen (%s)!*/
T_USERABORT,	/*Wollen Sie den gesamten L”schvorgang abbrechen?*/
T_ERRABORT,		/*Die Datei `%s' konnte nicht gel”scht werden. 
Wollen sie die gesamte Operation abbrechen?*/
T_NOWORK,		/*Es sind keine l”schbaren Objekte zu finden!*/
T_CANTDELETE,	/*Datei `%s' l„žt sich nicht l”schen (%s)!*/
T_DELFILE,		/*L”sche Datei:*/
T_DELFOLDER,	/*Entferne Ordner:*/
T_NOINFO, 		/*Die Informationen ber die zu l”schenden Objekte 
konnten nicht ermittelt werden. Zuwenig Speicher?*/
T_NAMETOOLONG,	/*Die Namen sind zu lang, bzw. zu tief verschachtelt
 um von Gemini gel”scht werden zu k”nnen.*/
};

/* internals
 */
static DIALINFO d;
static uword foldcount,filecount;
static long bytestodo;
static word verboseDelete = TRUE;

/*
 * check if the erased file is on the desktop and
 * remove the desktop icon if that's the case
 */
void ObjectWasErased (char *name, int redraw)
{
	OBJECT *tree;
	IconInfo *pii;
	word i, prevobj;
	GRECT rect;
	char path[MAX_PATH_LEN];
	
	strcpy (path, name);
	UndoFolderVanished (path);
	
	tree = NewDesk.tree;
	
	for(i = tree[0].ob_head, prevobj = 0;
		i > 0; prevobj = i, i = tree[i].ob_next)
	{
		if (tree[i].ob_type  != G_ICON
			&& tree[i].ob_type  != G_USERDEF)
		{
			continue;
		}
		
		if ((pii = GetIconInfo (&tree[i])) == NULL)
			continue;
			
		strcpy (path, name);

		switch (pii->type)
		{
			case DI_SHREDDER:
			case DI_FILESYSTEM:
			case DI_SCRAPDIR:
			case DI_TRASHCAN:
				break;

			case DI_PROGRAM:
			case DI_FOLDER:
				if (stricmp (path, pii->fullname))
				{
					strcat (path, "\\");
					if (stricmp (path, pii->fullname))
						continue;
				}
					
				UnMarkDraggedObject (&wnull, &tree[i]);
				if (isSelected (tree, i))
				{
					--NewDesk.total_selected;
				}
				objc_offset (tree, i, &rect.g_x, &rect.g_y);
				rect.g_w = tree[i].ob_width;
				rect.g_h = tree[i].ob_height;

				RemoveDeskIcon (i);

				buffredraw (&wnull, &rect);
				i = prevobj;
				break;

			default:
				break;
		}
	}
	
	if (redraw)
	{
		WindInfo *wp;
		
		wp = wnull.nextwind;
		while (wp)
		{
			if ((wp->kind == WK_FILE) 
				&& (strlen (path) > 3))
			{
				FileWindow_t *fwp = (FileWindow_t *)wp->kind_info;
				
				if (IsPathPrefix (fwp->path, path))
				{
					strcpy (fwp->path, path);
					strcpy (fwp->title, fwp->path);
					WindowClose (wp->handle, 0);
				}
			}
			
			wp = wp->nextwind;
		}
	}
}

/*
 * update the numbers in the dialog box if todraw is true
 */
static void updateCount (word todraw)
{
	if (NewDesk.silentRemove) return;
	
	sprintf (perasebox[ERFOLDNR].ob_spec.tedinfo->te_ptext,
			"%5u", foldcount);
	sprintf (perasebox[ERFILENR].ob_spec.tedinfo->te_ptext,
			"%5u", filecount);
	SetFileSizeField (perasebox[ERBYTES].ob_spec.tedinfo, bytestodo);
	if (todraw)
	{
		fulldraw (perasebox, ERFOLDNR);
		fulldraw (perasebox, ERFILENR);
		fulldraw (perasebox, ERBYTES);
	}
}

/*
 * Display the erase dialog and return wether the user wants
 * to erase the stuff or not.
 */
static word askForErase (void)
{
	word retcode;
	
	if (NewDesk.silentRemove) return TRUE;

	GrafMouse (ARROW, NULL);
	strcpy (perasebox[ERFILESC].ob_spec.free_string, "");
	strcpy (perasebox[ERFILEDS].ob_spec.free_string, "");
	setDisabled (perasebox, ERABORT, TRUE);
	updateCount (FALSE);
	
	DialCenter (perasebox);
	DialStart (perasebox, &d);
	DialDraw (&d);
	
	retcode = DialDo (&d, 0) & 0x7FFF;
	
	setSelected (perasebox, retcode, FALSE);
	if (retcode == EROK)
	{
		setDisabled (perasebox, ERABORT, FALSE);
		fulldraw (perasebox, ERABORT);
		GrafMouse (HOURGLASS, NULL);
		return TRUE;
	}
	return FALSE;
}

/*
 * Clean up things if dialog was used.
 */
static void EndDialog (void)
{
	if (!NewDesk.silentRemove) DialEnd (&d);
}

/*
 * Display the file name in the dialog box
 */
static void displayEraseAction (const char *action, const char *name)
{
	if (NewDesk.silentRemove) return;

	DispName (perasebox, ERFILESC, action);
	DispName (perasebox, ERFILEDS, name);
}

static word isValidFolder (char *fpath)
{
	IconInfo *pii;
	char *iname;
	word retcode = TRUE;
	
	pii = GetIconInfo (&NewDesk.tree[NewDesk.scrapNr]);
	if (pii)
	{
		/*
		 * check if someone wants to erase the scrapdir
		 */
		if (!stricmp (fpath, pii->fullname))
		{
			iname = pii->iconname;
			retcode = FALSE;
		}
	}

	pii = GetIconInfo (&NewDesk.tree[NewDesk.trashNr]);
	if (pii)
	{
		/*
		 * check if someone wants to erase the trashdir
		 */
		if (!stricmp (fpath, pii->fullname))
		{
			iname = pii->iconname;
			retcode = FALSE;
		}
	}
	
	if (!retcode)
	{
		venusErr (NlsStr (T_ILLFOLDER), iname);
		GrafMouse (HOURGLASS, NULL);
		verboseDelete = FALSE;
		return FALSE;
	}
	
	return TRUE;
}

static void cleanUpFolder (char *fpath)
{
	word wasErased = FALSE;
	size_t len;
	long ret;
		
	if ((len = strlen (fpath)) == 0) return;
	if (fpath[len - 1] == '\\') fpath[len - 1] = '\0';

	displayEraseAction (NlsStr (T_DELFOLDER), fpath);
	ret = Ddelete (fpath);
	if (ret != E_OK)
	{
		if (verboseDelete)
		{
			venusErr (NlsStr (T_NODELFOLDER), fpath,
				IsDirEmpty (fpath) ? StrError (ret) :
				NlsStr (T_CONTAINSFILES));
			GrafMouse (HOURGLASS, NULL);
			verboseDelete = FALSE;
		}
	}
	else
		wasErased = TRUE;

	if (wasErased, FALSE) ObjectWasErased (fpath, FALSE);
}

static long eraseFile (char *fpath, long size)
{
	long retcode;
	
	if (escapeKeyPressed ())
	{
		if (venusChoice (NlsStr (T_USERABORT)))
			return ERROR;
		else
			GrafMouse (HOURGLASS, NULL);
	}
	
	displayEraseAction (NlsStr (T_DELFILE), fpath);
	retcode = Fdelete (fpath);
	
	if (retcode != E_OK)
	{
		venusErr (NlsStr (T_CANTDELETE), fpath, StrError (retcode));

		if ((filecount > 1 || foldcount)
			&& venusChoice (NlsStr (T_ERRABORT), fpath))
		{
			return retcode;
		}
		else
			GrafMouse (HOURGLASS, NULL);
	}
	else
		ObjectWasErased (fpath, FALSE);

	--filecount;
	bytestodo -= size;
	updateCount (TRUE);
	
	return E_OK;
}

static long eraseInsideFolder (char *fpath)
{
	WalkInfo W;
	char filename[MAX_PATH_LEN];
	long stat;
	static long eraseFolder (char *fpath);
	
	if (strlen (fpath) >= MAX_PATH_LEN)
	{
		venusErr (NlsStr (T_NAMETOOLONG));
		return ENAMETOOLONG;
	}
	
	if (escapeKeyPressed ())
	{
		if (venusChoice (NlsStr (T_USERABORT)))
			return ERROR;
		else
			GrafMouse (HOURGLASS, NULL);
	}
	
	stat = InitDirWalk (&W, WDF_ATTRIBUTES|WDF_GEMINI_LOWER, fpath,
		MAX_PATH_LEN - strlen (fpath), filename);

	if (stat != E_OK) return stat;

	while ((stat = DoDirWalk (&W)) == E_OK)
	{
		if (!strcmp (filename, ".") || !strcmp (filename, ".."))
			continue;
				
		AddFileName (fpath, filename);
		
		if (W.xattr.attr & FA_FOLDER)
		{
			stat = eraseFolder (fpath);
		}
		else if (!(W.xattr.attr & FA_LABEL))
		{
			stat = eraseFile (fpath, W.xattr.size);
		}
		
		StripFileName (fpath);
		
		if (stat != E_OK) {
			ExitDirWalk (&W);
			return stat;
		}
	}
	
	if (stat == ENAMETOOLONG)
		venusErr (NlsStr (T_NAMETOOLONG));
	
	ExitDirWalk (&W);
	
	return E_OK;
}

static long eraseFolder (char *fpath)
{
	if (isValidFolder (fpath))
	{
		long stat = eraseInsideFolder (fpath);
		if (stat == E_OK) cleanUpFolder (fpath);
		return stat;
	}
	else
	{
		uword fileanz = 0,foldanz = 0;
		long size = 0L;
		
		if (AddFileAnz (fpath, FALSE, -1L, &foldanz, &fileanz, &size))
		{
			bytestodo -= size;
			foldcount -= foldanz;
			filecount -= fileanz;
		}
	}
	--foldcount;
	updateCount (TRUE);
	return E_OK;
}


long EraseFileObject (const char *filename)
{
	XATTR X;
	char buf[MAX_PATH_LEN];
	int drive, broken = FALSE;
	long ret = E_OK;

	strcpy (buf, filename);
	drive = strlen (buf) == 3 && buf[1] == ':' && buf[2] == '\\';
	if (!drive) broken = (int)GetXattr (buf, FALSE, FALSE, &X);
	
	if (broken) return ERROR;
	
	if (drive)
		ret = eraseInsideFolder (buf);
	else if (X.attr & FA_FOLDER)
	{
		verboseDelete = TRUE;
		ret = eraseFolder (buf);
	}
	else
		ret = eraseFile (buf, X.size);

	StripFileName (buf);
	BuffPathUpdate (buf);
	
	return ret;
}


word EraseArgv (ARGVINFO *A)
{
	long max_objects;
	long i;
	
	ArgvCheckNames (A);

	max_objects = -1L;
	if (NewDesk.useDeleteKobold && (NewDesk.minDeleteKobold > 0)
		&& KoboldPresent ())
	{
		max_objects = NewDesk.minDeleteKobold;
	}

	if (!CountArgvFolderAndFiles (A, FALSE, max_objects, 
		&foldcount, &filecount, &bytestodo))
	{
		return FALSE;
	}

	if (foldcount==0 && filecount==0)
	{
		venusInfo (NlsStr (T_NOWORK));
		return FALSE;
	}
	
	if ((max_objects > 0) && ((foldcount + filecount) >= max_objects))
	{
		return CopyWithKobold (A, NULL, kb_delete,
			!NewDesk.silentRemove, FALSE, FALSE);
	}


	if (!askForErase ())
	{
		EndDialog ();
		return FALSE;
	}
	
	for (i = 0; (i < A->argc) && E_OK == EraseFileObject (A->argv[i]);
		++i) ;
	
	EndDialog ();
	flushredraw ();
	FlushPathUpdate ();
	
	return TRUE;
}


/*
 * void emptyPaperbasket(void)
 * empty the TRASHDIR
 */
void emptyPaperbasket(void)
{
	IconInfo *pii;
	word savestate;
	char path[MAX_PATH_LEN];
	
	pii = GetIconInfo (&NewDesk.tree[NewDesk.trashNr]);
	if(pii)
	{
		strcpy (path, pii->fullname);
		GrafMouse (HOURGLASS, NULL);
		savestate = NewDesk.silentRemove;
		NewDesk.silentRemove = TRUE;

		eraseInsideFolder (path);
		
		NewDesk.silentRemove = savestate;
		PathUpdate (pii->fullname);
		GrafMouse (ARROW, NULL);
	}
}

word DeleteDraggedObjects (WindInfo *wp)
{
	ARGVINFO A;
	word retcode;
	
	GrafMouse (HOURGLASS, NULL);

	if (!wp->getDragged || !wp->getDragged (wp, &A, TRUE))
	{
		venusInfo (NlsStr (T_NOINFO));
		GrafMouse (ARROW, NULL);
		return FALSE;
	}
		
	retcode = EraseArgv (&A);

	FreeArgv (&A);
	GrafMouse (ARROW, NULL);
	DeselectAllExceptWindow (wp);
	DeselectNotDraggedObjects (wp);
	
	return retcode;
}

