/*
 * @(#) Gemini\windstak.c
 * @(#) Stefan Eissing, 28. Februar 1993
 *
 * description: keep stack of windows
 */

#include <stddef.h>
#include <stdio.h>
#include <aes.h>

#include "vs.h"

#include "windstak.h"
#include "myalloc.h"
#include "util.h"
#include "fileutil.h"


/* internals
 */
typedef struct windBox
{
	struct windBox *nextbox;
	word kind;
	GRECT box;
} WindBox;

static WindBox *boxList = NULL;

/*
 * push a WindBox onto the stack
 */
int PushWindBox (const GRECT *box, word kind)
{
	WindBox *pwb;
	
	if ((pwb = malloc (sizeof (WindBox))) != NULL)
	{
		pwb->nextbox = boxList;
		boxList = pwb;
		pwb->box = *box;
		pwb->kind = kind;
	}
	
	return (pwb != NULL);
}

/*
 * pop a WindBox from the stack
 */
int PopWindBox (GRECT *box, word kind)
{
	WindBox *pwb, **prev;

	if (boxList == NULL)
		return FALSE;

	prev = &boxList;
	pwb = boxList;
	while (pwb != NULL)
	{
		if (pwb->kind == kind)
			break;

		prev = &pwb->nextbox;
		pwb = pwb->nextbox;
	}
	
	if (pwb != NULL)
	{
		*prev = pwb->nextbox;
		*box = pwb->box;
		free (pwb);
		
		return TRUE;
	}

	return FALSE;
}

/*
 * write recursiv Box-coordinates in file fp
 */
static int writeRekursivBoxes (WindBox *pwb, MEMFILEINFO *mp, 
	char *buffer)
{
	word x,y,w,h;
	
	if (pwb)
	{
		if (!writeRekursivBoxes (pwb->nextbox, mp, buffer))
			return FALSE;
			
		x = (word)scale123 (ScaleFactor, pwb->box.g_x - Desk.g_x, Desk.g_w);
		w = (word)scale123 (ScaleFactor, pwb->box.g_w - Desk.g_x, Desk.g_w);
		y = (word)scale123 (ScaleFactor, pwb->box.g_y - Desk.g_y, Desk.g_h);
		h = (word)scale123 (ScaleFactor, pwb->box.g_h - Desk.g_y, Desk.g_h);

		sprintf (buffer, "#B@%d@%d@%d@%d@%d", x, y, w, h, pwb->kind);
		
		return !mputs (mp, buffer);
	}
	
	return TRUE;
}

/*
 * write all Boxes in File fp
 */
int WriteBoxes (MEMFILEINFO *mp, char *buffer)
{
	return writeRekursivBoxes (boxList, mp, buffer);	
}

void FreeWBoxes (void)
{
	WindBox *pb, *cp;
	
	pb = boxList;
	while (pb != NULL)
	{
		cp = pb;
		pb = pb->nextbox;
		free (cp);
	}
	
	boxList = NULL;
}
