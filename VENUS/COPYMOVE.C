/*
 * @(#) Gemini\Copymove.c
 * @(#) Stefan Eissing, 20. November 1994
 *
 * description: functions to copy/move files
 *
 * jr 22.4.95
 */

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "..\common\walkdir.h"
#include "..\common\strerror.h"

#include "vs.h"
#include "namebox.rh"
#include "copybox.rh"
#include "renambox.rh"

#include "window.h"
#include "basecopy.h"
#include "copymove.h"
#include "cpintern.h"
#include "cpkobold.h"
#include "fileutil.h"
#include "filewind.h"
#include "iconinst.h"
#include "myalloc.h"
#include "select.h"
#include "undo.h"
#include "util.h"
#include "venuserr.h"



/* externals
 */ 
extern OBJECT *prenamebox,*pcopybox,*pmovebox,*pnamebox;


/* internal texts
 */
#define NlsLocalSection "G.copymove"
enum NlsLocalText{
T_NODELFOLDER,	/*Konnte den Ordner %s nicht lîschen! 
Wahrscheinlich enthÑlt er noch Dateien.*/
T_USERABORT,	/*Wollen Sie die Operation wirklich abbrechen?*/
T_MEMABORT,		/*Die Operation muû aus Speichermangel 
abgebrochen werden.*/
T_ERRABORT,		/*Kann die Datei %s nicht %s! Wollen Sie 
die gesamte Operation abbrechen?*/
T_COPY,			/*kopieren*/
T_MOVE,			/*verschieben*/
T_RECURSION,	/*Das %s von %s nach %s wÅrde zu einer Endlos
schleife fÅhren! Die Operation wird daher abgebrochen.*/
T_COPIED,		/*Kopieren*/
T_MOVED,		/*Verschieben*/
T_SHREDDER,		/*Der Reiûwolf ist immer leer. Wenn sie gelîschte
 Dateien wiederfinden wollen, mÅssen Sie im Papierkorb suchen.*/
T_NOFOLDER,		/*Kann Dateien/Ordner nicht %s!
 Das Verzeichnis %s existiert nicht.*/
T_NOWORK,		/*Irgendwie gibt es hier nichts zu %s.*/
T_FOLDERR,		/*Kann den Ordner %s nicht anlegen!*/
T_NO_FOLDER_SPACE,	/*Kann den Ordner %s nicht anlegen, da das 
Laufwerk %c: keinen freien Speicherplatz mehr hat!*/
T_NOTFOUND,		/*Die angewÑhlten Objekte konnten nicht gefunden 
werden! PrÅfen Sie bitte, ob Sie das richtige Medium eingelegt haben.*/
T_NONAME,	/*Sie sollten einen Namen angeben!*/
T_INVALID,	/*Dies ist kein gÅltiger Name fÅr eine Datei 
oder einen Ordner! Bitte wÑhlen Sie einen anderen Namen.*/
T_DIRONFILE,	/*Es existiert schon eine Datei gleichen Namens.
 Diese sollte nicht durch einen Ordner Åberschrieben werden.
 Sie mÅssen einen anderen Namen wÑhlen.*/
T_NOINFO,	/*Die Informationen Åber `%s' konnten nicht ermittelt
 werden! Die Operation wird abgebrochen.*/
T_SAMEFILE,	/*Sie versuchen eine Datei auf sich selbst zu kopieren;
 dabei wÅrde die Datei zerstîrt. WÑhlen Sie bitte einen anderen Namen.*/
T_FILEONDIR, /*Ein Ordner gleichen Namens existiert schon. Dieser kann
 nicht durch die Datei Åberschrieben werden. WÑhlen Sie bitte einen
 anderen Namen.*/
T_CREATEFOLDER,	/*Lege neuen Ordner an:*/
T_DELFOLDER,	/*Entferne Ordner:*/
T_ILLNAME,		/*Dateiname in %s nicht ermittelbar!*/
T_OBJECTS,		/*die Objekte*/
T_DEL_MOVED,	/*Sollen alle bereits verschobenen Dateien gelîscht
 werden?*/
T_NAMETOOLONG	/*Die Namen sind zu lang, bzw. die Ordner sind zu tief
verschachtelt um von Gemini kopiert zu werden.*/
};


