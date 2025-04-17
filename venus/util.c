/*
 * @(#) Gemini\util.c
 * @(#) Stefan Eissing, 15. August 1994
 *
 * description: different functions 
 *
 * jr 25.7.94
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <aes.h>
#include <tos.h>
#include <flydial\flydial.h>
#include <flydial\evntevnt.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"

#include "vs.h"
#include "menu.rh"

#include "window.h"
#include "filewind.h"
#include "applmana.h"
#include "fileutil.h"
#include "iconinst.h"
#include "iconrule.h"
#include "myalloc.h"
#include "redraw.h"
#include "select.h"
#include "stand.h"
#include "undo.h"
#include "util.h"
#include "venuserr.h"



/* internals
 */
#define MAXPATHS	32
static char *pathStack[MAXPATHS];
static word pathStackTop = 0;

/* calc p1 * p2 / p3 with long arithmetic
 */
long scale123(register long p1,register long p2,register long p3)
{
	p2 = p1 *= p2;
	p1 /= p3;
	if(p2 && ((p2 % p3) * 2) >= p3)
		p1++;
	return p1; 
}


void fulldraw(OBJECT *tree,word objnr)
{
	word obx,oby;
	
	objc_offset(tree,objnr,&obx,&oby);
	ObjcDraw(tree,objnr,MAX_DEPTH,obx,oby,
				tree[objnr].ob_width,tree[objnr].ob_height);
}


static void pathUpdate (const char *path, int icons_also)
{
	WindInfo *wp;
	
	wp = wnull.nextwind;
	while (wp)
	{
		if (wp->kind == WK_FILE)
		{
			FileWindow_t *fwp = wp->kind_info;
			
			if (IsPathPrefix (fwp->path, path))
			{
				wp->add_to_display_path (wp, NULL);
			}
		}
		
		wp = wp->nextwind;
	}

	if (icons_also)
		UpdateSpecialIcons ();
}

void PathUpdate (const char *path)
{
	pathUpdate (path, TRUE);
}

/*
 * align an object lying on the desktop so that it 
 * fits perfectly well in this area
 */
void deskAlign(OBJECT *deskobj)
{
	word zw,zh;
	
	if(deskobj->ob_x < 0)
		deskobj->ob_x  = 0;
	if(deskobj->ob_y < 0)
		deskobj->ob_y  = 0;
	
	zw = Desk.g_w - (deskobj->ob_x + deskobj->ob_width);
	zh = Desk.g_h - (deskobj->ob_y + deskobj->ob_height);
	
	if(zw < 0)
		deskobj->ob_x += zw;
	if(zh < 0)
		deskobj->ob_y += zh;
}


word GotBlitter(void)
{
	return Blitmode(-1) & 0x0002;
}

word SetBlitter(word mode)
{
	word flag;
	
	flag = Blitmode(-1);
	switch (mode)
	{
		case -1:
			return flag & 0x01;
		case 0:
			flag &= ~0x01;
			break;
		default:
			flag |= 0x01;
			break;
	}
	Blitmode(flag);
	return TRUE;
}

/*
 * word killEvents(word eventtypes, word maxtimes)
 * kill pending events like MU_MESAG or MU_KEYBD
 * at most maxtimes times, return times used
 */
word killEvents(word eventtypes, word maxtimes)
{
	MEVENT E;
	word times, events;
	word messbuff[8];
	
	if(eventtypes & (MU_TIMER|MU_M1|MU_M2|MU_BUTTON))
		return 0;			/* this events can't be pending */
	E.e_flags = (eventtypes|MU_TIMER);
	E.e_time = 0L;
	E.e_mepbuf = messbuff;
	times = 0;
		
	do
	{
		events = evnt_event(&E);
		times++;
		
	} while((events & eventtypes) && (times < maxtimes));
	
	return times;
}

void WaitKeyButton(void)
{
	MEVENT E;
	
	E.e_flags = (MU_BUTTON|MU_KEYBD);
	E.e_bclk = 2;
	E.e_bmsk = E.e_bst = 1;
	evnt_event(&E);
}


word ButtonPressed(word *mx, word *my, word *bstate, word *kstate)
{
	MEVENT E;
	word etype;
	
	E.e_flags = (MU_BUTTON|MU_TIMER);
	E.e_bclk = 2;
	E.e_bmsk = 1;
	E.e_bst = 1;
	E.e_time = 0L;
	etype = evnt_event(&E);
	*kstate = E.e_ks;
	*mx = E.e_mx;
	*my = E.e_my;
	if (etype & MU_BUTTON)
	{
		*bstate = E.e_mb;
		return TRUE;
	}
	else
	{
		*bstate = 0;
		return FALSE;
	}
}


