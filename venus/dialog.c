/*
 * @(#) Gemini\Dialog.c
 * @(#) Stefan Eissing, 29. Oktober 1994
 *
 * description: Dialogstuff for Desktop
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <flydial\flydial.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\fileutil.h"
#include "..\mupfel\mupfel.h"

#include "vs.h"
#include "copyinfo.rh"
#include "opendbox.rh"
#include "optiobox.rh"
#include "ginsave.rh"

#include "window.h"
#include "color.h"
#include "dialog.h"
#include "infofile.h"
#include "stand.h"
#include "util.h"
#include "venuserr.h"


/* externals
 */
extern OBJECT *pshowbox,*pcopyinfo;
extern OBJECT *poptiobox,*popendbox, *pginsave;


/* internal texts
 */
#define NlsLocalSection "G.dialog"
enum NlsLocalText{
T_COPYRIGHT,	/*Dies ist ein Shareware-Programm.
 Alle Rechte bleiben beim Autor dieses Programms.||
Es ist keine Registrierung erforderlich; dies kann sich in knftigen
 Versionen aber „ndern. Teile des in Gemini verwendeten Programmcodes
 unterliegen dem Copyright von Gereon Steffens. Die fliegenden
 Dialoge sind von Julian F. Reschke.
 Die Routinen des Console-Fensters stammen von Arnd Beižner.||
Vielen Dank!*/
T_NOSPACE,	/*Die Aufl”sung ist zu gering, um diesen Dialog darzustellen!*/
T_MAKE_NAME,	/*Im Prinzip k”nnen Sie die GIN-Datei benennen, wie
 Sie wollen. Einige Namen haben aber eine besondere Bedeutung. So
 versucht Gemini beim Start zuerst eine Datei fr die passende
 Aufl”sung zu laden (z.B. 06400400.GIN fr die hohe ST-Aufl”sung).
 Ansonsten wird GEMINI.GIN geladen. Soll ein passender Name
 eingesetzt werden?*/
T_NONAME,	/*Sie sollten einen Namen angeben!*/
};


/* Dialog about copyright and shareware information
 */
word InfoAboutGemini (void)
{
	word retcode;
	DIALINFO d;

	pcopyinfo[BANANA].ob_spec.bitblk->bi_color = 
		ColorMap (YELLOW) & 0x0F;

	DialCenter (pcopyinfo);
	DialStart (pcopyinfo,&d);
	DialDraw (&d);

	do
	{
		retcode = DialDo (&d, 0) & 0x7FFF;
		setSelected (pcopyinfo, retcode, FALSE);
		if (retcode == GMNIINFO)
		{
			if (venusInfo (NlsStr (T_COPYRIGHT), PGMNAME) < 0)
			{
				venusErr (NlsStr (T_NOSPACE));
				DialDraw (&d);
			}
		}
		fulldraw (pcopyinfo, retcode);
	}
	while (retcode == GMNIINFO);

	DialEnd (&d);

	return retcode;
}


/* Dialog for general options
 */
void EditVerbosity (void)
{
	word retcode;
	DIALINFO d;

	DialCenter (poptiobox);
	DialStart (poptiobox,&d);
	
	setSelected (poptiobox, OPTEXIST, !NewDesk.replaceExisting);
	setSelected (poptiobox, CPVERB, !NewDesk.silentCopy);
	setSelected (poptiobox, RMVERB, !NewDesk.silentRemove);
	setSelected (poptiobox, GINVERB, !NewDesk.silentGINRead);
	
	setSelected (poptiobox, OPTCPKOB, NewDesk.useCopyKobold);
	setSelected (poptiobox, OPTDLKOB, NewDesk.useDeleteKobold);
	setSelected (poptiobox, OPTFOKOB, NewDesk.useFormatKobold);
	NewDesk.minCopyKobold %= 100;
	NewDesk.minDeleteKobold %= 100;
	sprintf (poptiobox[OPTCPNR].ob_spec.tedinfo->te_ptext,
		"%d", NewDesk.minCopyKobold);
	sprintf (poptiobox[OPTDELNR].ob_spec.tedinfo->te_ptext,
		"%d", NewDesk.minDeleteKobold);

	DialDraw (&d);

	retcode = DialDo (&d, 0) & 0x7FFF;
	setSelected (poptiobox, retcode, FALSE);

	DialEnd (&d);

	if (retcode == OPTIOK)
	{
		NewDesk.replaceExisting = !isSelected (poptiobox, OPTEXIST);
		NewDesk.silentCopy = !isSelected (poptiobox, CPVERB);
		NewDesk.silentRemove = !isSelected (poptiobox, RMVERB);
		NewDesk.silentGINRead = !isSelected (poptiobox, GINVERB);

		NewDesk.useCopyKobold = isSelected (poptiobox, OPTCPKOB);
		NewDesk.useDeleteKobold = isSelected (poptiobox, OPTDLKOB);
		NewDesk.useFormatKobold = isSelected (poptiobox, OPTFOKOB);
		NewDesk.minCopyKobold = 
			atoi (poptiobox[OPTCPNR].ob_spec.tedinfo->te_ptext);
		NewDesk.minDeleteKobold = 
			atoi (poptiobox[OPTDELNR].ob_spec.tedinfo->te_ptext);
	}
}

