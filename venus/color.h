/*
 * @(#) Gemini\color.h
 * @(#) Stefan Eissing, 31. Mai 1992
 *
 * description: functions to color icons
 */
 
#ifndef G_color__
#define G_color__

void ColorSetup (char color, OBJECT *tree, word foresel, word forebox,
				word backsel, word backbox);

char ColorSelect (char color, word foreground, OBJECT *tree,
				 word selindex, word boxindex, word cycle_obj);

char ColorMap (char truecolor);

#endif