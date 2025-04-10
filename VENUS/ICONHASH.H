/*
 * @(#) Gemini\iconhash.h
 * @(#) Stefan Eissing, 27. Dezember 1993
 *
 * description: Header File for iconhash.c
 */

#ifndef G_ICONHASH__
#define G_ICONHASH__

#include "iconrule.h"

void BuiltIconHash (void);

DisplayRule *GetHashedRule (word isFolder, const char *name,
				int case_sensitiv);

void ClearHashTable (void);

#endif	/* !G_ICONHASH__ */