int ButtonReleased (word *mx, word *my, word *bstate, word *kstate)
{
	MEVENT E;
	word etype;
	
	E.e_flags = (MU_BUTTON|MU_M1);
	E.e_bclk = 2;
	E.e_bmsk = 1;
	E.e_bst = 0;
	E.e_time = 0L;
	E.e_m1flags = 1;
	E.e_m1.g_x = *mx;
	E.e_m1.g_y = *my;
	E.e_m1.g_w = 1;
	E.e_m1.g_h = 1;
	etype = evnt_event(&E);
	*kstate = E.e_ks;
	*mx = E.e_mx;
	*my = E.e_my;
	
	if (etype & MU_BUTTON)
	{
		*bstate = E.e_mb;
		return (*bstate & 0x1) == 0;
	}
	else
	{
		*bstate = 0;
		return FALSE;
	}
}

/*
 * word escapeKeyPressed(void)
 * return, if the escape key was pressed
 */
word escapeKeyPressed(void)
{
	MEVENT E;
	word events;
	
	E.e_flags = (MU_TIMER|MU_KEYBD);
	E.e_time = 0L;
	
	events = evnt_event(&E);

	if(events & MU_KEYBD)
		return ((E.e_kr & 0xFF) == 0x1B);
	else
		return FALSE;
}

static word getSpecialDeskIcon(word type)
{
	IconInfo *pii;
	register word i;
	
	for (i = NewDesk.tree[0].ob_head; i > 0; 
		i = NewDesk.tree[i].ob_next)
	{
		if (isHidden (NewDesk.tree, i)
			|| ((NewDesk.tree[i].ob_type & 0x00ff) != G_USERDEF))
			continue;
			
		pii = GetIconInfo (&NewDesk.tree[i]);
		
		if (pii && (pii->type == type))
			return i;
	}
	
	return -1;		/* -1 is an illegal number */
}

/*
 * word getScrapIcon(void)
 * get objectnumber for scrapicon
 * or -1 if not found
 */
word getScrapIcon(void)
{
	return getSpecialDeskIcon(DI_SCRAPDIR);
}

/*
 * word getTrashIcon(void)
 * get objectnumber for scrapicon
 * or -1 if not found
 */
word getTrashIcon(void)
{
	return getSpecialDeskIcon(DI_TRASHCAN);
}

/*
 * for Scrap- and Trashicons: look if Dir is empty
 * return icon changed
 */
static int updateSpecialIcon (word iconNr)
{
	OBJECT *po;
	ICONBLK *ibp;
	IconInfo *pii;
	
	if (iconNr <= 0)
		return FALSE;
		
	po = &NewDesk.tree[iconNr];
	pii = GetIconInfo (po);
	if (!pii)
		return FALSE;
	if ((pii->type != DI_TRASHCAN) && (pii->type != DI_SCRAPDIR))
		return FALSE;

	if (NewDesk.scrapNr == iconNr)
	{
		pii->fullname = getSPath (pii->fullname);
		ReadLabel (pii->fullname[0], pii->label, MAX_FILENAME_LEN);
	}
	
	{
		int empty;
			
		empty =  isEmptyDir (pii->fullname);
		
		if (empty)
			po = GetDeskObject (pii->defnumber);
		else
			po = GetDeskObject (pii->altnumber);

		ibp = po->ob_spec.iconblk;

		if (pii->type == DI_SCRAPDIR)
			NewDesk.flags.scrap_empty = (empty != 0);
		else
			NewDesk.flags.trash_empty = (empty != 0);

		/* verschiedene Icons?
		 */
		if (pii->icon.icon.ib_pdata != ibp->ib_pdata) 
		{
			word (*getNumber) (void);
			void (*install) (word, word, const char *, word, char);
			word *pNumber;
			
			if (pii->type == DI_TRASHCAN)
			{
				pNumber = &NewDesk.trashNr;
				getNumber = getTrashIcon;
				install = InstTrashIcon;
			}
			else
			{
				pNumber = &NewDesk.scrapNr;
				getNumber = getScrapIcon;
				install = InstScrapIcon;
			}
			
			install (NewDesk.tree[iconNr].ob_x, 
				NewDesk.tree[iconNr].ob_y, pii->iconname, 
				pii->shortcut, pii->truecolor);
			redrawObj (&wnull, iconNr);
			RemoveDeskIcon (iconNr);
			*pNumber = getNumber ();
			redrawObj (&wnull, *pNumber);
			flushredraw ();

			return TRUE;
		}
	}
	
	return FALSE;
}


