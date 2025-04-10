/*
 * @(#) Gemini\Wildcard.c
 * @(#) Stefan Eissing, 17. Juli 1993
 *
 * description: Modul fÅr Wildcard-Extensions
 *
 */
 
#include <stdio.h>
#include <string.h>
#include <flydial\flydial.h>
#include <flydial\listman.h>
#include <nls\nls.h>

#include "..\common\wildmat.h"

#include "vs.h"
#include "wildbox.rh"
#include "wedit.rh"

#include "window.h"
#include "filewind.h"
#include "filedraw.h"
#include "fileutil.h"
#include "redraw.h"
#include "scancode.h"
#include "util.h"
#include "venuserr.h"
#include "wildcard.h"


/* externs
 */
extern OBJECT *pwildbox, *pweditbox;

/* internal texts
 */


#define MAX_WILD	10
/* MAX_WILD wildcards fÅr windows
 */
static char wildpattern[MAX_WILD][WILDLEN] =
{
	"*.[CH]", "*.[AP]??", "*.TOS", "*.TTP", "*.[AB][CD]?"
};

int WriteWildPattern (MEMFILEINFO *mp, char *buffer)
{
	sprintf (buffer, "#M@%s@%s@%s@%s@%s", wildpattern[0],
		wildpattern[1],	 wildpattern[2], wildpattern[3],
		wildpattern[4]);

	if (mputs (mp, buffer))
		return FALSE;
		
	sprintf (buffer, "#M2@%s@%s@%s@%s@%s", wildpattern[5],
		wildpattern[6],	 wildpattern[7], wildpattern[8],
		wildpattern[9]);

	return !mputs (mp, buffer);
}

void ExecPatternInfo (char *line)
{
	char *cp;
	int i;
	
	i = (line[2] == '2')? 5 : 0;
	strtok (line, "@\n");

	while (i < MAX_WILD && ((cp = strtok (NULL, "@\n")) != NULL))
	{
		strncpy (wildpattern[i], cp, WILDLEN - 1);
		wildpattern[i][WILDLEN-1] = '\0';
		++i;
	}
}


/*
 * editiere den Wildcard pattern, und gib zurÅck, ob er
 * verÑndert wurde.
 */
static word editWildcard (char *pattern)
{
	DIALINFO d;
	word retcode;
	char mypat[WILDLEN + 1];
	int edit_object = WEDITTXT;
	
	strcpy (mypat, pattern);
	pweditbox[WEDITTXT].ob_spec.tedinfo->te_ptext = mypat;
	
	DialCenter (pweditbox);
	DialStart (pweditbox, &d);
	DialDraw (&d);

	retcode = DialDo (&d, &edit_object) & 0x7fff;
	setSelected (pweditbox, retcode, FALSE);
	fulldraw (pweditbox, retcode);
	DialEnd (&d);
	
	if (retcode == WEDITOK)
	{
		if (strcmp (pattern, mypat))
		{
			if (mypat[0])
				strncpy (pattern, mypat, WILDLEN-1);
			else
				strcpy (pattern, "*");
			pattern[WILDLEN-1] = '\0';
			return TRUE;
		}
	}
	
	return FALSE;
}

/*
 * Zeichne einen Wildcard fÅr Listman
 */
static void drawWildcard (LISTSPEC *l, word x, word y, word offset,
				GRECT *clip, word how)
{
	char *str;
	word dummy, xy[8];
	
	if (!l)
		return;
	
	str = l->entry;
	
	RectGRECT2VDI (clip, xy);
	vs_clip (vdi_handle, 1, xy);

	if (how & LISTDRAWREDRAW)
	{
		vswr_mode (vdi_handle, MD_REPLACE);
		vst_alignment (vdi_handle, 0, 5, &dummy, &dummy);
		VstEffects (0);
		SetSystemFont ();
		v_gtext (vdi_handle, x-offset+HandXSize, y, str);
	}

	if ((l->flags.selected) || !(how & LISTDRAWREDRAW))	
	{
		RectGRECT2VDI(clip, xy);
		vs_clip(vdi_handle, 1, xy);
	
		vsf_interior (vdi_handle, FIS_SOLID);
		vsf_color (vdi_handle, 1);
		vswr_mode (vdi_handle, MD_XOR);
		v_bar (vdi_handle, xy);
	}
	
	vs_clip (vdi_handle, 0, xy);
}


static int wnr, new_nr;
static char mypattern[MAX_WILD + 1][WILDLEN];

static int _UpDown (OBJECT *tree, int object, int obnext, int ochar, 
					int kstate, int *nxtobject, int *nxtchar)
{
	if (ochar == CUR_UP && wnr > 0)
		new_nr = wnr - 1;
	else if (ochar == CUR_DOWN && wnr < MAX_WILD)
		new_nr = wnr + 1;
	else
		new_nr = wnr;
		
	if (wnr != new_nr)
	{
		*nxtchar = '\0';
		*nxtobject = 0;
		return 0;
	}
	
	return FormKeybd (tree, object, obnext, ochar, kstate, 
		nxtobject, nxtchar);
}

