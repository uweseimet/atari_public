*************************************
*                                   *
* ROMSPEED V3.11                    *
*                                   *
* Copies the ROM contents into RAM, *
* where they are executed faster.   *
*                                   *
* Supports MagiC and the            *
* virtual memory manager OUTSIDE.   *
*                                   *
* The sources are compatible with   *
* the Easy Rider assembler.         *
*                                   *
* (C) 1993-2012 Uwe Seimet          *
*                                   *
*************************************


GEMDOS	= 1
CCONWS	= 9
SUPER	= 32
PTERMRES	= 49
MALLOC	= 72
MSHRINK	= 74
PTERM	= 76

_sysbase	= $4f2
_p_cookies= $5a0


	mc68030
	loadfast
	ttmem
	ttram


	text

	move.l 4(sp),a0
	move.l 12(a0),a1
	add.l 20(a0),a1
	add.l 28(a0),a1
	lea $100(a1),a1
	move.l a1,prglen
	pea (a1)
	pea (a0)
	clr -(sp)
	move #MSHRINK,-(sp)
	trap #GEMDOS
	lea 12(sp),sp

	clr.l -(sp)
	move #SUPER,-(sp)
	trap #GEMDOS
	addq.l #6,sp
	move.l d0,d4

	move.l _sysbase,a0
	move.l $08(a0),a0
	move $1c(a0),d0
	lsr #1,d0
	cmp #7,d0	;SWF?
	bne *+2
	moveq #2,d0	;FRA
	cmp #8,d0	;SWG?
	bne *+2
	moveq #1,d0	;FRG
	cmp #3,d0
	bcs *+2
	clr d0	;UK otherwise
	move d0,country

	move.l _p_cookies,d0
	beq.s .nocook
	move.l d0,a0
.cloop:	movem.l (a0)+,d0-d1
	tst.l d0
	beq.s .nocook
	cmp.l #"_AKP",d0
	bne .cloop
	lsr #8,d1	;isolate language code
	cmp #7,d1	;SWF?
	bne *+2
	moveq #2,d1	;FRA
	cmp #8,d1	;SWG?
	bne *+2
	moveq #1,d1	;FRG
	cmp #3,d1
	bcs *+2
	clr d1	;UK otherwise
	move d1,country

.nocook:	lea g_tt,a3
	cmp #1,country
	beq.s .c1
	lea f_tt,a3
	cmp #2,country
	beq.s .c1
	lea e_tt,a3
.c1:	move.l _p_cookies,a6
loop:	movem.l (a6)+,d0-d1
	tst.l d0
	beq .endjar	;end of cookie list-
	lea g_tt,a3
	cmp #1,country
	beq.s .c2
	lea f_tt,a3
	cmp #2,country
	beq.s .c2
	lea e_tt,a3
.c2:	cmp.l #"MagX",d0
	bne.s .nomag
	st magxflg
.nomag:	cmp.l #"_MCH",d0
	bne .nomch
	cmp.l #$00020000,d1	;TT or newer?
	bcs error	;no-
.nomch:	lea g_mmu,a3
	cmp #1,country
	beq.s .c3
	lea f_mmu,a3
	cmp #2,country
	beq.s .c3
	lea e_mmu,a3
.c3:	cmp.l #"PMMU",d0	;MMU program already active?
	beq error
	cmp.l #"SMTT",d0	;SM124 emulator?
	beq error
	bra loop

.endjar:	move.l _sysbase,a0
	tst.b magxflg
	beq.s .nom
	lea $00e00000,a0
.nom:	move.l 8(a0),a0
	move.l a0,a1
	add.l #524288-8,a0	;skip checksums
	move.l -4(a0),d0
.getlen:	cmp.l -(a0),d0
	beq .getlen              	;skip unused bytes
	addq.l #4,a0
	sub.l a1,a0
	pea (a0)
	add.l #32767,a0
	pea (a0)
	move #MALLOC,-(sp)
	trap #GEMDOS
	addq.l #6,sp
	move.l (sp)+,a0
	lea g_ninst,a3
	cmp #1,country
	beq.s .c4
	lea f_ninst,a3
	cmp #2,country
	beq.s .c4
	lea e_ninst,a3
.c4:	tst.l d0
	beq error
	add.l #32767,d0
	and #$8000,d0
	move.l d0,rombuf
	exg.l d0,a0
	lsr.l #2,d0
	move.l _sysbase,a1
	tst.b magxflg
	beq.s .nom0
	lea $00e00000,a1	;do not use MagiC base address-
.nom0:	move.l 8(a1),a1
.copyrom:move.l (a1)+,(a0)+
	subq.l #1,d0
	bne .copyrom

	move.l rombuf,d0
	or #5,d0                	;mark descriptor and write protect page
	move.l _sysbase,a0
	tst.b magxflg
	beq.s .nom1
	lea $00e00000,a0
