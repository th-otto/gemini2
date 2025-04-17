/*****************************************************************************/
/*   AES.H: Common AES definitions and structures.                           */
/*****************************************************************************/
/*                                                                           */
/*   Authors: Dieter & Juergen Geiss                                         */
/*                                                                           */
/*****************************************************************************/

#ifndef __AES__
#define __AES__

#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  elif defined(__TURBOC__)
#    define _WORD int
#  else
#    define _WORD short
#  endif
#endif
#ifndef _UWORD
#  ifdef UWORD
#    define _UWORD UWORD
#  elif defined(__TURBOC__)
#    define _UWORD unsigned int
#  else
#    define _UWORD unsigned short
#  endif
#endif
#ifndef _BOOL
#  define _BOOL int
#endif
#ifndef _LONG
#  define _LONG long
#endif
#ifndef _ULONG
#  define _ULONG unsigned long
#endif
#ifndef cdecl
#  ifdef __GNUC__
#    define cdecl
#  endif
#endif


/****** Application library **************************************************/

_WORD  appl_init       (void);
_WORD  appl_read       (_WORD rwid, _WORD length, void *pbuff);
_WORD  appl_write      (_WORD rwid, _WORD length, void *pbuff);
_WORD  appl_find       (char *pname);
_WORD  appl_tplay      (void *tbuffer, _WORD tlenght, _WORD tscale);
_WORD  appl_trecord    (void *tbuffer, _WORD tlength);

_WORD  appl_bvset      (_UWORD bvdisk, _UWORD bvhard);
void  appl_yield      (void);

_WORD  appl_exit       (void);

/****** Event library ********************************************************/

typedef struct orect
{
  struct orect *o_link;
  _WORD         o_x;
  _WORD         o_y;
  _WORD         o_w;
  _WORD         o_h;
} ORECT;

typedef struct grect
{
  _WORD g_x;
  _WORD g_y;
  _WORD g_w;
  _WORD g_h;
} GRECT;

typedef struct mevent
{
  _UWORD e_flags;
  _UWORD e_bclk;
  _UWORD e_bmsk;
  _UWORD e_bst;
  _UWORD e_m1flags;
  GRECT e_m1;
  _UWORD e_m2flags;
  GRECT e_m2;
  _WORD *e_mepbuf;
  _ULONG e_time;
  _WORD  e_mx;       
  _WORD  e_my;
  _UWORD e_mb;
  _UWORD e_ks;
  _UWORD e_kr;
  _UWORD e_br;
  _UWORD e_m3flags;
  GRECT e_m3;
  _WORD  e_xtra0;
  _WORD *e_smepbuf;
  _ULONG e_xtra1;
  _ULONG e_xtra2;
} MEVENT;

/* multi flags */

#define MU_KEYBD    0x0001   
#define MU_BUTTON   0x0002
#define MU_M1       0x0004
#define MU_M2       0x0008
#define MU_MESAG    0x0010
#define MU_TIMER    0x0020
#define MU_M3       0x0040
#define MU_SYSMESAG 0x0080
#define MU_POSTEV   0x1000

/* keyboard states */

#define K_RSHIFT    0x0001
#define K_LSHIFT    0x0002
#define K_CTRL      0x0004
#define K_ALT       0x0008

/* message values */

#define SCR_MGR     0x0001 /* process id of the screen manager */

#define AP_MSG           0

#define MN_SELECTED     10
#define WM_REDRAW       20
#define WM_TOPPED       21
#define WM_CLOSED       22
#define WM_FULLED       23
#define WM_ARROWED      24
#define WM_HSLID        25
#define WM_VSLID        26
#define WM_SIZED        27
#define WM_MOVED        28
#define WM_NEWTOP       29 /* for compatibility */
#define WM_UNTOPPED     30
#define WM_ONTOP        31
#define WM_OFFTOP       32
#define PR_FINISH       33

