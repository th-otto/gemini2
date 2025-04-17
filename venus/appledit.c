/*
 * @(#) Gemini\Appledit.c
 * @(#) Stefan Eissing, 18. Oktober 1993
 *
 * description: functions to manage list of applications
 *
 * jr 25.7.94
 */

#include <stdlib.h>
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\listman.h>
#include <nls\nls.h>

#include "vs.h"
#include "applbox.rh"
#include "appledit.rh"

#include "window.h"
#include "appledit.h"
#include "applmana.h"
#include "filewind.h"
#include "filedraw.h"
#include "fileutil.h"
#include "myalloc.h"
#include "redraw.h"
#include "util.h"
#include "venuserr.h"
#include "wildcard.h"


/* externals
 */
extern OBJECT *papplbox, *pappledit;


/* internal texts
 */
#define NlsLocalSection "G.appledit.c"
enum NlsLocalText{
T_MEMERR,	/*Die Operation muž leider aus Speichermagel 
abgebrochen werden!*/
T_NOAPPL,	/*Es sind keine Applikationen angemeldet! 
Selektieren Sie ein Programm, um dies zu tun.*/
T_CLIP,		/*Dieses Klemmbrett enth„lt die zuletzt von 
Ihnen gel”schte Regel. Momentan k”nnen Sie 
es nicht benutzen, da es leer ist.*/
};

/* internals
 */
ApplInfo *applList = NULL;



/*
 * find ApplInfo-structure by the application-name
 * return NULL, if nothing found
 */
ApplInfo *GetApplInfo (ApplInfo *List, const char *name)
{
	ApplInfo *pai;
	
	pai = List;
	while (pai)
	{
		if (!stricmp (name, pai->name))
			return pai;
			
		pai = pai->nextappl;
	}
	
	return NULL;
}


/*
 * insert new Applinfo with given values
 */
word InsertApplInfo (ApplInfo **List, ApplInfo *prev, ApplInfo *ai)
{
	ApplInfo *myai;
	
	myai = malloc (sizeof (ApplInfo));
	if (!myai)
	{
		venusErr (NlsStr (T_MEMERR));
		return FALSE;
	}

	*myai = *ai;

	if (prev)
	{
		myai->nextappl = prev->nextappl;
		prev->nextappl = myai;
	}
	else
	{
		myai->nextappl = *List;
		*List = myai;
	}
	
	return TRUE;
}


/*
 * remove an ApplInfo from the List
 */
word DeleteApplInfo (ApplInfo **List, const char *name)
{
	ApplInfo *pai, **prevai;
	
	pai = *List;
	prevai = List;
	
	while (pai)
	{
		if (!stricmp (name, pai->name))
			break;
			
		prevai = &pai->nextappl;
		pai = pai->nextappl;
	}
	
	if(pai)
	{
		*prevai = pai->nextappl;
		free (pai);
	}
	else
	{
		venusDebug ("can't find ApplInfo.");
		return FALSE;
	}
	
	return TRUE;
}


/* 
 * edit a single ApplInfo structure
 * return if OK was pressed
 */
int EditSingleApplInfo (ApplInfo *pai)
{
	DIALINFO d;
	word startmode, retcode, redraw;
	char *pwildcard;
	int edit_object = APPLSPEX;
	int is_acc;
	

	pappledit[APPLNAME].ob_spec.tedinfo->te_ptext = pai->name;
	pwildcard = pappledit[APPLSPEX].ob_spec.tedinfo->te_ptext;

	startmode = pai->startmode;
	strcpy (pwildcard, pai->wildcard);

	setSelected (pappledit, APPLCLOS, startmode & WCLOSE_START);
	setSelected (pappledit, APPLGEM, startmode & GEM_START);
	setSelected (pappledit, APPLCOMM, startmode & TTP_START);
	setSelected (pappledit, APPLWAIT, startmode & WAIT_KEY);
	setSelected (pappledit, APPLOVL, startmode & OVL_START);
	setSelected (pappledit, APPLSING, startmode & SINGLE_START);
	setSelected (pappledit, APPLVAST, startmode & CANVA_START);
	
	is_acc = IsAccessory (pai->name);
	
	setDisabled (pappledit, APPLCLOS, is_acc);
	setDisabled (pappledit, APPLGEM, is_acc);
	setDisabled (pappledit, APPLCOMM, is_acc);
	setDisabled (pappledit, APPLWAIT, is_acc);
	setDisabled (pappledit, APPLOVL, is_acc);
	setDisabled (pappledit, APPLSING, is_acc);
	setDisabled (pappledit, APPLVAST, is_acc);
	
	DialCenter (pappledit);
	redraw = !DialStart (pappledit, &d);
	DialDraw (&d);
	
	retcode = DialDo (&d, &edit_object) & 0x7FFF;
	
	setSelected (pappledit, retcode, FALSE);
	DialEnd (&d);
	
	startmode = 0;
	if (isSelected (pappledit, APPLGEM))
		startmode |= GEM_START;
	if (isSelected (pappledit, APPLCOMM))
		startmode |= TTP_START;
	if (isSelected (pappledit, APPLCLOS))
		startmode |= WCLOSE_START;
	if (isSelected (pappledit, APPLWAIT))
		startmode |= WAIT_KEY;
	if (isSelected (pappledit, APPLOVL))
		startmode |= OVL_START;
	if (isSelected (pappledit, APPLSING))
		startmode |= SINGLE_START;
	if (isSelected (pappledit, APPLVAST))
		startmode |= CANVA_START;

	if (retcode == APPLOK)
	{
		pai->startmode = startmode;
		if (*pwildcard == '@')
			pai->wildcard[0] = '\0';
		else
			strcpy(pai->wildcard, pwildcard);
			
		strupr (pai->wildcard);

		
		return redraw? -1 : 1;
	}
	
	return redraw? -1 : 0;
}


