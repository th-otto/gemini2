/*
 * @(#) Gemini\Dispatch.c
 * @(#) Stefan Eissing, 12. November 1994
 *
 * description: Dispatch several events 
 *
 * jr 22.4.95
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\genargv.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"

#include "vs.h"
#include "ttpbox.rh"
#include "menu.rh"

#include "window.h"
#include "applmana.h"
#include "dispatch.h"
#include "filewind.h"
#include "fileutil.h"
#include "getinfo.h"
#include "icondrag.h"
#include "iconinst.h"
#include "menu.h"
#include "message.h"
#include "myalloc.h"
#include "pexec.h"
#include "redraw.h"
#include "select.h"
#include "undo.h"
#include "util.h"
#include "venuserr.h"


/* internal texts
 */
#define NlsLocalSection "G.dispatch"
enum NlsLocalText{
T_TWODISK,	/*Fr diese Operation mssten sie zwei Disketten
 gleichzeitig in ein Laufwerk schieben!*/
T_NESTED,	/*Die Operation wird nicht erlaubt, da die 
Ordner zu tief verschachtelt sind.*/
T_SHREDDER,	/*Der Reižwolf ist immer leer. Daher l„ž er sich
 nicht ”ffnen!*/
T_NODRIVE,	/*Das Laufwerk %c: ist GEMDOS nicht bekannt!*/
T_NOLABEL,	/*Kann das Label von Laufwerk %c: nicht lesen!
 Ist evtl. keine Diskette/Cartridge eingelegt?*/
T_NOFILE,	/*Kann das Objekt %s nicht starten!*/
T_DELAPPL,	/*Die angemeldete Applikation %s konnte nicht
 gefunden werden! Wollen Sie die entsprechende Regel l”schen?*/
T_LOCATION,	/*Kann das Objekt %s nicht finden. Soll das Icon
 entfernt werden, oder wollen Sie danach suchen?*/
T_LOCBUTTON, /*[Entfernen|[Suchen|[Abbruch*/
T_LOCOBJ,	/*Objekt suchen*/
T_NOACC,	/*Das Accessory %s ist dem AES nicht bekannt!*/
T_NOAPPL,	/*Es ist keine Anwendung fr %s angemeldet.*/
};


/* Externs
 */
extern OBJECT *pmenu,*pttpbox;


/*
 * static char *getCommandLine(const char *name)
 * get a Commandline from user by dialog
 */
static char *getCommandLine (const char *name)
{
	DIALINFO d;
	word retcode;
	char *pline;
	char *cp;
	int edit_object = TTPCOMM;
	
	pttpbox[TTPNAME].ob_spec.tedinfo->te_ptext = (char *)name;
	pline = pttpbox[TTPCOMM].ob_spec.tedinfo->te_ptext;
	cp = malloc (1024 * sizeof(char));
	if (!cp) return NULL;
	strcpy (cp, pline);

	DialCenter (pttpbox);
	DialStart (pttpbox,&d);
	DialDraw (&d);
	
	retcode = DialDo (&d, &edit_object) & 0x7FFF;
	setSelected (pttpbox, retcode, FALSE);
	
	if (retcode == TTPOK)
	{
		if (*pline == '@')
			*cp = '\0';
		else
			strcpy (cp, pline);
	}
	else
	{
		strcpy (pline, cp);
		free (cp);
		cp = NULL;
	}
	DialEnd (&d);
	
	return cp;
}

/* 
 * Schaut nach, ob das Laufwerk <drive> dasselbe Label wie <label>
 * hat und fordert den Benutzer ggfs. auf das Medium einzulegen.
 * Im Fehlerfall oder bei Abbruch wird FALSE zurckgegeben.
 */
static int checkLabel (char drive, const char *label)
{
	char real_label[MAX_FILENAME_LEN];
	
	if (E_OK == DriveAccessible (drive, real_label, MAX_FILENAME_LEN))
	{
		if (!sameLabel (real_label, label))
		{
			return changeDisk (drive, label);
		}
		return TRUE;
	}
	else
		venusErr (NlsStr(T_NOLABEL), drive);
	
	return FALSE;	
}

/*
 * try to start file with commandline
 */
