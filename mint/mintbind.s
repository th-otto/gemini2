/*
 * MiNT-Bindings fÅr Pure-C								Stephan Baucke, 2/92
 * Stand: MiNT 0.92
 * Assembler: PASM
 */
/* jr: added binding for Dxreaddir and xPvfork */

	export xPvfork
	module xPvfork
		movem.l	a0-a2,-(sp)
		move.w  #$113,-(a7)
		trap	#1
		addq.l	#2,a7
		movem.l	(sp)+,a0-a2
		tst.l	d0
		beq.s	is_child
		rts
is_child:
		lea		-100(a0),a0
		move.l	36(sp),36(a0)
		move.l	32(sp),32(a0)
		move.l	28(sp),28(a0)
		move.l	24(sp),24(a0)
		move.l	20(sp),20(a0)
		move.l	16(sp),16(a0)
		move.l	12(sp),12(a0)
		move.l	8(sp),8(a0)
		move.l	4(sp),4(a0)
		move.l	0(sp),0(a0)
		move.l	-4(sp),-4(a0)
		move.l	-8(sp),-8(a0)
		move.l	-12(sp),-12(a0)
		move.l	-16(sp),-16(a0)
		move.l	-20(sp),-20(a0)
		move.l	-24(sp),-24(a0)
		move.l	-28(sp),-28(a0)
		move.l	-32(sp),-32(a0)
		move.l	-36(sp),-36(a0)
		move.l	a0,sp
		rts
	endmod
	
	export Syield
	module Syield
		move.l	a2,-(a7)
		move.w  #$ff,-(a7)
		trap	#1
		addq.l	#2,a7
		move.l (a7)+,a2
		rts
	endmod

	export Fpipe
	module Fpipe
		move.l	a2,-(a7)
		move.l	a0,-(a7)
		move.w  #$100,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l (a7)+,a2
		rts
	endmod

	export Fcntl
	module Fcntl
		move.l	a2,-(a7)
		move.w	d2,-(a7)
		move.l	d1,-(a7)
		move.w	d0,-(a7)
		move.w  #$104,-(a7)
		trap	#1
		lea		10(a7),a7
		move.l (a7)+,a2
		rts
	endmod

	export Finstat
	module Finstat
		move.l	a2,-(a7)
		move.w	d0,-(a7)
		move.w  #$105,-(a7)
		trap	#1
		addq.l	#4,a7
		move.l (a7)+,a2
		rts
	endmod

	export Foutstat
	module Foutstat
		move.l	a2,-(a7)
		move.w	d0,-(a7)
		move.w  #$106,-(a7)
		trap	#1
		addq.l	#4,a7
		move.l (a7)+,a2
		rts
	endmod

	export Fgetchar
	module Fgetchar
		move.l	a2,-(a7)
		move.w	d1,-(a7)
		move.w	d0,-(a7)
		move.w  #$107,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l (a7)+,a2
		rts
	endmod

	export Fputchar
	module Fputchar
		move.l	a2,-(a7)
		move.w	d2,-(a7)
		move.l	d1,-(a7)
		move.w	d0,-(a7)
		move.w  #$108,-(a7)
		trap	#1
		lea		10(a7),a7
		move.l (a7)+,a2
		rts
	endmod

	export Pwait
	module Pwait
		move.l	a2,-(a7)
		move.w  #$109,-(a7)
		trap	#1
		addq.l	#2,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pnice
	module Pnice
		move.l	a2,-(a7)
		move.w	d0,-(a7)
		move.w  #$10a,-(a7)
		trap	#1
		addq.l	#4,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pgetpid
	module Pgetpid
		move.l	a2,-(a7)
		move.w  #$10b,-(a7)
		trap	#1
		addq.l	#2,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pgetppid
	module Pgetppid
		move.l	a2,-(a7)
		move.w  #$10c,-(a7)
		trap	#1
		addq.l	#2,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pgetpgrp
	module Pgetpgrp
		move.l	a2,-(a7)
		move.w  #$10d,-(a7)
		trap	#1
		addq.l	#2,a7
		move.l (a7)+,a2
		rts
	endmod

	export Psetpgrp
	module Psetpgrp
		move.l	a2,-(a7)
		move.w	d1,-(a7)
		move.w	d0,-(a7)
		move.w  #$10e,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pgetuid
	module Pgetuid
		move.l	a2,-(a7)
		move.w  #$10f,-(a7)
		trap	#1
		addq.l	#2,a7
		move.l (a7)+,a2
		rts
	endmod

	export Psetuid
	module Psetuid
		move.l	a2,-(a7)
		move.w	d0,-(a7)
		move.w  #$110,-(a7)
		trap	#1
		addq.l	#4,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pkill
	module Pkill
		move.l	a2,-(a7)
		move.w	d1,-(a7)
		move.w	d0,-(a7)
		move.w  #$111,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l (a7)+,a2
		rts
	endmod

	export Psignal
	module Psignal
		move.l	a2,-(a7)
		move.l	a0,-(a7)
		move.w	d0,-(a7)
		move.w  #$112,-(a7)
		trap	#1
		addq.l	#8,a7
		move.l	d0,a0
		move.l (a7)+,a2
		rts
	endmod

	export Pvfork
	module Pvfork
		move.l	a2,-(a7)
		move.w  #$113,-(a7)
		trap	#1
		addq.l	#2,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pgetgid
	module Pgetgid
		move.l	a2,-(a7)
		move.w  #$114,-(a7)
		trap	#1
		addq.l	#2,a7
		move.l (a7)+,a2
		rts
	endmod

	export Psetgid
	module Psetgid
		move.l	a2,-(a7)
		move.w	d0,-(a7)
		move.w  #$115,-(a7)
		trap	#1
		addq.l	#4,a7
		move.l (a7)+,a2
		rts
	endmod

	export Psigblock
	module Psigblock
		move.l	a2,-(a7)
		move.l	d0,-(a7)
		move.w  #$116,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l (a7)+,a2
		rts
	endmod

	export Psigsetmask
	module Psigsetmask
		move.l	a2,-(a7)
		move.l	d0,-(a7)
		move.w  #$117,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pusrval
	module Pusrval
		move.l	a2,-(a7)
		move.l	d0,-(a7)
		move.w  #$118,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pdomain
	module Pdomain
		move.l	a2,-(a7)
		move.w	d0,-(a7)
		move.w  #$119,-(a7)
		trap	#1
		addq.l	#4,a7
		move.l (a7)+,a2
		rts
	endmod

	export Psigreturn
	module Psigreturn
		move.l	a2,-(a7)
		move.w  #$11a,-(a7)
		trap	#1
		addq.l	#2,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pfork
	module Pfork
		move.l	a2,-(a7)
		move.w  #$11b,-(a7)
		trap	#1
		addq.l	#2,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pwait3
	module Pwait3
		move.l	a2,-(a7)
		move.l	a0,-(a7)
		move.w	d0,-(a7)
		move.w  #$11c,-(a7)
		trap	#1
		addq.l	#8,a7
		move.l (a7)+,a2
		rts
	endmod

	export Fselect
	module Fselect
		move.l	a2,-(a7)
		move.l	8(a7),-(a7)
		move.l	a1,-(a7)
		move.l	a0,-(a7)
		move.w	d0,-(a7)
		move.w  #$11d,-(a7)
		trap	#1
		lea		16(a7),a7
		move.l (a7)+,a2
		rts
	endmod

	export Prusage
	module Prusage
		move.l	a2,-(a7)
		move.l	a0,-(a7)
		move.w  #$11e,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l (a7)+,a2
		rts
	endmod

	export Psetlimit
	module Psetlimit
		move.l	a2,-(a7)
		move.l	d1,-(a7)
		move.w	d0,-(a7)
		move.w  #$11f,-(a7)
		trap	#1
		addq.l	#8,a7
		move.l (a7)+,a2
		rts
	endmod

	export Talarm
	module Talarm
		move.l	a2,-(a7)
		move.l	d0,-(a7)
		move.w  #$120,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pause
	module Pause
		move.l	a2,-(a7)
		move.w  #$121,-(a7)
		trap	#1
		addq.l	#2,a7
		move.l (a7)+,a2
		rts
	endmod

	export Sysconf
	module Sysconf
		move.l	a2,-(a7)
		move.w	d0,-(a7)
		move.w  #$122,-(a7)
		trap	#1
		addq.l	#4,a7
		move.l (a7)+,a2
		rts
	endmod

	export Psigpending
	module Psigpending
		move.l	a2,-(a7)
		move.w  #$123,-(a7)
		trap	#1
		addq.l	#2,a7
		move.l (a7)+,a2
		rts
	endmod

	export Dpathconf
	module Dpathconf
		move.l	a2,-(a7)
		move.w	d0,-(a7)
		move.l	a0,-(a7)
		move.w  #$124,-(a7)
		trap	#1
		addq.l	#8,a7
		move.l (a7)+,a2
		rts
	endmod

	export Pmsg
	module Pmsg
		movem.l	d1/a0/a2,-(a7)
		move.w	d0,-(a7)
		move.w	#$125,-(a7)
		trap	#1
		lea		12(a7),a7
		move.l	(a7)+,a2
		rts
	endmod

	export Fmidipipe
	module Fmidipipe
		move.l a2,-(a7)
		move.w	d2,-(a7)
		move.w	d1,-(a7)
		move.w	d0,-(a7)
		move.w	#$126,-(a7)
		trap	#1
		addq.l	#8,a7
		move.l	(a7)+,a2
		rts
	endmod

	export Prenice
	module Prenice
		move.l	a2,-(a7)
		move.w	d1,-(a7)
		move.w	d0,-(a7)
		move.w	#$127,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l	(a7)+,a2
		rts
	endmod

	export Dopendir
	module Dopendir
		move.l	a2,-(a7)
		move.w	d0,-(a7)
		move.l	a0,-(a7)
		move.w	#$128,-(a7)
		trap	#1
		addq.l	#8,a7
		move.l	(a7)+,a2
		rts
	endmod

	export Dreaddir
	module Dreaddir
		movem.l d1/a0/a2,-(a7)
		move.w	d0,-(a7)
		move.w	#$129,-(a7)
		trap	#1
		lea		12(a7),a7
		move.l	(a7)+,a2
		rts
	endmod

	export Dxreaddir
	module Dxreaddir
		move.l	a2,-(a7)
		move.l	8(a7),-(a7)	; xret
		move.l	a1,-(a7)	; xattr
		move.l	a0,-(a7)	; buf
		move.l	d1,-(a7)	; handle
		move.w	d0,-(a7)	; len
		move.w	#$142,-(a7)
		trap	#1
		lea		20(a7),a7
		move.l	(a7)+,a2
		rts
	endmod

	export Drewinddir
	module Drewinddir
		move.l	a2,-(a7)
		move.l	d0,-(a7)
		move.w	#$12a,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l	(a7)+,a2
		rts
	endmod

	export Dclosedir
	module Dclosedir
		move.l a2,-(a7)
		move.l	d0,-(a7)
		move.w	#$12b,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l	(a7)+,a2
		rts
	endmod

	export Fxattr
	module Fxattr
		movem.l	a0/a1/a2,-(a7)
		move.w	d0,-(a7)
		move.w	#$12c,-(a7)
		trap	#1
		lea		12(a7),a7
		move.l	(a7)+,a2
		rts
	endmod

	export Flink
	module Flink
		movem.l a0/a1/a2,-(a7)
		move.w	#$12d,-(a7)
		trap	#1
		lea		10(a7),a7
		move.l	(a7)+,a2
		rts
	endmod

	export Fsymlink
	module Fsymlink
		movem.l	a0/a1/a2,-(a7)
		move.w	#$12e,-(a7)
		trap	#1
		lea		10(a7),a7
		move.l	(a7)+,a2
		rts
	endmod

	export Freadlink
	module Freadlink
		movem.l	a0/a1/a2,-(a7)
		move.w	d0,-(a7)
		move.w	#$12f,-(a7)
		trap	#1
		lea		12(a7),a7
		move.l	(a7)+,a2
		rts
	endmod

	export Dcntl
	module Dcntl
		move.l  a2,-(a7)
		move.l	d1,-(a7)
		move.l	a0,-(a7)
		move.w	d0,-(a7)
		move.w	#$130,-(a7)
		trap	#1
		lea		12(a7),a7
		move.l	(a7)+,a2
		rts
	endmod

	export Fchown
	module Fchown
		move.l	a2,-(a7)
		move.w	d1,-(a7)
		move.w	d0,-(a7)
		move.l	a0,-(a7)
		move.w	#$131,-(a7)
		trap	#1
		lea		10(a7),a7
		move.l	(a7)+,a2
		rts
	endmod

	export Fchmod
	module Fchmod
		move.l a2,-(a7)
		move.w	d0,-(a7)
		move.l	a0,-(a7)
		move.w	#$132,-(a7)
		trap	#1
		addq.l	#8,a7
		move.l	(a7)+,a2
		rts
	endmod

	export Pumask
	module Pumask
		move.l a2,-(a7)
		move.w	d0,-(a7)
		move.w	#$133,-(a7)
		trap	#1
		addq.l	#4,a7
		move.l	(a7)+,a2
		rts
	endmod

	export Psemaphore
	module Psemaphore
		movem.l	d1/d2/a2,-(a7)
		move.w	d0,-(a7)
		move.w	#$134,-(a7)
		trap	#1
		lea		12(a7),a7
		move.l	(a7)+,a2
		rts
	endmod

	export Dlock
	module Dlock
		move.l	a2,-(a7)
		move.w	d1,-(a7)
		move.w	d0,-(a7)
		move.w	#$135,-(a7)
		trap	#1
		addq.l	#6,a7
		move.l	(a7)+,a2
		rts
	endmod

