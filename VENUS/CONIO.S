;
; @(#) Gemini\conio.s
; @(#) Stefan Eissing, 09. MÑrz 1993
;
; -  interface for "BIOS in Windows"
;
; jr: added line a stuff
; jr: added support for MagiC task manager

		.import	ConsoleDispCanChar		; Vector for conout
		.import	ConsoleDispRawChar		; Vector for rconout

		.export conoutXBRA, rconoutXBRA, MyXBiosXBRA, MyXBios30XBRA
		.export costatXBRA, rcostatXBRA
		
		.export	LineAInit, LineAExit, LineASavX, LineASavY
		.export LineACelMX, LineACelMY, LineACurX, LineACurY

		.export PIsTaskman

		.text

costatXBRA:
		dc.b	'XBRA'					; Magic
		dc.b	'GMNI'					; ID
		ds.l	1						; alter Vektor	
costat:
		moveq.l	#-1,d0					; Zeichen kann gesendet werden
		rts
		
rcostatXBRA:
		dc.b	'XBRA'					; Magic
		dc.b	'GMNI'					; ID
		ds.l	1						; alter Vektor	
rcostat:
		moveq.l	#-1,d0					; Zeichen kann gesendet werden
		rts
		
conoutXBRA:
		dc.b	'XBRA'					; Magic
		dc.b	'GMNI'					; ID
		ds.l	1						; alter Vektor	
conout:	
		movem.l	d0-a6, -(a7)
		move.l	PIsTaskman, a0
		btst	#0, 3(a0)
		bne.s	oldconout
		move.w	66(sp),d0
		jsr		ConsoleDispCanChar
		movem.l	(a7)+, d0-a6
		rts
		
oldconout:
		movem.l	(a7)+, d0-a6
		move.l	conoutXBRA+8, -(sp)
		rts

rconoutXBRA:
		dc.b	'XBRA'					; Magic
		dc.b	'GMNI'					; ID
		ds.l	1						; alter Vektor	
rconout:
		movem.l	d0-a6, -(a7)
		move.l	PIsTaskman, a0
		btst	#0, 3(a0)
		bne.s	oldrconout
		move.w	66(sp),d0
		jsr		ConsoleDispRawChar
		movem.l	(a7)+, d0-a6
		rts

oldrconout:
		movem.l	(a7)+, d0-a6
		move.l	rconoutXBRA+8, -(sp)
		rts

		.super
		
MyXBiosXBRA:
			dc.b	'XBRA'					; Magic
			dc.b	'GMNI'					; ID
oldXBios:	ds.l	1						; alter Vektor	
MyXBios:  move    usp, a0
          btst.b  #5,(a7)          ; Supervisor-Modus
          beq     _usp
          movea.l a7,a0 
		  addq.w  #6,a0            ; 6 Bytes auf dem Stack 
_usp:     cmpi.w  #21,(a0)        	 ; Code fÅr 'cursconf'
          bne.b   NormalXBios
          cmp.w   #5,2(a0)  	     ; cursconf: getrate
          beq.b   NormalXBios
          rte                     ; Schluss
NormalXBios:
          move.l  oldXBios, a0      ; alten Vektor nehmen
          jmp     (a0)

MyXBios30XBRA:
			dc.b	'XBRA'					; Magic
			dc.b	'GMNI'					; ID
old30XBios:	ds.l	1						; alter Vektor	
MyXBios030:
          move    usp,a0
          btst.b  #5,(a7) 
          beq     usp030
          movea.l a7,a0 
          addq.w  #8,a0            ; 8 Bytes auf dem Stack
usp030:
          cmpi.w  #21,(a0)
          bne.b	  Normal30
          cmp.w   #5,2(a0)        ; cursconf: getrate
          beq.b   Normal30
          rte                     ; Schluss
Normal30: move.l  old30XBios, a0
          jmp     (a0)


LineAInit:
		movem.l	a1-a6/d0-d7,-(sp)
		dc.w	$a000
		movem.l	(sp)+,a1-a6/d0-d7

		move.l	-$14e(a0), oldsavx
		pea		-$14e(a0)
		move.l	(sp)+, LineASavX

		move.l	-$14c(a0), oldsavy
		pea		-$14c(a0)
		move.l	(sp)+, LineASavY

		move.l	-$2c(a0), oldcelmx
		pea		-$2c(a0)
		move.l	(sp)+, LineACelMX

		move.l	-$2a(a0), oldcelmy
		pea		-$2a(a0)
		move.l	(sp)+, LineACelMY
		
		move.l	-$1c(a0), oldcurx
		pea		-$1c(a0)
		move.l	(sp)+, LineACurX
		
		move.l	-$1a(a0), oldcury
		pea		-$1a(a0)
		move.l	(sp)+, LineACurY
		
		rts
		
LineAExit:
		movem.l	a1-a6/d0-d7,-(sp)
		dc.w	$a000
		movem.l	(sp)+,a1-a6/d0-d7
	
		move.w	oldcelmx, -$2c(a0)
		move.w	oldcelmy, -$2a(a0)
		move.w	oldcurx, -$1c(a0)
		move.w	oldcury, -$1a(a0)
		move.w	oldsavx, -$14e(a0)
		move.w	oldsavy, -$14c(a0)

		clr.l	d0
		move.l	d0, LineACelMX
		move.l	d0, LineACelMY
		move.l	d0, LineACurX
		move.l	d0, LineACurY
		move.l	d0, LineASavX
		move.l	d0, LineASavY

		rts

		.data

a_zero:		.dc.l	0
PIsTaskman:	.dc.l	a_zero
	
		.bss
	
LineACelMX:	.ds.l	1
LineACelMY:	.ds.l	1
LineACurX:	.ds.l	1
LineACurY:	.ds.l	1
LineASavX:	.ds.l	1
LineASavY:	.ds.l	1

oldcelmx:		.ds.l	1
oldcelmy:		.ds.l	1
oldcurx:		.ds.l	1
oldcury:		.ds.l	1
oldsavx:		.ds.l	1
oldsavy:		.ds.l	1


		.end