int StartFile (WindInfo *wp, word fobj, word showpath, char *label,
	char *path, char *name, ARGVINFO *args, word kbd_state,
	word *retcode)
{
	word tofree = FALSE;
	char *mycomm, *command;
	word startmode, doit = TRUE;
	char filename[MAX_PATH_LEN] = "";
	char *cp, prglabel[MAX_FILENAME_LEN];
	word wasApplRule = FALSE;
	word started = FALSE;
	int name_is_data = FALSE;
	int broken;
	int ignore_rules = (kbd_state & (K_LSHIFT|K_RSHIFT));
	
	GrafMouse (HOURGLASS, NULL);
	command = args? Argv2String (pMainAllocInfo, args, FALSE) : NULL;
	mycomm = command;			/* erstmal auf dasselbe zeigen */

	DeselectAllObjects ();
	SelectObject (wp, fobj, TRUE);
	flushredraw ();

	cp = strrchr (name,'.');
	if (cp)
		cp++;				/* pointer to extension */

	/* Jetzt werden bei Shift-Doppelklick wirklich alle eingebauten
	 * Regeln ignoriert.
	 */
	if (ignore_rules)
		name_is_data = TRUE;
	else if (IsAccessory (name))
	{
		started = StartAcc (wp, fobj, name, args, command);
		*filename = '\0';
		/* Wenn es nicht gestartet werden konnte, suchen wir eine
		 * Applikation, die es evtl. kann.
		 */
		if (!started)
			name_is_data = TRUE;
	}
#if OLD_STYLE
	/* Dies war der alte Stil, bei dem Executables nicht mit
	 * Shift-Doppelklick angesehen werden konnten.
	 */
	else if (cp && (!ignore_rules || stricmp (cp, "MUP"))
		&& isExecutable (name))
#endif
	else if (cp && IsExecutable (name))
	{
		if (checkLabel (path[0], label))
		{
			strcpy (filename,path);
			AddFileName (filename,name);
			GetStartMode (name, &startmode);
			if ((startmode & TTP_START) && (mycomm == NULL))
			{
				GrafMouse (ARROW,NULL);
				mycomm = getCommandLine (name);
				GrafMouse (HOURGLASS,NULL);
				
				if (mycomm == NULL)
				{
					GrafMouse (ARROW,NULL);
					return FALSE;		/* Cancel gedrckt */
				}
				else
					tofree = TRUE;
			}
		}
	}
	else if (cp && !stricmp (cp, "GIN"))
	{
		strcpy (filename, path);
		AddFileName (filename, name);
		started = ExecGINFile (filename);
		*filename = '\0';
	}
	else
		name_is_data = TRUE;
	
	if (name_is_data)
	{
		if (GetApplForData (name, filename, prglabel, 
			ignore_rules, 1, &startmode))
		{
			size_t len;

			/* zust„ndiges Programm in filename
			 */
			if ((filename[1] != ':')
				|| ((path[0] != filename[0])
				|| sameLabel(prglabel,label)))
			{
				if (filename[1] == ':')
				{
					doit = checkLabel (filename[0], prglabel);
				}
				
				if (doit)
				{
					/* got programm from application-rules
					 */
					wasApplRule = TRUE;
					
					len = strlen (path) + strlen (name) + 2;
					if (command)
						len += strlen (command) + 1;
					
					mycomm = malloc (len * sizeof (char));
					tofree = (mycomm != NULL);
					if (tofree)
					{
						strcpy (mycomm, path);
						strcat (mycomm, name);
					
						if(command)
						{
							strcat (mycomm, " ");
							strcat (mycomm, command);
						}
					}
				}
				else
					filename[0] = '\0';	/* don't execute */
			}
			else
			{
				venusErr(T_TWODISK);
				filename[0] = '\0'; /* don't execute */
			}
		}
		else
		{
			if (startmode == 0)
				venusErr(NlsStr(T_NOAPPL), name);
			filename[0] = '\0'; /* don't execute */
		}
	}
	
	if (strlen (filename))
	{
		int done = FALSE;
		
		if (IsAccessory (filename) || (startmode & CANVA_START))
		{
			done = StartAcc (wp, fobj, filename, NULL, mycomm);
			if (done)
				started = TRUE;
			else if (IsAccessory (filename))
			{
				venusErr (NlsStr (T_NOACC), filename);
				done = TRUE;
			}
		}
		
		if (!done && ((filename[1] != ':')
			 || access (filename, A_EXIST, &broken)))
		{
			if (wp)
			{
				OBJECT *tree;

				if (wp->window_get_tree)
					tree = wp->window_get_tree (wp);
				else
					tree = NULL;
				
				if (tree)
					doFullGrowBox (tree, fobj);
			}
	
			started = TRUE;
			*retcode = executer (startmode, showpath, filename, 
							mycomm);
	
			if (*retcode < E_OK) venusErr (StrError (*retcode));

		}
		else if (!done)
		{
			if (wasApplRule)
			{
				if (venusChoice (NlsStr (T_DELAPPL), filename))
				{
					char *cp;

					cp = strrchr (filename, '\\');
					if (!cp)
						cp = filename;
					else
						++cp;
						
					if (!RemoveApplInfo (cp))
						venusDebug ("Konnte applinfo nicht l”schen!");
				}
			}
			else
				venusErr (NlsStr (T_NOFILE), filename);
		}
	}
	
	if(tofree)
	{
		free(mycomm);
		mycomm = NULL;
	}
	free (command);
	GrafMouse(ARROW,NULL);
	
	return started;
}

