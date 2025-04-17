/*
 * @(#) Gemini\filenew.h
 * @(#) Stefan Eissing, 13. November 1994
 *
 */

#ifndef G_filenew__
#define G_filenew__

#include "..\common\argvinfo.h"

int fileWindowMakeNewObject (WindInfo *wp);
int fileWindowMakeAlias (WindInfo *wp);
int fileWindowGetDragged (WindInfo *wp, ARGVINFO *A,
	int preferLowerCase);
int fileWindowGetSelected (WindInfo *wp, ARGVINFO *A,
	int preferLowerCase);

int  GetFiles (FileWindow_t *wp);
void FreeFiles (FileWindow_t *fwp);

#endif	/* !G_filenew__ */