****************************
*                          *
* RSSCONV 3.01             *
*                          *
* RSC-Konverter            *
*                          *
* (C) 1993-2023 Uwe Seimet *
*                          *
****************************

	include "SYSTEM.ASM"
	include "RSCCONV.ASM"


	ttram
	ttmem
	loadfast
	locoff


	text

start:
	move.l sp,a0
	lea stack+400,sp		;Stackpointer initialisieren
	move.l 4(a0),a0
	lea 128(a0),a4		;Pointer auf Kommandozeile
	move.l 12(a0),a1
	add.l 20(a0),a1
	add.l 28(a0),a1
	lea $100(a1),a1
	pea (a1)
	pea (a0)
	clr -(sp)
	move #MSHRINK,-(sp)
	trap #GEMDOS
	lea 12(sp),sp
	tst.l d0
	bne error
	tst.b (a4)		;Parameter vorhanden?
	sne parflg
	beq nocom		;nein-

.loop:	addq.l #1,a4
	tst.b (a4)		;Ende der Kommandozeile?
	beq skip 		;ja-
	cmp.b #$0d,(a4)
	beq skip 		;ja-
	cmp.b #'-',(a4)		;Option?
	beq.s .skip		;ja-
	cmp.b #' ',(a4)
	beq .loop
	tst.b namflg		;Name schon vorhanden?
	bne perror		;ja-
	st namflg
	lea inpath,a0
.lop:	move.b (a4)+,d0
	cmp #$0d,d0
	beq.s .eskip		;Ende der Kommandozeile-
	cmp #' ',d0
	beq .loop
	move.b d0,(a0)+		;Dateinamen und Pfad kopieren
	bne .lop
.eskip: 	cmp.l #inpath,a0
	beq.s .next
	cmp.b #"\",-(a0)
	bne.s .eskip
	addq.l #1,a0
.next:	lea nam,a1
	move.l a0,a2
	moveq #11,d0
.copy:	move.b (a2)+,(a1)+	;Filenamen kopieren
	dbra d0,.copy
	bsr setpath
	bne perror
	bra skip
.skip:	addq.l #1,a4
	move.b (a4)+,d0
	bclr #5,d0
 	cmp.b #'S',d0		;Startobjekt? -s
	bne.s .skip2		;nein-
	bsr get			;Startobjekt eintragen
	bra .loop+2
.skip2:	cmp.b #'L',d0		;Labels global? -l
	bne.s .skip3		;nein-
	st globflg
	bra .loop
.skip3: 	cmp.b #'D',d0		;DATA-Segment? -d
	bne.s .nodat		;nein-
	st datflg
	bra .loop
.nodat:	cmp.b #'U',d0		;Buttons als USERDEF auswerten? -u
	bne.s .nouser
	st userflg
	bra .loop
.nouser:	cmp.b #'X',d0		;langes RSC-Format?
	bne.s .noext
	st extflg
	bra .loop
.noext:	cmp.b #'N',d0		;objcnr-Vorgabe? -n
	bne perror		;nein-
	move.b (a4)+,d0
	cmp.b #'0',d0
	bcs perror
	cmp.b #':',d0
	bcc perror
	move.b d0,onr
	move.b d0,tnr
	move.b d0,nobs3
	move.b d0,tobs3
	bra .loop

nocom:	lea intin,a5		;Pointer auf INTIN-Array
	lea intout,a6		;Pointer auf INTOUT-Array
	moveq #APPL_INIT,d0
	move.l #$00010000,d1
	bsr aes
	tst (a6) 		;Fehler?
	bmi error		;ja-
	move #_objcnr0,(a5)
	bra.s fix1
fix:	moveq #RSRC_OBFIX,d0
	move.l #$01010100,d1
	lea objc0000,a0
	bsr aesobj		;Koordinaten umrechnen
fix1:	subq #1,(a5)
	bpl fix			;nÑchstes Objekt
	moveq #WIND_GET,d0
	move.l #$02050000,d1
	clr (a5)
	move #5,2(a5)
	bsr aes
	move.l 2(a6),deskxy
	move.l 6(a6),deskbh
act:	lea intin,a5		;Pointer auf
	lea intout,a6		;GEM-Arrays
	move #DGETDRV,-(sp)
	trap #GEMDOS
	addq.l #2,sp
	move d0,d4
	add.b #'A',d0
	lea inpath,a4
	move.b d0,(a4)+
	move.b #':',(a4)+
	addq #1,d4
	move d4,-(sp)
	pea (a4)
	move #DGETPATH,-(sp)
	trap #GEMDOS
	addq.l #8,sp
.epath:	tst.b (a4)+
	bne .epath
	subq.l #1,a4
	cmp.b #'\',(a4)
	beq.s .bsl
	move.b #'\',(a4)+
.bsl:	move.b #'*',(a4)+
	move.b #'.',(a4)+
	move.b #'R',(a4)+
	move.b #'S',(a4)+
	move.b #'C',(a4)+
	clr.b (a4)
	moveq #GRAF_MOUSE,d0
	move.l #$01010100,d1
	clr (a5) 		;Pfeil als Mauscursor
	bsr aes
	lea objc0000,a2		;Adresse der Dialogbox
	bsr form_do		;zum Benutzerdialog
	cmp.b #OK,d7		;Konvertierung erwÅnscht?
	bne quit 		;nein-Programm verlassen
	lea objc0000,a0
	move #LONGFORMAT*24,d0
	btst #0,11(a0,d0)		;langes Format?
	sne extflg
	move #DATA*24,d0
	btst #0,11(a0,d0) 	;DATA-Segment?
	sne datflg
	move #GLOBAL*24,d0
	btst #0,11(a0,d0) 	;Globale Objekte?
	sne globflg
	move #START*24,d0
	move.l ob_spec(a0,d0),a4
	move.l (a4),a4
	bsr get			;Startobjekt holen
	moveq #FSEL_INPUT,d0
	move.l #$00020200,d1
	move.l #insel,addrin+4
	lea inpath,a0
	bsr aesobj		;Eingabe des Filenamens
	tst 2(a6)		;Abbruch?
	beq quit			;ja-
	moveq #GRAF_MOUSE,d0
	move.l #$01010100,d1
	move #2,(a5)		;Biene als Mauscursor
	bsr aes
	lea inpath,a0
.test:	tst.b (a0)+		;Pfadende suchen
	bne .test
.test0: 	cmp.l #inpath,a0
	beq.s .next
	cmp.b #"\",-(a0)
	bne .test0
	addq.l #1,a0
