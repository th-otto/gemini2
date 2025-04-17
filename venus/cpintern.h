/*
 * @(#) Gemini\cpintern.h
 * @(#) Stefan Eissing, 09. April 1993
 *
 * description: basic functions to copy/move files
 *
 */


#ifndef G_cpintern__
#define G_cpintern__

#include "..\common\argvinfo.h"

void DispName (OBJECT *tree, word obj, const char *name);
void DisplayFileName (const char *src_name, const char *dest_name);

void FileWasCopied (long size);

void CheckUserBreak (void);
void AskForAbort (int ask_for_abort, const char *filename);

int CheckValidTargetFile (char *src_name, char *dest_name, 
	int do_rename);

void ArgvCheckNames (ARGVINFO *A);
int CountArgvFolderAndFiles (ARGVINFO *A, int follow_links,
	long max_objects, uword *foldnr, uword *filenr, long *size);

#endif	/* !G_cpintern__ */