/*
	@(#)FlyDial/wndl.h
	
	Julian F. Reschke, 27. Oktober 1995
*/

#ifndef __WNDL_H
#define __WNDL_H

#ifndef __FLYDIAL__
#include "flydial/flydial.h"
#endif 

#ifndef __EVNTEVNT_H
#include "flydial/evntevnt.h"
#endif

typedef struct
{
	int		handle;
	char		*window_title;
	int			vertical_offset;
	GRECT		g;
	OBJECT		*tree;
	long		old_border;
} WNDL_INFO;

/* WNDL_INFO-Struktur fÅr gegebenen Dialog initialisieren */
WNDL_INFO *WndlStart (OBJECT *tree);

/* Dialog zentrieren */
void WndlCenter (WNDL_INFO *W);

/* Dialog îffnen */
void WndlOpen (WNDL_INFO *W);

/* EVENT-Struktur einspeisen */
int WndlFeedEvent (WNDL_INFO *W, MEVENT *E, int event_type,
	int *exit_button);

/* Dialog durchfÅhren, sollte in etwa DialDo entsprechen */
short WndlDo (WNDL_INFO *W);

/* Dialog beenden wieder entfernen */
void WndlEnd (WNDL_INFO *W);



#endif
