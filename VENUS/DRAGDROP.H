/*
 * @(#) Gemini\dragdrop.h
 * @(#) Stefan Eissing, 12. November 1994
 *
 */


#ifndef G_dragdrop__
#define G_dragdrop__

int TryAtariDragAndDrop (word owner, word to_window, 
	ARGVINFO *A, word mx, word my, word kstate);

int ReceiveDragAndDrop (word sender, word to_window,
	word mx, word my, word kstate, word pipe_id);
	
#endif