.next:	st namflg
	bsr setpath
	bne frmerr
	lea nam,a1
	lea insel,a0
tsel:	move.b (a0)+,(a1)+	;Filenamen kopieren
	bne tsel

*Einsprung bei Parametern in Kommandozeile
skip:	tst.b namflg		;Dateiname vorhanden?
	beq perror		;nein-
	pea dta
	move #FSETDTA,-(sp)
	trap #GEMDOS
	addq.l #6,sp
	lea nam,a0
fnam3:	move.b (a0)+,d0
	beq.s nonam3
	cmp.b #".",d0
	bne fnam3
nonam3: 	move.b #".",-1(a0)
	move.b #"H",(a0)+ 	;.H Extension an Filenamen anhÑngen
	bsr load
	move.l a3,a0		;Pointer auf Dateibeginn
	move.l dta+26,d0
	clr.b (a0,d0)
	move.l d0,-(sp)
	move #MALLOC,-(sp)	;Speicherplatz fÅr das zu erzeugende
	trap #GEMDOS		;File reservieren
	addq.l #6,sp
	tst.l d0
	beq memerr1		;nicht genug Speicherplatz-
	move.l d0,a4
	move.l a4,a2
	move.b (a3)+,d0
	bra.s nonew		;erster Durchlauf

conv:	move.b (a3)+,d0
	cmp.b #$0d,d0		;Zeilenbeginn?
	bne.s nonew		;nein-
	cmp.b #' ',-1(a2) 	;Leerzeichen am Zeilenende?
	bne.s nonew		;nein-
.loop:	cmp.b #' ',-(a2)		;ÅberzÑhlige Leerzeichen entfernen
	beq .loop
	addq.l #1,a2
nonew:	tst.b d0			;Dateiende?
	beq.s end		;ja-
	cmp.b #"#",d0		;'#define'?
	bne conv			;nein-
	addq.l #7,a3
conv1:	move.b (a3)+,d0
	move.b d0,(a2)+		;Definition in ASM-File Åbertragen
	cmp.b #" ",d0
	bne conv1		;Ende der "#define"-Anweisung suchen
	move.b #"=",(a2)+ 	;Gleichheitszeichen in ASM-File einfÅgen
	move.b #' ',(a2)+
conv2:	move.b (a3)+,d0
	cmp.b #'0',d0
	bcs conv2
	cmp.b #'9'+1,d0
	bcc conv2
	subq.l #1,a3
conv3:	move.b (a3)+,d0
	cmp.b #'0',d0
	bcs conv4
	cmp.b #'9'+1,d0
	bcc conv4
	move.b d0,(a2)+
	bra conv3
conv4:	move.b #$0d,(a2)+
	move.b #$0a,(a2)+
	bra conv

*Ende der .H.-Konvertierung
end:	sub.l a4,a2
	lea nam,a0
fnam:	move.b (a0)+,d0
	beq.s nonam
	cmp.b #".",d0
	bne fnam
nonam:	move.b #".",-1(a0)
	move.b #"A",(a0)+ 	;ASM-Extension an Filenamen anhÑngen
	move.b #"S",(a0)+
	move.b #"M",(a0)+
	clr.b (a0)
	clr -(sp)
	pea nam
	move #FCREATE,-(sp)
	trap #GEMDOS		;das wird die Zieldatei
	addq.l #8,sp
	move d0,d7		;Fehler?
	bmi operr		;ja-
	pea (a4)
	pea (a2)
	move d7,-(sp)
	move #FWRITE,-(sp)
	trap #GEMDOS		;.ASM-Datei sichern
	lea 12(sp),sp
	cmp.l d0,a2		;Fehler?
	bne operr		;ja-
	move d7,-(sp)
	move #FCLOSE,-(sp)
	trap #GEMDOS		;Arbeit erledigt
	addq.l #4,sp
ferr:	pea (a4)
	move #MFREE,-(sp)
	trap #GEMDOS		;benîtigten Speicherplatz wieder freigeben
	addq.l #6,sp
	pea (a3)
	move #MFREE,-(sp)
	trap #GEMDOS		;dito
	addq.l #6,sp
noch:

*Konvertierung von .RSC-Files
	lea nam,a0
fnam2:	move.b (a0)+,d0
	beq.s nonam2
	cmp.b #".",d0
	bne fnam2
nonam2: 	move.b #".",-1(a0) 	;.RSC-Extension setzen
	move.b #"R",(a0)+
	move.b #"S",(a0)+
	move.b #"C",(a0)+
	bsr load 		;Datei laden

	lea nam,a4
nam1:	move.b (a4)+,d0
	beq.s nonam1
	cmp.b #".",d0
	bne nam1
nonam1: 	move.b #".",-1(a4)
	move.b #"D",(a4)+
	move.b #"A",(a4)+
	move.l a4,tmp
	move.b #"T",(a4)+
	clr.b (a4)
	clr -(sp)
	pea nam
	move #FCREATE,-(sp)	;DAT-File îffnen
	trap #GEMDOS
	addq.l #8,sp
	lea opal,a0
	move d0,texth		;alles in Ordnung?
	bmi operr		;dummerweise nicht-
	clr.b -(a4)
	clr -(sp)
	pea nam
	move #FCREATE,-(sp)	;DA-File îffnen
	trap #GEMDOS
	addq.l #8,sp
	lea opal,a0
	move d0,datah		;Fehler?
	bmi operr		;scheint so-

	lea bsstext,a5		;Pointer auf Objektdaten
	lea bssdata,a6		;Pointer auf weitere Objektdaten
	move.l a3,a4		;Adresse der Struktur merken

	tst.b extflg
	beq.s .noext		;kein langes Format-
	add.l 4(a3),a4
	move.l 40(a3),d6
	bra.s .ext

.noext:	add 2(a3),a4		;Pointer auf baumgebundene Daten
	move 20(a3),d6		;Anzahl der Objekte
.ext:	lea dat,a0
	tst.b datflg		;DATA-Segment?
	beq.s .nodat		;nein-
.dat:	move.b (a0)+,(a5)+
	bne .dat
	subq.l #1,a5
.nodat:	lea txt0(pc),a0
	tst.b globflg		;Labels global?
	beq.s glob		;nein-
	lea tobs2(pc),a0
glob:	move.b (a0)+,(a5)+
	bne glob
	subq.l #1,a5
