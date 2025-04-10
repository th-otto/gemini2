/*
 * @(#) Mupfel\shrink.c
 * @(#) Gereon Steffens & Stefan Eissing, 20. April 1992
 *
 * das Kommando shrink
 */


#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "mglob.h"

#include "chario.h"
#include "getopt.h"
#include "shrink.h"


/* internal texts
 */
#define NlsLocalSection "M.shrink"
enum NlsLocalText{
OPT_shrink,	/*[-b val][-t val][-f][-i]*/
HLP_shrink,	/*Speicher verkleinern/freigeben*/
SK_MFREE,	/*%s: Fehler bei Mfree()\n*/
SK_CNTALLOC,	/*%s: kann %lu Bytes nicht allozieren\n*/
SK_NOSHRINK,	/*%s: kann nicht um mehr verkleinern als frei ist!\n*/
SK_BUYRAM,	/*%s: kaufen Sie eine RAM-Erweiterung!\n*/
SK_NOTSHRNK,	/*Speicher ist nicht verkleinert*/
SK_SHRINKBY,	/*Speicher ist um %lu Bytes (%luK) verkleinert*/
SK_FREE,		/*, gr”žter freier Block: %lu Bytes (%luK)\n*/
SK_ILLSIZE,	/*%s: unerlaubte Gr”ženangabe\n*/
SK_BADARG,	/*%s: Argument nicht numerisch oder Null\n*/
SK_ARGUSE,	/*%s: nur eins von -b, -t, -f oder -i erlaubt\n*/
};

typedef struct
{
	MGLOBAL *M;
	
	long free_mem;
	const char *cmd;
} SHRINKINFO;

void InitShrink (MGLOBAL *M)
{
	M->shrink_address = NULL;
	M->shrink_size = 0L;
}

void ExitShrink (MGLOBAL *M)
{
	if (M->shrink_address != NULL)
	{
		Mfree (M->shrink_address);
		InitShrink (M);
	}
}

static int doMfree (SHRINKINFO *S, void *addr)
{
	if (Mfree (addr) != 0)
	{
		eprintf (S->M, NlsStr(SK_MFREE), S->cmd);
		return 1;
	}
	return 0;
}

/*
 * static int doshrink(size_t by)
 * Shrink memory by the amount "by". If memory is already shrinked,
 * add "by" to the current shrink value.
 */
static int doshrink (SHRINKINFO *S, size_t by)
{
	int retcode = 0;
	
	/* Already shrinked?
	 */
	if (S->M->shrink_address != NULL)
	{
		/* Free memory
		 */
		doMfree (S, S->M->shrink_address);
		/* new size
		 */
		S->M->shrink_size += by;
	}
	else
		/* first call
		 */
		S->M->shrink_size = by;

	/* Shrink memory
	 */
	S->M->shrink_address = Malloc (S->M->shrink_size);
	
	if (S->M->shrink_address == NULL)
	{
		eprintf (S->M, NlsStr(SK_CNTALLOC), S->cmd, S->M->shrink_size);
		S->M->shrink_size = 0L;
		retcode = 1;
	}
	
	return retcode;
}

static int doshrinkby (SHRINKINFO *S, size_t val)
{
	int retcode;

	/* Value out of range?
	 */
	if (val >= S->free_mem)
	{
		eprintf (S->M, NlsStr(SK_NOSHRINK), S->cmd);
		retcode = 1;
	}
	else
		retcode = doshrink (S, val);
		
	return retcode;
}

static int doshrinkto (SHRINKINFO *S, size_t val)
{
	/* Already shrinked?
	 */
	if (S->M->shrink_address != NULL)
	{
		/* Free memory
		 */
		doMfree (S, S->M->shrink_address);
		S->M->shrink_address = NULL;
		S->M->shrink_size = 0L;
	}

	/* Calculate total free memory
	 */
	S->free_mem = (size_t)Malloc (-1L);
	if (val >= S->free_mem)
	{
		/* He wants to keep free more than he's got, the fool
		 */
		eprintf (S->M, NlsStr(SK_BUYRAM), S->cmd);
		return 1;
	}
	/* Shrink so that val bytes keep useable
	 */
	return doshrink (S, S->free_mem - val);
}

