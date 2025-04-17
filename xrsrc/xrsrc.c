/******************************************************************************
 * XRSRC.C
 *
 *			Extended Resource-Manager. RSC-Files can now have up to
 *			4294967295 bytes length.
 *			You can modify this source to handle more than one RSC-File
 *			by calling the MLOCAL-Functions
 *
 *				rs_load(pglobal, re_lpfname);
 *				rs_free(pglobal);
 *				rs_gaddr(pglobal, re_gtype, re_gindex, re_gaddr);
 *				rs_sadd(pglobal, re_stype, re_sindex, re_saddr);
 *
 *			with an integer-pointer to a 15 int array which will be
 *			handled as single global-arrays for each RSC-File.
 *
 *			This Source is copyrighted material by
 *					Oliver Groeger
 *					Graf-Konrad-Str.25
 *					8000 Munich 40
 *					Germany
 *
 * Version  :  1.00
 * Date     :  Aug 15th 1991
 * Author   :  Oliver Groeger
 *
 * Version  :  1.10
 * Datum    :  27 Sept. 1992
 * Autor    :  Olaf Meisiek
 * énderung :  - xrsrc_load lÑdt jetzt auch das alte Atari-RCS-Format
 *             - TOS-unabhÑngige Farbicon-UnterstÅtzung hinzugefÅgt
 *
 *
 * Anm.     : Um diese Routinen benutzen zu dÅrfen, muû man im
 *            Besitz eines Interface Originals sein!
 *            
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>
#include "xaes.h"
#include "xrsrc.h"
#include "xvdi.h"

#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

/****** Compilerswitches ******************************************************/

#define GEMINI		1

#define COLOR_ICONS TRUE	/* FarbiconunterstÅtzung?              */
#define SAVE_MEMORY TRUE	/* Speicherplatz bei Farbicons sparen? */

#ifdef __TURBOC__
#pragma warn -sig		/* Warnung "Conversion may loose significant digits" ausschalten */
#endif

/****** TYPES ****************************************************************/

#ifndef __MYDIAL__
typedef struct
{
	USERBLK	ublk;
	_UWORD		old_type;
} OBBLK;
#endif

/****** VARIABLES ************************************************************/

static _WORD  xgl_wbox, xgl_hbox;
static GRECT xdesk;

#if COLOR_ICONS == TRUE
static _WORD xvdi_handle;
#endif

static _WORD		*rs_global;
static RSXHDR  *rs_hdr;
static RSXHDR	*hdr_buf;

#if COLOR_ICONS == TRUE
static _WORD    farbtbl[256][32];
static _ULONG   farbtbl2[32];
static _WORD    is_palette;
static _WORD    rgb_palette[256][4];
static _WORD    xpixelbytes;
static _WORD    xscrn_planes;
#endif

/****** FUNCTIONS ************************************************************/

static void rs_obfix      (OBJECT *rs_otree, _WORD rs_oobject);
static void rs_sglobal    (_WORD *base);
static _WORD rs_free       (_WORD *base);
static _WORD rs_gaddr      (_WORD *base, _WORD re_gtype, _WORD re_gindex, OBJECT **re_gaddr);
static _WORD rs_sadd       (_WORD *base, _WORD rs_stype, _WORD rs_sindex, OBJECT *re_saddr);
static _WORD rs_load       (_WORD *global, const char *fname);
static void *get_address  (_WORD type, _WORD index);
static void *get_sub      (_WORD index, _LONG offset, _WORD size);
static _WORD rs_read       (_WORD *global, const char *fname);
static void rs_fixindex   (_WORD *global);
static void do_rsfix      (_ULONG rs_size, _LONG file_size);
static void fix_treeindex (void);
static void fix_object    (void);
static void fix_tedinfo   (void);
static void fix_nptr      (_LONG index, _WORD ob_type);
static _WORD fix_ptr       (_WORD type, _LONG index);
static _WORD fix_long      (_LONG *lptr);
static void fix_chp       (_WORD *pcoord, _WORD flag);

static void do_ciconfix   (_ULONG header, RSXHDR *rsxhdr, _LONG rs_len);
static _WORD xadd_cicon    (CICONBLK *cicnblk, OBJECT *obj, _WORD nub);
static void draw_bitblk   (_WORD *p, _WORD x, _WORD y, _WORD w, _WORD h, _WORD num_planes, _WORD mode, _WORD *index);
static void xfix_cicon    (_UWORD *col_data, _LONG len, _WORD old_planes, _WORD new_planes, MFDB *s);
static void std_to_byte   (_UWORD *col_data, _LONG len, _WORD old_planes, _ULONG *farbtbl2, MFDB *s);
static void xrect2array   (const GRECT *rect, _WORD *array);
static _WORD test_rez      (void);
static void xfill_farbtbl (void);
static _WORD fill_cicon_liste  (_LONG *cicon_liste, _ULONG header, RSXHDR *rsxhdr);
static _WORD cdecl xdraw_cicon (PARMBLK *pb);

/*****************************************************************************/

_WORD xrsrc_load (const char *re_lpfname, _WORD *pglobal)
{
	return (rs_load (pglobal, re_lpfname));
}

/*****************************************************************************/

_WORD xrsrc_free (_WORD *pglobal)
{
	return (rs_free (pglobal));
}

/*****************************************************************************/

_WORD xrsrc_gaddr (_WORD re_gtype, _WORD re_gindex, void *re_gaddr, _WORD *pglobal)
{
	return (rs_gaddr (pglobal, re_gtype, re_gindex, re_gaddr));
}

/*****************************************************************************/

_WORD xrsrc_saddr (_WORD re_stype, _WORD re_sindex, void *re_saddr, _WORD *pglobal)
{
	return (rs_sadd (pglobal, re_stype, re_sindex, re_saddr));
}

/*****************************************************************************/

_WORD xrsrc_obfix (OBJECT *re_otree, _WORD re_oobject)
{
	rs_obfix (re_otree, re_oobject);

	return (TRUE);
}

/*****************************************************************************/

static void rs_obfix (OBJECT *rs_otree, _WORD rs_oobject)
{
	_WORD *coord;
	_WORD tmp = FALSE;
	_WORD count = 0;

	coord = &rs_otree[rs_oobject].ob_x;

	while (count++ < 4)
	{
		fix_chp (coord++, tmp);
		tmp = tmp ? FALSE : TRUE;
	}
}

/*****************************************************************************/

