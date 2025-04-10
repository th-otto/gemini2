/*
 * @(#) Gemini\Init.c
 * @(#) Stefan Eissing, 13. November 1994
 *
 * description: all the Initialisation for the Desktop
 *
 * jr 19.5.95
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <vdi.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "rscvers.h"

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
			
#include "vs.h"

#include "window.h"
#include "applmana.h"
#include "cpkobold.h"
#include "filewind.h"
#include "filedraw.h"
#include "fileutil.h"
#include "gemtrees.h"
#include "iconinst.h"
#include "init.h"
#include "menu.h"
#if MERGED
#include "mvwindow.h"
#endif
#include "pexec.h"
#include "redraw.h"
#include "select.h"
#include "stand.h"
#include "util.h"
#include "venus.h"
#include "venuserr.h"

#include "..\mupfel\mupfel.h"
#include "..\mupfel\mglob.h"



/* externals
*/
extern OBJECT *pmenu,*pcopyinfo,*pstdbox,*pshowbox;
extern OBJECT *pwildbox,*prulebox, *pruleedit;
extern OBJECT *pinstbox,*prenamebox,*pcopybox;
extern OBJECT *pnamebox,*perasebox,*pfoldbox;
extern OBJECT *pfileinfo,*pfoldinfo,*pdrivinfo;
extern OBJECT *papplbox,*pttpbox,*pchangebox;
extern OBJECT *pshredbox,*ptrashbox,*pscrapbox;
extern OBJECT *piconfile,*pfrmtbox, *pconselec;
extern OBJECT *pspecinfo,*poptiobox,*pdivbox;
extern OBJECT *pinitbox,*pfontbox, *pnewdesk;
extern OBJECT *pweditbox,*popendbox, *pfmtbox;
extern OBJECT *pappledit, *pshortcut, *pcolorbox;
extern OBJECT *ptotalinf, *pginsave, *psetdesk;
extern OBJECT *ppattbox, *pdrivesel, *pswitchoff;

/* internal texts
 */
#define NlsLocalSection "G.init"
enum NlsLocalText{
T_ABOUT,		/*  Åber Gemini... */
T_ERASE,		/*Diese Operation lîscht sÑmtliche Daten auf 
der Diskette in Laufwerk %c:. Wollen Sie fortfahren?*/
T_FMTHELP,		/*Beim Formatieren einer Diskette werden alle
 Daten gelîscht. Sie kînnen das Format der Diskette selbst bestimmen.
 360 KB und 400 KB sind die Formate fÅr einseitige Disketten. Alle
 anderen sind doppelseitig. 1.4 MB und 1.6 MB kînnen nur bei
 HD-Disketten erfolgreich benutzt werden.|
Zum Lîschen muû eine Diskette bereits einmal formatiert worden sein.*/
};

#define FIXTREE		1


/* îffne virtuelle Workstation
*/
int InitVDI (word v_handle)
{
	word dummy;
	word work_out[57];				/* VDI Parameter Arrays */
	word pxy[4];
	
	vdi_handle = v_handle;
	phys_handle = graf_handle (&wchar, &hchar, &wbox, &hbox);

	vq_extnd (vdi_handle, 0, work_out);
	number_of_settable_colors = work_out[13];
	filework.handle = vdi_handle;
	filework.loaded = FALSE;
	filework.sysfonts = work_out[10];
	filework.addfonts = 0;
	filework.list = NULL;
	
	wind_get (0, WF_WORKXYWH, &Desk.g_x, &Desk.g_y, &Desk.g_w, 
		&Desk.g_h);

	pxy[0] = Desk.g_x; 
	pxy[1] = Desk.g_y;
	pxy[2] = Desk.g_x + Desk.g_w - 1;
	pxy[3] = Desk.g_y + Desk.g_h - 1;
	vst_alignment (vdi_handle, 0, 5, &dummy, &dummy);
	vs_clip (vdi_handle, TRUE, pxy);

	vq_extnd (vdi_handle, 1, work_out);

	number_of_planes = work_out[4];
	number_of_colors = (1L << work_out[4]);

	return TRUE;
}

void ExitVDI (void)
{
}


