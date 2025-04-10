/*
 * @(#) Gemini\image.c
 * @(#) Guido Vollbeding & Stefan Eissing, 2. November 1992
 *
 * Laden eines (X)IMG-Bildes
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <aes.h>
#include <vdi.h>
#include <tos.h>

#include "image.h"



#ifndef FALSE
#define FALSE 	(0)
#define TRUE	(!FALSE)
#endif

#ifndef min
#define min(a,b)	(((a) < (b))? (a) : (b))
#endif


#define X_MAGIC  "XIMG"
#define FM_MAGIC "FORM"
#define FM_IMAGE "ILBM"
#define BM_MAGIC "BMHD"
#define CM_MAGIC "CMAP"
#define BO_MAGIC "BODY"

typedef struct
{
  int version, 
  	headlen, 
  	planes, 
  	pat_run,
    pix_width, 
    pix_height, 
    sl_width, 
    sl_height;
}
IMG_HEADER;


static int maxconv;
static unsigned char *Pli2Vdi, *Vdi2Pli;

static int defcols[][3] = {
				1000, 1000, 1000,
				1000,    0,    0,
				   0, 1000,    0,
				1000, 1000,    0, 
				   0,    0, 1000,
				1000,    0, 1000,
				   0, 1000, 1000,
				 666,  666,  666,
				 333,  333,  333,
				 666,    0,    0,
				   0,  666,    0,
				 666,  666,    0,
				   0,    0,  666,
				 666,    0,  666,
				   0,  666,  666,
				   0,    0,    0
			  };

static char	dithermatrix[] = {
	17, 61, 27, 51, 18, 63, 25, 49,
	41,  5, 38, 14, 42,  7, 37, 13,
	31, 55, 20, 56, 28, 52, 22, 59,
	34, 10, 44,  0, 32,  8, 46,  3,
	19, 62, 24, 48, 16, 60, 26, 50,
	43,  6, 36, 12, 40,  4, 39, 15,
	29, 53, 23, 58, 30, 54, 21, 57,
	33,  9, 47,  2, 35, 11, 45,  1
};


typedef struct
{
	unsigned char *pbuf, *fbuf, *ebuf, sbuf[4];
	int handle;
}
FBUF;

static void bufopen (FBUF *fp)
{
	long curr, len;
	
	curr = Fseek (0, fp->handle, 1);
	len = Fseek (0, fp->handle, 2) - curr;
	Fseek (curr, fp->handle, 0);
	
	if ((fp->fbuf = Malloc (len)) == 0)
	{
		if ((len = (long)Malloc (-1L)) > 9000L)
			fp->fbuf = Malloc (len - 8196);
		else
		{
			len = 4;
			fp->fbuf = fp->sbuf;
		}
	}

	fp->pbuf = fp->ebuf = fp->fbuf + len;
}

static int bufgetc (FBUF *fp)
{
	long count;
	
	if (fp->pbuf >= fp->ebuf)
	{
		fp->pbuf = fp->fbuf;
		if ((count = fp->ebuf - fp->fbuf) <= 0)
			return -1;
			
		fp->ebuf = fp->fbuf + Fread (fp->handle, count, fp->fbuf);
		if (fp->ebuf <= fp->fbuf)
			return -1;
	}
	
	return *fp->pbuf++;
}

static void bufclose (FBUF *fp)
{
	if (fp->fbuf != fp->sbuf)
		Mfree( fp->fbuf );
	Fclose( fp->handle );
}


static int get_pix (int vdi_handle, int *rgb)
{
  int pel[2];

  if (rgb)
  	vs_color (vdi_handle, 1, rgb);
  	
  *(long *)pel = 0;
  v_pmarker (vdi_handle, 1, pel);
  v_get_pixel (vdi_handle, 0, 0, pel, pel + 1);
  
  return *pel;
}

static long get_true (int vdi_handle, int vdi_planes, int *rgb)
{
  static int pxy[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  static long buf[32];
  static MFDB screen = { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	      check  = { buf, 32, 1, 2, 0, 0, 0, 0, 0 };

  vs_color (vdi_handle, 1, rgb );
  v_pmarker (vdi_handle, 1, pxy );
  check.fd_nplanes = vdi_planes;
  vro_cpyfm (vdi_handle, S_ONLY, pxy, &screen, &check );
  
  return *buf;
}


static long gray (int *rgb)
{
	return ((598L * rgb[0]) + (1174L * rgb[1]) 
		+ (228L * rgb[2]) + 15625) / 31250;
}

static int pli2vdi (int pli, int plimax)
{
	if (pli == plimax)
		return 1;
	if (pli > maxconv)
		return pli;
		
	return Pli2Vdi[pli];
}

static int vdi2pli (int vdi, int plimax)
{
	if (vdi == 1)
		return plimax;
	if (vdi > maxconv)
		return vdi;
		
	return Vdi2Pli[vdi];
}

static void initColorConversion (int vdi_handle, int vdi_planes)
{
	static char vdi2pli[] = { 0,15,1,2,4,6,3,5,7,8,9,10,12,14,11,13 };
	int i, k, pel[4];
	
	vs_clip	(vdi_handle, 0, pel);
	i = 255;
	if (vdi_planes < 8)
		i = (1 << vdi_planes) - 1;
	
	k = 15;
	if (i < k || vdi_planes > k)
		i = k;
	
	maxconv = i;

	do
	{
		if (maxconv <= 15)
			k = vdi2pli[i];
		else
		{
			vsm_color (vdi_handle, i);
			k = get_pix (vdi_handle, 0);
			if ((unsigned)maxconv < (unsigned)k)
				k = maxconv;
		}
	
		Vdi2Pli[i] = k;
		Pli2Vdi[k] = i;
	}
	while (--i >= 0);
}


static void switchImageColors (IMAGEVIEW *tv, int to_original)
{
	if (!to_original && !tv->orig_colors_set)
		return;
	
	if (tv->mfdb.fd_nplanes == 1 && tv->vdi_planes > 1)
	{
		/* Wir tun nix. Die Draw-Routine nimmt die
		 * richtige Farbe
		 */
	}
	else if (tv->vdi_planes < tv->planes || tv->vdi_planes > 8)
	{
		/* Bei mehr als 8 Planes haben wir wahrscheinlich ein
		 * 32K Modul und da k”nnen wir keine Farben einstellen.
		 * Nun gut, man k”nnte weitere Werte aus work_out abfragen,
		 * aber...
		 */
		return;
	}
	else
	{
		int colors, k, i, *rgb_list, rgb_get[3];
		int *rgb_orig;

		rgb_list = tv->rgb_list;
		rgb_orig = (int*)tv->orig_colors;
			
		if (*rgb_list++)
			return;

		if (!to_original)
			rgb_list = rgb_orig;

		colors = 1 << min (tv->planes, tv->vdi_planes);
		for (k = 0; k < colors; ++k)
		{
			int setcol;
			
			if (tv->color_offset && k == (colors-1))
				setcol = (int)(tv->color_offset - 1);
			else
				setcol = (int)(tv->color_offset + k);
	
			vq_color (tv->vdi_handle, setcol, 0, rgb_get);
			vs_color (tv->vdi_handle, setcol, rgb_list);
			rgb_list += 3;
			if (to_original && !tv->orig_colors_set)
			{
				for (i = 0; i < 3; ++i)
					*rgb_orig++ = rgb_get[i];
			}
		}
	}
	
	tv->orig_colors_set = to_original;
}


