/*
 * @(#) Mupfel\getopt1.c
 * @(#) Stefan Eissing, 07. Juni 1992
 *
 * Frontends for long options
 */

#include "..\common\alloc.h"
#include "mglob.h"

#include "getopt.h"

int getopt_long (MGLOBAL *M, GETOPTINFO *G, int argc, 
	char **argv, const char *options,
	const struct option *long_options, int *opt_index)
{
	int val;
	
	G->_getopt_long_options = long_options;
	val = getopt (M, G, argc, argv, options);

	if (val == 0)
		*opt_index = G->option_index;

	return val;
}

/* Like getopt_long, but '-' as well as '+' can indicate a long option.
 * If an option that starts with '-' doesn't match a long option,
 * but does match a short option, it is parsed as a short option
 * instead.
 */
int getopt_long_only (MGLOBAL *M, GETOPTINFO *G, int argc, 
	char **argv, const char *options, 
	const struct option *long_options, int *opt_index)
{
	int val;
	
	G->_getopt_long_options = long_options;
	G->_getopt_long_only = 1;
	val = getopt (M, G, argc, argv, options);

	if (val == 0)
		*opt_index = G->option_index;

	return val;
}
     
