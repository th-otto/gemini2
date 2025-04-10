/*
 * @(#) Gemini\draglogi.h
 * @(#) Stefan Eissing, 13. November 1994
 *
 * description: Header File for draglogi.c
 *
 */

#ifndef G_draglogi__
#define G_draglogi__

int doDragLogic (WindInfo *fwp, word to_window, word toobj, 
	word kstate, word fromx,word fromy,word tox,word toy);

int DropFiles (int mx, int my, int kstate, ARGVINFO *A);
		
#endif	/* !G_draglogi__ */