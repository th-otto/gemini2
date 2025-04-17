/*
 * @(#) Gemini\Getinfo.c
 * @(#) Stefan Eissing, 13. November 1994
 *
 * description: handles dialogue for the 'get Info...' option
 * 				in venus
 *
 * jr 14.11.95
 */

#include <stdio.h>
#include <string.h>
#include <flydial\flydial.h>
#include <mint\mintbind.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "..\common\strerror.h"

#include "vs.h"

#include "drivinfo.rh"
#include "drivinfo.rh"
#include "fileinfo.rh"
#include "foldinfo.rh"
#include "iconfile.rh"
#include "specinfo.rh"
#include "totalinf.rh"


#include "window.h"
#include "applmana.h"
#include "cpintern.h"
#include "color.h"
#include "drives.h"
#include "filewind.h"
#include "fileutil.h"
#include "getinfo.h"
#include "icondraw.h"
#include "iconinst.h"
#include "iconrule.h"
#include "redraw.h"
#include "select.h"
#include "scut.h"
#include "stand.h"
#include "util.h"
#include "venuserr.h"
#include "..\mupfel\mupfel.h"


/* externals
 */
extern OBJECT *pfileinfo, *pfoldinfo, *pdrivinfo;
extern OBJECT *piconfile, *pspecinfo, *ptotalinf;

/* internal texts
 */
#define NlsLocalSection "G.getinfo"
enum NlsLocalText{
T_FILENAME,		/*Sie mÅssen der Datei einen Namen geben!*/
T_ATTRIB,		/*Kann Dateiattribute nicht Ñndern (%s)!*/
T_FOLDER,		/*Sie mÅssen dem Ordner einen Namen geben!*/
T_DISKERR,		/*Kann keine vernÅnftigen Daten von dem Laufwerk 
lesen! Bitte ÅberprÅfen Sie das Medium!*/
T_FOLDABOVE,	/*Dieses Objekt reprÑsentiert den Ordner, der 
Åber dem gerade aktiven Ordner liegt.*/
T_RENAME,		/*Kann die Datei nicht in `%s' umbenennen (%s)!*/
T_ILLDRIVE,		/*Das Laufwerk %c: ist GEMDOS nicht bekannt!*/
T_ILLNAME,		/*Der Name `%s' ist nicht gÅltig, da er die 
Zeichen *?: nicht enthalten darf!*/
FILE_COUNT_ERR,	/*Fehler beim Lesen der Datei-Informationen!*/
};


/* jr: given a filename, return the set of TOS attributes
   supported by the file system */
   
static int validAttributes (const char *filename)
{
	int attr = FA_RDONLY|FA_HIDDEN|FA_SYSTEM|FA_LABEL|FA_FOLDER|FA_ARCHIV;
	
	if (DP_MODEATTR <= Dpathconf ((char *)filename, -1))
		attr = (int) (0xff & Dpathconf ((char *)filename, DP_MODEATTR));

	return attr;
}


static void installNewIconText (IconInfo *pii, word objnr,
	const char *new_name)
{
	redrawObj(&wnull, objnr);
	strcpy(pii->iconname, new_name);
	SetIconTextWidth (&pii->icon.icon, &NewDesk.tree[objnr]);
	deskAlign (&NewDesk.tree[objnr]);
	redrawObj(&wnull, objnr);
	flushredraw();
}

typedef struct
{
	USERBLK user;
	ICON_OBJECT icon;
} ICON_EXCHANGE;


/* Macht das Objekt <po> zu einem Userdef, in dem ein Icon
 * dargestellt wird. Falls <pib> != NULL ist, wird das Icon
 * genommen. Ansonsten wird mit <name> und <is_folder> ein Icon
 * aus den Regeln gesucht.
 * <ex_info> muû vom Aufrufer mitgegeben werden und enthÑlt die
 * nîtigen Informationen fÅr den USERDEF.
 */
