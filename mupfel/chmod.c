/*
 * @(#)Mupfel/chmod.c
 * @(#)Gereon Steffens, Julian F. Reschke,
 * @(#)Stefan Eissing, 14. M„rz 1994
 *
 * jr 27.5.1996
*/
 
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"

#include "mglob.h"

#include "chario.h"
#include "chmod.h"
#include "mdef.h"


#define RW	0
#define ARCH	1
#define SYS	2
#define HID	3

#define GET_ATTRIBUTE		0
#define SET_ATTRIBUTE		1

/* stevie */

#if CMD_RUNOPTS
	#define OPT_runopts	"[-csv][-t tpasiz][[+-][flm]] dateien"
	#define HLP_runopts	"Optionen fr Programmtyp setzen"
	#define RO_NOTEXEC	"%s: %s ist keine Programmdatei\n"
	#define RO_ISDIR	"%s: %s ist ein Verzeichnis\n"
	#define RO_NOFILE	"%s: kann %s nicht ”ffnen\n"
	#define RO_CANTOPEN	"%s: kann %s nicht ”ffnen\n"
	#define RO_READHDR	"%s: kann Dateikopf von %s nicht lesen\n"
	#define RO_WRITEHDR	"%s: kann Dateikopf von %s nicht schreiben\n"
	#define RO_CANTWRITE "%s: %s ist schreibgeschtzt\n"
	#define RO_ILLSIZE	"%s: ungltige Gr”ženangabe %s\n"
	#define RO_OUTRANGE	"%s: Angabe %s aužerhalb des erlaubten Bereichs\n"
	#define RO_NOTPA	"%s: Gr”ženangabe fehlt\n"
#endif /* CMD_RUNOPTS */

/* internal texts
 */
#define NlsLocalSection "M.chmod"
enum NlsLocalText{
OPT_chmod,		/*[-cv][[+-][ahsw]] dateien*/
HLP_chmod,		/*Attribute setzen/l”schen*/
CH_ISDIR,		/*%s ist ein Verzeichnis*/
CH_GETATTR,		/*chmod: kann Attribute von %s nicht lesen*/
CH_SETATTR,		/*chmod: kann Attribute von %s nicht setzen\n*/
ILLOPT,			/*%s: ungltige Option %c\n*/
ILLOPTST,		/*%s: ungltige Option %s\n*/
};

#if CMD_RUNOPTS

typedef struct
{
	const char *name;
	
	const char *file;
	
	int fastload;
	int loadtofast;
	int mallocfast;
	int	tpa_size;
	int verbose;
	int changes_only;
	int silent;
	
	int broken;
	
} ROINFO;

/* program/object header
 */
typedef struct
{
	int branch;
	long textlen;
	long datalen;
	long bsslen;
	long symlen;
	long reserved1;
	struct
	{
		unsigned tpasize: 4;
		unsigned res1: 12;
		unsigned res2: 13;
		unsigned malt: 1;
		unsigned lalt: 1;
		unsigned fload: 1;
	} loadflags;
	int flag;
} HEADER;

