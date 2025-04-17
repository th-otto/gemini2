/*
 * @(#) Gemini\Draglogi.c
 * @(#) Stefan Eissing, 12. November 1994
 *
 * description: logic to handle dragged icons
 *
 */

#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "..\common\fileutil.h"

#include "vs.h"

#include "window.h"
#include "applmana.h"
#include "copymove.h"
#include "dispatch.h"
#include "draglogi.h"
#include "erase.h"
#include "fileutil.h"
#include "filewind.h"
#include "icondrag.h"
#include "iconinst.h"
#include "myalloc.h"
#include "menu.h"
#include "message.h"
#include "order.h"
#include "redraw.h"
#include "select.h"
#include "stand.h"
#include "util.h"
#include "venuserr.h"

/* internal texts
 */
#define NlsLocalSection "G.draglogi"
enum NlsLocalText{
T_MYWIND,	/*Auf dieses Fenster k”nnen Sie keine Objekte 
ziehen! Deshalb bleiben die Objekte dort, wo sie waren.*/
};

/*
 * move with dragged icons in Window wp about dx,dy
 */
static void moveDeskIcons (WindInfo *wp, DeskInfo *dip, word dx, 
	word dy)
{
	word i;
	
	if (dx || dy)
	{
		for (i = dip->tree[0].ob_head; i > 0; i = dip->tree[i].ob_next)
		{
			if (!ObjectWasDragged (&dip->tree[i]))
				continue;

			/* gedraggtes Object
			 */
			redrawObj (wp, i);
			dip->tree[i].ob_x += dx;
			dip->tree[i].ob_y += dy;

			if (NewDesk.snapAlways)
			{
				SnapObject (wp, i, TRUE);
			}

			redrawObj (wp, i);
		}
		
		flushredraw ();					/* to be shure */
	}
}


static void pasteDragNames (ARGVINFO *A)
{
	char *names = Argv2String (pMainAllocInfo, A, 1);
	if (names)
		PasteStringToConsole (names);
	free (names);

	/* H„nge ein Space an, damit der Cursor nicht direkt hinter
	 * dem letzten Dateinamen steht.
	 */	
	PasteStringToConsole (" ");
}

typedef enum Action {
	Ignore,
	StartedProgram,
	Copy,
	Move,
	Erase,
	PasteConsole,
	PasteAccessory,
	DeskAction
};

int DropAction (char *to_path, WindInfo *to_wp, 
	word to_object, word kstate, ARGVINFO *A)
{
	OBJECT *to_tree = NULL;
	
	if (!to_wp)
		return PasteAccessory;
		
	if (to_wp->window_get_tree)
		to_tree = to_wp->window_get_tree (to_wp);

	if (to_wp->kind == WK_FILE)
	{
		strcpy (to_path, 
			((FileWindow_t *)to_wp->kind_info)->path);
		
		if (to_object > 0)
		{
			FileInfo *pf;
			
			pf = GetFileInfo (&to_tree[to_object]);
			
			if (pf->icon.flags.is_folder)
			{
				if (strcmp (pf->name, ".."))
					AddFolderName (to_path, pf->name);
				else
					StripFolderName (to_path);	/* folder .. */
				
				return (kstate & K_CTRL)? Move : Copy;
			}
			else
			{
				/* Wenn es kein Ordner ist, fhren wir es
				 * aus.
				 */
				char to_label[MAX_FILENAME_LEN];
				int retcode;

				strcpy (to_label, 
					((FileWindow_t *)to_wp->kind_info)->label);

				StartFile (to_wp, to_object, TRUE,  to_label,
					to_path, pf->name, A, 0, &retcode);

				return StartedProgram;
			}
		}
		else
			return (kstate & K_CTRL)? Move : Copy;
	}
	else if (to_wp->kind == WK_DESK)
	{
		IconInfo *pii = NULL;
		char *cp;
		
		if (to_object > 0)
			pii = GetIconInfo (&to_tree[to_object]);
		if (!pii)
			return DeskAction;
			
		switch (pii->type)
		{
			case DI_FOLDER:
			{
				int cancelled;
				
				if (!CheckInteractiveForAccess (
						&pii->fullname, pii->label, TRUE, 
						to_object, &cancelled) || cancelled)
				{
					return Ignore;
				}
				/* no break */
			}
						
			case DI_FILESYSTEM:
			case DI_SCRAPDIR:
				strcpy (to_path, pii->fullname);
				return (kstate & K_CTRL)? Move : Copy;

			case DI_TRASHCAN:
				strcpy (to_path, pii->fullname);
				return Move;

			case DI_PROGRAM:
			{
				char path[MAX_PATH_LEN];
				int retcode;
				
				strcpy (path, pii->fullname);
				cp = strrchr (path, '\\');
				strcpy (to_path, cp + 1);
				StripFileName (path);

				StartFile (to_wp, to_object, TRUE, pii->label,
					path, to_path, A, 0, &retcode);

				return StartedProgram;
			}

			case DI_SHREDDER:
				return Erase;

			default:
				venusDebug ("unbekannter Icontyp!");
				return Ignore;
		}
	}
	else if (to_wp->kind == WK_CONSOLE)
		return PasteConsole;
	else if (to_wp->kind == WK_ACC)
		return PasteAccessory;
	
	return Ignore;
}


