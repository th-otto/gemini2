/*
 * @(#) Mupfel\gemutil.h 
 * @(#) Stefan Eissing, 27. Februar 1993
 *
 */

#ifndef __M_GEMUTIL__
#define __M_GEMUTIL__

int MupfelWindUpdate (int mode);
int MupfelGrafMouse (int mode);

void WindNew (void);
void InstallGemBackground (MGLOBAL *M, const char *name, 
	int show_full, int draw_background);

#endif /* __M_GEMUTIL__ */