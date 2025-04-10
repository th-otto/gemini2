/*
 * @(#) Gemini\conselec.h
 * @(#) Stefan Eissing, 23. Juli 1991
 *
 * description: selection in console window
 */

#ifndef G_conselec__
#define G_conselec__

void ConSelection (word con_window_handle, word mx, word my, 
	word kstate, int doubleClick);

#endif	/* !G_conselec__ */