static void rs_sglobal (_WORD *base)
{
	rs_global = base;
	hdr_buf = (RSXHDR *)*(_LONG *)&rs_global[7];
	(_LONG)rs_hdr = (_LONG)hdr_buf + sizeof (RSXHDR);

	return;
}

/*****************************************************************************/

static _WORD rs_free (_WORD *base)

{
#if COLOR_ICONS == TRUE
	_WORD     i;
	CICON    *color_icn;
#endif

	rs_global = base;

#if COLOR_ICONS == TRUE

	if ((color_icn = (CICON *)*(_LONG *)&rs_global[2]) != NULL)
	{
		_WORD *start = *(_WORD **)&rs_global[7];
		_WORD *end = *(_WORD **)&rs_global[10];
		
		for (i = 0; i < rs_global[4]; i++)
		{
			if (color_icn[i].num_planes > 1)
			{
				if ((color_icn[i].col_data  < start)
					|| (color_icn[i].col_data  > end))
				{
					free (color_icn[i].col_data);
				}
					
				if ((color_icn[i].sel_data  < start)
					|| (color_icn[i].sel_data  > end))
				{
					free (color_icn[i].sel_data);
				}

				if (color_icn[i].sel_data == NULL
					&& ((color_icn[i].sel_mask  < start)
					|| (color_icn[i].sel_mask  > end)))
				{
					free (color_icn[i].sel_mask);
				}
			}
		}
		free (color_icn);
	}

	if (*(_LONG *)rs_global)
		free ((void *)*(_LONG *)rs_global);

#endif

	free ((RSXHDR *)*(_LONG *)&rs_global[7]);

	return (TRUE);
}

/*****************************************************************************/

static _WORD rs_gaddr (_WORD *base, _WORD re_gtype, _WORD re_gindex, OBJECT **re_gaddr)
{
	rs_sglobal (base);

	*re_gaddr = get_address (re_gtype, re_gindex);

	if (*re_gaddr == (OBJECT *)NULL)
		return (FALSE);

	return (TRUE);
}

/*****************************************************************************/

static _WORD rs_sadd (_WORD *base, _WORD rs_stype, _WORD rs_sindex, OBJECT *re_saddr)
{
	OBJECT *old_addr;

	rs_sglobal (base);

	old_addr = get_address (rs_stype, rs_sindex);

	if (old_addr == (OBJECT *)NULL)
		return (FALSE);

	*old_addr = *re_saddr;

	return (TRUE);
}

/*****************************************************************************/

static _WORD rs_load (_WORD *global, const char *fname)
{
	if (!rs_read (global, fname))
		return (FALSE);

	rs_fixindex (global);

	return (TRUE);
}

/*****************************************************************************/

static void *get_address (_WORD type, _WORD index)
{
	void *the_addr = (void *)NULL;
	union
	{
		void		*dummy;
		char		*string;
		OBJECT	**dpobject;
		OBJECT	*object;
		TEDINFO	*tedinfo;
		ICONBLK	*iconblk;
		BITBLK	*bitblk;
	} all_ptr;

	switch (type)
	{
		case R_TREE:
			all_ptr.dpobject = (OBJECT **)(*(long **)&rs_global[5]);
			the_addr = all_ptr.dpobject[index];
			break;

		case R_OBJECT:
			the_addr = get_sub (index, hdr_buf->rsh_object, sizeof(OBJECT));
			break;

		case R_TEDINFO:
		case R_TEPTEXT:
			the_addr = get_sub (index, hdr_buf->rsh_tedinfo, sizeof(TEDINFO));
			break;

		case R_ICONBLK:
		case R_IBPMASK:
			the_addr = get_sub (index, hdr_buf->rsh_iconblk, sizeof(ICONBLK));
			break;

		case R_BITBLK:
		case R_BIPDATA:
			the_addr = get_sub (index, hdr_buf->rsh_bitblk, sizeof(BITBLK));
			break;

		case R_OBSPEC:
			all_ptr.object = get_address(R_OBJECT, index);
			the_addr = &all_ptr.object->ob_spec;
			break;

		case R_TEPVALID:
		case R_TEPTMPLT:
			all_ptr.tedinfo = get_address(R_TEDINFO, index);
			if (type == R_TEPVALID)
				the_addr = &all_ptr.tedinfo->te_pvalid;
			else
				the_addr = &all_ptr.tedinfo->te_ptmplt;
			break;

		case R_IBPDATA:
		case R_IBPTEXT:
			all_ptr.iconblk = get_address(R_ICONBLK, index);
			if (type == R_IBPDATA)
				the_addr = &all_ptr.iconblk->ib_pdata;
			else
				the_addr = &all_ptr.iconblk->ib_ptext;
			break;

		case R_STRING:
			the_addr = get_sub (index, hdr_buf->rsh_frstr, sizeof (char *));
			the_addr = (void *)*(char *)the_addr;
			break;

		case R_IMAGEDATA:
			the_addr = get_sub (index, hdr_buf->rsh_imdata, sizeof (char *));
			the_addr = (void *)*(char *)the_addr;
			break;

		case R_FRIMG:
			the_addr = get_sub (index, hdr_buf->rsh_frimg, sizeof (char *));
			the_addr = (void *)*(char *)the_addr;
			break;

		case R_FRSTR:
			the_addr = get_sub (index, hdr_buf->rsh_frstr, sizeof (char *));
			break;
	}

	return (the_addr);
}

/*****************************************************************************/

static void *get_sub (_WORD index, _LONG offset, _WORD size)
{
	char *ptr = (char *)rs_hdr;

	ptr += offset;
	ptr += (index * size);

	return ((void *)ptr);
}

/*****************************************************************************/

