/*
 *	@(#)Mupfel/cpmv.c
 *	@(#)Stefan Eissing & Gereon Steffens, 29. Juni 1994
 *
 * jr 31.12.1996
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\walkdir.h"
#include "..\common\strerror.h"
#include "mglob.h"

#include "chario.h"
#include "chmod.h"
#include "cpmv.h"
#include "cpmvutil.h"
#include "getopt.h"
#include "mdef.h"
#include "mkdir.h"
#include "nameutil.h"


/* internal texts
 */
#define NlsLocalSection "M.cpmv"
enum NlsLocalText{
OPT_cp,		/*[-adfinprRsv] f1 f2  oder  cp f1..fn dir*/
HLP_cp,		/*Dateien kopieren*/
OPT_mv,		/*[-fiv] f1 f2 oder mv f1..fn dir oder mv dir1 dir2*/
HLP_mv,		/*dateien umbenennen/verschieben*/
CP_ARCHWARN,/*%s: ACHTUNG: -a funktioniert erst ab TOS 1.04 (GEMDOS 0.15) richtig\n*/
CP_NODIR,	/*%s: `%s' ist kein Verzeichnis\n*/
CP_RENAME,	/*%s: kann `%s' nicht in `%s' umbenennen (%s)\n*/
CP_RECUR,	/*%s: Benutzen Sie -r um Verzeichnisse zu kopieren\n*/
CP_DUPFILE,	/*%s: `%s' und `%s' sind dieselbe Datei\n*/
CP_CANTCRT,	/*%s: kann `%s' nicht anlegen (%s)\n*/
CP_CPDONE,	 /*fertig.\n*/
CP_ILLNAME,	/*%s: `%s' ist kein gltiger Name.\n*/
CP_NOFILE,	/*%s: Datei `%s' existiert nicht\n*/
CP_READONLY,/*%s: `%s' ist schreibgeschtzt\n*/
CP_ENDLESS,	/*%s: `%s' als Quelle und `%s' als Ziel fhrt zu einer Endlosschleife\n*/
CP_ISWILDCARD,/*%s: `%s' enth„lt Wildcards\n*/
CP_RENAMING,/*benenne `%s' in `%s' um...*/
};

void
FreeCopyInfoContents (COPYINFO *C)
{
	if (C->buffer) Mfree (C->buffer);
}



static
int isDir (COPYINFO *C, const char *file)
{
	char *realname;
	
	realname = NormalizePath (&C->M->alloc, file,
		C->M->current_drive, C->M->current_dir);
	
	if (realname != NULL)
	{
		int retcode;
		
		retcode = IsDirectory (realname, &C->broken);
		mfree (&C->M->alloc, realname);
		
		return retcode;
	}
	else
	{
		C->broken = 1;
		return FALSE;
	}
}



/* Return the name of an unused filename in the same directory,
   suited as temp file. */
   
static char *
tempname (const char *origname, char *newname)
{
	long ret;

	do
	{
		char *c;

		strcpy (newname, origname);
		c = strrchr (newname, '\\');
		if (!c) c = strrchr (newname, ':');
		if (!c) c = newname; else c += 1;
	
		sprintf (c, "mv%06lx.tmp", 0xffffffL & Random ()); 

		ret = Fattrib (newname, 0, 0);
	} while (ret >= 0);
	
	return newname;
}


/* Intelligent implementation of rename() compatible to POSIX
   semantics. Note: returns ENMFIL when trying to mv file
   to itself */

