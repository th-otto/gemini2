/*
 * @(#) Gemini\icondraw.c
 * @(#) Stefan Eissing, 02. Juni 1993
 *
 * description: Routinen um ein Icon auszugeben
 *
 */

#include <stddef.h>
#include <string.h>
#include <vdi.h>
#include <aes.h>

#include "vs.h"

#include "window.h"
#include "filewind.h"
#include "filedraw.h"
#include "icondraw.h"


#ifndef WHITEBAK
#define WHITEBAK	0x0040
#endif


static void draw_bitblk (word *p, word x, word y, word w, 
	word h, word num_planes, word mode, word *index)

{
	word pxy[8];
	MFDB s, d;

	d.fd_addr	= NULL; /* screen */
	s.fd_addr	= (void *)p;
	s.fd_w = w;
	s.fd_h	= h;
	s.fd_wdwidth = w >> 4;
	s.fd_stand	= FALSE;
	s.fd_nplanes = num_planes;

	pxy[0] = 0;
	pxy[1] = 0;
 	pxy[2] = s.fd_w - 1;
 	pxy[3] = s.fd_h - 1;

	pxy[4] = x;
	pxy[5] = y;
	pxy[6] = pxy[4] + pxy [2];
	pxy[7] = pxy[5] + pxy [3];

	if (num_planes > 1)
		vro_cpyfm (vdi_handle, mode, pxy, &s, &d);
	else
		vrt_cpyfm (vdi_handle, mode, pxy, &s, &d, index);	 /* copy it */
}



