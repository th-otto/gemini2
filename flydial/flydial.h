/*
	@(#)FlyDial/flydial.h
	
	Julian F. Reschke, 3. Oktober 1996
	bitte aufmerksam den GEN-File durchlesen!
*/

#ifndef __FLYDIAL__
#define __FLYDIAL__

#if defined(__MSDOS__)
#	include "vdi.h"
#	include "aes.h"
#elif defined (__TURBOC__)
#	include <vdi.h>
#	include <aes.h>
#	include <tos.h>
#  define _DTA DTA
#  define dta_name d_fname
#  define dta_attribute d_attrib
#  define dta_size d_length
#  define dta_time d_time
#  define dta_date d_date
#else
#   define __GEMLIB_OLDNAMES /* FIXME later */
#	include <gem.h>
#	include <osbind.h>
#endif
#include <stddef.h>

#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  elif defined(__TURBOC__)
#    define _WORD int
#  else
#    define _WORD short
#  endif
#endif
#ifndef _BOOL
#  define _BOOL int
#endif

#ifdef __GNUC__
#  define cdecl
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#ifndef TXT_THICKENED
#define TXT_NORMAL			0x0000
#define TXT_THICKENED		0x0001
#define TXT_LIGHT			0x0002
#define TXT_SKEWED			0x0004
#define TXT_UNDERLINED		0x0008
#define TXT_OUTLINED		0x0010
#define TXT_SHADOWED		0x0020
#endif

#if defined(__PORTAES_H__) || defined(_GEMLIB_H_)
#  define EVNT_TIME(lo, hi)  (((unsigned long)(unsigned short)(hi) << 16) | ((unsigned long)(unsigned short)(lo)))
#else
#  define EVNT_TIME(lo, hi) lo, hi
#endif

#define ALCENTER "\001"
#define ALRIGHT "\002"


/* FlyDial extended object types */

#define FD_ESELSOHR		17
#define FD_SHORTCUT		18
#define FD_UNDERLINE	19
#define FD_TITLELINE	20
#define FD_HELPKEY		21
#define FD_CYCLEFRAME	22
#define FD_VSTRETCH		23
#define FD_3DFRAME		24
#define FD_EDITFRAME	25
#define FD_CUT			26


extern _WORD DialWk;	/* wird von allen meinen Routinen benutzt */

typedef void *(*DIALMALLOC) (size_t);
typedef void (*DIALFREE) (void *);

extern DIALMALLOC dialmalloc;
extern DIALFREE dialfree;

/*
	FlyDial Raster Manager
*/

typedef struct RDB
	{
	_WORD	saved;

	char	filename[14];
	_WORD	handle;
	_WORD	fullSlices;
	_WORD	lastSliceHeight;
	_WORD	sliceHeight;
	_WORD	scanlineSize;
	_WORD	planes;
	}
RDB;

extern MFDB RastScreenMFDB;

/* DOS-only: Initialize the raster manager and set the tempdir.*/
_WORD RastInit(char *tempdir);

/* DOS-only: Terminate the raster manager */
void RastTerm(void);

/* DOS-only: write a part of the screen to disk */
_BOOL RastDiskSave(RDB *rdb, _WORD x, _WORD y, _WORD w, _WORD h);

/* DOS-only: retrieve a part of the screen from disk */
_BOOL RastDiskRestore(RDB *rdb, _WORD x, _WORD y, _WORD w, _WORD h);

/* Grî·e eines Bildschirmausschnitts in Bytes */
unsigned long RastSize (_WORD w, _WORD h, MFDB *TheBuf);

/* Bildschirmausschnitt in Buffer speichern */
void RastSave (_WORD x, _WORD y, _WORD w, _WORD h, _WORD dx, _WORD dy,
	MFDB *TheBuf);

/* Bildschirmausschnitt aus Buffer zurÅckholen */
void RastRestore (_WORD x, _WORD y, _WORD w, _WORD h, _WORD sx, _WORD sy, MFDB *TheBuf);

