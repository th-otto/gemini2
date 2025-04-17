/*
 * @(#) Gemini\venuserr.h
 * @(#) Stefan Eissing, 30. Dezember 1991
 *
 * description: Header File for venuserr.c
 *
 * jr 14.4.95
 */

#ifndef V_VENUSERR__
#define V_VENUSERR__

word venusErr (const char *errmessage,...);
word changeDisk (const char drive,const char *label);
word venusChoice (const char *message,...);
word venusInfo (const char *s,...);
word venusInfoFollow (const char *s,...);
word venusDebug (const char *s,...);

void ErrorAlert (const char *str);

#endif /* !V_VENUSERR__ */