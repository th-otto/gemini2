/*
 * @(#) Common\fileutil.c
 * @(#) Stefan Eissing, 30. Juli 1994
 *
 * jr 29.6.1996
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <tos.h>
#include <mint\mintbind.h>

#include "alloc.h"
#include "cookie.h"
#include "charutil.h"
#include "fileutil.h"
#include "strerror.h"
#include "walkdir.h"
#include "xhdi.h"

#ifndef TRUE
#define FALSE	(0)
#define TRUE	(!FALSE)
#endif


int
GetBaseName (const char *path, char *target, size_t maxlen)
{
	const char *cp;
	char *lasts;
	
	lasts = NULL;
	*target = '\0';
	cp = strrchr (path, '\\');
	
	if (cp != NULL)
	{
		/* Ist es ein Backslash am Ende des Pfades?
		 */
		if (cp[1] == '\0')
		{
			lasts = (char *)cp;
			*lasts = '\0';
			cp = strrchr (path, '\\');
			*lasts = '\\';
			
			if (cp == NULL)
				return FALSE;
		}
		
		++cp;
	}
	else
		cp = path;
	
	--maxlen;
	if (strlen (cp) >= maxlen)
		return FALSE;
	
	if (lasts != NULL)
		*lasts = '\0';	
	strncpy (target, cp, maxlen);
	target[maxlen] = '\0';
	if (lasts != NULL)
		*lasts = '\\';	

	return TRUE;
}

int
StripFileName (char *path)
{
	char *cp;
	
	if ((cp = strrchr (path, '\\')) != NULL)
	{
		/* Wenn das Zeichen hinter dem Backslash 0 ist, dann
		 * endete dort der Pfad und wir mÅssen weiter abtrennen
		 */
		if (cp[1] == '\0')
		{
			cp[0] = '\0';
			return StripFileName (path);
		}
		
		cp[1] = '\0';
		return TRUE;
	}
	else if (*path)
	{
		/* Wenn wir etwas wie a:datei haben, soll hinter dem
		 * Punkt abgetrennt werden.
		 */
		if (path[1] == ':' && path[2] != '\0')
			path[2] = '\0';
		else
			*path = '\0';
			
		return TRUE;
	}
	
	return FALSE;
}

int
StripFolderName (char *path)
{
	char *cp1, *cp2;
	
	if ((cp1 = strrchr (path, '\\')) != NULL)
	{
		if (cp1 == &path[2])
		{
			/* Wir hatten etwas wie A:\ORDNER, dann mÅssen wir
			 * den Backslash dranlassen, da es fÅr GEMDOS einen
			 * fundamentalen Unterschied zwischen A: und A:\ gibt.
			 */
			if (cp1[1] == '\0')
			{
				/* Wir hatten schon X:\, da kînnen wir nix mehr
				 * abschneiden.
				 */
				return FALSE;
			}

			cp1[1] = '\0';
			return TRUE;
		}
		
		*cp1 = '\0';
		
		/* Es stand noch etwas hinter dem Backslash, also schneide
		 * nur das ab.
		 */
		if (cp1[1] != '\0')
			return TRUE;
			
		if ((cp2 = strrchr (path,'\\')) != NULL)
		{
			cp2[1] = '\0';
			return TRUE;
		}
		else
			*cp1 = '\\';		/* restore path */
	}
	
	/* Wenn wir hier ankommen, hatten wir entweder keinen oder nur
	 * einen Backslash am Ende von <path>. Vielleicht handelt es
	 * sich um x:datei.ext?
	 */
	if (strlen (path) > 2 && path[1] == ':' && path[2] != '\\')
	{
		/* Wir haben a:datei
		 */
		path[2] = '\0';
		return TRUE;
	}
	
	return FALSE;
}

void
AddFileName (char *path, const char *name)
{
	size_t len = strlen (path);
	
	if ((len != 2 || path[1] != ':') && (path[len - 1] != '\\'))
		strcat (path, "\\");
	strcat (path, name);
}

void
AddFolderName (char *path, const char *name)
{
	AddFileName (path, name);
	strcat(path,"\\");
}


int
LegalDrive (char drv)
{
	unsigned long drivemap = Dsetdrv (Dgetdrv ());
	
	drv = toupper (drv) - 'A';
	
	return 0 != (drivemap & (1L << drv));
}

/*
 * ValidFileName (const char *name)
 * check if name (filname or wildcard) contains illegal elements:
 *   - illegal drive specifiers
 *   - double backslashes
 */