static _WORD rs_read (_WORD *global, const char *fname)
{
	_WORD i, fh;
	char tmpnam[128];
  DTA  dta, *old_dta;
  _LONG size;
  _WORD ret = TRUE;

	strcpy (tmpnam, fname);

	if (!shel_find (tmpnam))
		return (FALSE);

	rs_global = global;

  old_dta = Fgetdta ();
  Fsetdta (&dta);
	if (Fsfirst (tmpnam, 0x10) == 0)
		size = dta.d_length;
	else
		size = 0;
	Fsetdta (old_dta);

	if (size > sizeof (RSHDR) && (fh = Fopen (tmpnam, 0)) > 0)
	{
		if ((hdr_buf = (RSXHDR *)malloc (size + sizeof (RSXHDR))) != NULL)
		{
			(_LONG)rs_hdr = (_LONG)hdr_buf + sizeof (RSXHDR);
			
			if (Fread (fh, size, rs_hdr) == size)
			{
				if (((RSHDR *)rs_hdr)->rsh_vrsn == 3)
					memcpy (hdr_buf, rs_hdr, sizeof (RSXHDR));
				else
					for (i = 0; i < sizeof (RSXHDR) / sizeof (_LONG); i++)
						((_ULONG *)hdr_buf)[i] = ((_UWORD *)rs_hdr)[i];

				do_rsfix (hdr_buf->rsh_rssize, size);

				if (size > hdr_buf->rsh_rssize + 72L)	/* Farbicons in der Resource? */
					do_ciconfix ((_ULONG)rs_hdr, hdr_buf, size);
			}
			else
				ret = FALSE;
		}
		else
			ret = FALSE;
		
		Fclose (fh);
	}
	else
		ret = FALSE;

	return (ret);
}

/*****************************************************************************/

static void rs_fixindex (_WORD *global)
{
	rs_sglobal (global);

	fix_object ();
}

/*****************************************************************************/

static void do_rsfix (_ULONG size, _LONG file_size)
{
	rs_global[7] = ((_LONG)hdr_buf >> 16) & 0xFFFF;
	rs_global[8] = (_LONG)hdr_buf & 0xFFFF;
	rs_global[9] = (_UWORD)size;
	*(char **)&rs_global[10] = ((char *)hdr_buf) + file_size;

	fix_treeindex ();
	fix_tedinfo ();

	fix_nptr (hdr_buf->rsh_nib - 1, R_IBPMASK);
	fix_nptr (hdr_buf->rsh_nib - 1, R_IBPDATA);
	fix_nptr (hdr_buf->rsh_nib - 1, R_IBPTEXT);

	fix_nptr (hdr_buf->rsh_nbb - 1, R_BIPDATA);
	fix_nptr (hdr_buf->rsh_nstring - 1, R_FRSTR);
	fix_nptr (hdr_buf->rsh_nimages - 1, R_FRIMG);
}

/*****************************************************************************/

static void fix_treeindex (void)
{
	OBJECT **adr;
	_LONG   count;

	count = hdr_buf->rsh_ntree - 1L;

	adr = get_sub (0, hdr_buf->rsh_trindex, sizeof (OBJECT *));

	rs_global[5] = ((_LONG)adr >> 16) & 0xFFFF;
	rs_global[6] = (_LONG)adr & 0xFFFF;

	while (count >= 0)
	{
		fix_long ((_LONG *)(count * sizeof (OBJECT *) + (_LONG)adr));
		count--;
	}
}

/*****************************************************************************/

static void fix_object (void)
{
	_WORD 	 count;
	OBJECT *obj;

	count = hdr_buf->rsh_nobs - 1;

	while (count >= 0)
	{
		obj = get_address (R_OBJECT, count);
		rs_obfix (obj, 0);
		if ((obj->ob_type & 0xff) != G_BOX && (obj->ob_type & 0xff) != G_IBOX && (obj->ob_type & 0xff) != G_BOXCHAR)
			fix_long ((_LONG *)&obj->ob_spec);

		count--;
	}
}

/*****************************************************************************/

static void fix_tedinfo()
{
	_LONG		count;
	TEDINFO *tedinfo;

	count = hdr_buf->rsh_nted - 1;

	while (count >= 0)
	{
		tedinfo = get_address (R_TEDINFO, count);

		if (fix_ptr (R_TEPTEXT, count))
			tedinfo->te_txtlen = strlen (tedinfo->te_ptext) + 1;

		if (fix_ptr (R_TEPTMPLT, count))
			tedinfo->te_tmplen = strlen (tedinfo->te_ptmplt) + 1;

		fix_ptr (R_TEPVALID, count);

		count--;
	}

	return;
}

/*****************************************************************************/

static void fix_nptr (_LONG index, _WORD ob_type)
{
	while (index >= 0)
		fix_long (get_address(ob_type, index--));
}

/*****************************************************************************/

static _WORD fix_ptr (_WORD type, _LONG index)
{
	return (fix_long (get_address (type, index)));
}

/*****************************************************************************/

static _WORD fix_long (_LONG *lptr)
{
	_LONG base;

	base = *lptr;
	if (base == 0L)
		return (FALSE);

	base += (_LONG)rs_hdr;

	*lptr = base;

	return (TRUE);
}

/*****************************************************************************/

static void fix_chp (_WORD *pcoord, _WORD flag)
{
	_WORD ncoord;

	ncoord = *pcoord & 0xff;

	if (!flag && ncoord == 0x50)
		ncoord = xdesk.g_w;											/* xdesk.w = Breite des Bildschirms in Pixel */
	else
		ncoord *= (flag ? xgl_hbox : xgl_wbox);	/* xgl_wbox, xgl_hbox = Zeichenbreite, Zeichenhîhe in Pixel */

	if (((*pcoord >> 8) & 0xff) > 0x80)
		ncoord += (((*pcoord >> 8) & 0xff) | 0xff00);
	else
		ncoord += ((*pcoord >> 8) & 0xff);

	*pcoord = ncoord;
}

/*****************************************************************************/
/* Farbicons fÅr aktuelle Auflîsung initialisieren                           */
/*****************************************************************************/

static void do_ciconfix (_ULONG header, RSXHDR *rsxhdr, _LONG rs_len)

