/*
 * @(#) Gemini\color.c
 * @(#) Stefan Eissing, 20. August 1992
 *
 * description: functions to color icons
 */
 
#include <stdlib.h>
#include <string.h>
#include <flydial\flydial.h>

#include "vs.h"
#include "colorbox.rh"

#include "color.h"
#include "util.h"

/* externals
 */
extern OBJECT *pcolorbox;

/* internals
 */
static char color2IndexTable[] =
{
	CLWHITE,
	CLBLACK,
	CLRED,
	CLGREEN,
	CLBLUE,
	CLCYAN,
	CLYELLOW,
	CLMAGENT,
	CLLWHITE,
	CLLBLACK,
	CLLRED,
	CLLGREEN,
	CLLBLUE,
	CLLCYAN,
	CLLYELLO,
	CLLMAGEN,
};

static word color2Index (unsigned char color, word foreground)
{
	if (foreground)
		color = color >> 4;
	else
		color &= 0x0F;
	
	return color2IndexTable[color];
}

static void color2Obspecs (char color, OBSPEC *fore, OBSPEC *back)
{
	word foreindex;
	word backindex;
	
	foreindex = color2Index (color, TRUE);
	backindex = color2Index (color, FALSE);
	
	*fore = pcolorbox[foreindex].ob_spec;
	*back = pcolorbox[backindex].ob_spec;
}

static char index2Color (char color, word retcode, word foreground)
{
	int index;
	
	for (index = 0; index < 16; ++index)
	{
		if (color2IndexTable[index] == retcode)
			break;
	}
	
	if (index >= 16)
		index = foreground? 1 : 0;

	if (foreground)
		return ((index << 4) | (color & 0x0F));
	else
		return (index | (0xF0 & color));
}

void ColorSetup(char color, OBJECT *tree, word foresel, word forebox,
				word backsel, word backbox)
{
	char fcolor, bcolor;
	OBSPEC fobs, bobs;
	
	fcolor = color >> 4;
	bcolor = color & 0x0F;
	
	color2Obspecs(color, &fobs, &bobs);
	
	if (foresel >= 0)
		tree[foresel].ob_spec = fobs;

	if (forebox >= 0)
		tree[forebox].ob_spec.obspec.interiorcol = fcolor;

	if (backsel >= 0)
		tree[backsel].ob_spec = bobs;

	if (backbox >= 0)
		tree[backbox].ob_spec.obspec.interiorcol = bcolor;
}

char ColorSelect(char color, word foreground, OBJECT *tree,
				 word selindex, word boxindex, word cycle_obj)
{
	word retcode;
	OBSPEC obspec;

	obspec = tree[selindex].ob_spec;

	retcode = JazzSelect (tree, selindex, -1, pcolorbox,
					 TRUE, (cycle_obj > 0)? -2:0, (long *)&obspec);
	
	if (retcode > -1)
	{
/* jr: dieser Kram fhrte dazu, daž bei den Farbpopups das
   Cyclen nicht funktioniert hat.

		if ((cycle_obj > 0) && (retcode > 0))
			++retcode;
*/		
		color = index2Color (color, retcode, foreground);

		if (foreground)
			ColorSetup (color, tree, selindex, boxindex, -1, -1);
		else
			ColorSetup (color, tree, -1, -1, selindex, boxindex);

		fulldraw (tree, selindex);
	}

	return color;
}

char ColorMap (char truecolor)
{
	char color;
	
	color = ((unsigned char)truecolor) >> 4;
	truecolor &= 0x0F;
	
	if (color > number_of_settable_colors)
		color = 1;
		
	if (truecolor > number_of_settable_colors)
		truecolor = 0;

	return (color << 4) | truecolor;
}