nog:	move.l a5,d0
	lea bsstext,a5
	sub.l a5,d0		;LÑnge der zu schreibenden Daten
	move.l d0,-(sp)
	pea (a5)
	move.l d0,-(sp)
	move texth,-(sp)
	move #FWRITE,-(sp)
	trap #GEMDOS		;konvertierte Daten auf Disk schreiben
	lea 12(sp),sp
	cmp.l (sp)+,d0
	bne operr		;Fehler-
	bra clop

*Schleife fÅr die Konvertierung einzelner Objekte
loop:	move.l a5,a0
	bsr crlf
	bsr crlf
	cmp #-1,(a4)		;neues Objekt?
	bne.s noobj		;nein-
	bsr objc 		;Objektnummer eintragen
	addq.l #1,objcnr
	move (a4),d1		;ob_next
	bsr dcw1
	bra.s noobj1
noobj:	move (a4),d1
	bsr dcw
noobj1: 	bsr word
	bsr crlf
	move ob_head(a4),d1
	bsr dcword
	move.b #',',(a0)+
	move ob_tail(a4),d1
	bsr word
	bsr crlf
	move ob_type(a4),d1
	move d1,d0
	lsr #8,d0
	tst.b d0 		;Extended Type?
	beq.s .skip		;nein-
	move d1,-(sp)
	and #$ff00,d1
	bsr dcword
	move (sp)+,d1
	and #$ff,d1
	move.b #'|',(a0)+
	bra.s .ext
.skip:	bsr dcw
.ext:	sub #20,d1
	bcs endr
	cmp #34,d1
	bcc endr
	asl #2,d1		;ergibt Offset auf String fÅr Objekttyp
	lea g_adr(pc),a5
	move.l (a5,d1),a5
	move.l a0,typpnt
.loop:	move.b (a5)+,(a0)+
	bne .loop
	subq.l #1,a0
	bsr crlf
	move ob_flags(a4),d1
	bsr dcword
	move.b #',',(a0)+
	move ob_state(a4),d1
	bsr word
	bsr crlf
	move.l a0,a5
	move ob_type(a4),d0
	and #$ff,d0
	sub #20,d0
	asl #2,d0
	lea typadr(pc),a0
	move.l (a0,d0),a0
	jsr (a0) 		;speziellen Objekttyp bearbeiten
	move.l a5,a0
	bsr crlf
	moveq #0,d0
	move ob_x(a4),d1
	bsr dcword
	move.b #',',(a0)+
	move ob_y(a4),d1
	bsr word
	bsr crlf
	move ob_width(a4),d1
	bsr dcword
	move.b #',',(a0)+
	move ob_height(a4),d1
	bsr word
	lea 24(a4),a4
	move.l a0,d0
	lea bsstext,a5
	sub.l a5,d0		;LÑnge der zu schreibenden Daten
	move.l d0,-(sp)
	pea (a5)
	move.l d0,-(sp)
	move texth,-(sp)
	move #FWRITE,-(sp)
	trap #GEMDOS		;konvertierte Daten auf Disk schreiben
	lea 12(sp),sp
	cmp.l (sp)+,d0
	bne operr		;Fehler-
	move.l a6,d0
	lea bssdata,a6
	sub.l a6,d0		;LÑnge der zu schreibenden Daten
	move.l d0,-(sp)
	pea (a6)
	move.l d0,-(sp)
	move datah,-(sp)
	move #FWRITE,-(sp)
	trap #GEMDOS		;konvertierte Daten schreiben
	lea 12(sp),sp
	cmp.l (sp)+,d0
	bne operr		;Fehler-
clop:	dbra d6,loop

*Ende der .RSC-Konvertierung
endr:	move.l a3,a4
	moveq #0,d0

	tst.b extflg
	beq.s .noext0		;kein langes Format-

	move.l 20(a3),d0
	add.l d0,a4
	move.l 60(a3),d6
	bra.s free

.noext0:	move 10(a3),d0
	add.l d0,a4		;Pointer auf Liste der freien Strings
	move 30(a3),d6		;Zahl der freien Strings
	bra.s free
floop:	move.l a6,a0
	bsr freeobj		;Objektnummer eintragen
	addq.l #1,freenr
	move.l (a4)+,a1		;Pointer auf freien String
	bsr maketxt1
	move.l a0,d0
	lea bssdata,a6
	sub.l a6,d0		;LÑnge der zu schreibenden Daten
	move.l d0,-(sp)
	pea (a6)
	move.l d0,-(sp)
	move datah,-(sp)
	move #FWRITE,-(sp)
	trap #GEMDOS		;konvertierte Daten auf Disk schreiben
	lea 12(sp),sp
	cmp.l (sp)+,d0
	bne operr		;Fehler-
free:	dbra d6,floop

	move.l a3,a4
	moveq #0,d0

	tst.b extflg
	beq.s .noext1		;kein langes Format-

	move.l 32(a3),d0
	add.l d0,a4
	move.l 64(a3),d6
	bra.s free1

.noext1:	move 16(a3),d0
	add.l d0,a4		;Pointer auf Liste der freien Images
	move 32(a3),d6		;Zahl der freien Images
	bra.s free1
floop1: 	move.l a6,a0
	bsr freeobj		;Objektnummer eintragen
	move.l (a4)+,a2		;Pointer auf freies Image
	add.l a3,a2
	bsr dcl1
	moveq #14,d1		;bi_pdata
	bsr disp1
	addq.l #1,freenr
	bsr bitblk
	move.l a0,d0
	lea bssdata,a6
	sub.l a6,d0		;LÑnge der zu schreibenden Daten
	move.l d0,-(sp)
	pea (a6)
	move.l d0,-(sp)
	move texth,-(sp)
	move #FWRITE,-(sp)
	trap #GEMDOS		;konvertierte Daten auf Disk schreiben
	lea 12(sp),sp
	cmp.l (sp)+,d0
	bne operr		;Fehler-
free1:	dbra d6,floop1

	tst.b extflg
	beq.s .noext2		;kein langes Format-

	lea 40(a3),a3
	moveq #0,d0
	move.l 4(a3),d0
	move.l d0,-(sp)		;Anzahl der BÑume
	move.l (a3),d0		;Anzahl der Objekte
	move.l d0,-(sp)
	lea nradr(pc),a1
	moveq #6,d3		;7 Nummern eintragen
.endrl:	moveq #0,d0
	move.l (a3)+,d0		;Anzahl der Objekte
	move.l (a1)+,a0
	moveq #3,d7
	move.l #1000,d2
	bsr int1
	dbra d3,.endrl
	bra.s endl

