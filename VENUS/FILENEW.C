/*
 * @(#) Gemini\filenew.c
 * @(#) Stefan Eissing, 13. November 1994
 *
 * jr 22.4.95
 */

#include <stdio.h>
#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\argvinfo.h"
#include "..\common\genargv.h"
#include "..\common\memfile.h"
#include "..\common\strerror.h"
#include "..\common\walkdir.h"

#include "vs.h"
#include "foldbox.rh"

#include "window.h"
#include "filewind.h"
#include "filenew.h"
#include "fileutil.h"
#include "filewind.h"
#include "select.h"
#include "sortfile.h"
#include "util.h"
#include "wildcard.h"
#include "venuserr.h"

/* externals
 */
extern OBJECT *pfoldbox;


/* internal texts
 */
#define NlsLocalSection "G.filenew"
enum NlsLocalText{
T_FOLDER,	/*Der Pfad dieses Fensters ist zu lang, um neue Ordner anzulegen!*/
T_NONAME,	/*Sie sollten einen Namen angeben!*/
T_EXISTS,	/*Ein Objekt dieses Namens existiert schon.*/
T_INVALID,	/*Dies ist kein gltiger Name fr eine Datei 
oder einen Ordner! Bitte w„hlen Sie einen anderen Namen.*/
T_CANT_CREATE, /*Das Objekt konnte leider nicht angelegt werden.*/
T_NOEXEC,	/*Es k”nnen auf diesem Laufwerk nur Aliase auf
 Programme angelegt werden.*/
T_NOSELECTED,	/*Oops, ich konnte kein selektiertes Objekt finden.*/
T_CANTALIAS	/*Auf dieses Objekt kann kein Alias angelegt werden.*/
};

/* dialog for a new object, try to create it
 */
int fileWindowMakeNewObject (WindInfo *wp)
{
	DIALINFO d;
	FileWindow_t *fwp;
	word ok, retcode;
	int broken, gemdosNaming;
	long ret;
	char name[MAX_FILENAME_LEN], path[MAX_PATH_LEN];
	char tmpsave[MAX_FILENAME_LEN], *pfoldname;
	int edit_object;
	
	fwp = (FileWindow_t *)wp->kind_info;
	
	if (strlen (fwp->path) >= MAX_PATH_LEN - 32)
	{
		venusErr (NlsStr (T_FOLDER));
		return FALSE;
	}

	strcpy (path, fwp->path);
	ret = Dpathconf ((char *)path, 5);
	gemdosNaming = (ret < 0 || ret == 2);

	/* se: initialize text fields */
	{
		SelectFrom2EditFields (pfoldbox, FONAME, FONAME2,
			gemdosNaming);
		
		if (gemdosNaming)
		{
			pfoldname = pfoldbox[FONAME].ob_spec.tedinfo->te_ptext;
			edit_object = FONAME;
		}
		else
		{
			long maxlen = Dpathconf ((char *)path, 3);

			AdjustTextObjectWidth (pfoldbox + FONAME2, maxlen);

			pfoldname = pfoldbox[FONAME2].ob_spec.tedinfo->te_ptext;
			edit_object = FONAME2;
		}
	}		

	pfoldname[0] = '\0';
	strcpy (tmpsave, pfoldname);
	
	DialCenter (pfoldbox);
	DialStart (pfoldbox, &d);
	DialDraw (&d);
	
	ok = TRUE;
	while (ok)
	{
		int redraw_object, create_folder = FALSE;
		
		retcode = DialDo (&d, &edit_object) & 0x7FFF;
		redraw_object = retcode;
		
		switch (retcode)
		{
			case FOCANCEL:
				ok = FALSE;
				break;

			case FOFOLDER:
				create_folder = TRUE;
			case FOFILE:
			{
				int retcode;
				
				if((*pfoldname == '@') || !strlen (pfoldname))
				{
					venusErr (NlsStr (T_NONAME));
					break;
				}
				
				strcpy (name, pfoldname);
				if (gemdosNaming)
					makeFullName (name);
				if (!ValidFileName (name))
				{
					venusErr (NlsStr (T_INVALID));
					break;
				}
				
				GrafMouse (HOURGLASS, NULL);
				strcpy (path, fwp->path);
				AddFileName (path, name);
				
				if (create_folder)
					retcode = CreateFolder (path);
				else
					retcode = CreateFile (path);
					
				if (retcode < 0)
				{
					if (access (path, A_EXIST, &broken))
						venusErr (NlsStr (T_EXISTS));
					else
						venusErr (NlsStr (T_CANT_CREATE));
				}
				else
					ok = FALSE;

				GrafMouse (ARROW, NULL);
			}
		}

		setSelected (pfoldbox, redraw_object, FALSE);
		fulldraw (pfoldbox, redraw_object);
	}
	
	DialEnd (&d);
	
	strcpy (pfoldname, tmpsave);
	if (retcode != FOCANCEL)
		PathUpdate (fwp->path);
	
	return retcode != FOCANCEL;
}


