/***************************************/
/* Afterburner040 PMMU fix module 1.00 */
/*                                     */
/* (C) 2022-2023 Uwe Seimet            */
/***************************************/

	globl execute

execute:
	move sr,-(sp)
	ori #$0700,sr
	move.l sp,a1
	move.l $2c,a2
	lea .nommu(pc),a3
	move.l a3,$2c				;Ignore LINE-F exception if there is no PMMU
	nop
	moveq #0,d0
	movec d0,cacr
	nop
	dc.w $f4d8					;CINVA BC
	nop
	dc.w $f518					;PFLUSHA
	nop
	move.l #$ffe000,d1
	dc.w $4e7b,$1004		;MOVEC D1,ITT0
	dc.w $4e7b,$1005		;MOVEC D1,ITT1
	move.l #$ffe040,d1
	dc.w $4e7b,$1006		;MOVEC D1,DTT0
	dc.w $4e7b,$1007		;MOVEC D1,DTT1
	dc.w $4e7b,$0003		;MOVEC D0,TC
	dc.w $4e7b,$0807		;MOVEC D0,SRP
	dc.w $4e7b,$0806		;MOVEC D0,URP
	movec d0,vbr
.nommu:	move.l a2,$2c
	nop
	move.l a1,sp
	move (sp)+,sr
	rts