static int doshrinkfree (SHRINKINFO *S)
{
	int retcode = 0;
	
	/* Did he shrink?
	 */
	if (S->M->shrink_address == NULL)
	{
		mprintf (S->M, NlsStr(SK_NOTSHRNK));
		crlf (S->M);
	}
	else
	{
		/* Free memory
		 */
		retcode = doMfree (S, S->M->shrink_address);
		S->M->shrink_address = NULL;
		S->M->shrink_size = 0L;
	}
	
	return retcode;
}

static int doshrinkinfo (SHRINKINFO *S)
{
	if (S->M->shrink_address == NULL)
		mprintf (S->M, NlsStr(SK_NOTSHRNK));
	else
		mprintf (S->M, NlsStr(SK_SHRINKBY),	S->M->shrink_size, 
			S->M->shrink_size / 1024L);
		
	mprintf (S->M, NlsStr(SK_FREE),	S->free_mem, S->free_mem / 1024L);
	return 0;
}

/*
 * Returns the numeric value of arg. arg may be octal (leading 0),
 * decimal or hexadecimal (leading 0x or 0X), and it may be trailed
 * by "m" or "k" to indicate K-Byte (1024) or M-Byte (1024*1024)
 * units.
 */
static size_t memval (SHRINKINFO *S, char *arg)
{
	size_t val, mult = 1L;
	char factor, *endptr;
	
	factor = toupper (arg[strlen (arg) - 1]);
	
	if (!isdigit (factor))
	{
		/* Check for valid size indicator
		 */
		switch (factor)
		{
			case 'K':
				mult = 1024L;
				break;

			case 'M':
				mult = 1048576L;	/* 1024*1024 */
				break;

			default:
				eprintf (S->M, NlsStr(SK_ILLSIZE), S->cmd);
				return 0;
		}
	}
	
	/* Was suffixed by "m" or "k", delete that
	 */
	if (mult != 1L)
		arg[strlen(arg)-1] = '\0';
		
	val = strtoul (arg, &endptr, 0);
	if (val == 0L || *endptr != '\0')
	{
		/* It was 0 or something wasn't convertible
		 */
		eprintf (S->M, NlsStr(SK_BADARG), S->cmd);
		val = 0L;
	}
	
	return val * mult;
}

int m_shrink (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	SHRINKINFO S;
	int c;
	int shrinkto, shrinkby;
	int shrinkfree, shrinkinfo;
	size_t shrinkmem;
	char *shrinkarg;
	static struct option long_option[] =
	{
		{ "by", TRUE, NULL, 'b' },
		{ "to", TRUE, NULL, 't' },
		{ "free", FALSE, NULL, 'f' },
		{ "info", FALSE, NULL, 'i' },
		{ NULL,0,0,0 },
	};
	int opt_index = 0;

	shrinkto = shrinkby = shrinkfree = shrinkinfo = FALSE;	

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_shrink), 
			NlsStr(HLP_shrink));
	else if (argc==1)
		shrinkinfo = TRUE;
		
	optinit (&G);
	
	while ((c = getopt_long (M, &G, argc, argv, "b:t:fi", long_option,
		&opt_index)) != EOF)
	{
		if (!c)			c = long_option[G.option_index].val;
		switch (c)
		{
			case 'b':
				shrinkby = TRUE;
				shrinkarg = G.optarg;
				break;

			case 't':
				shrinkto = TRUE;
				shrinkarg = G.optarg;
				break;

			case 'f':
				shrinkfree = TRUE;
				break;

			case 'i':
				shrinkinfo = TRUE;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_shrink));
		}
	}
	
	if (shrinkby + shrinkto + shrinkfree + shrinkinfo > 1)
	{
		eprintf (M, NlsStr(SK_ARGUSE), argv[0]);
		return 1;
	}

	S.M = M;
	S.cmd = argv[0];
	S.free_mem = (size_t) Malloc (-1L);
	
	if (shrinkto || shrinkby)
	{
		shrinkmem = memval (&S, shrinkarg);
		
		if (shrinkmem == 0L)
			return 1;
			
		if (shrinkby)
			return doshrinkby (&S, shrinkmem);
		else
			return doshrinkto (&S, shrinkmem);
	}
	else
	{
		if (shrinkfree)
			return doshrinkfree (&S);
		else
			return doshrinkinfo (&S);
	}
}