int
ValidFileName (const char *name)
{
	char c;
	const char *cp;
	
	c = toupper (*name);
	if (*name && name[1] == ':' && (c < 'A' || c > 'Z'))
		return FALSE;

	if (strpbrk(name, "?*") != NULL)
		return FALSE;

	if (strstr (name, "\\\\"))
		return FALSE;

	for (cp = name; *cp == ' '; ++cp)
		;
		
	return *cp;
}


int
IsAbsoluteName (const char *name)
{
	if (strlen (name) >= 3)
	{
		return (name[1] == ':' && name[2] == '\\');
	}

	return FALSE;	
}


/* Gibt an, ob <name> eine Extension hat. Dabei kann <name> auch
 * ein kompletter Pfad vorangehen.
 */
const char *
Extension (const char *name)
{
	const char *point, *separator;
	
	point = strrchr (name, '.');
	if (!point)
		return NULL;
	
	separator = strrchr (name, '\\');
	if (!separator || (separator < point))
		return ++point;

	return NULL;
}


/* öbertrage die Dateiattribute */

void
FillXattrFromDTA (const char *path, XATTR *X, DTA *D)
{
	memset (X, 0, sizeof (XATTR));
	
	if (path[1] == ':') X->dev = toupper (path[0]) - 'A';

	X->blksize = 1024L;
	X->ctime = X->mtime = X->atime = D->d_time;
	X->cdate = X->mdate = X->adate = D->d_date;
	X->attr = D->d_attrib;
	X->size = D->d_length;
	X->uid = X->gid = 0;

	X->nlink = D->d_attrib & FA_SUBDIR ? 2 : 1;
	
	X->nblocks = X->size / X->blksize;
	if (X->size % X->blksize) ++X->nblocks;
	X->mode = (X->attr & FA_SUBDIR) ? 
		(S_IFDIR|DEFAULT_DIRMODE) : 
		(S_IFREG|DEFAULT_MODE);
	X->index = 0;
}


#define X_LEN	256

long
GetXattr (const char *filename, int with_label, int follow_links,
	XATTR *X)
{
	char name[X_LEN];
	long stat;
	size_t len;
	
	if (!ValidFileName (filename)) return EFILNF;

	if (!*filename) return EPTHNF;
		
	len = strlen (filename);
	if (filename[1] == ':' && len <= 3 
		&& (filename[2] == '\\' || len == 2))
	{
		/* Es ist einfach nur ein Laufwerksbezeichner */
		DTA D;
		
		memset (&D, 0, sizeof (DTA));
		D.d_attrib = FA_SUBDIR;
		FillXattrFromDTA (filename, X, &D);
		
		return 0L;
	}
	
	if (len < X_LEN)
	{
		char *cp;
		
		strcpy (name, filename);
		if (filename[1] == ':')
			name[0] = toupper (name[0]);

		cp = strrchr (name, '\\');
		if (cp && cp[1] == '\0')
			cp[0] = '\0';
		filename = name;
	}

	if (((stat = Fxattr (!follow_links, (char *)filename, X)) != EINVFN)
		&& (stat != EDRIVE)) /* MagiX auf CDROMS macht son Quatsch */
	{
		if (stat) return stat;
			
		if (!with_label) return ((X->attr & FA_VOLUME) != 0);
	}
	else
	{
		DTA D, *old_dta;
		
		old_dta = Fgetdta ();
		Fsetdta (&D);
		
		stat = Fsfirst (filename, with_label?
				(FA_HIDDEN|FA_SYSTEM|FA_VOLUME|FA_SUBDIR) :
				(FA_HIDDEN|FA_SYSTEM|FA_SUBDIR));
		
		Fsetdta (old_dta);
		if (stat) return stat;
		
		FillXattrFromDTA (filename, X, &D);
	}

	return 0L;
}

/* jr: changed to return a long GEMDOS error code */

long
GetFileInformation (const char *file, long *size, short *attrib,
	unsigned short *date, unsigned short *time, int *broken)
{
	DTA dta, *olddta;
	long found;
	
	if (!ValidFileName (file)) return EFILNF; /* ??? */
		
	*broken = 0;

	olddta = Fgetdta ();
	Fsetdta (&dta);
	found = Fsfirst (file, FA_NOLABEL);
	Fsetdta (olddta);

	*size =	dta.d_length;
	*attrib = dta.d_attrib;
	*date = dta.d_date;
	*time = dta.d_time;

	if (IsBiosError (found)) *broken = (int) found;

	return found;
}


