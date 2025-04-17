/*
	@(#)Nls/nls2c.c
	@(#)Stefan Eissing, 23. Oktober 1994
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <tos.h>

#include "getopt.h"
#include "stderr.h"

#include "nlsdef.h"
#include "nlsfix.h"

#ifndef TRUE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

char *sccsid (void)
{
	return "@(#)Nls/nlsdcomp.ttp, Copyright (c) Stefan Eissing, "__DATE__;
}

/* Name des Programms */
char *progname;

/* Name des Eingabefiles */
char *infilename;

/* Flag fÅr die GeschwÑtzigkeit */
static int verbose = 0;

/* Zeiger auf das geladene File */
char *TextBuffer;

/* Sektion, die gerade bearbeitet wird */
static BINSECTION *Section;

void usage (void)
{
	fprintf (stderr, "usage: %s [-v] infile outfile\n", progname);
	exit (2);
}

static int loadFile(void)
{
	int fhandle;
	long fsize;
	
	fhandle = (int)Fopen(infilename, 0);
	if (fhandle < 0)
	{
		fprintf(stderr, "%s: cannot open %s!\n", progname,
				infilename);
		return FALSE;
	}
	
	fsize = Fseek(0L, fhandle, 2);
	Fseek(0L, fhandle, 0);
	
	TextBuffer = Malloc(fsize);
	if (!TextBuffer)
	{
		printf("%s: not enough memory\n", progname);
		Fclose(fhandle);
		return FALSE;
	}
	
	if (Fread(fhandle, fsize, TextBuffer) != fsize)
	{
		printf("%s: error reading %s\n", progname, infilename);
		Mfree(TextBuffer);
		Fclose(fhandle);
		return FALSE;
	}
	Fclose(fhandle);
	
	return TRUE;
}

static void putString(unsigned char *str)
{
	unsigned char *cp, c;
	int done = FALSE;
	
	cp = str;
	
	while (*str && !done)
	{
		/* Hack fÅr ASCII-ZeichensÑtze */
		while ((*cp >= ' ') && (!strchr("\\\'\"\?", *cp)))
			++cp;
	
		c = *cp;
		*cp = '\0';
		printf("%s", str);
		*cp = c;
		str = ++cp;
		
		if (c != '\0')
		{
			switch (c)
			{
				case '\\':
				case '\'':
				case '\"':
				case '\?':
					printf("\\%c", c);
					break;
					
				case '\a':
					printf("\\a");
					break;
					
				case '\b':
					printf("\\b");
					break;
					
				case '\f':
					printf("\\f");
					break;
					
				case '\n':
					printf("\\x0A");
					break;
					
				case '\r':
					if (*cp == '\n')
					{
						printf("\\n");
						str = ++cp;
					}
					else
						printf("\\r");
					break;
					
				case '\t':
					printf("\\t");
					break;
					
				case '\v':
					printf("\\v");
					break;
					
				default:
					printf("\\x%2x", (int)((unsigned char)c));
					break;
			}
		}
		else
			done = TRUE;
	}
}

static void writeStrings(BINSECTION *sec)
{
	char *str = sec->SectionStrings;
	long count;
	
	for (count = 0L; count < sec->StringCount; ++count)
	{
		size_t len = strlen(str);
		size_t cutter;
		char line[71];
		
		while (len > 70)
		{
			cutter = 70L;
			while (cutter && (!strchr(" \t", str[cutter])))
				--cutter;
			if (!cutter)
				cutter = 70;
				
			strncpy(line, str, cutter);
			line[cutter] = '\0';
			str = str+cutter;
			len = strlen(str);
			printf ("\n        \"");
			putString ((unsigned char *)line);
			printf ("\"");
		}
		printf ("\n        \"");
		putString ((unsigned char *)str);
		printf ("\\x00\"");

		while (*str)
			++str;
		++str;
	}
}


static void printHeader (void)
{
	time_t now = time (NULL);
	
	printf ( 
		"/* %s translation into C code\n"
		" * done by %s\n"
		" * at %s"
		" */\n\n",
		infilename, progname, ctime (&now));
	printf ("#include <stddef.h>\n\n");
	printf ("#define NLS_DEFAULT\n");
	printf ("#include <nls\\nls.h>\n\n");
	printf ("typedef struct sec\n");
	printf ("{\n");
	printf ("	char *SectionTitel;		/* Offset des Section-Strings im File */\n");
	printf ("	char *SectionStrings;	/* Offset des ersten Strings dieser\n");
	printf ("							 * Section im File die weiteren Strings\n");
	printf ("							 * liegen direkt dahinter */\n");
	printf ("	long StringCount;		/* Anzahl der Strings */\n");
	printf ("	struct sec *NextSection;/* Offset der nÑchsten Section im File\n");
	printf ("							 * 0L ist gleich Ende */\n");
	printf ("} BINSECTION;\n\n");
	printf ("static BINSECTION internalSections[] =\n{\n");
}


static void printTrailer (void)
{
	printf ("    { \"\", \"\\x00\\x00\", 0, NULL }\n};\n\n");
	printf ("BINSECTION *NlsDefaultSection = internalSections;\n\n");
}


static int decompile(void)
{
	long i;
	
	if (!loadFile())
		return 1;
	
	Section = NlsFix(TextBuffer);
	if (!Section)
		return 1;
	
	printHeader ();
	for (i = 0L; Section ; ++i, Section = Section->NextSection)
	{
		printf ("    { \"%s\", ", Section->SectionTitel);
		if (Section->StringCount)
			writeStrings(Section);
			
		printf (",\n    %ld, internalSections+%ld },\n",
			(long)(Section->StringCount), i+1);
	
	}
	printTrailer ();
	
	Mfree(TextBuffer);
	return 0;
}

int argvmain (int argc, char *argv[])
{
	int c, option_index, retcode;
	static int giveHelp = 0;
	static struct option long_options[]=
	{
		{"help", 0, &giveHelp, 1},
		{"verbose", 0, &verbose, 1},
		{ 0, 0, 0, 0}
	};
	patchstderr ();

	if ((!argv[0]) || (!argv[0][0])) argv[0] = "nlsdcomp";
	progname = argv[0];

	while ((c = getopt_long(argc, argv, "hv", long_options, 
				&option_index)) != EOF)
		switch (c)
		{
			case 0:
				break;
		
			case 'h':
				giveHelp = 1;
				break;
			
			case 'v':
				verbose = 1;
				break;
					
			default :
				usage();
		}

	if (giveHelp || (optind >= argc))
		usage();

	infilename = argv[optind];
	
	retcode = decompile();
	
	return retcode;
}

