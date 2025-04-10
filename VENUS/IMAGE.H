/*
 * @(#) image.h
 * @(#) Guido Vollbeding & Stefan Eissing, 11. Juli 1992
 *
 * Laden eines GEM-Images
 */

#ifndef IMAGE__
#define IMAGE__

typedef struct view
{
	int	vdi_handle;
	int	vdi_planes;
	int planes;
	char	path[128];		/* Datei-Pfad (Fenstertitel).		*/

	int work[4];			/* Zu fÅllendes Rechteck 			*/
	int w, h;				/* Hîhe und Breite des Bildes 		*/
	
	void (*draw) (struct view *tv, int *clip, int center, int pattern);
	void (*switch_colors) (struct view *tv, int to_original);
	void (*free) (struct view *tv);

	int	ximage_flag;
	long color_offset;
	char orig_colors_set;
	char alternate_color;	/* alternative Farbe fÅr Monochrom */ 
	MFDB	mfdb;
	int   width, height;
	int orig_colors[256][3];/* Vorher eingestellte Farben */
	rgb_list[];				/* Farbpalette.				*/
}
IMAGEVIEW;


IMAGEVIEW *LoadImage (const char *filename, int vdi_handle);

#endif	/* !IMAGE__ */