/* internals
 */
static BASECOPYINFO Base;
static jmp_buf jumpenv;
static DIALINFO d;
static uword foldcount, filecount;
static word writeOver, tomove;
static int do_rename;
static long bytestodo;
static word wasError;
static delete_moved_files;

#define EMPTY_FILENAME	""


/*
 * get other filename (with dialog and other stuff),
 * return new name in 'name' and state of 'Cancel'-Button.
 * 'tpath' is the full target name.
 */
static word getOtherName (char *name, int same_file, 
	int source_is_dir, int target_is_dir, const char *tpath)
{
	DIALINFO d;
	word ok;
	char newname[MAX_PATH_LEN], oldname[MAX_PATH_LEN];
	int done, do_overwrite, do_abort, retcode;
	int redraw;
	word gemdosNaming;

	/* jr: filename format stuff */
	{
		long ret = Dpathconf ((char *)tpath, 5);
		
		gemdosNaming = (ret < 0) || (ret == 2);

		SelectFrom2EditFields (pnamebox, NNEWNAM1, NNEWNAM2,
			gemdosNaming);
		SelectFrom2EditFields (pnamebox, NOLDNAM1, NOLDNAM2,
			gemdosNaming);

		pnamebox[NOLDNAM1].ob_flags &= ~EDITABLE;
		pnamebox[NOLDNAM2].ob_flags &= ~EDITABLE;

		pnamebox[NOLDNAM2].ob_y = pnamebox[NOLDNAM1].ob_y;
		pnamebox[NNEWNAM2].ob_y = pnamebox[NNEWNAM1].ob_y;

		if (gemdosNaming)
		{
			pnamebox[NOLDNAM1].ob_spec.tedinfo->te_ptext = oldname;
			strcpy (oldname, name);
			makeEditName (oldname);
	
			pnamebox[NNEWNAM1].ob_spec.tedinfo->te_ptext = newname;
			strcpy (newname, name);
			makeEditName (newname);
			
			pnamebox[NOLDTEXT].ob_y = pnamebox[NOLDNAM1].ob_y;
			pnamebox[NOLDTEXT].ob_x = pnamebox[NOLDNAM1].ob_x +
				pnamebox[NOLDNAM1].ob_width + HandXSize;
				
			pnamebox[NNEWTEXT].ob_x = pnamebox[NNEWNAM1].ob_x +
				pnamebox[NNEWNAM1].ob_width + HandXSize;
		}
		else
		{
			long maxlen = Dpathconf ((char *)tpath, 3);
			char *cp = strrchr (name, '\\');
			if (!cp) cp = strrchr (name, ':');
			if (!cp) cp = name;

			AdjustTextObjectWidth (pnamebox + NOLDNAM2, maxlen);
			maxlen = AdjustTextObjectWidth (pnamebox + NNEWNAM2, maxlen);

			pnamebox[NOLDNAM2].ob_spec.tedinfo->te_ptext = oldname;
			strncpy (oldname, cp, maxlen);
			oldname[maxlen] = '\0';

			pnamebox[NNEWNAM2].ob_spec.tedinfo->te_ptext = newname;
			strncpy (newname, cp, maxlen);
			newname[maxlen] = '\0';

			pnamebox[NOLDTEXT].ob_y = pnamebox[NOLDNAM1].ob_y + HandYSize;
			pnamebox[NOLDTEXT].ob_x = pnamebox[NOLDNAM1].ob_x;

			pnamebox[NNEWTEXT].ob_x = pnamebox[NNEWNAM2].ob_x +
				((int)maxlen + 1) * HandXSize;
		}
	}		
	
	setDisabled (pnamebox, NOVER, same_file 
		|| (target_is_dir ^ source_is_dir));
	
	DialCenter (pnamebox);
	redraw = !DialStart (pnamebox, &d);
	DialDraw (&d);
	
	done = FALSE;
	retcode = FALSE;
	
	while (!done)
	{
		do_overwrite = writeOver;
		do_abort = FALSE;
		
		ok = DialDo (&d, 0) & 0x7FFF;
		
		switch (ok)
		{
			case NOVER:					/* overwrite all */
				do_overwrite = TRUE;
			case NOK:					/* take new name */
				strcpy (name, newname);
				
				if (gemdosNaming)
					makeFullName (name);
	
				if (!strcmp (name, EMPTY_FILENAME) 
					|| !strlen (name))
				{
					venusErr (NlsStr (T_NONAME));
					break;
				}

				if (!ValidFileName (name))
				{
					venusErr (NlsStr (T_INVALID));
					break;
				}
	
				retcode = TRUE;
				done = TRUE;
				break;
				
			case NABORT:
				do_abort = TRUE;
			default:					/* skip this file */
				retcode = FALSE;
				done = TRUE;
				break;
		}

		setSelected (pnamebox, ok, FALSE);
		fulldraw (pnamebox, ok);
	
	}

	DialEnd (&d);
	
	if (do_abort)
		longjmp(jumpenv,2);		/* User aborts copy/move */
	
	if (do_overwrite && !writeOver)
	{
		/* Der Benutzer hat auf öberschreiben gestellt. Stelle
		 * dies auch im Dialog dar.
		 */
		setSelected (pcopybox, CPOVERWR, FALSE);
		fulldraw (pcopybox, CPOVERWR);
	}
	writeOver = do_overwrite;
	
	if (redraw)
		fulldraw (pcopybox, 0);
		
	return retcode;
}

