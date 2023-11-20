/****************************/
/* SCSI_MON 1.50            */
/*                          */
/* (C) 1999-2023 Uwe Seimet */
/****************************/

	export conout, auxout, prtout

conout:
	move.l $586,a0
out:	movem.l a1-a6/d1-d7,-(sp)
	move d0,-(sp)
	clr -(sp)
	jsr (a0)
	addq.l #4,sp
	movem.l (sp)+,a1-a6/d1-d7
	rts
	
auxout:
	move.l $582,a0
	bra out

prtout:
	move.l $57e,a0
	bra out
