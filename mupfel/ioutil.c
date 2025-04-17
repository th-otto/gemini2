/*
 * @(#) MParse\IOUtil.c
 * @(#) Stefan Eissing, 06. Februar 1993
 *
 * jr 21.10.95
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>

#include "..\common\alloc.h"
#include "..\common\cookie.h"	/* xxx */
#include "..\common\fileutil.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "ioutil.h"
#include "mdef.h"
#include "mupfel.h"
#include "names.h"


#define OS_ACT_PD		((BASPAG **)0x602cL)
#define OS_ACT_PD_SPA	((BASPAG **)0x873cL)
#define SPA		4		/* Spanish TOS Country code */

SYSHDR *
GetSysHdr (void)
{
	extern SYSHDR *SysBase;

	if (CookiePresent ('SVAR', 0)) return 0L;

	return SysBase->os_base;
}

/*
 * getactpd(void)
 * return a pointer to the basepage of the currently running
 * process.
 * For TOS 1.2 and higher, use the SYSHDR structure, for TOS 1.0
 * use the value at 0x602c (Worldwide) or 0x873c (Spain).
 * This is used to give child processes spawned via system() the
 * correct (i.e. not our own) parent process pointer in their xArg
 * structure (see setupxarg() in exec.c).
 */
BASPAG *getactpd (void)
{
	SYSHDR *sys;
	BASPAG *act;

	if (CookiePresent ('SVAR', 0L)) return 0;	/* xxx */
	
	sys = GetSysHdr ();
	sys = sys->os_base;
	
	if (sys->os_version >= 0x102)
		act = *sys->_run;
	else
	{
		if ((sys->os_palmode>>1) == SPA)
			act = *OS_ACT_PD_SPA;
		else
			act = *OS_ACT_PD;
	}

	return act;
}


/* Ermittle das (ein) Handle, das hinter dem Standard-Kanal
 * steckt.
 */
int GetStdHandle (int std_stream, int dupHandles, int *opened)
{
	*opened = FALSE;
	
	if (dupHandles)
	{
		*opened = TRUE;
		return (int)Fdup (std_stream);
	}
	else
	{
		BASPAG *B = getactpd ();
		
		if (B->p_stdfh[std_stream])
			return (int)((signed char)B->p_stdfh[std_stream]);
	}
	
	*opened = TRUE;
	switch (std_stream)
	{
		case 0:
		case 1:
			return (int)Fopen ("CON:", 2);
		case 2:
			return (int)Fopen ("AUX:", 2);
		case 3:
			return (int)Fopen ("PRN:", 2);
	}

	return 0;
}

#define Ssystem(a,b,c)	gemdos(0x154,(int)a,(long)b,(long)c)	


static long getms (void)
{
	return *((unsigned long *)(0x4baL));
}

long GetHz200Timer (void)
{
	if (E_OK == Ssystem (-1, 0L, 0L))
		return Ssystem (10, 0x4ba, NULL);
	else
		return Supexec (getms);
}

/* Gibt (allozierten) String mit Namen einer tempor„ren
 * Datei zurck.
 */
char *TmpFileName (MGLOBAL *M)
{
	NAMEINFO *n;
	char tf[MAX_PATH_LEN];
	char name[MAX_FILENAME_LEN];
	int handle;
	long hz;
	
	*tf = 0;
	
	n = GetNameInfo (M, "TMPDIR");
	if (n)
		strcpy (tf, NameInfoValue (n));
		
	hz = GetHz200Timer ();
	
	handle = 0;
	
	while (handle >= 0)
	{
		sprintf (name, "%08lx.$$$", hz);
		AddFileName (tf, name);

		handle = (int)Fopen (tf, 0);
		if (handle >= 0)
		{
			Fclose (handle);
			hz++;
			StripFileName (tf);
		}
	}

	return NormalizePath (&M->alloc, tf, M->current_drive, 
		M->current_dir);
}

/*
 * is_a_tty (int handle)
 * return status of the device/file associated with handle
 * TRUE	for devices (CON: PRN: AUX:)
 * FALSE	for file handles
 */
int is_a_tty (int handle)
{
	long oldoffset, rc;
	
	if (handle < 0)
		return TRUE;
		
	oldoffset = Fseek (0L, handle, 1);
	rc = Fseek (1L, handle, 0);
	Fseek (oldoffset, handle, 0);
	
	return !rc;
}