{	_LONG   *cicon_liste;
	_WORD   i;
	OBJECT *obj;

	cicon_liste = (_LONG *)(*(_LONG *)(rsxhdr->rsh_rssize + (rsxhdr->rsh_rssize & 1L) + header + sizeof (_LONG)) + header);
	if ((_LONG)cicon_liste - header > rsxhdr->rsh_rssize && (_LONG)cicon_liste - header < rs_len)
	{
		if (fill_cicon_liste (cicon_liste, header, rsxhdr) != NIL)
		{
#if COLOR_ICONS == TRUE
			_WORD nub = 0, work_out [57], *palette;
			
			if (*(_LONG *)rs_global && *(_LONG *)&rs_global[2])
			{
				vq_extnd (xvdi_handle, TRUE, work_out);	/* Anzahl der Planes ermitteln */
				xscrn_planes = work_out[4];

				xpixelbytes = test_rez ();
				palette = (_WORD *)*(_LONG *)(rsxhdr->rsh_rssize + (rsxhdr->rsh_rssize & 1L) + header + 2 * sizeof (_LONG));
				if (palette != NULL)
				{	(_LONG)palette += header;
					memcpy (rgb_palette, palette, sizeof (rgb_palette));
					is_palette = TRUE;
				}
				else
					is_palette = FALSE;
				
				xfill_farbtbl ();
			
				for (i = 0; i < rsxhdr->rsh_nobs; i++)
				{
					obj = &((OBJECT *)(rsxhdr->rsh_object + header))[i];
					if ((obj->ob_type & 0xff) == G_CICON)
					{
						if (xadd_cicon ((CICONBLK *)obj->ob_spec, obj, nub++) == FALSE)
						{
							memset (&((CICON *)*(_LONG *)&rs_global[2])[nub-1], 0, sizeof (CICON));
							obj->ob_type = (obj->ob_type & 0xff00) | G_ICON;
						}
						obj->ob_spec -= header;
					}
				}
			}
			else
#endif
			{
				for (i = 0; i < rsxhdr->rsh_nobs; i++)
				{
					obj = &((OBJECT *)(rsxhdr->rsh_object + header))[i];
					if ((obj->ob_type & 0xff) == G_CICON)
					{	obj->ob_type = (obj->ob_type & 0xff00) | G_ICON;
						obj->ob_spec -= header;
					}
				}
			}
		}
	}
}

/*****************************************************************************/
/* Pointer fÅr Farbicons in der Resource initialisieren                      */
/*****************************************************************************/

static _WORD fill_cicon_liste (_LONG *cicon_liste, _ULONG header, RSXHDR *rsxhdr)

{	_WORD     i, i2, num = 0;
	char     *p;
	_LONG     iclen, num_cicon, ob, p2;
	CICONBLK *cblk;
	CICON    *cicon, *cold;
	OBJECT   *pobject;

	while (!cicon_liste[num])
		num++;
	
	if (cicon_liste[num] != -1L)
		return (NIL);

	cblk = (CICONBLK *)&cicon_liste[num+1];
	
	for (i = 0; i < num; i++)
	{
		cicon_liste[i] = (_LONG)cblk;
		p = (char *)&cblk[1];
		cblk->monoblk.ib_pdata = (_WORD *)p;
		iclen = cblk->monoblk.ib_wicon / 8 * cblk->monoblk.ib_hicon;
		p += iclen;
		cblk->monoblk.ib_pmask = (_WORD *)p;
		p += iclen;
		p2 = (_LONG)cblk->monoblk.ib_ptext;
		if (!p2 || header + p2 == (_LONG)p || p2 < rsxhdr->rsh_string || p2 > rsxhdr->rsh_rssize)
			cblk->monoblk.ib_ptext = (char *)p;
		else
			(_LONG)cblk->monoblk.ib_ptext = header + (_LONG)cblk->monoblk.ib_ptext;
		
		cicon = (CICON *)&p[12];
		p += 12L;
		cold = cicon;
		if ((num_cicon = (_LONG)cblk->mainlist) > 0)
		{
			cblk->mainlist = cicon;
	
			for (i2 = 0; i2 < num_cicon; i2++)
			{
				p = (char *)&cicon[1];
				cicon->col_data = (_WORD *)p;
				p += iclen * cicon->num_planes;
				cicon->col_mask = (_WORD *)p;
				p += iclen;
				if (cicon->sel_data != NULL)
				{	cicon->sel_data = (_WORD *)p;
					p += iclen * cicon->num_planes;
					cicon->sel_mask = (_WORD *)p;
					p += iclen;
				}
				cicon->next_res = (CICON *)p;
				cold = cicon;
				cicon = (CICON *)p;
			}
			cold->next_res = NULL;
		}
		cblk = (CICONBLK *)p;
	}

	if (num != NIL)
	{
		pobject = (OBJECT *)(header + rsxhdr->rsh_object);
  
	  for (ob = 0; ob < rsxhdr->rsh_nobs; ob++)
  		if ((pobject[ob].ob_type & 0xff) == G_CICON)
  			pobject[ob].ob_spec = cicon_liste[pobject[ob].ob_spec];

		if ((*(_LONG *)rs_global = (_LONG)malloc (num * sizeof (OBBLK))) != 0L)
			memset ((void *)*(_LONG *)rs_global, 0, num * sizeof (OBBLK));
		if ((*(_LONG *)&rs_global[2] = (_LONG)malloc (num * sizeof (CICON))) != 0L)
			memset ((void *)*(_LONG *)&rs_global[2], 0, num * sizeof (CICON));
		rs_global[4] = num;
	}
	
	return (num);
}

#if COLOR_ICONS == TRUE
/*****************************************************************************/
/* Icon ins gerÑteabhÑngige Format wandeln und ggf an andere Auflîsungen     */
/* anpassen                                                                  */
/*****************************************************************************/

static _WORD xadd_cicon (CICONBLK *cicnblk, OBJECT *obj, _WORD nub)

