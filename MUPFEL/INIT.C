/*
 * @(#) Mupfel\init.c
 * @(#) Gereon Steffens & Stefan Eissing, 14. M„rz 1994
 *
 *  -  internal "init" command
 *
 * jr 22.10.95
 */
 
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <mint\mintbind.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"
#include "mglob.h"

#include "chario.h"
#include "getopt.h"
#include "init.h"
#include "label.h"
#include "mdef.h"
#include "mupfel.h"

/* internal texts
 */
#define NlsLocalSection "M.init"
enum NlsLocalText{
OPT_init,	/*[-y][-s seiten][-c sektoren][-l label] drv:*/
HLP_init,	/*L”schen einer Diskette*/
IN_WRTBOOT,	/*%s: kann Boot-Sektor auf Laufwerk %c: nicht schreiben\n*/
IN_ILLSECS,	/*%s: Sektorzahl nicht erlaubt\n*/
IN_ILLNAME,	/*%s: ungltiger Laufwerksname `%s'\n*/
IN_ILLDRV,	/*%s: %s ist kein Laufwerksbezeichner\n*/
IN_CONFIRM,	/*Diskette in Laufwerk %c: wirklich initialisieren (j/n)? */
IN_CHOICE,	/*JN*/
IN_READERR,	/*%s: kann Boot-Sektor von Laufwerk %c: nicht lesen (%s)\n*/
IN_NONTOS,	/*%s: Unbekannter Diskettentyp, bitte Sektoren und Seiten angeben.\n*/
IN_CANTLOCK,	/*%s: Laufwerk `%c:' ist in Gebrauch (%s)\n"*/
IN_LOCKEDBY,	/*%s: Laufwerk `%c:' wird von `%s' (Prozess %d) benutzt\n"*/
};

/* offsets in bootsector 
 */
#define	NSECTS	0x13		/* sectors/disk */
#define	SPF		0x16		/* sectors/fat */
#define	SPT		0x18		/* sectors/track */
#define	NSIDES	0x1A		/* sides/disk */
#define	CHKSUM	0x1FF	/* low byte of checksum */


static int clearTrack (int trk, char *trkbuf, int drv, int sides, 
	int sectors)
{
	int retcode;
	
	retcode = Flopwr (trkbuf, 0L, drv, 1, trk, 0, sectors);

	if (retcode == 0 && sides == 2)
		retcode = Flopwr (trkbuf, 0L, drv, 1, trk, 1, sectors);

	return retcode;
}

/*
 * store an int in 80x86 format into buf at offset
 */
static void storeint (char *buf, int offset, int value)
{
	buf[offset] = value & 0xFF;
	buf[offset+1] = value >> 8;
}

static int getint (unsigned char *buf, int offset)
{
	return buf[offset] | (buf[offset+1]<<8);
}

static int writeBootSector (char *trkbuf, int device, int sides, 
	int sectors, int tracks)
{
	int nsects = sides * sectors * tracks;	/* sectors on disk */
	int i, spf;
	unsigned int chk = 0;
	unsigned int *bufp = (unsigned int *)trkbuf;
	int retcode;
	
	Protobt (trkbuf, 0x1000000L, sides+1, 0);

	trkbuf[0] = 0xe9;
	trkbuf[1] = 0x00;
	trkbuf[2] = 0x4e;
	trkbuf[0x15] = 0xF7 + sides;

	storeint (trkbuf, NSECTS, nsects);	/* sectors/disk */
	storeint (trkbuf, SPT, sectors);		/* sectors/track */
	storeint (trkbuf, NSIDES, sides);	/* sides/disk */

	spf = getint ((unsigned char *)trkbuf, SPF);

	/* patch last 2 bytes of boot sector so the Macintosh(TM) will
	 * recognize it.
	 */
	trkbuf[510] = 0x55;
	trkbuf[511] = 0xAA;
	
	/* make sure boot sector isn't executable
	 */
	for (i = 0; i < 256; ++i)
	{
		chk += *bufp++;
		++i;
	}
	if (chk == 0x1234)
		--trkbuf[CHKSUM];
		
	retcode = (int)Rwabs (3, trkbuf, 1, 0, device);
	
	if (retcode == 0)
	{
		/* write MS-DOS compatible media descr. to FAT
		 */
		memset(trkbuf,0,512);
		trkbuf[0] = 0xF9;
		trkbuf[1] = trkbuf[2] = 0xFF;
		
		/* 1st FAT at sector 1
		 */
		retcode = (int)Rwabs (3, trkbuf, 1, 1, device);
		
		/* 2nd FAT at sector spf+1
		 */
		if (retcode == 0)
			retcode = (int)Rwabs (3, trkbuf, 1, spf + 1, device);
	}

	return retcode;
}

