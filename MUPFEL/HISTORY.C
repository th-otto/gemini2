/*
 * @(#) Mupfel\history.c
 * @(#) Stefan Eissing, 28. Januar 1995
 *
 * - History fÅr Zeileneditor
 *
 * jr 970118
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <nls\nls.h>
 
#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "getopt.h"
#include "history.h"
#include "mdef.h"
#include "..\common\memfile.h"
#include "nameutil.h"
#include "parsinpt.h"

/* internal texts
 */
#define NlsLocalSection "M.history"
enum NlsLocalText{
OPT_history,	/*[-s nummer] [-a|-n] [[-r|-w] [datei]]*/
HLP_history,	/*History anzeigen, lesen, schreiben*/
OPT_fc,			/*TBD*/
HLP_fc,			/*Kommandoliste bearbeiten*/
HI_CANTSET,		/*%s: kann Grîûe %d nicht setzen\n*/
};

#define DEFAULT_HISTORY_SIZE	20
#define DEFAULT_HISTFILE		"mupfel.hst"


/* Gib den Speicher fÅr die Zeilen in M->history.lines wieder
   frei. */

static void
freeHistoryLines (MGLOBAL *M, int from)
{
	int i;
	
	for (i = from; i < M->history.size; ++i)
	{
		if (M->history.lines[i].str)
		{
			mfree (&M->alloc, M->history.lines[i].str);
			M->history.lines[i].str = NULL;
			M->history.lines[i].first = -1;
		}
	}
}


/* Setze die Grîûe der History auf die Grîûe <new_size>. Alte
   Zeilen bleiben (soweit mîglich) erhalten. Bei negativer Grîûe
   oder mangelndem Speicher wird FALSE zurÅckgeliefert. */

static int
historySetSize (MGLOBAL *M, int new_size)
{
	HISTLINE *new_lines;
	int to_copy;
	
	if (new_size < 1) return FALSE;
	
	new_lines = mcalloc (&M->alloc, new_size, sizeof (HISTLINE));
	if (!new_lines) return FALSE;

	to_copy = min (new_size, M->history.size);

	if (to_copy)
	{
		memcpy (new_lines, M->history.lines, 
			to_copy * sizeof (HISTLINE));
	}
	
	freeHistoryLines (M, to_copy);
	
	if (M->history.lines) mfree (&M->alloc, M->history.lines);

	M->history.lines = new_lines;
	M->history.size = new_size;
	
	return TRUE;
}


/* Initialisiere die Informationen im Shell-Level <M> fÅr die
   History */

int
InitHistory (MGLOBAL *M)
{
	memset (&M->history, 0, sizeof (HISTORYINFO));
	
	return historySetSize (M, DEFAULT_HISTORY_SIZE);
}


/* Vererbe die Informationen von Shell-Level <M> an den Level <new> */

int
ReInitHistory (MGLOBAL *M, MGLOBAL *new)
{
	int i;

	memset (&new->history, 0, sizeof (HISTORYINFO));
	new->history.size = M->history.size;
	new->history.flags = M->history.flags;

	if (!new->history.size) return TRUE;
	
	new->history.lines = mcalloc (&new->alloc, new->history.size,
			sizeof (HISTLINE));
	
	if (!new->history.lines) return FALSE;
	 
	for (i = 0; i < M->history.size; ++i)
	{
		if (M->history.lines[i].str)
		{
			new->history.lines[i].str = StrDup (&new->alloc, 
				M->history.lines[i].str);
			new->history.lines[i].first = M->history.lines[i].first;
			new->history.lines[i].last = M->history.lines[i].last;
		}
	}
	
	return TRUE;	
}


/* Gib den Speicher fÅr History am Ende des Shell-Levels <M>
   wieder frei. */

void
ExitHistory (MGLOBAL *M)
{
	if (M->history.lines)
	{
		freeHistoryLines (M, 0);
		mfree (&M->alloc, M->history.lines);
	}
}


/* Gib einen Zeiger auf die zuletzt eingegeben Zeile zurÅck */

const char *
GetLastHistoryLine (MGLOBAL *M)
{
	return M->history.lines[0].str;
}


/* FÅge Zeile <line> in die History ein. Bei FALSE war nicht
   genug Speicher vorhanden. */