void RastBufCopy (_WORD sx, _WORD sy, _WORD w, _WORD h, _WORD dx, _WORD dy, MFDB *TheBuf );

/* Setze udsty auf eine so gepunktete Linie, daû bei einem grauen
   Desktophintergrund eine schwarze Linie erscheint (xy[]: Anfangs-
   und Endpunkt der Linie */
void RastSetDotStyle (_WORD ha, _WORD *xy);

/* Malt gepunktetes Rechteck */
void RastDotRect (_WORD ha, _WORD x, _WORD y, _WORD w, _WORD h);

void RastDrawRect (_WORD ha, _WORD x, _WORD y, _WORD w, _WORD h);
void RastTrans (void *saddr, _WORD swb, _WORD h, _WORD handle);
void RastGetQSB(void **qsbAddr, _WORD *qsbSize);


/* Setzt FlyDial-Konfiguration (passiert auch automatisch bei
DialInit aus $FLYDIAL_PREFS */

void DialSetConf (char *conf);

/*
	ersetzt form_alert (locker)

	Image: 		Pointer auf Bitimage
	String:		Text, UNFORMATIERT. Senkrechter Strich fÅr 'harten'
				Umbruch erlaubt.
	Default:	Nummer des Default-Buttons (zero-based)
	Buttons:	Button-Namen, getrennt durch '|', Shortcuts durch
				vorangestelltes '[' gekennzeichnet.
*/

_WORD DialAlert (BITBLK *Image, const char *String, _WORD Default, const char *Buttons);

/* DialAlert mit formatierten String. ACHTUNG: andere Reihenfolge
   als bei DialAlert! */

_WORD DialAlertf (BITBLK *Bit, const char *but, _WORD def, const char *msg,	...);


/* Liefert zurÅck, ob beim letzten DialAlert das Hintergrund-
   Retten geklappt hat */

_WORD DialSuccess (void);

/*
	Unterschied zu DialAlert: Icon ist animiert

	Image:		Zeiger auf Liste von BITBLKS, NULL-terminiert
	Durations:	Zeiger auf Liste von Warteintervallen (ms)
*/

_WORD DialAnimAlert (BITBLK **Images, _WORD *Durations, const char *String,
	_WORD Default, const char *Buttons,
	_WORD cdecl (*callback)(struct _animbitblk *block),
	long parm);

typedef struct
{
	OBJECT 	*tree;
	MFDB 	bg_buffer;
	_WORD 	x, y, w, h;
	_WORD 	offset;
#ifdef __MSDOS_
	RDB		rdb;
#endif
} DIALINFO;



/*
	Zum Anfang des Dialogs aufrufen

	Return: 0: Hintergrund konnte NICHT gebuffert werden!

	Hinweis: nach DialStart darf man NICHT mehr mit form_- oder
	DialCenter die Position verÑndern!!!!!
*/
_BOOL DialStart (OBJECT *TheTree, DIALINFO *D);
_BOOL DialExStart (OBJECT *TheTree, DIALINFO *D, _BOOL use_qsb);

/*
	Bildschirmplatz wieder freigeben
*/
void DialEnd (DIALINFO *D);


/* Dialogbox Åber den Bildschirm bewegen */

_BOOL DialMove (DIALINFO *D, _WORD sx, _WORD sy, _WORD sw, _WORD sh);


/*
	équivalent zu form_do

	aktiviert die Move-Routine bei Anklicken eines Objekt mit
	extended object type 17 und TOUCHEXIT-Status
	liefert in StartOb auch das AKTUELLE Edit-Obj ZURöCK!!!
	
	DialXDo: zusÑtzlich wird die benutzte Taste zurÅckgeliefert
	(und die Mausposition)
*/
_WORD DialDo (DIALINFO *D, _WORD *StartOb);
_WORD DialXDo (DIALINFO *D, _WORD *StartOb, _WORD *kstate, _WORD *key, _WORD *mx, _WORD *my, _WORD *mb);

void DialDraw (DIALINFO *D);

