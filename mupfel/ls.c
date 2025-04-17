/*
 *	@(#)Mupfel/ls.c
 *	@(#)Julian F. Reschke & Stefan Eissing, 04. April 1994
 *
 *	06.01.91: `Angleichung' an gnu-ls (jr)
 *	Optionen umgeworfen, lu, lc, ll gekillt. dir und vdir eingebaut
 *	22.01.91: Bugfixes, zB sort-by-size
 *	25.01.91: use unsafe mallocs!
 *  30.04.91: auf neue Mupfel umgestellt, keine globalen Variablen
 *
 * jr 31.12.96
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tos.h>

#include <nls/nls.h>
#include <mint/mintbind.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\cookie.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"
#include "..\common\walkdir.h"
#include "..\common\wildmat.h"
#include "mglob.h"

#include "chario.h"
#include "date.h"
#include "getopt.h"
#include "language.h"
#include "locate.h"
#include "ls.h"
#include "mdef.h"
#include "nameutil.h"
#include "redirect.h"

#if !defined(_SYS_STAT_H) && !defined(__EXT_H__)
#undef S_IFCHR
#define S_IFCHR   0x2000
#undef S_IFBLK
#define S_IFBLK   0x6000
#undef S_IFDIR
#define S_IFDIR   0x4000
#undef S_IFREG
#define S_IFREG   0x8000
#undef S_IFLNK
#define S_IFLNK   0xe000
#undef S_IFIFO
#define S_IFIFO   0xa000
#undef S_IFSOCK
#define S_IFSOCK  0x1000
#undef S_IFMEM
#define S_IFMEM   0xc000

#define S_IEXEC   0x0040
#define S_IREAD   0x0100
#define S_IWRITE  0x0080

#define S_IFMT 0170000

#undef S_IRUSR
#define	S_IRUSR	00400	/* Read by owner.  */
#undef S_IWUSR
#define	S_IWUSR	00200	/* Write by owner.  */
#undef S_IXUSR
#define	S_IXUSR	00100	/* Execute by owner.  */
#undef S_IRWXU
#define	S_IRWXU	(S_IRUSR|S_IWUSR|S_IXUSR)

#undef S_IRGRP
#define	S_IRGRP	(S_IRUSR >> 3)	/* Read by group.  */
#undef S_IWGRP
#define	S_IWGRP	(S_IWUSR >> 3)	/* Write by group.  */
#undef S_IXGRP
#define	S_IXGRP	(S_IXUSR >> 3)	/* Execute by group.  */
#undef S_IRWXG
#define	S_IRWXG	(S_IRWXU >> 3)

#undef S_IROTH
#define	S_IROTH	(S_IRGRP >> 3)	/* Read by others.  */
#undef S_IWOTH
#define	S_IWOTH	(S_IWGRP >> 3)	/* Write by others.  */
#undef S_IXOTH
#define	S_IXOTH	(S_IXGRP >> 3)	/* Execute by others.  */
#undef S_IRWXO
#define	S_IRWXO	(S_IRWXG >> 3)

#define	S_ISUID 04000	/* Set user ID on execution.  */
#define	S_ISGID	02000	/* Set group ID on execution.  */
#define S_ISVTX	01000
#define S_ISTXT	S_ISVTX

#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)
#define S_ISMEM(m)	(((m) & S_IFMT) == S_IFMEM)
#endif

/* internal texts
 */
#define NlsLocalSection "M.ls"
enum NlsLocalText{
OPT_ls,		/*[-acdiklmpqrstuxABCDFHLMRSUX1][-w zeichen][-I muster]*/
HLP_ls,		/*Dateien anzeigen*/
LS_NOFILE,	/*%s: `%s': %s\n*/
LS_WRONGW,	/*falsche Breitenangabe*/
LS_NOMEMD,	/*%s: nicht genug Speicher fr %s\n*/
};

#define NAMESIZE 20		/* hand-crafted to make sizeof(finfo)==32 */


typedef struct finfo
{
	union
	{
		char *ptr;
		char str[NAMESIZE];
	} name;
	unsigned int date, time;
	long size;
	struct {
		unsigned allocname : 1;
		unsigned expand : 1;
		unsigned sort_reverse : 1;
	} flags;
	char attrib;
	unsigned int fmode;
	unsigned int rdev;
	unsigned long index;
	unsigned int nlink;
	unsigned int uid, gid;
	MACINFO macinfo;
} finfo;

#define fname(f)	((f)->flags.allocname ? (f)->name.ptr : (f)->name.str)


/* Option flags */

/* long_format for lots of info, one per line.
   one_per_line for just names, one per line.
   many_per_line for just names, many per line, sorted vertically.
   horizontal for just names, many per line, sorted horizontally.
   with_commas for just names, many per line, separated by commas.

   -l, -1, -C, -x and -m control this parameter.  */

enum format
{
  long_format,			/* -l */
  one_per_line,			/* -1 */
  many_per_line,		/* -C */
  horizontal,			/* -x */
  with_commas			/* -m */	/* noch nicht implementiert */
};


/* The file characteristic to sort by.  Controlled by -t, -S, and -U. */

enum sort_type
{
  sort_none,			/* -U */
  sort_name,			/* default */
  sort_time,			/* -t */
  sort_size,			/* -S */
  sort_extension		/* -X */
};


/* none means don't mention the type of files.
   all means mention the types of all files.
   not_programs means do so except for executables.

   Controlled by -F and -p.  */

enum indicator_style
{
  none,				/* default */
  all,				/* -F */
  not_programs		/* -p */
};

typedef struct pendir
{
	char *name;
	struct pendir *next;
} PENDIR;


/* A linked list of shell-style globbing patterns.  If a non-argument
   file name matches any of these patterns, it is omitted.
   Controlled by -I.  Multiple -I options accumulate.
   The -B option adds `*~' and `.*~' to this list.  */

struct ignore_pattern
{
  char *pattern;
  struct ignore_pattern *next;
};

#define FSIZEVAL	100	/* start & increment value for nfiles */

