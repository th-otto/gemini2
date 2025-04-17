/*
 * @(#) Gemini\gemini.h
 * @(#) Stefan Eissing, 10. April 1994
 *
 * description: functions to manage accessorie windows
 */


#ifndef G_gemini__
#define G_gemini__

#include <aes.h>

extern OBJECT EmptyBackground;

int EarlyInitGemini (void *mupfelInfo, void *allocInfo, int v_handle, 
	unsigned long *dirty, const char *function_keys[20]);
int LateInitGemini (void);

void ExitGemini (int mupfel_doing_overlay);

int GetCharFromGemini (int *key_code, int *reinit_line);

void PrepareExternalStart (void *actualInfo, int starting);
void PrepareInternalStart (int *wasOpened, int *oldTopWindow, 
		int starting);
void SetSystemFont (void);

void SetMenuBar (int install);

int StartProgramInWindow (const char *name, const COMMAND *args, 
	const char *dir, const char *env);

int StartAccFromMupfel (char *acc_name, ARGVINFO *A, char *command);

#endif	/* !G_gemini__ */