int set_run_flags (MGLOBAL *M, ROINFO *R)
{
	int hnd, can_write = TRUE;
	HEADER header;
	int changed = FALSE;
	
	if (!access (R->file, A_EXEC, &R->broken))
	{
		if (R->silent) 
			return FALSE;
	
		if (access (R->file, A_EXIST, &R->broken))
			eprintf (M, RO_NOTEXEC, R->name, R->file);
		else
		{
			if (IsDirectory (R->file, &R->broken))
				eprintf(M, RO_ISDIR, R->name, R->file);
			else
				eprintf(M, RO_NOFILE, R->name, R->file);
		}
		
		return FALSE;
	}

	hnd = (int)Fopen (R->file, 2);
	
	if (hnd < 0)
	{
		/* Datei kann nicht beschrieben werden -- also nur zum
		Lesen ”ffnen */
		
		can_write = FALSE;
		hnd = (int)Fopen (R->file, 0);
		can_write = FALSE;
	}
	
	if (hnd < 0)
	{
		eprintf (M, RO_CANTOPEN, R->name, R->file);
		return FALSE;
	}
		
	if (Fread (hnd, sizeof(HEADER), &header) != sizeof(HEADER))
		eprintf (M, RO_READHDR, R->name, R->file);
	else
	{
		if ((R->fastload != -1)
			&& (header.loadflags.fload != R->fastload))
		{
			/* set fastload
			 */
			changed = TRUE;
			header.loadflags.fload = R->fastload;
		}

		if ((R->loadtofast != -1)
			&& (header.loadflags.lalt != R->loadtofast))
		{
			/* set load to fast ram
			 */
			changed = TRUE;
			header.loadflags.lalt = R->loadtofast;
		}

		if ((R->mallocfast != -1) 
			&& (header.loadflags.malt != R->mallocfast))
		{
			/* set malloc from fast ram
			 */
			changed = TRUE;
			header.loadflags.malt = R->mallocfast;
		}

		if ((R->tpa_size != -1)
			&& (header.loadflags.tpasize != R->tpa_size))
		{
			/* set tpasize
			 */
			changed = TRUE;
			header.loadflags.tpasize = R->tpa_size;
		}
		
		if (changed && !can_write)
		{
			eprintf (M, RO_CANTWRITE, R->name, R->file);
			Fclose(hnd);
			return FALSE;
		}
		else
		{
			/* rewrite the header?
			 */
			if (changed)
			{
				Fseek (0L, hnd, 0);		/* back to start */
				
				if (Fwrite (hnd, sizeof(HEADER), &header) 
					!= sizeof (HEADER))
				{
					eprintf (M, RO_WRITEHDR, R->name, R->file);
					Fclose (hnd);
					return FALSE;
				}
			}
	
			if (R->verbose || (R->changes_only && changed))
			{
				mprintf (M, "%c%c%c %4dK  %s\n",
					(header.loadflags.fload) ? 'f' : '-',
					(header.loadflags.lalt) ? 'l' : '-',
					(header.loadflags.malt) ? 'm' : '-',
					(header.loadflags.tpasize + 1) * 128,
					R->file);
			}
		}
	}

	Fclose(hnd);
	return TRUE;
}

/* calculate tpa size from ascii string, return -1 for error */

int calc_tpa (MGLOBAL *M, char *str, const char *program_name)
{
	int mega_bytes = FALSE;
	int size;
	
	if (toupper (lastchr (str)) == 'M')
	{
		mega_bytes = TRUE;
		str[strlen(str)-1] = 0;
	}
	
	if (toupper (lastchr (str)) == 'K')	/* ignore k's */
		str[strlen(str)-1] = 0;

	if (!isdigit (lastchr (str)))
	{
		eprintf (M, RO_ILLSIZE, program_name, str);
		return -1;
	}
		
	size = atoi (str);
	
	if (mega_bytes) size *= 1024;
	size = (size + 63) & ~127;	/* round to multiples of 128 */

	if ((size < 128) || (size > 2048))
	{
		if (!size)
			eprintf (M, RO_ILLSIZE, program_name, str);
		else
			eprintf (M, RO_OUTRANGE, program_name, str);
		return -1;
	}

	return size;	
}


