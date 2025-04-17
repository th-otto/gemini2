/*
 * @(#) Gemini\sortfile.h
 * @(#) Stefan Eissing, 01. November 1994
 *
 * description: Header File for sortfile.c
 */


#ifndef G_sortfile__
#define G_sortfile__

typedef enum { 
	SortNone, SortDate, SortName, SortType, SortSize
} SortMode;

void SortFileWindow (FileWindow_t *fwp, SortMode mode);

SortMode MenuEntryToSortMode (word menuEntry);

#endif	/* !G_sortfile__ */