/*
 * @(#) Mupfel\date.c
 * @(#) Gereon Steffens & Stefan Eissing, 14. Juni 1993
 *
 * - internal "date" and "touch" commands
 *
 * 21.01.91 loctime.c eingebaut, date +format (jr)
 * stdate killed
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "date.h"
#include "getopt.h"
#include "loctime.h"
#include "mdef.h"

/* internal texts
 */
#define NlsLocalSection "M.date"
enum NlsLocalText{
DT_BADSPEC,	/*Ungltige Datumsangabe\n*/
DT_NEWDATE,	/*Neues Datum: %s*/
DT_BADCONV,	/*Ungltige Datumsangabe*/
DT_ISDIR,	/*%s ist ein Verzeichnis*/
DT_CANTCRT,	/*kann %s nicht anlegen*/
DT_NONEXIST,/*%s existiert nicht*/
DT_CANTSET,	/*kann Datum fr %s nicht setzen*/
DT_ILLNAME,	/*%s ist kein gltiger Name*/
OPT_date,	/*[hhmm|mmddhhmm[yy]]*/
HLP_date,	/*Systemdatum anzeigen/„ndern*/
OPT_touch,	/*[-c][-d hhmm|mmddhhmm[yy]] dateien*/
HLP_touch,	/*Modifikationsdatum „ndern*/
};

#define assign0(x,t,max)	if (t>max) return FALSE; else x=t;
#define assign1(x,t,max)	if (t<1 || t>max) return FALSE; else x=t;

#define SETDATE		1


typedef struct
{
	MGLOBAL *M;
	
	const char *prog_name;
	const char *prog_options;
	
	xtime systime;
	
} DATEINFO;


static uint tatoi (char *t, int offs)
{
	char tmp[3] = "??";
	
	strncpy (tmp, &t[offs], 2);
	return atoi (tmp);
}

static int convDate (DATEINFO *D, char *timestr, xtime *xt)
{
	int l = (int)strlen (timestr);

	switch (l)
	{
		case 10:
			assign0 (xt->s.year, (tatoi (timestr, 8) + 20) % 100, 99);

		case 8:
			xt->s.sec = 0;
			assign0 (xt->s.min, tatoi (timestr, 6), 59);
			assign0 (xt->s.hour, tatoi (timestr, 4), 23);
			assign1 (xt->s.day, tatoi (timestr, 2), 31);
			assign1 (xt->s.month, tatoi (timestr, 0), 12);
			break;

		case 4:
			xt->s.sec = 0;
			assign0 (xt->s.min, tatoi (timestr, 2), 59);
			assign0 (xt->s.hour, tatoi (timestr, 0), 23);
			break;

		default:
			eprintf(D->M, NlsStr(DT_BADSPEC));
			return FALSE;
	}
	
	return TRUE;
}

#if DATE_INTERNAL
/* set date & time */
static int setDate (DATEINFO *D, char *timestr)
{
	dostime dt;
	dosdate dd;
	
	if (convDate (D, timestr, &D->systime))
	{
		/* for old TOS versions, set GEMDOS date/time
		 */
		dd.s.year = D->systime.s.year;
		dd.s.month = D->systime.s.month;
		dd.s.day = D->systime.s.day;
		dt.s.hour = D->systime.s.hour;
		dt.s.min = D->systime.s.min;
		dt.s.sec = D->systime.s.sec;
		
		Settime (D->systime.dt);
		Tsetdate (dd.d);
		Tsettime (dt.t);
		
		print_date (D->M, NULL);
		crlf (D->M);
	 	return 0;
	}
	else
	{
		eprintf (D->M, "date: " DT_BADCONV "\n");
		return PrintUsage (D->M, D->prog_name, D->prog_options);
	}
}

int m_date (MGLOBAL *M, int argc, char **argv)
{
	DATEINFO D;
	
	if (!argc)
		return PrintHelp (M, argv[0], 
			NlsStr(OPT_date), NlsStr(HLP_date));
	
	D.M = M;
	D.prog_name = argv[0];
	D.prog_options = OPT_date;
	D.systime.dt = Gettime();

	if (argc > 2)
		return 1 + PrintUsage (M, argv[0], NlsStr(OPT_date));

	if (argc == 1)
	{	
		print_date (D.M, NULL);
		crlf (M);
		return 0;
	}
	else
	{
		if (argv[1][0] == '+')
		{
			print_date (D.M, &argv[1][1]);
			crlf (M);
			return 0;
		}
		else
			return setDate (&D, argv[1]);
	}
}
#endif