static void alignMenu (void)
{
	word diff;
	
#if MERGED
		pmenu[MNBAR].ob_width += wchar;
		pmenu[DESK].ob_spec.free_string = " GEMINI";
		pmenu[DESK].ob_width += wchar;
		pmenu[FILES].ob_x += wchar;
		pmenu[MNFILE].ob_x += wchar;
		pmenu[EDIT].ob_x += wchar;
		pmenu[MNEDIT].ob_x += wchar;
		pmenu[SHOW].ob_x += wchar;
		pmenu[MNSHOW].ob_x += wchar;
		pmenu[OPTIONS].ob_x += wchar;
		pmenu[MNOPTION].ob_x += wchar;
		pmenu[DESKINFO].ob_spec.free_string = (char *)NlsStr (T_ABOUT);
#endif

	MenuTune (pmenu, FALSE);
	
	if ((pmenu[OPTIONS].ob_x + pmenu[OPTIONS].ob_width) > Desk.g_w)
	{
		word edit_width = pmenu[EDIT].ob_width;
		
		setHideTree (pmenu, EDIT, TRUE);
		setDisabled (pmenu, EDIT, TRUE);
		setHideTree (pmenu, MNEDIT, TRUE);
		setDisabled (pmenu, MNEDIT, TRUE);
		
		pmenu[SHOW].ob_x -= edit_width;
		pmenu[MNSHOW].ob_x -= edit_width;
		pmenu[OPTIONS].ob_x -= edit_width;
		pmenu[MNOPTION].ob_x -= edit_width;
		pmenu[MNBAR].ob_width -= edit_width;
	}

	diff = Desk.g_w - (pmenu[MNOPTION].ob_x + pmenu[MNOPTION].ob_width);
	if (diff < 0)
	{
		pmenu[MNOPTION].ob_x += (diff - 1);
	}
}

