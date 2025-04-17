/*
 * @(#) Mupfel\lineedit.h
 * @(#) Stefan Eissing, 05. Juni 1991
 *
 */

#ifndef __M_LINEEDIT__
#define __M_LINEEDIT__

typedef size_t (PromptFunction) (MGLOBAL *, const char *);

char *ReadLine (MGLOBAL *M, int with_history, const char *prompt,
	PromptFunction *show_prompt, int *broken);

#endif	/* __M_LINEEDIT__ */