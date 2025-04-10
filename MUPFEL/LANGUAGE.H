/*
	@(#)mupfel/language.h
	
	Julian F. Reschke, 29. Mai 1996
	Support functions for locales
*/

#define DO_LOCALE_POSIX	0
#define DO_LOCALE_GERMAN 1
#define DO_LOCALE_FRENCH 2
#define DO_LOCALE_ENGLISH 3
#define DO_LOCALE_FED 4

int GetLocale (MGLOBAL *M, const char *type);
