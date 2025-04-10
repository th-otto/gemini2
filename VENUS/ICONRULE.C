/*
 * @(#) Gemini\Iconrule.c
 * @(#) Stefan Eissing, 01. November 1994
 *
 * description: functions to administrate icons
 * 
 * jr 26.7.94
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\listman.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"

#include "vs.h"
#include "rulebox.rh"
#include "stdbox.rh"
#include "ruleedit.rh"

#include "window.h"
#include "color.h"
#include "filewind.h"
#include "filedraw.h"
#include "fileutil.h"
#include "icondraw.h"
#include "iconhash.h"
#include "iconinst.h"
#include "iconrule.h"
#include "myalloc.h"
#include "redraw.h"
#include "select.h"
#include "util.h"
#include "venuserr.h"
#include "wildcard.h"


#define LOAD_COLOR_ICONS	1

#ifdef LOAD_COLOR_ICONS

#include <portab.h>
#include "..\xrsrc\xrsrc.h"

#endif

/* externals
 */
extern OBJECT *prulebox, *pstdbox, *pruleedit;

 
/*
 * internal texts
 */
#define NlsLocalSection "G.iconrule"
enum NlsLocalText{
T_NOMEMEDIT,	/*Es ist nicht genug Speicher vorhanden, um die 
Regeln zu editieren!*/
T_ICONS,		/*Es sind zuwenig Icons in der Datei %s.*/
T_ICONNR,		/*Die Icon-Information in der Regel %s stimmt 
nicht. Das normale Icon wird dafÅr eingesetzt.*/
};


#define DEFAULTFOLDER	2
#define DEFAULTDATA		1
#define DEFAULTDRIVE	6

#define DESKTREE	0		/* trees in venusicn.h */
#define FILENORM	1
#define FILEMINI	2

#define TEXTOFFSET		80


static RSIB0MASK[] =
{ 0x03C0, 0x7FE0, 0xFFF8, 0xFFF8, 
  0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 
  0xFFF8, 0xFFF8, 0xFFF8, 0x7FF8, 
  0x3FF0
};

static RSIB0DATA[] =
{ 0x03C0, 0x7C20, 0x83D8, 0x8018, 
  0x8018, 0x8018, 0x8018, 0x8018, 
  0x8018, 0x8018, 0x8018, 0x7FF8, 
  0x3FF0
};

static RSIB1MASK[] =
{ 0x0600, 0x7F00, 0xFFC0, 0xFFC0, 
  0xFFC0, 0xFFC0, 0xFFC0, 0xFFC0, 
  0x7FC0, 0x3F80
};

static RSIB1DATA[] =
{ 0x0600, 0x7900, 0x86C0, 0x80C0, 
  0x80C0, 0x80C0, 0x80C0, 0x80C0, 
  0x7FC0, 0x3F80
};

static RSIB2MASK[] =
{ 0x0C00, 0x7E00, 0xFF00, 0xFF00, 
  0xFF00, 0xFF00, 0xFF00, 0x7E00
};

static RSIB2DATA[] =
{ 0x0C00, 0x7200, 0x8F00, 0x8300, 
  0x8300, 0x8300, 0xFF00, 0x7E00
};

static ICONBLK small_folders[] =
{ RSIB0MASK, RSIB0DATA, "", 0x1000,
    0,   0,  31,   0,  13,  13,   0,  16,  78,   8,
  RSIB1MASK, RSIB1DATA, "", 0x1000,
    0,   0,  31,   0,  10,  10,   0,  16,  78,   8,
  RSIB2MASK, RSIB2DATA, "", 0x1000,
    0,   0,  31,   0,  8,   8,   0,  16,  78,   8
};


/* internals
 */
typedef struct
{
	/* trees from file 'GEMINIIC.RSC'
	 */
	OBJECT *pdeskicon;
	OBJECT *pnormicon;
	OBJECT *pminiicon;

	int desk_count;	
	int norm_count;	
	int mini_count;	
	
	/* Die grîûten Objekte im normalen und kleinen Iconbaum
	 */
	OBJECT stdBigIcon, stdMiniIcon;
	ICONBLK stdBigIconBlk, stdSmallIconBlk;

	/* AufhÑnger der Regel-fÅr-Icons-Liste
	 */
	DisplayRule *RuleList;

} ICON_BASE;

static ICON_BASE IBase;

/* Standardregeln
 */
static DisplayRule stdDataRule =
{
	NULL, NULL, NULL, FALSE, FALSE, "*", DEFAULTDATA, 0x10, 0x10
};
static DisplayRule stdFoldRule = 
{
	NULL, NULL, NULL, FALSE, TRUE, "*", DEFAULTFOLDER, 0x10, 0x10
};


void GetDeskIconMinMax (int *minIconNr, int *maxIconNr)
{
	*minIconNr = DEFAULTDRIVE;
	*maxIconNr = IBase.desk_count - 1;
}


/* ermittle das grîûte Icon in tree und speichere es in po
 */