static void makeObjectToIcon (OBJECT *po, char *name, int is_folder, 
	ICON_OBJECT *pio, ICON_EXCHANGE *ex_info)
{
	char color;
	OBJECT *picon = NULL;
	ICON_OBJECT icon;

	memset (ex_info, 0, sizeof (ICON_EXCHANGE));
	ex_info->user.ub_code = DrawIconObject;
	ex_info->user.ub_parm = (long)&ex_info->icon;
	po->ob_type = G_USERDEF;
	po->ob_spec.userblk = &ex_info->user;
	
	if (pio == NULL)
	{	
		picon = GetBigIconObject (is_folder, name, &color);
		if (picon->ob_spec.iconblk->ib_hicon > po->ob_height)
			picon = GetSmallIconObject (is_folder, name, &color);
		
		pio = &icon;
		FillIconObject (pio, picon);
	}
	
	memcpy (&ex_info->icon, pio, sizeof (ex_info->icon));
	ex_info->icon.icon.ib_ptext = NULL;
	if (picon != NULL)
	{
		ex_info->icon.icon.ib_char = (ex_info->icon.icon.ib_char 
			& 0xFF)	| (ColorMap (color) << 8);
	}
	ex_info->icon.flags.is_icon = 1;
		
	ex_info->icon.icon.ib_xicon = 
		(po->ob_width - ex_info->icon.icon.ib_wicon) / 2;
	ex_info->icon.icon.ib_yicon = 
		(po->ob_height - ex_info->icon.icon.ib_hicon) / 2;
}


/* information about a file shown in a dialog */

static int
infoFileDialog (const char *path, FileInfo *pf, WindInfo *wp)
{
	DIALINFO d;
	word retcode, ok, update = FALSE;
	word gemdosNaming;
	char *pfliname, *pflidate, *pflitime;
	char tmpstr[MAX_PATH_LEN], tmp2[MAX_PATH_LEN];
	char oldname[MAX_FILENAME_LEN];
	ICON_EXCHANGE ex_info;
	
	makeObjectToIcon (&pfileinfo[FLIICON], pf->name, FALSE,
		NULL, &ex_info);

	pflidate = pfileinfo[FLIDATE].ob_spec.tedinfo->te_ptext;
	pflitime = pfileinfo[FLITIME].ob_spec.tedinfo->te_ptext;

	strcpy (tmpstr, pf->name);

	/* jr: initialize text fields */
	{
		long ret = Dpathconf ((char *)path, DP_TRUNC);

		gemdosNaming = ret < 0 || ret == 2;
		
		SelectFrom2EditFields (pfileinfo, FLINAME, FLINAME2,
			gemdosNaming);

		if (gemdosNaming)
		{
			pfliname = pfileinfo[FLINAME].ob_spec.tedinfo->te_ptext;
			makeEditName (tmpstr);
			strcpy (pfliname, tmpstr);
		}
		else
		{
			long maxlen = Dpathconf ((char *)path, DP_NAMEMAX);
			char *cp = strrchr (tmpstr, '\\');
			if (!cp) cp = strrchr (tmpstr, ':');
			if (!cp) cp = tmpstr;
			
			maxlen = AdjustTextObjectWidth (pfileinfo + FLINAME2, maxlen);

			pfliname = pfileinfo[FLINAME2].ob_spec.tedinfo->te_ptext;
			strncpy (pfliname, cp, maxlen);
			pfliname[maxlen] = '\0';
		}
	}		

	strcpy (oldname, pfliname);		/* alten Namen merken */

	SetFileSizeField (pfileinfo[FLISIZE].ob_spec.tedinfo, pf->size);
	dateString (pflidate, pf->date);
	timeString (pflitime, pf->time);

	setSelected (pfileinfo, FILEREON, pf->attrib & FA_RDONLY);
	setSelected (pfileinfo, FILEHIDD, pf->attrib & FA_HIDDEN);
	setSelected (pfileinfo, FILESYST, pf->attrib & FA_SYSTEM);
	setSelected (pfileinfo, FILEARCH, pf->attrib & FA_ARCHIV);

	/* jr: inquire valid attribs */
	{
		int attribs = validAttributes (path) ^ 0xffff;

		setDisabled (pfileinfo, FILEREON, attribs & FA_RDONLY);
		setDisabled (pfileinfo, FILEHIDD, attribs & FA_HIDDEN);
		setDisabled (pfileinfo, FILESYST, attribs & FA_SYSTEM);
		setDisabled (pfileinfo, FILEARCH, attribs & FA_ARCHIV);
	}

	DialCenter (pfileinfo);
	DialStart (pfileinfo, &d);
	DialDraw (&d);
	
	do
	{
		retcode = DialDo (&d, 0) & 0x7FFF;
		setSelected (pfileinfo, retcode, FALSE);
		fulldraw (pfileinfo, retcode);

		ok = strlen (pfliname) > 0 || retcode != FLIOK;

		if (!ok)
		{
			venusErr (NlsStr (T_FILENAME));
			ok = FALSE;
		}			
		else if (retcode == FLIOK)
		{
			char name[MAX_FILENAME_LEN];
			
			strcpy (name, pfliname);
			if (gemdosNaming) makeFullName (name);
			if (!ValidFileName (name))
			{
				ok = FALSE;
				venusErr (NlsStr (T_ILLNAME), name);
			}
		}

	} while (!ok);

	DialEnd (&d);
	
	if (retcode == FLIOK)
	{
		word newattrib = pf->attrib;
		
		if (isSelected (pfileinfo, FILEREON))
			newattrib |= FA_RDONLY;
		else
			newattrib &= ~FA_RDONLY;

		if (isSelected (pfileinfo, FILEHIDD))
			newattrib |= FA_HIDDEN;
		else
			newattrib &= ~FA_HIDDEN;

		if (isSelected (pfileinfo, FILESYST))
			newattrib |= FA_SYSTEM;
		else
			newattrib &= ~FA_SYSTEM;

		if (isSelected (pfileinfo, FILEARCH))
			newattrib |= FA_ARCHIV;
		else
			newattrib &= ~FA_ARCHIV;
			
		if (strcmp (oldname, pfliname))	/* Name verÑndert */
		{
			long err;
			word change = FALSE;
			
			GrafMouse (HOURGLASS, NULL);
			strcpy (oldname, pfliname);
			if (gemdosNaming) makeFullName (oldname);

			strcpy (tmpstr, path);
			AddFileName (tmpstr, oldname);
			strcpy (tmp2, path);
			AddFileName (tmp2, pf->name);
			
			if (pf->attrib & FA_RDONLY)
			{
				/* Schreibschutz aus */
				pf->attrib &= ~FA_RDONLY;
				change = TRUE;
			}

			if ((change && ((err = setFileAttribut (tmp2, pf->attrib)) < 0))
				|| ((err = fileRename (tmp2, tmpstr)) != E_OK))
			{
				venusErr (NlsStr (T_RENAME), tmpstr, StrError (err));
				strcpy (oldname, pf->name);
			}
			else
			{
				FileWasMoved (tmp2, tmpstr);
				RehashDeskIcon ();
			}

			update = TRUE;
		}
		else
			strcpy (oldname, pf->name);

		if (pf->attrib != newattrib)
		{
			long err;
		
			update = TRUE;
			GrafMouse (HOURGLASS, NULL);
			strcpy (tmpstr, path);
			AddFileName (tmpstr, oldname);
			err = setFileAttribut (tmpstr, newattrib);
			
			if (err < E_OK)
				venusErr (NlsStr (T_ATTRIB), StrError ((int) err));
			else
				pf->attrib = newattrib;
		}

		if (update)
		{
			BuffPathUpdate (path);
			if (wp && wp->kind == WK_FILE && wp->selectAnz == 1)
			{
				FlushPathUpdate ();
				StringSelect (wp, oldname, TRUE);
				retcode = !FLIOK;
			}
		}
	}
	
	GrafMouse (ARROW, NULL);
	
	return retcode == FLIOK;
}