#define KSTATE_MASK	(K_CTRL|K_ALT|K_LSHIFT|K_RSHIFT)

word ClickInfo (WindInfo *wp, word object, word kstate)
{
	if ((kstate & KSTATE_MASK) == K_CTRL)
	{
		DeselectAllObjects ();
		SelectObject (wp, object, TRUE);
		flushredraw ();
		menu_tnormal (pmenu, FILES, FALSE);
		getInfoDialog ();
		menu_tnormal (pmenu, FILES, TRUE);
		return TRUE;
	}
	return FALSE;
}

/*
 * manage doubleclick on object fobj in window wp
 */
static void Dclickon (WindInfo *wp, word fobj, word kstate)
{
	FileWindow_t *fwp;
	FileInfo *pf;
	word retcode;
	
	if (wp->kind != WK_FILE)
	{
		venusInfo ("Dclickon: falscher Fenstertyp");
		return;
	}
	
	fwp = (FileWindow_t *)wp->kind_info;
	
	pf = GetFileInfo (&fwp->tree[fobj]);	/* Pointer on FileInfo */

	if (pf->attrib & FA_FOLDER)				/* is directory */
	{
		DeselectAllObjects ();
		SelectObject (wp, fobj, TRUE);
		flushredraw ();
		GrafMouse (HOURGLASS, NULL);

		if (strcmp (pf->name, ".."))
		{
			if (strlen (fwp->path) >= MAX_PATH_LEN-28)
			{
				venusErr (NlsStr (T_NESTED));
				GrafMouse (ARROW, NULL);
				return;
			}

			if ((kstate & KSTATE_MASK) == K_ALT)
			{
				char path[MAX_PATH_LEN], title[MAX_PATH_LEN];
				
				strcpy (path, fwp->path);
				AddFolderName (path, pf->name);
				strcpy (title, pf->name);
				strcat (title, "\\");
				FileWindowOpen (path, fwp->label, title, 
					fwp->wildcard, 0);
			}
			else if (wp->add_to_display_path != NULL)
			{
				wp->add_to_display_path (wp, pf->name);
				if (GetTopWindInfo () == wp)
					IsTopWindow (wp->handle);
			}
		}
		else
		{
			if ((kstate & KSTATE_MASK) == K_ALT)
			{
				char path[MAX_PATH_LEN], fname[MAX_FILENAME_LEN];
				WindInfo *newwp;
				
				if (!GetBaseName (fwp->path, fname, MAX_FILENAME_LEN))
					fname[0] = '\0';

				strcpy (path, fwp->path);
				StripFolderName (path);
				
				newwp = FileWindowOpen (path, fwp->label, path, 
					fwp->wildcard, 0);
				
				if (newwp && strlen (fname))
				{
					StringSelect (newwp, fname, FALSE);
				}
			}
			else
			{
				((FileWindow_t *)
					wp->kind_info)->flags.go_to_parent = 1;
				WindowClose (wp->handle, 0);
			}
		}
		
		GrafMouse (ARROW, NULL);
	}
	else
	{
		/* A normal file in a file window was opened.
		 * On Alternate we look for the corresponding application
		 * and try to open a file window for the application.
		 * Otherwise we just start the file.
		 */
		if ((kstate & KSTATE_MASK) == K_ALT)
		{
			char path[MAX_PATH_LEN], applname[MAX_FILENAME_LEN];
			char label[MAX_FILENAME_LEN];
			word dummy;
			WindInfo *newwp;
				
			if (!GetApplForData (pf->name, path, label, 
				kstate & (K_LSHIFT|K_RSHIFT), 0, &dummy))
			{
				venusInfo (NlsStr (T_NOAPPL), pf->name);
				return;
			}

			GetBaseName (path, applname, MAX_FILENAME_LEN);
			StripFileName (path);

			newwp = FileWindowOpen (path, label, path, "*", 0);
			if (newwp)
				StringSelect (newwp, applname, 0);
				
		}
		else
		{
			StartFile (wp, fobj, FALSE, fwp->label, fwp->path,
					pf->name, NULL, kstate, &retcode);
		}
	}
}


