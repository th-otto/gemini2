/*
 * @(#) Common\WalkDir.c
 * @(#) Stefan Eissing, 08. Februar 1994
 *
 * jr 29.12.96
 */


#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tos.h>


#include "alloc.h"
#include "cookie.h"
#include "fileutil.h"
#include "fncase.h"
#include "walkdir.h"
#include "strerror.h"


long
InitDirWalk (WalkInfo *wi, int flags, const char *dir_to_walk,
	size_t max_name_len, char *name_buffer)
{
	long stat;
	
	memset (wi, 0, sizeof (WalkInfo));
	wi->flags.with_attributes = (flags & WDF_ATTRIBUTES) != 0;
	wi->flags.compatible_mode = (flags & WDF_COMPATIBLE_MODE) != 0;
	wi->flags.prefer_lower = (flags & WDF_LOWER) != 0;
	wi->flags.with_label = (flags & WDF_LABEL) != 0;
	wi->flags.follow_links = (flags & WDF_FOLLOW) != 0;
	wi->flags.ignore_Fxattr_errors = (flags & WDF_IGN_XATTR_ERRORS) != 0;
	wi->flags.is_8and3 = 1;
	wi->flags.hfs_info = (flags & WDF_GET_MAC_ATTRIBS) != 0;

	if (strlen (dir_to_walk) > (MAX_DIRWALK_LEN - 5)) return ERANGE;

	strcpy (wi->dir_to_walk, dir_to_walk);

	if (wi->dir_to_walk[0] == '\0')
		strcpy (wi->dir_to_walk, ".\\");
	else if (wi->dir_to_walk[1] == ':')
		wi->dir_to_walk[0] = toupper (wi->dir_to_walk[0]);
	
	AddFolderName (wi->dir_to_walk, ".");
	stat = Dpathconf (wi->dir_to_walk, 6);
	if (stat != EINVFN)
	{
		if (stat < 0) return stat;
			
		wi->flags.mint = wi->flags.use_dxreaddir = 1;
		wi->flags.case_sensitiv = (stat != 1);

		stat = Dpathconf (wi->dir_to_walk, 5);
		if (stat < 0) return stat;
		
		wi->flags.is_8and3 = (stat == 2);

#if 0
		if (wi->flags.is_8and3 && !wi->flags.case_sensitiv 
			&& wi->flags.with_attributes)
		{
			/* Das Dateisystem ist 8+3, nicht casesensitiv und wir wollen
			 * die Attribute mitlesen. Daher lesen wir besser ohne
			 * MiNT, denn das geht schneller. (Aber nur, wenn wir 
			 * nicht auf Laufwerk U: sind. Das ist n„mlich nicht
			 * case-sensitiv, kann aber sonst alles.
			 */
			 /* jr: naja, TOS-FS kann unter MagiC Symlinks! Wir lassen
			 das, denn (1) MiNT 1.11 kann Dxreaddir und ist dann
			 genauso schnell und (2) MagiC 3.0 wird auch
			 Dxreaddir haben.
			 */
			wi->flags.mint = 0;
		}
#endif
	}
	else
		wi->flags.case_sensitiv = 0;
	
	StripFolderName (wi->dir_to_walk);

	wi->name_buffer_len = max_name_len;
	wi->name_buffer = name_buffer;

	if (!wi->flags.compatible_mode && wi->flags.mint)
	{
		wi->last_status = Dopendir ((char *)dir_to_walk, 
			wi->flags.compatible_mode);

		if ((wi->last_status & 0xFF000000L) != 0xFF000000L)
		{
			wi->mint_dir_handle = wi->last_status;
			wi->last_status = 0L;
		}
		else if (wi->last_status == EINVFN)
		{
			wi->flags.mint = 0;
		}
	}
	
	if (wi->flags.compatible_mode || !wi->flags.mint)
	{
		wi->old_dta = Fgetdta ();
		Fsetdta (&wi->dta);
		
		AddFileName (wi->dir_to_walk, "*.*");
		if (flags & WDF_ATTRIBUTES)
		{
			/* Wir k”nnen diese Daten unter TOS nicht ermitteln,
			 * also setzen wir einen sinnvollen Default ein.
			 */
			if (dir_to_walk[1] == ':')
				wi->xattr.dev = toupper (dir_to_walk[0]) - 'A';
			wi->xattr.nlink = 1;
			wi->xattr.blksize = 1024L;
		}
		
		wi->last_status = 0L;
	}
	
	return wi->last_status;
}

void ExitDirWalk (WalkInfo *wi)
{
	if (!wi->flags.compatible_mode && wi->flags.mint)
		Dclosedir (wi->mint_dir_handle);
	else
		Fsetdta (wi->old_dta);
}

extern long Dxreaddir (int a, long b, char *c, XATTR *d, long *e);