static void setStdIcon (OBJECT *tree, word anz, OBJECT *po, 
	ICONBLK *icon)
{
	word i;
	*po = tree[1];
	*icon = *tree[1].ob_spec.iconblk;
	po->ob_spec.iconblk = icon;

	for (i = 2; i < anz; ++i)
	{
		if (po->ob_width < tree[i].ob_width)
			po->ob_width = tree[i].ob_width;

		if (po->ob_height < tree[i].ob_height)
			po->ob_height = tree[i].ob_height;
		
		if (icon->ib_wicon < tree[i].ob_spec.iconblk->ib_wicon)
			icon->ib_wicon = tree[i].ob_spec.iconblk->ib_wicon;

		if (icon->ib_hicon < tree[i].ob_spec.iconblk->ib_hicon)
			icon->ib_hicon = tree[i].ob_spec.iconblk->ib_hicon;
	}
}

/* Setze die Breite der Textfahne des Icons der LÑnge
 * des Textes entsprechend
 */
void SetIconTextWidth (ICONBLK *pib, OBJECT *po)
{
	if (pib->ib_xtext >= pib->ib_xicon + pib->ib_wicon)
	{
		/* Der Text liegt rechts vom Bild. Wir fixen nur die
		 * Breite der Textfahne.
		 */
		pib->ib_wtext = ((word)strlen (pib->ib_ptext) + 1)
			 * SystemIconFont.char_width;
	}
	else
	{
		word center;

		center = pib->ib_xtext + (pib->ib_wtext / 2);
		pib->ib_wtext = ((word)strlen (pib->ib_ptext) + 1)
			 * SystemIconFont.char_width;
		pib->ib_xtext = center - (pib->ib_wtext / 2);
	}
	
	if (po != NULL)
	{
		word min_max, tmp;
		
		/* Schiebe Text und Icon soweit es geht nach links
		 */
		min_max = min (pib->ib_xicon, pib->ib_xtext);
		if (min_max)
		{
			pib->ib_xicon -= min_max;
			pib->ib_xtext -= min_max;
			po->ob_x += min_max;
		}
		
		/* Setze die Breite des Objektes neu
		 */
		min_max = pib->ib_xicon + pib->ib_wicon;
		tmp = pib->ib_xtext + pib->ib_wtext;
		po->ob_width = max (min_max, tmp);
	}
}

static void setBarHeight (OBJECT *tree, int text_to_right)
{
	word height = pstdbox[STDICON].ob_spec.iconblk->ib_htext;
	word i, diff;
	
	for (i = tree[0].ob_head; i > 0; i = tree[i].ob_next)
	{
		ICONBLK *pib = tree[i].ob_spec.iconblk;
		
		if (text_to_right)
		{
			pib->ib_xicon = 0;
			pib->ib_yicon = 0;
			pib->ib_xtext = pib->ib_wicon;
			pib->ib_ytext = (pib->ib_hicon - pib->ib_htext) / 2;
			tree[i].ob_height = pib->ib_hicon;
			tree[i].ob_width = pib->ib_xtext + pib->ib_wtext;
		}

		SetIconTextWidth (pib, NULL);
		
		diff = height - pib->ib_htext;
		pib->ib_htext = height;
		
		if (!text_to_right)
		{
			diff = (pib->ib_ytext + pib->ib_htext) - tree[i].ob_height;
			if (diff > 0)
				tree[i].ob_height += diff;
		}
	}
}

/* Bearbeitet die Icons im nachhineine noch ein biûchen, so daû
 * alle Fahnen gleich hoch sind und auch schon das grîûte Objekt
 * bekannt ist.
 */
void FixIcons (void)
{
	/* Setze die Hîhe der Icon-Fahnen
	 */
	setBarHeight (IBase.pdeskicon, FALSE);
	setBarHeight (IBase.pnormicon, FALSE);
	setBarHeight (IBase.pminiicon, TRUE);
			
	/* hole die grîûten Objekte der BÑume
	 */
	setStdIcon (IBase.pnormicon, IBase.norm_count, 
		&IBase.stdBigIcon, &IBase.stdBigIconBlk);
	setStdIcon (IBase.pminiicon, IBase.norm_count, 
		&IBase.stdMiniIcon, &IBase.stdSmallIconBlk);
}

#if !LOAD_COLOR_ICONS
static void *icon_rsc;
#else
static word xrs_global[16];
#endif

void ExitIcons (void)
{
#if LOAD_COLOR_ICONS
	xrsrc_free (xrs_global);
	term_xrsrc ();
#else
	*((char **)(&_GemParBlk.global[7])) = icon_rsc;
	rsrc_free ();
#endif
}

/*
 * load file <name> and initialize the treepointers
 */
