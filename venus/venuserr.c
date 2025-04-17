/*
 * @(#) Gemini\Venuserr.c
 * @(#) Stefan Eissing, 26. M„rz 1993
 *
 * jr 14.4.95
 */

#include <stdio.h>
#include <string.h>
#include <aes.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "vs.h"
#include "changbox.rh"

#include "util.h"
#include "stand.h"



/* externals
 */
extern OBJECT *pchangebox;

#define DEBUG	1

/* internal texts
 */
#define NlsLocalSection	"VenusErr"
enum NlsLocalText{
T_ERR,		/*[Oh*/
T_CHOICE,	/*[Nein|[Ja*/
T_INFO,		/*[OK*/
T_INFOFOLLOW,		/*[Weiter*/
};

#define PGM_ALERT	"Gemini:|"
#define PGM_SIZE	8

void ErrorAlert (const char *str)
{
	DialAlert (ImSqExclamation(), str, 0, NlsStr (T_ERR));
}

/*
 * void venusErr(const char *errmessage)
 * makes form_alert with errmessage
 */
word venusErr(const char *errmessage,...)
{
	char tmp[1024];
	va_list argpoint;
	word retcode;
	
	strcpy (tmp, PGM_ALERT);
	va_start (argpoint, errmessage);
	vsprintf (tmp + PGM_SIZE, errmessage, argpoint);
	va_end (argpoint);
	retcode = DialAlert (ImSqExclamation (), tmp, 0, NlsStr (T_ERR));
	
	return retcode;
}

word venusDebug(const char *s,...)
{
#if DEBUG
	char tmp[1024];
	va_list argpoint;
	
	strcpy (tmp, PGM_ALERT);
	va_start (argpoint, s);
	vsprintf (tmp + PGM_SIZE, s, argpoint);
	va_end(argpoint);
	return DialAlert(ImSqExclamation(), tmp, 0, NlsStr (T_ERR));
#else
	(void)s;
	return 1;
#endif
}

/*
 * word venusChoice(const char *message)
 * return 1, if the answer was "Yes", 0 otherwise
 */
word venusChoice(const char *message,...)
{
	char tmp[1024];
	va_list argpoint;
	
	strcpy (tmp, PGM_ALERT);
	va_start (argpoint, message);
	vsprintf (tmp + PGM_SIZE, message, argpoint);
	va_end (argpoint);
	return DialAlert (ImSqQuestionMark(), tmp, 1, NlsStr (T_CHOICE));
}

word venusInfo(const char *s,...)
{
	char tmp[1024];
	va_list argpoint;
	
	strcpy (tmp, PGM_ALERT);
	va_start (argpoint, s);
	vsprintf (tmp + PGM_SIZE, s, argpoint);
	va_end (argpoint);
	return DialAlert (ImInfo(), tmp, 0, NlsStr (T_INFO));
}

word venusInfoFollow(const char *s,...)
{
	char tmp[1024];
	va_list argpoint;
	
	va_start(argpoint,s);
	vsprintf(tmp,s,argpoint);
	va_end(argpoint);
	return DialAlert(ImInfo(), tmp, 0, NlsStr (T_INFOFOLLOW));
}

/* 
 * word changeDisk(const char drive,const char *label)
 * give possibility to change disk(TRUE) or cancel command(FALSE)
 */
word changeDisk(const char drive,const char *label)
{
	DIALINFO d;
	word retcode;
	char *pdisk;
	word tmp_mouse;
	MFORM tmp_mform;
	
	GrafGetForm (&tmp_mouse, &tmp_mform);
	GrafMouse (ARROW, NULL);
	
	pdisk = pchangebox[CHANDRIV].ob_spec.tedinfo->te_ptext;
	pchangebox[CHANLABL].ob_spec.tedinfo->te_ptext = (char *)label;
	
	pdisk[0] = drive;
	
	DialCenter(pchangebox);
	DialStart(pchangebox,&d);
	DialDraw(&d);
	
	retcode = DialDo(&d,0) & 0x7FFF;
	setSelected(pchangebox,retcode,FALSE);
	
	DialEnd(&d);
	GrafMouse (tmp_mouse, &tmp_mform);
	
	return (retcode == CHANOK);
}