#define AC_OPEN         40
#define AC_CLOSE        41

#define AP_TERM         50
#define AP_TFAIL        51
#define AP_RESCHG       57
#define SHUT_COMPLETED  60
#define RESCH_COMPLETED 61

#define CH_EXIT         90

typedef struct
{
  _WORD m_out;
  _WORD m_x;
  _WORD m_y;
  _WORD m_w;
  _WORD m_h;
} MOBLK;

_UWORD evnt_keybd      (void);
_WORD  evnt_button     (_WORD clicks, _UWORD mask, _UWORD state,
                         _WORD *pmx, _WORD *pmy,
                         _WORD *pmb, _WORD *pks);
_WORD  evnt_mouse      (_WORD flags, _WORD x, _WORD y, _WORD width, _WORD height,
                         _WORD *pmx, _WORD *pmy,
                         _WORD *pmb, _WORD *pks);
_WORD  evnt_mesag      (_WORD *pbuff);
_WORD  evnt_timer      (_UWORD locnt, _UWORD hicnt);
_WORD  evnt_multi      (_UWORD flags, _UWORD bclk, _UWORD bmsk, _UWORD bst,
                         _UWORD m1flags,
                         _UWORD m1x, _UWORD m1y, _UWORD m1w, _UWORD m1h,
                         _UWORD m2flags,
                         _UWORD m2x, _UWORD m2y, _UWORD m2w, _UWORD m2h,
                         _WORD *mepbuff, _UWORD tlc, _UWORD thc,
                         _WORD *pmx, _WORD *pmy, _WORD *pmb,
                         _WORD *pks, _UWORD *pkr, _WORD *pbr);
_WORD  evnt_event      (MEVENT *pmevent);
_WORD  evnt_dclick     (_WORD rate, _WORD setit);

/****** Object structure *****************************************************/

typedef struct object
{
  _WORD  ob_next;   /* -> object's next sibling       */
  _WORD  ob_head;   /* -> head of object's children   */
  _WORD  ob_tail;   /* -> tail of object's children   */
  _UWORD ob_type;   /* type of object- BOX, CHAR, ... */
  _UWORD ob_flags;  /* flags                          */
  _UWORD ob_state;  /* state- SELECTED, CROSSED, ...  */
  _LONG  ob_spec;   /* "out"- -> anything else        */
  _WORD  ob_x;      /* upper left corner of object    */
  _WORD  ob_y;      /* upper left corner of object    */
  _WORD  ob_width;  /* width of object                */
  _WORD  ob_height; /* height of object               */
} OBJECT;

/****** Menu library *********************************************************/

_WORD  menu_bar        (OBJECT *tree, _WORD showit);
_WORD  menu_icheck     (OBJECT *tree, _WORD itemnum, _WORD checkit);
_WORD  menu_ienable    (OBJECT *tree, _WORD itemnum, _WORD enableit);
_WORD  menu_tnormal    (OBJECT *tree, _WORD titlenum, _WORD normalit);
_WORD  menu_text       (OBJECT *tree, _WORD inum, char *ptext);
_WORD  menu_register   (_WORD pid, char *pstr);

_WORD  menu_unregister (_WORD mid);

_WORD  menu_click      (_WORD click, _WORD setit);

/****** Object library *******************************************************/

#define OB_NEXT(tree, id)    tree [id].ob_next
#define OB_HEAD(tree, id)    tree [id].ob_head
#define OB_TAIL(tree, id)    tree [id].ob_tail
#define OB_TYPE(tree, id)    (tree [id].ob_type & 0xff)
#define OB_EXTYPE(tree, id)  (tree [id].ob_type >> 8)
#define OB_FLAGS(tree, id)   tree [id].ob_flags
#define OB_STATE(tree, id)   tree [id].ob_state
#define OB_SPEC(tree, id)    tree [id].ob_spec
#define OB_X(tree, id)       tree [id].ob_x
#define OB_Y(tree, id)       tree [id].ob_y
#define OB_WIDTH(tree, id)   tree [id].ob_width
#define OB_HEIGHT(tree, id)  tree [id].ob_height

