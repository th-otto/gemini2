/*
 * @(#) Common\WalkDir.h
 * @(#) Stefan Eissing, 20. Dezember 1993
 *
 * jr 29.12.96
 */

#ifndef C_WALKDIR__
#define C_WALKDIR__

#include <stddef.h>
#include <tos.h>

#define MAX_DIRWALK_LEN		256

typedef struct
{
	char type[4], creator[4];
} MACINFO;

typedef struct
{
	long last_status;

	struct
	{
		unsigned char mint;
		unsigned char with_attributes;
		unsigned char use_dxreaddir;
		unsigned char ignore_Fxattr_errors;
		unsigned char compatible_mode;
		unsigned char case_sensitiv;
		unsigned char prefer_lower;
		unsigned char first_read;
		unsigned char with_label;
		unsigned char follow_links;
		unsigned char is_8and3;
		unsigned char hfs_info;
	} flags;
	
	long mint_dir_handle;
	DTA *old_dta;
	DTA dta;
	
	XATTR xattr;
	
	MACINFO macinfo;
	
	size_t name_buffer_len;
	char *name_buffer;

	char dir_to_walk[MAX_DIRWALK_LEN];
} WalkInfo;

#define WDF_ATTRIBUTES			0x01
#define WDF_COMPATIBLE_MODE		0x02
#define WDF_LOWER				0x04
#define WDF_LABEL				0x08
#define WDF_FOLLOW				0x10
#define WDF_IGN_XATTR_ERRORS	0x20
#define WDF_GET_MAC_ATTRIBS		0x40

/* jr: hier wird's tricky :-) */
#define WDF_GEMINI_LOWER	(NewDesk.preferLowerCase ? WDF_LOWER : 0)

long InitDirWalk (WalkInfo *wi, int flags, const char *dir_to_walk,
	size_t max_name_len, char *name_buffer);

void ExitDirWalk (WalkInfo *wi);

long DoDirWalk (WalkInfo *wi);
long DirReadSymlink (WalkInfo *wi, int slen, char *slink);


#endif