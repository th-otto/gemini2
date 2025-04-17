/*****************************************************************************/
/*   VDI.H: Common VDI definitions and structures.                           */
/*****************************************************************************/
/*                                                                           */
/*   Authors: Dieter & Juergen Geiss                                         */
/*                                                                           */
/*****************************************************************************/

#ifndef __VDI__
#define __VDI__

/****** Control library ******************************************************/

#define OW_FILE          0 /* output device type in the low-order byte */
#define OW_SERIAL        1
#define OW_PARALLEL      2
#define OW_DEVICE        3
#define OW_NOCHANGE    255

#define OW_LETTER1       0 /* page size index in the high-order byte */
#define OW_HALF          5
#define OW_B5           10
#define OW_LETTER2      20
#define OW_A4           30
#define OW_LEGAL        40
#define OW_DOUBLE       50
#define OW_BROAD        55
#define OW_INDIRECT    255 /* use work_in [101] and work_in [102] */

void v_get_driver_info (_WORD device_id, _WORD info_select,
                          char *info_string);
void v_opnwk           (_WORD *work_in, _WORD *handle,
                          _WORD *work_out);
void v_clswk           (_WORD handle);
void v_clrwk           (_WORD handle);
void v_updwk           (_WORD handle);
void v_opnvwk          (_WORD *work_in, _WORD *handle, 
                          _WORD *work_out);
void v_clsvwk          (_WORD handle);
_WORD vst_load_fonts    (_WORD handle, _WORD select);
void vst_unload_fonts  (_WORD handle, _WORD select);

#if GEM & (GEM3 | XGEM)
_WORD vst_ex_load_fonts (_WORD handle, _WORD select, _WORD font_max,
                          _WORD font_free);
#endif

void vs_clip           (_WORD handle, _WORD clip_flag, _WORD *pxyarray);
void v_set_app_buff    (void *address, _WORD nparagraphs);
_WORD v_bez_on          (_WORD handle);
_WORD v_bez_off         (_WORD handle);
_WORD v_bez_qual        (_WORD handle, _WORD prcnt);
void v_pat_rotate      (_WORD handle, _WORD angle);

/****** Output library *******************************************************/

void v_pline           (_WORD handle, _WORD count, _WORD *xy);
void v_pmarker         (_WORD handle, _WORD count, _WORD *xy);
void v_gtext           (_WORD handle, _WORD x, _WORD y, char *string);

#if GEM & (GEM3 | XGEM)
void v_etext           (_WORD handle, _WORD x, _WORD y, char *string,
                          _WORD *offsets);
#endif

void v_fillarea        (_WORD handle, _WORD count, _WORD *xy);
void v_cellarray       (_WORD handle, _WORD *pxyarray, _WORD row_length,
                          _WORD el_used, _WORD num_rows, _WORD wrt_mode,
                          _WORD *colarray);
void v_bar             (_WORD handle, _WORD *pxyarray);
void v_arc             (_WORD handle, _WORD x, _WORD y, _WORD radius,
                          _WORD begang, _WORD endang);
void v_pieslice        (_WORD handle, _WORD x, _WORD y, _WORD radius,
                          _WORD begang, _WORD endang);
void v_circle          (_WORD handle, _WORD x, _WORD y, _WORD radius);
void v_ellipse         (_WORD handle, _WORD x, _WORD y, _WORD xradius,
                          _WORD yradius);
void v_ellarc          (_WORD handle, _WORD x, _WORD y, _WORD xradius,
                          _WORD yradius, _WORD begang, _WORD endang);
void v_ellpie          (_WORD handle, _WORD x, _WORD y, _WORD xradius,
                          _WORD yradius, _WORD begang, _WORD endang);
void v_rbox            (_WORD handle, _WORD *xyarray);
void v_rfbox           (_WORD handle, _WORD *xyarray);
void v_justified       (_WORD handle, _WORD x, _WORD y, char *string,
                          _WORD length, _WORD word_space, _WORD char_space);
void v_contourfill     (_WORD handle, _WORD x, _WORD y, _WORD index);
void vr_recfl          (_WORD handle, _WORD *pxyarray);
void v_bez             (_WORD handle, _WORD count, _WORD *xyarr,
                          unsigned char *bezarr, _WORD *minmax, _WORD *npts,
                          _WORD *nmove);