#define ROOT             0
#define NIL             -1 /* nil object index */
#define MAX_LEN         81 /* max string length */
#define MAX_DEPTH        8 /* max depth of search or draw for objects */

#define IP_HOLLOW        0 /* inside patterns */
#define IP_1PATT         1
#define IP_2PATT         2
#define IP_3PATT         3
#define IP_4PATT         4
#define IP_5PATT         5
#define IP_6PATT         6
#define IP_SOLID         7
                           /* system foreground and */
                           /*   background rules    */
#define SYS_FG      0x1100 /*   but transparent     */

#define WTS_FG      0x11a1 /* window title selected */
                           /*   using pattern 2 &   */
                           /*   replace mode text   */
#define WTN_FG      0x1100 /* window title normal   */

#define IBM              3 /* font types */
#define SMALL            5

#define G_BOX           20 /* graphic types of obs */
#define G_TEXT          21
#define G_BOXTEXT       22
#define G_IMAGE         23
#define G_USERDEF       24
#define G_PROGDEF       24 /* for compatibility */
#define G_IBOX          25
#define G_BUTTON        26
#define G_BOXCHAR       27
#define G_STRING        28
#define G_FTEXT         29
#define G_FBOXTEXT      30
#define G_ICON          31
#define G_TITLE         32
#define G_CICON         33

#define NONE        0x0000 /* Object flags */
#define SELECTABLE  0x0001
#define DEFAULT     0x0002
#define EXIT        0x0004
#define EDITABLE    0x0008
#define RBUTTON     0x0010
#define LASTOB      0x0020
#define TOUCHEXIT   0x0040
#define HIDETREE    0x0080
#define INDIRECT    0x0100

#define NORMAL      0x0000 /* Object states */
#define SELECTED    0x0001
#define CROSSED     0x0002
#define CHECKED     0x0004
#define DISABLED    0x0008
#define OUTLINED    0x0010
#define SHADOWED    0x0020
#define WHITEBAK    0x0040
#define DRAW3D      0x0080

#ifndef WHITE
#define WHITE            0 /* Object colors */
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

#define EDSTART          0 /* editable text field definitions */
#define EDINIT           1
#define EDCHAR           2
#define EDEND            3

#define TE_LEFT          0 /* editable text justification */
#define TE_RIGHT         1
#define TE_CNTR          2

typedef struct text_edinfo
{
  char  *te_ptext;     /* ptr to text (must be 1st)     */
  char  *te_ptmplt;    /* ptr to template               */
  char  *te_pvalid;    /* ptr to validation chrs.       */
  _WORD  te_font;           /* font                          */
  _WORD  te_junk1;          /* junk word                     */
  _WORD  te_just;           /* justification- left, right... */
  _UWORD te_color;          /* color information word        */
  _WORD  te_junk2;          /* junk word                     */
  _WORD  te_thickness;      /* border thickness              */
  _WORD  te_txtlen;         /* length of text string         */
  _WORD  te_tmplen;         /* length of template string     */
} TEDINFO;

typedef struct icon_block
{
  _WORD  *ib_pmask;  /* ptr to mask of icon                */
  _WORD  *ib_pdata;  /* ptr to data of icon                */
  char  *ib_ptext;  /* ptr to text of icon                */
  _UWORD ib_char;        /* character in icon                  */
  _WORD  ib_xchar;       /* x-coordinate of ib_char            */
  _WORD  ib_ychar;       /* y-coordinate of ib_char            */
  _WORD  ib_xicon;       /* x-coordinate of icon               */
  _WORD  ib_yicon;       /* y-coordinate of icon               */
  _WORD  ib_wicon;       /* width of icon in pixels            */
  _WORD  ib_hicon;       /* height of icon in pixels           */
  _WORD  ib_xtext;       /* x-coordinate of the icon's text    */
  _WORD  ib_ytext;       /* y-coordinate of the icon's text    */
  _WORD  ib_wtext;       /* width of rectangle for icon's text */
  _WORD  ib_htext;       /* height of icon's text in pixels    */
} ICONBLK;