/* information about a folder shown in a dialog */

static int
infoFolderDialog (const char *path, FileInfo *pf, WindInfo *wp)
{
	DIALINFO d;
	uword fileanz, foldanz;
	word retcode, ok;
	long size, ret;
	int gemdosNaming;
	char *pfdiname, *pfdifilenr;
	char *pfditime, *pfdidate, *pfdifoldnr;
	char tmpstr[MAX_PATH_LEN], tmp2[MAX_PATH_LEN], oldname[MAX_FILENAME_LEN];
	ICON_EXCHANGE ex_info;
	
	makeObjectToIcon (&pfoldinfo[FDIICON], pf->name, TRUE,
		NULL, &ex_info);
		
	if (GEMDOSVersion < 0x15)
	{
		pfoldinfo[FDINAME].ob_flags &= ~EDITABLE;
		setHideTree(pfoldinfo, FDICANC, TRUE);
	}
		 
	ret = Dpathconf ((char *)path, DP_TRUNC);
	gemdosNaming = (ret < 0 || ret == 2);
	strcpy (tmpstr, pf->name);
		
	/* se: initialize text fields */
	SelectFrom2EditFields (pfoldinfo, FDINAME, FDINAME2,
		gemdosNaming);
		
	if (gemdosNaming)
	{
		pfdiname = pfoldinfo[FDINAME].ob_spec.tedinfo->te_ptext;
		makeEditName (tmpstr);
		strcpy (pfdiname, tmpstr);
	}
	else
	{
		long maxlen = Dpathconf ((char *)path, DP_NAMEMAX);
		char *cp = strrchr (tmpstr, '\\');
		if (!cp) cp = strrchr (tmpstr, ':');
		if (!cp) cp = tmpstr;
		
		maxlen = AdjustTextObjectWidth (pfoldinfo + FDINAME2, maxlen);

		pfdiname = pfoldinfo[FDINAME2].ob_spec.tedinfo->te_ptext;
		strncpy (pfdiname, cp, maxlen);
		pfdiname[maxlen] = '\0';
	}
	
	pfdifilenr = pfoldinfo[FDIFILNR].ob_spec.tedinfo->te_ptext;
	pfdifoldnr = pfoldinfo[FDIFOLNR].ob_spec.tedinfo->te_ptext;
	pfdidate = pfoldinfo[FDIDATE].ob_spec.tedinfo->te_ptext;
	pfditime = pfoldinfo[FDITIME].ob_spec.tedinfo->te_ptext;
	
	GrafMouse(HOURGLASS, NULL);
	size = foldanz = fileanz = 0;
	strcpy (tmpstr, path);
	AddFileName (tmpstr, pf->name);
	if (!AddFileAnz (tmpstr, FALSE, -1L, &foldanz, &fileanz, &size))
	{
		GrafMouse(ARROW, NULL);
		venusErr (NlsStr (FILE_COUNT_ERR));
		return FALSE;
	}
	sprintf(pfdifoldnr, "%5u", foldanz);
	sprintf(pfdifilenr, "%5u", fileanz);
	SetFileSizeField (pfoldinfo[FDISIZE].ob_spec.tedinfo, size);
	dateString(pfdidate, pf->date);
	timeString(pfditime, pf->time);

	strcpy(oldname, pfdiname);
	GrafMouse(ARROW, NULL);
	
	DialCenter(pfoldinfo);
	DialStart(pfoldinfo, &d);
	DialDraw(&d);
	
	do
	{
		retcode = DialDo (&d, 0) & 0x7FFF;
		setSelected (pfoldinfo, retcode, FALSE);
		fulldraw (pfoldinfo, retcode);
		ok = (strlen(pfdiname) != 0) || (retcode != FDIOK);
		if(!ok)
			venusErr (NlsStr (T_FOLDER));
		if (retcode == FDIOK)
		{
			char name[MAX_FILENAME_LEN];
			
			strcpy (name, pfdiname);
			makeFullName (name);
			if(!ValidFileName (name))
			{
				ok = FALSE;
				venusErr (NlsStr (T_ILLNAME), name);
			}
		}

	} while(!ok);

	DialEnd (&d);
	
	if (retcode == FDIOK)
	{
		if (strcmp (oldname, pfdiname))	/* Name verÑndert */
		{
			long err;
		
			GrafMouse (HOURGLASS, NULL);
			strcpy (oldname, pfdiname);
			if (gemdosNaming)
				makeFullName (oldname);
			strcpy (tmpstr, path);
			AddFileName (tmpstr, oldname);
			strcpy (tmp2, path);
			AddFileName (tmp2, pf->name);
			err = fileRename (tmp2, tmpstr);
			if (err < E_OK)
				venusErr (NlsStr (T_RENAME), oldname, StrError (err));
			else
			{
				FolderWasMoved (tmp2, tmpstr);

				BuffPathUpdate (path);
				if (wp && wp->kind == WK_FILE && wp->selectAnz == 1)
				{
					FlushPathUpdate ();
					StringSelect (wp, oldname, TRUE);
					retcode = !FDIOK;
				}

				RehashDeskIcon ();
			}

			GrafMouse(ARROW, NULL);
		}
	}
	
	return retcode == FDIOK;
}

