/*
 * @(#) Mupfel\gemutil.c 
 * @(#) Stefan Eissing, 27. Februar 1993
 *
 * - verschiedene Funktionen im Zusammenhang mit GEM
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <aes.h>
#include <vdi.h>

#include "stand.h"
#include "..\common\alloc.h"
#include "..\common\argvinfo.h"
#if GEMINI
#include <flydial\flydial.h>
#include "..\venus\gemini.h"
#endif

#include "mglob.h"

#include "cookie.h"
#include "gemutil.h"
#include "mdef.h"


static void deleteAccWindows (void)
{
	int window;
	
	while (wind_get (0, WF_TOP, &window) != 0 && window > 0)
	{
		wind_close (window);
		wind_delete (window);
	}
}

void WindNew (void)
{
	int GEMVersion = _GemParBlk.global[0];
	int maxApps = _GemParBlk.global[1];
	
	if (maxApps != 1)
		return;
		
	if (GEMVersion == 0x104 
		|| (GEMVersion >= 0x130 && GEMVersion != 0x210))
	{
		wind_new ();
#if GEMINI
		WindRestoreControl ();
#else
		wind_update (BEG_UPDATE);
#endif
	}
	else
		deleteAccWindows ();
}


void InstallGemBackground (MGLOBAL *M, const char *name, 
	int show_full, int draw_background)
{
	int pxy[4];
	int textx, texty;
	char filename[MAX_FILENAME_LEN];
	char *display_name;

	/* force redraw of desktop background
	 */
#if GEMINI
	wind_set (0, WF_NEWDESK, &EmptyBackground, 0, 0);
#else
	wind_set (0, WF_NEWDESK, NULL, 0, 0);
#endif
	
	if (draw_background)
		form_dial (FMD_FINISH, 0, 0, 0, 0, 0, 0, 
			M->desk_x + M->desk_w, M->desk_y + M->desk_h);
	
	/* white bar at top
	 */
	pxy[0] = pxy[1] = 0;
	pxy[2] = M->desk_x + M->desk_w;
	pxy[3] = M->desk_y + M->desk_h;
	
	vs_clip(M->vdi_handle, TRUE, pxy);
	vswr_mode (M->vdi_handle, MD_REPLACE);

	vsf_interior (M->vdi_handle, FIS_SOLID);
	vsf_color (M->vdi_handle, WHITE);
	pxy[0] = pxy[1] = 0;
	pxy[3] = M->char_h + 2;
	v_bar (M->vdi_handle, pxy);
	
	/* command's name centered
	 */
	display_name = (char *)name;

	if (!show_full)
	{
		char *cp = strrchr (name, '\\');
		if (cp)
		{
			strcpy (filename, cp + 1);
			display_name = filename;
		}
	}

#if GEMINI
	SetSystemFont ();
#endif
	vst_color (M->vdi_handle, BLACK);
	textx = (pxy[2] - (int) strlen (display_name) * M->char_w) / 2;
	texty = 1;
	v_gtext (M->vdi_handle, textx, texty, display_name);
	
	/* black line under bar */
	vsl_type (M->vdi_handle, 1);
	vsl_color (M->vdi_handle, BLACK);
	pxy[0] = 0;
	pxy[1] = pxy[3] = M->char_h + 2;
	v_pline (M->vdi_handle, 2, pxy);
}

int MupfelWindUpdate (int mode)
{
#if GEMINI
	WindUpdate (mode);
	return TRUE;
#else
	return wind_update (mode);
#endif
}

int MupfelGrafMouse (int mode)
{
#if GEMINI
	GrafMouse (mode, NULL);
	return TRUE;
#else
	return graf_mouse (mode, NULL);
#endif
}


