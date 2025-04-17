/*
 * @(#) Gemini\icondrag.h
 * @(#) Stefan Eissing, 22. Juli 1991
 *
 * description: Header File for icondrag.c
 */

#ifndef G_icondrag__
#define G_icondrag__

word FindObject (OBJECT *tree, word x, word y);

void doIcons (WindInfo *wp, word mx, word my, word kstate);

#endif	/* !G_icondrag */