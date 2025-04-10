/*
 * @(#) Mupfel\format.c
 * @(#) Gereon Steffens & Stefan Eissing, 14. M„rz 1994
 *
 *  -  internal "format" command
 *
 * jr 23.9.95
 */

#include <ctype.h>
#include <stdio.h>
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
#include "format.h"
#include "getopt.h"
#include "init.h"
#include "mupfel.h"
#include "stand.h"
#include "vt52.h"

/* internal texts
 */
#define NlsLocalSection "M.format"
enum NlsLocalText{
OPT_format,	/*[-vy][-s seiten][-c sektoren][-l label] drv:*/
HLP_format,	/*Diskette formatieren*/
FM_NOMEM,	/*%s: nicht genug Speicher vorhanden\n*/
FM_INFO,	/*Formatiere Spur %02d Seite %d*/
FM_FMTERR,	/*%s: Fehler beim Formatieren von Spur %d (%s)\n*/
FM_ABORT,	/*%s: abgebrochen\n*/
FM_ILLSECS,	/*%s: nur 9 bis 20 Sektoren erlaubt\n*/
FM_ILLNAME,	/*%s: kann Laufwerk `%s' nicht formatieren\n*/
FM_ILLDRV,	/*%s: `%s' ist kein Laufwerksbezeichner\n*/
FM_CONFIRM,	/*Diskette in Laufwerk %c: wirklich formatieren (j/n)? */
FM_CHOICE,	/*JN*/
FM_CANTLOCK,/*%s: Laufwerk %c: ist in Gebrauch\n*/
};

static int *spiral (int max_side, int track, int side, int spt,
	int *sector_field)
{
	unsigned int start_sector, spiral_factor;
	int i;

	spiral_factor = spt - 3;

	start_sector = (1 + (((track + 1) * max_side - 1 + side) - 1)
		* spiral_factor) % spt;

	if (start_sector == 0)
		start_sector = 1;

	for (i = 0; i < spt; ++i)
		sector_field[i] = ((start_sector + i - 1) % spt) + 1;

	return sector_field;
}

int FormatDrive (MGLOBAL *M, const char *command, int drv,
	int sides, int sectors, int verbose, char *label, 
	void (*progress) (int))
{
	char *buf;
	int trk, side, err;
	int retcode = FALSE;
	int use_filler;
	int *filler;
	int intleave, drive_locked = FALSE;
	int sector_field[MAX_SECTORS];
	char path[4];
	short bios_dev;
	
	sprintf (path, "%c:\\", drv);
	if (BiosDeviceOfPath (path, &bios_dev))
		return EDRIVE;

	buf = mmalloc (&M->alloc, MAX_SECTORS * 2 * SECTOR_SIZE);
	if (buf == NULL)
	{
		eprintf (M, NlsStr(FM_NOMEM), command);
		return ENSMEM;
	}

	{
		long code;
		
		code = Dlock (1, bios_dev);
		if (code != 0L && code != EINVFN)
		{
			mfree (&M->alloc, buf);
			eprintf (M, NlsStr(FM_CANTLOCK), command, drv);
			return (int)code;
		}
		else
			drive_locked = TRUE;
	}

	DirtyDrives |= DriveBit (drv);

	use_filler = (TOSVersion >= 0x102);
	
	err = 0;
	for (trk = 79; (!err) && (trk >= 0); --trk)
	{
		for (side = sides - 1; (!err) && (side >= 0); --side)
		{
			if (progress != NULL)
			{
				progress (1000 - (int)(((trk * 2 + side)
					 * 1000L) / 160L));
			}

			if (verbose)
			{
				cursor_off (M);
				canout (M, '\r');
				mprintf (M, NlsStr(FM_INFO), trk, side);
				cursor_on (M);
			}
			
			if (use_filler)
			{
				filler = spiral (sides, trk, side, sectors,
					sector_field);
				intleave = -1;
			}
			else
			{
				filler = NULL;
				intleave = 1;
			}

			err = Flopfmt (buf, filler, bios_dev, sectors, trk, side,
					intleave, 0x87654321L, 0xe5e5);

			if (err)
			{
				if (verbose) crlf (M);
	
				eprintf (M, NlsStr(FM_FMTERR), command, trk,
					StrError (err));
			}
			else if (checkintr (M))
			{
				if (verbose) crlf (M);

				eprintf (M, NlsStr(FM_ABORT), command);
				err = ERROR;
			}
		}
	}

	if (!err)
	{
		if (verbose)
			crlf (M);
	
		/* write empty root dir, FAT and boot sector
		 */
		retcode = InitDrive (M, command, drv, bios_dev, sides, 
			sectors, 80, label, buf, TRUE);
	}
	
	mfree (&M->alloc, buf);
	if (drive_locked)
		Dlock (0, bios_dev);

	return retcode;
}

int m_format (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	int c;
	int drv, sides = '2';
	int sectors = 9;
	int sureformat, verbose;
	char answer;
	char *label = NULL;
	struct option long_option[] =
	{
		{ "verbose", FALSE, NULL, 'v' },
		{ "yes", FALSE, NULL, 'y' },
		{ "sides", TRUE, NULL, 's' },
		{ "sectors", TRUE, NULL, 'c' },
		{ "label", TRUE, NULL, 'l' },
		{ NULL,0,0,0 },
	};
	int opt_index = 0;
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_format), 
			NlsStr(HLP_format));
		
	verbose = sureformat = FALSE;
	optinit (&G);

	while ((c = getopt_long (M, &G, argc, argv, "vys:c:l:", 
		long_option, &opt_index)) != EOF)
	{
		if (!c)
			c = long_option[opt_index].val;
			
		switch (c)
		{
			case 0:
				break;

			case 'v':
				verbose = TRUE;
				break;

			case 's':
				sides = *G.optarg;
				break;

			case 'y':
				sureformat = TRUE;
				break;

			case 'c':
				sectors = atoi (G.optarg);
				break;

			case 'l':
				label = G.optarg;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_format));
		}
	}

	if (sides != '1' && sides != '2' || argc == G.optind)
		return PrintUsage (M, argv[0], NlsStr(OPT_format));
		
	sides -= '0';
	
	if (sectors < 8 || sectors > MAX_SECTORS)
	{
		eprintf (M, NlsStr(FM_ILLSECS), argv[0]);
		return EACCDN;
	}
	
	if (strlen (argv[G.optind]) != 2 || argv[G.optind][1] != ':')
	{
		eprintf (M, NlsStr(FM_ILLNAME), argv[0], argv[G.optind]);
		return EACCDN;
	}
	
	drv = toupper (*argv[G.optind]);
	if (drv < 'A' || drv > 'B' || !LegalDrive (drv))
	{
		eprintf (M, NlsStr(FM_ILLDRV), argv[0], argv[G.optind]);
		return EACCDN;
	}

	if (!M->shell_flags.system_call && !sureformat)
	{
		eprintf (M, NlsStr(FM_CONFIRM), drv);
		answer = CharSelect (M, NlsStr(FM_CHOICE));
		if (answer == 'J')
			answer = 'Y';
	}
	else
		answer = 'Y';
		
	if (answer == 'Y')
	{
		return FormatDrive (M, argv[0], drv, sides, sectors, verbose,
			label, (void (*)(int))NULL);
	}
	
	return 0;
}