void v_bez_fill        (_WORD handle, _WORD count, _WORD *xyarr,
                          unsigned char *bezarr, _WORD *minmax, _WORD *npts,
                          _WORD *nmove);

/****** Attribute library ****************************************************/

#define MD_REPLACE       1 /* gsx modes */
#define MD_TRANS         2
#define MD_XOR           3
#define MD_ERASE         4

#define FIS_HOLLOW       0 /* gsx styles */
#define FIS_SOLID        1
#define FIS_PATTERN      2
#define FIS_HATCH        3
#define FIS_USER         4

#define ALL_WHITE        0 /* bit blt rules */
#define S_AND_D          1
#define S_AND_NOTD       2
#define S_ONLY           3
#define NOTS_AND_D       4
#define D_ONLY           5
#define S_XOR_D          6
#define S_OR_D           7
#define NOT_SORD         8
#define NOT_SXORD        9
#define D_INVERT        10
#define S_OR_NOTD       11
#define NOT_D           12
#define NOTS_OR_D       13
#define NOT_SANDD       14
#define ALL_BLACK       15

#ifndef WHITE
#define WHITE            0 /* colors */
#define BLACK            1
#define RED              2
#define GREEN            3
#define BLUE             4
#define CYAN             5
#define YELLOW           6
#define MAGENTA          7
#define DWHITE           8
#define DBLACK           9
#define DRED            10
#define DGREEN          11
#define DBLUE           12
#define DCYAN           13
#define DYELLOW         14
#define DMAGENTA        15
#endif

#define SOLID            1 /* line types */
#define LONGDASH         2
#define DOT              3
#define DASHDOT          4
#define DASH             5
#define DASH2DOT         6
#define USERLINE         7

#define SQUARED          0 /* line ends */
#define ARROWED          1
#define ROUNDED          2

#define PM_DOT           1 /* polymarker types */
#define PM_PLUS          2
#define PM_ASTERISK      3
#define PM_SQUARE        4
#define PM_DIAGCROSS     5
#define PM_DIAMOND       6

#define TXT_NORMAL       0x0000 /* text effects */
#define TXT_THICKENED    0x0001
#define TXT_LIGHT        0x0002
#define TXT_SKEWED       0x0004
#define TXT_UNDERLINED   0x0008
#define TXT_OUTLINED     0x0010
#define TXT_SHADOWED     0x0020

#define ALI_LEFT         0 /* horizontal text alignment */
#define ALI_CENTER       1
#define ALI_RIGHT        2

#define ALI_BASE         0 /* vertical text alignment */
#define ALI_HALF         1
#define ALI_ASCENT       2
#define ALI_BOTTOM       3
#define ALI_DESCENT      4
#define ALI_TOP          5

/* fill pattern */

typedef struct patarray
{
  _WORD patword [16];
} FILLPAT;

_WORD vswr_mode         (_WORD handle, _WORD mode);
void vs_color          (_WORD handle, _WORD index, _WORD *rgb_in);
_WORD vsl_type          (_WORD handle, _WORD style);
void vsl_udsty         (_WORD handle, _WORD pattern);
_WORD vsl_width         (_WORD handle, _WORD width);
_WORD vsl_color         (_WORD handle, _WORD color_index);
void vsl_ends          (_WORD handle, _WORD beg_style, _WORD end_style);
_WORD vsm_type          (_WORD handle, _WORD symbol);
_WORD vsm_height        (_WORD handle, _WORD height);
_WORD vsm_color         (_WORD handle, _WORD color_index);
void vst_height        (_WORD handle, _WORD height,
                          _WORD *char_width, _WORD *char_height,
                          _WORD *cell_width, _WORD *cell_height);
_WORD vst_point         (_WORD handle, _WORD point,
                          _WORD *char_width, _WORD *char_height,
                          _WORD *cell_width, _WORD *cell_height);
_WORD vst_rotation      (_WORD handle, _WORD angle);
_WORD vst_font          (_WORD handle, _WORD font);
_WORD vst_color         (_WORD handle, _WORD color_index);
_WORD vst_effects       (_WORD handle, _WORD effect);
void vst_alignment     (_WORD handle, _WORD hor_in, _WORD vert_in,
                          _WORD *hor_out, _WORD *vert_out);
