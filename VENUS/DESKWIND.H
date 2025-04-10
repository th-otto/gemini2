/*
 * @(#) Gemini\deskwind.h
 * @(#) Stefan Eissing, 27. Februar 1993
 *
 * description: all the Initialisation for the Desktop
 */


#ifndef G_deskwind__
#define G_deskwind__

#include "..\common\memfile.h"


void DeskWindowOpen (void);
void DeskWindowClose (int make_background);

int SetDeskBackground (void);
void ReInstallBackground (int redraw);

void SetDeskState (char *line);

int WriteDeskState (MEMFILEINFO *mp, char *buffer);

#endif	/* !G_deskwind__ */