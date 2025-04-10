/*
 * @(#) Gemini\Termio.c
 * @(#) Arnd Beissner und Stefan Eissing, 08. Januar 1994
 *
 * description: basic functions for mupfel window
 *
 * jr 26.7.94
 */


#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <vdi.h>
#include <aes.h>
#include <tos.h>

#include "vs.h"
#include "wintool.h"
#include "util.h"
#include "terminal.h"
#include "termio.h"


/* Handle fr alle Text-Ausgaben 
 */
word std_handle;

/* Handle fr invertierte Ausgaben
 */
static word inv_handle;

/* Eingestellter Effekt auf der VWS std_handle
 */
static word std_effect;


/* jr: Konvertiere die Bitmaske in fr's GDOS geeignete
   Attribute */

static int makefontbits (TermInfo *terminal, int mask)
{
	int bits = 0;
	
	if (mask & V_STANDOUT) mask |= terminal->inv_type;

	if (mask & V_BOLD) bits |= 1;
	if (mask & V_LIGHT) bits |= 2;
	if (mask & V_ITALIC) bits |= 4;
	if (mask & V_UNDERLINE) bits |= 8;
	
	return bits;
}

void TM_GetCursor (TermInfo *terminal, int *x, int *y)
{
	*x = (*terminal->p_cur_x);
	*y = (*terminal->p_cur_y);
}


static int vdi_initialized = FALSE;

/* VDI initialisieren
 */
char TM_Init (TermInfo *terminal, word *sysfonts)
{
	if (!vdi_initialized)
	{
		word dummy;
		word aes_handle;
		word pxy[4];
		word work_out[57];				/* VDI Parameter Arrays */
		word work_in[11] = { 1,1,1,1,1,1,1,1,1,1,2 };
	
		vdi_initialized = TRUE;

		work_in[0] = Getrez () + 2;
		aes_handle = graf_handle(&dummy, &dummy, &dummy, &dummy);
		
		std_handle = aes_handle;
		v_opnvwk(work_in, &std_handle, work_out);
		if (std_handle <= 0)
		{
			return FALSE;
		}
	
		inv_handle = aes_handle;
		v_opnvwk(work_in, &inv_handle, work_out);
		if (inv_handle <= 0)
		{
			v_clsvwk(std_handle);
			return FALSE;
		}
	
		/* Anzahl der System-Fonts
		 */
		*sysfonts = work_out[10];
		
	    vswr_mode (std_handle, MD_REPLACE);
	    
		/* Defaults fr die Invert-VWKS setzen
		 */
		vswr_mode (inv_handle, MD_XOR);
		vsf_color (inv_handle, 1);
		vsf_interior (inv_handle, FIS_SOLID);

		pxy[0] = 0;
		pxy[1] = 0;
		pxy[2] = work_out[0];
		pxy[3] = work_out[1];
		vs_clip (std_handle, 1, pxy);
		vst_alignment (std_handle, 0, 5, &dummy, &dummy);
		vsf_interior (std_handle, FIS_SOLID);
		std_effect = 0;
	}

	terminal->backColor = 1;
	terminal->foreColor = 1;
	terminal->cur_on = TRUE;

	return TRUE;
}


/* VDI deinitialisieren
 */
void TM_Deinit (TermInfo *terminal)
{
	if (terminal->flags.is_console && vdi_initialized)
	{
		v_clsvwk (std_handle);
		v_clsvwk (inv_handle);
	}
}


void SetStdHandleEffect (int effect)
{
	if (effect != std_effect)
	{
		std_effect = effect;
		vst_effects (std_handle, effect);
	}
}

void TM_ForeColor (TermInfo *terminal, char color)
{
	terminal->foreColor = color - '0';
/*
	vst_color(std_handle, terminal->foreColor);
*/
}

void TM_BackColor(TermInfo *terminal, char color)
{
	terminal->backColor = color - '0';
/*
	vsf_color(inv_handle, terminal->foreColor);
*/
}

