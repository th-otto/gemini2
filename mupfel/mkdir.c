/*
 * @(#) Mupfel\mkdir.c
 * @(#) Stefan Eissing & Gereon Steffens, 08. Januar 1994
 *
 *  -  internal "mkdir" command
 *
 * jr 22.4.95
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tos.h>
#include <nls\nls.h>

#include "..\common\alloc.h"
#include "..\common\charutil.h"
#include "..\common\fileutil.h"
#include "..\common\strerror.h"

#include "mglob.h"

#include "chario.h"
#include "getopt.h"
#include "mkdir.h"

/* internal texts
 */
#define NlsLocalSection "M.mkdir"
enum NlsLocalText{
MK_EXISTS,	/*mkdir: %s existiert bereits\n*/
MK_ILLNAME,	/*mkdir: %s ist kein gltiger Name\n*/
OPT_mkdir,	/*[-p] dirs*/
HLP_mkdir,	/*Directories anlegen*/
};



static int domkdir (MGLOBAL *M, const char *dir, 
	const char *orig_name, int *broken)
{
	long ret;

	if (!ValidFileName (dir))
	{
		*broken = EACCDN;
		if (!LegalDrive (*dir))
			eprintf (M, "mkdir: `%c:' -- %s\n", *dir, StrError (EDRIVE));
		else
			eprintf (M, NlsStr(MK_ILLNAME), orig_name);
		return FALSE;
	}
	
	if (access (dir, A_READ, broken)
		|| (!broken && IsDirectory (dir, broken)))
	{
		*broken = EACCDN;
		eprintf (M, NlsStr(MK_EXISTS), orig_name);
		return FALSE;
	}

	ret = Dcreate (dir);
	
	if (IsBiosError (ret)) M->oserr = (int) ret;
	
	if (ret != E_OK)
		eprintf (M, "mkdir: `%s' -- %s\n", orig_name, StrError (ret));

	return ret == E_OK;
}

int mkdir (MGLOBAL *M, char *dir, const char *orig_name, int with_path)
{
	int dir_exist, retcode = TRUE;
	char *dir_dup, *dir_part, *d;
	int broken = FALSE;
	
	if (lastchr (dir) == '\\' && strlen (dir) > 1)
		dir[strlen (dir) - 1] = '\0';
	
	if (with_path)
	{
		char *save_token;
		
		dir_dup = StrDup (&M->alloc, dir);
		if (!dir_dup)
			return FALSE;
			
		dir_part = StrDup (&M->alloc, dir);
		if (!dir_part)
		{
			mfree (&M->alloc, dir_dup);
			return FALSE;
		}

		*dir_part = '\0';
		d = StrToken (dir_dup, "\\", &save_token);
		
		if (strlen (d) == 2 && d[1] == ':')
		{
			if (!LegalDrive (*d))
			{
				retcode = FALSE;
				d = NULL;
			}
			else
			{
				strcpy (dir_part, d);
				strcat (dir_part, "\\");
				d = StrToken (NULL, "\\", &save_token);
			}
		}
		
		while (d != NULL)
		{
			AddFileName (dir_part, d);
			dir_exist = IsDirectory (dir_part, &broken);
			
			if (broken)
			{
				ioerror (M, "mkdir", dir_part, broken);
				retcode = FALSE;
				break;
			}
			
			if (!dir_exist)
			{
				if (!domkdir (M, dir_part, orig_name, &broken))
				{
					if (broken)
						ioerror (M, "mkdir", dir_part, broken);
					retcode = FALSE;
					break;
				}
			}

			d = StrToken (NULL, "\\", &save_token);
		}
		
		mfree(&M->alloc, dir_part);
		mfree(&M->alloc, dir_dup);
		return retcode;
	}
	else
	{
		if (!domkdir (M, dir, orig_name, &broken))
			return FALSE;
	}

	if (!M->shell_flags.system_call)
		DirtyDrives |= DriveBit (dir[0]);

	return retcode;
}

int m_mkdir (MGLOBAL *M, int argc, char **argv)
{
	GETOPTINFO G;
	int i, c, retcode = 0;
	int with_path = FALSE;
	
	static struct option long_option[] =
	{
		{ "path", FALSE, NULL, 'p' },
		{ NULL,0,0,0 },
	};
	int opt_index;

	if (!argc)
		return PrintHelp (M, argv[0], NlsStr(OPT_mkdir), 
			NlsStr(HLP_mkdir));
	
	optinit (&G);
	
	while ((c = getopt_long (M, &G, argc, argv, "p", 
		long_option, &opt_index)) != EOF)
	{
		if (!c)
			c = long_option[opt_index].val;
			
		switch(c)
		{
			case 0:
				break;

			case 'p':
				with_path = TRUE;
				break;

			default:
				return PrintUsage (M, argv[0], NlsStr(OPT_mkdir));
		}
	}

	if (G.optind == argc)
	{
		return PrintUsage (M, argv[0], NlsStr(OPT_mkdir));
	}
		
	for (i = G.optind; i < argc && retcode == 0; ++i)
	{
		char *realname = NULL;
		
		if (!ContainsGEMDOSWildcard (argv[i]))
		{		
			realname = NormalizePath (&M->alloc, argv[i], 
				M->current_drive, M->current_dir);
		}

		if (realname == NULL)
		{
			eprintf (M, NlsStr(MK_ILLNAME), argv[i]);
			retcode = 1;
		}
		else
		{
			if (!mkdir (M, realname, argv[i], with_path))
			{
				retcode = 1;
			}
			mfree (&M->alloc, realname);
		}
	}

	return retcode;
}