void DialCenter (OBJECT *tree);

/*
	Initialisiert bzw. deinitialisiert die Dial-Routinen
	malloc() und free() sind jetzt konfigurierbar, zB
	DialInit (malloc, free);
*/
_WORD DialInit (void *mallocfunc, void *freefunc);


void DialExit (void);


/* Frontends fÅr DialAlertf */

void DialSay (const char *what, ...);
void DialWarn (const char *what, ...);
void DialFatal (const char *what, ...);
_WORD DialAsk (const char *bt, _WORD def, const char *what, ...);

/* setzt Strings fÅr obige Funktionen */
void DialSetButtons (const char *cont, const char *abbruch, const char *ok);




/*
	Ersetzen jeweils die gleichnamigen
*/

_WORD FormButton (OBJECT *tree, _WORD obj, _WORD clicks, _WORD *nextobj);
_WORD FormWButton (_WORD whandle, OBJECT *tree, _WORD obj, _WORD clicks, _WORD *nextobj);
_WORD FormKeybd (OBJECT *tree, _WORD edit_obj, _WORD next_obj, _WORD key, _WORD kstate, _WORD *onext_obj, _WORD *okr);
_WORD FormXDo (OBJECT *tree, _WORD *startfld, _WORD *ks, _WORD *kr, _WORD *mx, _WORD *my, _WORD *mb);
_WORD FormDo (OBJECT *tree, _WORD startfld);


/*
	Installiert einen neuen Keyboard-Handler in FormDo
*/

typedef _WORD (*FORMKEYFUNC) (OBJECT *, _WORD edit_obj, _WORD next_obj, _WORD key, _WORD ks, _WORD *onext_obj, _WORD *okey);
void FormSetFormKeybd (FORMKEYFUNC fun);
FORMKEYFUNC FormGetFormKeybd (void);

typedef _BOOL (*VALFUN)(OBJECT *tree, _WORD ob, _WORD *chr, _WORD *shift, _WORD idx);
void FormSetValidator (const char *valchars, VALFUN *funs);


/* Help-Handler */

typedef void (*FORMHELPFUN) (OBJECT *, _WORD, _WORD, _WORD);
void FormSetHelpFun (FORMHELPFUN fun, _WORD delay);


extern _WORD HandStdWorkIn[];
extern _WORD HandAES, HandXSize, HandYSize, HandBXSize, HandBYSize;

/* Id und Grîûe des Systemzeichensatzes (*Height: fÅr vst_height) */

extern _WORD HandSFId, HandSFHeight, HandSIId, HandSIHeight;


_BOOL HandFast (void);
_WORD HandYText (void);
void HandScreenSize (_WORD *x, _WORD *y, _WORD *w, _WORD *h);
void HandInit (void);
void HandClip (_WORD x, _WORD y, _WORD w, _WORD h, _WORD flag);


#define G_ANIMIMAGE 42

typedef struct _animbitblk
{
	BITBLK **images;	/* Liste der Bitblocks, durch Nullpointer
						abgeschlossen */
	_WORD *durations;
	_WORD current;
	
    _WORD cdecl (*callback)(struct _animbitblk *block);
	long parm;
} ANIMBITBLK;

_WORD cdecl ObjcDefaultCallback (ANIMBITBLK *p);


