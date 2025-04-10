/*
 * @(#) Gemini\FileDraw.c
 * @(#) Stefan Eissing, 06. November 1994
 *
 * description: functions to redraw text in file windows
 *
 * jr 26.7.94
 */

#include <vdi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\fontsel.h>
#include <nls\nls.h>


#include "vs.h"
#include "showbox.rh"
#include "stdbox.rh"

#include "window.h"
#include "filewind.h"
#include "filedraw.h"
#include "fileutil.h"
#include "gemini.h"
#include "icondraw.h"
#include "menu.h"
#include "message.h"
#include "redraw.h"
#include "terminal.h"
#include "util.h"
#include "venuserr.h"

/* external
 */
extern OBJECT *pshowbox,*pstdbox;

/* internal texts
 */
#define NlsLocalSection "G.filedraw"
enum NlsLocalText{
T_TEST,			/*The quick brown fox jumps over the lazy dog.*/
T_WRONGFONT,	/*Der gewnschte Font fr die Dateifenster konnte 
nicht eingestellt werden, %s!*/
T_NOTINST,		/*da er nicht installiert ist*/
T_NOGDOS,		/*da kein GDOS installiert ist*/
};

/*
 * internals
 */
FONTWORK filework;

FILEFONTINFO TextFont, SIconFont;
FILEFONTINFO SystemNormFont, SystemIconFont;

#define ICON_FONT_HEIGHT	4



void GetFileFont(word *id, word *points)
{
	*id = TextFont.id;
	*points = TextFont.size;
}


static void fileTextDrawRect (OBJECT *tree, const GRECT *rect,
	int only_this)
{
	word i, pxy[4];
	PARMBLK pb;
	FileInfo *pf;
	int is_clipped;
	FILEFONTINFO *ffi = NULL;
	
	pxy[0] = rect->g_x;
	pxy[1] = rect->g_y;
	pxy[2] = rect->g_x + rect->g_w - 1;
	pxy[3] = rect->g_y + rect->g_h - 1;

	vswr_mode (vdi_handle, MD_REPLACE);
	vsf_color (vdi_handle, WHITE);
	vst_color (vdi_handle, BLACK);
	vs_clip (vdi_handle, FALSE, pxy);
	is_clipped = FALSE;
	vr_recfl (vdi_handle, pxy);

	pb.pb_tree = tree;
	
	if (only_this > 0)
		i = only_this;
	else
		i = tree[0].ob_head;
		
	for (; i > 0; i = tree[i].ob_next)
	{
		GRECT object;
		word x, y;
		FILEFONTINFO *now;
		
		pf = GetFileInfo (&tree[i]);
		if (!pf)
			continue;

		now = pf->flags.text_display? &TextFont : &SIconFont;
		if (now != ffi)
		{
			ffi = now;
			SetFileFont (ffi);
		}
			
		VstEffects (pf->icon.flags.is_symlink? 0x04 : 0);
		
		x = object.g_x = tree[0].ob_x + tree[i].ob_x; 
		y = object.g_y = tree[0].ob_y + tree[i].ob_y;
		object.g_w = tree[i].ob_width;
		object.g_h = tree[i].ob_height;

		if (GRECTIntersect (rect, &object))
		{
			if (((object.g_w >= tree[i].ob_width) 
				&& (object.g_h >= tree[i].ob_height))
				^ (!is_clipped))
			{
				vs_clip (vdi_handle, !is_clipped, pxy);
				is_clipped = !is_clipped;
			}
				
			if (!pf->flags.text_display || (pf->attrib & FA_FOLDER))
			{
				pb.pb_obj = i;
				pb.pb_prevstate = pb.pb_currstate = tree[i].ob_state;
				pb.pb_x = x;
				pb.pb_y = y;
				pb.pb_w = tree[i].ob_width;
				pb.pb_h = tree[i].ob_height;
				pb.pb_xc = rect->g_x;
				pb.pb_yc = rect->g_y;
				pb.pb_wc = rect->g_w;
				pb.pb_hc = rect->g_h;
				pb.pb_parm = (long)&pf->icon;
				DrawIconObject (&pb);
			}
			
			x += pf->icon.x;
			y += pf->icon.y;

			if (ffi->is_proportional)
			{
				char *cp;
				word x1, i;
				
				cp = pf->icon.string;
				x1 = x;
				if (pf->icon.flags.is_symlink)
					x1 += ffi->kursiv_offset_links;

				/* jr: mit diesem Flag fragen wir ab, ob der
				Dateiname das 8+3-Format hat und deshalb die
				Extension in eine eigene Spalte muž */
				
				if (pf->icon.freelength == 0)
				{
					char save;
		
					save = cp[8];
					cp[8] = '\0';
					v_gtext (vdi_handle, x1, y, cp);
					cp[8] = save;
				
					cp += 9;
					x1 += 9 * ffi->wchar;
				
					if (cp[0] != ' ' && cp[0] != '\0')
					{
						save = cp[3];
						cp[3] = '\0';
						v_gtext (vdi_handle, x1, y, cp);
						cp[3] = save;
					}
				
					cp += 4;
					x1 += 4 * ffi->wchar;
				}
				else
				{
					int len = pf->icon.freelength;
				
					cp[len] = '\0';
					v_gtext (vdi_handle, x1, y, cp);
					cp[len] = ' ';
					cp += len;
					x1 += len * ffi->wchar;
				}
				
				i = 0;
				if (ffi->show.size)
					++i;
				if (ffi->show.date)
					++i;
				if (ffi->show.time)
					++i;
				
				while (i)
				{
					char save;
				
					if (x1 > pxy[2])
						break;
					
					cp[0] = ' ';
					save = cp[9];
					cp[9] = '\0';
					v_justified (vdi_handle, x1, y, cp,
						9 * ffi->num_wchar, 1, 0);
					cp[9] = save;
					
					cp += 9;
					x1 += 9 * ffi->num_wchar;
					--i;
				}
			}
			else
				v_gtext (vdi_handle, x,	y, pf->icon.string);
	
			if (tree[i].ob_state & SELECTED)
			{
				word v_rect[4];
				
				v_rect[0] = x - 1;
				v_rect[1] = y;
				v_rect[2] = v_rect[0] + pf->icon.icon.ib_wtext - 1;
				v_rect[3] = v_rect[1] + pf->icon.icon.ib_htext - 1;
				
				vswr_mode (vdi_handle, MD_XOR);
				vsf_color (vdi_handle, BLACK);
				vsf_interior (vdi_handle, FIS_SOLID);
				vr_recfl (vdi_handle, v_rect);
				vswr_mode (vdi_handle, MD_REPLACE);
			}
			
		}
		
		if (only_this > 0)
			break;
	}
}


