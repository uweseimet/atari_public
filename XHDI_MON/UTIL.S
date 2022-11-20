ERROR  = -1
EDRIVE = -46

	globl dispatch

	text

	dc.l $27011992
dispatch:
	link a6,#-4
	movem.l a0-a5/d1-d7,-(sp)
	move 8(a6),d2	;Funktionsnummer
	moveq #EDRIVE,d0	;ungÅltige Funktion?
	cmp #20,d2	;korrekte Funktionsnummer?
	bcc.s .user	;nein-
	mulu #8,d2
	lea dispatcher(pc),a1
	moveq #ERROR,d0
	move sr,d1
	btst #13,d1
	beq.s .user
	move.l 4(a1,d2),d0
	move.l d0,-4(a6)
	sub.l d0,sp
	bra.s .push1
.push0:	move.b 10(a6,d0),(sp,d0)
.push1:	dbra d0,.push0
	move.l (a1,d2),a0
	jsr (a0)
	move.l -4(a6),d2
	add.l d2,sp
.user:	movem.l (sp)+,a0-a5/d1-d7
	unlk a6
	cmp.l #EDRIVE,d0
	bne.s .noedrv
	move.l xhdi,-(sp)
.noedrv:	rts
