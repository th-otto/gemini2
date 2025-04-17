/*
 * @(#) Mupfel\cookie.c
 * @(#) Stefan Eissing & Gereon Steffens &
 * @(#) Julian Reschke, 10. November 1992
 *
 *  -  Cookie Jar management
 *
 * jr 2.2.95
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\cookie.h"
#include "..\common\strerror.h"
#include "mglob.h"

#include "chario.h"
#include "cookie.h"
#include "getopt.h"
#include "mdef.h"
#include "sysvars.h"
#include "version.h"

#define Ssystem(a,b,c)	gemdos(0x154,(int)a,(long)b,(long)c)

/* internal texts
 */
#define NlsLocalSection "M.cookie"
enum NlsLocalText{
OPT_cookie,	/*[-flsv]*/
HLP_cookie,	/*Cookie-Jar anzeigen*/
CO_HEADER,	/*Cookie  Wert\n*/
CO_LHEADER,	/*Cookie  Hex       Wert\n*/
CO_NOCOOKIE,/*%s: Cookie Jar nicht vorhanden\n*/
CO_C_SWI,	/*DIP-Schalter*/
CO_C_SLM,	/*Diablo-Emulator*/
CO_C_FLK,	/*GEMDOS File Locking verfÅgbar*/
CO_C_NET,	/*Netzwerk*/
CO_VERSION,	/*Version*/
CO_ROOM,	/*Es ist Platz fÅr insgesamt %ld EintrÑge (%ld belegt)\n*/
CO_LANG,	/*Landessprache: %s, Tastatur: %s*/
CO_NOATARI,	/*kein Atari*/
};

typedef enum { silent, unset, small, verbose, verylong } verbosity;

/* return char value that can be printed */

int
as_printable (int c)
{
	if (!c)
		return ' ';
	
	if ((c <= ' ') && (c >= 0))
		return '?';

	return c;
}


/* compare argv value with cookie name */

int
cookie_match (char *arg_val, long cookie_name)
{
	long to_compare;
	
	/* zu lang? */
	
	if (strlen (arg_val) > 4) return FALSE;

	strncpy ((char *)&to_compare, arg_val, 4);
	
	return to_compare == cookie_name;
}


static const char *vdoval (long val)
{
	static char *vdotxt[] =
	{
		"ST", "STE", "TT", "Falcon"
	};

	if (val == 0xffffffffL)
		return NlsStr(CO_NOATARI);

	val >>= 16;	
	if (val >= 0 && val < DIM(vdotxt))
		return vdotxt[val];
	else
		return "???";
}

static void fpuval (MGLOBAL *M, long val)
{
	int comma = 0, hword, bit0, bit12, bit3;
	
	hword = *((int *)&val);
	bit0 = hword & 0x01;
	bit12 = hword & 0x06;
	bit3 = hword & 0x08;
	
	if (bit0)
		mprintf (M, "SFP004"), ++comma;

	if (bit12)
	{	
		if (bit12 == 2)
			mprintf (M, "%s68881/68882", comma ? ", " : "");
		else if (bit12 == 4)
			mprintf (M, "%s68881", comma ? ", " : "");
		else if (bit12 == 6)
			mprintf(M, "%s68882", comma ? ", " : "");

		++comma;
	}

	if (bit3)
		mprintf (M, "%s68040", comma ? ", " : "");
}

static const char *mchval (long val)
{
	if (val == 0xffffffffL)
		return NlsStr(CO_NOATARI);

	switch ((int)(val >> 16))
	{
		case 0:
			return "(Mega) ST";
		
		case 1:
		{
			switch ((int)(val & 0xffffL))
			{
				case 0x0001:    return "ST-Book";
				case 0x0008:    return "STE+";
				case 0x0010:    return "Mega-STE";
				case 0x0100:    return "Sparrow";
				default:        return "STE";
			}
		}
			
		case 2:
			return "TT";
		
		case 3:
		    return "Falcon";

		default:
			return "???";
	}
}