word cdecl drawFileText (PARMBLK *pb)
{
	GRECT clip;

	if (pb->pb_prevstate != pb->pb_currstate)
		return pb->pb_currstate;

	clip.g_x = pb->pb_xc;
	clip.g_y = pb->pb_yc;
	clip.g_w = pb->pb_wc;
	clip.g_h = pb->pb_hc;

	fileTextDrawRect (pb->pb_tree, &clip, pb->pb_obj);
	
	return pb->pb_currstate;
}


void FileWindowDrawRect (FileWindow_t *fwp, const GRECT *rect)
{
	if (fwp->display != DisplayBigIcons)
	{
		fileTextDrawRect (fwp->tree, rect, -1);
	}
	else
	{
		objc_draw (fwp->tree, 0, MAX_DEPTH, rect->g_x, 
			rect->g_y, rect->g_w, rect->g_h);
	}
}


void FileWindowRedraw (word wind_handle, FileWindow_t *fwp, 
	const GRECT *orig_rect)
{
	GRECT wind, rect;
	
	rect = *orig_rect;
	if (!GRECTIntersect (&Desk, &rect))
		return;
		
	vsf_interior (vdi_handle, FIS_SOLID);
	
	GrafMouse (M_OFF, NULL);
	wind_get (wind_handle, WF_FIRSTXYWH, &wind.g_x, &wind.g_y,
		&wind.g_w, &wind.g_h);

	while (wind.g_w && wind.g_h)
	{
		if (GRECTIntersect (&rect, &wind))
			FileWindowDrawRect (fwp, &wind);
		
		wind_get (wind_handle, WF_NEXTXYWH, &wind.g_x, &wind.g_y,
			&wind.g_w, &wind.g_h);
	}
	
	GrafMouse (M_ON, NULL);
	setBigClip (vdi_handle);
}