{	_WORD     x, y, line, xmax, best_planes, find_planes;
	CICON    *cicn, *color_icn, *best_icn = NULL;
  _LONG     len, *next;
  MFDB     d;
#if SAVE_MEMORY == TRUE
	CICON    *max_icn = NULL;
#endif

	len = cicnblk->monoblk.ib_wicon / 8 * cicnblk->monoblk.ib_hicon;

	color_icn = &((CICON *)*(_LONG *)&rs_global[2])[nub];

	best_planes = 1;
	if (xscrn_planes > 8)
		find_planes = 4;
	else
		find_planes = xscrn_planes;

	cicn = cicnblk->mainlist;
	next = (_LONG *)&cicnblk->mainlist;

	while (cicn != NULL)
	{
		*next = (_LONG)cicn;
		next = (_LONG *)&cicn->next_res;

#if SAVE_MEMORY == TRUE
		if (cicn->num_planes > xscrn_planes)
			max_icn = cicn;
#endif
		if (cicn->num_planes >= best_planes && cicn->num_planes <= find_planes)
		{
			best_planes = cicn->num_planes;
			best_icn = cicn;
		}
		cicn = cicn->next_res;
	}
	
	if (best_icn == NULL)		/* kein passendes Farbicon gefunden */
		return (FALSE);
	else
		*color_icn = *best_icn;

	if (best_planes > 1)
		color_icn->num_planes = xscrn_planes;
	else
		color_icn->num_planes = 1;
	
	/* Platz fÅr das gerÑteabhÑngige Format allozieren */
	if ((color_icn->col_data = malloc (len * color_icn->num_planes)) == NULL)
		return (FALSE);
	if (color_icn->sel_data)
	{
		if ((color_icn->sel_data = malloc (len * color_icn->num_planes)) == NULL)
		{
			free (color_icn->col_data);
			return (FALSE);
		}
	}

	if (best_planes > 1)
	{
		if (best_icn->sel_data == NULL)
		{
			/* Selected-Maske vorbereiten */
			if ((color_icn->sel_mask = malloc (len)) == NULL)
			{
				free (color_icn->col_data);
				if (color_icn->sel_data)
					free (color_icn->sel_data);
				return (FALSE);
			}
	
			xmax = cicnblk->monoblk.ib_wicon / 16;
	
			for (y = 0; y < cicnblk->monoblk.ib_hicon; y++)
			{
				line = y * xmax;
	
				for (x = 0; x < xmax; x++)
				{
					if (y & 1)
						color_icn->sel_mask[line + x] = best_icn->col_mask[line + x] & 0xaaaa;
					else
						color_icn->sel_mask[line + x] = best_icn->col_mask[line + x] & 0x5555;
				}
			}
		}
		
		d.mp  = color_icn->col_data;
		d.fwp = cicnblk->monoblk.ib_wicon;
		d.fh	= cicnblk->monoblk.ib_hicon;
		d.fww = d.fwp >> 4;
		d.ff	= TRUE;
		d.np	= xscrn_planes;
	
		xfix_cicon ((_UWORD *)best_icn->col_data, len, best_planes, xscrn_planes, &d);
		if (best_icn->sel_data)
		{	d.mp = color_icn->sel_data;
			xfix_cicon ((_UWORD *)best_icn->sel_data, len, best_planes, xscrn_planes, &d);
		}
	}
	else
	{
		memcpy (color_icn->col_data, best_icn->col_data, len);
		memcpy (color_icn->sel_data, best_icn->sel_data, len);
	}

#if SAVE_MEMORY == TRUE
	if (best_icn->num_planes < color_icn->num_planes && max_icn != NULL)
	{	if (best_icn->sel_data == NULL || max_icn->sel_data != NULL)
		{
			best_icn->col_data = max_icn->col_data;
			best_icn->sel_data = max_icn->sel_data;
			best_icn->num_planes = xscrn_planes;
		}
	}
	
	if (best_icn->num_planes == color_icn->num_planes)
	{
		memcpy (best_icn->col_data, color_icn->col_data, len * color_icn->num_planes);
		free (color_icn->col_data);
		color_icn->col_data = NULL;
		if (best_icn->sel_data != NULL)
		{	memcpy (best_icn->sel_data, color_icn->sel_data, len * color_icn->num_planes);
			free (color_icn->sel_data);
			color_icn->sel_data = NULL;
		}
		else
		{	memcpy (cicnblk->monoblk.ib_pmask, color_icn->sel_mask, len);
			best_icn->sel_mask = cicnblk->monoblk.ib_pmask;
			free (color_icn->sel_mask);
			color_icn->sel_mask = NULL;
		}
	}
	else
	{
		if (best_icn->sel_data == NULL)
		{
			memcpy (cicnblk->monoblk.ib_pmask, color_icn->sel_mask, len);
			free (color_icn->sel_mask);
			color_icn->sel_mask = NULL;
			best_icn->col_data = color_icn->col_data;
			best_icn->sel_mask = cicnblk->monoblk.ib_pmask;
			best_icn->num_planes = color_icn->num_planes;
		}
		else
			*best_icn = *color_icn;
	}
	cicnblk->mainlist = best_icn;
#else
	color_icn->next_res = cicnblk->mainlist;
	cicnblk->mainlist = color_icn;
#endif

#if GEMINI
	(void)obj;
#else
	{
		OBBLK    *ub;

		ub = (OBBLK *)*(_LONG *)rs_global;
		ub[nub].old_type = G_CICON;
		ub[nub].ublk.ub_parm = obj->ob_spec;
		ub[nub].ublk.ub_code = xdraw_cicon;
		obj->ob_spec = (_LONG)&ub[nub].ublk;
		obj->ob_type = (obj->ob_type & 0xff00) | G_USERDEF;
	}
#endif
	
	return (TRUE);
}

/*****************************************************************************/
/* Testen wieviel Bytes pro Pixel im gerÑteabhÑngigen Format verwendet werden*/
/*****************************************************************************/

static _WORD test_rez (void)

{	_WORD     i, np, color, pxy[8], rgb[3], bpp = 0;
	_UWORD    backup[32], test[32];
	_WORD     black[3] = {0, 0, 0};
	_WORD     white[3] = {1000, 1000, 1000};
	MFDB     screen;
	MFDB     pixel = {NULL, 16, 1, 1, 0, 1, 0, 0, 0};
	MFDB     stdfm = {NULL, 16, 1, 1, 1, 1, 0, 0, 0};

	if (xscrn_planes >= 8)
	{
		stdfm.np = pixel.np = xscrn_planes;

		if (xscrn_planes == 8)
		{
			color = 0xff;
			memset (test, 0, xscrn_planes * sizeof (_WORD));
			for (np = 0; np < xscrn_planes; np++)
				test[np] = (color & (1 << np)) << (15 - np);
	
			pixel.mp = stdfm.mp = test;
			vr_trnfm (xvdi_handle, &stdfm, &pixel);
			
			for (i = 1; i < xscrn_planes; i++)
				if (test[i])	break;
			
			if (i >= xscrn_planes && !(test[0] & 0x00ff))
				bpp = 1;
		}
		else
		{
			xrect2array (&xdesk, pxy);
			vs_clip (xvdi_handle, FALSE, pxy);
			screen.mp = NULL;
			
			memset (backup, 0, sizeof (backup));
		
			vswr_mode (xvdi_handle, MD_REPLACE);
			vsl_ends (xvdi_handle, 0, 0);
			vsl_type (xvdi_handle, 1);
			vsl_width (xvdi_handle, 1);
			memset (pxy, 0, sizeof (pxy));
			
			graf_mouse (M_OFF, NULL);
		
			pixel.mp = backup;	/* Punkt retten */
			vro_cpyfm (xvdi_handle, S_ONLY, pxy, &screen, &pixel);
		
			/* Alte Farbe retten */
			vq_color (xvdi_handle, 15, 1, rgb);
	
			/* GerÑteabhÑngiges Format testen */
			pixel.mp = test;
			vsl_color (xvdi_handle, 15);
			vs_color (xvdi_handle, 15, white);
			v_pline (xvdi_handle, 2, pxy);
			
			memset (test, 0, xscrn_planes * sizeof (_WORD));
			vro_cpyfm (xvdi_handle, S_ONLY, pxy, &screen, &pixel);
			
			for (i = (xscrn_planes + 15) / 16 * 2; i < xscrn_planes; i++)
				if (test[i])	break;
			
			if (i >= xscrn_planes)
			{
				vs_color (xvdi_handle, 15, black);
				v_pline (xvdi_handle, 2, pxy);
				
				memset (test, 0, xscrn_planes * sizeof (_WORD));
				vro_cpyfm (xvdi_handle, S_ONLY, pxy, &screen, &pixel);
				
				for (i = (xscrn_planes + 15) / 16 * 2; i < xscrn_planes; i++)
					if (test[i])	break;
				
				if (i >= xscrn_planes)
					bpp = (xscrn_planes + 7) / 8;
			}

			/* Alte Farbe restaurieren */
			vs_color (xvdi_handle, 15, rgb);
	
			pixel.mp = backup;	/* Punkt restaurieren */
			vro_cpyfm (xvdi_handle, S_ONLY, pxy, &pixel, &screen);
		
			graf_mouse (M_ON, NULL);
	
			xrect2array (&xdesk, pxy);
			vs_clip (xvdi_handle, TRUE, pxy);
		}
	}

	return (bpp);
}