/*
	Alle Routinen sollten call-kompatibel zu den Vorbildern sein

	Wichtigster Unterschied:

	Extended object type 18: spezielle Buttons

	- durch vorangestelltes '[' wird Shortcut gekennzeichnet.
	  Beispiel: "[Abbruch" ergibt Text "Abbruch" und kann auch
	  mit ALT-A aktiviert werden

	- handelt es sich bei dem Button um einen Exit-Button, dann
	  wird die Hîhe um 2 Pixel vergrîûert, um Platz fÅr die
	  Unterstreichung zu lassen

	- bei anderen Button-Typen wird der Text NEBEN einen kleinen
	  Knopf gesetzt, der -- je nach 'Radio Button' oder nicht --
	  verschieden aussieht


	Extended object type 17: Dialog-Mover

	FÅr ein 'Eselsohr' wie in den Alertboxen bitte folgendes
	Objekt benutzen:

	- I-BOX, Extended type 17, TOUCHEXIT, OUTLINE, CROSSED

	
	Extended object type 19: UNDERLINE

	Das Objekt wird mit einer horizontalen Linie unterstrichen
	(10.6.1989)

	Extended object type 20: TITLELINE

	Speziell fÅr beschriftete Rahmen. Man nehme einen normalen
	Button (mit Outline). Bei TITLELINE-Objekten wird der
	Text dann nicht in der Mitte zentriert, sondern direkt
	(type-over) Åber dem oberen Rand ausgegeben. Im Beispiel-RSC
	ansehen!

	Extended object type 21: HELP-Taste
	
	Objekt kann auch durch DrÅcken der HELP-Taste betÑtigt
	werden. Zum Design: mîglichst wie in SCSI-Tool, also:
	
	Text, Outlined, Shadowed, Font klein, zentriert, 'HELP'

*/ 

#ifdef __MSDOS__
void ObjcMyButton(void);
void ObjcAnimImage(void);
#else
_WORD cdecl ObjcMyButton (PARMBLK *pb);
_WORD cdecl ObjcAnimImage (PARMBLK *pb);
#endif

void ObjcInit(void); /* private; will be called by DialInit() */
_WORD ObjcChange (OBJECT *tree, _WORD obj, _WORD resvd, _WORD cx, _WORD cy, _WORD cw, _WORD ch, _WORD newstate, _WORD redraw);
void ObjcWChange (_WORD whandle, OBJECT *tree, _WORD obj, _WORD resvd, _WORD cx, _WORD cy, _WORD cw, _WORD ch, _WORD newstate, _WORD redraw);
void ObjcXywh (OBJECT *tree, _WORD obj, GRECT *p);
void ObjcToggle (OBJECT *tree, _WORD obj);
void ObjcWToggle (_WORD whandle, OBJECT *tree, _WORD obj);
_WORD ObjcGParent (OBJECT *tree, _WORD obj);
void ObjcDsel (OBJECT *tree, _WORD obj);
void ObjcWDsel (_WORD whandle, OBJECT *tree, _WORD obj);
void ObjcSel (OBJECT *tree, _WORD obj);
void ObjcWSel (_WORD whandle, OBJECT *tree, _WORD obj);
_WORD ObjcDraw (OBJECT *tree, _WORD startob, _WORD depth, _WORD xclip, _WORD yclip, _WORD wclip, _WORD hclip);
_BOOL ObjcTreeInit (OBJECT *tree);
_BOOL ObjcRemoveTree (OBJECT *tree);
_BOOL ObjcOffset (OBJECT *tree, _WORD obj, _WORD *x, _WORD *y);
OBSPEC *ObjcGetObspec (OBJECT *tree, _WORD index);
_WORD ObjcDeepDraw(void);

/*
	Die vertikalen Koordinaten der Objekte im Baum werden mit
	a/b multipliziert. 1.5-zeiliger Zeilenabstand also mittels
	a=3, b=2. Ansehen!
*/
void ObjcVStretch (OBJECT *tree, _WORD ob, _WORD a, _WORD b);

/* Stelle fest, ob ein Objekt vom Typ UNDERLINE ist */
_WORD ObjcIsUnderlined (OBJECT *tree, short ob);



#define FLYDIALMAGIC 0x464C5944L /* 'FLYD' */

typedef struct
{
	long	me_magic;		/* == FLYDIALMAGIC */
	OBJECT	*me_sub;
	_WORD	me_obnum;
	char	*me_title;
	USERBLK	me_ublk;
} MENUSPEC;

