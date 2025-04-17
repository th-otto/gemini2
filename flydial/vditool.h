/*
	@(#)FlyDial/vditool.h
	@(#)Julian F. Reschke, 12. November 1992

	Tools fÅr VDI-Workstation-Verwaltung und Ñhnliches
*/

#ifndef __VDITOOL_
#define __VDITOOL_

#include <vdi.h>

#ifndef WORD
#define WORD int
#endif

#ifndef UWORD
#define UWORD unsigned int
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif


typedef struct
{
	WORD	handle;			/* VDI handle */

	/* stuff returned from v_opnvwk */
	
	WORD	max_x, max_y;
	WORD	no_scaling;
	WORD	pixel_width, pixel_height;
	WORD	character_heights;
	WORD	line_types;
	WORD	line_widths;
	WORD	marker_types;
	WORD	marker_sizes;
	WORD	system_fonts;
	WORD	patterns, hatches, colors;
	WORD	num_gdps;
	WORD	has_gdp[10];
	WORD	gdp_attr[10];
	WORD	has_color;
	WORD	has_text_rotation;
	WORD	has_fill_area;
	WORD	has_cell_array;
	WORD	palette;
	WORD	locators, valuators, choice_devs, string_devs;
	WORD	type;
	WORD	min_char_width, min_char_height;
	WORD	max_char_width, max_char_height;
	WORD	min_line_width;
	WORD	reserved_1;
	WORD	max_line_width;
	WORD	reserved_2;
	WORD	min_marker_width, min_marker_height;
	WORD	max_marker_width, max_marker_height;

	/* private part */
	
	WORD	has_loaded_fonts;
	WORD	clipping, clip_rect[4];
	unsigned char *buffer;

} VDIWK;

void V_ClearWorkstation (VDIWK *V);
void V_CloseVirtualWorkstation (VDIWK *V);
void V_CloseWorkstation (VDIWK *V);
void V_CopyRasterOpaque (VDIWK *V, WORD mode, WORD *xy, MFDB *src, MFDB *dst);
void V_EnterAlphaMode (VDIWK *V);
void V_ExitAlphaMode (VDIWK *V);
void V_HideCursor (VDIWK *V);
WORD V_OpenVirtualWorkstation (VDIWK *V, WORD coord_system);
WORD V_OpenWorkstation (VDIWK *V, WORD device, WORD coord_system);
void V_PolyLine (VDIWK *V, WORD count, WORD *xy);
void V_SetClipping (VDIWK *V, WORD clip_flag, WORD *array);
void V_ShowCursor (VDIWK *V, WORD restore);
int V_UpdateWorkstation (VDIWK *V);

#endif