int
EnterHistory (MGLOBAL *M, const char *line)
{
	char *tmp;
	size_t len;
	
	tmp = StrDup (&M->alloc, line);
	if (!tmp) return FALSE;
	
	len = strlen (tmp);
	if (len && tmp[len - 1] == '\n') tmp[len - 1] = '\0';
	
	if (M->history.flags.no_doubles)
	{
		if (M->history.lines[0].str && !strcmp (M->history.lines[0].str, tmp))
		{
			M->history.lines[0].last = (unsigned int) ParsLineCount (M);
			mfree (&M->alloc, tmp);
			return TRUE;
		}
	}
	
	if (M->history.lines[M->history.size - 1].str)
		mfree (&M->alloc, M->history.lines[M->history.size - 1].str);
	
	memmove (&M->history.lines[1], &M->history.lines[0],
		(M->history.size - 1) * sizeof (HISTLINE));
	
	M->history.lines[0].str = tmp;
	M->history.lines[0].last =
	M->history.lines[0].first = (unsigned int) ParsLineCount (M);
	
	return TRUE;
}


/* Funktion, die vom Benutzer von GetHistoryLine benutzt wird, um
   die Suche wieder zurÅckzusetzen. */

void
HistoryResetSaveBuffer (MGLOBAL *M, int *save_buffer)
{
	(void)M;
	*save_buffer = -1;
}


/* Suche nach einer Zeile in der History. Diese Funktion hat folgende
   Aufgaben:
   - Wenn <pattern> gleich NULL ist, wird einfach nur die nÑchste
     Zeile zurÅckgeliefert. (Richtung ist abhÑngig von <upward>.
   - Falls Pattern ungleich NULL ist, wird nach der nÑchsten Zeile
     gesucht, die mit <pattern> beginnt.

   Der Ort der letzten Suche wird in <save_state> abgespeichert,
   damit beim nÑchsten Aufruf dort weitergesucht werden kann. */

const char *
GetHistoryLine (MGLOBAL *M, int upward, int *save_state,
	const char *pattern)
{
	int i, walk;
	
	walk = upward? 1 : -1;
		
	for (i = *save_state + walk; i < M->history.size && i >= 0;
		i += walk)
	{
		const char *line = M->history.lines[i].str;
		
		if (!line) break;

		if (pattern == NULL || (line == strstr (line, pattern)))
		{
			*save_state = i;
			return line;
		}
	}
	
	return NULL;
}
	
	
/* Speicher History in Datei <filename> ab. */

static long 
writeHistory (MGLOBAL *M, const char *filename)
{
	long fhandle;
	int i;
	long retcode = E_OK;
	
	if (M->history.size <= 0) return E_OK;

	fhandle = Fcreate (filename, 0);
	if (fhandle < 0) return fhandle;
	
	for (i = 0; i < M->history.size; ++i)
	{
		if (M->history.lines[i].str)
		{
			size_t len;
			
			len = strlen (M->history.lines[i].str);
			
			if (Fwrite ((int) fhandle, len, M->history.lines[i].str) != len
				|| Fwrite ((int) fhandle, 2, "\r\n") != 2)
			{
				retcode = ERROR;
				break;
			}
		}
	}
	
	Fclose ((int) fhandle);
		
	return retcode;
}

#define HISTLINE_LEN	1024

/* Lese die History aus der Datei <filename> wieder ein. Eine
   Zeile in der History sollte dabei nicht lÑnger als HISTLINE_LEN
   Zeichen sein. */

static long
readHistory (MGLOBAL *M, const char *filename)
{
	MEMFILEINFO *mp;
	int i;
	long retcode = E_OK;
	
	if (M->history.size <= 0) return E_OK;
		
	freeHistoryLines (M, 0);

	mp = mopen (&M->alloc, filename, &retcode);
	if (!mp) return retcode;
	
	for (i = 0; i < M->history.size; ++i)
	{
		char buffer[HISTLINE_LEN];
		size_t last;
		
		if (mgets (buffer, HISTLINE_LEN - 1, mp) == NULL)
			break;

		buffer[HISTLINE_LEN - 1] = '\0';
		last = strlen (buffer) - 1;
		if (last > 0 && buffer[last] == '\n')
			buffer[last] = '\0';
		
		M->history.lines[i].str = StrDup (&M->alloc, buffer);
	}
	
	mclose (&M->alloc, mp);
	
	return retcode;
}

/* Gebe eine Zeile der History aus */

static void
display_histline (MGLOBAL *M, HISTLINE *hp, unsigned int lineno,
	int with_number)
{
	if (with_number) mprintf (M, "%d", lineno);
	mprintf (M, "\t");
	domprint (M, hp->str, strlen (hp->str));
	crlf (M);
}


