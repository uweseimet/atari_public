/************************************************************/
/* HDDRIVER driver module startup code for Pure C 1.00 Beta */
/*                                                          */
/* To be added as first item in the project file            */
/*                                                          */
/* (C) 2019 Uwe Seimet                                      */
/************************************************************/

	export isHddriverModule		;to be used by module-aware modules
	export Ptermres						;overrides the standard Ptermres()


	text

	bra.w .nomodule						;started as regular program-

	st ismodule								;started as module

	lea $1c(a2),a0
	add.l 2(a2),a0
	add.l 6(a2),a0
	move.l 10(a2),d0
	bra.s .clrbss
.loop:	clr.b (a0)+					;clear BSS
.clrbss:	subq.l #1,d0
	bpl .loop

	move.l sp,savssp
	lea stk+400,sp
	movem.l d2-d7/a2-a6,-(sp)	;D2-D7 and A2-A6 must not be modified

	clr d0										;argc
	sub.l a0,a0								;argv
	jmp main

.nomodule:	jmp __text			;execute as regular Pure C program-

;for modules Ptermres() is replaced by Mshrink()
Ptermres:
	tst.b ismodule
	beq.s .ptermres						;normal termination if this is no module

	lea stk+400-44,sp
	movem.l (sp),d2-d7/a2-a6

	move.l 2(a2),a1
	add.l 6(a2),a1
	add.l 10(a2),a1
	pea $1c(a1)
	pea (a2)
	clr -(sp)
	move #$4a,-(sp)						;Mshrink
	trap #1
	lea 12(sp),sp

	movem.l (sp)+,d2-d7/a2-a6
	move.l savssp,sp
	rts

.ptermres:	move d1,-(sp)
	move.l d0,-(sp)
	move #$31,-(sp)						;Ptermres
	trap #1

;extern boolean isHddriverModule(void)
isHddriverModule:
	clr d0
	move.b ismodule,d0
	rts


	data

savssp: ds.l 1

stk:	ds.l 100

ismodule:	ds.b 1