_WORD vsf_interior      (_WORD handle, _WORD style);
_WORD vsf_style         (_WORD handle, _WORD style_index);
_WORD vsf_color         (_WORD handle, _WORD color_index);
_WORD vsf_perimeter     (_WORD handle, _WORD per_vis);
_WORD vsf_xperimeter    (_WORD handle, _WORD per_vis, _WORD per_style);
void vsf_udpat         (_WORD handle, _WORD *pfill_pat, _WORD planes);
void vs_grayoverride   (_WORD handle, _WORD grayval);

/****** Raster library *******************************************************/

/* Memory Form Definition Block */

typedef struct memform
{
  void *mp;
  _WORD fwp;
  _WORD fh;
  _WORD fww;
  _WORD ff;
  _WORD np;
  _WORD r1;
  _WORD r2;
  _WORD r3;
} MFDB;

typedef struct fdbstr   /* for compatibility */
{
  void *fd_addr;
  _WORD fd_w;
  _WORD fd_h;
  _WORD fd_wdwidth;
  _WORD fd_stand;
  _WORD fd_nplanes;
  _WORD fd_r1;
  _WORD fd_r2;
  _WORD fd_r3;
} FDB;

void v_get_pixel       (_WORD handle, _WORD x, _WORD y, _WORD *pel,
                          _WORD *index);
void vro_cpyfm         (_WORD handle, _WORD wr_mode, _WORD *xy,
                          MFDB *srcMFDB, MFDB *desMFDB);
void vr_trnfm          (_WORD handle, MFDB *srcMFDB, MFDB *desMFDB);
void vrt_cpyfm         (_WORD handle, _WORD wr_mode, _WORD *xy,
                          MFDB *srcMFDB, MFDB *desMFDB,
                          _WORD *index);

/****** Input library ********************************************************/

#define DEV_LOCATOR      1 /* input device */
#define DEV_VALUATOR     2
#define DEV_CHOICE       3
#define DEV_STRING       4

#define MODE_REQUEST     1 /* input mode */
#define MODE_SAMPLE      2

void vsin_mode         (_WORD handle, _WORD dev_type, _WORD mode);
void vrq_locator       (_WORD handle, _WORD initx, _WORD inity,
                          _WORD *xout, _WORD *yout, _WORD *term);
_WORD vsm_locator       (_WORD handle, _WORD initx, _WORD inity,
                          _WORD *xout, _WORD *yout, _WORD *term);
void vrq_valuator      (_WORD handle, _WORD val_in, _WORD *val_out,
                          _WORD *term);
void vsm_valuator      (_WORD handle, _WORD val_in, _WORD *val_out,
                          _WORD *term, _WORD *status);
void vrq_choice        (_WORD handle, _WORD in_choice, _WORD *out_choice);
_WORD vsm_choice        (_WORD handle, _WORD *choice);
void vrq_string        (_WORD handle, _WORD length, _WORD echo_mode,
                          _WORD *echo_xy, char *string);
_WORD vsm_string        (_WORD handle, _WORD length, _WORD echo_mode,
                          _WORD *echo_xy, char *string);
void vsc_form          (_WORD handle, _WORD *cur_form);
#if HIGH_C
void vex_timv          (_WORD handle, _WORD (*tim_addr)(),
                          _WORD (* *old_addr)(), _WORD *scale);
#else
void vex_timv          (_WORD handle, _WORD (*tim_addr)(),
                          _WORD (* *old_addr)(), _WORD *scale);
#endif
void v_show_c          (_WORD handle, _WORD reset);
void v_hide_c          (_WORD handle);
void vq_mouse          (_WORD handle, _WORD *status, _WORD *px,
                          _WORD *py);
#if HIGH_C
void vex_butv          (_WORD handle, _WORD (*usercode)(),
                          _WORD (* *savecode)());
void vex_motv          (_WORD handle, _WORD (*usercode)(),
                          _WORD (* *savecode)());
void vex_curv          (_WORD handle, _WORD (*usercode)(),
                          _WORD (* *savecode)());
#else
void vex_butv          (_WORD handle, _WORD (*usercode)(),
                          _WORD (* *savcode)());
