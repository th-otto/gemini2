/*
 * @(#) Gemini\undo.c
 * @(#) Stefan Eissing, 29. September 1993
 *
 * description: functions to undo/redo things
 */

#include <string.h>
#include <aes.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"

#include "vs.h"
#include "menu.rh"

#include "window.h"
#include "conwind.h"
#include "filewind.h"
#include "fileutil.h"
#include "undo.h"
#include "redraw.h"
#include "venuserr.h"



#define UNDOWOPEN	0x0001
#define UNDOICON	0x0002
#define UNDOWMOVE	0x0004
#define UNDOPATH	0x0008
#define UNDOWFULL	0x0010

/* externals
 */
extern OBJECT *pmenu;


struct UndoWOpen
{
	word whandle;
	word yskip, kind;
	char wildcard[WILDLEN];
	char label[MAX_FILENAME_LEN];
	char path[MAX_PATH_LEN];
	char title[MAX_PATH_LEN];
};

struct UndoWMove
{
	word whandle;		/* handle des windows */
	GRECT wind;			/* rechteck des windows */
};

struct UndoPath
{
	word whandle;		/* handle des windows */
	word yskip;		/* position des vertikalen Sliders */
	char path[MAX_PATH_LEN];
	char title[MAX_PATH_LEN];
};

struct UndoIcon
{
	word x,y;
	word type;
	word iconNr;
	char iconname[MAX_FILENAME_LEN];
	char path[MAX_PATH_LEN];
};

static struct UndoInfo
{
	word type;				/* typ der structure in der union */
	word isUndo;			/* Undo oder Redo */
	union
	{
		struct UndoWOpen wo;	/* ™ffnen und Schliežen von windows */
		struct UndoWMove wm;	/* Verschieben von windows */
		struct UndoPath p;		/* cd undo */
		struct UndoIcon i;		/* De/Installieren von Icons */
	}u;
}ui;


void storeWOpenUndo (word open, WindInfo *wp)
{
	struct UndoWOpen *p = &ui.u.wo;

	if (wp->kind == WK_ACC)
		return;
		
	ui.isUndo = open;
	ui.type = UNDOWOPEN;
	p->kind = wp->kind;
	p->whandle = wp->handle;
	menu_ienable (pmenu,DOUNDO,TRUE);

	switch (wp->kind)
	{
		case WK_FILE:
		{
			FileWindow_t *fwp = wp->kind_info;
			
			strcpy (p->path, fwp->path);
			strcpy (p->title, fwp->title);
			strcpy (p->wildcard, fwp->wildcard);	
			strcpy (p->label, fwp->label);
			p->yskip = fwp->yskip;
			break;
		}
		
		case WK_CONSOLE:
		{
			ConsoleWindow_t *cwp = wp->kind_info;
			
			strcpy (p->path, cwp->path);
			strcpy (p->title, cwp->title);
			break;
		}
	}
}

void storeWMoveUndo (WindInfo *wp)
{
	struct UndoWMove *p = &ui.u.wm;
	
	ui.type = UNDOWMOVE;
	p->whandle = wp->handle;
	p->wind = wp->wind;
	menu_ienable (pmenu, DOUNDO, TRUE);
}

void storePathUndo (WindInfo *wp)
{
	FileWindow_t *fwp = wp->kind_info;
	struct UndoPath *p = &ui.u.p;
	
	if (wp->kind != WK_FILE)
		return;
		
	ui.type = UNDOPATH;
	p->whandle = wp->handle;
	p->yskip = fwp->yskip;
	strcpy (p->path, fwp->path);
	strcpy (p->title, fwp->title);
	menu_ienable (pmenu, DOUNDO, TRUE);
}

static void undoWOpen (void)
{
	struct UndoWOpen *p = &ui.u.wo;
	
	if (ui.isUndo)
	{
		WindowDestroy (p->whandle, TRUE, FALSE);
	}
	else
	{
		switch (p->kind)
		{
			case WK_FILE:
				FileWindowOpen (p->path, p->label, p->title, 
					p->wildcard, p->yskip);
				break;
			
			case WK_CONSOLE:
				TerminalWindowOpen (p->path, p->title, TRUE);
				break;
		}

		wind_get (0, WF_TOP, &p->whandle);
	}
}

static void undoWMove (void)
{
	struct UndoWMove *p = &ui.u.wm;
	GRECT wind = p->wind;

	WindowSize (p->whandle, &wind);
}

static void undoWFull(void)
{
	struct UndoWMove *p = &ui.u.wm;

	WindowFull (p->whandle);
}

static void undoPath (void)
{
	struct UndoPath *p = &ui.u.p;
	WindInfo *wp;
	word lastskip;
	
	if (((wp = GetWindInfo (p->whandle)) != NULL)
		&& wp->kind == WK_FILE)
	{
		FileWindow_t *fwp = wp->kind_info;
		char tmp[MAX_PATH_LEN];
		
		strcpy (tmp, fwp->path);
		strcpy (fwp->path, p->path);
		strcpy (p->path, tmp);

		strcpy (tmp, fwp->title);
		strcpy (fwp->title, p->title);
		strcpy (p->title, tmp);

		lastskip = fwp->yskip;
		fwp->yskip = p->yskip;
		p->yskip = lastskip;
		wp->add_to_display_path (wp, NULL);
	}
}

static void undoIconInst (void)
{
	if(ui.isUndo)
	{
	}
	else
	{
	}
}

void doUndo(void)
{
	switch(ui.type)
	{
		case UNDOWOPEN:
			undoWOpen();
			break;
		case UNDOWMOVE:
			undoWMove();
			break;
		case UNDOWFULL:
			undoWFull();
			break;
		case UNDOPATH:
			undoPath();
			break;
		case UNDOICON:
			undoIconInst();
			break;
		default:
			venusDebug("unbekannter Undotyp!");
			break;
	}
}

/*
 * void clearUndo(void)
 * clear undo disable menu
 */
void clearUndo(void)
{
	ui.type = 0;
	menu_ienable(pmenu,DOUNDO,FALSE);
}

void UndoFolderVanished (const char *path)
{
	struct UndoPath *p = &ui.u.p;
	
	if (ui.type == UNDOPATH) 
	{
		char dir[MAX_PATH_LEN];
		
		strcpy (dir, p->path);
		if (path[strlen (path) - 1] != '\\')
			dir[strlen (dir) - 1] = '\0';
		if (IsPathPrefix (dir, path))
			clearUndo ();
	}
}