static void freeListmanList (LISTSPEC *list)
{
	LISTSPEC *pl;
	
	while (list)
	{
		if (list->entry)
			free (list->entry);
		pl = list->next;
		free (list);
		list = pl;
	}
}

static LISTSPEC *buildListmanList (ApplInfo *aList, ApplInfo *selai,
	LISTSPEC **psel)
{
	LISTSPEC *list, *pl, **prev;
	ApplInfo *pai, **pprevai, *head;
	word sel = FALSE;
	
	pprevai = &head;
	list = NULL;
	prev = &list;
	
	while (aList)
	{
		pl = calloc (1, sizeof (LISTSPEC));
		if (!pl)
		{
			freeListmanList (list);
			return NULL;
		}

		pai = malloc (sizeof (ApplInfo));
		if (!pai)
		{
			free (pl);
			freeListmanList (list);
			return NULL;
		}
		*prev = pl;
		pl->next = NULL;
		
		*pai = *aList;
		pl->entry = pai;
		*pprevai = pai;
		pprevai = &pai->nextappl;
		pai->nextappl = NULL;
		
		if (aList == selai)
		{
			pl->flags.selected = 1;
			sel = TRUE;
			*psel = pl;
		}
		
		prev = &pl->next;
		aList = aList->nextappl;
	}
	
	if (list && !sel)
	{
		list->flags.selected = 1;
		*psel = list;
	}

	return list;
}

/*
 * Zeichne die ApplInfo <entry>
 */
static void drawApplInfo (LISTSPEC *l, word x, word y, word offset,
	GRECT *clip, word how)
{
	ApplInfo *pai;
	word dummy, xy[8];
	
	(void)how;
	if (!l)
		return;
		
	pai = l->entry;

	RectGRECT2VDI (clip, xy);
	vs_clip (vdi_handle, 1, xy);
	
	vswr_mode (vdi_handle, MD_REPLACE);
	vsf_interior (vdi_handle, FIS_SOLID);
	vsf_color (vdi_handle, l->flags.selected? 1 : 0);
	v_bar (vdi_handle, xy);

	vswr_mode (vdi_handle, MD_TRANS);
	vst_color (vdi_handle, l->flags.selected? 0 : 1);
	vst_alignment (vdi_handle, 0, 5, &dummy, &dummy);
	VstEffects (0);
	SetSystemFont ();
	v_gtext (vdi_handle, x-offset+HandXSize, y, pai->name);
	v_gtext (vdi_handle, x-offset+(MAX_FILENAME_LEN*HandXSize), 
		y, pai->wildcard);
}