void EditFinishOptions(void)
{
	word retcode;
	DIALINFO d;

	DialCenter(popendbox);
	DialStart(popendbox,&d);
	
	setSelected(popendbox,OPTEMPTY,NewDesk.emptyPaper);
	setSelected(popendbox,OPTWAIT,NewDesk.waitKey);
	setSelected(popendbox,OPTQUEST,NewDesk.askQuit);
	setSelected(popendbox,OPTSAVE,NewDesk.saveState);
	setSelected(popendbox,OPTOVL,NewDesk.ovlStart);
	
	DialDraw(&d);

	retcode = DialDo(&d,0) & 0x7FFF;
	setSelected(popendbox,retcode,FALSE);

	DialEnd(&d);

	if(retcode == OPENDOK)
	{
		NewDesk.emptyPaper = isSelected(popendbox,OPTEMPTY);
		NewDesk.waitKey = isSelected(popendbox,OPTWAIT);
		NewDesk.askQuit = isSelected(popendbox,OPTQUEST);
		NewDesk.saveState = isSelected(popendbox,OPTSAVE);
		NewDesk.ovlStart = isSelected(popendbox,OPTOVL);
	}
}


void SaveStatus (void)
{
	word retcode, edit, done;
	DIALINFO d;
	char *name, *env_name;

	env_name = GetEnv (MupfelInfo, "GEMINI_GIN");
	if (env_name)
	{
		char tmp[MAX_FILENAME_LEN];
		
		GetBaseName (env_name, tmp, sizeof (tmp));
		strncpy (ShowInfo.gin_name, tmp, 8);
	}

	name = pginsave[GINNAME].ob_spec.tedinfo->te_ptext;
	strncpy (name, ShowInfo.gin_name, 8);
	name[8] = '\0';
	setSelected (pginsave, GSGIN, !ShowInfo.dont_save_gin_file);

	DialCenter (pginsave);
	DialStart (pginsave, &d);
	DialDraw (&d);

	done = FALSE;
	edit = GINNAME;

	while (!done)
	{
		retcode = DialDo (&d, &edit) & 0x7FFF;
		setSelected (pginsave, retcode, FALSE);
		
		switch (retcode)
		{
			case GSOK:
			{
				char *gin_name = name;
				
				ShowInfo.dont_save_gin_file = 
					!isSelected (pginsave, GSGIN);
					
				if (ShowInfo.dont_save_gin_file)
					gin_name = NULL;
				else if (*name == '\0')
				{
					venusInfo (NlsStr (T_NONAME));
					break;
				}
				else
					strcpy (ShowInfo.gin_name, strlwr(name));
					
				WriteInfoFile (pgmname".inf", gin_name, TRUE);
				done = TRUE;
				break;
			}
			
			case GINHELP:
				if (venusChoice (NlsStr (T_MAKE_NAME)))
				{
					sprintf (name, "%04d%04d", 
						Desk.g_x + Desk.g_w,
						Desk.g_y + Desk.g_h);
					fulldraw (pginsave, GINNAME);
				}
				break;
				
			default:
				done = TRUE;
				break;
		}

		fulldraw (pginsave, retcode);
	}

	DialEnd (&d);

	FlushPathUpdate ();
}

int InvokeHyperTextHelp (void)
{
	return 0;
}
