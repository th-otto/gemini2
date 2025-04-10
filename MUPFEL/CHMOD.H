/*
 * @(#) Mupfel\Chmod.h 
 * @(#) Stefan Eissing, 14. M„rz 1994
 *
 * jr 27.5.1996
 */

#ifndef __M_CHMOD__
#define __M_CHMOD__

#define CMD_RUNOPTS		0

#if CMD_RUNOPTS
int m_runopts (MGLOBAL *M, int argc, char **argv);
#endif


#define CHMOD_INTERNAL	0
#ifdef CHMOD_INTERNAL
int m_chmod (MGLOBAL *M, int argc, char **argv);
#endif

long SetWriteMode (const char *filename, int mode);

#endif /* __M_CHMOD__ */