word InitIcons (const char *name)
{
	word ok = TRUE;
	
#if LOAD_COLOR_ICONS

	if (!init_xrsrc (vdi_handle, &Desk, wchar, hchar)
		|| !xrsrc_load (name, xrs_global))
		return FALSE;

	ok = xrsrc_gaddr (R_TREE, DESKTREE, &IBase.pdeskicon, xrs_global);
	if (ok)
		ok = xrsrc_gaddr (R_TREE, FILENORM, &IBase.pnormicon, xrs_global);
	
	/* Wenn keine kleinen Icons da sind, nehmen wir die groûen
	 * dafÅr
	 */
	if (ok && (!xrsrc_gaddr (R_TREE, FILEMINI, 
		&IBase.pminiicon, xrs_global)
		|| (IBase.pminiicon == NULL)))
	{
		IBase.pminiicon = IBase.pnormicon;
	}

#else
	if (!rsrc_load (name))
		return FALSE;

	icon_rsc = *((char **)(&_GemParBlk.global[7]));

	ok = rsrc_gaddr (R_TREE, DESKTREE, &IBase.pdeskicon);
	if (ok)
		ok = rsrc_gaddr (R_TREE, FILENORM, &IBase.pnormicon);
	
	/* Wenn keine kleinen Icons da sind, nehmen wir die groûen
	 * dafÅr
	 */
	if (ok && (!rsrc_gaddr (R_TREE, FILEMINI, 
		&IBase.pminiicon)
		|| (IBase.pminiicon == NULL)))
	{
		IBase.pminiicon = IBase.pnormicon;
	}
#endif	/* LOAD_COLOR_ICONS - else part */
			
	if (!ok)
		return FALSE;

	IBase.RuleList = NULL;
	IBase.desk_count = countObjects (IBase.pdeskicon, 0);
	IBase.norm_count = countObjects (IBase.pnormicon, 0);
	IBase.mini_count = countObjects (IBase.pminiicon, 0);
	IBase.norm_count = IBase.mini_count = 
		min (IBase.norm_count, IBase.mini_count);
		
	if ((IBase.norm_count < 3)||(IBase.desk_count < 7))
	{
		venusErr (NlsStr (T_ICONS), name);
		return FALSE;
	}
	
	return TRUE;
}

/*
 * gibt die Regel fÅr den Datei-/Ordnernamen <fname> zurÅck
 */
static DisplayRule *getDisplayRule (word isFolder, char *fname,
	int case_sensitiv)
{
	DisplayRule *actrule = IBase.RuleList, *pd;

	if ((pd = GetHashedRule (isFolder, fname, case_sensitiv)) != NULL)
		return pd;
	else if (isFolder)
	{
		while (actrule != NULL)
		{
			if ((actrule->isFolder) && (!actrule->wasHashed)
				&& (FilterFile (actrule->wildcard, fname, case_sensitiv)))
			{
				return actrule;
			}
			actrule = actrule->nextrule;
		}
	}
	else
	{
		while (actrule != NULL)
		{
			if ((!actrule->wasHashed) && (!actrule->isFolder)
				&&(FilterFile (actrule->wildcard, fname, case_sensitiv)))
			{
				return actrule;
			}
			actrule = actrule->nextrule;
		}
	}
	return NULL;
}

/*
 * get number and color of the icon for file fname by looking at
 * the filerules
 */
static word getIconNr (word isFolder,char *fname, char *color,
	int case_sensitiv)
{
	DisplayRule *rule;
	word iconnr;
	
	rule = getDisplayRule (isFolder, fname, case_sensitiv);
	
	if (!rule)
	{
		*color = 0x10;
		return (isFolder)? DEFAULTFOLDER : DEFAULTDATA;
	}

	iconnr = rule->iconNr;
	*color = rule->color;
	
	if(iconnr < IBase.norm_count)			/* gÅltige Iconnummer */
		return iconnr;
	else
	{
		venusDebug ("IconNr zu hoch!");
		return (isFolder)? DEFAULTFOLDER : DEFAULTDATA;
	}
}

/*
 * return pointer to object of icon for file fname
 */
OBJECT *GetBigIconObject (word isFolder, char *fname, char *color)
{
	word nr;

	nr = getIconNr (isFolder, fname, color, FALSE);
	return &IBase.pnormicon[nr];
}

OBJECT *GetSmallIconObject (word isFolder, char *fname, char *color)
{
	word nr;

	nr = getIconNr (isFolder, fname, color, FALSE);
	return &IBase.pminiicon[nr];
}


OBJECT *GetStdFolderIcon (int height)
{
	ICONBLK *icon;
	static OBJECT obj;
	
	if (height >= IBase.stdBigIconBlk.ib_hicon) 
		return &IBase.pnormicon[DEFAULTFOLDER];
	else if (height >= IBase.stdSmallIconBlk.ib_hicon)
		return &IBase.pminiicon[DEFAULTFOLDER];
	else if (height >= small_folders[0].ib_hicon)
		icon = small_folders;
	else if (height >= small_folders[1].ib_hicon)
		icon = &small_folders[1];
	else
		icon = &small_folders[2];
	
	obj.ob_type = G_ICON;
	obj.ob_spec.iconblk = icon;
	return &obj;
}

/*
 * get object structure fo first object in icontree
 */
OBJECT *GetStdBigIcon (void)
{
	return &IBase.stdBigIcon;
}

OBJECT *GetStdSmallIcon (void)
{
	return &IBase.stdMiniIcon;
}

