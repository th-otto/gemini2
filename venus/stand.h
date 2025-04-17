/*
 * @(#) Gemini\stand.h
 * @(#) Stefan Eissing, 12. Januar 1995
 *
 *  -  standalone / merged version definitions
 */

#ifndef __stand__
#define __stand__

#define MERGED		1
#define STANDALONE	(!MERGED)


#if MERGED

#define PGMNAME		"GEMINI"
#define pgmname		"gemini"

#else
#define PGMNAME		"VENUS"
#define pgmname		"venus"
#endif

#define TMPINFONAME		pgmname ".tmp"
#define FINFONAME		pgmname ".inf"
#define MAINRSC			pgmname ".rsc"
#define ICONRSC			pgmname "ic.rsc"	
#define AUTOEXEC		pgmname ".mup"
#define DEFAULTDEVICE	pgmname " drive"

#define RUNNER			"runner"

#define MSGFILE			pgmname ".msg"

#endif