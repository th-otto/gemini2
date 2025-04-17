/*
 * @(#) MParse\Mdef.h
 * @(#) Stefan Eissing, 29. Januar 1993
 *
 */

#ifndef __MupfelDef__
#define __MupfelDef__

#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

#ifndef DIM
#define DIM(x)		(sizeof (x) / sizeof (x[0]))
#endif

#ifndef min
#define min(a,b)	((a) < (b) ? (a) : (b))
#define max(a,b)	((a) > (b) ? (a) : (b))
#endif

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;

#define ILLHND (-99)	/* illegal handle */
#define FORHND	(-4)
#define FIVEHND	(-5)
#define PRNHND (-3)		/* handle for PRN: */
#define AUXHND (-2)		/* handle for AUX: */
#define CONHND (-1)		/* handle for CON: */
#define MINHND (-6)		/* smallest legal handle */

/* Maximale LÑnge von Pfaden, wie wir sie vom OS bekommen.
 */
#define MAX_PATH_LEN		256

/* Die zu erwartende LÑnge fÅr volle Pfade, die i.a. nicht
 * Åberschritten wird.
 */
#define AVERAGE_PATH_LEN	128

/* Maximale LÑnge eines Dateinamens + 1 */
#define MAX_FILENAME_LEN	128


/* Maximale Rekursionstiefe fÅr Funktionen
 */
#define MAX_FUNCTION_COUNT	50

/* POSIX Returncodes
 */
#define ERROR_COMMAND_NOT_FOUND		127
#define ERROR_NOT_EXECUTABLE		126

#endif /* __MupfelDef__ */

