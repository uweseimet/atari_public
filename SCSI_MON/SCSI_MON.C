/****************************/
/* SCSI_MON 1.33 Beta       */
/*                          */
/* (C) 1999-2019 Uwe Seimet */
/****************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <scsidrv/scsidefs.h>
#include "../modules/modstart.h"


#define ERROR -1
#define EDRVNR -2
#define EUNDEV -15
#define ENHNDL -35
#define EINVHNDL -37
#define EACCDN -36


typedef int bool;
#define true TRUE
#define false FALSE


#define BYTECOUNT 8


typedef LONG cdecl (*cookieVal)(UWORD opcode,...);


LONG cdecl In(tpSCSICmd);
LONG cdecl Out(tpSCSICmd);
LONG cdecl InquireSCSI(WORD, tBusInfo *);
LONG cdecl InquireBus(WORD, WORD, tDevInfo *);
LONG cdecl CheckDev(WORD, const DLONG *, char *, UWORD *);
LONG cdecl RescanBus(WORD);
LONG cdecl Open(WORD, const DLONG *, ULONG *);
LONG cdecl Close(tHandle);
LONG cdecl Error(tHandle, WORD, WORD);

LONG cdecl Install(WORD, tpTargetHandler);
LONG cdecl Deinstall(WORD, tpTargetHandler);
LONG cdecl GetCmd(WORD, BYTE *);
LONG cdecl SendData(WORD, BYTE *, ULONG);
LONG cdecl GetData(WORD, void *, ULONG);
LONG cdecl SendStatus(WORD, UWORD);
LONG cdecl SendMsg(WORD, UWORD);
LONG cdecl GetMsg(WORD, UWORD *);

WORD cdecl TSel(WORD, UWORD, UWORD);
WORD cdecl TCmd(WORD, BYTE *);
UWORD cdecl TCmdLen(WORD, UWORD);
void cdecl TReset(UWORD bus);
void cdecl TEOP(UWORD bus);
void cdecl TPErr(UWORD bus);
void cdecl TPMism(UWORD bus);
void cdecl TBLoss(UWORD bus);
void cdecl TUnknownInt(UWORD bus);


tpScsiCall scsiCall;
tScsiCall oldScsiCall;
tScsiCall myScsiCall = {
	0x0000,
	In,
	Out,
	InquireSCSI,
	InquireBus,
	CheckDev,
	RescanBus,
	Open,
	Close,
	Error,
	Install /*,
	Deinstall,
	GetCmd,
	SendData,
	GetData,
	SendStatus,
	SendMsg,
	GetMsg*/
};

tTargetHandler targetHandler =
	{ NULL, TSel, TCmd, TCmdLen, TReset, TEOP, TPErr, TPMism, TBLoss,
		TUnknownInt };


typedef struct {
	tTargetHandler *handler;
	UWORD len;
} HandlerInfo;

HandlerInfo handlerInfo[32];

bool con = false;
bool aux = false;
bool prt = false;


typedef void (*out)(WORD);
extern void conout(WORD);
extern void auxout(WORD);
extern void prtout(WORD);


int terminate(const char *);
void installHandler(void);
void prres(tpSCSICmd, LONG);
void prerr(LONG);
void prparms(tpSCSICmd, bool);
void prbuffer(tpSCSICmd, bool);
LONG result(LONG);
void sysprintf(const char *);
static int getcookie(long, ULONG *p);


