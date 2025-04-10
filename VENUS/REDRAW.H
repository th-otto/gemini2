/*
 * @(#) Gemini\redraw.h
 * @(#) Stefan Eissing, 16. August 1991
 *
 * description: Header File for redraw.c
 */

#ifndef G_redraw__
#define G_redraw__


void flushredraw (void);
void buffredraw (WindInfo *wp, const GRECT *rect);
void redrawObj (WindInfo *wp, word objnr);

void rewindow (WindInfo *wp, word upflag);
void allrewindow (word upflag);

void allFileChanged (void);

#endif	/* !G_redraw */