/*
 * @(#) Venus\fileutil.h
 * @(#) Stefan Eissing, 18. Dezember 1994
 *
 * description: Header File for fileutil.c
 *
 * jr 14.4.95
 */


#ifndef G_fileutil__
#define G_fileutil__

word AddFileAnz (char *path, int follow_links, long max_objects,
	uword *foldnr, uword *filenr,long *size);
	
void timeString (char *cp, word time);
void dateString (char *cp, word date);


char GetDrive (void);
word SetDrive (const char drive);
long GetFullPath (char *path, size_t maxlen);
long SetFullPath (const char *path);

int IsInExtensionList (const char *ext, const char *list,
	const char *defList);
int IsExecutable (const char *name);
int IsGemExecutable (const char *name);
int IsAccessory (const char *filename);
int TakesParameter (const char *filename);

word sameLabel (const char *label1,const char *label2);
word isEmptyDir (const char *path);

char *getSPath (char *oldpath);
char *getTPath (char *oldpath);

word getDiskSpace (word drive, long *bytesused, long *bytesfree);
long fileRename (char *oldname,char *newname);
long setFileAttribut (char *name,word attrib);
word CreateFolder (char *path);
word CreateFile (char *path);
word delFile (const char *fname);
void char2LongKey (char c, long *key);

int GetFileName (char fname[MAX_PATH_LEN], const char *pattern,
	const char *message);


#endif	/* !G_fileutil__ */