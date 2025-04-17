/*
 * @(#) Gemini\keylogi.h
 * @(#) Stefan Eissing, 06. M„rz 1995
 *
 * description: Header File for keylogi.c
 */


#ifndef G_KEYLOGIC__
#define G_KEYLOGIC__

extern const char **FunctionKeys;

word doKeys (word kreturn, word kstate, int *mupfel_key, int *silent);

int ResolutionSwitchRequest (void);
int SwitchResolution (void);

#endif /* !G_KEYLOGIC */