static LISTSPEC *addNewAppl (LISTINFO *L, LISTSPEC *act, ApplInfo *appl)
{
	word bx, by;
	GRECT box;
	word tmpx, tmpy;
	long itemnr;
	LISTSPEC *pl;
	
	if (!appl)
		return act;
		
	objc_offset (papplbox, APBOX, &box.g_x, &box.g_y);
	box.g_w = papplbox[APBOX].ob_width;
	box.g_h = papplbox[APBOX].ob_height;
	objc_offset (papplbox, APNEW1, &tmpx, &tmpy);
	
	graf_dragbox (papplbox[APNEW1].ob_width, papplbox[APNEW1].ob_height, 
			tmpx, tmpy, papplbox[0].ob_x, papplbox[0].ob_y,
			papplbox[0].ob_width, papplbox[0].ob_height,  &bx, &by);
	
	if (!PointInGRECT (bx, by, &box))
		return act;
	
	objc_offset (papplbox, APBOX, &tmpx, &tmpy);
	bx -= tmpx;
	by -= tmpy;
	
	tmpy = papplbox[APSPEC].ob_height;
	itemnr = by / tmpy;
	if ((by % tmpy) < (tmpy / 2))
		--itemnr;
	
	pl = calloc (1, sizeof (LISTSPEC));
	if (!pl)
		return act;
		
	itemnr += L->startindex;
	pl->flags.selected = 1;
	pl->entry = appl;
	
	if ((itemnr < 0) || (L->listlength < 1))
	{
		pl->next = L->list;
		L->list = pl;
	}
	else
	{
		LISTSPEC *tmp;
		
		if (itemnr >= L->listlength)
			itemnr = L->listlength - 1;
		tmp = ListIndex2List (L->list, itemnr);
		
		pl->next = tmp->next;
		tmp->next = pl;
	}
	
	if (act)
		act->flags.selected = 0;
	
	return pl;
}

/*
 * l”sche den LISTSPEC <act> im LISTINFO <L>
 */
static LISTSPEC *deleteListSpec (LISTINFO *L, LISTSPEC *act)
{
	LISTSPEC *pl, **prevp, *prevspec;
	
	pl = L->list;
	prevp = &L->list;
	prevspec = NULL;
	while (pl)
	{
		if (pl == act)
			break;
		
		prevp = &pl->next;
		prevspec = pl;
		pl = pl->next;
	}
	
	*prevp = act->next;
	free (act);
	
	if (*prevp)
		pl = (*prevp);
	else if (prevspec)
		pl = prevspec;
	else
		return NULL;
	
	pl->flags.selected = 1;
	
	return pl;
}

void FreeApplList (ApplInfo **list)
{
	ApplInfo *pai, *next;
	
	pai = *list;
	*list = NULL;
	
	while (pai)
	{
		next = pai->nextappl;
		free (pai);
		pai = next;
	}
}

static void makeNewList (ApplInfo **head, LISTSPEC *list)
{
	ApplInfo **prev;
	
	FreeApplList (head);

	prev = head;
	while (list)
	{
		*prev = list->entry;
		prev = &(((ApplInfo *)list->entry)->nextappl);
		list->entry = NULL;
		
		list = list->next;
	}
	
	*prev = NULL;
}

static void switchClip (word full)
{
	word obx, oby, item;
	
	setHideTree (papplbox, APNEW1, full);
	setHideTree (papplbox, APNEW2, !full);
	
	item = full? APNEW2 : APNEW1;
	objc_offset (papplbox, item, &obx, &oby);
	ObjcDraw (papplbox, 0, MAX_DEPTH, obx, oby,
		papplbox[item].ob_width, papplbox[item].ob_height);
}

