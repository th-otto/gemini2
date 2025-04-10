/*
 * @(#) Gemini\Menu.c
 * @(#) Stefan Eissing, 02. Juni 1993
 *
 * description: menufunktionen zu Venus
 *
 * jr 19.5.95
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <flydial\flydial.h>

#include "vs.h"
#include "menu.rh"

#include "window.h"
#include "applmana.h"
#include "clipbrd.h"
#include "conwind.h"
#include "deskwind.h"
#include "dialog.h"
#include "dispatch.h"
#include "filewind.h"
#include "getinfo.h"
#include "iconinst.h"
#include "iconrule.h"
#include "init.h"
#include "menu.h"
#include "message.h"
#include "order.h"
#include "redraw.h"
#include "select.h"
#include "stand.h"
#include "undo.h"
#include "util.h"
#include "venuserr.h"
#include "wildcard.h"


/* externals
 */
extern OBJECT *pmenu;


static void switchGeneralOptions (word entry)
{
	switch (entry)
	{
		case DISK:				/* install Diskicon */
			DoInstDialog ();
			break;

		case APPL:
			ApplDialog();
			break;

		case RULEOPT:
			if (EditIconRule ())
			{
				allrewindow (FW_NEW_RULES);
				RehashDeskIcon ();
			}
			break;

		case DESKSET:
			SetDeskBackground ();
			break;

		case TOMUPFEL:
		{
			int was_opened;
			word old_top;

			MakeConsoleTopWindow (&was_opened, &old_top);
			break;
		}

		case SAVEWORK:
			SaveStatus ();
			break;

		case SHUTDOWN:
			Shutdown (AP_TERM);
			break;

		case PDIVERSE:
			if (doDivOptions ())
				allFileChanged ();
			break;

		case PGESPRAE:
			EditVerbosity ();
			break;

		case PPROGRAM:
			EditFinishOptions ();
			break;
	}
}


static void switchEditMenu (word entry)
{
	WindInfo *wp = NULL;

	ThereAreSelected (&wp);

	switch (entry)
	{
		case MNCUT:
			if (wp && wp->cut)
				wp->cut (wp, FALSE);
			break;

		case MNCOPY:
			if (wp && wp->copy)
				wp->copy (wp, FALSE);
			break;

		case MNDEL:
			if (wp && wp->delete)
				wp->delete (wp, FALSE);
			break;

		case MNPASTE:
			wp = GetTopWindInfo ();
			if (wp && wp->paste)
				wp->paste (wp, FALSE);
			break;

		case DOUNDO:
			doUndo ();
			break;

		case SELECALL:
			SelectAllInTopWindow ();
			break;
	}
}


/* kmmere dich um die Menueintr„ge
 */