static void setFFInfo (FILEFONTINFO *ffi)
{
	word i, width, num_width, w, dummy;
	ulong total = 0L;
	word distances[5];
	word effects[3];
	int c;
	
	num_width = 0;
	c = ' ';
	for (i = 0; c <= ':'; ++i, ++c)
	{
		vqt_width (filework.handle, c, &w, &dummy, &dummy);
		total += w;
		if (w > num_width)
			num_width = w;
	}
	
	width = num_width;
	for (; c <= 'z'; ++i, ++c)
	{
		vqt_width (filework.handle, c, &w, &dummy, &dummy);
		total += w;
		if (w > width)
			width = w;
	}

	total /= i;
	ffi->is_proportional = (width != ffi->char_width
		|| width != total);
	
	ffi->filled_for_id = ffi->id;
	ffi->filled_for_size = ffi->size;
	ffi->wchar = width;
	ffi->num_wchar = num_width;
	ffi->hchar = ffi->char_height;
	
	VstEffects (0x04);
	vqt_fontinfo (filework.handle, &dummy, &dummy,
					distances, &width, effects);
	ffi->kursiv_offset_links = effects[1];
	ffi->kursiv_offset_rechts = effects[2];
}


void setAESFont (FONTWORK *fw, word *id, word *size)
{
	word i;
	word my_size, char_height, char_width;
	
	my_size = 10;
	
	for (i = 0; i < 11; i++)
	{
		*size = my_size;
		SetFont (fw, id, size, &char_width, &char_height);

		if (char_height < hchar)
			++my_size;
		else if (char_height > hchar)
			--my_size;
		else			/* it fit's */
			break;
	}
}

static void deinst_font (void)
{
	FontUnLoad (&filework);
}

/*
 * make dialog for showoptions (options for display of files
 * in windows)
 */
int FileWindowDarstellung (WindInfo *wp, int only_asking)
{
	DIALINFO d;
	FONTSELINFO F;
	FILEFONTINFO *ffi;
	FileWindow_t *fwp;
	int id, size, retcode;
	word changed, length, date, time;
	word clicks, draw, item;
	int edit_object = SHOWFBG;

	if (!wp || (wp->kind != WK_FILE))
		return 0;
	fwp = wp->kind_info;
	
	if (only_asking)
		return fwp->display != DisplayBigIcons;
	
	ffi = (fwp->display == DisplayText)? &TextFont : &SIconFont;
	SetFileFont (ffi);
	
	setSelected (pshowbox, SOPTSIZE, ffi->show.size);
	setSelected (pshowbox, SOPTDATE, ffi->show.date);
	setSelected (pshowbox, SOPTTIME, ffi->show.time);
	
	DialCenter (pshowbox);

	id = ffi->id;
	size = ffi->size;

	SetSystemFont ();
	GrafMouse (HOURGLASS, NULL);
	if (!FontGetList (&filework, FALSE, TRUE))
		venusInfo ("getlist failed");
	if (!FontSelInit (&F, &filework, pshowbox, SHOWFB, SHOWFBG, SHOWPB,
		SHOWPBG, SHOWST, SHOWSTB, SHOWFONT, 
		(char *)NlsStr (T_TEST), TRUE, &id, &size))
		venusInfo ("selinit failed");
		
	GrafMouse (ARROW, NULL);

	DialStart (pshowbox, &d);
	DialDraw (&d);
	FontSelDraw (&F, id, size);

	do
	{
		uword key;
	
		draw = TRUE;
		item = -1;
		
		retcode = DialXDo (&d, &edit_object, NULL, &key, NULL, NULL, NULL);
		
		clicks = (retcode & 0x8000)? 2 : 1;
		retcode &= 0x7FFF;
		
		/* jr: Support fr tastaturbedienbare Listen */
		
		if (retcode == SHOWFBG || retcode == SHOWFB ||
			retcode == SHOWST || retcode == SHOWSTB ||
			retcode == SHOWPB || retcode == SHOWPBG)
		{
			if ((key & 0xff00) == 0x4800)
				clicks = FONT_CL_UP;
			else  if ((key & 0xff00) == 0x5000)
				clicks = FONT_CL_DOWN;
			else if (key != 0)
				retcode = -1;
		}
		
		if (retcode == SHOWFB || retcode == SHOWFBG)
		{
			edit_object = SHOWFBG;
			item = FontClFont (&F, clicks, &id, &size);
		}
		else if (retcode == SHOWPB || retcode == SHOWPBG)
		{
			edit_object = SHOWPBG;
			item = FontClSize (&F, clicks, &id, &size);
		}
		else if (retcode == SHOWST || retcode == SHOWSTB)
		{
			edit_object = SHOWSTB;
			item = FontClForm (&F, clicks, &id, &size);
		}
		
		if ((item >= 0) && (clicks == 2))
		{
			retcode = SHOWOK;
			draw = FALSE;
		}

	} 
	while ((retcode != SHOWCANC) && (retcode != SHOWOK));
	
	if (draw)
	{
		setSelected (pshowbox, retcode, FALSE);
		fulldraw (pshowbox, retcode);
	}
	
	if (retcode == SHOWOK)
	{
		length = isSelected (pshowbox, SOPTSIZE);
		date = isSelected (pshowbox, SOPTDATE);
		time = isSelected (pshowbox, SOPTTIME);
		
		changed = ((fwp->display != DisplayBigIcons)
					&&((length != ffi->show.size)
					||(date != ffi->show.date)
					||(time != ffi->show.time)));

		ffi->show.size = length;
		ffi->show.date = date;
		ffi->show.time = time;
		
		if ((changed) || (id != ffi->id) || (size != ffi->size))
		{
			changed = TRUE;
			ffi->id = id;
			ffi->size = size;
		}
	}
	else
	{
		/* all was cancelled */
		SetFont (&filework, &ffi->id, &ffi->size,
					&ffi->char_width, &ffi->char_height);
		changed = FALSE;
	}

	FontSelExit (&F);
	DialEnd (&d);

	if (changed)
	{
		SetFileFont (ffi);
		allrewindow (FW_NEW_FONT);
		if (ffi == &TextFont)
			FontChanged (TextFont.id, TextFont.size,
				BiosTerminal.font_id, BiosTerminal.font_size);
	}
	
	return TRUE;
}