.noext2:	lea 20(a3),a3
	moveq #0,d0
	move 2(a3),d0
	move.l d0,-(sp)		;Anzahl der BÑume
	move (a3),d0		;Anzahl der Objekte
	move.l d0,-(sp)
	lea nradr(pc),a1
	moveq #6,d3		;7 Nummern eintragen
endr1:	moveq #0,d0
	move (a3)+,d0		;Anzahl der Objekte
	move.l (a1)+,a0
	moveq #3,d7
	move.l #1000,d2
	bsr int1
	dbra d3,endr1
endl:	move.l (sp)+,d0		;Objektzahl
	lea inobj(pc),a0
	moveq #3,d7
	move.l #1000,d2
	bsr int1 		;Objektzahl eintragen
	move.l (sp)+,d0		;Baumzahl
	lea tnobj(pc),a0
	moveq #3,d7
	move.l #1000,d2
	bsr int1 		;Baumzahl eintragen	
	pea header(pc)
	move.l #tobs2-header,-(sp)
	move datah,-(sp)
	move #FWRITE,-(sp)
	trap #GEMDOS
	lea 12(sp),sp
	tst.l d0
	bmi operr
	move datah,-(sp)
	move #FCLOSE,-(sp)
	trap #GEMDOS
	addq.l #4,sp
	tst.l d0
	bmi operr
	pea endcr(pc)
	pea 4
	move texth,-(sp)
	move #FWRITE,-(sp)
	trap #GEMDOS
	lea 12(sp),sp
	cmp #4,d0		;Fehler?
	bne operr		;ja-
	clr -(sp)
	pea nam
	move #FSFIRST,-(sp)
	trap #GEMDOS
	addq.l #8,sp
	tst d0
	bne operr
	move.l dta+26,-(sp)
	move #MALLOC,-(sp)
	trap #GEMDOS
	addq.l #6,sp
	move.l d0,d6
	beq operr
	clr -(sp)
	pea nam
	move #FOPEN,-(sp)		;.DA-Datei îffnen
	trap #GEMDOS
	addq.l #8,sp
	move.l d0,d7
	bmi operr
	move.l d6,-(sp)
	move.l dta+26,-(sp)
	move d7,-(sp)
	move #FREAD,-(sp)
	trap #GEMDOS
	lea 12(sp),sp
	tst.l d0
	bmi.s operr
	move d7,-(sp)
	move #FCLOSE,-(sp)
	trap #GEMDOS
	addq.l #6,sp
	bsr unlink		;.DA-Datei lîschen
	move.l d6,-(sp)
	move.l dta+26,-(sp)
	move texth,-(sp)
	move #FWRITE,-(sp)
	trap #GEMDOS
	lea 12(sp),sp
	cmp.l dta+26,d0
	bne.s operr
	move texth,-(sp)
	move #FCLOSE,-(sp)
	trap #GEMDOS
	addq.l #6,sp
	tst.b parflg
	bne.s noerr

quit:
	moveq #19,d0		;appl_exit
	move.l #$00010000,d1
	bsr aes

noerr:
	clr -(sp)
	trap #GEMDOS

perror:
	pea alert(pc)
wrer:	move #CCONWS,-(sp)
	trap #GEMDOS
	addq.l #6,sp
error:	move #-1,-(sp)
	move #PTERM,-(sp)
	trap #GEMDOS

operr:
	move.l tmp,a4
	clr.b (a4)
	bsr.s unlink
	move.b #'T',(a4)
	bsr.s unlink
	tst.b parflg
	beq.s .skip
	pea wrerr(pc)
	bra wrer
.skip:	lea opal(pc),a0
	moveq #52,d0		;form_alert
	move.l #$01010100,d1
	move #1,intin		;Default-Button
	bsr aesobj
	bra quit

unlink:
	pea nam
	move #FDELETE,-(sp)
	trap #GEMDOS
	addq.l #6,sp
	rts

*Fehler, da zu wenig freier Speicherplatz vorhanden
memerr1:
	pea (a3)
	move #MFREE,-(sp)
	trap #GEMDOS
	addq.l #6,sp

memerr:
	moveq #8,d1
frmerr:
	tst.b parflg
	bne error
	move d1,-(sp)
	moveq #GRAF_MOUSE,d0
	move.l #$01010100,d1
	clr (a5) 		;Pfeil als Mauscursor
	bsr aes
	move (sp)+,(a5)
	moveq #FORM_ERROR,d0
	move.l #$01010000,d1
	bsr aes
	bra quit

*Konvertierung der diversen Objekttypen

box:
	move.l a5,a0
	bsr dcl
	move.l ob_spec(a4),d1
	bsr long
	move.l a0,a5
	rts

text:
	move.l a5,a0
	bsr dcl
	move.l ob_spec(a4),a1
	add.l a3,a1
	move.l (a1),a1
	add.l a3,a1
text1:	bsr spec
	move.l a0,a5
	move.l a6,a0
	bsr even
	bsr spec
	move.b #":",(a0)+
	bsr dcl1
	move.l ob_spec(a4),a2
	add.l a3,a2
	bsr txt
	move.l specnr,d0
	bsr int
	move.b #',',(a0)+
	bsr plt
	move.l specnr,d0
	bsr int
	move.b #',',(a0)+
	bsr val
	move.l specnr,d0
	bsr int
	bsr crlf
	addq.l #1,specnr
	move 12(a2),d1		;te_font
	bsr dcword
	bsr crlf
	move 14(a2),d1		;te_resvd1
	bsr dcword
	bsr crlf
	move 16(a2),d1		;te_just
	bsr dcword
	bsr crlf
	move 18(a2),d1		;te_color
	bsr dcword
	bsr crlf
	move 20(a2),d1		;te_resvd2
	bsr dcword
	bsr crlf
	move 22(a2),d1		;te_thickness
	bsr dcword
	bsr crlf
	move 24(a2),d1		;te_txtlen
	bsr dcword
	move.b #',',(a0)+
	move 26(a2),d1		;te_tmplen
	bsr word
	bsr crlf
	move.l (a2),a1		;te_ptext
	bsr txt
	bsr spc
	bsr maketxt1
	move.l 4(a2),a1		;te_ptmplt
	bsr plt
	bsr spc
	bsr maketxt1
	move.l 8(a2),a1		;te_ptvalid
	bsr val
	bsr spc
	bsr maketxt1
	bsr crlf
	move.l a0,a6
	rts

boxtext:
	bra text