/*
 * display icon in listdialog
 */
static void drawFileIcon (LISTSPEC *l, word x, word y, word offset,
				GRECT *clip, word how)
{
	word nr, xy[8];
	word len, smalloff;
	PARMBLK param;
	ICON_OBJECT icon;
	(void)how;
	if (!l)
		return;
	
	nr = (word)(l->entry);

	RectGRECT2VDI (clip, xy);
	vs_clip (vdi_handle, 1, xy);

	vsf_interior (vdi_handle, FIS_SOLID);
	vsf_color (vdi_handle, (l->flags.selected)? 1 : 0);
	vswr_mode (vdi_handle, MD_REPLACE);
	v_bar (vdi_handle, xy);

	/* offset zum zweiten Icon
	 */
	smalloff = (pruleedit[RESPEC].ob_width / 16) * 9;
	
	memset (&param, 0, sizeof (param));
	param.pb_xc = clip->g_x;
	param.pb_yc = clip->g_y;
	param.pb_wc = clip->g_w;
	param.pb_hc = clip->g_h;
	param.pb_parm = (long)&icon;

	FillIconObject (&icon, &IBase.pnormicon[nr]);
	
	len = (smalloff - IBase.pnormicon[nr].ob_width) / 2;
	param.pb_x = x + 1 - offset + len;
	len = (pruleedit[RESPEC].ob_height - 
		IBase.pnormicon[nr].ob_height) / 2;
	param.pb_y = y + 1 + len;
	param.pb_w = IBase.pnormicon[nr].ob_width;
	param.pb_h = IBase.pnormicon[nr].ob_height;
	
	param.pb_wc = min (clip->g_w, smalloff);
	DrawIconObject (&param);
	
	FillIconObject (&icon, &IBase.pminiicon[nr]);
		
	len = (pruleedit[RESPEC].ob_width - smalloff - 
		IBase.pminiicon[nr].ob_width) / 2;
	param.pb_x = x + 1 - offset + len + smalloff;

	len = (pruleedit[RESPEC].ob_height - 
		IBase.pminiicon[nr].ob_height) / 2;
	param.pb_y = y + 1 + len;

	param.pb_w = IBase.pminiicon[nr].ob_width;
	param.pb_h = IBase.pminiicon[nr].ob_height;

	param.pb_wc = clip->g_w;
	DrawIconObject (&param);
}


static LISTSPEC *buildFileIconList (word selindex)
{
	LISTSPEC *l;
	word i, maxindex = IBase.norm_count - 2;
	
	l = calloc (maxindex + 1, sizeof (LISTSPEC));
	if (!l)
		return NULL;
	
	for (i = 0; i < maxindex; ++i)
	{
		l[i].next = &l[i+1];
		l[i].entry = (void *)(i+1);
	}
	l[maxindex].next = NULL;
	l[maxindex].entry = (void *)(maxindex + 1);
	
	/* setze selektiert
	 */
	if (selindex <= maxindex)
		l[selindex].flags.selected = 1;
	
	return l;
}

/*
 * edit a single DisplayRule and return if something
 * has changed
 */
static word editSingleRule (DisplayRule *rule)
{
	DIALINFO d;
	LISTINFO L;
	LISTSPEC *list;
	word iconNr, redraw;
	word retcode, clicks;
	long listresult;
	word draw, exit, cycle_obj;
	char *wildcard;
	char color;
	int edit_object = REBGBOX;
	uword key;
	
	iconNr = rule->iconNr - 1;
	wildcard = pruleedit[REWILD].ob_spec.tedinfo->te_ptext;
	strcpy (wildcard, rule->wildcard);

	setSelected (pruleedit, REFOLDER, rule->isFolder);
	
	list = buildFileIconList (iconNr);
	if (!list)
	{
		venusErr (NlsStr (T_NOMEMEDIT));
		return FALSE;
	}
	
	color = rule->truecolor;
	ColorSetup (color, pruleedit, RECOLFG,
		RECLFBOX, RECOLBG, RECLBBOX);
				
	DialCenter (pruleedit);

	ListStdInit (&L, pruleedit, REBOX, REBGBOX, drawFileIcon, 
		list, 0, iconNr, 1);
	ListInit (&L);

	redraw = !DialStart (pruleedit, &d);
	DialDraw (&d);
	ListDraw (&L);
	
	exit = FALSE;
	do
	{
		uword shift_state;

		draw = FALSE;
		cycle_obj = -1;
		
		retcode = DialXDo (&d, &edit_object, &shift_state, &key,
			NULL, NULL, NULL);
		clicks = (retcode & 0x8000)? 2 : 1;
		retcode &= 0x7FFF;

		switch (retcode)
		{
			case REBOX:
			case REBGBOX:
				edit_object = REBGBOX;
				
				if ((key & 0xff00) == 0x4800)
					listresult = ListMoveSelection (&L, 1);
				else if ((key & 0xff00) == 0x5000)
					listresult = ListMoveSelection (&L, 0);
				else if (key == 0)
					listresult = ListClick (&L, clicks);
				else
					listresult = -1;
					
				if (listresult >= 0)
				{
					iconNr = (word)listresult;
					exit = (clicks == 2);
					retcode = REOK;
				}
				break;

			case RECOLFG0:
				cycle_obj = RECOLFG0;
			case RECOLFG:
				if (shift_state & 3)
					cycle_obj = RECOLFG0;
			case REFGSC:
				color = ColorSelect (color, TRUE, pruleedit,
				 			RECOLFG, RECLFBOX, cycle_obj);
				break;

			case RECOLBG0:
				cycle_obj = RECOLBG0;
			case RECOLBG:
				if (shift_state & 3)
					cycle_obj = RECOLBG0;
			case REBGSC:
				color = ColorSelect (color, FALSE, pruleedit,
				 			RECOLBG, RECLBBOX, cycle_obj);
				break;

			default:
				draw = exit = TRUE;
				break;
		}

		if (draw)
		{				
			setSelected (pruleedit, retcode, FALSE);
			fulldraw (pruleedit, retcode);
		}

	} 
	while (!exit);

	ListExit (&L);
	free (list);
	DialEnd (&d);
	
	if (retcode == REOK)
	{
		strcpy (rule->wildcard, wildcard);
		rule->isFolder = isSelected (pruleedit, REFOLDER);
		rule->iconNr = iconNr + 1;
		rule->truecolor = color;
		rule->color = ColorMap (rule->truecolor);
		rule->wasHashed = FALSE;
		
		return redraw? -1 : 1;
	}
	
	return redraw? -1 : 0;
}