typedef struct cicon_data
{
	_WORD num_planes;							/* number of planes in the following data          */
	_WORD *col_data;						/* pointer to color bitmap in standard form        */
	_WORD *col_mask;						/* pointer to single plane mask of col_data        */
	_WORD *sel_data;						/* pointer to color bitmap of selected icon        */
	_WORD *sel_mask;						/* pointer to single plane mask of selected icon   */
	struct cicon_data *next_res;	/* pointer to next icon for a different resolution */
}	CICON;

typedef struct cicon_blk
{
	ICONBLK monoblk;		/* default monochrome icon                         */
	CICON *mainlist;		/* list of color icons for different resolutions */
}	CICONBLK;

typedef struct bit_block
{
  _WORD *bi_pdata;   /* ptr to bit forms data  */
  _WORD bi_wb;           /* width of form in bytes */
  _WORD bi_hl;           /* height in scan lines   */
  _WORD bi_x;            /* source x in bit form   */
  _WORD bi_y;            /* source y in bit form   */
  _WORD bi_color;        /* fg color of blt        */
} BITBLK;

typedef struct parm_blk
{
  OBJECT *pb_tree;               /* ptr to obj tree for user defined obj */
  _WORD   pb_obj;                     /* index of user defined object         */
  _WORD   pb_prevstate;               /* old state to be changed              */
  _WORD   pb_currstate;               /* changed (new) state of object        */
  _WORD   pb_x, pb_y, pb_w, pb_h;     /* location of object on screen         */
  _WORD   pb_xc, pb_yc, pb_wc, pb_hc; /* current clipping rectangle on screen */
  _LONG   pb_parm;                    /* same as ub_parm in USERBLK struct    */
} PARMBLK;

typedef struct user_blk
{
#ifdef __MSDOS__
#if HIGH_C
  _WORD (*ub_code) (void);              /* pointer to drawing function */
#else
  _WORD (*ub_code) (void);              /* pointer to drawing function */
#endif
#else
  _WORD cdecl (*ub_code) (PARMBLK *pb); /* pointer to drawing function */
#endif
  _LONG ub_parm;                               /* parm for drawing function   */
} USERBLK;

typedef struct appl_blk /* for compatibility */
{
#ifdef __MSDOS__
#if HIGH_C
  _WORD (*ab_code) (void);              /* pointer to drawing function */
#else
  _WORD (*ab_code) (void);              /* pointer to drawing function */
#endif
#else
  _WORD cdecl (*ab_code) (PARMBLK *pb); /* pointer to drawing function */
#endif
  _LONG ab_parm;                               /* parm for drawing function   */
} APPLBLK;

_WORD  objc_add        (OBJECT *tree, _WORD parent, _WORD child);
_WORD  objc_delete     (OBJECT *tree, _WORD delob);
_WORD  objc_draw       (OBJECT *tree, _WORD drawob, _WORD depth,
                         _WORD xc, _WORD yc, _WORD wc, _WORD hc);
_WORD  objc_find       (OBJECT *tree, _WORD startob, _WORD depth,
                         _WORD mx, _WORD my);
_WORD  objc_offset     (OBJECT *tree, _WORD obj, _WORD *poffx,
                         _WORD *poffy);
_WORD  objc_order      (OBJECT *tree, _WORD mov_obj, _WORD newpos);
_WORD  objc_edit       (OBJECT *tree, _WORD obj, _WORD inchar,
                         _WORD *idx, _WORD kind);