int UpdateSpecialIcons (void)
{
	int retcode;
	
	retcode = updateSpecialIcon (NewDesk.scrapNr);
	retcode |= updateSpecialIcon (NewDesk.trashNr);
	
	return retcode;
}

word BuffPathUpdate (const char *path)
{
	char *cp;
	word i;
	
	/*
	 * look if path is already stored
	 */
	for (i=0; i < pathStackTop; i++)
	{
		if (IsPathPrefix (path, pathStack[i]))
			return TRUE;
	}
	
	if (pathStackTop < MAXPATHS)
	{
		cp = StrDup (pMainAllocInfo, path);
		if (cp == NULL)
			return FALSE;
		
		pathStack[pathStackTop++] = cp;
		return TRUE;
	}
	else
		return FALSE;
}

void FlushPathUpdate (void)
{
	while (pathStackTop > 0)
	{
		char **cpp;
		
		cpp = &pathStack[--pathStackTop];
		if (*cpp != NULL)
		{
			pathUpdate (*cpp, (pathStackTop == 0));
			free (*cpp);
			*cpp = NULL;
		}
	}
}

/*
 * void setSelected(OBJECT *tree,word objnr, word flag)
 * set SELECTED Bit in tree[objnr].ob_state depending on flag
 */
void setSelected(OBJECT *tree, word objnr, word flag)
{
	if(flag)
		tree[objnr].ob_state |= SELECTED;
	else
		tree[objnr].ob_state &= ~SELECTED;
}

word isSelected(OBJECT *tree, word objnr)
{
	return tree[objnr].ob_state & SELECTED;
}

void setHideTree(OBJECT *tree, word objnr, word flag)
{
	if(flag)
		tree[objnr].ob_flags |= HIDETREE;
	else
		tree[objnr].ob_flags &= ~HIDETREE;
}

word isHidden(OBJECT *tree, word objnr)
{
	return tree[objnr].ob_flags & HIDETREE;
}

void setDisabled(OBJECT *tree, word objnr, word flag)
{
	if(flag)
		tree[objnr].ob_state |= DISABLED;
	else
		tree[objnr].ob_state &= ~DISABLED;
}

word isDisabled(OBJECT *tree, word objnr)
{
	return tree[objnr].ob_state & DISABLED;
}

void doFullGrowBox(OBJECT *tree,word objnr)
{
	word x,y,w,h;
	
	objc_offset(tree,objnr,&x,&y);
	w = tree[objnr].ob_width;
	h = tree[objnr].ob_height;
	
	graf_growbox (x, y, w, h, Desk.g_x, Desk.g_y, Desk.g_w, Desk.g_h);
}


static int changeOldPath (char **oldpath, const char *oldname,
	const char *newname)
{
	/* gleicher Anfang?
	 */
	if (IsPathPrefix (*oldpath, oldname))
	{
		char newpath[MAX_PATH_LEN], *new;
		
		strcpy (newpath, newname);
		strcat (newpath, (*oldpath) + strlen (oldname));

		new = StrDup (pMainAllocInfo, newpath);
		if (new == NULL)
			return FALSE;
		
		free (*oldpath);
		*oldpath = new;
		return TRUE;
	}
	
	return FALSE;
}

/*
 * look if moved file oldname is installed on the desktop and
 * change then its path to newname
 * return if change has happend
 */
