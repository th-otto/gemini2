/*
 * @(#) Gemini\drives.c
 * @(#) Stefan Eissing, 17. Juli 1994
 *
 * description: Installation von Laufwerksicons
 *
 * jr 26.7.94
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <flydial\flydial.h>
#include <flydial\listman.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"

#include "vs.h"
#include "instbox.rh"
#include "drivesel.rh"

#include "window.h"
#include "color.h"
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
#include "timeevnt.h"
#include "util.h"
#include "venuserr.h"


/* externals
 */
extern OBJECT *pinstbox, *pdrivesel;

#define SHOW_NEVER	0x01
#define SHOW_ALWAYS	0x02
#define SHOW_AVAIL	0x03

typedef struct
{
	char name[MAX_FILENAME_LEN];
	char show_mode;
	word defnumber;					/* Nummer des default Icons */
	word altnumber;					/* Nummer eines altern. Icons */
	word shortcut;					/* shortcut fÅr das Icon */
	char color;						/* gewÅnschte Iconfarbe */
	int x, y;
} DriveInfo;

static unsigned long last_drive_map;
static DriveInfo Drives[] = 
{
	{ "FLOPPY", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "FLOPPY", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
	{ "HARDDISK", SHOW_AVAIL, 6, 6, 0, 0x10},
};


void ExecDriveInfo (char *line)
{
	static int maxDeskIcon = -1;
	static int minDeskIcon = -1;
	word obx, oby, iconNr;
	word shortcut = 0;
	char color = 0x10;
	char *cp;
	int drive;
	
	if (maxDeskIcon < 0)
		GetDeskIconMinMax (&minDeskIcon, &maxDeskIcon);
		
	strtok (line, "@\n");		/* skip first */
	
	if ((cp = strtok (NULL, "@\n")) != NULL)
	{
		obx = atoi (cp);
		if ((cp = strtok (NULL, "@\n")) != NULL)
		{
			oby = atoi (cp);
			if ((cp = strtok (NULL, "@\n")) != NULL)
			{
				/* check ob iconNr gÅltig ist und nimm sonst das
				 * das Default-Laufwerksicon.
				 */
				iconNr = atoi (cp);
				if (iconNr < minDeskIcon || iconNr > maxDeskIcon)
					iconNr = minDeskIcon;

				if ((cp = strtok (NULL,"@\n")) != NULL)
				{
					drive = toupper(*cp) - 'A';
					if (drive > 25)
						drive = 0;
						
					if (((cp = strtok (NULL, "@\n")) != NULL)
						&& strcmp (cp, " "))
						strcpy (Drives[drive].name, cp);
					else
						Drives[drive].name[0] = '\0';
						
					if ((cp = strtok (NULL, "@\n")) != NULL)
						shortcut = atoi (cp);

					if ((cp = strtok (NULL, "@\n")) != NULL)
						color = (char)atoi (cp);

					if ((cp = strtok (NULL, "@\n")) != NULL)
						Drives[drive].show_mode = atoi (cp);
						
					obx = checkPromille (obx, 0);
					oby = checkPromille (oby, 0);
					obx = (word)scale123 (Desk.g_w, obx, ScaleFactor);
					oby = (word)scale123 (Desk.g_h, oby, ScaleFactor);

					Drives[drive].defnumber = iconNr;
					Drives[drive].x = obx;
					Drives[drive].y = oby;
					Drives[drive].shortcut = shortcut;
					Drives[drive].color = color;
				}
			}
		}
	}
}


int WriteDriveInfos (MEMFILEINFO *mp, char *buffer)
{
	int i, ok = TRUE;
	
	for (i = 0; ok && (i < DIM(Drives)); ++i)
	{
		word obx, oby;
		
		obx = (word)scale123 (ScaleFactor, Drives[i].x, Desk.g_w);
		oby = (word)scale123 (ScaleFactor, Drives[i].y, Desk.g_h);

		sprintf (buffer, "#D@%d@%d@%d@%c@%s@%d@%d@%d",
			obx, oby, Drives[i].defnumber, i + 'A',
			Drives[i].name[0]?  Drives[i].name : " ",
			Drives[i].shortcut, Drives[i].color,
			Drives[i].show_mode);

		ok = (mputs (mp, buffer) == 0);
	}
	
	return ok;
}


/*
 * fill IconInfo structure, type will be FILESYSTEM
 */ 
static void setDriveIcon (IconInfo *pii, word iconNr, char drive,
					 const char *name, word shortcut, OBJECT **ppo,
					 char truecolor)
{
	OBJECT *po;
	
	pii->type = DI_FILESYSTEM;
	pii->defnumber = iconNr;
	pii->altnumber = 0;
	pii->shortcut = shortcut;
	SCutInstall (pii->shortcut);
	pii->fullname[0] = drive;
	pii->fullname[1] = ':';
	pii->fullname[2] = '\\';
	pii->fullname[3] = '\0';
	pii->label[0] = '\0';			/* Label ist egal */
	
	po = GetDeskObject (pii->defnumber);
	
	InitIconInfo (pii, po, name, truecolor);
	
	pii->icon.icon.ib_char = 
		(pii->icon.icon.ib_char & 0xFF00) | drive;
	
	*ppo = po;	/* gib das Object zur Installation zurÅck */
}

/*
 * install icon for a drive on desktop
 */
void InstDriveIcon (word obx, word oby, word todraw, word iconNr,
					char drive, const char *name, word shortcut,
					char truecolor)
{
	IconInfo *pi;
	OBJECT *po;
	
	if ((pi = malloc (sizeof (IconInfo))) != NULL)
	{
		pi->magic = ICON_MAGIC;
		pi->fullname = malloc (4);
		if (pi->fullname == NULL)
		{
			free (pi);
			return;
		}
		setDriveIcon (pi, iconNr, drive, name, shortcut, &po,
						truecolor);

		AddDeskIcon (obx, oby, todraw, po, &pi->user);
	}
	
	if (todraw)
		flushredraw ();
}


/*
 * display icon in listdialog
 */
static void drawDeskIcon (LISTSPEC *l, word x, word y, word offset,
				GRECT *clip, word how)
{
	word nr;
	PARMBLK param;
	ICON_OBJECT icon;
	OBJECT *po;
	word len, xy[8];
	
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

	memset (&param, 0, sizeof (param));
	param.pb_xc = clip->g_x;
	param.pb_yc = clip->g_y;
	param.pb_wc = clip->g_w;
	param.pb_hc = clip->g_h;
	param.pb_parm = (long)&icon;

	po = GetDeskObject (nr);
	FillIconObject (&icon, po);
	SetIconTextWidth (&icon.icon, po);
	
	param.pb_w = po->ob_width;
	param.pb_h = po->ob_height;

	len = (pinstbox[INSTSPEC].ob_width - po->ob_width) / 2;
	param.pb_x = x + offset + len;
	len = (pinstbox[INSTSPEC].ob_height - po->ob_height) / 2;
	param.pb_y = y + 1 + len;
	DrawIconObject (&param);
	
	vs_clip (vdi_handle, 0, xy);
}


static int selectDeskIcon (LISTSPEC *l, word selindex)
{
	int maxIconNr, minIconNr;
	int icon_count, i;
	
	if (l[selindex].flags.selected)
		return FALSE;

	GetDeskIconMinMax (&minIconNr, &maxIconNr);
	icon_count = maxIconNr - minIconNr + 1;

	if (selindex >= icon_count || selindex < 0)
		selindex = 0;
		
	if (l[selindex].flags.selected)
		return FALSE;

	for (i = 0; i < icon_count; ++i)
		memset (&l[i].flags, 0, sizeof (l[i].flags));
		
	if (selindex < icon_count)
		l[selindex].flags.selected = 1;
	
	return TRUE;
}

static LISTSPEC *buildDeskIconList (void)
{
	LISTSPEC *l;
	word i;
	int maxIconNr, minIconNr;
	int icon_count;
	
	GetDeskIconMinMax (&minIconNr, &maxIconNr);
	icon_count = maxIconNr - minIconNr + 1;
	
	l = calloc (icon_count, sizeof (LISTSPEC));
	if (!l)
		return NULL;
	
	for (i = 0; i < icon_count; ++i)
	{
		l[i].next = &l[i+1];
		l[i].entry = (void *)(i + minIconNr);
	}

	l[icon_count - 1].next = NULL;
	
	return l;
}


word DriveSelect (OBJECT *tree, word index, word drive, word cycle_obj)
{
	word retcode;

	if (cycle_obj > 0)
		retcode = JazzCycle (tree, index, cycle_obj, pdrivesel, 1);
	else
		retcode = JazzId (tree, index, -1, pdrivesel, 1);
	
	if (retcode > -1)
		return retcode - 2;
	else
		return drive;
}


static int setupDriveInfoInDialog (DriveInfo *actDrive, int drive,
	LISTSPEC *list, int *origscut)
{
	memcpy (actDrive, &Drives[drive], sizeof (DriveInfo));
	*origscut = actDrive->shortcut;
	pinstbox[INSTSC].ob_spec = SCut2Obspec (actDrive->shortcut);
	ColorSetup (actDrive->color, pinstbox, INSTFORE, INSTFBOX,
		INSTBACK, INSTBBOX);
	pinstbox[INDRVSC].ob_spec = pdrivesel[drive + 2].ob_spec;
	pinstbox[INSTNAME].ob_spec.tedinfo->te_ptext = actDrive->name;
	
	setSelected (pinstbox, INSTNEVE, actDrive->show_mode == SHOW_NEVER);
	setSelected (pinstbox, INSTALWA, actDrive->show_mode == SHOW_ALWAYS);
	setSelected (pinstbox, INSTAVAI, actDrive->show_mode == SHOW_AVAIL);

	return selectDeskIcon (list, actDrive->defnumber - 6);
}


/* dialog for installing drive icons
 */
void DriveInstDialog (const char *drivePath)
{
	DIALINFO d;
	LISTINFO L;
	LISTSPEC *list;
	word drive;
	DriveInfo tmpDrive;
	DriveInfo *actDrive = &tmpDrive;
	word retcode, clicks, origscut;
	long listresult;
	word draw, exit, circle;
	int edit_object = INSTBG;
	
	drive = 0;
	if (drivePath != NULL)
	{
		drive = toupper (drivePath[0]) - 'A';
		if (drive < 0 || drive >= DIM(Drives))
			drive = 0;
	}

	DialCenter (pinstbox);

	list = buildDeskIconList ();
	if (!list)
		return;
	
	setupDriveInfoInDialog (actDrive, drive, list, &origscut);
	ListStdInit (&L, pinstbox, INSTB, INSTBG, drawDeskIcon, 
		list, 0, actDrive->defnumber - 6, 1);
	ListInit (&L);

	DialStart (pinstbox, &d);
	DialDraw (&d);
	ListDraw (&L);

	/* jr: Support fÅr Tastaturgesteuerte Liste */
	
	exit = FALSE;
	do
	{
		word key, shift_state;
	
		circle = draw = FALSE;
		
		retcode = DialXDo (&d, &edit_object, &shift_state, &key, NULL, NULL, NULL);
		clicks = (retcode & 0x8000)? 2 : 1;
		retcode &= 0x7FFF;
	
		switch (retcode)
		{
			case INSTB:
			case INSTBG:
				edit_object = INSTBG;
				
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
					actDrive->defnumber = (int)(listresult + 6L);
					exit = (clicks == 2);
					if (exit)
						retcode = INSTOK;
				}
				break;

			case INSTSC0:
				circle = INSTSC0;
			case INSTSCSC:
				if (shift_state & 3)
					circle = INSTSC0;
			case INSTSC:
			{
				word retcut = SCutSelect (pinstbox, INSTSC, 
						actDrive->shortcut, origscut, circle);
				if (retcut != actDrive->shortcut)
				{
					actDrive->shortcut = retcut;
					pinstbox[INSTSC].ob_spec = 
							SCut2Obspec (actDrive->shortcut);
					fulldraw (pinstbox, INSTSC);
				}
				break;
			}

			case INSTFR0:
				circle = INSTFR0;
			case INSTFGSC:
				if (shift_state & 3)
					circle = INSTFR0;
			case INSTFORE:
				actDrive->color = ColorSelect (actDrive->color, 
							TRUE, pinstbox,
				 			INSTFORE, INSTFBOX, circle);
				 break;

			case INSTBK0:
				circle = INSTBK0;
			case INSTBGSC:
				if (shift_state & 3)
					circle = INSTBK0;
			case INSTBACK:
				actDrive->color = ColorSelect (actDrive->color, 
							FALSE, pinstbox,
				 			INSTBACK, INSTBBOX, circle);
				break;

			case INDRVSC0:
				circle = INDRVSC0;
			case INDRSCSC:
				if (shift_state & 3)
					circle = INDRVSC0;
			case INDRVSC:
			{
				word newdrive;
				
				newdrive = DriveSelect (pinstbox, INDRVSC, drive,
							circle);
				if (newdrive != drive)
				{
					if (actDrive->shortcut != origscut)
						SCutRemove (origscut);
			
					if (isSelected (pinstbox, INSTNEVE))
						actDrive->show_mode = SHOW_NEVER;
					else if (isSelected (pinstbox, INSTALWA))
						actDrive->show_mode = SHOW_ALWAYS;
					else
						actDrive->show_mode = SHOW_AVAIL;
					memcpy (&Drives[drive], actDrive, sizeof (DriveInfo));

					drive = newdrive;
					pinstbox[INDRVSC].ob_spec = 
						pdrivesel[drive + 2].ob_spec;
		
					if (setupDriveInfoInDialog (actDrive, drive, 
						list, &origscut))
					{
						ListExit (&L);
						ListInit (&L);
						ListScroll2Selection (&L);
						ListDraw (&L);
					}

					fulldraw (pinstbox, INDRVSC);
					fulldraw (pinstbox, INSTFGSC);
					fulldraw (pinstbox, INSTBGSC);
					fulldraw (pinstbox, INSTSC);
					fulldraw (pinstbox, INSTNAME);
					fulldraw (pinstbox, INSHOWB);
				}
				
				break;
			}
					
			default:
				draw = exit = TRUE;
				break;
		}

		if (draw)
		{				
			setSelected (pinstbox, retcode, FALSE);
			fulldraw (pinstbox, retcode);
		}

	} 
	while (!exit);

	ListExit (&L);
	free (list);
	DialEnd (&d);
	
	if (retcode == INSTOK)
	{
		if (actDrive->shortcut != origscut)
			SCutRemove (origscut);
			
		if (isSelected (pinstbox, INSTNEVE))
			actDrive->show_mode = SHOW_NEVER;
		else if (isSelected (pinstbox, INSTALWA))
			actDrive->show_mode = SHOW_ALWAYS;
		else
			actDrive->show_mode = SHOW_AVAIL;
		memcpy (&Drives[drive], actDrive, sizeof (DriveInfo));
		UpdateDrives (TRUE, TRUE);
	}
}

