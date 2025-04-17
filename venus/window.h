/*
 * @(#) Gemini\Window.h
 * @(#) Stefan Eissing, 13. November 1994
 *
 * description: Header File for window.c
 *
 */

#ifndef G_window__
#define G_window__

#include "..\common\alloc.h"
#include "..\common\argvinfo.h"


/* udate-flags for WindInfo
 */
#define WU_SHOWTYPE		0x0001
#define WU_WINDNAME		0x0002
#define WU_WINDINFO		0x0004
#define WU_HOSLIDER		0x0008
#define WU_VESLIDER		0x0010
#define WU_SLIDERRESET	0x0020
#define WU_POSITION		0x0040
#define WU_REDRAW		0x0080
#define WU_DISPLAY		0x0100



/* kind of window
 */
#define WK_FILE			0x0001	/* normales window mit Dateien */
#define WK_CONSOLE      0x0002  /* Mupfel-Window */
#define WK_DESK			0x0003  /* Desktophintergrund */
#define WK_ACC			0x0004	/* Fenster gehîrt Accessory */
#define WK_TERMINAL		0x0005	/* Terminal-Fenster */

typedef struct windInfo
{
	struct windInfo *nextwind;		/* Pointer zum nÑchsten Window */
	word kind;						/* Art des Windows */
	word owner;						/* ApId. des Besitzers */
	word update;					/* Flags fÅr updates */

	word handle;					/* Kennung des Windows */
	word type;						/* Windowtyp */
	GRECT wind;						/* totale Grenzen des Windows */
	GRECT work;						/* Grenzen der Workarea */
	GRECT oldwind;					/* fÅr Fullbox */

	word vslpos;					/* Position vertikaler Slider */
	word vslsize;					/* dessen Grîûe */
	word hslpos;					/* Position horizontaler Slider */
	word hslsize;					/* dito */
	int selectAnz;					/* Anzahl der selektierten Objekte */

	void *kind_info;				/* Pointer auf weiter Struktur */

	void (*window_calc_windrect) (struct windInfo *wp, GRECT *rect);
	void (*window_sized) (struct windInfo *wp);
	void (*window_moved) (struct windInfo *wp);
	void (*window_topped) (struct windInfo *wp);
	void (*window_untopped) (struct windInfo *wp);
	void (*window_closed) (struct windInfo *wp, word kstate);
	void (*window_destroyed) (struct windInfo *wp);
	void (*window_update) (struct windInfo *wp);
	void (*window_redraw) (struct windInfo *wp, const GRECT *rect);
	OBJECT *(*window_get_tree) (struct windInfo *wp);
	void (*window_get_statusline) (struct windInfo *wp, char *buffer);
	int  (*make_ready_to_drag) (struct windInfo *wp);

	int (*window_feed_key) (struct windInfo *wp, word code, word kstate);
	
	int (*getSelected) (struct windInfo *wp, ARGVINFO *A,
		int preferLowerCase);
	int (*getDragged) (struct windInfo *wp, ARGVINFO *A,
		int preferLowerCase);
		
	int (*make_new_object) (struct windInfo *wp);
	int (*make_alias) (struct windInfo *wp);

	int (*cut) (struct windInfo *wp, int only_asking);
	int (*copy) (struct windInfo *wp, int only_asking);
	int (*paste) (struct windInfo *wp, int only_asking);
	int (*delete) (struct windInfo *wp, int only_asking);
	int (*select_all) (struct windInfo *wp);
	void (*deselect_all) (struct windInfo *wp);
	int (*select_object) (struct windInfo *wp, word obj, int draw);
	int (*deselect_object) (struct windInfo *wp, word obj, int draw);

	void (*mouse_click) (struct windInfo *wp, word clicks,
		word mx, word my, word kstate);

	int (*get_display_pattern) (struct windInfo *wp, char *pattern);
	int (*set_display_pattern) (struct windInfo *wp, const char *pattern);
	int (*add_to_display_path) (struct windInfo *wp, const char *name);

	int (*darstellung) (struct windInfo *wp, int only_asking);
} WindInfo;

extern WindInfo wnull;


WindInfo *NewWindInfo (void);
WindInfo *GetWindInfo (word whandle);
word FreeWindInfo (WindInfo *wp);

WindInfo *WindowCreate (word attributes, word wind_handle);
int WindowOpen (WindInfo *wp);

int WindowClose (word window_handle, word kstate);
int WindowDestroy (word window_handle, int is_still_open, 
		int definitely_delete_it);

int WindowFull (word window_handle);
int WindowSize (word window_handle, GRECT *wind);
int WindowMove (word window_handle, GRECT *wind);

void WindowUpdateData (WindInfo *wp);
void UpdateAllWindowData (void);

void DoTopWindow (word whandle);
void DoBottomWindow (word whandle);
void IsTopWindow (word topwindow);


void cycleWindow (void);
void delAccWindows (void);
void CloseAllWind (int close_accs_too);

WindInfo *GetTopWindInfo (void);
WindInfo *GetTopWindInfoOfKind (word kind);

int WindGetSelected (ARGVINFO *A, int preferLowerCase);
int WindGetDragged (ARGVINFO *A, int preferLowerCase);

#endif	/* !G_window */