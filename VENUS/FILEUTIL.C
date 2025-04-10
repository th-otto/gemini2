/*
 * @(#) Gemini\fileutil.c
 * @(#) Stefan Eissing, 26. M„rz 1994
 *
 * description: utility functions for files
 *
 * jr 22.4.95
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <aes.h>
#include <tos.h>
#include <nls\nls.h>
#include <flydial\flydial.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\cookie.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"
#include "..\common\walkdir.h"

#include "vs.h"

#include "window.h"
#include "fileutil.h"
#include "iconinst.h"
#include "message.h"
#include "myalloc.h"
#include "select.h"
#include "util.h"
#include "venuserr.h"
#include "..\mupfel\mupfel.h"



/*
 * internal texts
 */
#define NlsLocalSection "G.fileutil"
enum NlsLocalText{
T_NESTED,	/*Operation wird abgebrochen, da die Schachtelung
 der Ordner zu tief ist.*/
T_CLIPDIR,	/*Konnte den Ordner (`%s') fr das Klemmbrett nicht 
anlegen (%s).*/
T_TRASHDIR,	/*Konnte den Ordner fr den Papierkorb nicht anlegen.*/
T_USERABORT,	/*Wollen Sie die Operation wirklich abbrechen?*/
};



word AddFileAnz(char *path, int follow_links, long max_objects,
	uword *foldnr, uword *filenr, long *size)
{
	WalkInfo W;
	char filename[MAX_PATH_LEN];
	long stat;
	word ok = TRUE;
	int flags;
	
	if (strlen (path) > MAX_PATH_LEN - MAX_FILENAME_LEN)
	{
		venusErr (NlsStr (T_NESTED));
		return FALSE;
	}

	if (escapeKeyPressed ())
	{
		if (venusChoice (NlsStr (T_USERABORT)))
			return FALSE;
	}

	flags = WDF_ATTRIBUTES|WDF_IGN_XATTR_ERRORS|WDF_GEMINI_LOWER;
	if (follow_links) flags |= WDF_FOLLOW;
	
	stat = InitDirWalk (&W, flags, path, MAX_PATH_LEN, filename);
	
	if (!stat)
	{	
		while (ok && ((stat = DoDirWalk (&W)) == 0L))
		{
			/* match? */
			
			if (W.xattr.attr & FA_FOLDER)	/* is folder */
			{
				if(strcmp (filename, ".")
					&& strcmp (filename, ".."))
				{
					if (strlen (path) + strlen (filename) 
						>= MAX_PATH_LEN - 1)
					{
						ok = FALSE;
						stat = ERANGE;	
					}
					else
					{
						(*foldnr)++;
						AddFolderName (path, filename);
						ok = AddFileAnz (path, follow_links, max_objects,
							foldnr, filenr, size);
						StripFolderName (path);
					}
				}
			}
			else if (!(W.xattr.attr & FA_LABEL))
			{
				*size += W.xattr.size;
				(*filenr)++;
			}

			if ((max_objects > 0) 
				&& ((*foldnr + *filenr) >= max_objects))
				break;
		}	
	
		ExitDirWalk (&W);
	}
	
	if (!ok && stat)
	{
		venusErr (StrError (stat));
		ok = FALSE;
	}

	return ok;
}


/*
 * void dateString(char *cp, word date)
 * makes a string out of a TOS-date-integer
 */
void dateString(char *cp, word date)
{
	register word year,month,day;
	register char *pt;

	cp[8] = '\0';
	pt = &cp[7];
	
	year = (((date & 0xFE00) >> 9) + 80) % 100;
	month = (date & 0x01E0) >> 5;
	day = date & 0x001F;
#if GERMAN
	*(pt--) = "0123456789"[year % 10];
	year /= 10;
	*(pt--) = "0123456789"[year % 10];
	*(pt--) = '.';
	*(pt--) = "0123456789"[month % 10];
	*(pt--) = "0123456789"[month / 10];
	*(pt--) = '.';
	*(pt--) = "0123456789"[day % 10];
	*(pt--) = "0123456789"[day / 10];
#endif
#if ENGLISH
	*(pt--) = "0123456789"[year % 10];
	year /= 10;
	*(pt--) = "0123456789"[year % 10];
	*(pt--) = '/';
	*(pt--) = "0123456789"[day % 10];
	*(pt--) = "0123456789"[day / 10];
	*(pt--) = '/';
	*(pt--) = "0123456789"[month % 10];
	*(pt--) = "0123456789"[month / 10];
#endif
}