static int editApplListInternal (ApplInfo *pai)
{
	DIALINFO D;
	LISTINFO L;
	LISTSPEC *Llist, *actlist;
	ApplInfo *clipAppl, *actAppl;
	word retcode, draw;
	word clicks;
	word edit_object = APBGBOX;
	word key;
	long listresult;
	
	if (applList == NULL)
	{
		venusInfo (NlsStr (T_NOAPPL));
		return FALSE;
	}
	
	Llist = buildListmanList (applList, pai, &actlist);
	if (!Llist)
	{
		venusErr (NlsStr (T_MEMERR));
		return FALSE;
	}
	actAppl = actlist->entry;
	
	clipAppl = NULL;
	setHideTree (papplbox, APNEW1, FALSE);
	setHideTree (papplbox, APNEW2, TRUE);
	setDisabled (papplbox, APDEL, FALSE);
	setDisabled (papplbox, APEDIT, FALSE);

	DialCenter (papplbox);

	ListStdInit (&L, papplbox, APBOX, APBGBOX, drawApplInfo, 
		Llist, 0, 0, 1);
	L.maxwidth = HandXSize * (WILDLEN + 15);
	L.hstep = HandXSize;
	ListInit (&L);
	
	WindUpdate (BEG_MCTRL);
	
	DialStart (papplbox,&D);
	DialDraw (&D);
	ListScroll2Selection (&L);
	ListDraw (&L);
	
	do
	{
		draw = TRUE;
		
		retcode = DialXDo (&D, &edit_object, NULL, &key, NULL, NULL, NULL);
		clicks = (retcode & 0x8000)? 2 : 1;
		retcode &= 0x7FFF;

		switch (retcode)
		{
			case APBOX:
			case APBGBOX:
				edit_object = APBGBOX;
				
				if ((key & 0xff00) == 0x4800)
					listresult = ListMoveSelection (&L, 1);
				else if ((key & 0xff00) == 0x5000)
					listresult = ListMoveSelection (&L, 0);
				else if (key == 0)
					listresult = ListClick (&L, clicks);
				else
					listresult = -1;
					
				draw = FALSE;
				if (listresult >= 0)
				{
					actlist = ListIndex2List (L.list, listresult);
					actAppl = actlist->entry;
					if (clicks == 2)
					{
						draw = TRUE;
						retcode = APEDIT;
						setSelected (papplbox, APEDIT, TRUE);
						fulldraw (papplbox, APEDIT);
					}
				}
				break;

			case APNEW1:
			case APNEW2:
				if (clipAppl)
				{
					actlist = addNewAppl (&L, actlist, clipAppl);
				
					/* ist wirklich eingefgt worden?
					 */
					if (actlist && (actlist->entry != actAppl))
					{
						switchClip (FALSE);
						clipAppl = NULL;
						
						if (isDisabled (papplbox, APDEL))
						{
							setDisabled (papplbox, APDEL, FALSE);
							fulldraw (papplbox, APDEL);
							setDisabled (papplbox, APEDIT, FALSE);
							fulldraw (papplbox, APEDIT);
						}

						actAppl = actlist->entry;
						ListExit (&L);
						ListInit (&L);
						ListScroll2Selection (&L);
						ListDraw (&L);
					}
				}
				else
					venusInfo (NlsStr (T_CLIP));
				break;

			case APDEL:
			case APCOPY:
				if (clipAppl)
				{
					free (clipAppl);
					clipAppl = NULL;
				}
				else
				{
					switchClip (TRUE);
				}

				if (retcode == APCOPY)
				{
					clipAppl = malloc (sizeof (ApplInfo));
					if (clipAppl)
					{
						*clipAppl = *actAppl;
						clipAppl->nextappl = NULL;
					}
					else
						venusErr (NlsStr (T_MEMERR));
				}
				else
				{
					actlist = deleteListSpec (&L, actlist);
					clipAppl = actAppl;

					if (actlist)
						actAppl = actlist->entry;
					else
					{
						actAppl = NULL;
						setDisabled (papplbox, APDEL, TRUE);
						fulldraw (papplbox, APDEL);
						setDisabled (papplbox, APEDIT, TRUE);
						fulldraw (papplbox, APEDIT);
					}

					ListExit (&L);
					ListInit (&L);
					ListScroll2Selection (&L);
					ListDraw (&L);
				}
				break;
		}
		
		if (retcode == APEDIT)
		{
			DialEnd (&D);

			EditSingleApplInfo (actAppl);

			DialStart (papplbox,&D);
			DialDraw (&D);

			ListExit (&L);
			ListInit (&L);
			ListScroll2Selection (&L);
			ListDraw (&L);
		}
		
		if (draw)
		{
			setSelected (papplbox, retcode, FALSE);
			fulldraw (papplbox, retcode);
		}
		
	} 
	while (retcode != APOK && retcode != APCANCEL);
	
	ListExit (&L);
	DialEnd (&D);
	vst_color (vdi_handle, 1);
	
	WindUpdate (END_MCTRL);

	if (clipAppl)			/* noch was im Clipboard? */
		free (clipAppl);
	
	if (retcode == APOK)
	{
		makeNewList (&applList, L.list);
	}

	freeListmanList (L.list);
	
	return (retcode == APOK);
}

/*
 * make dialog for application installation
 */
void EditApplList (const char *name, const char *path,
	const char *label, word defstartmode)
{
	ApplInfo myai, *pai;
	word newrule;
	
	pai = NULL;
	if (name)
	{
		pai = GetApplInfo (applList, name);

		newrule = (pai == NULL);
	
		if (newrule)
		{
			strcpy (myai.path, path);
			strcpy (myai.name, name);
			strcpy (myai.label, label);
			myai.wildcard[0] = '\0';
			myai.nextappl = NULL;
			myai.startmode = defstartmode;
			pai = &myai;
		}

		if (EditSingleApplInfo (pai))
		{
			if (newrule)
			{
				if (!InsertApplInfo (&applList, NULL, pai))
					venusDebug ("inserting appl failed!");
			}
		}
		else
			return;
	}
	
	editApplListInternal (pai);
}