/*
 * information about a Drive shown in a dialog
 */
static int infoDriveDialog (IconInfo *pii, word objnr)
{
	DIALINFO d;
	uword fileanz, foldanz;
	word retcode;
	word shortcut, cycle_obj;
	long size;
	long bytesfree, bytesused;
	char *pfdiname, *pfdifilenr;
	char *pdrivechar, *pfdifoldnr, *pdrvname;
	char tmpstr[MAX_PATH_LEN], oldlabel[MAX_FILENAME_LEN];
	int edit_object = DRVNAME, done;
	ICON_EXCHANGE ex_info;
	
	makeObjectToIcon (&pdrivinfo[DRVICON], NULL, FALSE,
		&pii->icon, &ex_info);

	pdrivechar = pdrivinfo[DRVDRIVE].ob_spec.tedinfo->te_ptext;
	pfdiname = pdrivinfo[DRVLABEL].ob_spec.tedinfo->te_ptext;
	pfdifilenr = pdrivinfo[DRVFILNR].ob_spec.tedinfo->te_ptext;
	pfdifoldnr = pdrivinfo[DRVFOLNR].ob_spec.tedinfo->te_ptext;
	pdrvname = pdrivinfo[DRVNAME].ob_spec.tedinfo->te_ptext;

	GrafMouse (HOURGLASS, NULL);
	strcpy (pdrvname, pii->iconname);
	size = foldanz = fileanz = 0;
	pdrivechar[0] = pii->fullname[0];
	pdrivechar[1] = '\0';

	if (!LegalDrive (pii->fullname[0]))
	{
		GrafMouse (ARROW, NULL);
		venusErr (NlsStr (T_ILLDRIVE), pii->fullname[0]);
		return FALSE;
	}

	/* xxx: das ist Schwachsinn */	
	if (E_OK != ReadLabel (pii->fullname[0], tmpstr, sizeof (tmpstr)))
		strcpy (tmpstr, "");
#if 0
		GrafMouse (ARROW, NULL);
		return FALSE;
#endif
	
	if (strlen (tmpstr))
	{
		makeEditName (tmpstr);
	}
	
	strcpy (pfdiname, tmpstr);
	strcpy (oldlabel, tmpstr);
	
	strcpy (tmpstr, pii->fullname);

	setHideTree (pdrivinfo, DRVFILNR, FALSE);
	setHideTree (pdrivinfo, DRVFOLNR, FALSE);

	if (!AddFileAnz (tmpstr, FALSE, -1L, &foldanz, &fileanz, &size))
	{
		size = foldanz = fileanz = 0;
		setHideTree (pdrivinfo, DRVFILNR, TRUE);
		setHideTree (pdrivinfo, DRVFOLNR, TRUE);
	}

	sprintf (pfdifoldnr, "%5u", foldanz);
	sprintf (pfdifilenr, "%5u", fileanz);
	
	if (!getDiskSpace (pii->fullname[0] - 'A', &bytesused, &bytesfree))
	{
		GrafMouse (ARROW, NULL);
		venusErr (NlsStr (T_DISKERR));
		return FALSE;
	}
	
	SetFileSizeField (pdrivinfo[DRVSIZE].ob_spec.tedinfo, bytesused);
	SetFileSizeField (pdrivinfo[DRVFREE].ob_spec.tedinfo, bytesfree);
	GrafMouse (ARROW, NULL);
	
	shortcut = pii->shortcut;
	pdrivinfo[DRVSC].ob_spec = SCut2Obspec (shortcut);
	
	DialCenter (pdrivinfo);
	DialStart (pdrivinfo, &d);
	DialDraw (&d);
	
	do
	{
		uword shift_state;

		done = TRUE;
		cycle_obj = -1;
		retcode = 0x7fff & DialXDo (&d, &edit_object, &shift_state,
			NULL, NULL, NULL, NULL);
		
		if (retcode == DRVSCSC)
			retcode = shift_state & 3 ? DRVSC0 : DRVSC;
		
		if (retcode == DRVSC0)
		{
			retcode = DRVSC;
			cycle_obj = DRVSC0;
		}
		
		if (retcode == DRVSC)
		{
			word retcut;
			
			done = FALSE;
			retcut = SCutSelect (pdrivinfo, DRVSC, shortcut, 
						pii->shortcut, cycle_obj);
			if (retcut != shortcut)
			{
				shortcut = retcut;
				pdrivinfo[DRVSC].ob_spec = SCut2Obspec (shortcut);
				fulldraw (pdrivinfo, ICONSC);
			}
		}

		if (retcode == DRVOK)
		{
			char newlabel[MAX_FILENAME_LEN];
			
			strcpy (newlabel, pfdiname);
			makeFullName (newlabel);
			if ((strlen (newlabel)) && !ValidFileName (newlabel))
			{
				venusErr (NlsStr (T_ILLNAME), newlabel);
				done = FALSE;
			}
			
			/* label verÑndert?
			 */
			if (done && strcmp (oldlabel, pfdiname))
			{
				int ret;
				
				GrafMouse (HOURGLASS, NULL);
	
				InstallErrorFunction (MupfelInfo, ErrorAlert);
				ret = SetLabel (MupfelInfo, pii->fullname, newlabel, 
						"label");
				DeinstallErrorFunction (MupfelInfo);
	
				if (ret == 0)
				{
					makeFullName (oldlabel);
					WindNewLabel (pii->fullname[0], oldlabel, newlabel);
					DeskIconNewLabel (pii->fullname[0], oldlabel, 
						newlabel);
					ApNewLabel (pii->fullname[0], oldlabel, newlabel);
				}
				else
					done = FALSE;
	
				GrafMouse (ARROW, NULL);
			}
		
			if (!done)
			{
				setSelected (pdrivinfo, DRVOK, FALSE);
				fulldraw (pdrivinfo, DRVOK);
			}
		}
	}
	while (!done);

	setSelected (pdrivinfo, retcode, FALSE);
	fulldraw (pdrivinfo, retcode);
	DialEnd (&d);
	
	if (retcode == DRVOK)
	{
		if (strcmp (pdrvname, pii->iconname))	/* name verÑndert */
		{
			installNewIconText (pii, objnr, pdrvname);
			DriveHasNewName (pii->fullname[0], pdrvname);
		}
		
		if (shortcut != pii->shortcut)
		{
			SCutRemove (pii->shortcut);
			SCutInstall (shortcut);
			pii->shortcut = shortcut;
			DriveHasNewShortcut (pii->fullname[0], shortcut);
		}
	}
	
	return retcode == DRVOK;
}