long
ReadLabel (char drv, char *name_buffer, size_t maxlen)
{
	DTA dta, *olddta = Fgetdta ();
	char label[7];
	long fstat;
	
	Fsetdta (&dta);
	sprintf (label, "%c:\\*.*", drv);
	fstat = Fsfirst (label, FA_VOLUME);
	Fsetdta (olddta);

	name_buffer[0] = '\0';
	
	if (fstat == E_OK) {
		--maxlen;
		strncpy (name_buffer, dta.d_fname, maxlen);
		name_buffer[maxlen] = '\0';
	}
	
	return fstat;
}


/* jr: find out whether the device is accessible by trying to
   read the label */

long DriveAccessible (char drv, char *path, size_t len)
{
	char tmp[2];
	long stat;
	
	if (path)
		stat = ReadLabel (drv, path, len);
	else
		stat = ReadLabel (drv, tmp, sizeof (tmp));
		
	/* not every GEMDOS error is a problem for us */
	
	if (stat == EFILNF || stat == ENAMETOOLONG || stat == EINVFN
		|| stat == ENMFIL)
		stat = E_OK;
		
	return stat;
}



/*
 * access(char *file, int mode)
 * determine accessibility of file according to mode
 * mode values:	check performed:
 *	A_EXITS		could be opened for reading
 *	A_READ		dto
 *	A_RDONLY		exists, is read-only
 *	A_WRITE		could be opened for writing
 *	A_RDWR		dto
 *	A_EXEC		exists, and is a TOS binary
 *				(magic number X_MAGIC in 1st 2 bytes == X_MAGIC)
 */

/* xxx jr: sollte richtige Fehlernummern produzieren */

int access (const char *file, int mode, int *broken)
{
	XATTR X;
	int hnd, retcode, magic, readonly;
	
	if (GetXattr ((char *)file, FALSE, TRUE, &X)) 
		return FALSE;
	
	if (X.attr & FA_SUBDIR)
		return mode == A_ISDIR || mode == A_EXIST;
	
	readonly = X.attr & FA_READONLY;
	
	switch (mode)
	{
		case A_EXIST:
		case A_READ:
			return TRUE;

		case A_WRITE:
		case A_RDWR:
			return !readonly;

		case A_RDONLY:
			return readonly;

		case A_EXEC:
			if ((hnd = (int)Fopen (file, 0)) >= 0)
			{
				if (Fread (hnd, sizeof(magic), &magic) != sizeof (magic))
					retcode = FALSE;
				else
					retcode = (magic == X_MAGIC);

				Fclose(hnd);
			}
			else
			{
				retcode = FALSE;
				if (IsBiosError (hnd))
					*broken = hnd;
			}
			break;

		default:
			retcode = FALSE;
			break;
	}
	
	return retcode;
}


int IsRealFile (const char *full_name, int *broken)
{
	return access (full_name, A_READ, broken);
}

int IsRootDirectory (const char *dir)
{
	return ((strlen (dir) == 3)
		&& !strcmp (dir + 1, ":\\"));
}

int IsDirectory (const char *dir, int *broken)
{
	char *last = NULL;
	char save;
	int retcode;
	size_t len = strlen (dir);

	last = (char *)dir + len - 1;
	if (*last == '\\')
	{
		if (len == 3 && dir[1] == ':')
			return LegalDrive (*dir);
		
		save = *last;
		*last = '\0';
	}
	else
		last = NULL;
	
	retcode = access (dir, A_ISDIR, broken);
	if (last) *last = save;
		
	return retcode;
}


long GetPathOfDrive (char *path, size_t maxlen, int drive_nr,
	int prefer_uppercase)
{
	char *cp = NULL;
	int case_sensitiv = FALSE;
	long stat;
	
	(void)maxlen; /* Stevie: Dgetcwd muû noch rein! */
	path[0] = '\0';
	
	if (((stat = Dgetpath (path, drive_nr + 1)) != 0L)
		|| (*path && ((cp = strchr (path, '\\')) == NULL)))
	{
		return stat? stat : EPTHNF;
	}
	
	if (cp)
	{
		if (cp != path)
			strcpy (path, cp);
	}
	else
		strcpy (path, "\\");
		
	{
		char tmp[256];
		
		tmp[0] = drive_nr + 'A';
		tmp[1] = ':';
		strncpy (tmp+2, path, 255);
		tmp[255] = '\0';
		
		case_sensitiv = !Dpathconf (tmp, 6);
	}
	
	if (!case_sensitiv)
	{
		if (prefer_uppercase)
			strupr (path);
		else
			strlwr (path);
	}
	
	return 0L;	
}

