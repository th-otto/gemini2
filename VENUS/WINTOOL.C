/*
 * @(#) Gemini\wintool.c
 * @(#) Arnd Beissner (changed by gs & se), 31. Januar 1992
 *
 * description: functions for mupfel window
 */

#include <aes.h>
#include <vdi.h>

#include "vs.h"
#include "wintool.h"



static int work[4];
 
static int Rect[50][4];
static int MaxIndex = -1;

static int wasClipped = FALSE;

void WT_Clip (int handle1, int handle2, int pxy[4])
{
	if (wasClipped)
		return;
	
	if (MaxIndex == 0)
	{
		wasClipped = TRUE;
		
		if ((work[0] != pxy[0])
			||(work[1] != pxy[1])
			||(work[2] != pxy[2])
			||(work[3] != pxy[3]))
		{
			vs_clip (handle1, 1, pxy);
			vs_clip (handle2, 1, pxy);
		}
		else
		{
			vs_clip (handle1, 0, pxy);
			vs_clip (handle2, 0, pxy);
		}
	}
	else
	{
		vs_clip (handle1, 1, pxy);
		vs_clip (handle2, 1, pxy);
	}
}

char WT_GetRect (int index, int pxy[4])
{
	int *rc;
	
	if (index <= MaxIndex)
	{
		rc = Rect[index];
		pxy[0] = rc[0];
		pxy[1] = rc[1];
		pxy[2] = rc[2];
		pxy[3] = rc[3];
		return TRUE;
	}
	else
		return FALSE;
}

void WT_BuildRectList (int whandle)
{
	int recnum;
	int *rc;

	recnum = 0;
	rc = (int *)Rect;
	
	/* we haven't clipped this yet
	 */
	wasClipped = FALSE;
	
	/* get the work area
	 */
	wind_get (whandle, WF_WORKXYWH, &work[0], &work[1],
		&work[2], &work[3]);
	work[2] += work[0] - 1;
	work[3] += work[1] - 1;
	
	/* get the first Rectangle
	 */
	wind_get (whandle, WF_FIRSTXYWH, &rc[0], &rc[1], &rc[2], &rc[3]);

    while ((rc[2] != 0) || (rc[3] != 0)) 
    {
        if ((rc[0] < (Desk.g_x + Desk.g_w)) 
        	&& (rc[1] < (Desk.g_y + Desk.g_h)))
      	{
	        rc[2] += rc[0] - 1;
    	    rc[3] += rc[1] - 1;
    	    if (rc[2] >= (Desk.g_x + Desk.g_w)) 
    	    	rc[2] = (Desk.g_x + Desk.g_w) - 1;
    	    if (rc[3] >= (Desk.g_y + Desk.g_h))
    	    	rc[3] = (Desk.g_y + Desk.g_h) - 1;
    	    
    	    rc += 4;
        	recnum++;
       	}

        /* get the next Rectangle
         */
		wind_get (whandle, WF_NEXTXYWH, &rc[0], &rc[1], &rc[2], &rc[3]);
	}
	
	MaxIndex = recnum -1;
}


void WT_ClearRectList (int whandle)
{
	(void)whandle;
	MaxIndex = -1;
}