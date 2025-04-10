#ifndef	_IOCTL_H
#define _IOCTL_H

#define FSTAT		(('F'<< 8) | 0)
#define FIONREAD	(('F'<< 8) | 1)
#define FIONWRITE	(('F'<< 8) | 2)

#define FUTIME      (('F'<< 8) | 3)

struct mutimbuf {
	unsigned int actime, acdate;	/* GEMDOS format */
	unsigned int modtime, moddate;
};

#define TIOCGETP	(('T'<< 8) | 0)
#define TIOCSETP	(('T'<< 8) | 1)
#define TIOCSETN	TIOCSETP
#define TIOCGETC	(('T'<< 8) | 2)
#define TIOCSETC	(('T'<< 8) | 3)
#define TIOCGLTC	(('T'<< 8) | 4)
#define TIOCSLTC	(('T'<< 8) | 5)
#define TIOCGPGRP	(('T'<< 8) | 6)
#define TIOCSPGRP	(('T'<< 8) | 7)
#define TIOCFLUSH	(('T'<< 8) | 8)
#define TIOCSTOP	(('T'<< 8) | 9)
#define TIOCSTART	(('T'<< 8) | 10)
#define TIOCGWINSZ	(('T'<< 8) | 11)
#define TIOCSWINSZ	(('T'<< 8) | 12)
#define TIOCGXKEY	(('T'<< 8) | 13)
#define TIOCSXKEY	(('T'<< 8) | 14)

/* not yet implemented in MiNT */
#define TIOCGETD	(('T'<<8) | 16)
#define TIOCSETD	(('T'<<8) | 17)
#define TIOCLGET	(('T'<<8) | 18)
#define TIOCLSET	(('T'<<8) | 19)

#define NTTYDISC	1

/* ioctl's to act on processes */
#define PPROCADDR	(('P'<<8) | 1)
#define PBASEADDR	(('P'<<8) | 2)

struct tchars {
	char	t_intrc;
	char	t_quitc;
	char	t_startc;
	char	t_stopc;
	char	t_eofc;
	char	t_brkc;
};

struct ltchars {
	char	t_suspc;
	char	t_dsuspc;
	char	t_rprntc;
	char	t_flushc;
	char	t_werasc;
	char	t_lnextc;
};

#define	CRMOD		0x0001
#define	CBREAK		0x0002
#define	ECHO		0x0004
#define	XTABS		0x0008
#define	RAW		0x0010
#define LCASE		0x0020
#if 0
#define TOS		0x0080
#endif

#define TOSTOP		0x0100
#define XKEY		0x0200

/* The ones below aren't supported by the kernel, at least not yet */
#define TANDEM		0x1000
#define ANYP		0
#define EVENP		0x4000
#define ODDP		0x8000
#define VTDELAY		0
#define ALLDELAY	0

/* some fake defines for the line discipline stuff */

#define LCRTBS		0x01
#define LCRTERA		0x02
#define LCRTKIL		0x04
#define LPRTERA		0x10
#define LFLUSHO		0x20
#define LLITOUT		0x100

struct sgttyb {
	char	sg_ispeed;
	char	sg_ospeed;
	char	sg_erase;
	char	sg_kill;
	short	sg_flags;
};

struct winsize {
	short	ws_row;
	short	ws_col;
	short	ws_xpixel;
	short	ws_ypixel;
};

struct xkey {
	short	xk_num;
	char	xk_def[8];
};


#endif	/* _IOCTL_H */
