/*****************************/
/* SCSI Driver/Firmware Test */
/*                           */
/* (C) 2014-2026 Uwe Seimet  */
/*****************************/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tos.h>
#include <scsi3.h>
#include <scsidrv/scsidefs.h>
#include "std.h"
#include "util.h"


WORD SortBuses(const void *, const void *);


tpScsiCall
GetScsiDriver(const char *msg)
{
	tpScsiCall scsiCall;

	printf("%s\n", msg);
	printf("½ 2021-2026 Uwe Seimet\n\n");

	getCookie('SCSI', (ULONG *)&scsiCall);
	if(scsiCall) {
		printf("Found SCSI Driver version %d.%02d\n\n", scsiCall->Version >> 8,
			scsiCall->Version & 0xff);
	}
	else {
		printf("No SCSI Driver found\n");
	}

	return scsiCall;
}


tHandle
GetHandle(tpScsiCall scsiCall, UWORD *bus, ULONG *device, UWORD *lun)
{
	tBusInfo busInfos[32];
	tBusInfo busInfo;
	bool buses[32];
	DLONG scsiId = { 0, 0 };
	ULONG maxLen;
	UWORD busId;
	UWORD busCount = 0;
	tHandle handle;
	LONG result;
	int s;

	memset(buses, false, sizeof(buses));

	result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!result && busCount < 32) {
		memcpy(&busInfos[busCount], &busInfo, sizeof(tBusInfo));

		buses[busInfo.BusNo] = true;

		busCount++;

		result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}

	qsort(busInfos, busCount, sizeof(tBusInfo), SortBuses);

	for(busId = 0; busId < busCount; busId++) {
		printf("Bus ID: %d, Bus name: '%s'\n", busInfos[busId].BusNo,
		busInfos[busId].BusName);
	}

	if(lun) {
		printf("\nEnter bus ID, device ID and LUN of the device to test (x,y,z): ");
		s = scanf("%d,%ld,%d", bus, &scsiId.lo, lun);
	}
	else {
		printf("\nEnter bus ID and device ID of the device to test (x,y): ");
		s = scanf("%d,%ld", bus, &scsiId.lo);
	}
	printf("\n");

	if(!s) {
		printf("Input format error\n");
		
		Cconin();

		return NULL;
	}

	if(*bus > 31 || !buses[*bus]) {
		printf("Invalid bus ID: %d\n", *bus);
		
		Cconin();

		return NULL;
	}

	if(*lun > 7) {
		printf("Invalid LUN: %d\n", *lun);
		
		Cconin();

		return NULL;
	}

	handle = (tHandle)scsiCall->Open(*bus, &scsiId, &maxLen);
	if(((LONG)handle >> 24) < 0) {
		printf("Device with ID %d.%Ld not found\n", *bus, scsiId.lo);

		Cconin();

		return NULL;
	}

	if(device) {
		*device = scsiId.lo;
	}

	return handle;
}


WORD
SortBuses(const void *b1, const void *b2)
{
	const tBusInfo *i1 = b1;
	const tBusInfo *i2 = b2;

	return i1->BusNo - i2->BusNo;
}


bool
Inquiry(tpScsiCall scsiCall, tpSCSICmd cmd, UWORD lun)
{
	SENSE_BLK Inquiry = {
		0x12, 0x00, 0x00, 0x00, 0x00, (UBYTE)sizeof(INQUIRY_DATA), 0x00, 0x00, 0x00
	};

	LONG status;
	INQUIRY_DATA inquiryData;

	Inquiry.lun = lun;
	cmd->Cmd = (void *)&Inquiry;
	cmd->CmdLen = 6;
	cmd->Buffer = &inquiryData;
	cmd->TransferLen = Inquiry.length;

	memset(&inquiryData, 0, sizeof(INQUIRY_DATA));

	status = scsiCall->In(cmd);
	if(status) {
		printf("INQUIRY failed: %ld\n", status);

		return false;
	}

	inquiryData.revision[0] = 0;
	printf("Device name: '%s'\n\n", inquiryData.vendor);

	printf("Removable media support: %s\n",
		inquiryData.RMB ? "Yes" : "No");

	if(!inquiryData.RMB) 	{
		printf("\nRemovable media support is required\n");
	};

	return inquiryData.RMB;
}


LONG
cookieptr()
{
	return *((LONG *)0x5a0);
}


bool
getCookie(LONG cookie, ULONG *p_value)
{
	LONG *cookiejar = (LONG *)Supexec(cookieptr);

	if(!cookiejar) return false;

	do {
		if(cookiejar[0] == cookie) {
			if (p_value) *p_value = (ULONG)cookiejar[1];
			return true;
		}
		else
			cookiejar = &(cookiejar[2]);
	} while(cookiejar[-2]);

	return false;
}
