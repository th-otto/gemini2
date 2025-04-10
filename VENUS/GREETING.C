/*
 * @(#) Gemini\greeting.c
 * @(#) Stefan Eissing, 14. M„rz 1994
 *
 * description: functions to greet user
 *
 * jr 27.6.96
 */

#include <aes.h>
#include <flydial\flydial.h>

#include "vs.h"
#include "greeting.h"
#include "util.h"
#include "..\common\cookie.h"

#include "..\mupfel\mglob.h"

#include "hello.rsh"

static int whandle = -1;

static void
blinzeln (word start)
{
	word drawnr, hidenr, obx, oby;
	
	drawnr = start? BLINK : NORM;
	hidenr = start? NORM : BLINK;

	setHideTree (rs_object, hidenr, TRUE);
	setHideTree (rs_object, drawnr, FALSE);
	
	objc_offset (rs_object, drawnr, &obx, &oby);
	objc_draw (rs_object, 0, 1, obx, oby,
		rs_object[drawnr].ob_width,
		rs_object[drawnr].ob_height);
}

static void
fixTree (void)
{
	OBJECT *tree = rs_trindex[HTREE];
	int i;
	
	for (i = 0; ; ++i)
	{
		rsrc_obfix (tree, i);
		if (tree[i].ob_flags & LASTOB) break;
	}
	
	rs_object[BLINK].ob_flags |= HIDETREE;
}

/* jr: get right machine icon */

ICONBLK *
GetMachineIcon (void)
{
	static int CASETable[] =
	{
		IC_FALCO,
		IC_FALCO,
		IC_MEGA,
		IC_BOOK,
		IC_TT,
		IC_BOOK,
		IC_TOWER,
	};

	int icon = IC_MEGA;
	long machine;

	if (CookiePresent ('CASE', &machine)
		&& machine >= 0L && machine < DIM(CASETable))
	{
		icon = CASETable[machine];
	}
	else if (CookiePresent ('_MCH', &machine))
	{
		switch ((int)(machine >> 16))
		{
			case 0:
				icon = IC_MEGA;
				break;
							
			case 1:
				switch ((int)(machine & 0xffffL))
				{
					case 0x0001:
						icon = IC_BOOK;
						break;
					case 0x0010:
						icon = IC_TT;
						break;
					case 0x0100:
						icon = IC_FALCO;
						break;
				}
				break;
				
			case 2:
				icon = IC_TT;
				break;

			case 3:
				icon = IC_FALCO;
				break;
		}
	}

	return rs_object[icon].ob_spec.iconblk;
}


static int
displayIcon (void)
{
	word obx, oby, obw, obh;
	
	fixTree ();
	
	rs_object[IC_TT].ob_spec.iconblk = GetMachineIcon ();
	rs_object[HELLOVER].ob_spec.tedinfo->te_ptext = "Version "MUPFELVERSION"";

	form_center (rs_object, &obx, &oby, &obw, &obh);

	whandle = wind_create (0, obx, oby, obw, obh);
	if (whandle <= 0) return FALSE;
		
	if (wind_open (whandle, obx, oby, obw, obh))
	{
		objc_draw (rs_object, 0, 10, obx, oby, obw, obh);
		return TRUE;
	}
	else
	{
		wind_delete (whandle);
		whandle = -1;
		return FALSE;
	}
}

static void
hideIcon (void)
{
	wind_close (whandle);
	wind_delete (whandle);
	whandle = -1;
}

void
Greetings (void)
{
	static word Times = 0;

	WindUpdate (BEG_UPDATE);

	switch (Times)
	{
		case 0:
			if (displayIcon ())
				++Times;
			else
				Times = 99;
			break;

		case 1:
			blinzeln (TRUE);
			++Times;
			break;

		case 2:
			blinzeln (FALSE);
			++Times;
			break;

		case 3:
			hideIcon ();
			++Times;
			break;
	}
	
	WindUpdate (END_UPDATE);
	return;
}

void
ExitGreetings (void)
{
	if (whandle >= 0) hideIcon ();
}