/* Gebe die History aus. Die High-Words enthalten den Index
   in der History, die Low-Words... */

static void
display_history (MGLOBAL *M, unsigned long first_i, unsigned long last_i,
	int numbers, int nodoubles)
{
	long i;
	unsigned int first, last;
	
	first = (unsigned int) (first_i >> 16);
	last = (unsigned int) (last_i >> 16);

	if (last > first)
	{
		for (i = 0; i < M->history.size && !intr (M); i++)
		{
			if (!M->history.lines[i].str) continue;
			if (i >= first && i <= last)
			{
				unsigned int to = M->history.lines[i].first;
				long j;
				
				if (!nodoubles)
					to = M->history.lines[i].last;	
			
				for (j = to; j >= M->history.lines[i].first; j--)
					display_histline (M, &M->history.lines[i],
						(unsigned int) j, numbers);
			}
		}
	}
	else
	{
		for (i = M->history.size - 1; i >= 0&& !intr (M); i--)
		{
			if (!M->history.lines[i].str) continue;
			if (i >= last && i <= first)
			{
				unsigned int to = M->history.lines[i].first;
				long j;
				
				if (!nodoubles)
					to = M->history.lines[i].last;	
						
				for (j = M->history.lines[i].first; j <= to; j++)
					display_histline (M, &M->history.lines[i],
						(unsigned int) j, numbers);
			}
		}
	}
}



/* Internes Kommando <history> der Mupfel. Kann Grîûe Setzen,
   speichern, laden und die History anzeigen. */
   
int
m_history (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	int c, opt_index;
	int i, new_size, set_size, write_history, read_history;
	int print_history;
	const char *hist_file;
	static struct option long_options[] =
	{
		{ "all", TRUE, NULL, 'a'},
		{ "no-doubles", TRUE, NULL, 'n'},
		{ "size", TRUE, NULL, 's'},
		{ "read", FALSE, NULL, 'r'},
		{ "write", FALSE, NULL, 'w'},
		{ NULL,0,0,0},
	};
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_history), 
			NlsStr(HLP_history));
	
	optinit (&G);
	read_history = write_history = set_size = 0;
	print_history = TRUE;
	
	while ((c = getopt_long (M, &G, argc, argv, "ans:rw", 
		long_options, &opt_index)) != EOF)
	{
		if (!c)			c = long_options[G.option_index].val;

		switch (c)
		{
			case 0:
				break;

			case 'a':
				M->history.flags.no_doubles = 0;
				print_history = FALSE;
				break;
			
			case 'n':
				M->history.flags.no_doubles = 1;
				print_history = FALSE;
				break;
				
			case 's':
				set_size = 1;
				new_size = atoi (G.optarg);
				print_history = FALSE;
				break;

			case 'r':
				read_history = 1;
				break;

			case 'w':
				write_history = 1;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_history));
		}
	}

	if ((argc - G.optind > 1) || (write_history && read_history))
		return PrintUsage (M, argv[0], NlsStr(OPT_history));
	
	if (set_size)
	{
		/* Grîûe der History neu Setzen */
		if (!historySetSize (M, new_size))
		{
			eprintf (M, NlsStr(HI_CANTSET), argv[0], new_size);
			return 1;
		}
	}
	
	if (write_history || read_history)
	{
		long retcode = E_OK;
		char *realname;
		const char *home = NULL;
		
		/* Wenn noch Parameter da sind, benutze ihn als Namen der
		   Datei. Ansonsten nehme $HISTFILE. Falls dies nicht
		   vorhanden ist, nimm $HOME\DEFAULT_HISTFILE */

		if (argc > G.optind)
			hist_file = argv[G.optind];
		else
		{
			hist_file = GetVarValue (M, "HISTFILE");

			if ((!hist_file || !*hist_file)
				&& (home = GetVarValue (M, "HOME")) != NULL)
			{
				char *tmp;
				
				tmp = mmalloc (&M->alloc, 
					strlen (home) + MAX_FILENAME_LEN + 2);
				if (!tmp)
				{
					eprintf (M, "%s: %s\n", argv[0], StrError (ENSMEM));
					return 1;
				}
				
				strcpy (tmp, home);
				AddFileName (tmp, DEFAULT_HISTFILE);
				hist_file = home = tmp;
			}
			
			if (!hist_file || !*hist_file)
				hist_file = DEFAULT_HISTFILE;
		}
		
		realname = NormalizePath (&M->alloc, hist_file, 
			M->current_drive, M->current_dir);

		if (realname)
		{
			if (write_history)
			{
				retcode = writeHistory (M, realname);
				if (retcode != E_OK)
					eprintf (M, "%s: `%s' -- %s\n", argv[0],
						hist_file, StrError (retcode));
			}
			else
			{
				retcode = readHistory (M, realname);
				if (retcode != E_OK)
					eprintf (M, "%s: `%s' -- %s\n", argv[0],
						hist_file, StrError (retcode));
			}
			
			mfree (&M->alloc, realname);
		}
		else
		{
			eprintf (M, "%s: %s\n", argv[0], StrError (ENSMEM));
			retcode = 1;
		}

		if (home) mfree (&M->alloc, home);
		
		return (int) retcode;
	}
	else if (print_history)
	{
		/* Ansonsten hatten wir keinen Parameter und zeigen
		   den Inhalt der History an. */

		if (G.optind < argc)
			i = atoi (argv[G.optind]);
		else
			i = M->history.size - 1;

		if (i >= M->history.size)
			i = M->history.size - 1;
			
		display_history (M, (unsigned long) i << 16L, 1L << 16, 1, 1);
	}
	
	return 0;
}