static long readBootSector (char drv, int *sides, int *sectors, 
	int *nsects, int *tracks)
{
	long ret;
	unsigned char buf[512];
	
	ret = Floprd (buf, 0L, drv - 'A', 1, 0, 0, 1);
	
	if (ret == E_OK)
	{
		*sectors = getint (buf,SPT);	/* sectors/track */
		*sides = getint (buf, NSIDES);	/* sides/disk */
		*nsects = getint (buf, NSECTS);	/* sectors/disk */
		*tracks = *nsects / (*sides * *sectors);
	}
	
	return ret;
}

static int checkValues (int sides, int sectors, int nsects, 
	int tracks)
{
	if (sides < 1 || sides > 2)
		return FALSE;
		
	if (sectors < 8 || sectors > MAX_SECTORS)
		return FALSE;
		
	if (tracks < 40 || tracks > 86)
		return FALSE;
		
	if (nsects < 300 || nsects > 3440)
		return FALSE;
		
	return TRUE;
}

int InitDrive (MGLOBAL *M, const char *command, char drv, 
	short bios_dev, int sides, int sectors, int tracks, char *label, 
	char *trkbuf, int is_locked)
{
	int buf_malloced = FALSE;
	int retcode;
	int we_locked_it = FALSE;
	char drive_path[4];
		
	sprintf (drive_path, "%c:\\", drv);
	
	if (trkbuf == NULL)
	{
		trkbuf = mmalloc (&M->alloc, MAX_SECTORS * 2 * SECTOR_SIZE);
		if (trkbuf == NULL)
		{
			eprintf (M, "%s: %s", command, StrError (ENSMEM));
			return ENSMEM;
		}
		buf_malloced = TRUE;
	}

	if (!is_locked)
	{
		long code;
		
		code = Dlock (1|2, bios_dev);
		
		if (code != EINVFN && code != E_OK)
		{
			char tmp[MAX_PATH_LEN];
		
			if (buf_malloced) mfree (&M->alloc, trkbuf);

			if (code < E_OK)
				eprintf (M, NlsStr(IN_CANTLOCK), command, drv);
			else
				eprintf (M, NlsStr(IN_LOCKEDBY), command, drv,
					ProcessName ((int) code, tmp), (int) code);

			return (int)code;
		}
		
		we_locked_it = (code == 0);
	}
	
	DirtyDrives |= DriveBit (drv);

	memset (trkbuf, 0, MAX_SECTORS * 2 * SECTOR_SIZE);

	retcode = clearTrack (0, trkbuf, bios_dev, sides, sectors);
	
	if (retcode == 0 && (retcode = clearTrack (1, trkbuf, 
		drv - 'A', sides, sectors)) == 0)
	{
		retcode = writeBootSector (trkbuf, bios_dev, sides, sectors, 
			tracks);
		if (retcode != 0)
			eprintf (M, NlsStr(IN_WRTBOOT), command, drv);		
	}
	else
		eprintf (M, "%s: `%c:' -- %s\n", command, drv, StrError (retcode));

	if (buf_malloced)
		mfree (&M->alloc, trkbuf);
	
	if (we_locked_it)
		Dlock (0, bios_dev);
	else
		ForceMediaChange (drive_path);

	if (retcode == 0)
	{
		if (label && *label && strcmp (label, "-"))
			SetLabel (M, drive_path, label, command);
	
		if (!M->shell_flags.system_call)
			DirtyDrives |= DriveBit (drv);
	}

	return retcode;
}
	
