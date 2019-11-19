/***********************************************/
/* SCSI Driver for Hatari and ARAnyM 1.10 Beta */
/*                                             */
/* (C) 2016-2019 Uwe Seimet                    */
/***********************************************/


resvalid	= $426
resvector= $42a
_p_cookies = 0x5a0

MAX_JAR_SIZE = 1024

NF_ID = $7300
NF_CALL = $7301


						globl getCookie,setCookie,nfDetect,nfId,nfCall,jarSize

getCookie:
						move.l _p_cookies,d1
						beq.s .nocookie
						move.l d1,a1
.loop:			movem.l (a1)+,d1-d2
						tst.l d1
						beq.s .nocookie
						cmp.l d0,d1
						bne .loop
						move.l d2,(a0)
						moveq #1,d0
						rts
.nocookie:	clr.l d0
						rts

setCookie:
						movem.l d2-d3,-(sp)

						lea cookies,a1
						move.l _p_cookies,d2
						move.l d2,a0
						bne.s .hasJar					;cookie jar already exists-

						move.l resvalid,valsave
						move.l resvector,vecsave
						move.l #handleReset,resvector
						move.l #$31415926,resvalid
						addq.l #8,a1
						moveq #8,d3						;8 new entries
						bra.s .createJar

.hasJar:		movem.l (a0)+,d2-d3		;copy existing entries,
						movem.l d2-d3,(a1)		;they may be needed for a new jar
						addq.l #8,a1
						tst.l d2
						bne .hasJar

						move.l a0,d2
						sub.l _p_cookies,d2
						lsr.l #3,d2						;number of existing cookies
						cmp.l d3,d2
						bcs.s .notFull				;there is still room-

						addq.l #8,d3					;8 additional entries
.createJar:	move.l #cookies,_p_cookies
						move.l a1,a0

.notFull:		clr.l (a0)+
						move.l d3,(a0)
						movem.l d0-d1,-12(a0)
						movem.l (sp)+,d2-d3
						rts

handleReset:
						clr.l _p_cookies
						move.l vecsave(pc),resvector
						move.l valsave(pc),resvalid
						jmp (a6)

nfDetect:
						clr.l d0
						move.l sp,a1
						move.l $10,a0
						move.l #.noNf,$10
						pea nf_version(pc)
						subq.l #4,sp
						dc.w NF_ID
						tst.l d0
						beq.s .noNf						;NatFeats not available-
						moveq #1,d0
.noNf:			move.l a1,sp
 						move.l a0,$10
						rts

nfId:
						dc.w NF_ID
						rts

nfCall:
						dc.w NF_CALL
						rts


nf_version:	dc.b "NF_VERSION",0


						even

vecsave:		ds.l 1
valsave:		ds.l 1


						bss

cookies:		ds.b MAX_JAR_SIZE		;reserved for cookie jar extension