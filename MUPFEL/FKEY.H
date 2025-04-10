/*
 * @(#) Mupfel\fkey.h
 * @(#) Stefan Eissing & Gereon Steffens, 08. Juni 1991
 *
 * - definitions for fkey.c
 */

#ifndef __M_FKEY__
#define __M_FKEY__

/* get function key string
 */
const char *GetFKey (MGLOBAL *M, int key_no);

/* convert scancode to index
 */
int ConvKey (int key_no);

int m_fkey (MGLOBAL *M, int argc, char **argv);

#endif