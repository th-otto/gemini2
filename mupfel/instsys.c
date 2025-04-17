/*
 * @(#) Mupfel\instsys.c
 * @(#) Stefan Eissing, 17. September 1991
 *
 * installiere OS-Vektoren, etc.
 */

#include <stddef.h> 
#include <string.h>
#include <tos.h>

#include "instsys.h"
#include "mdef.h"
#include "shell.h"
#include "stand.h"
#include "sysvars.h"
#include "..\common\xbra.h"


#define STANDALONE_XBRA	'MUPF'
#define MERGED_XBRA		'GMNI'

#if STANDALONE
#define XBRA_ID	STANDALONE_XBRA
#else
#define XBRA_ID	MERGED_XBRA
#endif


void InitSystemCall (void)
{
	SystemXBRA.id = XBRA_ID;
	
	InstallXBRA (&SystemXBRA, _SHELL_P);
	Supexec ((long (*)())InitReset);
}

void ExitSystemCall (void)
{
	RemoveXBRA (&SystemXBRA, _SHELL_P);
	Supexec ((long (*)())ExitReset);
}
