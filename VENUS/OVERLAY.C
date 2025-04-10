/*
 * @(#) Gemini\overlay.c
 * @(#) Stefan Eissing, 27. Februar 1993
 *
 * description: functions to start overlays
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <aes.h>
#include <tos.h>
#include <nls\nls.h>

#include "vs.h"

#include "fileutil.h"
#include "infofile.h"
#include "keylogi.h"
#include "overlay.h"
#include "stand.h"
#include "util.h"
#include "venusErr.h"
#include "..\mupfel\mupfel.h"


/* internal texts
 */
#define NlsLocalSection		"Gmni.overlay"
enum NlsLocalText{
T_RUNNER,	/*Ein Start von %s ist nicht m”glich, da %s
 nicht gefunden werden konnte!*/
T_SHWRITE,	/*Das AES weigert sich, %s zu starten!*/
};


word doOverlay (int *its_an_overlay)
{
	int res_switch = ResolutionSwitchRequest ();
	
	if (!DoingOverlay (MupfelInfo) && !res_switch)
	{
		return FALSE;
	}

	*its_an_overlay = !res_switch;
	
	if (!NewDesk.saveState && !res_switch)
		WriteInfoFile (pgmname".tmp", "temp", FALSE);

	return TRUE;
}