static void draw_image (IMAGEVIEW *tv, int *clip, int center, 
	int pattern)
{
	static int index[2] = { 1, 0 };
	int  pxy[8], curx, cury;
	int work[4];
	MFDB d;
	
	if (!tv->orig_colors_set)
		index[0] = tv->alternate_color;

	vs_clip (tv->vdi_handle, 1, clip);

	if (center)
	{
		if (pattern)
		{
			vsf_style (tv->vdi_handle, pattern);
			vsf_interior (tv->vdi_handle, 
				(pattern >= 7)? FIS_SOLID : FIS_PATTERN);
		}
		else
			vsf_interior (tv->vdi_handle, FIS_HOLLOW);
		vsf_color (tv->vdi_handle, tv->alternate_color);
		vswr_mode (tv->vdi_handle, MD_REPLACE);
		vr_recfl (tv->vdi_handle, clip);
		
		work[0] = tv->work[0] + 
			(((tv->work[2] - tv->work[0] + 1) - tv->width) / 2);
		work[1] = tv->work[1] + 
			(((tv->work[3] - tv->work[1] + 1) - tv->height) / 2);
		work[2] = work[0] + tv->width;
		work[3] = work[1] + tv->height;
	}
	else
		memcpy (work, tv->work, 4 * sizeof (int));
		
	pxy[0] = 0;
	pxy[1] = 0;
	pxy[2] = pxy[0] + tv->width - 1;
	pxy[3] = pxy[1] + tv->height - 1;

	for (curx = work[0]; curx < work[2]; curx += tv->width)
	{
		for (cury = work[1]; cury < work[3]; cury += tv->height)
		{
			memset (&d, 0, sizeof (MFDB));
		
			pxy[4] = curx;
			pxy[5] = cury;
			pxy[6] = curx + (pxy[2] - pxy[0]);
			pxy[7] = cury + (pxy[3] - pxy[1]);
		
			if (pxy[6] < clip[0] || pxy[7] < clip[1]
				|| pxy[4] > clip[2] || pxy[1] > clip[3])
				continue;
			
			if (tv->mfdb.fd_nplanes == 1)
			{
				vrt_cpyfm (tv->vdi_handle, MD_REPLACE, pxy, &tv->mfdb, 
					&d, index);
			}
			else 
			{
				vro_cpyfm (tv->vdi_handle, S_ONLY, pxy, &tv->mfdb, 
					&d );
			}
		}
	}
}


