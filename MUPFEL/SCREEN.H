/*
 * @(#) Mupfel\screen.h
 * @(#) Stefan Eissing, 17. M„rz 1993
 *
 * -  set screen for external command
 */


#ifndef M_SCREEN__
#define M_SCREEN__

int SetScreen (MGLOBAL *M, SCREENINFO *should, 
	const char *full_name, int show_full_path);

int UnsetScreen (MGLOBAL *M, SCREENINFO *should_have_been);

int ScreenWindowsAreOpen (void);

#endif	/* !M_SCREEN__ */