/*
 * @(#) Common\Argvinfo.h
 * @(#) Stefan Eissing, 17. Januar 1993
 */

#ifndef ARGVINFO__
#define ARGVINFO__

typedef struct
{
	ALLOCINFO *alloc;
	const char **argv;
	int argc;
	int array_size;

	struct
	{
		unsigned no_doubles : 1;
	} flags;
	
} ARGVINFO;


#endif /* !ARGVINFO__ */