int m_runopts (MGLOBAL *M, int argc, char **argv)
{
	ROINFO R;
	int i;
	int something_done;
		
	if (!argc)
		return PrintHelp (M, argv[0], 
			NlsStr(OPT_runopts), NlsStr(HLP_runopts));
	
	memset (&R, 0, sizeof (ROINFO));
	R.fastload = R.loadtofast = R.mallocfast = R.tpa_size = -1;
	R.name = argv[0];

	for (i = 1; i < argc; i++)
	{
		char opt_char = argv[i][0];

		if (!strcmp (argv[i], "--"))
		{
			argv[i][0] = 0;
			break;
		}

		if ((opt_char == '+') || (opt_char == '-'))
		{
			size_t slen = strlen (argv[i]);
			int j , break_it = FALSE;
			
			for (j = 1; (j < slen) && (!break_it); j++)
			{
				switch (argv[i][j])
				{
					case 'f':
						R.fastload = (opt_char == '+') ? 1 : 0;
						break;

					case 'l':
						R.loadtofast = (opt_char == '+') ? 1 : 0;
						break;

					case 'm':
						R.mallocfast = (opt_char == '+') ? 1 : 0;
						break;

					case 'c':
						if ((opt_char == '-') || (!strncmp ("+changes-only",
							argv[i], strlen (argv[i]))))
						{
							R.changes_only = TRUE;
							if (opt_char == '+') break_it = TRUE;
						}
						else	/* found beginning with '+' */
						{
							eprintf (M, ILLOPTST, argv[0], argv[i]);
							return 2;
						}
						break;

					case 's':
						if ((opt_char == '-') || (!strncmp ("+silent",
							argv[i], strlen (argv[i]))))
						{
							R.silent = TRUE;
							if (opt_char == '+') break_it = TRUE;
						}
						else	/* found beginning with '+' */
						{
							eprintf (M, ILLOPTST, argv[0], argv[i]);
							return 2;
						}
						break;

					case 't':
						if ((opt_char == '-') || (!strncmp ("+tpa-size",
							argv[i], strlen (argv[i]))))
						{
							/* parameter in same string? */
							
							if ((opt_char == '-') && (j < (slen-1)))
							{
								R.tpa_size = calc_tpa (M, 
									&argv[i][j+1], argv[0]);
								break_it = TRUE;
							}
							else
							{
								if (i < (argc - 1))
								{
									if (opt_char == '+') break_it = TRUE;
									R.tpa_size = calc_tpa (M, 
										argv[i+1], argv[0]);
									argv[i+1][0] = 0;
								}
								else
								{
									eprintf (M, RO_NOTPA, argv[0]);
									return 2;
								}	
																	
							}
						}
						else
						{
							eprintf (M, ILLOPT, argv[0], argv[i][j]);
							return 2;
						}
						break;

					case 'v':
						if ((opt_char == '-') || (!strncmp ("+verbose", argv[i],
							strlen (argv[i]))))
						{
							R.verbose = TRUE;
							if (opt_char == '+') break_it = TRUE;
						}
						else	/* found beginning with '+' */
						{
							eprintf (M, ILLOPTST, argv[0], argv[i]);
							return 2;
						}
						break;

					default:
						eprintf (M, ILLOPT, argv[0], argv[i][j]);
						return 2;
				}
			}
			
			argv[i][0] = 0;
		}
	} 

	if ((R.fastload == -1) && (R.loadtofast == -1) &&
		(R.mallocfast == -1) && (R.tpa_size == -1))
	{
		R.verbose = TRUE;
	}

	if (R.tpa_size != -1)
	{
		R.tpa_size -= 128;
		R.tpa_size /= 128;
	}

	something_done = FALSE;

	for (i = 1; (i < argc) && (!intr (M)) && !R.broken; i++)
	{
		if (argv[i][0])
		{
			something_done = TRUE;
			
			R.file = argv[i];
			set_run_flags (M, &R);
		}
	}
	
	if (!something_done)
		return PrintUsage (M, argv[0], NlsStr(OPT_runopts));
	
	return (R.broken != FALSE);
}

#endif /* CMD_RUNOPTS */


long
SetWriteMode (const char *filename, int mode)
{
	int attrib;
	
	attrib = Fattrib (filename, GET_ATTRIBUTE, 0);
	if (mode)
		attrib &= ~FA_READONLY;
	else
		attrib |= FA_READONLY;

	return Fattrib (filename, SET_ATTRIBUTE, attrib);
}

#if CHMOD_INTERNAL

