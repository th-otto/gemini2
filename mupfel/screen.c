/*
 * @(#) Mupfel\screen.c
 * @(#) Stefan Eissing, 13. November 1994
 *
 * -  set screen for external command
 */

#include <ctype.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vdi.h>
#include <aes.h>
#include <tos.h>

#include "..\common\alloc.h"
#include "..\common\argvinfo.h"
#include "..\common\cookie.h"
#include "mglob.h"

#include "chario.h"
#include "exec.h"
#include "gemutil.h"
#include "screen.h"
#include "vt52.h"
#include "stand.h"
#if GEMINI
#include "..\venus\gemini.h"
#endif


static SCREENINFO actScreen;

static void initScreenInfo (void)
{
	if (actScreen.initialized)
		return;
		
	actScreen.initialized = 1;
	actScreen.cursor_blink = 0;
#if GEMINI
	actScreen.mouse_on = 0;
	actScreen.cursor_on = 1;
	actScreen.grey_back = 1;
	actScreen.window_open = 1;
#else
	actScreen.mouse_on = 0;
	actScreen.cursor_on = 1;
	actScreen.grey_back = 0;
	actScreen.window_open = 0;
#endif	/* GEMINI */
}


int ScreenWindowsAreOpen (void)
{
	return actScreen.window_open;
}

int SetScreen (MGLOBAL *M, SCREENINFO *should, 
	const char *full_name, int show_full_path)
{
	if (M->shell_flags.system_call)
		return TRUE;

	initScreenInfo ();
	
#if GEMINI
	if (should->window_open == PARM_EGAL)
		should->window_open = actScreen.window_open;
		
	if ((should->window_open != 0) ^ (actScreen.window_open != 0))
	{
		if (should->window_open)
		{
			/* Wir waren vorher nicht im Fenster, also ”ffnen
			 * wir diese
			 */
			Cursconf (0, 0);

			if (!actScreen.mouse_on)
				MupfelGrafMouse (M_ON);
			
			PrepareExternalStart (M, FALSE);

			actScreen.mouse_on = 1;
			actScreen.cursor_on = 1;
			actScreen.grey_back = 1;
		}
		else
		{
			/* Die Fenster sind noch offen, schlieže sie
			 */
			PrepareExternalStart (M, TRUE);
			
			actScreen.cursor_on = 0;
			actScreen.mouse_on = 0;
			actScreen.grey_back = 1;
		}
		
		actScreen.window_open = should->window_open;
		should->window_open = 1;
	}
	else
		should->window_open = 0;
#endif		

	if ((should->cursor_on != 0) ^ (actScreen.cursor_on != 0))
	{
		if (should->cursor_on)
			Cursconf (1, 0);
		else
			Cursconf (0, 0);
		
		actScreen.cursor_on = should->cursor_on;
		should->cursor_on = 1;
	}
	else
	{
		/* Wenn der Cursor aus sein soll und wir ein GEM-Programm
		 * haben und nicht die Fenster von Gemini offen sind, dann 
		 * stelle ihn auf jeden Fall aus
		 */
		if (!should->cursor_on && should->grey_back
			&& !actScreen.window_open)
		{
			Cursconf (0, 0);
		}
		should->cursor_on = 0;
	}

	if (!actScreen.window_open)
	{
		if (should->grey_back)
		{
			if (should->window_open == 0)
				WindNew ();
			InstallGemBackground (M, full_name, show_full_path, 
				should->window_open || should->cursor_on);
			
			should->grey_back = !actScreen.grey_back;
			actScreen.grey_back = 1;
		}
		else if (actScreen.grey_back == 1)
		{
			clearscreen (M);
			actScreen.grey_back = 0;
			should->grey_back = 1;
		}
	}
	else
		should->grey_back = 0;

	if ((should->mouse_on != 0) ^ (actScreen.mouse_on != 0))
	{
		if (should->mouse_on)
			MupfelGrafMouse (M_ON);
		else
			MupfelGrafMouse (M_OFF);

		actScreen.mouse_on = should->mouse_on;
		should->mouse_on = 1;
	}
	else
		should->mouse_on = 0;
	
	if ((should->cursor_blink != 0) ^ (actScreen.cursor_blink != 0))
	{
		if (should->cursor_blink)
			Cursconf (2, 0);
		else
			Cursconf (3, 0);
		
		actScreen.cursor_blink = should->cursor_blink;
		should->cursor_blink = 1;
	}
	else
		should->cursor_blink = 0;
	
	return TRUE;
}


int UnsetScreen (MGLOBAL *M, SCREENINFO *changed)
{
	if (M->shell_flags.system_call)
		return TRUE;

	if (changed->cursor_blink)
	{
		if (!actScreen.cursor_blink)
			Cursconf (2, 0);
		else
			Cursconf (3, 0);
		
		actScreen.cursor_blink ^= 1;
	}

	if (changed->cursor_on)
	{
		if (!actScreen.cursor_on)
			Cursconf (1, 0);
		else
			Cursconf (0, 0);
		
		actScreen.cursor_on ^= 1;
	}
	
	if (changed->mouse_on)
	{
		if (!actScreen.mouse_on)
			MupfelGrafMouse (M_ON);
		else
			MupfelGrafMouse (M_OFF);

		actScreen.mouse_on ^= 1;
	}
	
#if GEMINI
	if (changed->window_open)
	{
		if (actScreen.window_open)
		{
			PrepareExternalStart (M, TRUE);

			actScreen.window_open = 0;
			actScreen.cursor_on = 0;
			actScreen.mouse_on = 0;
			actScreen.grey_back = 1;
		}
		else
		{
			int phys_handle, foo;
			
			phys_handle = graf_handle (&foo, &foo, &foo, &foo);

			/* wir mssen die Fenster ”ffnen
			 */
			if (!actScreen.mouse_on)
			{
				MupfelGrafMouse (M_ON);
			}
			
			Cursconf (0, 0);
			WindNew ();
			if (!CookiePresent ('KAOS', NULL))
				v_show_c (phys_handle, 0);

			PrepareExternalStart (M, FALSE);

			MupfelGrafMouse (ARROW);
			MupfelGrafMouse (M_OFF);
		
			actScreen.window_open = 1;
			actScreen.cursor_on = 1;
			actScreen.mouse_on = 0;
			actScreen.grey_back = 1;
		}
		
	}
	else if (actScreen.window_open && changed->mouse_on)
	{
		/* Wir haben ein Pogramm mit Maus, aber ohne Fenster
		 * schliežen gestartet. Sicherheitshalber stellen wir
		 * das Gemini-Men neu dar.
		 */
		SetMenuBar (TRUE);
	}
#endif	/* GEMINI */

	if (changed->grey_back && actScreen.grey_back
		&& !actScreen.window_open)
	{
		if (!M->shell_flags.dont_use_gem)
			WindNew ();
			
		clearscreen (M);
		actScreen.grey_back = 0;
	}

	return TRUE;
}

