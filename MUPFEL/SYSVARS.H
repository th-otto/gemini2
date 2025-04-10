/*
 * @(#) Mupfel\sysvars.h 
 * @(#) Stefan Eissing, 03. Juli 1992
 *
 */

#ifndef __M_SYSVARS__
#define __M_SYSVARS__

#define SPA		4		/* Spanish TOS Country code */

/* TOS system variables */
#define XBIOS			((long *)0xb8L)
#define PROC_LIVES		((ulong *)0x380L)
#define ETV_CRITIC		((long *)0x404L)
#define ETV_TERM		((long *)0x408L)
#define CONTERM		((char *)0x484L)
#define _NFLOPS		((int *)0x4a6L)
#define _HZ_200		((ulong *)0x4baL)
#define _SYSBASE		((SYSHDR **)0x4f2L)
#define _SHELL_P		((long *)0x4f6L)
#define PUN_PTR		((PUN_INFO **)0x516L)
#define _O_CON			((long *)0x586L)
#define _O_RAWCON		((long *)0x592L)
#define _OS_CON			((long *)0x566L)
#define _OS_RAWCON		((long *)0x572L)
#define _P_COOKIES		((long **)0x5a0L)
#define _CPUTYPE		((int *)0x59eL)
#define OS_ACT_PD		((BASPAG **)0x602cL)
#define OS_ACT_PD_SPA	((BASPAG **)0x873cL)


#endif /* __M_SYSVARS__ */