static long
lrename (const char *old, const char *new)
{
	XATTR X;
	char newname[PATH_MAX];
	int reset_wmode = 0, lastmode;
	long ret;
	
	/* First attempt to use the OS call. Maybe it will do it
	   for us */
retry:
	ret = Frename (0, old, new);
	
	/* Maybe we have to change the file permission... */
	
	if (ret == EACCDN)
	{
		long lret = Fattrib (old, 0, 0);
		
		if (lret >= 0 && (lret & FA_READONLY))
		{
			lastmode = (int) lret;
		
			if (E_OK == Fattrib (old, 1, lastmode & ~FA_READONLY))
			{
				reset_wmode = 1;
				goto retry;
			}
		}
	}

	/* reset file mode when needed */
	if (reset_wmode) Fattrib (ret == E_OK ? new : old, 1, lastmode);

	/* If this succeeds or fails because of cross file system
	   access, we return */
	
	if (ret == E_OK || ret == ENSAME) return ret;
	
	/* Make sure that the Frename didn't fail for some other
	   reason than an existing destination file. Thus we test
	   whether the newname already exists. In this case, we
	   just pass back the error code */

	if (E_OK != GetXattr (new, 0, 1, &X)) return ret;
	   
	/* Now we should delete the target and then retry. However
	   we have to make sure that old and new are not identical.
	   Unfortunately, we can't trust the file index (TOSFS
	   under MiNT), nor can we do simple assumptions on filenames
	   (long/sort notation, case sensitivity in MacFS). Therefore
	   we rename the 'old' filename to a 'save' name, and then
	   try to remove the target */
	   
	tempname (old, newname);
	ret = Frename (0, old, newname);
	
	if (ret == E_OK)
	{
		/* If the file has now disappeared, it was probably the
		   same as the original... */

		if (EFILNF == Fdelete (new))
		{
			Frename (0, newname, old);
			return ENMFIL;
		}
		
		ret = Frename (0, newname, new);

		/* fallback... */
		if (ret != E_OK) Frename (0, newname, old);
	}

	return ret;
}



int
cp (COPYINFO *C, char *origfrom, char *origto, char *realfrom, 
	char *realto, int *really_was_copied)
{
	char *old_realto = NULL;
	char *old_origto = NULL;
	char choice;
	long in, out;
	int retcode;
	int src_attrib;

	*really_was_copied = FALSE;
	retcode = 0;
	if (!strcmp (realfrom, realto))
	{
		C->M->oserr = EACCDN;
		eprintf (C->M, NlsStr(CP_DUPFILE), C->cmd, origfrom);
		return 1;
	}

	/* allocate space for the copy buffer */	
	if (C->buffer == NULL)
	{
		long keep_free;
		
		keep_free = GetVarInt (C->M, "KEEPFREE");
		if (keep_free < MIN_KEEPFREE) keep_free = MIN_KEEPFREE;

		C->buffer_len = (long)Malloc (-1L);
		C->buffer_len -= keep_free;
		
		if (C->buffer_len < 1024L) C->buffer_len = 1024L;
			
		while ((C->buffer = Malloc (C->buffer_len)) == NULL)
		{
			C->buffer_len /= 2;
			if (C->buffer_len < 1024L)
			{
				C->M->oserr = ENSMEM;
				eprintf (C->M, "%s: %s\n", C->cmd, StrError (ENSMEM));
				return 1;
			}
		}
	}
	
	if ((in = Fopen (realfrom, O_RDONLY)) >= 0)
	{
		int dest_exists = TRUE;
		int file_changed = FALSE;
		
		if (C->flags.confirm || C->flags.non_exist)
			dest_exists = access (realto, A_EXIST, &C->broken);

		if (C->flags.archiv)
		{
			/* do nothing if archive bit is not set */
			src_attrib = Fattrib (realfrom, GET_ATTRIBUTE, 0);
				
			file_changed = (src_attrib & FA_ARCHIVE);
			
			if (!file_changed && dest_exists)
			{
				/* file hasn't changed and target exists */
				Fclose ((int) in);
				return 0;
			}
		}

		if (C->flags.interactive)
		{
			choice = AskCopyMove (C, origfrom, origto);
			switch (choice)
			{
				case 'N':
				case 'Q':
					Fclose ((int) in);
					return (choice == 'N') ? 0 : 3;
				case 'A':
					C->flags.interactive = FALSE;
					break;
			}
		}
		else if (C->flags.confirm && dest_exists)
		{
			/* confirm copy if destination exists
			 */
			choice = ConfirmCopyAction (C, origto);
			switch (choice)
			{
				case 'N':
				case 'Q':
					Fclose ((int) in);
					return (choice == 'N') ? 0 : 3;
				case 'A':
					C->flags.confirm = FALSE;
					break;
			}
		}

		if (C->flags.non_exist && dest_exists && !file_changed)
		{
			Fclose ((int) in);
			return 0;
		}
		
		/* allow overwriting of read-only files */
		if (C->flags.force && access (realto, A_RDONLY, &C->broken))
		{
			SetWriteMode (realto, TRUE);
		}
			
		/* build a temp filename for secure copy */
		if (C->flags.secure)
		{
			old_realto = realto;
			old_origto = origto;
			
			realto = NewExtension (&C->M->alloc, realto, "$$$");
			origto = NewExtension (&C->M->alloc, origto, "$$$");
			if (realto == NULL || origto == NULL)
			{
				eprintf (C->M, "%s: %s\n", C->cmd, StrError (ENSMEM));
				Fclose ((int) in);
				mfree (&C->M->alloc, origto);
				mfree (&C->M->alloc, realto);
				return 1;
			}
		}

		if ((out = Fcreate (realto, 0)) >= 0)
		{
			if (C->flags.verbose)
				ShowCopyMove (C, origfrom, origto);
			
			/* at last. copy the file */	
			retcode = BasicMupfelCopy (C, (int) in, (int) out,
				realto, realfrom);
			
			/* delete the target if copy failed */
			if (retcode != E_OK)
				Fdelete (realto);
			else
				*really_was_copied = TRUE;
				
			if (retcode == 0 && C->flags.secure)
			{
				retcode = EndSecureMupfelCopy (C, origto, old_origto,
					realto, old_realto);
			}
				
			if (retcode == 0)
			{
				if (C->flags.archiv)
				{
					Fattrib (realfrom, SET_ATTRIBUTE, 
						src_attrib & ~FA_ARCHIVE);
				}
				
				/* xxx */
/*				CopyAttribute (realfrom, C->flags.secure ? old_realto : realto); */
			}
		}
		else
		{
			/* couldn't create destination file */
			Fclose ((int) in);
			C->M->oserr = (int) out;
			
			if (C->flags.verbose) crlf (C->M);
				
			eprintf (C->M, NlsStr(CP_CANTCRT), C->cmd, origto,
				StrError (out));
			retcode = 1;
		}
	}
	else
	{
		/* couldn't open source file */
		C->M->oserr = (int) in;
		eprintf (C->M, "%s: `%s' -- %s\n", C->cmd, origfrom,
			StrError (in));
		retcode = 1;
	}

	/* clean up */
	if (C->flags.secure && old_origto)
	{
		mfree (&C->M->alloc, origto);
		mfree (&C->M->alloc, realto);
	}

	if (C->flags.verbose && retcode == 0)
		mprintf (C->M, NlsStr(CP_CPDONE));
		
	if (retcode == 0 && !C->M->shell_flags.system_call)
		DirtyDrives |= DriveBit (realto[0]);

	return retcode;
}
	