/*
 * insert a at start of the list of rules if prev is NULL
 * else insert after prev
 */
static DisplayRule *insertIconRule (DisplayRule **plist,
				DisplayRule *prev, DisplayRule *pdr)
{
	DisplayRule *pa;
	
	pa = malloc (sizeof (DisplayRule));
	if(!pa)
		return NULL;

	memcpy (pa, pdr, sizeof (DisplayRule));
	
	if (prev)
	{
		pa->nextrule = prev->nextrule;
		pa->prevrule = prev;
		if (pa->nextrule)
			pa->nextrule->prevrule = pa;
		prev->nextrule = pa;
	}
	else
	{
		pa->nextrule = *plist;
		pa->nextHash = pa->prevrule = NULL;
		if (*plist != NULL)
			(*plist)->prevrule = pa;
		*plist = pa;
	}
	pa->wasHashed = FALSE;
	
	return pa;
}

/*
 * delete the DisplayRule <pdr> from the list <plist>
 */
static word deleteIconRule (DisplayRule **plist, DisplayRule *pdr)
{
	if (pdr->prevrule)
		pdr->prevrule->nextrule = pdr->nextrule;
	else
		*plist = pdr->nextrule;
	
	if (pdr->nextrule)
		pdr->nextrule->prevrule = pdr->prevrule;
	
	free (pdr);
	return TRUE;
}

/*
 * free list of rules starting at liststart
 */
static void freeIconRuleList (DisplayRule **liststart)
{
	DisplayRule *aktRule, *prevRule;
	
	aktRule = *liststart;
	*liststart = NULL;

	while (aktRule != NULL)
	{
		prevRule = aktRule;
		aktRule = aktRule->nextrule;
		free (prevRule);
	}
}

/*
 * insert the default rules (for *)
 */
void InsDefIconRules (void)
{
	insertIconRule (&IBase.RuleList, NULL, &stdFoldRule);
	insertIconRule (&IBase.RuleList, NULL, &stdDataRule);
}

/*
 * free the normal rulelist (starts at firstrule)
 */
void FreeIconRules (void)
{
	ClearHashTable ();
	freeIconRuleList (&IBase.RuleList);
}

/* 
 * duplicate a list of rules (for editing)
 */
static DisplayRule *dupRules (DisplayRule *liststart)
{
	DisplayRule *newlist,*pa,*prev;

	newlist = prev = NULL;

	while (liststart != NULL)
	{

		pa = (DisplayRule *)malloc (sizeof (DisplayRule));
		if (newlist == NULL)
			newlist = pa;
			
		if (pa == NULL)
			return NULL;

		memcpy (pa, liststart, sizeof (DisplayRule));
	
		if (prev)
		{
			pa->prevrule = IBase.RuleList;
			prev->nextrule = pa;
		}
		
		pa->prevrule = prev;
		pa->nextHash = pa->nextrule = NULL;
		pa->wasHashed = FALSE;
		prev = pa;
		liststart = liststart->nextrule;
	}

	return newlist;
}

static void freeListList (LISTSPEC *head)
{
	LISTSPEC *tmp;
	
	while (head)
	{
		tmp = head->next;
		free (head);
		head = tmp;
	}	
}

static LISTSPEC *buildListList(DisplayRule *head)
{
	LISTSPEC *start, **prev, *act;
	
	start = NULL;
	prev = &start;
	
	while (head)
	{
		act = calloc (1, sizeof (LISTINFO));
		if (!act)
		{
			freeListList (start);
			return NULL;
		}
		act->entry = head;
		*prev = act;
		prev = &act->next;
	
		head = head->nextrule;
	}
	
	return start;
}