/*****************************************************************************/
/* Unter TrueColor Pixelwerte der RGB-Palette ermitteln                      */
/*****************************************************************************/

static void xfill_farbtbl ()

{	_WORD np, color, pxy[8], backup[32], rgb[3];
	MFDB screen;
	MFDB pixel = {NULL, 16, 1, 1, 0, 1, 0, 0, 0};
	MFDB stdfm = {NULL, 16, 1, 1, 1, 1, 0, 0, 0};
	_WORD pixtbl[16] = {0, 2, 3, 6, 4, 7, 5, 8, 9, 10, 11, 14, 12, 15, 13, 16};
	
	if (xscrn_planes >= 8)
	{
		if (xscrn_planes > 8)
		{
			if (is_palette == FALSE)	/* Keine Palette in der Resource */
			{
				for (color = 0; color < 255; color++)
				{	if (color < 16)
					{	vq_color (xvdi_handle, pixtbl[color], 1, rgb_palette[color]);
						rgb_palette[color][3] = pixtbl[color];
					}
					else
					{	vq_color (xvdi_handle, color - 1, 1, rgb_palette[color]);
						rgb_palette[color][3] = color - 1;
					}
				}
				vq_color (xvdi_handle, 1, 1, rgb_palette[255]);
				rgb_palette[255][3] = 1;
				is_palette = TRUE;
			}
			
			vs_clip (xvdi_handle, FALSE, pxy);
			graf_mouse (M_OFF, NULL);

			memset (backup, 0, sizeof (backup));
	 		memset (farbtbl, 0, 32 * 256 * sizeof (_WORD));
			screen.mp = NULL;
			stdfm.np = pixel.np = xscrn_planes;
		
			vswr_mode (xvdi_handle, MD_REPLACE);
			vsl_ends (xvdi_handle, 0, 0);
			vsl_type (xvdi_handle, 1);
			vsl_width (xvdi_handle, 1);
			memset (pxy, 0, sizeof (pxy));
			
			pixel.mp = backup;	/* Punkt retten */
			vro_cpyfm (xvdi_handle, S_ONLY, pxy, &screen, &pixel);
		
			/* Alte Farbe retten */
			vq_color (xvdi_handle, 15, 1, rgb);

			for (color = 0; color < 256; color++)
			{
				vs_color (xvdi_handle, 15, rgb_palette[color]);
				vsl_color (xvdi_handle, 15);
				v_pline (xvdi_handle, 2, pxy);
				
				stdfm.mp = pixel.mp = farbtbl[color];
		
				/* vro_cpyfm, weil v_get_pixel nicht mit TrueColor (>=24 Planes) funktioniert */
				vro_cpyfm (xvdi_handle, S_ONLY, pxy, &screen, &pixel);
	
				if (farbtbl2 != NULL && xpixelbytes)
				{	farbtbl2[color] = 0L;
					memcpy (&farbtbl2[color], pixel.mp, xpixelbytes);
				}
					
				vr_trnfm (xvdi_handle, &pixel, &stdfm);
				for (np = 0; np < xscrn_planes; np++)
					if (farbtbl[color][np])
						farbtbl[color][np] = 0xffff;
			}
		
			/* Alte Farbe restaurieren */
			vs_color (xvdi_handle, 15, rgb);

			pixel.mp = backup;	/* Punkt restaurieren */
			vro_cpyfm (xvdi_handle, S_ONLY, pxy, &pixel, &screen);
		
			graf_mouse (M_ON, NULL);
		}
	}
}

/*****************************************************************************/
/* Icon an aktuelle Grafikauflîsung anpassen                                 */
/* (z.B. 4 Plane Icon an 24 Plane TrueColor)                                 */
/*****************************************************************************/

static void xfix_cicon (_UWORD *col_data, _LONG len, _WORD old_planes, _WORD new_planes, MFDB *s)

