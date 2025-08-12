/*********************************************************/
/* HDDRIVER module startup code for Pure C 2.00          */
/*                                                       */
/* To be added before PCSTART in the Pure C project file */
/*                                                       */
/* (C) 2020-2025 Uwe Seimet                              */
/*********************************************************/

	export isHddriverModule		;to be used by module-aware modules
	export Pterm0							;overrides the standard Pure C Pterm0()
	export Pterm							;overrides the standard Pure C Pterm()
	export Ptermres						;overrides the standard Pure C Ptermres()

STACKSIZE	= 400


	text

	bra.w .nomodule						;started as regular program-

	move.l $5a0,d0
	beq.s .nocpu							;no cookie jar-
	move.l d0,a0
.loop:	move.l (a0)+,d0
	beq.s .nocpu							;no _CPU cookie-
	move.l (a0)+,d1
	cmp.l #"_CPU",d0
	bne.s .loop
	cmp #40,d1
	bcs.s .nocpu
	dc.w $f4f8								;clear 040/060 cache after relocation

.nocpu:
	lea $1c(a2),a0						;on startup A2 points to the binary
	add.l 2(a2),a0
	add.l 6(a2),a0
	move.l 10(a2),d0
	bra.s .clrbss
.clr:	clr.b (a0)+						;clear BSS
.clrbss:	subq.l #1,d0
	bpl .clr

	st ismodule								;started as module

	move.l sp,savssp
	lea stk+STACKSIZE,sp
	movem.l d2-d7/a2-a6,-(sp)	;D2-D7 and A2-A6 must not be modified

	clr d0										;argc
	sub.l a0,a0								;argv
	jmp main

.nomodule:	jmp __text			;execute as regular Pure C program-


;for modules a call to Pterm0() is replaced by a call to Mfree()
Pterm0:
	tst.b ismodule
	beq.s .nomodule						;normal termination if this is no module

	lea stk+STACKSIZE-44,sp
	movem.l (sp),d2-d7/a2-a6

	pea (a2)									;A2 points to the binary
	move #$49,-(sp)						;Mfree(), but no Pterm0()
	trap #1
	addq.l #6,sp

	movem.l (sp)+,d2-d7/a2-a6
	move.l savssp,sp
	rts

.nomodule:	clr -(sp)				;non-module termination with Pterm0()
	trap #1


;for modules a call to Pterm() is replaced by a call to Mfree()
Pterm:
	tst.b ismodule
	beq.s .nomodule						;normal termination if this is no module

	lea stk+STACKSIZE-44,sp
	movem.l (sp),d2-d7/a2-a6

	pea (a2)									;A2 points to the binary
	move #$49,-(sp)						;Mfree(), but no Pterm()
	trap #1
	addq.l #6,sp

	movem.l (sp)+,d2-d7/a2-a6
	move.l savssp,sp
	rts

.nomodule:	move d0,-(sp)		;exit code
	move #$4a,-(sp)						;non-module termination with Pterm()
	trap #1


;for modules a call to Ptermres() is replaced by a call to Mshrink()
Ptermres:
	tst.b ismodule
	beq.s .nomodule						;normal termination if this is no module

	lea stk+STACKSIZE-44,sp
	movem.l (sp),d2-d7/a2-a6

	move.l 2(a2),a1						;A2 points to the binary
	add.l 6(a2),a1
	add.l 10(a2),a1
	pea $1c(a1)
	pea (a2)
	clr -(sp)
	move #$4a,-(sp)						;Mshrink(), but no Ptermres()
	trap #1
	lea 12(sp),sp

	movem.l (sp)+,d2-d7/a2-a6
	move.l savssp,sp
	rts

.nomodule:	move d1,-(sp)
	move.l d0,-(sp)
	move #$31,-(sp)						;non-module termination with Ptermres()
	trap #1

*int isHddriverModule(void);
isHddriverModule:
	clr d0
	move.b ismodule,d0
	rts


	bss

savssp: ds.l 1

stk:	ds.l STACKSIZE/4

ismodule:	ds.b 1