int getNewLocation (char **fullname, word isfolder, char *drive_label,
		int *cancelled)
{
	char tmp[1024];
	word retcode;
	
	*cancelled = FALSE;
	
	sprintf (tmp, NlsStr (T_LOCATION), *fullname);

	retcode = DialAlert (ImSqExclamation (), tmp, 0, 
				NlsStr (T_LOCBUTTON));
	
	if (retcode == 2)		/* Abbruch */
	{
		*cancelled = TRUE;
		return TRUE;
	}
	else if (retcode == 0)	/* Entfernen */
		return FALSE;
	else					/* Suchen */
	{
		word ok;
		char filename[MAX_FILENAME_LEN];
		
		strcpy (tmp, *fullname);
		
		if (isfolder)
		{
			StripFolderName (tmp);
			strcpy (filename, "*.*");
		}
		else
		{
			GetBaseName (tmp, filename, MAX_FILENAME_LEN);
			StripFileName (tmp);
		}
		AddFileName (tmp, "*.*");
		
		if (GotGEM14 ())
		{
			word tnum;
			MFORM tform;
			
			GrafGetForm (&tnum, &tform);
			GrafMouse (ARROW, NULL);
			
			retcode = fsel_exinput (tmp, filename, &ok, 
						(char *)NlsStr (T_LOCOBJ));

			GrafMouse (tnum, &tform);
		}
		else
			retcode = fsel_input (tmp, filename, &ok);
	
		if (retcode && ok)
		{
			StripFileName (tmp);
			if (!isfolder)
				AddFileName (tmp, filename);
			
			if (strlen (tmp) > MAX_PATH_LEN)
				venusErr (NlsStr (T_NESTED));
			else
			{
				char *new;
				
				new = StrDup (pMainAllocInfo, tmp);
				if (new == NULL)
					return FALSE;
				free (*fullname);
				*fullname = new;
				ReadLabel ((*fullname)[0], drive_label, MAX_FILENAME_LEN);
			}
		}
		return TRUE;
	}
}


int CheckInteractiveForAccess (char **name, char *label, int isFolder,
		int objnr, int *cancelled)
{
	int legal_drive = LegalDrive ((*name)[0]);
	int broken = FALSE;

	*cancelled = FALSE;
		
	if (!legal_drive || checkLabel ((*name)[0], label))
	{
		if (!legal_drive || !access (*name, A_EXIST, &broken))
		{
			if (getNewLocation (name, isFolder, label, cancelled))
				RehashDeskIcon();
			else
			{
				redrawObj (&wnull, objnr);
				RemoveDeskIcon (objnr);
				flushredraw ();
				return FALSE;
			}
		}
	}
	
	return TRUE;
}