int
main(WORD argc, const char *argv[])
{
	if(isHddriverModule()) {
		aux = true;
	}

	if(argc > 1) {
		con = !strcmp(argv[1], "--con");
		aux = !strcmp(argv[1], "--aux");
		prt = !strcmp(argv[1], "--prt");

		if(!con && !aux && !prt) {
			return terminate("\nIllegal output channel argument, SCSI_MON not installed");
		}
	}

	if(!getcookie('SCSI', (ULONG *)&scsiCall)) {
		return terminate("\nSCSI Driver not found, SCSI_MON not installed");
	}

	memcpy(&oldScsiCall, scsiCall, 38);
	myScsiCall.Version = scsiCall->Version;
	memcpy(scsiCall, &myScsiCall, 38);

	memset(handlerInfo, 0, sizeof(handlerInfo));

	printf("\n\x1b\x70SCSI_MON V1.33 Beta\x1b\x71");
	printf("\n½ 1999-2019 Uwe Seimet\n");

	Ptermres(_PgmSize, 0);

	return 0;
}


int
terminate(const char *errorMessage)
{
	printf(errorMessage);

	if(isHddriverModule()) {
		Pterm(-1);
	}

	return -1;
}


void
installHandler()
{
	tBusInfo busInfo;
	LONG result;

	result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!result) {
		if(busInfo.Features & cTarget) {
			scsiCall->Install(busInfo.BusNo, &targetHandler);
		}

		result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}
}


LONG cdecl
In(tpSCSICmd parms)
{
	LONG res;
	char str[256];

	sprintf(str, "In    tpSCSICmd $%p", parms);
	sysprintf(str);
	prparms(parms, false);

	res = oldScsiCall.In(parms);

	if(res <= 0 || res == 2) {
		prres(parms, res);
	}
	else {
		sprintf(str, "-> %ld", res);
		sysprintf(str);
	}
	prbuffer(parms, false);

	return res;
}


LONG cdecl
Out(tpSCSICmd parms)
{
	LONG res;
	char str[256];

	sprintf(str, "Out    tpSCSICmd $%p", parms);
	sysprintf(str);
	prparms(parms, true);

	res = oldScsiCall.Out(parms);

	if(res <= 0 || res == 2) {
		prres(parms, res);
	}
	else {
		sprintf(str, "-> %ld", res);
		sysprintf(str);
	}

	return res;
}


LONG cdecl
InquireSCSI(WORD what, tBusInfo *info)
{
	LONG res;
	char str[256];

	sprintf(str, "InquireSCSI    what %s", what ? "cInqNext" : "cInqFirst");
	sysprintf(str);
	res = oldScsiCall.InquireSCSI(what, info);

	if(res < 0) {
		prerr(res);
	}
	else {
		char s[32];

		sprintf(str, "-> BusName \"%s\"  BusNo %d  Features",
			info->BusName, info->BusNo);
		if(!(info->Features & 0x3f)) {
			char s[8];

			sprintf(s, " %d", info->Features);
			strcat(str, s);
		}
		if(info->Features & 0x01) strcat(str, " cArbit");
		if(info->Features & 0x02) strcat(str, " cAllCmds");
		if(info->Features & 0x04) strcat(str, " cTargCtrl");
		if(info->Features & 0x08) strcat(str, " cTarget");
		if(info->Features & 0x10) strcat(str, " cCanDisconnect");
		if(info->Features & 0x20) strcat(str, " cScatterGather");
		sprintf(s, "  MaxLen %ld", info->MaxLen);
		strcat(str, s);
		sysprintf(str);
	}

	return res;
}


LONG cdecl
InquireBus(WORD what, WORD busno, tDevInfo *info)
{
	LONG res;
	char str[256];

	sprintf(str, "InquireBus    what %s  BusNo %d",
		what ? "cInqNext" : "cInqFirst", busno);
	sysprintf(str);
	res = oldScsiCall.InquireBus(what, busno, info);

	if(res < 0) {
		prerr(res);
	}
	else {
		sprintf(str, "-> SCSIId { %ld, %ld }", info->SCSIId.hi, info->SCSIId.lo);
		sysprintf(str);
	}

	return res;
}


