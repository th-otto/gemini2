/*
 * @(#) Gemini\windstak.h
 * @(#) Stefan Eissing, 14. Februar 1993
 *
 * description: Header File for windstak.c
 */

#ifndef G_windstak
#define G_windstak

#include "..\common\memfile.h"

int PushWindBox (const GRECT *box, int kind);
int PopWindBox (GRECT *box, int kind);

int WriteBoxes (MEMFILEINFO *mp, char *buffer);
void FreeWBoxes (void);

#endif	/* !G_windstak */