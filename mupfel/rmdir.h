/*
 * @(#) Mupfel\rmdir.h
 * @(#) Stefan Eissing & Gereon Steffens, 14. Mai 1991
 *
 * - definitions for rmdir.c
 */

#ifndef __M_RMDIR__
#define __M_RMDIR__
 
int rmdir (MGLOBAL *M, char *dir, const char *orig_name, int verbose,
	int *broken);

int m_rmdir (MGLOBAL *M, int argc, char **argv);

#endif /* __M_RMDIR__ */