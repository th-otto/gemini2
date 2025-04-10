/*
 * @(#) Mupfel\date.h 
 * @(#) Gereon Steffens & Stefan Eissing, 26. Juni 1991
 *
 * definitions for date.c
 *
 * 21.01.91: strdate() removed in favor of loctime.h (jr)
 */

#ifndef __M_DATE__
#define __M_DATE__

typedef union
{
	unsigned int d;
	struct
	{
		unsigned year  :7;
		unsigned month :4;
		unsigned day   :5;
	} s;
} dosdate;

typedef union
{
	unsigned int t;
	struct
	{
		unsigned hour :5;
		unsigned min  :6;
		unsigned sec  :5;
	} s;
} dostime;

typedef union
{
	unsigned long dt;
	struct
	{
	    unsigned year:  7;
	    unsigned month: 4;
	    unsigned day:   5;
	    unsigned hour:  5;
	    unsigned min:   6;
	    unsigned sec:   5;
	} s;
} xtime;

#define DATE_INTERNAL	0

#if DATE_INTERNAL
int m_date (MGLOBAL *M, int argc, char **argv); 
#endif

int m_touch (MGLOBAL *M, int argc, char **argv); 

#endif /* __M_DATE__ */