static void free_image (IMAGEVIEW *v)
{
	if (v->orig_colors_set)
		v->switch_colors (v, FALSE);
	Mfree (v);
	Vdi2Pli = Pli2Vdi = NULL;
}

static IMAGEVIEW *load_image (int fh, int planes, int height, 
	int width, int special, int headlen, int vdi_handle, 
	int vdi_planes)
{
	IMAGEVIEW *rv;
	char cdata, *line_buf, *endl_buf, *line_ptr;
	char *raster_ptr, *buf_ptr;
	int  idata, i, vrc, line;
	long linelen;
	long bufsize, planesize, rastsize;
	MFDB s;
	FBUF f;

	memset (&s, 0, sizeof (MFDB));
	
	/* Bildbreite in words
	 */
	s.fd_wdwidth = (width + 15) >> 4;
	
	/* Bildbreite in Bytes
	 */
	linelen = s.fd_wdwidth << 1;
	
	/* Breite und H”he in Pixel auf Bytes ausgerichtet
	 */
	s.fd_w = (width + 7) & -8;
	s.fd_h = (height + 7) & -8;
	
	/* Gr”že eines Zeilenbuffers
	 */
	bufsize = (long)linelen * planes;
	
	/* Gr”že eines Planebuffers
	 */
	planesize = (long)linelen * s.fd_h;
	
	/* Planes im Bild: entweder Monochrom oder gleich der
	 * Anzahl auf dem Bildschirm
	 */
	s.fd_nplanes = (planes == 1) ? 1 : vdi_planes;
	
	/* Gr”že des gesamten Bildes in Bytes
	 */
	rastsize = planesize * s.fd_nplanes;
	
	/* Anzahl der Farben pro Pixel im Bild
	 */
	idata = 1 << planes;
	
	i = 0;
	/* Mehr Planes im Bild als auf dem Bildschirm?
	 */
	if (s.fd_nplanes < planes)
	{ 
		i = idata;
		rastsize += planesize;
	}
	
	/* Gr”že der Farbpalette bei XIMG-Bildern
	 */
	line = idata * 6 + 2;
	
	/* Speicher fr View + Farbpalette + Bildgr”že holen
	 */
	if ((rv = Malloc (sizeof(IMAGEVIEW) + line + rastsize)) == 0)
	{ 
		Fclose( fh );
		return NULL;
	}
	
	/* Speicher fr Zeilenbuffer holen
	 */
	{
		size_t buflen;
		
		buflen = bufsize + (s.fd_nplanes < 16 ? i : 
			idata << (s.fd_nplanes > 16 ? 2 : 1));
			
		if ((buf_ptr = Malloc (buflen + 512)) == 0)
		{ 
			Mfree (rv);
			Fclose (fh);
			return NULL;
		}
	
		memset (rv, 0, sizeof(IMAGEVIEW) + line + rastsize);
		memset (buf_ptr, 0, buflen + 512);
		Vdi2Pli = ((unsigned char *)buf_ptr) + buflen;
		Pli2Vdi = Vdi2Pli + 256;
	}
	

	if (s.fd_nplanes <= 8 && s.fd_nplanes > planes && planes > 1)
	{
		rv->color_offset = (1L << s.fd_nplanes) - (1L << planes);
	}

	initColorConversion (vdi_handle, vdi_planes);
	
	rv->width = width;
	rv->height = height;
	width = (int)(headlen ? (s.fd_w >> 3) : linelen);
	line_buf = (char *)rv->rgb_list;
	raster_ptr = line_buf + line;
	endl_buf = buf_ptr + bufsize;
	--idata;

	/* Zeigt an, ob zuviele Planes im Bild sind
	 */
	vrc = i;
	line = min (idata, 15);
	
	while (--i >= 0)
		endl_buf[i] = (i < line) ? gray (defcols[i]) : 0;
  	
	i = 0;
	/* Vielleich ein XIMG Bild?
	 */
	if (headlen >= 10)
	{
		Fread (fh, 4, line_buf);
		if (strncmp (line_buf, X_MAGIC, 4) == 0)
		{
			/* Es ist ein XIMG-Bild!
			 * Lese das Farbmodell.
			 */
			Fread (fh, 2, line_buf);
#ifndef __TOS__
			flipwords (line_buf, 2);
#endif
			line_buf += 2;
			
			/* Hier kommen jeweils 3 WORDS pro m”gliche
			 * Pixelfarbe
			 */
			do
			{
				long pos;
				
				/* line_ptr zeigt auf die Stelle in line_buf,
				 * an der die Farbdefinition fr Farbe i bei
				 * idata m”glichen Farben, hinsoll.
				 */
				if (i == idata && rv->color_offset)
					pos = (1 << planes) - 1;
				else
					pos = (pli2vdi ((int)(rv->color_offset + i), 
						idata) - rv->color_offset);
				
				if (pos < 0)
					pos = -pos;
				pos %= 256;
				line_ptr = line_buf + 6 * pos;
				
				/* Lese die Farbdefinition
				 */
				Fread (fh, 6, line_ptr);
#ifndef __TOS__
				flipwords (line_ptr, 6);
#endif
				/* Wenn zuviele Farben im Bild sind, dann
				 * wandle die Farbinformationen nach Grau(?)
				 */
				if (vrc)
				{
					endl_buf[vdi2pli (i, idata)] = 
						gray ((int *)line_ptr);
				}

				/* Wenn wir mehr als 8 Farbebenen haben, dann mssen
				 * wir den Wert eines Pixels anders bestimmen, da die
				 * normalen Zuordnungen nicht funktionieren.
				 */
				if (s.fd_nplanes >= 16)
				{
					if (s.fd_nplanes > 16)
					{
						((long *)endl_buf)[vdi2pli (i, idata)] = 
							get_true (vdi_handle, vdi_planes, 
								(int *)line_ptr);
					}
					else
					{
						((int *)endl_buf)[vdi2pli (i, idata)] = 
							get_pix (vdi_handle, (int *)line_ptr);
					}
				}
			}
			while (++i <= idata);
			
			/* Merke, daž wir ein XIMG hatten
			 */
			i = 1;
		}
	}
	
	/* Seeke zum Ende des Headers
	 */
	Fseek (headlen << 1, fh, 0);

 
	if (s.fd_nplanes >= 16)
	{
		if (s.fd_nplanes > 16)
		{
			if (i & 1)
				get_true (vdi_handle, vdi_planes, defcols[0]);
			else
			{
				long temp = 
					get_true (vdi_handle, vdi_planes, defcols[15]);
				
				do
				{
					((long *)endl_buf)[idata] = idata < line ? 
					get_true (vdi_handle, vdi_planes, defcols[idata]) : temp;
				}
				while (--idata >= 0);
			}
		}
		else
		{
			if (i & 1)
				get_pix (vdi_handle, defcols[0]);
			else
			{
				vrc = get_pix (vdi_handle, defcols[15]);
				
				do
				{
					((int *)endl_buf)[idata] =
					(idata < line) ? 
						get_pix (vdi_handle, defcols[idata]) : vrc;
				}
				while (--idata >= 0);
			}
		}
	    vs_color (vdi_handle, 1, defcols[15] );
	}

	/* Versuche einen weiteren Bildspeicher zu holen, in dem
	 * ein transform-form stattfinden kann. Ansonsten benutze
	 * denselben Speicherbereich.
	 */
	if (s.fd_nplanes >= 16 || (s.fd_addr = Malloc (rastsize)) == 0)
		s.fd_addr = raster_ptr;
	
	/* eigentlicher Bildspeicher
	 */
	rv->mfdb.fd_addr = raster_ptr;
	/* XIMG-Bild
	 */
	rv->ximage_flag = (i != 0L);
	/* Planes des Bildes
	 */
	rv->planes = planes;
	rv->mfdb.fd_stand = s.fd_stand = 1;
	rv->mfdb.fd_nplanes = s.fd_nplanes;
	rv->mfdb.fd_wdwidth = s.fd_wdwidth;
	rv->w = rv->mfdb.fd_w = s.fd_w;
	rv->h = rv->mfdb.fd_h = s.fd_h;
	rv->orig_colors_set = 0;
	rv->draw = draw_image;
	rv->switch_colors = switchImageColors;
	rv->free = free_image;
	rv->vdi_handle = vdi_handle;
	rv->vdi_planes = vdi_planes;
	
	f.handle = fh;
	bufopen (&f);

	/* Bildbereich initialisieren und Zeilenz„hler auf 0
	 */
	memset (raster_ptr = s.fd_addr, 0, rastsize);
	line = 0;
	
	for (;;)
	{
		vrc = 1;
		line_buf = buf_ptr;
		
		/* Bis zum Ende des Zeilenbuffers
		 */
		do
		{
			/* Setze Laufzeiger auf Zeile und erh”he den
			 * Zeilenzeiger
			 */
			line_ptr = line_buf;
			line_buf += width;
			
			/* Bis line_ptr >= line_buf
			 */
			do
			{
				/* Der Zeilenanfang entscheided ber die
				 * Art der Daten
				 */
				switch (idata = bufgetc (&f))
				{
					case 0:
					{
						/* pattern run:
						 * Wenn die Anzahl der Bytes > 0
						 */
						if ((idata = bufgetc( &f )) != 0)
						{
							char *rast_old = line_ptr;
							
							/* Lese bytes-per-pattern Bytes
							 * in den Zeilenbuffer
							 */
							for (i = special; --i >= 0;)
								*line_ptr++ = bufgetc( &f );

							/* Anzahl der Wiederholungen des
							 * Musters in der Zeile
							 */
							while (--idata > 0)
							{
							   for (i = special; --i >= 0;)
									  *line_ptr++ = *rast_old++;
							}
							break;
						}	
						
						/* vertical replication:
						 * vrc enth„lt die Zahl der Zeilenwiederholungen
						 */
						if (bufgetc( &f ) == 0xFF)
							vrc = bufgetc( &f );
						break;
					}
					
					case 0x80:
					{
						/* bit string: eazy
						 */
						idata = bufgetc( &f );
						while (--idata >= 0)
							*line_ptr++ = bufgetc( &f );
						break;
					}
						
					case -1:
					{
						/* unexpected end of file
						 */
						line_ptr = line_buf;
						break;
					}
					
					default:
					{
						/* solid run:
						 * Schwarze/Weiž x-mal eintragen
						 */
						i = idata & 0x7F;
						idata = (idata & 0x80) ? 0xFF : 0;
						
						while (--i >= 0)
							*line_ptr++ = idata;
						break;
					}
				}
			}
			while (line_ptr < line_buf);
		}
		while ((line_buf += width & 1) < endl_buf);
		/* Eine Zeile wurde eingelesen
		 */
		
#ifndef __TOS__
		flipwords( buf_ptr, bufsize );
#endif

		/* line ist der Z„hler fr die aktuelle Zeile
		 * vcr gibt die Anzahl der Zeilen an, die wir
		 * a) eingelesen haben (vrc == 1)
		 * b) zus„tzlich duplizieren mssen (vrc > 1)
		 *
		 * Auf jeden Fall mssen vrc Zeilen behandelt werden
		 */
		vrc += line;
		do
		{
			if (s.fd_nplanes >= 16)
			{
				line_ptr = buf_ptr;
				line_buf = line_ptr + linelen;
	
				do
				{
					cdata = 0x80;
					do
					{
						char *plane_ptr = line_ptr;

						idata = 1;
						i = 0;
						do
						{
							if (*plane_ptr & cdata)
								i |= idata;
							idata <<= 1;
						}
						while ((plane_ptr += linelen) < endl_buf);
						
						if (s.fd_nplanes < 24)
						{
							i <<= 1;
							*((int *)raster_ptr)++ = 
								*(int *)(endl_buf + i);
						}
						else
						{
							i <<= 2;
							if (s.fd_nplanes < 32)
							{
								plane_ptr = endl_buf + i;
								*raster_ptr++ = *plane_ptr++;
								*raster_ptr++ = *plane_ptr++;
								*raster_ptr++ = *plane_ptr++;
							}
							else
							{
								*((long *)raster_ptr)++ = 
									*(long *)(endl_buf + i);
							}
						}
					}
					while ((unsigned char)cdata >>= 1);
				}
				while (++line_ptr < line_buf);
			}
			else
			{
	      		/* Zeiger auf Anfang des Zeilenbuffers
				 */
				line_ptr = buf_ptr;
				idata = 1;
	
				do
				{
					if (--idata)
					{
						/* n-ter Durchlauf:
						 *
						 * line_buf auf die n„chste Plane im
						 * Bild setzen
						 */
						line_buf += planesize - linelen;
					}
					else
					{
						/* Erster Durchlauf:
						 *
						 * line_buf zeigt auf die erste Plane
						 * des Bild-Buffers.
						 * idata ist die Anzahl der Planes, in die
						 * die Zeile kopiert werden muž
						 */
						line_buf = raster_ptr;
						idata = s.fd_nplanes;
					}
					
					/* Kopiere die Zeile vom Zeilenbuffer in die
					 * entsprechende Plane des Bildes
					 */
					i = s.fd_wdwidth;
					do
					{
						*((int *)line_buf)++ |= *((int *)line_ptr)++;
					}
					while (--i);
				}
				while (line_ptr < endl_buf);
				/* Gesamte Zeile in die Planes des Bildes kopiert
				 */
	
				/* Wenn weniger Planes im Bild als auf dem Bildschirm
				 * sind, dann kopiere die gesetzten Pixel auf die
				 * weiteren Planes durch. 
				 */
				if (planes < s.fd_nplanes)
				{
					do
					{
						int mask;
		
						line_ptr = raster_ptr + i;
						mask = *(int *)line_ptr;
						idata = 1;
		
						do
						{
							line_ptr += planesize;
							if (idata < planes)
								mask &= *(int *)line_ptr;
							else if (rv->color_offset)
							{
								/* Wir arbeiten mir Offset in der
								 * Farbtabelle und kopieren alles
								 * durch. Die letzte Plane wird invers
								 * gesetzt, damit Schwarz nicht ver„ndert
								 * werden muž.
								 */
								if (idata == planes)
									*(int *)line_ptr = ~mask;
								else
									*(int *)line_ptr = ~0;
							}
							else
							{
								/* Wir arbeiten ohne Offset und kopieren
								 * nur die Farbe 1 (schwarz) durch.
								 */
								*(int *)line_ptr = mask;
							}
						}
						while (++idata < s.fd_nplanes);
					}
					while ((i += 2) < linelen);
				}
				else if (s.fd_nplanes < planes)
				{
					/* Es sind mehr Planes im Bild als auf dem
					 * Bildschirm. Nehme die letzte Plane und
					 * versuche das Ganze zu dithern
					 */
					line_buf = raster_ptr + rastsize - planesize;
					line_ptr = dithermatrix + ((line & 7) << 3);
	
					/* Einmal die Zeile durchgehen
					 */
					do
					{
						idata = 7;
						cdata = 1;
		
						do
						{
							char *plane_ptr = buf_ptr + i;
							int  val = 1, pix = 0;
		
							do
							{
								if (*plane_ptr & cdata)
									pix |= val;
						  		val <<= 1;
							}
							while ((plane_ptr += linelen) < endl_buf);
							
							if (endl_buf[pix] <= line_ptr[idata])
								line_buf[i] |= cdata;
							cdata <<= 1;
						}
						while (--idata >= 0);
					}
					while (++i < linelen);
				}
				
				/* raster_prt zeigt auf die n„chste Zeile.
				 * line wird incrementiert und wenn es height
				 * erreicht, haben wir das ganze Bild eingelesen
				 */
				raster_ptr += linelen;
			}
			
			if (++line == height)
			{
				/* Alle Zeilen sind eingelesen.
				 * Schlieže Datei und transformiere in des
				 * Ger„tespezifische Format
				 */
				bufclose( &f );
				if (s.fd_nplanes < 16)
					vr_trnfm (vdi_handle, &s, &rv->mfdb);
				
				line_ptr = s.fd_addr;
				if (s.fd_nplanes < planes)
				{
					(char *)(s.fd_addr) += rastsize - planesize;
					(char *)(rv->mfdb.fd_addr) += rastsize - planesize;
					rv->mfdb.fd_nplanes = s.fd_nplanes = 1;
					vr_trnfm( vdi_handle, &s, &rv->mfdb );
					rv->ximage_flag += 4;
				}

				/* Wenn wir einen extra transform-Buffer hatten,
				 * so free ihn. (What a deutsch!)
				 */
				if (s.fd_addr != rv->mfdb.fd_addr)
					Mfree( line_ptr );

				/* Free File buffer
				 */
				Mfree( buf_ptr );
				return rv;
			}
		}
		while (line < vrc);
		/* Es wurde die eingelesene Zeile plus deren Duplikate
		 * bearbeitet. Lese die n„chste Zeile
		 */
	} 
}


IMAGEVIEW *LoadImage (const char *filename, int vdi_handle)
{
	IMG_HEADER header;
	long fhandle;
	int fh;
	int vdi_planes;
	
	fhandle = Fopen (filename, FO_READ);
	
	if (fhandle < 0L) return NULL;
	
	fh = (int)fhandle;
	if ((Fread (fh, sizeof(IMG_HEADER), &header) != sizeof (IMG_HEADER))
		|| (header.planes > 8) || (header.planes < 1)
		|| (header.sl_height < 1) || (header.sl_width < 1)
		|| (header.headlen < (sizeof (IMG_HEADER)/2)))
	{
		Fclose (fh);
		return NULL;
	}
	
#ifndef __TOS__
	flipwords((char *)&header, sizeof(IMG_HEADER));
#endif

	{
		int workout[57];
		
		vq_extnd (vdi_handle, 1, workout);
		vdi_planes = workout[4];
	}
	
	return load_image (fh, header.planes, header.sl_height, 
		header.sl_width, header.pat_run, header.headlen,
		vdi_handle, vdi_planes);
}