/*
 * void timeString(char *cp, word time)
 * makes a string out of a TOS-time-integer
 */
void timeString(char *cp, word time)
{
	register word hour,min,sec;
	register char *pt;

	cp[8] = '\0';
	pt = &cp[7];
	
	hour = ((time & 0xF800) >> 11) % 24;
	min = ((time & 0x07E0) >> 5) % 60;
	sec = ((time & 0x001F) << 1) % 60;
	*(pt--) = "0123456789"[sec % 10];
	*(pt--) = "0123456789"[sec / 10];
	*(pt--) = ':';
	*(pt--) = "0123456789"[min % 10];
	*(pt--) = "0123456789"[min / 10];
	*(pt--) = ':';
	*(pt--) = "0123456789"[hour % 10];
	*(pt--) = "0123456789"[hour / 10];
}


long GetFullPath (char *path, size_t maxlen)
{
	int drive_nr;
	long retcode;
	
	drive_nr = Dgetdrv ();
	path[0] = (char)drive_nr + 'A';
	path[1] = ':';
	retcode = GetPathOfDrive (&path[2], maxlen-2, drive_nr, TRUE);
	if (!retcode && lastchr (path) != '\\')
		strcat (path, "\\");

	return retcode;
}

word SetDrive(const char drive)
{
	if (LegalDrive (drive))
	{
		Dsetdrv (drive - 'A');
		return TRUE;
	}
	else
		return FALSE;
}

char GetDrive(void)
{
	return Dgetdrv() + 'A';
}

long SetFullPath (const char *path)
{
	char tmp[MAX_PATH_LEN], new[MAX_PATH_LEN];
	long retcode;
	char drive;
	int drive_nr;
	size_t len;

	drive = toupper (path[0]);

	if (LegalDrive (drive))
	{
		drive_nr = drive - 'A';
		Dsetdrv (drive_nr);
	}
	else
		return EDRIVE;

	retcode = GetPathOfDrive (tmp, MAX_PATH_LEN, drive_nr, TRUE);
	if (retcode) return retcode;

	strcpy (new, path);
	len = strlen (new) - 1;
	if ((len > 2) && (new[len] == '\\'))
		new[len] = '\0';		/* kein backslash am Ende */
		
	retcode = Dsetpath (&new[2]);
	if (retcode != E_OK)
	{
		Dsetdrv (drive_nr);
		Dsetpath ("\\");
		Dsetpath (tmp);
	}

	return retcode;
}


int IsInExtensionList (const char *name, const char *listname,
	const char *defList)
{
	const char *pSuffix = NULL, *pext;
	char *mySuffix, *cp;
	
	if((pext = strrchr(name,'\\')) == NULL)	/* get name part */
		pext = name;
	pext = strrchr (pext, '.');
	if (pext)
		pext++;
	else
		return FALSE;		/* no extension, not executable */
	
	if (listname)
		pSuffix = GetEnv (MupfelInfo, listname);
	if (!pSuffix)
		pSuffix = defList;
	cp = malloc (strlen (pSuffix) + 1);

	if (cp)
	{
		strcpy (cp, pSuffix);
		mySuffix = cp;
		
		if ((cp = strtok (mySuffix, ";,")) != NULL)
		{
			do
			{
				if (!stricmp (cp, pext))
				{
					free (mySuffix);
					return TRUE;
				}
					
			}
			while ((cp = strtok (NULL, ";,")) != NULL);

		}
		
		free (mySuffix);
	}
	
	return FALSE;		/* malloc failed or fallen through */
}

int IsGemExecutable (const char *name)
{
	return IsInExtensionList (name, "GEMSUFFIX", "PRG;APP;GTP");
}

int TakesParameter (const char *name)
{
	return IsInExtensionList (name, "PARSUFFIX", "GTP;TTP");
}

/*
 * test in environmentstring SUFFIX, if it includes the extendion
 * if not available, test default (PRG,APP,TOS,TTP)
 */