/* Parse einen History-Wert und liefere den *Index* zurÅck */

static long
parse_arg (MGLOBAL *M, const char *str)
{
	char *endp = NULL;
	long val;
	unsigned int index;
	
	val = strtol (str, &endp, 10);

	if (*endp == '\0')	/* alles ok */
	{
		if (val < 0)
		{
			val = -val;
			
			if (val < M->history.size) return val;
			
			return -1L;
		}
		else
		{
			for (index = 0; index < M->history.size; index++)
				if (M->history.lines[index].str &&
					M->history.lines[index].first == val)
					return index;
		}

		return -1;
	}
	else				/* sonst ist es ein Prefix */
	{
		for (index = 0; index < M->history.size; index++)
		{
			const char *l = M->history.lines[index].str;
			
			if (l && l == strstr (l, str)) return index;
		}
		
		return -1;
	}
}


/* Liefere index/subindex fÅr n-ten Historyeintrag */

static unsigned long
get_index (MGLOBAL *M, unsigned int what)
{
	unsigned int i;
	unsigned long cnt = 0;

	HISTLINE *h = M->history.lines;
	
	for (i = 0; i < M->history.size; i++)
	{
		long entries = h[i].last - h[i].first + 1;
		
		if (cnt + entries >= what)
		{
			unsigned long res = (unsigned long)i << 16;
			res |= (what - cnt);
			
			return res;
		}
		else
		{
			cnt += entries;
		}
	}
	
	return 0xffffffffL;
}


int
m_fc (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	int c, opt_index;
	int do_list = 0, do_edit = 1, do_execute = 1;
	int with_numbers = 1, reverse_order = 0;
	unsigned long first, last;

	static struct option long_options[] =
	{
		{ NULL, 0, 0, 0},
	};
	
	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_fc), NlsStr(HLP_fc));
	optinit (&G);
	
	while ((c = getopt_long (M, &G, argc, argv, "e:lnrs", 
		long_options, &opt_index)) != EOF)
	{
		if (!c)			c = long_options[G.option_index].val;

		switch (c)
		{
			case 0:
				break;
				
			case 'l':
				do_list = 1;
				do_edit = do_execute = 0;
				break;
				
			case 'n':
				with_numbers = 0;
				break;
				
			case 'r':
				reverse_order = 1;
				break;

			case 's':
				do_edit = do_list = 0;
				do_execute = 1;

			default:
				return PrintUsage (M, argv[0], NlsStr(HLP_fc));
		}
	}

	if (do_list)
	{
		first = get_index (M, 16);
		last = get_index (M, 1);
		
		if (G.optind < argc)
		{
			first = parse_arg (M, argv[G.optind++]);
			if (first == 0xffffffffL) first = M->history.size - 1;
			last = first;
		}

		if (G.optind < argc)
			last = parse_arg (M, argv[G.optind++]);
			
		if (last == -1) last = 0;
		if (last >= M->history.size) last = M->history.size - 1;
		
		if (G.optind < argc)
			return PrintUsage (M, argv[0], NlsStr(HLP_fc));
			
		if (reverse_order)
		{
			long tmp = first; first = last; last = tmp;
		}
		
		display_history (M, first, last, with_numbers, 0);
	}

	return 0;
}
