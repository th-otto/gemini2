/*
 * @(#)Language/Nlsutil.c
 * @(#)Stefan Eissing, 23. Okrober 1994
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "nlsdef.h"
#define NLS_DEFAULT
#include "nls.h"
#include "nlsfix.h"

#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

/*
 * Interne, lokale Variablen
 */
/* LangInit schon erfolgreich aufgerufen? */
static int Initialisiert = FALSE;

/* Pointer fÅr malloc/free-Funktionen */
static void *(*NlsMalloc)(long);
static void (*NlsFree)(void *ptr);

/* Pointer auf den Inhalt der geladenen Datei */
static char *TextBuffer;

/* Zeiger auf die erste Section */
static BINSECTION *FirstSection;

int NlsInit(const char *TextFileName, 
			void *defaultSection,
			void *MallocFunction,
			void *FreeFunction)
{
	int fhandle;
	long fsize, stat;
	
	if (Initialisiert)
		return 0;

	Initialisiert = TRUE;
	
	NlsMalloc = (void *(*)(long))MallocFunction;
	NlsFree =  (void(*)(void *))FreeFunction;
	FirstSection = defaultSection;

	if (!TextFileName)
		return !!FirstSection;
	stat = Fopen(TextFileName, 0);
	if (stat < 0)
	{
		return !!FirstSection;
	}
	fhandle = (int)stat;
	fsize = Fseek(0L, fhandle, 2);
	Fseek(0L, fhandle, 0);
	
	TextBuffer = NlsMalloc(fsize);
	if (!TextBuffer)
	{
		Fclose(fhandle);
		return 0;
	}
	
	if (Fread(fhandle, fsize, TextBuffer) != fsize)
	{
		NlsFree(TextBuffer);
		Fclose(fhandle);
		return 0;
	}
	Fclose(fhandle);
	
	FirstSection = NlsFix(TextBuffer);
	if (FirstSection == NULL)
	{
		NlsFree(TextBuffer);
		FirstSection = defaultSection;
		return !!FirstSection;
	}
	
	return 1;
}

void NlsExit(void)
{
	if (!Initialisiert)
		return;
	
	NlsFree(TextBuffer);
	Initialisiert = FALSE;
}

const char *NlsGetStr(const char *Section, int Number)
{
	BINSECTION *sec, *nextsec;
	int found = 0;
	
	if (!Initialisiert)
		return "String missing!";
		
	nextsec = FirstSection;
	while (!found && nextsec)
	{
		sec = nextsec;
		found = !strcmp(Section, sec->SectionTitel);
		nextsec = sec->NextSection;
	}
	
	if (found)
	{
		if (sec->StringCount > Number)
		{
			char *str = sec->SectionStrings;
			
			/* öberspringe Number Strings */
			for (; Number; --Number)
			{
				while (*str)
					++str;
				++str;
			}
			return str;
		}
	}
	return "String missing!";
}