/* neuen Wildcard fÅr Window eingeben
 */
void doWildcard (WindInfo *wp)
{
	DIALINFO d;
	LISTINFO L;
	LISTSPEC list[MAX_WILD + 1];
	word retcode, i, exit, draw, clicks;
	char *edit_text = pwildbox[WILDTEXT].ob_spec.tedinfo->te_ptext;
	char orig_pattern[WILDLEN + 1];
	
	if (wp->get_display_pattern == NULL)
		return;
		
	memcpy (&mypattern[1], wildpattern, sizeof (wildpattern));
	strcpy (mypattern[0], "*");
	wp->get_display_pattern (wp, edit_text);
	strcpy (orig_pattern, edit_text);

	wnr = -1;
	for (i = 0; i <= MAX_WILD; ++i)
	{
		list[i].entry = mypattern[i];
		if (i)
			list[i-1].next = &list[i];
		
		if (wnr >= 0 || strcmp (mypattern[i], edit_text))
			list[i].flags.selected = 0;
		else
		{
			wnr = i;
			list[i].flags.selected = 1;
		}
	}
	
	list[MAX_WILD].next = NULL;

	if (wnr < 0)
	{
		wnr = 0;
		list[0].flags.selected = 1;
	}

	setDisabled (pwildbox, WILDEDIT, wnr == 0);
	
	DialCenter (pwildbox);
	
	ListStdInit (&L, pwildbox, WILDB, WILDBG, drawWildcard, 
				list, 0, wnr, 1);
	ListInit(&L);

	DialStart (pwildbox,&d);
	DialDraw (&d);
	ListDraw (&L);

	do
	{
		int edit_object = WILDTEXT;
		
		draw = exit = FALSE;
		
		FormSetFormKeybd (_UpDown);
		retcode = DialDo (&d, &edit_object);
		clicks = (retcode & 0x8000)? 2 : 1;
		retcode &= 0x7FFF;
		
		switch (retcode)
		{
			case WILDB:
			case WILDBG:
				new_nr = (word)ListClick (&L, clicks);
				/* kein break! */

			case 0:
				if (!retcode)
				{
					list[wnr].flags.selected = 0;
					ListInvertEntry (&L, wnr);
					list[new_nr].flags.selected = 1;
					ListInvertEntry (&L, new_nr);
					ListScroll2Selection (&L);
					ListDraw (&L);
				}
		
				if (clicks == 2)
					retcode = WILDEDIT;

				if (wnr != new_nr && new_nr >= 0)
				{
					if ((new_nr ==  0) || (wnr == 0))
					{
						setDisabled (pwildbox, WILDEDIT, new_nr == 0);
						fulldraw (pwildbox, WILDEDIT);
					}
					
					wnr = new_nr;

					strcpy (edit_text, mypattern[wnr]);
					fulldraw (pwildbox, WILDTEXT);
				}
				break;

			case WILDEDIT:
				draw = TRUE;
				break;

			default:
				draw = exit = TRUE;
				break;
		}
		
		if (retcode == WILDEDIT)
		{
			if ((wnr > 0) && (wnr <= MAX_WILD) 
				&& (editWildcard (mypattern[wnr])))
			{
				ListExit (&L);
				ListInit (&L);
				ListScroll2Selection (&L);
				ListDraw (&L);
			}
		}

		if (draw)
		{
			setSelected (pwildbox, retcode, FALSE);
			fulldraw (pwildbox, retcode);
		}
	} 
	while(!exit);

	ListExit (&L);
	DialEnd (&d);
	
	
	if ((retcode == WILDQUIT))
	{
		if (!*edit_text)
			strcpy (edit_text, "*");
			
		if (strcmp (edit_text, orig_pattern))
		{
			char *ct;
			
			ct = strchr (edit_text, '\\');
			if(ct)
				*ct = '\0';			/* kein Backslash im Wildcard */
			
			wp->set_display_pattern (wp, edit_text);	
		}
		
		for (i = 0; i < MAX_WILD; i++)
		{
			strcpy (wildpattern[i], mypattern[i + 1]);
		}
	}
}


int FilterFile (const char *wcard, const char *fname, 
	int case_sensitiv)
{
	char pattern[WILDLEN];
	char *cp;
	
	strcpy (pattern, wcard);
	cp = strtok (pattern, ",;");
	while (cp)
	{
		if (wildmatch (fname, cp, case_sensitiv))
			return TRUE;
			
		cp = strtok (NULL, ",;");
	}
	
	return FALSE;
}

void FileWindowSpecials (WindInfo *wp, int mx, int my)
{
	int half;
	
	if (my >= wp->work.g_y || my < wp->wind.g_y)
		return;

	(void)mx;
	half = wp->wind.g_y + ((wp->work.g_y - wp->wind.g_y) / 2);

	if (my < half)
	{
		/* Hier sind wir _wahrscheinlich_ in der Titelzeile des
		 * Fensters. Ganz sicher kann man da nicht sein, aber wenn
		 * nicht, geschieht auch nichts schlimmes...
		 */
		
	}
	else
		doWildcard (wp);
}