static void sndval (MGLOBAL *M, long val)
{
   int comma = 0;

   if (val & 0x01)
       mprintf (M, "GI"), ++comma;
   if (val & 0x02)
       mprintf (M, "%sDMA", comma++ ? ", " : "");
   if (val & 0x04)
       mprintf (M, "%sCODEC", comma++ ? ", " : "");
   if (val & 0x08)
       mprintf (M, "%sDSP", comma++ ? ", " : "");
   if (val & 0x10)
       mprintf (M, "%sConnection Matrix", comma++ ? ", " : "");
}

static void akpval (MGLOBAL *M, long val)
{
	mprintf (M, NlsStr(CO_LANG),
		GetCountryId ((int)((val >> 8) & 0xff)),
		GetCountryId ((int)(val & 0xff)));
}

static void idtval (MGLOBAL *M, long val)
{
	char delim = (int)(val & 0xff);
	long ordering = (val & 0x0f00) >> 8;
	
	if (!delim) delim = '/';

	mprintf (M, "Datumsformat: ");

	switch ((int) ordering)
	{
		case 3: mprintf (M, "yy%cdd%cmm", delim, delim); break;
		case 2: mprintf (M, "yy%cmm%cdd", delim, delim); break;
		case 1: mprintf (M, "dd%cmm%cyy", delim, delim); break;
		default: mprintf (M, "mm%cdd%cyy", delim, delim); break;
	}
	
	if (!(val & 0xf000)) mprintf (M, " am/pm");
}

static void fdcval (MGLOBAL *M, long val)
{
	int density;
	
	static char *fdctxt[] = 
	{
		"double density", "high density", "extra-high density", "??"
	};
	char *cp = (char *)&val;
	
	density = cp[0];
	if (density >= DIM (fdctxt))
		density = (int) DIM (fdctxt) - 1;
		
	mprintf (M, " (%s, %c%c%c)\n", fdctxt[density],
		as_printable (cp[1]),
		as_printable (cp[2]),
		as_printable (cp[3]));
}

static void netval (MGLOBAL *M, long val)
{
	struct net_info { char nid[4]; long version; };
	struct net_info *ni = (struct net_info *)val;

	mprintf (M, " (%s, %c%c%c%c, %s %ld)\n",
		NlsStr(CO_C_NET), as_printable (ni->nid[0]),
			as_printable (ni->nid[1]), as_printable (ni->nid[2]),
			as_printable (ni->nid[3]), NlsStr(CO_VERSION), ni->version);
}

static void showcookie (MGLOBAL *M, long *cj, int *first, verbosity v)
{
	char cookie[] = "____";
	long cookieval;
	int i;

	strncpy (cookie, (char *)cj, 4);
	cookieval = *(cj+1);

	if (v == silent) return;
	
	if (v == small)
	{
		mprintf (M, "%08lx\n", cookieval);
		return;
	}
	
	for (i = 0; i < 4; ++i)
		cookie[i] = as_printable (cookie[i]);
	
	if (*first)
	{
		mprintf (M, v == verylong ? 
			NlsStr(CO_LHEADER) : NlsStr(CO_HEADER));
		*first = FALSE;
	}
	
	mprintf (M, "%s   ", cookie);
	
	if (v == verylong)
		mprintf (M, " %08lx ", cj[0]);
	
	mprintf (M, " %08lx ", cookieval);
	
	if (!strcmp (cookie, "_CPU"))
	{
		mprintf (M, " (%ld)\n", 68000L + cookieval);
	}
	else if (!strcmp (cookie, "_VDO"))
	{
		mprintf (M, " (%s)\n", vdoval (cookieval));
	}
	else if (!strcmp (cookie, "_FPU"))
	{
		mprintf (M, " (");
		fpuval (M, cookieval);
		mprintf (M, ")\n");
	}
	else if (!strcmp (cookie, "_IDT"))
	{
		mprintf (M, " (");
		idtval (M, cookieval);
		mprintf (M, ")\n");
	}
	else if (!strcmp (cookie, "_AKP"))
	{
		mprintf (M, " (");
		akpval (M, cookieval);
		mprintf (M, ")\n");
	}
	else if (!strcmp (cookie, "_SND"))
	{
		mprintf (M, " (");
		sndval (M, cookieval);
		mprintf (M, ")\n");
	}
	else if (!strcmp (cookie, "_MCH"))
	{
		mprintf (M, " (%s)\n", mchval (cookieval));
	}
	else if (!strcmp (cookie, "MNAM"))
	{
		mprintf (M, " (%s)\n", cookieval);
	}
	else if (!strcmp (cookie, "_FDC"))
	{
		fdcval (M, cookieval);
	}
	else if (!strcmp (cookie, "_NET"))
	{
		netval (M, cookieval);
	}
	else if (!strcmp (cookie, "_SWI"))
	{
		mprintf (M, " (%s)\n", NlsStr(CO_C_SWI));
	}
	else if (!strcmp (cookie, "_FLK"))
	{
		mprintf (M, " (%s)\n", NlsStr(CO_C_FLK));
	}
	else if (!strcmp (cookie, "_SLM"))
	{
		mprintf (M, " (%s)\n", NlsStr(CO_C_SLM));
	}
	else if (!strcmp (cookie, "_FRB"))
	{
		mprintf (M, " (Fast RAM Buffer)\n");
	}
	else
		crlf (M);
}
 