typedef struct
{
	/* maximum file name length in this directory */

	int maxlen;

	/* Direction of sort.
	 * 0 means highest first if numeric,
	 * lowest first if alphabetic;
	 * these are the defaults.
	 * 1 means the opposite order in each case.  -r 
	 */
	int sort_reverse;

	/* Nonzero means mention the size in 512 byte blocks of each file. -s */
	int print_block_size;

	/* -k: show file sizes in kilobytes instead of blocks (the size
	   of which is system-dependant). */
	int kilobyte_blocks;

	/* -R: when a directory is found, display info on its contents. */
	int trace_dirs;

	/* -d: when an argument is a directory name, display info on it
	   itself. */
	int immediate_dirs;

	/* -q: output nongraphic chars in file names as `?'. */
	int qmark_funny_chars;

	/* -M: Merge all information in one long list (Gereon's format) */
	int merged_list;

	/* Print directory name above listing */
	int print_dir_name;

	/* The number of digits to use for block sizes.
    4, or more if needed for bigger numbers. */
	int block_size_size;

	/* The number of digits to use for the file size (def: 6) */
	int fsize_size;
	
	/* The number of digits to use for the # of links (def: 1) */
	int nlink_size;

	/* The number of digits to use for the optional inode (def: 1) */
	int inode_size;

	/* Number of characters for user and group id */
	int uid_size, gid_size;

	/* Language for time format */
	int german_time_format;

	/* use digits for timestamp */
	int digits;

	/* Current date */
	time_t date;

	/* Nonzero means don't omit files whose names start with `.'. -A */
	int all_files;

	/* Nonzero means don't omit files `.' and `..' This flag implies
	`all_files'.  -a */
	int really_all_files;

	/* The line length to use for breaking lines in many-per-line 
	format. Can be set with -w.  */
	int line_length;

	/* attribut byte for Fsfirst */
	int with_label;
	
	/* show inodes */
	int show_inodes;
	
	/* -H: show HFS information instead of Unix information */
	int show_HFS;

	/* use ctime instead of mtime */
	int what_time;
#define MOD_TIME 0
#define ACC_TIME 1
#define STA_TIME 2

	/* dereference symbolic links */
	int dereference;
	
	enum format format;
	enum sort_type sort_type;
	enum indicator_style indicator_style;

	/* Pointer to list of pending dirs
	 */
	PENDIR *pending_dirs;

	struct ignore_pattern *ignore_patterns;


	char *cmdname;	/* points to argv[0] */

	finfo *files;
	long files_index;		/* index into files */
	long nfiles;		/* size of files */

} LSINFO;

/*
 *	Discards trailing \\
 *	cares for . and ..
 */
static long
getNormXattr (MGLOBAL *M, char *filename, int follow, int labels,
	XATTR *X)
{
	long stat;
	char *normalized;

	normalized = NormalizePath (&M->alloc, filename,
		M->current_drive, M->current_dir);

	if (!normalized) return EPTHNF;

	if (lastchr (normalized) == '\\')
		normalized[strlen(normalized)-1] = 0;

	stat = GetXattr (normalized, labels, follow, X);
	
	mfree (&M->alloc, normalized);
	return stat;	
}
	

/* test for something like a: */

static int
is_drivespec (char *name)
{
	if (strlen (name) != 2) return FALSE;

	return name[1] == ':';
}

/* care for things like f:\ */

static int
might_be_dir (MGLOBAL *M, LSINFO *L, char *filename)
{
	XATTR X;
	long ret;
	
	if (filename[1] == ':')
		if (!LegalDrive (toupper(filename[0])))
			return FALSE;

	ret = getNormXattr (M, filename, L->dereference, L->with_label, &X);

	if (IsBiosError (ret))
	{
		ioerror (M, L->cmdname, filename, ret);
		return FALSE;
	}
		
	return (ret != -34);	/* -34: illegal dir */
}

/* Return non-zero if `name' doesn't end in `.' or `..'
   This is so we don't try to recurse on `././././. ...' */

static int
is_not_dot_or_dotdot (MGLOBAL *M, char *name)
{
	char *t;

	if (!name)
	{
		eprintf (M, "murks in name\n");
		return FALSE;
	}

	t = strrchr (name, '\\');
	if (t) name = t + 1;

	if (name[0] == '.'
		&& (name[1] == '\0'
		|| (name[1] == '.' && name[2] == '\0')))
    return FALSE;

	return TRUE;
}

static int
clear_files (MGLOBAL *M, LSINFO *L)
{
	L->files_index = 0L;
	L->nfiles = FSIZEVAL;
	L->maxlen = 0;
	L->files = mcalloc (&M->alloc, L->nfiles, sizeof(finfo));

	return (L->files != NULL);
}

static void
free_files (MGLOBAL *M, LSINFO *L)
{
	long i;

	if (!L->files)
		return;

	for (i = L->files_index - 1; i >= 0; --i)
	{
		if (L->files[i].flags.allocname)
			mfree (&M->alloc, L->files[i].name.ptr);
	}
			
	mfree (&M->alloc, L->files);
	L->files = NULL;	
}

/* Round file size to # of blocks */

static long
block_count (LSINFO *L, long size)
{
	if (L->kilobyte_blocks)
		return (size + 1023L) / 1024L;
	else
		return (size + 511L) / 512L;
}



/* Add `directory' to the list of pending directories */

static int
queue_directory (MGLOBAL *M, LSINFO *L, char *directory)
{
	PENDIR *new;

	new = mmalloc (&M->alloc, sizeof (PENDIR));

	if (!new)
	{
		eprintf (M, NlsStr(LS_NOMEMD), L->cmdname, directory);
		return FALSE;
	}
	
	new->name = StrDup (&M->alloc, directory);

	if (!new->name)
	{
		eprintf (M, NlsStr(LS_NOMEMD), L->cmdname, directory);
		mfree (&M->alloc, new);
		return FALSE;
	}
	
	new->next = L->pending_dirs;
	L->pending_dirs = new;
	return TRUE;
}


/* Remove any entries from `files' that are for directories (if -M),
   and queue them to be listed as directories instead. `dirname'
   is the prefix to prepend to each dirname to make it correct
   relative to ls's working dir. `recursive' is nonzero if we
   should not treat `.' and `..' as dirs. This is desirable when
   processing directories recursively.  */