image:
	move.l a5,a0
	bsr dcl
	bsr spec
	move.l a0,a5
	move.l a6,a0
	bsr spec
	move.b #":",(a0)+
	bsr dcl1
	move.l ob_spec(a4),a2
	add.l a3,a2
	moveq #14,d1		;bi_pdata
	bsr disp
	addq.l #1,specnr
bitblk:	bsr crlf
	move 4(a2),d1		;bi_wb
	bsr dcword
	bsr crlf
	move 6(a2),d1		;bi_hi
	bsr dcword
	bsr crlf
	move 8(a2),d1		;bi_x
	bsr dcword
	move.b #',',(a0)+
	move 10(a2),d1		;bi_y
	bsr word
	bsr crlf
	move 12(a2),d1		;bi_color
	bsr dcword
	bsr crlf
	move 4(a2),d3		;bi_wb
	addq #1,d3
	lsr #1,d3		;ergibt Anzahl der Worte fÅr Breite
	move d3,d4
	mulu 6(a2),d3		;mal Hîhe, ergibt Gesamtzahl an Worten
	move.l (a2),a2		;bi_pdata
	bra makbin		;ja-

userdef:
	move.l a5,a0
	bsr dcl
	bsr spec
	move.l a0,a5
	move.l a6,a0
	bsr spec
	addq.l #1,specnr
	move.b #":",(a0)+
	bsr dcl1
	move.l ob_spec(a4),a2
	add.l a3,a2
	bsr user
	bsr crlf
	move.l 4(a4),d1		;ab_parm
	bsr dcl
	bsr long
	bra crlf

ibox:
	bra box

button:
	tst.b userflg
	beq string
	btst #6,ob_state+1(a4)
	bne.s userobj
	bra.s string

userobj:
	move.l typpnt,a0
	lea typ24,a1
.copy:	move.b (a1)+,d0
	beq.s .cont
	move.b d0,(a0)+
	bra .copy
.cont:	move.l a5,a0
	bsr dcl
	bsr parm
	move.l a0,a5
	move.l a6,a0
	bsr even
	bsr parm
	addq.l #1,parmnr
	move.b #":",(a0)+
	bsr dcl
	move.b #'0',(a0)+
	move.b #',',(a0)+
	add.l a3,a1
	move.l ob_spec(a4),a1
	bsr spec
	bsr crlf
	bsr crlf
	bsr spec
	addq.l #1,specnr
	move.b #":",(a0)+
	move.l ob_spec(a4),a1
	bsr maketxt1
	bsr crlf
	move.l a0,a6
	rts

boxchar:
	bra box

string:
	btst #6,ob_state+1(a4)
	bne userobj
	move.l a5,a0
	bsr dcl
	move.l ob_spec(a4),a1
	add.l a3,a1
	bsr spec
	move.l a0,a5
	move.l a6,a0
	bsr spec
	addq.l #1,specnr
	move.b #":",(a0)+
	move.l ob_spec(a4),a1
	bsr maketxt1
	bsr crlf
	move.l a0,a6
	rts

ftext:
	move.l a5,a0
	bsr dcl
	move.l ob_spec(a4),a1
	add.l a3,a1
	move.l 4(a1),a1
	add.l a3,a1
	bra text1

fboxtext:
	bra ftext

icon:
	move.l a5,a0
	bsr dcl
	move.l ob_spec(a4),a1
	add.l a3,a1
	move.l 8(a1),a1		;ib_ptext
	add.l a3,a1
	bsr spec
	move.l a0,a5
	move.l a6,a0
	bsr spec
	move.b #":",(a0)+
	bsr dcl1
	move.l ob_spec(a4),a2
	add.l a3,a2
	moveq #36,d1		;ib_pmask
	bsr disp
	move.b #',',(a0)+
	move 22(a2),d1		;ib_wicon
	lsr #3,d1		;Breite in Bytes
	mulu 24(a2),d1		;Gesamtzahl an Bytes
	move d1,-(sp)
	add #36,d1		;ib_pdata
	bsr disp
	move.b #',',(a0)+
	move (sp)+,d1
	add d1,d1
	add #36,d1		;ib_ptext
	bsr disp
	bsr crlf
	addq.l #1,specnr
	move 12(a2),d1		;ib_char
	bsr dcword
	bsr crlf
	move 14(a2),d1		;ib_xchar
	bsr dcword
	move.b #',',(a0)+
	move 16(a2),d1		;ib_ychar
	bsr word
	bsr crlf
	move 18(a2),d1		;ib_xicon
	bsr dcword
	move.b #',',(a0)+
	move 20(a2),d1		;ib_yicon
	bsr word
	bsr crlf
	move 22(a2),d1		;ib_wicon
	bsr dcword
	move.b #',',(a0)+
	move 24(a2),d1		;ib_hicon
	bsr word
	bsr crlf
	move 26(a2),d1		;ib_xtext
	bsr dcword
	move.b #',',(a0)+
	move 28(a2),d1		;ib_ytext
	bsr word
	bsr crlf
	move 30(a2),d1		;ib_wtext
	bsr dcword
	move.b #',',(a0)+
	move 32(a2),d1		;ib_htext
	bsr word
	bsr crlf
	move 34(a2),d1
	bsr dcword
	bsr crlf
	bsr crlf
	pea (a2)
	move 22(a2),d3		;ib_wicon
	addq #1,d3
	lsr #4,d3		;ergibt Anzahl Worte fÅr die Breite
	move d3,d4
	mulu 24(a2),d3		;mal Hîhe, ergibt Gesamtzahl an Worten
	movem d3-d4,iconsize
	move.l (a2),a2		;ib_pmask
	bsr makbin		;nach BinÑr wandeln
	movem iconsize,d3-d4
	move.l (sp),a2
	move.l 4(a2),a2		;ib_pdata
	bsr makbin		;nach BinÑr wandeln
	move.l (sp)+,a2
	move.l 8(a2),a1		;ib_ptext
	bsr maketxt
	bsr crlf
	move.l a0,a6
	rts

cicon:
	move.l a4,d0
	add.l #38,d0
	illegal
	move.l d0,ob_spec(a4)
	bsr icon			;zunÑchst Monochrom-Daten wandeln
	move.l 34(a2),a2		;mainlist
	move.b #',',(a0)+
	move.l a2,d1
	bsr long
