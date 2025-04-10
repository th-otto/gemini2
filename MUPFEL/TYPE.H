/*
 * @(#) MParse\type.h
 * @(#) Stefan Eissing, 21. Juni 1991
 *
 * Internes Kommando "type"
 */


#ifndef __M_TYPE__
#define __M_TYPE__

/* jr: wird von m_command benutzt */

int ShowType (MGLOBAL *M, const char *cmdname, const char *what,
	int verbose);

/* Internes Kommando <type>, das ausgibt, wie etwas als Kommando
 * interpretiert wird.
 */
int m_type (MGLOBAL *M, int argc, char **argv);

#endif /* __M_TYPE__ */