static void
extract_dirs_from_files (MGLOBAL *M, LSINFO *L, char *dirname,
	int recursive, int remove_dirs)
{
	long i, j;
	char path[256];	/* enuff' for long paths! */

	if (!L->files) return;

	/* Queue the directories last one first, because queueing
	reverses the order. */

	for (i = L->files_index - 1; i >= 0; --i)
	{
		char *filename = fname(&L->files[i]);

		if ((L->files[i].attrib & FA_SUBDIR) &&
			(!recursive || is_not_dot_or_dotdot (M, filename)) &&
			L->files[i].flags.expand)
		{
			if (filename[0] == '\\' || dirname[0] == 0 ||
				!strcmp (dirname, "."))
				queue_directory (M, L, filename);
			else
			{
				strcpy (path, dirname);
				AddFileName (path, filename);
				queue_directory (M, L, path);
	  		}
	  	}
	}

	if (!remove_dirs) return;

	/* Now delete the directories from the table, compacting all the
	remaining entries. */

	if (L->files_index <= 0) return;
		
	for (i = 0, j = 0; i < L->files_index; i++)
	{
		if ((L->files[i].attrib & FA_SUBDIR)
			&& L->files[i].flags.expand)
		{
			if (L->files[i].flags.allocname)
				mfree (&M->alloc, L->files[i].name.ptr);		
		}
		else
			L->files[j++] = L->files[i];
	}
	
	L->files_index = j;
}

/* Add `pattern' to the list of patterns for which files that match are
   not listed.  */

static int
add_ignore_pattern (MGLOBAL *M, LSINFO *L, char *pattern)
{
	struct ignore_pattern *ignore;
	
	ignore = mmalloc (&M->alloc, sizeof (struct ignore_pattern));
	if (!ignore)
		return FALSE;
	
	ignore->pattern = pattern;
	/* Add it to the head of the linked list. */
	ignore->next = L->ignore_patterns;
	L->ignore_patterns = ignore;
	return TRUE;
}

static void
free_ignore_patterns (MGLOBAL *M, LSINFO *L)
{
	struct ignore_pattern *i = L->ignore_patterns;
	
	while (i)
	{
		struct ignore_pattern *nxt = i->next;
		mfree (&M->alloc, i);
		i = nxt;
	}
}

static int
file_interesting (LSINFO *L, char *filename)
{
	struct ignore_pattern *ignore;

	for (ignore = L->ignore_patterns; ignore; ignore = ignore->next)
	{
		if (wildmatch (filename, ignore->pattern, TRUE))
			return FALSE;
	}

	if (L->really_all_files
		|| filename[0] != '.'
		|| (L->all_files
		&& filename[1] != '\0'
		&& (filename[1] != '.' || filename[2] != '\0')))
	{
		return TRUE;
	}

	return FALSE;
}

static int
fappend (MGLOBAL *M, LSINFO *L, XATTR *xattr, MACINFO *mp, char *name,
	int explicit_arg, char *symlink)
{
	finfo *f = &L->files[L->files_index];
	int len;

	if ((xattr->attr & FA_HIDDEN) && !L->all_files)	{
		mfree (&M->alloc, name);
		return TRUE;
	}
	
	if (name == NULL) return FALSE;

	if (mp)
		f->macinfo = *mp;
	else
		memset (&f->macinfo, ' ', sizeof (MACINFO));
	
	switch (L->what_time)
	{
		case MOD_TIME:
			f->time = xattr->mtime;
			f->date = xattr->mdate;
			break;
			
		case STA_TIME:
			f->time = xattr->ctime;
			f->date = xattr->cdate;
			break;
			
		case ACC_TIME:
		default:
			f->time = xattr->atime;
			f->date = xattr->adate;
			break;
	}
	
	f->size = xattr->size;
	f->attrib = xattr->attr;
	f->fmode = xattr->mode;
	f->nlink = xattr->nlink;
	f->rdev = xattr->reserved1;
	f->index = xattr->index;
	f->uid = xattr->uid;
	f->gid = xattr->gid;
	
	f->flags.expand = (explicit_arg && !L->immediate_dirs) ? TRUE : FALSE;

	if ((xattr->mode & S_IFMT) == S_IFLNK)
	{
		if (L->format == long_format && symlink[0] != '\0')
		{
			char *new;

			new = mmalloc (&M->alloc, strlen (symlink) + strlen (name) + 6);
			if (new != NULL)
			{
				sprintf (new, "%s -> %s", name, symlink);
				mfree (&M->alloc, name);
				name = new;
			}
		}
		else if (L->indicator_style != none)
		{
			char *new;

			new = mmalloc (&M->alloc, strlen (name) + 2);
			if (new != NULL)
			{
				strcpy (new, name);
				strcat (new, "@");
				mfree (&M->alloc, name);
				name = new;
			}
		}	                                                       
	}
	
	f->name.ptr = name;
	f->flags.allocname = TRUE;
	f->flags.sort_reverse = L->sort_reverse;

	len = (int)strlen (name);
	if (len > L->maxlen) L->maxlen = len;

	++L->files_index;
	if (L->files_index >= L->nfiles)
	{
		finfo *newfiles;

		L->nfiles += FSIZEVAL;
		newfiles = mcalloc (&M->alloc, L->nfiles, sizeof (finfo));
		
		if (!newfiles)
		{
			free_files (M, L);
		}
		else	
		{
			memcpy (newfiles, L->files, 
				(L->nfiles - FSIZEVAL) * sizeof(finfo));
			mfree (&M->alloc, L->files);
			L->files = newfiles;
		} 
	}
	
	return (L->files != NULL);
}

/* called if qmark_funny_chars == TRUE */

static void
conv_filename (char *name)
{
	char *c = name;
	
	while (*c)
		if (*c & 0xe0)
			c++;
		else
			*c++ = '?';
}

/* return type character according to file mode and TOS attr */

static char
filetype_char (unsigned int m, int attrib)
{
	if (attrib & FA_VOLUME)
		return 'v';
	else if (S_ISDIR (m))
		return 'd';
	else if (S_ISBLK (m))
		return 'b';
	else if (S_ISCHR (m))
		return 'c';
	else if (S_ISFIFO (m))
		return 'p';
	else if (S_ISLNK (m))
		return 'l';
	else if (S_ISMEM (m))
		return 'm';
	else
		return '-';
}

/* fill file size string */

static char *
filesize_string (LSINFO *L, unsigned int mode, unsigned long size,
	unsigned int rdev, char *dst)
{
	if (S_ISCHR (mode) || S_ISBLK (mode))
		sprintf (dst, "%*d,%3d", L->fsize_size - 4,
			rdev >> 8, rdev & 0xff);
	else
		sprintf (dst, "%*lu", L->fsize_size, size);

	return dst;
}