int ObjectWasMoved (const char *oldname, const char *newname,
	int was_folder)
{
	WindInfo *wp;
	IconInfo *pii;
	word i, changed = FALSE;
	
	for (i = NewDesk.tree[0].ob_head; i > 0; 
		i = NewDesk.tree[i].ob_next)
	{
		if (isHidden (NewDesk.tree, i)
			|| ((NewDesk.tree[i].ob_type & 0x00ff) != G_USERDEF))
			continue;
			
		pii = GetIconInfo (&NewDesk.tree[i]);
		if (!pii)
			continue;
			
		switch (pii->type)
		{
			case DI_SCRAPDIR:
			case DI_PROGRAM:
			case DI_FOLDER:
			case DI_TRASHCAN:
				if (changeOldPath (&pii->fullname, oldname, newname))
				{
					/* neuen Pfad eintragen
					 */
					ReadLabel (newname[0], pii->label, MAX_FILENAME_LEN);

					if (pii->type == DI_SCRAPDIR)
						scrp_write ((char *)newname);
						
					changed = TRUE;
				}
				break;
		}
	}
	
	if (was_folder)
	{
		UndoFolderVanished (oldname);

		wp = wnull.nextwind;
		while (wp)
		{
			if (wp->kind == WK_FILE)
			{
				FileWindow_t *fwp = wp->kind_info;
				
				if (IsPathPrefix (fwp->path, oldname))
				{
					char tmp[MAX_PATH_LEN];
					
					strcpy (tmp, &fwp->path[strlen(oldname)]);
					strcpy (fwp->path, newname);
					strcat (fwp->path, tmp);
					strcpy (fwp->title, fwp->path);
					wp->update |= FW_PATH_CHANGED;
				}
			}
			wp = wp->nextwind;
		}
	}

	ApRenamed (oldname, newname, was_folder);

	return changed;
}

word countObjects(OBJECT *tree,word startobj)
{
	word currchild,anz = 1;
	
	if((currchild = tree[startobj].ob_head) > 0)
											/* Object hat Kinder */
	{
		while(currchild != startobj)		/* einmal rum */
		{
			anz += countObjects(tree,currchild);
			currchild = tree[currchild].ob_next;
		}
	}
	return anz;
}


/*
 * convert the filename name into a name that takes exactly
 * 11 characters and has no point ('venus.c' -> 'venus   c  ')
 */
void makeEditName (char *name)
{
	char *cs, *pext, mystr[MAX_FILENAME_LEN];
	size_t l;
	register word i;
	
	if (strlen (name) > 12)
		return;
		
	if ((cs = strchr(name,'.')) != NULL)	/* is there an extension? */
	{
		pext = cs + 1;
		*cs = '\0';
		strcpy (mystr, name);		/* get the name part */

		l = strlen (mystr);			/* fill the string with ' ' */
		for (i = 8; i > l; )
			mystr[--i] = ' ';
			
		mystr[8] = '\0';
		
		strcat (mystr, pext);		/* append the extension */
		strcpy (name, mystr);		/* give it back */
	}
}

/*
 * reverse the effect of makeEditName
 * ('venus   c  ' -> 'venus.c')
 */
void makeFullName (char *name)
{
#define NAME_PART		8
	char fname[MAX_FILENAME_LEN];
	char *read, *write;
	int has_extension;
	
	if (strlen (name) > 12)
		return;
		
	*fname = '\0';
	has_extension = (strlen (name) > NAME_PART);

	strncat (fname, name, NAME_PART);
	if (has_extension)
	{
		strcat (fname, ".");
		strcat (fname, &name[NAME_PART]);	
	}
	
	/* Nun l”sche evtl. vorhandene Leerzeichen
	 */
	read = write = fname;
	while (*read)
	{
		if (*read != ' ')
		{
			*write++ = *read;
		}
		
		++read;
	}
	*write = '\0';
	
	strcpy (name, fname);
}


/*
 * word checkPromille(word pm, word default)
 * check pm against [0,1000] and return default
 * if out if range, pm otherwise
 */
word checkPromille(word pm, word def)
{
	if(pm < 0 || pm > ScaleFactor)
		return def;
	
	return pm;
}

/*
 * search for the file named <fname> and return it's path
 * in <path>. function value is success.
 */
word getBootPath (char *path, const char *fname)
{
	strcpy (path, fname);
	if (shel_find (path))
	{
		if (strcmp (path, fname))
			StripFileName (path);
		else
			GetFullPath (path, MAX_PATH_LEN);
		return TRUE;
	}
	else
	{
		int broken;
		
		GetFullPath (path, MAX_PATH_LEN);
		AddFileName (path,fname);
		if (access (path, A_EXIST, &broken))
		{
			StripFileName (path);
			return TRUE;
		}
	}
	return FALSE;
}

void setBigClip(word handle)
{
	word pxy[4];
	
	pxy[0] = Desk.g_x;
	pxy[1] = Desk.g_y;
	pxy[2] = Desk.g_x + Desk.g_w - 1;
	pxy[3] = Desk.g_y + Desk.g_h - 1;
	vs_clip(handle,TRUE,pxy);
}

void remchr(char *str, char c)
{
	char *cp;
	
	while ((cp = strchr(str, c)) != NULL)
	{
		memmove(cp,cp+1,strlen(cp));
	}
}

