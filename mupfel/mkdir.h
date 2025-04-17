/*
 * @(#) Mupfel\mkdir.h
 * @(#) Stefan Eissing & Gereon Steffens, 09. Mai 1991
 *
 *  -  definitions for mkdir.c
 */
 
#ifndef __M_MKDIR__
#define __M_MKDIR__

int mkdir (MGLOBAL *M, char *dir, const char *orig_name, int with_path);
int m_mkdir (MGLOBAL *M, int argc, char **argv);

#endif /* __M_MKDIR__ */