void iconDclick (WindInfo *wp, word fobj, word kstate)
{
	IconInfo *pi;
	word retcode;
	char title[MAX_PATH_LEN] = "";
	char fname[MAX_FILENAME_LEN], path[MAX_PATH_LEN];
	char selname[MAX_FILENAME_LEN] = "";

	if (wp->kind == WK_DESK)
	{
		DeskInfo *dip = wp->kind_info;
		
		DeselectAllObjects ();
		SelectObject (wp, fobj, TRUE);
		flushredraw ();
		
		GrafMouse (HOURGLASS, NULL);

		pi = GetIconInfo (&dip->tree[fobj]);
		if (!pi)
			return;
			
		switch (pi->type)
		{
			case DI_SHREDDER:
				venusInfo(NlsStr(T_SHREDDER));
				return;

			case DI_FOLDER:
			{
				int cancelled;
				
				if (!CheckInteractiveForAccess (&pi->fullname, 
						pi->label, TRUE, fobj, &cancelled) || cancelled)
				{
					*path = '\0';
					break;
				}
			}

			case DI_SCRAPDIR:
			case DI_TRASHCAN:
				if ((kstate & KSTATE_MASK) == K_ALT)
				{
					strcpy (title, pi->fullname);
					StripFolderName (title);
					strcpy (path,title);
					GetBaseName (pi->fullname, selname, MAX_FILENAME_LEN);
				}
				else
				{
					strcpy (title, pi->iconname);
					strcat (title, "\\");
					strcpy (path, pi->fullname);
				}
				break;

			case DI_FILESYSTEM:
				if (!LegalDrive (pi->fullname[0]))
				{
					venusErr (NlsStr (T_NODRIVE), pi->fullname[0]);
					return;
				}
				pi->fullname[1] = '\0';
				strcat (pi->fullname, ":\\");
				strcpy (path, pi->fullname);
				
				if (E_OK != DriveAccessible (pi->fullname[0], pi->label, MAX_FILENAME_LEN))
				{
					venusErr (NlsStr (T_NOLABEL), path[0]);
					*path = '\0';
				}
				break;
				
			case DI_PROGRAM:
			{
				int cancelled;
				
				if (!CheckInteractiveForAccess (&pi->fullname, 
						pi->label, FALSE, fobj, &cancelled) || cancelled)
				{
					*path = '\0';
					break;
				}

				strcpy (path, pi->fullname);
				GetBaseName (path, fname, MAX_FILENAME_LEN);
				StripFileName (path);
				
				if ((kstate & KSTATE_MASK) == K_ALT)
				{
					strcpy (title, path);
					strcpy (selname, fname);
				}
				else	/* start the program */
				{
					StartFile (wp, fobj, TRUE, pi->label, path,
							fname, NULL, kstate, &retcode);
					return;
				}
				break;
			}

			default:
				venusDebug("unknown icon.");
				break;
		}

		if(strlen(path))
		{
			WindInfo *newwp;
			
			newwp = FileWindowOpen (path, pi->label, title, "*", 0);
				
			if (newwp && strlen(selname))
			{
				StringSelect (newwp, selname, FALSE);
			}
		}
		GrafMouse(ARROW,NULL);
	}
	else
		Dclickon (wp, fobj, kstate);
}



/*
 * manage doubleclick on position mx,my
 */
void doDclick (WindInfo *wp, OBJECT *tree, word found_object, 
	word kstate)
{
	if ((tree == NULL) || (found_object <= 0))
		return;
			
	if (ClickInfo (wp, found_object, kstate))
		return;
			
	switch (tree[found_object].ob_type & 0xFF)
	{
		case G_ICON:
		case G_USERDEF:
			iconDclick (wp, found_object, kstate);
			break;
	}
	
	GrafMouse (ARROW, NULL);
}


/*
 * simulate a doubleclick; called by selecting open
 */
void simDclick (word kstate)
{
	WindInfo *wp;
	OBJECT *tree = NULL;
	word objnr;
	
	kstate &= ~K_CTRL;
	
	if (!GetOnlySelectedObject (&wp, &objnr) || (objnr <= 0))
		return;

	if (wp->window_get_tree)
	{
		tree = wp->window_get_tree (wp);
	
		if (tree == NULL)
			return;
			
		switch (tree[objnr].ob_type & 0xFF)
		{
			case G_ICON:
			case G_USERDEF:
				iconDclick (wp, objnr, kstate);
				break;
		}
			
		GrafMouse (ARROW, NULL);
	}
}