_WORD  objc_change     (OBJECT *tree, _WORD drawob, _WORD depth,
                         _WORD xc, _WORD yc, _WORD wc, _WORD hc,
                         _WORD nestate, _WORD redraw);

/****** Form library *********************************************************/

/* Form flags */

#define FMD_START        0
#define FMD_GROW         1
#define FMD_SHRINK       2
#define FMD_FINISH       3
#define FMD_ASTART       4
#define FMD_AFINISH      5

_WORD  form_do         (OBJECT *form, _WORD start);
_WORD  form_dial       (_WORD dtype, _WORD ix, _WORD iy, _WORD iw, _WORD ih,
                         _WORD x, _WORD y, _WORD w, _WORD h);
_WORD  form_alert      (_WORD defbut, char *astring);
_WORD  form_error      (_WORD errnum);
_WORD  form_center     (OBJECT *tree, _WORD *pcx, _WORD *pcy,
                         _WORD *pcw, _WORD *pch);
_WORD  form_keybd      (OBJECT *form, _WORD obj, _WORD nxt_obj,
                         _UWORD thechar, _WORD *pnxt_obj, _UWORD *pchar);
_WORD  form_button     (OBJECT *form, _WORD obj, _WORD clks, 
                         _WORD *pnxt_obj);

/****** Graphics library ******************************************************/


/* Mouse Forms */

#define ARROW            0
#define TEXT_CRSR        1
#define HOURGLASS        2
#define BUSY_BEE         2 /* for compatibility */
#define POINT_HAND       3
#define FLAT_HAND        4
#define THIN_CROSS       5
#define THICK_CROSS      6
#define OUTLN_CROSS      7
#define M_PUSH         100
#define M_POP          101
#define USER_DEF       255
#define M_OFF          256
#define M_ON           257

/* Mouse Form Definition Block */

typedef struct mfstr
{
  _WORD mf_xhot;
  _WORD mf_yhot;
  _WORD mf_nplanes;
  _WORD mf_fg;
  _WORD mf_bg;
  _WORD mf_mask [16];
  _WORD mf_data [16];
} MFORM;

_WORD  graf_rubbox     (_WORD xorigin, _WORD yorigin, _WORD wmin, _WORD hmin,
                         _WORD *pwend, _WORD *phend);
_WORD  graf_dragbox    (_WORD w, _WORD h, _WORD sx, _WORD sy, _WORD xc, _WORD yc,
                         _WORD wc, _WORD hc, _WORD *pdx, _WORD *pdy);
_WORD  graf_mbox       (_WORD w, _WORD h, _WORD srcx, _WORD srcy,
                         _WORD dstx, _WORD dsty);
void  graf_growbox    (_WORD stx, _WORD sty, _WORD stw, _WORD sth,
                         _WORD finx, _WORD finy, _WORD finw, _WORD finh);
void  graf_shrinkbox  (_WORD finx, _WORD finy, _WORD finw, _WORD finh,
                         _WORD stx, _WORD sty, _WORD stw, _WORD sth);
_WORD  graf_watchbox   (OBJECT *tree, _WORD obj, _WORD instate,
                         _WORD outstate);
_WORD  graf_slidebox   (OBJECT *tree, _WORD parent, _WORD obj,
                         _WORD isvert);
_WORD  graf_handle     (_WORD *pwchar, _WORD *phchar,
                         _WORD *pwbox, _WORD *phbox);
_WORD  graf_mouse      (_WORD m_number, MFORM *m_addr);
void  graf_mkstate    (_WORD *pmx, _WORD *gpmy,
                         _WORD *pmstate, _WORD *pkstate);

/****** Scrap library ********************************************************/

#define SCRAP_CSV   0x0001
#define SCRAP_TXT   0x0002
#define SCRAP_GEM   0x0004
#define SCRAP_IMG   0x0008
#define SCRAP_DCA   0x0010
#define SCRAP_USR   0x8000

