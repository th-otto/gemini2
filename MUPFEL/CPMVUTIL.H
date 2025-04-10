/*
 * @(#) Mupfel\cpmvutil.h 
 * @(#) Stefan Eissing, 16. September 1991
 *
 * jr 30.12.96
 */

#ifndef M_CPMVUTIL__
#define M_CPMVUTIL__

/* Wir versuchen mindestens diesen Speicherplatz beim Kopieren
 * freizuhalten
 */
#define MIN_KEEPFREE	32767L


void ShowCopyMove (COPYINFO *C, char *from, char *to);
int CopyDate (const char *from, const char *to);

#define GET_ATTRIBUTE	0
#define SET_ATTRIBUTE	1

void CopyAttribute (const char *src, const char *dest);

int CheckDiskFull (char drive);

char ConfirmCopyAction (COPYINFO *C, char *file);
char AskCopyMove (COPYINFO *C, char *from, char *to);

int BasicMupfelCopy (COPYINFO *C, int inhnd, int outhnd, char *to, 
	char *from);

int EndSecureMupfelCopy (COPYINFO *C, char *origto, char *old_origto,
	char *realto, char *old_realto);


#endif /* M_CPMVUTIL__ */