static int doIconInfo (IconInfo *pii, word objnr)
{
	DIALINFO d;
	word retcode, shortcut, cycle_obj;
	size_t len;
	char fname[MAX_FILENAME_LEN], label[MAX_FILENAME_LEN];
	int edit_object = ICONNAME;
	ICON_EXCHANGE ex_info;

#define TEXT_FIELD_MIN_LEN	30
	len = strlen (pii->fullname);
	if (len <= TEXT_FIELD_MIN_LEN)
	{
		strcpy (piconfile[ICONPATH].ob_spec.free_string, pii->fullname);
	}
	else
	{
		strncpy (piconfile[ICONPATH].ob_spec.free_string, 
			pii->fullname, 3);
		strcpy (&piconfile[ICONPATH].ob_spec.free_string[3], "...");
		strcat (piconfile[ICONPATH].ob_spec.free_string, 
			&pii->fullname[len - (TEXT_FIELD_MIN_LEN - 6)]);
	}

	strcpy (fname, pii->iconname);
	piconfile[ICONNAME].ob_spec.tedinfo->te_ptext = fname;
	
	strcpy (label, pii->label);
	makeEditName (label);
	piconfile[ICONLABL].ob_spec.tedinfo->te_ptext = label;
	setHideTree (piconfile, ICONLABL, strlen(label) == 0);
	setHideTree (piconfile, ICONNOLA, strlen(label) != 0);

	shortcut = pii->shortcut;
	piconfile[ICONSC].ob_spec = SCut2Obspec (shortcut);
	
	{
		char filename[MAX_FILENAME_LEN];
		
		GetBaseName (pii->fullname, filename, MAX_FILENAME_LEN);
		makeObjectToIcon (&piconfile[ICONICON], filename,
			pii->type == DI_FOLDER, NULL, &ex_info);
	}

	DialCenter (piconfile);
	DialStart (piconfile, &d);
	DialDraw (&d);
	
	do
	{
		uword shift_state;
	
		cycle_obj = -1;
		retcode = 0x7fff & DialXDo (&d, &edit_object, &shift_state,
			NULL, NULL, NULL, NULL);
		
		if (retcode == ICONSCSC)
			retcode = shift_state & 3 ? ICONSC0 : ICONSC;
			
		if (retcode == ICONSC0)
		{
			retcode = ICONSC;
			cycle_obj = ICONSC0;
		}
		
		if (retcode == ICONSC)
		{
			word retcut;
			
			retcut = SCutSelect (piconfile, ICONSC, shortcut, 
				pii->shortcut, cycle_obj);
			if (retcut != shortcut)
			{
				shortcut = retcut;
				piconfile[ICONSC].ob_spec = SCut2Obspec (shortcut);
				fulldraw (piconfile, ICONSC);
			}
		}
	}
	while ((retcode == ICONSC) || (retcode == ICONSCSC));
	
	setSelected (piconfile, retcode, FALSE);
	
	DialEnd (&d);
	
	if (retcode == ICONOK)
	{
		if (shortcut != pii->shortcut)
		{
			SCutRemove (pii->shortcut);
			SCutInstall (shortcut);
			pii->shortcut = shortcut;
		}
		if (strcmp (pii->iconname, fname))
		{
			installNewIconText (pii, objnr, fname);
		}
	}
	
	return retcode == ICONOK;
}