static int automaticUpdate (long v)
{
	(void)v;
	UpdateDrives (FALSE, TRUE);
	
	return TRUE;
}

#include <time.h>

void UpdateDrives (int structure_changed, int redraw)
{	
	static automatic_installed = 0;
	unsigned long act_drives;
	int i, checkOnlyAccessibleDrives;
	
	if (!automatic_installed)
	{
		InstallTimerEvent (2500L, 0L, automaticUpdate);
		automatic_installed = 1;
	}
	
	act_drives = AccessibleDrives ();

/* printf ("%ld\n", time (NULL)); */

	checkOnlyAccessibleDrives = (!structure_changed 
		&& (act_drives == last_drive_map));
	
	for (i = 0; i < DIM(Drives); ++i)
	{
		int obj, visible;
		int was_accessible = 
				((last_drive_map & DriveBit (i+'A')) != 0);
		int is_accessible = 
				((act_drives & DriveBit  (i+'A')) != 0);
		int found = FALSE;

		if (checkOnlyAccessibleDrives && !is_accessible)
			continue;
		
		if (!structure_changed && (is_accessible == was_accessible))
			continue;
			
		if (Drives[i].show_mode == SHOW_NEVER)
			visible = FALSE;
		else if (Drives[i].show_mode == SHOW_ALWAYS)
			visible = TRUE;
		else
			visible = is_accessible;

		for (obj = NewDesk.tree[0].ob_head; obj > 0; 
			obj = NewDesk.tree[obj].ob_next) 
		{
			OBJECT *po = &NewDesk.tree[obj];
			IconInfo *pii;
		
			if (isHidden (NewDesk.tree, obj)
				|| ((NewDesk.tree[obj].ob_type & 0x00ff) != G_USERDEF))
				continue;
			
			pii = GetIconInfo (po);
			if (!pii || (pii->type != DI_FILESYSTEM)
				|| (pii->fullname[0] != ('A'+i)))
				continue;
				
			found = TRUE;
			
			Drives[i].x = NewDesk.tree[obj].ob_x;
			Drives[i].y = NewDesk.tree[obj].ob_y;

			if (!visible)
			{
				if (redraw)
					redrawObj (&wnull, obj);
				RemoveDeskIcon (obj);
				continue;
			}
			
			if (structure_changed)
			{
				/* versuche es zu installieren
				 */
				if (redraw)
					redrawObj (&wnull, obj);
	
				setDriveIcon (pii, Drives[i].defnumber, i + 'A', 
					Drives[i].name, Drives[i].shortcut, &po, 
					Drives[i].color);
				SetIconTextWidth (&pii->icon.icon, po);
				NewDesk.tree[obj].ob_x += 
					(NewDesk.tree[obj].ob_width - po->ob_width) / 2;
				NewDesk.tree[obj].ob_y += 
					(NewDesk.tree[obj].ob_height - po->ob_height) / 2;
				NewDesk.tree[obj].ob_width = po->ob_width;
				NewDesk.tree[obj].ob_height = po->ob_height;
				SetIconTextWidth (&pii->icon.icon, &NewDesk.tree[obj]);
				deskAlign (&NewDesk.tree[obj]);
				if (NewDesk.snapAlways)
					SnapObject (&wnull, obj, redraw);
			
				if (redraw)
					redrawObj (&wnull, obj);
			}
		}
		
		if (!found && visible)
		{
			/* installiere das Laufwerksicon
			 */
			InstDriveIcon (Drives[i].x, Drives[i].y, redraw, 
				Drives[i].defnumber, i + 'A', Drives[i].name, 
				Drives[i].shortcut, Drives[i].color);
		}
	}
	
	if (redraw)
		flushredraw ();

	last_drive_map = act_drives;
}

void RegisterDrivePosition (char drive, word x, word y)
{
	int nr = toupper (drive) - 'A';
	
	if ((nr < 0) || (nr > DIM (Drives)))
		return;
	
	Drives[nr].x = x;
	Drives[nr].y = y;
}

void DriveWasRemoved (char drive)
{
	int drv = toupper (drive) - 'A';
	
	if ((drv < 0) || (drv > DIM(Drives)))
		return;
	
	Drives[drv].show_mode = SHOW_NEVER;
}

void DriveHasNewShortcut (char drive, int shortcut)
{
	int drv = toupper (drive) - 'A';
	
	if ((drv < 0) || (drv > DIM(Drives)))
		return;
	
	Drives[drv].shortcut = shortcut;
}

void DriveHasNewName (char drive, const char *name)
{
	int drv = toupper (drive) - 'A';
	
	if ((drv < 0) || (drv > DIM(Drives)))
		return;
	
	strncpy (Drives[drv].name, name, sizeof (Drives[drv].name));
	Drives[drv].name[sizeof(Drives[drv].name)-1] = '\0';
}