/* selektiere die Regel, die fÅr ein selektiertes Objekt
 * zutrifft, wenn es ein solches gibt.
 */
LISTSPEC *selectListSpec (LISTSPEC *head)
{
	WindInfo *wp;
	word objnr;
	
	if (GetOnlySelectedObject (&wp, &objnr))
	{
		word isFolder = FALSE;
		word found = FALSE;
		char filename[MAX_PATH_LEN];
		
		switch (wp->kind)
		{
			case WK_FILE:
			{
				FileWindow_t *fwp = wp->kind_info;
				FileInfo *pfi;
				
				pfi = GetSelectedFileInfo (fwp);
				if (pfi != NULL)
				{
					isFolder = pfi->attrib & FA_FOLDER;
					strcpy (filename, pfi->name);
					found = TRUE;
				}
				break;
			}
			
			case WK_DESK:
			{
				DeskInfo *dip = wp->kind_info;
				IconInfo *pii;
				
				if ((pii = GetIconInfo (&dip->tree[objnr])) != NULL)
				{
					switch(pii->type)
					{
						case DI_FOLDER:
							isFolder = TRUE;

						case DI_PROGRAM:
							GetBaseName (pii->fullname, filename, MAX_PATH_LEN);
							found = TRUE;
							break;
					}
				}
				break;
			}
		}
		
		if (found)
		{
			DisplayRule *orig, *new;
			
			if ((orig = getDisplayRule (isFolder, filename, FALSE)) != NULL)
			{
				LISTSPEC *pls = head;

				while (pls)
				{
					new = pls->entry;
					if (!strcmp (new->wildcard, orig->wildcard)
						&& (new->isFolder == orig->isFolder))
					{
						pls->flags.selected = 1;
						return pls;
					}
					pls = pls->next;
				}
			}
		}
	}
	
	if (head)
		head->flags.selected = 1;
	
	return head;
}

/*
 * display Rule in listdialog
 */
static void drawRule (LISTSPEC *l, word x, word y, word offset,
				GRECT *clip, word how)
{
	DisplayRule *pr;
	PARMBLK param;
	ICON_OBJECT icon;
	word dummy, xy[8], width;
	char line[WILDLEN + 3];
	
	if (!l)
		return;
		
	(void)how;
	pr = l->entry;
	
	RectGRECT2VDI (clip, xy);
	vs_clip (vdi_handle, 1, xy);

	vsf_interior (vdi_handle, FIS_SOLID);
	vsf_color (vdi_handle, (l->flags.selected)? 1 : 0);
	vswr_mode (vdi_handle, MD_REPLACE);
	v_bar (vdi_handle, xy);

	memset (&param, 0, sizeof (param));
	param.pb_xc = clip->g_x;
	param.pb_yc = clip->g_y;
	param.pb_wc = clip->g_w;
	param.pb_hc = clip->g_h;
	param.pb_parm = (long)&icon;

	param.pb_w = IBase.pnormicon[pr->iconNr].ob_width;
	param.pb_h = IBase.pnormicon[pr->iconNr].ob_height;

	width = (TEXTOFFSET - param.pb_w) / 2;
	param.pb_x = x + 2 - offset + width;

	width = (prulebox[RUSPEC].ob_height - param.pb_h) / 2;
	param.pb_y = y + 2 + width;
	
	FillIconObject (&icon, &IBase.pnormicon[pr->iconNr]);
	icon.icon.ib_char = (pr->color << 8) | (icon.icon.ib_char & 0x00FF);
	
	param.pb_w = IBase.pnormicon[pr->iconNr].ob_width;
	param.pb_h = IBase.pnormicon[pr->iconNr].ob_height;

	width = (TEXTOFFSET - param.pb_w) / 2;
	param.pb_x = x + 2 - offset + width;

	width = (prulebox[RUSPEC].ob_height - param.pb_h) / 2;
	param.pb_y = y + 2 + width;

	param.pb_wc = min (clip->g_w, TEXTOFFSET);
	DrawIconObject (&param);

	strcpy (line, "   ");
	if (pr->isFolder)
		line[1] = pstdbox[FISTRING].ob_spec.free_string[0];
	strcat (line, pr->wildcard);
	
	vs_clip (vdi_handle, 1, xy);
	vswr_mode (vdi_handle, MD_TRANS);
	vst_color (vdi_handle, (l->flags.selected)? 0 : 1);
	vst_alignment (vdi_handle, 0, 5, &dummy, &dummy);
	VstEffects (0);
	SetSystemFont ();
	v_gtext (vdi_handle, x-offset+TEXTOFFSET, y + HandBYSize, line);
	
	vst_color (vdi_handle, 1);
	vs_clip (vdi_handle, 0, xy);
}

/*
 * lîsche die DisplayRule <entry> in der Liste <head>, die alle
 * in der LISTSPEC-Liste <list> hÑngen.
 */
