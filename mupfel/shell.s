;
; @(#) Mupfel\shell.s
; @(#) Stefan Eissing, 25. April 1992
;
; Handling des shell_p Pointers
;
		
		.import	SystemCall
		.export	SystemHandler, SystemXBRA
		.export InitReset, ExitReset
		.export SetStack

		.text

; SetStack: changes the stack pointer; called as
;     void SetStack( void *newsp )
;
; WARNING WARNING WARNING: after you do this, local variables may no longer
; be accessible!
; destroys a0 and a7
SetStack:
;		move.l	(sp)+, (a0)				; save return address
;		move.l	(sp)+, 4(a0)
;		move.l	(sp)+, 8(a0)
;		move.l	(sp)+, 12(a0)
;		move.l	a0, sp					; new stack pointer
;		move.l	(a0), a0
;		subq.l	#4, sp					; fixup for tidy upon return
;		jmp	(a0)						; back to caller

		move.l	0(sp),d0				; save return address
		move.l	a0,sp					; set stack
		move.l	d0,-(sp)
		rts

SystemXBRA:
		dc.b	'XBRA'					; Magic
		dc.b	'MUPF'					; ID
		ds.l	1						; alter Vektor	
SystemHandler:
		move.l	4(a7), d0				; Kommando holen
		movem.l	d1-d7/a0-a6, -(a7)		; Register sichern
		move.l	d0, -(a7)
		
		move.l	#16384,-(a7)			; hole 16 KB Speicher
		move.w	#$48,-(a7)				; Malloc
		trap	#1						; Gemdos
		addq.l	#6,a7					; korrigiere den Stack
		
		move.l	(a7)+, a0				; hole Kommando wieder
		tst.l	d0						; Speicher vorhanden?
		bne		gotMemory
		move.w	#-1, d0					; Miûerfolg
		bra		systemHandlerEnd		; und zurÅck

gotMemory:
		move.l	a7, d1					; Stackpointer sichern
		move.l	d0, d2					; Speicherbereich sichern
		add.l	#16376, d0				; d0 zeigt auf Speicherende
		move.l	d0, a7					; neuer Stack
		move.l	d2, d0					; setze Stack-Limit
		add.l	#1024, d0				; und Åbergebe es
		movem.l	d1-d2, -(a7)			; alte Werte sichern
		
		jsr		SystemCall				; Arbeite Kommando (a0) ab
		
		
correctStack:
		move.l	d0, d7					; Returnwert sichern
		movem.l	(a7)+, d1-d2			; hole die alten Werte
		move.l	d1, a7					; alten Stack setzen

		move.l	d2,-(a7)				; reservierter Speicher
		move.w	#$49,-(a7)				; Mfree
		trap	#1						; Gemdos
		addq.l	#6,a7					; korrigiere Stack
		
		move.l	d7, d0					; Returnwert wiederherstellen
		
systemHandlerEnd:
		movem.l	(a7)+, d1-d7/a0-a6		; Register wiederherstellen
		rts
		

;
; Reset-Handler, der Shell_p wieder lîscht
;
;
_resvalid		equ	$426
_resvector		equ	$42a
_shell_p		equ	$4f6
RESMAGIC		equ	$31415926

InitReset:
		move.l	_resvalid, valsave
		move.l	_resvector, reset_oldvec
		move.l	#clearShell, _resvector
		move.l	#RESMAGIC, _resvalid
		rts
	
ExitReset:
		move.l	valsave, _resvalid
		move.l	reset_oldvec, _resvector
		rts

valsave:
		dc.l	0
ResetXBRA:
		dc.b	'XBRA'
		dc.b	'MUPF'
reset_oldvec:
		dc.l	0
clearShell:
		clr.l	_shell_p
		move.l	reset_oldvec, _resvector
		move.l	valsave, _resvalid
		jmp		(a6)


