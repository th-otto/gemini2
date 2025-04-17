/*
 * @(#) Gemini\erase.h
 * @(#) Stefan Eissing, 13. November 1994
 *
 * description: Header File for erase.c
 */


#ifndef G_erase
#define G_erase

#include "..\common\argvinfo.h"

void ObjectWasErased (char *name, int redraw);

word EraseArgv (ARGVINFO *A);
word DeleteDraggedObjects (WindInfo *wp);

void emptyPaperbasket (void);

#endif	/* !G_erase	*/