void PoppInit (void);		/* tut das Offensichtliche */
void PoppExit (void);		/* ebenso */
void PoppResult (OBJECT **Tree, _WORD *obj);
void PoppChain (OBJECT *ParentTree, _WORD ParentOb, OBJECT *SubTree, _WORD ChildOb, MENUSPEC *TMe);
				/* entweder muû ein Zeiger auf eine zu fuellende
				MENUSPEC-Struktur Åbergeben werden, oder NULL
				(dann wird der benoetigte Speicher gemalloct) */
void PoppUp (OBJECT *Tree, _WORD x, _WORD y, OBJECT **ResTree, _WORD *resob);


/* ersetzt gestrichelte Linie in MenÅs durch durchgezogene */
void MenuSet2ThinLine (OBJECT *tree, _WORD ob);

/* macht die MenÅs im MenÅbaum ein Zeichen schmaler (wg. Kuma-Resource)
   (wenn tune != FALSE.) und Ñndert die gestrichelten Linien mit 
   MenuSet2ThinLine; patcht Breite der MenÅleiste */
void MenuTune (OBJECT *tree, _BOOL tune);

/* wie menu_ienable, nur wird vorher der Status gecheckt und
   das AES nur bei Bedarf aufgerufen */
_WORD MenuItemEnable (OBJECT *menu, _WORD item, _WORD enable);

/* fast wie menu_bar, nur wird unter AES >= 0x400 auf Ownership
   getestet */
   
#define MENUBAR_INSTALL	10
#define MENUBAR_REMOVE 11
#define MENUBAR_REDRAW 12
_WORD MenuBar (OBJECT *tree, _WORD opcode);

/* aktuellen Menubar disbablen oder enablen */
void MenuBarEnable (_WORD enable);

/* Grîûe eines Bildschirmausschnitts in Bytes */
unsigned long RastSize (_WORD w, _WORD h, MFDB *TheBuf);

/* Bildschirmausschnitt in Buffer speichern */
void RastSave (_WORD x, _WORD y, _WORD w, _WORD h, _WORD dx, _WORD dy, MFDB *TheBuf);

/* Bildschirmausschnitt aus Buffer zurÅckholen */
void RastRestore (_WORD x, _WORD y, _WORD w, _WORD h, _WORD sx, _WORD sy, MFDB *TheBuf);

void RastBufCopy (_WORD sx, _WORD sy, _WORD w, _WORD h, _WORD dx, _WORD dy, MFDB *TheBuf);

/* Setze udsty auf eine so gepunktete Linie, daû bei einem grauen
   Desktophintergrund eine schwarze Linie erscheint (xy[]: Anfangs-
   und Endpunkt der Linie */
void RastSetDotStyle (_WORD handle, _WORD *xy);

/* Malt gepunktetes Rechteck */
void RastDotRect (_WORD handle, _WORD x, _WORD y, _WORD w, _WORD h);

void RastDrawRect (_WORD handle, _WORD x, _WORD y, _WORD w, _WORD h);
void RastTrans (void *saddr, _WORD swb, _WORD h, _WORD handle);


void RectAES2VDI (_WORD x, _WORD y, _WORD w, _WORD h, _WORD *xy);
void RectGRECT2VDI (const GRECT *g, _WORD *xy);

_BOOL RectInter (_WORD x1,_WORD y1,_WORD w1,_WORD h1,_WORD x2,_WORD y2,_WORD w2,_WORD h2,
		  _WORD *x3,_WORD *y3,_WORD *w3,_WORD *h3);

_BOOL RectOnScreen (_WORD x, _WORD y, _WORD w, _WORD h);
_BOOL RectInside (_WORD x, _WORD y, _WORD w, _WORD h, _WORD x2, _WORD y2);
_BOOL RectGInter (const GRECT *a, const GRECT *b, GRECT *c);
_BOOL RectG_WORDer (GRECT *a, GRECT *b, GRECT *c);
void RectClipWithScreen (GRECT *g);