word cdecl DrawIconObject (PARMBLK *pb)
{
	ICON_OBJECT *icon;
	CICON *cip;
	word pxy[8];
	word icon_x, icon_y;
	word text_x, text_y, text_len;
	MFDB src_mfdb, dest_mfdb;
	word color_index[2];
	word selected, f_color, b_color;
	char zeichen[2] = " ";
	int white_back;
	int text_pixel_width = 0;

	icon = (ICON_OBJECT *)pb->pb_parm;
	cip = icon->mainlist;
	
	selected = pb->pb_currstate & SELECTED;
	white_back = pb->pb_currstate & WHITEBAK;
	
	dest_mfdb.fd_addr = NULL;

	zeichen[0] = icon->icon.ib_char & 0xFF;

	text_x = pb->pb_x + icon->icon.ib_xtext;
	text_y = pb->pb_y + icon->icon.ib_ytext;
	if (icon->icon.ib_ptext != NULL)
		text_len = (word)strlen (icon->icon.ib_ptext);
	else
		text_len = 0;
	
	if (zeichen[0] || text_len)
	{
		SetIconFont ();
		text_pixel_width = text_len * SystemIconFont.char_width;
	}

	pxy[0] = pb->pb_xc;
	pxy[1] = pb->pb_yc;
	pxy[2] = pb->pb_xc + pb->pb_wc - 1;
	pxy[3] = pb->pb_yc + pb->pb_hc - 1;
	if (pxy[0] > pb->pb_x
		|| pxy[1] > pb->pb_y
		|| pxy[2] < (pb->pb_x + pb->pb_w - 1)
		|| pxy[2] < (text_x + text_pixel_width - 1)
		|| pxy[3] < (pb->pb_y + pb->pb_h - 1))
	{
		vs_clip (vdi_handle, TRUE, pxy);
	}
	else
		vs_clip (vdi_handle, FALSE, pxy);

	icon_x = pb->pb_x + icon->icon.ib_xicon;
	icon_y = pb->pb_y + icon->icon.ib_yicon;

	vswr_mode (vdi_handle, MD_REPLACE);
	

	/* Wenn wir Text haben und nicht der WHITEBAK-Fall zutrifft,
	 * malen wir zuerst das Rechteck der Fahne
	 */
	if (text_len && (selected || !white_back))
	{
		vsf_interior (vdi_handle, FIS_SOLID);
		vsf_color (vdi_handle, selected? 1 : 0);
		pxy[0] = text_x;
		pxy[1] = text_y;
		pxy[2] = pxy[0] + icon->icon.ib_wtext - 1;
		pxy[3] = pxy[1] + icon->icon.ib_htext - 1;
		vr_recfl (vdi_handle, pxy);
	}
		
	/* Nur wenn das Icon auch wirklich Breite und H”he hat
	 * wird es gezeichnet. Ansonsten k”nnen die Pointer auf data
	 * und maske murks sein.
	 */
	if (icon->icon.ib_wicon && icon->icon.ib_hicon
		&& icon->icon.ib_pmask && icon->icon.ib_pdata)
	{
		if (cip != NULL)
		{
			word m_mode, i_mode, buf;
			word mindex[2], iindex[2];
			word *mask, *data, *dark = NULL;
			char invert = FALSE;
			
			m_mode  = MD_TRANS;
		
			if (selected)
			{
				if (cip->sel_data != NULL)
				{
					mask = cip->sel_mask;
					data = cip->sel_data;
					if (cip->num_planes > 1)
					{
						/* TrueColor, bzw RGB-orientierte Grafikkarte?
						 */
						if (cip->num_planes > 8)
							i_mode = S_AND_D;
						else
							i_mode = S_OR_D;
					}
					else
						i_mode = MD_TRANS;
				}
				else
				{
					mask = cip->col_mask;
					data = cip->col_data;
		
					if (cip->num_planes > 1)
					{
						if (cip->num_planes > 8)
							i_mode = S_AND_D;
						else
							i_mode = S_OR_D;
						dark = cip->sel_mask;
					}
					else
						invert = TRUE;
				}
			}
			else
			{
				mask = cip->col_mask;
				data = cip->col_data;
			
				if (cip->num_planes > 1)
				{
					if (cip->num_planes > 8)
						i_mode = S_AND_D;
					else
						i_mode = S_OR_D;
				}
				else
					i_mode = MD_TRANS;
			}
			
			mindex [0] = WHITE;
			mindex [1] = WHITE;
			
			iindex[0] = BLACK;
			iindex[1] = WHITE;
		
			if (invert)
			{
				buf       = iindex[0];
				iindex[0] = mindex[0];
				mindex[0] = buf;
				i_mode    = MD_TRANS;
			}
			
			draw_bitblk (mask, icon_x, icon_y, icon->icon.ib_wicon, 
				icon->icon.ib_hicon, 1, m_mode, mindex);
			draw_bitblk (data, icon_x, icon_y, icon->icon.ib_wicon, 
				icon->icon.ib_hicon, cip->num_planes, i_mode, iindex);
			
			if (dark)
			{
				mindex [0] = BLACK;
				mindex [1] = WHITE;
				draw_bitblk (dark, icon_x, icon_y, icon->icon.ib_wicon, 
					icon->icon.ib_hicon, 1, MD_TRANS, mindex);
			}
		}
		else
		{
			f_color = icon->icon.ib_char >> 8;
			b_color = f_color & 0xF;
			f_color = (f_color >> 4) & 0xF;
			
			color_index[0] = selected? f_color : b_color;
			color_index[1] = 0;
			src_mfdb.fd_w = icon->icon.ib_wicon;
			src_mfdb.fd_h = icon->icon.ib_hicon;
			src_mfdb.fd_wdwidth = src_mfdb.fd_w / 16;
			if (src_mfdb.fd_w % 16)
				++src_mfdb.fd_wdwidth;
			src_mfdb.fd_stand = src_mfdb.fd_nplanes = 1;
			
			pxy[0] = pxy[1] = 0;
			pxy[2] = icon->icon.ib_wicon - 1;
			pxy[3] = icon->icon.ib_hicon - 1;
			pxy[4] = icon_x;
			pxy[5] = icon_y;
			pxy[6] = pxy[4] + icon->icon.ib_wicon - 1;
			pxy[7] = pxy[5] + icon->icon.ib_hicon - 1;
		
			if ((color_index[0] != 0) || !white_back)
			{
				src_mfdb.fd_addr = icon->icon.ib_pmask;
				vrt_cpyfm (vdi_handle, MD_TRANS, pxy, &src_mfdb, 
					&dest_mfdb, color_index);
			}
		
			color_index[0] = selected? b_color : f_color;
			src_mfdb.fd_addr = icon->icon.ib_pdata;
			vrt_cpyfm (vdi_handle, MD_TRANS, pxy, &src_mfdb, &dest_mfdb,
				color_index);
		}
	}

	if (zeichen[0] || text_len)
	{
		vst_color (vdi_handle, selected? 0 : 1);
		vswr_mode (vdi_handle, MD_TRANS);
		VstEffects (icon->flags.is_symlink? 0x04 : 0x0);
		
		if (zeichen[0])
		{
			v_gtext (vdi_handle, icon_x + icon->icon.ib_xchar,
				icon_y + icon->icon.ib_ychar, zeichen);
		}
			
		if (text_len)
		{
			text_x += (icon->icon.ib_wtext - text_pixel_width) / 2;
			text_y += (icon->icon.ib_htext - 
				SystemIconFont.char_height) / 2;
		
			v_gtext (vdi_handle, text_x + 
				SystemIconFont.kursiv_offset_links, text_y, 
				icon->icon.ib_ptext);
		}
	}
		
	return (pb->pb_currstate & ~SELECTED);
}