extern int appl_getinfo (int type, int *out1, int *out2, int *out3, int *out4);


void initFileFonts (void)
{
	word dummy;
	

	FontAESInfo (filework.handle, &filework.loaded,
		&SystemNormFont.id, &SystemNormFont.size,
		&SystemIconFont.id, &SystemIconFont.size);

	SetFont (&filework, &SystemNormFont.id, &SystemNormFont.size,
		&SystemNormFont.char_width, &SystemNormFont.char_height);
	setFFInfo (&SystemNormFont);

	SetFont (&filework, &SystemIconFont.id, &SystemIconFont.size,
		&SystemIconFont.char_width, &SystemIconFont.char_height);
	vst_height (filework.handle, ICON_FONT_HEIGHT, &dummy, &dummy,
		&SystemIconFont.char_width, &SystemIconFont.char_height);
	setFFInfo (&SystemIconFont);

	TextFont.id = 1;
	TextFont.size = 10;
	SetFont (&filework, &TextFont.id, &TextFont.size,
				&TextFont.char_width, &TextFont.char_height);
	setFFInfo (&TextFont);
	SIconFont = TextFont;
}

void exitFileFonts (void)
{
	deinst_font ();
}

word WriteFontInfo (MEMFILEINFO *mp, char *buffer)
{
	int ishow;
	
	ishow = SIconFont.show.size | (SIconFont.show.date << 1)
		| (SIconFont.show.time << 2);
		
	sprintf (buffer, "#F@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d@%d",
			TextFont.id, TextFont.size, 32,
			NewDesk.snapx, NewDesk.snapy, 
			ShowInfo.normicon, TextFont.show.size, 
			TextFont.show.date, TextFont.show.time, 
			ShowInfo.showtext, ShowInfo.sortentry,
			SIconFont.id, SIconFont.size, ishow);
			
	return !mputs (mp, buffer);
}