/*
 * Build an absolute dir specifier from the possibly weird construct
 * in d. Understands about things like a\b\..\c\.\d\.. (which means
 * a\c).
 * Returns pointer to allocated copy of normalized pathname.
 * Written by Steve and Gereon, with help from the thirsty man :-)
 */

/* xxx: should return correcz error codes */

#define NORM_MAX_PATH_LEN	512	

char *
NormalizePath (ALLOCINFO *A, const char *path, const char cur_drive,
	const char *cur_dir)
{
	char dir[NORM_MAX_PATH_LEN], normdir[NORM_MAX_PATH_LEN];
	char *dir_start, *dir_end, *backslash;
	int lastback;

	/* remember if last char is \ */
	lastback = (lastchr (path)=='\\');

	/* changed by jr: first: look for drive! */
	if (path[1] != ':')
	{
		dir[0] = cur_drive;
		dir[1] = ':';
	}
	else
	{
		/* copy existing drive identifier */
		dir[0] = *path++;
		dir[1] = *path++;
	}
	dir[2] = 0;

	if (!LegalDrive (dir[0]))	/* valid drive name? */
		return NULL;
	
	if (path[0] != '\\')			/* absolute path? */
	{
		if (dir[0] == cur_drive)
		{
			strcat (dir, cur_dir);
		}
		else
		{
			if (GetPathOfDrive (normdir, NORM_MAX_PATH_LEN,
				toupper (dir[0]) - 'A', FALSE))
			{
				return NULL;
			}
			strcat (dir, normdir);
		}

		if (lastchr (dir) != '\\')
			chrcat (dir, '\\');
	}
	
	strcat (dir, path);

	/* tokenize dir at \; 
	 */
	dir_start = dir;
	*normdir = '\0';

	while (dir_start && *dir_start)
	{
		if ((dir_end = strchr (dir_start, '\\')) != NULL)
			*dir_end = '\0';
		
		if (*dir_start)
		{
			if (strcmp (dir_start, "."))			/* throw away ./ */
			{
				/* it's not . */
				if (strcmp (dir_start, ".."))
				{
					/* it's not ".." or "." */
	
					/* append \ only if not first token */
					if (strlen (normdir) >= 2)
						chrcat (normdir, '\\');
		
					strcat (normdir, dir_start);
				}
				else
				{
					/* it's .., delete previous entry
					 */
					backslash = strrchr (normdir, '\\');
					if (backslash == NULL)
					{
						/* No backslash -> too many ..'s
						 */
						return NULL;
					}
					else
							*backslash = '\0';
	
					/* why not? */
	
	#if COMMENT
					{
						/* don't delete first backslash */
						if (backslash != strchr (normdir, '\\'))
						else
							*(backslash + 1) = '\0';
					}
	#endif
				}
			}
		}
		
		dir_start = dir_end;
		if (dir_end)
			++dir_start;
	}

	/* restore trailing \ if we had one or drive specfier resulted
	 */
	if (lastback || (strlen (normdir) == 2 && normdir[1] == ':'))
		chrcat(normdir,'\\');

	return StrDup (A, normdir);
}



/* Diese Funktion versucht aus der ersten Zeile einer Textdatei zu
 * ermitteln, welches Programm fÅr diese Datei zustÑndig ist.
 * Dazu wird untersucht, ob am Anfang der Datei EXEC_MAGIC steht.
 * Wenn ja, wird der Rest der Zeile als der Name des zustÑndigen
 * Programms aufgefaût. Leerzeichen werden entfernt.
 */
#define LOOK_SIZE	128
#define EXEC_MAGIC	"#!"

