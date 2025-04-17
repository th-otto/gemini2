/*
 * @(#) Mupfel\free.c
 * @(#) Gereon Steffens & Stefan Eissing, 16. September 1991
 *
 *  -  internal "free" command
 *
 * jr 10.12.95
 */
 
#include <stddef.h>
#include <stdio.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "mglob.h"

#include "chario.h"
#include "free.h"
#include "getopt.h"

/* internal texts
 */
#define NlsLocalSection "M.free"
enum NlsLocalText{
OPT_free,	/*[-l]*/
HLP_free,	/*freien Speicher anzeigen*/
FR_FRAG,	/*%s: zu viele Speicherbl”cke\n*/
FR_HEADER,	/*Block     Adresse      Gr”že\n*/
FR_INFO,	/*%lu Bytes (%luK) in %d Bl”cken frei, gr”žter Block: %lu Bytes (%luK)\n*/
};

#define MEMBLKSIZE 20

static void showFree (MGLOBAL *M, int longlist)
{
	struct
	{
		void *addr;
		size_t size;
	} memblk[MEMBLKSIZE];
	int i, j;
	size_t s, freemem = 0, largest = 0;
	
	i = 0;
	while (i < MEMBLKSIZE && (s = (size_t)Malloc (-1L)) != 0)
	{
		memblk[i].addr = Malloc (s);
		if (memblk[i].addr == NULL)
			break;
		memblk[i].size = s;
		freemem += s;
		if (s > largest)
			largest = s;
		++i;
	}
	
	if (i == MEMBLKSIZE)
		eprintf (M, NlsStr(FR_FRAG), "free");
		
	for (j = 0; j < i; ++j)
		Mfree (memblk[j].addr);
		
	if (longlist && i)
	{
		mprintf (M, NlsStr(FR_HEADER));
		for (j = 0; j < i && !intr (M); ++j)
		{
			mprintf (M, "%5d  0x%08p %10lu\n",
				j + 1, memblk[j].addr, memblk[j].size);
		}
	}
	
	mprintf (M, NlsStr(FR_INFO), freemem, freemem / 1024L, i,
		largest, largest / 1024L);
}

int m_free (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	int c;
	int longlist;
	struct option long_options[] =
	{
		{ "long", FALSE, NULL, 'l' },
		{ NULL,0,0,0 },
	};
	int opt_index;

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_free), 
			NlsStr(HLP_free));
	
	longlist = FALSE;
	optinit (&G);

	while ((c = getopt_long (M, &G, argc, argv, "l", long_options,
		&opt_index)) != EOF)
	{
		if (!c)
			c = long_options[opt_index].val;
			
		switch (c)
		{
			case 0:
				break;

			case 'l':
				longlist = TRUE;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_free));
		}
	}
		
	showFree (M, longlist);
	return 0;
}