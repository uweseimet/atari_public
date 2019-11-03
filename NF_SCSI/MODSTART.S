/************************************************************/
/* HDDRIVER driver module startup code for Pure C 1.00 Beta */
/*                                                          */
/* To be added as first item in the project file            */
/*                                                          */
/* (C) 2019 Uwe Seimet                                      */
/************************************************************/

	export Ptermres						;overrides the standard Ptermres()

	text

	cmp.l #"HDDR",d0					;check HDDRIVER module magic
	seq ismodule
	bne.s .nomodule

	clr.l _PgmSize

	move.l sp,savssp
	lea stk+400(pc),sp
	movem.l d2-d7/a2-a6,-(sp)	;D2-D7 and A2-A6 must not be modified

	move.l a2,a6							;pointer to module code

	move.l 2(a6),a1
	add.l 6(a6),a1
	add.l 10(a6),a1
	pea $1c(a1)
	pea (a6)
	clr -(sp)
	move #$4a,-(sp)						;Mshrink
	trap #1
	lea 12(sp),sp

	lea $1c(a6),a0
	add.l 2(a6),a0
	add.l 6(a6),a0
	move.l 10(a6),d0
	bra.s .clrbss
.loop:	clr.b (a0)+					;clear BSS
.clrbss:	subq.l #1,d0
	bpl .loop

	clr d0										;argc
	sub.l a0,a0								;argv
	jmp main

.nomodule:	jmp __text

Ptermres: tst.b ismodule
	beq.s .ptermres						;normal termination if this is no module

	lea stk+400-44(pc),sp
	movem.l (sp)+,d2-d7/a2-a6
	move.l savssp(pc),sp
	rts

.ptermres:	move d1,-(sp)
	move.l d0,-(sp)
	move #$31,-(sp)							;Ptermres
	trap #1


	data

savssp: ds.l 1

stk:	ds.l 100

ismodule:	ds.b 1