/*
 * @(#) MParse\Expand.h
 * @(#) Stefan Eissing, 17. Januar 1993
 */



#ifndef __Expand__
#define __Expand__

#include "..\common\argvinfo.h"

/* Expandiere, wenn erforderlich und erlaubt <subst_string> und
 * trage es in <A> ein.
 * Gibt bei zuwenig Speicher FALSE zurÅck.
 */
int ExpandAndStore (MGLOBAL *M, ARGVINFO *A, char *subst_string,
		const char *subst_quoted);

#endif /* __Expand__ */