int
mv (COPYINFO *C, char *origfrom, char *origto, char *realfrom,
	char *realto, int *really_was_moved)
{
	int retcode = 0;
	long ret;
	long dest_ret;
	XATTR dest_info;
	
	*really_was_moved = FALSE;
	
	dest_ret = GetXattr (realto, 0, 0, &dest_info);
	
	/* If the destination path exists, the -f option is not
	   specified, and either of the following conditions is true:

	   (a) The permissions of the destination path do not permit
	       writing and the standard input is a terminal

	   (b) The -i option is specified
	   
	   the mv utility shall write a prompt to standard error and
	   read a line from standard input. If the response is not
	   affirmative, mv shall do nothing more with the current
	   source_file and go on to the remaining source_files.
	*/
	
	/* xxx: (a) omitted for now */

	if (dest_ret == E_OK && !C->flags.force)
	{
		if (C->flags.interactive)
		{
			switch (ConfirmCopyAction (C, origto))
			{
				case 'N':
					return 0;
				case 'Q':
					return 3;
				case 'A':
					C->flags.confirm = FALSE;
					break;
			}
		}
	}
	
	if (IsBiosError (dest_ret))
	{
		C->broken = (int) dest_ret;
		return (int)ioerror (C->M, C->cmd, origto, C->broken);
	}

	/* The mv utility shall perform actions equivalent to the
	   POSIX.1 rename() function, called with the following
	   arguments:
	   
	   (a) The source_file operand is used as the old argument.
	   
	   (b) The destination path is used as the new argument.
	   
	   If this succeeds, mv shall do nothing more with the
	   current source_file and go on to the remaining source_files.
	   If this fails for any reasons other than those described
	   for the errno EXDEV in POSIX.1, mv shall write a diagnostic
	   message to standard error, do nothing more with the current
	   source_file, and go on to the remaining source_files. */

	if (C->flags.verbose)
		mprintf (C->M, NlsStr(CP_RENAMING), origfrom, origto);
	
	ret = lrename (realfrom, realto);

	if (ret == E_OK)
	{
		if (C->flags.verbose) mprintf (C->M, NlsStr(CP_CPDONE));
		if (!C->M->shell_flags.system_call)
			DirtyDrives |= DriveBit (realfrom[0]) | DriveBit (realto[0]);
		*really_was_moved = 1;
		return 0;
	}
	
	if (IsBiosError (ret))
		return (int)ioerror (C->M, C->cmd, origto, ret);
	
	/* Frename() failed. We abort if the error code is not
	   ENSAME (rename across file systems), and the source
	   file is not a directory (because TOS doesn't do this
	   in all cases) */
	
	if (ret != ENSAME && !IsDirectory (realfrom, &C->broken))
	{
		C->M->oserr = (int) ret;
				
		if (C->flags.verbose)
			crlf (C->M);

		if (ret != ENMFIL)
			eprintf (C->M, NlsStr(CP_RENAME), C->cmd, origfrom,
				origto, StrError (ret));
		else
			eprintf (C->M, NlsStr(CP_DUPFILE), C->cmd, origfrom,
				origto);
		
		return 1;
	}
		
	/* If we arrive here, the file wasn't copied because of
	   cross dev or because TOS doesn't support it. In this
	   case we need to proceed by copying and removing the
	   original */
	   
	if (cp (C, origfrom, origto, realfrom, realto,
		really_was_moved) == E_OK)
	{
		if (*really_was_moved)
		{
			/* it worked, remove the source */
			Fdelete (realfrom);
		}

		if (!C->M->shell_flags.system_call)
			DirtyDrives |= DriveBit (realfrom[0]);
	}
	else
		retcode = 1;

	if (!C->M->shell_flags.system_call)
		DirtyDrives |= DriveBit (realfrom[0]) | DriveBit (realto[0]);

	if (C->flags.verbose && retcode == 0)
		mprintf (C->M, NlsStr(CP_CPDONE));
		
	return retcode;
}