char *GetExecuterFromFile (ALLOCINFO *A, const char *name, int *broken)
{
	int fhandle;
	char buffer[LOOK_SIZE];
	size_t flen, read;
	
	*broken = FALSE;
	
	/* Versuche die Datei <name> zu îffnen.
	 */
	fhandle = (int)Fopen (name, 0);
	if (fhandle < 0)
	{
		*broken = fhandle;
		return NULL;
	}
	
	/* Stelle die LÑnge der Datei fest und lese hîchstens
	 * LOOK_SIZE Bytes daraus in den <buffer>
	 */
	flen = Fseek (0L, fhandle, 2);
	Fseek (0L, fhandle, 0);
	
	if (flen > LOOK_SIZE)
		flen = LOOK_SIZE;
		
	read = Fread (fhandle, flen, buffer);
	Fclose (fhandle);
	
	/* Fehler beim Lesen?
	 */
	if (read != flen)
	{
		if (IsBiosError (read))
			*broken = (int)read;
		return NULL;
	}
	
	/* So, die ersten <flen> Bytes stehen in Buffer. Nun mÅsen wir
	 * schauen, ob er mit EXEC_MAGIC beginnt und den anschlieûenden
	 * String extrahieren.
	 */
	{
		size_t magic_len = strlen (EXEC_MAGIC);
		char *command;
		size_t i, start;
		
		if (strncmp (buffer, EXEC_MAGIC, magic_len))
			return NULL;
	
		/* öberspringe Leerzeichen
		 */
		i = magic_len;
		while (buffer[i] && (i < read) && strchr (" \t", buffer[i]))
			++i;

		/* öberspringe bis Leerzeichen.
		 */
		for (start = i; i < read; ++i)
		{
			if (strchr (" \t\n\r", buffer[i]))
				break;
		}
		
			
		if ((i == start) || (i >= read))
			return NULL;
		
		buffer[i] = '\0';
		command = StrDup (A, &buffer[start]);
		
		if (!command)
			*broken = TRUE;
			
		return command;
	}
}


/* Versehe <filename> mit einer neuen Extension und gib einen
 * allozierten neuen Namen zurÅck
 */
char *NewExtension (ALLOCINFO *A, char *filename, const char *ext)
{
	char *dot, *f;
	
	f = mmalloc (A, strlen (filename) + strlen (ext) + 2);
	if (f == NULL)
		return NULL;
		
	strcpy (f, filename);
	
	if (*ext == '.')
		++ext;

	dot = strrchr (f, '.');
	if (dot == NULL || dot < strrchr (f, '\\'))
	{
		if (*ext)
		{
			strcat (f, ".");
			strcat (f, ext);
		}
	}
	else
	{
		if (*ext)
			strcpy (dot + 1, ext);
		else
			*dot = '\0';
	}
	
	return f;
}


unsigned long DriveBit (const char drive_name)
{
	unsigned char c = toupper (drive_name);
	
	return 0x1UL << (c - 'A');
}


long BiosDeviceOfPath (char *path, short *device)
{
	XATTR X;
	long stat;
	
	if (path == NULL) return EPTHNF;
	stat = GetXattr (path, TRUE, TRUE, &X);
	*device = X.dev;
	return stat;
}


/* Forciert einen Media-Change auf dem Laufwerk des Åbergebenen
 * Pfades.
 */
int ForceMediaChange (char *path)
{
	int retcode;
	short bios_dev;

	if (BiosDeviceOfPath (path, &bios_dev))
		return FALSE;
		
	retcode = (int)Dlock (1, bios_dev);
	if (retcode != EINVFN)
	{
		/* Dlock ist verfÅgbar
		 */
		if (!retcode)
			Dlock (0, bios_dev);
	}
	else
	{
		extern int cdecl mediach (int drive);
		struct ofls_cookie *ofls;
		
		if (CookiePresent ('OFLS', (long *)&ofls)
			&& (ofls->product == 'OFLS'))
		{
			if (ofls->drives[bios_dev] != 0)
				return EACCDN;
		}
		
		retcode = mediach (bios_dev);
	}
	
	return retcode;
}

/* GEMDOS-Device ansprechbar? */

int PADriveAccessible (unsigned int gemdos_drive, 
						unsigned long *pDriveMap)
{
	static unsigned long gemdos_drives;
	static unsigned long xhdi_drives;
	static unsigned int xhdi_version;
	static int init = 0;
	
	if (!init)
	{
		init = 1;
		xhdi_version = XHGetVersion ();
		if (xhdi_version) xhdi_drives = XHDrvMap ();
	}

	if (pDriveMap)
		gemdos_drives = *pDriveMap;
	else
		gemdos_drives = Dsetdrv (Dgetdrv ());
	
	if (xhdi_version && (xhdi_drives & (1L << gemdos_drive)))
	{
		unsigned long startsec;
		long ret;
		
		ret = XHInqDev (gemdos_drive, NULL, NULL, &startsec, NULL);
		
		return ((ret == 0L) && (startsec != 0xffffffffL));
	}
	
	return ((gemdos_drives & (1L << gemdos_drive)) != 0);
}


