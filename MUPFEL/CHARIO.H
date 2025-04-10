/* 
 * @(#) MParse\chario.h 
 * @(#) Stefan Eissing & Gereon Steffens, 04. Juli 1993
 */

#ifndef M_CHARIO__
#define M_CHARIO__

int intr (MGLOBAL *M);			/* check for interrupt during output */
int wasintr (MGLOBAL *M);		/* last cmd was interrupted */
void resetintr (MGLOBAL *M);	/* reset interrupt state */
int checkintr (MGLOBAL *M);		/* direct check for ctrl-C */
void clearintr (MGLOBAL *M);	/* clear all interrupt flags */

void rawout (MGLOBAL *M, char c);	/* raw character output */
void canout (MGLOBAL *M, char c);	/* cooked character output */
									/* print count chars (raw) */
void rawoutn (MGLOBAL *M, char c, unsigned int count);	

void crlf (MGLOBAL *M);				/* print CR/LF */
void beep (MGLOBAL *M);				/* ring bell */
void vt52 (MGLOBAL *M, char c);		/* vt52 esc sequence */
									/* vt52 cursor motion */
void vt52cm (MGLOBAL *M, int x, int y);

int domprint (MGLOBAL *M, const char *str, size_t len);
int doeprint (MGLOBAL *M, const char *str, size_t len);
int mprintf (MGLOBAL *M, const char *fmt,...);	/* printf via outchar */
int eprintf (MGLOBAL *M, const char *fmt,...);	/* printf to stderr */
void dprintf (const char *fmt,...);				/* debug mprint via rawcon */
int printArgv (MGLOBAL *M, int argc, const char **argv);

long ioerror (MGLOBAL *M, const char *cmd, const char *str, 
	long errcode);

int PrintHelp (MGLOBAL *M, const char *name, const char *options,
	const char *help);
int PrintUsage (MGLOBAL *M, const char *name, const char *usage);
void PrintStringArray (MGLOBAL *M, int argc, const char **argv);

int inbuffchar (MGLOBAL *M, long *lp);	/* get char from typeahead */
long inchar (MGLOBAL *M, int cooked);	/* get char and scancode */

char CharSelect (MGLOBAL *M, const char *allowed);

int InitCharIO (MGLOBAL *M);	/* Initialisieren und */
int ReInitCharIO (MGLOBAL *old, MGLOBAL *new);

/* Deinit. des globalen TTYINFO; parent may be NULL
 */
void ExitCharIO (MGLOBAL *M, MGLOBAL *parent);	


void InitStdErr (MGLOBAL *M);
void SetRowsAndColumns (MGLOBAL *M, int rows, int columns);
void InitRowsAndColumns (MGLOBAL *M);	/* Setze ROWS/COLUMNS */

#endif /* M_CHARIO__ */