void vex_motv          (_WORD handle, _WORD (*usercode)(),
                          _WORD (* *savecode)());
void vex_curv          (_WORD handle, _WORD (*usercode)(),
                          _WORD (* *savecode)());
#endif

void vq_key_s          (_WORD handle, _WORD *status);

/****** Inquire library ******************************************************/

void vq_color          (_WORD handle, _WORD index, _WORD set_flag,
                          _WORD *rgb);
void vq_cellarray      (_WORD handle, _WORD *pxyarray,
                          _WORD row_length, _WORD num_rows,
                          _WORD *el_used, _WORD *rows_used,
                          _WORD *status, _WORD *colarray);
void vql_attributes    (_WORD handle, _WORD *attrib);
void vqm_attributes    (_WORD handle, _WORD *attrib);
void vqf_attributes    (_WORD handle, _WORD *attrib);
void vqt_attributes    (_WORD handle, _WORD *attrib);
void vq_extnd          (_WORD handle, _WORD owflag, _WORD *work_out);
void vqin_mode         (_WORD handle, _WORD dev_type, _WORD *input_mode);
void vqt_extent        (_WORD handle, char *string, _WORD *extent);
_WORD vqt_width         (_WORD handle, char character, _WORD *cell_width,
                          _WORD *left_delta, _WORD *right_delta);
_WORD vqt_name          (_WORD handle, _WORD element_num, char *name);
void vqt_fontinfo      (_WORD handle, _WORD *minADE, _WORD *maxADE,
                          _WORD *distances, _WORD *maxwidth,
                          _WORD *effects);
_WORD vqt_justified     (_WORD handle, _WORD x, _WORD y, char *string,
                          _WORD length, _WORD word_space, _WORD char_space,
                          _WORD *offsets);

/****** Escape library *******************************************************/

#define O_B_BOLDFACE     '0' /* OUT-File definitions for v_alpha_text */
#define O_E_BOLDFACE     '1'
#define O_B_ITALICS      '2'
#define O_E_ITALICS      '3'
#define O_B_UNDERSCORE   '4'
#define O_E_UNDERSCORE   '5'
#define O_B_SUPERSCRIPT  '6'
#define O_E_SUPERSCRIPT  '7'
#define O_B_SUBSCRIPT    '8'
#define O_E_SUBSCRIPT    '9'
#define O_B_NLQ          'A'
#define O_E_NLQ          'B'
#define O_B_EXPANDED     'C'
#define O_E_EXPANDED     'D'
#define O_B_LIGHT        'E'
#define O_E_LIGHT        'F'
#define O_PICA           'W'
#define O_ELITE          'X'
#define O_CONDENSED      'Y'
#define O_PROPORTIONAL   'Z'

#define O_GRAPHICS       "\033\033GEM,%d,%d,%d,%d,%s"

#define MUTE_RETURN     -1 /* definitions for vs_mute */
#define MUTE_ENABLE      0
#define MUTE_DISABLE     1

#define OR_PORTRAIT      0 /* definitions for v_orient */
#define OR_LANDSCAPE     1

#define TRAY_MANUAL     -1 /* definitions fpr v_tray */
#define TRAY_DEFAULT     0
#define TRAY_FIRSTOPT    1

#define XBIT_FRACT       0 /* definitions for v_xbit_image */
#define XBIT_INTEGER     1

#define XBIT_LEFT        0
#define XBIT_CENTER      1
#define XBIT_RIGHT       2

#define XBIT_TOP         0
#define XBIT_MIDDLE      1
#define XBIT_BOTTOM      2