int DoMenu (word mtitel, word mentry, word kstate)
{
	WindInfo *topwp;
	word fwind;

	switch (mtitel)
	{
		case DESK:
			if (mentry == DESKINFO)
			{
				InfoAboutGemini ();
			}
			break;

		case FILES:
			switch (mentry)
			{
				case OPEN:
					simDclick (kstate);
					break;

				case ALIAS:
					topwp = GetTopWindInfo ();
					if (topwp && topwp->make_alias)
						topwp->make_alias (topwp);
					break;

				case GETINFO:
					getInfoDialog ();
					break;

				case FOLDER:
					topwp = GetTopWindInfo ();
					if (topwp && topwp->make_new_object)
						topwp->make_new_object (topwp);
					break;

				case WINDCLOS:
					wind_get (0, WF_TOP, &fwind);
					if ((fwind != 0) && GetWindInfo (fwind) != NULL)
						WindowDestroy (fwind, TRUE, FALSE);
					break;

				case CLOSE:
					wind_get (0, WF_TOP, &fwind);
					if ((fwind!=0) && GetWindInfo (fwind) != NULL)
						WindowClose (fwind, kstate);
					break;

				case CYCLEWIN:
					cycleWindow ();
					break;

				case INITDISK:
					InitDisk ();
					break;
			}
			break;

		case EDIT:
			switchEditMenu (mentry);
			break;

		case SHOW:
			switch (mentry)
			{
				case BYNOICON:
					if (ShowInfo.showtext || (!ShowInfo.normicon))
					{
						menu_icheck (pmenu, ShowInfo.showtext? 
							BYTEXT : BYSMICON, FALSE);
						menu_icheck (pmenu, mentry, TRUE);
						ShowInfo.showtext = FALSE;
						ShowInfo.normicon = TRUE;
						allrewindow (FW_NEW_DISPLAY_TYPE);
					}
					break;

				case BYSMICON:
					if (ShowInfo.showtext || ShowInfo.normicon)
					{
						menu_icheck (pmenu, ShowInfo.showtext? 
							BYTEXT : BYNOICON, FALSE);
						menu_icheck (pmenu, mentry, TRUE);
						ShowInfo.showtext = FALSE;
						ShowInfo.normicon = FALSE;
						allrewindow (FW_NEW_DISPLAY_TYPE);
					}
					break;

				case BYTEXT:
					if (!ShowInfo.showtext)
					{
						menu_icheck (pmenu, ShowInfo.normicon? 
							BYNOICON : BYSMICON, FALSE);
						menu_icheck (pmenu, mentry, TRUE);
						ShowInfo.showtext = TRUE;
						allrewindow (FW_NEW_DISPLAY_TYPE);
					}
					break;

				case SORTNAME:
				case SORTDATE:
				case SORTTYPE:
				case SORTSIZE:
				case UNSORT:
					if (mentry != ShowInfo.sortentry)
					{
						menu_icheck (pmenu, ShowInfo.sortentry, FALSE);
						menu_icheck (pmenu, mentry, TRUE);
						ShowInfo.sortentry = mentry;

						allrewindow ((mentry == SORTTYPE)? 
							(WU_SHOWTYPE|FW_NEW_SORT_TYPE) : 
							FW_NEW_SORT_TYPE);
					}
					break;

				case PDARSTEL:
					if ((wnull.nextwind != NULL) &&
						wnull.nextwind->darstellung != NULL)
					{
						wnull.nextwind->darstellung 
							(wnull.nextwind, FALSE);
					}
					break;

				case ORDEDESK:
					orderDeskIcons ();
					break;

				case WILDCARD:
		    			if (wnull.nextwind != NULL)
		    				doWildcard (wnull.nextwind);
	    			break;

			}
			break;

		case OPTIONS:
			switchGeneralOptions (mentry);
			break;
	}

	menu_tnormal (pmenu, mtitel, 1);

	return !(mentry == QUIT);
}


static struct
{
	unsigned new : 1;
	unsigned open : 1;
	unsigned getinfo : 1;
	unsigned alias : 1;
	unsigned format : 1;
	unsigned cut : 1;
	unsigned copy : 1;
	unsigned paste : 1;
	unsigned delete : 1;
	unsigned select_all : 1;
	unsigned pattern : 1;
	unsigned darstellung : 1;
} menu_disabled;


/*
 * cares about selectable and disabled menu entries
 */