/* fill file mode string */

static char *
filemode_string (int attr, unsigned int mode, char *dst)
{
	char *d = dst;
	
	*d++ = mode & S_IRUSR ? 'r' : '-';
	*d++ = mode & S_IWUSR ? 'w' : '-';

	if (mode & S_ISUID)
		*d++ = mode & S_IXUSR ? 's' : 'S';
	else
		*d++ = mode & S_IXUSR ? 'x' : '-';

	*d++ = mode & S_IRGRP ? 'r' : '-';
	*d++ = mode & S_IWGRP ? 'w' : '-';

	if (mode & S_ISGID)
		*d++ = mode & S_IXGRP ? 's' : 'S';
	else
		*d++ = mode & S_IXGRP ? 'x' : '-';

	*d++ = mode & S_IROTH ? 'r' : '-';
	*d++ = mode & S_IWOTH ? 'w' : '-';

	if (mode & S_ISVTX)
		*d++ = mode & S_IXOTH ? 't' : 'T';
	else
		*d++ = mode & S_IXOTH ? 'x' : '-';

	/* System Hidden Archive   CHAR
       0      0      0         
       0      0      1         a
       0      1      0         h
       0      1      1         b
       1      0      0         -
       1      0      1         A
       1      1      0         H
       1      1      1         B */
       
	
	{
		static char chars[] = " ahb-AHB";
		int charnum = 0;
		
		if (attr & FA_ARCHIVE) charnum |= 1;
		if (attr & FA_HIDDEN) charnum |= 2;
		if (attr & FA_SYSTEM) charnum |= 4;
		
		*d++ = chars[charnum];
	}

	*d = '\0';

	return dst;
}

static time_t
tostime_to_unix (unsigned int date, unsigned int time)
{
	struct tm t;

	if (!date && !time) return -1;
	
	t.tm_sec = (time & 31) * 2;
   	t.tm_min = (time >> 5) & 63;
	t.tm_hour = (time >> 11) & 63;
	t.tm_mday = (date & 31);
	t.tm_mon = ((date >> 5) & 15) - 1;
	t.tm_year = 80 + ((date >> 9) & 127);
	t.tm_yday = t.tm_isdst = 0;

	return mktime (&t);
}

/* test for older than 6 months or newer then today */

static int
older_6_months (LSINFO *L, unsigned int date, unsigned int time)
{
	time_t fdate = tostime_to_unix (date, time);

	return (L->date - fdate > 6L * 30L * 24L * 60L * 60L ||
			L->date - fdate < 0L);
}


/* fill time string */

