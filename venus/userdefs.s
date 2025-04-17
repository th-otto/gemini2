; @(#) Gemini\userdef.s
; @(#) Stefan Eissing, 17. Mai 1992
;
; -  interface for DrawIconObject
;

		.import	DrawIconObject
		.export AESDrawIcon

		.text

AESDrawIcon:
		movem.l	d1-d7/a0-a6, savereg
		move.l	4(sp), a0
		move.l	sp, oldstack
		move.l	#newstack+16000, sp
		move.l	a0, -(sp)
		jsr		DrawIconObject
		addq.l	#4, sp
		move.l	oldstack, sp
		movem.l	savereg, d1-d7/a0-a6
		rts
		
		.data
savereg:	ds.l	14
oldstack:	ds.l	1
newstack:	ds.l	16010
