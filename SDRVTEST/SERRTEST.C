/**************************************/
/* SCSI Driver Error Status Test 1.01 */
/*                                    */
/* (C) 2021-2026 Uwe Seimet           */
/**************************************/


#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <tos.h>
#include <scsi3.h>
#include <scsidrv/scsidefs.h>
#include "std.h"
#include "util.h"


tpScsiCall scsiCall;
tSCSICmd cmd1, cmd2;
SENSE_DATA senseData;


#pragma warn -par
void
main(WORD argc, const char *argv[])
{
	UWORD bus;
	UWORD device;
	tBusInfo busInfos[32];
	DLONG scsiId;
	ULONG maxLen;
	UWORD busCount;
	UWORD busId;
	LONG oldstack = 0;
	LONG result1, result2, result3, result4;

	getCookie('SCSI', (ULONG *)&scsiCall);
	if(!scsiCall) {
		printf("SCSI Driver not found\n");

		Cconin();

		return;
	}

	printf("SCSI Driver Error Status Test V1.01\n");
	printf("½ 2021-2026 Uwe Seimet\n\n");

	printf("Found SCSI Driver version %d.%02d\n\n", scsiCall->Version >> 8,
		scsiCall->Version & 0xff);

	cmd1.Flags = 0;
	cmd2.Flags = 0;
	cmd1.SenseBuffer = (BYTE *)&senseData;
	cmd2.SenseBuffer = (BYTE *)&senseData;
	cmd1.Timeout = 2000;
	cmd2.Timeout = 2000;

	if(!Super((void *)1L)) {
		oldstack = Super(0L);
	}

	busCount = ScanBuses(busInfos, scsiCall);
	for(busId = 0; busId < busCount; busId++) {
		printf("Bus ID: %d, Bus name: '%s'\n", busInfos[busId].BusNo,
		busInfos[busId].BusName);
	}

	printf("\nEnter bus ID, device ID: ");
	scanf("%d,%d", &bus, &device);
	printf("\n");

	scsiId.hi = 0;
	scsiId.lo = device;

	cmd1.Handle = (tHandle)scsiCall->Open(bus, &scsiId, &maxLen);
	if(((LONG)cmd1.Handle >> 24) < 0) {
		printf("Unknown IDs or device not found\n");

		goto error;
	}

	cmd2.Handle = (tHandle)scsiCall->Open(bus, &scsiId, &maxLen);
	if(((LONG)cmd2.Handle >> 24) < 0) {
		printf("Unknown IDs or device not found\n");

		goto error;
	}

	printf("\nSetting error status for handle 1\n");
	scsiCall->Error(cmd1.Handle, cErrWrite, cErrMediach);

	result1 = scsiCall->Error(cmd1.Handle, cErrRead, cErrMediach);
	printf("\nError status 1 for handle 1 must be 0: %ld\n", result1);

	result2 = scsiCall->Error(cmd1.Handle, cErrRead, cErrMediach);
	printf("\nError status 2 for handle 1 must be 0: %ld\n", result2);

	result3 = scsiCall->Error(cmd2.Handle, cErrRead, cErrMediach);
	printf("\nError status 1 for handle 2 must be 1: %ld\n", result3);

	result4 = scsiCall->Error(cmd2.Handle, cErrRead, cErrMediach);
	printf("\nError status 2 for handle 2 must be 0: %ld\n", result4);

	scsiCall->Close(cmd1.Handle);
	scsiCall->Close(cmd2.Handle);

	if(oldstack) {
		Super((void *)oldstack);
	}

	/* The errpr status must be reflected by all handles except the
	   handle Error() was called for. After getting the status for
	   a handle, the status must be cleard. */
	if(result1 || result2 || !result3 || result4) {
		printf("\nTest failed\n");
	}
	else {
		printf("\nTest succceded\n");
	}

	Cconin();

	return;

error:

	scsiCall->Close(cmd1.Handle);
	scsiCall->Close(cmd2.Handle);

	if(oldstack) {
		Super((void *)oldstack);
	}

	printf("\nTest failed\n");

	Cconin();
}
#pragma warn .par
