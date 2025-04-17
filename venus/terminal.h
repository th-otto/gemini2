/*
 * @(#) Gemini\terminal.h
 * @(#) Stefan Eissing, 20. M„rz 1993
 *
 * description: functions to handle vt52 emulation
 *
 * jr 16.4.95
 */


#ifndef __terminal__
#define __terminal__


typedef struct terminfo
{
	int columns;
	int rows;

	int inv_type;
	/* Bitset aus V_STANDOUT, V_UNDERLINE, V_BOLD, V_LIGHT, etc
	 */
	int wr_mode;

	/* Flag: Cursor an/aus
	 */
	char cur_on;

	/* aktuelle Textcursorposition
	 */
	/* jr: make it a pointer so that we can redirect it to
	Line A */
	 
	int *p_cur_x, *p_cur_y;
	int _cur_x, _cur_y;

	/* Buffer fr 'save cursor'
 	*/
	int *p_x_saved, *p_y_saved;
	int _x_saved, _y_saved;

	int wrap;
	int tab_size;
	int tab_destructive;
	int back_destructive;
	
	char foreColor;
	char backColor;

	word window_handle;			/* Handle des Fensters */
	GRECT work;					/* Arbeitsfl„che des Fensters */


	/* Buffer fr Escape-Modus: letztes Zeichen
	 */	
	int last_chr;
	int lastcols, lastrows;

	/* vector for terminal output
	 */
	void (*term_out)(struct terminfo *t, char);

	struct
	{
		unsigned is_console : 1;
	} flags;
	
	word font_width;
	word font_height;
	word font_id;
	word font_size;
	
	/* Bildschirm-Buffer */
	char *pscreen, *pattrib;

	char termname[20];
	int pty;
	int process_grp;
	
} TermInfo;

#define ti_sline(a,b)	(&((a)->pscreen[(b)*((a)->columns+2L)]))
#define ti_aline(a,b)	(&((a)->pattrib[(b)*((a)->columns+2L)]))

extern TermInfo BiosTerminal;

/* Terminal initialisieren
 */
TermInfo *InitTerminal (int is_console, word *sysfonts);

/* Terminal deinitialisieren
 */
void ExitTerminal (TermInfo *terminal);

void ReInitTerminal (TermInfo *terminal, int newcol, int newlines);

/* Bildschirm-Buffer l”schen
 */	
void DelScreenBuffer (TermInfo *terminal);

/* Textcursor physikalisch ausschalten
 */
void rem_cur (TermInfo *terminal);

/* Textcursor physikalisch einschalten
 */
void disp_cur (TermInfo *terminal);

/* VT-52: Invers-Modus einschalten
 */
void rev_on (TermInfo *terminal);

/* VT-52: Invers-Modus ausschalten
 */
void rev_off (TermInfo *terminal);

/* VT-52: Wrap-Around einschalten
 */
void wrap_on (TermInfo *terminal);

/* VT-52: Wrap-Around ausschalten
 */
void wrap_off (TermInfo *terminal);
 
/* VT-52: Textcursor logisch einschalten
 */
void show_cursor (TermInfo *terminal);

/* VT-52: Textcursor logisch ausschalten
 */
void hide_cursor (TermInfo *terminal);

/* VT-52 Funktion: Carriage Return
 */
void vt_cr (TermInfo *terminal);
   
/* VT-52 Funktion: Carriage Return
 */
void vt_lf (TermInfo *terminal);

void vt_c_right (TermInfo *terminal);

void vt_c_left (TermInfo *terminal);

void vt_c_up (TermInfo *terminal);
    
void vt_c_down (TermInfo *terminal);

void vt_c_home (TermInfo *terminal);

void vt_cl_home (TermInfo *terminal);
   
void vt_c_set (TermInfo *terminal, word y, word x);

void vt_cui (TermInfo *terminal);

void vt_c_save (TermInfo *terminal);
  
void vt_c_restore (TermInfo *terminal);

/* insert line
 */
void vt_ins_line (TermInfo *terminal);
	
/* delete line
 */
void vt_del_line(TermInfo *terminal);

/* erase line
 */
void vt_erline (TermInfo *terminal);
 
void erase_to_linestart (TermInfo *terminal);

/* erase to line start
 */
void vt_erlst (TermInfo *terminal);

/* clear to end of line - subroutine
 */
void clr_eol (TermInfo *terminal);

/* clear to end of line
 */
void vt_cleol (TermInfo *terminal);
	
/* erase to end of page
 */
void vt_ereop (TermInfo *terminal);

/* erase to start of page
 */
void vt_ersop (TermInfo *terminal);

/* ein Zeichen ausgeben
 */
void disp_char (TermInfo *terminal, char ch);
void disp_canchar (TermInfo *terminal, char ch);
void disp_rawchar (TermInfo *terminal, char ch);

/* eine Zeile ausgeben - ohne Interpretation
 */
void disp_string (TermInfo *terminal, char *str);

extern void cur_home (TermInfo *terminal);

char escon (TermInfo *terminal);

void InitBiosOutput (void);
void ExitBiosOutput (void);

#endif