{	_LONG  x, i, old_len, rest_len, mul[32], pos;
	_UWORD np, *new_data, mask, pixel, bit, color, back[32], old_col[32], maxcol;
	_WORD  got_mem = FALSE;
	MFDB  d;
	
	len >>= 1;

	if (old_planes == new_planes)
	{	if (s != NULL)
		{	if (new_planes == xscrn_planes)
			{
				d = *s;
				d.ff = FALSE;
				s->mp = col_data;
				if (d.mp == s->mp)
				{	if ((d.mp = malloc (len * 2 * new_planes)) == NULL)
						d.mp = s->mp;
					else
						got_mem = TRUE;
				}
				
				vr_trnfm (xvdi_handle, s, &d);
				if (d.mp != s->mp && got_mem == TRUE)
				{
					memcpy (s->mp, d.mp, len * 2 * new_planes);
					free (d.mp);
				}
			}
			else
				memcpy (s->mp, col_data, len * 2 * new_planes);
		}
		return;
	}
	
	if (new_planes <= 8)
	{
		old_len  = old_planes * len;
		rest_len = new_planes * len - old_len;

		if (s != NULL)
		{
			new_data = &((_UWORD *)s->mp)[old_len];
			memset (new_data, 0, rest_len * 2);
			memcpy (s->mp, col_data, old_len * 2);
			col_data = s->mp;
		}
		else
			new_data = (_UWORD *)&col_data[old_len];
		
		for (x = 0; x < len; x++)
		{
			mask = 0xffff;
	
			for (i = 0; i < old_len; i += len)
				mask &= (_UWORD)col_data[x+i];
			
			if (mask)
				for (i = 0; i < rest_len; i += len)
					new_data[x+i] |= mask;
		}

		if (s != NULL)	/* ins gerÑteabhÑngige Format konvertieren */
		{
			d = *s;
			d.ff = 0;
			if ((d.mp = malloc (len * 2 * new_planes)) == NULL)
				d.mp = s->mp;
			
			vr_trnfm (xvdi_handle, s, &d);
			if (d.mp != s->mp)
			{
				memcpy (s->mp, d.mp, len * 2 * new_planes);
				free (d.mp);
			}
		}
	}
	else	/* TrueColor, bzw RGB-orientierte Pixelwerte */
	{
		if (!xpixelbytes || s == NULL)
		{
			for (i = 0; i < new_planes; i++)
				mul[i] = i * len;
			
			if (old_planes < 8)
			{
				maxcol = (1 << old_planes) - 1;
				memcpy (old_col, farbtbl[maxcol], new_planes * sizeof (_WORD));
				memset (farbtbl[maxcol], 0, new_planes * sizeof (_WORD));
			}
	
			if (s != NULL)
			{
				new_data = &((_UWORD *)s->mp)[old_len];
				memset (new_data, 0, rest_len * 2);
				memcpy (s->mp, col_data, old_len * 2);
				col_data = s->mp;
			}
			
			for (x = 0; x < len; x++)
			{
				bit = 1;
				for (np = 0; np < old_planes; np++)
					back[np] = col_data[mul[np] + x];
				
				for (pixel = 0; pixel < 16; pixel++)
				{
					color = 0;
					for (np = 0; np < old_planes; np++)
					{
						color += ((back[np] & 1) << np);
						back[np] >>= 1;
					}
					
					for (np = 0; np < new_planes; np++)
					{	pos = mul[np] + x;
						col_data[pos] = (col_data[pos] & ~bit) | (farbtbl[color][np] & bit);
					}
					
					bit <<= 1;
				}
			}
			if (old_planes < 8)
				memcpy (farbtbl[maxcol], old_col, new_planes * sizeof (_WORD));

			if (s != NULL)	/* ins gerÑteabhÑngige Format konvertieren */
			{
				d = *s;
				d.ff = 0;
				if ((d.mp = malloc (len * 2 * new_planes)) == NULL)
					d.mp = s->mp;
				
				vr_trnfm (xvdi_handle, s, &d);
				if (d.mp != s->mp)
				{
					memcpy (s->mp, d.mp, len * 2 * new_planes);
					free (d.mp);
				}
			}
		}
		else
			std_to_byte (col_data, len, old_planes, farbtbl2, s);
	}
}

/*****************************************************************************/
/* std_to_byte wandelt eine Grafik im Standardformat direkt ins gerÑte-      */
/* abhÑngige Format (in Auflîsungen mit >= 16 Planes)                        */
/*****************************************************************************/

static void std_to_byte (col_data, len, old_planes, farbtbl2, s)
_UWORD *col_data;
_LONG  len;
_WORD  old_planes;
_ULONG *farbtbl2;
MFDB  *s;

{	_LONG  x, i, mul[32], pos;
	_UWORD np, *new_data, pixel, color, back[32];
	_WORD  memflag = FALSE;
	unsigned char *p1, *p2;
	_ULONG  colback;

	if (s->mp == col_data)
	{
		if ((col_data = malloc (len * 2 * s->np)) == NULL)
			return;
		memcpy (col_data, s->mp, len * 2 * s->np);
		memflag = TRUE;
	}
	new_data = (_UWORD *)s->mp;
	p1 = (unsigned char *)new_data;

	if (old_planes < 8)
	{
		colback = farbtbl2[(1 << old_planes) - 1];
		farbtbl2[(1 << old_planes) - 1] = farbtbl2[255];
	}
		
	for (i = 0; i < old_planes; i++)
		mul[i] = i * len;
	
	pos = 0;
	
	for (x = 0; x < len; x++)
	{
		for (np = 0; np < old_planes; np++)
			back[np] = col_data[mul[np] + x];
		
		for (pixel = 0; pixel < 16; pixel++)
		{
			color = 0;
			for (np = 0; np < old_planes; np++)
			{
				color |= ((back[np] & 0x8000) >> (15 - np));
				back[np] <<= 1;
			}
			
			switch (xpixelbytes)
			{
				case 2:
					new_data[pos++] = *(_UWORD *)&farbtbl2[color];
					break;

				case 3:
					p2 = (unsigned char *)&farbtbl2[color];
					*(p1++) = *(p2++);
					*(p1++) = *(p2++);
					*(p1++) = *(p2++);
					break;

				case 4:
					((_ULONG *)new_data)[pos++] = farbtbl2[color];
					break;
			}
		}
	}

	if (old_planes < 8)
		farbtbl2[(1 << old_planes) - 1] = colback;

	if (memflag)
		free (col_data);
}

/*****************************************************************************/
/* Zeichnet Farb-Icon                                                        */
/*****************************************************************************/

static _WORD cdecl xdraw_cicon (PARMBLK *pb)