.nom1:	ptestr #2,([8,a0]),#7,a0	;refer to user program address space
	move.l d0,(a0)	;enter in descriptor table
	pflusha

	bsr add_cookies

	move.l d4,-(sp)
	move #SUPER,-(sp)
	trap #GEMDOS	;back to user mode
	addq.l #6,sp

	lea g_mess,a0
	cmp #1,country
	beq.s .c5
	lea f_mess,a0
	cmp #2,country
	beq.s .c5
	lea e_mess,a0
.c5:	pea (a0)
	move #CCONWS,-(sp)
	trap #GEMDOS	;display initialization message
	addq.l #6,sp

	clr -(sp)
	move.l prglen,-(sp)
	move #PTERMRES,-(sp)
	trap #GEMDOS	;keep program resident

error:
	move.l d4,-(sp)
	move #SUPER,-(sp)
	trap #GEMDOS	;back to user mode
	addq.l #6,sp
	pea (a3)
	move #CCONWS,-(sp)
	trap #GEMDOS	;display error message
	addq.l #6,sp
quit:	lea g_ninst,a0
	cmp #1,country
	beq.s .c6
	lea f_ninst,a0
	cmp #2,country
	beq.s .c6
	lea e_ninst,a0
.c6:	pea (a0)
	move #CCONWS,-(sp)
	trap #GEMDOS
	addq.l #6,sp
	move #-1,-(sp)
	move #PTERM,-(sp)
	trap #GEMDOS	;that's it-


add_cookies:
	move.l _p_cookies,a0
.loop:	movem.l (a0)+,d0-d1
	tst.l d0
	bne .loop
	move.l a0,d2
	sub.l _p_cookies,d2
	lsr.l #3,d2	;results in number of cookies so far
	addq.l #2,d2	;2 cookies for ROMSPEED
	cmp.l d1,d2	;cookie jar full?
	bcs.s .nofull	;no-

	addq.l #8,d2	;8 additional cookies
	asl.l #3,d2
	move.l d2,-(sp)
	move #MALLOC,-(sp)
	trap #GEMDOS
	addq.l #6,sp
	tst.l d0
	beq.s .err
	move.l _p_cookies,a1
	move.l d0,a0
	move.l d0,a2
.copy:	movem.l (a1)+,d0-d1
	movem.l d0-d1,(a0)
	addq.l #8,a0
	tst.l d0
	bne .copy
	move.l a2,_p_cookies

.nofull:	subq.l #8,a0
	move.l #"PMMU",(a0)+
	clr.l (a0)+	;no PMMU handler
	move.l #"USRS",(a0)+
	move.l #rombuf-4,(a0)+
	movem.l d0-d1,(a0)
.err:	rts


	data

	dc.l "3.11"
rombuf:	dc.l 0

g_mess:	dc.b $0d,$0a
	dc.b $1b,"pROMSPEED V3.11 installiert",$1b,"q",$0d,$0a
	dc.b "½ 1993-2012 Uwe Seimet",$0d,$0a,$00
e_mess:	dc.b $0d,$0a
	dc.b $1b,"pROMSPEED V3.11 installed",$1b,"q",$0d,$0a
	dc.b "½ 1993-2012 Uwe Seimet",$0d,$0a,$00
f_mess:	dc.b $0d,$0a
	dc.b $1b,"pROMSPEED V3.11 install‚",$1b,"q",$0d,$0a
	dc.b "½ 1993-2012 Uwe Seimet",$0d,$0a,$00

g_ninst:	dc.b $0d,$0a
	dc.b "ROMSPEED nicht "
	dc.b "installiert!",$0d,$0a,$00
e_ninst:	dc.b $0d,$0a
	dc.b "ROMSPEED not "
	dc.b "installed!",$0d,$0a,$00
f_ninst:	dc.b $0d,$0a
	dc.b "ROMSPEED pas "
	dc.b "install‚!",$0d,$0a,$00

g_tt:	dc.b $0d,$0a
	dc.b "ROMSPEED l„uft nur "
	dc.b "auf dem Atari TT oder Falcon030!"
	dc.b $0d,$0a,$00
e_tt:	dc.b $0d,$0a
	dc.b "ROMSPEED requires "
	dc.b "an Atari TT or Falcon030!"
	dc.b $0d,$0a,$00
f_tt:	dc.b $0d,$0a
	dc.b "ROMSPEED ne fonctionne que "
	dc.b "sur TT ou Falcon030!"
	dc.b $0d,$0a,$00

g_mmu:	dc.b $0d,$0a
	dc.b "Es ist bereits ein "
	dc.b "MMU-Programm aktiv!"
	dc.b $0d,$0a,$00
e_mmu:	dc.b $0d,$0a
	dc.b "MMU already in use!"
	dc.b $0d,$0a,$00
f_mmu:	dc.b $0d,$0a
	dc.b "MMU est d‚j… actif!"
	dc.b $0d,$0a,$00


        	bss

prglen:	ds.l 1

country:	ds.w 1

magxflg:	ds.b 1
