/*
 * @(#) Mupfel\times.c
 * @(#) Stefan Eissing & Gereon Steffens, 08. Februar 1994
 *
 * - Abgelaufene Zeit in der Shell ermitteln
 */
 
#include <stddef.h>
#include <string.h>
#include <tos.h>
#include <mint\mintbind.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "mglob.h"

#include "chario.h"
#include "ioutil.h"
#include "mdef.h"
#include "sysvars.h"
#include "times.h"

/* internal texts
 */
#define NlsLocalSection "M.times"
enum NlsLocalText{
HLP_times,	/*abgelaufene Zeit anzeigen*/
TI_TIME,	/*Abgelaufene Zeit: %02ld:%02ld:%02ld.%ld%ld\n*/
TI_USER,	/*       User-Zeit: %02ld:%02ld:%02ld.%ld%ld\n*/
TI_SYSTEM,	/*     System-Zeit: %02ld:%02ld:%02ld.%ld%ld\n*/
};

#define TIMERTICK	200L
#define SECSPERMINUTE	60
#define SECSPERHOUR		3600	/* adjust these to your planet */

void InitTimes (MGLOBAL *M)
{
	M->times = GetHz200Timer ();
}

static void printTime (MGLOBAL *M, const char *msgstr, 
	unsigned long timer_val, unsigned long timer_tick)
{
	unsigned long hours, minutes, seconds, tenths, hundreds;
	ldiv_t t;
	
	t = ldiv (timer_val, timer_tick);
	seconds = t.quot;

	tenths = t.rem / (timer_tick / 10);
	hundreds = (t.rem - (tenths * 20)) / (timer_tick / 100);
	
	hours = seconds / SECSPERHOUR;
	minutes = (seconds - (hours * SECSPERHOUR)) / SECSPERMINUTE;
	seconds -= (minutes * SECSPERMINUTE) + (hours * SECSPERHOUR);
	
	mprintf (M, msgstr, hours, minutes, seconds, tenths, hundreds);
}

int m_times (MGLOBAL *M, int argc, char **argv)
{
	unsigned long val[8];

	if (!argc)
		return PrintHelp (M, argv[0], "", NlsStr(HLP_times));
		
	if (argc != 1)
		return PrintUsage (M, argv[0], "");
	
	printTime (M, NlsStr(TI_TIME), GetHz200Timer () - M->times, TIMERTICK);

	memset (val, 0, sizeof (val));
	Prusage ((long *)val);
	if (val[0])
	{
		printTime (M, NlsStr(TI_USER), val[0], 1000L);
		printTime (M, NlsStr(TI_SYSTEM), val[1], 1000L);
	}

	return 0;
}