static char *
time_string (LSINFO *L, unsigned int date, unsigned int time, char *dst)
{
	char *d = dst;
	dosdate fd;
	dostime ft;

	fd.d = date;
	ft.t = time;

	if (L->digits)
	{
		if (!time && !date)
			d += IDTSprintf (d, 70, 1, 1);
		else
			d += IDTSprintf (d, (fd.s.year + 80) % 100, fd.s.month, fd.s.day);

		sprintf (d, " %02d:%02d:%02d", ft.s.hour, ft.s.min, 2 * ft.s.sec);
	}
	else
	{
		static char *GermanMonths[] = { "???", "Jan", "Feb", "M„r", "Apr", "Mai",
			"Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez" };
		static char *EnglishMonths[] = { "???", "Jan", "Feb", "Mar", "Apr", "May",
			"Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

		char **months = EnglishMonths;
		
		int month = fd.s.month;
		
		if (month < 1 || month > 12) month = 0;

		if (L->german_time_format) months = GermanMonths;
		
		sprintf (d, "%s %02d %02d:%02d", months[month], fd.s.day,
			ft.s.hour, ft.s.min);
			
		if (older_6_months (L, date, time))
			sprintf (d + 7, " %04d", fd.s.year + 1980);

		if (!time && !date)
			sprintf (d, "%s 01  1970", months[1]);
	}
		
	return dst;
}

static char *
id_string (MGLOBAL *M, char *dest, int id, int is_uid)
{
	char *c;
	
	c = is_uid ? getUserName (M, id) : getGroupName (M, id);	
	
	if (c)
		return c;
	else
	{
		sprintf (dest, "%u", id);
		return dest;		
	}
}

static char *
hfs_string (char *dest, char *c)
{
	sprintf (dest, "%c%c%c%c", c[0], c[1], c[2], c[3]);
	return dest;
}

static int
showfile (MGLOBAL *M, LSINFO *L, finfo *f, int trailing)
{
	int l = 0;
	
	if (L->show_inodes)
	{
		if (trailing)
			l += mprintf (M, "%*lu ", L->inode_size, f->index);
		else
			l += mprintf (M, "%lu ", f->index);
	}

	if (L->print_block_size)
	{
		if (trailing)
			l += mprintf (M, "%*lu ", L->block_size_size, block_count (L, f->size));
		else
			l += mprintf (M, "%lu ", block_count (L, f->size));
	}
		
	if (L->format == long_format)
	{
		char fsizestring[12];
		char modestring[15];
		char timestring[25];
		char uidstring[30], gidstring[30];
		char typestring[5], creastring[5];
		
		filemode_string (f->attrib, f->fmode, modestring);
		filesize_string (L, f->fmode, f->size, f->rdev, fsizestring);
		time_string (L, f->date, f->time, timestring);

		mprintf (M, "%c%s%*u %-*s %-*s %s %s ",
			filetype_char (f->fmode, f->attrib),
			modestring,
			L->nlink_size, f->nlink,
			L->show_HFS ? 4 : L->uid_size, 
			L->show_HFS ? hfs_string (typestring, f->macinfo.type)
						: id_string (M, uidstring, f->uid, 1),
			L->show_HFS ? 4 : L->gid_size,
			L->show_HFS ? hfs_string (creastring, f->macinfo.creator)
						: id_string (M, gidstring, f->gid, 0),
			fsizestring,
			timestring);
	}

	if (L->qmark_funny_chars)
		conv_filename (fname(f));

	l += mprintf (M, "%s", fname(f));
		
	switch (L->indicator_style)
	{
		case all:
			if (!(f->attrib & FA_SUBDIR) &&
				!(f->attrib & FA_VOLUME) &&
				ExtInSuffix (M, fname (f)))
			{
				l += mprintf(M, "*");
			}
			else if (f->attrib & FA_VOLUME)
			{
				l += mprintf (M, "%%");
			}

			/* fall thru!!! */

		case not_programs:
		{
			const char *filename = fname (f);
			
			if ((f->attrib & FA_SUBDIR)
				&& (filename[strlen (filename)-1] != '\\'))
				l += mprintf(M, "/");	/* xxx */
		}
	}

	/* Argh! Filenames with leading blanks have no length!!! */
	return intr (M) ? 0 : l+1;
}

static int
outcols (MGLOBAL *M, LSINFO *L)
{
	finfo *h = &L->files[0];
	finfo *h1;
	finfo *last = &L->files[L->files_index];
	int numcols, i, cols, nelem, height, leftcols, cnt;
	int l;

	cnt = 0;
	nelem = (int)L->files_index;
	if (nelem <= 0)
		return !intr(M);
		
	L->maxlen += 2;	/* wg. Spaltenabstand */

	if (L->maxlen > L->line_length)
		L->line_length = L->maxlen + 1;
		
	numcols = L->line_length / L->maxlen;
	
	height = nelem / numcols;
	leftcols = nelem % numcols;
	
	h1 = h;
	do
	{
		if (cnt >= (height*numcols))
			break;
			
		for (cols=0; cols<numcols && !intr (M); ++cols)
		{
			l = showfile (M, L, h1, TRUE)-1;

			if (cols != numcols-1) rawoutn(M, ' ', L->maxlen - l);
			++cnt;

			if (L->format == many_per_line)
				++h1;
			else
			{
				for (i=0; i<height && h1<last; ++i)
					++h1;
				if (cols<leftcols && h1<last)
					++h1;
			}
		}
		crlf (M);
		
		if (L->format == horizontal)
		{
			++h;
			h1 = h;
		}
	} while (cnt < (height*numcols) && !intr (M));

	while (cnt < nelem && !intr (M))
	{
		l=showfile (M, L, h1, TRUE)-1;
		rawoutn(M, ' ', L->maxlen-l);
		
		if (L->format == many_per_line)
			++h1;
		else
		{
			for (i=0; i<height+1 && h1<last; ++i)
				++h1;
		}
		++cnt;
	}
	if (cnt % numcols != 0)
		crlf (M);

	return !intr (M);
}

static int
outcommas (MGLOBAL *M, LSINFO *L)
{
	int i;
	int line_count;
	
	L->maxlen += 2;
	line_count = 0;
	
	if (L->files_index > 0)
	{
		for (i = 0; i < L->files_index && !intr (M); ++i)
		{
			int lc = showfile (M, L, &L->files[i], FALSE);
			
			if (!lc) 
				return FALSE;
	
			line_count += lc;
			
			if (i != (L->files_index - 1))
				line_count += mprintf (M, ", ");
			else
				crlf (M);
			
			if (line_count + L->maxlen > L->line_length)
			{
				crlf (M);
				line_count = 0;
			}		
		}
	}
	
	return !intr(M);
}

static int
sortreturn(int sort_reverse, int cmp)
{
	return sort_reverse ? -cmp : cmp;
}

static int
namecmp(const finfo *f1, const finfo *f2)
{
	return sortreturn(f1->flags.sort_reverse, strcmp(fname(f1),fname(f2)));
}

static int
extcmp (const finfo *f1, const finfo *f2)
{
	char *ext1, *ext2;
	
	ext1 = strrchr (fname (f1), '.');
	if (!ext1)
		ext1 = (f1->attrib & (FA_SUBDIR|FA_VOLUME)) ? "\001" : "\002";
	
	ext2 = strrchr (fname (f2), '.');
	if (!ext2)
		ext2 = (f2->attrib & (FA_SUBDIR|FA_VOLUME)) ? "\001" : "\002";

	if (!strcmp (ext1, ext2))
		return namecmp (f1, f2);

	return sortreturn (f1->flags.sort_reverse, strcmp (ext1, ext2));
}

static int
sign (long x)
{
	if (x<0)
		return -1;
	if (x>0)
		return 1;
	return 0;
}

static int
sizecmp (const finfo *f1, const finfo *f2)
{
	if (f1->size == f2->size)
		return namecmp (f1, f2);
	else
		return sortreturn(f1->flags.sort_reverse, sign(f2->size - f1->size));
}

static int
timecmp (const finfo *f1, const finfo *f2)
{
	if (f1->date == f2->date)
	{
		if (f1->time == f2->time)
			return namecmp (f1, f2);
		else
			return sortreturn(f1->flags.sort_reverse, f2->time - f1->time);
	}
	else
		return sortreturn(f1->flags.sort_reverse, f2->date - f1->date);
}

/* Add all files in directory dirname to files, with full name
   returns 0 for success or GEMDOS error code */

static int
gobble_dir (MGLOBAL *M, LSINFO *L, char *dirname)
{
	WalkInfo W;
	char filename[MAX_FILENAME_LEN];
	int only_point = 0;
	long stat;
	int flags = WDF_ATTRIBUTES|WDF_LOWER|WDF_IGN_XATTR_ERRORS;
	
	if (L->with_label) flags |= WDF_LABEL;
	if (L->dereference) flags |= WDF_FOLLOW;
	if (L->show_HFS) flags |= WDF_GET_MAC_ATTRIBS;
	
	/* Read the directory entries, and insert the subfiles into the
	`files' table.  */

	stat = InitDirWalk (&W, flags, dirname, MAX_FILENAME_LEN, filename);

	if (!stat)
	{
		while ((stat = DoDirWalk (&W)) == 0L)
		{
	    	char *newname;
	    	
	    	newname = mmalloc (&M->alloc, 
	    		strlen (dirname) + strlen (filename) + 2);
	
	    	if (newname)
	    	{
	    		if (only_point)
	    			strcpy (newname, filename);
	    		else
	    		{
		    		strcpy (newname, dirname);
		    		AddFileName (newname, filename);
		   		}
	    		
		    	if (file_interesting (L, filename))
		    	{
		    		char slink[MAX_PATH_LEN];
		    		
					slink[0] = '\0';
					if (L->format == long_format &&
						(W.xattr.mode & S_IFMT) == S_IFLNK)
			    		DirReadSymlink (&W, (int) sizeof (slink), slink);
		    	
		    		if (!fappend (M, L, &W.xattr, &W.macinfo, newname,
		    			L->trace_dirs, slink))
		    		{
		    			eprintf (M, NlsStr(LS_NOMEMD), L->cmdname, dirname);
						mfree (&M->alloc, newname);
						ExitDirWalk (&W);
		    			return ENSMEM;
		    		}
		    	}
				else
					mfree (&M->alloc, newname);
			}
			else
			{
				eprintf (M, NlsStr(LS_NOMEMD), L->cmdname, dirname);
				ExitDirWalk (&W);
	   			return ENSMEM;
			}
		}

		ExitDirWalk (&W);
	}

	return 0;	/* ok */
}


/* Add a file to the current table of files.
   Verify that the file exists, and print an error message if it does
   not. Return the number of blocks that the file occupies.  */

static long
gobble_file (MGLOBAL *M, LSINFO *L, char *filename, int explicit_arg)
{
	XATTR X;
	long stat;
	int ret = 0;
		
	if (ValidFileName (filename))
	{
		int follow = L->dereference;
		
		/* jr: Hack, ist vielleicht noch nicht korrekt */
		if (explicit_arg && L->format != long_format)
			follow = TRUE;

		stat = getNormXattr (M, filename, follow, L->with_label, &X);
		if (IsBiosError (stat))
			return ioerror (M, L->cmdname, filename, stat);
	}
	else
		stat = EPTHNF;
		
	if (stat == E_OK)	/* Fsfirst ok? */
	{
/* mprintf ("%x %o\n", X.attr, X.mode); */

		if ((X.attr & FA_SUBDIR) && L->merged_list)
			gobble_dir (M, L, filename);	
		else
		{
    		char slink[MAX_PATH_LEN];
    		MACINFO m;
		    		
			slink[0] = '\0';
			if (L->format == long_format &&
				(X.mode & S_IFMT) == S_IFLNK)
	    		Freadlink ((int) sizeof (slink), slink, filename);
	    	
	    	memset (&m, ' ', sizeof (MACINFO));
	    	if (L->show_HFS && !(S_ISDIR (X.mode)))
	    		GetMacFileinfo (filename, m.type, m.creator);
		
			if (!fappend (M, L, &X, &m, StrDup (&M->alloc, filename), 
				explicit_arg, slink))
			{
				eprintf (M, "%s: %s\n", L->cmdname, StrError (ENSMEM));
				ret = ENSMEM;
			}
		}
	}
	else
	{
		if (stat != EPTHNF && stat != EFILNF 
			&& might_be_dir (M, L, filename) 
			&& !L->immediate_dirs)
		{
			if (L->merged_list)
				gobble_dir (M, L, filename);
			else
				queue_directory (M, L, filename);
		}
		else
		{
			eprintf (M, NlsStr(LS_NOFILE), L->cmdname,
				filename, StrError ((int)stat));
			ret = -1;
		}
	}
	
	return ret;
}

static int
print_current_files (MGLOBAL *M, LSINFO *L)
{
	int ret = TRUE;

	if (!L->files) return FALSE;

	if (L->files_index > 0)
	{
		long maxsize = 0L;
		long maxfsize = 1000000L;	/* default: 7 digits */
		long maxlinks = 1;
		long maxinode = 1;
		int maxuid = 3, maxgid = 3;
		long i;

		for (i = 0; i < L->files_index; i++)
		{
			long merk;
			char *cp;
			
			merk = block_count (L, L->files[i].size);
			if (merk > maxsize) maxsize = merk;
			
			if (L->files[i].size > maxfsize)
				maxfsize = L->files[i].size;
				
			if (L->files[i].nlink > maxlinks)
				maxlinks = L->files[i].nlink;
				
			if (L->files[i].index > maxinode)
				maxinode = L->files[i].index;
			
			/* Only calculate max width when long format is required */
				
			if (L->format == long_format)
			{
				cp = getUserName (M, L->files[i].uid);
				if (cp && strlen (cp) > maxuid) maxuid = (int) strlen (cp);

				cp = getGroupName (M, L->files[i].gid);
				if (cp && strlen (cp) > maxgid) maxgid = (int) strlen (cp);
			}
		}
		
		{
			char tmp[15];
			
			L->block_size_size = sprintf (tmp, "%ld", maxsize);
			L->fsize_size = 1 + sprintf (tmp, "%ld", maxfsize);
			L->nlink_size = 1 + sprintf (tmp, "%ld", maxlinks);
			L->inode_size = sprintf (tmp, "%lu", maxinode);
			
			L->uid_size = maxuid;
			L->gid_size = maxgid;
		}

		if (L->print_block_size)
			L->maxlen += L->block_size_size + 2;
			
		/* add room for inode */
		if (L->show_inodes) L->maxlen += 1 + L->inode_size;

		switch (L->format)
		{
			case many_per_line:
			case horizontal:
				ret = outcols (M, L);
				break;
				
			case with_commas:
				ret = outcommas (M, L);
				break;

			default:
				for (i = 0; i < L->files_index && !intr (M); ++i)
				{
					showfile (M, L, &L->files[i], TRUE);
					crlf (M);
				}
				ret = !intr (M);
				break;
		}
	}

	return ret;
}

/* Sort the files now in the table.  */

static void
sort_files (LSINFO *L)
{
	int (*sortfunc)(const finfo *f1, const finfo *f2);

	if (!L->files) return;

	switch (L->sort_type)
    {
    	case sort_none:
      		return;
		case sort_time:
			sortfunc = timecmp;
			break;			
		case sort_name:
			sortfunc = namecmp;
			break;
		case sort_size:
			sortfunc = sizecmp;
			break;
		case sort_extension:
			sortfunc = extcmp;
			break;
    }
    
	qsort (L->files, L->files_index, sizeof (finfo), sortfunc);
}

/* Read directory `name', and list the files in it. */

static int
print_dir (MGLOBAL *M, LSINFO *L, char *dirname)
{
	WalkInfo W;
	char filename[MAX_FILENAME_LEN];
	long total_blocks = 0L;
	int retcode = 1;
	long stat;
	int flags = WDF_ATTRIBUTES|WDF_LOWER|WDF_IGN_XATTR_ERRORS;

	if (L->with_label) flags |= WDF_LABEL;
	if (L->dereference) flags |= WDF_FOLLOW;
	if (L->show_HFS) flags |= WDF_GET_MAC_ATTRIBS;

	/* Read the directory entries, and insert the subfiles into the
	`files' table.  */

	if (!clear_files (M, L))
	{
		eprintf (M, NlsStr(LS_NOMEMD), L->cmdname, dirname);
		return FALSE;
	}

	stat = InitDirWalk (&W, flags, dirname, MAX_FILENAME_LEN, filename);
		
	if (stat && (stat != EFILNF))	/* Not File not found or ok */
	{
		ExitDirWalk (&W);
		free_files (M, L);
		return FALSE;
	}
	
	if (!stat)
	{
		while ((stat = DoDirWalk (&W)) == 0L)
		{
		    if (file_interesting (L, filename))
		    {
	    		char slink[MAX_PATH_LEN];
		    		
				slink[0] = '\0';
				if (L->format == long_format &&
					(W.xattr.mode & S_IFMT) == S_IFLNK)
		    		DirReadSymlink (&W, (int) sizeof (slink), slink);
		    	
		    	if (!fappend (M, L, &W.xattr, &W.macinfo,
		    		StrDup (&M->alloc, filename), L->trace_dirs, slink))
		    	{
		    		ExitDirWalk (&W);
		    		eprintf (M, NlsStr(LS_NOMEMD), L->cmdname, filename);
		    		return FALSE;
		    	}
		    	
		    	total_blocks += block_count (L, W.xattr.size);
			}
		}
		
		ExitDirWalk (&W);
	}

	/* Sort the directory contents.  */
	sort_files (L);

	/* If any member files are subdirectories, perhaps they should have their
	contents listed rather than being mentioned here as files.  */

	if (L->trace_dirs)
		extract_dirs_from_files (M, L, dirname, TRUE, FALSE);

	if (L->print_dir_name)
		mprintf (M, "%s:\n", dirname);

	if (L->format == long_format || L->print_block_size)
		printf ("total %ld\n", total_blocks);

	if (L->files_index)
		retcode = print_current_files (M, L);

	if (L->pending_dirs)
		crlf (M);
	
	free_files (M, L);

	return retcode;
}




int
m_ls (MGLOBAL *M, int argc, char **argv)
{
	LSINFO L;
	GETOPTINFO G;
	int c, retcode = 0;
	int dir_defaulted = TRUE;
	
	static struct option long_options[] =
	{
		{"all",				0, NULL, 'a'},
		{"ctime",			0, NULL, 'c'},
		{"directory",		0, NULL, 'd'},
		{"inode",			0, NULL, 'i'},
		{"kilobytes",		0, NULL, 'k'},
		{"long-format",		0, NULL, 'l'},
		{"comma-format",	0, NULL, 'm'},
		{"append-backslash",0, NULL, 'p'},
		{"hide-control-chars",	0, NULL, 'q'},
		{"reverse",			0, NULL, 'r'},
		{"size",		0, NULL, 's'},
		{"time-sort",		0, NULL, 't'},
		{"atime",		0, NULL, 'u'},
		{"horizontal-format",	0, NULL, 'x'},
		{"almost-all",		0, NULL, 'A'},
		{"ignore-backups",	0, NULL, 'B'},
		{"vertical-format",	0, NULL, 'C'},
		{"digits",			0, NULL, 'D'},
		{"classify",		0, NULL, 'F'},
		{"hfs",				0, NULL, 'H'},
		{"dereference",		0, NULL, 'L'},
		{"merged-list",		0, NULL, 'M'},
		{"recursive",		0, NULL, 'R'},
		{"size-sort",		0, NULL, 'S'},
		{"no-sort",			0, NULL, 'U'},
		{"extension-sort",	0, NULL, 'X'},
		{"single-column",	0, NULL, '1'},
		{"width",			1, NULL, 'w'},
		{"ignore",			1, NULL, 'I'},
		{NULL, 0,0,0},
	};
	int opt_index = 0;

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_ls), 
			NlsStr(HLP_ls));
	
	L.cmdname = argv[0];
	
	L.print_dir_name = 1;
	L.trace_dirs = FALSE;
	L.immediate_dirs = FALSE;
	L.all_files = L.really_all_files = FALSE;
	L.print_block_size = L.kilobyte_blocks = FALSE;
	L.indicator_style = none;
	L.ignore_patterns = NULL;
	L.dereference = FALSE;
	L.show_inodes = FALSE;
	L.show_HFS = FALSE;
	L.digits = FALSE;

	if (GetEnv(M, "COLUMNS"))
		L.line_length = atoi (GetEnv (M, "COLUMNS"));
	else
		L.line_length = 80;

	L.format = isatty (1) ? horizontal : one_per_line;
	
	L.sort_type = sort_name;
	L.sort_reverse = FALSE;
	L.with_label = FALSE;
	L.merged_list = FALSE;
	L.pending_dirs = NULL;
	L.qmark_funny_chars = FALSE;

	L.what_time = MOD_TIME;
	L.german_time_format = DO_LOCALE_GERMAN == GetLocale (M, "LC_TIME");

	L.date = tostime_to_unix (Tgetdate (), Tgettime ());
	
	if (!strcmp (argv[0], "dir"))
		L.format = horizontal;
	else if (!strcmp (argv[0], "ls"))
		L.qmark_funny_chars = isatty (1);
	else if(!strcmp (argv[0], "vdir"))
		L.format = long_format;

	optinit (&G);

	while ((c = getopt_long (M, &G, argc, argv, "acdiklmpqrstuxABCDFLHMRSUX1w:I:",
		long_options, &opt_index)) != EOF)
	{
		if (!c)			c = long_options[G.option_index].val;
		switch (c)
		{
			case 0:
				break;
			case 'a':
				L.all_files = L.really_all_files = TRUE;
				L.with_label = TRUE;	/* include volume labels */
				break;
			case 'c':
				L.what_time = STA_TIME;
				break;
			case 'd':
				L.immediate_dirs = TRUE;
				break;
			case 'i':
				L.show_inodes = TRUE;
				break;
			case 'k':
				L.kilobyte_blocks = TRUE;
				break;
			case 'l':
				L.format = long_format;
				break;
			case 'm':
				L.format = with_commas;
				break;
			case 'p':
				L.indicator_style = not_programs;
				break;
			case 'q':
				L.qmark_funny_chars = TRUE;
				break;
			case 'r':
				L.sort_reverse = TRUE;
				break;
			case 's':
				L.print_block_size = TRUE;
				break;
			case 't':
				L.sort_type = sort_time;
				break;
			case 'u':
				L.what_time = ACC_TIME;
				break;
			case 'w':
				L.line_length = atoi (G.optarg);
				if (L.line_length < 1)
				{
					eprintf (M, NlsStr(LS_WRONGW), L.cmdname);
					return 1;
				}
				break;
			case 'x':
				L.format = many_per_line;
				break;
			case 'A':
				L.all_files = TRUE;
				break;
			case 'B':
				add_ignore_pattern (M, &L, "*.DUP");
				add_ignore_pattern (M, &L, "*.BAK");
				add_ignore_pattern (M, &L, "*~");
				break;
			case 'C':
				L.format = horizontal;
				break;
			case 'D':
				L.digits = TRUE;
				break;
			case 'F':
				L.indicator_style = all;
				break;
			case 'H':
				L.show_HFS = TRUE;
				break;
			case 'I':
				add_ignore_pattern (M, &L, G.optarg);
				break;
			case 'L':
				L.dereference = TRUE;
				break;
			case 'M':
				L.merged_list = TRUE;
				break;
			case 'R':
				L.trace_dirs = TRUE;
				break;
			case 'S':
				L.sort_type = sort_size;
				break;
			case 'U':
				L.sort_type = sort_none;
				break;
			case 'X':
				L.sort_type = sort_extension;
				break;
			case '1':
				L.format = one_per_line;
				break;
			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_ls));
		}
	}
	
	if (!clear_files (M, &L))
	{
		eprintf (M, "%s: %s\n", L.cmdname, StrError (ENSMEM));
		return 2;
	}

	if (G.optind < argc)
	{
		dir_defaulted = FALSE;

		for (; G.optind < argc /* && !retcode */; ++G.optind)
			retcode = (int)gobble_file (M, &L, argv[G.optind], TRUE);
	}
	
	if (dir_defaulted)
	{
		if (L.immediate_dirs)
			gobble_file (M, &L, ".", TRUE);
		else
			queue_directory (M, &L, ".");
    }

	if (L.merged_list)	/* loop until no more dirs */
	{
		extract_dirs_from_files (M, &L, "", TRUE, TRUE);
		
		while (L.pending_dirs)
		{
			PENDIR *thispend;		

			thispend = L.pending_dirs;
			L.pending_dirs = L.pending_dirs->next;
		
			if (!retcode)
				if (gobble_dir (M, &L, thispend->name))
					retcode = 2;

			mfree (&M->alloc, thispend->name);
			mfree (&M->alloc, thispend);	
			extract_dirs_from_files (M, &L, "", TRUE, TRUE);
		}
	
		sort_files (&L);
	}

	if (L.files_index > 0)
	{
		sort_files (&L);
		if (!L.immediate_dirs)
			extract_dirs_from_files (M, &L, "", FALSE, TRUE);

		/* `files_index' might be zero now.  */
    }

	if (L.files_index > 0)
	{
		print_current_files (M, &L);
		if (L.pending_dirs) crlf (M);
    }
	else
		if (L.pending_dirs && !L.pending_dirs->next)
			L.print_dir_name = FALSE;

	free_files (M, &L);

	while (L.pending_dirs)
	{
		PENDIR *thispend;		

		thispend = L.pending_dirs;
		L.pending_dirs = L.pending_dirs->next;
		
		if (!retcode)
			if (!print_dir (M, &L, thispend->name))
				retcode = 2;

		mfree (&M->alloc, thispend->name);
		mfree (&M->alloc, thispend);	
		L.print_dir_name = TRUE;
	}
	
	free_ignore_patterns (M, &L);
	return retcode;
}