/* Terminalcursor zeichnen - XOR-Modus
 */
void draw_cursor (TermInfo *terminal)
{
    word pxy[4], rect[4];
    word count;

    pxy[0] = terminal->work.g_x + terminal->font_width * (*terminal->p_cur_x);
    pxy[1] = terminal->work.g_y + terminal->font_height * (*terminal->p_cur_y);
    pxy[2] = pxy[0] + terminal->font_width - 1;
    pxy[3] = pxy[1] + terminal->font_height - 1;
    
    count = 0;
    while (WT_GetRect(count++, rect))
	{
		WT_Clip (inv_handle, std_handle, rect);
		vr_recfl (inv_handle, pxy); 
	}
}


/* Textcursor physikalisch ausschalten
 */
void rem_cur(TermInfo *terminal)
{
    if (terminal->cur_on == 1 )
    	draw_cursor (terminal);
}
    

/* Textcursor physikalisch einschalten
 */
void disp_cur(TermInfo *terminal)
{
    if (terminal->cur_on == 1)
    	draw_cursor (terminal);
}


/* vereinfachte Bitblit-Funktion
 */
static void bitblit (word pxy[8])
{
	MFDB src, dest;
    
	src.fd_addr = dest.fd_addr = NULL;
	src.fd_stand = dest.fd_stand = 0;
	vro_cpyfm (std_handle, 3, pxy, &src, &dest); 
}


static void fillLinePartially (TermInfo *terminal, int line, 
	int from, int to)
{
	word rect[4];
	
	rect[1] = terminal->work.g_y + (line * terminal->font_height);
	rect[3] = rect[1] + terminal->font_height - 1;
	rect[0] = terminal->work.g_x + (from * terminal->font_width);
	rect[2] = terminal->work.g_x + (to * terminal->font_width) - 1;
	
	vr_recfl (std_handle, rect);
}


static void print_line (TermInfo *terminal, word line, word yk)
{
	word x, pxy[4];
	char *ch_p;
	
	ti_sline (terminal, line)[terminal->columns] = '\0';
 
    /* erstes non-space-Zeichen der Zeile finden */
    ch_p = ti_sline (terminal, line);
    x = 0;
    while (*ch_p == ' ' && *ch_p != 0) 
   	{
    	ch_p++;
    	x++;
   	}

	if (x) fillLinePartially (terminal, line, 0, x);
	
	if (*ch_p != 0)
	{
		int len, i;
		char *temp, ch;
		
		/* letztes non-space-Zeichen der Zeile finden */
		len = (int)strlen (ch_p);
		temp = ch_p + len - 1;
		for (i = 0; i < len; i++)
		{
			if (*temp != ' ')
				break;
			temp--;
		}
		
		if (i < len)
		{
			ch = *(temp + 1);
			*(temp + 1) = 0;
			SetStdHandleEffect (0);
   	    	v_gtext(std_handle,
   	    		terminal->work.g_x + x * terminal->font_width, yk, ch_p);
   	    	*(temp+1) = ch;
   	    
			fillLinePartially (terminal, line, x + len - i, terminal->columns);
    	}
	}

    for (x = 0; x < terminal->columns; x++)
	{
		int attr = ti_aline (terminal, line)[x];
		char lch[2] = " ";

   		if (attr != V_NORMAL)
		{
			SetStdHandleEffect (makefontbits (terminal, attr));
			lch[0] = ti_sline (terminal, line)[x];
			v_gtext (std_handle,
				terminal->work.g_x + x * terminal->font_width, yk, lch);
    
   			if ((attr & V_STANDOUT && terminal->inv_type == V_STANDOUT)
   				|| attr & V_INVERTED)
			{
				pxy[0] = terminal->work.g_x + x * terminal->font_width;
				pxy[1] = yk;
				pxy[2] = pxy[0] + terminal->font_width - 1;
				pxy[3] = yk + terminal->font_height - 1;
				vr_recfl (inv_handle, pxy);
            }
		}
	}
}


