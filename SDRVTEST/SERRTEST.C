/**************************************/
/* SCSI Driver Error Status Test 1.03 */
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
int
main(WORD argc, const char *argv[])
{
	UWORD bus;
	DLONG scsiId = { 0, 0 };
	ULONG maxLen;
	LONG oldstack = 0;
	LONG result1, result2, result3;

	getCookie('SCSI', (ULONG *)&scsiCall);
	if(!scsiCall) {
		printf("SCSI Driver not found\n");

		Cconin();

		return 0;
	}

	printf("SCSI Driver Error Status Test V1.03\n");
	printf("½ 2021-2026 Uwe Seimet\n\n");

	printf("Found SCSI Driver version %d.%02d\n\n", scsiCall->Version >> 8,
		scsiCall->Version & 0xff);

	cmd1.Flags = 0;
	cmd2.Flags = 0;
	cmd1.SenseBuffer = (BYTE *)&senseData;
	cmd2.SenseBuffer = (BYTE *)&senseData;
	cmd1.Timeout = 2000;
	cmd2.Timeout = 2000;
	cmd1.Handle = NULL;
	cmd2.Handle = NULL;

	if(!Super((void *)1L)) {
		oldstack = Super(0L);
	}

	cmd1.Handle = GetHandle(scsiCall, &bus, &scsiId.lo, NULL);
	if(!cmd1.Handle) {
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
	printf("\nError status for handle 1 (expected: 0): %ld\n", result1);

	result2 = scsiCall->Error(cmd2.Handle, cErrRead, cErrMediach);
	printf("\nError status 1 for handle 2 (expected: 1): %ld\n", result2);

	result3 = scsiCall->Error(cmd2.Handle, cErrRead, cErrMediach);
	printf("\nError status 2 for handle 2 (expected: 0): %ld\n", result3);

	scsiCall->Close(cmd1.Handle);
	scsiCall->Close(cmd2.Handle);

	if(oldstack) {
		Super((void *)oldstack);
	}

	/* The error status must be reflected by all handles except the
	   handle Error() was called for. After getting the status for
	   a handle, the status must be cleard. */
	if(result1 || !result2 || result3) {
		printf("\nTest failed\n");
	}
	else {
		printf("\nTest succceded\n");
	}

	Cconin();

	return 0;

error:

	if(cmd1.Handle) {
		scsiCall->Close(cmd1.Handle);
	}

	if(cmd2.Handle) {
		scsiCall->Close(cmd2.Handle);
	}

	if(oldstack) {
		Super((void *)oldstack);
	}

	printf("\nTest failed\n");

	Cconin();

	return 0;
}
#pragma warn .par
