/*
 * @(#) Common\Cookie.h
 * @(#) Stefan Eissing, 01. November 1993
 *
 * jr 29.5.1996
 */


#ifndef C_ComCookie__
#define C_ComCookie__

/* Struktur des OFLS-Cookies
 */
struct ofls_cookie
{
	long product;
	unsigned short version;
	signed short drives[32];
	signed short reserved[32];
};
 
int LegalCookie (void *cj);

int CookiePresent (long cookie, long *value);

int MemoryIsReadable (void *mem);

int IDTSprintf (char *str, int year, int month, int day);

int DefaultLanguage (int def);

#endif