int IsPathPrefix (const char *path, const char *prefix)
{
	size_t len = strlen (prefix);
	
	/* gleicher Anfang?
	 */
	if (!strnicmp (path, prefix, len))
		return (path[len] == '\0')	/* sind identisch */
			|| (path[len] == '\\')
			|| (path[len-1] == '\\');	/* gleicher Anfang */
	
	return FALSE;
}

/* jr: liefere eine Drivemap aller Laufwerke, auf die zugegriffen
   werden kann */

unsigned long AccessibleDrives (void)
{
	unsigned long drives = Dsetdrv (Dgetdrv ());
	static int xhdi_version = -1;

#if 1
	if (xhdi_version < 0)
		xhdi_version = XHGetVersion ();
#endif

	if (xhdi_version >= 0x125)
	{
		unsigned long xh_drivemap = XHDrvMap ();
		int i;
		long last_major = -1, last_minor = -1;
				
		for (i = 0; i < 32; i++)
		{
			if (xh_drivemap & (1UL << i))
			{
				unsigned long startsec;
				unsigned int major, minor;
				long ret;
		
				ret = XHInqDev (i, &major, &minor, &startsec, NULL);

/* printf ("%d %ld %d %d %ld\n", i, ret, major, minor, startsec); */

				if ((ret == E_OK || ret == DRIVE_NOT_READY) &&
					(major != last_major || minor != last_minor))
				{
					XHReaccess (major, minor);
					last_major = major;
					last_minor = minor;
					ret = XHInqDev (i, &major, &minor, &startsec, NULL);
				}
		
				if (ret != E_OK || startsec == 0xffffffffL)
					drives &= ~(1UL << i);			
			}
		}
	}
	
	return drives;
}

/* Liefert einen Zeiger auf einen Prozeûnamen */

char *ProcessName (int pid, char *name)
{
	char fname[128];
	DTA *OldDTA = Fgetdta (), MyDTA;
	
	Fsetdta (&MyDTA);
	
	sprintf (fname, "u:\\proc\\*.%03d", pid);
	
	if (E_OK != Fsfirst (fname, 0x17))
	{
		strcpy (MyDTA.d_fname, "?");
	}
	else
	{
		char *c = strrchr (MyDTA.d_fname, '.');
		if (c) *c = '\0';
	}

	Fsetdta (OldDTA);
	strcpy (name, MyDTA.d_fname);
	return name;
}

/* jr: Stelle fest, ob ein Ordner leer ist */

int
IsDirEmpty (const char *dir)
{
	WalkInfo W;
	char filename[256];
	long stat;
	int empty = TRUE;
		
	stat = InitDirWalk (&W, WDF_IGN_XATTR_ERRORS, dir,
		sizeof (filename), filename);

	if (stat != E_OK) return FALSE;

	while (empty && (stat = DoDirWalk (&W)) == E_OK)
		if (strcmp (filename, ".") && strcmp (filename, ".."))
			empty = FALSE;			
	
	ExitDirWalk (&W);
	return empty;
}

typedef struct _MacFinderInfo {
	long fdType;		/*the type of the file*/
	long fdCreator;		/*file's creator*/
	unsigned short fdFlags;	/*flags ex. hasbundle,invisible,locked, etc.*/
	short fdLocation1;	/*file's location in folder*/
	short fdLocation2;	/* rest of location */
	short fdFldr;		/*folder containing file*/
} MacFinderInfo;

#define FMACOPENRES             (('F' << 8) | 72)
#define FMACGETTYCR             (('F' << 8) | 73)
#define FMACSETTYCR             (('F' << 8) | 74)

long
GetMacFileinfo (char *filename, char *type, char *creator)
{
	MacFinderInfo M;
	long ret;

	memset (type, ' ', 4);
	memset (creator, ' ', 4);

	ret = Dcntl (FMACGETTYCR, filename, (long) &M);

	if (ret == EINVFN)
	{
		long handle;
	
		handle = Fopen (filename, 0);
		if (handle >= 0) {
			ret = Fcntl ((int) handle, (long) &M, FMACGETTYCR);
			Fclose ((int) handle);
		}
	}
	
	if (ret == 0)
	{	
		memcpy (type, &M.fdType, 4);
		memcpy (creator, &M.fdCreator, 4);
	}
	
	return ret;
}