_WORD  scrp_read       (char *pscrap);
_WORD  scrp_write      (char *pscrap);

_WORD  scrp_clear      (void);

/****** File selector library ************************************************/

_WORD  fsel_input      (char *pipath, char *pisel,
                         _WORD *pbutton);

_WORD  fsel_exinput    (char *pipath, char *pisel, _WORD *pbutton,
                         char *plabel);

/****** Window library *******************************************************/

/* Window Attributes */

#define NAME        0x0001
#define CLOSER      0x0002
#define FULLER      0x0004
#define MOVER       0x0008
#define INFO        0x0010
#define SIZER       0x0020
#define UPARROW     0x0040
#define DNARROW     0x0080
#define VSLIDE      0x0100
#define LFARROW     0x0200
#define RTARROW     0x0400
#define HSLIDE      0x0800
#define HOTCLOSEBOX 0x1000 /* for compatibility */
#define HOTCLOSE    0x1000

/* wind_calc flags */

#define WC_BORDER        0
#define WC_WORK          1

/* wind_get/wind_set flags */

#define WF_KIND          1
#define WF_NAME          2
#define WF_INFO          3
#define WF_WXYWH         4
#define WF_WORKXYWH      4 /* for compatibility */
#define WF_CXYWH         5
#define WF_CURRXYWH      5 /* for compatibility */
#define WF_PXYWH         6
#define WF_PREVXYWH      6 /* for compatibility */
#define WF_FXYWH         7
#define WF_FULLXYWH      7 /* for compatibility */
#define WF_HSLIDE        8
#define WF_VSLIDE        9
#define WF_TOP          10
#define WF_FIRSTXYWH    11
#define WF_NEXTXYWH     12
#define WF_IGNORE       13
#define WF_NEWDESK      14
#define WF_HSLSIZE      15
#define WF_VSLSIZE      16
#define WF_SCREEN       17
#define WF_TATTRB       18 /* for compatibility */
#define WF_COLOR        18
#define WF_SIZTOP       19 /* for compatibility */
#define WF_DCOLOR       19
#define WF_TOPAP        20 /* for compatibility */
#define WF_OWNER        20
#define WF_BEVENT       24

/* update flags */

#define END_UPDATE       0
#define BEG_UPDATE       1
#define END_MCTRL        2
#define BEG_MCTRL        3
#define BEG_EMERG        4
#define END_EMERG        5

/* arrow message */

#define WA_SUBWIN        1
#define WA_KEEPWIN       2

#define WA_UPPAGE        0 /* Window Arrow Up Page    */
#define WA_DNPAGE        1 /* Window Arrow Down Page  */
#define WA_UPLINE        2 /* Window Arrow Up Line    */
#define WA_DNLINE        3 /* Window Arrow Down Line  */
#define WA_LFPAGE        4 /* Window Arrow Left Page  */
#define WA_RTPAGE        5 /* Window Arrow Right Page */
#define WA_LFLINE        6 /* Window Arrow Left Line  */
#define WA_RTLINE        7 /* Window Arrow Right Line */

_WORD  wind_create     (_UWORD kind, _WORD wx, _WORD wy, _WORD ww, _WORD wh);
_WORD  wind_open       (_WORD handle, _WORD wx, _WORD wy, _WORD ww, _WORD wh);
_WORD  wind_close      (_WORD handle);
_WORD  wind_delete     (_WORD handle);
#if defined(__PUREC__) && !defined(__PORTAES_H__)
_WORD  wind_get        (_WORD w_handle, _WORD wfield, ...);
_WORD  wind_set        (_WORD w_handle, _WORD wfield, ...);
#else
_WORD  wind_get        (_WORD w_handle, _WORD wfield,
                         _WORD *pw1, _WORD *pw2,
                         _WORD *pw3, _WORD *pw4);
_WORD  wind_set        (_WORD w_handle, _WORD wfield,
                         _WORD w1, _WORD w2, _WORD w3, _WORD w4);