/*
 * manage drag of icons from window fwp to window twp and decide,
 * if files should be copied, moved, renamed and so on.
 */
int doDragLogic (WindInfo *from_wp, word to_window, word to_object, 
	word kstate, word fromx, word fromy, word tox, word toy)
{	
	WindInfo *to_wp = GetWindInfo (to_window);
	char to_path[MAX_PATH_LEN];
	int action = 0;
	ARGVINFO A;
	
	InitArgv (&A);
	if (to_wp && from_wp)
	{
		OBJECT *from_tree = NULL;
		word from_object, fobj;
		OBJECT *to_tree = NULL;

		if (to_wp->window_get_tree)
			to_tree = to_wp->window_get_tree (to_wp);
		if (from_wp->window_get_tree)
			from_tree = from_wp->window_get_tree (from_wp);

		/* Wenn das Zielfenster nicht der Desktop ist, dann
		 * teste, ob sich die Start- und Zielkoordinaten auf 
		 * demselben Objekt befinden. Wenn dies der Fall ist,
		 * dann machen wir gar nichts
		 */
		if ((to_wp->kind != WK_DESK)
			&& from_tree && to_tree)
		{
			from_object = FindObject (from_tree, fromx, fromy);
			fobj = FindObject (to_tree, tox, toy);
	
			/* Don't do nothing when the drag was on the same object
			 */
			if ((from_wp->handle == to_window)
				&& (from_object > 0) && (from_object == fobj))
					return 0;
		}
	}

	if (from_wp->getDragged)
		from_wp->getDragged (from_wp, &A, FALSE);

	switch (DropAction (to_path, to_wp, to_object, kstate, &A))
	{
		case Copy:
			action = CopyDraggedObjects (from_wp, to_path, FALSE, 
				kstate & K_ALT);
			break;
			
		case Move:
			action = CopyDraggedObjects (from_wp, to_path, TRUE, 
				kstate & K_ALT);
			break;

		case Erase:
			if (from_wp->delete)
				action = from_wp->delete (from_wp, FALSE);
			break;

		case PasteAccessory:
			action = PasteAccWindow (to_window, tox, toy, 
				kstate, &A);
			if (!action)
				venusErr (NlsStr (T_MYWIND));
			break;

		case PasteConsole:
			pasteDragNames (&A);
			action = 1;
			break;

		case DeskAction:
			switch (from_wp->kind)
			{
				case WK_DESK:
					moveDeskIcons (from_wp, from_wp->kind_info, 
						tox - fromx, toy - fromy);
					break;
	
				case WK_FILE:
					InstDraggedIcons (from_wp, fromx, fromy, 
						tox, toy);
					break;
			}
			action = 1;
			break;
	}

	FreeArgv (&A);
	return action;
}


int DropFiles (int mx, int my, int kstate, ARGVINFO *A)
{
	WindInfo *wp;
	word window, object = -1;
	OBJECT *tree = NULL;
	char path[MAX_PATH_LEN];
	int action = 0;			
	
	window = wind_find (mx, my);
	wp = GetWindInfo (window);
	
	if (wp && wp->window_get_tree)
	{
		tree = wp->window_get_tree (wp);
		if (tree)
			object = FindObject (tree, mx, my);

		if (object > 0)
		{
			DeselectAll (wp);
			SelectObject (wp, object, 1);
			flushredraw ();
		}
	}

	switch (DropAction (path, wp, object, kstate, A))
	{
		case Copy:
			action = CopyArgv (A, path, FALSE, 
				kstate & K_ALT);
			break;
				
		case Move:
			action = CopyArgv (A, path, TRUE, 
				kstate & K_ALT);
			break;
	
		case Erase:
			action = EraseArgv (A);
			break;

		case PasteAccessory:
			action = PasteAccWindow (window, mx, my, 
				kstate, A);
			break;

		case PasteConsole:
			pasteDragNames (A);
			action = 1;
			break;
		
		case DeskAction:
			InstArgvOnDesktop (mx, my, A);
			action = 1;
			break;
	}
	
	return action;
}