void vq_chcells        (_WORD handle, _WORD *rows, _WORD *columns);
void v_exit_cur        (_WORD handle);
void v_enter_cur       (_WORD handle);
void v_curup           (_WORD handle);
void v_curdown         (_WORD handle);
void v_curright        (_WORD handle);
void v_curleft         (_WORD handle);
void v_curhome         (_WORD handle);
void v_eeos            (_WORD handle);
void v_eeol            (_WORD handle);
void vs_curaddress     (_WORD handle, _WORD row, _WORD column);
void v_curtext         (_WORD handle, char *string);
void v_rvon            (_WORD handle);
void v_rvoff           (_WORD handle);
void vq_curaddress     (_WORD handle, _WORD *row, _WORD *column);
_WORD vq_tabstatus      (_WORD handle);
void v_hardcopy        (_WORD handle);
void v_dspcur          (_WORD handle, _WORD x, _WORD y);
void v_rmcur           (_WORD handle);
void v_form_adv        (_WORD handle);
void v_output_window   (_WORD handle, _WORD *xyarray);
void v_clear_disp_list (_WORD handle);
void v_bit_image       (_WORD handle, char *filename,
                          _WORD aspect, _WORD x_scale, _WORD y_scale,
                          _WORD h_align, _WORD v_align, _WORD *xyarray);
void vq_scan           (_WORD handle, _WORD *g_height, _WORD *g_slices,
                          _WORD *a_height, _WORD *a_slices,
                          _WORD *factor);
void v_alpha_text      (_WORD handle, char *string);
_WORD vs_palette        (_WORD handle, _WORD palette);
void v_sound           (_WORD handle, _WORD frequency, _WORD duration);
_WORD vs_mute           (_WORD handle, _WORD action);
void vt_resolution     (_WORD handle, _WORD xres, _WORD yres,
                          _WORD *xset, _WORD *yset);
void vt_axis           (_WORD handle, _WORD xres, _WORD yres,
                          _WORD *xset, _WORD *yset);
void vt_origin         (_WORD handle, _WORD xorigin, _WORD yorigin);
void vq_tdimensions    (_WORD handle, _WORD *xdimension,
                          _WORD *ydimension);
void vt_alignment      (_WORD handle, _WORD dx, _WORD dy);
void vsp_film          (_WORD handle, _WORD index, _WORD lightness);
_WORD vqp_filmname      (_WORD handle, _WORD index, char *name);
void vsc_expose        (_WORD handle, _WORD state);
void v_meta_extents    (_WORD handle, _WORD min_x, _WORD min_y,
                          _WORD max_x, _WORD max_y);
void v_write_meta      (_WORD handle, _WORD num_intin, _WORD *intin,
                          _WORD num_ptsin, _WORD *ptsin);
void vm_filename       (_WORD handle, char *filename);

#if GEM & (GEM3 | XGEM)
void vm_pagesize       (_WORD handle, _WORD pgwidth, _WORD pgheight);
void vm_coords         (_WORD handle, _WORD llx, _WORD lly,
                          _WORD urx, _WORD ury);
void v_copies          (_WORD handle, _WORD count);
void v_orient          (_WORD handle, _WORD orientation);
void v_tray            (_WORD handle, _WORD tray);
_WORD v_xbit_image      (_WORD handle, char *filename, _WORD aspect,
                          _WORD x_scale, _WORD y_scale,
                          _WORD h_align, _WORD v_align, _WORD rotate,
                          _WORD background, _WORD foreground, _WORD *xy);
#endif

void vs_bkcolor        (_WORD handle, _WORD color);
void v_setrgbi         (_WORD handle, _WORD primtype, _WORD r, _WORD g, _WORD b,
                          _WORD i);
void v_topbot          (_WORD handle, _WORD height,
                          _WORD *char_width, _WORD *char_height,
                          _WORD *cell_width, _WORD *cell_height);
void v_ps_halftone     (_WORD handle, _WORD index, _WORD angle,
                          _WORD frequency);

#if GEM & GEM1
void vqp_films         (_WORD handle, char *film_names);
void vqp_state         (_WORD handle, _WORD *port, char *film_name,
                          _WORD *lightness, _WORD *interlace,
                          _WORD *planes, _WORD *indexes);
void vsp_state         (_WORD handle, _WORD port, _WORD film_num,
                          _WORD lightness, _WORD interlace, _WORD planes,
                          _WORD *indexes);
void vsp_save          (_WORD handle);
void vsp_message       (_WORD handle);
_WORD vqp_error         (_WORD handle);

void v_offset          (_WORD handle, _WORD offset);
void v_fontinit        (_WORD handle, _WORD fh_high, _WORD fh_low);
void v_escape2000      (_WORD times);
_WORD vq_gdos           (void);
#endif /* GEM1 */

/*****************************************************************************/

#endif /* __VDI__ */