/* Functions for user and group names */

void
ExitUsersGroups (MGLOBAL *M)
{
	IDINFO *next, *i;
	
	i = M->users;
	
	while (i)
	{
		next = i->next;
		mfree (&M->alloc, i);
		i = next;
	}
	
	M->users = NULL;
	
	i = M->groups;
	
	while (i)
	{
		next = i->next;
		mfree (&M->alloc, i);
		i = next;
	}
	
	M->groups = NULL;
}


static void
read_file (MGLOBAL *M, const char *fn, IDINFO **i)
{
	char fullname[40];
	MEMFILEINFO *mem;
	long ret = EFILNF;
	char line[256];
	
	if (LegalDrive ('U')) {
		sprintf (fullname, "u:\\etc\\%s", fn);
		mem = mopen (&M->alloc, fullname, &ret);
	}
	
	if (ret != E_OK) {
		sprintf (fullname, "c:\\etc\\%s", fn);
		mem = mopen (&M->alloc, fullname, &ret);
	}
	
	if (!mem) return;
	
	while (NULL != mgets (line, sizeof (line), mem))
	{
		char *id = strchr (line, ':');
		char *c = NULL;
		
		if (id) {
			*id = '\0';
			id = strchr (id + 1, ':');
			if (id) {
				id += 1;
				c = strchr (id, ':');
				if (c) *c = '\0';
			}
		}

		if (id && c)
		{
			IDINFO *new = mcalloc (&M->alloc, 1, sizeof (IDINFO));
			
			if (new)
			{
				new->id = atoi (id);
				strncpy (new->name, line, -1 + sizeof (new->name));
/* mprintf	(M, "%s %s\n", line, id); */

				*i = new;
				i = &new->next;
			}
		}
	}
	
	mclose (&M->alloc, mem);
}


void
InitUsersGroups (MGLOBAL *M)
{
	if (!M->users) read_file (M, "passwd", &M->users);
	if (!M->groups) read_file (M, "group", &M->groups);

	if (!M->users)
	{
		IDINFO *test = mcalloc (&M->alloc, 1, sizeof (IDINFO));
	
		if (test) {
			strcpy (test->name, "root");
			M->users = test;	
		}
	}

	if (!M->groups)
	{
		IDINFO *test = mcalloc (&M->alloc, 1, sizeof (IDINFO));
	
		if (test) {
			strcpy (test->name, "system");
			M->groups = test;	
		}
	}
}

static char *
get_name (IDINFO *i, int id)
{
	for ( ; i ; i = i->next)
		if (i->id == id) return i->name;
	
	return NULL;
}

char *
getUserName (MGLOBAL *M, int id)
{
	InitUsersGroups (M);

	return get_name (M->users, id);
}

char *
getGroupName (MGLOBAL *M, int id)
{
	InitUsersGroups (M);

	return get_name (M->groups, id);
}
