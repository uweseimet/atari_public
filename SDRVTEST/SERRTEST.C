/***************************************/
/* SCSI Driver Error Status Test 1.01ž */
/*                                     */
/* (C) 2021-2026 Uwe Seimet            */
/***************************************/


#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <std.h>
#include <tos.h>
#include <scsi3.h>
#include <scsidrv/scsidefs.h>


int sortBuses(const void *, const void *);
bool getCookie(LONG, ULONG *);


tpScsiCall scsiCall;
tSCSICmd cmd1, cmd2;
SENSE_DATA senseData;


#pragma warn -par
void
main(WORD argc, const char *argv[])
{
	UWORD bus;
	UWORD device;
	tBusInfo busInfo;
	tBusInfo busInfos[32];
	DLONG scsiId;
	ULONG maxLen;
	UWORD busCount = 0;
	LONG oldstack = 0;
	LONG result;
	int i;

	getCookie('SCSI', (ULONG *)&scsiCall);
	if(!scsiCall) {
		printf("SCSI Driver not found\n");

		goto error;
	}

	printf("SCSI Driver Error Status Test V1.01ž\n");
	printf("½ 2021-2026 Uwe Seimet\n\n");

	printf("Found SCSI Driver version %d.%02d\n\n", scsiCall->Version >> 8,
		scsiCall->Version & 0xff);

	cmd1.Flags = 0;
	cmd2.Flags = 0;
	cmd1.SenseBuffer = (BYTE *)&senseData;
	cmd2.SenseBuffer = (BYTE *)&senseData;
	cmd1.Timeout = 2000;
	cmd2.Timeout = 2000;

	if(!Super((void *)1L)) oldstack = Super(0L);

	result = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while(!result && busCount < 32) {
		memcpy(&busInfos[busCount++], &busInfo, sizeof(tBusInfo));

		result = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}

	qsort(busInfos, busCount, sizeof(tBusInfo), sortBuses);

	for(i = 0; i < busCount; i++) {
		printf("Bus ID: %d, Bus name: '%s'\n",
			busInfos[i].BusNo, busInfos[i].BusName);
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

	result = scsiCall->Error(cmd1.Handle, cErrRead, cErrMediach);
	printf("\nError status 1 for handle 1: %ld\n", result);

	result = scsiCall->Error(cmd1.Handle, cErrRead, cErrMediach);
	printf("\nError status 2 for handle 1: %ld\n", result);

	result = scsiCall->Error(cmd2.Handle, cErrRead, cErrMediach);
	printf("\nError status 1 for handle 2: %ld\n", result);

	result = scsiCall->Error(cmd2.Handle, cErrRead, cErrMediach);
	printf("\nError status 2 for handle 2: %ld\n", result);

	scsiCall->Close(cmd1.Handle);
	scsiCall->Close(cmd2.Handle);

	if(oldstack) Super((void *)oldstack);

	printf("\nStatus: %ld\n", result);

	Cconin();

	return;

error:

	scsiCall->Close(cmd1.Handle);
	scsiCall->Close(cmd2.Handle);

	if(oldstack) Super((void *)oldstack);

	printf("\nTest failed\n");

	Cconin();
}
#pragma warn .par


int
sortBuses(const void *b1, const void *b2)
{
	const tBusInfo *i1 = b1;
	const tBusInfo *i2 = b2;

	return i1->BusNo - i2->BusNo;
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