/* copy/move file "src" into "dir" */

static int copyIntoDir (COPYINFO *C, char *src, char *dir)
{
	char src_filename[MAX_FILENAME_LEN];
	char *realfrom;
	char *to, *realto;
	int retcode;

	RemoveAppendingBackslash (src);
	RemoveAppendingBackslash (dir);

	realfrom = NormalizePath (&C->M->alloc, src, C->M->current_drive,
		C->M->current_dir);

	if (realfrom == NULL)
	{
		eprintf (C->M, NlsStr(CP_ILLNAME), C->cmd, src);
		return 1;
	}	

	if (GetBaseName (realfrom, src_filename, MAX_FILENAME_LEN))
	{
		to = mmalloc (&C->M->alloc, strlen (src_filename) 
			+ strlen (dir) + 2);

		if (to == NULL)
		{
			eprintf (C->M, "%s: %s\n", C->cmd, StrError (ENSMEM));
			mfree (&C->M->alloc, realfrom);
			return 1;
		}
	
		/* if dir is a drive name, don't append a backslash so the 
		   target will be in the current dir of that drive */
		strcpy (to, dir);
		
		if (2 != strlen (to) || to[1] != ':')
			AddFileName (to, src_filename);
		else
			strcat (to, src_filename);
	
		realto = NormalizePath (&C->M->alloc, to, C->M->current_drive,
			C->M->current_dir);
	}
	else
		realto = NULL;
	
	if (realto)
	{
		int dummy;
		
		retcode = C->copyfile (C, src, to, realfrom, realto, &dummy);
		mfree (&C->M->alloc, realto);
	}
	else
	{
		eprintf (C->M, NlsStr(CP_ILLNAME), C->cmd, to);
		retcode = 1;
	}

	mfree (&C->M->alloc, realfrom);
	mfree (&C->M->alloc, to);
	return retcode;
}