static LISTSPEC *deleteListSpec (LISTSPEC **list,
					DisplayRule **head, LISTSPEC *act)
{
	LISTSPEC *pl, **prevp, *prevspec;
	DisplayRule *entry = act->entry;
	
	pl = *list;
	prevp = list;
	prevspec = NULL;
	while (pl)
	{
		if (pl->entry == entry)
			break;
		
		prevp = &pl->next;
		prevspec = pl;
		pl = pl->next;
	}
	
	*prevp = pl->next;
	deleteIconRule (head, pl->entry);
	free (pl);
	
	if (*prevp)
		pl = (*prevp);
	else if (prevspec)
		pl = prevspec;
	else
		return NULL;
	
	pl->flags.selected = 1;
	
	return pl;
}

static LISTSPEC *addNewRule (LISTINFO *L, DisplayRule **head,
					 LISTSPEC *act, DisplayRule *rule)
{
	word bx, by;
	GRECT box;
	word tmpx, tmpy;
	long itemnr;
	LISTSPEC *pl;
	
	objc_offset (prulebox, RUBOX, &box.g_x, &box.g_y);
	box.g_w = prulebox[RUBOX].ob_width;
	box.g_h = prulebox[RUBOX].ob_height;
	objc_offset (prulebox, RUNEW, &tmpx, &tmpy);
	
	graf_dragbox (prulebox[RUNEW].ob_width, prulebox[RUNEW].ob_height, 
			tmpx, tmpy, prulebox[0].ob_x, prulebox[0].ob_y,
			prulebox[0].ob_width, prulebox[0].ob_height,  &bx, &by);
	
	if (!PointInGRECT (bx, by, &box))
		return act;
	
	objc_offset (prulebox, RUBOX, &tmpx, &tmpy);
	bx -= tmpx;
	by -= tmpy;
	
	tmpy = prulebox[RUSPEC].ob_height;
	itemnr = by / tmpy;
	if ((by % tmpy) < (tmpy / 2))
		--itemnr;
	
	pl = calloc (1, sizeof (LISTSPEC));
	if (!pl)
		return act;
		
	itemnr += L->startindex;
	pl->flags.selected = 1;
	
	if (itemnr < 0)
	{
		pl->entry = insertIconRule (head, NULL, rule);
		if (!pl->entry)
		{
			free (pl);
			return act;
		}
		pl->next = L->list;
		L->list = pl;
	}
	else
	{
		LISTSPEC *tmp;
		
		if (itemnr >= L->listlength)
			itemnr = L->listlength - 1;

		tmp = ListIndex2List (L->list, itemnr);
		
		pl->entry = insertIconRule (head, tmp->entry, rule);
		if (!pl->entry)
		{
			free (pl);
			return act;
		}
		pl->next = tmp->next;
		tmp->next = pl;
	}
	act->flags.selected = 0;
	
	return pl;
}

/*
 * let user edit the rules with dialog and such things
 * and return if something has changed
 */
