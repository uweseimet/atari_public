/******************************************/
/* SCSI Driver for Hatari and ARAnyM 1.23 */
/*                                        */
/* (C) 2016-2025 Uwe Seimet               */
/******************************************/

#define VERSION "1.23"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <scsidrv/scsidefs.h>
#include "../modules/modstart.h"
#include "util.h"


#define EUNDEV -15L
#define ENHNDL -35L


typedef int bool;
#define true TRUE
#define false FALSE

#define INTERFACE_VERSION 0x0102

#define SCSI_MAX_HANDLES 32

LONG cdecl In(tpSCSICmd);
LONG cdecl Out(tpSCSICmd);
LONG cdecl InquireSCSI(WORD, tBusInfo *);
LONG cdecl InquireBus(WORD, WORD, tDevInfo *);
LONG cdecl CheckDev(WORD, const DLONG *, char *, UWORD *);
LONG cdecl RescanBus(WORD);
LONG cdecl Open(WORD, const DLONG *, ULONG *);
LONG cdecl Close(tHandle);
LONG cdecl Error(tHandle, WORD, WORD);


/* Bus characteristics */

UWORD busA = 30;
UWORD busB;
UWORD drvBusFeatures;
ULONG drvBusTransferLen;
static const char *BUS_NAME = "Linux SCSI (Bus %c)";


LONG id;

typedef struct {
	char version[4];
} UsSc;

UsSc usscCookie = {
	VERSION
};

typedef struct {
	UWORD features;
	ULONG transferLen;
	bool valid;
} DrvHandle;

DrvHandle handles[SCSI_MAX_HANDLES];

tpScsiCall scsiCall;
tScsiCall oldScsiCall;
tScsiCall myScsiCall = {
	0x0101,
	In,
	Out,
	InquireSCSI,
	InquireBus,
	CheckDev,
	RescanBus,
	Open,
	Close,
	Error
};


enum SCSIDRV_OPERATIONS {
	SCSI_INTERFACE_VERSION,
	SCSI_INTERFACE_FEATURES,
	SCSI_INQUIRE_BUS,
	SCSI_OPEN,
	SCSI_CLOSE,
	SCSI_INOUT,
	SCSI_ERROR,
	SCSI_CHECK_DEV
};


int terminate(void);
void installHandler(void);
LONG inout(tpSCSICmd, UWORD);
LONG cookieptr(void);
void addcookie(long, ULONG p);


int
main(WORD argc, const char *argv[])
{
	ULONG ussc;
	UWORD interfaceVersion;
	char drvBusName[20];
	LONG oldstack = 0;

	if(!Super((void *)1L)) {
		oldstack = Super(0L);
	}

	if(getCookie('USSC', &ussc)) {
		printf("\nSCSI Driver for Hatari and ARAnyM "
			"already installed\n");
		return terminate();
	}

	if(!nfDetect()) {
		printf("\nNative features not supported,\n"
			"SCSI Driver for Hatari and ARAnyM not installed\n");
		return terminate();
	}

	id = nfId("NF_SCSIDRV");
	if(!id) {
		printf("\nNative SCSI driver feature not supported,\n"
			"SCSI Driver for Hatari and ARAnyM not installed\n");
		return terminate();
	}

	interfaceVersion = (UWORD)nfCall(id | SCSI_INTERFACE_VERSION);
	if(interfaceVersion != INTERFACE_VERSION) {
		printf("\nSCSI Driver interface version mismatch,\n"
			"native version is %d.%02d, local version is %d.%02d\n"
			"SCSI Driver for Hatari and ARAnyM not installed\n",
			interfaceVersion >> 8, interfaceVersion & 0xff,
			INTERFACE_VERSION >> 8, INTERFACE_VERSION & 0xff);
		return terminate();
	}

	if(argc > 1) {
		busA = atoi(argv[1]);
		if(busA > 30) {
			printf("\nIllegal first bus ID %d, maximum is 30\n"
			"SCSI Driver for Hatari and ARAnyM not installed\n", busA);
			return terminate();
		}
	}
		
	busB = busA + 1;

	/* The bus name returned is ignored on purpose, because this name
	   might be too long to add the "(Bus x)" suffix. */
	nfCall(id | SCSI_INTERFACE_FEATURES, drvBusName, &drvBusFeatures,
		&drvBusTransferLen);

	if(!getCookie('SCSI', &scsiCall)) {
		setCookie('SCSI', (ULONG)&myScsiCall);
	}
	else {
		memcpy(&oldScsiCall, scsiCall, sizeof(tScsiCall));
		myScsiCall.Version = scsiCall->Version;
		/* Only copy the initiator vectors */
		memcpy(scsiCall, &myScsiCall, 2 + 4 * 9);
	}

	setCookie('USSC', (ULONG)&usscCookie);

	if(oldstack) {
		Super((void *)oldstack);
	}

	printf("\n\x1b\x70SCSI Driver for Hatari and ARAnyM V" VERSION
		"\x1b\x71\n");
	printf("� 2016-2025 Uwe Seimet\n");

	Ptermres(_PgmSize, 0);

	return 0;
}


int
terminate()
{
	if(isHddriverModule()) {
		Pterm(-1);
	}

	return -1;
}


void
installHandler()
{
	tBusInfo busInfo;

	LONG result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!result) {
		result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}
}


