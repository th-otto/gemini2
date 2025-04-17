/*
 * @(#) Gemini\scroll.h
 * @(#) Stefan Eissing, 29, Januar 1995
 *
 * description: Header File for scroll.c
 */


#ifndef G_SCROLL__
#define G_SCROLL__

int FileWindPageUp (WindInfo *wp);
int FileWindPageDown (WindInfo *wp);
int FileWindLineUp (WindInfo *wp);
int FileWindLineDown (WindInfo *wp);
int FileWindowScroll (WindInfo *wp, word div_skip, word redraw);

void DoArrowed (word whandle, word desire);

void DoVerticalSlider (word whandle, word position);

int FileWindowCharScroll (WindInfo *wp, char c, word kstate);


word CalcYSkip (FileWindow_t *fwp, GRECT *work, word toskip);

#endif	/* !G_SCROLL__ */