int IsExecutable (const char *name)
{
	return IsInExtensionList (name, "SUFFIX", 
		"PRG;APP;TTP;TOS;MUP;GTP");
}

int IsAccessory(const char *filename)
{
	return IsInExtensionList (filename, NULL, "ACC");
}



/*
 * word sameLabel(const char *label1,const char *label2)
 * vergleicht zwei Label. Ist ein String leer,
 * so wird TRUE zurckgeliefert
 */
word sameLabel(const char *label1,const char *label2)
{
	if(strlen(label1) && strlen(label2))
	{
		return (!strcmp(label1,label2));
	}
	return TRUE;
}


/*
 * word isEmptyDir(const char *path)
 * check if directory is empty
 */
word isEmptyDir(const char *path)
{
	WalkInfo W;
	long stat;
	char filename[MAX_PATH_LEN];
	word empty = TRUE;
	
	stat = InitDirWalk (&W, WDF_GEMINI_LOWER,
		(char *)path, MAX_PATH_LEN, filename);

	if (!stat)
	{
		while (empty && ((stat = DoDirWalk (&W)) == 0L))
		{
			empty = !strcmp (filename, ".")
					|| !strcmp (filename, "..")
					|| (W.xattr.attr & FA_LABEL);
		}
		
		ExitDirWalk (&W);
	}

	return empty;
}


static long getbdev (void)
{
	return (long) (*((word *)0x446L));
}

static word getBootDev(void)
{
	return (word)Supexec (getbdev);
}


/*
 * fill path with the AES-Scrapdir, if this is empty
 * try to set it by yourself and look in environment for
 * "CLIPBRD"
 */
char *getSPath (char *oldpath)
{
	word bootdev, found = FALSE;
	char *penv;
	char path[MAX_PATH_LEN];
	int broken;
	
	scrp_read (path);
	if ((!strlen (path)) || (!IsDirectory (path, &broken)))
	{
		if ((penv = GetEnv (MupfelInfo, "CLIPBRD")) == NULL)
			penv = GetEnv (MupfelInfo, "SCRAPDIR");
	
		if (penv)
		{
			strcpy (path, penv);
			path[0] = toupper (path[0]);
			if ((strlen (path) > 3)
				&& (path[strlen (path)-1] == '\\'))
			{
				path[strlen (path)-1] = '\0';
			}
			found = IsDirectory (path, &broken);
		}
		
		if (!found && (bootdev = getBootDev ()) >= 2) /* no Floppy */
		{
			path[0] = (char)bootdev + 'A';	/* try in root */
			strcpy (path+1, ":\\CLIPBRD");
			if (!IsDirectory (path, &broken))
				found = (Dcreate (path) >= 0);
			else
				found = TRUE;
		}
		
		if (!found)
		{
			strcpy (path, BootPath);		/* try in own dir */
			AddFileName (path, "CLIPBRD");
			if (!IsDirectory (path, &broken))
			{
				long ret = Dcreate (path);
				
				if (ret < 0)
					venusErr (NlsStr (T_CLIPDIR), path,
						StrError ((int) ret));
			}
		}
		
		if (path[strlen (path)-1] != '\\')
			strcat (path, "\\");
		scrp_write (path);
	}
	
	if (strchr (path,'*') || strchr (path,'?'))	/* wildcards? */
		StripFileName (path);
	if (path[strlen (path)-1] != '\\')
		strcat (path, "\\");
	
	if (oldpath == NULL || strcmp (path, oldpath))
	{
		char *new;
		
		new = StrDup (pMainAllocInfo, path);
		if (new != NULL)
		{
			free (oldpath);
			oldpath = new;
		}
	}
	
	return oldpath;
}


/*
 * get path for trashdir, look in environment for
 * variable "TRASHDIR"
 */
char *getTPath (char *oldpath)
{
	const char *penv;
	char path[MAX_PATH_LEN];
	int broken;
	
	penv = GetEnv (MupfelInfo, "TRASHDIR");
	
	if (penv && IsDirectory (penv, &broken))
	{
		strcpy (path, penv);
		if (path[strlen (path) - 1] != '\\')
			strcat (path, "\\");
	}
	else
	{
		penv = GetEnv (MupfelInfo, "HOME");
		strcpy (path, penv? penv : BootPath);
		AddFileName (path, "TRASHDIR");
		if (!IsDirectory (path, &broken))
		{
			long ret = Dcreate (path);

			if (ret != E_OK)
				venusErr (NlsStr (T_TRASHDIR), StrError (ret));
			strcat (path, "\\");
		}
	}
	
	if (oldpath == NULL || strcmp (path, oldpath))
	{
		char *new;
		
		new = StrDup (pMainAllocInfo, path);
		if (new != NULL)
		{
			new[0] = toupper (new[0]);
			free (oldpath);
			oldpath = new;
		}
	}
	
	return oldpath;
}