.next:	move (a2),d1
	bsr dcword
	bsr crlf
	pea (a2)
	move.l 2(a2),a2		;col_data
	movem iconsize,d3-d4
	bsr makbin
	move.l (sp),a2
	move.l 6(a2),a2		;col_mask
	movem iconsize,d3-d4
	bsr makbin
	move.l (sp),a2
	move.l 10(a2),a2		;sel_data
	movem iconsize,d3-d4
	bsr makbin
	move.l (sp),a2
	move.l 14(a2),a2		;sel_mask
	movem iconsize,d3-d4
	bsr makbin
	move.l (sp)+,a2
	move.l a0,a6
	move.l 18(a2),a2		;next_res
	move.l a2,d0
	bne .next
	rts

title:
	bra string

spec:
	move.b #"s",(a0)+
	move.b #"p",(a0)+
	move.b #"e",(a0)+
	move.b #"c",(a0)+
	move.l specnr,d0
	bra int

txt:
	move.b #"t",(a0)+
	move.b #"x",(a0)+
	move.b #"t",(a0)+
	rts

plt:
	move.b #"p",(a0)+
	move.b #"l",(a0)+
	move.b #"t",(a0)+
	rts

val:
	move.b #"v",(a0)+
	move.b #"a",(a0)+
	move.b #"l",(a0)+
	rts

even:
	move.b #$09,(a0)+
	move.b #"e",(a0)+
	move.b #"v",(a0)+
	move.b #"e",(a0)+
	move.b #"n",(a0)+
	bra crlf

spc:
	move.l specnr,d0
	subq.l #1,d0
	bsr int
	move.b #':',(a0)+
	move.b #' ',(a0)+
	rts

*Objektnummer eintragen
objc:
	bsr crlf
	tst.b globflg		;Labels global?
	beq.s .noglob		;nein-
	lea glo(pc),a1
.loop:	move.b (a1)+,(a0)+
	bne .loop
	subq.l #1,a0
	move.l objcnr,d0
	bsr int
	bsr crlf
	bsr crlf
.noglob: move.b #"o",(a0)+
	move.b #"b",(a0)+
	move.b #"j",(a0)+
	move.b #"c",(a0)+
	move.l objcnr,d0
	bsr int
	move.b #":",(a0)+
	rts

freeobj:
	bsr crlf
	tst.b globflg		;Labels global?
	beq.s .noglob		;nein-
	lea glo1(pc),a1
.loop:	move.b (a1)+,(a0)+
	bne .loop
	subq.l #1,a0
	move.l freenr,d0
	bsr int
	bsr crlf
	bsr crlf
.noglob:	move.b #"f",(a0)+
	move.b #"r",(a0)+
	move.b #"e",(a0)+
	move.b #"e",(a0)+
	move.l freenr,d0
	bsr int
	move.b #":",(a0)+
	rts

*D3: Gesamtzahl an Worten, D4: Breite in Worten
makbin:
	add.l a3,a2
makb:	move d4,d5
	bsr dcw
	move (a2)+,d1
	bsr bin
	subq #1,d5
	beq.s .mb
	subq #1,d3
	beq.s .binend
	move.b #',',(a0)+
	move (a2)+,d1
	bsr bin
	subq #1,d5
	beq.s .mb
	subq #1,d3
	beq.s .binend
	move.b #',',(a0)+
	move (a2)+,d1
	bsr bin
	subq #1,d5
.mb:	bsr crlf
	subq #1,d3
	bne makb
	bra.s .skip
.binend:	bsr crlf
.skip:	bsr crlf
	move.l a0,a6
	rts

parm:	move.b #"p",(a0)+
	move.b #"a",(a0)+
	move.b #"r",(a0)+
	move.b #"m",(a0)+
	move.l parmnr,d0
	bra int

*Pointer auf USERBLK-Struktur eintragen
user:	move.b #"u",(a0)+
	move.b #"s",(a0)+
	move.b #"e",(a0)+
	move.b #"r",(a0)+
	move.l usernr,d0
	addq.l #1,usernr
	bra int

dcbst:	dc.b " b.cd",$09

	even

dcb1:
	moveq #4,d0
	bra.s dcb+2
dcb:
	moveq #5,d0
dcbl:	move.b dcbst(pc,d0),(a0)+
	dbra d0,dcbl
	rts

dcwst:	dc.b " w.cd",$09

	even

dcw1:	moveq #4,d0
	bra.s dcw+2
dcw:
	moveq #5,d0
dcwl:	move.b dcwst(pc,d0),(a0)+
	dbra d0,dcwl
	rts
dcword:
	bsr dcw
word:
	moveq #16,d0
	asl.l d0,d1
	moveq #1,d0
	bra.s makehex

dclst:	dc.b " l.cd",$09

	even

dcl1:	moveq #4,d0
	bra.s dcll
dcl:
	moveq #5,d0
dcll:	move.b dclst(pc,d0),(a0)+
	dbra d0,dcll
	rts

*BinÑrzahl umwandeln
bin:
	moveq #15,d0
	move.b #'%',(a0)+
.loop:	moveq #$30,d2
	add d1,d1
	bcc *+2
	moveq #$31,d2
	move.b d2,(a0)+
	dbra d0,.loop
	rts

long:
	moveq #3,d0

*Hexzahlen umwandeln
makehex:
	move.b #'$',(a0)+
hexlop: 	rol.l #8,d1
	move.b d1,d2
	lsr.b #4,d2
	and #$0f,d2
	move.b hexasc(pc,d2),(a0)+
	move.b d1,d2
	and #$0f,d2
	move.b hexasc(pc,d2),(a0)+
	dbra d0,hexlop
	rts

hexasc: dc.b "0123456789abcdef"

*Strings kopieren
maketxt1:
	add.l a3,a1
	bsr dcb1
	bra.s mtxt
*Pointer auf Text in A1, kopieren bis Null gefunden
maketxt:
	add.l a3,a1
	bsr dcb
mtxt:	tst.b (a1)
	beq null
	cmp.b #'@',(a1)
	bne.s cop
	move.b #'0',(a0)+
	addq.l #1,a1
	tst.b (a1)		;folgt weiterer Text?
	beq nul0
	move.b #',',(a0)+
cop:	move.b #$22,(a0)+
coptxt: 	tst.b (a1)
	beq nul
	tst.b txtflg
	bne scopy
	cmp.b #$0a,(a1)
	bne.s .nolf
	addq.l #1,a1
	move.b #$22,(a0)+
	move.b #',',(a0)+
	move.b #'$',(a0)+
	move.b #'0',(a0)+
	move.b #'a',(a0)+
	tst.b (a1)		;folgt weiterer Text?
	beq nul0		;nein-
	move.b #',',(a0)+
	move.b #$22,(a0)+
	bra coptxt
