/*
 * @(#)Gemini\clipbrd.h
 * @(#)Stefan Eissing, 07. September 1991
 *
 * jr 31.7.94
 */


#ifndef G_CLIPBRD__
#define G_CLIPBRD__

int PasteString (char *string, int newline, char *attrib);

int GetClipText (char *buffer, size_t max_len);

#endif	/* !G_CLIPBRD */
