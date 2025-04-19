/*
 * @(#) Mupfel/version.c
 * @(#) Gereon Steffens & Julian F. Reschke &
 * @(#) Stefan Eissing, 30. Dezember 1993
 *
 * pun_info now comes from puninfo.h
 *
 * jr 31.12.1996
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tos.h> 
#include <aes.h> 
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\xhdi.h"
#include "mglob.h"

#include "chario.h"
#include "date.h"
#include "getopt.h"
#include "ioutil.h"
#include "mdef.h"
#include "puninfo.h"
#include "sysvars.h"
#include "version.h"

/* internal texts
 */
#define NlsLocalSection "M.version"
enum NlsLocalText{
OPT_version,/*[-amgdth]*/
HLP_version,/*Versionsnummern anzeigen*/
VE_VERSION,	/*Version*/
VE_HARDDISK,/*Festplattentreiber kompatibel zu AHDI-Version %d.%02d\n*/
VE_NOHARD,	/*Kein AHDI-kompatibler Festplattentreiber installiert.\n*/
VE_NOGEM,	/*%s: diese Mupfel benutzt kein GEM\n*/
VE_NOTOS,	/*Keine TOS-Version\n*/
};

#define MUPFELDATE		__DATE__

/* jr: exportiert nach cookie.c */
const char *GetCountryId (int index)
{
	static char *countryid[] =
	{
		"USA", "FRG", "FRA", "UK",  "SPA", "ITA", "SWE", "SWF",
		"SWG", "TUR", "FIN", "NOR", "DEN", "SAU", "HOL", "CZE", 
		"HUN",
	};

	if (index == 127)	/* international */
		return "int";

	if (index > DIM (countryid))
		return "???";

	return countryid[index];
}


int m_version (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	int c;
	int mupfelversion, gemdosversion, tosversion, gemversion,
		driverversion, allversions;
	struct option long_option[] =
	{
		{ "all", FALSE, NULL, 'a' },
		{ "mupfel", FALSE, NULL, 'm' },
		{ "gem", FALSE, NULL, 'g' },
		{ "gemdos", FALSE, NULL, 'd' },
		{ "tos", FALSE, NULL, 't' },
		{ "harddisk", FALSE, NULL, 'h' },
		{ NULL,0,0,0 },
	};
	int opt_index = 0;

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_version), 
			NlsStr(HLP_version));

	mupfelversion = gemdosversion = tosversion = 
		gemversion = driverversion = allversions = FALSE;

	optinit (&G);

	while ((c = getopt_long (M, &G, argc, argv, "amgdth", long_option,
		&opt_index)) != EOF)
	{
		if (!c)
			c = long_option[opt_index].val;

		switch(c)
		{
			case 0:
				break;

			case 'm':
				mupfelversion = TRUE;
				break;

			case 'g':
				gemversion = TRUE;
				break;

			case 't':
				tosversion = TRUE;
				break;

			case 'd':
				gemdosversion = TRUE;
				break;

			case 'h':
				driverversion = TRUE;
				break;

			case 'a':
				mupfelversion = gemdosversion = tosversion =
					gemversion = driverversion = allversions = TRUE;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_version));
		}
	}

	if (G.optind < argc)
		return PrintUsage (M, argv[0], NlsStr(OPT_version));

	if (!mupfelversion && !gemversion && !tosversion &&
		!driverversion)
		mupfelversion = TRUE;

	if (mupfelversion)
	{
		mprintf (M, "Mupfel %s %s [%s]\n",
			NlsStr(VE_VERSION), MUPFELVERSION, MUPFELDATE);
	}

	if (gemversion)
	{
		if (M->shell_flags.dont_use_gem)
		{
			if (!allversions)
				eprintf (M, NlsStr(VE_NOGEM), argv[0]);
		}
		else
		{
			int gemverlo, gemverhi;

			gemverhi = _GemParBlk.global[0] >> 8;
			gemverlo = _GemParBlk.global[0] & 0xff;
			mprintf (M, "GEM %s %x.%02x\n",
				NlsStr(VE_VERSION), gemverhi, gemverlo);
		}
	}

	if (tosversion)
	{
		if (TOSVersion)
		{
			dosdate d;
			
			d.d = TOSDate;
		
			mprintf (M, "TOS %s %d.%02x [%02d.%02d.%02d] (%s)\n",
				NlsStr(VE_VERSION), 
				TOSVersion >> 8, TOSVersion & 0xff,
				d.s.day, d.s.month, d.s.year+80,
				GetCountryId (TOSCountry));
		}
		else
		{
			mprintf (M, NlsStr(VE_NOTOS));
		}
		
		if (MagiCVersion)
			mprintf (M, "MagiC %s %x.%02x\n", NlsStr(VE_VERSION),
				MagiCVersion >> 8, MagiCVersion & 0xff);
	}

	if (gemdosversion)
	{
		mprintf (M, "GEMDOS %s %02x.%02x\n", NlsStr(VE_VERSION),
			GEMDOSVersion >> 8, GEMDOSVersion & 0xff);
	}

	if (driverversion)
	{
		PUN_INFO *pi = PunThere ();

		if (pi)
			mprintf (M, NlsStr(VE_HARDDISK), pi->P_version / 256,
				pi->P_version & 255);
		else
			mprintf (M, NlsStr(VE_NOHARD));

		if (XHGetVersion () >= 0x100)
		{
			int i;
			char name[18], version[8], vendor[18];

			name[0] = version[0] = vendor[0] = '\0';

			for (i = 0; i < 32; i++)
			{
				int print_cr = 0;
				char lname[18], lversion[18], lvendor[18];

				if (0 == XHInqDriver (i, lname, lversion,
					lvendor, NULL, NULL))
				{
					if (!strcmp (lname, name) && !strcmp (lversion,
						version) && !strcmp (lvendor, vendor))
					{
						mprintf (M, "%c", i + 'A');
					}
					else
					{
						if (print_cr) mprintf (M, "\n");

						mprintf (M, "%s %s (%s): %c", lname, lversion,
							lvendor, i + 'A');
						strcpy (name, lname);
						strcpy (version, lversion);
						strcpy (vendor, lvendor);
					}

					print_cr = 1;
				}
			}
			mprintf (M, "\n");
		}
	}

	return 0;
}
