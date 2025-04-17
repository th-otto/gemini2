/*
 * @(#) Gemini\wintool.h
 * @(#) Stefan Eissing, 31. Januar 1992
 *
 * description: functions for windows
 */


#ifndef G_wintool__
#define G_wintool__

void WT_Clip (word handle1, word handle2, word pxy[4]);

void WT_BuildRectList (word whandle);
void WT_ClearRectList (int whandle);

char WT_GetRect (word index, word pxy[4]);

#endif