#endif
_WORD  wind_find       (_WORD mx, _WORD my);
_WORD  wind_update     (_WORD beg_update);
_WORD  wind_calc       (_WORD wctype, _UWORD kind,
                         _WORD x, _WORD y, _WORD w, _WORD h,
                         _WORD *px, _WORD *py,
                         _WORD *pw, _WORD *ph);

void  wind_new        (void);

_WORD  wind_apfind     (_WORD mx, _WORD my);

/****** Resource library *****************************************************/

/* data structure types */

#define R_TREE           0
#define R_OBJECT         1
#define R_TEDINFO        2
#define R_ICONBLK        3
#define R_BITBLK         4
#define R_STRING         5 /* gets pointer to free strings     */
#define R_IMAGEDATA      6 /* gets pointer to free images      */
#define R_OBSPEC         7
#define R_TEPTEXT        8 /* sub ptrs in TEDINFO              */
#define R_TEPTMPLT       9
#define R_TEPVALID      10
#define R_IBPMASK       11 /* sub ptrs in ICONBLK              */
#define R_IBPDATA       12
#define R_IBPTEXT       13
#define R_BIPDATA       14 /* sub ptrs in BITBLK               */
#define R_FRSTR         15 /* gets addr of ptr to free strings */
#define R_FRIMG         16 /* gets addr of ptr to free images  */

/* used in RSCREATE.C */

typedef struct rshdr
{
  _UWORD rsh_vrsn;
  _UWORD rsh_object;
  _UWORD rsh_tedinfo;
  _UWORD rsh_iconblk;       /* list of ICONBLKS          */
  _UWORD rsh_bitblk;
  _UWORD rsh_frstr;
  _UWORD rsh_string;
  _UWORD rsh_imdata;        /* image data                */
  _UWORD rsh_frimg;
  _UWORD rsh_trindex;
  _UWORD rsh_nobs;          /* counts of various structs */
  _UWORD rsh_ntree;
  _UWORD rsh_nted;
  _UWORD rsh_nib;
  _UWORD rsh_nbb;
  _UWORD rsh_nstring;
  _UWORD rsh_nimages;
  _UWORD rsh_rssize;        /* total bytes in resource   */
} RSHDR;

#define F_ATTR 0           /* file attr for dos_create  */

_WORD  rsrc_load       (char *rsname);
_WORD  rsrc_free       (void);
_WORD  rsrc_gaddr      (_WORD rstype, _WORD rsid, void *paddr);
_WORD  rsrc_saddr      (_WORD rstype, _WORD rsid, void *lngval);
_WORD  rsrc_obfix      (OBJECT *tree, _WORD obj);

/****** Shell library ********************************************************/

_WORD  shel_read       (char *pcmd, char *ptail);
_WORD  shel_write      (_WORD doex, _WORD isgr, _WORD isover, char *pcmd,
                         char *ptail);
_WORD  shel_get        (char *addr, _WORD len);
_WORD  shel_put        (char *addr, _WORD len);
_WORD  shel_find       (char *ppath);
_WORD  shel_envrn      (char * *ppath, char *psrch);

_WORD  shel_rdef       (char *lpcmd, char *lpdir);
_WORD  shel_wdef       (char *lpcmd, char *lpdir);

/****** Extended graphics library ********************************************/

_WORD  xgrf_stepcalc   (_WORD orgw, _WORD orgh, _WORD xc, _WORD yc,
                         _WORD w, _WORD h, _WORD *pcx, _WORD *pcy,
                         _WORD *pcnt, _WORD *pxstep, _WORD *pystep);
_WORD  xgrf_2box       (_WORD xc, _WORD yc, _WORD w, _WORD h, _WORD corners,
                         _WORD cnt, _WORD xstep, _WORD ystep, _WORD doubled);

/*****************************************************************************/

#endif /* __AES__ */