word GotGEM14(void)
{
	static word version = 0;
	
	if (!version)
		version = _GemParBlk.global[0];

	return (version >= 0x140) && (version != 0x210);
}


static long getms (void)
{
	return *((unsigned long *)(0x4baL));
}

unsigned long GetHz200 (void)
{
	unsigned long time;
	
	time = Supexec (getms);
	
	return time;
}


char VDIRectIntersect (register word r1[4], register word r2[4])
{
	if ((r1[2] >= r2[0]) && (r1[0] <= r2[2]) 
		&& (r1[3] >= r2[1]) && (r1[1] <= r2[3]))
	{
		if (r1[0] > r2[0])
			r2[0] = r1[0];
		if (r1[2] < r2[2])
			r2[2] = r1[2];
		if (r1[1] > r2[1])
			r2[1] = r1[1];
		if (r1[3] < r2[3])
			r2[3] = r1[3];
			
		return TRUE;
	}
	else
		return FALSE;
}

/*
 * check if point px,py is in rectangle r
 */
int PointInGRECT (word px, word py, const GRECT *rect)
{
	px -= rect->g_x; py -= rect->g_y;
	return ((px >= 0) && (px < rect->g_w) 
		&& (py >= 0) && (py < rect->g_h));
}


int GRECTIntersect (const GRECT *g1, GRECT *g2)
{
	register word z1, z2;
	register word tx, ty, tw, th;
	
	z1 = g1->g_x + g1->g_w;
	z2 = g2->g_x + g2->g_w;
	tw = min (z1, z2);
	
	z1 = g1->g_y + g1->g_h;
	z2 = g2->g_y + g2->g_h;
	th = min (z1, z2);
	
	g2->g_x = tx = max (g1->g_x, g2->g_x);
	g2->g_y = ty = max (g1->g_y, g2->g_y);

	g2->g_w = tw - tx;
	g2->g_h = th - ty;

	return ((tw > tx) && (th > ty));
}


int SetFileSizeField (TEDINFO *ted, unsigned long size)
{
	char tmp[14];
	size_t numlen;
	
	sprintf (tmp, "%ld", size);
	numlen = strlen (tmp);
	if (numlen > 9)
	{
		strcpy (ted->te_ptext, "        NaN");
		return FALSE;
	}
	else
	{
		int i;
		char *cp;
		
		/* Flle das Textfeld von hinten mit den Zahlen
		 * auf. An der 4. und 7. Stelle (von hinten) muž
		 * wenn n”tig ein '.' eingesetzt werden.
		 */
		strcpy (ted->te_ptext, "           ");
		cp = &ted->te_ptext[10];

		for (i = 1; i <= numlen; --cp, ++i)
		{
			if ((i == 4) || (i == 7))
			{
				*cp = '.';
				--cp;
			}
			
			*cp = tmp[numlen - i];
		}
	}
	
	return TRUE;
}


/* jr: Textobjekt fr lange Dateinamen erzeugen. Die Templates
   etc im Resource-File mssen jeweils 32 Zeichen fassen!!!!!
   maxlength ist der Returncode von Dpathconf in Hinsicht auf
   die maximale L„nge.
   Liefert den korrigierten maxlen-Wert zurck */

long AdjustTextObjectWidth (OBJECT *object, long maxlength)
{
	TEDINFO *te = object->ob_spec.tedinfo;

	if (maxlength < 0) maxlength = 32;

	te->te_txtlen = te->te_tmplen = 33;
	if (te->te_tmplen > maxlength + 1)
		te->te_txtlen = te->te_tmplen = (int) maxlength + 1;

	memset (te->te_pvalid, 'X', te->te_tmplen - 1);
	te->te_pvalid[te->te_tmplen - 1] = '\0';
	memset (te->te_ptmplt, '_', te->te_tmplen - 1);
	te->te_ptmplt[te->te_tmplen - 1] = '\0';
	
	return te->te_txtlen - 1;
}

/* jr: w„hle eines von zwei Textobjekten aus, disable das andere */

void SelectFrom2EditFields (OBJECT *tree, int one, int two, int which)
{
	int sel = which ? one : two;
	int nosel = !which ? one : two;

	tree[nosel].ob_flags |= HIDETREE;
	tree[nosel].ob_flags &= ~EDITABLE;
	tree[sel].ob_flags &= ~HIDETREE;
	tree[sel].ob_flags |= EDITABLE;
}