#define ALIAS_PARAMETER	" \"$@\""
#define ALIAS_HEADER	"#!mupfel"

int fileWindowMakeAlias (WindInfo *wp)
{
	FileWindow_t *fwp = (FileWindow_t *)wp->kind_info;
	int retcode = FALSE, broken = FALSE;
	word is_executable;
	char *name, *filename;
	char aliasname[MAX_PATH_LEN + 10];
	size_t len;
	ARGVINFO A;
	
	if (!WindGetSelected (&A, FALSE) ||
		(name = Argv2String (pMainAllocInfo, &A, FALSE)) == NULL)
	{
		venusErr (NlsStr (T_NOSELECTED));
		return FALSE;
	}

	if (*name == '\0')
	{
		venusErr (NlsStr (T_CANTALIAS));
		free (name);
		return FALSE;
	}
	
	if ((filename = strchr (name, ' ')) != NULL)
		*filename = '\0';
	
	/* Verhindere, daž ein ungltiger '\\' am Ende des Namens
	 * steht.
	 */
	len = strlen (name);
	if (name[1] != ':' || len > 3)
	{
		if (name[len-1] == '\\')
			name[len-1] = '\0';
	} 

	filename = strrchr (name, '\\');
	if (filename == NULL || !filename[1])
		filename = name;
	else
		++filename;
	
	is_executable = IsExecutable (filename);
	
	strcpy (aliasname, fwp->path);
	AddFileName (aliasname, filename);

	if (!access (aliasname, A_EXIST, &broken) && !broken)
	{
		long stat;
			
		/* Symbolische Links sind m”glich?
		 */
		stat = Fsymlink (name, aliasname);
		retcode = !stat;

		if (stat == EINVFN)
		{
			if (!is_executable)
			{
				venusErr (NlsStr (T_NOEXEC));
				free (name);
				return FALSE;
			}
			else
			{
				MEMFILEINFO *mp;
				
				mp = mcreate (pMainAllocInfo, aliasname);
				if (mp != NULL)
				{
					mputs (mp, ALIAS_HEADER);
					strcpy (aliasname, name);
					strcat (aliasname, ALIAS_PARAMETER);
					mputs (mp, aliasname);
					mclose (pMainAllocInfo, mp);
					retcode = TRUE;
				}
			}
		}
		
		if (!retcode)
			venusErr (NlsStr (T_CANT_CREATE));
	}
	else
		venusErr (NlsStr (T_EXISTS));
	
	free (name);
	if (retcode)
		PathUpdate (fwp->path);
	
	return retcode;
}


int fileWindowGetDragged (WindInfo *wp, ARGVINFO *A,
	int preferLowerCase)
{
	FileWindow_t *fwp = (FileWindow_t *)wp->kind_info;
	uword i;
	FileBucket *bucket;
	char name[MAX_PATH_LEN], *filepart;

	InitArgv (A);
	if (!MakeGenericArgv (pMainAllocInfo, A, TRUE, 0, NULL))
		return FALSE;
	
	strcpy (name, fwp->path);
	filepart = &name[strlen(name)];
	bucket = fwp->files;
	
	while (bucket)
	{
		for (i = 0; i < bucket->usedcount; ++i)
		{
			if (bucket->finfo[i].flags.dragged
				& bucket->finfo[i].flags.selected)
			{
				AddFileName (name, bucket->finfo[i].name);
				if (!fwp->flags.case_sensitiv && preferLowerCase)
					strlwr (filepart);
					
				if (!StoreInArgv (A, name))
				{
					FreeArgv (A);
					return FALSE;
				}
				
				StripFileName (name);
			}
		}
	
		bucket = bucket->nextbucket;
	}

	return TRUE;
}

int fileWindowGetSelected (WindInfo *wp, ARGVINFO *A,
	int preferLowerCase)
{
	int retcode;
	
	MarkDraggedObjects (wp);
	retcode = fileWindowGetDragged (wp, A, preferLowerCase);
	UnMarkDraggedObjects (wp);
	
	return retcode;
}


/* Get the file list for a window
 */
 
/*
 * copy informations from DTA d to FileInfo structure p
 */
static int fillFileInfo (FileInfo *p, uword nummer, 
	const char *path, const char *filename, XATTR *X)
{
	p->name = StrDup (pMainAllocInfo, filename);
	if (p->name == NULL)
		return FALSE;
	
	p->flags.is_symlink = ((X->mode & S_IFMT) == S_IFLNK);

	if (p->flags.is_symlink)
	{
		XATTR x;
		long stat;
		char tmp[MAX_PATH_LEN];
		
		strcpy (tmp, path);
		AddFileName (tmp, p->name);
		stat = GetXattr (tmp, FALSE, TRUE, &x);
		if (stat >= 0)
			*X = x;
	}
	
	p->attrib = X->attr;
	p->size = X->size;
	p->date = X->mdate;
	p->time = X->mtime;
	p->magic = 'FilE';
	p->number = nummer;
	
	p->icon.flags.noObject = TRUE;
	p->icon.flags.is_folder = 
		((X->mode & S_IFMT) == S_IFDIR);
	p->icon.flags.is_symlink = p->flags.is_symlink;

	return TRUE;
}

