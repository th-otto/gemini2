/*
 * Header-File fÅr MiNT-Bindings					Stephan Baucke, 2/92
 */

#ifndef _MINTBIND_H
#define _MINTBIND_H

/*
 * defines for fcntl()
 */
#define	F_DUPFD		0	/* Duplicate fildes */
#define	F_GETFD		1	/* Get fildes flags */
#define	F_SETFD		2	/* Set fildes flags */
#define	F_GETFL		3	/* Get file flags */
#define	F_SETFL		4	/* Set file flags */
#define F_GETLK		5	/* Get file lock */
#define F_SETLK		6	/* Set file lock */

#define	O_NDELAY	0x100		/* Non-blocking I/O */
#define O_SYNC		0x00		/* sync after writes (not implemented) */

#if NEVER
typedef struct xattr {
    unsigned short  mode;
    long    index;
    unsigned short  dev;
    unsigned short  reserved1;
    unsigned short  nlink;
    unsigned short  uid;
    unsigned short  gid;
    long    size;
    long    blksize;
    long    nblocks;
    short   mtime, mdate;
    short   atime, adate;
    short   ctime, cdate;
    short   attr;
    short   reserved2;
    long    reserved3[2];
} XATTR;
#endif

/* The requests for Dpathconf() */
#define DP_IOPEN	0	/* internal limit on # of open files */
#define DP_MAXLINKS	1	/* max number of hard links to a file */
#define DP_PATHMAX	2	/* max path name length */
#define DP_NAMEMAX	3	/* max length of an individual file name */
#define DP_ATOMIC	4	/* # of bytes that can be written atomically */
#define DP_TRUNC	5	/* file name truncation behavior */
#	define	DP_NOTRUNC	0	/* long filenames give an error */
#	define	DP_AUTOTRUNC	1	/* long filenames truncated */
#	define	DP_DOSTRUNC	2	/* DOS truncation rules in effect */
#define DP_CASE		6
#	define	DP_CASESENS	0	/* case sensitive */
#	define	DP_CASECONV	1	/* case always converted */
#	define	DP_CASEINSENS	2	/* case insensitive, preserved */
#define DP_MODEATTR		7
#	define	DP_ATTRBITS	0x000000ffL	/* mask for valid TOS attribs */
#	define	DP_MODEBITS	0x000fff00L	/* mask for valid Unix file modes */
#	define	DP_FILETYPS	0xfff00000L	/* mask for valid file types */
#	define	DP_FT_DIR	0x00100000L	/* directories (always if . is there) */
#	define	DP_FT_CHR	0x00200000L	/* character special files */
#	define	DP_FT_BLK	0x00400000L	/* block special files, currently unused */
#	define	DP_FT_REG	0x00800000L	/* regular files */
#	define	DP_FT_LNK	0x01000000L	/* symbolic links */
#	define	DP_FT_SOCK	0x02000000L	/* sockets, currently unused */
#	define	DP_FT_FIFO	0x04000000L	/* pipes */
#	define	DP_FT_MEM	0x08000000L	/* shared memory or proc files */
#define DP_XATTRFIELDS	8
#	define	DP_INDEX	0x0001
#	define	DP_DEV		0x0002
#	define	DP_RDEV		0x0004
#	define	DP_NLINK	0x0008
#	define	DP_UID		0x0010
#	define	DP_GID		0x0020
#	define	DP_BLKSIZE	0x0040
#	define	DP_SIZE		0x0080
#	define	DP_NBLOCKS	0x0100
#	define	DP_ATIME	0x0200
#	define	DP_CTIME	0x0400
#	define	DP_MTIME	0x0800
#define DP_MAXREQ	8	/* highest legal request */


int Syield(void);
int Fpipe(int *ptr);
int Fcntl(int f, long arg, int cmd);
long Finstat(int f);
long Foutstat(int f);
long Fgetchar(int f, int mode);
long Fputchar(int f, long c, int mode);
long Pwait(void);
int Pnice(int delta);
int Pgetpid(void);
int Pgetppid(void);
int Pgetpgrp(void);
int Psetpgrp(int pid, int newgrp);
int Pgetuid(void);
int Psetuid(int id);
int Pkill(int pid, int sig);
int Pvfork(void);
int Pgetgid(void);
int Psetgid(int id);
long Psigblock(long mask);
long Psigsetmask(long mask);
long Pusrval(long arg);
int Pdomain(int newdom);
void Psigreturn(void);
void Prusage(long r[8]);
long Talarm(long secs);
void Pause(void);
long Sysconf(int n);
long Psigpending(void);
long Dpathconf(const char *name, int n);
long Fmidipipe(int pid, int in, int out);
int Prenice(int pid, int delta);
long Dreaddir(int buflen, long dir, char *buf);
long Drewinddir(long dir);
long Dclosedir(long dir);
long Flink(char *oldname, char *newname);
long Fsymlink(char *oldname, char *newname);
long Freadlink(int siz, char *buf, char *name);
long Fchown(char *name, int uid, int gid);
long Fchmod(char *name, int mode);
long Psemaphore(int mode, long id, long timeout);
long Dlock (int mode, int drive);

#endif /* _MINTBIND_H */