LONG cdecl
CheckDev(WORD busno, const DLONG *id, char *name, UWORD *features)
{
	LONG res;
	char str[256];

	sprintf(str, "CheckDev    BusNo %d  SCSIId { %ld, %ld }", busno,
		id->hi, id->lo);
	sysprintf(str);

	res = oldScsiCall.CheckDev(busno, id, name, features);

	if(res < 0) {
		prerr(res);
	}
	else {
		sprintf(str, "-> %ld  Name \"%s\"  Features", res, name);
		if(!(*features & 0x3f)) {
			char s[8];

			sprintf(s, " %d", *features);
			strcat(str, s);
		}
		if(*features & 0x01) strcat(str, " cArbit");
		if(*features & 0x02) strcat(str, " cAllCmds");
		if(*features & 0x04) strcat(str, " cTargCtrl");
		if(*features & 0x08) strcat(str, " cTarget");
		if(*features & 0x10) strcat(str, " cCanDisconnect");
		if(*features & 0x20) strcat(str, " cScatterGather");

		sysprintf(str);
	}

	return res;
}


LONG cdecl
RescanBus(WORD busno)
{
	char str[256];

	sprintf(str, "RescanBus    BusNo %d", busno);
	sysprintf(str);
	return result(oldScsiCall.RescanBus(busno));
}


LONG cdecl
Open(WORD busno, const DLONG *id, ULONG *maxlen)
{
	LONG res;
	char str[256];

	sprintf(str, "Open    BusNo %d  SCSIId { %ld, %ld }", busno,
		id->hi, id->lo);
	sysprintf(str);

	res = oldScsiCall.Open(busno, id, maxlen);

	if(res <= 0) {
		prerr(res);
	}
	else {
		tHandle handle = (tHandle)res;
		char s[32];

		sprintf(str, "-> Handle $%p  Features", handle);
		if(!(*handle & 0x3f)) {

			sprintf(s, " %d", *handle);
			strcat(str, s);
		}
		if(*handle & 0x01) strcat(str, " cArbit");
		if(*handle & 0x02) strcat(str, " cAllCmds");
		if(*handle & 0x04) strcat(str, " cTargCtrl");
		if(*handle & 0x08) strcat(str, " cTarget");
		if(*handle & 0x10) strcat(str, " cCanDisconnect");
		if(*handle & 0x20) strcat(str, " cScatterGather");
		sprintf(s, "  MaxLen %ld", *maxlen);
		strcat(str, s);
		sysprintf(str);
	}

	return res;
}


LONG cdecl
Close(tHandle handle)
{
	LONG res;
	char str[256];

	sprintf(str, "Close    Handle $%p", handle);
	sysprintf(str);

	res = oldScsiCall.Close(handle);
	prerr(res);

	return res;
}


LONG cdecl
Error(tHandle handle, WORD rwflag, WORD errno)
{
	LONG res;
	char str[256];

	if(rwflag) {
		sprintf(str, "Error    Handle $%p  rwflag cErrWrite  ErrNo", handle);
		if(!(errno & 0x03)) {
			char s[8];

			sprintf(s, " $%04X", errno);
			strcat(str, s);
		}
		if(errno & 0x01) strcat(str, " cErrMediach");
		if(errno & 0x02) strcat(str, " cErrReset");
	}
	else
		sprintf(str, "Error    Handle $%p  rwflag cErrRead", handle);
	sysprintf(str);

	res = oldScsiCall.Error(handle, rwflag, errno);
	sprintf(str, "-> %ld ", res);
	if(res & 0x01) strcat(str, " cErrMediach");
	if(res & 0x02) strcat(str, " cErrReset");
	sysprintf(str);

	return res;
}


LONG cdecl
Install(WORD bus, tpTargetHandler handler)
{
	char str[256];

	sprintf(str, "Install    Bus %d  Handler %p", bus, handler);
	sysprintf(str);

	return oldScsiCall.Install(bus, handler);
}


LONG cdecl
Deinstall(WORD bus, tpTargetHandler handler)
{
	char str[256];

	sprintf(str, "Deinstall    Bus %d  Handler %p", bus, handler);
	sysprintf(str);

	return oldScsiCall.Deinstall(bus, handler);
}