static int doSpecialInfo (IconInfo *pii, word objnr)
{
	DIALINFO d;
	uword fileanz, foldanz;
	word retcode;
	word shortcut, cycle_obj;
	long size;
	char *pname, *pfilenr;
	char *pfoldnr;
	char tmpstr[MAX_PATH_LEN];
	int edit_object = SPINAME;
	ICON_EXCHANGE ex_info;
	
	makeObjectToIcon (&pspecinfo[SPIICON], NULL, FALSE,
		&pii->icon, &ex_info);
	
	pname = pspecinfo[SPINAME].ob_spec.tedinfo->te_ptext;
	pfilenr = pspecinfo[SPIFILES].ob_spec.tedinfo->te_ptext;
	pfoldnr = pspecinfo[SPIFOLDE].ob_spec.tedinfo->te_ptext;
	
	GrafMouse(HOURGLASS, NULL);
	size = foldanz = fileanz = 0;
	strcpy (tmpstr, pii->fullname);
	if(!AddFileAnz(tmpstr, FALSE, -1L, &foldanz, &fileanz, &size))
	{
		GrafMouse(ARROW, NULL);
		return FALSE;
	}
	sprintf(pfoldnr, "%5u", foldanz);
	sprintf(pfilenr, "%5u", fileanz);
	SetFileSizeField (pspecinfo[SPISIZE].ob_spec.tedinfo, size);
	strcpy(pname, pii->iconname);
	
	shortcut = pii->shortcut;
	pspecinfo[SPISC].ob_spec = SCut2Obspec(shortcut);
	
	GrafMouse(ARROW, NULL);
	
	DialCenter(pspecinfo);
	DialStart(pspecinfo, &d);
	DialDraw(&d);
	
	do
	{
		uword shift_state;
	
		cycle_obj = -1;
		retcode = 0x7fff & DialXDo (&d, &edit_object, &shift_state,
			NULL, NULL, NULL, NULL);
		
		if (retcode == SPISCSC)
			retcode = shift_state & 3 ? SPISC0 : SPISC;
			
		if (retcode == SPISC0)
		{
			retcode = SPISC;
			cycle_obj = SPISC0;
		}
		if (retcode == SPISC)
		{
			word retcut;
			
			retcut = SCutSelect(pspecinfo, SPISC, 
							shortcut, pii->shortcut, cycle_obj);
			if (retcut != shortcut)
			{
				shortcut = retcut;
				pspecinfo[SPISC].ob_spec = SCut2Obspec(shortcut);
				fulldraw(pspecinfo, SPISC);
			}
		}
	}
	while ((retcode == SPISC) || (retcode == SPISCSC));
	
	setSelected(pspecinfo, retcode, FALSE);
	fulldraw(pspecinfo, retcode);
	DialEnd(&d);
	
	if(retcode == SPIOK)
	{
		if (shortcut != pii->shortcut)
		{
			SCutRemove (pii->shortcut);
			SCutInstall (shortcut);
			pii->shortcut = shortcut;
		}

		if (strcmp (pii->iconname, pname))	/* Name verÑndert */
		{
			installNewIconText (pii, objnr, pname);
		}
	}

	return (retcode == SPIOK);
}

