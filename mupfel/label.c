/*
 * @(#) Mupfel\label.c
 * @(#) Gereon Steffens & Stefan Eissing, 20. Mai 1993
 *
 *  -  internal "label" function
 *
 * jr 7.7.96
 */
 
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "label.h"
#include "mdef.h"

/* internal texts */

#define NlsLocalSection "M.label"
enum NlsLocalText{
OPT_label,	/*[drive [label]]*/
HLP_label,	/*Label anzeigen/„ndern*/
LB_NOLBL,	/*kein Label*/
LB_EXISTS,	/*%s: `%s' existiert bereits\n*/
};

/* xxx */
#define Dreadlabel(a,b,c)	gemdos(0x152,(char *)a,(char *)b,(int)c)
#define Dwritelabel(a,b)	gemdos(0x153,(char *)a,(char *)b)


/* Label einlesen */

static long
readlabel (MGLOBAL *M, const char *path, char *buffer, int buflen)
{
	char fsfmask[] = "x:\\*.*";
	char *realpath;
	long ret;
	
	realpath = NormalizePath (&M->alloc, path, M->current_drive,
		M->current_dir);
	if (!realpath) return ENSMEM;
	
	fsfmask[0] = realpath[0];
	ret = Dreadlabel (realpath, buffer, buflen);
	mfree (&M->alloc, realpath);

	if (ret == EINVFN)
	{
		DTA dta, *olddta = Fgetdta ();
	
		Fsetdta (&dta);
		ret = Fsfirst (fsfmask, FA_VOLUME);
		Fsetdta (olddta);

		buffer[0] = '\0';
	
		if (ret == E_OK) {
			--buflen;
			strncpy (buffer, dta.d_fname, buflen);
			buffer[buflen] = '\0';
		}
	}
	
	return ret;
}

/* Label schreiben */

static long
writelabel (MGLOBAL *M, const char *cmd, const char *path,
	const char *label)
{
	char tmp_label[MAX_FILENAME_LEN + 3];
	char *realpath;
	long ret;
	
	realpath = NormalizePath (&M->alloc, path, M->current_drive,
		M->current_dir);
	if (!realpath) return ENSMEM;	/* xxx blaaa */

	sprintf (tmp_label, "%c:\\%s", realpath[0], label);
	ret = Dwritelabel (realpath, label);
	mfree (&M->alloc, realpath);
	
	if (ret == EINVFN)
	{
		int broken;

		if (access (tmp_label, A_EXIST, &broken))
		{
			eprintf (M, NlsStr(LB_EXISTS), cmd, tmp_label);
			return EACCDN;
		}

		ret = Fcreate (tmp_label, FA_VOLUME);

		if (ret < 0)
		{
			eprintf (M, "%s: `%s' -- %s\n", cmd, label,
				StrError ((int) ret));
			return (int) ret;
		}

		Fclose ((int) ret);
		
		ret = E_OK;
	}

	if (ret == E_OK && !M->shell_flags.system_call)
		DirtyDrives |= DriveBit (tmp_label[0]);

	return ret;
}


static int
showLabel (MGLOBAL *M, char *path, const char *cmd)
{
	char label[MAX_FILENAME_LEN];
	long stat;
	
	stat = readlabel (M, path, label, (int) sizeof (label));
	
	if (stat) {
		eprintf (M, "%s: `%s' - %s\n", cmd, path, StrError (stat));
		return FALSE;
	}
	
	mprintf (M, "%s\n", *label ? label : NlsStr(LB_NOLBL));
	return TRUE;
}

int
SetLabel (MGLOBAL *M, char *path, char *newlabel, const char *cmd)
{
	return (int) writelabel (M, cmd, path, newlabel);
}	

int
m_label (MGLOBAL *M, int argc, char **argv)
{
	int retcode = 1;
	
	switch (argc)
	{
		case 0:
			return PrintHelp (M, argv[0], NlsStr(OPT_label), 
				NlsStr(HLP_label));
			
		case 1:
			retcode = !showLabel (M, ".", argv[0]);
			break;

		case 2:
			retcode = !showLabel (M, argv[1], argv[0]);
			break;

		case 3:
			retcode = SetLabel (M, argv[1], argv[2], argv[0]);
			break;

		default:
			return PrintUsage (M, argv[0], NlsStr(OPT_label));
	}
	
	return retcode;
}