LONG cdecl
GetCmd(WORD bus, BYTE *Cmd)
{
	return oldScsiCall.GetCmd(bus, Cmd);
}


LONG cdecl
SendData(WORD bus, BYTE *Buffer, ULONG Len)
{
	return oldScsiCall.SendData(bus, Buffer, Len);
}


LONG cdecl
GetData(WORD bus, void *Buffer, ULONG Len)
{
	return oldScsiCall.GetData(bus, Buffer, Len);
}


LONG cdecl
SendStatus(WORD bus, UWORD Status)
{
	char str[256];

	sprintf(str, "SendStatus    Bus %d  Status %d", bus, Status);
	sysprintf(str);

	return oldScsiCall.SendStatus(bus, Status);
}


LONG cdecl
SendMsg(WORD bus, UWORD Msg)
{
	char str[256];

	sprintf(str, "SendMsg    Bus %d  Msg %d", bus, Msg);
	sysprintf(str);

	return oldScsiCall.SendMsg(bus, Msg);
}


LONG cdecl
GetMsg(WORD bus, UWORD *Msg)
{
	return oldScsiCall.GetMsg(bus, Msg);
}


#pragma warn -par
WORD cdecl
TSel(WORD bus, UWORD CSB, UWORD CSD)
{
	/* Always assume selection */
	return !(CSB & 0x04);
}
#pragma warn .par


WORD cdecl
TCmd(WORD bus, BYTE *cmd)
{
	tTargetHandler *handler;
	int len;
	int i;
	char str[256];

	len = handlerInfo[bus].len;
	if(!len) {
		int c = ((UBYTE *)cmd)[0];
		switch(c >> 5) {
			case 0:		len = 6;
								break;
			case 4:		len = 16;
								break;
			case 5:		len = 12;
								break;
			case 1:
			case 2:		len = 10;
								break;
			default:	len = 12;
								break;
		}
	}

	sprintf(str, "TCmd    BusNo %d", bus);
	sysprintf(str);

	if(len) {
		strcpy(str, "  Cmd $");
		for(i = 0; i < len; i++) {
			char s[8];

			if(!i) sprintf(s, "%02X", cmd[i] & 0xff);
			else sprintf(s, ":%02X", cmd[i] & 0xff);
			strcat(str, s);
		}
		sysprintf(str);
	}

	handler = handlerInfo[bus].handler;

	handlerInfo[bus].handler = NULL;
	handlerInfo[bus].len = 0;

	if(handler) return handler->TCmd(bus, cmd);		
	else return false;
}


UWORD cdecl
TCmdLen(WORD bus, UWORD cmd)
{
	tTargetHandler *handler;
	char str[256];

	handlerInfo[bus].len = 0;
	handlerInfo[bus].handler = NULL;

	handler = targetHandler.next;
	while(handler) {
		if(handler->TCmdLen)
			handlerInfo[bus].len = handler->TCmdLen(bus, cmd);
		if(handlerInfo[bus].len) {
			handlerInfo[bus].handler = handler;
			break;
		}
	}

	if(handlerInfo[bus].len) {
		sprintf(str, "TCmdLen    BusNo %d  Cmd $%02X -> Len %d", bus, cmd,
			handlerInfo[bus].len);
		sysprintf(str);
	}

	return handlerInfo[bus].len;
}


void cdecl
TReset(UWORD bus)
{
	char str[256];

	sprintf(str, "TReset    BusNo %d", bus);
	sysprintf(str);
}


void cdecl
TEOP(UWORD bus)
{
	char str[256];

	sprintf(str, "TEOP    BusNo %d", bus);
	sysprintf(str);
}


void cdecl
TPErr(UWORD bus)
{
	char str[256];

	sprintf(str, "TPErr    BusNo %d", bus);
	sysprintf(str);
}