static word hiddenCorrectly( int attr, const char *name )
{
	/* Hidden files are shown depending on NewDesk.showHidden
	 */
	if (attr & FA_HIDDEN)
		return NewDesk.showHidden;

	if (name[0] == '.')
	{
		switch (name[1])
		{
			case '.':
				return !name[2];			/* ".." is always visible */
			case '\0':
				return 0;					/* "." is never displayed */
			default:
				return NewDesk.showHidden;  /* rest depends */
		}
	}
	
	return 1;								/* all others shown */
}		

/*
 * get Files in Directory wp->path and build a linked list
 * of Fileinfo structures starting at wp->files
 */
int GetFiles (FileWindow_t *fwp)
{
	WalkInfo W;
	FileBucket *newList;
	FileBucket *aktBucket, **previousBucket;
	char buffer[MAX_PATH_LEN];
	char filename[MAX_PATH_LEN];
	long stat;
	long newDirSize;
	word len, retcode;
	uword gesamtZahl, aktIndex;
	char getallfiles = !strcmp (fwp->wildcard, "*");
	
	previousBucket = &newList;
	aktBucket = newList = NULL;
	newDirSize = 0L;
	gesamtZahl = 0;
	aktIndex = FILEBUCKETSIZE;

	ReadLabel (fwp->path[0], buffer, MAX_FILENAME_LEN);
	if(!sameLabel (buffer, fwp->label))		/* Label ge„ndert */
	{
		while (SetFullPath (fwp->path))			/* Pfad nicht setzbar? */
		{
			if (!StripFolderName (fwp->title)) 	/* cd .. machbar? */
				break;
			StripFolderName (fwp->path);
		}		
	}
	if (strcmp (fwp->label, buffer))
		strcpy (fwp->label, buffer);			/* label merken */
		
	strcpy (buffer, fwp->path);
	strcat (buffer, "*.*");		/* wildcard anh„ngen */

	if (fwp->files)
	{
		FreeFiles (fwp);		/* alte Files freigeben */
	}

	/* Defaultwerte eintragen
	 */
	fwp->flags.case_sensitiv = FALSE;
	fwp->flags.is_8and3 = TRUE;
	fwp->max_filename_len = 12;
	retcode = TRUE;
	
	stat = InitDirWalk (&W, WDF_ATTRIBUTES|WDF_IGN_XATTR_ERRORS|
		WDF_GEMINI_LOWER, fwp->path, MAX_PATH_LEN, filename);

	if (!stat)
	{
		fwp->flags.case_sensitiv = W.flags.case_sensitiv;
		fwp->flags.is_8and3 = W.flags.is_8and3;
		
		while ((stat = DoDirWalk (&W)) == 0L)
		{
			if (hiddenCorrectly (W.xattr.attr, filename)
				&& (getallfiles || (W.xattr.attr & 0x10) 
					|| FilterFile (fwp->wildcard, filename, fwp->flags.case_sensitiv)))
			{
				if (aktIndex == FILEBUCKETSIZE)
				{
					if (aktBucket)
						aktBucket->usedcount = aktIndex;
					
					aktBucket = calloc (1, sizeof (FileBucket));
					*previousBucket = aktBucket;
					
					if (!aktBucket)
					{
						retcode = FALSE;
						break;
					}
					
					previousBucket = &(aktBucket->nextbucket);
					aktIndex = 0;
				}
	
				if (!fillFileInfo (&aktBucket->finfo[aktIndex], 
					++gesamtZahl, fwp->path, filename, &W.xattr))
				{
					retcode = FALSE;
					break;
				}
				
				len = (word)strlen (filename);
				if (len > fwp->max_filename_len)
					fwp->max_filename_len = len;
						
				newDirSize += W.xattr.size;
				++aktIndex;
			}
		}
		
		ExitDirWalk (&W);
	}
	else
		retcode = FALSE;
		
	if (aktBucket)
		aktBucket->usedcount = aktIndex;
	
	fwp->files = newList;
	fwp->fileanz = gesamtZahl;
	fwp->dirsize = newDirSize;
	fwp->sort = SortNone;			/* still unsorted */

	return retcode;
}


static void freeBucketContents (FileBucket *p)
{
	int i;
	
	for (i = 0; i < p->usedcount; ++i)
	{
		free (p->finfo[i].name);
		free (p->finfo[i].icon.string);
	}
}

/*
 * free all FileInfos in list at fwp->files
 */
void FreeFiles (FileWindow_t *fwp)
{
	FileBucket *nextp, *p;

	p = fwp->files;
	while (p != NULL)
	{
		nextp = p->nextbucket;
		freeBucketContents (p);
		free (p);
		p = nextp;
	}
}