void TM_DelLine (TermInfo *terminal, word which)
{
	word pxy[8], rect[4];
    word line, yk, count;

    vsf_color (std_handle, 0);

	count = 0;
	while (WT_GetRect (count++, rect))
    {
        if (rect[3] - terminal->work.g_y >= which * terminal->font_height)
       	{
	        WT_Clip (std_handle, inv_handle, rect);

	        /* scroll the visible area
	         */
			pxy[0] = pxy[4] = rect[0];
			pxy[2] = pxy[6] = rect[2];

	        if (rect[3] - rect[1] >= terminal->font_height)
        	{
				pxy[5] = terminal->work.g_y + which * terminal->font_height;
				if (pxy[5] < rect[1])
					pxy[5] = rect[1];

				pxy[7] = rect[3] - terminal->font_height;
				pxy[1] = pxy[5] + terminal->font_height;
				pxy[3] = rect[3];
				bitblit (pxy);
			}

			/* die sichtbar gewordene(n) Zeile(n) ausgeben
			 */
	        line = (rect[3] - terminal->work.g_y) / terminal->font_height;
	        yk = line * terminal->font_height + terminal->work.g_y;

	        if (rect[3] != terminal->work.g_y + terminal->work.g_h - 1)
	        	print_line (terminal, line - 1, yk - terminal->font_height);

       		print_line (terminal, line, yk);

			/* Die unterste Zeile l”schen
			 */
			pxy[3] = terminal->work.g_y + terminal->work.g_h - 1;
			pxy[1] = pxy[3] - terminal->font_height + 1;
			vr_recfl (std_handle, pxy);
		}
	}
}


void TM_InsLine(TermInfo *terminal, word which)
{
	word pxy[8], rect[4];
    word line, yk, count;

    vsf_color(std_handle, 0);

   	count = 0;
    while (WT_GetRect(count++, rect))
	{
        if (rect[3] - terminal->work.g_y >= which * terminal->font_height)
       	{
	        WT_Clip (std_handle, inv_handle, rect);

	        /* scroll the visible area */
			pxy[0] = pxy[4] = rect[0];
			pxy[2] = pxy[6] = rect[2];
	        if (rect[3] - rect[1] > terminal->font_height)
        	{
				pxy[1] = terminal->work.g_y + which * terminal->font_height;
				if (pxy[1] < rect[1])
					pxy[1] = rect[1];
				pxy[3] = rect[3] - terminal->font_height;
	
				pxy[5] = pxy[1] + terminal->font_height;
				pxy[7] = pxy[3] + terminal->font_height;
				bitblit(pxy);
			}

			/* die sichtbar gewordene(n) Zeile(n) ausgeben */
	        line = (rect[3] - terminal->work.g_y) / terminal->font_height;
	        yk = line * terminal->font_height + terminal->work.g_y;
        	print_line (terminal, line-1, yk - terminal->font_height);
        	print_line (terminal, line, yk);
			
			pxy[1] = terminal->work.g_y + which * terminal->font_height;
			pxy[3] = pxy[1] + terminal->font_height - 1;
			vr_recfl(std_handle, pxy);
		}
	}
}


void TM_YErase (TermInfo *terminal, word y1, word y2)
{
    word rect[4], pxy[4];
	word count;

   	vsf_color(std_handle, 0);
   	pxy[0] = terminal->work.g_x;
   	pxy[1] = terminal->work.g_y + terminal->font_height * y1;
   	pxy[2] = terminal->work.g_x + terminal->work.g_w - 1;
   	pxy[3] = terminal->work.g_y + terminal->font_height * (y2+1) - 1;
   	
   	count = 0;
    while (WT_GetRect(count++, rect)) 
    {
        WT_Clip (std_handle, inv_handle, rect);
    	vr_recfl (std_handle, pxy);
  	}
}