LONG cdecl
In(tpSCSICmd parms)
{
	return inout(parms, 0);
}


LONG cdecl
Out(tpSCSICmd parms)
{
	return inout(parms, 1);
}


LONG
inout(tpSCSICmd parms, UWORD dir) 
{
	DrvHandle *h = (DrvHandle *)parms->Handle;

	if(h < &handles[0] || h >= &handles[SCSI_MAX_HANDLES]) {
		if(dir) {
			return oldScsiCall.Out ? oldScsiCall.Out(parms) :
				ENHNDL;
		}
		else {
			return oldScsiCall.In ? oldScsiCall.In(parms) : ENHNDL;
		}
	}
	
	if(!h->valid) {
		return ENHNDL;
	}

	if(parms->TransferLen > h->transferLen) {
		return DATATOOLONG;
	}

	return nfCall(id | SCSI_INOUT, h - &handles[0], (LONG)dir,
		parms->Cmd, (LONG)parms->CmdLen, parms->Buffer,
		parms->TransferLen, parms->SenseBuffer, parms->Timeout);
}


LONG cdecl
InquireSCSI(WORD what, tBusInfo *info)
{
	if(!what) {
		info->Private.BusIds = 0L;
	}

	info->BusNo = info->Private.BusIds & (1L << busA) ? busB : busA;

	if(info->Private.BusIds & (1L << info->BusNo)) {
		return oldScsiCall.InquireSCSI ?
			oldScsiCall.InquireSCSI(what, info) : EUNDEV;
	}
	
	info->Private.BusIds |= 1L << info->BusNo;
	info->Features = drvBusFeatures;
	info->MaxLen = drvBusTransferLen;
	sprintf(info->BusName, BUS_NAME, info->BusNo - busA + 'A');

	return 0;
}


LONG cdecl
InquireBus(WORD what, WORD busno, tDevInfo *info)
{
	LONG result;
	ULONG *nextId = (ULONG *)info->Private;

	if(busno != busA && busno != busB) {
		return oldScsiCall.InquireBus ?
			oldScsiCall.InquireBus(what, busno, info) : EUNDEV;
	}

	if(!what) {
		info->SCSIId.hi = 0;
		nextId[0] = (busno - busA) * 8;
	}

	result = nfCall(id | SCSI_INQUIRE_BUS, *nextId);
	if(result >= 0) {
		info->SCSIId.lo = result - (busno - busA) * 8;

		*nextId = ++result;

		return info->SCSIId.lo > 7 ? -1 : 0;
	}

	return result;
}


LONG cdecl
CheckDev(WORD busno, const DLONG *scsiid, char *name, UWORD *features)
{
	if(busno != busA && busno != busB) {
		return oldScsiCall.CheckDev ?
			oldScsiCall.CheckDev(busno, scsiid, name, features) :
				EUNDEV;
	}

	if(scsiid->hi || nfCall(id | SCSI_CHECK_DEV,
		(busno - busA) * 8 + scsiid->lo)) {
		return EUNDEV;
	}

	sprintf(name, BUS_NAME, busno - busA + 'A');
	*features = drvBusFeatures;
		
	return 0;
}


LONG cdecl
RescanBus(WORD busno)
{
	if(busno != busA && busno != busB) {
		return oldScsiCall.RescanBus ? oldScsiCall.RescanBus(busno) :
			EUNDEV;
	}

	/* Nothing to do for the Linux bus */
	return 0;
}


LONG cdecl
Open(WORD busno, const DLONG *scsiid, ULONG *maxlen)
{
	int i;

	if(busno != busA && busno != busB) {
		return oldScsiCall.Open ?
			oldScsiCall.Open(busno, scsiid, maxlen) : EUNDEV;
	}

	if(scsiid->hi) {
		return EUNDEV;
	}

	for(i = 0; i < SCSI_MAX_HANDLES; i++) {
		if(!handles[i].valid) {
			break;
		}
	}

	if(i == SCSI_MAX_HANDLES) {
		return ENHNDL;
	}

	if(!nfCall(id | SCSI_OPEN, (LONG)i,
		(busno - busA) * 8 + scsiid->lo)) {
		handles[i].features = drvBusFeatures;
		handles[i].transferLen = drvBusTransferLen;
		handles[i].valid = true;

		*maxlen = drvBusTransferLen;

		return (LONG)&handles[i];
	}

	return ENHNDL;
}


LONG cdecl
Close(tHandle handle)
{
	DrvHandle *h = (DrvHandle *)handle;

	if(h < &handles[0] || h >= &handles[SCSI_MAX_HANDLES]) {
		return oldScsiCall.Close ? oldScsiCall.Close(handle) : ENHNDL;
	}

	if(!h->valid) {
		return ENHNDL;
	}

	h->valid = false;

	return nfCall(id | SCSI_CLOSE, h - &handles[0]);
}


LONG cdecl
Error(tHandle handle, WORD rwflag, WORD errno)
{
	DrvHandle *h = (DrvHandle *)handle;

	if(h < &handles[0] || h >= &handles[SCSI_MAX_HANDLES]) {
		return oldScsiCall.Error ?
			oldScsiCall.Error(handle, rwflag, errno) : ENHNDL;
	}

	if(!h->valid) {
		return ENHNDL;
	}

	return nfCall(id | SCSI_ERROR, h - &handles[0], (LONG)rwflag,
		(LONG)errno);
}