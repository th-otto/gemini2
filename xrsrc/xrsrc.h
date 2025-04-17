/*****************************************************************************/
/*                                                                           */
/* Modul: XRSRC.H                                                            */
/* Datum: 18.02.92                                                           */
/*                                                                           */
/*****************************************************************************/

#ifndef __XRSRC__
#define __XRSRC__

/****** DEFINES **************************************************************/

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

/****** TYPES ****************************************************************/

typedef struct
{
  _UWORD rsh_vrsn;				/* should be 3														     */
  _UWORD rsh_extvrsn;		/* not used, initialised to 'IN' for Interface */
  _ULONG rsh_object;
  _ULONG rsh_tedinfo;
  _ULONG rsh_iconblk; 		/* list of ICONBLKS			  				 	*/
  _ULONG rsh_bitblk;
  _ULONG rsh_frstr;
  _ULONG rsh_string;
  _ULONG rsh_imdata;			/* image data					  					*/
  _ULONG rsh_frimg;
  _ULONG rsh_trindex;
  _ULONG rsh_nobs; 			/* counts of various structs 					*/
  _ULONG rsh_ntree;
  _ULONG rsh_nted;
  _ULONG rsh_nib;
  _ULONG rsh_nbb;
  _ULONG rsh_nstring;
  _ULONG rsh_nimages;
  _ULONG rsh_rssize;			/* total bytes in resource   */
} RSXHDR;

/****** VARIABLES ************************************************************/

/****** FUNCTIONS ************************************************************/

_WORD    xrsrc_load  (const char *re_lpfname, _WORD *pglobal);
_WORD    xrsrc_free  (_WORD *pglobal);
_WORD    xrsrc_gaddr (_WORD re_gtype, _WORD re_gindex, void *re_gaddr, _WORD *pglobal);
_WORD    xrsrc_saddr (_WORD re_stype, _WORD re_sindex, void *re_saddr, _WORD *pglobal);
_WORD    xrsrc_obfix (OBJECT *re_otree, _WORD re_oobject);

_BOOL init_xrsrc  (_WORD vdi_handle, GRECT *desk, _WORD gl_wbox, _WORD gl_hbox);
void    term_xrsrc  (void);

#endif /* __XRSRC__ */
