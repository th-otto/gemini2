/*
 * @(#) MParse\fileutil.h
 * @(#) Stefan Eissing, 29. Juni 1991
 *
 */

#ifndef __fileutil__
#define __fileutil__


#define FA_ALL	(FA_HIDDEN | FA_SYSTEM | FA_SUBDIR)

int GetAttributes (const char *file, int *attrib, int *broken);
int GetLabel (char drv, char *name_buffer);

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


int ValidFileName (const char *name);
int HasExtension (const char *name);
char *NewExtension (ALLOCINFO *A, char *filename, const char *ext);

int GetBaseName (char *path, char target[]);
int StripFileName (char *path);
int StripFolderName (char *path);
void AddFileName (char *path, const char *name);
void AddFolderName (char *path, const char *name);

int LegalDrive (char drv);
int IsRealFile (const char *full_name, int *broken);
int IsRootDirectory (const char *dir);
int IsDirectory (const char *name, int *broken);
int IsAbsoluteName (const char *name);

unsigned long DriveBit (const char drive_name);


char *NormalizePath (ALLOCINFO *A, const char *path, 
	const char cur_drive, const char *cur_dir);


char *GetExecuterFromFile (ALLOCINFO *A, const char *name, int *broken);

#endif /* __fileutil__ */