word EditIconRule (void)
{
	DIALINFO d;
	LISTINFO L;
	LISTSPEC *Llist, *actlist;
	DisplayRule *actualRule;
	DisplayRule myRule, *tmplist;
	word retcode, draw;
	word clicks, rulecopied;
	word edit_object = RUBGBOX;
	uword key;
	long listresult;
	
	if (IBase.RuleList == NULL)
	{
		venusDebug ("got no Rules!");
		return FALSE;
	}
	
	tmplist = dupRules (IBase.RuleList);
	if (!tmplist)
	{
		venusErr (NlsStr (T_NOMEMEDIT));
		return FALSE;
	}
	
	actlist = Llist = buildListList (tmplist);
	if (!Llist)
	{
		freeIconRuleList (&tmplist);
		venusErr (NlsStr (T_NOMEMEDIT));
		return FALSE;
	}
	
	actlist = selectListSpec (Llist);
	actualRule = actlist->entry;
	
	setDisabled (prulebox, RUDEL, actualRule->nextrule == NULL);
	memcpy (&myRule, &stdDataRule, sizeof (DisplayRule));
	rulecopied = FALSE;

	DialCenter (prulebox);

	ListStdInit (&L, prulebox, RUBOX, RUBGBOX, drawRule, 
				Llist, 0, 0, 1);
	L.maxwidth = TEXTOFFSET + (HandXSize * (3 + WILDLEN));
	L.hstep = HandXSize;
	ListInit (&L);
		
	WindUpdate (BEG_MCTRL);

	DialStart (prulebox, &d);
	DialDraw (&d);
	ListScroll2Selection (&L);
	ListDraw (&L);
	
	do
	{
		draw = TRUE;
		
		retcode = DialXDo (&d, &edit_object, NULL, &key, NULL, NULL, NULL);
		clicks = (retcode & 0x8000)? 2 : 1;
		retcode &= 0x7FFF;

		switch(retcode)
		{
			case RUBOX:
			case RUBGBOX:
				edit_object = RUBGBOX;
				
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
					actualRule = (DisplayRule *)(actlist->entry);
					if (clicks == 2)
					{
						draw = TRUE;
						retcode = RUEDIT;
						setSelected (prulebox, RUEDIT, TRUE);
						fulldraw (prulebox, RUEDIT);
					}
				}
				break;

			case RUINS:
				memcpy (&myRule, actualRule, sizeof (DisplayRule));
				if (!rulecopied)
					rulecopied = TRUE;
				break;

			case RUNEW:
				actlist = addNewRule (&L, &tmplist, actlist, &myRule);
				
				/* ist wirklich eingefÅgt worden?
				 */
				if (actlist->entry != actualRule)
				{
					actualRule = actlist->entry;
					if (isDisabled (prulebox, RUDEL))
					{
						setDisabled (prulebox, RUDEL, FALSE);
						fulldraw (prulebox, RUDEL);
					}
					ListExit (&L);
					ListInit (&L);
					ListScroll2Selection (&L);
					ListDraw (&L);
				}
				break;

			case RUDEL:
				memcpy (&myRule, actualRule, sizeof (DisplayRule));
				if (!rulecopied)
					rulecopied = TRUE;

				actlist = deleteListSpec (&L.list, &tmplist, actlist);
				actualRule = actlist->entry;

				if (tmplist->nextrule == NULL)
				{
					setDisabled (prulebox, RUDEL, TRUE);
					fulldraw (prulebox, RUDEL);
				}
				
				ListExit (&L);
				ListInit (&L);
				ListScroll2Selection (&L);
				ListDraw (&L);
				break;
		}
		
		if (retcode == RUEDIT)
		{
			DialEnd (&d);
			
			editSingleRule (actualRule);
			
			DialStart (prulebox, &d);
			DialDraw (&d);

			ListExit (&L);
			ListInit (&L);
			ListScroll2Selection (&L);
			ListDraw (&L);
		}
		
		if (draw)
		{
			setSelected (prulebox, retcode, FALSE);
			fulldraw (prulebox, retcode);
		}
		
	} 
	while (retcode != RUOK && retcode != RUCANCEL);
	
	ListExit (&L);
	freeListList (L.list);
	DialEnd (&d);
	vst_color (vdi_handle, 1);

	WindUpdate (END_MCTRL);

	if (retcode == RUOK)
	{
		actualRule = IBase.RuleList;
		IBase.RuleList = tmplist;
		freeIconRuleList (&actualRule);
		BuiltIconHash ();
		return TRUE;
	}
	else
	{
		freeIconRuleList (&tmplist);
		return FALSE;
	}
}

/*
 * add a rule at start of list
 * line comes probably from venus.inf
 */
int AddIconRule (const char *line)
{
	DisplayRule pd;
	char *cp, *tp;
	
	cp = malloc (strlen (line) + 1);

	if (cp == NULL)
		return FALSE;

	strcpy (cp, line);
	strtok (cp, "@\n");					/* skip first */

	if ((tp = strtok (NULL, "@\n")) != NULL)
	{
		strcpy (pd.wildcard, tp);
		if ((tp = strtok (NULL, "@\n")) != NULL)
		{
			pd.isFolder = atoi (tp);
			if ((tp = strtok (NULL, "@\n")) != NULL)
			{
				pd.iconNr = atoi (tp);
				if ((pd.iconNr > IBase.norm_count) || (pd.iconNr < 1))
				{
					venusInfo (NlsStr (T_ICONNR), pd.wildcard);
					pd.iconNr = pd.isFolder? 2 : 1;
				}
				
				if ((tp = strtok (NULL, "@\n")) != NULL)
					pd.truecolor = (char)atoi (tp);
				else
					pd.truecolor = 0x10;

				pd.color = ColorMap (pd.truecolor);
				insertIconRule (&IBase.RuleList, NULL, &pd);
			}
		}
	}
	
	free(cp);
	return TRUE;
}

/* Schreibe die Liste der Regeln in die Datei fhandle
 */
static int writeRekursiv (MEMFILEINFO *mp, char *buffer, 
	DisplayRule *pd)
{
	if (pd == NULL)
		return TRUE;

	if (!writeRekursiv (mp, buffer, pd->nextrule))
		return FALSE;
	
	sprintf (buffer, "#R@%s@%d@%d@%d", pd->wildcard, pd->isFolder,
		pd->iconNr, (word)pd->truecolor);

	return (mputs (mp, buffer) == 0L);
}


/* Schreibe die Regeln in die Datei fhandle
 */
int WriteDisplayRules (MEMFILEINFO *mp, char *buffer)
{
	return writeRekursiv (mp, buffer, IBase.RuleList);
}


/*
 * get object structure of standard deskicon
 */
OBJECT *GetStdDeskIcon (void)
{
	return &IBase.pdeskicon[1];
}

/*
 * get pointer to deskicon nr
 */
OBJECT *GetDeskObject (word nr)
{
	if(nr >= IBase.desk_count)
		nr = DEFAULTDRIVE;

	return &IBase.pdeskicon[nr];
}

DisplayRule *GetFirstDisplayRule (void)
{
	return IBase.RuleList;
}