static long cpDir (COPYINFO *C, char *origfrom, char *origto,
	char *realfrom, char *realto)
{
	WalkInfo W;
	char filename[MAX_FILENAME_LEN];
	long stat = 0;
	char *new_origfrom, *new_origto;
	char *new_realfrom, *new_realto;
	
	/* Test, ob x:\dir in x:\dir\dir2 hineinkopiert werden soll.
	   Das wrde zu einer Endlosschleife fhren. */
	if (strstr (realto, realfrom) == realto)
	{
		size_t len = strlen (realfrom);
		
		if (realto[len] == '\\' || realto[len] == '\0')
		{
			eprintf (C->M, NlsStr(CP_ENDLESS), C->cmd, origfrom, origto);
			return 1;
		}
	}
	
	/* Erst einmal Speicher fr neue Dateinamen holen */
	new_origfrom = mmalloc (&C->M->alloc, strlen (origfrom) 
		+ MAX_FILENAME_LEN + 3);
	new_origto = mmalloc (&C->M->alloc, strlen (origto) 
		+ MAX_FILENAME_LEN + 3);
	new_realfrom = mmalloc (&C->M->alloc, strlen (realfrom) 
		+ MAX_FILENAME_LEN + 3);
	new_realto = mmalloc (&C->M->alloc, strlen (realto) 
		+ MAX_FILENAME_LEN + 3);
	
	if (!new_origfrom || !new_origto || !new_realfrom || !new_realto)
	{
		mfree (&C->M->alloc, new_origfrom);
		mfree (&C->M->alloc, new_origto);
		mfree (&C->M->alloc, new_realfrom);
		mfree (&C->M->alloc, new_realto);

		eprintf (C->M, "%s: %s\n", C->cmd, StrError (ENSMEM));
		return 1;
	}
	
	if (!IsDirectory (realto, &C->broken))
	{
		if (C->broken)
			stat = ioerror (C->M, C->cmd, origto, C->broken);
		else
		{
			if (C->flags.verbose)
				mprintf(C->M, "mkdir %s... ", origto);

			stat = !mkdir (C->M, realto, origto, FALSE);
			
			if (!stat && C->flags.verbose)
				mprintf (C->M, NlsStr(CP_CPDONE));
		}
	}
	
	if (!stat)
	{
		stat = InitDirWalk (&W, WDF_ATTRIBUTES|WDF_LOWER|
			WDF_FOLLOW, realfrom, MAX_FILENAME_LEN, filename);
			
		if (!stat)
		{
			while (!C->broken && !stat && !intr (C->M) 
				&& (DoDirWalk (&W) == 0L))
			{
				if (!strcmp (filename, ".")
					|| !strcmp (filename, "..")
					|| (W.xattr.attr & FA_VOLUME))
				{
					continue;
				}
				
				strcpy (new_origfrom, origfrom);
				strcpy (new_origto, origto);
				strcpy (new_realfrom, realfrom);
				strcpy (new_realto, realto);
				AddFileName (new_realfrom, filename);
				AddFileName (new_realto, filename);
				AddFileName (new_origfrom, filename);
				AddFileName (new_origto, filename);
	
				if (W.xattr.attr & FA_SUBDIR)
				{
					stat = cpDir (C, new_origfrom, new_origto,
						new_realfrom, new_realto);
				}
				else 
				{
					int dummy;
					
					stat = C->copyfile (C, new_origfrom, new_origto, 
						new_realfrom, new_realto, &dummy);
				}	
	
			}
			
			ExitDirWalk (&W);
		}
	}

	mfree (&C->M->alloc, new_origfrom);
	mfree (&C->M->alloc, new_origto);
	mfree (&C->M->alloc, new_realfrom);
	mfree (&C->M->alloc, new_realto);

	return stat;
}