#if TOUCH_INTERNAL

static int touchFile (DATEINFO *D, char *filename,
	const char *orig_name, int dont_create)
{
	int hnd, broken = FALSE;
	dosdate fdate;
	dostime ftime;
	DOSTIME dostime;

	if (IsDirectory (filename, &broken))
	{
		eprintf(D->M, "touch: " DT_ISDIR "\n", orig_name);
		return 1;
	}
	else if (broken)
	{
		return (int)ioerror (D->M, "touch", orig_name, broken);
	}

	hnd = (int)Fopen (filename, 0);
	
	if (IsBiosError (hnd))
		return (int)ioerror (D->M, "touch", orig_name, hnd);
		
	if (!dont_create && (hnd < 0))
		hnd = (int)Fcreate (filename, 0);
		
	if (IsBiosError (hnd))
		return (int)ioerror (D->M, "touch", orig_name, hnd);
	
	if (hnd < 0)
	{
		if (dont_create)
			eprintf(D->M, "touch: " DT_NONEXIST "\n", orig_name);
		else
			eprintf(D->M, "touch: " DT_CANTCRT "\n", orig_name);
			
		return hnd;
	}
	
	fdate.s.day = D->systime.s.day;
	fdate.s.month = D->systime.s.month;
	fdate.s.year = D->systime.s.year;
	ftime.s.hour = D->systime.s.hour;
	ftime.s.min = D->systime.s.min;
	ftime.s.sec = D->systime.s.sec;
	dostime.time = ftime.t;
	dostime.date = fdate.d;
	
	Fdatime (&dostime, hnd, SETDATE);
	
	if (dostime.date == 0 && dostime.time == 0)
		eprintf (D->M, DT_CANTSET "\n", orig_name);

	Fclose (hnd);

	if (!D->M->shell_flags.system_call)
		DirtyDrives |= DriveBit (filename[0]);

	return 0;
}

int m_touch (MGLOBAL *M, int argc, char **argv)
{
	DATEINFO D;
	GETOPTINFO G;
	int c, i;
	int dont_create;
	int retcode = 0;
	char *datestr = NULL;
	static struct option long_option[] =
	{
		{ "no-create", FALSE, NULL, 'c'},
		{ "date", TRUE, NULL, 'd'},
		{ NULL,0,0,0},
	};
	int opt_index = 0;

	if (!argc)
	{
		PrintHelp (M, argv[0], NlsStr(OPT_touch), NlsStr(HLP_touch));
		return 0;
	}
	
	if (argc == 1)
	{
		PrintUsage (M, argv[0], NlsStr(OPT_touch));
		return 0;
	}
		
	D.M = M;
	D.prog_name = argv[0];
	D.prog_options = OPT_touch;
	dont_create = FALSE;

	optinit (&G);
	while ((c = getopt_long (M, &G, argc, argv, "cd:", long_option,
		&opt_index))!=EOF)
	{
		if (!c)
			c = long_option[opt_index].val;
			
		switch (c)
		{
			case 0:
				break;

			case 'c':
				dont_create = TRUE;
				break;

			case 'd':
				datestr = strdup (G.optarg);
				break;

			default:
				PrintUsage (M, argv[0], NlsStr(OPT_touch));
				return 0;
		}
	}
	
	D.systime.dt = Gettime ();

	if (datestr != NULL && !convDate (&D, datestr, &D.systime))
	{
		eprintf(M, "touch: " DT_BADCONV "\n");
		mfree (&M->alloc, datestr);
		PrintUsage (M, argv[0], NlsStr(OPT_touch));
		return 0;
	}

	if (G.optind == argc)
	{
		if (datestr)
			mfree (&M->alloc, datestr);
		PrintUsage (M, argv[0], NlsStr(OPT_touch));
		return 0;
	}
			
	for (i = G.optind; i < argc; ++i)
	{
		char *realname = NULL;
		
		if (!ContainsGEMDOSWildcard (argv[i]))
		{
			realname = NormalizePath (&M->alloc, argv[i], 
				M->current_drive, M->current_dir);
		}

		if (realname == NULL)
		{
			eprintf (M, "touch: " DT_ILLNAME "\n", argv[i]);
		}
		else
		{
			if (touchFile (&D, realname, argv[i], dont_create))
				++retcode;

			mfree (&M->alloc, realname);
		}
	}
		
	if (datestr != NULL)
		mfree (&M->alloc, datestr);
		
	return retcode;
}

#endif