{	_WORD	 	ob_x, ob_y, x, y, dummy, pxy[4], m_mode, i_mode, mskcol, icncol;
	_LONG	 	ob_spec;
	ICONBLK *iconblk;
	CICON   *cicn;
	_WORD    *mask, *data, *dark = NULL;
	char    letter[2];
	_WORD    selected, mindex[2], iindex[2], buf, xy[4];
	_BOOL invert = FALSE;
	
	selected = pb->pb_currstate & SELECTED;
	
	xrect2array ((GRECT *)&pb->pb_xc, xy);
	vs_clip (xvdi_handle, TRUE, xy);   /* Setze Rechteckausschnitt */

	ob_spec	= pb->pb_parm;
	ob_x		= pb->pb_x;
	ob_y		= pb->pb_y;

	iconblk = (ICONBLK *)ob_spec;
	cicn    = ((CICONBLK *)ob_spec)->mainlist;
	m_mode  = MD_TRANS;

	if (selected) /* it was an objc_change */
	{
		if (cicn->sel_data != NULL)
		{
			mask = cicn->sel_mask;
			data = cicn->sel_data;
			if (cicn->num_planes > 1)
			{	if (cicn->num_planes > 8)	/* TrueColor, bzw RGB-orientierte Grafikkarte? */
					i_mode = S_AND_D;
				else
					i_mode = S_OR_D;
			}
			else
				i_mode = MD_TRANS;
		}
		else
		{
			mask = cicn->col_mask;
			data = cicn->col_data;

			if (cicn->num_planes > 1)
			{	if (cicn->num_planes > 8)
					i_mode = S_AND_D;
				else
					i_mode = S_OR_D;
				dark = cicn->sel_mask;
			}
			else
				invert = TRUE;
		}
	}
	else
	{
		mask = cicn->col_mask;
		data = cicn->col_data;
	
		if (cicn->num_planes > 1)
		{	if (cicn->num_planes > 8)
				i_mode = S_AND_D;
			else
				i_mode = S_OR_D;
		}
		else
			i_mode = MD_TRANS;
	}
	
	mindex [0] = ((iconblk->ib_char & 0x0f00) != 0x0100) ? (iconblk->ib_char & 0x0f00) >> 8 : WHITE;
	mindex [1] = WHITE;
	
	icncol = iindex[0] = (_WORD)(((_UWORD)iconblk->ib_char & 0xf000U) >> 12U);
	iindex[1] = WHITE;

	mskcol = (iconblk->ib_char & 0x0f00) >> 8;

	x = ob_x + iconblk->ib_xicon;
	y = ob_y + iconblk->ib_yicon;

	if (invert)
	{
		buf       = iindex[0];
		iindex[0] = mindex[0];
		mindex[0] = buf;
		i_mode    = MD_TRANS;
	}
	if (selected)
	{
		buf    = icncol;
		icncol = mskcol;
		mskcol = buf;
	}
	
	draw_bitblk (mask, x, y, iconblk->ib_wicon, iconblk->ib_hicon, 1, m_mode, mindex);
	draw_bitblk (data, x, y, iconblk->ib_wicon, iconblk->ib_hicon, cicn->num_planes, i_mode, iindex);
	
	if (dark)
	{
		mindex [0] = BLACK;
		mindex [1] = WHITE;
		draw_bitblk (dark, x, y, iconblk->ib_wicon, iconblk->ib_hicon, 1, MD_TRANS, mindex);
	}

	if (iconblk->ib_ptext[0])
	{
		x = ob_x + iconblk->ib_xtext;
		y = ob_y + iconblk->ib_ytext;
		
		pxy[0] = x;
		pxy[1] = y;
		pxy[2] = x + iconblk->ib_wtext - 1;
		pxy[3] = y + iconblk->ib_htext - 1;
		
		vswr_mode     (xvdi_handle, MD_REPLACE);		/* Textbox zeichnen */
		vsf_color     (xvdi_handle, mskcol);
		vsf_interior  (xvdi_handle, FIS_SOLID);
		vsf_perimeter (xvdi_handle, FALSE);
		v_bar         (xvdi_handle, pxy);
	}

	vswr_mode     (xvdi_handle, MD_TRANS);
  vst_font      (xvdi_handle, 1);	/* Systemfont */
	vst_height    (xvdi_handle, 4, &dummy, &dummy, &dummy, &dummy);
  vst_color     (xvdi_handle, icncol);
  vst_effects   (xvdi_handle, TXT_NORMAL);
  vst_alignment (xvdi_handle, ALI_LEFT, ALI_TOP, &dummy, &dummy);
  vst_rotation  (xvdi_handle, 0);
	
	if (iconblk->ib_ptext[0])
	{	x += (iconblk->ib_wtext - strlen (iconblk->ib_ptext) * 6) / 2;
		y += (iconblk->ib_htext - 6) / 2;
		
		v_gtext (xvdi_handle, x, y, iconblk->ib_ptext);
	}

	letter[0] = iconblk->ib_char & 0xff;
	if (letter[0])
	{
		letter[1] = '\0';
		x = ob_x + iconblk->ib_xicon + iconblk->ib_xchar;
		y = ob_y + iconblk->ib_yicon + iconblk->ib_ychar;
	
		v_gtext (xvdi_handle, x, y, letter);
	}
	
	return (pb->pb_currstate & ~ SELECTED);
} /* draw_userdef */

/*****************************************************************************/

static void draw_bitblk (_WORD *p, _WORD x, _WORD y, _WORD w, _WORD h, _WORD num_planes, _WORD mode, _WORD *index)

{	_WORD	 	pxy[8];
	MFDB	 	s, d;

	d.mp	= NULL; /* screen */
	s.mp	= (void *)p;
	s.fwp = w;
	s.fh	= h;
	s.fww = w >> 4;
	s.ff	= FALSE;
	s.np	= num_planes;

	pxy[0] = 0;
	pxy[1] = 0;
 	pxy[2] = s.fwp - 1;
 	pxy[3] = s.fh - 1;

	pxy[4] = x;
	pxy[5] = y;
	pxy[6] = pxy[4] + pxy [2];
	pxy[7] = pxy[5] + pxy [3];

	if (num_planes > 1)
		vro_cpyfm (xvdi_handle, mode, pxy, &s, &d);
	else
		vrt_cpyfm (xvdi_handle, mode, pxy, &s, &d, index);	 /* copy it */
}

/*****************************************************************************/

static void xrect2array (const GRECT *rect, _WORD *array)

{
  *array++ = rect->g_x;
  *array++ = rect->g_y;
  *array++ = rect->g_x + rect->g_w - 1;
  *array   = rect->g_y + rect->g_h - 1;
} /* xrect2array */

/*****************************************************************************/

_BOOL init_xrsrc (_WORD vdi_handle, GRECT *desk, _WORD gl_wbox, _WORD gl_hbox)

{
	xvdi_handle = vdi_handle;
	xdesk = *desk;
	xgl_wbox = gl_wbox;
	xgl_hbox = gl_hbox;
	
	return (TRUE);
}

/*****************************************************************************/

void term_xrsrc (void)

{
}

#endif