#if 0
static
long copyMoveDir (COPYINFO *C, char *src, char *dest)
{
	char *realfrom, *realto;
	long retcode;

	RemoveAppendingBackslash (src);
	RemoveAppendingBackslash (dest);

	realfrom = NormalizePath (&C->M->alloc, src, 
		C->M->current_drive, C->M->current_dir);
	realto = NormalizePath (&C->M->alloc, dest, 
		C->M->current_drive, C->M->current_dir);

	if (realto == NULL || realfrom == NULL)
	{
		eprintf (C->M, "%s: %s\n", C->cmd, StrError (ENSMEM));
		retcode = 1;
	}
	else if (C->flags.is_move)
	{
		/* it's a directory rename */
		
		retcode = Frename (0, realfrom, realto);
			
		if (retcode != E_OK)
			eprintf (C->M, NlsStr(CP_RENAME), C->cmd, src, dest,
				StrError (retcode));
		else
			DirtyDrives |= DriveBit (realfrom[0]);
	}
	else
	{
		/* silly if not recursive copy */
		if (!C->flags.recursive)
		{
			retcode = C->M->oserr = EACCDN;
			eprintf (C->M, NlsStr(CP_RECUR), C->cmd);
		}
		else
		{
			char *tmp;
			int free_dest = FALSE;
			
			/* Wenn realto ein Verzeichnis ist und realfrom ein
			   Ordner, dann wird so ein Ordner in realto angelegt */
			if (!IsRootDirectory (realfrom) 
				&& IsDirectory (realto, &C->broken))
			{
				char target[MAX_FILENAME_LEN];
				
				GetBaseName (realfrom, target, MAX_FILENAME_LEN);
					
				tmp = mmalloc (&C->M->alloc, 
					strlen (dest) + MAX_FILENAME_LEN + 1);
				if (tmp)
				{
					strcpy (tmp, dest);
					AddFolderName (tmp, target);
				}
				dest = tmp;
				free_dest = TRUE;

				tmp = mmalloc (&C->M->alloc, 
					strlen (realto) + MAX_FILENAME_LEN + 1);
				if (tmp)
				{
					strcpy (tmp, realto);
					AddFolderName (tmp, target);
				}
				mfree (&C->M->alloc, realto);
				realto = tmp;
			}
			
			if (realto && realfrom && dest)
				retcode = (int)cpDir (C, src, dest, realfrom, realto);
			else
			{
				eprintf (C->M, NlsStr(CP_ILLNAME), C->cmd, 
					realfrom? dest : src);
				retcode = 1;
			}
			
			if (free_dest)
				mfree (&C->M->alloc, dest);
		}
	}

	mfree (&C->M->alloc, realfrom);
	mfree (&C->M->alloc, realto);

	return retcode;			
}
#endif

static int
copySingleFile (COPYINFO *C, char *from, char *to)
{
	char *realfrom, *realto;
	int retcode;
	
	realfrom = NormalizePath (&C->M->alloc, from, C->M->current_drive, 
		C->M->current_dir);
	realto = NormalizePath (&C->M->alloc, to, C->M->current_drive, 
		C->M->current_dir);

	if (realfrom && realto)
	{
		int dummy;
		
		retcode = C->copyfile (C, from, to, realfrom, realto, &dummy);
	}
	else
	{
		if (!realfrom)
			eprintf (C->M, NlsStr(CP_ILLNAME), C->cmd, from);
		if (!realto)
			eprintf (C->M, NlsStr(CP_ILLNAME), C->cmd, to);
		retcode = 1;
	}

	if (realfrom) mfree (&C->M->alloc, realfrom);
	if (realto) mfree (&C->M->alloc, realto);
	
	return retcode;
}