static int validateZ (OBJECT *t, int ob, int *chr, int *shift, 
	int idx)
{
	static char legals[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"!0123456789_#$%^&()+-=~;,<>|[]{}"
		"\'\"\b\x1B\x7F";

	/* Hochkommata, AnfÅhrungszeichen und Klammeraffe entfernt!
	 * se 19.01.91
	 */
	(void) t, (void) ob, (void) shift, (void) idx;

	/* jr: Gemini-PrÑferenz hinsichtlich groû/klein beachten */

	if (NewDesk.preferLowerCase)
		*chr = tolower (*chr);
	else
		*chr = toupper (*chr);

	return (NULL != strchr (legals, toupper (*chr)));
}

VALFUN validFuns[] = {validateZ};


/* jr: Resource testen */

static int
test_resource (void)
{
	char *version_string;
	
	if (!rsrc_gaddr (5, 0, &version_string))
		return FALSE;
	
	if (strcmp (version_string, RSC_CRC_STRING))
		return FALSE;
		
	return TRUE;
}


/* get Addresses from Resourcefile
*/
int GetTrees (void)
{
	OBSPEC *pob;
	word ok;

	ok = test_resource ();

	if (ok)
		ok = rsrc_gaddr (R_TREE, MENU, &pmenu);
	if(ok)
		ok = rsrc_gaddr (R_TREE, COPYINFO, &pcopyinfo);
	if(ok)
		ok = rsrc_gaddr (R_TREE, NEWDESK, &pnewdesk);
	if(ok)
		ok = rsrc_gaddr (R_TREE, STDBOX, &pstdbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, SHOWBOX, &pshowbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, WILDBOX, &pwildbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, RULEBOX, &prulebox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, INSTBOX, &pinstbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, RENAMBOX, &prenamebox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, COPYBOX, &pcopybox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, NAMEBOX, &pnamebox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, ERASEBOX, &perasebox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, FOLDBOX, &pfoldbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, FOLDINFO, &pfoldinfo);
	if(ok)
		ok = rsrc_gaddr (R_TREE, FILEINFO, &pfileinfo);
	if(ok)
		ok = rsrc_gaddr (R_TREE, DRIVINFO, &pdrivinfo);
	if(ok)
		ok = rsrc_gaddr (R_TREE, APPLBOX, &papplbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, TTPBOX, &pttpbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, CHANGBOX, &pchangebox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, SHREDBOX, &pshredbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, TRASHBOX, &ptrashbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, SCRAPBOX, &pscrapbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, ICONFILE, &piconfile);
	if(ok)
		ok = rsrc_gaddr (R_TREE, SPECINFO, &pspecinfo);
	if(ok)
		ok = rsrc_gaddr (R_TREE, OPTIOBOX, &poptiobox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, FONTBOX, &pfontbox);
	if (ok)
		ok = rsrc_gaddr (R_TREE, WEDIT, &pweditbox);
	if (ok)
		ok = rsrc_gaddr (R_TREE, OPENDBOX, &popendbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, DIVBOX, &pdivbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, INITBOX, &pinitbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE,CONSELEC, &pconselec);
	if(ok)
		ok = rsrc_gaddr (R_TREE, RULEEDIT, &pruleedit);
	if(ok)
		ok = rsrc_gaddr (R_TREE, APPLEDIT, &pappledit);
	if(ok)
		ok = rsrc_gaddr (R_TREE, SHORTCUT, &pshortcut);
	if(ok)
		ok = rsrc_gaddr (R_TREE, COLORBOX, &pcolorbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, FMTBOX, &pfmtbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, TOTALINF, &ptotalinf);
	if(ok)
		ok = rsrc_gaddr (R_TREE, GINSAVE, &pginsave);
	if(ok)
		ok = rsrc_gaddr (R_TREE, DESKBACK, &psetdesk);
	if(ok)
		ok = rsrc_gaddr (R_TREE, PATTBOX, &ppattbox);
	if(ok)
		ok = rsrc_gaddr (R_TREE, DRIVESEL, &pdrivesel);
	if(ok)
		ok = rsrc_gaddr (R_TREE, SWITCHOF, &pswitchoff);

	if (ok)
	{
#if FIXTREE
		FixTree (pcopyinfo);
		FixTree (pshowbox);
		FixTree (pwildbox);
		FixTree (pinstbox);
		FixTree (prulebox);
		FixTree (prenamebox);
		FixTree (pcopybox);
		FixTree (pnamebox);
		FixTree (perasebox);
		FixTree (pfoldbox);
		FixTree (pfoldinfo);
		FixTree (pfileinfo);
		FixTree (pdrivinfo);
		FixTree (papplbox);
		FixTree (pttpbox);
		FixTree (pchangebox);
		FixTree (pshredbox);
		FixTree (ptrashbox);
		FixTree (pscrapbox);
		FixTree (piconfile);
		FixTree (pspecinfo);
		FixTree (poptiobox);
		FixTree (pfontbox);
		FixTree (pweditbox);
		FixTree (popendbox);
		FixTree (pdivbox);
		FixTree (pinitbox);
		FixTree (pconselec);
		FixTree (pruleedit);
		FixTree (pappledit);
		FixTree (pshortcut);
		FixTree (pcolorbox);
		FixTree (pfmtbox);
		FixTree (ptotalinf);
		FixTree (pginsave);
		FixTree (psetdesk);
		FixTree (ppattbox);
		FixTree (pdrivesel);
		FixTree (pswitchoff);
#endif /* FIXTREE */

		/* Setze Funktion, um Templates fÅr Filenamen zu scannen
		 */
		FormSetValidator ("Z", validFuns);
		
		alignMenu ();
		
		pob = ObjcGetObspec (pcopyinfo, CPITITLE);
		pob->free_string = venusVersionString;

		ShowInfo.normicon = TRUE;
		ShowInfo.showtext = TRUE;
		menu_icheck (pmenu, BYTEXT, TRUE);
		ShowInfo.sortentry = SORTNAME;
		ShowInfo.m_cols = 80;
		ShowInfo.m_rows = 24;
		ShowInfo.m_inv = 0;
		ShowInfo.m_font = 1;
		ShowInfo.m_fsize = 6;
		strcpy (ShowInfo.gin_name, "GEMINI");
		
		NewDesk.snapx = 80;
		NewDesk.snapy = 40;
		NewDesk.useCopyKobold =
		NewDesk.useDeleteKobold = TRUE;
		NewDesk.minCopyKobold = 5;
		NewDesk.minDeleteKobold = 10;

		menu_icheck (pmenu, ShowInfo.sortentry, TRUE);
	}
	
	return ok;
}


/*
 * setze Status der Parameterboxen im Init-Dialog
 */
static void setState (word state, word redraw)
{
	setDisabled (pinitbox, FMT360, state);
	setDisabled (pinitbox, FMT400, state);
	setDisabled (pinitbox, FMT720, state);
	setDisabled (pinitbox, FMT800, state);
	setDisabled (pinitbox, FMT140, state);
	setDisabled (pinitbox, FMT160, state);
	if (redraw)
	{
		fulldraw (pinitbox, FMT360);
		fulldraw (pinitbox, FMT400);
		fulldraw (pinitbox, FMT720);
		fulldraw (pinitbox, FMT800);
		fulldraw (pinitbox, FMT140);
		fulldraw (pinitbox, FMT160);
	}
}

#define FORMAT_HARD		0x0100
#define FORMAT_MASK		0x00FF
static word format_obj[] =
{
	FMT720, FMT360, FMT400, FMT800, FMT140, FMT160, 
};

static void setFormatInfoInDialog (void)
{
	word hard, modus, i;
	
	hard = ((NewDesk.format_info & FORMAT_HARD) != 0);
	modus = (NewDesk.format_info & FORMAT_MASK);
	if (modus >= DIM(format_obj))
		modus = 0;
	
	for (i = 0; i < DIM(format_obj); ++i)
		setSelected (pinitbox, format_obj[i], FALSE);
		
	setSelected (pinitbox, format_obj[modus], TRUE);
	setSelected (pinitbox, INITSOFT, !hard);
	setSelected (pinitbox, INITFORM, hard);
}

static void getFormatInfoFromDialog (void)
{
	word i;
	
	NewDesk.format_info = 0;
	for (i = 0; i < DIM(format_obj); ++i)
	{
		if (isSelected (pinitbox, format_obj[i]))
			NewDesk.format_info = i;
	}
	
	if (!isSelected (pinitbox, INITSOFT))
		NewDesk.format_info |= FORMAT_HARD;
}

void showProgress (int promille)
{
	word lastw, neww, x, y;
	
	neww = (int)((pfmtbox[FMTBACK].ob_width * (long)promille) / 1000L);
	lastw = pfmtbox[FMTFORE].ob_width;
	if (neww < 3)
		neww = 3;

	if (neww != lastw)
	{
		pfmtbox[FMTFORE].ob_width = neww;
		ObjcOffset (pfmtbox, FMTFORE, &x, &y);
		ObjcDraw (pfmtbox, FMTFORE, 0, 	x + lastw - 1, y,
			neww - lastw + 2, pfmtbox[FMTFORE].ob_height);
	}
}

/*
 * initialize Disk (wipe cleeeeaaaan!)
 */
void InitDisk (void)
{
	ARGVINFO A;
	DIALINFO d;
	DeskInfo *dip;
	WindInfo *wp;
	IconInfo *pii;
	word objnr, retcode, done;
	char drive[4];
	char label[MAX_FILENAME_LEN];
	int update, dial_closed;
	
	if ((!GetOnlySelectedObject (&wp, &objnr)) 
		|| (wp->kind != WK_DESK))
		return;

	if (NewDesk.useFormatKobold && KoboldPresent ())
	{
		MarkDraggedObjects (wp);
		retcode = wp->getDragged (wp, &A, TRUE);
		UnMarkDraggedObjects (wp);

		if (retcode)
		{
			CopyWithKobold (&A, NULL, kb_format, TRUE, FALSE, FALSE);
			FreeArgv (&A);
			return;
		}
	}
	
	dip = wp->kind_info;
	pii = GetIconInfo (&dip->tree[objnr]);
	if((pii->type != DI_FILESYSTEM) || (pii->fullname[0] > 'B'))
		return;
		
	drive[0] = pii->fullname[0];
	drive[1] = ':';
	drive[2] = '\0';
	update = FALSE;

	label[0] = '\0';
	pinitbox[INITNAME].ob_spec.tedinfo->te_ptext = label;
	
	setFormatInfoInDialog ();
	setState (isSelected (pinitbox, INITSOFT), FALSE);
	
	DialCenter (pinitbox);
	DialStart (pinitbox,&d);
	DialDraw (&d);
	dial_closed = FALSE;
	
	done = FALSE;
	do
	{
		retcode = DialDo (&d, 0) & 0x7FFF;

		switch (retcode)
		{
			case INITOK:
			{
				int format, sides, sectors;
				int error;
				char *cmd, *tmplabel, name[MAX_FILENAME_LEN];
				
				GrafMouse (HOURGLASS, NULL);

				format = isSelected (pinitbox, INITFORM);
				cmd = format? "format" : "init";
				
				if (isSelected (pinitbox, FMT360))
				{
					sides = 1; sectors = 9;
				}
				else if (isSelected (pinitbox, FMT400))
				{
					sides = 1; sectors = 10;
				}
				else
				{
					sides = 2;
					
					if (isSelected (pinitbox, FMT800))
						sectors = 10;
					else if (isSelected (pinitbox, FMT140))
						sectors = 18;
					else if (isSelected (pinitbox, FMT160))
						sectors = 20;
					else
						sectors = 9;
				}
				
				if (strlen (label))
				{
					strcpy (name, label);
					makeFullName (name);
					tmplabel = name;
				}
				else
					tmplabel = NULL;
				
				if (venusChoice (NlsStr (T_ERASE), drive[0]))
				{
					InstallErrorFunction (MupfelInfo, ErrorAlert);

 					if (format)
 					{
						DIALINFO FMT;

						DialEnd (&d);
						
						pfmtbox[FMTFORE].ob_width = 3;
						DialCenter (pfmtbox);
						DialStart (pfmtbox, &FMT);
						DialDraw (&FMT);
						
						error = FormatDrive (MupfelInfo, cmd, 
							drive[0], sides, sectors, FALSE, 
							tmplabel, showProgress);


						DialEnd (&FMT);
						
						if (error)
						{
							DialStart (pinitbox, &d);
							DialDraw (&d);
						}
						else
							dial_closed = TRUE;
 					}
 					else
 					{
 						char *margv[6];
 						int margc = 3;
 						
 						margv[0] = cmd;
 						margv[1] = drive;
 						margv[2] = "-y";
 						
 						if (tmplabel)
 						{
 							margv[margc++] = "-l";
 							margv[margc++] = tmplabel;
 						}
 						
 						error = m_init (MupfelInfo, margc, margv);
 					}

					DeinstallErrorFunction (MupfelInfo);

					if (error == 0)
						done = TRUE;
						
					update = TRUE;
				}
				
				if (!done)
				{
					GrafMouse (ARROW, NULL);
					setSelected (pinitbox, INITOK, FALSE);
					fulldraw (pinitbox, INITOK);
				}
				else
					getFormatInfoFromDialog ();
				
				break;
			}

			case INITFORM:
				if (isDisabled (pinitbox, FMT720))
					setState (FALSE,TRUE);
				break;

			case INITSOFT:
				if (!isDisabled (pinitbox, FMT720))
					setState (TRUE, TRUE);
				break;

			case FMTHELP:
				venusInfo (NlsStr (T_FMTHELP));
				setSelected (pinitbox, FMTHELP, FALSE);
				fulldraw (pinitbox, FMTHELP);
				break;
				
			default:
				done = TRUE;
				break;
		}
	}
	while (!done);

	setSelected (pinitbox, retcode, FALSE);
	if (!dial_closed)
		DialEnd (&d);
	
	if (update)
	{
		strcat (drive, "\\");
		PathUpdate (drive);
	}
	GrafMouse (ARROW, NULL);
}


int MupVenus (char *autoexec)
{
	word startmode;
	char cmdline[MAX_PATH_LEN];
	const char *path;
	int broken = 0, found = 0;
	
	path = GetEnv (MupfelInfo, "HOME");
	if (path)
	{	
		strcpy (cmdline, path);
		AddFileName (cmdline, autoexec);
		found = access (cmdline, A_EXIST, &broken);
	}
	
	if (!found && !broken)
	{
		strcpy (cmdline, BootPath);
		AddFileName (cmdline, autoexec);
		found = access (cmdline, A_EXIST, &broken);
	}
	
	if (found)
	{
		GetStartMode (autoexec, &startmode);
		if (!(startmode & WCLOSE_START))
			allrewindow (WU_SHOWTYPE);
	
		SetFullPath (path);
		
		return executer (startmode, TRUE, cmdline, NULL);
	}

	return 0;
}