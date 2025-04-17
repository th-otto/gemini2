/*
 * @(#) Common\xbra.h 
 * @(#) Stefan Eissing, 17. September 1991
 *
 */

#ifndef M_XBRA__
#define M_XBRA__


typedef struct
{
	long magic;
	long id;
	long old_vec;
	void *routine;
} XBRA;


void InstallXBRA (XBRA *x, long *vector);
void RemoveXBRA (XBRA *x, long *vector);



#endif