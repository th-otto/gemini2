/*
 * @(#) Mupfel\stand.h
 * @(#) Gereon Steffens & Stefan Eissing, 16. September 1991
 *
 *  -  standalone / merged version definitions
 */

#ifndef _M_STAND
#define _M_STAND

#ifndef MERGED
#define MERGED		0
#endif
#define GEMINI		MERGED
#define STANDALONE	(!MERGED)

#if MERGED
	#define PGMNAME	"gemini"
	#define pgmname	"gemini"
	#define TTP_VERSION		0
#else
	#define PGMNAME	"mupfel"
	#define pgmname	"mupfel"
	#define TTP_VERSION		1
#endif

#endif