int
m_cp (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	COPYINFO C;
	static struct option cp_long_option[] = 
	{
		{ "archive", FALSE, NULL, 'a'},
		{ "force", FALSE, NULL, 'f'},
		{ "interactive", FALSE, NULL, 'i'},
		{ "nonexistent", FALSE, NULL, 'n'},
		{ "preserve", FALSE, NULL, 'p'},
		{ "recursive", FALSE, NULL, 'r'},
		{ "secure", FALSE, NULL, 's'},
		{ "verbose", FALSE, NULL, 'v'},
		{ NULL,0,0,0},
	};
	static struct option mv_long_option[] =
	{
		{ "force", FALSE, NULL, 'f' },
		{ "interactive", FALSE, NULL, 'i'},
		{ "verbose", FALSE, NULL, 'v'},
		{ NULL,0,0,0},
	};
	struct option *long_option;
	static const char *opt_string, *hlp_string;
	int first_is_dir, last_is_dir, args_left, last_arg;
	int opt_index = 0;
	long ret = 0;
	int c;
	
	optinit (&G);

	memset (&C, 0, sizeof (COPYINFO));
	C.M = M;

	if (!strcmp (argv[0], "mv"))
	{
		C.flags.is_move = 1;
		long_option = mv_long_option;
		opt_string = NlsStr(OPT_mv);
		hlp_string = NlsStr(HLP_mv);
		C.copyfile = mv;
	}
	else
	{
		C.flags.is_move = 0;
		long_option = cp_long_option;
		opt_string = NlsStr(OPT_cp);
		hlp_string = NlsStr(HLP_cp);
		C.copyfile = cp;
	}
	
	if (!argc)
		return PrintHelp (M, argv[0], opt_string, hlp_string);
	
	C.cmd = argv[0];
	
	while ((c = getopt_long(M, &G, argc, argv, 
		C.flags.is_move ? "fiv" : "afinpsRrv",
		long_option, &opt_index)) != EOF)
	{
		if (!c)
			c = long_option[opt_index].val;
			
		switch (c)
		{
			case 0:
				break;

			case 'a':
				C.flags.archiv = TRUE;
				break;

			case 'f':
				C.flags.force = TRUE;
				C.flags.interactive = FALSE;
				break;

			case 'i':
				C.flags.interactive = TRUE;
				C.flags.force = FALSE;
				break;

			case 'n':
				C.flags.non_exist = TRUE;
				break;

			case 'p':
				C.flags.preserve = TRUE;
				break;

			case 's':
				C.flags.secure = TRUE;
				break;

			case 'R':
			case 'r':
				C.flags.recursive = TRUE;
				break;

			case 'v':
				C.flags.verbose = TRUE;
				break;

			default:
				return PrintUsage (M, argv[0], opt_string);
		}
	}
	
	/* mv always keeps the date */
	if (C.flags.is_move) C.flags.preserve = TRUE;
		
	/* less than two args left? */
	if (argc - G.optind < 2)
		return PrintUsage (M, argv[0], opt_string);

	/* print a warning if the GEMDOS version does not support
	   archive bits properly */
	if (C.flags.archiv && GEMDOSVersion < 0x15)
		eprintf (M, NlsStr(CP_ARCHWARN), argv[0]);

	args_left = argc - G.optind;
	last_arg = argc - 1;
	
	if (ContainsGEMDOSWildcard (argv[G.optind]))
	{
		eprintf (M, NlsStr(CP_ISWILDCARD), argv[0], argv[G.optind]);
		return EFILNF;
	}
	
	first_is_dir = isDir (&C, argv[G.optind]);
	if (C.broken)
		return (int)ioerror(M, argv[0], argv[G.optind], C.broken);
	
	if (ContainsGEMDOSWildcard (argv[last_arg]))
	{
		if (!C.flags.force)
			eprintf (M, NlsStr(CP_ISWILDCARD), argv[0], last_arg);
		return EFILNF;
	}
	
	last_is_dir = isDir (&C, argv[last_arg]);
	if (C.broken)
		return (int)ioerror(M, argv[0], argv[last_arg], C.broken);
	
	if (args_left == 2)
	{
		/* just two args left */
/*		if (first_is_dir)
			ret = copyMoveDir (&C, argv[G.optind], argv[last_arg]);
		else */
		
		if (last_is_dir)
			ret = copyIntoDir (&C, argv[G.optind], argv[last_arg]);
		else
			ret = copySingleFile (&C, argv[G.optind], argv[last_arg]);
	}		
	else
	{
		/* more than 2 args, so last one should be a dir */

		if (last_is_dir)
		{
			int i;
			
			/* cp/mv files into directory */

			for (i = G.optind; i < last_arg && !intr (M); ++i)
			{
				if (ContainsGEMDOSWildcard (argv[i]))
				{
					eprintf (M, NlsStr(CP_ISWILDCARD), argv[0], argv[i]);
					ret = EFILNF;
					continue;
				}
				
#if 0
				if (isDir (&C, argv[i]))
				{
					ret = copyMoveDir (&C, argv[i], argv[last_arg]);
				}
				else if (C.broken)
				{
					ioerror (M, argv[0], argv[i], C.broken);
					ret = 3;
					break;
				}
				else
#endif
					ret = copyIntoDir (&C, argv[i], argv[last_arg]);
				
				if (ret != E_OK && !C.flags.force) break;
				
				if (C.broken)
				{
					ret = C.broken;
					break;
				}
				else if (intr (M))	/* user abort */
				{
					ret = 2;
					break;
				}
			}
		}
		else
		{
			/* Mehr als zwei Argumente und das letzte ist kein
			   Verzeichnis... */

			eprintf (M, NlsStr(CP_NODIR), argv[0], argv[last_arg]);
			ret = 1;
		}
	}
	
	FreeCopyInfoContents (&C);

	return (int) ret;
}
