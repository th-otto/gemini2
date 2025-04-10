/*
 * @(#) Gemini\files.h
 * @(#) Stefan Eissing, 01. M„rz 1993
 *
 * description: Header File for files.c
 */


#ifndef G_files__
#define G_files__

void FreeFileTree (FileWindow_t *wp);
int MakeFileTree (FileWindow_t *fwp, GRECT *work, int update_type);

#endif	/* !G_files__ */