BITBLK *ImQuestionMark (void);
BITBLK *ImHand (void);
BITBLK *ImInfo (void);
BITBLK *ImFinger (void);
BITBLK *ImBomb (void);
BITBLK *ImPrinter (void);
BITBLK *ImDisk (void);
BITBLK *ImDrive (void);
BITBLK *ImExclamation (void);
BITBLK *ImSignQuestion (void);
BITBLK *ImSignStop (void);
BITBLK *ImSqExclamation (void);
BITBLK *ImSqQuestionMark (void);


void WindUpdate (_WORD mode);
void WindRestoreControl (void);

/* Fenster auf sichtbarem Bildschirm zentrieren (ruft intern
   DialCenter auf und berÅcksichtigt daher VSCR) */
  
void WindCenter (GRECT *g);


void GrafMouse (_WORD num, MFORM *form);
void GrafGetForm (_WORD *num, MFORM *form);

/* Stelle fest, ob die Maus noch Åber tree[obj] steht und die
   entsprechenden Maustasten gedrÅckt sind */

_WORD GrafMouseIsOver (OBJECT *tree, _WORD obj, _WORD btmask);


/*
	rel: 1: relativ zur Mausposition
	cob: Objekt, das unter der Maus erscheinen soll (oder -1)
	mustbuffer: 1: bei Speichermangel abbrechen

    wenn mustbuffer gesetzt ist und der Speicher nicht reicht,
    erhÑlt man in resob -2 zurÅck

*/
void JazzUp (OBJECT *Tree, _WORD x, _WORD y, _BOOL rel, _WORD cob, _BOOL mustbuffer, OBJECT **ResTree, _WORD *resob);

/*
Die Idee ist folgende: man hat einen Button, den Cycle-Button und das
Poppup-MenÅ. Das MenÅ enthÑlt mehrere EintrÑge vom gleichen Typ wie
der Button. Der ob_spec im Button -- also die aktuelle Einstellung --
ist IDENTISCH mit einem der ob_specs im MenÅ. Beim Aufruf gibt man
einen Zeiger auf den ob_spec des Buttons mit. JazzSelect sucht den
Eintrag im MenÅ, setzt einen Haken davor (also zwei Zeichen Leerraum
davor) und zentriert das MenÅ entsprechend. NACH JazzSelect zeigt der
Zeiger auf den ob_spec des zuletzt im MenÅ selektierten Objekts. Dieser
ob_spec wird nun in den Button eingetragen un der Button wieder
neu gemalt. docycle gibt an, ob ein PoppUp erscheinen soll (0) oder
nur `weitergeblÑttert' werden soll (-2). Dieser Modus wird auch
benutzt, wenn das Poppup-MenÅ nicht aufgerufen werden konnte (Speicher-
platz).

Noch ein Hinweis zu den Popups: man sollte einen String mit
Shortcut davor setzen. Wenn der Shortcut gedÅrckt wird, JazzUp
hochklappen (JazzId). Wenn zusÑtzlich eine der Shift-Tasten
gedrÅckt ist (Kbshift (-1) & 3), cyclen (JazzCycle).
*/

/*
	Box, ob: Tree und Objekt des betr. Knopfes
	txtob: Objektnummer des Beschriftungsstrings bzw -1
	Popup: Tree mit dem Popup-MenÅ
	docheck: 1: aktuellen Wert mit Haken versehen
	docycle: 0: Popup, -1: einen Wert zurÅck, 1: einen Wert vor
					-2: cyclen
	obs: neuer obspec
	returns: sel. Objekt bzw. -1
*/

_WORD JazzSelect (OBJECT *Box, _WORD ob, _WORD txtob, OBJECT *Popup, _BOOL docheck, _BOOL docycle, long *obs);


/* Diese beiden Funktionen sind Frontends fÅr JazzSelect, die
   folgende zusÑtzliche FunktionalitÑt haben:
   
   - der ob_spec im Button wird automatisch aktualisiert und
     neugemalt
   - bei JazzId wird der Cycle-Button invertiert (und bleibt es,
     bis man die Maustaste loslÑût)
*/