long DoDirWalk (WalkInfo *wi)
{
	if (wi->last_status != 0L)	return wi->last_status;
	
	if (!wi->flags.compatible_mode && wi->flags.mint)
	{
		int xattr_already_filled = 0;
	
		/* jr: ggfs Dxreaddir benutzen */
	
		if (wi->flags.use_dxreaddir && wi->flags.with_attributes)
		{
			long xattr_ret;
		
			wi->last_status = Dxreaddir ((int)wi->name_buffer_len, 
				wi->mint_dir_handle, wi->name_buffer,
				&wi->xattr, &xattr_ret);
			
			if (wi->last_status == EINVFN)
			{
				wi->flags.use_dxreaddir = 0;
				wi->last_status = Dreaddir ((int)wi->name_buffer_len, 
					wi->mint_dir_handle, wi->name_buffer);
			}
			else if (xattr_ret == 0)
			{
				if (S_IFLNK != (wi->xattr.mode & S_IFMT)
					|| !wi->flags.follow_links)
					xattr_already_filled = 1;
			}
		}
		else
		{
			wi->last_status = Dreaddir ((int)wi->name_buffer_len, 
				wi->mint_dir_handle, wi->name_buffer);
		}
		
		if (wi->last_status == 0L)
		{
			memmove (wi->name_buffer, &wi->name_buffer[4],
				wi->name_buffer_len - 4);
			
			if (!wi->flags.case_sensitiv)
			{
				if (wi->flags.prefer_lower)
					fn_strlwr (wi->name_buffer);
				else
					fn_strupr (wi->name_buffer);
			}

			if (wi->flags.with_attributes)
			{
				if ((strlen (wi->name_buffer) 
					+ strlen (wi->dir_to_walk))	< (MAX_DIRWALK_LEN-1))
				{
					/* jr: ggfs Werte von Dxreaddir nehmen */
					if (xattr_already_filled)
					{
						wi->last_status = 0;
					}
					else
					{
						AddFileName (wi->dir_to_walk, wi->name_buffer);
						wi->last_status = Fxattr (!wi->flags.follow_links, 
							wi->dir_to_walk, &wi->xattr);
						StripFileName (wi->dir_to_walk);
					}
					
					if (wi->last_status 
						&& wi->flags.ignore_Fxattr_errors)
					{
						/* Fxattr failed! But we ignore that...
						 */
						wi->last_status = 0L;
						memset (&wi->xattr, 0, sizeof (XATTR));
					}
				}
				else
					wi->last_status = -1L;
			}
			
			if (wi->flags.hfs_info)
			{
				memset (&wi->macinfo, ' ', sizeof (MACINFO));
				

				if (S_IFDIR != (wi->xattr.mode & S_IFMT) && 
					(strlen (wi->name_buffer) 
					+ strlen (wi->dir_to_walk))	< (MAX_DIRWALK_LEN-1))
				{
					AddFileName (wi->dir_to_walk, wi->name_buffer);
					GetMacFileinfo (wi->dir_to_walk, wi->macinfo.type, wi->macinfo.creator);
					StripFileName (wi->dir_to_walk);
				}
			}
			
		}
	}
	else
	{
		if (wi->flags.first_read)
		{
			wi->last_status = Fsnext ();
		}
		else
		{
			wi->last_status = Fsfirst (wi->dir_to_walk, 
				wi->flags.with_label?
				(FA_HIDDEN|FA_SYSTEM|FA_SUBDIR|FA_VOLUME) :
				(FA_HIDDEN|FA_SYSTEM|FA_SUBDIR));
			wi->flags.first_read = 1;
		}
		
		if (wi->last_status == 0L)
		{
			strncpy (wi->name_buffer, wi->dta.d_fname, 
				wi->name_buffer_len);
			wi->name_buffer[wi->name_buffer_len-1] = '\0';
			
			if (!wi->flags.case_sensitiv)
			{
				if (wi->flags.prefer_lower)
					strlwr (wi->name_buffer);
				else
					strupr (wi->name_buffer);
			}
				
			if (wi->flags.with_attributes)
			{
				/* šbertrage die Dateiattribute
				 */
				FillXattrFromDTA (wi->dir_to_walk, &wi->xattr, 
					&wi->dta);
			}
		}
	}

	if (!wi->last_status && !wi->name_buffer[0] 
		&& wi->xattr.attr & FA_SUBDIR)
		wi->last_status = EPTHNF;
			
	if (!wi->last_status && wi->flags.with_attributes
		&& !wi->flags.with_label && (wi->xattr.attr & FA_VOLUME))
	{
		return DoDirWalk (wi);
	}

	return wi->last_status;
}

/* jr: Symlink-Info rausholen */

long DirReadSymlink (WalkInfo *wi, int slen, char *slink)
{
	long ret;
	
	AddFileName (wi->dir_to_walk, wi->name_buffer);
	ret = Freadlink (slen, slink, wi->dir_to_walk);
	StripFileName (wi->dir_to_walk);
	
	return ret;
}
