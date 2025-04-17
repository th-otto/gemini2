/*
 * @(#) MParse\version.h
 * @(#) Stefan Eissing, 24. Juni 1991
 *
 * Versionnummer von Mupfel, TOS, GEMDOS, AES, etc.
 *
 * jr 26.5.1996
 */


#ifndef __M_VERSION__
#define __M_VERSION__


const char *GetCountryId (int index);
int m_version (MGLOBAL *M, int argc, char **argv);

#endif	/* __M_VERSION__ */