_WORD JazzCycle(OBJECT *Box, _WORD ob, _WORD cyc_button, OBJECT *Popup, _BOOL docheck);
_WORD JazzId(OBJECT *Box, _WORD ob, _WORD txtob, OBJECT *Popup, _BOOL docheck);

typedef struct
{
	_WORD	id;
	_WORD base_id;
	struct {
	unsigned isprop : 1;
	unsigned isfsm : 1;
	unsigned famentry : 1;
	} flags;
	_WORD nextform;
	_WORD weight;
	_WORD classification;
	char *fullname;
	char *fontname;
	char *facename;
	char *formname;
} FONTINFO;

typedef struct
{
	_WORD handle;		/* vdi-handle auf dem gearbeitet wird */
	_WORD loaded;		/* Fonts wurden geladen */
	_WORD sysfonts;	/* Anzahl der Systemfonts */
	_WORD addfonts;	/* Anzahl der Fonts, die geladen wurden */
	FONTINFO *list;	/* Liste von FONTINFO-Strukturen */
} FONTWORK;

/* Bei FontLoad muû das handle gesetzt werden und beim ersten
 * Aufruf muû loaded FALSE(0) sein. Weiterhin muû in sysfonts die
 * Anzahl der Fonts stehen, die man beim ôffnen der Workstation
 * in work_out bekommen hat.
 */
void FontLoad (FONTWORK *fwork);

/*
 * Baut die Liste list in fwork auf, falls noch nicht besetzt.
 * Bei RÅckgabe von FALSE hat das Ganze nicht geklappt.
 */ 
_WORD FontGetList (FONTWORK *fwork, _BOOL test_prop, _BOOL test_fsm);
/* Liste ist readonly */

/* Deinstalliert die Fonts auf der Workstation handle und gibt,
 * falls list != NULL ist die Liste wieder frei
 */
void FontUnLoad (FONTWORK *fwork);

/* Fontgrîûe unter BerÅcksichtigung von FSMGDOS setzen */
_WORD FontSetPoint (FONTWORK *F, _WORD handle, _WORD id, _WORD point,
	_WORD *cw, _WORD *ch, _WORD *lw, _WORD *lh);

/* Feststellen, ob FSM-Font */
_BOOL FontIsFSM (FONTWORK *F, _WORD id);

/* FONTINFO-Struktur ermitteln */
FONTINFO *FontGetEntry (FONTWORK *F, _WORD id);

/* Systemfonts erfragen */
void FontAESInfo(_WORD vdihandle, _WORD *pfontsloaded, _WORD *pnormid, _WORD *pnormheight, _WORD *piconid, _WORD *piconheight);

_WORD vst_arbpt (_WORD handle, _WORD point, _WORD *char_width, _WORD *char_height, _WORD *cell_width, _WORD *cell_height);
void vqt_devinfo (_WORD handle, _WORD devnum, _WORD *devexists, char *devstr);

_WORD objc_sysvar (_WORD mode, _WORD which, _WORD in1, _WORD in2, _WORD *out1, _WORD *out2);

									/* appl_control[ap_cwhat]: 						*/
#define APC_APP			0				/* Zeiger auf APP-Struktur liefern			*/
#define APC_VANISHED	1				/* Applikation ist MiNT-mÑûig terminiert	*/
#define APC_HIDE		10				/* Anwendung ausblenden						*/
#define APC_SHOW		11				/* Anwendung einblenden						*/
#define APC_TOP			12				/* Anwendung in den Vordergrund bringen 	*/
#define APC_HIDENOT		13				/* Alle anderen ausblenden					*/


_WORD appl_control(_WORD ap_cid, _WORD ap_cwhat, void *ap_cout);

_WORD appl_xgetinfo (_WORD ap_gtype, _WORD *ap_gout1, _WORD *ap_gout2, _WORD *ap_gout3, _WORD *ap_gout4);
	
#define FL3DMASK	0x0600
#define	FL3DNONE	0x0000
#define FL3DIND		0x0200
#define FL3DBAK		0x0400
#define FL3DACT		0x0600



#endif