void execFontInfo (char *line)
{
	char *cp;
	FILEFONTINFO *ffi;
	
	strtok (line, "@\n");		/* skip first */
	
	if((cp = strtok (NULL, "@\n")) != NULL)
	{
		TextFont.id = atoi (cp);
		if((cp = strtok (NULL, "@\n")) != NULL)
		{
			TextFont.size = atoi (cp);
			if((cp = strtok (NULL, "@\n")) != NULL)
			{
				if((cp = strtok (NULL, "@\n")) != NULL)
				{
					NewDesk.snapx = (char)atoi (cp);
					if((cp = strtok (NULL, "@\n")) != NULL)
					{
						NewDesk.snapy = (char) atoi(cp);
					}
				}
			}
		}
	}

	if ((cp = strtok (NULL, "@\n")) != NULL)
	{
		ShowInfo.normicon = atoi (cp);
		if ((cp = strtok (NULL, "@\n")) != NULL)
		{
			TextFont.show.size = atoi (cp);
			if ((cp = strtok (NULL, "@\n")) != NULL)
			{
				TextFont.show.date = atoi (cp);
				if ((cp = strtok (NULL, "@\n")) != NULL)
				{
					TextFont.show.time = atoi (cp);
					if ((cp = strtok (NULL, "@\n")) != NULL)
					{
						ShowInfo.showtext = atoi (cp);
						if ((cp = strtok (NULL, "@\n")) != NULL)
						{
							ShowInfo.sortentry = atoi (cp);
							SetShowInfo ();
						}
					}
				}
			}
		}
	}
	
	if (TextFont.id < 1)
		TextFont.id = 1;
	
	if ((cp = strtok (NULL, "@\n")) != NULL)
	{
		SIconFont.id = atoi (cp);
		if ((cp = strtok (NULL, "@\n")) != NULL)
		{
			SIconFont.size = atoi (cp);
			if ((cp = strtok (NULL, "@\n")) != NULL)
			{
				int d = atoi (cp);
				
				SIconFont.show.size = (d & 1) != 0;
				SIconFont.show.date = (d & 2) != 0;
				SIconFont.show.time = (d & 4) != 0;
			}
			else
				SIconFont.show = TextFont.show;
		}
		else
			SIconFont.size = TextFont.size;
	}
	else
		SIconFont = TextFont;
	
	ffi = ShowInfo.showtext? &TextFont : &SIconFont;
	
	if ((ffi->id == 1)	&& (ffi->size == 10))
	{
		/* system font, try to scale
		 */
		setAESFont (&filework, &ffi->id, &ffi->size);
	}
	else
	{
		if (!SetFont (&filework, &ffi->id, &ffi->size,
					&ffi->char_width, &ffi->char_height))
		{
			venusErr (NlsStr (T_WRONGFONT),
				filework.addfonts > 0?
				NlsStr (T_NOTINST) : NlsStr (T_NOGDOS));
		}
	}
	
	setFFInfo (ffi);
}


typedef enum 
{
	None, SystemFont, IconFont, FileFont
} FontSetting;

static FontSetting fontSet;

word SetFont (FONTWORK *fw, word *id, word *points, word *width, 
	word *height)
{
	word distances[5];
	word effects[3];
	word cw,ch;
	word dummy, desiredId;
	
	FontLoad (fw);
	desiredId = *id;
	*id = vst_font (fw->handle, *id);
	vqt_fontinfo (fw->handle, &dummy, &dummy,
					distances, width, effects);
	if (fw->list && FontIsFSM (fw, *id))
		*points = vst_arbpt (fw->handle, *points, &cw, &ch, width, height);
	else
		*points = vst_point (fw->handle, *points, &cw, &ch, width, height);

	fontSet = None;
	return desiredId == *id;
}


void SetIconFont (void)
{
	word dummy;
	
	if (fontSet == IconFont)
		return;

	SetFont (&filework, &SystemIconFont.id, &SystemIconFont.size,
				&SystemIconFont.char_width, 
				&SystemIconFont.char_height);
	vst_height (filework.handle, ICON_FONT_HEIGHT, &dummy, &dummy,
		&SystemIconFont.char_width, &SystemIconFont.char_height);
		
	fontSet = IconFont;
}

void SetFileFont (FILEFONTINFO *ffi)
{
	static FILEFONTINFO *last = NULL;
	int fill_new_info;
	
	fill_new_info = ((ffi->id != ffi->filled_for_id)
			|| (ffi->size != ffi->filled_for_size));
	
	if (!fill_new_info && (fontSet == FileFont) && (last == ffi))
		return;

	SetFont (&filework, &ffi->id, &ffi->size,
				&ffi->char_width, &ffi->char_height);

	fontSet = FileFont;
	last = ffi;
	if (fill_new_info)
	{
		/* Die Informationen stammen nicht von diesem Font
		 * und mssen neu berechnet werden.
		 */
		setFFInfo (ffi);
	}

	pstdbox[FISTRING].ob_height = ffi->hchar;
}

void SetSystemFont (void)
{
	if (fontSet == SystemFont)
		return;

	SetFont (&filework, &SystemNormFont.id, &SystemNormFont.size,
				&SystemNormFont.char_width, 
				&SystemNormFont.char_height);
	fontSet = SystemFont;
	VstEffects (0);
}


void VstEffects (word effect)
{
	static word has_effect = 0;
	
	if (effect != has_effect)
	{
		vst_effects (vdi_handle, effect);
		has_effect = effect;
	}
}
