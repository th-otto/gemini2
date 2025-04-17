/*
 * @(#) Gemini\copymove.h
 * @(#) Stefan Eissing, 13. November 1994
 *
 * description: Header File for copymove.c
 */


#ifndef G_copymove__
#define G_copymove__

int CopyArgv (ARGVINFO *A, char *tpath, int move, int rename);

int CopyDraggedObjects (WindInfo *fwp, char *tpath, 
	int move, int rename);

#endif	/* !G_copymove */