/*
 * word getDiskSpace (word drive,unsigned long *bytesused,
 * 						unsigned long *bytesfree)
 * get the free and used space on disk drive: 
 */
word getDiskSpace (word drive, long *bytesused,
						long *bytesfree)
{
	DISKINFO di;
	long bytespercluster;
	word ok, trymedia = TRUE;
	long correction = 2;

	if (GEMDOSVersion >= 0x30 || MagiCVersion)
		correction = 0;
	
	do
	{
		if (Dfree (&di, drive + 1) < 0)
			ok = FALSE;
		else
		{
			bytespercluster = di.b_secsiz * di.b_clsiz;
			*bytesfree = di.b_free * bytespercluster;
			*bytesused = (di.b_total - di.b_free - 
					correction) * bytespercluster;

			if (*bytesused < 0)
			{
				*bytesused = 0;
			}
			ok = (*bytesfree >= 0);
		}
		
		if(!ok)
		{
			if (trymedia)
			{
				char *p = "X:\\";
				
				p[0] = drive + 'A';
				ForceMediaChange (p);
				trymedia = FALSE;
			}
			else
				return FALSE;
		}
		
	} 
	while (!ok);

	return TRUE;
}


long fileRename(char *oldname,char *newname)
{
	return Frename(0,oldname,newname);
}


long setFileAttribut(char *name,word attrib)
{
	return Fattrib(name,TRUE,attrib);
}


word CreateFolder (char *path)
{
	int broken = FALSE;
	
	if (access (path, A_EXIST, &broken))
		return EACCDN;
	else if (broken)
		return broken;
	else
		return Dcreate (path);
}



word CreateFile (char *path)
{
	int broken = FALSE;
	
	if (!access (path, A_EXIST, &broken))
	{
		int handle = (int)Fcreate (path, 0);
		
		if (handle >= 0)
			Fclose (handle);
		
		return handle;
	}
	else
		return broken? broken : EACCDN;
}


word delFile(const char *fname)
{
	return !Fdelete (fname);
}


void char2LongKey(char c, long *key)
{
	KEYTAB *kt;
	char *cp, value;
	word i;
	
	
	value = tolower(c);
	kt = Keytbl((void *)-1L, (void *)-1L, (void *)-1L);
	cp = kt->unshift;
	for (i=0; i < 128; ++i)
	{
		if (cp[i] == value)
		{
			*key = ((long)i) << 16;
			break;
		}
	}
	*key |= (long)c;
}


int GetFileName (char fname[MAX_PATH_LEN], const char *pattern,
	const char *message)
{
	char tmp[2*MAX_PATH_LEN], filename[MAX_FILENAME_LEN];
	word retcode, ok;
	
	strcpy (tmp, fname);
	GetBaseName (tmp, filename, MAX_FILENAME_LEN);

	if (!*tmp)
	{
		if (GetFullPath (tmp, 2*MAX_PATH_LEN))
			return FALSE;
	}
	else
		StripFileName (tmp);
	AddFileName (tmp, pattern);
	
	WindUpdate (BEG_MCTRL);
	if (GotGEM14 ())
	{
		retcode = fsel_exinput (tmp, filename, &ok, (char *)message);
	}
	else
		retcode = fsel_input (tmp, filename, &ok);
	WindUpdate (END_MCTRL);
	
	HandlePendingMessages (FALSE);
	
	if (retcode && ok)
	{
		StripFileName (tmp);
		AddFileName (tmp, filename);
		
		if (strlen (tmp) > MAX_PATH_LEN)
		{
			venusErr (NlsStr (T_NESTED));
		}
		else
		{
			strcpy (fname, tmp);
			return TRUE;
		}
	}
	
	return FALSE;
}