static void change_mode (MGLOBAL *M, char *file, int attr[], 
	int verbose, int changes_only)
{
	int attrib, i, set, reset;
	int saveattrib;
	
	if ((attrib = Fattrib (file, GET_ATTRIBUTE, 0)) < 0)
	{
		eprintf (M, NlsStr(CH_GETATTR), file);
		return;
	}
	saveattrib = attrib;
	
	for (i=0; i<4; ++i)
	{
		if (attr[i]!=-1)
		{
			switch (i)
			{
				case RW:
					set=FA_READONLY;
					break;
				case ARCH:
					set=FA_ARCHIVE;
					break;
				case SYS:
					set=FA_SYSTEM;
					break;
				case HID:
					set=FA_HIDDEN;
					break;
			}
			
			reset = ~set;
			if (attr[i])
				attrib |= set;
			else
				attrib &= reset;
		}
	}
	
	if (saveattrib != attrib)
	{
		if (Fattrib (file, SET_ATTRIBUTE, attrib) < 0)
		{
			eprintf (M, NlsStr(CH_SETATTR), file);
			return;
		}
		else if (!M->shell_flags.system_call)
			DirtyDrives |= DriveBit (file[0]);
	}

	if (verbose || (changes_only && (saveattrib != attrib)))
		mprintf (M, "%c%c%c%c (%03o) %s\n",
			attrib & FA_READONLY ? 'r' : '-',
			attrib & FA_HIDDEN ? 'h' : '-',
			attrib & FA_SYSTEM ? 's' : '-',
			attrib & FA_ARCHIVE   ? 'a' : '-',
			attrib, file);
}


int m_chmod (MGLOBAL *M, int argc, char **argv)
{
	int i, attr[4] = {-1,-1,-1,-1};
	int something_done = FALSE;
	int changes_only = FALSE;
	int verbose = FALSE;
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_chmod), 
			NlsStr(HLP_chmod));

	for (i = 1; i < argc; i++)
	{
		char opt_char = argv[i][0];

		if (!strcmp (argv[i], "--"))
		{
			argv[i][0] = 0;
			break;
		}

		if ((opt_char == '+') || (opt_char == '-'))
		{
			size_t slen = strlen (argv[i]);
			int j , break_it = FALSE;
			
			for (j = 1; (j < slen) && (!break_it); j++)
			{
				switch (argv[i][j])
				{
					case 'w':
						attr[RW] = (opt_char == '+') ? 0 : 1;
						break;

					case 'r':
						attr[RW] = (opt_char == '+') ? 1 : 0;
						break;

					case 'a':
						attr[ARCH] = (opt_char == '+') ? 1 : 0;
						break;

					case 's':
						attr[SYS] = (opt_char == '+') ? 1 : 0;
						break;

					case 'h':
						attr[HID] = (opt_char == '+') ? 1 : 0;
						break;

					case 'c':
						if ((opt_char == '-') || (!strncmp ("+changes-only",
							argv[i], strlen (argv[i]))))
						{
							changes_only = TRUE;
							if (opt_char == '+') break_it = TRUE;
						}
						else	/* found beginning with '+' */
						{
							eprintf (M, NlsStr(ILLOPTST), argv[0], argv[i]);
							return 2;
						}
						break;

					case 'v':
						if ((opt_char == '-') || (!strncmp ("+verbose", argv[i],
							strlen (argv[i]))))
						{
							verbose = TRUE;
							if (opt_char == '+') break_it = TRUE;
						}
						else	/* found beginning with '+' */
						{
							eprintf (M, NlsStr(ILLOPTST), argv[0], argv[i]);
							return 2;
						}
						break;

					default:
						eprintf (M, NlsStr(ILLOPT), argv[0], argv[i][j]);
						return 2;
				}
			}
			
			argv[i][0] = 0;
		}
	} 
	
	for (i = 1; (i < argc) && (!intr (M)); ++i)
	{
		if (argv[i][0])
		{
			something_done = TRUE;
			change_mode (M, argv[i], attr, verbose, changes_only);
		}
	}
	
	if (!something_done)
		return PrintUsage (M, argv[0], NlsStr(OPT_chmod));
	
	return 0;
}

#endif
