/*
 *	@(#)Mupfel/cpmvutil.c
 *	@(#)Stefan Eissing & Gereon Steffens, 30. Dezember 1992
 *
 * jr 31.12.96
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <tos.h>
#include <nls\nls.h>
#include <mint\ioctl.h>
#include <mint\mintbind.h>

#include "..\common\alloc.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "cpmv.h"
#include "cpmvutil.h"
#include "mdef.h"


/* internal texts
 */
#define NlsLocalSection "M.cpmvutil"
enum NlsLocalText{
CP_CONFCP,	/*%s: %s existiert. šberschreiben (j/n/a/q)? */
CP_CHOICE,	/*JNAQ*/
CP_ASKCP,	/*%s: %s nach %s kopieren (j/n/a/q)? */
CP_ASKMV,	/*%s: %s nach %s verschieben (j/n/a/q)? */
CP_SHOWCP,	/*%s %s nach %s... */
CP_COPYING,	/*kopiere*/
CP_MOVING,	/*verschiebe*/
CP_OVERWRT,	/*%s: kann %s nicht berschreiben\n*/
CP_ENDSEC,	/* fertig.\n%s wird in %s umbenannt... */
CP_FULLDISK,/*%s: Ziel-Laufwerk ist voll\n*/
CP_WRITEERR,/*%s: Schreibfehler in `%s'\n*/
CP_NOTIMES,	/*%s: Dateidatum fr `%s' nicht kopiert\n*/
};

/* Frage nach, ob wirklich berschrieben werden soll.
 */
char ConfirmCopyAction (COPYINFO *C, char *file)
{
	char answer;
	
	if (C->M->shell_flags.system_call)
		return 'Y';
		
	mprintf(C->M, NlsStr(CP_CONFCP), C->cmd, file);	
	answer = CharSelect (C->M, NlsStr(CP_CHOICE));

	if (answer=='J')
		answer = 'Y';
		
	return answer;
}

/* Frage nach, ob kopiert/verschoben werden soll.
 */
char AskCopyMove (COPYINFO *C, char *from, char *to)
{
	char answer;
	
/* Dies war eine schlechte Idee, wenn cp -i ber find abgesetzt
 * wurde.
 */
#if NEVER
 	if (C->M->shell_flags.system_call)
		return 'Y';
#endif
	
	if (C->flags.is_move)
		mprintf (C->M, NlsStr(CP_ASKMV), C->cmd, from, to);
	else
		mprintf (C->M, NlsStr(CP_ASKCP), C->cmd, from, to);

	answer = CharSelect (C->M, NlsStr(CP_CHOICE));
	
	if (answer=='J')
		answer = 'Y';

	return answer;
}

/* Zeige an, was wir tun.
 */
void ShowCopyMove (COPYINFO *C, char *from, char *to)
{
	mprintf (C->M, NlsStr(CP_SHOWCP), C->flags.is_move ? 
		NlsStr(CP_MOVING) : NlsStr(CP_COPYING), from, to);
}


#define GET_DATE	0
#define SET_DATE	1

/* Kopiere Datum und Zeit von <from> nach <to> */
int CopyDate (const char *from, const char *to)
{
	int from_handle, to_handle;
	DOSTIME dtime;
	
	from_handle = (int)Fopen (from, 0);
	if (from_handle < 0) return FALSE;
		
	to_handle = (int)Fopen (to, 2);
	if (to_handle < 0)
	{
		Fclose (from_handle);
		return FALSE;
	}
	
	Fdatime (&dtime, from_handle, GET_DATE);
	Fdatime (&dtime, to_handle, SET_DATE);

	Fclose (from_handle);
	Fclose (to_handle);
	return TRUE;
}


/* Kopiere die Attribute der Datei <src> nach <dest> */
void CopyAttribute (const char *src, const char *dest)
{
	long attrib;
	
	attrib = Fattrib (src, GET_ATTRIBUTE, 0);
	
	if (attrib > 0)
		Fattrib (dest, SET_ATTRIBUTE, FA_ARCHIVE|(int) attrib);
}


/*
 * after a secure copy (cp -s), rename the temp file to the
 * original destination
 */
int EndSecureMupfelCopy (COPYINFO *C, char *origto, char *old_origto,
	char *realto, char *old_realto)
{
	int retcode = 0;
	int err;
	
	if (C->flags.verbose)
		mprintf (C->M, NlsStr(CP_ENDSEC), origto, old_origto);

	if ((err = Fdelete (old_realto)) == 0 || err == EFILNF)
		Frename (0, realto, old_realto);
	else
	{
		if (C->flags.verbose)
			crlf (C->M);
		mprintf(C->M, NlsStr(CP_OVERWRT), C->cmd, old_origto);
		Fdelete (realto);
		retcode = 1;
	}
	
	return retcode;
}