void TM_XErase (TermInfo *terminal, word y, word x1, word x2)
{
    word rect[4], pxy[4];
    word count;

   	vsf_color(std_handle, 0);
   	pxy[0] = terminal->work.g_x + x1 * terminal->font_width;
   	pxy[1] = terminal->work.g_y + y * terminal->font_height;
   	pxy[2] = terminal->work.g_x + x2 * terminal->font_width - 1;
   	pxy[3] = pxy[1] + terminal->font_height - 1;

   	count = 0;
    while (WT_GetRect(count++, rect))
    {
        WT_Clip(std_handle, inv_handle, rect);
    	vr_recfl(std_handle, pxy);
   	}
}



/* ein Zeichen ausgeben
 */
void TM_DispChar (TermInfo *terminal, char ch)
{
	char lch[2] = " ";
	word count;
	word pxy[4];

	if (ch == '\0')
		return;
		
	count = 0;
	while (WT_GetRect (count++, pxy)) 
    {
        word xk, yk;
        
        WT_Clip (std_handle, inv_handle, pxy);

	    /* Zeichen ausgeben */
		xk = terminal->work.g_x + (*terminal->p_cur_x) * terminal->font_width;
		yk = terminal->work.g_y + (*terminal->p_cur_y) * terminal->font_height;
	    
		lch[0] = ch;

		SetStdHandleEffect (makefontbits (terminal, terminal->wr_mode));
		v_gtext (std_handle, xk, yk, lch);

		if (((terminal->wr_mode & V_STANDOUT) && terminal->inv_type == V_STANDOUT)
			|| terminal->wr_mode & V_INVERTED)
    	{
			pxy[0] = xk;
       		pxy[1] = yk;
       		pxy[2] = xk + terminal->font_width - 1;
           	pxy[3] = yk + terminal->font_height - 1;
  			vr_recfl (inv_handle, pxy);
    	}
    }
}


/* eine Zeile ausgeben
 */
void TM_DispString (TermInfo *terminal, char *str)
{
	word pxy[4];
	word count;

	count = 0;
	while (WT_GetRect (count++, pxy))
    {
        WT_Clip (std_handle, inv_handle, pxy);

		/* Zeile ausgeben */

		SetStdHandleEffect (makefontbits (terminal, terminal->wr_mode));
		v_gtext (std_handle,
			terminal->work.g_x + (*terminal->p_cur_x) * terminal->font_width,
			terminal->work.g_y + (*terminal->p_cur_y) * terminal->font_height,
			str);

		if (((terminal->wr_mode & V_STANDOUT) && terminal->inv_type == V_STANDOUT)
			|| terminal->wr_mode & V_INVERTED)
		
  		{
			pxy[0] = terminal->work.g_x + (*terminal->p_cur_x)
				 * terminal->font_width;
           	pxy[1] = terminal->work.g_y + (*terminal->p_cur_y)
   				 * terminal->font_height;
			pxy[2] = pxy[0] + terminal->font_width
				 * (word)strlen (str) - 1;
			pxy[3] = pxy[1] + terminal->font_height - 1;
    		vr_recfl (inv_handle, pxy);
    	}
    }
}


/* ganzes Fenster neuzeichnen
 */