void cdecl
TPMism(UWORD bus)
{
	char str[256];

	sprintf(str, "TPMism    BusNo %d", bus);
	sysprintf(str);
}


void cdecl
TBLoss(UWORD bus)
{
	char str[256];

	sprintf(str, "TBLoss    BusNo %d", bus);
	sysprintf(str);
}


void cdecl
TUnknownInt(UWORD bus)
{
	char str[256];

	sprintf(str, "TUnknownInt    BusNo %d", bus);
	sysprintf(str);
}


void
sysprintf(const char *s)
{
	if(!con && !aux && !prt) {
		gemdos(0x1069, s);
	}
	else {
		size_t i;
		size_t len;
	  out channel;

		if(con) channel = conout;
		else if(aux) channel = auxout;
		else channel = prtout;

		len = strlen(s);
		for(i = 0; i < len; i++) {
			channel(s[i]);
		}

		channel(0x0d);
		channel(0x0a);
	}
}


void
prres(tpSCSICmd parms, LONG res)
{
	char str[256];

	strcpy(str, "-> ");

	switch((WORD)res) {
		case SELECTERROR:
			strcat(str, "SELECTERROR");
			break;
		case STATUSERROR:
			strcat(str, "STATUSERROR");
			break;
		case PHASEERROR:
			strcat(str, "PHASEERROR");
			break;
		case BSYERROR:
			strcat(str, "BSYERROR");
			break;
		case BUSERROR:
			strcat(str, "BUSERROR");
			break;
		case TRANSERROR:
			strcat(str, "TRANSERROR");
			break;
		case FREEERROR:
			strcat(str, "FREEERROR");
			break;
		case TIMEOUTERROR:
			strcat(str, "TIMEOUTERROR");
			break;
		case DATATOOLONG:
			strcat(str, "DATATOOLONG");
			break;
		case LINKERROR:
			strcat(str, "LINKERROR");
			break;
		case TIMEOUTARBIT:
			strcat(str, "TIMEOUTARBIT");
			break;
		case PENDINGERROR:
			strcat(str, "PENDINGERROR");
			break;
		case PARITYERROR:
			strcat(str, "PARITYERROR");
			break;
		case 0x00:
			strcat(str, "GOOD");
			break;
		case 0x02:
			if(parms->SenseBuffer) {
				char s[64];

				sprintf(s, "CHECK CONDITION  SenseKey $%02X  ASC $%02X  ASCQ $%02X",
					parms->SenseBuffer[2] & 0x0f, parms->SenseBuffer[12] & 0xff,
					parms->SenseBuffer[13] & 0xff);
				strcat(str, s);
			}
			else
				strcat(str, "CHECK CONDITION");
			break;
		case 0x04:
			strcat(str, "CONDITION MET");
			break;
		case 0x08:
			strcat(str, "BUSY");
			break;
		case 0x10:
			strcat(str, "INTERMEDIATE");
			break;
		case 0x14:
			strcat(str, "INTERMEDIATE - CONDITION MET");
			break;
		case 0x18:
			strcat(str, "RESERVATION CONFLICT");
			break;
		case 0x22:
			strcat(str, "COMMAND TERMINATED");
			break;
		case 0x28:
			strcat(str, "TASK SET FULL");
			break;
		case 0x30:
			strcat(str, "ACA ACTIVE");
			break;
		case 0x40:
			strcat(str, "TASK ABORTED");
			break;
		default:
			sprintf(str, "-> %ld", res);
			break;
	}

	sysprintf(str);
}


void
prerr(LONG res)
{
	char str[256];

	strcpy(str, "-> ");

	switch((WORD)res) {
		case ERROR:
			strcat(str, "ERROR");
			break;
		case EDRVNR:
			strcat(str, "EDRVNR");
			break;
		case EUNDEV:
			strcat(str, "EUNDEV");
			break;
		case EACCDN:
			strcat(str, "EACCDN");
			break;
		case ENHNDL:
			strcat(str, "ENHNDL");
			break;
		case EINVHNDL:
			strcat(str, "EINVHNDL");
			break;
		default:
			sprintf(str, "-> %ld", res);
			break;
	}

	sysprintf(str);
}