int m_init (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	int c;
	int sureinit;
	int keepsides, keepsectors, keeplabel;
	char answer, drv;
	int newsides = 2;
	int newsectors = 9;
	int newtracks = 80;
	char newlabel[MAX_FILENAME_LEN];
	struct option long_option[] =
	{
		{ "yes", FALSE, NULL, 'y' },
		{ "sides", TRUE, NULL, 's' },
		{ "sectors", TRUE, NULL, 'c' },
		{ "label", TRUE, NULL, 'l' },
		{ NULL, 0,0,0},
	};
	int opt_index = 0;

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_init), 
			NlsStr(HLP_init));
		
	sureinit = FALSE;
	keepsides = keepsectors = keeplabel = TRUE;
		
	if (argc < 2)
		return PrintUsage (M, argv[0], NlsStr(OPT_init));
		
	optinit (&G);
	
	while ((c = getopt_long (M, &G, argc, argv, "ys:c:l:", 
		long_option, &opt_index)) != EOF)
	{
		if (!c)
			c = long_option[opt_index].val;
			
		switch (c)
		{
			case 0:
				break;

			case 'y':
				sureinit = TRUE;
				break;

			case 's':
				newsides = atoi (G.optarg);
				keepsides = FALSE;
				break;

			case 'c':
				newsectors = atoi (G.optarg);
				keepsectors = FALSE;
				break;

			case 'l':
				strncpy (newlabel, G.optarg, MAX_FILENAME_LEN - 1);
				newlabel[MAX_FILENAME_LEN - 1] = '\0';
				keeplabel = FALSE;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_init));
		}
	}

	if ((newsides != 1 && newsides != 2) || argc == G.optind)
		return PrintUsage (M, argv[0], NlsStr(OPT_init));
	
	if (newsectors < 8 || newsectors > MAX_SECTORS)
	{
		eprintf (M, NlsStr(IN_ILLSECS), argv[0]);
		return ERROR;
	}
	
	if (strlen (argv[G.optind]) != 2 || argv[G.optind][1] != ':')
	{
		eprintf (M, NlsStr(IN_ILLNAME), argv[0], argv[G.optind]);
		return EACCDN;
	}
	
	drv = toupper (*argv[G.optind]);
	if (drv < 'A' || drv > 'B' || !LegalDrive (drv))
	{
		eprintf (M, NlsStr(IN_ILLDRV), argv[0], argv[G.optind]);
		return EACCDN;
	}

	if (!M->shell_flags.system_call && !sureinit)
	{
		mprintf (M, NlsStr(IN_CONFIRM), drv);
		answer = CharSelect (M, NlsStr(IN_CHOICE));
		if (answer == 'J')
			answer = 'Y';
	}
	else
		answer = 'Y';
		
	if (answer != 'Y')
		return 0;
		
	if (keepsides || keepsectors)
	{
		long ret;
		int nsects, sides, sectors, tracks;
		
		ret = readBootSector (drv, &sides, &sectors, &nsects, &tracks);
		if (ret != E_OK)
		{
			eprintf (M, NlsStr(IN_READERR), argv[0], drv, StrError (ret));
			return (int) ret;
		}
		
		if (!checkValues (sides, sectors, nsects, tracks))
		{
			eprintf (M, NlsStr(IN_NONTOS), argv[0]);
			return ERROR;
		}
		
		if (keepsides)
			newsides = sides;
			
		if (keepsectors)
			newsectors = sectors;
			
		newtracks = tracks;
	}
	
	if (keeplabel) ReadLabel (drv, newlabel, MAX_FILENAME_LEN);
		
	{
		char path[4];
		short bios_dev;
		long code;
	
		sprintf (path, "%c:\\", drv);
		if ((code = BiosDeviceOfPath (path, &bios_dev)) != 0)
		{
			eprintf (M, "%s: `%s' -- %s\n", argv[0], path,
				StrError (code));
			return (int)code;
		}

		return InitDrive (M, argv[0], drv, bios_dev, newsides, 
			newsectors, newtracks, newlabel, NULL, FALSE);
	}
}
