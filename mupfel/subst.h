/*
 * @(#) MParse\Subst.h
 * @(#) Stefan Eissing, 28. Mai 1995
 *
 * jr 8.8.94
 */



#ifndef __Subst__
#define __Subst__

#include "partypes.h"

int SubstNoCommand (MGLOBAL *M, WORDINFO *name, 
	char **subst_string, char **subst_quote,
	int *slashesConverted);
/* Description:
 * Substitute the word in "name" in the context "M" and returned
 * the substituted string (and its quoting information). Additional
 * return if any / chars were converted to \
 */

int SubstWithCommand (MGLOBAL *M, WORDINFO *name, 
	char **subst_string, char **subst_quote);
const char *StringVarSubst (MGLOBAL *M, const char *string);

#endif /* __Subst__ */