void
prparms(tpSCSICmd parms, bool isOut)
{
	int i;
	char str[256];

	sprintf(str, "  Handle $%p", parms->Handle);
	sysprintf(str);

	sprintf(str, "  CmdLen %d  Cmd $", parms->CmdLen);
	for(i = 0; i < parms->CmdLen; i++) {
		char s[8];

		if(!i) sprintf(s, "%02X", parms->Cmd[i] & 0xff);
		else sprintf(s, ":%02X", parms->Cmd[i] & 0xff);
		strcat(str, s);

		if(i >= 16) {
			strcat(str, "...");
			break;
		}
	}
	sysprintf(str);		

	sprintf(str, "  TransferLen %ld  Buffer $%p", parms->TransferLen,
		parms->Buffer);
	sysprintf(str);

	if(isOut) {
		prbuffer(parms, isOut);
	}

	sprintf(str, "  SenseBuffer $%p  Timeout %ld", parms->SenseBuffer,
		parms->Timeout);
	sysprintf(str);

	if(parms->Flags & 0x10) {
		sprintf(str, "  Flags Disconnect|$%02X", parms->Flags & 0xef);
	}
	else {
		sprintf(str, "  Flags $%02X", parms->Flags);
	}

	sysprintf(str);
}


void
prbuffer(tpSCSICmd parms, bool isOut)
{
	int i;
	char str[256];

	if(!parms->TransferLen) {
		return;
	}

	/* Print parameter lists or returned data for selected commands */
	switch(parms->Cmd[0]) {
		/* REQUEST SENSE */
		case 0x03:
		/* FORMAT */
		case 0x04:
		/* INQUIRY */
		case 0x12:
		/* MODE SELECT (6) */
		case 0x15:
		/* MODE SENSE (6) */
		case 0x1a:
		/* SEND DIAGNOSTIC */
		case 0x1d:
		/* READ FORMAT CAPACITIES */
		case 0x23:
		/* READ CAPACITY (10) */
		case 0x25:
		/* GET CONFIGURATION */
		case 0x46:
		/* LOG SELECT */
		case 0x4c:
		/* LOG SENSE */
		case 0x4d:
		/* MODE SELECT (10) */
		case 0x55:
		/* MODE SENSE (10) */
		case 0x5a:
		/* READ CAPACITY (16) */
		case 0x9e:
		/* REPORT LUNS */
		case 0xa0:
			break;

		default:
			return;
	}

	strcpy(str, isOut ? "  Parameters $" : "  Result $");

	for(i = 0; i < parms->TransferLen; i++) {
		char s[3];

		if(i && !(i % BYTECOUNT)) {
			strcpy(str, isOut ? "             $" : "          ");
		}

		if(i % BYTECOUNT) {
			strcat(str, ":");
		}

		sprintf(s, "%02X", ((UBYTE *)parms->Buffer)[i]);	
		strcat(str, s);

		if(!((i + 1) % BYTECOUNT)) {
			sysprintf(str);
		}
	}

	if(i % BYTECOUNT) {
		sysprintf(str);
	}
}


LONG
result(LONG res)
{
	char str[256];

	sprintf(str, "-> %ld", res);
	sysprintf(str);

	return res;
}


static long
cookieptr(void)
{
	return *((long *)0x5a0);
}


static int
getcookie(long cookie, ULONG *p_value)
{
	long *cookiejar = (long *)Supexec(cookieptr);

	if(!cookiejar) return 0;

	do {
		if(cookiejar[0] == cookie) {
			if(p_value) *p_value = cookiejar[1];
			return 1;
		}
		else
			cookiejar = &(cookiejar[2]);
	} while (cookiejar[-2]);

	return 0;
}
