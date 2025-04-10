/*
 * @(#) Common\fileutil.h
 * @(#) Stefan Eissing, 30. Juli 1994
 *
 * jr 16.4.95
 */

#ifndef __fileutil__
#define __fileutil__

#include <tos.h>


#ifndef FA_ALL
/* Zum Suchen nach allen mîglichen Dateien
 */
#define FA_ALL		(FA_HIDDEN|FA_SYSTEM|FA_LABEL|FA_SUBDIR)

/* Zum Suchen nach allen auûer Labels
 */
#define FA_NOLABEL	(FA_HIDDEN|FA_SYSTEM|FA_SUBDIR)
#endif

/* Modes fÅr access
 */
#define A_READ		1
#define A_WRITE		2
#define A_RDWR		3
#define A_EXEC		4
#define A_EXIST		5
#define A_RDONLY	6
#define A_ISDIR		7
/*
 * determine accessibility of file according to mode
 * mode values:	check performed:
 *	A_EXITS		could be opened for reading
 *	A_READ		dto
 *	A_RDONLY	exists, is read-only
 *	A_WRITE		could be opened for writing
 *	A_RDWR		dto
 *	A_EXEC		exists, and is a TOS binary
 *				(magic number X_MAGIC in 1st 2 bytes == X_MAGIC)
 *  A_ISDIR		Ist ein Ordner
 */
int access (const char *file, int mode, int *broken);

void FillXattrFromDTA (const char *path, XATTR *X, DTA *D);
long GetXattr (const char *filename, int with_label, int follow_links,
	XATTR *X);
long GetFileInformation (const char *file, long *size, short *attrib,
	unsigned short *date, unsigned short *time, int *broken);

long ReadLabel (char drv, char *name_buffer, size_t maxlen);
long DriveAccessible (char drv, char *name_buffer, size_t maxlen);
int GetBaseName (const char *path, char *target, size_t maxlen);

int ValidFileName (const char *name);
const char *Extension (const char *name);
char *NewExtension (ALLOCINFO *A, char *filename, const char *ext);

int StripFileName (char *path);
int StripFolderName (char *path);
void AddFileName (char *path, const char *name);
void AddFolderName (char *path, const char *name);

int LegalDrive (char drv);
unsigned long DriveBit (const char drive_name);

int IsRealFile (const char *full_name, int *broken);
int IsRootDirectory (const char *dir);
int IsDirectory (const char *name, int *broken);
int IsAbsoluteName (const char *name);

long GetPathOfDrive (char *path, size_t maxlen, int drive_nr,
	int prefer_uppercase);

char *NormalizePath (ALLOCINFO *A, const char *path, 
	const char cur_drive, const char *cur_dir);

char *GetExecuterFromFile (ALLOCINFO *A, const char *name, int *broken);


long BiosDeviceOfPath (char *path, short *device);
int ForceMediaChange (char *path);
int PADriveAccessible (unsigned int gemdos_drive,
						unsigned long *pDriveMap);

int IsPathPrefix (const char *path, const char *prefix);

unsigned long AccessibleDrives (void);
char *ProcessName (int pid, char *name);
int IsDirEmpty (const char *dir);

long GetMacFileinfo (char *filename, char *type, char *creator);


#endif /* __fileutil__ */