/*
 * update the numbers in the dialog box if todraw is true
 */
static void updateCount (int todraw)
{
/*	if (NewDesk.silentCopy)
		return;
*/	
	sprintf (pcopybox[CPFOLDNR].ob_spec.tedinfo->te_ptext,
			"%5u", foldcount);
	sprintf (pcopybox[CPFILENR].ob_spec.tedinfo->te_ptext,
			"%5u", filecount);
	SetFileSizeField (pcopybox[CPBYTES].ob_spec.tedinfo, bytestodo);

	if (todraw)
	{
		fulldraw (pcopybox, CPFOLDNR);
		fulldraw (pcopybox, CPFILENR);
		fulldraw (pcopybox, CPBYTES);
	}
}

/*
 * Display the copy dialog and return wether the user wants
 * to copy the stuff or not.
 */
static word askForCopy (void)
{
	word retcode;
	
/*	if (NewDesk.silentCopy)
		return TRUE;
*/	
	GrafMouse (ARROW, NULL);
	strcpy (pcopybox[CPFILESC].ob_spec.free_string, 
		EMPTY_FILENAME);
	strcpy (pcopybox[CPFILEDS].ob_spec.free_string, 
		EMPTY_FILENAME);
	setDisabled (pcopybox, CPABORT, TRUE);
	setHideTree (pcopybox, CPTITLE, tomove);
	setHideTree (pcopybox, MVTITLE, !tomove);
	setSelected (pcopybox, CPOVERWR, !writeOver);
	setSelected (pcopybox, CPRENAME, do_rename);
	updateCount (FALSE);
	
	DialCenter (pcopybox);
	DialStart (pcopybox, &d);
	DialDraw (&d);
	
	if (NewDesk.silentCopy)
	{
		retcode = CPOK;
		setSelected (pcopybox, CPOK, TRUE);
		fulldraw (pcopybox, CPOK);
	}
	else
		retcode = DialDo (&d, 0) & 0x7FFF;
	
	setSelected (pcopybox, retcode, FALSE);
	if (retcode == CPOK)
	{
		setDisabled (pcopybox, CPABORT, FALSE);
		writeOver = !isSelected (pcopybox, CPOVERWR);
		do_rename = isSelected (pcopybox, CPRENAME);
		fulldraw (pcopybox, CPABORT);
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
/*	if (!NewDesk.silentCopy)
*/		DialEnd (&d);
}


void DispName (OBJECT *tree, word obj, const char *name)
{
	size_t len;
#define TEXT_FIELD_MIN_LEN	30

	len = strlen (name);
	if (len <= TEXT_FIELD_MIN_LEN)
	{
		if (!strcmp (tree[obj].ob_spec.free_string, name))
			return;
		strcpy (tree[obj].ob_spec.free_string, name);
	}
	else
	{
		strncpy (tree[obj].ob_spec.free_string, name, 3);
		strcpy (&tree[obj].ob_spec.free_string[3], "...");
		strcat (tree[obj].ob_spec.free_string, &name[len - 
			(TEXT_FIELD_MIN_LEN - 6)]);
	}

	{
		word obx, oby;
		
		objc_offset (tree, obj, &obx, &oby);
		ObjcDraw (tree, 0, MAX_DEPTH, obx, oby,
			tree[obj].ob_width, tree[obj].ob_height);
	}
}

/*
 * Display the file name in the dialog box
 */
void DisplayFileName (const char *src_name, const char *dest_name)
{
/*	if (NewDesk.silentCopy)
		return;
*/	
	DispName (pcopybox, CPFILESC, src_name);
	DispName (pcopybox, CPFILEDS, dest_name);
}


void CheckUserBreak (void)
{
	if (escapeKeyPressed ())
	{
		if (venusChoice (NlsStr (T_USERABORT)))
		{
			longjmp (jumpenv, 2);
		}
		else
			GrafMouse (HOURGLASS, NULL);
	}
}

void AskForAbort (int ask_for_abort, const char *filename)
{
	if (!ask_for_abort)
		longjmp (jumpenv, 1);
	
	if ((filecount > 1 || foldcount) 
		&& venusChoice (NlsStr (T_ERRABORT), filename,
		 tomove? NlsStr (T_MOVE) : NlsStr (T_COPY)))
	{
		longjmp (jumpenv, 1);
	}
	else
	{
		GrafMouse (HOURGLASS, NULL);
		wasError = TRUE;
	}
}


void FileWasCopied (long size)
{
	--filecount;
	bytestodo -= size;
	updateCount (TRUE);
}

/* jr: Ask for a new namefor the given file. name is the basename
   of tpath */

static int askForNewName (char name[MAX_PATH_LEN], const char *tpath)
{
	DIALINFO d;
	word retcode;
	char oldname[MAX_PATH_LEN], newname[MAX_PATH_LEN];
	MFORM tmp_form;
	word tmp_num, redraw;
	word gemdosNaming;
	long ret;
	
	ret = Dpathconf ((char *)tpath, 5);
	gemdosNaming = ret < 0 || ret == 2;
	
	SelectFrom2EditFields (prenamebox, ROLDNAM1, ROLDNAM2, gemdosNaming);
	SelectFrom2EditFields (prenamebox, RNEWNAM1, RNEWNAM2, gemdosNaming);

	prenamebox[ROLDNAM1].ob_flags &= ~EDITABLE;
	prenamebox[ROLDNAM2].ob_flags &= ~EDITABLE;
	
	if (gemdosNaming)
	{
		strcpy (newname, name);
		makeEditName (newname);
		strcpy (oldname, newname);
		prenamebox[ROLDNAM1].ob_spec.tedinfo->te_ptext = oldname;
		prenamebox[RNEWNAM1].ob_spec.tedinfo->te_ptext = newname;
	}
	else
	{
		long maxlen = Dpathconf ((char *)tpath, 3);
		char *cp = strrchr (name, '\\');
		if (!cp) cp = strrchr (name, ':');
		if (!cp) cp = name;
			
		AdjustTextObjectWidth (prenamebox + ROLDNAM2, maxlen);
		maxlen = AdjustTextObjectWidth (prenamebox + RNEWNAM2, maxlen);

		strncpy (newname, cp, maxlen);
		newname[maxlen] = '\0';
		strcpy (oldname, newname);
		prenamebox[ROLDNAM2].ob_spec.tedinfo->te_ptext = oldname;
		prenamebox[RNEWNAM2].ob_spec.tedinfo->te_ptext = newname;
	}
	
	GrafGetForm (&tmp_num, &tmp_form);
	GrafMouse (ARROW, NULL);
	
	DialCenter (prenamebox);
	redraw = !DialStart (prenamebox, &d);
	DialDraw (&d);

	retcode = DialDo (&d, 0) & 0x7FFF;
	setSelected (prenamebox, retcode, FALSE);			
	
	DialEnd (&d);

	GrafMouse (tmp_num, &tmp_form);
	
	if (retcode == RENOK)
	{
		if (gemdosNaming) makeFullName (newname);
		strcpy (name, newname);
	}
	
	if (redraw) fulldraw (pcopybox, 0);
		
	return retcode == RENOK;
}


/*
 * Tough to understand: we need to prevent an overwrite
 * of existing files if writeover is false. But we should
 * never copy a file onto itself.
 */
int CheckValidTargetFile (char *fpath, char *tpath, int do_rename)
{
	word ok = TRUE;
	int broken;
	char name[MAX_PATH_LEN], oldname[MAX_PATH_LEN];
	int target_is_dir, same_file, target_exists;
	unsigned short date, time;
	short attrib;
	long size;
	
	if (!GetBaseName (tpath, name, MAX_PATH_LEN))
		return FALSE;
	
	/* Wenn wir umbennen, dann frage nach einem neuen Namen
	 */
	if (do_rename)
	{
		if (!askForNewName (name, tpath)) return FALSE;
		
		StripFileName (tpath);
		AddFileName (tpath, name);
	}
	
	/* öberschreiben und verschiedene Dateien und Ziel kein
	 * Verzeichnis?
	 */
	same_file = !stricmp (fpath, tpath);
	target_exists = E_OK == GetFileInformation (tpath, &size,
		&attrib, &date, &time, &broken);
	target_is_dir = (target_exists && (attrib & FA_SUBDIR));	
	if (writeOver && !same_file	&& !target_is_dir)
		return TRUE;
		
	if (!GetBaseName (tpath, name, MAX_PATH_LEN))
		return FALSE;
	
	/* Solange wie die Zieldatei schon existiert und der User
	 * auf OK gedrÅckt hat...
	 */
	while (target_exists && ok)
	{
		GrafMouse (ARROW, NULL);
		
		/* merke dir den alten Namen
		 */
		strcpy (oldname, name);
		
		/* Dialog zum öberspringen, OK, Abbruch, Erstzen.
		 * ergibt nur TRUE bei Ersetzen und OK.
		 */
		ok = getOtherName (name, same_file, FALSE, target_is_dir, tpath);
		GrafMouse (HOURGLASS, NULL);
		
		/* Bastele den neuen Zielpfad
		 */
		StripFileName (tpath);
		AddFileName (tpath, name);
		
		/* Hier sind wir entweder beim öberschreiben oder der Name
		 * ist gleich geblieben. Dann brauchen wir nur noch zu
		 * testen, ob Quelle und Ziel verschieden sind
		 */
		same_file = !stricmp (fpath, tpath);
		target_exists = E_OK == GetFileInformation (tpath, &size,
			&attrib, &date, &time, &broken);
		target_is_dir = (target_exists && (attrib & FA_SUBDIR));	

		/* nicht öberschreiben und neuer Namen -> neu ÅberprÅfen
		 */
		if (!writeOver && stricmp (oldname, name))
			continue;
		
		if (ok)
		{
			if (same_file)
				venusErr (NlsStr (T_SAMEFILE));
			else if (target_is_dir)
				venusErr (NlsStr (T_FILEONDIR));
			else
				break;
		}
	}
	
	return ok;
}

/*
 * Make the folder tpath; if its already there ask the
 * user wether he wants to use it or what the heck he
 * wants at all.
 */
static word makeValidTargetFolder (char *tpath)
{
	word ok = FALSE;
	int broken;
	char name[MAX_PATH_LEN], oldname[MAX_PATH_LEN];
	
	if (!GetBaseName (tpath, name, MAX_PATH_LEN))
		return FALSE;
	
	StripFolderName (tpath);
	AddFileName (tpath, name);
	
	/* Wenn wir umbennen, dann frage nach einem neuen Namen
	 */
	if (do_rename)
	{
		if (!askForNewName (name, tpath)) return FALSE;
		
		StripFileName (tpath);
		AddFileName (tpath, name);
	}
	
	while (TRUE)
	{
		int target_exists, target_is_dir;
		unsigned short date, time;
		short attrib;
		long size;

		target_exists = E_OK == GetFileInformation (tpath, &size,
			&attrib, &date, &time, &broken);
		target_is_dir = (target_exists && (attrib & FA_SUBDIR));	
		
		/* Wenn der Ordner existiert und wir Åberschreiben, ist
		 * alles in Ordnung.
		 */
		if (target_is_dir && writeOver)
		{
			StripFileName (tpath);
			AddFolderName (tpath, name);
			return TRUE;
		}
		
		/* Wenn nichts dergleichen existiert und das Anlegen des
		 * Ordners schiefgeht, zeigen wir eine Fehlermeldung und
		 * geben auf.
		 */
		if (!target_exists)
		{
			DISKINFO di;
			const char *err;
			
			DisplayFileName (NlsStr (T_CREATEFOLDER), tpath);
			if (Dcreate (tpath) == 0)
			{
				StripFileName (tpath);
				AddFolderName (tpath, name);
				return TRUE;
			}
				
			if (!Dfree (&di, tpath[0] - 'A' + 1) && di.b_free == 0L)
				err = NlsStr (T_NO_FOLDER_SPACE);
			else
				err = NlsStr (T_FOLDERR);
				
			venusErr (err, tpath, tpath[0]);
			longjmp (jumpenv, 2);
		}
		
		/* Wenn wir hier ankommen, so existiert ein Objekt gleichen
		 * Namens und wir geben dem Benutzer die Gelegenheit, einen
		 * neuen Namen zu wÑhlen.
		 */
		while (TRUE)
		{
			strcpy (oldname, name);

			GrafMouse (ARROW, NULL);
			ok = getOtherName (name, FALSE, TRUE, target_is_dir, tpath);
			GrafMouse (HOURGLASS, NULL);
			
			StripFileName (tpath);
			AddFileName (tpath, name);

			/* Wenn eine Datei gleichen Namens existiert, dann *muû*
			 * ein neuer Name vergeben werden.
			 */
			if (ok && !target_is_dir && !stricmp (oldname, name))
				venusErr (NlsStr (T_DIRONFILE));
			else
				break;
		}
		
		/* Wenn öberspringen gewÑhlt wurde, oder in einen 
		 * existierenden Ordner kopiert werden soll, verlasse diese
		 * Schleife.
		 */
		if (!ok || (target_is_dir && !stricmp (oldname, name)))
			break;
	}
	
	StripFileName (tpath);
	AddFolderName (tpath, name);
	return ok;
}

/*
 * While moving files/folders we need to delete a moved folder
 * afterwards.
 */
static void cleanUpFolder (char *name, char *fpath, char *tpath)
{
	FlushFilesInBuffer (&Base);

	if (!tomove)
		return;
	
	FileWasMoved (fpath, tpath);
	UndoFolderVanished (fpath);
	
	StripFolderName (fpath);
	AddFileName (fpath, name);
	DisplayFileName (NlsStr (T_DELFOLDER), fpath);
	if (Ddelete (fpath) < 0 && !wasError)
	{
		venusErr (NlsStr (T_NODELFOLDER), fpath);
		GrafMouse (HOURGLASS, NULL);
		wasError = TRUE;
	}
	
	StripFileName (fpath);
	AddFolderName (fpath, name);
}


/*
 * kopiere alle Objekte vom Ordner fpath in den Ordner tpath
 */
static void cpInsideFolder (char *fpath, char *tpath)
{
	WalkInfo W;
	char filename[MAX_PATH_LEN];
	int old_do_rename;
	size_t maxlen;
	long stat;
	static void cpFolder(char *fpath, char *tpath);
	
	CheckUserBreak ();

	{
		size_t tmp;
		
		maxlen = MAX_PATH_LEN - strlen (fpath);
		tmp = MAX_PATH_LEN - strlen (tpath);
		if (tmp < maxlen)
			maxlen = tmp;
		
		if (maxlen < 1)
		{
			venusErr (NlsStr (T_NAMETOOLONG));
			longjmp (jumpenv, 2);
		}
	}
	
	old_do_rename = do_rename;
	do_rename = FALSE;

	stat = InitDirWalk (&W, WDF_ATTRIBUTES|WDF_GEMINI_LOWER, fpath, maxlen, filename);

	if (!stat)
	{
		while ((stat = DoDirWalk (&W)) == 0L)
		{
			if (!strcmp (filename, ".")
				|| !strcmp (filename, ".."))
				continue;
					
			AddFileName (fpath, filename);

			if (W.xattr.attr & FA_FOLDER)
			{
				cpFolder (fpath, tpath);
			}
			else if (!(W.xattr.attr & FA_LABEL))
			{
				BaseCopyFile (&Base, fpath, tpath, 
					W.xattr.size, W.xattr.attr, W.xattr.mdate,
					W.xattr.mtime, do_rename);
			}
			
			StripFileName (fpath);
		}
		
		if (stat == ENAMETOOLONG)
		{
			venusErr (NlsStr (T_NAMETOOLONG));
			longjmp (jumpenv, 2);
		}
		
		ExitDirWalk (&W);
	}
		
	do_rename = old_do_rename;
}

/*
 * kopiere den Ordner name im Ordner fpath in den Ordner tpath.
 * name wird dabei im Ordner tpath angelegt.
 */
static void cpFolder (char *fpath, char *tpath)
{
	char name[MAX_PATH_LEN];
	
	if (!GetBaseName (fpath, name, MAX_PATH_LEN))
	{
		venusErr (NlsStr (T_ILLNAME), fpath);
		AskForAbort (TRUE, fpath);
		return;
	}

	if (IsPathPrefix (tpath, fpath))	/* Fano condition */
	{
		venusErr (NlsStr (T_RECURSION), 
				tomove? NlsStr (T_MOVED) : NlsStr (T_COPIED), 
				fpath, tpath);
		longjmp (jumpenv, 2);
	}
	
	AddFileName (tpath, name);
	FlushFilesInBuffer (&Base);
	
	if (makeValidTargetFolder (tpath))
	{
		cpInsideFolder (fpath, tpath);
		cleanUpFolder (name, fpath, tpath);
	}
	else
	{
		uword fileanz = 0,foldanz = 0;
		long size = 0L;
		
		if (AddFileAnz (fpath, TRUE, -1L, &foldanz, &fileanz, &size))
		{
			bytestodo -= size;
			foldcount -= foldanz;
			filecount -= fileanz;
		}
	}
	--foldcount;
	updateCount (TRUE);
	StripFileName (tpath);
}


/* Entferne unnîtige \\ am Ende von Dateinamen, jedoch
 * nicht bei Laufwerken.
 */
void ArgvCheckNames (ARGVINFO *A)
{
	int i;
	
	for (i = 0; i < A->argc; ++i)
	{
		size_t len = strlen (A->argv[i]);
		
		if (len && A->argv[i][len-1] == '\\')
		{
			if ((len != 3) || (A->argv[i][1] != ':'))
			{
				((char *)A->argv[i])[len-1] = '\0';
			}
		}
	}
}

int CopyArgv (ARGVINFO *A, char *tpath, int move, int rename)
{
	DTA *saveDta;
	char *savetpath;
	char path[MAX_PATH_LEN];
	long max_objects;

	tomove = move;
	do_rename = rename;
	writeOver = NewDesk.replaceExisting;
	wasError = FALSE;
	delete_moved_files = FALSE;
	
	max_objects = -1L;
	if (NewDesk.useCopyKobold && (NewDesk.minCopyKobold > 0)
		&& KoboldPresent ())
	{
		max_objects = NewDesk.minCopyKobold;
	}
	
	GrafMouse (HOURGLASS, NULL);
	ArgvCheckNames (A);
	if (!CountArgvFolderAndFiles (A, TRUE, max_objects, 
		&foldcount, &filecount, &bytestodo))
	{
		GrafMouse (ARROW, NULL);
		return FALSE;
	}
	
	if (filecount == 0 && foldcount == 0)
	{
		venusErr (NlsStr (T_NOWORK),
			tomove? NlsStr (T_MOVE) : NlsStr (T_COPY));
		GrafMouse (ARROW, NULL);
		return FALSE;
	}
	
	if (bytestodo < 0)
	{
		venusErr (NlsStr (T_NOTFOUND));
		GrafMouse (ARROW, NULL);
		return FALSE;
	}

	if ((max_objects > 0) && ((foldcount + filecount) >= max_objects))
	{
		GrafMouse (ARROW, NULL);
		return CopyWithKobold (A, tpath, move? kb_move : kb_copy,
			!NewDesk.silentCopy, NewDesk.replaceExisting, rename);
	}

	if (!InitBaseCopy (&Base, move))
	{
		venusErr (NlsStr (T_MEMABORT));
		GrafMouse (ARROW, NULL);
		return FALSE;
	}

	if (!askForCopy ())
	{
		ExitBaseCopy (&Base, FALSE);
		EndDialog ();
		return FALSE;
	}
	
	savetpath = StrDup (pMainAllocInfo, tpath);

	saveDta = Fgetdta ();
	if (setjmp (jumpenv) == 0)
	{
		long fsize;
		unsigned short date, time;
		short attr;
		int i, drive;
		int broken = FALSE;
		
		for (i = 0; (!broken) && (i < A->argc); ++i)
		{
			long filestat;
			
			strcpy (path, A->argv[i]);
			if (savetpath) strcpy (tpath, savetpath);
			
			drive = strlen (path) == 3 && path[1] == ':'
				&& path[2] == '\\';
			
			if (drive || E_OK ==
				(filestat = GetFileInformation (path, &fsize, &attr, 
				&date, &time, &broken)))
			{
				if (drive)
				{
					cpInsideFolder (path, tpath);
				}
				else if (attr & FA_FOLDER)
				{
					cpFolder (path, tpath);
				}
				else
				{
					BaseCopyFile (&Base, path, tpath, 
						fsize, attr, date, time, do_rename);
				}
			}
			else
			{
Bconout (2, 7);
				venusErr (NlsStr (T_NOINFO), path);
				broken = TRUE;
			}
			
			if (move)
			{
				StripFileName (path);
				BuffPathUpdate (path);
			}
		}
		
		FlushFilesInBuffer (&Base);
		BuffPathUpdate (tpath);
	}
	else
	{
		/* Die Operation wurde abgebrochen. Bei Verschieben und
		 * Dateien im Puffer fragen wir, ob diese gelîscht werden
		 * sollen.
		 */
		if (tomove && Base.first_file)
			delete_moved_files = venusChoice (NlsStr (T_DEL_MOVED));
	}
	
	if (savetpath)
	{
		strcpy (tpath, savetpath);
		free (savetpath);
	}

	BuffPathUpdate (tpath);
	ExitBaseCopy (&Base, delete_moved_files);
	RehashDeskIcon ();

	writeOver = NewDesk.replaceExisting;
	Fsetdta (saveDta);
	EndDialog ();
	FlushPathUpdate ();
	
	GrafMouse (ARROW, NULL);
	
	return TRUE;
}


/*
 * main entry for copying files/folders.
 */
int CopyDraggedObjects (WindInfo *fwp, char *tpath, 
	int move, int rename)
{
	ARGVINFO A;
	word retcode;

	if (!fwp->getDragged || !fwp->getDragged (fwp, &A, TRUE))
	{
		venusInfo (NlsStr (T_NOINFO), NlsStr (T_OBJECTS));
		GrafMouse (ARROW, NULL);
		return FALSE;
	}

	retcode = CopyArgv (&A, tpath, move, rename);
	FreeArgv (&A);
	DeselectAllExceptWindow (fwp);
	DeselectNotDraggedObjects (fwp);

	return retcode;
}


int CountArgvFolderAndFiles (ARGVINFO *A, int follow_links,
	long max_objects, uword *foldnr, uword *filenr, long *size)
{
	char oldpath[MAX_PATH_LEN];
	char path[MAX_PATH_LEN];
	int i;
	
	*size = *foldnr = *filenr = 0;
	GetFullPath (oldpath, MAX_PATH_LEN);
	
	for (i = 0; i < A->argc; ++i)
	{
		XATTR X;
		int drive;
		
		strcpy (path, A->argv[i]);
		
		drive = (strlen (path) == 3) && (path[1] == ':')
			&& (path[2] == '\\');
			
		if (drive || !GetXattr (path, FALSE, follow_links, &X))
		{
			if (drive || (X.attr & FA_FOLDER))
			{
				if (!drive)
					++(*foldnr);
				if (!AddFileAnz (path, follow_links, max_objects,
					foldnr, filenr, size))
				{
					return FALSE;
				}
			}
			else
			{
				++(*filenr);
				*size += X.size;
			}
		}
		else
		{
			venusErr (NlsStr (T_NOINFO), path);
			return FALSE;
		}
		
		if ((max_objects > 0) && ((*foldnr + *filenr) >= max_objects))
			break;
	}
	
	SetFullPath (oldpath);
	return TRUE;
}