static int showTotalInfoAndDoMore (WindInfo *wp)
{
	DIALINFO d;
	ARGVINFO A;
	uword fileanz, foldanz;
	word retcode;
	long bytes;
	
	if (!wp->getDragged) return FALSE;
		
	GrafMouse (HOURGLASS, NULL);

	retcode = wp->getDragged (wp, &A, TRUE);
	if (retcode)
	{
		retcode = CountArgvFolderAndFiles (&A, TRUE, -1L, &foldanz, 
			&fileanz, &bytes);
		if (!retcode)
			FreeArgv (&A);
	}

	if (!retcode || (bytes < 0L))
	{
		if (bytes < 0L) venusErr (NlsStr(FILE_COUNT_ERR));
		GrafMouse (ARROW, NULL);
		return FALSE;
	}
	
	sprintf (ptotalinf[TIFOLDNR].ob_spec.tedinfo->te_ptext,
			"%5u", foldanz);
	sprintf (ptotalinf[TIFILENR].ob_spec.tedinfo->te_ptext,
			"%5u", fileanz);
	SetFileSizeField (ptotalinf[TIBYTES].ob_spec.tedinfo, bytes);

	FreeArgv (&A);
	GrafMouse (ARROW,NULL);
	
	DialCenter (ptotalinf);
	DialStart (ptotalinf, &d);
	DialDraw (&d);
	
	retcode = DialDo (&d, 0) & 0x7FF;
	setSelected (ptotalinf, retcode, FALSE);

	DialEnd (&d);	
	GrafMouse (ARROW, NULL);
	
	return retcode == TIMORE;
}


