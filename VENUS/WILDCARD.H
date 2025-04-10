/*
 * @(#) Gemini\wildcard.h
 * @(#) Stefan Eissing, 28. Februar 1993
 *
 * description: Header File for wildcard.c
 */

#ifndef G_wildcard__
#define G_wildcard__

#include "..\common\memfile.h"


int WriteWildPattern (MEMFILEINFO *mp, char *buffer);

void ExecPatternInfo (char *line);

void doWildcard (WindInfo *wp);

int FilterFile (const char* wcard, const char *fname, 
	int case_sensitiv);

void FileWindowSpecials (WindInfo *wp, int mx, int my);

#endif /* !G_wildcard__ */