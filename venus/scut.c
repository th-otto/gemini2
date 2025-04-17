/*
 * @(#) Gemini\scut.c
 * @(#) Stefan Eissing, 18. August 1992
 *
 * description: handles dialog for Shortcuts
 * 				in venus
 */

#include <string.h>
#include <flydial\flydial.h>

#include "shortcut.rh"

#include "vs.h"

#include "window.h"
#include "dispatch.h"
#include "iconinst.h"
#include "redraw.h"
#include "select.h"
#include "scancode.h"
#include "scut.h"
#include "util.h"


/* externals
 */
extern OBJECT *pshortcut;

static struct
{
	word obj;
	word scut;
} scuttab[21] = {
{SHORTNO, SCUT_NO},
{SHORT0, SCUT_0},
{SHORT1, SCUT_1},
{SHORT2, SCUT_2},
{SHORT3, SCUT_3},
{SHORT4, SCUT_4},
{SHORT5, SCUT_5},
{SHORT6, SCUT_6},
{SHORT7, SCUT_7},
{SHORT8, SCUT_8},
{SHORT9, SCUT_9},
{SHF1, SCUT_F1},
{SHF2, SCUT_F2},
{SHF3, SCUT_F3},
{SHF4, SCUT_F4},
{SHF5, SCUT_F5},
{SHF6, SCUT_F6},
{SHF7, SCUT_F7},
{SHF8, SCUT_F8},
{SHF9, SCUT_F9},
{SHF10, SCUT_F10},
};

word Object2SCut(word index)
{
	int i;
	
	for (i = 0; i < DIM(scuttab); ++i)
	{
		if (scuttab[i].obj == index)
			return scuttab[i].scut;
	}

	return SCUT_NO;	
}

word SCut2Object(word shortcut)
{
	int i;
	
	for (i = 0; i < DIM(scuttab); ++i)
	{
		if (scuttab[i].scut == shortcut)
			return scuttab[i].obj;
	}

	return SHORTNO;	
}

OBSPEC SCut2Obspec(word shortcut)
{
	word index;
	
	index = SCut2Object(shortcut);
	return pshortcut[index].ob_spec;
}

word SCutSelect (OBJECT *tree, word index, 
				word newshort, word origshort, word cycle_obj)
{
	word retcode, origindex;
	word restore = FALSE;

	origindex = SCut2Object(origshort);
	if (isDisabled(pshortcut, origindex))
	{
		setDisabled(pshortcut, origindex, FALSE);
		restore = TRUE;
	}
	
	if (cycle_obj > 0)
		retcode = JazzCycle (tree, index, cycle_obj, pshortcut, 1);
	else
		retcode = JazzId (tree, index, -1, pshortcut, 1);
	
	if (restore)
		setDisabled(pshortcut, origindex, TRUE);
	
	if (retcode > -1)
	{
		return Object2SCut(retcode);
	}
	else
		return newshort;
}

word DoSCut (word kstate, word kreturn)
{
	word shortcut, i;
	char c;
	
	if ((kreturn >= F1) && (kreturn <= F10))
	{
		shortcut = SCUT_F1 + ((kreturn - F1) / 256);
	}
	else
	{
		if (!(kstate & K_ALT))
			return FALSE;
	
		c = kreturn & 0xFF;
		if (c && (strchr ("0123456789", c) == NULL))
			return FALSE;
	
		switch (((uword)kreturn) >> 8)
		{
			case 112:			/* Alt-0 */
			case 129:
				shortcut = SCUT_0;
				break;
			case 109:			/* Alt-1 */
			case 120:
				shortcut = SCUT_1;
				break;
			case 110:			/* Alt-2 */
			case 121:
				shortcut = SCUT_2;
				break;
			case 111:			/* Alt-3 */
			case 122:
				shortcut = SCUT_3;
				break;
			case 106:			/* Alt-4 */
			case 123:
				shortcut = SCUT_4;
				break;
			case 107:			/* Alt-5 */
			case 124:
				shortcut = SCUT_5;
				break;
			case 108:			/* Alt-6 */
			case 125:
				shortcut = SCUT_6;
				break;
			case 103:			/* Alt-7 */
			case 126:
				shortcut = SCUT_7;
				break;
			case 104:			/* Alt-8 */
			case 127:
				shortcut = SCUT_8;
				break;
			case 105:			/* Alt-9 */
			case 128:
				shortcut = SCUT_9;
				break;
			default:
				return FALSE;
		}
	}
	
	for (i = NewDesk.tree[0].ob_head; i > 0; 
		i = NewDesk.tree[i].ob_next)
	{
		IconInfo *pii;
		
		if (isHidden (NewDesk.tree, i)
			|| ((NewDesk.tree[i].ob_type & 0x00ff) != G_USERDEF))
			continue;
		
		pii = GetIconInfo (&NewDesk.tree[i]);
		if (!pii)
			continue;
		
		if (pii->shortcut == shortcut)
		{
			static word lastcut = SCUT_NO;
			static long lasttime = 0L;
			word selected, done = FALSE;
			long now;
			
			now = GetHz200 ();
			selected = isSelected (NewDesk.tree, i);
			
			if (selected && (shortcut == lastcut))
			{
				long maxtime, difftime;
				word clickspeed;
				
				clickspeed = evnt_dclick (3, 0);
				maxtime = (5 - clickspeed) * 40;
				difftime = now - lasttime;
				if (difftime < 0)
					difftime = -difftime;
									
				if (difftime <= maxtime)	/* Doppelklick */
				{
					done = TRUE;
					simDclick (kstate & ~K_ALT);
				}
			}
			
			if (!done)
			{
				if (kstate & (K_LSHIFT|K_RSHIFT))
				{
					if (selected)
						DeselectObject (&wnull, i, TRUE);
					else
						SelectObject (&wnull, i, TRUE);
				}
				else
				{
					DeselectAllObjects ();
					SelectObject (&wnull, i, TRUE);
				}
				
				flushredraw ();
			}
			
			lasttime = now;
			lastcut = shortcut;
			
			return TRUE;
		}
	}
	
	return FALSE;
}

word SCutInstall (word shortcut)
{
	word index;
	
	if (shortcut == SCUT_NO)
		return TRUE;
		
	index = SCut2Object(shortcut);
	if ((index >= 0) && !isDisabled(pshortcut, index))
	{
		setDisabled(pshortcut, index, TRUE);
		return TRUE;
	}
	return FALSE;
}

word SCutRemove(word shortcut)
{
	word index;
	
	if (shortcut == SCUT_NO)
		return TRUE;
		
	index = SCut2Object(shortcut);
	if ((index >= 0) && isDisabled(pshortcut, index))
	{
		setDisabled(pshortcut, index, FALSE);
		return TRUE;
	}
	return FALSE;
	
}