static void showFileWindInfo (WindInfo *wp)
{
	FileWindow_t *fwp = wp->kind_info;
	FileBucket *bucket = fwp->files;
	uword i;
	int showMore = TRUE;
		
	while (bucket && showMore)
	{
		for (i = 0; showMore && i < bucket->usedcount; ++i)
		{
			if (!bucket->finfo[i].flags.dragged)
				continue;

			if (bucket->finfo[i].attrib & FA_FOLDER)
			{
				if (strcmp (bucket->finfo[i].name, ".."))
				{
					showMore = infoFolderDialog (fwp->path, 
								&bucket->finfo[i], wp);
				}
				else
					venusInfo (NlsStr (T_FOLDABOVE));
			}
			else
			{
				showMore = infoFileDialog (fwp->path, 
								&bucket->finfo[i], wp);
			}
				
		}
		
		bucket = bucket->nextbucket;
	}
}


static void showDeskWindInfo (WindInfo *wp)
{
	DeskInfo *dip = wp->kind_info;
	IconInfo *pii;
	int j, showMore = TRUE;
		
	for (j = dip->tree[0].ob_head; 
			showMore && (j > 0); j = dip->tree[j].ob_next)
	{
		if (!ObjectWasDragged (&dip->tree[j]))
			continue;

		pii = GetIconInfo (&dip->tree[j]);
		if (!pii)
			continue;
		
		switch (pii->type)
		{
			case DI_FILESYSTEM:
				showMore = infoDriveDialog (pii, j);
				break;

			case DI_TRASHCAN:
			case DI_SCRAPDIR:
				showMore = doSpecialInfo (pii, j);
				break;

			case DI_SHREDDER:
				showMore = DoShredderDialog (pii, j);
				break;

			case DI_PROGRAM:
			case DI_FOLDER:
				showMore = doIconInfo (pii, j);
				break;

			default:
				venusDebug("unknown Icontype!");
				showMore = FALSE;
				break;
		}

	}
}

/*
 * void getInfoDialog(void)
 *
 * main routine to handle 'get Info...'
 */
void getInfoDialog(void)
{
	WindInfo *wp;
	word objnr;
	
	if (!ThereAreSelected (&wp))
		return;
	
	if (wp->make_ready_to_drag != NULL)
		wp->make_ready_to_drag (wp);
	MarkDraggedObjects (wp);
	
	/* Ist mehr als 1 Objekt selektiert, dann wird
	 * die Gesamtzahl von Dateien, etc. angezeigt. DrÅckt
	 * der Benutzer "Mehr...", geht die normale Info weiter
	 */
	if (GetOnlySelectedObject (&wp, &objnr)
		|| showTotalInfoAndDoMore (wp))
	{
		switch (wp->kind)
		{
			case WK_FILE:
				showFileWindInfo (wp);
				break;
	
			case WK_DESK:
				showDeskWindInfo (wp);
				break;
			
			default:
				break;
		}
	}
	
	UnMarkDraggedObjects (wp);
	FlushPathUpdate ();
}