/* xxx: to be done: CheckDisk auf U:\ etc */
int CheckDiskFull (char drive)
{
	DISKINFO di;
	
	if (!Dfree (&di, toupper (drive) - 'A' + 1))
	{
		return (di.b_free == 0L);
	}
	
	return FALSE;
}

/* do the basic work of copying the file */

static int
do_copy (COPYINFO *C, int inhnd, int outhnd, char *to, char *from)
{
	int retcode = 0;
	long rd, wr;

	while ((rd = Fread (inhnd, C->buffer_len, C->buffer)) > 0)
	{
		wr = Fwrite (outhnd, rd, C->buffer);

		if (wr != rd)
		{
			C->M->oserr = EWRITF;

			if (C->flags.verbose) crlf (C->M);
			
			if (wr < 0)
				eprintf (C->M, "%s: `%s' -- %s\n", C->cmd, to, StrError ((int) wr));
			else if (CheckDiskFull (*to))
				eprintf(C->M, NlsStr(CP_FULLDISK), C->cmd);
			else
				eprintf(C->M, NlsStr(CP_WRITEERR), C->cmd, to);
			retcode = 1;
			break;
		}
	}
	
	if (rd < 0)
	{
		C->M->oserr = EREADF;

		if (C->flags.verbose) crlf (C->M);
			
		eprintf (C->M, "%s: `%s' -- %s\n", C->cmd, from, StrError ((int) rd));
		retcode = 1;
	}

	return retcode;
}


/* xxx fattrib */

int
BasicMupfelCopy (COPYINFO *C, int inhnd, int outhnd, char *to, 
	char *from)
{
	int retcode;
	DOSTIME mod, ac;
	unsigned int uid, gid, mode;
	unsigned keep_mtime = FALSE, keep_atime = FALSE;
	unsigned keep_owner = FALSE, keep_mode = FALSE;
	MacFinderInfo M;
	unsigned keep_macinfo = FALSE;
	XATTR X;
	long attr = EINVFN;
	
	if (E_OK == Fcntl (inhnd, (long) &X, FSTAT))
	{
		keep_mode = TRUE;
		mode = X.mode & 07777;
		attr = X.attr;

		if (C->flags.preserve)
		{
			keep_owner = keep_mtime = keep_atime = TRUE;
			uid = X.uid; gid = X.gid;
			mod.time = X.mtime; mod.date = X.mdate;
			ac.time = X.atime; ac.date = X.adate;
		}
	}
	else if (C->flags.preserve)
	{
		Fdatime (&mod, inhnd, 0);
		keep_mtime = TRUE;
	}

	/* try to get Mac HFS information */
	
	keep_macinfo = (E_OK == Fcntl (inhnd, (long) &M, FMACGETTYCR));
	
	/* Get file attributes if FSTAT didn't retrieve them for us */
	
	if (attr < 0) attr = Fattrib (from, 0, 0);
	
	/* copy data fork */

	retcode = do_copy (C, inhnd, outhnd, to, from);

	/* try resource fork... */
	
	if (retcode == 0 && E_OK == Fcntl (inhnd, 0, FMACOPENRES)
		&& E_OK == Fcntl (outhnd, 0, FMACOPENRES))
	{
		retcode = do_copy (C, inhnd, outhnd, to, from);
	}

	Fclose (inhnd);

	/* restore file information? */
	if (keep_atime)
	{
		struct mutimbuf m;
		
		m.actime = ac.time; m.acdate = ac.date;
		m.modtime = mod.time; m.moddate = mod.date;

		if (E_OK == Fcntl (outhnd, (long) &m, FUTIME))
			keep_atime = keep_mtime = FALSE;
	}
	
	if (keep_mtime)
		if (E_OK == Fdatime (&mod, outhnd, 1))
			keep_mtime = FALSE;

	if (keep_atime || keep_mtime)
		eprintf (C->M, NlsStr(CP_NOTIMES), C->cmd, to);

	if (keep_macinfo)
	{
		M.fdFlags &= 0xfeff;	/* reset hasInited flag */
		
		/* dies k”nnte uU wegfallen, wenn ganze Verzeichnisse
		kopiert werden */		
		M.fdLocation1 = M.fdLocation2 = -1;	/* clear icon pos */

		Fcntl (outhnd, (long) &M, FMACSETTYCR);
	}

	Fclose (outhnd);

	if (keep_owner)
	{
		/* clear S_ISUID and S_ISGID if Fchown fails */
	
		if (E_OK != Fchown (to, uid, gid))
			mode &= ~(S_ISUID|S_ISGID);
	}
	
	if (attr >= 0)
		Fattrib (to, 1, (int) attr);

	if (keep_mode)
		Fchmod (to, mode);	/* xxx report errors */
		
	return retcode;
}
