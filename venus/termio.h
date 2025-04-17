/*
 * @(#) Gemini\termio.h
 * @(#) Stefan Eissing, 09. MÑrz 1993
 *
 * description: functions to basic character output
 *
 * jr 13.8.94
 */


#ifndef G_TERMIO__
#define G_TERMIO__


/* Handle fÅr alle Text-Ausgaben
 */
extern word std_handle;


#define 	V_NORMAL  		0
#define 	V_STANDOUT		1
#define 	V_UNDERLINE		2
#define 	V_BOLD			4
#define 	V_LIGHT			8
#define 	V_ITALIC		16
#define 	V_INVERTED		32



/* VDI initialisieren
 */
char TM_Init (TermInfo *terminal, word *sysfonts);

/* VDI deinitialisieren
 */
void TM_Deinit (TermInfo *terminal);


/* std_handle auf <effect> setzen
 */
void SetStdHandleEffect (int effect);


/* Terminalcursor zeichnen - XOR-Modus
 */
void draw_cursor (TermInfo *terminal);


/* Textcursor physikalisch ausschalten
 */
void rem_cur (TermInfo *terminal);

/* Textcursor physikalisch einschalten 
 */
void disp_cur (TermInfo *terminal);

void TM_DelLine (TermInfo *terminal, word which);

void TM_InsLine (TermInfo *terminal, word which);

void TM_XErase (TermInfo *terminal, word y, word x1, word x2);

void TM_YErase (TermInfo *terminal, word y1, word y2);

/* ein Zeichen ausgeben
 */
void TM_DispChar (TermInfo *terminal, char ch);

/* eine Zeile ausgeben
 */
void TM_DispString (TermInfo *terminal, char *str);

/* ganzes Fenster neuzeichnen
 */
void TM_RedrawTerminal (TermInfo *terminal, word clip_pxy[4]);

void TM_FreshenTerminal (TermInfo *terminal);
	
char TM_IsTopWindow (TermInfo *terminal);

void TM_ForeColor (TermInfo *terminal, char color);
void TM_BackColor (TermInfo *terminal, char color);


#endif /* !G_TERMIO__ */