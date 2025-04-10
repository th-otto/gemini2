/*
 * @(#) Common\strerror.c
 * @(#) Julian Reschke, 14. April 1995
 *
 */

#ifndef __toserr__
#define __toserr__

/* BIOS/XBIOS error codes */ 

#define E_OK				0
#define	ERROR				-1
#define	DRIVE_NOT_READY		-2
#define	UNKNOWN_CMD			-3
#define	CRC_ERROR			-4
#define	BAD_REQUEST			-5
#define	SEEK_ERROR			-6
#define	UNKNOWN_MEDIA		-7
#define	SECTOR_NOT_FOUND	-8
#define	NO_PAPER			-9
#define	WRITE_FAULT			-10
#define	EWRITF				-10
#define	READ_FAULT			-11
#define	EREADF				-11
#define	GENERAL_HISHAP		-12
#define	WRITE_PROTECT		-13
#define	EWRPRO				-13
#define	MEDIA_CHANGE		-14
#define	UNKNOWN_DEVICE		-15
#define	BAD_SECTORS			-16
#define	INSERT_DISK			-17

/* GEMDOS error codes */
#define	EINVFN				-32	/* invalid function number */
#define	EFILNF				-33	/* file not found */
#define	EPTHNF				-34	/* path not found */
#define	ENHNDL				-35	/* no more handles */
#define	EACCDN				-36	/* access denied */
#define	EIHNDL				-37	/* invalid handle */
#define	ENSMEM				-39	/* no more memory */
#define	EIMBA				-40	/* invalid memory block address */
#define	EDRIVE				-46	/* invalid drive */
#define	ENSAME				-48	/* not same drive */
#define	ENMFIL				-49	/* no more files */
#define	ERANGE				-64	/* invalid file offset */
#define	EINTRN				-65	/* internal error */
#define	EPLFMT				-66	/* invalid program load format */
#define	EGSBF				-67	/* Mshrink()/Mfree() error */
#define EBREAK				-68
#define EXCPT 				-69	/* Mag!x */
#define EPTHOV				-70

#define EMLINK				-80	/* too many symbolic links */
#define EEXIST				-85 /* file exists, try again later */
#define ENAMETOOLONG 		ERANGE	/* name too long */
#define ENOTTY				-87
#define EDOM				-89
#define EINTR				-128 /* this *should* be fake */

#define	IsBiosError(x)		(x<=ERROR && x>=INSERT_DISK)
#define	IsGemdosError(x)	(x<=EINVFN)

#define X_MAGIC	((int)0x601A)

#endif /* __toserr__ */

extern const char * StrError (long code);