void TM_RedrawTerminal (TermInfo *terminal, word clip_pxy[4])
{
    word pxy[4];
    size_t y, x, i, len;
    word yk;
    word start_line, end_line;
    char *ch_p, *temp;
    char ach[2], ch;

    /* redraw background
     */ 
    pxy[0] = terminal->work.g_x;
    pxy[1] = terminal->work.g_y;
    pxy[2] = terminal->work.g_x + terminal->work.g_w - 1;
    pxy[3] = terminal->work.g_y + terminal->work.g_h - 1;

	WT_Clip (std_handle, inv_handle, clip_pxy);
    vsf_color (std_handle, 0);
    vr_recfl (std_handle, pxy);

    /* redraw text area
     */
    vsf_color (std_handle, 1);
    vsf_perimeter (std_handle, 0);
    ach[1] = 0;

    /* berechnen, welcher Teil sichtbar ist
     */
    start_line = (clip_pxy[1] - terminal->work.g_y) / terminal->font_height;
	if (start_line < 0)
		start_line = 0;
	end_line = terminal->rows - 1;
		
    yk = terminal->work.g_y + (start_line * terminal->font_height);
    for (y = start_line; y <= end_line; y++)
    {
        /* erstes non-space-Zeichen der Zeile finden */
        ti_sline (terminal, y)[terminal->columns] = '\0';
        ch_p = ti_sline (terminal, y);
        x = 0;
        while ((*ch_p == ' ') && (*ch_p != '\0'))
       	{
        	ch_p++;
        	x++;
       	}

		if (*ch_p != 0)
		{
			/* letztes non-space-Zeichen der Zeile finden
			 */
			len = strlen (ch_p);
			temp = ch_p + len - 1;
			for (i = 0; i < len; i++)
			{
				if (*temp != ' ')
					break;
				temp--;
			}
			
			if (i < len)
			{
				word gx;
				size_t slen;
				char *cp;
				
				ch = *(temp + 1);
				*(temp + 1) = 0;
				SetStdHandleEffect (0);
				gx = (word)(terminal->work.g_x + (x * terminal->font_width));
				cp = ch_p;
				slen = strlen (cp);
#define VGTEXT_CAN	120
				if (slen >= VGTEXT_CAN)
				{
					char c;
					
					c = cp[VGTEXT_CAN];
					cp[VGTEXT_CAN] = '\0';
					v_gtext (std_handle, gx, yk, cp);
					cp[VGTEXT_CAN] = c;
					cp += VGTEXT_CAN;
					gx += (VGTEXT_CAN * terminal->font_width);
				}
	   	    	v_gtext(std_handle, gx, yk, cp);
	   	    	*(temp+1) = ch;
	    	}
		}

        ch_p = ti_aline (terminal, y);
        for (x = 0; x < terminal->columns; x++)
        {
			if (*ch_p != V_NORMAL)
            {
				ach[0] = ti_sline (terminal, y)[x];
            	SetStdHandleEffect (makefontbits (terminal, *ch_p));
				v_gtext (std_handle, (word)(terminal->work.g_x 
					+ (x * terminal->font_width)), yk, ach);
            	
                if ((*ch_p & V_STANDOUT && terminal->inv_type == V_STANDOUT)
                	|| *ch_p & V_INVERTED)
                {
					pxy[0] = (word)(terminal->work.g_x + 
						(x * terminal->font_width));
					pxy[1] = yk;
					pxy[2] = pxy[0] + terminal->font_width - 1;
					pxy[3] = yk + terminal->font_height - 1;
					vr_recfl (inv_handle, pxy);
				}
            }
            
            ch_p++;
        }
        
        yk += terminal->font_height;
    }
    
    /* Cursor zeichnen, falls eingeschaltet
     */
    if (terminal->cur_on == 1)
    {
		pxy[0] = terminal->work.g_x + terminal->font_width * (*terminal->p_cur_x);
	   	pxy[1] = terminal->work.g_y + terminal->font_height * (*terminal->p_cur_y);
	    pxy[2] = pxy[0] + terminal->font_width - 1;
	    pxy[3] = pxy[1] + terminal->font_height - 1;
		vr_recfl (inv_handle, pxy); 
	}
}


void TM_FreshenTerminal (TermInfo *terminal)
{
    word Rect[4], count;

	WT_BuildRectList (terminal->window_handle);

	count = 0;
    while (WT_GetRect (count++, Rect))
    {
        TM_RedrawTerminal (terminal, Rect);
	}
}