int m_cookie (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	int i, c;
	long *cookiejar;
	long count = 0;
	int first;
	enum verbosity verbosity = unset;
	struct option long_option[] =
	{
		{ "free", FALSE, NULL, 'f' },
		{ "long", FALSE, NULL, 'l' },
		{ "silent", FALSE, NULL, 's' },
		{ "very-long", FALSE, NULL, 'v' },
		{ NULL,0,0,0 },
	};
	int opt_index = 0;
	int show_free = FALSE;
	int show_special = FALSE;
	int to_show;
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_cookie), 
			NlsStr(HLP_cookie));

	optinit (&G);
	
	while ((c = getopt_long (M, &G, argc, argv, "flsv", long_option,
		&opt_index)) != EOF)
	{
		if (!c)
			c = long_option[opt_index].val;
			
		switch(c)
		{
			case 0:
				break;

			case 'f':
				show_free = TRUE;
				break;

			case 'l':
				verbosity = verbose;
				break;

			case 's':
				verbosity = silent;
				break;

			case 'v':
				verbosity = verylong;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_cookie));
		}
	}

	/* ermittle, wieviele Cookies angezeigt werden sollen */
	
	show_special = to_show = (argc - G.optind) + (show_free ? 1 : 0);

	if (verbosity == unset)
		verbosity = show_special ? small : verbose;

	cookiejar = (long *) Ssystem (10, 0x000005a0L, NULL);

	if (cookiejar == (long *) EINVFN)
	{
		long oldstack = Super (NULL);
		cookiejar = *_P_COOKIES;
		Super ((void *)oldstack);
	}
	
	/* Wenn Cookies gezielt angezeigt werden sollten: Anzahl der
	nicht ausgegebenen Cookies zurÅck. Sonst: 1 */
	
	if (!LegalCookie (cookiejar))
	{
		if (verbosity != silent)
			eprintf (M, NlsStr(CO_NOCOOKIE), argv[0]);
		return show_special ? to_show : 1;
	}

	first = TRUE;	
	while (*cookiejar != 0L)
	{
		if (show_special)
		{
			for (i = G.optind; i < argc; i++)
				if (cookie_match (argv[i], cookiejar[0]))
				{
					showcookie (M, cookiejar, &first, verbosity);
					to_show--;
					break;
				}
		}
		else
			showcookie (M, cookiejar, &first, verbosity);

		cookiejar += 2;
		count += 1;
	}

	if (!show_special || show_free)
	{
		switch (verbosity)
		{
			case verylong:
			case verbose:
				mprintf (M, NlsStr(CO_ROOM), cookiejar[1], count + 1);	
				break;
			
			case small:
				mprintf (M, "%08lx\n", cookiejar[1]);
				break;
		}
		to_show--;
	}
	
	return show_special ? to_show : 0;
}