.nolf:	cmp.b #$0d,(a1)
	bne.s .nocr
	addq.l #1,a1
	move.b #$22,(a0)+
	move.b #',',(a0)+
	move.b #'$',(a0)+
	move.b #'0',(a0)+
	move.b #'d',(a0)+
	tst.b (a1)		;folgt weiterer Text?
	beq.s nul0		;nein-
	move.b #',',(a0)+
	move.b #$22,(a0)+
	bra coptxt
.nocr:	cmp.b #"'",(a1)
	beq.s .skip
	cmp.b #$22,(a1)
	bne.s scopy
.skip:	cmp.b #$22,-1(a0)
	bne.s .noqu0
	subq.l #1,a0
	bra.s .qu0
.noqu0:	move.b #$22,(a0)+
	move.b #',',(a0)+
.qu0:	move.b #'$',(a0)+
	move.b #'2',(a0)+
	move.b #'2',(a0)+
	cmp.b #$22,(a1)+
	beq.s .qu
	move.b #'7',-1(a0)
.qu:	tst.b (a1)		;folgt weiterer Text?
	beq.s nul0		;nein-
	move.b #',',(a0)+
	move.b #$22,(a0)+
	bra coptxt
scopy:	move.b (a1)+,(a0)+
	bra coptxt
nul:	move.b #$22,(a0)+
nul0:	move.b #',',(a0)+
	move.b #"0",(a0)+
	bra.s crlf
null:	move.b #"0",(a0)+

*neue Zeile beginnen
crlf:
	move.b #$0d,(a0)+
	move.b #$0a,(a0)+
	rts

disp:
	move d1,-(sp)
	bsr spec
	move (sp)+,d1
	move.b #"+",(a0)+
	bra word

disp1:
	move d1,-(sp)
	move.b #"f",(a0)+
	move.b #"r",(a0)+
	move.b #"e",(a0)+
	move.b #"e",(a0)+
	move.l freenr,d0
	bsr.s int
	move (sp)+,d1
	move.b #"+",(a0)+
	bra word

*Ziffern umwandeln
int:
	moveq #3,d7
	move.l #1000,d2		;Startwert fÅr Subtraktion
int1:	moveq #-1,d1		;zÑhlt Subtraktionen
int0:	addq.b #1,d1
	sub.l d2,d0		;so oft wie mîglich subtrahieren
	bcc int0
	add.l d2,d0
	divu #10,d2		;nÑchste Stelle
	add.b #"0",d1		;Ziffer nach ASCII wandeln
	move.b d1,(a0)+		;und in Puffer schreiben
	dbra d7,int1
	rts

*Dialog mit dem Benutzer
form_do:
	move.l a2,a0
	moveq #FORM_CENTER,d0
	move.l #$00050100,d1
	bsr aesobj
	movem.l 2(a6),a3/a4	;form_xy und form_wh
	clr d2			;FMD_START
	movem.l a3/a4,2(a5)
	movem.l a3/a4,10(a5)
	bsr form_dial
	moveq #1,d2		;sich ausdehnendes Rechteck zeichnen
	move.l #$014000c8,2(a5)	;Startkoordinaten
	move.l #$00140014,6(a5)
	bsr.s fo_dial
	moveq #OBJC_DRAW,d0
	move.l #$06010100,d1
	move.l #$00000002,(a5)
	move.l 2(a6),4(a5)
	move.l 6(a6),8(a5)
	move.l a2,a0
	bsr.s aesobj
	moveq #FORM_DO,d0
	move.l #$01010101,d1
	move #START,(a5)		;Eingabefeld
	move.l a2,a0
	bsr.s aesobj
	move (a6),d7
	moveq #2,d2		;schrumpfendes Rechteck zeichnen
	move.l #$014000c8,2(a5)
	move.l #$00140014,6(a5)
	bsr.s fo_dial
	moveq #3,d2		;FMD_FINISH
	movem.l a3/a4,2(a5)
	bsr.s fo_dial
	moveq #OBJC_CHANGE,d0
	move.l #$08010100,d1
	move.b d7,1(a5)
	clr 2(a5)
	move.l deskxy,4(a5)
	move.l deskbh,8(a5)
	clr.l 12(a5)
	move.l a2,a0
	bra.s aesobj

fo_dial:
	movem.l a3/a4,10(a5)
form_dial:
	moveq #FORM_DIAL,d0
	move.l #$09010100,d1
	move d2,(a5)
	bra.s aes

aesobj:
	move.l a0,addrin

*allgemeiner AES-Aufruf
aes:
	lea contrl,a0
	move d0,(a0)
	movep.l d1,3(a0)
	move.l #aespb,d1
	move #$c8,d0
	trap #2
	rts

load:
	clr.b (a0)		;Ende des Filenamens kennzeichnen
	move #$01,-(sp)
	pea nam
	move #FSFIRST,-(sp)
	trap #GEMDOS		;File suchen
	addq.l #8,sp
	moveq #2,d1
	tst d0			;File vorhanden?
	bne frmerr		;nein-
	move.l dta+26,-(sp)
	addq.l #1,(sp)
	move #MALLOC,-(sp)	;Speicherplatz fÅr File
	trap #GEMDOS		;bereitstellen
	addq.l #6,sp
	tst.l d0 		;genug Speicherplatz vorhanden?
	beq memerr		;nein-
	move.l d0,a3
	clr -(sp)
	pea nam
	move #FOPEN,-(sp)
	trap #GEMDOS		;File îffnen
	addq.l #8,sp
	move.l d0,d7
	bmi ferr
	pea (a3)
	move.l dta+26,-(sp)
	move d7,-(sp)
	move #FREAD,-(sp)
	trap #GEMDOS		;File laden
	lea 12(sp),sp
	tst.l d0
	bmi ferr
	move d7,-(sp)
	move #FCLOSE,-(sp)
	trap #GEMDOS		;so weit, so gut
	addq.l #4,sp
	rts

setpath:
	clr.b (a0)
	cmp.b #':',inpath+1	;Laufwerk angegeben?
	bne.s .nodrv		;nein-
	clr d0
	move.b inpath,d0
	bclr #5,d0
	sub.b #'A',d0
	move d0,-(sp)
	move #DSETDRV,-(sp)
	trap #GEMDOS
	addq.l #4,sp
.nodrv: 	pea inpath
	move #DSETPATH,-(sp)
	trap #GEMDOS
	addq.l #6,sp
	tst d0
	rts