void ManageMenu (void)
{
	WindInfo *selectwp, *topwp;
	int thereare, onlyone, objnr;
	int isFloppy;
	int cut_on, copy_on, paste_on, delete_on;
	int select_all_on, pattern_on, make_new, make_alias;
	int make_darstellung;

	selectwp = topwp = NULL;
	onlyone = GetOnlySelectedObject (&selectwp, &objnr);
	thereare = ThereAreSelected (&selectwp);
	topwp = GetTopWindInfo ();

	if (menu_disabled.open ^ !onlyone)
	{
		menu_ienable (pmenu, OPEN, onlyone);
		menu_disabled.open = !onlyone;
	}

	if (menu_disabled.getinfo ^ !thereare)
	{
		menu_ienable (pmenu, GETINFO, thereare);
		menu_disabled.getinfo = !thereare;
	}

	cut_on = ((selectwp && selectwp->cut 
		&& selectwp->cut (selectwp, TRUE))
		|| (topwp && topwp->cut && topwp->cut (topwp, TRUE)));
	copy_on = ((selectwp && selectwp->copy 
		&& selectwp->copy (selectwp, TRUE))
		|| (topwp && topwp->copy && topwp->copy (topwp, TRUE)));
	paste_on = (!NewDesk.flags.scrap_empty && topwp
		&& topwp->paste && topwp->paste (topwp, TRUE));
	delete_on = ((selectwp && selectwp->delete 
		&& selectwp->delete (selectwp, TRUE))
		|| (topwp && topwp->delete && topwp->delete (topwp, TRUE)));
	select_all_on = (topwp && topwp->select_all);

	if (!cut_on ^ menu_disabled.cut )
	{
		menu_ienable (pmenu, MNCUT, cut_on);
		menu_disabled.cut = !cut_on;
	}
	if (!copy_on ^ menu_disabled.copy)
	{
		menu_ienable (pmenu, MNCOPY, copy_on);
		menu_disabled.copy = !copy_on;
	}
	if (!delete_on ^ menu_disabled.delete)
	{
		menu_ienable (pmenu, MNDEL, delete_on);
		menu_disabled.delete = !delete_on;
	}
	if (!paste_on ^ menu_disabled.paste)
	{
		menu_ienable (pmenu, MNPASTE, paste_on);
		menu_disabled.paste = !paste_on;
	}
	if (!select_all_on ^ menu_disabled.select_all)
	{
		menu_ienable (pmenu, SELECALL, select_all_on);
		menu_disabled.select_all = !select_all_on;
	}

	if (onlyone && selectwp->kind == WK_DESK)
	{
		DeskInfo *dip = selectwp->kind_info;
		IconInfo *pii;

		pii = GetIconInfo (&dip->tree[objnr]);
		isFloppy = (pii && pii->type == DI_FILESYSTEM)
					&& (pii->fullname[0] < 'C');
	}
	else
		isFloppy = FALSE;

	if (menu_disabled.format ^ !isFloppy)
	{
		menu_ienable (pmenu, INITDISK, isFloppy);
		menu_disabled.format = !isFloppy;
	}

	pattern_on = (topwp->get_display_pattern != NULL);
	if (menu_disabled.pattern ^ !pattern_on)
	{
		menu_ienable (pmenu, WILDCARD, pattern_on);
		menu_disabled.pattern = !pattern_on;
	}

	make_new = (topwp->make_new_object != NULL);
	if (menu_disabled.new ^ !make_new)
	{
		menu_ienable (pmenu, FOLDER, make_new);
		menu_disabled.new = !make_new;
	}

	make_alias = (topwp->make_alias != NULL)
			&& onlyone && (selectwp != topwp);
	if (menu_disabled.alias ^ !make_alias)
	{
		menu_ienable (pmenu, ALIAS, make_alias);
		menu_disabled.alias = !make_alias;
	}

	make_darstellung = (topwp->darstellung != NULL)
			&& (topwp->darstellung (topwp, TRUE));
	if (menu_disabled.darstellung ^ !make_darstellung)
	{
		menu_ienable (pmenu, PDARSTEL, make_darstellung);
		menu_disabled.darstellung = !make_darstellung;
	}

	/* xxx: Steve to be done right */
	MenuItemEnable (pmenu, SHUTDOWN, HasShutdown ());
}


void SetShowInfo (void)
{
	menu_icheck (pmenu, BYTEXT, FALSE);
	menu_icheck (pmenu, BYNOICON, FALSE);
	menu_icheck (pmenu, BYSMICON, FALSE);
	menu_icheck (pmenu, SORTNAME, FALSE);
	menu_icheck (pmenu, SORTDATE, FALSE);
	menu_icheck (pmenu, SORTSIZE, FALSE);
	menu_icheck (pmenu, SORTTYPE, FALSE);
	menu_icheck (pmenu, UNSORT, FALSE);

	if (ShowInfo.showtext)
		menu_icheck (pmenu, BYTEXT, TRUE);
	else
	{
		menu_icheck (pmenu, (ShowInfo.normicon)? 
			BYNOICON : BYSMICON, TRUE);
	}

	if (ShowInfo.sortentry < SORTNAME
		|| ShowInfo.sortentry > UNSORT)
	{
		ShowInfo.sortentry = SORTNAME;
	}

	menu_icheck (pmenu, ShowInfo.sortentry, TRUE);

	allrewindow (FW_NEW_SORT_TYPE|FW_NEW_DISPLAY_TYPE);
}


void SetMenuBar (word install)
{
	ReInstallBackground (FALSE);
	menu_bar (pmenu, install);
}