get:
	moveq #0,d0		;Ergebnis lîschen
	clr d1
.loop:	move.b (a4)+,d1		;Ziffer holen
	cmp.b #':',d1
	bcc.s .skip
	sub.b #'0',d1		;in HEX wandeln
	bcs.s .skip
	mulu #10,d0
	add d1,d0
	bra .loop
.skip:	move.l d0,objcnr
	move.l d0,freenr
	rts

	include "RSCCONV.DAT"


	data

typadr: 	dc.l box,text,boxtext,image,userdef,ibox,button,boxchar
	dc.l string,ftext,fboxtext,icon,title,cicon

aespb:	dc.l contrl,global,intin,intout,addrin,addrout

glo:	dc.b $09,"xdef objc",0

glo1:	dc.b $09,"xdef free",0

header: 	dc.b $0d,$0a,$0d,$0a
	dc.b "***********************************",$0d,$0a
	dc.b "* Konvertiert durch RSCCONV V3.01 *",$0d,$0a
	dc.b "* (C) 1993-2023 Uwe Seimet        *",$0d,$0a
	dc.b "***********************************"
	dc.b $0d,$0a,$0d,$0a
	dc.b "*Gesamtzahl an Objekten: "
nobs:	dc.b "0000",$0d,$0a
	dc.b "*ObjektbÑume: "
ntree:	dc.b "0000",$0d,$0a
	dc.b "*Tedinfos: "
nted:	dc.b "0000",$0d,$0a
	dc.b "*Icons: "
nib:	dc.b "0000",$0d,$0a
	dc.b "*Bitblîcke: "
nbb:	dc.b "0000",$0d,$0a,$0d,$0a
	dc.b "*freie Strings: "
nstring: dc.b "0000",$0d,$0a
	dc.b "*freie Images: "
nimages: dc.b "0000",$0d,$0a,$0d,$0a
	dc.b "_treenr"
tnr:	dc.b '0'," equ "
tnobj:	dc.b "0000",$0d,$0a
	dc.b "_objcnr"
onr:	dc.b '0'," equ "
inobj:	dc.b "0000",$0d,$0a

tobs2:	dc.b $09,"xdef _treenr"
tobs3:	dc.b '0',$0d,$0a
	dc.b $09,"xdef _objcnr"
nobs3:	dc.b '0'
endcr:	dc.b $0d,$0a,$0d,$0a
txt0:	dc.b "_G_BOX      = 20",$0d,$0a
	dc.b "_G_TEXT     = 21",$0d,$0a
	dc.b "_G_BOXTEXT  = 22",$0d,$0a
	dc.b "_G_IMAGE    = 23",$0d,$0a
	dc.b "_G_USERDEF  = 24",$0d,$0a
	dc.b "_G_IBOX     = 25",$0d,$0a
	dc.b "_G_BUTTON   = 26",$0d,$0a
	dc.b "_G_BOXCHAR  = 27",$0d,$0a
	dc.b "_G_STRING   = 28",$0d,$0a
	dc.b "_G_FTEXT    = 29",$0d,$0a
	dc.b "_G_FBOXTEXT = 30",$0d,$0a
	dc.b "_G_ICON     = 31",$0d,$0a
	dc.b "_G_TITLE    = 32",$0d,$0a
	dc.b "_G_CICON    = 33",0

dat:	dc.b $09,"data",$0d,$0a,$0d,$0a,0

typ20:	dc.b "_G_BOX",0
typ21:	dc.b "_G_TEXT",0
typ22:	dc.b "_G_BOXTEXT",0
typ23:	dc.b "_G_IMAGE",0
typ24:	dc.b "_G_USERDEF",0
typ25:	dc.b "_G_IBOX",0
typ26:	dc.b "_G_BUTTON ",0
typ27:	dc.b "_G_BOXCHAR",0
typ28:	dc.b "_G_STRING ",0
typ29:	dc.b "_G_FTEXT",0
typ30:	dc.b "_G_FBOXTEXT",0
typ31:	dc.b "_G_ICON",0
typ32:	dc.b "_G_TITLE",0
typ33:	dc.b "_G_CICON",0

opal:	dc.b "[3][Fehler beim Schreiben|der Ausgabedateien.][Abbruch]",0

alert:	dc.b "UngÅltige Parameterliste",$0d,$0a,0

wrerr:	dc.b "Fehler beim Schreiben der Ausgabedateien",$0d,$0a,0

	even

nradr:	dc.l nobs,ntree,nted,nib,nbb,nstring,nimages

g_adr:	dc.l typ20,typ21,typ22,typ23,typ24,typ25,typ26
	dc.l typ27,typ28,typ29,typ30,typ31,typ32,typ33


*--------------------------------------------------------------------------*


	bss

contrl: 	ds.w 11

global: 	ds.w 15

intin:	ds.w 64

intout: 	ds.w 64

addrin: 	ds.w 64

addrout:	ds.w 64

tmp:	ds.l 1

deskxy: 	ds.l 1

deskbh: 	ds.l 1

objcnr: 	ds.l 1		;ZÑhler fÅr Åbergeordnetet Objekte

specnr: 	ds.l 1		;ZÑhler fÅr ob_spec

freenr: 	ds.l 1		;ZÑhler fÅr freie Strings

usernr: 	ds.l 1		;ZÑhler fÅr benutzerdefinierte Objekte

parmnr:	ds.l 1

typpnt:	ds.l 1

iconsize:ds.w 2

texth:	ds.w 1		;File-

datah:	ds.w 1		;Handles

dta:	ds.b 44

	even

inpath: 	ds.b 80

nam:	ds.b 13

insel:	ds.b 13

	even

globflg:	ds.b 1		;Flag fÅr globale Labels

datflg: 	ds.b 1		;Flag fÅr DATA-Segment

userflg:	ds.b 1		;Flag fÅr Buttons als USERDEF auswerten

extflg:	ds.b 1		;Flag fÅr langes RSC-Format von INTERFACE

parflg: 	ds.b 1		;Flag fÅr Parameter in Kommandozeile

namflg: 	ds.b 1		;Flag fÅr vorhandenen Dateinamen

txtflg: 	ds.b 1		;Flag fÅr Kommentarstring

	align.l

stack:	ds.l 100 	;Platz fÅr Stack

bsstext:ds.b 16384	;zum Zwischenspeichern der Objektdaten

bssdata:ds.b 65536	;mit Sicherheit ausreichend
